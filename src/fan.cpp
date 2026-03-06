// ==========================================================================
//  Fan / cooling control implementation
// ==========================================================================

#include "fan.h"
#include "config.h"
#include <Arduino.h>

static constexpr uint8_t LEDC_CHANNEL_FAN = 1;

namespace fan {

static uint16_t s_duty = 0;

void init() {
    ledcSetup(LEDC_CHANNEL_FAN, FAN_PWM_FREQ, FAN_PWM_RES);
    ledcAttachPin(PIN_FAN_PWM, LEDC_CHANNEL_FAN);
    ledcWrite(LEDC_CHANNEL_FAN, 0);        // start OFF

    pinMode(PIN_FAN_POTI, INPUT);
}

void update() {
    uint16_t poti = analogRead(PIN_FAN_POTI);
    s_duty = (uint16_t)map(poti, 0, (long)ADC_MAX_VALUE, 0, (long)FAN_PWM_MAX_DUTY);
    ledcWrite(LEDC_CHANNEL_FAN, s_duty);
}

uint16_t duty() { return s_duty; }

}  // namespace fan
