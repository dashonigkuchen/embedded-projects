#pragma once

// ==========================================================================
//  Heater control – PWM output with potentiometer input + NTC safety
// ==========================================================================

#include <cstdint>

namespace heater {

/// Call once in setup().
void init();

/// Call every loop iteration.  Uses millis-based internal timing for
/// thermistor sampling so it is safe to call at any rate.
void update(unsigned long now_ms);

/// Last valid temperature reading (°C).  Returns NAN before first read.
float temperature();

/// True when heater is shut down for safety (overtemp or sensor fault).
bool  isSafetyActive();

/// True when the NTC reading is considered invalid.
bool  isSensorFault();

/// Current PWM duty cycle (0 – HEATER_PWM_MAX_DUTY).
uint16_t duty();

}  // namespace heater
