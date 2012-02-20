#include "Menu.hh"
#include "Configuration.hh"

// TODO: Kill this, should be hanlded by build system.
#ifdef HAS_INTERFACE_BOARD

#include "Steppers.hh"
#include "Commands.hh"
#include "Command.hh"
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


#define HOST_PACKET_TIMEOUT_MS 20
#define HOST_PACKET_TIMEOUT_MICROS (1000L*HOST_PACKET_TIMEOUT_MS)

#define HOST_TOOL_RESPONSE_TIMEOUT_MS 50
#define HOST_TOOL_RESPONSE_TIMEOUT_MICROS (1000L*HOST_TOOL_RESPONSE_TIMEOUT_MS)


uint8_t pauseType = 1;
bool abpForwarding,fanOn,toggle =false;
const uint8_t MAX_FILE_LEN = host::MAX_FILE_LEN;
Point pausedPosition;
int16_t oes=0;

bool estimatingBuild = false;


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

UserViewMenu::UserViewMenu() {
	itemCount = 4;
	reset();
}

void UserViewMenu::resetState() {
        uint8_t jogModeSettings = eeprom::getEeprom8(mbeeprom::JOG_MODE_SETTINGS, 0);

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
	uint8_t jogModeSettings = eeprom::getEeprom8(mbeeprom::JOG_MODE_SETTINGS, 0);

	switch (index) {
	case 2:
		// Model View
		eeprom_write_byte((uint8_t *)mbeeprom::JOG_MODE_SETTINGS, (jogModeSettings & (uint8_t)0xFE));
		interface::popScreen();
		break;
	case 3:
		// User View
		eeprom_write_byte((uint8_t *)mbeeprom::JOG_MODE_SETTINGS, (jogModeSettings | (uint8_t)0x01));
                interface::popScreen();
		break;
	}
}

void JogMode::reset() {
	uint8_t jogModeSettings = eeprom::getEeprom8(mbeeprom::JOG_MODE_SETTINGS, 0);

	jogDistance = (enum distance_t)((jogModeSettings >> 1 ) & 0x07);
	if ( jogDistance > DISTANCE_CONT ) jogDistance = DISTANCE_0_1MM;

	distanceChanged = false;
	lastDirectionButtonPressed = (ButtonArray::ButtonName)0;

	userViewMode = jogModeSettings & 0x01;
	userViewModeChanged = false;
	
	steppers::switchToRegularDriver();
}

void JogMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar jog1[]      = "Jog mode: ";
	const static PROGMEM prog_uchar jog2[] 	    = "   Y+         Z+";
	const static PROGMEM prog_uchar jog3[]      = "X- V  X+  (mode)";
	const static PROGMEM prog_uchar jog4[]      = "   Y-         Z-";
	const static PROGMEM prog_uchar jog2_user[] = "  Y           Z+";
	const static PROGMEM prog_uchar jog3_user[] = "X V X     (mode)";
	const static PROGMEM prog_uchar jog4_user[] = "  Y           Z-";

	const static PROGMEM prog_uchar distance0_1mm[] = ".1mm";
	const static PROGMEM prog_uchar distance1mm[] = "1mm";
	const static PROGMEM prog_uchar distanceCont[] = "Cont..";

	if ( userViewModeChanged ) userViewMode = eeprom::getEeprom8(mbeeprom::JOG_MODE_SETTINGS, 0) & 0x01;

	if (forceRedraw || distanceChanged || userViewModeChanged) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(jog1);

		switch (jogDistance) {
		case DISTANCE_0_1MM:
			lcd.write(0xF3);	//Write tilde
			lcd.writeFromPgmspace(distance0_1mm);
			break;
		case DISTANCE_1MM:
			lcd.write(0xF3);	//Write tilde
			lcd.writeFromPgmspace(distance1mm);
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
			else {
				lastDirectionButtonPressed = (ButtonArray::ButtonName)0;
				steppers::abort();
			}
		}
	}
}

void JogMode::jog(ButtonArray::ButtonName direction) {
	Point position = steppers::getPosition();

	int32_t interval = 2000;
	float	speed;	//In mm's

	if ( jogDistance == DISTANCE_CONT )	interval = 1000;

	switch(jogDistance) {
	case DISTANCE_0_1MM:
		speed = 0.1;   //0.1mm
		break;
	case DISTANCE_1MM:
		speed = 1.0;   //1mm
		break;
	case DISTANCE_CONT:
		speed = 1.5;   //1.5mm
		break;
	}

	//Reverse direction of X and Y if we're in User View Mode and
	//not model mode
	int32_t vMode = 1;
	if ( userViewMode ) vMode = -1;

	switch(direction) {
        case ButtonArray::XMINUS:
		position[0] -= vMode * AxisPerMM::mmToSteps(speed,AxisPerMM::AXIS_X);
		break;
        case ButtonArray::XPLUS:
		position[0] += vMode * AxisPerMM::mmToSteps(speed,AxisPerMM::AXIS_X);
		break;
        case ButtonArray::YMINUS:
		position[1] -= vMode * AxisPerMM::mmToSteps(speed,AxisPerMM::AXIS_Y);
		break;
        case ButtonArray::YPLUS:
		position[1] += vMode * AxisPerMM::mmToSteps(speed,AxisPerMM::AXIS_Y);
		break;
        case ButtonArray::ZMINUS:
		position[2] -= AxisPerMM::mmToSteps(speed,AxisPerMM::AXIS_Z);
		break;
        case ButtonArray::ZPLUS:
		position[2] += AxisPerMM::mmToSteps(speed,AxisPerMM::AXIS_Z);
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
			case DISTANCE_0_1MM:
				jogDistance = DISTANCE_1MM;
				break;
			case DISTANCE_1MM:
				jogDistance = DISTANCE_CONT;
				break;
			case DISTANCE_CONT:
				jogDistance = DISTANCE_0_1MM;
				break;
		}
		distanceChanged = true;
		eeprom_write_byte((uint8_t *)mbeeprom::JOG_MODE_SETTINGS, userViewMode | (jogDistance << 1));
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
       		steppers::switchToAcceleratedDriver();
					break;
	}
}

void ExtruderMode::reset() {
	extrudeSeconds = (enum extrudeSeconds)eeprom::getEeprom8(mbeeprom::EXTRUDE_DURATION, 1);
	updatePhase = 0;
	timeChanged = false;
	lastDirection = 1;
	oes = 0;
	steppers::switchToRegularDriver();
}

void ExtruderMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar extrude1[] = "Extrude: ";
	const static PROGMEM prog_uchar extrude2[] = "(set rpm)    Fwd";
	const static PROGMEM prog_uchar extrude3[] = " (stop)    (dur)";
	const static PROGMEM prog_uchar extrude4[] = "---/---C     Rev";
	const static PROGMEM prog_uchar secs[]     = "SECS";
	const static PROGMEM prog_uchar blank[]    = "       ";

	if (oes)	extrude(oes, true);

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
				oes = seconds;
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
	int16_t stepsPerSecond = (int16_t)((200.0 * 8.0 * rpm) / 60.0);
	//50.235479 is ToM stepper extruder speed, we use this as a baseline
	stepsPerSecond = (int16_t)((float)stepsPerSecond * AxisPerMM::stepsToMM((int32_t)50.235479, AxisPerMM::AXIS_A));

	if ( seconds == 0 )	steppers::abort();
	else {
		position[3] += seconds * stepsPerSecond;
		steppers::setTarget(position, interval);
	}

	if (overrideTempCheck)	oes = 0;
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

			eeprom_write_byte((uint8_t *)mbeeprom::EXTRUDE_DURATION, (uint8_t)extrudeSeconds);

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
       			steppers::switchToAcceleratedDriver();
						break;
	}
}

void ExtruderSetRpmScreen::reset() {
	rpm = eeprom::getEeprom8(mbeeprom::EXTRUDE_RPM, 19);
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
			eeprom_write_byte((uint8_t *)mbeeprom::EXTRUDE_RPM, rpm);
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

void MonitorMode::reset() {
	updatePhase =  UPDATE_PHASE_FIRST;
	buildTimePhase = BUILD_TIME_PHASE_FIRST;
	lastBuildTimePhase = BUILD_TIME_PHASE_FIRST;
	lastElapsedSeconds = 0.0;
	pausePushLockout = false;
	pauseMode.autoPause = false;
	buildCompleteBuzzPlayed = false;
	overrideForceRedraw = false;
	copiesPrinted = 0;
	timeLeftDisplayed = false;
}

void MonitorMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar extruder_temp[]      =   "Tool: ---/---C";
	const static PROGMEM prog_uchar platform_temp[]      =   "Bed:  ---/---C";
	const static PROGMEM prog_uchar elapsed_time[]       =   "Elapsed:   0h00m";
	const static PROGMEM prog_uchar completed_percent[]  =   "Completed:   0% ";
	const static PROGMEM prog_uchar time_left[]          =   "TimeLeft:  0h00m";
	const static PROGMEM prog_uchar duration[]           =   "Duration:  0h00m";
	const static PROGMEM prog_uchar time_left_secs[]     =   "secs";
	const static PROGMEM prog_uchar time_left_none[]     =   "   none";
	const static PROGMEM prog_uchar zpos[] 		     =   "ZPos:           ";
	const static PROGMEM prog_uchar zpos_mm[] 	     =   "mm";
	const static PROGMEM prog_uchar estimate2[]          =   "Estimating:   0%";
	const static PROGMEM prog_uchar estimate3[]          =   "          (skip)";
	const static PROGMEM prog_uchar filament[]           =   "Filament:0.00m  ";
	const static PROGMEM prog_uchar copies[]	     =   "Copy:           ";
	const static PROGMEM prog_uchar of[]		     =   " of ";
	char buf[17];

	if ( command::isPaused() ) {
		if ( ! pausePushLockout ) {
			pausePushLockout = true;
			pauseMode.autoPause = true;
			interface::pushScreen(&pauseMode);
			return;
		}
	} else pausePushLockout = false;

	if ( host::getHostState() != host::HOST_STATE_ESTIMATING_FROM_SD )
	estimatingBuild = false;

	//Check for a build complete, and if we have more than one copy
	//to print, setup another one
	if (( ! estimatingBuild ) && ( host::isBuildComplete() )) {
		uint8_t copiesToPrint = eeprom::getEeprom8(mbeeprom::ABP_COPIES, 1);
		if ( copiesToPrint > 1 ) {
			if ( copiesPrinted < (copiesToPrint - 1)) {
				copiesPrinted ++;
				overrideForceRedraw = true;
				command::buildAnotherCopy();
			}
		}
	}

	if ((forceRedraw) || (overrideForceRedraw)) {
		lcd.clear();
		lcd.setCursor(0,0);
		switch(host::getHostState()) {
		case host::HOST_STATE_READY:
			lcd.writeString(host::getMachineName());
			break;
		case host::HOST_STATE_BUILDING:
		case host::HOST_STATE_BUILDING_FROM_SD:
		case host::HOST_STATE_ESTIMATING_FROM_SD:
			lcd.writeString(host::getBuildName());
			lcd.setCursor(0,1);
			if ( estimatingBuild ) {
				lcd.writeFromPgmspace(estimate2);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(estimate3);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(duration);
			} else {
				lcd.writeFromPgmspace(completed_percent);
			}
			break;
		case host::HOST_STATE_ERROR:
			lcd.writeString("error!");
			break;
		}

		if ( ! estimatingBuild ) {
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(extruder_temp);

			lcd.setCursor(0,3);
			lcd.writeFromPgmspace(platform_temp);

			lcd.setCursor(15,3);
			if ( command::getPauseAtZPos() == 0 )	lcd.write(' ');
			else					lcd.write('*');
		}
	}

	overrideForceRedraw = false;

	//Display estimating stats
	if ( estimatingBuild ) {
		//If preheat_during_estimate is set, then preheat
		OutPacket responsePacket;
		if ( eeprom::getEeprom8(mbeeprom::PREHEAT_DURING_ESTIMATE, 0) ) {
			uint8_t value = eeprom::getEeprom8(mbeeprom::TOOL0_TEMP, 220);
			extruderControl(SLAVE_CMD_SET_TEMP, EXTDR_CMD_SET, responsePacket, (uint16_t)value);
			value = eeprom::getEeprom8(mbeeprom::PLATFORM_TEMP, 110);
			extruderControl(SLAVE_CMD_SET_PLATFORM_TEMP, EXTDR_CMD_SET, responsePacket, value);
		}

		//Write out the % estimated
		lcd.setCursor(12,1);
		buf[0] = '\0';
		appendUint8(buf, sizeof(buf), (uint8_t)sdcard::getPercentPlayed());
		strcat(buf, "%");
		lcd.writeString(buf);

		//Write out the time calculated
		buf[0] = '\0';
		lcd.setCursor(9,3);
		appendTime(buf, sizeof(buf), (uint32_t)command::estimateSeconds());
		lcd.writeString(buf);

		//Check for estimate finished, and switch states to building
		if ( host::isBuildComplete() ) {
			//Store the estimate seconds
			buildDuration = command::estimateSeconds();
			host::setHostStateBuildingFromSD();
			command::setEstimation(false);
			overrideForceRedraw = true;
			estimatingBuild = false;
		}
	
		return;
	}

	OutPacket responsePacket;

	// Redraw tool info
	switch (updatePhase) {
	case UPDATE_PHASE_TOOL_TEMP:
		lcd.setCursor(6,2);
		if (extruderControl(SLAVE_CMD_GET_TEMP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case UPDATE_PHASE_TOOL_TEMP_SET_POINT:
		lcd.setCursor(10,2);
		if (extruderControl(SLAVE_CMD_GET_SP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case UPDATE_PHASE_PLATFORM_TEMP:
		lcd.setCursor(6,3);
		if (extruderControl(SLAVE_CMD_GET_PLATFORM_TEMP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}
		break;

	case UPDATE_PHASE_PLATFORM_SET_POINT:
		lcd.setCursor(10,3);
		if (extruderControl(SLAVE_CMD_GET_PLATFORM_SP, EXTDR_CMD_GET, responsePacket, 0)) {
			uint16_t data = responsePacket.read16(1);
			lcd.writeInt(data,3);
		} else {
			lcd.writeString("XXX");
		}

		lcd.setCursor(15,3);
		if ( command::getPauseAtZPos() == 0 )	lcd.write(' ');
		else					lcd.write('*');
		break;
	case UPDATE_PHASE_BUILD_PHASE_SCROLLER:
		enum host::HostState hostState = host::getHostState();
		
		if ( (hostState != host::HOST_STATE_BUILDING ) && ( hostState != host::HOST_STATE_BUILDING_FROM_SD )) break;

		//Signal buzzer if we're complete
		if (( ! buildCompleteBuzzPlayed ) && ( sdcard::getPercentPlayed() >= 100.0 )) {
			buildCompleteBuzzPlayed = true;
       			Motherboard::getBoard().buzz(2, 3, eeprom::getEeprom8(mbeeprom::BUZZER_REPEATS, 3));
		}

		bool okButtonHeld = interface::isButtonPressed(ButtonArray::OK);

		//Holding the ok button stops rotation
        	if ( okButtonHeld )	buildTimePhase = lastBuildTimePhase;

		float secs;
		int32_t tsecs;
		Point position;
		uint8_t precision;
		float completedPercent;
		float filamentUsed, lastFilamentUsed;

		switch (buildTimePhase) {
			case BUILD_TIME_PHASE_COMPLETED_PERCENT:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(completed_percent);
				lcd.setCursor(11,1);
				buf[0] = '\0';

				if ( buildDuration == 0 ) completedPercent = sdcard::getPercentPlayed();
				else			  completedPercent = ((float)command::estimateSeconds() / (float)buildDuration) * 100.0;

				appendUint8(buf, sizeof(buf), (uint8_t)completedPercent);
				strcat(buf, "% ");
				lcd.writeString(buf);
				break;
			case BUILD_TIME_PHASE_ELAPSED_TIME:
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
			case BUILD_TIME_PHASE_TIME_LEFT:
				lcd.setCursor(0,1);
				if (( timeLeftDisplayed ) || ( command::getFilamentLength() >= 1 )) {
					lcd.writeFromPgmspace(time_left);
					timeLeftDisplayed = true;
				}
				else					 lcd.writeFromPgmspace(duration);
				lcd.setCursor(9,1);

				tsecs = buildDuration - command::estimateSeconds();
				
				buf[0] = '\0';
				if 	  ((tsecs > 0 ) && (tsecs < 60) && ( host::isBuildComplete() ) ) {
					appendUint8(buf, sizeof(buf), (uint8_t)tsecs);
					lcd.writeString(buf);
					lcd.writeFromPgmspace(time_left_secs);	
				} else if (( tsecs <= 0) || ( host::isBuildComplete()) ) {
					command::addFilamentUsed();
					lcd.writeFromPgmspace(time_left_none);
				} else {
					appendTime(buf, sizeof(buf), (uint32_t)tsecs);
					lcd.writeString(buf);
				}
				break;
			case BUILD_TIME_PHASE_ZPOS:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(zpos);
				lcd.setCursor(6,1);

				position = steppers::getPosition();
			
				//Divide by the axis steps to mm's
				lcd.writeFloat(AxisPerMM::stepsToMM(position[2], AxisPerMM::AXIS_Z), 3);

				lcd.writeFromPgmspace(zpos_mm);
				break;
			case BUILD_TIME_PHASE_FILAMENT:
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(filament);
				lcd.setCursor(9,1);
				lastFilamentUsed = AxisPerMM::stepsToMM(command::getLastFilamentLength(), AxisPerMM::AXIS_A);
				if ( lastFilamentUsed != 0.0 )	filamentUsed = lastFilamentUsed;
				else				filamentUsed = AxisPerMM::stepsToMM(command::getFilamentLength(), AxisPerMM::AXIS_A);
				filamentUsed /= 1000.0;	//convert to meters
				if	( filamentUsed < 0.1 )	{
					 filamentUsed *= 1000.0;	//Back to mm's
					precision = 1;
				}
				else if ( filamentUsed < 10.0 )	 precision = 4;
				else if ( filamentUsed < 100.0 ) precision = 3;
				else				 precision = 2;
				lcd.writeFloat(filamentUsed, precision);
				if ( precision == 1 ) lcd.write('m');
				lcd.write('m');
				break;
			case BUILD_TIME_PHASE_COPIES_PRINTED:
				uint8_t totalCopies = eeprom::getEeprom8(mbeeprom::ABP_COPIES, 1);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(copies);
				lcd.setCursor(7,1);
				lcd.writeFloat((float)(copiesPrinted + 1), 0);
				lcd.writeFromPgmspace(of);
				lcd.writeFloat((float)totalCopies, 0);
				break;
		}

        	if ( ! okButtonHeld ) {
			//Advance buildTimePhase and wrap around
			lastBuildTimePhase = buildTimePhase;
			buildTimePhase = (enum BuildTimePhase)((uint8_t)buildTimePhase + 1);

			//Skip Time left if we skipped the estimation
			if (( buildTimePhase == BUILD_TIME_PHASE_TIME_LEFT ) && ( buildDuration == 0 ))
				buildTimePhase = (enum BuildTimePhase)((uint8_t)buildTimePhase + 1);

			//If we're setup to print more than one copy, then show that build phase,
			//otherwise skip it
			if ( buildTimePhase == BUILD_TIME_PHASE_COPIES_PRINTED ) {
				uint8_t totalCopies = eeprom::getEeprom8(mbeeprom::ABP_COPIES, 1);
				if ( totalCopies <= 1 )	
					buildTimePhase = (enum BuildTimePhase)((uint8_t)buildTimePhase + 1);
			}

			if ( buildTimePhase >= BUILD_TIME_PHASE_LAST )
				buildTimePhase = BUILD_TIME_PHASE_FIRST;
		}
		break;
	}

	//Advance updatePhase and wrap around
	updatePhase = (enum UpdatePhase)((uint8_t)updatePhase + 1);
	if (updatePhase >= UPDATE_PHASE_LAST)
		updatePhase = UPDATE_PHASE_FIRST;

	steppers::doLcd();
}


void MonitorMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	switch (button) {
        case ButtonArray::CANCEL:
		switch(host::getHostState()) {
		case host::HOST_STATE_BUILDING:
		case host::HOST_STATE_BUILDING_FROM_SD:
		case host::HOST_STATE_ESTIMATING_FROM_SD:
                        interface::pushScreen(&cancelBuildMenu);
			break;
		default:
                        interface::popScreen();
			break;
		}
        case ButtonArray::ZPLUS:
		if ( host::getHostState() == host::HOST_STATE_BUILDING_FROM_SD )
			updatePhase = UPDATE_PHASE_BUILD_PHASE_SCROLLER;
		break;
	case ButtonArray::OK:
		//Skip build if user hit the button during the estimation phase
		if (( host::getHostState() == host::HOST_STATE_ESTIMATING_FROM_SD ) && ( estimatingBuild )) {
			buildDuration = 0;
			host::setHostStateBuildingFromSD();
			command::setEstimation(false);
			overrideForceRedraw = true;
			estimatingBuild = false;
		}
		break;
	}
}

CancelBuildMenu::CancelBuildMenu() {
	pauseMode.autoPause = false;
	itemCount = 3;
	reset();
	pauseDisabled = false;
	if ( ( estimatingBuild ) || ( steppers::isHoming() ) ||
	   (sdcard::getPercentPlayed() >= 100.0))	pauseDisabled = true;

	if (( ! estimatingBuild ) && ( host::isBuildComplete() ))
		printAnotherEnabled = true;
	else	printAnotherEnabled = false;

}

void CancelBuildMenu::resetState() {
	pauseMode.autoPause = false;
	pauseDisabled = false;	
	if ( ( estimatingBuild ) || ( steppers::isHoming() ) ||
	     (sdcard::getPercentPlayed() >= 100.0))	pauseDisabled = true;

	if (( ! estimatingBuild ) && ( host::isBuildComplete() ))
		printAnotherEnabled = true;
	else	printAnotherEnabled = false;
		
	itemCount = 3;
	
	if ( pauseDisabled )	{
		itemIndex = 2;
	} else {
		itemIndex = 1;
		firstSubItemIndex = 1;
		if ( printAnotherEnabled ) {
			lastSubItemIndex = 5;
		} else lastSubItemIndex = 4;
	}

	firstItemIndex = itemIndex;
	subItemIndex = pauseType;
}

void CancelBuildMenu::drawItemSub(uint8_t index, uint8_t subIndex, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar title[]  = "Use <> to choose";
	const static PROGMEM prog_uchar pauseNZ[]= "Pause on Next Z";
	const static PROGMEM prog_uchar pause[]  = "Pause          ";
	const static PROGMEM prog_uchar abort[]  = "Abort Print    ";
	const static PROGMEM prog_uchar printAnother[]	= "Print Another";
	const static PROGMEM prog_uchar pauseZP[]= "Pause at ZPos  ";
	const static PROGMEM prog_uchar pauseFM[]= "Pause free move";
	const static PROGMEM prog_uchar nopaus[] = "*Pause Disabled*";

	if ( ( estimatingBuild ) || ( steppers::isHoming() ) ||
	    (sdcard::getPercentPlayed() >= 100.0))	pauseDisabled = true;

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
					case 5:
						lcd.writeFromPgmspace(printAnother);
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
				case 5:
					command::buildAnotherCopy();
					interface::popScreen();
				break;
			}
		}
		break;
	case 2:
		// Cancel build, returning to whatever menu came before monitor mode.
		// TODO: Cancel build.
		command::addFilamentUsed();
		interface::popScreen();
		host::stopBuild();
		break;
	}
}

/*
CardMenu::CardMenu() {
}

void CardMenu::resetState(){

	sdlasterror=sdcardmanager::initializeSD();

	if (sdlasterror!=sdcardmanager::SD_SUCCESS && sdlasterror!=sdcardmanager::SD_ERR_CARD_LOCKED) {
		itemCount = 1;
		itemIndex = 0;
	} else {
		itemCount = countFiles();
		updatePhase = 0;
		lastItemIndex = 0;
		drawItemLockout = false;
	}
}

uint8_t CardMenu::countFiles() {
	uint8_t count = 0;

	// First, reset the directory index
	sdlasterror = sdcardmanager::directoryReset();
	if (sdlasterror != sdcardmanager::SD_SUCCESS) {
		return 6;
	}

	const int MAX_FILE_LEN = 2;
	char fnbuf[MAX_FILE_LEN];

	// Count the files
	do {
		if (inDirectories) {
			sdlasterror = sdcardmanager::directoryNextDirEntry(fnbuf,MAX_FILE_LEN);
		} else {
			sdlasterror = sdcardmanager::directoryNextEntry(fnbuf,MAX_FILE_LEN);
		}
		if (fnbuf[0] == '\0') {
			break;
		}

		// If it's a dot file, don't count it.
		if (fnbuf[0] == '.') {
		}
		else {
			count++;
		}
		if (count>32000) {
			sdlasterror=sdcardmanager::SD_ERR_GENERIC;
		}
	} while (sdlasterror == sdcardmanager::SD_SUCCESS);

	// TODO: Check for error again?

	return count;
}

bool CardMenu::getFilename(uint8_t index, char buffer[], uint8_t buffer_size) {

	// First, reset the directory list
	sdlasterror = sdcardmanager::directoryReset();
	if (sdlasterror != sdcardmanager::SD_SUCCESS) {
    return false;
	}


	for(uint8_t i = 0; i < index+1; i++) {
		// Ignore dot-files
		do {
			if (inDirectories) {
				sdlasterror = sdcardmanager::directoryNextDirEntry(buffer,buffer_size);
			} else {
				sdlasterror = sdcardmanager::directoryNextEntry(buffer,buffer_size);
			}
			if (buffer[0] == '\0') {
        return false;
			}
		} while (sdlasterror == sdcardmanager::SD_SUCCESS && buffer[0] == '.');

		if (sdlasterror != sdcardmanager::SD_SUCCESS) {
       return false;
		}
	}
   return true;
}

void CardMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar err0[] = " Error with SD";
	const static PROGMEM prog_uchar err1[] = "No card Inserted";
	const static PROGMEM prog_uchar err2[] = "Unable to read";
	const static PROGMEM prog_uchar err3[] = "partition";
	const static PROGMEM prog_uchar err4[] = "filesystem";
	const static PROGMEM prog_uchar err5[] = "File not found  ";
	const static PROGMEM prog_uchar err6[] = "Unknown Error";
	const static PROGMEM prog_uchar err7[] = "Failed to";
	const static PROGMEM prog_uchar err8[] = "Initialise Card";
	const static PROGMEM prog_uchar err9[] = "find root dir  ";
	const static PROGMEM prog_uchar err10[]= "Machine not";
	const static PROGMEM prog_uchar err11[]= "ready to process";
	const static PROGMEM prog_uchar err12[]= "file";
	
	if (sdlasterror!=sdcardmanager::SD_SUCCESS && sdlasterror!=sdcardmanager::SD_ERR_CARD_LOCKED) {
		lcd.setCursor(0,0);
	}
	
	switch(sdlasterror) {
		case sdcardmanager::SD_ERR_NO_CARD_PRESENT:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err1);
			break;
		case sdcardmanager::SD_ERR_INIT_FAILED:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err7);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err8);
			break;
		case sdcardmanager::SD_ERR_PARTITION_READ:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err2);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err3);
			break;
		case sdcardmanager::SD_ERR_OPEN_FILESYSTEM:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err2);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err4);
			break;
		case sdcardmanager::SD_ERR_NO_ROOT:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err7);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err9);
			break;
		case sdcardmanager::SD_ERR_FILE_NOT_FOUND:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err5);
			break;
		case sdcardmanager::SD_ERR_GENERIC:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err6);
			break;
		case sdcardmanager::SD_ERR_HOST_NR:
			lcd.writeFromPgmspace(err10);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err11);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err12);
			break;
		default:
			if (index > itemCount - 1) {
				// TODO: report error
				sdlasterror=sdcardmanager::SD_ERR_GENERIC;
				return;
			}
					
			char fnbuf[MAX_FILE_LEN];
		
		  if ( !getFilename(index, fnbuf, MAX_FILE_LEN) ) {
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
}

void CardMenu::handleSelect(uint8_t index){
	if (inDirectories) {
		char fnbuf[MAX_FILE_LEN];
    if (getFilename(index, fnbuf, MAX_FILE_LEN)) {
    	sdcardmanager::cd(fnbuf);
    }
    inDirectories=false;
    invalidate=true;
	} else {
	
		if (host::getHostState() != host::HOST_STATE_READY) {
			sdlasterror=sdcardmanager::SD_ERR_HOST_NR;
			return;
		}

		drawItemLockout = true;

		char* buildName = host::getBuildName();

    if ( !getFilename(index, buildName, host::MAX_FILE_LEN) ) {
			return;
		}

		sdlasterror = host::startBuildFromSD();
		if (sdlasterror != sdcardmanager::SD_SUCCESS) {
			return;
		}
	}
}

void CardMenu::handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex){
	switch (button) {
		case ButtonArray::XPLUS:
			inDirectories=true;
			resetState();
			invalidate=true;
			break;
		case ButtonArray::XMINUS:
			inDirectories=false;
			resetState();
			invalidate=true;
			break;
	}
}
*/
SDMenu::SDMenu() {
	reset();
	updatePhase = 0;
	drawItemLockout = false;
	inDirectories = false;
	invalidate=false;
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
	uint8_t idx = 0;

	// First, reset the directory index
	sdlasterror = sdcard::directoryReset();
	if (sdlasterror != sdcard::SD_SUCCESS) {
		return 6;
	}





	int maxFileLength = 64; /// SD card max lenghth
	char fnbuf[maxFileLength];

	// Count the files
	do {
		/*if (inDirectories) {
			sdlasterror = sdcard::directoryNextDirEntry(fnbuf,MAX_FILE_LEN);
		} else {*/
			sdlasterror = sdcard::directoryNextEntry(fnbuf,MAX_FILE_LEN,&idx);
		/*}*/
		if (fnbuf[0] == '\0') {
			break;
		}

		// If it's a dot file, don't count it.
		if (fnbuf[0] == '.') {
		}
		
		else {
			if ((fnbuf[idx-3] == 's') && (fnbuf[idx-2] == '3') && (fnbuf[idx-1] == 'g'))
			count++;
		}
	} while (sdlasterror == sdcard::SD_SUCCESS);

	// TODO: Check for error again?

	return count;
}

bool SDMenu::getFilename(uint8_t index, char buffer[], uint8_t buffer_size) {

	uint8_t idx;

	// First, reset the directory list
	sdlasterror = sdcard::directoryReset();
	if (sdlasterror != sdcard::SD_SUCCESS) {
                return false;
	}

	int maxFileLength = 64; /// SD card max lenghth
	char fnbuf[maxFileLength];

	for(uint8_t i = 0; i < index+1; i++) {
		// Ignore dot-files
		do {
			/*if (inDirectories) {
				sdlasterror = sdcard::directoryNextDirEntry(buffer,buffer_size);
			} else {*/
				sdlasterror = sdcard::directoryNextEntry(fnbuf,maxFileLength, &idx);
			/*}*/
			if (fnbuf[0] == '\0') {
         return false;
			}
		} while ((sdlasterror == sdcard::SD_SUCCESS) && ((fnbuf[0] == '.')) || 
			!((fnbuf[idx-3] == 's') && (fnbuf[idx-2] == '3') && (fnbuf[idx-1] == 'g')));

		if (sdlasterror != sdcard::SD_SUCCESS) {
       return false;
		}
	}

  uint8_t bufSize = (buffer_size <= idx) ? buffer_size-1 : idx; 	
	for(uint8_t i = 0; i < bufSize; i++)
		buffer[i] = fnbuf[i];
	buffer[bufSize] = 0;
    return true;
}

void SDMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar err0[] = " Error with SD";
	const static PROGMEM prog_uchar err1[] = "No card Inserted";
	const static PROGMEM prog_uchar err2[] = "Unable to read";
	const static PROGMEM prog_uchar err3[] = "partition";
	const static PROGMEM prog_uchar err4[] = "filesystem";
	const static PROGMEM prog_uchar err5[] = "File not found  ";
	const static PROGMEM prog_uchar err6[] = "Unknown Error";
	const static PROGMEM prog_uchar err7[] = "Failed to";
	const static PROGMEM prog_uchar err8[] = "Initialise Card";
	const static PROGMEM prog_uchar err9[] = "find root dir  ";
	const static PROGMEM prog_uchar err10[]= "Machine not";
	const static PROGMEM prog_uchar err11[]= "ready to process";
	const static PROGMEM prog_uchar err12[]= "file";
	const static PROGMEM prog_uchar err13[]= "read the ";
	
	if (sdlasterror!=sdcard::SD_SUCCESS && sdlasterror!=sdcard::SD_ERR_CARD_LOCKED) {
		lcd.setCursor(0,0);
	}
	
	switch(sdlasterror) {
		case sdcard::SD_ERR_NO_CARD_PRESENT:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err1);
			break;
		case sdcard::SD_ERR_INIT_FAILED:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err7);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err8);
			break;
		case sdcard::SD_ERR_PARTITION_READ:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err2);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err3);
			break;
		case sdcard::SD_ERR_OPEN_FILESYSTEM:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err2);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err4);
			break;
		case sdcard::SD_ERR_NO_ROOT:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err7);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err9);
			break;
		case sdcard::SD_ERR_FILE_NOT_FOUND:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err5);
			break;
		case sdcard::SD_ERR_GENERIC:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err6);
			break;
		case sdcard::SD_ERR_HOST_NR:
			lcd.writeFromPgmspace(err10);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err11);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err12);
			break;
		case sdcard::SD_ERR_READING:
			lcd.writeFromPgmspace(err0);
			lcd.setCursor(0,1);
			lcd.writeFromPgmspace(err7);
			lcd.setCursor(0,2);
			lcd.writeFromPgmspace(err13);
			lcd.writeFromPgmspace(err12);
			break;
		default:
		if (index > itemCount - 1) {
			sdlasterror=sdcard::SD_ERR_GENERIC;
			return;
		}
	
		
		char fnbuf[MAX_FILE_LEN];
	
	  if ( !getFilename(index, fnbuf, MAX_FILE_LEN) ) {
	    sdlasterror=sdcard::SD_ERR_READING;      	
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
}

void SDMenu::update(LiquidCrystal& lcd, bool forceRedraw) {
	
	if (( ! forceRedraw ) && ( ! drawItemLockout ))  { //|| invalidate
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
	
	/*if (invalidate) 
		{
			forceRedraw=true;
			invalidate=false;
		}*/

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

/*void SDMenu::handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex) {
	switch (button) {
		case ButtonArray::XPLUS:
			inDirectories=true;
			resetState();
			invalidate=true;
			break;
		case ButtonArray::XMINUS:
			inDirectories=false;
			resetState();
			invalidate=true;
			break;
	}
}*/
		

void SDMenu::handleSelect(uint8_t index) {
	
	/*if (inDirectories) {
		char fnbuf[MAX_FILE_LEN];
    if (getFilename(index, fnbuf, MAX_FILE_LEN)) {
    	sdcard::cd(fnbuf);
    }
    inDirectories=false;
    invalidate=true;
	} else {*/
	
		if (host::getHostState() != host::HOST_STATE_READY) {
			sdlasterror=sdcard::SD_ERR_HOST_NR;
			return;
		}

		drawItemLockout = true;

		char* buildName = host::getBuildName();

    if ( !getFilename(index, buildName, host::MAX_FILE_LEN) ) {
    	sdlasterror= sdcard::SD_ERR_READING;
			return;
		}
		estimatingBuild = true;
		command::setEstimation(true);
	  sdlasterror = host::startBuildFromSD(true);
		if (sdlasterror != sdcard::SD_SUCCESS) {
			return;
		}
	//}
}

void Tool0TempSetScreen::reset() {
	value = eeprom::getEeprom8(mbeeprom::TOOL0_TEMP, 220);
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
        	break;
        case ButtonArray::OK:
		eeprom_write_byte((uint8_t*)mbeeprom::TOOL0_TEMP,value);
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
	value = eeprom::getEeprom8(mbeeprom::PLATFORM_TEMP, 110);;
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
    	break;    	
    case ButtonArray::OK:
			eeprom_write_byte((uint8_t*)mbeeprom::PLATFORM_TEMP,value);
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
				uint8_t value = eeprom::getEeprom8(mbeeprom::TOOL0_TEMP, 220);
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
				uint8_t value = eeprom::getEeprom8(mbeeprom::PLATFORM_TEMP, 110);
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

void PauseMode::reset() {
	pauseState = 0;
	lastDirectionButtonPressed = (ButtonArray::ButtonName)0;
}

void PauseMode::jog(ButtonArray::ButtonName direction) {
	uint8_t steps = 50;
	bool extrude = false;
	int32_t interval = 1000;
	float	speed = 1.5;	//In mm's
	Point position = steppers::getPosition();

	switch(direction) {
    case ButtonArray::XMINUS:
			position[0] -= AxisPerMM::mmToSteps(speed, AxisPerMM::AXIS_X);
			break;
    case ButtonArray::XPLUS:
		  position[0] += AxisPerMM::mmToSteps(speed, AxisPerMM::AXIS_X);
		  break;
    case ButtonArray::YMINUS:
			position[1] -= AxisPerMM::mmToSteps(speed, AxisPerMM::AXIS_Y);
			break;
    case ButtonArray::YPLUS:
			position[1] += AxisPerMM::mmToSteps(speed, AxisPerMM::AXIS_Y);
			break;
    case ButtonArray::ZMINUS:
			position[2] -= AxisPerMM::mmToSteps(speed, AxisPerMM::AXIS_Z);
			break;
    case ButtonArray::ZPLUS:
			position[2] += AxisPerMM::mmToSteps(speed, AxisPerMM::AXIS_Z);
			break;
		case ButtonArray::OK:
		case ButtonArray::ZERO:
			float rpm = (float)eeprom::getEeprom8(mbeeprom::EXTRUDE_RPM, 19) / 10.0;

			//60 * 1000000 = # uS in a minute
			//200 * 8 = 200 steps per revolution * 1/8 stepping
			interval = (int32_t)(60L * 1000000L) / (int32_t)((float)(200 * 8) * rpm);
			int16_t stepsPerSecond = (int16_t)((200.0 * 8.0 * rpm) / 60.0);

			//50.235479 is ToM stepper extruder speed, we
			//use this as a baseline
			stepsPerSecond = (int16_t)((float)stepsPerSecond * AxisPerMM::stepsToMM((int32_t)50.235479, AxisPerMM::AXIS_A));

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
	const static PROGMEM prog_uchar retractFilament[]	= "Retract Filament";
	const static PROGMEM prog_uchar movingZ[] 		 = "Moving Z up 2mm ";
	const static PROGMEM prog_uchar movingY[]			 = "Moving Y 2mm    ";
	const static PROGMEM prog_uchar leavingPaused[]= "Leaving pause.. ";
	const static PROGMEM prog_uchar paused1[] 		 = "Paused(";
	const static PROGMEM prog_uchar paused2[] 		 = "    Y+       Z+ ";
	const static PROGMEM prog_uchar paused3[] 		 = " X- Rev X+ (Fwd)";
	const static PROGMEM prog_uchar paused4[] 		 = "    Y-       Z- ";
	const static PROGMEM prog_uchar paused5[]			 = "Free Move";
	const static PROGMEM prog_uchar paused6[]			 = "         ";

	int32_t interval = 1000;
	Point newPosition = pausedPosition;
	 	
	if (forceRedraw)	lcd.clear();

	lcd.setCursor(0,0);

	switch (pauseState) {
		case 0:	//Entered pause, waiting for steppers to finish last command
			lcd.writeFromPgmspace(waitForCurrentCommand);

			steppers::switchToRegularDriver();

			if ( ! steppers::isRunning() && command::isPaused()) pauseState ++;
			break;

		case 1:	//Last command finished, record current position and
			//retract filament
			lcd.writeFromPgmspace(retractFilament);

			pausedPosition = steppers::getPosition();
			newPosition = pausedPosition;
			newPosition[3] += AxisPerMM::mmToSteps(1.0, AxisPerMM::AXIS_A);	//Retract the filament so we don't get blobs
			steppers::setTarget(newPosition, interval / 2);
			
			pauseState ++;
			break;

		case 2: //Wait for the retract to complete
			lcd.writeFromPgmspace(retractFilament);
			if ( ! steppers::isRunning()) {
				pauseState ++;
			}
			break;


		case 3: //Last command finished, record current position and move
			//Z away from build

			pausedPosition = steppers::getPosition();
			newPosition = pausedPosition;
			
			if (!freeMove) {
				lcd.writeFromPgmspace(movingZ);
				newPosition[2] += AxisPerMM::mmToSteps(4.0, AxisPerMM::AXIS_Z);
				steppers::setTarget(newPosition, interval / 2);
			}
			
			pauseState ++;
			break;
			
		case 4: //Wait for the X move to complete
			if ( ! steppers::isRunning()) {
				pauseState ++;
			}
			break;
			
		case 5: //Last command finished, record position and move Y to dislodge filament
			
			newPosition = steppers::getPosition();
			
			if (!freeMove) {
				lcd.writeFromPgmspace(movingY);
				newPosition[1] += AxisPerMM::mmToSteps(4.0, AxisPerMM::AXIS_Y);
				steppers::setTarget(newPosition, interval / 2);
			}
			
			pauseState ++;
			break;

		case 6: //Wait for the Y move up to complete
			
			if (!freeMove) lcd.writeFromPgmspace(movingY);
				
			if ( ! steppers::isRunning()) {
				pauseState ++;

				//We write this here to avoid tieing up the processor
				//in the next state
				lcd.clear();
				lcd.writeFromPgmspace(paused1);
				if (!freeMove) {	
					lcd.writeFloat(AxisPerMM::stepsToMM(pausedPosition[2], AxisPerMM::AXIS_Z), 3);
					lcd.writeString("):");
				}	else lcd.writeFromPgmspace(paused5);
				lcd.setCursor(0,1);
				lcd.writeFromPgmspace(paused2);
				lcd.setCursor(0,2);
				lcd.writeFromPgmspace(paused3);
				lcd.setCursor(0,3);
				lcd.writeFromPgmspace(paused4);
			}
			break;
	
		case 7: //Buzz if we're a Pause@ZPos
			if ( autoPause ) Motherboard::getBoard().buzz(4, 3, eeprom::getEeprom8(mbeeprom::BUZZER_REPEATS, 3));
			pauseState ++;
			break;
		
		case 8: //We're now paused
			break;


		case 9: //Leaving paused, wait for any steppers to finish
			if ( autoPause ) command::pauseAtZPos(0);
			lcd.clear();
			lcd.writeFromPgmspace(leavingPaused);
			if ( ! steppers::isRunning()) pauseState ++;
			break;

		case 10:	//Return to original position
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

		case 11: //Wait for return to original position
			lcd.writeFromPgmspace(leavingPaused);
			if ( ! steppers::isRunning()) {
				pauseState = 0;
     		interface::popScreen();
				command::pause(false);
				if ( ! autoPause ) interface::popScreen();
				steppers::switchToAcceleratedDriver();
			}
			break;
	}

	if ( lastDirectionButtonPressed ) {
		if (interface::isButtonPressed(lastDirectionButtonPressed))
			jog(lastDirectionButtonPressed);
		else {
			lastDirectionButtonPressed = (ButtonArray::ButtonName)0;
			steppers::abort();
		}

	}
}

void PauseMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	if ( pauseState == 8 ) {
		if ( button == ButtonArray::CANCEL )	pauseState ++;
		else	jog(button);
	}
}

void PauseAtZPosScreen::reset() {
	int32_t currentPause = command::getPauseAtZPos();
	if ( currentPause == 0 ) {
		Point position = steppers::getPosition();
		pauseAtZPos = AxisPerMM::stepsToMM(position[2], AxisPerMM::AXIS_Z);
	} else  pauseAtZPos = AxisPerMM::stepsToMM(currentPause, AxisPerMM::AXIS_Z);
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
		case ButtonArray::ZERO:
			break;
		case ButtonArray::OK:
			//Set the pause
			command::pauseAtZPos(AxisPerMM::mmToSteps(pauseAtZPos, AxisPerMM::AXIS_Z));
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
}

void AdvanceABPMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar abp1[] = "Ext Output Ctrl:";
	const static PROGMEM prog_uchar abp2[] = " Z=(mode) ";
	const static PROGMEM prog_uchar abp3[] = " 0=(Fan)  ";
	const static PROGMEM prog_uchar abp4[] = "OK=(abp)  ";
	const static PROGMEM prog_uchar on[]   = "ON    ";
	const static PROGMEM prog_uchar off[]  = "OFF   ";
  const static PROGMEM prog_uchar pushmode[] = "Moment";
  const static PROGMEM prog_uchar toggleMsg[]   = "Toggle";

	if (forceRedraw) {
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(abp1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(abp2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(abp3);
		
		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(abp4);
		
	}

	lcd.setCursor(10,1);
	if (!toggle) {
		lcd.writeFromPgmspace(pushmode);
		if (( abpForwarding ) && ( ! interface::isButtonPressed(ButtonArray::OK) )) {
			OutPacket responsePacket;
	
			abpForwarding = false;
			extruderControl(SLAVE_CMD_TOGGLE_ABP, EXTDR_CMD_SET8, responsePacket, (uint16_t)0);
		}
		
		if (( fanOn ) && ( ! interface::isButtonPressed(ButtonArray::ZERO) )) {
			OutPacket responsePacket;
	
			fanOn = false;
			extruderControl(SLAVE_CMD_TOGGLE_FAN, EXTDR_CMD_SET8, responsePacket, (uint16_t)0);
		}
	} else {
		lcd.writeFromPgmspace(toggleMsg);
	}
	
	lcd.setCursor(10,2);
	if (fanOn) lcd.writeFromPgmspace(on);
	else lcd.writeFromPgmspace(off);
		
	lcd.setCursor(10,3);
	if (abpForwarding) lcd.writeFromPgmspace(on);
	else lcd.writeFromPgmspace(off);	
	
	
	
}

void AdvanceABPMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	OutPacket responsePacket;

	switch (button) {
        case ButtonArray::OK:
					abpForwarding = !abpForwarding || !toggle;
					extruderControl(SLAVE_CMD_TOGGLE_ABP, EXTDR_CMD_SET8, responsePacket, (uint16_t)abpForwarding);
					break;
				case ButtonArray::ZERO:
					fanOn = !fanOn || !toggle;
					extruderControl(SLAVE_CMD_TOGGLE_FAN, EXTDR_CMD_SET8, responsePacket, (uint16_t)fanOn);
					break;
				case ButtonArray::CANCEL:
        	interface::popScreen();
					break;
				case ButtonArray::ZPLUS:
					toggle=!toggle;
				break;
        case ButtonArray::YMINUS:
        case ButtonArray::ZMINUS:
        case ButtonArray::YPLUS:
        
        case ButtonArray::XMINUS:
        	break;
	}
}

void CurrentPositionMode::reset() {
}

void CurrentPositionMode::update(LiquidCrystal& lcd, bool forceRedraw) {
	const static PROGMEM prog_uchar msg1[] = "X:";
	const static PROGMEM prog_uchar msg2[] = "Y:";
	const static PROGMEM prog_uchar msg3[] = "Z:";
	const static PROGMEM prog_uchar msg4[] = "A:";
	const static PROGMEM prog_uchar mm[] = "mm";

	if (forceRedraw) {
		lcd.clear();

		lcd.setCursor(0,0);
		lcd.writeFromPgmspace(msg1);

		lcd.setCursor(0,1);
		lcd.writeFromPgmspace(msg2);

		lcd.setCursor(0,2);
		lcd.writeFromPgmspace(msg3);

		lcd.setCursor(0,3);
		lcd.writeFromPgmspace(msg4);
	}

	Point position = steppers::getPosition();

	lcd.setCursor(3, 0);
	lcd.writeFloat(stepsToMM(position[0], AxisPerMM::AXIS_X), 3);
	lcd.writeFromPgmspace(mm);

	lcd.setCursor(3, 1);
	lcd.writeFloat(stepsToMM(position[1], AxisPerMM::AXIS_Y), 3);
	lcd.writeFromPgmspace(mm);

	lcd.setCursor(3, 2);
	lcd.writeFloat(stepsToMM(position[2], AxisPerMM::AXIS_Z), 3);
	lcd.writeFromPgmspace(mm);

	lcd.setCursor(3, 3);
	lcd.writeFloat(stepsToMM(position[3], AxisPerMM::AXIS_A), 3);
	lcd.writeFromPgmspace(mm);
}

void CurrentPositionMode::notifyButtonPressed(ButtonArray::ButtonName button) {
	interface::popScreen();
}


MainMenu::MainMenu() {
	itemCount = 9;
	reset();
}

void MainMenu::drawItem(uint8_t index, LiquidCrystal& lcd) {
	const static PROGMEM prog_uchar monitor[] 				= "Monitor";
	const static PROGMEM prog_uchar build[] 					= "Build from SD";
	const static PROGMEM prog_uchar jog[] 						= "Jog";
	const static PROGMEM prog_uchar preheat[] 				= "Preheat";
	const static PROGMEM prog_uchar extruder[] 				= "Extrude";
	const static PROGMEM prog_uchar currentPosition[] = "Position";
	const static PROGMEM prog_uchar steppersS[]				= "Steppers";
	const static PROGMEM prog_uchar advanceABP[]			= "Extruder CTRL";
	const static PROGMEM prog_uchar utilsmenu[] 			= "Utils Menu > X+";

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
		lcd.writeFromPgmspace(advanceABP);
		break;
	case 7:
		lcd.writeFromPgmspace(currentPosition);
		break;
	case 8:
		lcd.writeFromPgmspace(utilsmenu);
		break;
	}
}

void MainMenu::handleButtonPressed(ButtonArray::ButtonName button,uint8_t index, uint8_t subIndex) {
	switch (button) {
		case ButtonArray::XPLUS:
			interface::pushScreen(&utilsmenu);
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
			interface::pushScreen(&advanceABPMode);
			break;
		case 7:
			// Show advance Position
      interface::pushScreen(&currentPositionMode);
			break;
		case 8:
			interface::pushScreen(&utilsmenu);
			break;
		}
}

#endif
