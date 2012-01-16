#include "MenuClass.hh"
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

#define MAX_ITEMS_PER_SCREEN 4

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