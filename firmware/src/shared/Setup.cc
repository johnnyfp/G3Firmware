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
#include <Math.h>
#include "SDCard.hh"
#include "SharedEepromMap.hh"
#include "eeprom.hh"
#include <avr/eeprom.h>
#include "ExtruderControl.hh"
#include "AxisPerMM.hh"
#include "Menu.hh"

int16_t overrideExtrudeSeconds = 0;


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

void MoodLightMode::reset() {
	updatePhase = 0;
	scriptId = eeprom_read_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT);

}

void MoodLightMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar mood1[] = "Mood: ";
	const static PROGMEM prog_uchar mood3_1[] = "(set RGB)";
	const static PROGMEM prog_uchar msg4[] = "Up/Dn/Ent to Set";
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

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(msg4);

	}

 	//Redraw tool info

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

	
	if ( ! interface::moodLightController().blinkM.blinkMIsPresent )	interface::popScreen();
	
	uint8_t i;
	
	switch (button) {
		case ButtonArray::OK:
		
			eeprom_write_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT, scriptId);
			interface::popScreen();
			break;
		
		case ButtonArray::ZERO:
			scriptId = eeprom_read_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT);
			if ( scriptId == 1 )
			{
			//Set RGB Values
			interface::pushScreen(&moodLightSetRGBScreen);
			}
			
			break;
		
		case ButtonArray::ZPLUS:
			// increment more
			for ( i = 0; i < 5; i ++ )
			scriptId = interface::moodLightController().nextScriptId(scriptId);
			interface::moodLightController().playScript(scriptId);
			break;
		
		case ButtonArray::ZMINUS:
			// decrement more
			for ( i = 0; i < 5; i ++ )
			scriptId = interface::moodLightController().prevScriptId(scriptId);
			interface::moodLightController().playScript(scriptId);
			break;
		
		case ButtonArray::YPLUS:
			// increment less
			scriptId = interface::moodLightController().nextScriptId(scriptId);
			interface::moodLightController().playScript(scriptId);
			break;
		
		case ButtonArray::YMINUS:
			// decrement less
			scriptId = interface::moodLightController().prevScriptId(scriptId);
			interface::moodLightController().playScript(scriptId);
			break;
		
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
		
		case ButtonArray::CANCEL:
			scriptId = eeprom_read_byte((uint8_t *)mbeeprom::MOOD_LIGHT_SCRIPT);
			interface::moodLightController().playScript(scriptId);
			
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


void BuzzerSetRepeatsMode::reset() {
	repeats = eeprom::getEeprom8(mbeeprom::BUZZER_REPEATS, 3);
}

void BuzzerSetRepeatsMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Repeat Buzzer:";
	const static PROGMEM prog_uchar message2[] = "(0=Buzzer Off)";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar times[]    = " times ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(message2);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(0,2);
	lcd.writeInt(repeats, 3);
	lcd.writeFromPgmspace(times);
}

void BuzzerSetRepeatsMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::CANCEL:
			interface::popScreen();
			break;
		case ButtonArray::ZERO:
			break;
		case ButtonArray::OK:
			eeprom_write_byte((uint8_t *)mbeeprom::BUZZER_REPEATS, repeats);
			interface::popScreen();
			break;
		case ButtonArray::ZPLUS:
			// increment more
			if (repeats <= 249) repeats += 5;
			break;
		case ButtonArray::ZMINUS:
			// decrement more
			if (repeats >= 5) repeats -= 5;
			break;
		case ButtonArray::YPLUS:
			// increment less
			if (repeats <= 253) repeats += 1;
			break;
		case ButtonArray::YMINUS:
			// decrement less
			if (repeats >= 1) repeats -= 1;
			break;
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}
}

void ABPCopiesSetScreen::reset() {
	value = eeprom::getEeprom8(mbeeprom::ABP_COPIES, 1);
	if ( value < 1 ) {
		eeprom_write_byte((uint8_t*)mbeeprom::ABP_COPIES,1);
		value = eeprom::getEeprom8(mbeeprom::ABP_COPIES, 1); //Just in case
	}
}

void ABPCopiesSetScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "ABP Copies (SD):";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(0,1);
	lcd.writeInt(value,3);
}

void ABPCopiesSetScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
		break;
        case ButtonArray::OK:
		eeprom_write_byte((uint8_t*)mbeeprom::ABP_COPIES,value);
		interface::popScreen();
		interface::popScreen();
		break;
        case ButtonArray::ZPLUS:
		// increment more
		if (value <= 249) {
			value += 5;
		}
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		if (value >= 6) {
			value -= 5;
		}
		break;
        case ButtonArray::YPLUS:
		// increment less
		if (value <= 253) {
			value += 1;
		}
		break;
        case ButtonArray::YMINUS:
		// decrement less
		if (value >= 2) {
			value -= 1;
		}
		break;

        case ButtonArray::XMINUS:
        case ButtonArray::XPLUS:
		break;
	}
}

PreheatDuringEstimateMenu::PreheatDuringEstimateMenu() {
	itemCount = 4;
	reset();
}

void PreheatDuringEstimateMenu::resetState() {
	if ( eeprom::getEeprom8(mbeeprom::PREHEAT_DURING_ESTIMATE, 0) ) 	itemIndex = 3;
	else						 		itemIndex = 2;
	firstItemIndex = 2;
}

void PreheatDuringEstimateMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg1[]    = "Preheat during";
	const static PROGMEM prog_uchar msg2[]    = "estimate phase:";
	const static PROGMEM prog_uchar disable[] = "Disable";
	const static PROGMEM prog_uchar enable[]  = "Enable";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(msg1);
		break;
	case 1:
		lcd.writeFromPgmspace(msg2);
		break;
	case 2:
		lcd.writeFromPgmspace(disable);
		break;
	case 3:
		lcd.writeFromPgmspace(enable);
		break;
	}
}

void PreheatDuringEstimateMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 2:
			eeprom_write_byte((uint8_t*)mbeeprom::PREHEAT_DURING_ESTIMATE,0);
			interface::popScreen();
			break;
		case 3:
			eeprom_write_byte((uint8_t*)mbeeprom::PREHEAT_DURING_ESTIMATE,1);
                	interface::popScreen();
			break;
	}
}

OverrideGCodeTempMenu::OverrideGCodeTempMenu() {
	itemCount = 4;
	reset();
}

void OverrideGCodeTempMenu::resetState() {
	if ( eeprom::getEeprom8(mbeeprom::OVERRIDE_GCODE_TEMP, 0) ) 	itemIndex = 3;
	else						 		itemIndex = 2;
	firstItemIndex = 2;
}

void OverrideGCodeTempMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg1[]   = "Override GCode";
	const static PROGMEM prog_uchar msg2[]   = "Temperature:";
	const static PROGMEM prog_uchar disable[] =  "Disable";
	const static PROGMEM prog_uchar enable[]  =  "Enable";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(msg1);
		break;
	case 1:
		lcd.writeFromPgmspace(msg2);
		break;
	case 2:
		lcd.writeFromPgmspace(disable);
		break;
	case 3:
		lcd.writeFromPgmspace(enable);
		break;
	}
}

void OverrideGCodeTempMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 2:  
			eeprom_write_byte((uint8_t*)mbeeprom::OVERRIDE_GCODE_TEMP,0);
			interface::popScreen();
			break;
		case 3:
			eeprom_write_byte((uint8_t*)mbeeprom::OVERRIDE_GCODE_TEMP,1);
                	interface::popScreen();
			break;
	}
}

StepperDriverAcceleratedMenu::StepperDriverAcceleratedMenu() {
	itemCount = 5;
	reset();
}

void StepperDriverAcceleratedMenu::resetState() {
	uint8_t accel = eeprom::getEeprom8(mbeeprom::STEPPER_DRIVER, 0);
	if	( accel == 0x03 )	itemIndex = 4;
	else if ( accel == 0x01 )	itemIndex = 3;
	else				itemIndex = 2;
	firstItemIndex = 2;
}

void StepperDriverAcceleratedMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg1[]   = "Accelerated";
	const static PROGMEM prog_uchar msg2[]   = "Stepper Driver:";
	const static PROGMEM prog_uchar off[]    =  "Off";
	const static PROGMEM prog_uchar on[]     =  "On - No Planner";
	const static PROGMEM prog_uchar planner[]=  "On - Planner";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(msg1);
		break;
	case 1:
		lcd.writeFromPgmspace(msg2);
		break;
	case 2:
		lcd.writeFromPgmspace(off);
		break;
	case 3:
		lcd.writeFromPgmspace(on);
		break;
	case 4:
		lcd.writeFromPgmspace(planner);
		break;
	}
}


void AcceleratedSettingsMode::reset() {
	cli();
	values[0]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_X, 160);
	values[1]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_Y, 160);
	values[2]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_Z, 10);
	values[3]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_A, 100);
        values[4]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_X, 2000);
        values[5]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_Y, 2000);
        values[6]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_Z, 150);
        values[7]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_A, 60000);
        values[8]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_EXTRUDER_NORM, 5000);
        values[9]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_EXTRUDER_RETRACT, 3000);
	values[10]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MIN_FEED_RATE,0);
	values[11]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MIN_TRAVEL_FEED_RATE,0);
	values[12]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_XY_JERK,2);
	values[13]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_MAX_Z_JERK,100);
	values[14]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_ADVANCE_K,50);
	values[15]	= eeprom::getEepromUInt32(mbeeprom::ACCEL_FILAMENT_DIAMETER,175);
	sei();

	lastAccelerateSettingsState= AS_NONE;
	accelerateSettingsState= AS_MAX_FEEDRATE_X;
}

void AcceleratedSettingsMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1xMaxFeedRate[]  	= "X MaxFeedRate:";
	const static PROGMEM prog_uchar message1yMaxFeedRate[]  	= "Y MaxFeedRate:";
	const static PROGMEM prog_uchar message1zMaxFeedRate[]  	= "Z MaxFeedRate:";
	const static PROGMEM prog_uchar message1aMaxFeedRate[]  	= "A MaxFeedRate:";
	const static PROGMEM prog_uchar message1xMaxAccelRate[] 	= "X Max Accel:";
	const static PROGMEM prog_uchar message1yMaxAccelRate[] 	= "Y Max Accel:";
	const static PROGMEM prog_uchar message1zMaxAccelRate[] 	= "Z Max Accel:";
	const static PROGMEM prog_uchar message1aMaxAccelRate[] 	= "A Max Accel:";
	const static PROGMEM prog_uchar message1ExtruderNorm[]  	= "Acc Norm Move:";
	const static PROGMEM prog_uchar message1ExtruderRetract[]	= "Acc Extr Move:";
	const static PROGMEM prog_uchar message1MinFeedRate[]		= "Min Feed Rate:";
	const static PROGMEM prog_uchar message1MinTravelFeedRate[]	= "MinTrvlFeedRate:";
	const static PROGMEM prog_uchar message1MaxXYJerk[]		= "Max XY Jerk:";
	const static PROGMEM prog_uchar message1MaxZJerk[]		= "Max Z Jerk:";
	const static PROGMEM prog_uchar message1AdvanceK[]		= "Advance K:";
	const static PROGMEM prog_uchar message1FilamentDiameter[]	= "Filament Dia:";
	const static PROGMEM prog_uchar message4[]  = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = "    ";

	if ( accelerateSettingsState != lastAccelerateSettingsState )	forceRedraw = true;

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		switch(accelerateSettingsState) {
			case AS_MAX_FEEDRATE_X:
				lcd.writeFromPgmspace(message1xMaxFeedRate);
				break;
                	case AS_MAX_FEEDRATE_Y:
				lcd.writeFromPgmspace(message1yMaxFeedRate);
				break;
                	case AS_MAX_FEEDRATE_Z:
				lcd.writeFromPgmspace(message1zMaxFeedRate);
				break;
                	case AS_MAX_FEEDRATE_A:
				lcd.writeFromPgmspace(message1aMaxFeedRate);
				break;
                	case AS_MAX_ACCELERATION_X:
				lcd.writeFromPgmspace(message1xMaxAccelRate);
				break;
                	case AS_MAX_ACCELERATION_Y:
				lcd.writeFromPgmspace(message1yMaxAccelRate);
				break;
                	case AS_MAX_ACCELERATION_Z:
				lcd.writeFromPgmspace(message1zMaxAccelRate);
				break;
                	case AS_MAX_ACCELERATION_A:
				lcd.writeFromPgmspace(message1aMaxAccelRate);
				break;
                	case AS_MAX_EXTRUDER_NORM:
				lcd.writeFromPgmspace(message1ExtruderNorm);
				break;
                	case AS_MAX_EXTRUDER_RETRACT:
				lcd.writeFromPgmspace(message1ExtruderRetract);
				break;
                	case AS_MIN_FEED_RATE:
				lcd.writeFromPgmspace(message1MinFeedRate);
				break;
                	case AS_MIN_TRAVEL_FEED_RATE:
				lcd.writeFromPgmspace(message1MinTravelFeedRate);
				break;
                	case AS_MAX_XY_JERK:
				lcd.writeFromPgmspace(message1MaxXYJerk);
				break;
                	case AS_MAX_Z_JERK:
				lcd.writeFromPgmspace(message1MaxZJerk);
				break;
                	case AS_ADVANCE_K:
				lcd.writeFromPgmspace(message1AdvanceK);
				break;
                	case AS_FILAMENT_DIAMETER:
				lcd.writeFromPgmspace(message1FilamentDiameter);
				break;
		}

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	uint32_t value = 0;

	uint8_t currentIndex = accelerateSettingsState - AS_MAX_FEEDRATE_X;

	value = values[currentIndex];

	lcd.setCursor(0,1);

	switch(accelerateSettingsState) {
		case AS_MIN_FEED_RATE:
		case AS_MIN_TRAVEL_FEED_RATE:
		case AS_MAX_XY_JERK:
		case AS_MAX_Z_JERK:
					lcd.writeFloat((float)value / 10.0, 1);
					break;
		case AS_ADVANCE_K:
					lcd.writeFloat((float)value / 100000.0, 5);
					break;
		case AS_FILAMENT_DIAMETER:
					lcd.writeFloat((float)value / 100.0, 2);
					break;
		default:
					lcd.writeFloat((float)value, 0);
					break;
	}

	lcd.writeFromPgmspace(blank);

	lastAccelerateSettingsState = accelerateSettingsState;
}

void AcceleratedSettingsMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	if (( accelerateSettingsState == AS_FILAMENT_DIAMETER ) && (button == ButtonArray::OK )) {
		//Write the data
		cli();
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_X,		values[0]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_Y,		values[1]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_Z,		values[2]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_FEEDRATE_A,		values[3]);
 		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_X,	values[4]);
  	eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_Y,	values[5]);
  	eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_Z,	values[6]);
  	eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_ACCELERATION_A,	values[7]);
  	eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_EXTRUDER_NORM,	values[8]);
  	eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_EXTRUDER_RETRACT,	values[9]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MIN_FEED_RATE,		values[10]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MIN_TRAVEL_FEED_RATE,	values[11]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_XY_JERK,		values[12]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_MAX_Z_JERK,		values[13]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_ADVANCE_K,		values[14]);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_FILAMENT_DIAMETER,	values[15]);
		sei();

		host::stopBuild();
		return;
	}

	uint8_t currentIndex = accelerateSettingsState - AS_MAX_FEEDRATE_X;

	uint32_t lastValue = values[currentIndex];

	switch (button) {
		case ButtonArray::CANCEL:
			interface::popScreen();
			break;
		case ButtonArray::ZERO:
			break;
		case ButtonArray::OK:
			accelerateSettingsState = (enum accelerateSettingsState)((uint8_t)accelerateSettingsState + 1);
			return;
			break;
		case ButtonArray::ZPLUS:
			// increment more
			values[currentIndex] += 100;
			break;
		case ButtonArray::ZMINUS:
			// decrement more
			values[currentIndex] -= 100;
			break;
		case ButtonArray::YPLUS:
			// increment less
			values[currentIndex] += 1;
			break;
		case ButtonArray::YMINUS:
			// decrement less
			values[currentIndex] -= 1;
			break;
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}

	if (!(( accelerateSettingsState == AS_MIN_FEED_RATE ) || ( accelerateSettingsState == AS_MIN_TRAVEL_FEED_RATE ) || ( accelerateSettingsState == AS_ADVANCE_K ))) {
		if ( values[currentIndex] < 1 )	values[currentIndex] = 1;
	}

	if ( values[currentIndex] > 200000 ) values[currentIndex] = 1;
}

AccelerationMenu::AccelerationMenu() {
	itemCount = 3;

	reset();
}

void AccelerationMenu::resetState() {
	if ( eeprom::getEeprom8(mbeeprom::STEPPER_DRIVER, 0) )	acceleration = true;
	else							acceleration = false;

	if ( acceleration )	itemCount = 3;
	else			itemCount = 1;

	itemIndex = 0;
	firstItemIndex = 0;
}

void AccelerationMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg1[]  = "Stepper Driver";
	const static PROGMEM prog_uchar msg2[]  = "Accel. Settings";
	const static PROGMEM prog_uchar msg3[]  = "Extdr. Steps/mm";

	if (( ! acceleration ) && ( index > 0 ))	return;

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
	}
}

void AccelerationMenu::handleSelect(uint8_t index) {
	if (( ! acceleration ) && ( index > 0 ))	return;

	switch (index) {
		case 0:
			interface::pushScreen(&stepperDriverAcceleratedMenu);
			break;
		case 1:
			interface::pushScreen(&acceleratedSettingsMode);
			break;
		case 2:
			interface::pushScreen(&eStepsPerMMMode);
			break;
	}
}

void EStepsPerMMMode::reset() {
	value = eeprom::getEepromUInt32(mbeeprom::ACCEL_E_STEPS_PER_MM, 44);
	if ( value < 1 ) {
		eeprom::putEepromUInt32(mbeeprom::ACCEL_E_STEPS_PER_MM, 44);
		value = eeprom::getEepromUInt32(mbeeprom::ACCEL_E_STEPS_PER_MM, 44); //Just in case
	}
}

void EStepsPerMMMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Extrdr Steps/mm:";
	const static PROGMEM prog_uchar message2[] = "(calib)";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = "  ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(message2);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(8,2);
	lcd.writeFloat((float)value / 10.0,1);
	lcd.writeFromPgmspace(blank);
}

void EStepsPerMMMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
		break;
        case ButtonArray::OK:
		eeprom::putEepromUInt32(mbeeprom::ACCEL_E_STEPS_PER_MM,value);
		//Reset to read in the new value
		host::stopBuild();
		return;
		break;
        case ButtonArray::ZPLUS:
		// increment more
		value += 25;
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		value -= 25;
		break;
        case ButtonArray::YPLUS:
		// increment less
		value += 1;
		break;
        case ButtonArray::YMINUS:
		// decrement less
		value -= 1;
		break;

        case ButtonArray::XMINUS:
		interface::pushScreen(&eStepsPerMMStepsMode);
		break;
        case ButtonArray::XPLUS:
		break;
	}

	if (( value < 1 ) || ( value > 200000 )) value = 1;
}

void EStepsPerMMStepsMode::reset() {
	value = 200;
	steppers::switchToRegularDriver();
	overrideExtrudeSeconds = 0;
}

void EStepsPerMMStepsMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Extrude N steps:";
	const static PROGMEM prog_uchar message2[] = "(extrude)";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = " ";

	if (overrideExtrudeSeconds)	extrude(true);

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(message2);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(10,2);
	lcd.writeFloat((float)value,0);
	lcd.writeFromPgmspace(blank);
}

void EStepsPerMMStepsMode::extrude(bool overrideTempCheck) {
	//Check we're hot enough
	if ( ! overrideTempCheck )
	{
		OutPacket responsePacket;
		if (extruderControl(SLAVE_CMD_IS_TOOL_READY, EXTDR_CMD_GET, responsePacket, 0)) {
			uint8_t data = responsePacket.read8(1);
		
			if ( ! data )
			{
				overrideExtrudeSeconds = 1;
				interface::pushScreen(&extruderTooColdMenu);
				return;
			}
		}
	}

	Point position = steppers::getPosition();

	float rpm = (float)eeprom::getEeprom8(mbeeprom::EXTRUDE_RPM, 19) / 10.0;

	//60 * 1000000 = # uS in a minute
	//200 * 8 = 200 steps per revolution * 1/8 stepping
	int32_t interval = (int32_t)(60L * 1000000L) / (int32_t)((float)(200 * 8) * rpm);

	position[3] += -value;
	steppers::setTarget(position, interval);

	if (overrideTempCheck)	overrideExtrudeSeconds = 0;
}


void EStepsPerMMStepsMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
    		steppers::switchToAcceleratedDriver();
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
		break;
        case ButtonArray::OK:
    		steppers::switchToAcceleratedDriver();
		eStepsPerMMLengthMode.steps = value;
		interface::pushScreen(&eStepsPerMMLengthMode);
		break;
        case ButtonArray::ZPLUS:
		// increment more
		value += 25;
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		value -= 25;
		break;
        case ButtonArray::YPLUS:
		// increment less
		value += 1;
		break;
        case ButtonArray::YMINUS:
		// decrement less
		value -= 1;
		break;

        case ButtonArray::XMINUS:
		extrude(false);
		break;
        case ButtonArray::XPLUS:
		break;
	}
}

void EStepsPerMMLengthMode::reset() {
	value = 1;
}

void EStepsPerMMLengthMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Enter noodle";
	const static PROGMEM prog_uchar message2[] = "length in mm's";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = "  ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(message2);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(0,2);
	lcd.writeFloat((float)value,0);
	lcd.writeFromPgmspace(blank);
}

void EStepsPerMMLengthMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	uint32_t espm;

	switch (button) {
        case ButtonArray::CANCEL:
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
		break;
        case ButtonArray::OK:
		if ( steps < 0 ) steps *= -1;
		espm = (uint32_t)lround(((float)steps / (float)value) * 10.0);
		eeprom::putEepromUInt32(mbeeprom::ACCEL_E_STEPS_PER_MM,espm);
	
		//Reset to read in the new value
		host::stopBuild();
		return;
		break;
        case ButtonArray::ZPLUS:
		// increment more
		value += 5;
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		value -= 5;
		break;
        case ButtonArray::YPLUS:
		// increment less
		value += 1;
		break;
        case ButtonArray::YMINUS:
		// decrement less
		value -= 1;
		break;

        case ButtonArray::XMINUS:
        case ButtonArray::XPLUS:
		break;
	}

	if (( value < 1 ) || ( value > 200000 )) value = 1;
}

ExtruderTooColdMenu::ExtruderTooColdMenu() {
	itemCount = 4;
	reset();
}

void ExtruderTooColdMenu::resetState() {
	itemIndex = 2;
	firstItemIndex = 2;
}

void ExtruderTooColdMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar warning[]  = "Tool0 too cold!";
	const static PROGMEM prog_uchar cancel[]   =  "Cancel";
	const static PROGMEM prog_uchar override[] =  "Override";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(warning);
		break;
	case 1:
		break;
	case 2:
		lcd.writeFromPgmspace(cancel);
		break;
	case 3:
		lcd.writeFromPgmspace(override);
		break;
	}
}

void ExtruderTooColdMenu::handleSelect(uint8_t index) {
	switch (index) {
	case 2:
		// Cancel extrude
		overrideExtrudeSeconds = 0;
		interface::popScreen();
		break;
	case 3:
		// Override and extrude
                interface::popScreen();
		break;
	}
}

void StepperDriverAcceleratedMenu::handleSelect(uint8_t index) {
	uint8_t oldValue = eeprom::getEeprom8(mbeeprom::STEPPER_DRIVER, 0);
	uint8_t newValue = oldValue;
	
	switch (index) {
		case 2:  
			newValue = 0x00;
			interface::popScreen();
			break;
		case 3:
			newValue = 0x01;
                	interface::popScreen();
			break;
		case 4:
			newValue = 0x03;
                	interface::popScreen();
			break;
	}

	//If the value has changed, do a reset
	if ( newValue != oldValue ) {
		cli();
		eeprom_write_byte((uint8_t*)mbeeprom::STEPPER_DRIVER, newValue);
		sei();
		//Reset
		host::stopBuild();
	}
}


SetupMode::SetupMode() {
	itemCount = 9;
	reset();
	AxisPerMM::populateAxisArray();
}

void SetupMode::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar setupdisp[] = "Setup Display";
	const static PROGMEM prog_uchar moodlight[] = "Mood Light";
	const static PROGMEM prog_uchar buzzer[]	  = "Buzzer";
	const static PROGMEM prog_uchar item1[] 		= "EstimatePreheat";
	const static PROGMEM prog_uchar item2[] 		= "Override Temp";
	const static PROGMEM prog_uchar item3[] 		= "ABP Copies (SD)";
	const static PROGMEM prog_uchar item4[] 		= "Acceleration";
	const static PROGMEM prog_uchar mbprefs[]   = "MB Prefs   > X+";
	const static PROGMEM prog_uchar utilsmenu[] = "Utils Menu < X-";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(setupdisp);
		break;
  case 1:
		lcd.writeFromPgmspace(moodlight);
		break;
	case 2:
		lcd.writeFromPgmspace(buzzer);
		break;
	case 3:
		lcd.writeFromPgmspace(item1);
		break;
	case 4:
		lcd.writeFromPgmspace(item2);
		break;
	case 5:
		lcd.writeFromPgmspace(item3);
		break;
	case 6:
		lcd.writeFromPgmspace(item4);
		break;
	case 7:
		lcd.writeFromPgmspace(mbprefs);
		break;
	case 8:
		lcd.writeFromPgmspace(utilsmenu);
		break;
	}
}

void SetupMode::handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex) {
	switch (button) {
		case ButtonArray::XPLUS:
			interface::pushScreen(&mbPrefMenu);
			break;
		case ButtonArray::XMINUS:
			interface::popScreen();
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
			interface::pushScreen(&moodLightMode);
			break;
		case 2:
			// Show home axis
			interface::pushScreen(&buzzerSetRepeats);
			break;
	  case 3:
			//Preheat during estimation
			interface::pushScreen(&preheatDuringEstimateMenu);
			break;
		case 4:
			//Override the gcode temperature
			interface::pushScreen(&overrideGCodeTempMenu);
			break;
		case 5:
			//Change number of ABP copies
			interface::pushScreen(&abpCopiesSetScreen);
			break;
		case 6:
			//Acceleration menu
			interface::pushScreen(&accelerationMenu);
			break;
		case 7:
      interface::pushScreen(&mbPrefMenu);
			break;
		case 8:
      interface::popScreen();
			break;	
		}
}
