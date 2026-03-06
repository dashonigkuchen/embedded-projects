#pragma once

// ==========================================================================
//  Car Seat Control – Configuration
//
//  Every tuneable parameter lives here so the rest of the code stays clean.
//  Change pins, limits or timing here – no need to touch logic files.
// ==========================================================================

#include <cstdint>

// ─── Pin assignments (from KiCad schematic) ─────────────────────────
//
// WARNING  On the ESP32-C3 only GPIO 0-5 support analogRead (ADC).
//          HEATER_POTI (GPIO21/TXD) and FAN_POTI (GPIO18) are NOT ADC-capable.
//          If you use analog potentiometers, you must re-route those signals
//          to ADC-capable pins or add an external ADC.  The firmware will
//          still compile, but analogRead() on those pins returns 0.

// Heater
static constexpr uint8_t PIN_HEATER_PWM   =  7;   // PWM to heater MOSFET gate
static constexpr uint8_t PIN_HEATER_POTI  = 21;   // Potentiometer input  (TXD)
static constexpr uint8_t PIN_HEATER_THERM =  3;   // NTC thermistor       (ADC1_CH3)

// Fan / Cooling
static constexpr uint8_t PIN_FAN_PWM      =  6;   // PWM to fan MOSFET gate
static constexpr uint8_t PIN_FAN_POTI     = 18;   // Potentiometer input

// On ESP32-C3 only GPIO 0-5 are ADC-capable. Prevent builds that keep
// potentiometer inputs on non-ADC pins, which would make analogRead()
// always return 0 and disable manual controls.
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32C3)
  #if (PIN_HEATER_POTI > 5)
    #error "PIN_HEATER_POTI must be mapped to an ADC-capable GPIO (0-5) on ESP32-C3 or routed via an external ADC."
  #endif
  #if (PIN_FAN_POTI > 5)
    #error "PIN_FAN_POTI must be mapped to an ADC-capable GPIO (0-5) on ESP32-C3 or routed via an external ADC."
  #endif
#endif
// Charger (BQ25792)
static constexpr uint8_t PIN_BQ_SDA       =  9;   // I2C data
static constexpr uint8_t PIN_BQ_SCL       =  8;   // I2C clock
static constexpr uint8_t PIN_BQ_INT       = 19;   // Interrupt (active-low)
static constexpr uint8_t PIN_BQ_CE        =  0;   // Charge-Enable (active-low)

// ─── ADC ─────────────────────────────────────────────────────────────
static constexpr uint8_t  ADC_BITS      = 12;
static constexpr uint16_t ADC_MAX_VALUE = (1 << ADC_BITS) - 1;          // 4095

// ─── PWM ─────────────────────────────────────────────────────────────
static constexpr uint32_t HEATER_PWM_FREQ     =  1000;  // Hz  – low for resistive load
static constexpr uint8_t  HEATER_PWM_RES      =     8;  // bits
static constexpr uint16_t HEATER_PWM_MAX_DUTY = (1 << HEATER_PWM_RES) - 1;  // 255 for 8-bit

static constexpr uint32_t FAN_PWM_FREQ        = 25000;  // Hz  – 25 kHz for quiet fan
static constexpr uint8_t  FAN_PWM_RES         =     8;  // bits
static constexpr uint16_t FAN_PWM_MAX_DUTY    = (1 << FAN_PWM_RES) - 1;     // 255 for 8-bit

// ─── NTC thermistor ──────────────────────────────────────────────────
//  10 kΩ NTC, B = 3435, voltage-divider with 10 kΩ pull-up to 3.3 V:
//    3.3 V ──[R_PULLUP]── ADC ──[NTC]── GND
static constexpr float NTC_R25       = 10000.0f;   // Ω at 25 °C
static constexpr float NTC_BETA      =  3435.0f;   // B-parameter
static constexpr float NTC_R_PULLUP  = 10000.0f;   // pull-up Ω
static constexpr float NTC_T25_K     =   298.15f;  // 25 °C in Kelvin

// ─── Temperature safety ─────────────────────────────────────────────
static constexpr float HEATER_TEMP_LIMIT      = 45.0f;   // °C  – heater OFF above this
static constexpr float HEATER_TEMP_HYSTERESIS =  5.0f;   // °C  – re-enable below (limit − hyst)
static constexpr float TEMP_SENSOR_MIN        = -10.0f;  // °C  – below → sensor fault
static constexpr float TEMP_SENSOR_MAX        =  80.0f;  // °C  – above → sensor fault

// ─── BQ25792 charger ────────────────────────────────────────────────
static constexpr uint8_t  BQ25792_I2C_ADDR      = 0x6B;
static constexpr uint16_t BQ_CHARGE_VOLTAGE_MV   = 12600;  // 3S × 4.2 V
static constexpr uint16_t BQ_CHARGE_CURRENT_MA   =  2000;  // mA
static constexpr uint16_t BQ_MIN_SYS_VOLTAGE_MV  =  9000;  // 3S × 3.0 V
static constexpr uint16_t BQ_INPUT_CURRENT_MA    =  3000;  // mA  (adapter limit)
static constexpr uint8_t  BQ_INPUT_VOLTAGE_100MV =    42;  // × 100 mV → 4.2 V min VINDPM

// ─── Timing ─────────────────────────────────────────────────────────
static constexpr unsigned long LOOP_INTERVAL_MS       =   50;  // 20 Hz control loop
static constexpr unsigned long THERM_READ_INTERVAL_MS =  500;  // thermistor sample rate
static constexpr unsigned long CHARGER_POLL_MS        = 1000;  // charger status poll
static constexpr unsigned long STATUS_PRINT_MS        = 2000;  // serial debug log
static constexpr uint32_t     WDT_TIMEOUT_S          =    5;  // watchdog (seconds)

// ─── Serial ─────────────────────────────────────────────────────────
static constexpr unsigned long SERIAL_BAUD = 115200;
