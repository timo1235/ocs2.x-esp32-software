#ifndef bu2506_h
#define bu2506_h

#include <Arduino.h>
#include <SPI.h>

#define BU2506FV_LIB_VERSION       (F("0.0.1"))

class BU2506FV
{
public:
  BU2506FV();

  // if only select is given ==> HW SPI
  void     begin(uint8_t ldPin);

  bool     analogWrite(uint16_t value, uint8_t channel = 0);
  uint16_t lastValue(uint8_t channel = 0) { return _value[channel]; };
  void     fastWriteA(uint16_t value);
  void     fastWriteB(uint16_t value);

  bool     increment(uint8_t channel = 0);
  bool     decrement(uint8_t channel = 0);

  //       convenience wrappers
  //       percentage = 0..100.0%
  void     setPercentage(float percentage, uint8_t channel = 0);
  float    getPercentage(uint8_t channel = 0);

  //       trigger LDAC = LatchDAC pin - if not set it does nothing
  void     setLdPin( uint8_t ldPin);

  //       speed in Hz
  void     setSPIspeed(uint32_t speed);
  uint32_t getSPIspeed() { return _SPIspeed; };

  void     resetOutputs();

  // ESP32 specific
  #if defined(ESP32)
  void     selectHSPI() { _useHSPI = true;  };
  void     selectVSPI() { _useHSPI = false; };
  bool     usesHSPI()   { return _useHSPI;  };
  bool     usesVSPI()   { return !_useHSPI; };
  #endif

protected:
  uint8_t  _ldPin = 255;
  uint32_t _SPIspeed = 10000000;

  uint16_t _maxValue = 1023;
  uint16_t _value[8];

  void transfer(uint16_t data);
  void resetChannelValues();

  SPIClass    * mySPI;
  SPISettings _spi_settings;

  #if defined(ESP32)
  bool        _useHSPI = false;
  #endif
};


#endif