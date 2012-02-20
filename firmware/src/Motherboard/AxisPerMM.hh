#ifndef AxisPerMM_HH_
#define AxisPerMM_HH_

#include "Types.hh"

namespace AxisPerMM {
	
	enum Axis {
		AXIS_X = 0,
		AXIS_Y,
		AXIS_Z,
		AXIS_A,
		AXIS_B
	};
  

	int32_t mmToSteps(float mm, enum Axis axis);
	float stepsToMM(int32_t steps, enum Axis axis);
	int64_t checkAndGetEepromDefault(const uint16_t location, const int64_t default_value);
	int64_t getAxis(Axis axis);
	int64_t getAxis(uint8_t axis);
	void setAxis(enum Axis axis,int64_t value);
	void setAxis(uint8_t axis,int64_t value);
	void populateAxisArray();
	void invalidate();
	void saveValues();

}

#endif