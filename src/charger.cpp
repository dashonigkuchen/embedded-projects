// ==========================================================================
//  BQ25792 charger driver implementation
// ==========================================================================

#include "charger.h"
#include "config.h"
#include <Arduino.h>
#include <Wire.h>

// ─── BQ25792 register map (subset) ──────────────────────────────────
namespace reg {
    constexpr uint8_t VSYS_MIN           = 0x00;
    constexpr uint8_t CHARGE_VOLT_LIM_H  = 0x01;
    constexpr uint8_t CHARGE_VOLT_LIM_L  = 0x02;
    constexpr uint8_t CHARGE_CURR_LIM_H  = 0x03;
    constexpr uint8_t CHARGE_CURR_LIM_L  = 0x04;
    constexpr uint8_t INPUT_VOLT_LIM     = 0x05;
    constexpr uint8_t INPUT_CURR_LIM_H   = 0x06;
    constexpr uint8_t INPUT_CURR_LIM_L   = 0x07;

    constexpr uint8_t CHG_CTRL0          = 0x0F;  // bit 5 = EN_CHG
    constexpr uint8_t CHG_CTRL1          = 0x10;
    constexpr uint8_t CHG_CTRL2          = 0x11;

    constexpr uint8_t CHG_STATUS0        = 0x1B;
    constexpr uint8_t CHG_STATUS1        = 0x1C;
    constexpr uint8_t CHG_STATUS2        = 0x1D;
    constexpr uint8_t CHG_STATUS3        = 0x1E;
    constexpr uint8_t CHG_STATUS4        = 0x1F;
    constexpr uint8_t FAULT_STATUS0      = 0x20;
    constexpr uint8_t FAULT_STATUS1      = 0x21;

    constexpr uint8_t ADC_CTRL           = 0x26;
    constexpr uint8_t VBUS_ADC_H         = 0x27;
    constexpr uint8_t VBUS_ADC_L         = 0x28;
    constexpr uint8_t VBAT_ADC_H         = 0x29;
    constexpr uint8_t VBAT_ADC_L         = 0x2A;
    constexpr uint8_t IBUS_ADC_H         = 0x2B;
    constexpr uint8_t IBUS_ADC_L         = 0x2C;
    constexpr uint8_t IBAT_ADC_H         = 0x2D;
    constexpr uint8_t IBAT_ADC_L         = 0x2E;
    constexpr uint8_t VSYS_ADC_H         = 0x2F;
    constexpr uint8_t VSYS_ADC_L         = 0x30;

    constexpr uint8_t PART_INFO          = 0x38;
}

namespace charger {

// ── state ────────────────────────────────────────────────────────────
volatile bool interruptPending = false;

static Status        s_status       = {};
static unsigned long s_lastPoll     = 0;
static uint8_t       s_comErrors    = 0;
static constexpr uint8_t MAX_COM_ERRORS = 5;

// ── low-level I2C helpers ────────────────────────────────────────────

static bool writeReg(uint8_t regAddr, uint8_t value) {
    Wire.beginTransmission(BQ25792_I2C_ADDR);
    Wire.write(regAddr);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static bool readReg(uint8_t regAddr, uint8_t& out) {
    Wire.beginTransmission(BQ25792_I2C_ADDR);
    Wire.write(regAddr);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(BQ25792_I2C_ADDR, (uint8_t)1) != 1) return false;
    out = Wire.read();
    return true;
}

static bool readReg16(uint8_t regAddrH, uint16_t& out) {
    Wire.beginTransmission(BQ25792_I2C_ADDR);
    Wire.write(regAddrH);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(BQ25792_I2C_ADDR, (uint8_t)2) != 2) return false;
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    out = ((uint16_t)hi << 8) | lo;
    return true;
}

/// Read-modify-write a single bit in a register.
static bool setRegBit(uint8_t regAddr, uint8_t bit, bool value) {
    uint8_t val = 0;
    if (!readReg(regAddr, val)) return false;
    if (value) val |=  (1 << bit);
    else       val &= ~(1 << bit);
    return writeReg(regAddr, val);
}

// ── charger configuration ────────────────────────────────────────────

static bool configure() {
    bool ok = true;

    // Minimum system voltage: (value × 250 mV) + 2500 mV
    // For 9000 mV: (9000 − 2500) / 250 = 26
    uint8_t vsysMin = (uint8_t)((BQ_MIN_SYS_VOLTAGE_MV - 2500) / 250);
    ok &= writeReg(reg::VSYS_MIN, vsysMin);

    // Charge voltage limit: 10 mV per bit, 16-bit across two registers
    uint16_t vregRaw = BQ_CHARGE_VOLTAGE_MV / 10;
    ok &= writeReg(reg::CHARGE_VOLT_LIM_H, (uint8_t)(vregRaw >> 8));
    ok &= writeReg(reg::CHARGE_VOLT_LIM_L, (uint8_t)(vregRaw & 0xFF));

    // Charge current limit: 10 mA per bit
    uint16_t ichgRaw = BQ_CHARGE_CURRENT_MA / 10;
    ok &= writeReg(reg::CHARGE_CURR_LIM_H, (uint8_t)(ichgRaw >> 8));
    ok &= writeReg(reg::CHARGE_CURR_LIM_L, (uint8_t)(ichgRaw & 0xFF));

    // Input voltage limit: 100 mV per bit, offset 3600 mV
    ok &= writeReg(reg::INPUT_VOLT_LIM, BQ_INPUT_VOLTAGE_100MV);

    // Input current limit: 10 mA per bit
    uint16_t iinRaw = BQ_INPUT_CURRENT_MA / 10;
    ok &= writeReg(reg::INPUT_CURR_LIM_H, (uint8_t)(iinRaw >> 8));
    ok &= writeReg(reg::INPUT_CURR_LIM_L, (uint8_t)(iinRaw & 0xFF));

    // Enable continuous ADC conversion (bit 7 of ADC_CTRL)
    ok &= setRegBit(reg::ADC_CTRL, 7, true);

    // Enable charging via I2C (EN_CHG = bit 5 of CHG_CTRL0)
    ok &= setRegBit(reg::CHG_CTRL0, 5, true);

    return ok;
}

// ── status read ──────────────────────────────────────────────────────

static void readStatus() {
    uint8_t st0 = 0, st1 = 0, fault0 = 0, fault1 = 0;
    bool ok = true;

    ok &= readReg(reg::CHG_STATUS0, st0);
    ok &= readReg(reg::CHG_STATUS1, st1);
    ok &= readReg(reg::FAULT_STATUS0, fault0);
    ok &= readReg(reg::FAULT_STATUS1, fault1);

    if (!ok) {
        s_comErrors++;
        return;
    }
    s_comErrors = 0;

    // CHG_STATUS0 bits [7:5] = CHG_STAT
    s_status.chargeState = (ChargeState)((st0 >> 5) & 0x07);

    // CHG_STATUS1: bit 7 = VBUS_PRESENT, bit 3 = PG_STAT
    s_status.vbusPresent = (st1 >> 7) & 1;
    s_status.powerGood   = (st1 >> 3) & 1;

    s_status.fault = (fault0 != 0) || (fault1 != 0);

    // Read ADC values (best-effort; non-critical if they fail)
    uint16_t raw = 0;
    if (readReg16(reg::VBUS_ADC_H, raw)) s_status.vbus_mV = raw;
    if (readReg16(reg::VBAT_ADC_H, raw)) s_status.vbat_mV = raw;
    if (readReg16(reg::IBAT_ADC_H, raw)) s_status.ichg_mA = (int16_t)raw;
}

// ── public API ───────────────────────────────────────────────────────

bool init() {
    // CE pin: LOW = charging enabled (default state)
    pinMode(PIN_BQ_CE, OUTPUT);
    digitalWrite(PIN_BQ_CE, LOW);

    // Interrupt pin (active-low, open-drain from charger)
    pinMode(PIN_BQ_INT, INPUT_PULLUP);

    // I2C bus
    Wire.begin(PIN_BQ_SDA, PIN_BQ_SCL);
    Wire.setClock(400000);                 // 400 kHz

    // Probe: read PART_INFO register
    uint8_t partInfo = 0;
    if (!readReg(reg::PART_INFO, partInfo)) {
        Serial.println("[CHG] BQ25792 not found on I2C bus");
        s_status.present = false;
        return false;
    }
    Serial.printf("[CHG] BQ25792 detected  (PART_INFO = 0x%02X)\n", partInfo);
    s_status.present = true;

    if (!configure()) {
        Serial.println("[CHG] WARNING: configuration write incomplete");
    }

    // Initial status read
    readStatus();
    return true;
}

void update(unsigned long now_ms) {
    if (!s_status.present) return;

    // Too many consecutive I2C errors → mark charger as absent
    if (s_comErrors >= MAX_COM_ERRORS) {
        Serial.println("[CHG] too many I2C errors – charger marked offline");
        s_status.present = false;
        return;
    }

    bool shouldRead = false;

    // Make check-and-clear of interruptPending atomic with respect to ISRs
    noInterrupts();
    if ((now_ms - s_lastPoll >= CHARGER_POLL_MS) || interruptPending) {
        shouldRead = true;
        interruptPending = false;
        s_lastPoll = now_ms;
    }
    interrupts();

    if (shouldRead) {
        readStatus();
    }
}

void enableCharging(bool enable) {
    // Hardware pin: CE LOW = enabled, HIGH = disabled
    digitalWrite(PIN_BQ_CE, enable ? LOW : HIGH);

    // Software register: EN_CHG bit 5 in CHG_CTRL0
    if (s_status.present) {
        setRegBit(reg::CHG_CTRL0, 5, enable);
    }
}

const Status& status() {
    return s_status;
}

}  // namespace charger
