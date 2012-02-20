#ifndef MENU_HH_
#define MENU_HH_

#include "Types.hh"
#include "ButtonArray.hh"
#include "LiquidCrystal.hh"
#include "Screen.hh"
#include "MenuClass.hh"
#include "Setup.hh"
#include "UtilsMenu.hh"
#include "SDCard.hh"

/// Display a welcome splash screen, that removes itself when updated.
class SplashScreen: public Screen {
private:


public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

  void notifyButtonPressed(ButtonArray::ButtonName button);
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
	  DISTANCE_0_1MM = 0,
	  DISTANCE_1MM,
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
	bool  inDirectories;
	bool	drawItemLockout;
	bool  invalidate;
	sdcard::SdErrorCode sdlasterror;
public:
	SDMenu();

	void resetState();

	micros_t getUpdateRate() {return 500L * 1000L;}
	void notifyButtonPressed(ButtonArray::ButtonName button);
	/*void handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex);*/

	void update(LiquidCrystal& lcd, bool forceRedraw);
protected:
	uint8_t countFiles();

        bool getFilename(uint8_t index,
                         char buffer[],
                         uint8_t buffer_size);

	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};
/*
class CardMenu: public Menu {
private:
	uint8_t updatePhase;
	uint8_t lastItemIndex;
	bool  dir;
	bool  inDirectories;
	bool	drawItemLockout;
	bool  invalidate;
	sdcardmanager::SdErrorCode sdlasterror;
public:
	CardMenu();

	micros_t getUpdateRate() {return 500L * 1000L;}

	void resetState();

protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);
	void handleSelect(uint8_t index);
	void handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex);
	uint8_t countFiles();
  bool getFilename(uint8_t index, char buffer[],uint8_t buffer_size);
};
*/
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
	bool			printAnotherEnabled;
};

class MonitorMode: public Screen {
private:
	CancelBuildMenu cancelBuildMenu;

		enum UpdatePhase {
		UPDATE_PHASE_FIRST = 0,
		UPDATE_PHASE_TOOL_TEMP = UPDATE_PHASE_FIRST,
		UPDATE_PHASE_TOOL_TEMP_SET_POINT,
		UPDATE_PHASE_PLATFORM_TEMP,
		UPDATE_PHASE_PLATFORM_SET_POINT,
		UPDATE_PHASE_BUILD_PHASE_SCROLLER,
		UPDATE_PHASE_LAST	//Not counted, just an end marker
	};

	enum BuildTimePhase {
		BUILD_TIME_PHASE_FIRST = 0,
		BUILD_TIME_PHASE_COMPLETED_PERCENT = BUILD_TIME_PHASE_FIRST,
		BUILD_TIME_PHASE_ELAPSED_TIME,
		BUILD_TIME_PHASE_TIME_LEFT,
		BUILD_TIME_PHASE_ZPOS,
		BUILD_TIME_PHASE_FILAMENT,
		BUILD_TIME_PHASE_COPIES_PRINTED,
		BUILD_TIME_PHASE_LAST	//Not counted, just an end marker
	};

	enum UpdatePhase updatePhase;
	enum BuildTimePhase buildTimePhase, lastBuildTimePhase;
	float   lastElapsedSeconds;
	PauseMode pauseMode;
	bool	pausePushLockout;
  bool  buildCompleteBuzzPlayed;
 	int32_t buildDuration;
	bool	overrideForceRedraw;
	uint8_t	copiesPrinted;
	bool	timeLeftDisplayed;


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

class SteppersMenu: public Menu {
public:
	SteppersMenu();

	void resetState();
protected:
	void drawItem(uint8_t index, LiquidCrystal& lcd);

	void handleSelect(uint8_t index);
};

class AdvanceABPMode: public Screen {
private:
	

public:
	micros_t getUpdateRate() {return 50L * 1000L;}

	void update(LiquidCrystal& lcd, bool forceRedraw);

	void reset();

        void notifyButtonPressed(ButtonArray::ButtonName button);
};

class CurrentPositionMode: public Screen {
private:

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
	void handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex);

private:
        /// Static instances of our menus
        MonitorMode monitorMode;
        SDMenu sdMenu;
        JogMode jogger;
				PreheatMenu preheatMenu;
				ExtruderMode extruderMenu;
				SteppersMenu steppersMenu;
				AdvanceABPMode advanceABPMode;
				UtilsMenu utilsmenu;
				CurrentPositionMode currentPositionMode;

};

#endif
