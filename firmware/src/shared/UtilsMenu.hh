#ifndef UTILSMENU_HH_
#define UTILSMENU_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"
#include "Screen.hh"
#include "MenuClass.hh"
#include "Setup.hh"

class TestMode: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
	
	private:
	  ButtonArray::ButtonName buttonPressed;
};

class VersionMode: public Screen {
private:

public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class CalibrateMode: public Screen {
private:
	enum calibrateState {
		CS_NONE,
		CS_START1,	//Disable steppers
		CS_START2,	//Disable steppers
		CS_PROMPT_MOVE,	//Prompt user to move build platform
		CS_HOME_Z,
		CS_HOME_Z_WAIT,
		CS_HOME_Y,
		CS_HOME_Y_WAIT,
		CS_HOME_X,
		CS_HOME_X_WAIT,
		CS_PROMPT_CALIBRATED
	};

	enum calibrateState calibrationState, lastCalibrationState;

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class HomeOffsetsMode: public Screen {
private:
	enum homeOffState {
		HOS_NONE,
		HOS_OFFSET_X,
		HOS_OFFSET_Y,
		HOS_OFFSET_Z,
	};

	enum homeOffState homeOffsetState, lastHomeOffsetState;

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class TestEndStopsMode: public Screen {
private:

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class HomeAxisMode: public Screen {
private:
        void home(ButtonArray::ButtonName direction);

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class FilamentUsedResetMenu: public Menu {
public:
	FilamentUsedResetMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class FilamentUsedMode: public Screen {
private:
	FilamentUsedResetMenu filamentUsedResetMenu;

	bool overrideForceRedraw;
	bool lifetimeDisplay;
public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);
};

class ProfileChangeNameMode: public Screen {
private:
	uint8_t	cursorLocation;
	uint8_t profileName[8+1];

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);

	uint8_t profileIndex;
};

class ProfileDisplaySettingsMenu: public Menu {
private:
	uint8_t profileName[8+1];
	int32_t homeX, homeY, homeZ;
	uint8_t hbpTemp, tool0Temp, tool1Temp, extruderRpm;
public:
	ProfileDisplaySettingsMenu();

	void resetState();

	uint8_t profileIndex;
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class ProfileSubMenu: public Menu {
private:
	ProfileChangeNameMode	   profileChangeNameMode;
	ProfileDisplaySettingsMenu profileDisplaySettingsMenu;

public:
	ProfileSubMenu();

	void resetState();

	uint8_t profileIndex;
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class ProfilesMenu: public Menu {
private:
	ProfileSubMenu profileSubMenu;
public:
	ProfilesMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};


class UtilsMenu: public Menu {
public:
	UtilsMenu();
	
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	void handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex);
	
private:
	HomeAxisMode homeAxisMode;
	TestEndStopsMode testEndStopsMode;
  VersionMode versionMode;
	SetupMode setupMode;
	TestMode test;
	CalibrateMode calibrateMode;
	HomeOffsetsMode homeOffsetsMode;
	ProfilesMenu profilesMenu;
	FilamentUsedMode filamentUsedMode;
};

#endif