#include <includes.h>

// void receiveEvent(int count){
// Serial.println(count);
// }

uint8_t newMACAddress[] = CONTROLLER_MAC_ADDRESS;

DATA_TO_CONTROL dataToControl = {512,512,512,0,0,0,0,0,0,0,0,0,0,0,0};

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *address, const uint8_t *incomingData, int len)
{
    // DPRINT("OnDataRecv: Got Message");
    memcpy(&dataToControl, incomingData, sizeof(dataToControl));
    // ios.writeDataBag(&dataToControl);
}

void protocol_setup()
{
    WiFi.enableLongRange(true);
    WiFi.mode(WIFI_STA);

    DPRINT("My old Mac Address: ");
    DPRINTLN(WiFi.macAddress());

    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);

    DPRINT("My new Mac Address: ");
    DPRINTLN(WiFi.macAddress());

    if (esp_now_init() != ESP_OK)
    {
        DPRINTLN("Error initializing ESP-NOW. Things wont work");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
}

char *getMacStrFromAddress(uint8_t *address)
{
    static char macStr[18];
    // Copies the sender mac address to a string
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", address[0], address[1], address[2], address[3], address[4], address[5]);
    return macStr;
}
