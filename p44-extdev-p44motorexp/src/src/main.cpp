//
//  Copyright (c) 2016-2022 plan44.ch / Lukas Zeller, Zurich, Switzerland
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

#include "dcmotor.hpp"
#include "jsoncomm.hpp"

using namespace p44;

// set this to a unique string for your particular app or use --uniqueid command line parameter
#define DEFAULT_UNIQUE_ID "P44MotorExpansionExternalDeviceApp"

#define DEFAULT_API_HOST "localhost"
#define DEFAULT_API_SERVICE "8999"
#define DEFAULT_LOGLEVEL LOG_NOTICE

#define DEFAULT_MAX_OPEN_TIME (42*Second) // %%% tbd
#define DEFAULT_MAX_CLOSE_TIME (60*Second) // %%% tbd

class DCMotor : public P44Obj
{
public:
  DCMotor() : isOpening(false) {};
  ~DCMotor() { if (motor) motor->stop(); };
  DcMotorDriverPtr motor;
  bool isOpening; // current movement direction is up
  MLTicket moveTimer;
  MLMicroSeconds maxOpenTime; // max overall opening time
  MLMicroSeconds maxCloseTime; // max overall closing time
  int index;
};
typedef boost::intrusive_ptr<DCMotor> DCMotorPtr;


/// Main program for plan44.ch P44-DSB-DEH in form of the "vdcd" daemon)
class P44MotorDeviceApp : public CmdLineApp
{
  typedef CmdLineApp inherited;

  typedef std::list<string> StringList;
  typedef std::vector<DCMotorPtr> MotorVector;

  StringList motorDefs;
  MotorVector motors;
  MLMicroSeconds defaultMaxOpenTime;
  MLMicroSeconds defaultMaxCloseTime;

  string uniqueId;
  JsonCommPtr deviceConnection;

  // log to API
  void logToApi(int aLevel, const char *aLinePrefix, const char *aLogMessage)
  {
    JsonObjectPtr msg = JsonObject::newObj();
    msg->add("message", JsonObject::newString("log"));
    // - index
    msg->add("level", JsonObject::newInt32(aLevel));
    // - value
    msg->add("text", JsonObject::newString(aLogMessage));
    // Send message
    if (deviceConnection) {
      deviceConnection->sendMessage(msg);
    }
  }

public:

  P44MotorDeviceApp() :
    defaultMaxOpenTime(DEFAULT_MAX_OPEN_TIME),
    defaultMaxCloseTime(DEFAULT_MAX_CLOSE_TIME)
  {
  }


  virtual bool processOption(const CmdLineOptionDescriptor &aOptionDescriptor, const char *aOptionValue)
  {
    if (strcmp(aOptionDescriptor.longOptionName,"motor")==0) {
      motorDefs.push_back(aOptionValue);
    }
    else {
      return inherited::processOption(aOptionDescriptor, aOptionValue);
    }
    return true;
  }




  virtual int main(int argc, char **argv)
  {
    const char *usageText =
    "Usage: %1$s [options]\n";
    const CmdLineOptionDescriptor options[] = {
      // specific to P44 motor expansion
      { 'm', "motor",           true,  "pwm,sensor,cw,cww; define a DC motor device" },
      { 0  , "maxopentime",     true,  "seconds;default max opening time" },
      { 0  , "maxclosetime",    true,  "seconds;default max closing time" },
      { 0  , "adctest",         true,  "analogpinspec;pin to read analog value from" },
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
    if (!isTerminated()) {
      // pin test mode?
      string testPinSpec;
      if (getStringOption("adctest", testPinSpec)) {
        // test only
        AnalogIoPtr analogTestPin = AnalogIoPtr(new AnalogIo(testPinSpec.c_str(), false, 0));
        double v = analogTestPin->value();
        printf("Pin '%s' analog value is = %.3f\n", testPinSpec.c_str(), v);
        terminateApp(EXIT_SUCCESS);
      }
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
        SETLOGHANDLER(boost::bind(&P44MotorDeviceApp::logToApi, this, _1, _2, _3), false);
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
    }
    // run
    return run();
  };


  virtual void initialize()
  {
    int mcount = 0;
    for (StringList::iterator pos = motorDefs.begin(); pos!=motorDefs.end(); ++pos) {
      // create new motor definition
      const char *c = pos->c_str();
      string sensor, num, pwm, cw, cww;
      if (nextPart(c, sensor, ',')) {
        if (nextPart(c, num, ',')) {
          double maxCurrent = 0;
          if (sscanf(num.c_str(), "%lf", &maxCurrent)!=1) {
            terminateAppWith(TextError::err("invalid max current specification, needs to be float number"));
            return;
          }
          if (nextPart(c, pwm, ',')) {
            if (nextPart(c, cw, ',')) {
              nextPart(c, cww, ','); // optional CCW
              // create motor
              DCMotorPtr m = DCMotorPtr(new DCMotor);
              m->index = mcount++;
              // optional individual max move time
              m->maxOpenTime = defaultMaxOpenTime;
              m->maxCloseTime = defaultMaxCloseTime;
              if (nextPart(c, num, ',')) {
                double secs;
                if (sscanf(num.c_str(), "%lf", &secs)==1) {
                  m->maxOpenTime = secs*Second;
                  m->maxCloseTime = secs*Second;
                }
                if (nextPart(c, num, ',')) {
                  if (sscanf(num.c_str(), "%lf", &secs)==1) {
                    m->maxCloseTime = secs*Second;
                  }
                }
              }
              // create driver
              m->motor = DcMotorDriverPtr(new DcMotorDriver(
                AnalogIoPtr(new AnalogIo(pwm.c_str(), true, 0)),
                DigitalIoPtr(new DigitalIo(cw.c_str(), true)),
                cww.empty() ? DigitalIoPtr() : DigitalIoPtr(new DigitalIo(cww.c_str(), true))
              ));
              m->motor->setCurrentSensor(AnalogIoPtr(new AnalogIo(sensor.c_str(), false, 0)), 100*MilliSecond);
              m->motor->setCurrentLimits(maxCurrent);
              m->motor->setStopCallback(boost::bind(&P44MotorDeviceApp::stopReached, this, m, _1, _2, _3));
              motors.push_back(m);
              continue;
            }
          }
        }
      }
      // error
      terminateAppWith(TextError::err("invalid motor specification %s, needs pinspecs as follows: sensor,currentlimit,pwm,cw[,cww]", pos->c_str()));
      return;
    }
    // open plan44 vdcd external device API connection
    // - set handlers first
    deviceConnection->setConnectionStatusHandler(boost::bind(&P44MotorDeviceApp::connectionStatusHandler, this, _2));
    deviceConnection->setMessageHandler(boost::bind(&P44MotorDeviceApp::jsonMessageHandler, this, _1, _2));
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
      initDevices();
    }
    else {
      // connecion failed, exit
      terminateAppWith(aError);
    }
  }


  void initDevices()
  {
    JsonObjectPtr initMsg = JsonObject::newArray();
    // the vdc
    JsonObjectPtr vdcInit = JsonObject::newObj();
    vdcInit->add("message", JsonObject::newString("initvdc"));
    vdcInit->add("modelname", JsonObject::newString("P44 DC Motor Extension"));
    vdcInit->add("iconname", JsonObject::newString("dcmotor"));
    initMsg->arrayAppend(vdcInit);
    // the motors
    for (int i=0; i<motors.size(); i++) {
      // create vdcd external device API init message
      JsonObjectPtr devInit = JsonObject::newObj();
      devInit->add("message", JsonObject::newString("init"));
      // - tag
      devInit->add("tag", JsonObject::newString(string_format("M%d", i)));
      // - unique ID
      devInit->add("uniqueid", JsonObject::newString(string_format("%s-%d", uniqueId.c_str(), i)));
      devInit->add("modelname", JsonObject::newString("DC Motor Output"));
      devInit->add("iconname", JsonObject::newString("dcmotor"));
      // - always JSON
      devInit->add("protocol", JsonObject::newString("json"));
      // - projection screen roller blind
      devInit->add("output", JsonObject::newString("shadow"));
      devInit->add("kind", JsonObject::newString("roller")); // simple rollerblind, no angle
      devInit->add("endcontacts", JsonObject::newBool(true)); // has "end contacts", i.e. reports reaching end of movement
      devInit->add("move", JsonObject::newBool(true));
      devInit->add("name", JsonObject::newString(string_format("Motor %d", i)));
      initMsg->arrayAppend(devInit);
    }
    // Send init message
    deviceConnection->sendMessage(initMsg);
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
    JsonObjectPtr o;
    int mindex = -1;
    if (aJsonObject->get("tag", o)) {
      // only process messages with tag (but there might be valid other ones, just ignore them)
      if (sscanf(o->c_strValue(), "M%d", &mindex)!=1 || mindex>=motors.size() || mindex<0) {
        LOG(LOG_ERR, "tag must be M0..M%lu", motors.size()-1);
      }
      else {
        // addressing an existing motor, decode message
        if (aJsonObject->get("message", o)) {
          string msg = o->stringValue();
          if (msg=="move") {
            // move command
            if (aJsonObject->get("direction", o)) {
              int d = o->int32Value();
              move(motors[mindex], d);
            }
          }
        }
      }
    }
  }


  void reportPosition(DCMotorPtr aMotor, double aPos)
  {
    LOG(LOG_NOTICE, "Reporting position: %.1f", aPos);
    // create vdcd external device API channel update message
    JsonObjectPtr msg = JsonObject::newObj();
    msg->add("tag", JsonObject::newString(string_format("M%d", aMotor->index)));
    msg->add("message", JsonObject::newString("channel"));
    // - index
    msg->add("index", JsonObject::newInt32(0));
    // - value
    msg->add("value", JsonObject::newDouble(aPos));
    // Send message
    deviceConnection->sendMessage(msg);
  }


  void move(DCMotorPtr aMotor, int aDirection)
  {
    LOG(LOG_NOTICE, "M%d Received move command, direction = %d", aMotor->index, aDirection);
    // First, always stop everything, no matter what state
    aMotor->motor->stop();
    aMotor->moveTimer.cancel();
    if (aDirection!=0) {
      aMotor->isOpening = aDirection>0;
      aMotor->motor->rampToPower(100, aDirection, 0.5);
      aMotor->moveTimer.executeOnce(
        boost::bind(&P44MotorDeviceApp::motorTimeout, this, aMotor),
        aDirection>0 ? aMotor->maxOpenTime : aMotor->maxCloseTime
      );
    }
  }


  void motorTimeout(DCMotorPtr aMotor)
  {
    LOG(LOG_WARNING, "M%d timeout, should have reached fully %s end position by now", aMotor->index, aMotor->isOpening ? "open" : "closed");
    aMotor->motor->stop();
  }


  void stopReached(DCMotorPtr aMotor, double aCurrentPower, int aDirection, ErrorPtr aError)
  {
    // only handle stops caused by errors (current limiter, endswitches if we had them - we dont)
    if (Error::notOK(aError)) {
      LOG(LOG_NOTICE, "M%d reached fully %s position (current limiter)", aMotor->index, aMotor->isOpening ? "open" : "closed");
      aMotor->moveTimer.cancel();
      reportPosition(aMotor, aMotor->isOpening ? 100 : 0);
    }
  }

};


int main(int argc, char **argv)
{
  // prevent debug output before application.main scans command line
  SETLOGLEVEL(LOG_EMERG);
  SETERRLEVEL(LOG_EMERG, false); // messages, if any, go to stderr
  // create app with current mainloop
  static P44MotorDeviceApp application;
  // pass control
  return application.main(argc, argv);
}
