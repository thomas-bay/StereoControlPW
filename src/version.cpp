#include "version.h"

Version::Version ()
{
  _file = __FILE__;
  _version = "Bubble V1.02";
  _date = __DATE__;
  _time = __TIME__;
}

String Version::getDate()
{
  return _date;
}

String Version::getTime()
{
  return _time;
}

String Version::getVersionAndTimeDate()
{
  return _version + " " + _date + " " + _time;
}
