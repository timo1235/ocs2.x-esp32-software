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

## Safety implementations
- Outputs are resettet if WiFi handwheel is not responding for some milliseconds
- Autosquaring only starts after pressing the button for some time
- Ignore requests from multiple ESP32 if they try to control the same Ouput - for example a joystick


# Changelog
## 1.0.0
- inital version