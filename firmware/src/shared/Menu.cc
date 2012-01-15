#include "Menu.hh"
#include "Configuration.hh"

// TODO: Kill this, should be hanlded by build system.
#ifdef HAS_INTERFACE_BOARD

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
#include "EepromMap.hh"
#include "Eeprom.hh"
#include <avr/eeprom.h>
#include "ExtruderControl.hh"


#define HOST_PACKET_TIMEOUT_MS 20
#define HOST_PACKET_TIMEOUT_MICROS (1000L*HOST_PACKET_TIMEOUT_MS)

#define HOST_TOOL_RESPONSE_TIMEOUT_MS 50
#define HOST_TOOL_RESPONSE_TIMEOUT_MICROS (1000L*HOST_TOOL_RESPONSE_TIMEOUT_MS)

#define MAX_ITEMS_PER_SCREEN 4

int16_t overrideExtrudeSeconds=0;
uint8_t pauseType = 1;

Point pausedPosition, homePosition;

float getRevsPerMM(){
	uint16_t whole = eeprom::getEeprom16(eeprom::ZAXIS_MM_PER_TURN_W, 200);
	uint16_t frac  = eeprom::getEeprom16(eeprom::ZAXIS_MM_PER_TURN_P, 0);
	return ((float)whole+((float)frac/10000));	
}

uint16_t pow (uint16_t x, uint8_t y) {
    int i,base;
    base = 1;
    for (i = 1; i <= y; ++i) base *= x;
    return base;
}

void strcat(char *buf, const char* str)
{
	char *ptr = buf;
	while (*ptr) ptr++;
	while (*str) *ptr++ = *str++;
	*ptr++ = '\0';
}


int appendTime(char *buf, uint8_t buflen, uint32_t val)
{
	bool hasdigit = false;
	uint8_t idx = 0;
	uint8_t written = 0;

	if (buflen < 1) {
		return written;
	}

	while (idx < buflen && buf[idx]) idx++;
	if (idx >= buflen-1) {
		buf[buflen-1] = '\0';
		return written;
	}

	uint8_t radidx = 0;
	const uint8_t radixcount = 5;
	const uint8_t houridx = 2;
	const uint8_t minuteidx = 4;
	uint32_t radixes[radixcount] = {360000, 36000, 3600, 600, 60};
	if (val >= 3600000) {
		val %= 3600000;
	}
	for (radidx = 0; radidx < radixcount; radidx++) {
		char digit = '0';
		uint8_t bit = 8;
		uint32_t radshift = radixes[radidx] << 3;
		for (; bit > 0; bit >>= 1, radshift >>= 1) {
			if (val > radshift) {
				val -= radshift;
				digit += bit;
			}
		}
		if (hasdigit || digit != '0' || radidx >= houridx) {
			buf[idx++] = digit;
			hasdigit = true;
		} else {
			buf[idx++] = ' ';
		}
		if (idx >= buflen) {
			buf[buflen-1] = '\0';
			return written;
		}
		written++;
		if (radidx == houridx) {
			buf[idx++] = 'h';
			if (idx >= buflen) {
				buf[buflen-1] = '\0';
				return written;
			}
			written++;
		}
		if (radidx == minuteidx) {
			buf[idx++] = 'm';
			if (idx >= buflen) {
				buf[buflen-1] = '\0';
				return written;
			}
			written++;
		}
	}

	if (idx < buflen) {
		buf[idx] = '\0';
	} else {
		buf[buflen-1] = '\0';
	}

	return written;
}



int appendUint8(char *buf, uint8_t buflen, uint8_t val)
{
	bool hasdigit = false;
	uint8_t written = 0;
	uint8_t idx = 0;

	if (buflen < 1) {
		return written;
	}

	while (idx < buflen && buf[idx]) idx++;
	if (idx >= buflen-1) {
		buf[buflen-1] = '\0';
		return written;
	}

	if (val >= 100) {
		uint8_t res = val / 100;
		val -= res * 100;
		buf[idx++] = '0' + res;
		if (idx >= buflen) {
			buf[buflen-1] = '\0';
			return written;
		}
		hasdigit = true;
		written++;
	} else {
		buf[idx++] = ' ';
		if (idx >= buflen) {
			buf[buflen-1] = '\0';
			return written;
		}
		written++;
	}

	if (val >= 10 || hasdigit) {
		uint8_t res = val / 10;
		val -= res * 10;
		buf[idx++] = '0' + res;
		if (idx >= buflen) {
			buf[buflen-1] = '\0';
			return written;
		}
		hasdigit = true;
		written++;
	} else {
		buf[idx++] = ' ';
		if (idx >= buflen) {
			buf[buflen-1] = '\0';
			return written;
		}
		written++;
	}

	buf[idx++] = '0' + val;
	if (idx >= buflen) {
		buf[buflen-1] = '\0';
		return written;
	}
	written++;

	if (idx < buflen) {
		buf[idx] = '\0';
	} else {
		buf[buflen-1] = '\0';
	}

	return written;
}

void SplashScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar splash1[] = "                ";
	static PROGMEM prog_uchar splash2[] = "Gen4 Electronics";
	static PROGMEM prog_uchar splash3[] = "----------------";
  static PROGMEM prog_uchar splash4[] = "            v";



	if (forceRedraw) {
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(splash1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(splash2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(splash3);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(splash4);
		lcd.writeFloat((float)VERSION/100,2);
	}
	else {
		// The machine has started, so we're done!
                interface::popScreen();
        }
}

void SplashScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	// We can't really do anything, since the machine is still loading, so ignore.
}

void SplashScreen::reset() {
}

SetupMode::SetupMode() {
	itemCount = 8;
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
		case 7:
			break;
		}
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
		eeprom_write_byte((uint8_t *)eeprom::DISPLAY_SIZE, (uint8_t)selSize);
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
	initSize=eeprom::getEeprom8(eeprom::DISPLAY_SIZE,16);
  selSize=initSize;
  resetRequired=false;
}

void ZAxisRevs::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[]   = "ZAxis rev per mm";
	static PROGMEM prog_uchar choice0[] = "Thing-o-matic";
	static PROGMEM prog_uchar choice1[] = "Cupcake";
	static PROGMEM prog_uchar choice2[] = "Ultimaker";
	static PROGMEM prog_uchar choice3[] = "RepRap";
	static PROGMEM prog_uchar choice4[] = "Mendel";
	static PROGMEM prog_uchar choice5[] = "Custom";
	static PROGMEM prog_uchar hat[] = "^";
	if (forceRedraw) {
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(line1);
	} 
	lcd.writeBlankLine(1);
	switch(selSize) {
		case(0):
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(choice0);
		break;
		case(1):
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(choice1);
		break;
		case(2):
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(choice2);
		break;
		case(3):
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(choice3);
		break;
		case(4):
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(choice4);
		break;
		default:
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(choice5);
	}
	lcd.writeBlankLine(3);
	if (selSize>4) {
		if (selSize>8)lcd.setCursor(selSize-4,3);
		else lcd.setCursor(selSize-5,3);
		lcd.writeFromPgmspace(hat);
	}
			
	lcd.writeBlankLine(2);
	lcd.setCursor(0,2);
	lcd.writeInt(wholePart,4);
	lcd.writeString(".");
	lcd.writeInt(fracPart,4);
	lcd.writeString("mm");
	
}

void ZAxisRevs::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
 	  case ButtonArray::CANCEL:
		  interface::popScreen();
		break;
		case ButtonArray::OK:
			eeprom_write_word((uint16_t*)eeprom::ZAXIS_MM_PER_TURN_W,wholePart);
			eeprom_write_word((uint16_t*)eeprom::ZAXIS_MM_PER_TURN_P,fracPart);
		  interface::popScreen();
		break;
		case ButtonArray::XMINUS:
			if (selSize>4) selSize--;
		break;
		case ButtonArray::XPLUS:
			selSize++;
			if (selSize>12) selSize=12;
		break;
		case ButtonArray::YMINUS:
			if (selSize<5) selSize++;
			else {
				if (selSize<9) {
					wholePart=wholePart-(pow(10,3-(selSize-5)));
					if (wholePart>9999) wholePart=9999;
				} else {
					fracPart=fracPart-(pow(10,3-(selSize-9)));
					if (fracPart>9999) fracPart=9999;
				}
			}
		break;
		case ButtonArray::YPLUS:
			if (selSize<5) selSize--;
			else {
				if (selSize<9) {
					wholePart=wholePart+(pow(10,3-(selSize-5)));
					if (wholePart>9999) wholePart=0;
				} else {
					fracPart=fracPart+(pow(10,3-(selSize-9)));
					if (fracPart>9999) fracPart=0;
				}
			}
		break;
	}
	
	switch(selSize){
		case 0:
			wholePart=200;
			fracPart=0;
		break;
		case 1:
			wholePart=1280;
			fracPart=0;
		break;
		case 2:
			wholePart=160;
			fracPart=0;
		break;
		case 3:
			wholePart=1133;
			fracPart=8580;
		break;
		case 4:
			wholePart=160;
			fracPart=0;
		break;
	}
}

void ZAxisRevs::reset() {
	wholePart=eeprom::getEeprom16(eeprom::ZAXIS_MM_PER_TURN_W,200);
  fracPart=eeprom::getEeprom16(eeprom::ZAXIS_MM_PER_TURN_P,0);
  if (wholePart==200 && fracPart==0) selSize=0;
  else if (wholePart==1280 && fracPart==0) selSize=1;
  else if (wholePart==160 && fracPart==0) selSize=2;
  else if (wholePart==1133 && fracPart==8580) selSize=3;
  else if (wholePart==160 && fracPart==0) selSize=4;
  else selSize=5; 				
}


MBMenu::MBMenu() {
	itemCount = 6;
	reset();
}

void MBMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar item1[]  = "Machine Name    ";
	const static PROGMEM prog_uchar item2[]  = "Invert Axis     ";
	const static PROGMEM prog_uchar item3[]  = "E-Stop Installed";
	const static PROGMEM prog_uchar item4[]  = "End Stop Homes  ";
	const static PROGMEM prog_uchar item5[]  = "End Stop Invert ";
	const static PROGMEM prog_uchar item6[]  = "ZAxis Rev per mm";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(item1);
		break;
	case 1:
		lcd.writeFromPgmspace(item2);
		break;
	case 2:
		lcd.writeFromPgmspace(item3);
		break;
  case 3:
		lcd.writeFromPgmspace(item4);
		break;
	case 4:
		lcd.writeFromPgmspace(item5);
		break;
	case 5:
		lcd.writeFromPgmspace(item6);
		break;
	}
}


void MBMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 0:
			interface::pushScreen(&notyetimplemented);
			break;
		case 1:
			interface::pushScreen(&notyetimplemented);
			break;
		case 2:
			interface::pushScreen(&notyetimplemented);
			break;
	  case 3:
			interface::pushScreen(&notyetimplemented);
			break;
		case 4:
			interface::pushScreen(&notyetimplemented);
			break;
		case 5:
			interface::pushScreen(&zaxisrevs);
			break;
		}
}

void NYI::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar item1[] = "Feature not yet";
	const static PROGMEM prog_uchar item2[] = "implemented";
	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(item1);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(item2);
	} if (waitfor++>4) {
		interface::popScreen();
	}
}

void NYI::notifyButtonPressed(ButtonArray::ButtonName button) {
}

void NYI::reset() {
	waitfor=0;
}

UserViewMenu::UserViewMenu() {
	itemCount = 4;
	reset();
}

void UserViewMenu::resetState() {
        uint8_t jogModeSettings = eeprom::getEeprom8(eeprom::JOG_MODE_SETTINGS, 0);

	if ( jogModeSettings & 0x01 )	itemIndex = 3;
	else				itemIndex = 2;

	firstItemIndex = 2;
}

void UserViewMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar msg[]  = "X/Y Direction:";
	const static PROGMEM prog_uchar model[]= "Model View";
	const static PROGMEM prog_uchar user[] = "User View";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(msg);
		break;
	case 1:
		break;
	case 2:
		lcd.writeFromPgmspace(model);
		break;
	case 3:
		lcd.writeFromPgmspace(user);
		break;
	}
}

void UserViewMenu::handleSelect(uint8_t index) {
	uint8_t jogModeSettings = eeprom::getEeprom8(eeprom::JOG_MODE_SETTINGS, 0);

	switch (index) {
	case 2:
		// Model View
		eeprom_write_byte((uint8_t *)eeprom::JOG_MODE_SETTINGS, (jogModeSettings & (uint8_t)0xFE));
		interface::popScreen();
		break;
	case 3:
		// User View
		eeprom_write_byte((uint8_t *)eeprom::JOG_MODE_SETTINGS, (jogModeSettings | (uint8_t)0x01));
                interface::popScreen();
		break;
	}
}


void JogMode::reset() {
	uint8_t jogModeSettings = eeprom::getEeprom8(eeprom::JOG_MODE_SETTINGS, 0);

	jogDistance = (enum distance_t)((jogModeSettings >> 1 ) & 0x07);
	if ( jogDistance > DISTANCE_CONT ) jogDistance = DISTANCE_SHORT;

	distanceChanged = false;
	lastDirectionButtonPressed = (ButtonArray::ButtonName)0;

	userViewMode = jogModeSettings & 0x01;
	userViewModeChanged = false;
}


void JogMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar jog1[]      = "Jog mode: ";
	const static PROGMEM prog_uchar jog2[] 	    = "   Y+         Z+";
	const static PROGMEM prog_uchar jog3[]      = "X- V  X+  (mode)";
	const static PROGMEM prog_uchar jog4[]      = "   Y-         Z-";
	const static PROGMEM prog_uchar jog2_user[] = "  Y           Z+";
	const static PROGMEM prog_uchar jog3_user[] = "X V X     (mode)";
	const static PROGMEM prog_uchar jog4_user[] = "  Y           Z-";

	const static PROGMEM prog_uchar distanceShort[] = "SHORT";
	const static PROGMEM prog_uchar distanceLong[] = "LONG";
	const static PROGMEM prog_uchar distanceCont[] = "CONT";

	if ( userViewModeChanged ) userViewMode = eeprom::getEeprom8(eeprom::JOG_MODE_SETTINGS, 0) & 0x01;

	if (forceRedraw || distanceChanged || userViewModeChanged) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(jog1);

		switch (jogDistance) {
		case DISTANCE_SHORT:
			lcd.writeFromPgmspace(distanceShort);
			break;
		case DISTANCE_LONG:
			lcd.writeFromPgmspace(distanceLong);
			break;
		case DISTANCE_CONT:
			lcd.writeFromPgmspace(distanceCont);
			break;
		}

		lcd.setCursor(0,1);
		if ( userViewMode )	lcd.writeFromPgmspace(jog2_user);
		else			lcd.writeFromPgmspace(jog2);

		lcd.setCursor(0,2);
		if ( userViewMode )	lcd.writeFromPgmspace(jog3_user);
		else			lcd.writeFromPgmspace(jog3);

		lcd.setCursor(0,3);
		if ( userViewMode )	lcd.writeFromPgmspace(jog4_user);
		else			lcd.writeFromPgmspace(jog4);

		distanceChanged = false;
		userViewModeChanged    = false;
	}

	if ( jogDistance == DISTANCE_CONT ) {
		if ( lastDirectionButtonPressed ) {
			if (interface::isButtonPressed(lastDirectionButtonPressed))
				JogMode::notifyButtonPressed(lastDirectionButtonPressed);
			else	lastDirectionButtonPressed = (ButtonArray::ButtonName)0;
		}
	}
}


void JogMode::jog(ButtonArray::ButtonName direction) {
	Point position = steppers::getPosition();

	int32_t interval = 2000;
	int32_t steps;

	if ( jogDistance == DISTANCE_CONT )	interval = 1000;

	switch(jogDistance) {
	case DISTANCE_SHORT:
		steps = 20;
		break;
	case DISTANCE_LONG:
		steps = 200;
		break;
	case DISTANCE_CONT:
		steps = 50;
		break;
	}

	//Reverse direction of X and Y if we're in User View Mode and
	//not model mode
	uint32_t vMode = 1;
	if ( userViewMode ) vMode = -1;;

	switch(direction) {
        case ButtonArray::XMINUS:
		position[0] -= vMode * steps;
		break;
        case ButtonArray::XPLUS:
		position[0] += vMode * steps;
		break;
        case ButtonArray::YMINUS:
		position[1] -= vMode * steps;
		break;
        case ButtonArray::YPLUS:
		position[1] += vMode * steps;
		break;
        case ButtonArray::ZMINUS:
		position[2] -= steps;
		break;
        case ButtonArray::ZPLUS:
		position[2] += steps;
		break;
	}

	if ( jogDistance == DISTANCE_CONT )	lastDirectionButtonPressed = direction;
	else					lastDirectionButtonPressed = (ButtonArray::ButtonName)0;

	steppers::setTarget(position, interval);
}

void JogMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::ZERO:
		userViewModeChanged = true;
		interface::pushScreen(&userViewMenu);
		break;
        case ButtonArray::OK:
		switch(jogDistance)
		{
			case DISTANCE_SHORT:
				jogDistance = DISTANCE_LONG;
				break;
			case DISTANCE_LONG:
				jogDistance = DISTANCE_CONT;
				break;
			case DISTANCE_CONT:
				jogDistance = DISTANCE_SHORT;
				break;
		}
		distanceChanged = true;
		eeprom_write_byte((uint8_t *)eeprom::JOG_MODE_SETTINGS, userViewMode | (jogDistance << 1));
		break;
        case ButtonArray::YMINUS:
        case ButtonArray::ZMINUS:
        case ButtonArray::YPLUS:
        case ButtonArray::ZPLUS:
        case ButtonArray::XMINUS:
        case ButtonArray::XPLUS:
		jog(button);
		break;
        case ButtonArray::CANCEL:
		steppers::abort();
		steppers::enableAxis(0, false);
		steppers::enableAxis(1, false);
		steppers::enableAxis(2, false);
                interface::popScreen();
		break;
	}
}


void ExtruderMode::reset() {
	extrudeSeconds = (enum extrudeSeconds)eeprom::getEeprom8(eeprom::EXTRUDE_DURATION, 1);
	updatePhase = 0;
	timeChanged = false;
	lastDirection = 1;
	overrideExtrudeSeconds = 0;
}

void ExtruderMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar extrude1[] = "Extrude: ";
	const static PROGMEM prog_uchar extrude2[] = "(set rpm)    Fwd";
	const static PROGMEM prog_uchar extrude3[] = " (stop)    (dur)";
	const static PROGMEM prog_uchar extrude4[] = "---/---C     Rev";
	const static PROGMEM prog_uchar secs[]     = "SECS";
	const static PROGMEM prog_uchar blank[]    = "       ";

	if (overrideExtrudeSeconds)	extrude(overrideExtrudeSeconds, true);

	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(extrude1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(extrude2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(extrude3);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(extrude4);
	}

  if (elapsedTime>0) {
  	elapsedTime--;
  	lcd.setCursor(9,0);
		lcd.writeFromPgmspace(blank);
  	lcd.setCursor(9,0);
		lcd.writeFloat((float)elapsedTime/17, 0);
		lcd.writeFromPgmspace(secs);
		if (elapsedTime==0) forceRedraw=true;
	}

	if ((forceRedraw) || (timeChanged)) {
		lcd.setCursor(9,0);
		lcd.writeFromPgmspace(blank);
		lcd.setCursor(9,0);
		lcd.writeFloat((float)extrudeSeconds, 0);
		lcd.writeFromPgmspace(secs);
		timeChanged = false;
	}

	OutPacket responsePacket;
	Point position;

	// Redraw tool info
	switch (updatePhase) {
	case 0:
		lcd.setCursor(0,3);
		if (extruderControl(SLAVE_CMD_GET_TEMP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case 1:
		lcd.setCursor(4,3);
		if (extruderControl(SLAVE_CMD_GET_SP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;
	}

	updatePhase++;
	if (updatePhase > 1) {
		updatePhase = 0;
	}
}

void ExtruderMode::extrude(seconds_t seconds, bool overrideTempCheck) {
	//Check we're hot enough
	if ( ! overrideTempCheck )
	{
		OutPacket responsePacket;
		if (extruderControl(SLAVE_CMD_IS_TOOL_READY, EXTDR_CMD_GET, responsePacket, 0)) {
			uint8_t data = responsePacket.read8(1);
		
			if ( ! data )
			{
				overrideExtrudeSeconds = seconds;
				interface::pushScreen(&extruderTooColdMenu);
				return;
			}
		}
	}

	Point position = steppers::getPosition();

	float rpm = (float)eeprom::getEeprom8(eeprom::EXTRUDE_RPM, 19) / 10.0;

	//60 * 1000000 = # uS in a minute
	//200 * 8 = 200 steps per revolution * 1/8 stepping
	int32_t interval = (int32_t)(60L * 1000000L) / (int32_t)((float)(200 * 8) * rpm);
	int16_t stepsPerSecond = (int16_t)((200.0 * 8.0 * rpm) / 60.0);

	if ( seconds == 0 )	steppers::abort();
	else {
		position[3] += seconds * stepsPerSecond;
		steppers::setTarget(position, interval);
	}

	if (overrideTempCheck)	overrideExtrudeSeconds = 0;
}

void ExtruderMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	int16_t zReverse = -1;

	switch (button) {
        	case ButtonArray::OK:
			switch(extrudeSeconds) {
        case EXTRUDE_SECS_1S:
					extrudeSeconds = EXTRUDE_SECS_2S;
					break;
        case EXTRUDE_SECS_2S:
					extrudeSeconds = EXTRUDE_SECS_5S;
					break;
        case EXTRUDE_SECS_5S:
					extrudeSeconds = EXTRUDE_SECS_10S;
					break;
				case EXTRUDE_SECS_10S:
					extrudeSeconds = EXTRUDE_SECS_30S;
					break;
				case EXTRUDE_SECS_30S:
					extrudeSeconds = EXTRUDE_SECS_60S;
					break;
				case EXTRUDE_SECS_60S:
					extrudeSeconds = EXTRUDE_SECS_90S;
					break;
				case EXTRUDE_SECS_90S:
					extrudeSeconds = EXTRUDE_SECS_120S;
					break;
        case EXTRUDE_SECS_120S:
					extrudeSeconds = EXTRUDE_SECS_240S;
					break;
        case EXTRUDE_SECS_240S:
					extrudeSeconds = EXTRUDE_SECS_1S;
					break;
				default:
					extrudeSeconds = EXTRUDE_SECS_1S;
					break;
			}

			eeprom_write_byte((uint8_t *)eeprom::EXTRUDE_DURATION, (uint8_t)extrudeSeconds);

			//If we're already extruding, change the time running
			if (steppers::isRunning())
				extrude((seconds_t)(zReverse * lastDirection * extrudeSeconds), false);

			timeChanged = true;
			break;
        	case ButtonArray::YPLUS:
			// Show Extruder RPM Setting Screen
              interface::pushScreen(&extruderSetRpmScreen);
			break;
        	case ButtonArray::ZERO:
        	case ButtonArray::YMINUS:
        	case ButtonArray::XMINUS:
        	case ButtonArray::XPLUS:
			extrude((seconds_t)EXTRUDE_SECS_CANCEL, true);
        		break;
        	case ButtonArray::ZMINUS:
        	case ButtonArray::ZPLUS:
			if ( button == ButtonArray::ZPLUS )	lastDirection = 1;
			else					lastDirection = -1;
			elapsedTime = extrudeSeconds*17;
			extrude((seconds_t)(zReverse * lastDirection * extrudeSeconds), false);
			break;
       	 	case ButtonArray::CANCEL:
			steppers::abort();
			steppers::enableAxis(3, false);
			elapsedTime=0;
               		interface::popScreen();
			break;
	}
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

void ExtruderSetRpmScreen::reset() {
	rpm = eeprom::getEeprom8(eeprom::EXTRUDE_RPM, 19);
}

void ExtruderSetRpmScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Extruder RPM:";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]    = " ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(0,1);
	lcd.writeFloat((float)rpm / 10.0, 1);
	lcd.writeFromPgmspace(blank);
}

void ExtruderSetRpmScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::CANCEL:
			interface::popScreen();
			break;
		case ButtonArray::ZERO:
		case ButtonArray::OK:
			eeprom_write_byte((uint8_t *)eeprom::EXTRUDE_RPM, rpm);
			interface::popScreen();
			break;
		case ButtonArray::ZPLUS:
			// increment more
			if (rpm <= 250) rpm += 5;
			break;
		case ButtonArray::ZMINUS:
			// decrement more
			if (rpm >= 8) rpm -= 5;
			break;
		case ButtonArray::YPLUS:
			// increment less
			if (rpm <= 254) rpm += 1;
			break;
		case ButtonArray::YMINUS:
			// decrement less
			if (rpm >= 4) rpm -= 1;
			break;
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}
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
	uint8_t scriptId = eeprom_read_byte((uint8_t *)eeprom::MOOD_LIGHT_SCRIPT);

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
			scriptId = eeprom_read_byte((uint8_t *)eeprom::MOOD_LIGHT_SCRIPT);
			scriptId = interface::moodLightController().nextScriptId(scriptId);
			eeprom_write_byte((uint8_t *)eeprom::MOOD_LIGHT_SCRIPT, scriptId);
			interface::moodLightController().playScript(scriptId);
			break;

        	case ButtonArray::ZERO:
			scriptId = eeprom_read_byte((uint8_t *)eeprom::MOOD_LIGHT_SCRIPT);
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

	red   = eeprom::getEeprom8(eeprom::MOOD_LIGHT_CUSTOM_RED,   255);;
	green = eeprom::getEeprom8(eeprom::MOOD_LIGHT_CUSTOM_GREEN, 255);;
	blue  = eeprom::getEeprom8(eeprom::MOOD_LIGHT_CUSTOM_BLUE,  255);;
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
			eeprom_write_byte((uint8_t*)eeprom::MOOD_LIGHT_CUSTOM_RED,  red);
			eeprom_write_byte((uint8_t*)eeprom::MOOD_LIGHT_CUSTOM_GREEN,green);
			eeprom_write_byte((uint8_t*)eeprom::MOOD_LIGHT_CUSTOM_BLUE, blue);

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


void MonitorMode::reset() {
	updatePhase = 0;
	buildTimePhase = 0;
	buildComplete = false;
	extruderStartSeconds = 0.0;
	lastElapsedSeconds = 0.0;
	pausePushLockout = false;
	pauseMode.autoPause = false;
}

void MonitorMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar extruder_temp[]      =   "Tool: ---/---C";
	const static PROGMEM prog_uchar platform_temp[]      =   "Bed:  ---/---C";
	const static PROGMEM prog_uchar elapsed_time[]       =   "Elapsed:   0h00m";
	const static PROGMEM prog_uchar completed_percent[]  =   "Completed:   0% ";
	const static PROGMEM prog_uchar time_left[]          =   "TimeLeft:  0h00m";
	const static PROGMEM prog_uchar time_left_calc[]     =   " calc..";
	const static PROGMEM prog_uchar time_left_1min[]     =   "  <1min";
	const static PROGMEM prog_uchar time_left_none[]     =   "   none";
	const static PROGMEM prog_uchar zpos[] 		     =   "ZPos:           ";
	const static PROGMEM prog_uchar zpos_mm[] 	     =   "mm";
	char buf[17];

	if ( command::isPaused() ) {
		if ( ! pausePushLockout ) {
			pausePushLockout = true;
			pauseMode.autoPause = true;
			interface::pushScreen(&pauseMode);
			return;
		}
	} else pausePushLockout = false;



	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		switch(host::getHostState()) {
		case host::HOST_STATE_READY:
			lcd.writeString(host::getMachineName());
			break;
		case host::HOST_STATE_BUILDING:
		case host::HOST_STATE_BUILDING_FROM_SD:
			lcd.writeString(host::getBuildName());
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(completed_percent);
			break;
		case host::HOST_STATE_ERROR:
			lcd.writeString("error!");
			break;
		}
    
		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(extruder_temp);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(platform_temp);
		
		lcd.setCursor(15,3);
		if ( command::getPauseAtZPos() == 0.0 )	lcd.write(' ');
		else					lcd.write('*');


	} else {
	}


	OutPacket responsePacket;

	// Redraw tool info
	switch (updatePhase) {
	case 0:
		lcd.setCursor(6,2);
		if (extruderControl(SLAVE_CMD_GET_TEMP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case 1:
		lcd.setCursor(10,2);
		if (extruderControl(SLAVE_CMD_GET_SP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case 2:
		lcd.setCursor(6,3);
		if (extruderControl(SLAVE_CMD_GET_PLATFORM_TEMP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case 3:
		lcd.setCursor(10,3);
		if (extruderControl(SLAVE_CMD_GET_PLATFORM_SP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		
		lcd.setCursor(15,3);
		if ( command::getPauseAtZPos() == 0.0 )	lcd.write(' ');
		else					lcd.write('*');
		break;
	case 4:
		enum host::HostState hostState = host::getHostState();
		
		if ( (hostState != host::HOST_STATE_BUILDING ) && ( hostState != host::HOST_STATE_BUILDING_FROM_SD )) break;

		float secs;

		//Holding the zero button stops rotation
        	if ( ! interface::isButtonPressed(ButtonArray::OK) ) buildTimePhase ++;

		if ( buildTimePhase >= 4 )	buildTimePhase = 0;

		switch (buildTimePhase) {
			case 0:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(completed_percent);
				lcd.setCursor(11,1);
				buf[0] = '\0';
				appendUint8(buf, sizeof(buf), (uint8_t)sdcard::getPercentPlayed());
				strcat(buf, "% ");
				lcd.writeString(buf);
				break;
			case 1:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(elapsed_time);
				lcd.setCursor(9,1);
				buf[0] = '\0';

				if ( host::isBuildComplete() ) secs = lastElapsedSeconds; //We stop counting elapsed seconds when we are done
				else {
					lastElapsedSeconds = Motherboard::getBoard().getCurrentSeconds();
					secs = lastElapsedSeconds;
				}
				appendTime(buf, sizeof(buf), (uint32_t)secs);
				lcd.writeString(buf);
				break;
			case 2:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(time_left);
				lcd.setCursor(9,1);

				if (( sdcard::getPercentPlayed() >= 1.0 ) && ( extruderStartSeconds > 0.0)) {
					buf[0] = '\0';
					float currentSeconds = Motherboard::getBoard().getCurrentSeconds() - extruderStartSeconds;
					secs = ((currentSeconds / sdcard::getPercentPlayed()) * 100.0 ) - currentSeconds;

					if ((secs > 0.0 ) && (secs < 60.0) && ( ! buildComplete ) )
						lcd.writeFromPgmspace(time_left_1min);	
					else if (( secs <= 0.0) || ( host::isBuildComplete() ) || ( buildComplete ) ) {
						buildComplete = true;
						lcd.writeFromPgmspace(time_left_none);
					} else {
						appendTime(buf, sizeof(buf), (uint32_t)secs);
						lcd.writeString(buf);
					}
				}
				else	lcd.writeFromPgmspace(time_left_calc);

				//Set extruderStartSeconds to when the extruder starts extruding, so we can 
				//get an accurate TimeLeft:
				if ( extruderStartSeconds == 0.0 ) {
					if (extruderControl(SLAVE_CMD_GET_MOTOR_1_PWM, EXTDR_CMD_GET, responsePacket, 0)) {
						uint8_t pwm = responsePacket.read8(1);
						if ( pwm ) extruderStartSeconds = Motherboard::getBoard().getCurrentSeconds();
					}
				}
				break;
			case 3:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(zpos);
				lcd.setCursor(6,1);

				Point position = steppers::getPosition();
			
				lcd.writeFloat((float)position[2] / getRevsPerMM(), 3);

				lcd.writeFromPgmspace(zpos_mm);
				break;
		}
	
		break;
	}

	updatePhase++;
	if (updatePhase > 4) {
		updatePhase = 0;
	}
}

void MonitorMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
		switch(host::getHostState()) {
		case host::HOST_STATE_BUILDING:
		case host::HOST_STATE_BUILDING_FROM_SD:
                        interface::pushScreen(&cancelBuildMenu);
			break;
		default:
                        interface::popScreen();
			break;
		}
	}
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

void Menu::update(LiquidCrystal& lcd, bool forceRedraw) {
	  
	// Do we need to redraw the whole menu?
	if ((itemIndex/LCD_SCREEN_HEIGHT) != (lastDrawIndex/LCD_SCREEN_HEIGHT)
			|| forceRedraw ) {
		// Redraw the whole menu
		lcd.clear();

		for (uint8_t i = 0; i < LCD_SCREEN_HEIGHT; i++) {
			// Instead of using lcd.clear(), clear one line at a time so there
			// is less screen flickr.

			if (i+(itemIndex/LCD_SCREEN_HEIGHT)*LCD_SCREEN_HEIGHT +1 > itemCount) {
				break;
			}

			lcd.setCursor(1,i);
			// Draw one page of items at a time
			drawItem(i+(itemIndex/LCD_SCREEN_HEIGHT)*LCD_SCREEN_HEIGHT, lcd);
			drawItemSub(i+(itemIndex/LCD_SCREEN_HEIGHT)*LCD_SCREEN_HEIGHT,subItemIndex, lcd);
		}
	} else if (lastSubIndex!=subItemIndex) {
		drawItemSub(itemIndex,subItemIndex, lcd);
	} else {
		// Only need to clear the previous cursor
		lcd.setCursor(0,(lastDrawIndex%LCD_SCREEN_HEIGHT));
		lcd.write(' ');
	}

	lcd.setCursor(0,(itemIndex%LCD_SCREEN_HEIGHT));
	lcd.write('>');
	lastDrawIndex = itemIndex;
	lastSubIndex = subItemIndex;
}

void Menu::reset() {
	firstItemIndex = 0;
	itemIndex = 0;
	lastDrawIndex = 255;
	subItemIndex=0;
	firstSubItemIndex = 1;
	lastSubItemIndex = 255;
	lastSubIndex = 255;
	resetState();
}

void Menu::resetState() {
}

void Menu::handleSelect(uint8_t index) {
}

void Menu::handleSelectSub(uint8_t index,uint8_t subIndex) {
}

void Menu::drawItemSub(uint8_t index, uint8_t subIndex, LiquidCrystal& lcd) {
}

void Menu::drawItem(uint8_t index, LiquidCrystal& lcd) {
}

void Menu::handleCancel() {
	// Remove ourselves from the menu list
        interface::popScreen();
}

void Menu::notifyButtonPressed(ButtonArray::ButtonName button) {
	uint8_t steps = MAX_ITEMS_PER_SCREEN;
	switch (button) {
    case ButtonArray::ZERO:
    case ButtonArray::OK:
			handleSelect(itemIndex);
			handleSelectSub(itemIndex,subItemIndex);
			break;
    case ButtonArray::CANCEL:
			handleCancel();
			break;
    case ButtonArray::YMINUS:
			steps = 1;
    case ButtonArray::ZMINUS:
			// increment index
			if (itemIndex < itemCount - steps) itemIndex+=steps;
			else if (itemIndex==itemCount-1) itemIndex=firstItemIndex;
			else	itemIndex=itemCount-1;
			break;
    case ButtonArray::YPLUS:
			steps = 1;
    case ButtonArray::ZPLUS:
			// decrement index
			if (itemIndex-steps > firstItemIndex)	itemIndex-=steps;
			else if (itemIndex==firstItemIndex) itemIndex=itemCount - 1;
			else itemIndex=firstItemIndex;
			break;
    case ButtonArray::XMINUS:
    	subItemIndex--;
    	if (subItemIndex<firstSubItemIndex) subItemIndex=lastSubItemIndex;
    	break;
    case ButtonArray::XPLUS:
    	subItemIndex++;
    	if (subItemIndex>lastSubItemIndex) subItemIndex=firstSubItemIndex;
    	break;
	}
}


CancelBuildMenu::CancelBuildMenu() {
	pauseMode.autoPause = false;
	itemCount = 3;
	reset();
	pauseDisabled = false;
	if (( steppers::isHoming() ) || (sdcard::getPercentPlayed() >= 100.0))	pauseDisabled = true;

}

void CancelBuildMenu::resetState() {
	pauseMode.autoPause = false;
	pauseDisabled = false;	
	if (( steppers::isHoming() ) || (sdcard::getPercentPlayed() >= 100.0))	pauseDisabled = true;
		
	itemCount = 3;
	
	if ( pauseDisabled )	{
		itemIndex = 2;
	} else {
		itemIndex = 1;
		firstSubItemIndex = 1;
		lastSubItemIndex = 4;
	}

	firstItemIndex = itemIndex;
	subItemIndex = pauseType;
}

void CancelBuildMenu::drawItemSub(uint8_t index, uint8_t subIndex, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar title[]  = "Use <> to choose";
	const static PROGMEM prog_uchar pauseNZ[]= "Pause on Next Z";
	const static PROGMEM prog_uchar pause[]  = "Pause          ";
	const static PROGMEM prog_uchar abort[]  = "Abort Print    ";
	const static PROGMEM prog_uchar pauseZP[]= "Pause at ZPos  ";
	const static PROGMEM prog_uchar pauseFM[]= "Pause free move";
	const static PROGMEM prog_uchar nopaus[] = "*Pause Disabled*";

	if (( steppers::isHoming() ) || (sdcard::getPercentPlayed() >= 100.0))	pauseDisabled = true;

	switch (index) {
		case 0:
			lcd.setCursor(0,0);
			lcd.writeFromPgmspace(title);
			break;
		case 1:
			if ( ! pauseDisabled ) {
				switch (subIndex) {
					case 1:
						lcd.writeFromPgmspace(pauseNZ);
						break;
					case 2:
						lcd.writeFromPgmspace(pauseFM);
						break;
					case 3:
						lcd.writeFromPgmspace(pause);
						break;
					case 4:
						lcd.writeFromPgmspace(pauseZP);
						break;
				}
			} else {
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(nopaus);
			}
			break;
		case 2:
			lcd.writeFromPgmspace(abort);
			break;
	}
	
}

void CancelBuildMenu::handleSelectSub(uint8_t index, uint8_t subIndex) {
	int32_t interval = 2000;

	switch (index) {
	case 1:
		if ( ! pauseDisabled ) {
			switch (subIndex) {
				case 1:
					pauseMode.freeMove = false;
					command::pauseNextZ(true);
					pauseMode.autoPause = false;
					interface::pushScreen(&pauseMode);
					pauseType=1;
					break;
				case 2:
					pauseMode.freeMove = true;
					command::pause(true);
					pauseMode.autoPause = false;
					interface::pushScreen(&pauseMode);
					pauseType=2;
					break;
				case 3:
					pauseMode.freeMove = false;
					command::pause(true);
					pauseMode.autoPause = false;
					interface::pushScreen(&pauseMode);
					pauseType=3;
				case 4:
					pauseMode.freeMove = false;
					pauseType=4;
					interface::pushScreen(&pauseAtZPosScreen);
				break;
			}
		}
		break;
	case 2:
		// Cancel build, returning to whatever menu came before monitor mode.
		// TODO: Cancel build.
		interface::popScreen();
		host::stopBuild();
		break;
	}
}


MainMenu::MainMenu() {
	itemCount = 10;
	reset();
}

void MainMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar monitor[] =  "Monitor";
	const static PROGMEM prog_uchar build[] =    "Build from SD";
	const static PROGMEM prog_uchar jog[] =      "Jog";
	const static PROGMEM prog_uchar setup[] =    "Setup";
	const static PROGMEM prog_uchar preheat[] =  "Preheat";
	const static PROGMEM prog_uchar extruder[] = "Extrude";
	const static PROGMEM prog_uchar steppersS[]= "Steppers";
	const static PROGMEM prog_uchar advanceABP[]="Advance ABP";
	const static PROGMEM prog_uchar calibrate[]= "Calibrate";
	const static PROGMEM prog_uchar homeOffsets[]="Home Offsets";




	switch (index) {
	case 0:
		lcd.writeFromPgmspace(monitor);
		break;
	case 1:
		lcd.writeFromPgmspace(build);
		break;
	case 2:
		lcd.writeFromPgmspace(jog);
		break;
	case 3:
		lcd.writeFromPgmspace(preheat);
		break;
	case 4:
		lcd.writeFromPgmspace(extruder);
		break;
	case 5:
		lcd.writeFromPgmspace(steppersS);
		break;
	case 6:
		lcd.writeFromPgmspace(setup);
		break;
	case 7:
		lcd.writeFromPgmspace(advanceABP);
		break;
	case 8:
		lcd.writeFromPgmspace(calibrate);
		break;
	case 9:
		lcd.writeFromPgmspace(homeOffsets);
		break;
	}
}

void MainMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 0:
			// Show monitor build screen
      interface::pushScreen(&monitorMode);
			break;
		case 1:
			// Show build from SD screen
      interface::pushScreen(&sdMenu);
			break;
		case 2:
			// Show build from SD screen
      interface::pushScreen(&jogger);
			break;
		case 3:
			// Show preheat menu
			interface::pushScreen(&preheatMenu);
			preheatMenu.fetchTargetTemps();
			break;
		case 4:
			// Show extruder menu
			interface::pushScreen(&extruderMenu);
			break;
		case 5:
			// Show steppers menu
			interface::pushScreen(&steppersMenu);
			break;
		case 6:
			// Show build from Setup screen
			interface::pushScreen(&setup);
			break;
		case 7:
			// Show advance ABP
			interface::pushScreen(&advanceABPMode);
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

SDMenu::SDMenu() {
	reset();
	updatePhase = 0;
	drawItemLockout = false;
}

void SDMenu::resetState() {
	itemCount = countFiles();
	updatePhase = 0;
	lastItemIndex = 0;
	drawItemLockout = false;
}

// Count the number of files on the SD card
uint8_t SDMenu::countFiles() {
	uint8_t count = 0;

	sdcard::SdErrorCode e;

	// First, reset the directory index
	e = sdcard::directoryReset();
	if (e != sdcard::SD_SUCCESS) {
		// TODO: Report error
		return 6;
	}

	const int MAX_FILE_LEN = 2;
	char fnbuf[MAX_FILE_LEN];

	// Count the files
	do {
		e = sdcard::directoryNextEntry(fnbuf,MAX_FILE_LEN);
		if (fnbuf[0] == '\0') {
			break;
		}

		// If it's a dot file, don't count it.
		if (fnbuf[0] == '.') {
		}
		else {
			count++;
		}
	} while (e == sdcard::SD_SUCCESS);

	// TODO: Check for error again?

	return count;
}

bool SDMenu::getFilename(uint8_t index, char buffer[], uint8_t buffer_size) {
	sdcard::SdErrorCode e;

	// First, reset the directory list
	e = sdcard::directoryReset();
	if (e != sdcard::SD_SUCCESS) {
                return false;
	}


	for(uint8_t i = 0; i < index+1; i++) {
		// Ignore dot-files
		do {
			e = sdcard::directoryNextEntry(buffer,buffer_size);
			if (buffer[0] == '\0') {
                                return false;
			}
		} while (e == sdcard::SD_SUCCESS && buffer[0] == '.');

		if (e != sdcard::SD_SUCCESS) {
                        return false;
		}
	}

        return true;
}

void SDMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	if (index > itemCount - 1) {
		// TODO: report error
		return;
	}

	const uint8_t MAX_FILE_LEN = host::MAX_FILE_LEN;
	char fnbuf[MAX_FILE_LEN];

        if ( !getFilename(index, fnbuf, MAX_FILE_LEN) ) {
                // TODO: report error
		return;
	}

	//Figure out length of filename
	uint8_t filenameLength;
	for (filenameLength = 0; (filenameLength < MAX_FILE_LEN) && (fnbuf[filenameLength] != 0); filenameLength++) ;

	uint8_t idx;
	uint8_t longFilenameOffset = 0;
	uint8_t displayWidth = lcd.getDisplayWidth() - 1;

	//Support scrolling filenames that are longer than the lcd screen
	if (filenameLength >= displayWidth) {
		if ((!dir) && (updatePhase % (filenameLength - displayWidth + 1)==0) && (updatePhase>0)) {
			updatePhase--;
			dir=true;
		}
		longFilenameOffset = updatePhase % (filenameLength - displayWidth + 1);
	}

	for (idx = 0; (idx < displayWidth) && (fnbuf[longFilenameOffset + idx] != 0) &&
		      ((longFilenameOffset + idx) < MAX_FILE_LEN); idx++)
		lcd.write(fnbuf[longFilenameOffset + idx]);

	//Clear out the rest of the line
	while ( idx < displayWidth ) {
		lcd.write(' ');
		idx ++;
	}
}

void SDMenu::update(LiquidCrystal& lcd, bool forceRedraw) {
	
	if (( ! forceRedraw ) && ( ! drawItemLockout )) {
		//Redraw the last item if we have changed
		if (((itemIndex/LCD_SCREEN_HEIGHT) == (lastDrawIndex/LCD_SCREEN_HEIGHT)) &&
		     ( itemIndex != lastItemIndex ))  {
			lcd.setCursor(1,lastItemIndex % LCD_SCREEN_HEIGHT);
			drawItem(lastItemIndex, lcd);
		}
		lastItemIndex = itemIndex;

		lcd.setCursor(1,itemIndex % LCD_SCREEN_HEIGHT);
		drawItem(itemIndex, lcd);
	}

	Menu::update(lcd, forceRedraw);
 
  if (!dir)
		updatePhase ++;
	else {
		updatePhase--;
		if (updatePhase==0) dir=false;
	}
	
}

void SDMenu::notifyButtonPressed(ButtonArray::ButtonName button) {
	updatePhase = 0;
	Menu::notifyButtonPressed(button);
}

void SDMenu::handleSelect(uint8_t index) {
	if (host::getHostState() != host::HOST_STATE_READY) {
		// TODO: report error
		return;
	}

	drawItemLockout = true;

	char* buildName = host::getBuildName();

        if ( !getFilename(index, buildName, host::MAX_FILE_LEN) ) {
		// TODO: report error
		return;
	}

        sdcard::SdErrorCode e;
	e = host::startBuildFromSD();
	if (e != sdcard::SD_SUCCESS) {
		// TODO: report error
		return;
	}
}


void Tool0TempSetScreen::reset() {
	value = eeprom::getEeprom8(eeprom::TOOL0_TEMP, 220);
}

void Tool0TempSetScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Tool0 Target Temp:";
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

void Tool0TempSetScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
        case ButtonArray::OK:
		eeprom_write_byte((uint8_t*)eeprom::TOOL0_TEMP,value);
		interface::popScreen();
		break;
        case ButtonArray::ZPLUS:
		// increment more
		if (value <= 250) {
			value += 5;
		}
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		if (value >= 5) {
			value -= 5;
		}
		break;
        case ButtonArray::YPLUS:
		// increment less
		if (value <= 254) {
			value += 1;
		}
		break;
        case ButtonArray::YMINUS:
		// decrement less
		if (value >= 1) {
			value -= 1;
		}
		break;

        case ButtonArray::XMINUS:
        case ButtonArray::XPLUS:
		break;
	}
}


void PlatformTempSetScreen::reset() {
	value = eeprom::getEeprom8(eeprom::PLATFORM_TEMP, 110);;
}

void PlatformTempSetScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Bed Target Temp:";
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

void PlatformTempSetScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
		interface::popScreen();
		break;
        case ButtonArray::ZERO:
        case ButtonArray::OK:
		eeprom_write_byte((uint8_t*)eeprom::PLATFORM_TEMP,value);
		interface::popScreen();
		break;
        case ButtonArray::ZPLUS:
		// increment more
		if (value <= 250) {
			value += 5;
		}
		break;
        case ButtonArray::ZMINUS:
		// decrement more
		if (value >= 5) {
			value -= 5;
		}
		break;
        case ButtonArray::YPLUS:
		// increment less
		if (value <= 254) {
			value += 1;
		}
		break;
        case ButtonArray::YMINUS:
		// decrement less
		if (value >= 1) {
			value -= 1;
		}
		break;

        case ButtonArray::XMINUS:
        case ButtonArray::XPLUS:
		break;
	}
}


PreheatMenu::PreheatMenu() {
	itemCount = 4;
	reset();
}

void PreheatMenu::fetchTargetTemps() {
	OutPacket responsePacket;
	if (extruderControl(SLAVE_CMD_GET_SP, EXTDR_CMD_GET, responsePacket, 0)) {
		tool0Temp = responsePacket.read16(1);
	}
	if (extruderControl(SLAVE_CMD_GET_PLATFORM_SP, EXTDR_CMD_GET, responsePacket, 0)) {
		platformTemp = responsePacket.read16(1);
	}
}

void PreheatMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar heat[]     = "Heat ";
	const static PROGMEM prog_uchar cool[]     = "Cool ";
	const static PROGMEM prog_uchar tool0[]    = "Tool0";
	const static PROGMEM prog_uchar platform[] = "Bed";
	const static PROGMEM prog_uchar tool0set[] = "Set Tool0 Temp";
	const static PROGMEM prog_uchar platset[]  = "Set Bed Temp";

	switch (index) {
	case 0:
		fetchTargetTemps();
		if (tool0Temp > 0) {
			lcd.writeFromPgmspace(cool);
		} else {
			lcd.writeFromPgmspace(heat);
		}
		lcd.writeFromPgmspace(tool0);
		break;
	case 1:
		if (platformTemp > 0) {
			lcd.writeFromPgmspace(cool);
		} else {
			lcd.writeFromPgmspace(heat);
		}
		lcd.writeFromPgmspace(platform);
		break;
	case 2:
		lcd.writeFromPgmspace(tool0set);
		break;
	case 3:
		lcd.writeFromPgmspace(platset);
		break;
	}
}

void PreheatMenu::handleSelect(uint8_t index) {
	OutPacket responsePacket;
	switch (index) {
		case 0:
			// Toggle Extruder heater on/off
			if (tool0Temp > 0) {
				extruderControl(SLAVE_CMD_SET_TEMP, EXTDR_CMD_SET, responsePacket, 0);
			} else {
				uint8_t value = eeprom::getEeprom8(eeprom::TOOL0_TEMP, 220);
				extruderControl(SLAVE_CMD_SET_TEMP, EXTDR_CMD_SET, responsePacket, (uint16_t)value);
			}
			fetchTargetTemps();
			lastDrawIndex = 255; // forces redraw.
			break;
		case 1:
			// Toggle Platform heater on/off
			if (platformTemp > 0) {
				extruderControl(SLAVE_CMD_SET_PLATFORM_TEMP, EXTDR_CMD_SET, responsePacket, 0);
			} else {
				uint8_t value = eeprom::getEeprom8(eeprom::PLATFORM_TEMP, 110);
				extruderControl(SLAVE_CMD_SET_PLATFORM_TEMP, EXTDR_CMD_SET, responsePacket, value);
			}
			fetchTargetTemps();
			lastDrawIndex = 255; // forces redraw.
			break;
		case 2:
			// Show Extruder Temperature Setting Screen
                        interface::pushScreen(&tool0TempSetScreen);
			break;
		case 3:
			// Show Platform Temperature Setting Screen
                        interface::pushScreen(&platTempSetScreen);
			break;
		}
}

void HomeAxisMode::reset() {
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

SteppersMenu::SteppersMenu() {
	itemCount = 4;
	reset();
}

void SteppersMenu::resetState() {
	if (( steppers::isEnabledAxis(0) ) ||
	    ( steppers::isEnabledAxis(1) ) ||
	    ( steppers::isEnabledAxis(2) ) ||
	    ( steppers::isEnabledAxis(3) ))	itemIndex = 3;
	else					itemIndex = 2;
	firstItemIndex = 2;
}

void SteppersMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar title[]  = "Stepper Motors:";
	const static PROGMEM prog_uchar disable[]= "Disable";
	const static PROGMEM prog_uchar enable[] = "Enable";

	switch (index) {
	case 0:
		lcd.writeFromPgmspace(title);
		break;
	case 1:
		break;
	case 2:
		lcd.writeFromPgmspace(disable);
		break;
	case 3:
		lcd.writeFromPgmspace(enable);
		break;
	}
}

void SteppersMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 2:
			//Disable Steppers
			steppers::enableAxis(0, false);
			steppers::enableAxis(1, false);
			steppers::enableAxis(2, false);
			steppers::enableAxis(3, false);
			interface::popScreen();
			break;
		case 3:
			//Enable Steppers
			steppers::enableAxis(0, true);
			steppers::enableAxis(1, true);
			steppers::enableAxis(2, true);
			steppers::enableAxis(3, true);
                	interface::popScreen();
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
	
	uint8_t minmax=eeprom::getEeprom8(eeprom::AXIS_HOME_MINMAX, 0b00000001);
	
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

void PauseMode::reset() {
	pauseState = 0;
	lastDirectionButtonPressed = (ButtonArray::ButtonName)0;
}

void PauseMode::jog(ButtonArray::ButtonName direction) {
	uint8_t steps = 50;
	bool extrude = false;
	int32_t interval = 1000;
	Point position = steppers::getPosition();

	switch(direction) {
       		case ButtonArray::XMINUS:
			position[0] -= steps;
			break;
        	case ButtonArray::XPLUS:
			position[0] += steps;
			break;
        	case ButtonArray::YMINUS:
			position[1] -= steps;
			break;
       	 	case ButtonArray::YPLUS:
			position[1] += steps;
			break;
        	case ButtonArray::ZMINUS:
			position[2] -= steps;
			break;
       		case ButtonArray::ZPLUS:
			position[2] += steps;
			break;
		case ButtonArray::OK:
		case ButtonArray::ZERO:
			float rpm = (float)eeprom::getEeprom8(eeprom::EXTRUDE_RPM, 19) / 10.0;
      float mmperstep = getRevsPerMM();
			//60 * 1000000 = # uS in a minute
			//200 * 8 = 200 steps per revolution * 1/8 stepping
			interval = (int32_t)(60L * 1000000L) / (int32_t)((float)(mmperstep * 8) * rpm);
			int16_t stepsPerSecond = (int16_t)(((float)mmperstep * 8.0 * rpm) / 60.0);

			//Handle reverse
			if ( direction == ButtonArray::OK )	stepsPerSecond *= -1;

			//Extrude for 0.5 seconds
			position[3] += 0.5 * stepsPerSecond;
			break;
	}

	lastDirectionButtonPressed = direction;

	steppers::setTarget(position, interval);
}


void PauseMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar waitForCurrentCommand[] = "Entering pause..";
	const static PROGMEM prog_uchar movingZ[] 		 = "Moving Z up 2mm ";
	const static PROGMEM prog_uchar leavingPaused[]= "Leaving pause.. ";
	const static PROGMEM prog_uchar paused1[] 		 = "Paused:";
	const static PROGMEM prog_uchar paused2[] 		 = "    Y+       Z+ ";
	const static PROGMEM prog_uchar paused3[] 		 = " X- Rev X+ (Fwd)";
	const static PROGMEM prog_uchar paused4[] 		 = "    Y-       Z- ";
	const static PROGMEM prog_uchar paused5[]			 = "Free Move";
	const static PROGMEM prog_uchar paused6[]			 = "         ";

	int32_t interval = 2000;
	Point newPosition = pausedPosition;
	
  float mmperstep = getRevsPerMM();
  	
	if (forceRedraw)	lcd.clear();

	lcd.setCursor(0,0);

	switch (pauseState) {
		case 0:	//Entered pause, waiting for steppers to finish last command
			lcd.writeFromPgmspace(waitForCurrentCommand);

			if ( ! steppers::isRunning() && command::isPaused()) pauseState ++;
			break;

		case 1: //Last command finished, record current position and move
			//Z away from build

			pausedPosition = steppers::getPosition();
			newPosition = pausedPosition;
			
			if (!freeMove) {
				lcd.writeFromPgmspace(movingZ);
				newPosition[2] += 2 * mmperstep;
				steppers::setTarget(newPosition, interval);
			}
			
			pauseState ++;
			break;

		case 2: //Wait for the Z move up to complete
			lcd.writeFromPgmspace(movingZ);
			if ( ! steppers::isRunning()) {
				pauseState ++;

				//We write this here to avoid tieing up the processor
				//in the next state
				lcd.clear();
				lcd.writeFromPgmspace(paused1);
				if (!freeMove) lcd.writeFromPgmspace(paused6);
				else lcd.writeFromPgmspace(paused5);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(paused2);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(paused3);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(paused4);
			}
			break;
	
		case 3: //We're now paused
			break;

		case 4: //Leaving paused, wait for any steppers to finish
			if ( autoPause ) command::pauseAtZPos(0.0);
			lcd.clear();
			lcd.writeFromPgmspace(leavingPaused);
			if ( ! steppers::isRunning()) pauseState ++;
			break;

		case 5:	//Return to original position
			lcd.writeFromPgmspace(leavingPaused);

			//The extruders may have moved, so it doesn't make sense
			//to go back to the old position, or we'll eject the filament
			if (!freeMove) {
				newPosition = steppers::getPosition();
				pausedPosition[3] = newPosition[3];
				pausedPosition[4] = newPosition[4];
				steppers::setTarget(pausedPosition, interval);
			} else {
				newPosition = steppers::getPosition();
				pausedPosition[3] = newPosition[3];
				pausedPosition[4] = newPosition[4];
				steppers::definePosition(pausedPosition);
			}
			pauseState ++;
			break;

		case 6: //Wait for return to original position
			lcd.writeFromPgmspace(leavingPaused);
			if ( ! steppers::isRunning()) {
				pauseState = 0;
     		interface::popScreen();
				command::pause(false);
				if ( ! autoPause ) interface::popScreen();
			}
			break;
	}

	if ( lastDirectionButtonPressed ) {
		if (interface::isButtonPressed(lastDirectionButtonPressed))
			jog(lastDirectionButtonPressed);
		else	lastDirectionButtonPressed = (ButtonArray::ButtonName)0;
	}
}

void PauseMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	if ( button == ButtonArray::CANCEL ) {
		if ( pauseState == 3 )	pauseState ++;
	} else jog(button);
}

void PauseAtZPosScreen::reset() {
	float currentPause = command::getPauseAtZPos();
	if ( currentPause == 0.0 ) {
		Point position = steppers::getPosition();
		pauseAtZPos = (float)position[2] / getRevsPerMM();
	} else  pauseAtZPos = currentPause;
}

void PauseAtZPosScreen::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar message1[] = "Pause at ZPos:";
	const static PROGMEM prog_uchar message4[] = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar mm[]    = "mm   ";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(message1);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(message4);
	}

	// Redraw tool info
	lcd.setCursor(0,1);
	lcd.writeFloat((float)pauseAtZPos, 3);
	lcd.writeFromPgmspace(mm);
}

void PauseAtZPosScreen::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::OK:
		case ButtonArray::ZERO:
			//Set the pause
			command::pauseAtZPos(pauseAtZPos);
		case ButtonArray::CANCEL:
			interface::popScreen();
			interface::popScreen();
			break;
		case ButtonArray::ZPLUS:
			// increment more
			if (pauseAtZPos <= 250) pauseAtZPos += 1.0;
			break;
		case ButtonArray::ZMINUS:
			// decrement more
			if (pauseAtZPos >= 1.0) pauseAtZPos -= 1.0;
			else			pauseAtZPos = 0.0;
			break;
		case ButtonArray::YPLUS:
			// increment less
			if (pauseAtZPos <= 254) pauseAtZPos += 0.05;
			break;
		case ButtonArray::YMINUS:
			// decrement less
			if (pauseAtZPos >= 0.05) pauseAtZPos -= 0.05;
			else			 pauseAtZPos = 0.0;
			break;
		case ButtonArray::XMINUS:
		case ButtonArray::XPLUS:
			break;
	}

	if ( pauseAtZPos < 0.001 )	pauseAtZPos = 0.0;
}

void AdvanceABPMode::reset() {
	abpForwarding = false;
}

void AdvanceABPMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar abp1[] = "Advance ABP:";
	const static PROGMEM prog_uchar abp2[] = "hold key...";
	const static PROGMEM prog_uchar abp3[] = "           (fwd)";

	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(abp1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(abp2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(abp3);
	}

	if (( abpForwarding ) && ( ! interface::isButtonPressed(ButtonArray::OK) )) {
		OutPacket responsePacket;

		abpForwarding = false;
		extruderControl(SLAVE_CMD_TOGGLE_ABP, EXTDR_CMD_SET8, responsePacket, (uint16_t)0);
	}
}

void AdvanceABPMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	OutPacket responsePacket;

	switch (button) {
        	case ButtonArray::OK:
			abpForwarding = true;
			extruderControl(SLAVE_CMD_TOGGLE_ABP, EXTDR_CMD_SET8, responsePacket, (uint16_t)1);
			break;
        	case ButtonArray::YMINUS:
        	case ButtonArray::ZMINUS:
        	case ButtonArray::YPLUS:
        	case ButtonArray::ZPLUS:
        	case ButtonArray::XMINUS:
        	case ButtonArray::XPLUS:
        	case ButtonArray::ZERO:
        	case ButtonArray::CANCEL:
               		interface::popScreen();
			break;
	}
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
					uint16_t offset = eeprom::AXIS_HOME_POSITIONS + 4*i;
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
		uint16_t offset = eeprom::AXIS_HOME_POSITIONS + 4*i;
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
			uint16_t offset = eeprom::AXIS_HOME_POSITIONS + 4*i;
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


#endif
