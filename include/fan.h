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

/// Current PWM duty cycle (0–255).
uint8_t duty();

}  // namespace fan
