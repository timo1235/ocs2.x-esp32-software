#ifndef ocs_includes_h
#define ocs_includes_h

// WiFi Connection - Should not be used. ESPNow wont work if the WiFi has not channel 1
// #define WIFI_SSID "asdf"
// #define WIFI_PASS "1234567890"

// Enable debug output over the serial monitor with a baud of 115200
// Default: true
#define OCS_DEBUG true

// versioning
#define DEFAULT_TASK_PRIORITY 1
#define DEFAULT_TASK_CPU 1
#define TASK_PROTOCOL_CPU DEFAULT_TASK_CPU
#define TASK_IOCONTROL_CPU 0
#define TASK_STEPPERCONTROL_CPU DEFAULT_TASK_CPU

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>
#include <esp_wifi.h>
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
// Config for Webinterface
#include <ConfigAssist.h>   // Config assist class

// Temperature Sensor library
#include <DallasTemperature.h>
#include <OneWire.h>

#include <configManager.h>
#include <grbl_jogging.h>
#include <iocontrol.h>
#include <pinmap.h>
#include <protocol.h>
#include <steppercontrol.h>
#include <versionManager.h>
// #include <uiHandler.h>

// Testing CPU utilization

// This makes it usable in all files
extern IOCONTROL ioControl;
extern STEPPERCONTROL stepperControl;
extern DATA_TO_CONTROL dataToControl;
extern DATA_TO_CLIENT dataToClient;
extern PROTOCOL protocol;
extern GRBL_JOGGING grblJogging;
// extern UIHANDLER uiHandler;
extern VERSIONMANAGER versionManager;

typedef struct {
    bool IOControlInitialized;
    bool protocolInitialized;
    bool grblJoggingInitialized;
    bool uiHandlerInitialized;
    bool configManagerWiFiInitialized;
} GLOBAL_VARS;
extern GLOBAL_VARS GLOBAL;

// DEBUG MACRO
#if OCS_DEBUG == true                                   // Macros are usually in all capital letters.
    #define DPRINT(...) Serial.print(__VA_ARGS__)       // DPRINT is a macro, debug print
    #define DPRINTLN(...) Serial.println(__VA_ARGS__)   // DPRINTLN is a macro, debug print with new line
#else
    #define DPRINT(...)     // now defines a blank line
    #define DPRINTLN(...)   // now defines a blank line
#endif

#endif