#ifndef ocs_ioexpander_h
#define ocs_ioexpander_h

#include <protocol.h>

#define PCA9555_ADRESS_1 0x20
#define PCA9555_ADRESS_2 0x24

#define DAC_JOYSTICK_RESET_VALUE 1023/2

// channel map
#define DAC_JOYSTICK_X 3
#define DAC_JOYSTICK_Y 4
#define DAC_JOYSTICK_Z 5
#define DAC_ROTATION_SPEED 2
#define DAC_FEEDRATE 1
#define DAC_6 6
#define DAC_7 7
#define DAC_8 8

/** enum with mapping names of ports */
enum {
    IO1_DIRX, IO1_DIRY, IO1_DIRZ , IO1_DIRA , IO1_DIRB , IO1_DIRC , IO1_SPEED1 , IO1_SPEED2 ,
    IO1_AUSWAHLZ, IO1_AUSWAHLY, IO1_AUSWAHLX, IO1_OK, IO1_MOTORSTART, IO1_PROGRAMMSTART, IO1_ALARMALL, IO1_AUTOSQUARE
};

enum {
    IO2_IN1, IO2_IN2, IO2_IN3 , IO2_IN4 , IO2_IN5 , IO2_IN6 , IO2_IN7 , IO2_IN8 ,
    IO2_IN9, IO2_IN10, IO2_ENA, IO2_SPINDEL, IO2_OUT1, IO2_OUT2, IO2_OUT3, IO2_OUT4
};

class IOCONTROL {
    public:
        IOCONTROL();
        void setup();
        // getters / inputs
        bool getAlarmAll();
        bool getAutosquare();
        bool getIn1();
        bool getIn2();
        bool getIn3();
        bool getIn4();
        bool getIn5();
        bool getIn6();
        bool getIn7();
        bool getIn8();
        bool getIn9();
        bool getIn10();        
        bool getSpindelOnOff();        
        // setters / outputs
        void setDirX(bool value);
        void setDirY(bool value);
        void setDirZ(bool value);
        void setDirA(bool value);
        void setDirB(bool value);
        void setDirC(bool value);
        
        #ifdef ESP_HANDWHEEL
        void setSpeed1(bool value);
        void setSpeed2(bool value);
        void setAuswahlX(bool value);
        void setAuswahlY(bool value);
        void setAuswahlZ(bool value);
        void setOK(bool value);
        void setMotorStart(bool value);
        void setProgrammStart(bool value);
        #endif

        void setENA(bool value);
        void setOut1(bool value);
        void setOut2(bool value);
        void setOut3(bool value);
        void setOut4(bool value);
        // DAC
        #ifdef ESP_HANDWHEEL
        void resetJoySticksToDefaults();
        void dacSetAllChannel(int value);
        #endif
        
        // general
        void writeDataBag(DATA_TO_CONTROL *data);
};




#endif