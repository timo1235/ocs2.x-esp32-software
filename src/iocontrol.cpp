#include <includes.h>

BU2506FV dac;

PCA9555 ioport1(PCA9555_ADRESS_1);
PCA9555 ioport2(PCA9555_ADRESS_2);

IOCONTROL::IOCONTROL()
{
}

void IOCONTROL::setup()
{
    while (!ioport1.begin(I2C_BUS_TWO_SDA, I2C_BUS_TWO_SCL))
    {
        Serial.println("PCA9555_1 not found!!");
        delay(500);
    }

    while (!ioport2.begin(I2C_BUS_TWO_SDA, I2C_BUS_TWO_SCL))
    {
        Serial.println("PCA9555_2 not found!!");
        delay(500);
    }

    for (uint8_t i = 0; i < 15; i++)
    {
        ioport1.pinMode(i, INPUT);
    }

    for (uint8_t i = 0; i < 15; i++)
    {
        ioport2.pinMode(i, INPUT);
    }

    // Set output pin modes
    ioport1.pinMode(IO1_DIRX, OUTPUT);
    ioport1.pinMode(IO1_DIRY, OUTPUT);
    ioport1.pinMode(IO1_DIRZ, OUTPUT);
    ioport1.pinMode(IO1_DIRA, OUTPUT);
    ioport1.pinMode(IO1_DIRB, OUTPUT);
    ioport1.pinMode(IO1_DIRC, OUTPUT);
    #ifdef ESP_HANDWHEEL
    ioport1.pinMode(IO1_SPEED1, OUTPUT);
    ioport1.pinMode(IO1_SPEED2, OUTPUT);
    ioport1.pinMode(IO1_AUSWAHLX, OUTPUT);
    ioport1.pinMode(IO1_AUSWAHLY, OUTPUT);
    ioport1.pinMode(IO1_AUSWAHLZ, OUTPUT);
    ioport1.pinMode(IO1_OK, OUTPUT);
    ioport1.pinMode(IO1_MOTORSTART, OUTPUT);
    ioport1.pinMode(IO1_PROGRAMMSTART, OUTPUT);
    #endif
    
    setOut1(LOW);
    setOut2(LOW);
    setOut3(LOW);
    setOut4(LOW);

    ioport2.pinMode(IO2_OUT1, OUTPUT);
    ioport2.pinMode(IO2_OUT2, OUTPUT);
    ioport2.pinMode(IO2_OUT3, OUTPUT);
    ioport2.pinMode(IO2_OUT4, OUTPUT);
    ioport2.pinMode(IO2_ENA, OUTPUT);

    // Set default states
    setDirX(LOW);
    setDirY(LOW);
    setDirZ(LOW);
    setDirA(LOW);
    setDirB(LOW);
    setDirC(LOW);
    #ifdef ESP_HANDWHEEL
    setSpeed1(LOW);
    setSpeed2(LOW);
    setAuswahlX(LOW);
    setAuswahlY(LOW);
    setAuswahlZ(LOW);
    setOK(LOW);
    setMotorStart(LOW);
    setProgrammStart(LOW);
    #endif
    setENA(LOW);

    #ifdef ESP_HANDWHEEL
    // DAC
    dac.begin(BU2506_LD_PIN);
    dac.resetOutputs();
    resetJoySticksToDefaults();
    #endif
}

bool IOCONTROL::getAlarmAll()
{
    return ioport1.digitalRead(IO1_ALARMALL);
}
bool IOCONTROL::getAutosquare()
{
    return ioport1.digitalRead(IO1_AUTOSQUARE);
}
bool IOCONTROL::getIn1()
{
    return ioport2.digitalRead(IO2_IN1);
}
bool IOCONTROL::getIn2()
{
    return ioport2.digitalRead(IO2_IN2);
}
bool IOCONTROL::getIn3()
{
    return ioport2.digitalRead(IO2_IN3);
}
bool IOCONTROL::getIn4()
{
    return ioport2.digitalRead(IO2_IN4);
}
bool IOCONTROL::getIn5()
{
    return ioport2.digitalRead(IO2_IN5);
}
bool IOCONTROL::getIn6()
{
    return ioport2.digitalRead(IO2_IN6);
}
bool IOCONTROL::getIn7()
{
    return ioport2.digitalRead(IO2_IN7);
}
bool IOCONTROL::getIn8()
{
    return ioport2.digitalRead(IO2_IN8);
}
bool IOCONTROL::getIn9()
{
    return ioport2.digitalRead(IO2_IN9);
}
bool IOCONTROL::getIn10()
{
    return ioport2.digitalRead(IO2_IN10);
}
bool IOCONTROL::getSpindelOnOff()
{
    return ioport2.digitalRead(IO2_SPINDEL);
}
// setters / outputs
void IOCONTROL::setDirX(bool value)
{
    ioport1.digitalWrite(IO1_DIRX, value);
}
void IOCONTROL::setDirY(bool value)
{
    ioport1.digitalWrite(IO1_DIRY, value);
}
void IOCONTROL::setDirZ(bool value)
{
    ioport1.digitalWrite(IO1_DIRZ, value);
}
void IOCONTROL::setDirA(bool value)
{
    ioport1.digitalWrite(IO1_DIRA, value);
}
void IOCONTROL::setDirB(bool value)
{
    ioport1.digitalWrite(IO1_DIRB, value);
}
void IOCONTROL::setDirC(bool value)
{
    ioport1.digitalWrite(IO1_DIRC, value);
}
#ifdef ESP_HANDWHEEL
void IOCONTROL::setSpeed1(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_SPEED1, !value);
#else
    ioport1.digitalWrite(IO1_SPEED1, value);
#endif
}
void IOCONTROL::setSpeed2(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_SPEED2, !value);
#else
    ioport1.digitalWrite(IO1_SPEED2, value);
#endif
}
void IOCONTROL::setAuswahlX(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_AUSWAHLX, !value);
#else
    ioport1.digitalWrite(IO1_AUSWAHLX, value);
#endif
}
void IOCONTROL::setAuswahlY(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_AUSWAHLY, !value);
#else
    ioport1.digitalWrite(IO1_AUSWAHLY, value);
#endif
}
void IOCONTROL::setAuswahlZ(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_AUSWAHLZ, !value);
#else
    ioport1.digitalWrite(IO1_AUSWAHLZ, value);
#endif
}
void IOCONTROL::setOK(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_OK, !value);
#else
    ioport1.digitalWrite(IO1_OK, value);
#endif
}
void IOCONTROL::setMotorStart(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_MOTORSTART, !value);
#else
    ioport1.digitalWrite(IO1_SPEED1, value);
#endif
}
void IOCONTROL::setProgrammStart(bool value)
{
#ifdef ESTLCAM_CONTROLLER
    ioport1.digitalWrite(IO1_PROGRAMMSTART, !value);
#else
    ioport1.digitalWrite(IO1_PROGRAMMSTART, value);
#endif
}
#endif
void IOCONTROL::setENA(bool value)
{
    ioport2.digitalWrite(IO2_ENA, value);
}
void IOCONTROL::setOut1(bool value)
{
    ioport2.digitalWrite(IO2_OUT1, value);
}
void IOCONTROL::setOut2(bool value)
{
    ioport2.digitalWrite(IO2_OUT2, value);
}
void IOCONTROL::setOut3(bool value)
{
    ioport2.digitalWrite(IO2_OUT3, value);
}
void IOCONTROL::setOut4(bool value)
{
    ioport2.digitalWrite(IO2_OUT4, value);
}

// DAC functions
#ifdef ESP_HANDWHEEL
void IOCONTROL::resetJoySticksToDefaults() {
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_X);
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_Y);
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_Z);
}

void IOCONTROL::dacSetAllChannel(int value){
    for (byte i = 1; i < 9; i++)
    {
        dac.analogWrite(value, i);
    }
}
#endif

void IOCONTROL::writeDataBag(DATA_TO_CONTROL *data) {
    #ifdef ESP_HANDWHEEL
    dac.analogWrite(data->joystickX, DAC_JOYSTICK_X);
    dac.analogWrite(data->joystickY, DAC_JOYSTICK_Y);
    dac.analogWrite(data->joystickZ, DAC_JOYSTICK_Z);

    dac.analogWrite(data->feedrate, DAC_FEEDRATE);
    dac.analogWrite(data->rotationSpeed, DAC_ROTATION_SPEED);
     
    setAuswahlX(data->auswahlX);
    setAuswahlY(data->auswahlY);
    setAuswahlZ(data->auswahlZ);

    setOK(data->ok);
    setMotorStart(data->motorStart);
    setProgrammStart(data->programmStart);

    #endif
}

IOCONTROL ios;