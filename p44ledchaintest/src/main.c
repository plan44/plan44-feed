//
//  main.c
//  p44ledchaintest
//
//  Copyright © 2020 plan44.ch. All rights reserved.
//

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h> // for IOSSIOSPEED
#endif


#define DEFAULT_NUMLEDS 720
#define DEFAULT_UPDATEINTERVAL_MS 30
#define DEFAULT_FGCOLOR "FF0000"
#define DEFAULT_BGCOLOR "000040"
#define DEFAULT_COLORSTEP "000000"
#define DEFAULT_NUMREPEATS 1
#define DEFAULT_EFFECTINC 1

static void usage(const char *name)
{
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  %s [options] ledchaindevice [ledchaindevice, ...]\n", name);
  fprintf(stderr, "    -n numleds : number of LEDs per chain (default: %d)\n", DEFAULT_NUMLEDS);
  fprintf(stderr, "    -2 : LED needs 2 bytes per channel (default=1)\n");
  fprintf(stderr, "    -4 : LED has 4 channles (default=3)\n");
  fprintf(stderr, "    -U : led chain device is an UART, using 7-N-1 @ 3Mbaud, inverse WS28xx at TX\n");
  fprintf(stderr, "    -o : (only with -U) old, slower led chip (WS2812) timing, 7-1-N @ 2.5Mbaud\n");
  fprintf(stderr, "    -g : use gamma correction for output\n");
  fprintf(stderr, "    -i interval[ms] : update interval (default: %d)\n", DEFAULT_UPDATEINTERVAL_MS);
  fprintf(stderr, "    -r repeats : how many repeated updates, 0=continuously, (default: %d)\n", DEFAULT_NUMREPEATS);
  fprintf(stderr, "    -e inc : effect increment, (default: %d)\n", DEFAULT_EFFECTINC);
  fprintf(stderr, "    -c rrggbb[ww] : hex color (default: %s)\n", DEFAULT_FGCOLOR);
  fprintf(stderr, "    -C rrr,ggg,bbb[,www] : decimal color power (direct PWM output)\n");
  fprintf(stderr, "    -b rrggbb[ww] : alternate (background) hex color (default: %s)\n", DEFAULT_BGCOLOR);
  fprintf(stderr, "    -s rrggbb[ww] : color increment added at each step (default: %s)\n", DEFAULT_COLORSTEP);
  fprintf(stderr, "    -H ooccttttrr : send header data (needed for ledchain in variable led type mode)\n");
  fprintf(stderr, "       (for p44-ledchain >=v6: oo=layout, cc=chip, tttt=tmaxpassive, rr=maxretries)\n");
  fprintf(stderr, "    -F : fill up / empty led chain with foreground color\n");
  fprintf(stderr, "    -S : single wandering LED with foreground color\n");
  fprintf(stderr, "    -v : verbose\n");
}


static long long now()
{
  #if defined(__APPLE__) && __DARWIN_C_LEVEL < 199309L
  // pre-10.12 MacOS does not yet have clock_gettime
  static bool timeInfoKnown = false;
  static mach_timebase_info_data_t tb;
  if (!timeInfoKnown) {
    mach_timebase_info(&tb);
  }
  double t = mach_absolute_time();
  return t * (double)tb.numer / (double)tb.denom / 1e3; // uS
  #else
  // platform has clock_gettime
  struct timespec tsp;
  clock_gettime(CLOCK_MONOTONIC, &tsp);
  // return microseconds
  return ((uint64_t)(tsp.tv_sec))*1000000ll + tsp.tv_nsec/1000; // uS
  #endif
}


static uint16_t pwmtable[256] = {
      0,    19,    39,    59,    79,   100,   121,   142,   163,   185,   208,   230,   253,   277,   300,   324,
    349,   374,   399,   425,   451,   477,   504,   531,   559,   587,   616,   645,   674,   704,   735,   766,
    797,   829,   862,   894,   928,   962,   996,  1032,  1067,  1103,  1140,  1178,  1216,  1254,  1293,  1333,
   1373,  1414,  1456,  1498,  1542,  1585,  1630,  1675,  1721,  1767,  1814,  1862,  1911,  1961,  2011,  2062,
   2114,  2167,  2220,  2275,  2330,  2386,  2443,  2501,  2560,  2620,  2681,  2742,  2805,  2869,  2933,  2999,
   3066,  3134,  3203,  3273,  3344,  3416,  3489,  3564,  3639,  3716,  3794,  3874,  3954,  4036,  4119,  4204,
   4289,  4377,  4465,  4555,  4646,  4739,  4833,  4929,  5026,  5125,  5226,  5328,  5431,  5536,  5643,  5752,
   5862,  5974,  6088,  6203,  6321,  6440,  6561,  6684,  6809,  6936,  7065,  7196,  7329,  7465,  7602,  7741,
   7883,  8027,  8173,  8322,  8473,  8626,  8782,  8940,  9101,  9264,  9430,  9598,  9769,  9943, 10119, 10299,
  10481, 10666, 10854, 11045, 11239, 11436, 11636, 11839, 12046, 12255, 12469, 12685, 12905, 13128, 13355, 13586,
  13820, 14058, 14299, 14545, 14794, 15047, 15304, 15566, 15831, 16101, 16374, 16653, 16935, 17222, 17514, 17810,
  18111, 18417, 18727, 19043, 19363, 19689, 20019, 20355, 20696, 21043, 21395, 21752, 22115, 22484, 22859, 23240,
  23627, 24020, 24419, 24824, 25236, 25654, 26079, 26511, 26949, 27395, 27847, 28307, 28773, 29248, 29729, 30219,
  30716, 31221, 31734, 32255, 32784, 33322, 33868, 34423, 34986, 35559, 36140, 36731, 37331, 37940, 38560, 39189,
  39827, 40476, 41136, 41805, 42486, 43177, 43879, 44592, 45316, 46052, 46799, 47558, 48330, 49113, 49909, 50717,
  51538, 52373, 53220, 54081, 54955, 55843, 56745, 57662, 58593, 59539, 60499, 61475, 62466, 63473, 64496, 65535
};


// globals
// - options and params from cmdline
int numleds = DEFAULT_NUMLEDS;
int chanbytes = 1;
int numchannels = 3;
int pwmgamma = 0;
int interval = DEFAULT_UPDATEINTERVAL_MS;
uint16_t fgcolor[4] = { 0xFFFF, 0, 0, 0 };
uint16_t bgcolor[4] = { 0, 0, 0x4000, 0 };
uint16_t colorstep[4] = { 0, 0, 0, 0 };
int effectinc = DEFAULT_EFFECTINC;
int repeats = DEFAULT_NUMREPEATS;
int verbose = 0;
int uart = 0;
int oldchiptiming = 0;
// - run time
uint8_t *ledbuffer = NULL;



enum {
  mode_static, // static foreground fill
  mode_fillup, // fill and empty chain with foreground
  mode_single, // single wandering LED with foreground color
};
int mode = mode_static;

const int maxchains = 4;
const int maxhdrlen = 20;


void setLed(int ledidx, uint16_t* color)
{
  int bidx, ch;
  if (pwmgamma) {
    // use PWM gamma (exponent 4) table from color MSB -> 16bit PWM
    for (ch=0; ch<numchannels; ch++) {
      for (bidx=0; bidx<chanbytes; bidx++) {
        ledbuffer[(ledidx*numchannels+ch)*chanbytes+bidx] = (bidx==0 ? pwmtable[color[ch]>>8]>>8 : pwmtable[color[ch]>>8])&0xFF;
      }
    }
  }
  else {
    // put color 1:1 into MSB (and LSB if chanbytes>1)
    for (ch=0; ch<numchannels; ch++) {
      for (bidx=0; bidx<chanbytes; bidx++) {
        ledbuffer[(ledidx*numchannels+ch)*chanbytes+bidx] = bidx==0 ? color[ch]>>8 : color[ch]&0xFF;
      }
    }
  }
}


void scanhexcol(const char* arg, uint16_t* col)
{
  uint16_t col8[4];
  int i;
  int n = sscanf(optarg, "%2hx%2hx%2hx%2hx", &col8[0], &col8[1], &col8[2], &col8[3]);
  for (i=0; i<n; i++) {
    col[i] = col8[i]<<8;
  }
  for (i=n; i<4; i++) {
    col[i] = 0;
  }
}


// set to see data dumps on stdout
#define UARTDEBUG 0

static int setupUart(int aUartFd, int aSlowTiming)
{
  int ret = 0;

  #ifdef __APPLE__
  // macOS does not have non-standard baud rate codes, but separate speed setting API
  int baudRateCode = B9600; // dummy but standard
  speed_t speed = aSlowTiming ? 2500000 : 3000000; // actual speed we need
  #else
  int baudRateCode = aSlowTiming ? B2500000 : B3000000;
  #endif
  struct termios newtio;
  memset(&newtio, 0, sizeof(newtio));
  // - charsize, stopbits, parity, no modem control lines (local), reading enabled
  newtio.c_cflag =
    CS7 |
    0 | // !CSTOPB : no second stop bit
    0 | // !PARENB and !PARODD : no parity
    CLOCAL | // ignore status lines
    0; // !CREAD : do not enable receiver
  // - ignore parity errors (just to make sure)
  newtio.c_iflag = IGNPAR;
  // - no output control
  newtio.c_oflag = 0;
  // - no input control (non-canonical)
  newtio.c_lflag = 0;
  // - no inter-char time
  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  // - receive every single char seperately
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */
  // - set speed (as this ors into c_cflag, this must be after setting c_cflag initial value)
  cfsetspeed(&newtio, baudRateCode);
  tcflush(aUartFd, TCIFLUSH);
  // now apply new settings
  ret = tcsetattr(aUartFd, TCSANOW, &newtio);
  #ifdef __APPLE__
  // on macOS, we need to set the non-standard speed separately
  if (ret>=0) {
    ret = ioctl(aUartFd, IOSSIOSPEED, &speed);
  }
  #endif // __APPLE__
  return ret;
}



void sendToUart(int aUartFd, uint8_t* aLedData, size_t aLedDataSize)
{
  // We can generate appropriate WS28xx timing using 7-N-1 @ 3Mhz
  // - UART are LSBit first
  // - WS28xx are MSBit first
  // - we need 1 byte output for 3 bits LED data = 8 byte output for 3 byte LED data
  // - UART idle is H, start bit is L, stop bit is H
  // - as we need idle=L, start=H, stop=L for WS, we need to invert the UART data, and send inverted data
  // UART   : | start | Bit0 | Bit1 | Bit2 | Bit3 | Bit4 | Bit5 | Bit6 | Stop |
  // WS28xx : |   1   | LED7 |   0  |   1  | LED6 |   0  |   1  | LED5 |   0  |
  // WS28xx : |   1   | LED4 |   0  |   1  | LED3 |   0  |   1  | LED2 |   0  |
  // WS28xx : |   1   | LED1 |   0  |   1  | LED0 |   0  |   1  | next |   0  |
  size_t uartDataSize = (aLedDataSize*8+2)/3;
  uint8_t* uartData = malloc(uartDataSize);
  uint8_t* outP = uartData;
  int outBitMask = 0x01; // first LED data bit in UART output
  uint8_t uartByte = 0;
  #if UARTDEBUG
  size_t i;
  printf("leddata[%d]: ", aLedDataSize);
  for (i = 0; i<aLedDataSize; i++) {
     printf(" %02X", aLedData[i]);
  }
  printf("\n");
  #endif
  while(aLedDataSize-- > 0) {
    uint8_t ledByte = *aLedData++;
    uint8_t ledBitMask = 0x80;
    while (ledBitMask) {
      if (outBitMask>1) uartByte |= (outBitMask>>1); // 2nd or 3rd bit in this uart byte: enable the bit position (not idle) by setting the always 1 sync bit (T0H)
      if (ledByte & ledBitMask) uartByte |= outBitMask; // actual data bit, extending T0H to become T1H in case bit is set
      ledBitMask >>= 1; // next input bit
      outBitMask <<= 3; // next output bit position
      if ((outBitMask & 0x7F)==0) {
        // uart byte complete
        *(outP++) = uartByte^0x7F; // send inverted, but only 7 bits
        uartByte = 0;
        outBitMask = 0x01;
      }
    }
  }
  // send
  write(aUartFd, uartData, uartDataSize);
  tcflush(aUartFd, TCIFLUSH);
  #if UARTDEBUG
  printf("uartdata[%d]: ", uartDataSize);
  outP = uartData;
  while(uartDataSize>0) {
    printf(" %02X", *(outP++));
    uartDataSize--;
  }
  printf("\n");
  #endif
  // done
  free(uartData);
}


int main(int argc, char **argv)
{
  int chainFds[maxchains];
  int numchains;
  int loopidx, cidx, lidx, eidx, bidx;
  struct timespec ts;
  uint8_t *rawbuffer;
  const char* headerStr = NULL;
  int hdrlen = 0;

  long long start;
  long long loopStart;
  long long beforeUpdate;
  long long afterUpdate;
  long long afterSleep;
  long long afterPrint;
  long long lastAfterSleep;
  long long total;
  long long wait;

  if (argc<2) {
    // show usage
    usage(argv[0]);
    exit(1);
  }

  int c;
  while ((c = getopt(argc, argv, "hUoH:n:24gi:e:r:c:C:b:s:vFS")) != -1)
  {
    switch (c) {
      case 'h':
        usage(argv[0]);
        exit(0);
      case 'U':
        uart = 1;
        break;
      case 'o':
        oldchiptiming = 1;
        break;
      case 'n':
        numleds = atoi(optarg);
        break;
      case '2':
        chanbytes = 2;
        break;
      case '4':
        numchannels = 4;
        break;
      case 'g':
        pwmgamma = 1;
        break;
      case 'i':
        interval = atoi(optarg);
        break;
      case 'e':
        effectinc = atoi(optarg);
        break;
      case 'r':
        repeats = atoi(optarg);
        break;
      case 'c':
        scanhexcol(optarg, fgcolor);
        break;
      case 'C':
        sscanf(optarg, "%hd,%hd,%hd,%hd", &fgcolor[0], &fgcolor[1], &fgcolor[2], &fgcolor[3]);
        break;
      case 'b':
        scanhexcol(optarg, bgcolor);
        break;
      case 's':
        scanhexcol(optarg, colorstep);
        break;
      case 'H':
        headerStr = optarg;
        break;
      case 'v':
        verbose = 1;
        break;
      case 'F':
        mode = mode_fillup;
        break;
      case 'S':
        mode = mode_single;
        break;
      default:
        exit(-1);
    }
  }
  // open chains
  numchains = 0;
  while (optind<argc) {
    chainFds[numchains] = open(argv[optind], O_RDWR|O_NONBLOCK);
    if (chainFds[numchains]<0) {
      fprintf(stderr, "cannot open ledchain device '%s': %s\n", argv[optind], strerror(errno));
      exit(1);
    }
    else if (uart) {
      // must also set uart params
      if (setupUart(chainFds[numchains], oldchiptiming)<0) {
        fprintf(stderr, "UART mode. Cannot set serial speed and options for '%s': %s\n", argv[optind], strerror(errno));
        exit(1);
      }
    }
    numchains++;
    optind++;
  }
  if (numchains<1) {
    fprintf(stderr, "must specify at least one LED chain device\n");
    exit(1);
  }
  // allocate buffer
  rawbuffer = malloc(numleds*numchannels*chanbytes+maxhdrlen+1);
  ledbuffer = rawbuffer;
  // maybe we have a header
  if (headerStr) {
    ledbuffer++; // room for length
    hdrlen = 1;
    while (hdrlen<maxhdrlen && sscanf(headerStr, "%2hhx", ledbuffer)==1) {
      headerStr +=2;
      hdrlen++;
      ledbuffer++;
    }
    *rawbuffer = hdrlen-1;
  }
  // loop
  start = now();
  eidx = 0; // effect index
  for (loopidx = 0; repeats==0||loopidx<repeats; loopidx++) {
    loopStart = now();
    // prepare pattern
    switch(mode) {
      case mode_static: {
        for (lidx=0; lidx<numleds; lidx++) {
          setLed(lidx, fgcolor);
        }
        break;
      }
      case mode_fillup:
      {
        for (lidx=0; lidx<(eidx%numleds); lidx++) {
          setLed(lidx, fgcolor);
        }
        for (; lidx<numleds; lidx++) {
          setLed(lidx, bgcolor);
        }
        break;
      }
      case mode_single:
      {
        for (lidx=0; lidx<numleds; lidx++) {
          if ((eidx%numleds)==lidx) {
            setLed(lidx, fgcolor);
          }
          else {
            setLed(lidx, bgcolor);
          }
        }
        break;
      }
    }
    // update chains
    beforeUpdate = now();
    for (cidx = 0; cidx<numchains; cidx++) {
      if (uart) {
        // UART, need to
        sendToUart(chainFds[cidx], rawbuffer, numleds*numchannels*chanbytes+hdrlen);
      }
      else {
        // raw LED device, can send data directly
        write(chainFds[cidx], rawbuffer, numleds*numchannels*chanbytes+hdrlen);
      }
    }
    afterUpdate = now();
    // calculate remaining wait time
    wait = interval*1000 - (afterUpdate-loopStart);
    // wait
    ts.tv_sec = wait/1000000;
    ts.tv_nsec = (wait%1000000)*1000;
    nanosleep(&ts, NULL);
    lastAfterSleep = afterSleep;
    afterSleep = now();
    total = now()-start;
    // statistics
    if (verbose) {
      printf("Loop #%d: TOTAL:%lld, average loop: %lld - THIS loop:%lld, generate: %lld, update: %lld, wait: %lld, prev. printf: %lld [µS]\n",
        loopidx,
        total,
        total/(loopidx+1),
        afterSleep-loopStart,
        beforeUpdate-loopStart,
        afterUpdate-beforeUpdate,
        afterSleep-afterUpdate,
        afterPrint-lastAfterSleep
      );
    }
    afterPrint = now();
    eidx += effectinc;
    fgcolor[0] += colorstep[0];
    fgcolor[1] += colorstep[1];
    fgcolor[2] += colorstep[2];
    fgcolor[3] += colorstep[3];
  }
  printf("TOTAL time: %lld, average per loop: %lld [microseconds]\n", total, total/loopidx);
  // close
  for (cidx = 0; cidx<numchains; cidx++) {
    close(chainFds[cidx]);
  }
  // free buffer
  free(rawbuffer); rawbuffer = NULL; ledbuffer = NULL;
  // done
  exit(0);
}
