//
//  Copyright (c) 2016-2019 plan44.ch / Lukas Zeller, Zurich, Switzerland
//
//  Author: Lukas Zeller <luz@plan44.ch>
//
//  This file implements a external device for the vdcd project. It
//  uses p44utils which are free software licensed under GPLv3,
//  so this sample code and code derived from it also fall under
//  the terms of GPLv3.
//
//  You should have received a copy of the GNU General Public License
//  along with vdcd. If not, see <http://www.gnu.org/licenses/>.
//

#include "application.hpp"

#include "serialcomm.hpp"
#include "jsoncomm.hpp"

using namespace p44;

// set this to a unique string for your particular app or use --uniqueid command line parameter
#define DEFAULT_UNIQUE_ID "ElsnerP03ExternalDeviceApp"

#define DEFAULT_API_HOST "localhost"
#define DEFAULT_API_SERVICE "8999"
#define DEFAULT_LOGLEVEL LOG_NOTICE

#define MAINLOOP_CYCLE_TIME_uS 33333 // 33mS


/// Main program for plan44.ch P44-DSB-DEH in form of the "vdcd" daemon)
class ExternalDeviceApp : public CmdLineApp
{
  typedef CmdLineApp inherited;

  string uniqueId;
  JsonCommPtr deviceConnection;

  string weatherStationSerialPort;

  SerialCommPtr serial;
  int telegramIndex;
  int checksum;
  static const size_t maxTelegramBytes = 100; // max known: Elsner P04/3-RS485-GPS has 61, but reserve some for future versions
  uint8_t telegram[maxTelegramBytes];

  // config
  bool hasSunDirections;

  // sensor indices
  int temperatureSensorIndex;
  int sunlightSensorIndex;
  int sunWestSensorIndex;
  int sunEastSensorIndex;
  int daylightSensorIndex;
  int windSensorIndex;
  int gustSensorIndex;
  // input indices
  int twilightInputIndex;
  int rainInputIndex;

  // log to API
  static void logToApi(void *aContextPtr, int aLevel, const char *aLinePrefix, const char *aLogMessage)
  {
    ExternalDeviceApp *app = static_cast<ExternalDeviceApp *>(aContextPtr);
    JsonObjectPtr msg = JsonObject::newObj();
    msg->add("message", JsonObject::newString("log"));
    // - index
    msg->add("level", JsonObject::newInt32(aLevel));
    // - value
    msg->add("text", JsonObject::newString(aLogMessage));
    // Send message
    if (app->deviceConnection) {
      app->deviceConnection->sendMessage(msg);
    }
  }

public:

  ExternalDeviceApp() :
    hasSunDirections(true)
  {
  }

  virtual int main(int argc, char **argv)
  {
    const char *usageText =
    "Usage: %1$s [options]\n";
    const CmdLineOptionDescriptor options[] = {
      // specific to P03 weather station example
      { 's', "serialport",      true,  "serialport;serial port to which the Elsner P03 or P04 weather station is connected" },
      { 0  , "singlesunsensor", false, "weather station only has singe sun sensor" },
      // generic
      { 'h', "apihost",         true,  "hostname/IP;specifies vdcd external device API host to connect to, defaults to " DEFAULT_API_HOST },
      { 'p', "apiport",         true,  "port;external device API port or local socket name, defaults to " DEFAULT_API_SERVICE },
      { 'i', "unqiueid",        true,  "UUID/unique string;device's dSUID is derived from this, if UUID is used, it must be globally unique, "
                                       "if other string is used it must be unique for the vdcd it connects to. Defaults to " DEFAULT_UNIQUE_ID },
      { 'l', "loglevel",        true,  "level;set max level of log message detail to show on stdout" },
      { 0  , "logtoapi",        false, "log via API log command, so log messages will appear in vdcd log" },
      { 'h', "help",            false, "show this text" },
      { 0, NULL } // list terminator
    };

    // parse the command line, exits when syntax errors occur
    setCommandDescriptors(usageText, options);
    parseCommandLine(argc, argv);
    if (numArguments()>0) {
      // show usage
      showUsage();
      terminateApp(EXIT_FAILURE);
    }
    // set log level
    int loglevel = DEFAULT_LOGLEVEL;
    getIntOption("loglevel", loglevel);
    SETLOGLEVEL(loglevel);
    if (getOption("logtoapi")) {
      SETLOGHANDLER(logToApi, this);
    }
    // create device connection
    deviceConnection = JsonCommPtr(new JsonComm(MainLoop::currentMainLoop()));
    // - get parameters for device connection
    string hostname = DEFAULT_API_HOST;
    string service = DEFAULT_API_SERVICE;
    getStringOption("apihost", hostname);
    getStringOption("apiport", service);
    deviceConnection->setConnectionParams(hostname.c_str(), service.c_str());
    // - get uniqueID to use
    uniqueId = DEFAULT_UNIQUE_ID;
    getStringOption("unqiueid", uniqueId);
    // - get serial port
    getStringOption("serialport", weatherStationSerialPort);
    // - sun directions?
    hasSunDirections = !getOption("singlesunsensor");
    // run
    return run();
  };


  virtual void initialize()
  {
    // open plan44 vdcd external device API connection
    // - set handlers first
    deviceConnection->setConnectionStatusHandler(boost::bind(&ExternalDeviceApp::connectionStatusHandler, this, _2));
    deviceConnection->setMessageHandler(boost::bind(&ExternalDeviceApp::jsonMessageHandler, this, _1, _2));
    // - initiate connection. Status handler will be called if it fails
    deviceConnection->initiateConnection();
  };


  virtual void cleanup(int aExitCode)
  {
  }


  void connectionStatusHandler(ErrorPtr aError)
  {
    if (Error::isOK(aError)) {
      // connection successfully established, init device now
      initDevice();
    }
    else {
      // connecion failed, exit
      terminateAppWith(aError);
    }
  }


  void initDevice()
  {
    // create vdcd external device API init message
    JsonObjectPtr initMsg = JsonObject::newObj();
    initMsg->add("message", JsonObject::newString("init"));
    // - unique ID
    initMsg->add("uniqueid", JsonObject::newString(uniqueId));
    // - always JSON
    initMsg->add("protocol", JsonObject::newString("json"));
    // - primary group: 8 = black
    initMsg->add("group", JsonObject::newInt32(8));
    // - model and name
    initMsg->add("modelname", JsonObject::newString("Elsner P03/P04"));
    initMsg->add("name", JsonObject::newString("Elsner P03/P04 via RS485"));
    // - binary inputs
    JsonObjectPtr inputs = JsonObject::newArray();
    JsonObjectPtr i;
    //   - twilight indicator
    i = JsonObject::newObj();
    i->add("inputtype", JsonObject::newInt32(4)); // 4=twilight sensor
    i->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    i->add("group", JsonObject::newInt32(1)); // 1=yellow/light
    i->add("hardwarename", JsonObject::newString("Twilight"));
    twilightInputIndex = inputs->arrayLength();
    inputs->arrayAppend(i);
    //   - rain indicator
    i = JsonObject::newObj();
    i->add("inputtype", JsonObject::newInt32(9)); // 9=rain detector
    i->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    i->add("group", JsonObject::newInt32(8)); // 8=black
    i->add("hardwarename", JsonObject::newString("Rain"));
    rainInputIndex = inputs->arrayLength();
    inputs->arrayAppend(i);
    //   - add to init message
    initMsg->add("inputs", inputs);
    // - sensors
    JsonObjectPtr sensors = JsonObject::newArray();
    JsonObjectPtr s;
    //   - low daylight sensor, only up to 999lx
    s = JsonObject::newObj();
    s->add("sensortype", JsonObject::newInt32(3)); // 3=illumination
    s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    s->add("group", JsonObject::newInt32(1)); // 1=yellow/light
    s->add("hardwarename", JsonObject::newString("Low Daylight"));
    s->add("min", JsonObject::newDouble(0));
    s->add("max", JsonObject::newDouble(999));
    s->add("resolution", JsonObject::newDouble(1));
    s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
    daylightSensorIndex = sensors->arrayLength();
    sensors->arrayAppend(s);
    //   - outdoor temperature
    s = JsonObject::newObj();
    s->add("sensortype", JsonObject::newInt32(1)); // 1=temperature
    s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    s->add("group", JsonObject::newInt32(3)); // 3=blue/climate
    s->add("hardwarename", JsonObject::newString("Outdoor Temperature"));
    s->add("min", JsonObject::newDouble(-40));
    s->add("max", JsonObject::newDouble(80));
    s->add("resolution", JsonObject::newDouble(0.1));
    s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
    temperatureSensorIndex = sensors->arrayLength();
    sensors->arrayAppend(s);
    //   - wind speed sensor
    s = JsonObject::newObj();
    s->add("sensortype", JsonObject::newInt32(13)); // 13=wind speed
    s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    s->add("group", JsonObject::newInt32(3)); // 3=blue/climate
    s->add("hardwarename", JsonObject::newString("Wind Speed"));
    s->add("min", JsonObject::newDouble(0));
    s->add("max", JsonObject::newDouble(99));
    s->add("resolution", JsonObject::newDouble(0.1));
    s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
    windSensorIndex = sensors->arrayLength();
    sensors->arrayAppend(s);
    //   - gust speed sensor
    s = JsonObject::newObj();
    s->add("sensortype", JsonObject::newInt32(23)); // 23=gust speed
    s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    s->add("group", JsonObject::newInt32(3)); // 3=blue/climate
    s->add("hardwarename", JsonObject::newString("Gust Speed"));
    s->add("min", JsonObject::newDouble(0));
    s->add("max", JsonObject::newDouble(99));
    s->add("resolution", JsonObject::newDouble(0.1));
    s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
    gustSensorIndex = sensors->arrayLength();
    sensors->arrayAppend(s);
    //   - sunlight (south or general sun when there is no east and west)
    s = JsonObject::newObj();
    s->add("id", JsonObject::newString(hasSunDirections ? "sun_south" : "sunlight"));
    s->add("sensortype", JsonObject::newInt32(3)); // 3=illumination
    s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
    s->add("group", JsonObject::newInt32(1)); // 1=yellow/light
    s->add("hardwarename", JsonObject::newString(hasSunDirections ? "Sun South" : "Sunlight"));
    s->add("min", JsonObject::newDouble(0));
    s->add("max", JsonObject::newDouble(99000));
    s->add("resolution", JsonObject::newDouble(1000));
    s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
    sunlightSensorIndex = sensors->arrayLength();
    sensors->arrayAppend(s);
    if (hasSunDirections) {
      // - sun west
      s = JsonObject::newObj();
      s->add("id", JsonObject::newString("sun_west"));
      s->add("sensortype", JsonObject::newInt32(3)); // 3=illumination
      s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
      s->add("group", JsonObject::newInt32(1)); // 1=yellow/light
      s->add("hardwarename", JsonObject::newString("Sun West"));
      s->add("min", JsonObject::newDouble(0));
      s->add("max", JsonObject::newDouble(99999));
      s->add("resolution", JsonObject::newDouble(1000));
      s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
      sunWestSensorIndex = sensors->arrayLength();
      sensors->arrayAppend(s);
      // - sun east
      s = JsonObject::newObj();
      s->add("id", JsonObject::newString("sun_east"));
      s->add("sensortype", JsonObject::newInt32(3)); // 3=illumination
      s->add("usage", JsonObject::newInt32(2)); // 2=outdoors
      s->add("group", JsonObject::newInt32(1)); // 1=yellow/light
      s->add("hardwarename", JsonObject::newString("Sun East"));
      s->add("min", JsonObject::newDouble(0));
      s->add("max", JsonObject::newDouble(99999));
      s->add("resolution", JsonObject::newDouble(1000));
      s->add("updateinterval", JsonObject::newInt32(3)); // updates every 3 seconds
      sunEastSensorIndex = sensors->arrayLength();
      sensors->arrayAppend(s);
    }
    //   - add to init message
    initMsg->add("sensors", sensors);
    // Send init message
    deviceConnection->sendMessage(initMsg);
    // now initialize hardware
    serial = SerialCommPtr(new SerialComm(MainLoop::currentMainLoop()));
    serial->setConnectionSpecification(weatherStationSerialPort.c_str(), 2103, "19200,8,N,1");
    serial->setReceiveHandler(boost::bind(&ExternalDeviceApp::serialReceiveHandler, this, _1));
    serial->establishConnection();
    telegramIndex = -1;
  }


  void jsonMessageHandler(ErrorPtr aError, JsonObjectPtr aJsonObject)
  {
    if (!Error::isOK(aError)) {
      // error, exit
      terminateAppWith(aError);
      return;
    }
    // process messages
    LOG(LOG_NOTICE, "Received message: %s", aJsonObject->c_strValue());
    // Note: weather station does not process any messages
  }



  void reportSensor(int aIndex, double aValue)
  {
    LOG(LOG_INFO, "Reporting sensor[%d]: %.1f", aIndex, aValue);
    // create vdcd external device API channel update message
    JsonObjectPtr msg = JsonObject::newObj();
    msg->add("message", JsonObject::newString("sensor"));
    // - index
    msg->add("index", JsonObject::newInt32(aIndex));
    // - value
    msg->add("value", JsonObject::newDouble(aValue));
    // Send message
    deviceConnection->sendMessage(msg);
  }


  void reportInput(int aIndex, bool aState)
  {
    LOG(LOG_INFO, "Reporting input[%d]: %d", aIndex, aState);
    // create vdcd external device API channel update message
    JsonObjectPtr msg = JsonObject::newObj();
    msg->add("message", JsonObject::newString("input"));
    // - index
    msg->add("index", JsonObject::newInt32(aIndex));
    // - value
    msg->add("value", JsonObject::newDouble(aState ? 1 : 0));
    // Send message
    deviceConnection->sendMessage(msg);
  }


  void serialReceiveHandler(ErrorPtr aError)
  {
    if (!Error::isOK(aError)) {
      LOG(LOG_ERR, "serialReceiveHandler error: %s", aError->description().c_str());
    }
    else {
      size_t n = serial->numBytesReady();
      LOG(LOG_DEBUG, "serialReceiveHandler sees %zu bytes ready", n);
      while (n-->0) {
        uint8_t byte;
        serial->receiveBytes(1, &byte, aError);
        if (!Error::isOK(aError)) {
          LOG(LOG_ERR, "receiveBytes error: %s", aError->description().c_str());
        }
        else {
          LOG(LOG_DEBUG, "- processing byte 0x%02X '%c' ", byte, byte<0x20 || byte>0x7E ? '.' : (char)byte);
          if (telegramIndex<0) {
            // wait for start of telegram (G for P04/3-RS485-GPS, W for all other models)
            if (byte=='W' || byte=='G') {
              telegramIndex = 0;
              telegram[telegramIndex++] = byte;
              LOG(LOG_DEBUG, "Found beginning of telegram");
            }
          }
          else {
            // within telegram
            if (byte!=0x03) {
              if (telegramIndex<maxTelegramBytes) {
                // just accumulate
                telegram[telegramIndex++] = byte;
              }
              else {
                // too long -> discard
                LOG(LOG_WARNING, "Telegram too long, (%d bytes) -> discard", telegramIndex);
                telegramIndex = -1;
              }
            }
            else {
              LOG(LOG_DEBUG, "Found end of telegram (ETX) at index %d", telegramIndex);
              // verify checksum: all bytes excluding start byte, end byte and checksum itself
              int checksum = 0;
              for (int i=0; i<telegramIndex-4; ++i) {
                checksum += telegram[i];
              }
              int repchecksum =
                (telegram[telegramIndex-4]-'0')*1000 +
                (telegram[telegramIndex-3]-'0')*100 +
                (telegram[telegramIndex-2]-'0')*10 +
                (telegram[telegramIndex-1]-'0');
              if (repchecksum!=checksum) {
                LOG(LOG_WARNING, "Telegram checksum wrong: expected %04d, received %04d -> discarded", checksum, repchecksum);
              }
              else if (telegramIndex<21) {
                LOG(LOG_ERR, "Telegram with correct checksum, but too short (only %d bytes, unknown P0x version?) -> discarded", telegramIndex);
              }
              else {
                LOG(LOG_DEBUG, "Telegram with correct checksum received -> analyze now");
                // evaluate
                // TODO: checksum
                // - temperature
                double temp =
                  (telegram[2]-'0')*10 +
                  (telegram[3]-'0')*1 +
                  (telegram[5]-'0')*0.1;
                if (telegram[1]=='-') temp = -temp;
                reportSensor(temperatureSensorIndex, temp);
                // - sun (south)
                double sun =
                  (telegram[6]-'0')*10000 +
                  (telegram[7]-'0')*1000;
                reportSensor(sunlightSensorIndex, sun);
                if (hasSunDirections) {
                  // - sun west
                  sun =
                    (telegram[8]-'0')*10000 +
                    (telegram[9]-'0')*1000;
                  reportSensor(sunWestSensorIndex, sun);
                  // - sun east
                  sun =
                    (telegram[10]-'0')*10000 +
                    (telegram[11]-'0')*1000;
                  reportSensor(sunEastSensorIndex, sun);
                }
                // - twilight indicator
                bool isTwilight = telegram[12]=='J';
                reportInput(twilightInputIndex, isTwilight);
                // - (low, <999lx) daylight
                double daylight =
                  (telegram[13]-'0')*100 +
                  (telegram[14]-'0')*10 +
                  (telegram[15]-'0')*1;
                reportSensor(daylightSensorIndex, daylight);
                // - wind
                double wind =
                  (telegram[16]-'0')*10 +
                  (telegram[17]-'0')*1 +
                  (telegram[19]-'0')*0.1;
                reportSensor(windSensorIndex, wind);
                reportSensor(gustSensorIndex, wind);
                // - rain
                bool rain = telegram[20]=='J';
                reportInput(rainInputIndex, rain);
              }
              // telegram consumed, wait for next
              telegramIndex = -1;
            }
          }
        }
      }
    }
  }

};


int main(int argc, char **argv)
{
  // prevent debug output before application.main scans command line
  SETLOGLEVEL(LOG_EMERG);
  SETERRLEVEL(LOG_EMERG, false); // messages, if any, go to stderr
  // create app with current mainloop
  static ExternalDeviceApp application;
  // pass control
  return application.main(argc, argv);
}
