#include "AxisPerMM.hh"
#include <stdlib.h>
#include "SharedEepromMap.hh"
#include "Eeprom.hh"
#include <avr/interrupt.h>

namespace AxisPerMM {


int64_t axisStepsPerMM[5];
bool isValid;

//Convert mm's to steps for the given axis
//Accurate to 1/1000 mm

int32_t mmToSteps(float mm, enum Axis axis) {
	//Multiply mm by 1000 to avoid floating point errors
	int64_t intmm = (int64_t)(mm * 1000.0);

	//Calculate the number of steps
	int64_t ret = intmm * axisStepsPerMM[axis];

	//Divide the number of steps by the fixed precision and
	//mm 1000;
	for (uint8_t i=0 ; i < STEPS_PER_MM_PRECISION; i ++ )
		ret /= 10;
	ret /= 1000;
	
	return (int32_t)ret;
}

//Convert steps to mm's
//As accurate as floating point is

float stepsToMM(int32_t steps, enum Axis axis) {
	//Convert axisStepsPerMM to a float	
	float aspmf = (float)axisStepsPerMM[axis];
	for (uint8_t i=0 ; i < STEPS_PER_MM_PRECISION; i ++ )
		aspmf /= 10.0;
	return (float)steps / aspmf;
}

int64_t  checkAndGetEepromDefault(const uint16_t location, const int64_t default_value) {
        int64_t value = eeprom::getEepromInt64(location, default_value);

        if (( value <= STEPS_PER_MM_LOWER_LIMIT ) || ( value >= STEPS_PER_MM_UPPER_LIMIT )) {
                eeprom::putEepromInt64(location, default_value);

                //Just to be on the safe side
                value = eeprom::getEepromInt64(location, default_value);
        }

        return value;
}

void saveValues() {
	if (isValid) {
		cli();
		eeprom::putEepromInt64(mbeeprom::STEPS_PER_MM_X, axisStepsPerMM[AXIS_X]);
		eeprom::putEepromInt64(mbeeprom::STEPS_PER_MM_Y, axisStepsPerMM[AXIS_Y]);
		eeprom::putEepromInt64(mbeeprom::STEPS_PER_MM_Z, axisStepsPerMM[AXIS_Z]);
		eeprom::putEepromInt64(mbeeprom::STEPS_PER_MM_A, axisStepsPerMM[AXIS_A]);
		eeprom::putEepromInt64(mbeeprom::STEPS_PER_MM_B, axisStepsPerMM[AXIS_B]);
		sei();
	}
}

void invalidate() {
	isValid=false;
}

void populateAxisArray() {
	if (!isValid) {
		cli();
  	axisStepsPerMM[AXIS_X] = checkAndGetEepromDefault(mbeeprom::STEPS_PER_MM_X, STEPS_PER_MM_X_DEFAULT);
  	axisStepsPerMM[AXIS_Y] = checkAndGetEepromDefault(mbeeprom::STEPS_PER_MM_Y, STEPS_PER_MM_Y_DEFAULT);
  	axisStepsPerMM[AXIS_Z] = checkAndGetEepromDefault(mbeeprom::STEPS_PER_MM_Z, STEPS_PER_MM_Z_DEFAULT);
  	axisStepsPerMM[AXIS_A] = checkAndGetEepromDefault(mbeeprom::STEPS_PER_MM_A, STEPS_PER_MM_A_DEFAULT);
  	axisStepsPerMM[AXIS_B] = checkAndGetEepromDefault(mbeeprom::STEPS_PER_MM_B, STEPS_PER_MM_B_DEFAULT);
  	sei();
  	isValid=true;
	}
}

int64_t getAxis(enum Axis axis) {
	return axisStepsPerMM[axis];
}

int64_t getAxis(uint8_t axis) {
	return axisStepsPerMM[axis];
}

void setAxis(enum Axis axis,int64_t value) {
	axisStepsPerMM[axis]=value;
}

void setAxis(uint8_t axis,int64_t value) {
	axisStepsPerMM[axis]=value;
}
	
}