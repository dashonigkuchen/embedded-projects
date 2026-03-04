// ==========================================================================
//  Car Seat Control – main
//
//  Runs on ESP32-C3.  Controls seat heating + cooling via PWM and
//  communicates with a BQ25792 charger IC over I2C.
//
//  Everything starts OFF.  The watchdog timer resets the MCU if the
//  main loop stalls for more than WDT_TIMEOUT_S seconds.
// ==========================================================================

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "heater.h"
#include "fan.h"
#include "charger.h"

// ─── ISR for charger interrupt ───────────────────────────────────────
static void IRAM_ATTR onChargerInterrupt() {
    charger::interruptPending = true;
}

// ─── timing ──────────────────────────────────────────────────────────
static unsigned long s_lastLoop  = 0;
static unsigned long s_lastPrint = 0;

// ─── periodic serial status ──────────────────────────────────────────
static void printStatus(unsigned long now) {
    if (now - s_lastPrint < STATUS_PRINT_MS) return;
    s_lastPrint = now;

    // Heater
    Serial.printf("[HTR] duty=%3u  temp=%.1f°C  %s\n",
                  heater::duty(),
                  heater::temperature(),
                  heater::isSafetyActive()
                      ? (heater::isSensorFault() ? "SENSOR-FAULT" : "OVERTEMP")
                      : "OK");

    // Fan
    Serial.printf("[FAN] duty=%3u\n", fan::duty());

    // Charger
    const auto& cs = charger::status();
    if (cs.present) {
        Serial.printf("[CHG] state=%u  vbus=%umV  vbat=%umV  ichg=%dmA  %s%s\n",
                      (unsigned)cs.chargeState,
                      cs.vbus_mV, cs.vbat_mV, cs.ichg_mA,
                      cs.vbusPresent ? "VBUS " : "",
                      cs.fault       ? "FAULT" : "");
    } else {
        Serial.println("[CHG] offline");
    }
}

// ─── setup ───────────────────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);                            // let USB-CDC enumerate
    Serial.println("\n=== Car Seat Control ===");

    // ADC global settings
    analogReadResolution(ADC_BITS);

    // Subsystem init
    heater::init();
    fan::init();

    bool chargerOk = charger::init();

    // Attach charger interrupt (falling edge – BQ_INT is active-low)
    if (chargerOk) {
        attachInterrupt(digitalPinToInterrupt(PIN_BQ_INT),
                        onChargerInterrupt, FALLING);
    }

    // Watchdog – resets MCU if loop stalls
    esp_task_wdt_init(WDT_TIMEOUT_S, true);  // timeout (s), panic on expire
    esp_task_wdt_add(NULL);                   // subscribe loop task

    Serial.println("Init complete – entering main loop");
}

// ─── loop ────────────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    // Fixed-rate control loop
    if (now - s_lastLoop < LOOP_INTERVAL_MS) return;
    s_lastLoop = now;

    // Feed the watchdog
    esp_task_wdt_reset();

    // Update subsystems
    heater::update(now);
    fan::update();
    charger::update(now);

    // Debug output
    printStatus(now);
}