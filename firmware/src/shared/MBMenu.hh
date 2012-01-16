#ifndef MBMenu_HH_
#define MBMenu_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"
#include "Screen.hh"
#include "MenuClass.hh"
#include "NYI.hh"


class ZAxisRevs: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  uint16_t wholePart;
  uint16_t fracPart;
  uint8_t selSize;
};

class MBMenu: public Menu {
public:
	MBMenu();

protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
private:
  NYI notyetimplemented;
  ZAxisRevs zaxisrevs;
};

#endif