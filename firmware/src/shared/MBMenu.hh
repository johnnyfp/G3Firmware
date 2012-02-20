#ifndef MBMenu_HH_
#define MBMenu_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"
#include "Screen.hh"
#include "MenuClass.hh"
#include "NYI.hh"
#include "AxisPerMM.hh"


class StepsPerMMMode: public Screen {
private:
	enum StepsPerMMState {
		SPM_NONE,
		SPM_THINGO,
		SPM_CUPCAKE,
		SPM_REPRAP,
		SPM_ULTIMAKER,
		SPM_CUSTOM,
		SPM_SET_X,
		SPM_SET_Y,
		SPM_SET_Z,
		SPM_SET_A,
		SPM_SET_B
	};

	enum StepsPerMMState stepsPerMMState, lastStepsPerMMState;

	uint8_t	cursorLocation;
	
	void populateDefaults(enum StepsPerMMState stepsPerMMState);
	
public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);
};

class ExtruTherm: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  uint16_t beta;
  uint32_t ohms;
  uint8_t base;
  uint8_t xpos;
  uint8_t ypos;
};

class HBPTherm: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  uint16_t beta;
  uint32_t ohms;
  uint8_t base;
  uint8_t xpos;
  uint8_t ypos;
};

class HBPPID: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  float ppid;
  float ipid;
  float dpid;
  uint8_t xpos;
  uint8_t ypos;
};

class ExtruPID: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  float ppid;
  float ipid;
  float dpid;
  uint8_t xpos;
  uint8_t ypos;
};

class OutputC: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};

class HommingFR: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};

class MachineName: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};

class InvertAxis: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};

class EStop: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};

class EndStopHome: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};

class EndStopAxis: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};


/*class DisplaySteps: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  
};*/

class MBMenu: public Menu {
public:
	MBMenu();

protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	void handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex);
private:
  NYI notyetimplemented;
  ExtruTherm extrutherm;
  HBPTherm nbptherm;
  StepsPerMMMode stepsPerMMMode;
  HBPPID hbppid;
  ExtruPID extpid;
  //DisplaySteps dispsteps; /*For Debugging code*/
};

#endif