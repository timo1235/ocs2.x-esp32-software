#include <includes.h>

HardwareSerial SerialGRBL(1);

int nextIntervalWaitTime = 10;
unsigned long lastCommandTime = 0;
unsigned long lastSettingsCheckTimeMS = 0;

GRBL_JOGGING::GRBL_JOGGING()
{
}

void GRBL_JOGGING::setup()
{
    xSettings = {0, 0, 0};
    ySettings = {0, 0, 0};
    zSettings = {0, 0, 0};

    SerialGRBL.begin(115200, SERIAL_8N1, FLUIDNC_RX_PIN, FLUIDNC_TX_PIN);

#ifdef USE_GRBL_JOGGING
    xTaskCreatePinnedToCore(
        GRBL_JOGGING::loopTask,
        "GRBL JOGGING Task",
        10000,
        this,
        1,
        NULL,
        1);
#endif
}

void GRBL_JOGGING::loopTask(void *pvParameters)
{
    auto *grblJogging = (GRBL_JOGGING *)pvParameters;
    for (;;)
    {
        // Do not send jogging commands if axis settings are not read
        if (grblJogging->axisSettingsRead)
        {
            grblJogging->checkAndSendJoggingCommands();
        }

        // Debugging - print out all messages from GRBL
        // if (SerialGRBL.available() > 0)
        // {
        //     String line = SerialGRBL.readStringUntil('\n');
        //     Serial.println(line);
        // }

        // Try to read axis settings every 2 seconds
        if (!grblJogging->axisSettingsRead && millis() - lastSettingsCheckTimeMS > 2000)
        {
            grblJogging->checkAndReadAxisSettings();
            lastSettingsCheckTimeMS = millis();
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void GRBL_JOGGING::checkAndSendJoggingCommands()
{
    unsigned long currentTime = millis();

    // Array to hold commands for X, Y, and Z axes
    AxisCommand commands[3];
    bool isJoystickCentered = true;

    // Iterate through each axis to check for movement
    for (int axis = 0; axis < 3; axis++)
    {
        int joystickPosition = axis == 0 ? dataToControl.joystickX : axis == 1 ? dataToControl.joystickY
                                                                               : dataToControl.joystickZ;
        int deviation = abs(joystickPosition - joystickCenter);
        int direction = joystickPosition > joystickCenter ? 1 : -1;

        // Check if joystick moved beyond tolerance
        if (deviation > tolerance)
        {
            isJoystickCentered = false;
            float speed = calculateSpeed(axis, joystickPosition);         // Speed in mm/second
            float timeIntervalInSeconds = nextIntervalWaitTime / 1000.0f; // Convert ms to seconds
            float distance = speed * timeIntervalInSeconds;               // Distance covered in the interval

            // If not jogging, multiply the distance by 5, otherwise grbl starts already deaccelerating before the next command
            if (!isJogging)
            {
                distance = distance * 5;
            }

            commands[axis].speed = speed;
            commands[axis].distance = distance * direction / 60; // Apply direction to the distance
            commands[axis].move = true;
        }
    }

    // Send jog commands if joystick is not centered and the interval has passed
    if (!isJoystickCentered && currentTime - lastCommandTime >= nextIntervalWaitTime)
    {
        if (isJogging)
        {
            nextIntervalWaitTime = 150;
        }
        sendJoggingCommand(commands);
        lastCommandTime = currentTime;
    }

    // Always check for the jog cancel command, allowing for an immediate stop
    if (isJoystickCentered)
    {
        sendJogCancelCommand(); // Sends the jog cancel command if the joystick is centered
    }
}

int GRBL_JOGGING::calculateSpeed(int axis, int joystickPosition)
{
    int maxSpeedForAxis;
    switch (axis)
    {
    case 0:
        maxSpeedForAxis = xSettings.maxRate;
        break;
    case 1:
        maxSpeedForAxis = ySettings.maxRate;
        break;
    case 2:
        maxSpeedForAxis = zSettings.maxRate;
        break;
    default:
        maxSpeedForAxis = 0;
        break;
    }

    // Calculate based on the deviation from the center
    int deviation = abs(joystickPosition - joystickCenter) - tolerance;
    return map(deviation, 0, joystickCenter - tolerance, MIN_JOGGING_SPEED, maxSpeedForAxis);
}

void GRBL_JOGGING::sendJoggingCommand(AxisCommand commands[3])
{
    String command = "$J=G91 G21";

    for (int i = 0; i < 3; i++)
    {
        if (commands[i].move)
        {
            command += axisNames[i];
            command += commands[i].distance;
        }
    }

    // Find max speed for axes
    int maxSpeed = 0;
    for (int i = 0; i < 3; i++)
    {
        if (commands[i].move && abs(commands[i].speed) > maxSpeed)
        {
            maxSpeed = abs(commands[i].speed);
        }
    }

    if (maxSpeed > 0)
    {
        command += "F";
        command += String(maxSpeed);
    }
    else
    {
        // Without speed, we do not need to send a command
        return;
    }

    isJogging = true;
    SerialGRBL.println(command);
    SerialGRBL.flush();
    // Serial.println(command);

    // Wait for "ok" acknowledgment
    waitForAcknowledgment();
}

void GRBL_JOGGING::waitForAcknowledgment()
{
    unsigned long startTime = millis();
    const unsigned long timeout = 1000; // Timeout after 1000 milliseconds (1 second)
    String response = "";

    while (millis() - startTime < timeout)
    {
        if (SerialGRBL.available())
        {
            char c = SerialGRBL.read();
            response += c;
            if (response.endsWith("\n"))
            { // End of line character, process the response
                if (response.indexOf("ok") != -1)
                {
                    return; // "ok" received, exit the function
                }
                response = ""; // Reset response for the next line
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS); // Yield to other tasks
    }
    Serial.println("Timeout waiting for ok"); // Debugging
}

void GRBL_JOGGING::checkBuffer()
{
    SerialGRBL.write("?");
    String line;
    while (SerialGRBL.available() > 0)
    {
        line = SerialGRBL.readStringUntil('\n');
        // Serial.println(line);
    }
}

void GRBL_JOGGING::sendJogCancelCommand()
{
    isJogging = false;
    nextIntervalWaitTime = 50;
    SerialGRBL.write(0x85);
    SerialGRBL.flush();
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void GRBL_JOGGING::checkAndReadAxisSettings()
{
    getAxisInformationFromFluidNC();

    if (xSettings.maxRate > 0 && ySettings.maxRate > 0 && zSettings.maxRate > 0)
    {
        axisSettingsRead = true;
    }
    else
    {
        Serial.println("Failed to read axis settings from FluidNC, trying again in some seconds.");
        return;
    }

    // Print axis Settings
    Serial.println("FluidNC Axis Settings:");
    Serial.print("X: ");
    Serial.print(xSettings.maxRate);
    Serial.print(" ");
    Serial.print(xSettings.resolution);
    Serial.print(" ");
    Serial.println(xSettings.acceleration);
    Serial.print("Y: ");
    Serial.print(ySettings.maxRate);
    Serial.print(" ");
    Serial.print(ySettings.resolution);
    Serial.print(" ");
    Serial.println(ySettings.acceleration);
    Serial.print("Z: ");
    Serial.print(zSettings.maxRate);
    Serial.print(" ");
    Serial.print(zSettings.resolution);
    Serial.print(" ");
    Serial.println(zSettings.acceleration);
}

void GRBL_JOGGING::getAxisInformationFromFluidNC()
{
    unsigned long startTime = millis();
    String line;

    // Read all settings
    SerialGRBL.println("$S");
    SerialGRBL.flush();

    while (millis() - startTime < 500)
    {
        while (SerialGRBL.available() > 0)
        {
            line = SerialGRBL.readStringUntil('\n');
            parseFluidNCSettingLine(line);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void GRBL_JOGGING::parseFluidNCSettingLine(String line)
{
    // Parsen von MaxRate
    if (line.startsWith("$Grbl/MaxRate/X="))
    {
        xSettings.maxRate = line.substring(16).toFloat();
    }
    else if (line.startsWith("$Grbl/MaxRate/Y="))
    {
        ySettings.maxRate = line.substring(16).toFloat();
    }
    else if (line.startsWith("$Grbl/MaxRate/Z="))
    {
        zSettings.maxRate = line.substring(16).toFloat();
    }

    // Parsen von Resolution
    else if (line.startsWith("$Grbl/Resolution/X="))
    {
        xSettings.resolution = line.substring(19).toFloat();
    }
    else if (line.startsWith("$Grbl/Resolution/Y="))
    {
        ySettings.resolution = line.substring(19).toFloat();
    }
    else if (line.startsWith("$Grbl/Resolution/Z="))
    {
        zSettings.resolution = line.substring(19).toFloat();
    }

    // Parsen von Acceleration
    else if (line.startsWith("$Grbl/Acceleration/X="))
    {
        xSettings.acceleration = line.substring(21).toFloat();
    }
    else if (line.startsWith("$Grbl/Acceleration/Y="))
    {
        ySettings.acceleration = line.substring(21).toFloat();
    }
    else if (line.startsWith("$Grbl/Acceleration/Z="))
    {
        zSettings.acceleration = line.substring(21).toFloat();
    }
}

GRBL_JOGGING grblJogging;