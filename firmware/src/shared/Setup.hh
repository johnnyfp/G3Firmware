#ifndef SETUP_HH_
#define SETUP_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"
#include "Screen.hh"
#include "MenuClass.hh"
#include "MBMenu.hh"

class DisplaySetupMode: public Screen {
public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	// Refresh the display information
	void update(LiquidCrystal& lcd, bool forceRedraw);

	// Reset the menu to it's default state
	void reset();

	// Get notified that a button was pressed
	void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  uint8_t initSize;
  uint8_t selSize;
  bool resetRequired;
};

class MoodLightSetRGBScreen: public Screen {
private:
	uint8_t red;
	uint8_t green;
	uint8_t blue;

	int inputMode;	//0 = red, 1 = green, 2 = blue
	bool redrawScreen;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class MoodLightMode: public Screen {
private:
	uint8_t updatePhase;

	uint8_t scriptId;

  MoodLightSetRGBScreen   moodLightSetRGBScreen;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

	void notifyButtonPressed(ButtonArray::ButtonName button);
};

class BuzzerSetRepeatsMode: public Screen {
private:
	uint8_t repeats;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class ABPCopiesSetScreen: public Screen {
private:
	uint8_t value;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class PreheatDuringEstimateMenu: public Menu {
public:
	PreheatDuringEstimateMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class OverrideGCodeTempMenu: public Menu {
public:
	OverrideGCodeTempMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class StepperDriverAcceleratedMenu: public Menu {
public:
	StepperDriverAcceleratedMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class AcceleratedSettingsMode: public Screen {
private:
	enum accelerateSettingsState {
		AS_NONE,
		AS_MAX_FEEDRATE_X,
		AS_MAX_FEEDRATE_Y,
		AS_MAX_FEEDRATE_Z,
		AS_MAX_FEEDRATE_A,
		AS_MAX_ACCELERATION_X,
		AS_MAX_ACCELERATION_Y,
		AS_MAX_ACCELERATION_Z,
		AS_MAX_ACCELERATION_A,
		AS_MAX_EXTRUDER_NORM,
		AS_MAX_EXTRUDER_RETRACT,
		AS_MIN_FEED_RATE,
		AS_MIN_TRAVEL_FEED_RATE,
		AS_MAX_XY_JERK,
		AS_MAX_Z_JERK,
		AS_ADVANCE_K,
		AS_FILAMENT_DIAMETER,
	};

	enum accelerateSettingsState accelerateSettingsState, lastAccelerateSettingsState;

	uint32_t values[16];

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class ExtruderTooColdMenu: public Menu {
public:
	ExtruderTooColdMenu();
	void resetState();
	
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	
};

class EStepsPerMMLengthMode: public Screen {
private:
	uint32_t value;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);

	int32_t steps;
};

class EStepsPerMMStepsMode: public Screen {
private:
	int32_t value;
	ExtruderTooColdMenu extruderTooColdMenu;
	EStepsPerMMLengthMode eStepsPerMMLengthMode;

	void extrude(bool overrideTempCheck);

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);
};

class EStepsPerMMMode: public Screen {
private:
	uint32_t value;
	EStepsPerMMStepsMode eStepsPerMMStepsMode;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class AccelerationMenu: public Menu {
private:
	StepperDriverAcceleratedMenu	stepperDriverAcceleratedMenu;
	AcceleratedSettingsMode		acceleratedSettingsMode;
	EStepsPerMMMode			eStepsPerMMMode;
	
	bool acceleration;
public:
	AccelerationMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class SetupMode: public Menu {
public:
	SetupMode();
	
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	void handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex);
	
private:
	MoodLightMode	moodLightMode;
	MBMenu mbPrefMenu;
	DisplaySetupMode displaySetup;
	NYI notyetimplemented;
	BuzzerSetRepeatsMode buzzerSetRepeats;
	PreheatDuringEstimateMenu	preheatDuringEstimateMenu;
	OverrideGCodeTempMenu		overrideGCodeTempMenu;
	ABPCopiesSetScreen		abpCopiesSetScreen;
	AccelerationMenu		accelerationMenu;
};

#endif