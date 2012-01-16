#include "NYI.hh"

#include <stdlib.h>
#include "Interface.hh"

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