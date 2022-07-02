
# BU2506FV DAC

Arduino library for 10bit 8-channel SPI DAC Microchip BU2506FV.

## Description

The library is experimental..
Please post an issue if there are problems.

The output voltage of the DAC depends on the voltage supplied, 
which is in the range of 2.7V .. 5.5V. Check datasheet for the details.

#### ESP32 connections (example)

ESP32 has **four** SPI peripherals from which two can be used.

SPI0 and SPI1 are used to access flash memory. SPI2 and SPI3 are "user" SPI controllers a.k.a. HSPI and VSPI.


| BU2506FV  |  HSPI = SPI2  |  VSPI = SPI3  |
|:--------:|:-------------:|:-------------:|
|  LD      |  SELECT = 15  |  SELECT = 5   | 
|  CK     |  SCLK   = 14  |  SCLK   = 18  | 
|  DI     |  MOSI   = 13  |  MOSI   = 23  | 
| not used |  MISO   = 12  |  MISO   = 19  |

## Operation

See examples
