# EEG Test Project

This project captures EEG signals using a microcontroller and classifies brain states in real time. A separate Python script reads the serial output and visualizes the signals live.

## Folders

- **EEG First Test MCU/**  
  Contains the firmware written for an Arduino Uno using PlatformIO. It filters EEG signals, computes band powers (alpha, beta, theta, delta), and classifies states such as Focused, Relaxed, or Moving.

- **EGG Live Plotter Python/**  
  Contains the Python script for live plotting of EEG signals over serial.

- **Documentation/**  
  Contains this README and related documentation files.

## Requirements

### Microcontroller
- Arduino Uno (or compatible)
- PlatformIO installed (VSCode extension or CLI)
- EEG sensor such as BioAmp EXG Pill
- Serial baud rate: `115200`

### Python Plotting
- Python 3.8 or newer
- Required libraries:
  - `pyserial`
  - `matplotlib`
  - `numpy`

Install them with:

```bash
pip install pyserial matplotlib numpy
