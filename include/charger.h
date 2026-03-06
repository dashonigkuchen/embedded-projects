#pragma once

// ==========================================================================
//  BQ25792 charger driver – I2C communication + interrupt handling
// ==========================================================================

#include <cstdint>

namespace charger {

/// Charging state reported by the BQ25792 CHG_STAT field.
enum class ChargeState : uint8_t {
    NOT_CHARGING = 0,
    TRICKLE      = 1,
    PRE_CHARGE   = 2,
    FAST_CC      = 3,
    TAPER_CV     = 4,
    RESERVED_5   = 5,
    TOP_OFF      = 6,
    DONE         = 7,
};

/// Snapshot of charger status – updated by update().
struct Status {
    bool        present;         // true if BQ25792 responded on I2C
    bool        vbusPresent;     // VBUS detected
    bool        powerGood;       // PG_STAT
    ChargeState chargeState;
    uint16_t    vbus_mV;         // VBUS voltage
    uint16_t    vbat_mV;         // battery voltage
    int16_t     ichg_mA;         // charge current (positive = charging)
    bool        fault;           // any fault flag set
};

/// Call once in setup().  Returns true if the BQ25792 was detected.
bool init();

/// Periodic poll – call from the main loop.
void update(unsigned long now_ms);

/// Enable or disable charging via the CE pin + I2C EN_CHG bit.
void enableCharging(bool enable);

/// Read-only access to the latest status snapshot.
const Status& status();

/// Set this flag from the BQ_INT ISR.  update() will clear it and
/// perform a status read on the next call.
extern volatile bool interruptPending;

}  // namespace charger
