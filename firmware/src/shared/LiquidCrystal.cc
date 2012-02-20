#include "LiquidCrystal.hh"

#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "SharedEepromMap.hh"
#include "eeprom.hh"
#include <avr/eeprom.h>

// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

LiquidCrystal::LiquidCrystal(Pin rs, Pin rw, Pin enable,
			     Pin d0, Pin d1, Pin d2, Pin d3,
			     Pin d4, Pin d5, Pin d6, Pin d7)
{
  init(0, rs, rw, enable, d0, d1, d2, d3, d4, d5, d6, d7);
}

LiquidCrystal::LiquidCrystal(Pin rs, Pin enable,
			     Pin d0, Pin d1, Pin d2, Pin d3,
			     Pin d4, Pin d5, Pin d6, Pin d7)
{
  init(0, rs, Pin(), enable, d0, d1, d2, d3, d4, d5, d6, d7);
}

LiquidCrystal::LiquidCrystal(Pin rs, Pin rw, Pin enable,
			     Pin d0, Pin d1, Pin d2, Pin d3)
{
  init(1, rs, rw, enable, d0, d1, d2, d3, Pin(), Pin(), Pin(), Pin());
}

LiquidCrystal::LiquidCrystal(Pin rs,  Pin enable,
			     Pin d0, Pin d1, Pin d2, Pin d3)
{
  init(1, rs, Pin(), enable, d0, d1, d2, d3, Pin(), Pin(), Pin(), Pin());
}

void LiquidCrystal::init(uint8_t fourbitmode, Pin rs, Pin rw, Pin enable,
			 Pin d0, Pin d1, Pin d2, Pin d3,
			 Pin d4, Pin d5, Pin d6, Pin d7)
{
  _rs_pin = rs;
  _rw_pin = rw;
  _enable_pin = enable;
  
  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3; 
  _data_pins[4] = d4;
  _data_pins[5] = d5;
  _data_pins[6] = d6;
  _data_pins[7] = d7; 

  _rs_pin.setDirection(true);
  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
  if (!_rw_pin.isNull()) { _rw_pin.setDirection(true); }
 _enable_pin.setDirection(true);
  
  if (fourbitmode)
    _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  else 
    _displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
  
  dispsize = eeprom::getEeprom16(mbeeprom::DISPLAY_SIZE,16);
  row_offsets[0] = 0x00;
  row_offsets[1] = 0x40; 
  row_offsets[2] = dispsize;
  row_offsets[3] = 0x40+dispsize;
  begin(dispsize, 1);  
}

void LiquidCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _currline = 0;

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != 0) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  _delay_us(50000);
  // Now we pull both RS and R/W low to begin commands
  _rs_pin.setValue(false);
  _enable_pin.setValue(false);
  if (!_rw_pin.isNull()) {
    _rw_pin.setValue(false);
  }
  
  //put the LCD into 4 bit or 8 bit mode
  if (! (_displayfunction & LCD_8BITMODE)) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03);
    _delay_us(4500); // wait min 4.1ms

    // second try
    write4bits(0x03);
    _delay_us(4500); // wait min 4.1ms
    
    // third go!
    write4bits(0x03); 
    _delay_us(150);

    // finally, set to 8-bit interface
    write4bits(0x02); 
  } else {
    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    command(LCD_FUNCTIONSET | _displayfunction);
    _delay_us(4500);  // wait more than 4.1ms

    // second try
    command(LCD_FUNCTIONSET | _displayfunction);
    _delay_us(150);

    // third go
    command(LCD_FUNCTIONSET | _displayfunction);
  }

  // finally, set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);  

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

}

/********** high level commands, for the user! */
void LiquidCrystal::clear()
{
  command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  _delay_us(2000);  // this command takes a long time!
}

void LiquidCrystal::home()
{
  command(LCD_RETURNHOME);  // set cursor position to zero
  _delay_us(2000);  // this command takes a long time!
}

void LiquidCrystal::setCursor(uint8_t col, uint8_t row)
{
  
  if ( row > _numlines ) {
    row = _numlines-1;    // we count rows starting w/0
  }
  
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LiquidCrystal::noDisplay() {
  _displaycontrol &= ~LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LiquidCrystal::noCursor() {
  _displaycontrol &= ~LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal::cursor() {
  _displaycontrol |= LCD_CURSORON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LiquidCrystal::noBlink() {
  _displaycontrol &= ~LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal::blink() {
  _displaycontrol |= LCD_BLINKON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LiquidCrystal::scrollDisplayLeft(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal::scrollDisplayRight(void) {
  command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LiquidCrystal::leftToRight(void) {
  _displaymode |= LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LiquidCrystal::rightToLeft(void) {
  _displaymode &= ~LCD_ENTRYLEFT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LiquidCrystal::autoscroll(void) {
  _displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LiquidCrystal::noAutoscroll(void) {
  _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void LiquidCrystal::createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    write(charmap[i]);
  }
}

/*********** mid level commands, for sending data/cmds */

inline void LiquidCrystal::command(uint8_t value) {
  send(value, false);
}

inline void LiquidCrystal::write(uint8_t value) {
  send(value, true);
}


void LiquidCrystal::writeInt(uint16_t value, uint8_t digits) {

	uint16_t currentDigit;
	uint16_t nextDigit;

	switch (digits) {
	case 1:		currentDigit = 10;		break;
	case 2:		currentDigit = 100;		break;
	case 3:		currentDigit = 1000;	break;
	case 4:		currentDigit = 10000;	break;
	default: 	return;
	}

	for (uint8_t i = 0; i < digits; i++) {
		nextDigit = currentDigit/10;
		write((value%currentDigit)/nextDigit+'0');
		currentDigit = nextDigit;
	}
}

void LiquidCrystal::writeLong(uint32_t value, uint8_t digits) {

	uint32_t currentDigit;
	uint32_t nextDigit;

	switch (digits) {
	case 1:		currentDigit = 10;		break;
	case 2:		currentDigit = 100;		break;
	case 3:		currentDigit = 1000;	break;
	case 4:		currentDigit = 10000;	break;
	case 5:		currentDigit = 100000;	break;
	case 6:		currentDigit = 1000000;	break;
	case 7:		currentDigit = 10000000;	break;
	case 8:		currentDigit = 100000000;	break;
	case 9:		currentDigit = 1000000000;	break;
	default: 	return;
	}

	for (uint8_t i = 0; i < digits; i++) {
		nextDigit = currentDigit/10;
		write((value%currentDigit)/nextDigit+'0');
		currentDigit = nextDigit;
	}
}


//From: http://www.arduino.cc/playground/Code/PrintFloats
//tim [at] growdown [dot] com   Ammended to write a float to lcd

void LiquidCrystal::writeFloat(float value, uint8_t decimalPlaces) {
	LiquidCrystal::writeFloat(value,decimalPlaces,0);
}

void LiquidCrystal::writeFloat(float value, uint8_t decimalPlaces,uint8_t lpad) {
	// this is used to cast digits 
	int digit;
	float tens = 0.1;
	int tenscount = 0;
	int i;
	float tempfloat = value;

	// make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
	// if this rounding step isn't here, the value  54.321 prints as 54.3209

	// calculate rounding term d:   0.5/pow(10,decimalPlaces)  
	float d = 0.5;
	if (value < 0) d *= -1.0;

	// divide by ten for each decimal place
	for (i = 0; i < decimalPlaces; i++) d/= 10.0;    

	// this small addition, combined with truncation will round our values properly 
	tempfloat +=  d;

	// first get value tens to be the large power of ten less than value
	// tenscount isn't necessary but it would be useful if you wanted to know after this how many chars the number will take

	if (value < 0)	tempfloat *= -1.0;
	while ((tens * 10.0) <= tempfloat) {
		tens *= 10.0;
		tenscount += 1;
	}

	// write out the negative if needed
	if (value < 0) write('-');

	if (lpad>0) {
		for (i=0;i<lpad-tenscount;i++) {
			write('0');
		}
	} else {
		if (tenscount == 0) write('0');
	}

	for (i=0; i< tenscount; i++) {
		digit = (int) (tempfloat/tens);
		write(digit + '0');
		tempfloat = tempfloat - ((float)digit * tens);
		tens /= 10.0;
	}

	// if no decimalPlaces after decimal, stop now and return
	if (decimalPlaces <= 0) return;

	// otherwise, write the point and continue on
	write('.');

	// now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
	for (i = 0; i < decimalPlaces; i++) {
		tempfloat *= 10.0; 
		digit = (int) tempfloat;
		write(digit+'0');
		// once written, subtract off that digit
		tempfloat = tempfloat - (float) digit; 
	}
}

//Writes a fixed point number stored in padding.precision format
//where numbers are padded with leading zeros to "padding", and
//Displays "overflow" if the number doesn't fit within padding
//Example:  writeFixedPoint(2000 00000, 5, 5) displays 02000.00000

void LiquidCrystal::writeFixedPoint(int64_t value, uint8_t padding, uint8_t precision) {
        const static PROGMEM prog_uchar overflow[]  = "overflow";

	int64_t divisor = 1;
	for (uint8_t i = 0; i < (padding + precision); i ++ )
		divisor *= 10;
	
	if (( value / divisor ) > 0) {
        	writeFromPgmspace(overflow);
		return;
	}

	uint8_t i = 0;
	do {
		divisor /= 10;
		if ( i == padding )	write('.');
		write(((uint8_t)(value / divisor)) + '0');
		value %= divisor;
		i ++;	
	}
	while ( divisor > 1 );
}



void LiquidCrystal::writeString(char message[]) {
	char* letter = message;
	while (*letter != 0) {
		write(*letter);
		letter++;
	}
}

void LiquidCrystal::writeFromPgmspace(const prog_uchar message[]) {
	char letter;
	while (letter = pgm_read_byte(message++)) {
		write(letter);
	}
}

void LiquidCrystal::writeBlankLine(uint8_t line) {
	int i;
	
	setCursor(0,line);
	
	for (i=0;i<dispsize;i++) {
		write(' ');
	}
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void LiquidCrystal::send(uint8_t value, bool mode) {
  _rs_pin.setValue(mode);

  // if there is a RW pin indicated, set it low to Write
  if (!_rw_pin.isNull()) {
    _rw_pin.setValue(false);
  }
  
  if (_displayfunction & LCD_8BITMODE) {
    write8bits(value); 
  } else {
    write4bits(value>>4);
    write4bits(value);
  }
}

void LiquidCrystal::pulseEnable(void) {
  _enable_pin.setValue(false);
  _delay_us(1);
  _enable_pin.setValue(true);
  _delay_us(1);    // enable pulse must be >450ns
  _enable_pin.setValue(false);
  _delay_us(1);   // commands need > 37us to settle [citation needed]
}

void LiquidCrystal::write4bits(uint8_t value) {
  for (int i = 0; i < 4; i++) {
    _data_pins[i].setDirection(true);
    _data_pins[i].setValue(((value >> i) & 0x01) != 0);
  }

  pulseEnable();
}

void LiquidCrystal::write8bits(uint8_t value) {
  for (int i = 0; i < 8; i++) {
    _data_pins[i].setDirection(true);
    _data_pins[i].setValue(((value >> i) & 0x01) != 0);
  }
  
  pulseEnable();
}

uint8_t LiquidCrystal::getDisplayWidth() {
	return dispsize;
}
