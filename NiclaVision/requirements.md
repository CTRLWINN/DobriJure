# Nicla Vision Dependencies

This code runs on **MicroPython** (OpenMV firmware) on the Nicla Vision board.
It does NOT use `pip` packages.

## Required Firmware
- OpenMV Firmware for Arduino Nicla Vision
- Libraries used (Built-in):
    - `sensor`
    - `image`
    - `time`
    - `pyb`
    - `network`
    - `usocket`
    - `sys`

## Installation
1. Install **OpenMV IDE**.
2. Connect Nicla Vision via USB.
3. Open `nicla_vision.py`.
4. Save as `main.py` on the device storage to run on boot.
