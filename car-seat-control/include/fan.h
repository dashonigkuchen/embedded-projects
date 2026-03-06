#pragma once

// ==========================================================================
//  Fan / cooling control – PWM output with potentiometer input
// ==========================================================================

#include <cstdint>

namespace fan {

/// Call once in setup().
void init();

/// Call every loop iteration.
void update();

/// Current PWM duty cycle (0 – FAN_PWM_MAX_DUTY).
uint16_t duty();

}  // namespace fan
