#include <Arduino.h>
const char *VARIABLES_DEF_YAML PROGMEM = R"~(
General Configuration:
  - PCB_Type:
      label: <strong>Type of the PCB</strong>
      options: 'OCS2:OCS2, OCS2_Mini:OCS2_Mini, undefined:undefined'
      default: undefined
  - PCB_Version:
      label: <strong>PCB Version</strong> can be found on the PCB itself
      options: '1.2', '2.0', '2.1', '2.2', '2.3', '2.4', '2.5', '2.6', '2.7', '2.8', '2.9', '2.10', '2.11', '2.12', '2.13'
      default: 2.0
  - enable_WiFi:
      label: <strong>Enable WiFi Hotspot</strong>
        Enables/disables the WiFi hotspot. If disabled, the ESP32 will not start the WiFi hotspot after booting.
        To manually start the WiFi hotspot, press the function button on the OCS2_Mini or 
        <a href="https://docs.timos-werkstatt.de/open-cnc-shield-2/mainboard/anschluesse-jumper#esp32-pinout">ESP32_D34 on the ESP32 Pinout for the OCS2</a>
        If the configuration is finished, I recommend to disable the WiFi hotspot. That saves performance and you dont have an unnecessary WiFi network.
      checked: True
Handwheel / ESP32 Panel:
  - Wireless_ID:
      label: Can be any number between 0 and 255. It has to match the number in the Wireless Panel.
        To avoid collisions when there are several controllers around, a custom ID can be set here.
        The default is 0
      default: 0
      attribs: min='0' max='255' step='1'
  - control_handwheel_functions:
      label: <strong>Control Handwheel functions</strong>
        Needs to be activated if an ESP32 Panel should be used, otherwise it can be deactivated.
      checked: False
  - control_driver_enable:
      label: <strong>Control Driver Enable State</strong> - enabling/disabling the stepper drivers.
        Should be set to false, if the controller board is used for ENA, like the GRBL Controller.
        Should also be set to false, if there is a wired handwheel for controlling the ENA state. For example if the panelModule breakout or panelModule D-SUB25 is used.
        This config option only has an effect if the controlling of handwheel functions is activated.
      checked: False
  - reverse_driver_enable:
      label: <strong>Reverse Driver Enable State</strong>
        This is needed for some stepper drivers.
        Active = When ENA is LOW the stepper drivers are disabled.
        Inactive = When ENA is high the stepper drivers are disabled(DRV8825/TMC2209v3.1)
      checked: False
  - outputs_inverted:
      label: <strong>Outputs Inverted</strong>
        Used to invert some outputs, since for example estlcam has pullups on its inputs. That means a signal of 0V is interpreted as 5V and vice versa. Should be active in 99% of the cases.
        Affectcs the following outputs - Programm Start, Motor Start, OK, SelectAxisX, Y, and Z, Speed1, Speed2
      checked: True
  - reset_on_connection_loss:
      label:  <strong>Reset Feedrate and Rotation Speed on Connection Loss</strong>
        If the connection to the ESP32 Panel is lost, the feedrate and rotation speed are reset to 0.
        Has no effect when the controlling of handwheel functions is inactive.
      checked: False
FluidNC Jogging:
  - fluidNC_jogging:
      label: <strong>FluidNC Jogging - This is an experimental feature</strong>. It is not recommended to use this feature in production.
        If activated, the serial connection with the handwheel wont work anymore, since the serial connection is used for the jogging commands to FluidNC.
        See <a href="https://blog.altholtmann.com/fluidnc-jogging-mit-wireless-handrad/">here</a> for more information.
      checked: False
  - jogging_min_speed:
      label: <strong>Min Jogging Speed</strong> in mm/min.
        The max speed is read from the FluidNC settings via
        the serial interface. This means max speed will be the value from your FluidNC config.yaml
      default: 100
Autosquare Configuration:
  - autosquare_button_time:
      label: <strong>Autosquare Button Press Time</strong>
        The time the button has to be pressed to start the autosquare function.
        The time is in milliseconds. The default is 1500ms.
      default: 1500
  - stepper_acceleration:
      label: <strong>Stepper Acceleration</strong>
        Acceleration - Change of speed as step/s².
        If for example the speed should ramp up from 0 to 10000 steps/s within 10s, then the acceleration is 10000 steps/s / 10s = 1000 steps/s².
      default: 20000
Autosquare Axis 1:
  - axis_1_active:
      label: <strong>Axis 1 Active</strong>
      checked: False
  - axis_1_motor_1:
      label: <strong>Motor 1</strong>
        Assign which motor on Axis 1 is used for the first autosquaring sequence. Choose from the available axes.
      options: 'X:X, Y:Y, Z:Z, A:A, B:B, C:C'
      default: X
  - axis_1_motor_1_endstop_input:
      label: <strong>Motor 1 Endstop Input</strong>
        Select which endstop input to use for Motor 1.
      options: '1', '2', '3', '4', '5', '6', '7', '8', '9', '10'
      default: 1
  - axis_1_motor_1_endstop_inverted:
      label: <strong>Motor 1 Endstop Inverted</strong>
        Defines if the endstop signal for Motor 1 is inverted. Useful for different types of switches.
      checked: False
  - axis_1_motor_1_back_distance:
      label: <strong>Motor 1 Drive Back Distance(mm)</strong>
        Specifies how far Motor 1 should drive back after triggering the endstop. Set in millimeters.
      default: 10
      attribs: min='0' step='1'
  - axis_1_motor_2:
      label: <strong>Motor 2</strong>
        Select the second motor used for autosquaring.
      options: 'X:X, Y:Y, Z:Z, A:A, B:B, C:C'
      default: A
  - axis_1_motor_2_endstop_input:
      label: <strong>Motor 2 Endstop Input</strong>
        Select which endstop input to use for Motor 2.
      options: '1', '2', '3', '4', '5', '6', '7', '8', '9', '10'
      default: 2
  - axis_1_motor_2_endstop_inverted:
      label: <strong>Motor 2 Endstop Inverted</strong>
        Defines if the endstop signal for Motor 2 is inverted. Useful for different types of switches.
      checked: False
  - axis_1_motor_2_back_distance:
      label: <strong>Motor 2 Drive Back Distance(mm)</strong>
        Specifies how far Motor 1 should drive back after triggering the endstop. Set in millimeters.
      default: 10
      attribs: min='0' step='1'
  - axis_1_steps_per_revolution:
      label: <strong>Steps per Revolution</strong>
        Set the number of stepper motor steps required to complete one full revolution. Critical for precise motion settings.
      default: 1600
  - axis_1_mm_per_revolution:
      label: <strong>mm per Revolution</strong>
        Indicate the linear movement in millimeters that occurs with one full revolution of the motor.
      default: 32
  - axis_1_as_speed:
      label: <strong>Speed for Autosquaring</strong>
        The speed for the motors for autosquaring. The speed is in mm/s. 
      default: 20
      attribs: min='1' max='100' step='1'
  - axis_1_reverse_motor_direction:
      label: <strong>Reverse Motor Direction</strong>
        Reverse the motor direction for the autosquaring sequence.
      checked: False
Autosquare Axis 2:
  - axis_2_active:
      label: <strong>Axis 2 Active</strong>
      checked: False
  - axis_2_motor_1:
      label: <strong>Motor 1</strong>
        Assign which motor on Axis 2 is used for the first autosquaring sequence. Choose from the available axes.
      options: 'X:X, Y:Y, Z:Z, A:A, B:B, C:C'
      default: Y
  - axis_2_motor_1_endstop_input:
      label: <strong>Motor 1 Endstop Input</strong>
        Select which endstop input to use for Motor 1.
      options: '1', '2', '3', '4', '5', '6', '7', '8', '9', '10'
      default: 3
  - axis_2_motor_1_endstop_inverted:
      label: <strong>Motor 1 Endstop Inverted</strong>
        Defines if the endstop signal for Motor 1 is inverted. Useful for different types of switches.
      checked: False
  - axis_2_motor_1_back_distance:
      label: <strong>Motor 1 Drive Back Distance(mm)</strong>
        Specifies how far Motor 1 should drive back after triggering the endstop. Set in millimeters.
      default: 10
      attribs: min='0' step='1'
  - axis_2_motor_2:
      label: <strong>Motor 2</strong>
        Select the second motor used for autosquaring.
      options: 'X:X, Y:Y, Z:Z, A:A, B:B, C:C'
      default: B
  - axis_2_motor_2_endstop_input:
      label: <strong>Motor 2 Endstop Input</strong>
        Select which endstop input to use for Motor 2.
      options: '1', '2', '3', '4', '5', '6', '7', '8', '9', '10'
      default: 4
  - axis_2_motor_2_endstop_inverted:
      label: <strong>Motor 2 Endstop Inverted</strong>
        Defines if the endstop signal for Motor 2 is inverted. Useful for different types of switches.
      checked: False
  - axis_2_motor_2_back_distance:
      label: <strong>Motor 2 Drive Back Distance(mm)</strong>
        Specifies how far Motor 1 should drive back after triggering the endstop. Set in millimeters.
      default: 10
      attribs: min='0' step='1'
  - axis_2_steps_per_revolution:  
      label: <strong>Steps per Revolution</strong>
        Set the number of stepper motor steps required to complete one full revolution. Critical for precise motion settings.
      default: 1600 
  - axis_2_mm_per_revolution: 
      label: <strong>mm per Revolution</strong>
        Indicate the linear movement in millimeters that occurs with one full revolution of the motor.
      default: 32
  - axis_2_as_speed:
      label: <strong>Speed for Autosquaring</strong>
        The speed for the motors for autosquaring. The speed is in mm/s. 
      default: 20
      attribs: min='1' max='100' step='1'
  - axis_2_reverse_motor_direction:
      label: <strong>Reverse Motor Direction</strong>
        Reverse the motor direction for the autosquaring sequence.
      checked: False
Autosquare Axis 3:
  - axis_3_active:
      label: <strong>Axis 3 Active</strong>
      checked: False
  - axis_3_motor_1:
      label: <strong>Motor 1</strong>
        Assign which motor on Axis 3 is used for the first autosquaring sequence. Choose from the available axes.
      options: 'X:X, Y:Y, Z:Z, A:A, B:B, C:C'
      default: Z
  - axis_3_motor_1_endstop_input:
      label: <strong>Motor 1 Endstop Input</strong>
        Select which endstop input to use for Motor 1.
      options: '1', '2', '3', '4', '5', '6', '7', '8', '9', '10'
      default: 5
  - axis_3_motor_1_endstop_inverted:
      label: <strong>Motor 1 Endstop Inverted</strong>
        Defines if the endstop signal for Motor 1 is inverted. Useful for different types of switches.
      checked: False
  - axis_3_motor_1_back_distance:
      label: <strong>Motor 1 Drive Back Distance(mm)</strong>
        Specifies how far Motor 1 should drive back after triggering the endstop. Set in millimeters.
      default: 10
      attribs: min='0' step='1'
  - axis_3_motor_2:
      label: <strong>Motor 2</strong>
        Select the second motor used for autosquaring.
      options: 'X:X, Y:Y, Z:Z, A:A, B:B, C:C'
      default: C
  - axis_3_motor_2_endstop_input:
      label: <strong>Motor 2 Endstop Input</strong>
        Select which endstop input to use for Motor 2.
      options: '1', '2', '3', '4', '5', '6', '7', '8', '9', '10'
      default: 6
  - axis_3_motor_2_endstop_inverted:
      label: <strong>Motor 2 Endstop Inverted</strong>
        Defines if the endstop signal for Motor 2 is inverted. Useful for different types of switches.
      checked: False
  - axis_3_motor_2_back_distance:
      label: <strong>Motor 2 Drive Back Distance(mm)</strong>
        Specifies how far Motor 1 should drive back after triggering the endstop. Set in millimeters.
      default: 10
      attribs: min='0' step='1'
  - axis_3_steps_per_revolution:
      label: <strong>Steps per Revolution</strong>
        Set the number of stepper motor steps required to complete one full revolution. Critical for precise motion settings.
      default: 1600
  - axis_3_mm_per_revolution:
      label: <strong>mm per Revolution</strong>
        Indicate the linear movement in millimeters that occurs with one full revolution of the motor.
      default: 32
  - axis_3_as_speed:
      label: <strong>Speed for Autosquaring</strong>
        The speed for the motors for autosquaring. The speed is in mm/s. 
      default: 20
      attribs: min='1' max='100' step='1'
  - axis_3_reverse_motor_direction:
      label: <strong>Reverse Motor Direction</strong>
        Reverse the motor direction for the autosquaring sequence.
      checked: False
)~";   // VARIABLES_DEF_YAML

// - debug_output:
//     label: <strong>Serial Debug Output</strong>
//       Output some additional data when esp32 is connected via serial. Start the serial monitor to see the output.
//     checked: True