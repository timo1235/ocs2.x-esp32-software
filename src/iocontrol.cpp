#include <includes.h>

BU2506FV dac;
PCA9555 ioport1(PCA9555_ADRESS_1);
PCA9555 ioport2(PCA9555_ADRESS_2);

OneWire oneWire(TEMPERATURE_SENSOR_PIN);
DallasTemperature tempSensors(&oneWire);

IOCONTROL::IOCONTROL()
{
}

void IOCONTROL::setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    this->checkPCA9555();

#if ESP_HANDWHEEL == true
    // DAC
    dac.begin(BU2506_LD_PIN);
    dac.resetOutputs();
    resetJoySticksToDefaults();
#endif

    tempSensors.begin();

    pinMode(IOInterrupt1Pin, INPUT_PULLUP);
    pinMode(IOInterrupt2Pin, INPUT_PULLUP);

    // Create a task for the iocontrol
    xTaskCreatePinnedToCore(
        IOCONTROL::ioControlTask, /* Task function. */
        "IO Task",                /* name of task. */
        10000,                    /* Stack size of task */
        this,                     /* parameter of the task */
        1,                        /* priority of the task */
        &ioControlTaskHandle,     /* Task handle to keep track of created task */
        1);
    // Create a task for reading the PCA9555 inputs
    xTaskCreatePinnedToCore(
        IOCONTROL::ioPortTask, /* Task function. */
        "IO Task",             /* name of task. */
        10000,                 /* Stack size of task */
        this,                  /* parameter of the task */
        1,                     /* priority of the task */
        &ioPortTaskHandle,     /* Task handle to keep track of created task */
        1);                    // Has to be cpu 1because otherwi
}

void IOCONTROL::checkPCA9555()
{
    bool io1Success = false;
    bool io2Success = false;
    while (!io1Success || !io2Success)
    {
        io1Success = ioport1.begin(I2C_BUS_SDA, I2C_BUS_SCL);
        io2Success = ioport2.begin(I2C_BUS_SDA, I2C_BUS_SCL);
        if (!io1Success)
        {
            DPRINTLN("PCA9555_1 not found!!");
            this->IOInitialized = false;
        }
        if (!io2Success)
        {
            DPRINTLN("PCA9555_2 not found!!");
            this->IOInitialized = false;
        }

        if (!io1Success || !io2Success)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
    }

    if (!this->IOInitialized)
    {
        this->initPCA9555();
    }
}

void IOCONTROL::initPCA9555()
{
    // Set output pin modes for IO Expander 1
    ioport1.pinMode(IO1_DIRX, OUTPUT);
    ioport1.pinMode(IO1_DIRY, OUTPUT);
    ioport1.pinMode(IO1_DIRZ, OUTPUT);
    ioport1.pinMode(IO1_DIRA, OUTPUT);
    ioport1.pinMode(IO1_DIRB, OUTPUT);
    ioport1.pinMode(IO1_DIRC, OUTPUT);
#if ESP_HANDWHEEL == true
    ioport1.pinMode(IO1_SPEED1, OUTPUT);
    ioport1.pinMode(IO1_SPEED2, OUTPUT);
    ioport1.pinMode(IO1_SELECT_AXIS_X, OUTPUT);
    ioport1.pinMode(IO1_SELECT_AXIS_Y, OUTPUT);
    ioport1.pinMode(IO1_SELECT_AXIS_Z, OUTPUT);
    ioport1.pinMode(IO1_OK, OUTPUT);
    ioport1.pinMode(IO1_MOTORSTART, OUTPUT);
    ioport1.pinMode(IO1_PROGRAMMSTART, OUTPUT);
#else
    ioport1.pinMode(IO1_SPEED1, INPUT);
    ioport1.pinMode(IO1_SPEED2, INPUT);
    ioport1.pinMode(IO1_SELECT_AXIS_Z, INPUT);
    ioport1.pinMode(IO1_SELECT_AXIS_Y, INPUT);
    ioport1.pinMode(IO1_SELECT_AXIS_X, INPUT);
    ioport1.pinMode(IO1_OK, INPUT);
    ioport1.pinMode(IO1_MOTORSTART, INPUT);
    ioport1.pinMode(IO1_PROGRAMMSTART, INPUT);

#endif
    ioport1.pinMode(IO1_ALARMALL, INPUT);
    ioport1.pinMode(IO1_AUTOSQUARE, INPUT);

    // Set output pin modes for IO Expander 2
    ioport2.pinMode(IO2_IN1, INPUT);
    ioport2.pinMode(IO2_IN2, INPUT);
    ioport2.pinMode(IO2_IN3, INPUT);
    ioport2.pinMode(IO2_IN4, INPUT);
    ioport2.pinMode(IO2_IN5, INPUT);
    ioport2.pinMode(IO2_IN6, INPUT);
    ioport2.pinMode(IO2_IN7, INPUT);
    ioport2.pinMode(IO2_IN8, INPUT);
    ioport2.pinMode(IO2_IN9, INPUT);
    ioport2.pinMode(IO2_IN10, INPUT);
    ioport2.pinMode(IO2_SPINDEL, INPUT);
    ioport2.pinMode(IO2_OUT1, OUTPUT);
    ioport2.pinMode(IO2_OUT2, OUTPUT);
    ioport2.pinMode(IO2_OUT3, OUTPUT);
    ioport2.pinMode(IO2_OUT4, OUTPUT);

    // ioport2.pinMode(IO2_ENA, INPUT);
    ioport2.pinMode(IO2_ENA, OUTPUT);

    setOut1(LOW);
    setOut2(LOW);
    setOut3(LOW);
    setOut4(LOW);

    // Set default states
    setDirX(LOW);
    setDirY(LOW);
    setDirZ(LOW);
    setDirA(LOW);
    setDirB(LOW);
    setDirC(LOW);
#if ESP_HANDWHEEL == true
    setSpeed1(LOW);
    setSpeed2(LOW);
    setAuswahlX(LOW);
    setAuswahlY(LOW);
    setAuswahlZ(LOW);
    setOK(LOW);
    setMotorStart(LOW);
    setProgrammStart(LOW);
    setENA(LOW);
#endif
    ioport1.pinStates();
    DPRINTLN("Reading Port 1");
    ioport2.pinStates();
    DPRINTLN("Reading Port 2");

    this->IOInitialized = true;
}

void IOCONTROL::loop()
{
}

void IOCONTROL::ioControlTask(void *pvParameters)
{
    auto *ioControl = (IOCONTROL *)pvParameters;
    for (;;)
    {
        ioControl->panelLED.loop();

        // Read temperature if needed
        if (millis() - ioControl->lastTemperatureRead > TEMPERATURE_READ_INTERVAL_MS)
        {
            ioControl->lastTemperatureRead = millis();
            ioControl->readTemperatures();
        }

        // Update client data if needed
        if (millis() - ioControl->lastClientDataUpdate > CLIENT_DATA_UPDATE_INTERVAL_MS)
        {
            ioControl->lastClientDataUpdate = millis();
            ioControl->updateClientData();
        }

        ioControl->updateBounceInputs();
        vTaskDelay(1);
    }
}

void IOCONTROL::ioPortTask(void *pvParameters)
{
    auto *ioControl = (IOCONTROL *)pvParameters;
    for (;;)
    {
        if (ioControl->IOInitialized)
        {
            byte interrupt1 = digitalRead(IOInterrupt1Pin);
            byte interrupt2 = digitalRead(IOInterrupt2Pin);
            if (interrupt1 == LOW)
            {
                ioport1.pinStates();
                DPRINTLN("Reading Port 1");
            }
            if (interrupt2 == LOW)
            {
                ioport2.pinStates();
                DPRINTLN("Reading Port 2");
            }

            ioControl->checkPCA9555();
        }

        vTaskDelay(1);
    }
}

bool IOCONTROL::getAlarmAll(bool forceDirectRead)
{
    if (forceDirectRead)
    {
        return !ioport1.stateOfPin(IO1_ALARMALL);
    }
    else
    {
        return !this->bounceInputs[0].state;
    }
}
bool IOCONTROL::getAutosquare(bool forceDirectRead)
{
    if (forceDirectRead)
    {
        return !ioport1.stateOfPin(IO1_AUTOSQUARE);
    }
    else
    {
        return !this->bounceInputs[1].state;
    }
}
bool IOCONTROL::getIn1()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN1) : ioport2.stateOfPin(IO2_IN1);
}
bool IOCONTROL::getIn2()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN2) : ioport2.stateOfPin(IO2_IN2);
}
bool IOCONTROL::getIn3()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN3) : ioport2.stateOfPin(IO2_IN3);
}
bool IOCONTROL::getIn4()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN4) : ioport2.stateOfPin(IO2_IN4);
}
bool IOCONTROL::getIn5()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN5) : ioport2.stateOfPin(IO2_IN5);
}
bool IOCONTROL::getIn6()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN6) : ioport2.stateOfPin(IO2_IN6);
}
bool IOCONTROL::getIn7()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN7) : ioport2.stateOfPin(IO2_IN7);
}
bool IOCONTROL::getIn8()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN8) : ioport2.stateOfPin(IO2_IN8);
}
bool IOCONTROL::getIn9()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN9) : ioport2.stateOfPin(IO2_IN9);
}
bool IOCONTROL::getIn10()
{
    return REVERSE_INPUTS ? !ioport2.stateOfPin(IO2_IN10) : ioport2.stateOfPin(IO2_IN10);
}
bool IOCONTROL::getSpindelOnOff(bool forceDirectRead)
{
    if (forceDirectRead)
    {
        return ioport2.stateOfPin(IO2_SPINDEL);
    }
    else
    {
        return this->bounceInputs[2].state;
    }
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
#if ESP_HANDWHEEL == true
void IOCONTROL::setSpeed1(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_SPEED1, !value);
#else
    ioport1.digitalWrite(IO1_SPEED1, value);
#endif
}
void IOCONTROL::setSpeed2(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_SPEED2, !value);
#else
    ioport1.digitalWrite(IO1_SPEED2, value);
#endif
}
void IOCONTROL::setAuswahlX(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_SELECT_AXIS_X, !value);
#else
    ioport1.digitalWrite(IO1_SELECT_AXIS_X, value);
#endif
}
void IOCONTROL::setAuswahlY(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_SELECT_AXIS_Y, !value);
#else
    ioport1.digitalWrite(IO1_SELECT_AXIS_Y, value);
#endif
}
void IOCONTROL::setAuswahlZ(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_SELECT_AXIS_Z, !value);
#else
    ioport1.digitalWrite(IO1_SELECT_AXIS_Z, value);
#endif
}
void IOCONTROL::setOK(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_OK, !value);
#else
    ioport1.digitalWrite(IO1_OK, value);
#endif
}
void IOCONTROL::setMotorStart(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_MOTORSTART, !value);
#else
    ioport1.digitalWrite(IO1_SPEED1, value);
#endif
}
void IOCONTROL::setProgrammStart(bool value)
{
#if OUTPUTS_INVERTED == true
    ioport1.digitalWrite(IO1_PROGRAMMSTART, !value);
#else
    ioport1.digitalWrite(IO1_PROGRAMMSTART, value);
#endif
}
#endif // ESP_HANDWHEEL == true
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
#if  ESP_HANDWHEEL == true
void IOCONTROL::resetJoySticksToDefaults()
{
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_X);
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_Y);
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_Z);
}

void IOCONTROL::dacSetAllChannel(int value)
{
    for (byte i = 1; i < 9; i++)
    {
        dac.analogWrite(value, i);
    }
}
void IOCONTROL::setFeedrate(int value)
{
    dac.analogWrite(value, DAC_FEEDRATE);
}
void IOCONTROL::setRotationSpeed(int value)
{
    dac.analogWrite(value, DAC_ROTATION_SPEED);
}
#endif

void IOCONTROL::writeDataBag(DATA_TO_CONTROL *data)
{
#if ESP_HANDWHEEL == true
    if (data->command.setJoystick)
    {
        dac.analogWrite(data->joystickX, DAC_JOYSTICK_X);
        dac.analogWrite(data->joystickY, DAC_JOYSTICK_Y);
        dac.analogWrite(data->joystickZ, DAC_JOYSTICK_Z);
    }
    if (data->command.setFeedrate)
    {
        dac.analogWrite(data->feedrate, DAC_FEEDRATE);
    }
    if (data->command.setRotationSpeed)
    {
        dac.analogWrite(data->rotationSpeed, DAC_ROTATION_SPEED);
    }
    if (data->command.setAxisSelect)
    {
        setAuswahlX(data->selectAxisX);
        setAuswahlY(data->selectAxisY);
        setAuswahlZ(data->selectAxisZ);
    }
    if (data->command.setOk)
    {
        setOK(data->ok);
    }
    if (data->command.setProgrammStart)
    {
        setProgrammStart(data->programmStart);
    }
    if (data->command.setMotorStart)
    {
        setMotorStart(data->motorStart);
    }
    if (data->command.setEna)
    {
        setENA(data->ena);
    }
#endif
    if(data->command.setOutput1) {
        setOut1(data->output1);
    }
    if(data->command.setOutput2) {
        setOut2(data->output2);
    }
    if(data->command.setOutput3) {
        setOut3(data->output3);
    }
    if(data->command.setOutput4) {
        setOut4(data->output4);
    }
}

void IOCONTROL::setAllIOsRandom()
{
    setAuswahlX(random(0, 1));
    setAuswahlY(random(0, 1));
    setAuswahlZ(random(0, 1));

    setOK(random(0, 1));
    setMotorStart(random(0, 1));
    setProgrammStart(random(0, 1));
}

/**
 * @param number Number of the input. Possible values are 1-10.
 */
bool IOCONTROL::getIn(byte number)
{
    switch (number)
    {
    case 1:
        return getIn1();
        break;
    case 2:
        return getIn2();
        break;
    case 3:
        return getIn3();
        break;
    case 4:
        return getIn4();
        break;
    case 5:
        return getIn5();
        break;
    case 6:
        return getIn6();
        break;
    case 7:
        return getIn7();
        break;
    case 8:
        return getIn8();
        break;
    case 9:
        return getIn9();
        break;
    case 10:
        return getIn10();
        break;
    default:
        return false;
        break;
    }
}

void IOCONTROL::resetInterrupts()
{
    // for (byte i = 0; i < 15; i++)
    // {
    //     DPRINT(i);
    //     DPRINT(": ");
    //     DPRINT(ioport1.stateOfPin(i));
    //     DPRINT(" | ");

    //     // ioport1.pinStates();
    //     // ioport2.stateOfPin(i);
    // }
    // DPRINT("IO16: ");
    // DPRINT(getAutosquare());
    // DPRINTLN("");
}

void IOCONTROL::readTemperatures()
{
    tempSensors.requestTemperatures();
    for (uint8_t i = 0; i < tempSensors.getDS18Count(); i++)
    {
        // The order is turned around, so that the onboard sensor is always the first one
        uint8_t index = tempSensors.getDS18Count() - 1 - i;
        this->temperatures[index] = tempSensors.getTempCByIndex(i);
        dataToClient.temperatures[index] = this->temperatures[index];
        DPRINTLN("Temperature " + String(index) + ": " + String(this->temperatures[index]));
    }
}

int IOCONTROL::getTemperature(byte number)
{
    return this->temperatures[number];
}

void IOCONTROL::startBlinkRJ45LED()
{
    this->panelLED.startBlink();
}

void IOCONTROL::stopBlinkRJ45LED()
{
    this->panelLED.stopBlink();
}

void IOCONTROL::updateBounceInputs()
{
    for (byte i = 0; i < sizeof(bounceInputs) / sizeof(BOUNCE_INPUT); i++)
    {
        uint32_t currentMillis = millis();
        BOUNCE_INPUT *input = &this->bounceInputs[i];
        if (currentMillis - input->lastRead > input->readInterval_MS)
        {
            input->lastRead = currentMillis;
            bool currentState;
            if (input->ioport == 1)
            {
                currentState = ioport1.stateOfPin(input->port);
            }
            else
            {
                currentState = ioport2.stateOfPin(input->port);
            }
            if (currentState != input->lastState)
            {
                input->lastState = currentState;
                input->lastChange = currentMillis;
            }
            if (currentMillis - input->lastChange > input->debounceInterval_MS)
            {
                if (currentState != input->state)
                {
                    input->state = currentState;
                    input->lastChange = currentMillis;
                    DPRINT("Input changed after some ms, IOPORT: ");
                    DPRINT(input->ioport);
                    DPRINT(" Port: ");
                    DPRINT(input->port);
                    DPRINT(" State: ");
                    DPRINTLN(input->state);
                }
            }
        }
    }
}

void IOCONTROL::updateClientData()
{
    dataToClient.autosquareRunning = stepperControl.autosquareRunning;
    dataToClient.alarmState = this->getAlarmAll();
    dataToClient.spindelState = this->getSpindelOnOff();
}

IOCONTROL ioControl;