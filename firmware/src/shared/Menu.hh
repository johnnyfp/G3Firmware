#ifndef MENU_HH_
#define MENU_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"

/// The screen class defines a standard interface for anything that should
/// be displayed on the LCD.
class Screen {
public:
        /// Get the rate that this display should be updated. This is called
        /// after every screen display, so it can be used to dynamically
        /// adjust the update rate. This can be as fast as you like, however
        /// refreshing too fast during a build is certain to interfere with
        /// the serial and stepper processes, which will decrease build quality.
        /// \return refresh interval, in microseconds.
	virtual micros_t getUpdateRate();

        /// Update the screen display,
        /// \param[in] lcd LCD to write to
        /// \param[in] forceRedraw if true, redraw the entire screen. If false,
        ///                        only updated sections need to be redrawn.
	virtual void update(LiquidCrystal& lcd, bool forceRedraw);

        /// Reset the screen to it's default state
	virtual void reset();

        /// Get a notification that a button was pressed down.
        /// This function is called for every button that is pressed. Screen
        /// logic can be updated, however the screen should not be redrawn
        /// until update() is called again.
        ///
        /// Note that the current implementation only supports one button
        /// press at a time, and will discard any other events.
        /// \param button Button that was pressed
        virtual void notifyButtonPressed(ButtonArray::ButtonName button);
};


/// The menu object can be used to display a list of options on the LCD
/// screen. It handles updating the display and responding to button presses
/// automatically.
class Menu: public Screen {
public:
	virtual micros_t getUpdateRate() {return 500L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

	virtual void resetState();

        void notifyButtonPressed(ButtonArray::ButtonName button);

protected:

				uint8_t	subItemIndex;
				uint8_t firstSubItemIndex;
				uint8_t lastSubItemIndex;
				uint8_t lastSubIndex;
        uint8_t itemIndex;              ///< The currently selected item
        uint8_t lastDrawIndex;          ///< The index used to make the last draw
        uint8_t itemCount;              ///< Total number of items
        uint8_t firstItemIndex;         ///< The first selectable item. Set this
                                        ///< to greater than 0 if the first
                                        ///< item(s) are a title)

        /// Draw an item at the current cursor position.
        /// \param[in] index Index of the item to draw
        /// \param[in] LCD screen to draw onto
	virtual void drawItem(uint8_t index, LiquidCrystal& lcd);
	
	virtual void drawItemSub(uint8_t index, uint8_t subIndex, LiquidCrystal& lcd);

        /// Handle selection of a menu item
        /// \param[in] index Index of the menu item that was selected
	virtual void handleSelect(uint8_t index);
	
	virtual void handleSelectSub(uint8_t index, uint8_t subIndex);

        /// Handle the menu being cancelled. This should either remove the
        /// menu from the stack, or pop up a confirmation dialog to make sure
        /// that the menu should be removed.
	virtual void handleCancel();
};

/// Display a welcome splash screen, that removes itself when updated.
class SplashScreen: public Screen {
private:


public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class NYI: public Screen {     
public:
  micros_t getUpdateRate() {return 500L * 1000L;}
	void update(LiquidCrystal& lcd, bool forceRedraw);
	void reset();
  void notifyButtonPressed(ButtonArray::ButtonName button);
private:
  uint8_t waitfor;
};

class UserViewMenu: public Menu {
public:
	UserViewMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};


class JogMode: public Screen {
private:
	enum distance_t {
	  DISTANCE_SHORT = 0,
	  DISTANCE_LONG,
	  DISTANCE_CONT,
	};

	UserViewMenu userViewMenu;

	distance_t jogDistance;
	bool distanceChanged;
	bool userViewMode;
	bool userViewModeChanged;
	ButtonArray::ButtonName lastDirectionButtonPressed;

  void jog(ButtonArray::ButtonName direction);

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};


class SDMenu: public Menu {
private:
	uint8_t updatePhase;
	uint8_t lastItemIndex;
	bool  dir;
	bool	drawItemLockout;
public:
	SDMenu();

	void resetState();

	micros_t getUpdateRate() {return 500L * 1000L;}
	void notifyButtonPressed(ButtonArray::ButtonName button);

	void update(LiquidCrystal& lcd, bool forceRedraw);
protected:
	uint8_t countFiles();

        bool getFilename(uint8_t index,
                         char buffer[],
                         uint8_t buffer_size);

	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};


class PauseMode: public Screen {
private:
	ButtonArray::ButtonName lastDirectionButtonPressed;

        void jog(ButtonArray::ButtonName direction);

	uint8_t pauseState;

public:
 	bool autoPause;
 	bool freeMove;

	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);
};

class PauseAtZPosScreen: public Screen {
private:
	float pauseAtZPos;

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);
};



class CancelBuildMenu: public Menu {
public:
	CancelBuildMenu();
	void resetState();
	
protected:
	void drawItemSub(uint8_t index, uint8_t subIndex, LiquidCrystal& lcd);
	void handleSelectSub(uint8_t index, uint8_t subIndex);
	
private:
	PauseMode	        pauseMode;
	bool		          pauseDisabled;
	PauseAtZPosScreen	pauseAtZPosScreen;

};


class MonitorMode: public Screen {
private:
	CancelBuildMenu cancelBuildMenu;

	uint8_t updatePhase;
	uint8_t buildTimePhase;
	float   lastElapsedSeconds;
	float   extruderStartSeconds; 
	bool	  buildComplete;		//For solving floating point rounding issues
	PauseMode pauseMode;
	bool	pausePushLockout;


public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

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

class VersionMode: public Screen {
private:

public:
	micros_t getUpdateRate() {return 500L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};


class Tool0TempSetScreen: public Screen {
private:
	uint8_t value;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};


class PlatformTempSetScreen: public Screen {
private:
	uint8_t value;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};


class PreheatMenu: public Menu {
public:
	PreheatMenu();

	void fetchTargetTemps();

protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);

private:
	uint16_t tool0Temp;
	uint16_t platformTemp;

        /// Static instances of our menus
        Tool0TempSetScreen tool0TempSetScreen;
        PlatformTempSetScreen platTempSetScreen;
};

class ExtruderTooColdMenu: public Menu {
public:
	ExtruderTooColdMenu();
	void resetState();
	
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	
};

class ExtruderSetRpmScreen: public Screen {
private:
	uint8_t rpm;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class ExtruderMode: public Screen {
private:
	enum extrudeSeconds {
		EXTRUDE_SECS_CANCEL = 0,
		EXTRUDE_SECS_1S     = 1,
		EXTRUDE_SECS_2S     = 2,
		EXTRUDE_SECS_5S     = 5,
		EXTRUDE_SECS_10S    = 10,
		EXTRUDE_SECS_30S    = 30,
		EXTRUDE_SECS_60S    = 60,
		EXTRUDE_SECS_90S    = 90,
		EXTRUDE_SECS_120S   = 120,
		EXTRUDE_SECS_240S   = 240,
	};

	enum extrudeSeconds extrudeSeconds;
	bool timeChanged;
	int16_t lastDirection;
	ExtruderTooColdMenu extruderTooColdMenu;
        ExtruderSetRpmScreen extruderSetRpmScreen;

	uint8_t updatePhase;
	uint16_t elapsedTime;

	void extrude(seconds_t steps, bool overrideTempCheck);

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

	void notifyButtonPressed(ButtonArray::ButtonName button);
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

        MoodLightSetRGBScreen   moodLightSetRGBScreen;

public:
	micros_t getUpdateRate() {return 100L * 1000L;}

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

class SteppersMenu: public Menu {
public:
	SteppersMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
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

class TestEndStopsMode: public Screen {
private:

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class SetupMode: public Menu {
public:
	SetupMode();
	
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	
private:
	HomeAxisMode homeAxisMode;
	TestEndStopsMode testEndStopsMode;
  VersionMode versionMode;
	MoodLightMode	moodLightMode;
	MBMenu mbPrefMenu;
	TestMode test;
	DisplaySetupMode displaySetup;
	NYI notyetimplemented;
};

class AdvanceABPMode: public Screen {
private:
	bool abpForwarding;

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

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


class MainMenu: public Menu {
public:
	MainMenu();

protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);

private:
        /// Static instances of our menus
        MonitorMode monitorMode;
        SDMenu sdMenu;
        JogMode jogger;
				PreheatMenu preheatMenu;
				ExtruderMode extruderMenu;
				SteppersMenu steppersMenu;
				AdvanceABPMode advanceABPMode;
				CalibrateMode calibrateMode;
				HomeOffsetsMode homeOffsetsMode;
				SetupMode setup;
				
};

#endif