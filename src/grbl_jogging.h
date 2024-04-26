#pragma once

class GRBL_JOGGING {
    struct AxisSettings {
        float maxRate;
        float resolution;
        float acceleration;
    };

    struct AxisCommand {
        float speed;      // Geschwindigkeit für die Achse
        float distance;   // Distanz für die Bewegung
        bool  move;       // Flag, ob die Achse bewegt werden soll

        AxisCommand() : speed(0), distance(0), move(false) {}   // Standardkonstruktor
    };

    const int joystickCenter = 512;
    const int tolerance      = 100;   // ca. 20% of 1023/2
    char      axisNames[3]   = {'X', 'Y', 'Z'};

  public:
    AxisSettings xSettings, ySettings, zSettings;

    GRBL_JOGGING();
    void setup();

    void sendJoggingCommand(AxisCommand commands[3]);
    void checkAndSendJoggingCommands();
    void sendJogCancelCommand();

  private:
    bool        axisSettingsRead = false;
    bool        isJogging        = false;
    static void loopTask(void *pvParameters);
    void        checkAndReadAxisSettings();
    float       calculateDistance(int joystickPosition);
    int         calculateSpeed(int axis, int joystickPosition);
    void        getAxisInformationFromFluidNC();
    void        parseFluidNCSettingLine(String line);
    void        checkBuffer();
    void        waitForAcknowledgment();
};