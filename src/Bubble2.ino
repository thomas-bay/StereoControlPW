#include "Particle.h"
//#include "stdhdr.h"
#include "EventClass.h"
#include "BlinkClass.h"
#include "version.h"
#include <time.h>

// --------------------------------
// Main file of the Bubble project.
// --------------------------------


SYSTEM_THREAD(ENABLED);


#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)
#define TCP_LOG_MSG(msg)  Serial.println(msg)
#define CMD_LOG_MSG(msg)  Serial.print("CMD: "); Serial.println(msg);
#define CMD_LOG_MSG1(msg, p1) Serial.print("CMD: "); Serial.printlnf(msg, p1);
#define CMD_LOG_MSG2(msg, p1, p2) Serial.print("CMD: "); Serial.printlnf(msg, p1, p2);
#define IO_LOG_MSG1(msg, p1)  Serial.print("IO: "); Serial.printlnf(msg, p1);

String Version;

int Enable = TRUE;

int LEDStatus_;
int LoopCounter = 0;

class Version V;
class Event E;

#ifndef USE_SWD_JTAG
//  class Blink B(D7);
#else
//  class Blink B(D2);
#endif

// The setup function is a standard part of any microcontroller program.
// It runs only once when the device boots up or is reset.
int ledCommand(String command);

// telnet defaults to port 10000

int RxCh;
int TestCount1, TestCount2;
bool TestFlag = false, PushFlag = false;
bool CheckTimeFlag = false;
time_t StartTime, StopTime;

enum _TimeCheckState
{
  INACTIVE,
  WAIT_FOR_START,
  WAIT_FOR_STOP
};

_TimeCheckState TimeCheckState = INACTIVE;


void CheckTime()
{
  CheckTimeFlag = true;
}

void Test1()
{
  TestCount1++;
}

void Test2()
{
  TestCount2++;
  TestFlag = true;
}

void TimeSync()
{
  Particle.syncTime();
  CMD_LOG_MSG ("timesync");
}

// Network setup
IPAddress myAddress(192,168,0,5); // Photon IP
IPAddress subnetMask(255,255,255,0);
IPAddress gateway(192,168,0,1);
IPAddress dns(192,168,0,1);
#define SERVERPORT 8003
TCPServer server = TCPServer(SERVERPORT);
TCPClient client;


void PushCommand()
{
  PushFlag = true;
}

#define MAX_RX 50
unsigned char Incoming[MAX_RX];

enum {OK, NOK, OUTTOKENS} OutTokens;
const String OutgoingTokens[OUTTOKENS] = {"OK!", "NOK!"};

// Hardware resources
#define POWERCTRL_IO  D7
#define SIGNALSEL0_IO D5    // Support for four signal inputs
#define SIGNALSEL1_IO D6
#define MAX_SIGNALS   4

// Frame definition
#define CMD       0
#define SPACER    1
#define SIGNAL    2
#define STARTTIME 2
#define SPACER2   7
#define STOPTIME  8
#define COLON1    4
#define COLON2    10

void PowerControlSignal(int Level)
{
  pinMode(POWERCTRL_IO, OUTPUT);
  digitalWrite(POWERCTRL_IO, Level);
  IO_LOG_MSG1("Power: %d", Level);
}

void SignalSelect(int signal)
{
  pinMode(SIGNALSEL0_IO, OUTPUT);
  pinMode(SIGNALSEL1_IO, OUTPUT);
  switch(signal)
  {
    case 0: /* Radio */
      digitalWrite(SIGNALSEL0_IO, 0);
      digitalWrite(SIGNALSEL1_IO, 0);
      break;
    case 1: /* Aux */
    digitalWrite(SIGNALSEL0_IO, 1);
    digitalWrite(SIGNALSEL1_IO, 0);
      break;
    case 2: /* TV */
      digitalWrite(SIGNALSEL0_IO, 0);
      digitalWrite(SIGNALSEL1_IO, 1);
      break;
    default:
      digitalWrite(SIGNALSEL0_IO, 1);
      digitalWrite(SIGNALSEL1_IO, 1);
  }
  IO_LOG_MSG1("Sel: %d", signal);
}

bool isDigit(unsigned char c)
{
  return ((c >= '0') && (c<='9'));
}

bool isFirstHour(unsigned char c)  // First hour digit
{
  return ((c >= '0') && (c<='2'));
}

bool isFirstMinute(unsigned char c)  // First minute digit
{
  return ((c >= '0') && (c<='5'));
}

bool CheckFrame(unsigned char *frame)
{
  if (frame[SPACER] == '#'
  && frame[SPACER2] == '#'
  && frame[COLON2] == ':'
  && frame[COLON2] == ':'
  && isFirstHour(frame[STARTTIME])      // 0-2
  && isDigit(frame[STARTTIME+1])        // 0-9
  && isFirstMinute(frame[STARTTIME+3])  // 0-5
  && isDigit(frame[STARTTIME+4])        // 0-9
  && isFirstHour(frame[STOPTIME])
  && isDigit(frame[STOPTIME+1])
  && isFirstMinute(frame[STOPTIME+3])
  && isDigit(frame[STOPTIME+4]))
    return true;
  else
    return false;
}

/*
-- Returns true if summertime is active.
-- NOTE: Only approximate.
*/
bool Summertime()
{
  if (Time.month() > 3 && Time.month() < 11)
    return true;
  else
    return false;
}

/*
-- Checks if stop time is before start time.
*/
int StopBeforeStart(int StartHour, int StartMinute, int StopHour, int StopMinute)
{
  if (StopHour < StartHour || (StopHour == StartHour) && StopMinute < StartMinute)
    return true;
  return false;
}


void CheckForIncomingData()
{
    if (client.connected())
    {
      TCP_LOG_MSG("Client connected");
      // echo all available bytes back to the client
      while (client.available())
      {
        int num = client.read(Incoming, MAX_RX);
        int result = NOK;
        unsigned int Signal;
        {
          if (num>0 && Incoming[SPACER] == '#')
          {
            switch(Incoming[CMD])
            {
              case '0':
                result = OK;
                PowerControlSignal(0);
                CMD_LOG_MSG("OFF");
                break;
              case '1':
                {
                  result = OK;
                  Signal = Incoming[SIGNAL] - '0';
                  if(Signal < MAX_SIGNALS)
                  {
                      PowerControlSignal(1);  // Turn on power
                      SignalSelect(Signal);
                      CMD_LOG_MSG1("ON: %c", Incoming[SIGNAL]);
                  }
                  else
                    result = NOK;
                }
                break;
              case '2':
                CMD_LOG_MSG("SET_TIME");
                result = NOK;

                // Verify the received format

                if (CheckFrame(Incoming) && (num == 13))
                {
                  bool pastMidnight = false;

                  result = OK;

                  // Calculate the number of milliseconds until start and stop.
                  int StartHour = 10*(Incoming[STARTTIME] -'0') + (Incoming[STARTTIME+1] - '0');
                  int StartMinute = 10*(Incoming[STARTTIME+3] -'0') + (Incoming[STARTTIME+4] - '0');
                  int StartSecond = 0;
                  int StopHour = 10*(Incoming[STOPTIME] -'0') + (Incoming[STOPTIME+1] - '0');
                  int StopMinute = 10*(Incoming[STOPTIME+3] -'0') + (Incoming[STOPTIME+4] - '0');
                  int StopSecond = 0;  // Set seconds is always 0

                  //////////////////////////////////////////////////////
                  int h = Time.hour();
                  int m = Time.minute();
                  int s = Time.second();

                  int now = Time.now();
                  CMD_LOG_MSG(Time.timeStr());

                  int SecondsToStart = 0;
                  int SecondsToStop = 0;
                  int SecondsMidnightToStart = 0;
                  int SecondsMidnightToStop = 0;
                  int SecondsToMidnight = 0;

                  if ((StartHour < h) || ((StartHour == h) && (StartMinute < m)))
                  {
                    // Set time is past midnight, case 3 + 4
                    SecondsToMidnight = (24-(h+1))*60*60 + (60-(m+1))*60 + (60-s);
                    SecondsMidnightToStart = StartHour*60*60 + StartMinute*60;
                    SecondsMidnightToStop =  StopHour*60*60 + StopMinute*60;

                    if(StopBeforeStart(StartHour, StartMinute, StopHour, StopMinute))
                      SecondsMidnightToStop += 24*60*60;    // Case 4

//                    CMD_LOG_MSG1("Seconds to midnight: %d", SecondsToMidnight);
                    CMD_LOG_MSG2("Seconds to start,stop: %d,%d", SecondsToMidnight + SecondsMidnightToStart, SecondsToMidnight + SecondsMidnightToStop);

                    StartTime = now + SecondsToMidnight + SecondsMidnightToStart;
                    StopTime = now + SecondsToMidnight + SecondsMidnightToStop;
                  }
                  else
                  {
                    // Set time is before midnight, case 1 + 2
                    SecondsToStart = (StartHour-h)*60*60 + (StartMinute-m)*60 + (StartSecond-s);
                    SecondsToStop =  (StopHour-h)*60*60 + (StopMinute-m)*60 + (StopSecond-s);

                    if(StopBeforeStart(StartHour, StartMinute, StopHour, StopMinute))
                        SecondsToStop += 24*60*60;  // Case 2: Start > Stop

                    CMD_LOG_MSG2("Seconds to start,stop: %d,%d", SecondsToStart, SecondsToStop);

                    StartTime = now + SecondsToStart;
                    StopTime = now + SecondsToStop;
                  }

                  // Correct for summertime.
                  if (Summertime())
                  {
                    StartTime -= 60*60;
                    StopTime -= 60*60;
                  }

                  // Set start and stop time Event
                  TimeCheckState = WAIT_FOR_START;
                }
                break;
              case '3':
                CMD_LOG_MSG("RESET_TIME");
                TimeCheckState = INACTIVE;
                result = OK;
                break;
            }
          }
        }
        server.write(OutgoingTokens[result]);
      }
      client.stop();
      TCP_LOG_MSG("Client connection closed");
    }
    else
    {
      // if no client is yet connected, check for a new connection
      client = server.available();
    }
}

void CheckStereoOnOff()
{
  if (TimeCheckState == WAIT_FOR_START)
  {
    if (Time.now() > StartTime)
    {
      PowerControlSignal(1);  // Turn on power
      TimeCheckState = WAIT_FOR_STOP;

    }
  }
  else if (TimeCheckState == WAIT_FOR_STOP)
  {
    if (Time.now() > StopTime)
    {
      PowerControlSignal(0);  // Turn on power
      TimeCheckState = INACTIVE;

    }
  }
}

void PrepForStaticIP()
{
  WiFi.setStaticIP(myAddress, subnetMask, gateway, dns);

  // now let's use the configured IP
  WiFi.useStaticIP();
}


void setup() {

  int StartupTimeout = 0;

  // We are going to tell our device that D0 and D7 (which we named led1 and led2 respectively) are going to be output
  // (That means that we will be sending voltage to them, rather than monitoring voltage that comes from them)

  // Setup the version/compilation date
  Version = V.getVersionAndTimeDate().c_str();
  Time.zone(1);   // Set timezone to UTC+1 (and summertime)

  // Publish the variables and functions
  Particle.function("Led", ledCommand);
  Particle.variable("LEDStatus_", LEDStatus_);
  Particle.variable("LoopCounter", LoopCounter);
  Particle.variable("Vers", Version);
  Particle.variable("Enable event handling", Enable);

  // Wait for something to happen in the serial input
  Serial.begin(9600);
  //while(!Serial.available()) Particle.process();
  //while(Serial.read() != -1);

  // Start TCP server and print the WiFi data
  //PrepForStaticIP();
  server.begin();
  client.stop();
  TCP_LOG_MSG(WiFi.localIP());
  TCP_LOG_MSG(WiFi.subnetMask());
  TCP_LOG_MSG(WiFi.gatewayIP());
  TCP_LOG_MSG(WiFi.SSID());

//  E.AddEvent(1000,  Test1);
  E.AddEvent(10000, Test2);
  E.AddEvent(ONE_DAY_MILLIS, TimeSync);   // Syncronize time once a day
  E.AddEvent(500, PushCommand);           // <<-- PROBLEM, delay other actions
  E.AddEvent(1000, CheckTime);         // <<-- PROBLEM, delay other actions


  Wire.begin();
}

// Next we have the loop function, the other essential part of a microcontroller program.
// This routine gets repeated over and over, as quickly as possible and as many times as possible, after the setup function is called.
// Note: Code that blocks for too long (like more than 5 seconds), can make weird things happen (like dropping the network connection).  The built-in delay function shown below safely interleaves required background activity, so arbitrarily long delays can safely be done if you need them.

void loop() {


  if (Enable == TRUE) {

//    B.Action(); // This makes the LED blink without using delay().
    E.Tick();

  }

  if (TestFlag)
  {
    String str;
    TestFlag = false;

    switch (TimeCheckState)
    {
      case WAIT_FOR_START:
        str = "Waiting for start time";
        break;

      case WAIT_FOR_STOP:
        str = "Waiting for stop time";
        break;

      default:
          str = "Idle";
    }
    CMD_LOG_MSG2("%s: %s", Time.timeStr().c_str(), str.c_str());
  }

  if (PushFlag)
  {
    PushFlag = false;
    CheckForIncomingData();
  }

  if (CheckTimeFlag)
  {
    CheckTimeFlag = false;
    CheckStereoOnOff();
  }
}

int ledCommand(String command) {

  if (command == "on")
  {
    Enable = TRUE;
    return 1;
  }
  else
    Enable = FALSE;

  return 0;
}
