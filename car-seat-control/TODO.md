# Car Seat Control – Detailed TODO

This TODO is based on the current repository state (`README.md`, `platformio.ini`, `src/*`, `include/*`, and project assets) and is ordered so the project can be completed step by step.

## 1) Current state snapshot

### Implemented in firmware
- `src/main.cpp`: main loop, watchdog, periodic status printing, charger interrupt hook.
- `src/heater.cpp`: heater PWM + thermistor safety (temperature conversion + overtemperature hysteresis).
- `src/fan.cpp`: fan PWM controlled by potentiometer ADC.
- `src/charger.cpp`: BQ25792 I2C driver with init/config, status polling, interrupt-triggered reads.
- `include/config.h`: centralized pin and tuning constants.

### Missing or incomplete
- No WiFi implementation (only mentioned as optional in README).
- No test implementation (`test/README` exists, no real tests).
- No persistent configuration mechanism (all values compile-time constants).
- README still contains stale information (claims `src/main.cpp` is template, but it is now implemented).

---

## 2) Issues and documentation gaps

### Critical functional blockers
- [ ] **Fix ADC pin mapping mismatch**
  - In `include/config.h`, `PIN_HEATER_POTI=21` and `PIN_FAN_POTI=18` are not ADC-capable on ESP32-C3.
  - The file now contains compile-time `#error` checks for this, so firmware cannot build until routing/pin mapping is fixed.

### Hardware / schematic verification
- [ ] **Verify schematic net-to-pin mapping against actual ESP32-C3 symbol**
  - README already warns about possible `ESP_EN` / `POWER_EN` labeling inconsistencies.
  - Confirm symbol correctness before fabrication/assembly decisions.

### Firmware quality gaps
- [ ] Add filtering/smoothing for potentiometer and thermistor ADC readings (noise robustness).
- [ ] Define charger error recovery policy (currently device is marked offline after repeated I2C errors).
- [ ] Decide and implement when `charger::enableCharging(false)` should be called (currently available but unused in control policy).

### Documentation gaps (not self-explanatory today)
- [ ] Update README “Status/Roadmap” to reflect current code reality.
- [ ] Add explicit calibration/tuning procedure (thermistor constants, temp limits, PWM frequencies, charger limits).
- [ ] Add a clear “definition of done” for MVP vs optional features (especially WiFi).
- [ ] Finalize project license section in `car-seat-control/README.md`.

---

## 3) Step-by-step completion plan

## Phase 0 — Align hardware and firmware assumptions (must finish first)
- [ ] **0.1 Confirm final board pinout**
  - Cross-check KiCad net labels with intended ESP32-C3 GPIO mapping.
  - Deliverable: verified pin map table.
- [ ] **0.2 Resolve ADC inputs for potentiometers**
  - Move poti signals to ADC-capable GPIOs (0–5) or add external ADC and adapt firmware.
  - Deliverable: schematic + `config.h` aligned and buildable.
- [ ] **0.3 Re-validate charger control pins (`BQ_*`) and interrupt line**
  - Deliverable: confirmed hardware wiring assumptions for `charger.cpp`.

## Phase 1 — Make firmware baseline robust
- [ ] **1.1 Build and flash baseline firmware after pin fix**
  - Verify boot, serial output, heater/fan PWM activity, charger detection.
- [ ] **1.2 Add ADC signal conditioning**
  - Introduce lightweight filtering (moving average or EMA) for thermistor and potentiometers.
- [ ] **1.3 Add deterministic safety fallback behavior**
  - Define and enforce safe outputs on sensor faults / charger failure conditions.
- [ ] **1.4 Harden charger comms behavior**
  - Improve retry/backoff and recovery strategy instead of permanent offline state in normal runtime.

## Phase 2 — Add validation (tests + hardware checks)
- [ ] **2.1 Add PlatformIO unit tests for pure logic**
  - Thermistor conversion edge cases.
  - Duty-cycle mapping behavior.
  - Charger register parsing logic.
- [ ] **2.2 Add bring-up checklist for hardware verification**
  - ADC channel checks, PWM frequency checks, I2C charger read/write checks.
- [ ] **2.3 Run and document repeatable validation commands**
  - `pio run`, `pio test`, serial monitor sanity scenarios.

## Phase 3 — Improve configurability and usability
- [ ] **3.1 Introduce runtime/persistent configuration**
  - Store safety and tuning parameters (NVS or equivalent), with safe defaults.
- [ ] **3.2 Document full tuning guide**
  - How to calibrate thermistor, set safety limits, and tune charger settings.
- [ ] **3.3 Decide optional WiFi scope**
  - Explicitly choose: no WiFi for MVP, or minimal WiFi telemetry/control implementation.

## Phase 4 — Final project completion criteria
- [ ] **4.1 Firmware completeness**
  - Heater and fan controls stable with validated safety behavior.
  - Charger integration reliable under expected fault cases.
- [ ] **4.2 Test completeness**
  - Core safety/control logic covered by automated tests.
- [ ] **4.3 Documentation completeness**
  - README accurate, setup/build steps verified, calibration + hardware assumptions documented.
- [ ] **4.4 Release readiness**
  - Clear MVP definition met and recorded (what is in/out of scope).

---

## 4) Suggested execution order (quick reference)
1. Pinout and ADC routing fixes.
2. Build/flash and baseline hardware sanity checks.
3. Safety and robustness improvements (ADC filtering, charger recovery policy).
4. Add automated tests for core logic.
5. Documentation refresh and explicit MVP definition.
6. Optional WiFi decision/implementation.

