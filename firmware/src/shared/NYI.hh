#ifndef NYI_HH_
#define NYI_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"
#include "Screen.hh"


class NYI: public Screen {     
public:
  micros_t getUpdateRate(){return 500L * 1000L;}
	void update(LiquidCrystal& lcd, bool forceRedraw);
	void reset();
  void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  uint8_t waitfor;
};

#endif