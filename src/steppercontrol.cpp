#include <includes.h>

STEPPERCONTROL::STEPPERCONTROL(){};

void STEPPERCONTROL::setup() {
    if (versionManager.isBoardType(BOARD_TYPE::undefined)) {
        DPRINTLN("StepperControl: Board type is undefined. Functionalities are disabled.");
        return;
    }
    // Stop if autosquare is not active
    if (!mainConfig.autosquareConfig[0].active && !mainConfig.autosquareConfig[1].active && !mainConfig.autosquareConfig[2].active) {
        DPRINTLN("StepperControl: Autosquare is not active for any axis - skipping stepper setup");
        return;
    }
    xTaskCreatePinnedToCore(
        [](void *parameter) {
        STEPPERCONTROL *stepperControl = static_cast<STEPPERCONTROL *>(parameter);
        for (;;) {
            if (GLOBAL.IOControlInitialized == false) {
                DPRINTLN("StepperControl: waiting for IOControl to be initialized");
            }
            while (GLOBAL.IOControlInitialized == false) {
                vTaskDelay(pdMS_TO_TICKS(1000));   // Wait for 1 second
            }
            stepperControl->stepperEngine.init();
            stepperControl->steppers[AXIS::x] = stepperControl->stepperEngine.stepperConnectToPin(STEP_X_PIN);
            stepperControl->steppers[AXIS::y] = stepperControl->stepperEngine.stepperConnectToPin(STEP_Y_PIN);
            stepperControl->steppers[AXIS::z] = stepperControl->stepperEngine.stepperConnectToPin(STEP_Z_PIN);
            stepperControl->steppers[AXIS::a] = stepperControl->stepperEngine.stepperConnectToPin(STEP_A_PIN);
            stepperControl->steppers[AXIS::b] = stepperControl->stepperEngine.stepperConnectToPin(STEP_B_PIN);
            if (versionManager.isBoardType(BOARD_TYPE::OCS2)) {
                stepperControl->steppers[AXIS::c] = stepperControl->stepperEngine.stepperConnectToPin(STEP_C_PIN);
            }

            // Adding the axes according to the configuration
            stepperControl->addAxis(mainConfig.autosquareConfig[0]);
            stepperControl->addAxis(mainConfig.autosquareConfig[1]);
            stepperControl->addAxis(mainConfig.autosquareConfig[2]);

            // Create a task for the stepper control
            xTaskCreatePinnedToCore(STEPPERCONTROL::stepperTaskHandler, /* Task function. */
                                    "Stepper task",                     /* name of task. */
                                    4096,                               /* Stack size of task */
                                    stepperControl,                     /* parameter of the task */
                                    DEFAULT_TASK_PRIORITY,              /* priority of the task */
                                    NULL,                               /* Task handle to keep track of created task */
                                    TASK_STEPPERCONTROL_CPU);
            DPRINTLN("StepperControl: initialized");
            vTaskDelete(NULL);   // Delete the task after setup
        }
    },
        "StepperControl setup", 2048, this, DEFAULT_TASK_PRIORITY, NULL, DEFAULT_TASK_CPU);
};

void STEPPERCONTROL::stepperTaskHandler(void *pvParameters) {
    auto *stepperControl = (STEPPERCONTROL *) pvParameters;
    for (;;) {

        if (stepperControl->getAutosquareButtonState()) {
            if (!stepperControl->ASButtonPressed) {
                DPRINTLN("StepperControl: Autosquare button first pressed - Wait the configured delay before starting");
                stepperControl->ASButtonPressed = true;
                stepperControl->timeASButtonPressed = millis();
            } else if (millis() - stepperControl->timeASButtonPressed > mainConfig.autosquareButtonPressTime) {
                stepperControl->autosquareProcess();
            }
        } else {
            stepperControl->ASButtonPressed = false;
        }

        vTaskDelay(CHECK_AS_BUTTON_DELAY_MS);
    }
}

bool STEPPERCONTROL::getAutosquareButtonState() {
    // Read hardware pin for autosquare button - skip debouncing if autosquare is running
    bool directAutosquareButtonState = this->autosquareRunning ? ioControl.getAutosquare(true) : ioControl.getAutosquare();
    // Read autosquare button state from wifi
    bool wifiAutosquareButtonState = dataToControl.autosquare;

    return wifiAutosquareButtonState || directAutosquareButtonState;
}

void STEPPERCONTROL::addAxis(AUTOSQUARE_CONFIG config) {
    if (config.active == false) {
        return;
    }
    // stop if its the ocs2 mini and axis is c - since it does not have a c axis
    if (versionManager.isBoardType(BOARD_TYPE::OCS2_Mini) && (config.motor1 == AXIS::c || config.motor2 == AXIS::c)) {
        DPRINTLN("StepperControl: Skipping one of the autosquare axis configurations - OCS2 Mini does not have a C axis");
        return;
    }
    // Finding the first inactive configuration slot
    byte index = 0;
    while (index < 3 && autosquareConfigs[index].active) {
        index++;
    }

    autosquareConfigs[index] = config;               // Direct assignment if structure is assignable
    autosquareConfigs[index].motor1ASState = none;   // Assuming 'none' is some default state
    autosquareConfigs[index].motor2ASState = none;

    this->initializeStepper(index);
};

void STEPPERCONTROL::initializeStepper(byte configIndex) {
    // steps per mm
    int stepsPerMM = autosquareConfigs[configIndex].stepsPerRevolution / autosquareConfigs[configIndex].mmPerRevolution;
    int stepsPerSecond = autosquareConfigs[configIndex].asSpeed_mm_s * stepsPerMM;

    steppers[autosquareConfigs[configIndex].motor1]->setSpeedInHz(stepsPerSecond);
    steppers[autosquareConfigs[configIndex].motor2]->setSpeedInHz(stepsPerSecond);
    steppers[autosquareConfigs[configIndex].motor1]->setAcceleration(mainConfig.stepperAcceleration);
    steppers[autosquareConfigs[configIndex].motor2]->setAcceleration(mainConfig.stepperAcceleration);
}

void STEPPERCONTROL::autosquareProcess() {
    String axisMapping[] = {"X", "Y", "Z", "A", "B", "C"};
    DPRINTLN("Autosquare: start");
    this->autosquareRunning = true;
    // printAutosquareConfig();
    initializeAutosquare();

    // DPRINTLN("Autosquare: loop");
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
        // Continue if config is not active
        if (!autosquareConfigs[i].active) {
            continue;
        }
        // Continue loop if steppers finished
        if (autosquareConfigs[i].motor1ASState == AS_STATES::finish && autosquareConfigs[i].motor2ASState == AS_STATES::finish) {
            continue;
        }
        // Continue loop if steppers are squared
        if (autosquareConfigs[i].motor1ASState == AS_STATES::squared && autosquareConfigs[i].motor2ASState == AS_STATES::squared) {
            continue;
        }
        steppers[autosquareConfigs[i].motor1]->runForward();
        steppers[autosquareConfigs[i].motor2]->runForward();

        DPRINTLN("Autosquare: Config: " + String(i));
        DPRINTLN("Autosquare: Motor1 started - axis: " + String(axisMapping[autosquareConfigs[i].motor1]));
        DPRINTLN("Autosquare: Motor2 started - axis: " + String(axisMapping[autosquareConfigs[i].motor2]));
    }
    // Check endstops
    bool allFinished = false;
    while (!allFinished) {
        allFinished = true;
        // Abort if the button is no longer pressed
        if (!this->getAutosquareButtonState()) {
            terminateAutosquare();
            return;
        }
        for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
            // Continue if config is not active
            if (!autosquareConfigs[i].active) {
                continue;
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor1ASState != AS_STATES::squared) {
                if (ioControl.getIn(autosquareConfigs[i].motor1EndstopInput, autosquareConfigs[i].motor1EndstopInverted)) {
                    DPRINTLN("Autosquare: Squared motor1 - axis: " + String(axisMapping[autosquareConfigs[i].motor1]));
                    autosquareConfigs[i].motor1ASState = AS_STATES::squared;
                    dataToClient.autosquareState[i].axisMotor1State = autosquareConfigs[i].motor1ASState;
                    steppers[autosquareConfigs[i].motor1]->forceStopAndNewPosition(0);
                } else {
                    allFinished = false;
                }
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor2ASState != AS_STATES::squared) {
                if (ioControl.getIn(autosquareConfigs[i].motor2EndstopInput, autosquareConfigs[i].motor2EndstopInverted)) {
                    DPRINTLN("Autosquare: Squared motor2 - axis: " + String(axisMapping[autosquareConfigs[i].motor2]));
                    autosquareConfigs[i].motor2ASState = AS_STATES::squared;
                    dataToClient.autosquareState[i].axisMotor2State = autosquareConfigs[i].motor2ASState;
                    steppers[autosquareConfigs[i].motor2]->forceStopAndNewPosition(0);
                } else {
                    allFinished = false;
                }
            }
        }
    }
    DPRINTLN("Autosquare: All configured Axes squared - Start driving back from endstops");
    // Drive down from the endstops
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
        // Continue if config is not active
        if (!autosquareConfigs[i].active) {
            continue;
        }
        // Continue loop if steppers finished or off
        if (autosquareConfigs[i].motor1ASState == AS_STATES::finish && autosquareConfigs[i].motor2ASState == AS_STATES::finish) {
            continue;
        }
        setDirectionByAxisLabel(autosquareConfigs[i].motor1, autosquareConfigs[i].reverseMotorDirection ? HIGH : LOW);
        setDirectionByAxisLabel(autosquareConfigs[i].motor2, autosquareConfigs[i].reverseMotorDirection ? HIGH : LOW);
        steppers[autosquareConfigs[i].motor1]->move(
            autosquareConfigs[i].motor1DriveBackDistance_mm * autosquareConfigs[i].stepsPerRevolution / autosquareConfigs[i].mmPerRevolution, false);
        steppers[autosquareConfigs[i].motor2]->move(
            autosquareConfigs[i].motor2DriveBackDistance_mm * autosquareConfigs[i].stepsPerRevolution / autosquareConfigs[i].mmPerRevolution, false);

        DPRINTLN("Autosquare: Config: " + String(i));
        DPRINTLN("Autosquare: Motor1 start drive from endstop - axis: " + String(axisMapping[autosquareConfigs[i].motor1]) +
                 " - distance: " + String(autosquareConfigs[i].motor1DriveBackDistance_mm));
        DPRINTLN("Autosquare: Motor2 start drive from endstop - axis: " + String(axisMapping[autosquareConfigs[i].motor2]) +
                 " - distance: " + String(autosquareConfigs[i].motor2DriveBackDistance_mm));
    }

    allFinished = false;
    while (!allFinished) {
        allFinished = true;
        // Abort if the button is no longer pressed
        if (!this->getAutosquareButtonState()) {
            terminateAutosquare();
            return;
        }
        for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
            // Continue if config is not active
            if (!autosquareConfigs[i].active) {
                continue;
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor1ASState != AS_STATES::finish) {
                if (!steppers[autosquareConfigs[i].motor1]->isRunning()) {
                    DPRINTLN("Autosquare: Finished motor1 - axis: " + String(axisMapping[autosquareConfigs[i].motor1]));
                    autosquareConfigs[i].motor1ASState = AS_STATES::finish;
                    dataToClient.autosquareState[i].axisMotor1State = autosquareConfigs[i].motor1ASState;
                } else {
                    allFinished = false;
                }
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor2ASState != AS_STATES::finish) {
                if (!steppers[autosquareConfigs[i].motor2]->isRunning()) {
                    DPRINTLN("Autosquare: Finished motor2 - axis: " + String(axisMapping[autosquareConfigs[i].motor2]));
                    autosquareConfigs[i].motor2ASState = AS_STATES::finish;
                    dataToClient.autosquareState[i].axisMotor2State = autosquareConfigs[i].motor2ASState;
                } else {
                    allFinished = false;
                }
            }
        }
    }

    terminateAutosquare();
};

void STEPPERCONTROL::initializeAutosquare() {
    DPRINTLN("Autosquare: initialize");
    // Prevent the ControllerModule from using the DIR pins of the motors
    ioControl.disableControllerDirBuffer();
    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isVersion(2, 12)) {
        // Init direction pins as outputs
        ioControl.initDirPins();
    }

    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
        // Update date for the wifi clients
        dataToClient.autosquareState[i].axisActive = autosquareConfigs[i].active;
        dataToClient.autosquareState[i].axisMotor1State = 0;
        dataToClient.autosquareState[i].axisMotor2State = 0;
        // Continue if config is not active
        if (!autosquareConfigs[i].active) {
            continue;
        }
        // Reset axis
        autosquareConfigs[i].motor1ASState = none;
        autosquareConfigs[i].motor2ASState = none;

        setDirectionByAxisLabel(autosquareConfigs[i].motor1, autosquareConfigs[i].reverseMotorDirection ? LOW : HIGH);
        setDirectionByAxisLabel(autosquareConfigs[i].motor2, autosquareConfigs[i].reverseMotorDirection ? LOW : HIGH);
    }
};

void STEPPERCONTROL::terminateAutosquare() {
    DPRINTLN("Autosquare: terminate");
    // Force stop all steppers
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
        // Continue if config is not active
        if (!autosquareConfigs[i].active) {
            continue;
        }

        steppers[autosquareConfigs[i].motor1]->forceStopAndNewPosition(0);
        steppers[autosquareConfigs[i].motor2]->forceStopAndNewPosition(0);
    }

    // Reset the direction outputs
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
        setDirectionByAxisLabel(autosquareConfigs[i].motor1, LOW);
        setDirectionByAxisLabel(autosquareConfigs[i].motor2, LOW);
    }

    if (versionManager.isBoardType(BOARD_TYPE::OCS2) && versionManager.isVersion(2, 12)) {
        // Set direction pins to input to prevent interference with the controller module
        ioControl.freeDirPins();
    }
    // Enable the ControllerModule to use the DIR pins of the motors again
    ioControl.enableControllerDirBuffer();

    // Force wait until autosquare button is released
    DPRINTLN("Autosquare: Waiting for button release");
    while (this->getAutosquareButtonState()) {
        delay(100);
    }
    DPRINTLN("Autosquare: Button released - Autosquare finished");
    // Reset button state
    this->ASButtonPressed = false;
    this->autosquareRunning = false;
}

void STEPPERCONTROL::setDirectionByAxisLabel(AXIS axis, bool direction) {
    switch (axis) {
    case AXIS::x:
        ioControl.setDirX(direction);
        break;
    case AXIS::y:
        ioControl.setDirY(direction);
        break;
    case AXIS::z:
        ioControl.setDirZ(direction);
        break;
    case AXIS::a:
        ioControl.setDirA(direction);
        break;
    case AXIS::b:
        ioControl.setDirB(direction);
        break;
    case AXIS::c:
        ioControl.setDirC(direction);
        break;
    default:
        break;
    }
};

void STEPPERCONTROL::printAutosquareConfig() {
    DPRINTLN("----------------------------------------------------");
    DPRINTLN("Autosquare: Printing configuration for active autosquare axis");
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++) {
        if (!autosquareConfigs[i].active) {
            continue;
        }
        DPRINT("Motor1: ");
        DPRINT(autosquareConfigs[i].motor1);
        DPRINT("\tMotor1EndstopInput: ");
        DPRINT(autosquareConfigs[i].motor1EndstopInput);
        DPRINT("\tMotor1ASState: ");
        DPRINT(autosquareConfigs[i].motor1ASState);
        DPRINT("\tMotor1DriveBackDistance: ");
        DPRINT(autosquareConfigs[i].motor1DriveBackDistance_mm);
        DPRINT("\tMotor2: ");
        DPRINT(autosquareConfigs[i].motor2);
        DPRINT("\tMotor2EndstopInput: ");
        DPRINT(autosquareConfigs[i].motor2EndstopInput);
        DPRINT("\tMotor2ASState: ");
        DPRINT(autosquareConfigs[i].motor2ASState);
        DPRINT("\tMotor2DriveBackDistance: ");
        DPRINT(autosquareConfigs[i].motor2DriveBackDistance_mm);
        DPRINT("\tStepsPerRevolution: ");
        DPRINT(autosquareConfigs[i].stepsPerRevolution);
        DPRINT("\tMmPerRevolution: ");
        DPRINT(autosquareConfigs[i].mmPerRevolution);
        DPRINT("\tAsSpeed_mm_s: ");
        DPRINT(autosquareConfigs[i].asSpeed_mm_s);
        DPRINT("\tReverseMotorDirection: ");
        DPRINTLN(autosquareConfigs[i].reverseMotorDirection);
    }
    DPRINTLN("----------------------------------------------------");
}

STEPPERCONTROL stepperControl;