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
#include "Eeprom.hh"
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

MBMenu::MBMenu() {
	itemCount = 12;
	reset();
}

void MBMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar item1[]  = "Machine Name    ";
	const static PROGMEM prog_uchar item2[]  = "Invert Axis     ";
	const static PROGMEM prog_uchar item3[]  = "E-Stop Installed";
	const static PROGMEM prog_uchar item4[]  = "End Stop Homes  ";
	const static PROGMEM prog_uchar item5[]  = "End Stop Invert ";
	const static PROGMEM prog_uchar item6[]  = "ZAxis Rev per mm";
	const static PROGMEM prog_uchar item7[]	 = "Extru thermistor";
	const static PROGMEM prog_uchar item8[]	 = "HBP thermistor  ";
	const static PROGMEM prog_uchar item9[]	 = "Extru PID param "; 
	const static PROGMEM prog_uchar item10[] = "HBP PID param   ";
	const static PROGMEM prog_uchar item11[] = "Ouput Channels  ";
	const static PROGMEM prog_uchar item12[] = "Homing FeedRate ";

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
		case 6:
			interface::pushScreen(&extrutherm);
			break;
		case 7:
			interface::pushScreen(&nbptherm);
			break;
		case 8:
			interface::pushScreen(&extpid);
			break;
		case 9:
			interface::pushScreen(&hbppid);
			break;
		case 10:
			interface::pushScreen(&notyetimplemented);
			break;
		case 11:
			interface::pushScreen(&notyetimplemented);
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
			eeprom_write_word((uint16_t*)mbeeprom::ZAXIS_MM_PER_TURN_W,wholePart);
			eeprom_write_word((uint16_t*)mbeeprom::ZAXIS_MM_PER_TURN_P,fracPart);
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
	wholePart=eeprom::getEeprom16(mbeeprom::ZAXIS_MM_PER_TURN_W,200);
  fracPart=eeprom::getEeprom16(mbeeprom::ZAXIS_MM_PER_TURN_P,0);
  if (wholePart==200 && fracPart==0) selSize=0;
  else if (wholePart==1280 && fracPart==0) selSize=1;
  else if (wholePart==160 && fracPart==0) selSize=2;
  else if (wholePart==1133 && fracPart==8580) selSize=3;
  else if (wholePart==160 && fracPart==0) selSize=4;
  else selSize=5; 				
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
	lcd.writeFloat(ppid,7,3);
	lcd.setCursor(2,2);
	lcd.writeFloat(ipid,7,3);
	lcd.setCursor(2,3);
	lcd.writeFloat(dpid,7,3);
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
	temp=((float)responsePacket.read8(1)) + ((float)responsePacket.read8(2))/256.0;
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
	extruderControl(SLAVE_CMD_WRITE_TO_EEPROM, EXTDR_CMD_SET, responsePacket, pidTable+extrudereeprom::D_TERM_OFFSET,cnt);
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
			if (xpos>11) xpos=11;
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
			if (tempfloat>=255.0) tempfloat=255.9999999;	
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