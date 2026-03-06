# ESP32 Relay Module

This project targets a 16-channel relay module with an integrated ESP32-WROOM. The main goal is to connect the board to Home Assistant through ESPHome and use Home Assistant as the primary control layer.

## Project idea

The relay board is meant to act as a network-connected output module:

- 16 relay channels
- integrated ESP32-WROOM controller
- relay driving via a `74HC595` shift register
- Home Assistant integration through ESPHome

The intended architecture is simple: the ESP32 provides connectivity and I/O exposure, while the actual automation logic lives in Home Assistant instead of custom firmware.

## Software approach

This repository is currently set up as a PlatformIO project for the Arduino framework, but the long-term intention is to use ESPHome for the final device behavior.

That means:

- little to no custom application logic should be required on the ESP32
- relay control should ultimately be described in ESPHome configuration
- Home Assistant should be the main user interface and automation engine

## Current repository status

At the moment, the firmware source is still in its default starter state:

- [platformio.ini](platformio.ini) targets the `denky32` board on the `espressif32` platform
- [src/main.cpp](src/main.cpp) still contains the default PlatformIO example code

So this repository currently documents the hardware intention and project direction more than a finished firmware implementation.

## Hardware notes

Known hardware details from the project so far:

- MCU/module: ESP32-WROOM
- Outputs: 16 relays
- Output expansion: `74HC595` shift register

Additional details such as relay pin mapping, output enable behavior, default relay state, and power-stage characteristics still need to be documented.

## Suggested next steps

To move this project forward, the next useful additions would be:

1. document the ESP32-to-`74HC595` pin mapping
2. define how the 16 relay outputs map to Home Assistant entities
3. add an ESPHome configuration example
4. document power requirements and safe switching limits
5. replace the default starter code if custom firmware is still needed

## Development environment

- Build system: PlatformIO
- Platform: `espressif32`
- Board: `denky32`
- Framework: Arduino

## Planned usage with Home Assistant

The expected workflow is:

1. flash the ESP32-based relay module
2. expose the relay channels through ESPHome
3. add the device to Home Assistant
4. control relays from dashboards, automations, and scripts

If you want, the next step I can take is to extend this README with a dedicated wiring section, an ESPHome example, or a relay channel mapping table.