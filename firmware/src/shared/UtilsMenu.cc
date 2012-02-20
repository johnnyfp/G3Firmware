#include "UtilsMenu.hh"

#include <stdlib.h>
#include "Interface.hh"
#include "Steppers.hh"
#include "Commands.hh"
#include "Errors.hh"
#include "Tool.hh"
#include "Host.hh"
#include "Timeout.hh"
#include "Interface.hh"
#include "Motherboard.hh"
#include "Version.hh"
#include <util/delay.h>
#include <stdlib.h>
#include "SDCard.hh"
#include "SharedEepromMap.hh"
#include "eeprom.hh"
#include <avr/eeprom.h>
#include "ExtruderControl.hh"
#include "AxisPerMM.hh"

Point homePosition;


//Stack checking
//http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=52249
extern uint8_t _end; 
extern uint8_t __stack;

#define STACK_CANARY 0xc5

void StackPaint(void) __attribute__ ((naked)) __attribute__ ((section (".init1"))); 

void StackPaint(void) 
{ 
#if 0 
    uint8_t *p = &_end; 

    while(p <= &__stack) 
    { 
        *p = STACK_CANARY; 
        p++; 
    } 
#else 
    __asm volatile ("    ldi r30,lo8(_end)\n" 
                    "    ldi r31,hi8(_end)\n" 
                    "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */ 
                    "    ldi r25,hi8(__stack)\n" 
                    "    rjmp .cmp\n" 
                    ".loop:\n" 
                    "    st Z+,r24\n" 
                    ".cmp:\n" 
                    "    cpi r30,lo8(__stack)\n" 
                    "    cpc r31,r25\n" 
                    "    brlo .loop\n" 
                    "    breq .loop"::); 
#endif 
}
uint16_t StackCount(void) 
{ 
    const uint8_t *p = &_end; 
    uint16_t       c = 0; 

    while(*p == STACK_CANARY && p <= &__stack) 
    { 
        p++; 
        c++; 
    } 

    return c; 
}

void TestMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[] = "To quit press   ";
	static PROGMEM prog_uchar line2[] = "Cancel then Ok  ";
	static PROGMEM prog_uchar line3[] = "----------------";
	static PROGMEM prog_uchar butt1[] = "---  Zero    ---";
	static PROGMEM prog_uchar butt2[] = "---  Ok      ---";
	static PROGMEM prog_uchar butt3[] = "---  Y Minus ---";
	static PROGMEM prog_uchar butt4[] = "---  Z Minus ---";
	static PROGMEM prog_uchar butt5[] = "---  Y Plus  ---";
	static PROGMEM prog_uchar butt6[] = "---  Z Plus  ---";
	static PROGMEM prog_uchar butt7[] = "---  X Minus ---";
	static PROGMEM prog_uchar butt8[] = "---  X Plus  ---";
	static PROGMEM prog_uchar butt9[] = "---  Cancel  ---";
	static PROGMEM prog_uchar butt10[] ="-Unknown Button--";
	if (forceRedraw) {
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(line1);
		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(line2);
		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(line3);
	}
	
		lcd.setCursor(0,3);
	switch (buttonPressed) {
	case ButtonArray::ZERO:
		lcd.writeFromPgmspace(butt1);
		break;
	case ButtonArray::OK:
		lcd.writeFromPgmspace(butt2);
		break;
	case ButtonArray::YMINUS:
		lcd.writeFromPgmspace(butt3);
		break;
	case ButtonArray::ZMINUS:
		lcd.writeFromPgmspace(butt4);
		break;
	case ButtonArray::YPLUS:
		lcd.writeFromPgmspace(butt5);
		break;
	case ButtonArray::ZPLUS:
		lcd.writeFromPgmspace(butt6);
		break;
	case ButtonArray::XMINUS:
		lcd.writeFromPgmspace(butt7);
		break;
	case ButtonArray::XPLUS:
		lcd.writeFromPgmspace(butt8);
		break;
	case ButtonArray::CANCEL:
		lcd.writeFromPgmspace(butt9);
		break;
	case ButtonArray::UNKNOWN:
		lcd.writeFromPgmspace(butt10);
		break;
	}
}

void TestMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
	case ButtonArray::ZERO:
	case ButtonArray::OK:
	case ButtonArray::YMINUS:
	case ButtonArray::ZMINUS:
	case ButtonArray::YPLUS:
	case ButtonArray::ZPLUS:
	case ButtonArray::XMINUS:
	case ButtonArray::XPLUS:
	case ButtonArray::CANCEL:
		if ((button==ButtonArray::OK) && (buttonPressed==ButtonArray::CANCEL)) interface::popScreen();
		buttonPressed=button;
		break;
	default:
		buttonPressed=ButtonArray::UNKNOWN;
	}
}

void TestMode::reset() {
	buttonPressed=ButtonArray::UNKNOWN;
}

void VersionMode::reset() {
}

void VersionMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar version1[] = "    MBoard:__.__";
	const static PROGMEM prog_uchar version2[] = "  Extruder:__.__";
	const static PROGMEM prog_uchar version4[] = "FreeSram: ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(version1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(version2);

		//Display the motherboard version
		lcd.setCursor(13, 0);
		lcd.writeInt(firmware_version / 100, 1);

		lcd.setCursor(15, 0);
		lcd.writeInt(firmware_version-((firmware_version / 100)*100), 1);

		//Display the extruder version
		OutPacket responsePacket;

		if (extruderControl(SLAVE_CMD_VERSION, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t extruderVersion = responsePacket.read16(1);

			lcd.setCursor(13, 1);
			lcd.writeInt(extruderVersion / 100, 1);

			lcd.setCursor(15, 1);
			lcd.writeInt(extruderVersion-((extruderVersion / 100)*100), 1);
		} else {
			lcd.setCursor(13, 1);
			lcd.writeString("XX.XX");
		}
		
		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(version4);
		lcd.writeFloat((float)StackCount(),0);
		
	} else {
	}
}

void VersionMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	interface::popScreen();
}

void CalibrateMode::reset() {
	//Disable stepps on axis 0, 1, 2, 3, 4
	steppers::enableAxis(0, false);
	steppers::enableAxis(1, false);
	steppers::enableAxis(2, false);
	steppers::enableAxis(3, false);
	steppers::enableAxis(4, false);

	lastCalibrationState = CS_NONE;
	calibrationState = CS_START1;
}

void CalibrateMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar calib1[] = "Calibrate: Move ";
	const static PROGMEM prog_uchar calib2[] = "build platform";
	const static PROGMEM prog_uchar calib3[] = "until nozzle...";
	const static PROGMEM prog_uchar calib4[] = "          (cont)";
	const static PROGMEM prog_uchar calib5[] = "lies in center,";
	const static PROGMEM prog_uchar calib6[] = "turn threaded";
	const static PROGMEM prog_uchar calib7[] = "rod until...";
	const static PROGMEM prog_uchar calib8[] = "nozzle just";
	const static PROGMEM prog_uchar calib9[] = "touches.";
	const static PROGMEM prog_uchar homeZ[]  = "Homing Z...";
	const static PROGMEM prog_uchar homeY[]  = "Homing Y...";
	const static PROGMEM prog_uchar homeX[]  = "Homing X...";
	const static PROGMEM prog_uchar done[]   = "! Calibrated !";
	const static PROGMEM prog_uchar regen[]  = "Regenerate gcode";
	const static PROGMEM prog_uchar reset[]  = "         (reset)";

	if ((forceRedraw) || (calibrationState != lastCalibrationState)) {
		lcd.clear();
		lcd.setCursor(0,0);
		switch(calibrationState) {
			case CS_START1:
				lcd.writeFromPgmspace(calib1);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(calib2);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(calib3);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(calib4);
				break;
			case CS_START2:
				lcd.writeFromPgmspace(calib5);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(calib6);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(calib7);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(calib4);
				break;
			case CS_PROMPT_MOVE:
				lcd.writeFromPgmspace(calib8);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(calib9);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(calib4);
				break;
			case CS_HOME_Z:
			case CS_HOME_Z_WAIT:
				lcd.writeFromPgmspace(homeZ);
				break;
			case CS_HOME_Y:
			case CS_HOME_Y_WAIT:
				lcd.writeFromPgmspace(homeY);
				break;
			case CS_HOME_X:
			case CS_HOME_X_WAIT:
				lcd.writeFromPgmspace(homeX);
				break;
			case CS_PROMPT_CALIBRATED:
				lcd.writeFromPgmspace(done);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(regen);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(reset);
				break;
		}
	}

	lastCalibrationState = calibrationState;

	//Change the state
	//Some states are changed when a button is pressed via notifyButton
	//Some states are changed when something completes, in which case we do it here
	uint8_t axes;
	
	float interval = 2000.0;

	switch(calibrationState) {
		case CS_HOME_Z:
			//Declare current position to be x=0, y=0, z=0, a=0, b=0
			steppers::definePosition(Point(0,0,0,0,0));
			interval *= AxisPerMM::stepsToMM((int32_t)200.0, AxisPerMM::AXIS_Z); //Use ToM as baseline
			steppers::startHoming(true, 0x04, (uint32_t)interval);
			calibrationState = CS_HOME_Z_WAIT;
			break;
		case CS_HOME_Z_WAIT:
			if ( ! steppers::isHoming() )	calibrationState = CS_HOME_Y;
			break;
		case CS_HOME_Y:
			interval *= AxisPerMM::stepsToMM((int32_t)47.06, AxisPerMM::AXIS_Y); //Use ToM as baseline
			steppers::startHoming(false, 0x02, (uint32_t)interval);
			calibrationState = CS_HOME_Y_WAIT;
			break;
		case CS_HOME_Y_WAIT:
			if ( ! steppers::isHoming() )	calibrationState = CS_HOME_X;
			break;
		case CS_HOME_X:
			interval *= AxisPerMM::stepsToMM((int32_t)47.06, AxisPerMM::AXIS_X); //Use ToM as baseline
			steppers::startHoming(false, 0x01, (uint32_t)interval);
			calibrationState = CS_HOME_X_WAIT;
			break;
		case CS_HOME_X_WAIT:
			if ( ! steppers::isHoming() ) {
				//Record current X, Y, Z, A, B co-ordinates to the motherboard
				for (uint8_t i = 0; i < STEPPER_COUNT; i++) {
					uint16_t offset = mbeeprom::AXIS_HOME_POSITIONS + 4*i;
					uint32_t position = steppers::getPosition()[i];
					cli();
					eeprom_write_block(&position, (void*) offset, 4);
					sei();
				}

				//Disable stepps on axis 0, 1, 2, 3, 4
				steppers::enableAxis(0, false);
				steppers::enableAxis(1, false);
				steppers::enableAxis(2, false);
				steppers::enableAxis(3, false);
				steppers::enableAxis(4, false);

				calibrationState = CS_PROMPT_CALIBRATED;
			}
			break;
	}
}

void CalibrateMode::notifyButtonPressed(ButtonArray::ButtonName button) {

	if ( calibrationState == CS_PROMPT_CALIBRATED ) {
		host::stopBuild();
		return;
	}

	switch (button) {
        	case ButtonArray::OK:
        	case ButtonArray::YMINUS:
        	case ButtonArray::ZMINUS:
        	case ButtonArray::YPLUS:
        	case ButtonArray::ZPLUS:
        	case ButtonArray::XMINUS:
        	case ButtonArray::XPLUS:
        	case ButtonArray::ZERO:
			if (( calibrationState == CS_START1 ) || ( calibrationState == CS_START2 ) ||
			    (calibrationState == CS_PROMPT_MOVE ))	calibrationState = (enum calibrateState)((uint8_t)calibrationState + 1);
			break;
        	case ButtonArray::CANCEL:
               		interface::popScreen();
			break;
	}
}

void HomeOffsetsMode::reset() {
	homePosition = steppers::getPosition();

	for (uint8_t i = 0; i < STEPPER_COUNT; i++) {
		uint16_t offset = mbeeprom::AXIS_HOME_POSITIONS + 4*i;
		cli();
		eeprom_read_block(&(homePosition[i]), (void*) offset, 4);
		sei();
	}

	lastHomeOffsetState = HOS_NONE;
	homeOffsetState	    = HOS_OFFSET_X;
}

void HomeOffsetsMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1x[] = "X Offset:";
	const static PROGMEM prog_uchar message1y[] = "Y Offset:";
	const static PROGMEM prog_uchar message1z[] = "Z Offset:";
	const static PROGMEM prog_uchar message4[]  = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = " ";
	const static PROGMEM prog_uchar mm[]        = "mm";

	if ( homeOffsetState != lastHomeOffsetState )	forceRedraw = true;

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		switch(homeOffsetState) {
			case HOS_OFFSET_X:
				lcd.writeFromPgmspace(message1x);
				break;
                	case HOS_OFFSET_Y:
				lcd.writeFromPgmspace(message1y);
				break;
                	case HOS_OFFSET_Z:
				lcd.writeFromPgmspace(message1z);
				break;
		}

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	float position = 0.0;

	switch(homeOffsetState) {
		case HOS_OFFSET_X:
			position = AxisPerMM::stepsToMM(homePosition[0], AxisPerMM::AXIS_X);
			break;
		case HOS_OFFSET_Y:
			position = AxisPerMM::stepsToMM(homePosition[1], AxisPerMM::AXIS_Y);
			break;
		case HOS_OFFSET_Z:
			position = AxisPerMM::stepsToMM(homePosition[2], AxisPerMM::AXIS_Z);
			break;
	}

	lcd.setCursor(0,1);
	lcd.writeFloat((float)position, 3);
	lcd.writeFromPgmspace(mm);


	lastHomeOffsetState = homeOffsetState;
}

void HomeOffsetsMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	if (( homeOffsetState == HOS_OFFSET_Z ) && (button == ButtonArray::OK )) {
		//Write the new home positions
		for (uint8_t i = 0; i < STEPPER_COUNT; i++) {
			uint16_t offset = mbeeprom::AXIS_HOME_POSITIONS + 4*i;
			uint32_t position = homePosition[i];
			cli();
			eeprom_write_block(&position, (void*) offset, 4);
			sei();
		}

		host::stopBuild();
		return;
	}

	float currentPosition;
	uint8_t currentIndex = homeOffsetState - HOS_OFFSET_X;

	currentPosition = (float)homePosition[currentIndex];

	switch (button) {
		case ButtonArray::CANCEL:
			interface::popScreen();
			break;
		case ButtonArray::ZERO:
			break;
		case ButtonArray::OK:
			if 	( homeOffsetState == HOS_OFFSET_X )	homeOffsetState = HOS_OFFSET_Y;
			else if ( homeOffsetState == HOS_OFFSET_Y )	homeOffsetState = HOS_OFFSET_Z;
			break;
		case ButtonArray::ZPLUS:
			// increment more
			homePosition[currentIndex] += 20;
			break;
		case ButtonArray::ZMINUS:
			// decrement more
			homePosition[currentIndex] -= 20;
			break;
		case ButtonArray::YPLUS:
			// increment less
			homePosition[currentIndex] += 1;
			break;
		case ButtonArray::YMINUS:
			// decrement less
			homePosition[currentIndex] -= 1.0;
			break;
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}

	homePosition[currentIndex] = (int32_t)currentPosition;
}


void TestEndStopsMode::reset() {
}

void TestEndStopsMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar test1[] = "Test End Stops: ";
	const static PROGMEM prog_uchar test2[] = "(press end stop)";
	const static PROGMEM prog_uchar test3[] = "X___:N    Y___:N";
	const static PROGMEM prog_uchar test4[] = "Z___:N";
	const static PROGMEM prog_uchar strY[]  = "Y";
	const static PROGMEM prog_uchar strN[]  = "N";
	const static PROGMEM prog_uchar strmin[]   = "Min";
	const static PROGMEM prog_uchar strmax[]   = "Max";
	
	uint8_t minmax=eeprom::getEeprom8(mbeeprom::AXIS_HOME_MINMAX, 0b00000001);
	
	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(test1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(test2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(test3);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(test4);
	}

	lcd.setCursor(1,2);
	if (minmax&0b00000001==0b00000001) {
		lcd.writeFromPgmspace(strmin);
		lcd.setCursor(5, 2);
		if ( steppers::isAtMinimum(0) ) lcd.writeFromPgmspace(strY);
		else	lcd.writeFromPgmspace(strN);
	} else {
		lcd.writeFromPgmspace(strmin);
		lcd.setCursor(5, 2);
		if ( steppers::isAtMaximum(0) ) lcd.writeFromPgmspace(strY);
		else	lcd.writeFromPgmspace(strN);
	}

	lcd.setCursor(11,2);
	if (minmax&0b00000010==0b00000010) {
		lcd.writeFromPgmspace(strmin);
		lcd.setCursor(15, 2);
		if ( steppers::isAtMinimum(0) ) lcd.writeFromPgmspace(strY);
		else	lcd.writeFromPgmspace(strN);
	} else {
		lcd.writeFromPgmspace(strmin);
		lcd.setCursor(15, 2);
		if ( steppers::isAtMaximum(0) ) lcd.writeFromPgmspace(strY);
		else	lcd.writeFromPgmspace(strN);
	}
	
	lcd.setCursor(1,3);
	if (minmax&0b00000100==0b00000100) {
		lcd.writeFromPgmspace(strmin);
		lcd.setCursor(5, 3);
		if ( steppers::isAtMinimum(0) ) lcd.writeFromPgmspace(strY);
		else	lcd.writeFromPgmspace(strN);
	} else {
		lcd.writeFromPgmspace(strmin);
		lcd.setCursor(5, 3);
		if ( steppers::isAtMaximum(0) ) lcd.writeFromPgmspace(strY);
		else	lcd.writeFromPgmspace(strN);
	}
}

void TestEndStopsMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        	case ButtonArray::YMINUS:
        	case ButtonArray::ZMINUS:
        	case ButtonArray::YPLUS:
        	case ButtonArray::ZPLUS:
        	case ButtonArray::XMINUS:
        	case ButtonArray::XPLUS:
        	case ButtonArray::ZERO:
        	case ButtonArray::OK:
        	case ButtonArray::CANCEL:
               		interface::popScreen();
			break;
	}
}

void HomeAxisMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar home1[] = "Home Axis: ";
	const static PROGMEM prog_uchar home2[] = "   Y          Z ";
	const static PROGMEM prog_uchar home3[] = " X   X          ";
	const static PROGMEM prog_uchar home4[] = "   Y          Z ";

	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(home1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(home2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(home3);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(home4);
	}
}

void HomeAxisMode::home(ButtonArray::ButtonName direction) {
	uint8_t axis = 0;
	bool 	maximums;
	float	interval = 2000.0;


	switch(direction) {
	    case ButtonArray::XMINUS:
      case ButtonArray::XPLUS:
				axis 	 = 0x01;
				maximums = true;
  			interval *= AxisPerMM::stepsToMM((int32_t)47.06, AxisPerMM::AXIS_X); //Use ToM as baseline
				break;
      case ButtonArray::YMINUS:
      case ButtonArray::YPLUS:
				axis 	 = 0x02;
				maximums = false;
				interval *= AxisPerMM::stepsToMM((int32_t)47.06, AxisPerMM::AXIS_Y); //Use ToM as baseline
				break;
     	case ButtonArray::ZMINUS:
     	case ButtonArray::ZPLUS:
				axis 	 = 0x04;
				maximums = false;
				interval /= 4.0;	//Speed up Z
				interval *= AxisPerMM::stepsToMM((int32_t)200.0, AxisPerMM::AXIS_Z); //Use ToM as baseline
				break;
	}

	steppers::startHoming(maximums, axis, (uint32_t)interval);
}

void HomeAxisMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        	case ButtonArray::YMINUS:
        	case ButtonArray::ZMINUS:
        	case ButtonArray::YPLUS:
        	case ButtonArray::ZPLUS:
        	case ButtonArray::XMINUS:
        	case ButtonArray::XPLUS:
			home(button);
			break;
        	case ButtonArray::ZERO:
        	case ButtonArray::OK:
        	case ButtonArray::CANCEL:
			steppers::abort();
			steppers::enableAxis(0, false);
			steppers::enableAxis(1, false);
			steppers::enableAxis(2, false);
               		interface::popScreen();
			break;
	}
}


FilamentUsedResetMenu::FilamentUsedResetMenu() {
	itemCount = 4;
	reset();
}

void FilamentUsedResetMenu::resetState() {
	itemIndex = 2;
	firstItemIndex = 2;
}

void FilamentUsedResetMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg[]  = "Reset To Zero?";
	const static PROGMEM prog_uchar no[] = "No";
	const static PROGMEM prog_uchar yes[]= "Yes";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(msg);
		break;
	case 1:
		break;
	case 2:
		lcd.writeFromPgmspace(no);
		break;
	case 3:
		lcd.writeFromPgmspace(yes);
		break;
	}
}

void FilamentUsedResetMenu::handleSelect(uint8_t index) {
	switch (index) {
	case 3:
		//Reset to zero
                eeprom::putEepromInt64(mbeeprom::FILAMENT_USED, 0);
                eeprom::putEepromInt64(mbeeprom::FILAMENT_USED_TRIP, 0);
	case 2:
		interface::popScreen();
                interface::popScreen();
		break;
	}
}

void FilamentUsedMode::reset() {
	lifetimeDisplay = true;
	overrideForceRedraw = false;
}

void FilamentUsedMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar lifetime[] = "Lifetime Odo.:";
	const static PROGMEM prog_uchar trip[]	   = "Trip Odometer:";
	const static PROGMEM prog_uchar but_life[] = "(trip)   (reset)";
	const static PROGMEM prog_uchar but_trip[] = "(life)   (reset)";

	if ((forceRedraw) || (overrideForceRedraw)) {
		lcd.clear();

		lcd.setCursor(0,0);
		if ( lifetimeDisplay )	lcd.writeFromPgmspace(lifetime);
		else			lcd.writeFromPgmspace(trip);

	        int64_t filamentUsed = eeprom::getEepromInt64(mbeeprom::FILAMENT_USED, 0);

		if ( ! lifetimeDisplay ) {
			int64_t trip = eeprom::getEepromInt64(mbeeprom::FILAMENT_USED_TRIP, 0);
			filamentUsed = filamentUsed - trip;	
		}

		float filamentUsedMM = AxisPerMM::stepsToMM(filamentUsed, AxisPerMM::AXIS_A);

		lcd.setCursor(0,1);
		lcd.writeFloat(filamentUsedMM / 1000.0, 4);
		lcd.write('m');

		lcd.setCursor(0,2);
		if ( lifetimeDisplay )	lcd.writeFromPgmspace(but_life);
		else			lcd.writeFromPgmspace(but_trip);

		lcd.setCursor(0,3);
		lcd.writeFloat(((filamentUsedMM / 25.4) / 12.0), 4);
		lcd.writeString("ft");

		overrideForceRedraw = false;
	}
}

void FilamentUsedMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::CANCEL:
			interface::popScreen();
			break;
		case ButtonArray::ZERO:
			lifetimeDisplay ^= true;
			overrideForceRedraw = true;
			break;
		case ButtonArray::OK:
			if ( lifetimeDisplay )
				interface::pushScreen(&filamentUsedResetMenu);
			else {
                		eeprom::putEepromInt64(mbeeprom::FILAMENT_USED_TRIP, eeprom::getEepromInt64(mbeeprom::FILAMENT_USED, 0));
				interface::popScreen();
			}
			break;
		case ButtonArray::ZPLUS:
		case ButtonArray::ZMINUS:
		case ButtonArray::YPLUS:
		case ButtonArray::YMINUS:
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}
}

#define NUM_PROFILES 4
#define PROFILES_SAVED_AXIS 3

void writeProfileToEeprom(uint8_t pIndex, uint8_t *pName, int32_t homeX,
			  int32_t homeY, int32_t homeZ, uint8_t hbpTemp,
			  uint8_t tool0Temp, uint8_t tool1Temp, uint8_t extruderRpm) {
	uint16_t offset = mbeeprom::PROFILE_BASE + (uint16_t)pIndex * PROFILE_NEXT_OFFSET;

	cli();

	//Write profile name
	if ( pName )	eeprom_write_block(pName,(uint8_t*)offset, PROFILE_NAME_LENGTH);
	offset += PROFILE_NAME_LENGTH;

	//Write home axis
	eeprom_write_block(&homeX, (void*) offset, 4);		offset += 4;
	eeprom_write_block(&homeY, (void*) offset, 4);		offset += 4;
	eeprom_write_block(&homeZ, (void*) offset, 4);		offset += 4;

	//Write temps and extruder RPM
	eeprom_write_byte((uint8_t *)offset, hbpTemp);		offset += 1;
	eeprom_write_byte((uint8_t *)offset, tool0Temp);	offset += 1;
	eeprom_write_byte((uint8_t *)offset, tool1Temp);	offset += 1;
	eeprom_write_byte((uint8_t *)offset, extruderRpm);	offset += 1;
	
	sei();
}

void readProfileFromEeprom(uint8_t pIndex, uint8_t *pName, int32_t *homeX,
			   int32_t *homeY, int32_t *homeZ, uint8_t *hbpTemp,
			   uint8_t *tool0Temp, uint8_t *tool1Temp, uint8_t *extruderRpm) {
	uint16_t offset = mbeeprom::PROFILE_BASE + (uint16_t)pIndex * PROFILE_NEXT_OFFSET;

	cli();

	//Read profile name
	if ( pName )	eeprom_read_block(pName,(uint8_t*)offset, PROFILE_NAME_LENGTH);
	offset += PROFILE_NAME_LENGTH;

	//Write home axis
	eeprom_read_block(homeX, (void*) offset, 4);		offset += 4;
	eeprom_read_block(homeY, (void*) offset, 4);		offset += 4;
	eeprom_read_block(homeZ, (void*) offset, 4);		offset += 4;

	//Write temps and extruder RPM
	*hbpTemp	= eeprom_read_byte((uint8_t *)offset);	offset += 1;
	*tool0Temp	= eeprom_read_byte((uint8_t *)offset);	offset += 1;
	*tool1Temp	= eeprom_read_byte((uint8_t *)offset);	offset += 1;
	*extruderRpm	= eeprom_read_byte((uint8_t *)offset);	offset += 1;
	
	sei();
}

//buf should have length PROFILE_NAME_LENGTH + 1 

void getProfileName(uint8_t pIndex, uint8_t *buf) {
	uint16_t offset = mbeeprom::PROFILE_BASE + PROFILE_NEXT_OFFSET * (uint16_t)pIndex;

	cli();
	eeprom_read_block(buf,(void *)offset,PROFILE_NAME_LENGTH);
	sei();

	buf[PROFILE_NAME_LENGTH] = '\0';
}

#define NAME_CHAR_LOWER_LIMIT 32
#define NAME_CHAR_UPPER_LIMIT 126

bool isValidProfileName(uint8_t pIndex) {
	uint8_t buf[PROFILE_NAME_LENGTH + 1];

	getProfileName(pIndex, buf);
	for ( uint8_t i = 0; i < PROFILE_NAME_LENGTH; i ++ ) {
		if (( buf[i] < NAME_CHAR_LOWER_LIMIT ) || ( buf[i] > NAME_CHAR_UPPER_LIMIT ) || ( buf[i] == 0xff )) return false;
	}

	return true;
}

ProfilesMenu::ProfilesMenu() {
	itemCount = NUM_PROFILES;
	reset();

	//Setup defaults if required
	//If the value is 0xff, write the profile number
	uint8_t buf[PROFILE_NAME_LENGTH];

        const static PROGMEM prog_uchar defaultProfile[] =  "Profile?";

	//Get the home axis positions, we may need this to write the defaults
	homePosition = steppers::getPosition();

	for (uint8_t i = 0; i < PROFILES_SAVED_AXIS; i++) {
		uint16_t offset = mbeeprom::AXIS_HOME_POSITIONS + 4*(uint16_t)i;
		cli();
		eeprom_read_block(&homePosition[i], (void*)offset, 4);
		sei();
	}

	for (int i = 0; i < NUM_PROFILES; i ++ ) {
		if ( ! isValidProfileName(i)) {
			//Create the default profile name
			for( uint8_t i = 0; i < PROFILE_NAME_LENGTH; i ++ )
				buf[i] = pgm_read_byte_near(defaultProfile+i);
			buf[PROFILE_NAME_LENGTH - 1] = '1' + i;

			//Write the defaults
			writeProfileToEeprom(i, buf, homePosition[0], homePosition[1], homePosition[2],
					    100, 210, 210, 19);
		}
	}
}

void ProfilesMenu::resetState() {
	firstItemIndex = 0;
	itemIndex = 0;
}

void ProfilesMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	uint8_t buf[PROFILE_NAME_LENGTH + 1];

	getProfileName(index, buf);

	lcd.writeString((char *)buf);
}

void ProfilesMenu::handleSelect(uint8_t index) {
	profileSubMenu.profileIndex = index;
	interface::pushScreen(&profileSubMenu);
}

ProfileSubMenu::ProfileSubMenu() {
	itemCount = 4;
	reset();
}

void ProfileSubMenu::resetState() {
	itemIndex = 0;
	firstItemIndex = 0;
}

void ProfileSubMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg1[]  = "Restore";
	const static PROGMEM prog_uchar msg2[]  = "Display Config";
	const static PROGMEM prog_uchar msg3[]  = "Change Name";
	const static PROGMEM prog_uchar msg4[]  = "Save To Profile";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(msg1);
		break;
	case 1:
		lcd.writeFromPgmspace(msg2);
		break;
	case 2:
		lcd.writeFromPgmspace(msg3);
		break;
	case 3:
		lcd.writeFromPgmspace(msg4);
		break;
	}
}

void ProfileSubMenu::handleSelect(uint8_t index) {
	uint8_t hbpTemp, tool0Temp, tool1Temp, extruderRpm;

	switch (index) {
		case 0:
			//Restore
			//Read settings from eeprom
			readProfileFromEeprom(profileIndex, NULL, &homePosition[0], &homePosition[1], &homePosition[2],
					      &hbpTemp, &tool0Temp, &tool1Temp, &extruderRpm);

			//Write out the home offsets
			for (uint8_t i = 0; i < PROFILES_SAVED_AXIS; i++) {
				uint16_t offset = mbeeprom::AXIS_HOME_POSITIONS + 4*(uint16_t)i;
				cli();
				eeprom_write_block(&homePosition[i], (void*)offset, 4);
				sei();
			}

			cli();
			eeprom_write_byte((uint8_t *)mbeeprom::PLATFORM_TEMP, hbpTemp);
			eeprom_write_byte((uint8_t *)mbeeprom::TOOL0_TEMP,    tool0Temp);
			eeprom_write_byte((uint8_t *)mbeeprom::TOOL1_TEMP,    tool1Temp);
			eeprom_write_byte((uint8_t *)mbeeprom::EXTRUDE_RPM,   extruderRpm);
			sei();

                	interface::popScreen();
                	interface::popScreen();

			//Reset
			host::stopBuild();
			break;
		case 1:
			//Display settings
			profileDisplaySettingsMenu.profileIndex = profileIndex;
			interface::pushScreen(&profileDisplaySettingsMenu);
			break;
		case 2:
			//Change Profile Name
			profileChangeNameMode.profileIndex = profileIndex;
			interface::pushScreen(&profileChangeNameMode);
			break;
		case 3: //Save To Profile 
			//Get the home axis positions
			homePosition = steppers::getPosition();
			for (uint8_t i = 0; i < PROFILES_SAVED_AXIS; i++) {
				uint16_t offset = mbeeprom::AXIS_HOME_POSITIONS + 4*(uint16_t)i;
				cli();
				eeprom_read_block(&homePosition[i], (void*)offset, 4);
				sei();
			}

			hbpTemp		= eeprom::getEeprom8(mbeeprom::PLATFORM_TEMP, 110);
			tool0Temp	= eeprom::getEeprom8(mbeeprom::TOOL0_TEMP, 220);
			tool1Temp	= eeprom::getEeprom8(mbeeprom::TOOL1_TEMP, 220);
			extruderRpm	= eeprom::getEeprom8(mbeeprom::EXTRUDE_RPM, 19);

			writeProfileToEeprom(profileIndex, NULL, homePosition[0], homePosition[1], homePosition[2],
					     hbpTemp, tool0Temp, tool1Temp, extruderRpm);

                	interface::popScreen();
			break;
	}
}

void ProfileChangeNameMode::reset() {
	cursorLocation = 0;
	getProfileName(profileIndex, profileName);
}

void ProfileChangeNameMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Profile Name:";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]	   = " ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	lcd.setCursor(0,1);
	lcd.writeString((char *)profileName);

	//Draw the cursor
	lcd.setCursor(cursorLocation,2);
	lcd.write('^');

	//Write a blank before and after the cursor if we're not at the ends
	if ( cursorLocation >= 1 ) {
		lcd.setCursor(cursorLocation-1, 2);
		lcd.writeFromPgmspace(blank);
	}
	if ( cursorLocation < PROFILE_NAME_LENGTH ) {
		lcd.setCursor(cursorLocation+1, 2);
		lcd.writeFromPgmspace(blank);
	}
}

void ProfileChangeNameMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	uint16_t offset;

	switch (button) {
		case ButtonArray::CANCEL:
			interface::popScreen();
			break;
		case ButtonArray::ZERO:
			break;
		case ButtonArray::OK:
			//Write the profile name
			offset = mbeeprom::PROFILE_BASE + (uint16_t)profileIndex * PROFILE_NEXT_OFFSET;

			cli();
			eeprom_write_block(profileName,(uint8_t*)offset, PROFILE_NAME_LENGTH);
			sei();

			interface::popScreen();
			break;
		case ButtonArray::YPLUS:
			profileName[cursorLocation] += 1;
			break;
		case ButtonArray::ZPLUS:
			profileName[cursorLocation] += 5;
			break;
		case ButtonArray::YMINUS:
			profileName[cursorLocation] -= 1;
			break;
		case ButtonArray::ZMINUS:
			profileName[cursorLocation] -= 5;
			break;
		case ButtonArray::XMINUS:
			if ( cursorLocation > 0 )			cursorLocation --;
			break;
		case ButtonArray::XPLUS:
			if ( cursorLocation < (PROFILE_NAME_LENGTH-1) )	cursorLocation ++;
			break;
	}

	//Hard limits
	if ( profileName[cursorLocation] < NAME_CHAR_LOWER_LIMIT )	profileName[cursorLocation] = NAME_CHAR_LOWER_LIMIT;
	if ( profileName[cursorLocation] > NAME_CHAR_UPPER_LIMIT )	profileName[cursorLocation] = NAME_CHAR_UPPER_LIMIT;
}

ProfileDisplaySettingsMenu::ProfileDisplaySettingsMenu() {
	itemCount = 8;
	reset();
}

void ProfileDisplaySettingsMenu::resetState() {
	readProfileFromEeprom(profileIndex, profileName, &homeX, &homeY, &homeZ,
			      &hbpTemp, &tool0Temp, &tool1Temp, &extruderRpm);
	itemIndex = 2;
	firstItemIndex = 2;
}

void ProfileDisplaySettingsMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar xOffset[]     = "XOff: ";
	const static PROGMEM prog_uchar yOffset[]     = "YOff: ";
	const static PROGMEM prog_uchar zOffset[]     = "ZOff: ";
	const static PROGMEM prog_uchar hbp[]         = "HBP Temp:   ";
	const static PROGMEM prog_uchar tool0[]       = "Tool0 Temp: ";
	const static PROGMEM prog_uchar extruder[]    = "ExtrdrRPM: ";

	switch (index) {
	case 0:
		lcd.writeString((char *)profileName);
		break;
	case 2:
		lcd.writeFromPgmspace(xOffset);
		lcd.writeFloat(stepsToMM(homeX, AxisPerMM::AXIS_X), 3);
		break;
	case 3:
		lcd.writeFromPgmspace(yOffset);
		lcd.writeFloat(stepsToMM(homeY, AxisPerMM::AXIS_Y), 3);
		break;
	case 4:
		lcd.writeFromPgmspace(zOffset);
		lcd.writeFloat(stepsToMM(homeZ, AxisPerMM::AXIS_Z), 3);
		break;
	case 5:
		lcd.writeFromPgmspace(hbp);
		lcd.writeFloat((float)hbpTemp, 0);
		break;
	case 6:
		lcd.writeFromPgmspace(tool0);
		lcd.writeFloat((float)tool0Temp, 0);
		break;
	case 7:
		lcd.writeFromPgmspace(extruder);
		lcd.writeFloat((float)extruderRpm / 10.0, 1);
		break;
	}
}

void ProfileDisplaySettingsMenu::handleSelect(uint8_t index) {
}


UtilsMenu::UtilsMenu() {
	itemCount = 10;
	reset();
}

void UtilsMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar versions[]  		= "Version";
	const static PROGMEM prog_uchar filamentUsed[]	= "Filament Used";
	const static PROGMEM prog_uchar profiles[]			= "Profiles";
	const static PROGMEM prog_uchar testkeys[]  		= "Test Keys";
	const static PROGMEM prog_uchar endStops[]  		= "Test End Stops";
	const static PROGMEM prog_uchar calibrate[] 		= "Calibrate";
	const static PROGMEM prog_uchar homeAxis[]  		= "Home Axis";
	const static PROGMEM prog_uchar homeOffsets[]		= "Home Offsets";
	const static PROGMEM prog_uchar setupmenu[]  		= "Setup Menu > X+";
	const static PROGMEM prog_uchar mainmenu[]   		= "Main Menu  < X-";
	
	
	switch (index) {
	case 0:
		lcd.writeFromPgmspace(versions);
		break;
	case 1:
		lcd.writeFromPgmspace(filamentUsed);
		break;
	case 2:
		lcd.writeFromPgmspace(profiles);
		break;
	case 3:
		lcd.writeFromPgmspace(testkeys);
		break;
	case 4:
		lcd.writeFromPgmspace(endStops);
		break;
	case 5:
		lcd.writeFromPgmspace(calibrate);
		break;
	case 6:
		lcd.writeFromPgmspace(homeAxis);
		break;
	case 7:
		lcd.writeFromPgmspace(homeOffsets);
		break;
	case 8:
		lcd.writeFromPgmspace(setupmenu);
		break;
	case 9:
		lcd.writeFromPgmspace(mainmenu);
		break;
	}
}

void UtilsMenu::handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex) {
	switch (button) {
		case ButtonArray::XPLUS:
			interface::pushScreen(&setupMode);
			break;
		case ButtonArray::XMINUS:
			interface::popScreen();
			break;
	}
}

void UtilsMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 0:
			interface::pushScreen(&versionMode);
			break;
		case 1:
			interface::pushScreen(&filamentUsedMode);
			break;
		case 2:
			interface::pushScreen(&profilesMenu);
			break;
	  case 3:
      interface::pushScreen(&test);
			break;
		case 4:
			interface::pushScreen(&testEndStopsMode);
			break;
		case 5:
      interface::pushScreen(&calibrateMode);
			break;
		case 6:
			interface::pushScreen(&homeAxisMode);
			break;
		case 7:
      interface::pushScreen(&homeOffsetsMode);
			break;
		case 8:
      interface::pushScreen(&setupMode);
			break;
		case 9:
      interface::popScreen();
			break;
		}
}
