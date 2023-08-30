# OPEN-CNC-Shield 2.x ESP32 Software

This project contains the software for the ESP32 on the OPEN-CNC-Shield 2.x and is still under development.

## Howto install

Open this project folder in VS Code with PlatformIO IDE and upload it to the ESP32. A good documentation how to get the IDE up and running can be found here:
[Getting Started with VS Code and PlatformIO IDE](https://randomnerdtutorials.com/vs-code-platformio-ide-esp32-esp8266-arduino/).
The corresponding ESP32 software for the remote control can be found here: [OPEN-CNC-Shield 2.x ESP32 Panel Software](https://github.com/timo1235/-ocs2.x-esp32-panel-software-).

### Configuration

Take a look at the `src/configuration` file for the configuration.

## Features implemented

- control the dac chip(analog outputs like joystck x....)
- control the two PCA9555 chips (I/O port expander)
- establish a Wifi connection to a handwheel and control the I/Os accordingly
- autosquaring process
- WiFi handling with several clients
- Serial communication with the ESP32 panel if the panelmodule RJ45 is connected and the esp32 panel is connected with an RJ45 cable
- Serial connection has higher priority than WiFi connection

## Safety implementations

- Outputs are resettet if WiFi handwheel is not responding for some milliseconds
- Autosquaring only starts after pressing the button for some time
- Ignore requests from multiple ESP32 if they try to control the same Ouput - for example a joystick

# Changelog

## latest changes - not released

## 1.0.1

- removed the configuration variable "DRIVE_FROM_ENDSTOP_AFTER_AUTOSQUARE" since this can now be controlled individually for every motor
- added configuration for every autosquare motor to define the drive back distance after autosquaring
- added `Serial Transfer` library
- added code for serial communication with the ESP32 panel
- added new configuration `RESET_FEEDRATE_AND_ROTATION_SPEED_ON_CONNCTION_LOSS` to control the communication loss reset behavior
  - changed default behaviour of sketch to not reset feedrate and rotation speed on connection loss (this can have a negative effect on the machine if the connection is lost for a short time and a job is running)

## 1.0.0

- inital version
