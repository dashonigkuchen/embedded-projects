// ==========================================================================
//  Heater control implementation
// ==========================================================================

#include "heater.h"
#include "config.h"
#include <Arduino.h>
#include <math.h>

static constexpr uint8_t LEDC_CHANNEL_HEATER = 0;

namespace heater {

// ── internal state ───────────────────────────────────────────────────
static float         s_temperature   = NAN;
static bool          s_overtemp      = false;
static bool          s_sensorFault   = false;
static uint16_t      s_duty          = 0;
static unsigned long s_lastThermRead = 0;

// ── helpers ──────────────────────────────────────────────────────────

/// Convert raw ADC value to temperature (°C) using the B-parameter equation.
/// Returns NAN if the reading indicates an open or shorted sensor.
static float adcToTemperature(uint16_t raw) {
    if (raw == 0 || raw >= ADC_MAX_VALUE) {
        return NAN;   // open circuit or short
    }

    // Voltage divider: 3.3 V ──[R_PULLUP]── ADC ──[NTC]── GND
    // R_ntc = R_pullup × raw / (ADC_MAX − raw)
    float r_ntc = NTC_R_PULLUP * (float)raw / (float)(ADC_MAX_VALUE - raw);

    // Simplified Steinhart–Hart (B-parameter form):
    //   1/T = 1/T25 + (1/B) × ln(R/R25)
    float steinhart = (1.0f / NTC_T25_K)
                    + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R25);
    return (1.0f / steinhart) - 273.15f;
}

static uint16_t readPoti() {
    return analogRead(PIN_HEATER_POTI);
}

// ── public API ───────────────────────────────────────────────────────

void init() {
    // PWM – uses the ESP32-C3 LEDC peripheral
    ledcSetup(LEDC_CHANNEL_HEATER, HEATER_PWM_FREQ, HEATER_PWM_RES);
    ledcAttachPin(PIN_HEATER_PWM, LEDC_CHANNEL_HEATER);
    ledcWrite(LEDC_CHANNEL_HEATER, 0);     // start OFF

    pinMode(PIN_HEATER_POTI,  INPUT);
    pinMode(PIN_HEATER_THERM, INPUT);

    // Make sure ADC range covers 0–3.3 V on the thermistor pin
    analogSetPinAttenuation(PIN_HEATER_THERM, ADC_11db);
}

void update(unsigned long now_ms) {
    // ── thermistor sampling (at THERM_READ_INTERVAL_MS) ─────────────
    if (now_ms - s_lastThermRead >= THERM_READ_INTERVAL_MS) {
        s_lastThermRead = now_ms;

        float t = adcToTemperature(analogRead(PIN_HEATER_THERM));

        if (isnan(t) || t < TEMP_SENSOR_MIN || t > TEMP_SENSOR_MAX) {
            s_sensorFault = true;
        } else {
            s_sensorFault = false;
            s_temperature = t;
        }

        // Overtemp with hysteresis (only evaluated on valid readings)
        if (!s_sensorFault) {
            if (s_temperature >= HEATER_TEMP_LIMIT) {
                s_overtemp = true;
            } else if (s_temperature <= HEATER_TEMP_LIMIT - HEATER_TEMP_HYSTERESIS) {
                s_overtemp = false;
            }
        }
    }

    // ── determine duty cycle ─────────────────────────────────────────
    if (s_sensorFault || s_overtemp) {
        s_duty = 0;                        // safety: heater OFF
    } else {
        uint16_t poti = readPoti();
        s_duty = (uint16_t)map(poti, 0, (long)ADC_MAX_VALUE, 0, (long)HEATER_PWM_MAX_DUTY);
    }

    ledcWrite(LEDC_CHANNEL_HEATER, s_duty);
}

float   temperature()    { return s_temperature; }
bool    isSafetyActive() { return s_overtemp || s_sensorFault; }
bool    isSensorFault()  { return s_sensorFault; }
uint16_t duty()          { return s_duty; }

}  // namespace heater
