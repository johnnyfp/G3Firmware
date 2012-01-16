#include "MBMenu.hh"
#include "Configuration.hh"

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

uint16_t pow (uint16_t x, uint8_t y) {
    int i,base;
    base = 1;
    for (i = 1; i <= y; ++i) base *= x;
    return base;
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