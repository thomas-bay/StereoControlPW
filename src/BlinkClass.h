// Class Blink
// Used for Blinking the LED without use of delay().
// The Action() is called in loop and uses the software timer
// for changing state of the LED (IOPort).
// Any number of states can be defined to generate advanced
// blink patterns.

#define DEBUG_BLINK

class Blink
{
  // enum {INIT=0, LED_ON, LED_OFF} _state;
  // char *_statestr[3] {"INIT", "LED_ON", "LED_OFF"};
  int _state_num = 0;
  int _num_periods=0;
  int _port=D7;
  int * Per = NULL;
  byte * Lev = NULL;
  int _action_calls = 0;
  unsigned long _nexttime = 0;   // Shift nxt time millis() is greater than this.

public:
  Blink(int IOPort);
  void AddState(int Period, int level);
  void Action();
  //int GetState() {return _state;};
  //char * GetStateStr() {return _statestr[_state];};
  int getCalls() {return _action_calls;};
  void Print();
};
