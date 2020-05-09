#include <Particle.h>
#include <EventClass.h>

// Contructor for Blink class
Event::Event()
{
}

// Adds a state (level) to the Blink class. Use this function during setup to
// to create advanced blink patterns.
void Event::AddEvent(uint Period, void (*f)())
{
  // Initialize the new element
  struct ActionElement *elem = new ActionElement;
  elem->Period = Period;
  elem->f = f;
  elem->next = NULL;
  elem->nextcall = millis() + Period;

  // insert the element into the list of elements. For now no sorting of elements
  // is made and the element can be put in the start of the list.
  elem->next = first;
  first = elem;
}

// NOTE: _nexttime will overrun after 1,5 months in the current implementation and
// event actions will not be called anymore.
void Event::Tick()
{
  unsigned long now = millis();
  struct ActionElement *next = first;

  while (next != NULL)
  {
    if (now > next->nextcall)
    {
      next->nextcall = now + next->Period;
      next->f();
    }
    next = next->next;
  }
  ++_action_calls;
}

void Event::Print()
{
  // For debugging only
  Serial.printf("first = ", first);
}
