#pragma once

#define BU2506_LD_PIN 33

#define ESP_PANEL_LED_PIN 5

#define TEMPERATURE_SENSOR_PIN 25 // DS18B20 Sensor

// Motor Step pins
#define STEP_X_PIN 12
#define STEP_Y_PIN 14
#define STEP_Z_PIN 26
#define STEP_A_PIN 13
#define STEP_B_PIN 15
#define STEP_C_PIN 4

#define IOInterrupt1Pin 36
#define IOInterrupt2Pin 39

// I2C Pins
#if OCS2_VERSION == 4
#define I2C_BUS_SDA 17 
#define I2C_BUS_SCL 16
#else 
#define I2C_BUS_SDA 21
#define I2C_BUS_SCL 22
#endif


