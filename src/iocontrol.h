#pragma once

#include <protocol.h>
// Temperature Sensor library
#include <DallasTemperature.h>
#include <LEDController.h>
#include <OneWire.h>
#include <pinmap.h>

#define PCA9555_ADRESS_1 0x20
#define PCA9555_ADRESS_2 0x24

#define DAC_JOYSTICK_RESET_VALUE 1023 / 2
#define REVERSE_INPUTS true   // Since the inputs have pullups installed, reading a digital 1 is OFF and a digital 0 is ON -> reserveInputs = true
#define TEMPERATURE_READ_INTERVAL_MS 10000
#define CLIENT_DATA_UPDATE_INTERVAL_MS 5

#define WRITE_OUPUTS_INTERVAL_MS 20
#define CHECK_CHIPS_INTERVAL_MS 100

// DAC channel map
#define DAC_JOYSTICK_X 3
#define DAC_JOYSTICK_Y 4
#define DAC_JOYSTICK_Z 5
#define DAC_ROTATION_SPEED 2
#define DAC_FEEDRATE 1
#define DAC_6 6
#define DAC_7 7
#define DAC_8 8

typedef struct {
    byte ioport;                    // 1 or 2
    byte port;                      // E.g. IO1_ALARMALL
    uint32_t lastRead;              // miliis() of last read
    uint32_t lastChange;            // miliis() of last change
    bool lastState;                 // last state of the input
    bool state;                     // current state of the input
    uint16_t readInterval_MS;       // How often should the input be read
    uint16_t debounceInterval_MS;   // How long should the input be stable before it is considered stable
} BOUNCE_INPUT;

/** enum with mapping names of ports */
enum IOPort1 {
    IO1_DIRX,
    IO1_DIRY,
    IO1_DIRZ,
    IO1_DIRA,
    IO1_DIRB,
    IO1_DIRC,
    IO1_SPEED1,
    IO1_SPEED2,
    IO1_SELECT_AXIS_Z,
    IO1_SELECT_AXIS_Y,
    IO1_SELECT_AXIS_X,
    IO1_OK,
    IO1_MOTORSTART,
    IO1_PROGRAMMSTART,
    IO1_ALARMALL,
    IO1_AUTOSQUARE
};

enum IOPort2 { IO2_IN1, IO2_IN2, IO2_IN3, IO2_IN4, IO2_IN5, IO2_IN6, IO2_IN7, IO2_IN8, IO2_IN9, IO2_IN10, IO2_ENA, IO2_SPINDEL, IO2_OUT1, IO2_OUT2, IO2_OUT3, IO2_OUT4 };

class IOCONTROL {
  public:
    IOCONTROL();
    void setup();
    void initPCA9555();
    void initBU2560();
    void initDirPins();
    void freeDirPins();
    void loop();
    // getters / inputs
    bool getAlarmAll(bool forceDirectRead = false);
    bool getAutosquare(bool forceDirectRead = false);
    bool getIn1(bool invert = false);
    bool getIn2(bool invert = false);
    bool getIn3(bool invert = false);
    bool getIn4(bool invert = false);
    bool getIn5(bool invert = false);
    bool getIn6(bool invert = false);
    bool getIn7(bool invert = false);
    bool getIn8(bool invert = false);
    bool getIn9(bool invert = false);
    bool getIn10(bool invert = false);
    bool getIn(byte number, bool invert = false);
    bool getSpindelOnOff(bool forceDirectRead = false);
    // setters / outputs
    void setDirX(bool value);
    void setDirY(bool value);
    void setDirZ(bool value);
    void setDirA(bool value);
    void setDirB(bool value);
    void setDirC(bool value);

    void setSpeed1(bool value);
    void setSpeed2(bool value);
    void setAuswahlX(bool value);
    void setAuswahlY(bool value);
    void setAuswahlZ(bool value);
    void setOK(bool value);
    void setMotorStart(bool value);
    void setProgrammStart(bool value);
    void blinkPanelLED();

    void setENA(bool value);
    void setOut1(bool value);
    void setOut2(bool value);
    void setOut3(bool value);
    void setOut4(bool value);

    // DAC
    void resetJoySticksToDefaults();
    void dacSetAllChannel(int value);
    void setFeedrate(int value);
    void setRotationSpeed(int value);

    // general
    void writeIOPortOutputs(DATA_TO_CONTROL *data);
    void writeDACOutputs(DATA_TO_CONTROL *data);

    void startBlinkRJ45LED();
    void stopBlinkRJ45LED();

    // This lets the ControllerModule control the direction of the motors
    void enableControllerDirBuffer();
    // This stops the ControllerModule from controlling the direction of the motors - Used for Autosquaring
    void disableControllerDirBuffer();

    // Enable / disable the onboard DAC for the analog outputs from esp32 to the controller(when using the ESP32 Handwheel)
    // Disable means, the ESP32 has no control over the analog outputs
    void enableDACOutputs();
    void disableDACOutputs();

    int getTemperature(byte number);   // Number can be 0-4. 0 is the onboard sensor

    bool IOInitialized = false;

  private:
    // Task handler
    static void ioControlTask(void *pvParameters);
    static void ioPortTask(void *pvParameters);
    static void checkChipsConnection(void *pvParameters);
    static void writeOutputsTask(void *pvParameters);

    // Temp sensors
    int temperatures[5];
    void readTemperatures();
    uint32_t lastTemperatureRead = 20000;

    TaskHandle_t ioPortTaskHandle;

    LEDCONTROLLER panelLED = LEDCONTROLLER(ESP_PANEL_LED_PIN);
    BOUNCE_INPUT bounceInputs[3] = {
        {/* ioport */ 1, /* port */ IO1_ALARMALL, /* lastRead */ 0, /* lastChange */ 0, /* lastState */ false, /* state */ true, /* readInterval */ 1, /* debounce */ 10},
        {/* ioport */ 1, /* port */ IO1_AUTOSQUARE, /* lastRead */ 0, /* lastChange */ 0, /* lastState */ false, /* state */ true, /* readInterval */ 1, /* debounce */ 10},
        {/* ioport */ 2, /* port */ IO2_SPINDEL, /* lastRead */ 0, /* lastChange */ 0, /* lastState */ false, /* state */ false, /* readInterval */ 1, /* debounce */ 10}};
    void updateBounceInputs();
    void updateClientData();
    void checkPCA9555();
    uint32_t lastI2CCheck = 0;

    uint32_t lastClientDataUpdate = 20000;
};