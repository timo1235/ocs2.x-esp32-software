#pragma once

#include <includes.h>

// The version of the open cnc shield 2 mainboard. This is used to configure the pinmap
// Can be 4 or newer - for example if your shield has version 2.04 use 4 as value, if the shield has 2.12 use 12 as value
#define OCS2_VERSION 8

// Use this ESP32 for controlling the handwheel inputs like Joystick, Programm Start, Motor start, etc.
// True = ESP32 is used for handwheel inputs
// False = ESP32 is not used for handwheel inputs
// Default: true
#define ESP_HANDWHEEL true

// Use this ESP32 for enabling/disabling the stepper drivers
// Should be set to false, if the controller board is used for ENA, like the GRBL Controller.
// Should also be set to false, if there is a wired handwheel for controlling the ENA state. For example
// if the panelModule breakout or panelModule D-SUB25 is used.
// This config option only has an effect if ESP_HANDWHEEL is set to true.
// True = ESP32 is used for stepper driver enable/disable - ENA
// False = ESP32 is not used for stepper driver enable/disable - ENA
// Default: true
#define ESP_SET_ENA true

// Resverse the ENA state for the stepper drivers
// This is needed for some stepper drivers.
// True = When ENA is LOW the stepper drivers are disabled
// False = When ENA is high the stepper drivers are disabled - DRV8825/TMC2209v3.1
// Default: false
#define REVERSE_ENA_STATE false

// Used to invert some outputs, since for example estlcam has pullups on its inputs. That means 
// a signal of 0V is interpreted as 5V and vice versa.
// Affectcs the following outputs: 
// Programm Start, Motor Start, OK, SelectAxisX, Y, and Z, Speed1, Speed2
// Default: true
#define OUTPUTS_INVERTED true

// This is the new mac address of this ESP32 and has to be matched by remote ESP32s
// Default: 0x5E, 0x0, 0x0, 0x0, 0x0, 0x1
#define CONTROLLER_MAC_ADDRESS        \
    {                                 \
        0x5E, 0x0, 0x0, 0x0, 0x0, 0x1 \
    }

// Enable debug output over the serial monitor with a baud of 115200
// Default: true
#define OCS_DEBUG true

//===========================================================================
// *************************   Autosquare    ********************************
//===========================================================================
// AUTOSQUARE_CONFIG config1 = {}
// Comment to prevent the stepper from driving off the endshop when autosquaring has finished
// Default: true
#define DRIVE_FROM_ENDSTOP_AFTER_AUTOSQUARE true
// How long needs the autosquare button to be pressed before starting autosquaring
// This is a safety setting. So that autosquare is not started by accident
// Default: 1500
#define AUTOSQUARE_BUTTON_PRESS_TIME_MS 1500

//===========================================================================
// ***************************   Stepper    *********************************
//===========================================================================
// Acceleration - Change of speed
// as step/s².
// If for example the speed should ramp up from 0 to 10000 steps/s within
// 10s, then the acceleration is 10000 steps/s / 10s = 1000 steps/s²
// Default: 50000
#define STEPPER_ACCELERATION 50000

//--- Axis configuration for autosquare ---
//-- Axis 1 --
#define AXIS1_ACTIVE false
#if AXIS1_ACTIVE
#define AXIS1_MOTOR1 AXIS::x // Can be AXIS::x, AXIS::y, AXIS::z, AXIS::a, AXIS::b, AXIS::c
#define AXIS1_MOTOR1_ENDSTOP_INPUT 1 // Can be a number between 1 and 10 for input 1-10
#define AXIS1_MOTOR1_ENDSTOP_INVERTED true // Can be true or false. If false the endstop is NO(normally open), if true the endstop is NC(normally closed)
#define AXIS1_MOTOR2 AXIS::a // Can be AXIS::x, AXIS::y, AXIS::z, AXIS::a, AXIS::b, AXIS::c
#define AXIS1_MOTOR2_ENDSTOP_INPUT 2 // Can be a number between 1 and 10 for input 1-10
#define AXIS1_MOTOR2_ENDSTOP_INVERTED true // Can be true or false. If false the endstop is NO(normally open), if true the endstop is NC(normally closed)
#define AXIS1_STEPS_PER_REVOLUTION 1600 // How many steps are needed for a complete turn. Normal steppers need 200 steps. Now multiply the microstepp config. E.g. 200*8=1600
#define AXIS1_MM_PER_REVOLUTION 10 // How many mm has the machine moved after one turn.
#define AXIS1_AS_SPEED_MM_S 20 // The speed for autosquaring in mm/s
#define AXIS1_REVERSE_MOTOR_DIRECTION false // Lets the motors rotate counter clockwise
#endif
//-- Axis 2 --
#define AXIS2_ACTIVE false
#if AXIS2_ACTIVE
#define AXIS2_MOTOR1 AXIS::y // Can be AXIS::x, AXIS::y, AXIS::z, AXIS::a, AXIS::b, AXIS::c
#define AXIS2_MOTOR1_ENDSTOP_INPUT 3 // Can be a number between 1 and 10 for input 1-10
#define AXIS2_MOTOR1_ENDSTOP_INVERTED true // Can be true or false. If false the endstop is NO(normally open), if true the endstop is NC(normally closed)
#define AXIS2_MOTOR2 AXIS::b // Can be AXIS::x, AXIS::y, AXIS::z, AXIS::a, AXIS::b, AXIS::c
#define AXIS2_MOTOR2_ENDSTOP_INPUT 4 // Can be a number between 1 and 10 for input 1-10
#define AXIS2_MOTOR2_ENDSTOP_INVERTED true // Can be true or false. If false the endstop is NO(normally open), if true the endstop is NC(normally closed)
#define AXIS2_STEPS_PER_REVOLUTION 1600 // How many steps are needed for a complete turn. Normal steppers need 200 steps. Now multiply the microstepp config. E.g. 200*8=1600
#define AXIS2_MM_PER_REVOLUTION 32 // How many mm has the machine moved after one turn.
#define AXIS2_AS_SPEED_MM_S 20 // The speed for autosquaring in mm/s
#define AXIS2_REVERSE_MOTOR_DIRECTION false // Lets the motors rotate counter clockwise
#endif
//-- Axis 3 --
#define AXIS3_ACTIVE false
#if AXIS3_ACTIVE
#define AXIS3_MOTOR1 AXIS::z // Can be AXIS::x, AXIS::y, AXIS::z, AXIS::a, AXIS::b, AXIS::c
#define AXIS3_MOTOR1_ENDSTOP_INPUT 6 // Can be a number between 1 and 10 for input 1-10
#define AXIS3_MOTOR1_ENDSTOP_INVERTED true // Can be true or false. If false the endstop is NO(normally open), if true the endstop is NC(normally closed)
#define AXIS3_MOTOR2 AXIS::c // Can be AXIS::x, AXIS::y, AXIS::z, AXIS::a, AXIS::b, AXIS::c
#define AXIS3_MOTOR2_ENDSTOP_INPUT 7 // Can be a number between 1 and 10 for input 1-10
#define AXIS3_MOTOR2_ENDSTOP_INVERTED true // Can be true or false. If false the endstop is NO(normally open), if true the endstop is NC(normally closed)
#define AXIS3_STEPS_PER_REVOLUTION 1600 // How many steps are needed for a complete turn. Normal steppers need 200 steps. Now multiply the microstepp config. E.g. 200*8=1600
#define AXIS3_MM_PER_REVOLUTION 10 // How many mm has the machine moved after one turn.
#define AXIS3_AS_SPEED_MM_S 20 // The speed for autosquaring in mm/s
#define AXIS3_REVERSE_MOTOR_DIRECTION false // Lets the motors rotate counter clockwise
#endif
