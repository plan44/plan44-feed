//
//  Copyright (c) 2016 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/param.h>


// the library wrapper, API compatible with spark core version
#include "p44_ws2812.hpp"

// p44 utils
#include "utils.hpp"


// ugly mapping to mimick spark core lib
#include <string>
#define String string
typedef uint8_t byte;

using namespace std;
using namespace p44;


// Utilities
// =========


uint16_t random(uint16_t aMinOrMax, uint16_t aMax = 0)
{
  if (aMax==0) {
    aMax = aMinOrMax;
    aMinOrMax = 0;
  }
  uint32_t r = aMinOrMax;
  aMax = aMax - aMinOrMax + 1;
  r += rand() % aMax;
  return r;
}


inline void reduce(byte &aByte, byte aAmount, byte aMin = 0)
{
  int r = aByte-aAmount;
  if (r<aMin)
    aByte = aMin;
  else
    aByte = (byte)r;
}


inline void increase(byte &aByte, byte aAmount, byte aMax = 255)
{
  int r = aByte+aAmount;
  if (r>aMax)
    aByte = aMax;
  else
    aByte = (byte)r;
}


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
void wheel(byte WheelPos, byte &red, byte &green, byte &blue) {
  if(WheelPos < 85) {
    red = WheelPos * 3;
    green = 255 - WheelPos * 3;
    blue = 0;
  } else if(WheelPos < 170) {
    WheelPos -= 85;
    red = 255 - WheelPos * 3;
    green = 0;
    blue = WheelPos * 3;
  } else {
    WheelPos -= 170;
    red = 0;
    green = WheelPos * 3;
    blue = 255 - WheelPos * 3;
  }
}


int hexToInt(char aHex)
{
  if (aHex<'0') return 0;
  aHex -= '0';
  if (aHex>9) aHex -= 7;
  if (aHex>15) return 0;
  return aHex;
}




// Simple 7 pixel height dot matrix font
// =====================================
// Note: the font is derived from a monospaced 7*5 pixel font, but has been adjusted a bit
//       to get rendered proportionally (variable character width, e.g. "!" has width 1, whereas "m" has 7)
//       In the fontGlyphs table below, every char has a number of pixel colums it consists of, and then the
//       actual column values encoded as a string.

typedef struct {
  uint8_t width;
  const char *cols;
} glyph_t;

const int numGlyphs = 102; // 96 ASCII 0x20..0x7F plus 6 ÄÖÜäöü
const int rowsPerGlyph = 7;
const int glyphSpacing = 2;

static const glyph_t fontGlyphs[numGlyphs] = {
  { 5, "\x00\x00\x00\x00\x00" },  //   0x20 (0)
  { 1, "\x5f" },                  // ! 0x21 (1)
  { 3, "\x03\x00\x03" },          // " 0x22 (2)
  { 5, "\x28\x7c\x28\x7c\x28" },  // # 0x23 (3)
  { 5, "\x24\x2a\x7f\x2a\x12" },  // $ 0x24 (4)
  { 5, "\x4c\x2c\x10\x68\x64" },  // % 0x25 (5)
  { 5, "\x30\x4e\x55\x22\x40" },  // & 0x26 (6)
  { 1, "\x01" },                  // ' 0x27 (7)
  { 3, "\x1c\x22\x41" },          // ( 0x28 (8)
  { 3, "\x41\x22\x1c" },          // ) 0x29 (9)
  { 5, "\x01\x03\x01\x03\x01" },  // * 0x2A (10)
  { 5, "\x08\x08\x3e\x08\x08" },  // + 0x2B (11)
  { 2, "\x50\x30" },              // , 0x2C (12)
  { 5, "\x08\x08\x08\x08\x08" },  // - 0x2D (13)
  { 2, "\x60\x60" },              // . 0x2E (14)
  { 5, "\x40\x20\x10\x08\x04" },  // / 0x2F (15)

  { 5, "\x3e\x51\x49\x45\x3e" },  // 0 0x30 (0)
  { 3, "\x42\x7f\x40" },          // 1 0x31 (1)
  { 5, "\x62\x51\x49\x49\x46" },  // 2 0x32 (2)
  { 5, "\x22\x41\x49\x49\x36" },  // 3 0x33 (3)
  { 5, "\x0c\x0a\x09\x7f\x08" },  // 4 0x34 (4)
  { 5, "\x4f\x49\x49\x49\x31" },  // 5 0x35 (5)
  { 5, "\x3e\x49\x49\x49\x32" },  // 6 0x36 (6)
  { 5, "\x03\x01\x71\x09\x07" },  // 7 0x37 (7)
  { 5, "\x36\x49\x49\x49\x36" },  // 8 0x38 (8)
  { 5, "\x26\x49\x49\x49\x3e" },  // 9 0x39 (9)
  { 2, "\x66\x66" },              // : 0x3A (10)
  { 2, "\x56\x36" },              // ; 0x3B (11)
  { 4, "\x08\x14\x22\x41" },      // < 0x3C (12)
  { 4, "\x24\x24\x24\x24" },      // = 0x3D (13)
  { 4, "\x41\x22\x14\x08" },      // > 0x3E (14)
  { 5, "\x02\x01\x59\x09\x06" },  // ? 0x3F (15)

  { 5, "\x3e\x41\x5d\x55\x5e" },  // @ 0x40 (0)
  { 5, "\x7c\x0a\x09\x0a\x7c" },  // A 0x41 (1)
  { 5, "\x7f\x49\x49\x49\x36" },  // B 0x42 (2)
  { 5, "\x3e\x41\x41\x41\x22" },  // C 0x43 (3)
  { 5, "\x7f\x41\x41\x22\x1c" },  // D 0x44 (4)
  { 5, "\x7f\x49\x49\x41\x41" },  // E 0x45 (5)
  { 5, "\x7f\x09\x09\x01\x01" },  // F 0x46 (6)
  { 5, "\x3e\x41\x49\x49\x7a" },  // G 0x47 (7)
  { 5, "\x7f\x08\x08\x08\x7f" },  // H 0x48 (8)
  { 3, "\x41\x7f\x41" },          // I 0x49 (9)
  { 5, "\x30\x40\x40\x40\x3f" },  // J 0x4A (10)
  { 5, "\x7f\x08\x0c\x12\x61" },  // K 0x4B (11)
  { 5, "\x7f\x40\x40\x40\x40" },  // L 0x4C (12)
  { 7, "\x7f\x02\x04\x0c\x04\x02\x7f" },  // M 0x4D (13)
  { 5, "\x7f\x02\x04\x08\x7f" },  // N 0x4E (14)
  { 5, "\x3e\x41\x41\x41\x3e" },  // O 0x4F (15)

  { 5, "\x7f\x09\x09\x09\x06" },  // P 0x50 (0)
  { 5, "\x3e\x41\x51\x61\x7e" },  // Q 0x51 (1)
  { 5, "\x7f\x09\x09\x09\x76" },  // R 0x52 (2)
  { 5, "\x26\x49\x49\x49\x32" },  // S 0x53 (3)
  { 5, "\x01\x01\x7f\x01\x01" },  // T 0x54 (4)
  { 5, "\x3f\x40\x40\x40\x3f" },  // U 0x55 (5)
  { 5, "\x1f\x20\x40\x20\x1f" },  // V 0x56 (6)
  { 5, "\x7f\x40\x38\x40\x7f" },  // W 0x57 (7)
  { 5, "\x63\x14\x08\x14\x63" },  // X 0x58 (8)
  { 5, "\x03\x04\x78\x04\x03" },  // Y 0x59 (9)
  { 5, "\x61\x51\x49\x45\x43" },  // Z 0x5A (10)
  { 3, "\x7f\x41\x41" },          // [ 0x5B (11)
  { 5, "\x04\x08\x10\x20\x40" },  // \ 0x5C (12)
  { 3, "\x41\x41\x7f" },          // ] 0x5D (13)
  { 4, "\x04\x02\x01\x02" },      // ^ 0x5E (14)
  { 5, "\x40\x40\x40\x40\x40" },  // _ 0x5F (15)

  { 2, "\x01\x02" },              // ` 0x60 (0)
  { 5, "\x20\x54\x54\x54\x78" },  // a 0x61 (1)
  { 5, "\x7f\x44\x44\x44\x38" },  // b 0x62 (2)
  { 5, "\x38\x44\x44\x44\x08" },  // c 0x63 (3)
  { 5, "\x38\x44\x44\x44\x7f" },  // d 0x64 (4)
  { 5, "\x38\x54\x54\x54\x18" },  // e 0x65 (5)
  { 5, "\x08\x7e\x09\x09\x02" },  // f 0x66 (6)
  { 5, "\x48\x54\x54\x54\x38" },  // g 0x67 (7)
  { 5, "\x7f\x08\x08\x08\x70" },  // h 0x68 (8)
  { 3, "\x48\x7a\x40" },          // i 0x69 (9)
  { 5, "\x20\x40\x40\x48\x3a" },  // j 0x6A (10)
  { 4, "\x7f\x10\x28\x44" },      // k 0x6B (11)
  { 3, "\x3f\x40\x40" },          // l 0x6C (12)
  { 7, "\x7c\x04\x04\x38\x04\x04\x78" },  // m 0x6D (13)
  { 5, "\x7c\x04\x04\x04\x78" },  // n 0x6E (14)
  { 5, "\x38\x44\x44\x44\x38" },  // o 0x6F (15)

  { 5, "\x7c\x14\x14\x14\x08" },  // p 0x70 (0)
  { 5, "\x08\x14\x14\x7c\x40" },  // q 0x71 (1)
  { 5, "\x7c\x04\x04\x04\x08" },  // r 0x72 (2)
  { 5, "\x48\x54\x54\x54\x24" },  // s 0x73 (3)
  { 5, "\x04\x04\x7f\x44\x44" },  // t 0x74 (4)
  { 5, "\x3c\x40\x40\x40\x7c" },  // u 0x75 (5)
  { 5, "\x1c\x20\x40\x20\x1c" },  // v 0x76 (6)
  { 7, "\x7c\x40\x40\x38\x40\x40\x7c" },  // w 0x77 (7)
  { 5, "\x44\x28\x10\x28\x44" },  // x 0x78 (8)
  { 5, "\x0c\x50\x50\x50\x3c" },  // y 0x79 (9)
  { 5, "\x44\x64\x54\x4c\x44" },  // z 0x7A (10)
  { 3, "\x08\x36\x41" },          // { 0x7B (11)
  { 1, "\x7f" },                  // | 0x7C (12)
  { 3, "\x41\x36\x08" },          // } 0x7D (13)
  { 4, "\x04\x02\x04\x08" },      // ~ 0x7E (14)
  { 5, "\x7F\x41\x41\x41\x7F" },  //   0x7F (15)

  { 5, "\x7D\x0a\x09\x0a\x7D" },  // Ä 0x41 (1)
  { 5, "\x3F\x41\x41\x41\x3F" },  // Ö 0x4F (15)
  { 5, "\x3D\x40\x40\x40\x3D" },  // Ü 0x55 (5)
  { 5, "\x20\x55\x54\x55\x78" },  // ä 0x61 (1)
  { 5, "\x38\x45\x44\x45\x38" },  // ö 0x6F (15)
  { 5, "\x3c\x41\x40\x41\x7c" },  // ü 0x75 (5)
};


// Main program, torch simulation
// ==============================


const uint16_t maxLeds = 512;
uint16_t numLeds;

// LED chain configuration: the values set here are the defaults, and can be overridden by command line options

// Number of LEDs around the tube. One too much looks better (italic text look)
// than one to few (backwards leaning text look)
// Higher number = diameter of the torch gets larger
uint16_t ledsPerLevel = 16; // Original: 13, smaller tube 11, high density small 17

// Number of "windings" of the LED strip around (or within) the tube
// Higher number = torch gets taller
uint16_t levels = 16; // original 18, smaller tube 21, high density small 7

// set to true if you wound the torch clockwise (as seen from top). Note that
// this reverses the entire animation (in contrast to mirrorText, which only
// mirrors text).
bool reversedX = false;
// set to true if every other row in the LED matrix is ordered backwards.
// This mode is useful for WS2812 modules which have e.g. 16x16 LEDs on one
// flexible PCB. On these modules, the data line starts in the lower left
// corner, goes right for row 0, then left in row 1, right in row 2 etc.
bool alternatingX = true;
// set to true if your WS2812 chain runs up (or up/down, with alternatingX set) the "torch",
// for example if you want to do a wide display out of multiple 16x16 arrays
bool swapXY = false;

// Set this to true if you wound the LED strip clockwise, starting at the bottom of the
// tube, when looking onto the tube from the top. The default winding direction
// for versions of MessageTorch which did not have this setting was 0, which
// means LED strip was usually wound counter clock wise from bottom to top.
// Note: this setting only reverses the direction of text rendering - the torch
//   animation itself is not affected
bool mirrorText = false;

// if set, assume SK6812 RGBW LEDs
bool rgbw = false;

// variables

p44_ws2812 *leds = NULL;


// global parameters: the values set here are the defaults, and can be changed via sending UDP command messages

enum {
  mode_off = 0,
  mode_torch = 1, // torch
  mode_colorcycle = 2, // moving color cycle
  mode_lamp = 3, // lamp
  mode_testpattern = 4, // test pattern
};

byte mode = mode_torch; // main operation mode
int brightness = 255; // overall brightness
byte fade_base = 140; // crossfading base brightness level

// text params

int text_intensity = 255; // intensity of last column of text (where text appears)
int cycles_per_px = 5;
int text_repeats = 15; // text displays until faded down to almost zero
int fade_per_repeat = 15; // how much to fade down per repeat
int text_base_line = 8;
byte red_text = 0;
byte green_text = 255;
byte blue_text = 180;


// clock parameters

int clock_interval = 0; // 15*60; // by default, show clock every 15 mins (0=never)
char clock_fmt[30] = "%k:%M"; // use format specifiers from strftime, see e.g. http://linux.die.net/man/3/strftime. %k:%M is 24h hour/minute clock

// torch parameters

uint16_t cycle_wait = 1; // 0..255

byte flame_min = 100; // 0..255
byte flame_max = 220; // 0..255

byte random_spark_probability = 2; // 0..100
byte spark_min = 200; // 0..255
byte spark_max = 255; // 0..255

byte spark_tfr = 40; // 0..256 how much energy is transferred up for a spark per cycle
uint16_t spark_cap = 200; // 0..255: spark cells: how much energy is retained from previous cycle

uint16_t up_rad = 40; // up radiation
uint16_t side_rad = 35; // sidewards radiation
uint16_t heat_cap = 0; // 0..255: passive cells: how much energy is retained from previous cycle

byte red_bg = 0;
byte green_bg = 0;
byte blue_bg = 0;
byte red_bias = 10;
byte green_bias = 0;
byte blue_bias = 0;
int red_energy = 180;
int green_energy = 145;
int blue_energy = 0;

byte upside_down = 0; // if set, flame (or rather: drop) animation is upside down. Text remains as-is


// lamp mode params

byte lamp_red = 220;
byte lamp_green = 220;
byte lamp_blue = 200;



// text layer
// ==========

// text layer, but only one strip around the tube (ledsPerLevel) with the height of the characters (rowsPerGlyph)
const int maxTextPixels = 100*rowsPerGlyph;
int textPixels = 0;
byte textLayer[maxTextPixels];
String text;

int textPixelOffset;
int textCycleCount;
int repeatCount;


// this function automagically gets called upon a matching POST request
int newMessage(String aText)
{
  // URL decode
  text = "";
  int i = 0;
  char c;
  while (i<(int)aText.length()) {
    // Ä = C3 84
    // Ö = C3 96
    // Ü = C3 9C
    // ä = C3 A4
    // ö = C3 B6
    // ü = C3 BC
    if (aText[i]==0xC3) {
      if ((int)aText.length()<=i+1) break; // end of text
      switch (aText[i+1]) {
        case 0x84: c = 0x80; break; // Ä
        case 0x96: c = 0x81; break; // Ö
        case 0x9C: c = 0x82; break; // Ü
        case 0xA4: c = 0x83; break; // ä
        case 0xB6: c = 0x84; break; // ö
        case 0xBC: c = 0x85; break; // ü
        default: c = 0x7F; break; // unknown
      }
      i += 1;
    }
    else {
      c = aText[i];
    }
    // put to output string
    text += c;
    i++;
  }
  // initiate display of new text
  textPixelOffset = -ledsPerLevel;
  textCycleCount = 0;
  repeatCount = 0;
  return 1;
}


void resetText()
{
  for(int i=0; i<textPixels; i++) {
    textLayer[i] = 0;
  }
}


void crossFade(byte aFader, byte aValue, byte &aOutputA, byte &aOutputB)
{
  byte baseBrightness = (aValue*fade_base)>>8;
  byte varBrightness = aValue-baseBrightness;
  byte fade = (varBrightness*aFader)>>8;
  aOutputB = baseBrightness+fade;
  aOutputA = baseBrightness+(varBrightness-fade);
}


int glyphIndexForChar(const char aChar)
{
  int i = aChar-0x20;
  if (i<0 || i>=numGlyphs) i = 95; // ASCII 0x7F-0x20
  return i;
}


void renderText()
{
  // fade between rows
  byte maxBright = text_intensity-repeatCount*fade_per_repeat;
  byte thisBright, nextBright;
  crossFade(255*textCycleCount/cycles_per_px, maxBright, thisBright, nextBright);
  // generate vertical rows
  int activeCols = ledsPerLevel-2;
  // calculate text length in pixels
  int totalTextPixels = 0;
  int textLen = (int)text.length();
  for (int i=0; i<textLen; i++) {
    // sum up width of individual chars
    totalTextPixels += fontGlyphs[glyphIndexForChar(text[i])].width + glyphSpacing;
  }
  for (int x=0; x<ledsPerLevel; x++) {
    uint8_t column = 0;
    // determine font column
    if (x<activeCols) {
      int colPixelOffset = textPixelOffset + x;
      if (colPixelOffset>=0) {
        // visible column
        // - calculate character index
        int charIndex = 0;
        int glyphOffset = colPixelOffset;
        const glyph_t *glyphP = NULL;
        while (charIndex<textLen) {
          glyphP = &fontGlyphs[glyphIndexForChar(text[charIndex])];
          int cw = glyphP->width + glyphSpacing;
          if (glyphOffset<cw) break; // found char
          glyphOffset -= cw;
          charIndex++;
        }
        // now we have
        // - glyphP = the glyph,
        // - glyphOffset=column offset within that glyph (but might address a spacing column not stored in font table)
        if (charIndex<textLen) {
          // is a column of a visible char
          if (glyphOffset<glyphP->width) {
            // fetch glyph column
            column = glyphP->cols[glyphOffset];
          }
        }
      }
    }
    // now render columns
    for (int glyphRow=0; glyphRow<rowsPerGlyph; glyphRow++) {
      int i;
      int leftstep;
      if (mirrorText) {
        i = (glyphRow+1)*ledsPerLevel - 1 - x; // LED index, x-direction mirrored
        leftstep = 1;
      }
      else {
        i = glyphRow*ledsPerLevel + x; // LED index
        leftstep = -1;
      }
      if (glyphRow < rowsPerGlyph) {
        if (column & (0x40>>glyphRow)) {
          textLayer[i] = thisBright;
          // also adjust pixel left to this one
          if (x>0) {
            increase(textLayer[i+leftstep], nextBright, maxBright);
          }
          continue;
        }
      }
      textLayer[i] = 0; // no text
    }
  }
  // increment
  textCycleCount++;
  if (textCycleCount>=cycles_per_px) {
    textCycleCount = 0;
    textPixelOffset++;
    if (textPixelOffset>totalTextPixels) {
      // text shown, check for repeats
      repeatCount++;
      if (text_repeats!=0 && repeatCount>=text_repeats) {
        // done
        text = ""; // remove text
      }
      else {
        // show again
        textPixelOffset = -ledsPerLevel;
        textCycleCount = 0;
      }
    }
  }
}



// torch mode
// ==========

byte currentEnergy[maxLeds]; // current energy level
byte nextEnergy[maxLeds]; // next energy level
byte energyMode[maxLeds]; // mode how energy is calculated for this point

enum {
  torch_passive = 0, // just environment, glow from nearby radiation
  torch_nop = 1, // no processing
  torch_spark = 2, // slowly looses energy, moves up
  torch_spark_temp = 3, // a spark still getting energy from the level below
};



void resetEnergy()
{
  for (int i=0; i<numLeds; i++) {
    currentEnergy[i] = 0;
    nextEnergy[i] = 0;
    energyMode[i] = torch_passive;
  }
}




void calcNextEnergy()
{
  int i = 0;
  for (int y=0; y<levels; y++) {
    for (int x=0; x<ledsPerLevel; x++) {
      byte e = currentEnergy[i];
      byte m = energyMode[i];
      switch (m) {
        case torch_spark: {
          // loose transfer up energy as long as the is any
          reduce(e, spark_tfr);
          // cell above is temp spark, sucking up energy from this cell until empty
          if (y<levels-1) {
            energyMode[i+ledsPerLevel] = torch_spark_temp;
          }
          break;
        }
        case torch_spark_temp: {
          // just getting some energy from below
          byte e2 = currentEnergy[i-ledsPerLevel];
          if (e2<spark_tfr) {
            // cell below is exhausted, becomes passive
            energyMode[i-ledsPerLevel] = torch_passive;
            // gobble up rest of energy
            increase(e, e2);
            // loose some overall energy
            e = ((int)e*spark_cap)>>8;
            // this cell becomes active spark
            energyMode[i] = torch_spark;
          }
          else {
            increase(e, spark_tfr);
          }
          break;
        }
        case torch_passive: {
          e = ((int)e*heat_cap)>>8;
          increase(e, ((((int)currentEnergy[i-1]+(int)currentEnergy[i+1])*side_rad)>>9) + (((int)currentEnergy[i-ledsPerLevel]*up_rad)>>8));
        }
        default:
          break;
      }
      nextEnergy[i++] = e;
    }
  }
}


const uint8_t energymap[32] = {0, 64, 96, 112, 128, 144, 152, 160, 168, 176, 184, 184, 192, 200, 200, 208, 208, 216, 216, 224, 224, 224, 232, 232, 232, 240, 240, 240, 240, 248, 248, 248};

void calcNextColors()
{
  int textStart = text_base_line*ledsPerLevel;
  int textEnd = textStart+rowsPerGlyph*ledsPerLevel;
  for (int i=0; i<numLeds; i++) {
    if (i>=textStart && i<textEnd && textLayer[i-textStart]>0) {
      // overlay with text color
      leds->setColorDimmed(i, red_text, green_text, blue_text, 0, (brightness*textLayer[i-textStart])>>8);
    }
    else {
      int ei; // index into energy calculation buffer
      if (upside_down)
        ei = numLeds-i;
      else
        ei = i;
      uint16_t e = nextEnergy[ei];
      currentEnergy[ei] = e;
      if (e>250)
        leds->setColorDimmed(i, 170, 170, e, 0, brightness); // blueish extra-bright spark
      else {
        if (e>0) {
          // energy to brightness is non-linear
          byte eb = energymap[e>>3];
          byte r = red_bias;
          byte g = green_bias;
          byte b = blue_bias;
          increase(r, (eb*red_energy)>>8);
          increase(g, (eb*green_energy)>>8);
          increase(b, (eb*blue_energy)>>8);
          leds->setColorDimmed(i, r, g, b, 0, brightness);
        }
        else {
          // background, no energy
          leds->setColorDimmed(i, red_bg, green_bg, blue_bg, 0, brightness);
        }
      }
    }
  }
}


void injectRandom()
{
  // random flame energy at bottom row
  for (int i=0; i<ledsPerLevel; i++) {
    currentEnergy[i] = random(flame_min, flame_max);
    energyMode[i] = torch_nop;
  }
  // random sparks at second row
  for (int i=ledsPerLevel; i<2*ledsPerLevel; i++) {
    if (energyMode[i]!=torch_spark && random(100)<random_spark_probability) {
      currentEnergy[i] = random(spark_min, spark_max);
      energyMode[i] = torch_spark;
    }
  }
}



// this function processes parameters
int handleParams(String command)
{
  size_t p = 0;
  while (p<(int)command.length()) {
    size_t i = command.find(",", p);
    if (i==string::npos) i = command.length();
    size_t j = command.find("=",p);
    if (j==string::npos) break;
    string key = command.substr(p,j);
    string value = command.substr(j+1,i);
    int val = atoi(value.c_str());
    // global params
    if (key=="wait")
      cycle_wait = val;
    else if (key=="mode")
      mode = val;
    else if (key=="brightness")
      brightness = val;
    else if (key=="fade_base")
      fade_base = val;
    // simple lamp params
    else if (key=="lamp_red")
      lamp_red = val;
    else if (key=="lamp_green")
      lamp_green = val;
    else if (key=="lamp_blue")
      lamp_blue = val;
    // text color params
    else if (key=="red_text")
      red_text = val;
    else if (key=="green_text")
      green_text = val;
    else if (key=="blue_text")
      blue_text = val;
    // text params
    else if (key=="cycles_per_px")
      cycles_per_px = val;
    else if (key=="text_repeats")
      text_repeats = val;
    else if (key=="text_base_line")
      text_base_line = val;
    else if (key=="fade_per_repeat")
      fade_per_repeat = val;
    else if (key=="text_intensity")
      text_intensity = val;
    // clock display params
    else if (key=="clock_interval")
      clock_interval = val;
    else if (key=="clock_fmt")
      strncpy(clock_fmt, value.c_str(), 30);
    // torch color params
    else if (key=="red_bg")
      red_bg = val;
    else if (key=="green_bg")
      green_bg = val;
    else if (key=="blue_bg")
      blue_bg = val;
    else if (key=="red_bias")
      red_bias = val;
    else if (key=="green_bias")
      green_bias = val;
    else if (key=="blue_bias")
      blue_bias = val;
    else if (key=="red_energy")
      red_energy = val;
    else if (key=="green_energy")
      green_energy = val;
    else if (key=="blue_energy")
      blue_energy = val;
    // torch params
    else if (key=="spark_prob") {
      random_spark_probability = val;
      resetEnergy();
    }
    else if (key=="spark_cap")
      spark_cap = val;
    else if (key=="spark_tfr")
      spark_tfr = val;
    else if (key=="side_rad")
      side_rad = val;
    else if (key=="up_rad")
      up_rad = val;
    else if (key=="heat_cap")
      heat_cap = val;
    else if (key=="flame_min")
      flame_min = val;
    else if (key=="flame_max")
      flame_max = val;
    else if (key=="spark_min")
      spark_min = val;
    else if (key=="spark_max")
      spark_max = val;
    else if (key=="upside_down")
      upside_down = val;
    p = i+1;
  }
  return 1;
}



// Main program
// ============

static void ctrl_c_handler(int signum)
{
  // make sure LEDs are stopped before exiting
  leds->end();
}


static void setup_handlers(void)
{
  struct sigaction sa;
  sa.sa_handler = ctrl_c_handler;
  sigaction(SIGKILL, &sa, NULL);
}


#define DEFAULT_PORT_NO 4442

int commandPort = DEFAULT_PORT_NO;

static void makeNonBlocking(int aFd)
{
  int flags;
  if ((flags = fcntl(aFd, F_GETFL, 0))==-1)
    flags = 0;
  fcntl(aFd, F_SETFL, flags | O_NONBLOCK);
}


static int serverFD = -1;

static bool setup_server(void)
{
  struct sockaddr_in sin;
  int proto;
  int socketType;
  int one = 1;
  const int maxServerConnections = 1;

  memset((char *) &sin, 0, sizeof(sin));
  sin.sin_family = (sa_family_t)AF_INET;
  // set listening socket address
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  //sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  // set port
  sin.sin_port = htons(commandPort);
  // protocol derived from socket type
  //proto = IPPROTO_TCP;
  proto = IPPROTO_UDP;
  //socketType = SOCK_STREAM;
  socketType = SOCK_DGRAM;
  serverFD = socket(PF_INET, socketType, proto);
  if (serverFD<0) {
    fprintf(stderr, "Cannot create server socket: %s\n", strerror(errno));
    return false;
  }
  else {
    // socket created, set options
    if (setsockopt(serverFD,SOL_SOCKET,SO_REUSEADDR,(char *)&one,(int)sizeof(one)) == -1) {
      fprintf(stderr, "Cannot SETSOCKOPT SO_REUSEADDR: %s\n", strerror(errno));
      return false;
    }
    else {
      // options ok, bind to address
      if (bind(serverFD, (struct sockaddr *) &sin, (int)sizeof(sin)) < 0) {
        fprintf(stderr, "Cannot bind to port (server already running?): %s\n", strerror(errno));
        return false;
      }
    }
  }
  // listen
  if (socketType==SOCK_STREAM && listen(serverFD, maxServerConnections) < 0) {
    fprintf(stderr, "Cannot listen on port: %s\n", strerror(errno));
    return false;
  }
  else {
    // listen ok or not needed, make non-blocking
    makeNonBlocking(serverFD);
  }
  return true;
}


static void handle_command(string cmd)
{
  // commands start with a slash
  if (cmd[0]=='/') {
    fprintf(stderr, "**** Command received: '%s'\n", cmd.c_str());
    // process command (ugly q&d)
    if (cmd.substr(1,6)=="param ") {
      handleParams(cmd.substr(7,string::npos));
    }
  }
  else {
    // set new text to display
    fprintf(stderr, "**** New text: '%s'\n", cmd.c_str());
    newMessage(cmd);
  }
}


static void handle_data(void)
{
  // non-blocking poll
  // create poll structure
  struct pollfd *pollFds = NULL;
  size_t maxFDsToTest = 1;
  if (maxFDsToTest>0) {
    // allocate pollfd array (max, in case some are disabled, we'll need less)
    pollFds = new struct pollfd[maxFDsToTest];
  }
  // fill poll structure
  size_t numFDsToTest = 0; // start at 0
  // collect FDs
  struct pollfd *pollfdP = &pollFds[numFDsToTest];
  pollfdP->fd = serverFD;
  pollfdP->events = POLLIN;
  pollfdP->revents = 0; // no event returned so far
  ++numFDsToTest;
  // now check for data having arrived via UDP
  int numReadyFDs = poll(pollFds, (int)numFDsToTest, 0);
  if (numReadyFDs) {
    // we got data
    const int bufsize = 2000;
    uint8_t buffer[bufsize];
    ssize_t res = read(serverFD, buffer, bufsize); // read
    if (res<0) {
      if (errno==EWOULDBLOCK)
        return; // nothing received
      else {
        fprintf(stderr, "Error reading data: %s\n", strerror(errno));
        return;
      }
    }
    // process data
    string cmd;
    cmd.assign((const char *)buffer,(size_t)res);
    handle_command(cmd);
  }
}



static void usage(char *name)
{
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s [options] [text or command]...\n", name);
  fprintf(stderr, "    -P : UDP port number to listen for commands (default=%d)\n", DEFAULT_PORT_NO);
  fprintf(stderr, "         To show a text message just send the text via UDP\n");
  fprintf(stderr, "         To change a param send '/param paramname=value' via UDP\n");
  fprintf(stderr, "    -W leds per level (width) : number of LEDs in one winding/level of the torch\n");
  fprintf(stderr, "    -H levels (height) : number of windings/levels in the torch\n");
  fprintf(stderr, "    -r : reverse X direction (counter clock wound torch)\n");
  fprintf(stderr, "    -a : change X after every level\n");
  fprintf(stderr, "    -s : swap X and Y direction\n");
  fprintf(stderr, "    -m : mirror text\n");
  fprintf(stderr, "    -w : rgbW LEDs (SK6812) instead of rgb (WS2812)\n");
  fprintf(stderr, "    -h : show this help\n");
}



int main(int argc, char *argv[])
{
  int c;
  if (argc>1) {
    // reset flags
    reversedX = false;
    alternatingX = false;
    swapXY = false;
    mirrorText = false;
  }
  while ((c = getopt(argc, argv, "hP:W:H:rasmw")) != -1) {
    switch (c) {
      case 'h':
        usage(argv[0]);
        exit(0);
      case 'P':
        commandPort = atoi(optarg);
        break;
      case 'W':
        ledsPerLevel = atoi(optarg);
        break;
      case 'H':
        levels = atoi(optarg);
        break;
      case 'r':
        reversedX = true;
        break;
      case 'a':
        alternatingX = true;
        break;
      case 's':
        swapXY = true;
        break;
      case 'm':
        mirrorText = true;
        break;
      case 'w':
        rgbw = true;
        break;
      default:
        fprintf(stderr, "Error: Unknown option %c\n", c);
        usage(argv[0]);
        exit(-1);
    }
  }
  numLeds = ledsPerLevel*levels; // total number of LEDs
  textPixels = ledsPerLevel*rowsPerGlyph; // text pixels
  if (numLeds>maxLeds || textPixels>maxTextPixels) {
    fprintf(stderr, "Error: Maximum number of LEDs allowed is %d (current -W and -H would need %d LEDs)\n", maxLeds, numLeds);
    exit(-1);
  }
  // create LED driver class
  leds = new p44_ws2812(rgbw ? p44_ws2812::ledtype_sk6812 : p44_ws2812::ledtype_ws2812, numLeds, swapXY ? levels : ledsPerLevel, reversedX, alternatingX, swapXY); // create WS2812/SK6812 driver
  if (!leds) {
    fprintf(stderr, "Error: cannot allocate LED driver\n");
    exit(-1);
  }
  // now run
  int ret = 0;
  int cnt = 0;
  // make sure ctrl-c will stop DMA before quitting app
  setup_handlers();
  // reset display
  resetEnergy();
  resetText();
  if (!leds->begin()) {
    fprintf(stderr, "Error: cannot start LED driver\n");
    exit(-1);
  }
  // process text and commands passed on command line
  while (optind<argc) {
    handle_command(argv[optind++]);
  }
  // setup UDP server for remote control
  setup_server();
  // loop
  while (1) {
    // check clock display
    if (clock_interval>0) {
      time_t now = time(NULL);
      struct tm *loc;
      loc = localtime(&now);
      int secOfHour = loc->tm_min*60 + loc->tm_sec;
      if (secOfHour % clock_interval == 0) {
         // seconds of hour evenly dividable by clock_interval -> display time now
         char timeString[30];
         strftime(timeString, 30, clock_fmt, loc);
         newMessage(timeString);
      }
    }
    // render the text
    renderText();
    int textStart = text_base_line*ledsPerLevel;
    int textEnd = textStart+rowsPerGlyph*ledsPerLevel;
    switch (mode) {
      case mode_off: {
        // off
        for(int i=0; i<leds->getNumLeds(); i++) {
          leds->setColor(i, 0, 0, 0, 0);
        }
        break;
      }
      case mode_lamp: {
        // just single color lamp + text display
        for (int i=0; i<leds->getNumLeds(); i++) {
          if (i>=textStart && i<textEnd && textLayer[i-textStart]>0) {
            leds->setColorDimmed(i, red_text, green_text, blue_text, 0, (textLayer[i-textStart]*brightness)>>8);
          }
          else {
            leds->setColorDimmed(i, lamp_red, lamp_green, lamp_blue, 0, brightness);
          }
        }
        break;
      }
      case mode_torch: {
        // torch animation + text display + cheerlight background
        injectRandom();
        calcNextEnergy();
        calcNextColors();
        break;
      }
      case mode_colorcycle: {
        // simple color wheel animation
        cnt++;
        byte r,g,b;
        for(int i=0; i<leds->getNumLeds(); i++) {
          wheel(((i * 256 / leds->getNumLeds()) + cnt) & 255, r, g, b);
          if (i>=textStart && i<textEnd && textLayer[i-textStart]>0) {
            leds->setColorDimmed(i, r, g, b, 0, (textLayer[i-textStart]*brightness)>>8);
          }
          else {
            leds->setColorDimmed(i, r, g, b, 0, brightness>>1); // only half brightness for full area color
          }
        }
        break;
      }
    }
    // show
    leds->show();
    // check for data
    handle_data();
    // wait
    usleep(25000*cycle_wait); // 25mS minimal cycle
  }
  leds->end();
  return ret;
}


