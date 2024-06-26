#include <includes.h>

DATA_TO_CONTROL dataToControl = {.joystickX = DAC_JOYSTICK_RESET_VALUE,
                                 .joystickY = DAC_JOYSTICK_RESET_VALUE,
                                 .joystickZ = DAC_JOYSTICK_RESET_VALUE,
                                 .feedrate = 0,
                                 .rotationSpeed = 0,
                                 .command = {.setJoystick = true, .setFeedrate = true, .setRotationSpeed = true}};
DATA_TO_CLIENT dataToClient = {2, 0, 0, 0, 0, 0, 0};

byte PROTOCOL::clientCount = 0;
CLIENT_DATA PROTOCOL::clients[5];
CLIENT_DATA PROTOCOL::currentControls = {0};

CLIENT_DATA PROTOCOL::serialClient;

bool PROTOCOL::serialConnected;
uint32_t PROTOCOL::lastSerialPackageReceived;
uint16_t PROTOCOL::serialConnectionTimeout_MS = 1000;
HardwareSerial SerialPort(2);
SerialTransfer myTransfer;

void PROTOCOL::setup() {
    if (versionManager.isBoardType(BOARD_TYPE::undefined)) {
        DPRINTLN("Protocol: Board type is undefined. Functionalities are disabled.");
        return;
    }
    xTaskCreatePinnedToCore([](void *parameter) {
        PROTOCOL *protocol = static_cast<PROTOCOL *>(parameter);
        for (;;) {
            if (GLOBAL.IOControlInitialized == false) {
                DPRINTLN("Protocol: waiting for IOControl to be initialized");
            }
            while (GLOBAL.IOControlInitialized == false) {
                vTaskDelay(1000);   // Wait for 1 second
            }
            protocol->setupESPNOW();   // Call setupESPNOW of this instance
            protocol->setupSerial();   // Call setupSerial of this instance
            GLOBAL.protocolInitialized = true;
            DPRINTLN("Protocol: initialized");
            vTaskDelete(NULL);   // Delete the task after setup
        }
    }, "Protocol setup", 4096, this, DEFAULT_TASK_PRIORITY, NULL, DEFAULT_TASK_CPU);
}

void PROTOCOL::setupSerial() {
    SerialPort.begin(115200, SERIAL_8N1, ESP32_PANEL_RX_PIN, ESP32_PANEL_TX_PIN);
    myTransfer.begin(SerialPort);
    PROTOCOL::serialClient.isSerialClient = true;

    // Create a task for the protocol
    xTaskCreatePinnedToCore(PROTOCOL::serialTaskHandler, /* Task function. */
                            "Protocol serial task",      /* name of task. */
                            4096,                        /* Stack size of task */
                            this,                        /* parameter of the task */
                            DEFAULT_TASK_PRIORITY,       /* priority of the task */
                            NULL,                        /* Task handle to keep track of created task */
                            TASK_PROTOCOL_CPU);
}

void PROTOCOL::serialTaskHandler(void *pvParameters) {
    auto *protocol = (PROTOCOL *) pvParameters;
    for (;;) {
        int bytesAvailable = myTransfer.available();
        if (bytesAvailable) {
            DATA_TO_CONTROL tempDataToControl = {};
            myTransfer.rxObj(tempDataToControl);

            if (PROTOCOL::serialConnected == false) {
                PROTOCOL::serialConnected = true;
                DPRINTLN("Serial: Handwheel connection established. WiFi signals may be ignored, if they try to control the same ouptuts like the serial handwheel. Serial "
                         "handwheel has priority.");
            }
            PROTOCOL::lastSerialPackageReceived = millis();
            PROTOCOL::updateClientData(&PROTOCOL::serialClient, &tempDataToControl, true);
            PROTOCOL::setDataAccordingToCommand(&PROTOCOL::serialClient, &tempDataToControl);

            PROTOCOL::serialConnectionTimeout_MS = dataToControl.command.updateIntervalSerial_MS;

            myTransfer.sendDatum(dataToClient);

            // ioControl.writeDataBag(&dataToControl);
            ioControl.startBlinkRJ45LED();
        }

        if (PROTOCOL::isSerialConnected() && millis() - PROTOCOL::lastSerialPackageReceived > PROTOCOL::serialConnectionTimeout_MS * 4) {
            PROTOCOL::serialConnected = false;
            DPRINTLN("Serial: Handwheel connection timed out.");
            PROTOCOL::resetOutputsControlledByClient(&PROTOCOL::serialClient);
            ioControl.stopBlinkRJ45LED();
        }
        vTaskDelay(10);
    }
}

void PROTOCOL::setupESPNOW() {
    if (WiFi.getMode() != WIFI_AP_STA) {
        WiFi.mode(WIFI_AP_STA);
    }

    DPRINT("My Mac Address: ");
    DPRINTLN(WiFi.macAddress());
    esp_wifi_set_ps(WIFI_PS_NONE);
    uint8_t newMACAddress[] = {0x5E, 0x0, 0x0, 0x0, 0x0, 0x1};
    newMACAddress[4] = mainConfig.macAddressCustomByte;
    auto result = esp_wifi_set_mac(WIFI_IF_STA, newMACAddress);
    DPRINT("New Mac Address: ");
    DPRINTLN(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        DPRINTLN("Error initializing ESP-NOW. Things wont work");
        return;
    }

    esp_now_register_recv_cb(PROTOCOL::onDataRecv);
    esp_now_register_send_cb(PROTOCOL::onDataSent);

    // Create a task for the protocol
    xTaskCreatePinnedToCore(PROTOCOL::protocolTaskHandler, /* Task function. */
                            "Protocol task",               /* name of task. */
                            4096,                          /* Stack size of task */
                            this,                          /* parameter of the task */
                            DEFAULT_TASK_PRIORITY,         /* priority of the task */
                            NULL,                          /* Task handle to keep track of created task */
                            TASK_PROTOCOL_CPU);
}

void PROTOCOL::protocolTaskHandler(void *pvParameters) {
    auto *protocol = (PROTOCOL *) pvParameters;
    for (;;) {
        if (millis() - protocol->lastTimeoutCheck > WIFI_TIMEOUT_CHECK_INTERVAL_MS) {
            bool anyClientConnected = false;
            protocol->lastTimeoutCheck = millis();
            for (byte i = 0; i < PROTOCOL::clientCount; i++) {
                if (!PROTOCOL::clients[i].active || PROTOCOL::clients[i].ignored) {
                    continue;
                }
                if (millis() - PROTOCOL::clients[i].lastSeen > PROTOCOL::clients[i].updateInterval_MS * 4) {
                    DPRINT("WiFi: Peer: ");
                    DPRINT(i);
                    DPRINT(" with Mac-Address: ");
                    DPRINT(PROTOCOL::getMacStrFromAddress(PROTOCOL::clients[i].macAddress));
                    DPRINTLN(" timed out");
                    PROTOCOL::clients[i].active = 0;
                    PROTOCOL::resetOutputsControlledByClient(&PROTOCOL::clients[i]);
                } else {
                    anyClientConnected = true;
                }
            }
            // Stop blinking the led if there is no client connected
            if (!anyClientConnected && PROTOCOL::isSerialConnected() == false) {
                ioControl.stopBlinkRJ45LED();
            }
        }
        vTaskDelay(10);
    }
}

bool PROTOCOL::addPeerIfNotExists(uint8_t *address) {
    // Return if the peer already exists
    if (esp_now_is_peer_exist(address)) {
        return false;
    }

    DPRINT("Wifi: Adding peer with MAC: ");
    DPRINT(PROTOCOL::getMacStrFromAddress(address));
    DPRINTLN(", intAddress: " + String(PROTOCOL::getIntegerFromAddress(address)) + " to the peer list");

    PROTOCOL::clients[PROTOCOL::clientCount].integerAddress = PROTOCOL::getIntegerFromAddress(address);
    memcpy(PROTOCOL::clients[PROTOCOL::clientCount].macAddress, address, 6);
    PROTOCOL::clients[PROTOCOL::clientCount].lastSeen = millis();
    PROTOCOL::clientCount++;

    // register peer
    esp_now_peer_info_t peerInfo = {};
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, address, 6);

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        DPRINT("Wifi: Failed to add peer: ");
        DPRINTLN(PROTOCOL::getMacStrFromAddress(address));
        return false;
    }
    return true;
}

esp_err_t PROTOCOL::sendMessageToClient(uint8_t *address, DATA_TO_CLIENT *data) { return esp_now_send(address, (uint8_t *) data, sizeof(DATA_TO_CLIENT)); }

// callback function that will be executed when data is received
void PROTOCOL::onDataRecv(const uint8_t *address, const uint8_t *incomingData, int len) {
    // Add the peer to the list of known peers
    bool isNew = PROTOCOL::addPeerIfNotExists((uint8_t *) address);

    uint16_t intAddress = PROTOCOL::getIntegerFromAddress(address);
    // Search for the client in the list of known clients
    for (byte i = 0; i < sizeof(PROTOCOL::clients) / sizeof(CLIENT_DATA); i++) {
        if (PROTOCOL::clients[i].integerAddress == intAddress) {
            PROTOCOL::clients[i].lastSeen = millis();
            // Only use the data if the client is not ignored
            if (!PROTOCOL::clients[i].ignored) {
                DATA_TO_CONTROL tempDataToControl = {};
                memcpy(&tempDataToControl, incomingData, sizeof(dataToControl));
                PROTOCOL::setDataAccordingToCommand(&PROTOCOL::clients[i], &tempDataToControl);
                PROTOCOL::clients[i].active = 1;
            } else {
                // Peer is ignored
            }
            PROTOCOL::updateClientData(&PROTOCOL::clients[i], &dataToControl, isNew);
        }
    }

    ioControl.startBlinkRJ45LED();
    if (dataToControl.command.returnData) {
        PROTOCOL::sendMessageToClient((uint8_t *) address, &dataToClient);
    }
}

bool PROTOCOL::isSerialConnected() { return PROTOCOL::serialConnected; }

// This is where the incoming data is saved to the dataToControl variable
void PROTOCOL::setDataAccordingToCommand(CLIENT_DATA *client, DATA_TO_CONTROL *incomingDataToControl) {
    if (client->setJoystick && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setJoystick)) {
        dataToControl.joystickX = incomingDataToControl->joystickX;
        dataToControl.joystickY = incomingDataToControl->joystickY;
        dataToControl.joystickZ = incomingDataToControl->joystickZ;
    }
    if (client->setFeedrate && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setFeedrate)) {
        dataToControl.feedrate = incomingDataToControl->feedrate;
    }
    if (client->setRotationSpeed && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setRotationSpeed)) {
        dataToControl.rotationSpeed = incomingDataToControl->rotationSpeed;
    }
    if (client->setAutosquare && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setAutosquare)) {
        dataToControl.autosquare = incomingDataToControl->autosquare;
    }
    if (client->setEna && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setEna)) {
        dataToControl.ena = incomingDataToControl->ena;
    }
    if (client->setAxisSelect && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setAxisSelect)) {
        dataToControl.selectAxisX = incomingDataToControl->selectAxisX;
        dataToControl.selectAxisY = incomingDataToControl->selectAxisY;
        dataToControl.selectAxisZ = incomingDataToControl->selectAxisZ;
    }
    if (client->setOk && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setOk)) {
        dataToControl.ok = incomingDataToControl->ok;
    }
    if (client->setProgrammStart && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setProgrammStart)) {
        dataToControl.programmStart = incomingDataToControl->programmStart;
    }
    if (client->setMotorStart && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setMotorStart)) {
        dataToControl.motorStart = incomingDataToControl->motorStart;
    }
    if (client->setSpeed1 && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setSpeed1)) {
        dataToControl.speed1 = incomingDataToControl->speed1;
    }
    if (client->setSpeed2 && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setSpeed2)) {
        dataToControl.speed2 = incomingDataToControl->speed2;
    }
    if (client->setOutput1 && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setOutput1)) {
        dataToControl.output1 = incomingDataToControl->output1;
    }
    if (client->setOutput2 && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setOutput2)) {
        dataToControl.output2 = incomingDataToControl->output2;
    }
    if (client->setOutput3 && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setOutput3)) {
        dataToControl.output3 = incomingDataToControl->output3;
    }
    if (client->setOutput4 && (client->isSerialClient || !PROTOCOL::isSerialConnected() || !PROTOCOL::serialClient.setOutput4)) {
        dataToControl.output4 = incomingDataToControl->output4;
    }
    dataToControl.command = incomingDataToControl->command;
    dataToControl.softwareVersion = incomingDataToControl->softwareVersion;
}

// This is called when a client timed out. All outputs controlled by the client are reset to default
void PROTOCOL::resetOutputsControlledByClient(CLIENT_DATA *client) {
    if (client->isSerialClient) {
        DPRINTLN("Serial: Resetting outputs controlled by serial client.");
    } else {
        DPRINT("Wifi: Resetting outputs controlled by peer with address: ");
        DPRINTLN(PROTOCOL::getMacStrFromAddress(client->macAddress));
    }

    if (client->setEna) {
        ioControl.setENA(0);
    }
    if (mainConfig.controlHandWheelFunctions) {

        if (client->setJoystick) {
            ioControl.resetJoySticksToDefaults();
        }
        if (client->setFeedrate && mainConfig.resetFeedrateAndRotationOnTimeout) {
            ioControl.setFeedrate(0);
        }
        if (client->setRotationSpeed && mainConfig.resetFeedrateAndRotationOnTimeout) {
            ioControl.setRotationSpeed(0);
        }
        if (client->setAutosquare) {
            dataToControl.autosquare = 0;
        }
        if (client->setAxisSelect) {
            ioControl.setAuswahlX(0);
            ioControl.setAuswahlY(0);
            ioControl.setAuswahlZ(0);
        }
        if (client->setOk) {
            ioControl.setOK(0);
        }
        if (client->setProgrammStart) {
            ioControl.setProgrammStart(0);
        }
        if (client->setMotorStart) {
            ioControl.setMotorStart(0);
        }
        if (client->setSpeed1) {
            ioControl.setSpeed1(0);
        }
        if (client->setSpeed2) {
            ioControl.setSpeed2(0);
        }
        if (client->setOutput1) {
            ioControl.setOut1(0);
        }
        if (client->setOutput2) {
            ioControl.setOut2(0);
        }
        if (client->setOutput3) {
            ioControl.setOut3(0);
        }
        if (client->setOutput4) {
            ioControl.setOut4(0);
        }
    }
}

void PROTOCOL::sendIgnoreMessageToClient(CLIENT_DATA *client, bool silent) {
    client->active = 0;
    client->ignored = true;
    DATA_TO_CLIENT data = {};
    memcpy(&data, &dataToClient, sizeof(dataToClient));
    data.peerIgnored = true;
    esp_err_t success = PROTOCOL::sendMessageToClient(client->macAddress, &data);
    if (!silent) {
        DPRINTLN("Wifi: Client " + String(PROTOCOL::getMacStrFromAddress(client->macAddress)) +
                 " tried to control an output that is already controlled by another client. Aborting and ignoring the client in future requests.");
        if (success != ESP_OK) {
            DPRINTLN("Wifi: Error: Could not inform client about the problem");
        } else {
            DPRINTLN("Wifi: Successfully informed client about the problem");
        }
    }
}

bool PROTOCOL::validateClientCommand(CLIENT_DATA *client, DATA_TO_CONTROL *data, bool isNewClient) {
    // Update the currentControls map and see if there are problems
    // If the client is already ignored, just send him the stop message
    bool shouldBeIgnored = false;
    if (client->ignored) {
        PROTOCOL::sendIgnoreMessageToClient(client, true);
        return false;
    }

    // Skip check if the client is already known
    if (!isNewClient) {
        return true;
    }

    if (data->command.setJoystick) {
        if (!PROTOCOL::currentControls.setJoystick) {
            PROTOCOL::currentControls.setJoystick = true;
        } else {
            DPRINTLN("Wifi: Error: Joystick is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setFeedrate) {
        if (!PROTOCOL::currentControls.setFeedrate) {
            PROTOCOL::currentControls.setFeedrate = true;
        } else {
            DPRINTLN("Wifi: Error: Feedrate is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setRotationSpeed) {
        if (!PROTOCOL::currentControls.setRotationSpeed) {
            PROTOCOL::currentControls.setRotationSpeed = true;
        } else {
            DPRINTLN("Wifi: Error: Rotation speed is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setAutosquare) {
        if (!PROTOCOL::currentControls.setAutosquare) {
            PROTOCOL::currentControls.setAutosquare = true;
        } else {
            DPRINTLN("Wifi: Error: Autosquare is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setEna) {
        if (!PROTOCOL::currentControls.setEna) {
            PROTOCOL::currentControls.setEna = true;
        } else {
            DPRINTLN("Wifi: Error: ENA is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setAxisSelect) {
        if (!PROTOCOL::currentControls.setAxisSelect) {
            PROTOCOL::currentControls.setAxisSelect = true;
        } else {
            DPRINTLN("Wifi: Error: Axis select is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setOk) {
        if (!PROTOCOL::currentControls.setOk) {
            PROTOCOL::currentControls.setOk = true;
        } else {
            DPRINTLN("Wifi: Error: OK is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setProgrammStart) {
        if (!PROTOCOL::currentControls.setProgrammStart) {
            PROTOCOL::currentControls.setProgrammStart = true;
        } else {
            DPRINTLN("Wifi: Error: Programm start is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setMotorStart) {
        if (!PROTOCOL::currentControls.setMotorStart) {
            PROTOCOL::currentControls.setMotorStart = true;
        } else {
            DPRINTLN("Wifi: Error: Motor start is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setSpeed1) {
        if (!PROTOCOL::currentControls.setSpeed1) {
            PROTOCOL::currentControls.setSpeed1 = true;
        } else {
            DPRINTLN("Wifi: Error: Speed 1 is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setSpeed2) {
        if (!PROTOCOL::currentControls.setSpeed2) {
            PROTOCOL::currentControls.setSpeed2 = true;
        } else {
            DPRINTLN("Wifi: Error: Speed 2 is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setOutput1) {
        if (!PROTOCOL::currentControls.setOutput1) {
            PROTOCOL::currentControls.setOutput1 = true;
        } else {
            DPRINTLN("Wifi: Error: Output1 is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setOutput2) {
        if (!PROTOCOL::currentControls.setOutput2) {
            PROTOCOL::currentControls.setOutput2 = true;
        } else {
            DPRINTLN("Wifi: Error: Output2 is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setOutput3) {
        if (!PROTOCOL::currentControls.setOutput3) {
            PROTOCOL::currentControls.setOutput3 = true;
        } else {
            DPRINTLN("Wifi: Error: Output3 is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (data->command.setOutput4) {
        if (!PROTOCOL::currentControls.setOutput4) {
            PROTOCOL::currentControls.setOutput4 = true;
        } else {
            DPRINTLN("Wifi: Error: Output4 is already controlled by another client");
            shouldBeIgnored = true;
        }
    }
    if (shouldBeIgnored) {
        PROTOCOL::sendIgnoreMessageToClient(client, false);
        return false;
    }
    return true;
}

/**
 * This function updates the client data according to the data received from the client
 * @param client The client to update
 * @param data The data received from the client
 * @param isNewClient True if the client is new
 */
void PROTOCOL::updateClientData(CLIENT_DATA *client, DATA_TO_CONTROL *data, bool isNewClient) {
    client->setJoystick = data->command.setJoystick;
    client->setFeedrate = data->command.setFeedrate;
    client->setRotationSpeed = data->command.setRotationSpeed;
    client->setAutosquare = data->command.setAutosquare;
    client->setEna = data->command.setEna;
    client->setAxisSelect = data->command.setAxisSelect;
    client->setOk = data->command.setOk;
    client->setProgrammStart = data->command.setProgrammStart;
    client->setMotorStart = data->command.setMotorStart;
    client->setSpeed1 = data->command.setSpeed1;
    client->setSpeed2 = data->command.setSpeed2;
    client->updateInterval_MS = data->command.updateInterval_MS;
    client->setOutput1 = data->command.setOutput1;
    client->setOutput2 = data->command.setOutput2;
    client->setOutput3 = data->command.setOutput3;
    client->setOutput4 = data->command.setOutput4;
    if (!client->isSerialClient) {
        PROTOCOL::validateClientCommand(client, data, isNewClient);
    }
}

void PROTOCOL::onDataSent(const uint8_t *address, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
    } else {
    }
}

char *PROTOCOL::getMacStrFromAddress(uint8_t *address) {
    static char macStr[18];
    // Copies the sender mac address to a string
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", address[0], address[1], address[2], address[3], address[4], address[5]);
    return macStr;
}

uint16_t PROTOCOL::getIntegerFromAddress(const uint8_t *address) {
    uint16_t integer = 0;
    integer += address[0];
    integer += address[1];
    integer += address[2];
    integer += address[3];
    integer += address[4];
    integer += address[5];
    return integer;
}

void PROTOCOL::dumpDataToControl() {
    DPRINTLN("--- Data to control ---");
    DPRINT("Joystick X: \t\t");
    DPRINTLN(dataToControl.joystickX);
    DPRINT("Joystick Y: \t\t");
    DPRINTLN(dataToControl.joystickY);
    DPRINT("Joystick Z: \t\t");
    DPRINTLN(dataToControl.joystickZ);
    DPRINT("Feedrate: \t\t");
    DPRINTLN(dataToControl.feedrate);
    DPRINT("rotation speed: \t");
    DPRINTLN(dataToControl.rotationSpeed);
    DPRINT("autosquare: \t\t");
    DPRINTLN(dataToControl.autosquare);
    DPRINT("ENA: \t\t\t");
    DPRINTLN(dataToControl.ena);
    DPRINT("Select axis X: \t\t");
    DPRINTLN(dataToControl.selectAxisX);
    DPRINT("Select axis Y: \t\t");
    DPRINTLN(dataToControl.selectAxisY);
    DPRINT("Select axis Z: \t\t");
    DPRINTLN(dataToControl.selectAxisZ);
    DPRINT("OK: \t\t\t");
    DPRINTLN(dataToControl.ok);
    DPRINT("Programm start: \t");
    DPRINTLN(dataToControl.programmStart);
    DPRINT("Motor start: \t\t");
    DPRINTLN(dataToControl.motorStart);
    DPRINT("Speed1: \t\t");
    DPRINTLN(dataToControl.speed1);
    DPRINT("Speed2: \t\t");
    DPRINTLN(dataToControl.speed2);
    DPRINTLN("- Command -");
    DPRINT("Set joystick: \t\t");
    DPRINTLN(dataToControl.command.setJoystick);
    DPRINT("Set feedrate: \t\t");
    DPRINTLN(dataToControl.command.setFeedrate);
    DPRINT("Set rotation speed: \t");
    DPRINTLN(dataToControl.command.setRotationSpeed);
    DPRINT("Set autosquare: \t");
    DPRINTLN(dataToControl.command.setAutosquare);
    DPRINT("Set ENA: \t\t");
    DPRINTLN(dataToControl.command.setEna);
    DPRINT("Set axis select: \t");
    DPRINTLN(dataToControl.command.setAxisSelect);
    DPRINT("Set OK: \t\t");
    DPRINTLN(dataToControl.command.setOk);
    DPRINT("Set programm start: \t");
    DPRINTLN(dataToControl.command.setProgrammStart);
    DPRINT("Set motor start: \t");
    DPRINTLN(dataToControl.command.setMotorStart);
    DPRINT("Set speed1: \t\t");
    DPRINTLN(dataToControl.command.setSpeed1);
    DPRINT("Set speed2: \t\t");
    DPRINTLN(dataToControl.command.setSpeed2);
    DPRINT("Retrun ack: \t\t");
    DPRINTLN(dataToControl.command.returnACK);
    DPRINT("Retrun data: \t\t");
    DPRINTLN(dataToControl.command.returnData);
}

PROTOCOL protocol;