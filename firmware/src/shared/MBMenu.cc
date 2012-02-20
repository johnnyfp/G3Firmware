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
#include "SharedEepromMap.hh"
#include "eeprom.hh"
#include <avr/eeprom.h>
#include "ExtruderControl.hh"

uint32_t pow (uint16_t x, uint8_t y) {
    int i;
    int32_t base;
    base = 1;
    for (i = 1; i <= y; ++i) base *= x;
    return base;
}

double floor(double x)
{
	if(x>0)return (int)x;
	return (int)(x-0.9999999999999999);
}




/* For Debugging code
void DisplaySteps::reset() {
}

void DisplaySteps::update(LiquidCrystal& lcd, bool forceRedraw) {
	if (forceRedraw) {
		lcd.clear();
	lcd.setCursor(0,0);
	lcd.writeFixedPoint(eeprom::getEepromInt64(mbeeprom::STEPS_PER_MM_X, 90909), STEPS_PER_MM_PADDING, STEPS_PER_MM_PRECISION);
	lcd.setCursor(0,1);
	lcd.writeFixedPoint(eeprom::getEepromInt64(mbeeprom::STEPS_PER_MM_Y, 90909), STEPS_PER_MM_PADDING, STEPS_PER_MM_PRECISION);
	lcd.setCursor(0,2);
	lcd.writeFixedPoint(eeprom::getEepromInt64(mbeeprom::STEPS_PER_MM_Z, 90909), STEPS_PER_MM_PADDING, STEPS_PER_MM_PRECISION);
	lcd.setCursor(0,3);
	lcd.writeFixedPoint(eeprom::getEepromInt64(mbeeprom::STEPS_PER_MM_A, 90909), STEPS_PER_MM_PADDING, STEPS_PER_MM_PRECISION);
}
}

void DisplaySteps::notifyButtonPressed(ButtonArray::ButtonName button) {
	interface::popScreen();
	return;
}
*/

void StepsPerMMMode::reset() {
	lastStepsPerMMState = SPM_NONE;
	stepsPerMMState	    = SPM_THINGO;
	cursorLocation	    = 0;
	
  AxisPerMM::invalidate();
  AxisPerMM::populateAxisArray();
}

void StepsPerMMMode::populateDefaults(enum StepsPerMMState stepsPerMMState) {	
	switch(stepsPerMMState) {
			case SPM_THINGO:
					AxisPerMM::setAxis(AxisPerMM::AXIS_X,470698520000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Y,470698520000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Z,2000000000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_A,502354788069ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_B,502354788069ull);
				break;
			case SPM_CUPCAKE:
					AxisPerMM::setAxis(AxisPerMM::AXIS_X,470698520000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Y,470698520000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Z,12800000000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_A,502354788069ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_B,502354788069ull);
				break;
			case SPM_ULTIMAKER:
					AxisPerMM::setAxis(AxisPerMM::AXIS_X,470698520000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Y,470698520000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Z,1600000000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_A,18000000000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_B,18000000000000ull);
				break;
			case SPM_REPRAP:
					AxisPerMM::setAxis(AxisPerMM::AXIS_X,314960000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Y,314960000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_Z,11338580000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_A,10000000000000ull);
					AxisPerMM::setAxis(AxisPerMM::AXIS_B,10000000000000ull);
				break;
	}
				
}

#define STEPS_PER_MM_INCREMENT	0.000001

void StepsPerMMMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar msg1[]      = "Select Machine";
	const static PROGMEM prog_uchar msg2[]      = "use <> to choose";
	const static PROGMEM prog_uchar msg3[]      = "Key 0 to modify";
	const static PROGMEM prog_uchar machine1[]  = "Thing-o-matic";
	const static PROGMEM prog_uchar machine2[]  = "Cupcake";
	const static PROGMEM prog_uchar machine3[]  = "RepRap5d";
	const static PROGMEM prog_uchar machine4[]  = "Ultimaker";
	const static PROGMEM prog_uchar machine5[]  = "Custom";
	const static PROGMEM prog_uchar message1x[] = "X Steps per mm:";
	const static PROGMEM prog_uchar message1y[] = "Y Steps per mm:";
	const static PROGMEM prog_uchar message1z[] = "Z Steps per mm:";
	const static PROGMEM prog_uchar message1a[] = "A Steps per mm:";
	const static PROGMEM prog_uchar message1b[] = "B Steps per mm:";
	const static PROGMEM prog_uchar message4[]  = "Up/Dn/Ent to Set";
	const static PROGMEM prog_uchar blank[]     = "  ";

	if ( stepsPerMMState != lastStepsPerMMState )	forceRedraw = true;

	if (forceRedraw) {
		lcd.noCursor();
		lcd.clear();

		lcd.setCursor(0,0);
		switch(stepsPerMMState) {
			case SPM_THINGO:
			case SPM_CUPCAKE:
			case SPM_REPRAP:
			case SPM_ULTIMAKER:
			case SPM_CUSTOM:
				lcd.writeFromPgmspace(msg1);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(msg2);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(msg3);
				lcd.setCursor(0,3);
				break;
			default:
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(message4);
				break;
		}
		
		switch(stepsPerMMState) {
			case SPM_THINGO:
				lcd.writeFromPgmspace(machine1);
				break;
			case SPM_CUPCAKE:
				lcd.writeFromPgmspace(machine2);
				break;
			case SPM_REPRAP:
				lcd.writeFromPgmspace(machine3);
				break;
			case SPM_ULTIMAKER:
				lcd.writeFromPgmspace(machine4);
				break;
			case SPM_CUSTOM:
				lcd.writeFromPgmspace(machine5);
				break;
			default:
				lcd.setCursor(0,0);
		}
		switch(stepsPerMMState) {	
			case SPM_SET_X:
				lcd.writeFromPgmspace(message1x);
				break;
      case SPM_SET_Y:
				lcd.writeFromPgmspace(message1y);
				break;
     	case SPM_SET_Z:
				lcd.writeFromPgmspace(message1z);
				break;
     	case SPM_SET_A:
				lcd.writeFromPgmspace(message1a);
				break;
			case SPM_SET_B:
				lcd.writeFromPgmspace(message1b);
				break;
		}
	}

	switch(stepsPerMMState) {
			case SPM_THINGO:
			case SPM_CUPCAKE:
			case SPM_REPRAP:
			case SPM_ULTIMAKER:
			case SPM_CUSTOM:
				break;
			default:
				int64_t spm = 0;
			
				switch(stepsPerMMState) {
					case SPM_SET_X:
						spm =  AxisPerMM::getAxis(AxisPerMM::AXIS_X);
						break;
					case SPM_SET_Y:
						spm =  AxisPerMM::getAxis(AxisPerMM::AXIS_Y);
						break;
					case SPM_SET_Z:
						spm =  AxisPerMM::getAxis(AxisPerMM::AXIS_Z);
						break;
					case SPM_SET_A:
						spm =  AxisPerMM::getAxis(AxisPerMM::AXIS_A);
						break;
					case SPM_SET_B:
						spm =  AxisPerMM::getAxis(AxisPerMM::AXIS_B);
						break;
				}
			
				//Write the number
				lcd.setCursor(0,1);
				lcd.writeFixedPoint(spm, STEPS_PER_MM_PADDING, STEPS_PER_MM_PRECISION);
			
				//Draw the cursor
				lcd.setCursor(cursorLocation,1);
				lcd.cursor();
				/*lcd.write('^');
			
				//Write a blank before and after the cursor if we're not at the ends
				if ( cursorLocation >= 1 ) {
					lcd.setCursor(cursorLocation-1, 2);
					lcd.writeFromPgmspace(blank);
				}
				if ( cursorLocation < 15 ) {
					lcd.setCursor(cursorLocation+1, 2);
					lcd.writeFromPgmspace(blank);
				}*/
			break;
		}

	lastStepsPerMMState = stepsPerMMState;
}

void StepsPerMMMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	
	switch(stepsPerMMState) {
			case SPM_THINGO:
			case SPM_CUPCAKE:
			case SPM_REPRAP:
			case SPM_ULTIMAKER:
			case SPM_CUSTOM:
				switch (button) {
					case ButtonArray::CANCEL:
						interface::popScreen();
						return;
					case ButtonArray::XMINUS:
						if (stepsPerMMState != SPM_THINGO) stepsPerMMState = (enum StepsPerMMState)((uint8_t)stepsPerMMState - 1);
						break;
					case ButtonArray::XPLUS:
						if (stepsPerMMState != SPM_CUSTOM) stepsPerMMState = (enum StepsPerMMState)((uint8_t)stepsPerMMState + 1);
						break;
					case ButtonArray::ZERO:
						populateDefaults(stepsPerMMState);
						stepsPerMMState=SPM_SET_X;
						break;
					case ButtonArray::OK:
						switch(stepsPerMMState) {
							case SPM_THINGO:
							case SPM_CUPCAKE:
							case SPM_ULTIMAKER:
							case SPM_REPRAP:
								populateDefaults(stepsPerMMState);
								AxisPerMM::saveValues();
								interface::popScreen();
								break;
							case SPM_CUSTOM:
								populateDefaults(stepsPerMMState);
								stepsPerMMState=SPM_SET_X;
								break;
							}
					}
				break;
			default:
				int64_t spm;
				uint16_t offset;
				uint8_t currentIndex = stepsPerMMState - SPM_SET_X;
			
				spm = AxisPerMM::getAxis(currentIndex);
			
				//Calculate the increment based on the cursor location, allowing
				//for the decimal point
				int64_t increment = 1;
				for (uint8_t i = (STEPS_PER_MM_PADDING + STEPS_PER_MM_PRECISION); i >= 0; i -- ) {
					if ( i == cursorLocation ) break;
					if ( i != STEPS_PER_MM_PADDING ) increment *= 10;
				}
				
				//Don't increment if we're sitting on the decimcal point
				if ( cursorLocation == STEPS_PER_MM_PADDING )	increment = 0;
			
				switch (button) {
					case ButtonArray::CANCEL:
						AxisPerMM::invalidate();
						AxisPerMM::populateAxisArray();
						interface::getLcd().noCursor();
						interface::popScreen();
						return;
					case ButtonArray::ZERO:
						break;
					case ButtonArray::OK:
						if ( stepsPerMMState == SPM_SET_B ) {
							AxisPerMM::saveValues();
							interface::getLcd().noCursor();
							interface::popScreen();
						}
						else {
							//Increment to the next index
							stepsPerMMState = (enum StepsPerMMState)((uint8_t)stepsPerMMState + 1);
							cursorLocation	    = 0;
						}
						return;
					case ButtonArray::YPLUS:
					case ButtonArray::ZPLUS:
						// increment
						spm += increment;
						break;
					case ButtonArray::YMINUS:
					case ButtonArray::ZMINUS:
						// decrement
						spm -= increment;
						break;
					case ButtonArray::XMINUS:
						if ( cursorLocation > 0 )	cursorLocation --;
						if ( cursorLocation == STEPS_PER_MM_PADDING) cursorLocation--;
						break;
					case ButtonArray::XPLUS:
						if ( cursorLocation < 15 ) 	cursorLocation ++;
						if ( cursorLocation == STEPS_PER_MM_PADDING) cursorLocation++;
						break;
				}
			
				//Hard limits
				if ( spm >= STEPS_PER_MM_UPPER_LIMIT ) spm = STEPS_PER_MM_UPPER_LIMIT;
			        if ( spm <= STEPS_PER_MM_LOWER_LIMIT ) spm = STEPS_PER_MM_LOWER_LIMIT;
			
				AxisPerMM::setAxis(currentIndex,spm);
			}
}

void thermUpdate(LiquidCrystal& lcd, bool forceRedraw,uint16_t& beta,uint32_t& ohms,uint8_t& base,uint8_t& xpos,uint8_t& ypos) {
	
	static PROGMEM prog_uchar line2[]   = "Beta:";
	static PROGMEM prog_uchar line3[]   = "Ohms:";
	static PROGMEM prog_uchar line4[]   = "Base:";
	static PROGMEM prog_uchar inst[] = "^=Z+ v=z-";
	
	if (forceRedraw) {	
		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(line2);
		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(line3);
		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(line4);
		lcd.setCursor(8,3);
		lcd.writeFromPgmspace(inst);
	} 
	lcd.setCursor(5,1);
	lcd.writeInt(beta,4);
	lcd.setCursor(5,2);
	lcd.writeLong(ohms,7);
	lcd.setCursor(5,3);
	lcd.writeInt(base,2);
	lcd.cursor();
	lcd.blink();
	lcd.setCursor(xpos,ypos);
}

void thermButtonPressed(ButtonArray::ButtonName button,uint16_t& beta,uint32_t& ohms,uint8_t& base,uint8_t& xpos,uint8_t& ypos) {
	LiquidCrystal& lcd=interface::getLcd();
	switch (button) {
		case ButtonArray::OK:
 	  case ButtonArray::CANCEL:
 	  	lcd.noCursor();
 	  	lcd.noBlink();	
		  interface::popScreen();
		break;
		case ButtonArray::XMINUS:
			xpos--;
			if (xpos<5) xpos=5;
		break;
		case ButtonArray::XPLUS:
			xpos++;
			if ((ypos==1)&&(xpos>8)) xpos=8;
			else if ((ypos==2)&&(xpos>11)) xpos=11;
			else if ((ypos==3)&&(xpos>6)) xpos=6;	 
		break;
		case ButtonArray::YPLUS:
			ypos--;
			if (ypos<1) ypos=3;
			xpos=5;
		break;
		case ButtonArray::YMINUS:
			ypos++;
			if (ypos>3) ypos=1;
			xpos=5;
		break;
		case ButtonArray::ZPLUS:
			if (ypos==1) {
				beta=beta+(pow(10,3-(xpos-5)));
				if (beta>9999) beta=9999;
			} else if (ypos==2) {
				ohms=ohms+(pow(10,6-(xpos-5)));
				if (ohms>9999999) ohms=9999999;
			} else if (ypos==3) {
				base=base+(pow(10,1-(xpos-5)));
				if (base>99) base=99;
			}
		break;
		case ButtonArray::ZMINUS:
			if (ypos==1) {
				beta=beta-(pow(10,3-(xpos-5)));
				if (beta>9999) beta=0;
			} else if (ypos==2)  {
				ohms=ohms-(pow(10,6-(xpos-5)));
				if (ohms>9999999) ohms=0;
			} else if (ypos==3) {
				base=base-(pow(10,1-(xpos-5)));
				if (base>99) base=0;
			}
		break;
	}
}

void thermReset(uint16_t& beta,uint32_t& ohms,uint8_t& base,uint16_t thermTable) {
	OutPacket responsePacket;
	uint8_t cnt[1];
	cnt[0]=2;
	if (extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, thermTable+extrudereeprom::THERM_BETA_OFFSET,cnt)) {
		beta=responsePacket.read16(1);
	} else beta=0;
	cnt[0]=1;
	if (extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, thermTable+extrudereeprom::THERM_T0_OFFSET,cnt)) {
		base=responsePacket.read8(1);
	} else base=0;
	cnt[0]=4;
	if (extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, thermTable+extrudereeprom::THERM_R0_OFFSET,cnt)) {
		ohms=responsePacket.read32(1);
	} else {
  	ohms=0;
  }
}	

void thermWrite(uint16_t& beta,uint32_t& ohms,uint8_t& base,uint16_t thermTable) {
	OutPacket responsePacket;
	uint8_t cnt[5];
	cnt[0]=2;
	cnt[1]=(uint8_t)(beta&0xff);
	cnt[2]=(uint8_t)(beta >> 8);
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, thermTable+extrudereeprom::THERM_BETA_OFFSET,cnt);
	cnt[0]=1;
	cnt[1]=base;
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, thermTable+extrudereeprom::THERM_T0_OFFSET,cnt);
	cnt[0]=4;
	cnt[4]=((ohms>>24)&0xff);
	cnt[3]=((ohms>>16)&0xff);
	cnt[2]=((ohms>>8)&0xff);
	cnt[1]=(ohms&0xff);
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, thermTable+extrudereeprom::THERM_R0_OFFSET,cnt);
}

void ExtruTherm::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[]   = "Extruder Thermis";
	static PROGMEM prog_uchar line1x20[]= "tor ";
	
	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(line1);
		if (lcd.getDisplayWidth()>16) {
			lcd.writeFromPgmspace(line1x20);
		}

	} 
	thermUpdate(lcd,forceRedraw,beta,ohms,base,xpos,ypos);

}

void ExtruTherm::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::OK:
			thermWrite(beta,ohms,base,extrudereeprom::THERM_TABLE_0);
		default:
			thermButtonPressed(button,beta,ohms,base,xpos,ypos);
	}
}
	
void ExtruTherm::reset() {
	thermReset(beta,ohms,base,extrudereeprom::THERM_TABLE_0);
  ypos=1;
  xpos=5;
}

void HBPTherm::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[]   = "HBP Thermistor";
	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(line1);
	} 
	thermUpdate(lcd,forceRedraw,beta,ohms,base,xpos,ypos);
}

void HBPTherm::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::OK:
			thermWrite(beta,ohms,base,extrudereeprom::THERM_TABLE_1);
		default:
			thermButtonPressed(button,beta,ohms,base,xpos,ypos);
	}
}
	
void HBPTherm::reset() {
	thermReset(beta,ohms,base,extrudereeprom::THERM_TABLE_1);
  ypos=1;
  xpos=2;
}

void pidUpdate(LiquidCrystal& lcd, bool forceRedraw,float& ppid,float& ipid,float& dpid,uint8_t& xpos,uint8_t& ypos) {
	static PROGMEM prog_uchar line2[]   = "P:";
	static PROGMEM prog_uchar line3[]   = "I:";
	static PROGMEM prog_uchar line4[]   = "D:";
	
	if (ypos>0) {
	if (forceRedraw) {	
		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(line2);
		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(line3);
		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(line4);
	} 
	lcd.setCursor(2,1);
	lcd.writeFloat(ppid,3,3);
	lcd.setCursor(2,2);
	lcd.writeFloat(ipid,3,3);
	lcd.setCursor(2,3);
	lcd.writeFloat(dpid,3,3);
	lcd.cursor();
	lcd.blink();
	lcd.setCursor(xpos,ypos);
	}
}

void pidReset(float& ppid,float& ipid,float& dpid,uint16_t pidTable){
	OutPacket responsePacket;
	uint8_t cnt[1];
	cnt[0]=2;
	if (extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::P_TERM_OFFSET,cnt)) {
		ppid=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
	} else ppid=0.0;
	if (extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::I_TERM_OFFSET,cnt)) {
		ipid=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
	} else ipid=0.0;
	if (extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::D_TERM_OFFSET,cnt)) {
		dpid=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
	} else {
  	dpid=0.0;
  }
}

void pidWrite(float& ppid,float& ipid,float& dpid,uint16_t pidTable){
	LiquidCrystal& lcd=interface::getLcd();
		lcd.clear();
	Timeout t;
	OutPacket responsePacket;
	uint32_t temp;
	uint8_t cnt[3];
	cnt[0]=2;
	cnt[1]=(uint8_t)floor(ppid);
	cnt[2]=(uint8_t)((float)(ppid-floor(ppid))*256.0);
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::P_TERM_OFFSET,cnt);
	extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::P_TERM_OFFSET,cnt);	
	lcd.setCursor(0,1);
	lcd.writeInt(cnt[1],3);
	lcd.setCursor(0,2);
	lcd.writeInt(cnt[2],3);
	/*temp=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
	lcd.setCursor(0,1);
	lcd.writeFloat(temp,7,3);
	cnt[0]=2;
	cnt[1]=(uint8_t)floor(ipid);
	cnt[2]=(uint8_t)((float)(ipid-floor(ipid))*256.0);
	extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::I_TERM_OFFSET,cnt);
	temp=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
	lcd.setCursor(0,2);
	lcd.writeFloat(temp,7,3);
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::I_TERM_OFFSET,cnt);
	cnt[0]=2;
	cnt[1]=(uint8_t)floor(dpid);
	cnt[2]=(uint8_t)((float)(dpid-floor(dpid))*256.0);
	extruderControl(SLAVE_CMD_READ_FROM_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::D_TERM_OFFSET,cnt);
	temp=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
	lcd.setCursor(0,3);
	lcd.writeFloat(temp,7,3);
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::D_TERM_OFFSET,cnt);*/
}

void pidButtonPressed(ButtonArray::ButtonName button,float& ppid,float& ipid,float& dpid,uint8_t& xpos,uint8_t& ypos) {
	LiquidCrystal& lcd=interface::getLcd();
	float tempfloat;
	switch (button) {
		case ButtonArray::OK:
			ypos=0;
		  break;
 	  case ButtonArray::CANCEL:
 	  	lcd.noCursor();
 	  	lcd.noBlink();
 	  	interface::popScreen();
		break;
		case ButtonArray::XMINUS:
			xpos--;
			if (xpos<2) xpos=2;
			if (xpos==5) xpos=4;
		break;
		case ButtonArray::XPLUS:
			xpos++;
			if (xpos>8) xpos=8;
			if (xpos==5) xpos=6;
		break;
		case ButtonArray::YPLUS:
			ypos--;
			if (ypos<1) ypos=3;
			xpos=2;
		break;
		case ButtonArray::YMINUS:
			ypos++;
			if (ypos>3) ypos=1;
			xpos=2;
		break;
		case ButtonArray::ZPLUS:
			if (ypos==1) {
				tempfloat=ppid;
			} else if (ypos==2) {
				tempfloat=ipid;
			} else if (ypos==3) {
				tempfloat=dpid;
			}
			if (xpos<5) {
				tempfloat=tempfloat+(pow(10,2-(xpos-2)));
			} else {
				tempfloat=tempfloat+(float)(1.0/(float)pow(10,xpos-5));
			} 
			if (tempfloat>=256.0) tempfloat=255.999;	
			if (ypos==1) {
				ppid=tempfloat;
			} else if (ypos==2) {
				ipid=tempfloat;
			} else if (ypos==3) {
				dpid=tempfloat;
			}
		break;
		case ButtonArray::ZMINUS:
			if (ypos==1) {
				tempfloat=ppid;
			} else if (ypos==2) {
				tempfloat=ipid;
			} else if (ypos==3) {
				tempfloat=dpid;
			}
			if (xpos<5) {
				tempfloat=tempfloat-(pow(10,2-(xpos-2)));
			} else {
				tempfloat=tempfloat-(1.0/(float)pow(10,xpos-5));
			} 
			if (tempfloat<0.0) tempfloat=0.0;	
			if (ypos==1) {
				ppid=tempfloat;
			} else if (ypos==2) {
				ipid=tempfloat;
			} else if (ypos==3) {
				dpid=tempfloat;
			}
		break;
	}
	
}

void HBPPID::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[]   = "HBP PID Params";
	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(line1);
	} 
	pidUpdate(lcd,forceRedraw,ppid,ipid,dpid,xpos,ypos);
}

void HBPPID::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::OK:
			pidWrite(ppid,ipid,dpid,extrudereeprom::HBP_PID_BASE);
		default:
			pidButtonPressed(button,ppid,ipid,dpid,xpos,ypos);
	}
}
	
void HBPPID::reset() {
	pidReset(ppid,ipid,dpid,extrudereeprom::HBP_PID_BASE);
  ypos=1;
  xpos=2;
}

void ExtruPID::update(LiquidCrystal& lcd, bool forceRedraw) {
	static PROGMEM prog_uchar line1[]  		= "Extruder PID Par";
	static PROGMEM prog_uchar line1x20[]  = "ams";
	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(line1);
		if (lcd.getDisplayWidth()>16) {
			lcd.writeFromPgmspace(line1x20);
		}
	} 
	pidUpdate(lcd,forceRedraw,ppid,ipid,dpid,xpos,ypos);
}

void ExtruPID::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
		case ButtonArray::OK:
			pidWrite(ppid,ipid,dpid,extrudereeprom::EXTRUDER_PID_BASE);
		default:
			pidButtonPressed(button,ppid,ipid,dpid,xpos,ypos);
	}
}
	
void ExtruPID::reset() {
	pidReset(ppid,ipid,dpid,extrudereeprom::EXTRUDER_PID_BASE);
  ypos=1;
  xpos=2;
}

void OutputC::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void OutputC::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void OutputC::reset() {
}

void HommingFR::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void HommingFR::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void HommingFR::reset() {
}

void MachineName::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void MachineName::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void MachineName::reset() {
}

void InvertAxis::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void InvertAxis::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void InvertAxis::reset() {
}

void EStop::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void EStop::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void EStop::reset() {
}

void EndStopHome::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void EndStopHome::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void EndStopHome::reset() {
}

void EndStopAxis::update(LiquidCrystal& lcd, bool forceRedraw) {
}

void EndStopAxis::notifyButtonPressed(ButtonArray::ButtonName button) {
}
	
void EndStopAxis::reset() {
}



MBMenu::MBMenu() {
	itemCount = 14;
	reset();
}

void MBMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar item1[]  = "Axis Steps:mm   ";
	const static PROGMEM prog_uchar item2[]	 = "Extru thermistor";
	const static PROGMEM prog_uchar item3[]	 = "HBP thermistor  ";
	const static PROGMEM prog_uchar item4[]  = "Setup Menu < X- "; 
	const static PROGMEM prog_uchar item5[]	 = "*Extru PID param"; 
	const static PROGMEM prog_uchar item6[]  = "*HBP PID param  ";
	const static PROGMEM prog_uchar item7[]  = "*Machine Name   ";
	const static PROGMEM prog_uchar item8[]  = "*Invert Axis    ";
	const static PROGMEM prog_uchar item9[]  = "*EStop Installed";
	const static PROGMEM prog_uchar item10[] = "*End Stop Homes ";
	const static PROGMEM prog_uchar item11[] = "*End Stop Invert";
	const static PROGMEM prog_uchar item12[] = "*Ouput Channels ";
	const static PROGMEM prog_uchar item13[] = "*Homing FeedRate";
	const static PROGMEM prog_uchar item14[] = "*Toolhead Prefs ";

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
	case 6:
		lcd.writeFromPgmspace(item7);
		break;
	case 7:
		lcd.writeFromPgmspace(item8);
		break;
	case 8:
		lcd.writeFromPgmspace(item9);
		break;
  case 9:
		lcd.writeFromPgmspace(item10);
		break;
	case 10:
		lcd.writeFromPgmspace(item11);
		break;
	case 11:
		lcd.writeFromPgmspace(item12);
		break;
	case 12:
		lcd.writeFromPgmspace(item13);
		break;
	case 13:
		lcd.writeFromPgmspace(item14);
		break;
	}
}

void MBMenu::handleSelect(uint8_t index) {
	switch (index) {
		case 0:
			interface::pushScreen(&stepsPerMMMode);
			break;
		case 1:
			interface::pushScreen(&extrutherm);
			break;
		case 2:
			interface::pushScreen(&nbptherm);
			break;
		case 3:
			interface::popScreen();
			break;
		default:
			interface::pushScreen(&notyetimplemented);
		}
}

void MBMenu::handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex) {
	switch (button) {
		case ButtonArray::XMINUS:
			interface::popScreen();
			break;
	}
}