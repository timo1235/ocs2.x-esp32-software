#include <includes.h>

#include <configPMEM.h>

CONFIGMANAGER configManager;

WebServer server(80);
ConfigAssist conf(INI_FILE, VARIABLES_DEF_YAML);
std::map<std::string, BOARD_TYPE> boardTypeMap = {{"OCS2", BOARD_TYPE::OCS2}, {"OCS2_Mini", BOARD_TYPE::OCS2_Mini}, {"undefined", BOARD_TYPE::undefined}};
std::map<std::string, AXIS> axisMap = {{"X", AXIS::x}, {"Y", AXIS::y}, {"Z", AXIS::z}, {"A", AXIS::a}, {"B", AXIS::b}, {"C", AXIS::c}};

OCS2_CONFIG mainConfig = {{BOARD_TYPE::undefined, 0, 0},   // VersionInfo
                          false,                           // ESP_HANDWHEEL
                          false,                           // ESP_SET_ENA
                          false,                           // REVERSE_ENA_STATE
                          true,                            // OUTPUTS_INVERTED
                          0x5E,                            // CONTROLLER_MAC_ADDRESS
                          true,                            // OCS_DEBUG
                          false,                           // RESET_FEEDRATE_AND_ROTATION_SPEED_ON_CONNCTION_LOSS
                          false,                           // FLUIDNC_JOGGING
                          100,                             // MIN_JOGGING_SPEED
                          1500,                            // AUTOSQUARE_BUTTON_PRESS_TIME_MS
                          20000,                           // STEPPER_ACCELERATION
                          {
                              {false, AXIS::x, 1, true, AS_STATES::none, 10, AXIS::a, 2, true, AS_STATES::none, 10, 1600, 32, 20, false},   // 1.
                              {false, AXIS::y, 3, true, AS_STATES::none, 10, AXIS::b, 4, true, AS_STATES::none, 10, 1600, 32, 20, false},   // 2.
                              {false, AXIS::z, 5, true, AS_STATES::none, 10, AXIS::c, 6, true, AS_STATES::none, 10, 1600, 32, 20, false}    // 3.
                          },
                          true};   // ENABLE_WEB_INTERFACE

bool CONFIGMANAGER::startConfigHotspot = false;

uint16_t lastOutputUpdate = 0;

void CONFIGMANAGER::setup() {

    conf.setDisplayType(ConfigAssistDisplayType::AllOpen);
    conf.setRemotUpdateCallback([](String key) { Serial.println("Key: " + key + " changed"); });

    if (!conf.valid()) {
        DPRINTLN("ConfigManager: No valid config file found, using default values");
        conf.loadDict(VARIABLES_DEF_YAML, true);
        conf.saveConfigFile();
        delay(100);
    }

    mainConfig.versionInfo.boardType = boardTypeMap.find(conf["PCB_Type"].c_str())->second;
    mainConfig.versionInfo.major = conf["PCB_Version"].substring(0, conf["PCB_Version"].indexOf('.')).toInt();
    mainConfig.versionInfo.minor = conf["PCB_Version"].substring(conf["PCB_Version"].indexOf('.') + 1).toInt();
    mainConfig.enableWebInterface = conf["enable_WiFi"].toInt();
    mainConfig.controlHandWheelFunctions = conf["control_handwheel_functions"].toInt();
    mainConfig.controlDriverEnable = conf["control_driver_enable"].toInt();
    mainConfig.reverseEnableState = conf["reverse_driver_enable"].toInt();
    mainConfig.outputsInverted = conf["outputs_inverted"].toInt();
    mainConfig.macAddressCustomByte = conf["Wireless_ID"].toInt();
    mainConfig.debugSerialOutput = 1;   // unused at the moment
    mainConfig.resetFeedrateAndRotationOnTimeout = conf["reset_on_connection_loss"].toInt();
    mainConfig.fluidNCJogging = conf["fluidNC_jogging"].toInt();
    mainConfig.fluidNCMinJoggingSpeed = conf["jogging_min_speed"].toInt();
    mainConfig.autosquareButtonPressTime = conf["autosquare_button_time"].toInt();
    mainConfig.stepperAcceleration = conf["stepper_acceleration"].toInt();
    mainConfig.autosquareConfig[0].active = conf["axis_1_active"].toInt();
    mainConfig.autosquareConfig[0].motor1 = axisMap.find(conf["axis_1_motor_1"].c_str())->second;
    mainConfig.autosquareConfig[0].motor1EndstopInput = conf["axis_1_motor_1_endstop_input"].toInt();
    mainConfig.autosquareConfig[0].motor1EndstopInverted = conf["axis_1_motor_1_endstop_inverted"].toInt();
    mainConfig.autosquareConfig[0].motor1DriveBackDistance_mm = conf["axis_1_motor_1_back_distance"].toFloat();
    mainConfig.autosquareConfig[0].motor2 = axisMap.find(conf["axis_1_motor_2"].c_str())->second;
    mainConfig.autosquareConfig[0].motor2EndstopInput = conf["axis_1_motor_2_endstop_input"].toInt();
    mainConfig.autosquareConfig[0].motor2EndstopInverted = conf["axis_1_motor_2_endstop_inverted"].toInt();
    mainConfig.autosquareConfig[0].motor2DriveBackDistance_mm = conf["axis_1_motor_2_back_distance"].toFloat();
    mainConfig.autosquareConfig[0].stepsPerRevolution = conf["axis_1_steps_per_revolution"].toInt();
    mainConfig.autosquareConfig[0].mmPerRevolution = conf["axis_1_mm_per_revolution"].toInt();
    mainConfig.autosquareConfig[0].asSpeed_mm_s = conf["axis_1_as_speed"].toInt();
    mainConfig.autosquareConfig[0].reverseMotorDirection = conf["axis_1_reverse_motor_direction"].toInt();
    mainConfig.autosquareConfig[1].active = conf["axis_2_active"].toInt();
    mainConfig.autosquareConfig[1].motor1 = axisMap.find(conf["axis_2_motor_1"].c_str())->second;
    mainConfig.autosquareConfig[1].motor1EndstopInput = conf["axis_2_motor_1_endstop_input"].toInt();
    mainConfig.autosquareConfig[1].motor1EndstopInverted = conf["axis_2_motor_1_endstop_inverted"].toInt();
    mainConfig.autosquareConfig[1].motor1DriveBackDistance_mm = conf["axis_2_motor_1_back_distance"].toFloat();
    mainConfig.autosquareConfig[1].motor2 = axisMap.find(conf["axis_2_motor_2"].c_str())->second;
    mainConfig.autosquareConfig[1].motor2EndstopInput = conf["axis_2_motor_2_endstop_input"].toInt();
    mainConfig.autosquareConfig[1].motor2EndstopInverted = conf["axis_2_motor_2_endstop_inverted"].toInt();
    mainConfig.autosquareConfig[1].motor2DriveBackDistance_mm = conf["axis_2_motor_2_back_distance"].toFloat();
    mainConfig.autosquareConfig[1].stepsPerRevolution = conf["axis_2_steps_per_revolution"].toInt();
    mainConfig.autosquareConfig[1].mmPerRevolution = conf["axis_2_mm_per_revolution"].toInt();
    mainConfig.autosquareConfig[1].asSpeed_mm_s = conf["axis_2_as_speed"].toInt();
    mainConfig.autosquareConfig[1].reverseMotorDirection = conf["axis_2_reverse_motor_direction"].toInt();
    mainConfig.autosquareConfig[2].active = conf["axis_3_active"].toInt();
    mainConfig.autosquareConfig[2].motor1 = axisMap.find(conf["axis_3_motor_1"].c_str())->second;
    mainConfig.autosquareConfig[2].motor1EndstopInput = conf["axis_3_motor_1_endstop_input"].toInt();
    mainConfig.autosquareConfig[2].motor1EndstopInverted = conf["axis_3_motor_1_endstop_inverted"].toInt();
    mainConfig.autosquareConfig[2].motor1DriveBackDistance_mm = conf["axis_3_motor_1_back_distance"].toFloat();
    mainConfig.autosquareConfig[2].motor2 = axisMap.find(conf["axis_3_motor_2"].c_str())->second;
    mainConfig.autosquareConfig[2].motor2EndstopInput = conf["axis_3_motor_2_endstop_input"].toInt();
    mainConfig.autosquareConfig[2].motor2EndstopInverted = conf["axis_3_motor_2_endstop_inverted"].toInt();
    mainConfig.autosquareConfig[2].motor2DriveBackDistance_mm = conf["axis_3_motor_2_back_distance"].toFloat();
    mainConfig.autosquareConfig[2].stepsPerRevolution = conf["axis_3_steps_per_revolution"].toInt();
    mainConfig.autosquareConfig[2].mmPerRevolution = conf["axis_3_mm_per_revolution"].toInt();
    mainConfig.autosquareConfig[2].asSpeed_mm_s = conf["axis_3_as_speed"].toInt();
    mainConfig.autosquareConfig[2].reverseMotorDirection = conf["axis_3_reverse_motor_direction"].toInt();

    printMainConfig();
    if (!mainConfig.enableWebInterface) {
        return;
    }
    this->startWiFi();
    delay(100);
}

void CONFIGMANAGER::loop() {
    if (GLOBAL.configManagerWiFiInitialized) {
        server.handleClient();
        delay(2);
    }
}

void CONFIGMANAGER::startWiFi() {
    xTaskCreatePinnedToCore(
        [](void *parameter) {
        CONFIGMANAGER *configManager = static_cast<CONFIGMANAGER *>(parameter);
        for (;;) {
            if (!versionManager.isBoardType(BOARD_TYPE::undefined)) {
                if (GLOBAL.IOControlInitialized == false || GLOBAL.protocolInitialized == false) {
                    DPRINTLN("ConfigManager Wifi Setup: waiting for IOControl & Protocol to be initialized");
                }
                while (GLOBAL.IOControlInitialized == false || GLOBAL.protocolInitialized == false) {
                    vTaskDelay(pdMS_TO_TICKS(1000));   // Wait for 1 second
                }
            }

            if (WiFi.getMode() != WIFI_AP_STA) {
                WiFi.mode(WIFI_AP_STA);
            }

#if defined(WIFI_SSID) && defined(WIFI_PASS)
            configManager->setupWiFiConnect();
#else
            configManager->setupWiFiAP();
#endif
            server.on("/", [configManager]() {
                server.sendHeader("Location", "/cfg", true);
                server.send(302, "text/plain", "");
            });
            server.on("/d", []() {   // Append dump handler
                conf.dump(&server);
            });
            server.onNotFound([configManager]() { configManager->handleNotFound(); });
            conf.setup(server, false);
            server.begin();
            DPRINTLN("ConfigManager: HTTP server started");

            vTaskDelay(100);
            GLOBAL.configManagerWiFiInitialized = true;

            vTaskDelete(NULL);   // Delete the task after setup
        }
    },
        "ConfigManager Wifi setup", 4096, this, DEFAULT_TASK_PRIORITY, NULL, DEFAULT_TASK_CPU);
}

void CONFIGMANAGER::setupWiFiAP() {
    WiFi.setHostname("OCS2");
    Serial.println("Creating hotspot");
    delay(100);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8) {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    char ssid[30];
    snprintf(ssid, 31, "OCS2-%06X", chipId);
    WiFi.softAP(ssid, NULL, 1);

    DPRINTLN("Use your phone or computer to connect to the Wi-Fi network " + String(ssid));
    DPRINTLN("Then open http://" + WiFi.softAPIP().toString() + " in your browser");
    DPRINTLN("Adjust the configuration to your needs and reboot the ESP32 afterwards.");
    DPRINTLN("----------------------------------------");
}

void CONFIGMANAGER::setupWiFiConnect() {
    WiFi.begin("asdf", "1234567890");
    Serial.println("\nConnecting");

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }

    if (MDNS.begin("esp32")) {
        Serial.println("MDNS responder started");
    }

    Serial.println("\nConnected to the WiFi network");
    Serial.print("Local ESP32 IP: ");
    Serial.println(WiFi.localIP());
}

// Handler function for Home page
void CONFIGMANAGER::handleRoot() {

    String out("<h2>Hello and welcome to configure your OPEN-CNC-Shield 2</h2>");
    out += "<a href='/cfg'>Edit config</a>";

    server.send(200, "text/html", out);
}

// Handler for page not found
void CONFIGMANAGER::handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void CONFIGMANAGER::printMainConfig() {
    String boardTypes[] = {"OCS2", "OCS2_Mini", "undefinded"};
    String axisMapping[] = {"X", "Y", "Z", "A", "B", "C"};
    DPRINTLN("OCS2 configuration: ");
    DPRINTLN("Board Type: \t\t\t" + boardTypes[mainConfig.versionInfo.boardType]);
    DPRINTLN("Version: \t\t\t" + String(mainConfig.versionInfo.major) + "." + String(mainConfig.versionInfo.minor));
    DPRINTLN("Enable Web Interface: \t\t" + String(mainConfig.enableWebInterface));
    DPRINTLN("Control Handwheel Functions: \t" + String(mainConfig.controlHandWheelFunctions));
    DPRINTLN("Control Driver Enable: \t\t" + String(mainConfig.controlDriverEnable));
    DPRINTLN("Reverse Enable State: \t\t" + String(mainConfig.reverseEnableState));
    DPRINTLN("Outputs Inverted: \t\t" + String(mainConfig.outputsInverted));
    DPRINTLN("MAC Address Custom Byte: \t" + String(mainConfig.macAddressCustomByte));
    DPRINTLN("Debug Serial Output: \t\t" + String(mainConfig.debugSerialOutput));
    DPRINTLN("Reset on Connection Loss: \t" + String(mainConfig.resetFeedrateAndRotationOnTimeout));
    DPRINTLN("FluidNC Jogging: \t\t" + String(mainConfig.fluidNCJogging));
    DPRINTLN("Min Jogging Speed: \t\t" + String(mainConfig.fluidNCMinJoggingSpeed));
    DPRINTLN("Autosquare Button Press Time: \t" + String(mainConfig.autosquareButtonPressTime));
    DPRINTLN("Stepper Acceleration: \t\t" + String(mainConfig.stepperAcceleration));
    DPRINTLN("------------------------------------");
    DPRINTLN("Axis 1 Configuration: ");
    DPRINTLN("Active: \t\t\t" + String(mainConfig.autosquareConfig[0].active));
    DPRINTLN("Motor 1: \t\t\t" + axisMapping[mainConfig.autosquareConfig[0].motor1]);
    DPRINTLN("Motor 1 Endstop Input: \t\t" + String(mainConfig.autosquareConfig[0].motor1EndstopInput));
    DPRINTLN("Motor 1 Endstop Inverted: \t" + String(mainConfig.autosquareConfig[0].motor1EndstopInverted));
    DPRINTLN("Motor 1 Drive Back Distance: \t" + String(mainConfig.autosquareConfig[0].motor1DriveBackDistance_mm));
    DPRINTLN("Motor 2: \t\t\t" + axisMapping[mainConfig.autosquareConfig[0].motor2]);
    DPRINTLN("Motor 2 Endstop Input: \t\t" + String(mainConfig.autosquareConfig[0].motor2EndstopInput));
    DPRINTLN("Motor 2 Endstop Inverted: \t" + String(mainConfig.autosquareConfig[0].motor2EndstopInverted));
    DPRINTLN("Motor 2 Drive Back Distance: \t" + String(mainConfig.autosquareConfig[0].motor2DriveBackDistance_mm));
    DPRINTLN("Steps per Revolution: \t\t" + String(mainConfig.autosquareConfig[0].stepsPerRevolution));
    DPRINTLN("mm per Revolution: \t\t" + String(mainConfig.autosquareConfig[0].mmPerRevolution));
    DPRINTLN("AS Speed: \t\t\t" + String(mainConfig.autosquareConfig[0].asSpeed_mm_s));
    DPRINTLN("Reverse Motor Direction: \t" + String(mainConfig.autosquareConfig[0].reverseMotorDirection));
    DPRINTLN("------------------------------------");
    DPRINTLN("Axis 2 Configuration: ");
    DPRINTLN("Active: \t\t\t" + String(mainConfig.autosquareConfig[1].active));
    DPRINTLN("Motor 1: \t\t\t" + axisMapping[mainConfig.autosquareConfig[1].motor1]);
    DPRINTLN("Motor 1 Endstop Input: \t\t" + String(mainConfig.autosquareConfig[1].motor1EndstopInput));
    DPRINTLN("Motor 1 Endstop Inverted: \t" + String(mainConfig.autosquareConfig[1].motor1EndstopInverted));
    DPRINTLN("Motor 1 Drive Back Distance: \t" + String(mainConfig.autosquareConfig[1].motor1DriveBackDistance_mm));
    DPRINTLN("Motor 2: \t\t\t" + axisMapping[mainConfig.autosquareConfig[1].motor2]);
    DPRINTLN("Motor 2 Endstop Input: \t\t" + String(mainConfig.autosquareConfig[1].motor2EndstopInput));
    DPRINTLN("Motor 2 Endstop Inverted: \t" + String(mainConfig.autosquareConfig[1].motor2EndstopInverted));
    DPRINTLN("Motor 2 Drive Back Distance: \t" + String(mainConfig.autosquareConfig[1].motor2DriveBackDistance_mm));
    DPRINTLN("Steps per Revolution: \t\t" + String(mainConfig.autosquareConfig[1].stepsPerRevolution));
    DPRINTLN("mm per Revolution: \t\t" + String(mainConfig.autosquareConfig[1].mmPerRevolution));
    DPRINTLN("AS Speed: \t\t\t" + String(mainConfig.autosquareConfig[1].asSpeed_mm_s));
    DPRINTLN("Reverse Motor Direction: \t" + String(mainConfig.autosquareConfig[1].reverseMotorDirection));
    DPRINTLN("------------------------------------");
    DPRINTLN("Axis 3 Configuration: ");
    DPRINTLN("Active: \t\t\t" + String(mainConfig.autosquareConfig[2].active));
    DPRINTLN("Motor 1: \t\t\t" + axisMapping[mainConfig.autosquareConfig[2].motor1]);
    DPRINTLN("Motor 1 Endstop Input: \t\t" + String(mainConfig.autosquareConfig[2].motor1EndstopInput));
    DPRINTLN("Motor 1 Endstop Inverted: \t" + String(mainConfig.autosquareConfig[2].motor1EndstopInverted));
    DPRINTLN("Motor 1 Drive Back Distance: \t" + String(mainConfig.autosquareConfig[2].motor1DriveBackDistance_mm));
    DPRINTLN("Motor 2: \t\t\t" + axisMapping[mainConfig.autosquareConfig[2].motor2]);
    DPRINTLN("Motor 2 Endstop Input: \t\t" + String(mainConfig.autosquareConfig[2].motor2EndstopInput));
    DPRINTLN("Motor 2 Endstop Inverted: \t" + String(mainConfig.autosquareConfig[2].motor2EndstopInverted));
    DPRINTLN("Motor 2 Drive Back Distance: \t" + String(mainConfig.autosquareConfig[2].motor2DriveBackDistance_mm));
    DPRINTLN("Steps per Revolution: \t\t" + String(mainConfig.autosquareConfig[2].stepsPerRevolution));
    DPRINTLN("mm per Revolution: \t\t" + String(mainConfig.autosquareConfig[2].mmPerRevolution));
    DPRINTLN("AS Speed: \t\t\t" + String(mainConfig.autosquareConfig[2].asSpeed_mm_s));
    DPRINTLN("Reverse Motor Direction: \t" + String(mainConfig.autosquareConfig[2].reverseMotorDirection));
}
