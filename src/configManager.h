#pragma once

#include <Arduino.h>
#include <steppercontrol.h>
#include <versionManager.h>

#define APP_NAME "OCS2 Configuration"   // Define application name
#define INI_FILE "/OCS2Config.ini"      // Define SPIFFS storage file

typedef struct {
    String ssid;
    String pass;
    const char* hostname;
} WIFI_CONFIG;

typedef struct {
    VERSION_INFO versionInfo;
    bool controlHandWheelFunctions;           // ESP_HANDWHEEL
    bool controlDriverEnable;                 // ESP_SET_ENA
    bool reverseEnableState;                  // REVERSE_ENA_STATE
    bool outputsInverted;                     // OUTPUTS_INVERTED
    uint8_t macAddressCustomByte;             // CONTROLLER_MAC_ADDRESS
    bool debugSerialOutput;                   // OCS_DEBUG
    bool resetFeedrateAndRotationOnTimeout;   // RESET_FEEDRATE_AND_ROTATION_SPEED_ON_CONNCTION_LOSS
    bool fluidNCJogging;                      // FLUIDNC_JOGGING
    int fluidNCMinJoggingSpeed;               // MIN_JOGGING_SPEED
    int autosquareButtonPressTime;            // AUTOSQUARE_BUTTON_PRESS_TIME_MS
    int stepperAcceleration;                  // STEPPER_ACCELERATION
    AUTOSQUARE_CONFIG autosquareConfig[3];
    bool enableWebInterface;                  // ENABLE_WEB_INTERFACE
    WIFI_CONFIG wifiConfig;
} OCS2_CONFIG;

extern OCS2_CONFIG mainConfig;

class CONFIGMANAGER {
  public:
    // This is used to start the config hotspot, if there is no config is not saved in the memory
    static bool startConfigHotspot;

    void setup();
    void loop();
    void printMainConfig();

    void startWiFi();

  private:
    void handleRoot();
    void handleNotFound();
    void setupWiFiAP();
    void setupWiFiConnect();
};

extern CONFIGMANAGER configManager;
