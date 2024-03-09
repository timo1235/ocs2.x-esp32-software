#ifndef ocs_includes_h
#define ocs_includes_h

// versioning
#define OCS2_SOFTWARE_VERSION 2

#include <configuration.h>

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
// Onboard DAC library
#include <BU2506FV.h>
// Onboard PCA9555 library
#include <clsPCA9555.h>
// LED Controller library
#include <LEDController.h>
// Stepper library
#include "FastAccelStepper.h"
// Serial Transfer library
#include "SerialTransfer.h"

// Temperature Sensor library
#include <OneWire.h>
#include <DallasTemperature.h>

#include <protocol.h>
#include <pinmap.h>
#include <iocontrol.h>
#include <steppercontrol.h>
#include <grbl_jogging.h>

// This makes it usable in all files
extern IOCONTROL ioControl;
extern STEPPERCONTROL stepperControl;
extern DATA_TO_CONTROL dataToControl;
extern DATA_TO_CLIENT dataToClient;
extern PROTOCOL protocol;
extern GRBL_JOGGING grblJogging;

// DEBUG MACRO
#if OCS_DEBUG == true                             // Macros are usually in all capital letters.
#define DPRINT(...) Serial.print(__VA_ARGS__)     // DPRINT is a macro, debug print
#define DPRINTLN(...) Serial.println(__VA_ARGS__) // DPRINTLN is a macro, debug print with new line
#else
#define DPRINT(...)   // now defines a blank line
#define DPRINTLN(...) // now defines a blank line
#endif

#endif