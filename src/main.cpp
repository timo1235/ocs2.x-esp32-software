#include <includes.h>

TaskHandle_t DisplayTask;

void setup()
{
    Serial.begin(115200);

    ioControl.setup();
    protocol.setup();
#if AXIS1_ACTIVE || AXIS2_ACTIVE || AXIS3_ACTIVE
    stepperControl.setup();
#endif
#if AXIS1_ACTIVE
    stepperControl.addAxis(
        AXIS1_MOTOR1,
        AXIS1_MOTOR1_ENDSTOP_INPUT,
        AXIS1_MOTOR1_ENDSTOP_INVERTED,
        AXIS1_MOTOR2,
        AXIS1_MOTOR2_ENDSTOP_INPUT,
        AXIS1_MOTOR2_ENDSTOP_INVERTED,
        AXIS1_STEPS_PER_REVOLUTION,
        AXIS1_MM_PER_REVOLUTION,
        AXIS1_AS_SPEED_MM_S,
        AXIS1_REVERSE_MOTOR_DIRECTION);
#endif
#if AXIS2_ACTIVE
    stepperControl.addAxis(
        AXIS2_MOTOR1,
        AXIS2_MOTOR1_ENDSTOP_INPUT,
        AXIS2_MOTOR1_ENDSTOP_INVERTED,
        AXIS2_MOTOR2,
        AXIS2_MOTOR2_ENDSTOP_INPUT,
        AXIS2_MOTOR2_ENDSTOP_INVERTED,
        AXIS2_STEPS_PER_REVOLUTION,
        AXIS2_MM_PER_REVOLUTION,
        AXIS2_AS_SPEED_MM_S,
        AXIS2_REVERSE_MOTOR_DIRECTION);
#endif
#if AXIS3_ACTIVE
    stepperControl.addAxis(
        AXIS3_MOTOR1,
        AXIS3_MOTOR1_ENDSTOP_INPUT,
        AXIS3_MOTOR1_ENDSTOP_INVERTED,
        AXIS3_MOTOR2,
        AXIS3_MOTOR2_ENDSTOP_INPUT,
        AXIS3_MOTOR2_ENDSTOP_INVERTED,
        AXIS3_STEPS_PER_REVOLUTION,
        AXIS3_MM_PER_REVOLUTION,
        AXIS3_AS_SPEED_MM_S,
        AXIS3_REVERSE_MOTOR_DIRECTION);
#endif

    // DPRINT("Packagesize DATA_TO_CONTROL in bytes: ");
    // DPRINTLN(sizeof(DATA_TO_CONTROL));
    // DPRINT("Packagesize DATA_TO_CLIENT in bytes: ");
    // DPRINTLN(sizeof(DATA_TO_CLIENT));
}

void loop()
{
    // Nothing to do here, since all the work is done in seperate tasks
}
