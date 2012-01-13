/*
 * Copyright 2010 by Adam Mayer <adam@makerbot.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */


#ifndef EEPROMMAP_HH_
#define EEPROMMAP_HH_

#include <stdint.h>

namespace eeprom {

const static uint16_t EEPROM_SIZE				= 0x0200;

/// Version, low byte: 1 byte
const static uint16_t VERSION_LOW				      = 0x0000; //1-byte
/// Version, high byte: 1 byte
const static uint16_t VERSION_HIGH				    = 0x0001; //1-byte

/// Axis inversion flags: 1 byte.
/// Axis N (where X=0, Y=1, etc.) is inverted if the Nth bit is set.
/// Bit 7 is used for HoldZ OFF: 1 = off, 0 = on
const static uint16_t AXIS_INVERSION			    = 0x0002; //1-byte

/// Endstop inversion flags: 1 byte.
/// The endstops for axis N (where X=0, Y=1, etc.) are considered
/// to be logically inverted if the Nth bit is set.
/// Bit 7 is set to indicate endstops are present; it is zero to indicate
/// that endstops are not present.
/// Ordinary endstops (H21LOB et. al.) are inverted.
const static uint16_t ENDSTOP_INVERSION			  = 0x0003; //1-byte

/// Name of this machine: 32 bytes.
const static uint16_t MACHINE_NAME				    = 0x0020; //32-bytes

/// Default locations for the axis: 5 x 32 bit = 20 bytes
const static uint16_t AXIS_HOME_POSITIONS		  = 0x0060; //20-bytes

// Estop configuration byte: 1 byte.
const static uint16_t ESTOP_CONFIGURATION     = 0x0074; //1-byte

enum {
	ESTOP_CONF_NONE = 0x0,
	ESTOP_CONF_ACTIVE_HIGH = 0x1,
	ESTOP_CONF_ACTIVE_LOW = 0x2
};

const static uint16_t EMPTY_MEM_LOC0			    = 0x0075; //12-bytes

const static uint16_t TOOL0_TEMP      		    = 0x0080; //1-byte
const static uint16_t TOOL1_TEMP      		    = 0x0081; //1-byte
const static uint16_t PLATFORM_TEMP   		    = 0x0082; //1-byte
const static uint16_t EXTRUDE_DURATION		    = 0x0083; //1-byte
const static uint16_t EXTRUDE_RPM     		    = 0x0084; //1-byte
const static uint16_t MOOD_LIGHT_SCRIPT		    = 0x0085; //1-byte
const static uint16_t MOOD_LIGHT_CUSTOM_RED	  = 0x0086; //1-byte
const static uint16_t MOOD_LIGHT_CUSTOM_GREEN	= 0x0087; //1-byte
const static uint16_t MOOD_LIGHT_CUSTOM_BLUE	= 0x0088; //1-byte
//Bit 1 is Model mode or user view mode (user view mode = bit set)
//Bit 2-4 are the jog mode distance 0 = short, 1 = long, 2 = cont
const static uint16_t JOG_MODE_SETTINGS				= 0x0089; //1-byte
const static uint16_t AXIS_HOME_DIR           = 0x008A; //1-byte
const static uint16_t AXIS_HOME_MINMAX			  = 0x008B; //1-byte
const static uint16_t DISPLAY_SIZE			      = 0x008C; //1-byte
const static uint16_t ZAXIS_MM_PER_TURN_W     = 0x008D; //2-bytes
const static uint16_t ZAXIS_MM_PER_TURN_P     = 0x008F; //2-bytes
const static uint16_t EMPTY_MEM_LOC2			    = 0x0091; //1-byte

/// Reset all data in the EEPROM to a default.
void setDefaults();

} // namespace eeprom

#endif // EEPROMMAP_HH_
