//
//  Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>



class p44_ws2812 {

  uint16_t numLeds; // number of LEDs
  uint16_t ledsPerRow; // number of LEDs per row (physically, along WS2812 chain)
  uint16_t numRows; // number of rows (sections of WS2812 chain)
  bool xReversed; // even (0,2,4...) rows go backwards, or all if not alternating
  bool alternating; // direction changes after every row
  bool swapXY; // x and y swapped

  bool initialized;

  int ledFd; // the file descriptor for the LED device
  uint8_t *ledbuffer; // the raw bytes to be sent to the WS2812 device

public:
  /// create driver for a WS2812 LED chain
  /// @param aNumLeds number of LEDs in the chain
  /// @param aLedsPerRow number of consecutive LEDs in the WS2812 chain that build a row (usually x direction, y if swapXY was set)
  /// @param aXReversed X direction is reversed
  /// @param aAlternating X direction is reversed in first row, normal in second, reversed in third etc..
  /// @param aSwapXY X and Y reversed (for up/down wiring)
  p44_ws2812(uint16_t aNumLeds, uint16_t aLedsPerRow=0, bool aXReversed=false, bool aAlternating=false, bool aSwapXY=false);

  /// destructor
  ~p44_ws2812();

  /// begin using the driver
  bool begin();

  /// end using the driver
  void end();

  /// transfer RGB values to LED chain
  /// @note this must be called to update the actual LEDs after modifying RGB values
  /// with setColor() and/or setColorDimmed()
  void show();


  /// set color of one LED
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  void setColorXY(uint16_t aX, uint16_t aY, uint8_t aRed, uint8_t aGreen, uint8_t aBlue);
  void setColor(uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue);

  /// set color of one LED, scaled by a visible brightness (non-linear) factor
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  /// @param aBrightness brightness, will be converted non-linear to PWM duty cycle for uniform brightness scale, 0..255
  void setColorDimmedXY(uint16_t aX, uint16_t aY, uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aBrightness);
  void setColorDimmed(uint16_t aLedNumber, uint8_t aRed, uint8_t aGreen, uint8_t aBlue, uint8_t aBrightness);

  /// get current color of LED
  /// @param aRed set to intensity of red component, 0..255
  /// @param aGreen set to intensity of green component, 0..255
  /// @param aBlue set to intensity of blue component, 0..255
  /// @note for LEDs set with setColorDimmed(), this returns the scaled down RGB values,
  ///   not the original r,g,b parameters. Note also that internal brightness resolution is 5 bits only.
  void getColorXY(uint16_t aX, uint16_t aY, uint8_t &aRed, uint8_t &aGreen, uint8_t &aBlue);
  void getColor(uint16_t aLedNumber, uint8_t &aRed, uint8_t &aGreen, uint8_t &aBlue);

  /// @return number of LEDs
  uint16_t getNumLeds();

  /// @return size of array in X direction (x range is 0..getSizeX()-1)
  uint16_t getSizeX();

  /// @return size of array in Y direction (y range is 0..getSizeY()-1)
  uint16_t getSizeY();

private:

  uint16_t ledIndexFromXY(uint16_t aX, uint16_t aY);

};
