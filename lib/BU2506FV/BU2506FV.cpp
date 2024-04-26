#include "BU2506FV.h"

BU2506FV::BU2506FV(){
  resetChannelValues();
}

void BU2506FV::resetOutputs()
{
  for (uint8_t i = 1; i < 9; i++)
  {
    analogWrite(0, i);
  } 
  resetChannelValues();
}

void BU2506FV::begin(uint8_t ldPin)
{
  _ldPin = ldPin;
  pinMode(_ldPin, OUTPUT);
  digitalWrite(_ldPin, LOW);

  _spi_settings = SPISettings(_SPIspeed, MSBFIRST, SPI_MODE0);

  #if defined(ESP32)
    if (_useHSPI)      // HSPI
    {
      mySPI = new SPIClass(HSPI);
      mySPI->end();
      mySPI->begin(14, 12, 13);   // CLK=14 MISO=12 MOSI=13
    }
    else               // VSPI
    {
      mySPI = new SPIClass(VSPI);
      mySPI->end();
      mySPI->begin(18, 19, 23);   // CLK=18 MISO=19 MOSI=23
    }
  #else              // generic hardware SPI
  mySPI = &SPI;
  mySPI->end();
  mySPI->begin();
  #endif
}

bool BU2506FV::analogWrite(uint16_t value, uint8_t channel)
{
  uint16_t data = (value > 1023) ? 1023 : value;
  if(channel > 8 || channel == 0) channel = 1;
  _value[channel] = data;

  switch(channel) {
    case 8: data |= 0x1000 >> 2; break;
    case 7: data |= 0xE000 >> 2; break;
    case 6: data |= 0x6000 >> 2; break;
    case 5: data |= 0xA000 >> 2; break;
    case 4: data |= 0x2000 >> 2; break;
    case 3: data |= 0xC000 >> 2; break;
    case 2: data |= 0x4000 >> 2; break;
    default: data |= 0xE000; break;
  }

  transfer(data);
  return true;
}


void BU2506FV::fastWriteA(uint16_t value)
{
  transfer(0x3000 | value);
}


void BU2506FV::fastWriteB(uint16_t value)
{
  transfer(0xB000 | value);
}


bool BU2506FV::increment(uint8_t channel)
{
  if (_value[channel] == _maxValue) return false;
  return analogWrite(_value[channel] + 1,  channel);
}


bool BU2506FV::decrement(uint8_t channel)
{
  if (_value[channel] == 0) return false;
  return analogWrite(_value[channel] - 1,  channel);
}


void BU2506FV::setPercentage(float perc, uint8_t channel)
{
  if (perc < 0) perc = 0;
  if (perc > 100) perc = 100;
  analogWrite(perc * _maxValue, channel);
}


float BU2506FV::getPercentage(uint8_t channel)
{
  return (_value[channel] * 100.0) / _maxValue;
}


void BU2506FV::setLdPin(uint8_t ldPin)
{
  _ldPin = ldPin;
  pinMode(_ldPin, OUTPUT);
  digitalWrite(_ldPin, LOW);
}

void BU2506FV::setSPIspeed(uint32_t speed)
{
  _SPIspeed = speed;
  _spi_settings = SPISettings(_SPIspeed, MSBFIRST, SPI_MODE0);
};

//////////////////////////////////////////////////////////////////
//
// PROTECTED
//
void BU2506FV::transfer(uint16_t data)
{
  mySPI->beginTransaction(_spi_settings); 
  mySPI->write16(data);
  digitalWrite(_ldPin, HIGH);
  // delayMicroseconds(1);
  digitalWrite(_ldPin, LOW);
  mySPI->endTransaction();  
}

void BU2506FV::resetChannelValues() {
  _value[0] = 0;
  _value[1] = 0;
  _value[2] = 0;
  _value[3] = 0;
  _value[4] = 0;
  _value[5] = 0;
  _value[6] = 0;
  _value[7] = 0;
}


