#ifndef ocs_configuration_h
#define ocs_configuration_h

// Use ESP for controlling the handwheel inputs. This is needed, if a ESP32 handwheel should be connected.
// The connection can be over wifi or with a RJ45 cable
#define ESP_HANDWHEEL

// Used to invert some outputs, since estlcam has pullups on its inputs. 
#define ESTLCAM_CONTROLLER

#define CONTROLLER_MAC_ADDRESS { 0x5E, 0x0, 0x0, 0x0, 0x0, 0x1 }

#define WRITE_DATA_BAG_INTERVAL_MS 5

#define OCS_DEBUG

#endif