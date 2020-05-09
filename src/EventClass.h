
// Class Event
// Used for calling functions at regular interval. .
// The Action() is called in main loop and uses the software timer
// for detecting when to call the function.

struct ActionElement
{
  void (*f)();
  uint Period; // In ms
  unsigned long nextcall;
  struct ActionElement *next;
};

class Event
{
  // enum {INIT=0, LED_ON, LED_OFF} _state;
  // char *_statestr[3] {"INIT", "LED_ON", "LED_OFF"};
  int _state_num = 0;
  int _num_events=0;
  struct Action *events = NULL;
  int _action_calls = 0;
  struct ActionElement *first = NULL;

public:
  Event();
  void AddEvent(uint Period, void (*f)());
  void Tick();
  //int GetState() {return _state;};
  //char * GetStateStr() {return _statestr[_state];};
  int getCalls() {return _action_calls;};
  void Print();
};
