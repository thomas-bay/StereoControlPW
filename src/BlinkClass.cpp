#include <Particle.h>
#include <BlinkClass.h>

// Contructor for Blink class
Blink::Blink(int IOPort)
{
  _port = IOPort;
  pinMode(_port, OUTPUT);
}

// Adds a state (level) to the Blink class. Use this function during setup to
// to create advanced blink patterns.
void Blink::AddState(int Period, int Level)
{
  int*  periods = new int[_num_periods+1];  // Allocate new space
  byte* levels  = new byte[_num_periods+1];

  // Copy the existing patterns into the new space that holds one extra element.
  if (_num_periods>0)
  {
    // Copy the periods and levels to the new memory.
    for (int x=0; x < _num_periods; x++)
    {
      periods[x] = Per[x];
      levels[x]  = Lev[x];
    }
    //std::copy(tmp, tmp + _i, Per); // Does not seems to function at all!!
    delete [] Per;  // delete the earlier allocated memory
    delete [] Lev;
  }

  periods[_num_periods] = Period;
  levels[_num_periods] = Level;
  ++_num_periods;
  Per = periods;
  Lev = levels;
}

// NOTE: _nexttime will overrun after 1,5 months in the current implementation and
// Led will stop blinking.
void Blink::Action()
{
  unsigned long now = millis();

  if (now > _nexttime)
  {
    _nexttime = now + Per[_state_num];
    #ifdef DEBUG_BLINK
    Serial.printlnf(" now = %i, State = %i, next = %i, Level = %s", now, _state_num, _nexttime, (Lev[_state_num] ? "HIGH" : "LOW"));
    #endif
    digitalWrite(_port, Lev[_state_num]);
    _state_num = (_state_num + 1) % _num_periods;
  }
  ++_action_calls;
}

void Blink::Print()
{
  // For debugging only
  for (int i=0; i < _num_periods; i++)
  {
    Serial.printf("Per[%i] = ", i);
    Serial.printf("%i", Per[i]);
    Serial.printlnf(" Level = %s", (Lev[i] ? "HIGH" : "LOW"));
  }
}
