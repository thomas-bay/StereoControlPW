#include "Particle.h"
#include <string.h>

//String s;


class Version
{
  String _file;
  String _version;
  String _date;
  String _time;
  int i;

public:
  Version();
  String getFile() {return _file;}
  String getVersion() {return _version;}
  String getDate();
  String getTime();
  String getVersionAndTimeDate();
};
