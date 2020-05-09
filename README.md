# Bubble2
My Photon project for controlling stereo.

Project includes:

UDP server:
* receives the UDP datagram from client, parses the command string and executes the command.
* listens for connections at port 8003

EventClass:
Method AddEvent(void * f(), int period) add an function pointer to the event list together with a period (in ms) and the time (in ms) for next call.    
If period is 0, only one call is made.

NOTE: This is continuation of the Bubble project after conversion to the Particle Workbench.
