#pragma once

#define CHECK_AS_BUTTON_DELAY_MS 100

typedef struct
{
    byte stepPin;
    byte endStopPin;
    byte dirPin;
} STEPPER;

enum AS_STATES
{
    none,    // Motor has an unknown state
    squared, // Motor has reached his endstop and has stopped
    finish,  // Motor is driven down from the endstop and has stopped
    off      // Motor is off
};

enum AXIS
{
    x,
    y,
    z,
    a,
    b,
    c
};

/**
 * @param motor1                    Can be x,y,z,a,b or c
 * @param motor1EndstopInput        Can be a number between 1 and 10 for input 1-10
 * @param motor1EndstopInverted     False for normally open endstops and true for normally closed endstops
 * @param motor1ASState
 * @param motor2                    Can be x,y,z,a,b or c
 * @param motor2EndstopInput        Can be a number between 1 and 10 for input 1-10
 * @param motor2EndstopInverted     False for normally open endstops and true for normally closed endstops
 * @param motor2ASState
 * @param stepsPerRevolution        How many steps are needed for a complete turn. Normal steppers need 200 steps. Now multiply the microstepp config. E.g. 200*8=1600
 * @param mmPerRevolution           How many mm has the machine moved after one turn.
 * @param asSpeed_mm_s              The speed for autosquaring in mm/s
 * @param reverseMotorDirection     Lets the motors rotate counter clockwise
 */
typedef struct
{
    bool active;
    AXIS motor1;
    byte motor1EndstopInput;
    bool motor1EndstopInverted;
    AS_STATES motor1ASState;
    float motor1DriveBackDistance_mm;
    AXIS motor2;
    byte motor2EndstopInput;
    bool motor2EndstopInverted;
    AS_STATES motor2ASState;
    float motor2DriveBackDistance_mm;
    uint16_t stepsPerRevolution;
    uint16_t mmPerRevolution;
    uint16_t asSpeed_mm_s;
    bool reverseMotorDirection;
} AUTOSQUARE_CONFIG;

class STEPPERCONTROL
{
public:
    STEPPERCONTROL();
    void setup();
    void addAxis(AUTOSQUARE_CONFIG config);
    void autosquareProcess();
    void printAutosquareConfig();

    bool autosquareRunning;

private:
    FastAccelStepperEngine stepperEngine = FastAccelStepperEngine();
    FastAccelStepper *steppers[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    void initializeStepper(byte configIndex);
    uint32_t timeASButtonPressed = 0;
    bool ASButtonPressed = false;
    bool getAutosquareButtonState();
    static void stepperTaskHandler(void *pvParameters);

    AUTOSQUARE_CONFIG autosquareConfigs[3];
    void initializeAutosquare();
    void terminateAutosquare();
    void setDirectionByAxisLabel(AXIS axis, bool direction);
    void setStepMotor(AXIS motor, bool state);
};