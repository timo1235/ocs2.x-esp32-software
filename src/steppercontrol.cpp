#include <includes.h>

STEPPERCONTROL::STEPPERCONTROL(){};
extern DATA_TO_CONTROL dataToControl;

void STEPPERCONTROL::setup()
{
    stepperEngine.init();
    steppers[AXIS::x] = stepperEngine.stepperConnectToPin(STEP_X_PIN);
    steppers[AXIS::y] = stepperEngine.stepperConnectToPin(STEP_Y_PIN);
    steppers[AXIS::z] = stepperEngine.stepperConnectToPin(STEP_Z_PIN);
    steppers[AXIS::a] = stepperEngine.stepperConnectToPin(STEP_A_PIN);
    steppers[AXIS::b] = stepperEngine.stepperConnectToPin(STEP_B_PIN);
    steppers[AXIS::c] = stepperEngine.stepperConnectToPin(STEP_C_PIN);
    // Create a task for the stepper control
    xTaskCreatePinnedToCore(
        STEPPERCONTROL::stepperTaskHandler, /* Task function. */
        "Stepper task",                     /* name of task. */
        10000,                              /* Stack size of task */
        this,                               /* parameter of the task */
        tskIDLE_PRIORITY,                   /* priority of the task */
        &stepperTask,                       /* Task handle to keep track of created task */
        0);
};

void STEPPERCONTROL::stepperTaskHandler(void *pvParameters)
{
    auto *stepperControl = (STEPPERCONTROL *)pvParameters;
    for (;;)
    {
        if (millis() - stepperControl->lastASButtonCheck_MS > CHECK_AS_BUTTON_DELAY_MS)
        {
            stepperControl->lastASButtonCheck_MS = millis();

            if (stepperControl->getAutosquareButtonState())
            {
                if (!stepperControl->ASButtonPressed)
                {
                    DPRINTLN("Autosquare button first pressed - Wait the configured delay before starting");
                    stepperControl->ASButtonPressed = true;
                    stepperControl->timeASButtonPressed = millis();
                }
                else if (millis() - stepperControl->timeASButtonPressed > AUTOSQUARE_BUTTON_PRESS_TIME_MS)
                {
                    stepperControl->autosquareProcess();
                }
            }
            else
            {
                stepperControl->ASButtonPressed = false;
            }
        }
        vTaskDelay(1);
    }
}

bool STEPPERCONTROL::getAutosquareButtonState()
{
    // Read hardware pin for autosquare button - skip debouncing if autosquare is running
    bool directAutosquareButtonState = this->autosquareRunning ? ioControl.getAutosquare(true) : ioControl.getAutosquare();
    // Read autosquare button state from wifi
    bool wifiAutosquareButtonState = dataToControl.autosquare;

    return wifiAutosquareButtonState || directAutosquareButtonState;
}

void STEPPERCONTROL::addAxis(
    AXIS motor1,
    byte motor1EndstopInput,
    bool motor1EndstopInverted,
    float motor1DriveBackDistance_mm,
    AXIS motor2,
    byte motor2EndstopInput,
    bool motor2EndstopInverted,
    float motor2DriveBackDistance_mm,
    uint16_t stepsPerRevolution,
    uint16_t mmPerRevolution,
    uint16_t asSpeed_mm_s,
    bool reverseMotorDirection)
{
    byte index = 0;
    if (autosquareConfigs[index].active)
    {
        index = 1;
    }
    if (autosquareConfigs[index].active)
    {
        index = 2;
    }

    autosquareConfigs[index].active = true;
    autosquareConfigs[index].motor1 = motor1;
    autosquareConfigs[index].motor1EndstopInput = motor1EndstopInput;
    autosquareConfigs[index].motor1EndstopInverted = motor1EndstopInverted;
    autosquareConfigs[index].motor1ASState = none;
    autosquareConfigs[index].motor1DriveBackDistance_mm = motor1DriveBackDistance_mm;
    autosquareConfigs[index].motor2 = motor2;
    autosquareConfigs[index].motor2EndstopInput = motor2EndstopInput;
    autosquareConfigs[index].motor2EndstopInverted = motor2EndstopInverted;
    autosquareConfigs[index].motor2ASState = none;
    autosquareConfigs[index].motor2DriveBackDistance_mm = motor2DriveBackDistance_mm;
    autosquareConfigs[index].stepsPerRevolution = stepsPerRevolution;
    autosquareConfigs[index].mmPerRevolution = mmPerRevolution;
    autosquareConfigs[index].asSpeed_mm_s = asSpeed_mm_s;
    autosquareConfigs[index].reverseMotorDirection = reverseMotorDirection;

    this->initializeStepper(index);
};

void STEPPERCONTROL::initializeStepper(byte configIndex)
{
    // steps per mm
    int stepsPerMM = autosquareConfigs[configIndex].stepsPerRevolution / autosquareConfigs[configIndex].mmPerRevolution;
    int stepsPerSecond = autosquareConfigs[configIndex].asSpeed_mm_s * stepsPerMM;

    steppers[autosquareConfigs[configIndex].motor1]->setSpeedInHz(stepsPerSecond); 
    steppers[autosquareConfigs[configIndex].motor2]->setSpeedInHz(stepsPerSecond); 
    steppers[autosquareConfigs[configIndex].motor1]->setAcceleration(STEPPER_ACCELERATION);
    steppers[autosquareConfigs[configIndex].motor2]->setAcceleration(STEPPER_ACCELERATION);
}

void STEPPERCONTROL::autosquareProcess()
{
    DPRINTLN("Autosquare: start");
    this->autosquareRunning = true;
    printAutosquareConfig();
    initializeAutosquare();

    // DPRINTLN("Autosquare: loop");
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
    {
        // Continue if config is not active
        if (!autosquareConfigs[i].active)
        {
            continue;
        }
        // Continue loop if steppers finished
        if (autosquareConfigs[i].motor1ASState == AS_STATES::finish && autosquareConfigs[i].motor2ASState == AS_STATES::finish)
        {
            continue;
        }
        // Continue loop if steppers are squared
        if (autosquareConfigs[i].motor1ASState == AS_STATES::squared && autosquareConfigs[i].motor2ASState == AS_STATES::squared)
        {
            continue;
        }
        steppers[autosquareConfigs[i].motor1]->runForward();
        steppers[autosquareConfigs[i].motor2]->runForward();

        DPRINT("Autosquare: Config: ");
        DPRINTLN(i);
        DPRINT("Autosquare: Motor1 started - axis: ");
        DPRINTLN(autosquareConfigs[i].motor1);
        DPRINT("Autosquare: Motor2 started - axis: ");
        DPRINTLN(autosquareConfigs[i].motor2);
    }
    // Check endstops
    bool allFinished = false;
    while (!allFinished)
    {
        allFinished = true;
        // Abort if the button is no longer pressed
        if (!this->getAutosquareButtonState())
        {
            break;
        }
        for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
        {
            // Continue if config is not active
            if (!autosquareConfigs[i].active)
            {
                continue;
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor1ASState != AS_STATES::squared)
            {
                if (ioControl.getIn(autosquareConfigs[i].motor1EndstopInput, autosquareConfigs[i].motor1EndstopInverted))
                {
                    DPRINT("Autosquare: Squared motor1 - axis: ");
                    DPRINTLN(autosquareConfigs[i].motor1);
                    autosquareConfigs[i].motor1ASState = AS_STATES::squared;
                    dataToClient.autosquareState[i].axisMotor1State = autosquareConfigs[i].motor1ASState;
                    steppers[autosquareConfigs[i].motor1]->forceStopAndNewPosition(0);
                }
                else
                {
                    allFinished = false;
                }
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor2ASState != AS_STATES::squared)
            {
                if (ioControl.getIn(autosquareConfigs[i].motor2EndstopInput, autosquareConfigs[i].motor2EndstopInverted))
                {
                    DPRINT("Autosquare: Squared motor2 - axis: ");
                    DPRINTLN(autosquareConfigs[i].motor2);
                    autosquareConfigs[i].motor2ASState = AS_STATES::squared;
                    dataToClient.autosquareState[i].axisMotor2State = autosquareConfigs[i].motor2ASState;
                    steppers[autosquareConfigs[i].motor2]->forceStopAndNewPosition(0);
                }
                else
                {
                    allFinished = false;
                }
            }
        }
    }
    // Drive down from the endstops
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
    {
        // Continue if config is not active
        if (!autosquareConfigs[i].active)
        {
            continue;
        }
        // Continue loop if steppers finished or off
        if (autosquareConfigs[i].motor1ASState == AS_STATES::finish && autosquareConfigs[i].motor2ASState == AS_STATES::finish)
        {
            continue;
        }
        setDirectionByAxisLabel(autosquareConfigs[i].motor1, autosquareConfigs[i].reverseMotorDirection ? HIGH : LOW);
        setDirectionByAxisLabel(autosquareConfigs[i].motor2, autosquareConfigs[i].reverseMotorDirection ? HIGH : LOW);
        steppers[autosquareConfigs[i].motor1]->move(autosquareConfigs[i].motor1DriveBackDistance_mm * autosquareConfigs[i].stepsPerRevolution / autosquareConfigs[i].mmPerRevolution, false);
        steppers[autosquareConfigs[i].motor2]->move(autosquareConfigs[i].motor2DriveBackDistance_mm * autosquareConfigs[i].stepsPerRevolution / autosquareConfigs[i].mmPerRevolution, false);

        DPRINT("Autosquare: Config: ");
        DPRINTLN(i);
        DPRINTLN("Autosquare: Motor1 start drive from endstop - axis: " +  String(autosquareConfigs[i].motor1) + " - distance: " +  String(autosquareConfigs[i].motor1DriveBackDistance_mm));
        DPRINTLN("Autosquare: Motor2 start drive from endstop - axis: " +  String(autosquareConfigs[i].motor2) + " - distance: " +  String(autosquareConfigs[i].motor2DriveBackDistance_mm));
    }

    allFinished = false;
    while (!allFinished)
    {
        allFinished = true;
        // Abort if the button is no longer pressed
        if (!this->getAutosquareButtonState())
        {
            break;
        }
        for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
        {
            // Continue if config is not active
            if (!autosquareConfigs[i].active)
            {
                continue;
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor1ASState != AS_STATES::finish)
            {
                if (!steppers[autosquareConfigs[i].motor1]->isRunning())
                {
                    DPRINT("Autosquare: Finished motor1 - axis: ");
                    DPRINTLN(autosquareConfigs[i].motor1);
                    autosquareConfigs[i].motor1ASState = AS_STATES::finish;
                    dataToClient.autosquareState[i].axisMotor1State = autosquareConfigs[i].motor1ASState;
                }
                else
                {
                    allFinished = false;
                }
            }
            // Check endstop for axis/motor
            if (autosquareConfigs[i].motor2ASState != AS_STATES::finish)
            {
                if (!steppers[autosquareConfigs[i].motor2]->isRunning())
                {
                    DPRINT("Autosquare: Finished motor2 - axis: ");
                    DPRINTLN(autosquareConfigs[i].motor2);
                    autosquareConfigs[i].motor2ASState = AS_STATES::finish;
                    dataToClient.autosquareState[i].axisMotor2State = autosquareConfigs[i].motor2ASState;
                }
                else
                {
                    allFinished = false;
                }
            }
        }
    }

    terminateAutosquare();
};

void STEPPERCONTROL::initializeAutosquare()
{
    DPRINTLN("Autosquare: initialize");

    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
    {
        // Update date for the wifi clients
        dataToClient.autosquareState[i].axisActive = autosquareConfigs[i].active;
        dataToClient.autosquareState[i].axisMotor1State = 0;
        dataToClient.autosquareState[i].axisMotor2State = 0;
        // Continue if config is not active
        if (!autosquareConfigs[i].active)
        {
            continue;
        }
        // Reset axis
        autosquareConfigs[i].motor1ASState = none;
        autosquareConfigs[i].motor2ASState = none;

        setDirectionByAxisLabel(autosquareConfigs[i].motor1, autosquareConfigs[i].reverseMotorDirection ? LOW : HIGH);
        setDirectionByAxisLabel(autosquareConfigs[i].motor2, autosquareConfigs[i].reverseMotorDirection ? LOW : HIGH);
    }
};

void STEPPERCONTROL::terminateAutosquare()
{
    DPRINTLN("Autosquare: terminate");
    // Force stop all steppers
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
    {
        // Continue if config is not active
        if (!autosquareConfigs[i].active)
        {
            continue;
        }

        steppers[autosquareConfigs[i].motor1]->forceStopAndNewPosition(0);
        steppers[autosquareConfigs[i].motor2]->forceStopAndNewPosition(0);
    }

    // Reset the direction outputs
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
    {
        setDirectionByAxisLabel(autosquareConfigs[i].motor1, LOW);
        setDirectionByAxisLabel(autosquareConfigs[i].motor2, LOW);
    }
    // Force wait until autosquare button is released
    while (this->getAutosquareButtonState())
    {   
        DPRINTLN("Autosquare: Waiting for button release");
        delay(100);
    }
    // Reset button state
    this->ASButtonPressed = false;
    this->autosquareRunning = false;
}

void STEPPERCONTROL::setDirectionByAxisLabel(AXIS axis, bool direction)
{
    switch (axis)
    {
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

void STEPPERCONTROL::printAutosquareConfig()
{
    DPRINTLN("----------------------------------------------------");
    DPRINTLN("Autosquare: Printing configuration for active autosquare axis");
    for (byte i = 0; i < sizeof(autosquareConfigs) / sizeof(AUTOSQUARE_CONFIG); i++)
    {
        if(!autosquareConfigs[i].active) {
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