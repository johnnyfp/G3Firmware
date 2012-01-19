#include "Setup.hh"

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
#include "Eeprom.hh"
#include <avr/eeprom.h>
#include "ExtruderControl.hh"

Point homePosition;


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
	const static PROGMEM prog_uchar version1[] = "Firmware Version";
	const static PROGMEM prog_uchar version2[] = "----------------";
	const static PROGMEM prog_uchar version3[] = "    MBoard:__.__";
	const static PROGMEM prog_uchar version4[] = "  Extruder:__.__";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(version1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(version2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(version3);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(version4);

		//Display the motherboard version
		lcd.setCursor(13, 2);
		lcd.writeInt(firmware_version / 100, 1);

		lcd.setCursor(15, 2);
		lcd.writeInt(firmware_version-((firmware_version / 100)*100), 1);

		//Display the extruder version
		OutPacket responsePacket;

		if (extruderControl(SLAVE_CMD_VERSION, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t extruderVersion = responsePacket.read16(1);

			lcd.setCursor(13, 3);
			lcd.writeInt(extruderVersion / 100, 1);

			lcd.setCursor(15, 3);
			lcd.writeInt(extruderVersion-((extruderVersion / 100)*100), 1);
		} else {
			lcd.setCursor(13, 3);
			lcd.writeString("XX.XX");
		}
	} else {
	}
}

void VersionMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	interface::popScreen();
}

void DisplaySetupMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[] = "Select screen   ";
	static PROGMEM prog_uchar line2[] = "size:           ";
	static PROGMEM prog_uchar rst1[] 	= "Hit reset button";
	static PROGMEM prog_uchar rst2[]  = "to continue     ";
	if (resetRequired) {
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(rst1);
		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(rst2);
		while(1);
	} else {
		if (forceRedraw) {
			lcd.setCursor(0,0);
			lcd.writeFromPgmspace(line1);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(line2);
		}
		lcd.setCursor(6,1);
		lcd.writeInt(selSize,2);
	}
}

void DisplaySetupMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {

	case ButtonArray::OK:
		eeprom_write_byte((uint8_t *)mbeeprom::DISPLAY_SIZE, (uint8_t)selSize);
		resetRequired=true;
		break;
	case ButtonArray::ZMINUS:
		selSize=selSize+4;
		if (selSize>24) selSize=24;
		break;
	case ButtonArray::ZPLUS:
		selSize=selSize-4;
		if (selSize<16) selSize=16;
		break;
	case ButtonArray::CANCEL:
		interface::popScreen();
		break;
	}
}

void DisplaySetupMode::reset() {
	initSize=eeprom::getEeprom8(mbeeprom::DISPLAY_SIZE,16);
  selSize=initSize;
  resetRequired=false;
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

	switch(calibrationState) {
		case CS_HOME_Z:
			//Declare current position to be x=0, y=0, z=0, a=0, b=0
			steppers::definePosition(Point(0,0,0,0,0));
			steppers::startHoming(true, 0x04, (uint32_t)2000);
			calibrationState = CS_HOME_Z_WAIT;
			break;
		case CS_HOME_Z_WAIT:
			if ( ! steppers::isHoming() )	calibrationState = CS_HOME_Y;
			break;
		case CS_HOME_Y:
			steppers::startHoming(false, 0x02, (uint32_t)2000);
			calibrationState = CS_HOME_Y_WAIT;
			break;
		case CS_HOME_Y_WAIT:
			if ( ! steppers::isHoming() )	calibrationState = CS_HOME_X;
			break;
		case CS_HOME_X:
			steppers::startHoming(false, 0x01, (uint32_t)2000);
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
	const static PROGMEM prog_uchar message1x[] = "X Offset(steps):";
	const static PROGMEM prog_uchar message1y[] = "Y Offset(steps):";
	const static PROGMEM prog_uchar message1z[] = "Z Offset(steps):";
	const static PROGMEM prog_uchar message4[]  = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = " ";

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
			position = (float)homePosition[0];
			break;
		case HOS_OFFSET_Y:
			position = (float)homePosition[1];
			break;
		case HOS_OFFSET_Z:
			position = (float)homePosition[2];
			break;
	}

	lcd.setCursor(0,1);
	lcd.writeFloat((float)position, 0);

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
		case ButtonArray::OK:
			if 	( homeOffsetState == HOS_OFFSET_X )	homeOffsetState = HOS_OFFSET_Y;
			else if ( homeOffsetState == HOS_OFFSET_Y )	homeOffsetState = HOS_OFFSET_Z;
			break;
		case ButtonArray::ZPLUS:
			// increment more
			currentPosition += 5.0;
			break;
		case ButtonArray::ZMINUS:
			// decrement more
			currentPosition -= 5.0;
			break;
		case ButtonArray::YPLUS:
			// increment less
			currentPosition += 1;
			break;
		case ButtonArray::YMINUS:
			// decrement less
			currentPosition -= 1.0;
			break;
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}

	homePosition[currentIndex] = (int32_t)currentPosition;
}

void MoodLightMode::reset() {
	updatePhase = 0;
}

void MoodLightMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar mood1[] = "Mood: ";
	const static PROGMEM prog_uchar mood3_1[] = "(set RGB)";
	const static PROGMEM prog_uchar mood3_2[] = "(mood)";
	const static PROGMEM prog_uchar blank[]   = "          ";
	const static PROGMEM prog_uchar moodNotPresent1[] = "Mood Light not";
	const static PROGMEM prog_uchar moodNotPresent2[] = "present!!";
	const static PROGMEM prog_uchar moodNotPresent3[] = "See Thingiverse";
	const static PROGMEM prog_uchar moodNotPresent4[] = "   thing:15347";

	//If we have no mood light, point to thingiverse to make one
	if ( ! interface::moodLightController().blinkM.blinkMIsPresent ) {
		//Try once more to restart the mood light controller
		if ( ! interface::moodLightController().start() ) {
			if ( forceRedraw ) {
				lcd.clear();
				lcd.setCursor(0,0);
				lcd.writeFromPgmspace(moodNotPresent1);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(moodNotPresent2);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(moodNotPresent3);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(moodNotPresent4);
			}
		
			return;
		}
	}

	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(mood1);

		lcd.setCursor(10,2);
		lcd.writeFromPgmspace(mood3_2);
	}

 	//Redraw tool info
	uint8_t scriptId = eeprom_read_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT);

	switch (updatePhase) {
	case 0:
		lcd.setCursor(6, 0);
		lcd.writeFromPgmspace(blank);	
		lcd.setCursor(6, 0);
		lcd.writeFromPgmspace(interface::moodLightController().scriptIdToStr(scriptId));	
		break;

	case 1:
		lcd.setCursor(0, 2);
		if ( scriptId == 1 )	lcd.writeFromPgmspace(mood3_1);
		else			lcd.writeFromPgmspace(blank);	
		break;
	}

	updatePhase++;
	if (updatePhase > 1) {
		updatePhase = 0;
	}
}



void MoodLightMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	uint8_t scriptId;

	if ( ! interface::moodLightController().blinkM.blinkMIsPresent )	interface::popScreen();

	switch (button) {
        	case ButtonArray::OK:
			//Change the script to the next script id
			scriptId = eeprom_read_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT);
			scriptId = interface::moodLightController().nextScriptId(scriptId);
			eeprom_write_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT, scriptId);
			interface::moodLightController().playScript(scriptId);
			break;

        	case ButtonArray::ZERO:
			scriptId = eeprom_read_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT);
			if ( scriptId == 1 )
			{
				//Set RGB Values
                        	interface::pushScreen(&moodLightSetRGBScreen);
			}

			break;

        	case ButtonArray::YPLUS:
        	case ButtonArray::YMINUS:
        	case ButtonArray::XMINUS:
        	case ButtonArray::XPLUS:
        	case ButtonArray::ZMINUS:
        	case ButtonArray::ZPLUS:
        		break;

       	 	case ButtonArray::CANCEL:
               		interface::popScreen();
			break;
	}
}


void MoodLightSetRGBScreen::reset() {
	inputMode = 0;	//Red
	redrawScreen = false;

	red   = eeprom::getEeprom8(mbeeprom::MOOD_LIGHT_CUSTOM_RED,   255);;
	green = eeprom::getEeprom8(mbeeprom::MOOD_LIGHT_CUSTOM_GREEN, 255);;
	blue  = eeprom::getEeprom8(mbeeprom::MOOD_LIGHT_CUSTOM_BLUE,  255);;
}

void MoodLightSetRGBScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1_red[]   = "Red:";
	const static PROGMEM prog_uchar message1_green[] = "Green:";
	const static PROGMEM prog_uchar message1_blue[]  = "Blue:";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";

	if ((forceRedraw) || (redrawScreen)) {
		lcd.clear();

		lcd.setCursor(0,0);
		if      ( inputMode == 0 ) lcd.writeFromPgmspace(message1_red);
		else if ( inputMode == 1 ) lcd.writeFromPgmspace(message1_green);
		else if ( inputMode == 2 ) lcd.writeFromPgmspace(message1_blue);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);

		redrawScreen = false;
	}


	// Redraw tool info
	lcd.setCursor(0,1);
	if      ( inputMode == 0 ) lcd.writeInt(red,  3);
	else if ( inputMode == 1 ) lcd.writeInt(green,3);
	else if ( inputMode == 2 ) lcd.writeInt(blue, 3);
}

void MoodLightSetRGBScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	uint8_t *value = &red;

	if 	( inputMode == 1 )	value = &green;
	else if ( inputMode == 2 )	value = &blue;

	switch (button) {
        case ButtonArray::CANCEL:
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
        case ButtonArray::OK:
		if ( inputMode < 2 ) {
			inputMode ++;
			redrawScreen = true;
		} else {
			eeprom_write_byte((uint8_t*)mbeeprom::MOOD_LIGHT_CUSTOM_RED,  red);
			eeprom_write_byte((uint8_t*)mbeeprom::MOOD_LIGHT_CUSTOM_GREEN,green);
			eeprom_write_byte((uint8_t*)mbeeprom::MOOD_LIGHT_CUSTOM_BLUE, blue);

			//Set the color
			interface::moodLightController().playScript(1);

			interface::popScreen();
		}
		break;
        case ButtonArray::ZPLUS:
		// increment more
		if (*value <= 245) *value += 10;
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		if (*value >= 10) *value -= 10;
		break;
        case ButtonArray::YPLUS:
		// increment less
		if (*value <= 254) *value += 1;
		break;
        case ButtonArray::YMINUS:
		// decrement less
		if (*value >= 1) *value -= 1;
		break;

        case ButtonArray::XMINUS:
        case ButtonArray::XPLUS:
		break;
	}
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

	switch(direction) {
	        case ButtonArray::XMINUS:
      		case ButtonArray::XPLUS:
			axis 	 = 0x01;
			maximums = true;
			break;
        	case ButtonArray::YMINUS:
        	case ButtonArray::YPLUS:
			axis 	 = 0x02;
			maximums = false;
			break;
        	case ButtonArray::ZMINUS:
        	case ButtonArray::ZPLUS:
			axis 	 = 0x04;
			maximums = false;
			break;
	}

	steppers::startHoming(maximums, axis, (uint32_t)2000);
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

SetupMode::SetupMode() {
	itemCount = 10;
	reset();
}

void SetupMode::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar setupdisp[] = "Setup Display";
	const static PROGMEM prog_uchar testkeys[]  = "Test Keys";
	const static PROGMEM prog_uchar moodlight[] = "Mood Light";
	const static PROGMEM prog_uchar endStops[]  = "Test End Stops";
	const static PROGMEM prog_uchar versions[]  = "Version";
	const static PROGMEM prog_uchar homeAxis[]  = "Home Axis";
	const static PROGMEM prog_uchar mbprefs[]   = "MB Prefs ";
	const static PROGMEM prog_uchar thprefs[]   = "Toolhead Prefs";
	const static PROGMEM prog_uchar calibrate[] = "Calibrate";
	const static PROGMEM prog_uchar homeOffsets[]="Home Offsets";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(setupdisp);
		break;
	case 1:
		lcd.writeFromPgmspace(testkeys);
		break;
	case 2:
		lcd.writeFromPgmspace(homeAxis);
		break;
  case 3:
		lcd.writeFromPgmspace(moodlight);
		break;
	case 4:
		lcd.writeFromPgmspace(endStops);
		break;
	case 5:
		lcd.writeFromPgmspace(versions);
		break;
	case 6:
		lcd.writeFromPgmspace(mbprefs);
		break;
	case 7:
		lcd.writeFromPgmspace(thprefs);
		break;
	case 8:
		lcd.writeFromPgmspace(calibrate);
		break;
	case 9:
		lcd.writeFromPgmspace(homeOffsets);
		break;
	}
}

void SetupMode::handleSelect(uint8_t index) {
	switch (index) {
		case 0:
			// Show monitor build screen
			interface::pushScreen(&displaySetup);
			break;
		case 1:
			// Key Test
			interface::pushScreen(&test);
			break;
		case 2:
			// Show home axis
			interface::pushScreen(&homeAxisMode);
			break;
	  case 3:
			// Show Mood Light Mode
      interface::pushScreen(&moodLightMode);
			break;
		case 4:
			// Show test end stops menu
			interface::pushScreen(&testEndStopsMode);
			break;
		case 5:
			// Show build from SD screen
      interface::pushScreen(&versionMode);
			break;
		case 6:
			interface::pushScreen(&mbPrefMenu);
			break;
		case 7:
			interface::pushScreen(&notyetimplemented);
			break;
		case 8:
			// Show Calibrate Mode
      interface::pushScreen(&calibrateMode);
			break;
		case 9:
			// Show Home Offsets Mode
      interface::pushScreen(&homeOffsetsMode);
			break;
		}
}
