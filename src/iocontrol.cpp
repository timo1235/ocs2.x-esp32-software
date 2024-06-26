#include <includes.h>

BU2506FV dac;
PCA9555 ioport1(PCA9555_ADRESS_1);
PCA9555 ioport2(PCA9555_ADRESS_2);

OneWire oneWire(TEMPERATURE_SENSOR_PIN);
DallasTemperature tempSensors(&oneWire);

String IOPort1Mapping[] = {"DIRX",          "DIRY",          "DIRZ",          "DIRA", "DIRB",       "DIRC",          "SPEED1",   "SPEED2",
                           "SELECT_AXIS_Z", "SELECT_AXIS_Y", "SELECT_AXIS_X", "OK",   "MOTORSTART", "PROGRAMMSTART", "ALARMALL", "AUTOSQUARE"};
String IOPort2Mapping[] = {"IN1", "IN2", "IN3", "IN4", "IN5", "IN6", "IN7", "IN8", "IN9", "IN10", "ENA", "SPINDEL", "OUT1", "OUT2", "OUT3", "OUT4"};

bool ioPort1Flag = false;
bool ioPort2Flag = false;

bool functionButtonPressedFlag = false;

// Semaphore to control access
SemaphoreHandle_t BU2560_Semaphore = NULL;

void IRAM_ATTR readIOPort1() { ioPort1Flag = true; }
void IRAM_ATTR readIOPort2() { ioPort2Flag = true; }

IOCONTROL::IOCONTROL() {}

void IOCONTROL::setup() {
    if (versionManager.isBoardType(BOARD_TYPE::undefined)) {
        DPRINTLN("IOControl: Board type is undefined. Functionalities are disabled.");
        return;
    }
    // Create semaphore for BU2560
    BU2560_Semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(BU2560_Semaphore);

    ioPortTaskHandle = NULL;

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 9) || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        pinMode(CONTROLLER_DIR_BUFFER_ENABLE_PIN, OUTPUT);
        this->enableControllerDirBuffer();
    }
    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 10) || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        pinMode(ESP32_DAC_ENABLE_PIN, OUTPUT);
        this->disableDACOutputs();
    }

    this->initPCA9555();

    if (mainConfig.controlHandWheelFunctions) {
        this->enableDACOutputs();
        this->initBU2560();
        DATA_TO_CONTROL data = {.joystickX = DAC_JOYSTICK_RESET_VALUE,
                                .joystickY = DAC_JOYSTICK_RESET_VALUE,
                                .joystickZ = DAC_JOYSTICK_RESET_VALUE,
                                .feedrate = 0,
                                .rotationSpeed = 0,
                                .command = {.setJoystick = true, .setFeedrate = true, .setRotationSpeed = true}};
        this->writeDACOutputs(&data);
    }

    tempSensors.begin();

    pinMode(IOInterrupt1Pin, INPUT_PULLUP);
    pinMode(IOInterrupt2Pin, INPUT_PULLUP);
    pinMode(FUNCTION_BUTTON_PIN, INPUT_PULLUP);

    // Interrupt for function button - starting the wifi webinterface
    attachInterrupt(FUNCTION_BUTTON_PIN, []() {
        if (functionButtonPressedFlag == true) {
            return;
        }
        functionButtonPressedFlag = true;
        xTaskCreate([](void *parameter) {
            if (!GLOBAL.configManagerWiFiInitialized) {
                configManager.startWiFi();
            }
            vTaskDelete(NULL);
        }, "IOControl: Function Button", 2048, nullptr, DEFAULT_TASK_PRIORITY, NULL);
    }, FALLING);

    // Create a task for the iocontrol
    xTaskCreatePinnedToCore(IOCONTROL::ioControlTask, /* Task function. */
                            "IO Task",                /* name of task. */
                            4096,                     /* Stack size of task */
                            this,                     /* parameter of the task */
                            DEFAULT_TASK_PRIORITY,    /* priority of the task */
                            NULL,                     /* Task handle to keep track of created task */
                            TASK_IOCONTROL_CPU);
    // Create a task for writing all outputs
    xTaskCreatePinnedToCore(IOCONTROL::writeOutputsTask, /* Task function. */
                            "IO Task",                   /* name of task. */
                            4096,                        /* Stack size of task */
                            this,                        /* parameter of the task */
                            DEFAULT_TASK_PRIORITY,       /* priority of the task */
                            NULL,                        /* Task handle to keep track of created task */
                            TASK_IOCONTROL_CPU);
    // Create a task for reading the PCA9555 inputs
    xTaskCreatePinnedToCore(IOCONTROL::ioPortTask, /* Task function. */
                            "IO Task",             /* name of task. */
                            4096,                  /* Stack size of task*/
                            this,                  /* parameter of the task */
                            DEFAULT_TASK_PRIORITY, /* priority of the task */
                            NULL,                  /* Task handle to keep track of created task */
                            TASK_IOCONTROL_CPU);
    // Create a task for reading the PCA9555 inputs
    xTaskCreatePinnedToCore(IOCONTROL::checkChipsConnection, /* Task function. */
                            "IO Task",                       /* name of task. */
                            4096,                            /* Stack size of task*/
                            this,                            /* parameter of the task */
                            DEFAULT_TASK_PRIORITY,           /* priority of the task */
                            NULL,                            /* Task handle to keep track of created task */
                            TASK_IOCONTROL_CPU);
}

void IOCONTROL::checkPCA9555() {
    if (ioPortTaskHandle != NULL) {
        return;
    }

    xTaskCreatePinnedToCore([](void *parameter) {
        IOCONTROL *ioControl = static_cast<IOCONTROL *>(parameter);
        for (;;) {
            bool io1Success = false;
            bool io2Success = false;
            bool io1PreviouslyFailed = false;
            bool io2PreviouslyFailed = false;

            byte i2c_sda, i2c_scl;

            if (versionManager.isBoardType(BOARD_TYPE::OCS2) && (versionManager.isVersion(2, 4) || versionManager.isVersion(2, 5))) {
                i2c_sda = I2C_BUS_SDA_OLD;
                i2c_scl = I2C_BUS_SCL_OLD;
            } else {
                i2c_sda = I2C_BUS_SDA;
                i2c_scl = I2C_BUS_SCL;
            }

            while (!io1Success || !io2Success) {
                io1Success = ioport1.isConnected();
                io2Success = ioport2.isConnected();

                if (!io1Success && !io1PreviouslyFailed) {
                    DPRINTLN("IOControl: PCA9555_1 not found!!");
                    ioControl->IOInitialized = false;
                    io1PreviouslyFailed = true;
                } else if (io1Success && io1PreviouslyFailed) {
                    io1PreviouslyFailed = false;
                }

                if (!io2Success && !io2PreviouslyFailed) {
                    DPRINTLN("IOControl: PCA9555_2 not found!!");
                    ioControl->IOInitialized = false;
                    io2PreviouslyFailed = true;
                } else if (io2Success && io2PreviouslyFailed) {
                    io2PreviouslyFailed = false;
                }

                if (!io1Success || !io2Success) {
                    digitalWrite(LED_BUILTIN, HIGH);
                    vTaskDelay(100);
                    digitalWrite(LED_BUILTIN, LOW);
                    vTaskDelay(100);
                }
            }

            if (!ioControl->IOInitialized) {
                ioControl->initPCA9555();
            }

            ioControl->ioPortTaskHandle = NULL;
            vTaskDelete(NULL);
        }
    }, "Check PCA9555 Chips", 2048, this, DEFAULT_TASK_PRIORITY, &ioPortTaskHandle, DEFAULT_TASK_CPU);
}

void IOCONTROL::initDirPins() {
    ioport1.pinMode(IO1_DIRX, OUTPUT);
    ioport1.pinMode(IO1_DIRY, OUTPUT);
    ioport1.pinMode(IO1_DIRZ, OUTPUT);
    ioport1.pinMode(IO1_DIRA, OUTPUT);
    ioport1.pinMode(IO1_DIRB, OUTPUT);
    if (versionManager.isBoardType(BOARD_TYPE::OCS2)) {
        ioport1.pinMode(IO1_DIRC, OUTPUT);
        setDirC(LOW);
    } else if (versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        ioport1.pinMode(IO1_DIRC, INPUT);
    }
    setDirX(LOW);
    setDirY(LOW);
    setDirZ(LOW);
    setDirA(LOW);
    setDirB(LOW);
}

void IOCONTROL::freeDirPins() {
    ioport1.pinMode(IO1_DIRX, INPUT);
    ioport1.pinMode(IO1_DIRY, INPUT);
    ioport1.pinMode(IO1_DIRZ, INPUT);
    ioport1.pinMode(IO1_DIRA, INPUT);
    ioport1.pinMode(IO1_DIRB, INPUT);
    ioport1.pinMode(IO1_DIRC, INPUT);
}

void IOCONTROL::initBU2560() {
    if (xSemaphoreTake(BU2560_Semaphore, (TickType_t) 50) == pdTRUE) {
        dac.begin(BU2506_LD_PIN);
        xSemaphoreGive(BU2560_Semaphore);
    }
}

void IOCONTROL::initPCA9555() {
    byte i2c_sda, i2c_scl;
    bool io1Success, io2Success;

    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && (versionManager.isVersion(2, 4) || versionManager.isVersion(2, 5))) {
        i2c_sda = I2C_BUS_SDA_OLD;
        i2c_scl = I2C_BUS_SCL_OLD;
    } else {
        i2c_sda = I2C_BUS_SDA;
        i2c_scl = I2C_BUS_SCL;
    }

    io1Success = ioport1.begin(i2c_sda, i2c_scl);
    io2Success = ioport2.begin(i2c_sda, i2c_scl);

    if (!io1Success || !io2Success) {
        this->IOInitialized = false;
        return;
    }

    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isVersion(2, 12)) {
        // OCS2 Version 12 have DIR pins directly connected to the controller DIR pins
        // Therefore we only use the pins as output, when the controller DIR is not connected
        // That is the case during autosquaring
        this->freeDirPins();
    } else {
        this->initDirPins();
    }

    if (mainConfig.controlHandWheelFunctions) {
        ioport1.pinMode(IO1_SPEED1, OUTPUT);
        ioport1.pinMode(IO1_SPEED2, OUTPUT);
        ioport1.pinMode(IO1_SELECT_AXIS_X, OUTPUT);
        ioport1.pinMode(IO1_SELECT_AXIS_Y, OUTPUT);
        ioport1.pinMode(IO1_SELECT_AXIS_Z, OUTPUT);
        ioport1.pinMode(IO1_OK, OUTPUT);
        ioport1.pinMode(IO1_MOTORSTART, OUTPUT);
        ioport1.pinMode(IO1_PROGRAMMSTART, OUTPUT);
        if (mainConfig.controlDriverEnable) {
            ioport2.pinMode(IO2_ENA, OUTPUT);
        } else {
            ioport2.pinMode(IO2_ENA, INPUT);
        }
    } else {
        ioport1.pinMode(IO1_SPEED1, INPUT);
        ioport1.pinMode(IO1_SPEED2, INPUT);
        ioport1.pinMode(IO1_SELECT_AXIS_Z, INPUT);
        ioport1.pinMode(IO1_SELECT_AXIS_Y, INPUT);
        ioport1.pinMode(IO1_SELECT_AXIS_X, INPUT);
        ioport1.pinMode(IO1_OK, INPUT);
        ioport1.pinMode(IO1_MOTORSTART, INPUT);
        ioport1.pinMode(IO1_PROGRAMMSTART, INPUT);
        ioport2.pinMode(IO2_ENA, INPUT);
    }

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

    setOut1(LOW);
    setOut2(LOW);
    setOut3(LOW);
    setOut4(LOW);

    // Set default states
    if (mainConfig.controlHandWheelFunctions) {
        setENA(LOW);

        setSpeed1(LOW);
        setSpeed2(LOW);
        setAuswahlX(LOW);
        setAuswahlY(LOW);
        setAuswahlZ(LOW);
        setOK(LOW);
        setMotorStart(LOW);
        setProgrammStart(LOW);
        setENA(LOW);
    }
    ioport1.pinStates();
    ioport2.pinStates();

    // Add interrupt for IO Expander 1 & 2
    attachInterrupt(IOInterrupt1Pin, readIOPort1, FALLING);
    attachInterrupt(IOInterrupt2Pin, readIOPort2, FALLING);

    DPRINTLN("IOControl: PCA9555 1 & 2 initialized");
    this->IOInitialized = true;
    GLOBAL.IOControlInitialized = true;
}

void IOCONTROL::loop() {}

void IOCONTROL::checkChipsConnection(void *pvParameters) {
    auto *ioControl = (IOCONTROL *) pvParameters;
    for (;;) {
        ioControl->checkPCA9555();
        if (mainConfig.controlHandWheelFunctions) {
            ioControl->initBU2560();
        }

        vTaskDelay(CHECK_CHIPS_INTERVAL_MS);
    }
}

void IOCONTROL::writeOutputsTask(void *pvParameters) {
    auto *ioControl = (IOCONTROL *) pvParameters;
    for (;;) {
        if (!ioControl->IOInitialized) {
            vTaskDelay(1000);
            continue;
        }

        ioControl->writeIOPortOutputs(&dataToControl);
        ioControl->writeDACOutputs(&dataToControl);

        vTaskDelay(WRITE_OUPUTS_INTERVAL_MS);
    }
}

void IOCONTROL::ioControlTask(void *pvParameters) {
    auto *ioControl = (IOCONTROL *) pvParameters;
    for (;;) {
        if (!ioControl->IOInitialized) {
            vTaskDelay(1000);
            continue;
        }

        ioControl->panelLED.loop();

        // Read temperature if needed
        if (millis() - ioControl->lastTemperatureRead > TEMPERATURE_READ_INTERVAL_MS) {
            ioControl->lastTemperatureRead = millis();
            ioControl->readTemperatures();
        }

        // Update client data if needed
        if (millis() - ioControl->lastClientDataUpdate > CLIENT_DATA_UPDATE_INTERVAL_MS) {
            ioControl->lastClientDataUpdate = millis();
            ioControl->updateClientData();
        }

        ioControl->updateBounceInputs();

        vTaskDelay(10);
    }
}

void IOCONTROL::ioPortTask(void *pvParameters) {
    auto *ioControl = (IOCONTROL *) pvParameters;
    for (;;) {
        if (!ioControl->IOInitialized) {
            vTaskDelay(1000);
            continue;
        }

        if (ioPort1Flag) {
            ioPort1Flag = false;
            ioport1.pinStates();
        }
        if (ioPort2Flag) {
            ioPort2Flag = false;
            ioport2.pinStates();
        }

        vTaskDelay(1);
    }
}

bool IOCONTROL::getAlarmAll(bool forceDirectRead) {
    if (forceDirectRead) {
        return !ioport1.stateOfPin(IO1_ALARMALL);
    } else {
        return !this->bounceInputs[0].state;
    }
}
bool IOCONTROL::getAutosquare(bool forceDirectRead) {
    if (forceDirectRead) {
        return !ioport1.stateOfPin(IO1_AUTOSQUARE);
    } else {
        return !this->bounceInputs[1].state;
    }
}

bool IOCONTROL::getIn1(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN1) : ioport2.stateOfPin(IO2_IN1); }
bool IOCONTROL::getIn2(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN2) : ioport2.stateOfPin(IO2_IN2); }
bool IOCONTROL::getIn3(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN3) : ioport2.stateOfPin(IO2_IN3); }
bool IOCONTROL::getIn4(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN4) : ioport2.stateOfPin(IO2_IN4); }
bool IOCONTROL::getIn5(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN5) : ioport2.stateOfPin(IO2_IN5); }
bool IOCONTROL::getIn6(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN6) : ioport2.stateOfPin(IO2_IN6); }
bool IOCONTROL::getIn7(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN7) : ioport2.stateOfPin(IO2_IN7); }
bool IOCONTROL::getIn8(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN8) : ioport2.stateOfPin(IO2_IN8); }
bool IOCONTROL::getIn9(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN9) : ioport2.stateOfPin(IO2_IN9); }
bool IOCONTROL::getIn10(bool invert) { return (REVERSE_INPUTS ^ invert) ? !ioport2.stateOfPin(IO2_IN10) : ioport2.stateOfPin(IO2_IN10); }

bool IOCONTROL::getSpindelOnOff(bool forceDirectRead) {
    if (forceDirectRead) {
        return ioport2.stateOfPin(IO2_SPINDEL);
    } else {
        return this->bounceInputs[2].state;
    }
}
// setters / outputs
void IOCONTROL::setDirX(bool value) { ioport1.digitalWrite(IO1_DIRX, value); }
void IOCONTROL::setDirY(bool value) { ioport1.digitalWrite(IO1_DIRY, value); }
void IOCONTROL::setDirZ(bool value) { ioport1.digitalWrite(IO1_DIRZ, value); }
void IOCONTROL::setDirA(bool value) { ioport1.digitalWrite(IO1_DIRA, value); }
void IOCONTROL::setDirB(bool value) { ioport1.digitalWrite(IO1_DIRB, value); }
void IOCONTROL::setDirC(bool value) { ioport1.digitalWrite(IO1_DIRC, value); }
void IOCONTROL::setSpeed1(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_SPEED1, !value);
    } else {
        ioport1.digitalWrite(IO1_SPEED1, value);
    }
}
void IOCONTROL::setSpeed2(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_SPEED2, !value);
    } else {
        ioport1.digitalWrite(IO1_SPEED2, value);
    }
}
void IOCONTROL::setAuswahlX(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_SELECT_AXIS_X, !value);
    } else {
        ioport1.digitalWrite(IO1_SELECT_AXIS_X, value);
    }
}
void IOCONTROL::setAuswahlY(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_SELECT_AXIS_Y, !value);
    } else {
        ioport1.digitalWrite(IO1_SELECT_AXIS_Y, value);
    }
}
void IOCONTROL::setAuswahlZ(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_SELECT_AXIS_Z, !value);
    } else {
        ioport1.digitalWrite(IO1_SELECT_AXIS_Z, value);
    }
}
void IOCONTROL::setOK(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_OK, !value);
    } else {
        ioport1.digitalWrite(IO1_OK, value);
    }
}
void IOCONTROL::setMotorStart(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_MOTORSTART, !value);
    } else {
        ioport1.digitalWrite(IO1_MOTORSTART, value);
    }
}
void IOCONTROL::setProgrammStart(bool value) {
    if (mainConfig.outputsInverted) {
        ioport1.digitalWrite(IO1_PROGRAMMSTART, !value);
    } else {
        ioport1.digitalWrite(IO1_PROGRAMMSTART, value);
    }
}
void IOCONTROL::setENA(bool value) {
    if (!mainConfig.controlDriverEnable) {
        return;
    }
    if (mainConfig.reverseEnableState) {
        ioport2.digitalWrite(IO2_ENA, !value);
    } else {
        ioport2.digitalWrite(IO2_ENA, value);
    }
}
void IOCONTROL::setOut1(bool value) { ioport2.digitalWrite(IO2_OUT1, value); }
void IOCONTROL::setOut2(bool value) { ioport2.digitalWrite(IO2_OUT2, value); }
void IOCONTROL::setOut3(bool value) { ioport2.digitalWrite(IO2_OUT3, value); }
void IOCONTROL::setOut4(bool value) { ioport2.digitalWrite(IO2_OUT4, value); }

// DAC functions
void IOCONTROL::resetJoySticksToDefaults() {
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_X);
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_Y);
    dac.analogWrite(DAC_JOYSTICK_RESET_VALUE, DAC_JOYSTICK_Z);
}

void IOCONTROL::dacSetAllChannel(int value) {
    for (byte i = 1; i < 9; i++) {
        dac.analogWrite(value, i);
    }
}
void IOCONTROL::setFeedrate(int value) { dac.analogWrite(value, DAC_FEEDRATE); }
void IOCONTROL::setRotationSpeed(int value) { dac.analogWrite(value, DAC_ROTATION_SPEED); }

void IOCONTROL::writeIOPortOutputs(DATA_TO_CONTROL *data) {
    // Stop if the io is not initialized - mostly means, the mainboard has no power or is not connected
    if (!this->IOInitialized) {
        return;
    }

    if (mainConfig.controlHandWheelFunctions) {
        if (data->command.setAxisSelect) {
            setAuswahlX(data->selectAxisX);
            setAuswahlY(data->selectAxisY);
            setAuswahlZ(data->selectAxisZ);
        }
        if (data->command.setOk) {
            setOK(data->ok);
        }
        if (data->command.setProgrammStart) {
            setProgrammStart(data->programmStart);
        }
        if (data->command.setMotorStart) {
            setMotorStart(data->motorStart);
        }
        if (data->command.setEna) {
            setENA(data->ena);
        }
        if (data->command.setSpeed1) {
            setSpeed1(data->speed1);
        }
        if (data->command.setSpeed2) {
            setSpeed2(data->speed2);
        }
    }
    if (data->command.setOutput1) {
        setOut1(data->output1);
    }
    if (data->command.setOutput2) {
        setOut2(data->output2);
    }
    if (data->command.setOutput3) {
        setOut3(data->output3);
    }
    if (data->command.setOutput4) {
        setOut4(data->output4);
    }
}

void IOCONTROL::writeDACOutputs(DATA_TO_CONTROL *data) {
    if (xSemaphoreTake(BU2560_Semaphore, (TickType_t) 5) == pdTRUE) {
        if (mainConfig.controlHandWheelFunctions) {
            if (data->command.setJoystick) {
                dac.analogWrite(data->joystickX, DAC_JOYSTICK_X);
                dac.analogWrite(data->joystickY, DAC_JOYSTICK_Y);
                dac.analogWrite(data->joystickZ, DAC_JOYSTICK_Z);
            }
            if (data->command.setFeedrate) {
                dac.analogWrite(data->feedrate, DAC_FEEDRATE);
            }
            if (data->command.setRotationSpeed) {
                dac.analogWrite(data->rotationSpeed, DAC_ROTATION_SPEED);
            }
        }
        xSemaphoreGive(BU2560_Semaphore);
    }
}

/**
 * @param number Number of the input. Possible values are 1-10.
 */
bool IOCONTROL::getIn(byte number, bool invert) {
    switch (number) {
    case 1:
        return getIn1(invert);
        break;
    case 2:
        return getIn2(invert);
        break;
    case 3:
        return getIn3(invert);
        break;
    case 4:
        return getIn4(invert);
        break;
    case 5:
        return getIn5(invert);
        break;
    case 6:
        return getIn6(invert);
        break;
    case 7:
        return getIn7(invert);
        break;
    case 8:
        return getIn8(invert);
        break;
    case 9:
        return getIn9(invert);
        break;
    case 10:
        return getIn10(invert);
        break;
    default:
        return false;
        break;
    }
}

void IOCONTROL::readTemperatures() {
    tempSensors.requestTemperatures();
    for (uint8_t i = 0; i < tempSensors.getDS18Count(); i++) {
        // The order is turned around, so that the onboard sensor is always the first one
        uint8_t index = tempSensors.getDS18Count() - 1 - i;
        this->temperatures[index] = tempSensors.getTempCByIndex(i);
        dataToClient.temperatures[index] = this->temperatures[index];
        DPRINTLN("Temperature " + String(index) + ": " + String(this->temperatures[index]));
    }
}

int IOCONTROL::getTemperature(byte number) { return this->temperatures[number]; }

void IOCONTROL::startBlinkRJ45LED() { this->panelLED.startBlink(); }

void IOCONTROL::stopBlinkRJ45LED() { this->panelLED.stopBlink(); }

void IOCONTROL::updateBounceInputs() {
    const uint32_t currentMillis = millis();
    const size_t numInputs = sizeof(bounceInputs) / sizeof(BOUNCE_INPUT);

    for (byte i = 0; i < numInputs; ++i) {
        BOUNCE_INPUT &input = bounceInputs[i];

        if (currentMillis - input.lastRead > input.readInterval_MS) {
            input.lastRead = currentMillis;

            bool currentState = (input.ioport == 1 ? ioport1 : ioport2).stateOfPin(input.port);

            if (currentState != input.lastState) {
                input.lastState = currentState;
                input.lastChange = currentMillis;
            }

            if (currentMillis - input.lastChange > input.debounceInterval_MS) {
                if (currentState != input.state) {
                    input.state = currentState;
                    input.lastChange = currentMillis;

                    DPRINT("Input changed, IOPORT: " + String(input.ioport) + ", Port: " + String(input.port));
                    DPRINT(" (");
                    DPRINT(input.ioport == 1 ? IOPort1Mapping[input.port] : IOPort2Mapping[input.port]);
                    DPRINT("), State: ");
                    DPRINTLN(input.state);
                }
            }
        }
    }
}

void IOCONTROL::updateClientData() {
    dataToClient.autosquareRunning = stepperControl.autosquareRunning;
    dataToClient.alarmState = this->getAlarmAll();
    dataToClient.spindelState = this->getSpindelOnOff();
}

void IOCONTROL::enableControllerDirBuffer() {
    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 9) || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        DPRINTLN("Enable Controller dir buffer");
        digitalWrite(CONTROLLER_DIR_BUFFER_ENABLE_PIN, LOW);
    }
}
void IOCONTROL::disableControllerDirBuffer() {
    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 9) || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        DPRINTLN("Disable Controller dir buffer");
        digitalWrite(CONTROLLER_DIR_BUFFER_ENABLE_PIN, HIGH);
    }
}
void IOCONTROL::enableDACOutputs() {
    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 10) || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        digitalWrite(ESP32_DAC_ENABLE_PIN, HIGH);
    }
}
void IOCONTROL::disableDACOutputs() {
    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isHigherThan(2, 10) || versionManager.isBoardType(BOARD_TYPE::OCS2_Mini)) {
        digitalWrite(ESP32_DAC_ENABLE_PIN, LOW);
    }
}

IOCONTROL ioControl;