#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <steppercontrol.h>

#define WIFI_TIMEOUT_CHECK_INTERVAL_MS 5

// This struct describes what data is send and what is expected
typedef struct
{
    unsigned setJoystick : 1;
    unsigned setFeedrate : 1;
    unsigned setRotationSpeed : 1;
    unsigned setAutosquare : 1;
    unsigned setEna : 1;
    unsigned setAxisSelect : 1;
    unsigned setOk : 1;
    unsigned setProgrammStart : 1;
    unsigned setMotorStart : 1;
    unsigned setSpeed1 : 1;
    unsigned setSpeed2 : 1;
    unsigned returnACK : 1;  // Return ACK to client
    unsigned returnData : 1; // Return mainboard data to client, this can be the temperature, autosquaring state and so son and replaces ACK
    uint16_t updateInterval_MS; // Interval in ms the client sends data to the mainboard
} DATA_COMMAND;

typedef struct
{
    unsigned setJoystick : 1;
    unsigned setFeedrate : 1;
    unsigned setRotationSpeed : 1;
    unsigned setAutosquare : 1;
    unsigned setEna : 1;
    unsigned setAxisSelect : 1;
    unsigned setOk : 1;
    unsigned setProgrammStart : 1;
    unsigned setMotorStart : 1;
    unsigned setSpeed1 : 1;
    unsigned setSpeed2 : 1;
    uint32_t lastSeen;
    uint32_t sendMessageFailCount;
    uint32_t sendMessageFailSuccessivelyCount;
    uint32_t sendMessageSuccessCount;
    uint32_t sendMessageSuccessSuccessivelyCount;
    unsigned active : 1;
    uint16_t integerAddress;
    uint8_t macAddress[6];
    unsigned ignored : 1;
    uint16_t updateInterval_MS; // Interval in ms the client sends data to the mainboard
} CLIENT_DATA;

// This struct represents the typical message, a client sends to the main ESP32
// Max size of this struct is 250 bytes
// uint16_t : 2Bytes
// unsigned : 1 Bit each
typedef struct
{
    byte softwareVersion;
    uint16_t joystickX;
    uint16_t joystickY;
    uint16_t joystickZ;
    uint16_t feedrate;
    uint16_t rotationSpeed;
    unsigned autosquare : 1;
    unsigned ena : 1;
    unsigned selectAxisX : 1;
    unsigned selectAxisY : 1;
    unsigned selectAxisZ : 1;
    unsigned ok : 1;
    unsigned programmStart : 1;
    unsigned motorStart : 1;
    unsigned speed1 : 1;
    unsigned speed2 : 1;
    DATA_COMMAND command;
} DATA_TO_CONTROL;

// This struct describes the auto square data
typedef struct
{
    unsigned axisActive : 1;
    uint8_t axisMotor1State;
    uint8_t axisMotor2State;

} AUTOSQUARE_STATE;

// This struct represents the typical message, the main ESP32 sends to a client
// Max size of this struct is 250
// uint16_t : 2Bytes
// unsigned : 1 Bit each
typedef struct
{
    byte softwareVersion;
    int temperatures[5];
    unsigned autosquareRunning : 1;
    unsigned spindelState : 1;
    unsigned alarmState : 1;
    unsigned peerIgnored : 1; // This peer is ignored by the mainboard and should stop sending data
    AUTOSQUARE_STATE autosquareState[3];
} DATA_TO_CLIENT;

union ArrayToInteger {
  byte address[6];
  uint16_t integer;
};

class PROTOCOL
{
public:
    void setup();
    void loop();
    static char *getMacStrFromAddress(uint8_t *address);
    static uint16_t getIntegerFromAddress(const uint8_t *address);
    static bool lockSending;
private:
    static void onDataSent(const uint8_t *address, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *address, const uint8_t *incomingData, int len);
    static esp_err_t sendMessageToClient(uint8_t *address, DATA_TO_CLIENT *data);
    // Saves that kind of functions are currently send to the esp by external devices
    // For example Handwheel 1 sends joystick data and Handwheel 2 sends feedrate data
    static CLIENT_DATA currentControls; 

    static CLIENT_DATA clients[5];
    static byte clientCount;
    static void updateClientData(CLIENT_DATA *client, DATA_TO_CONTROL *data, bool isNewClient);
    // @return true if everything is ok, false if the client is ignored
    static bool validateClientCommand(CLIENT_DATA *client, DATA_TO_CONTROL *data, bool isNewClient);
    static void sendIgnoreMessageToClient(CLIENT_DATA *client, bool silent);
    static void resetOutputsControlledByClient(CLIENT_DATA *client);

    static void protocolTaskHandler(void *pvParameters);
    TaskHandle_t protocolTask;
    
    static void dumpDataToControl();
    // static esp_now_peer_info_t peerInfo;
    static bool addPeerIfNotExists(uint8_t *address);

    uint32_t lastTimeoutCheck = 5000;
    
};
