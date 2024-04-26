
#include <includes.h>

GLOBAL_VARS GLOBAL;

uint32_t cpu0Counter = 0;
uint32_t cpu1Counter = 0;
uint32_t cpu0LastMessage = 0;
uint32_t cpu1LastMessage = 0;

void setup() {
    GLOBAL.IOControlInitialized = false;
    GLOBAL.protocolInitialized = false;
    GLOBAL.grblJoggingInitialized = false;
    GLOBAL.uiHandlerInitialized = false;
    GLOBAL.configManagerWiFiInitialized = false;

    Serial.begin(115200);
    Serial.print("OCS2 ESP32 Firmware version: ");
    Serial.println(FIRMWARE_VERSION);

    // start config manager first - this loads all configuration
    configManager.setup();
    versionManager.setup();
    ioControl.setup();
    protocol.setup();

    grblJogging.setup();
    stepperControl.setup();

    // DPRINT("Packagesize DATA_TO_CONTROL in bytes: ");
    // DPRINTLN(sizeof(DATA_TO_CONTROL));
    // DPRINT("Packagesize DATA_TO_CLIENT in bytes: ");
    // DPRINTLN(sizeof(DATA_TO_CLIENT));
}

void loop() { configManager.loop(); }
