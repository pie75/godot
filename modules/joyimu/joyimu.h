#ifndef JOYIMU_H
#define JOYIMU_H

#include "core/reference.h"
#include "core/os/input_event.h"
#include "thirdparty/jsl/JoyShockLibrary.h"

class InputEventJoypadIMU : public InputEvent {
	GDCLASS(InputEventJoypadIMU, InputEvent);
	int JSLdevice;
	MOTION_STATE joy_state;
	IMU_STATE imu_state;

	Quat joy_rotation;
	Vector3 joy_gravity;
	Vector3 joy_accel;
	Vector3 accel_value;
	Vector3 gyro_value;

protected:
	static void _bind_methods();

public:
	void set_joy_state();
	Quat get_joy_rotation() const;
	Vector3 get_joy_gravity() const;
	Vector3 get_joy_accel() const;

	void set_imu_state();
	Vector3 get_accel_state() const;
	Vector3 get_gyro_state() const;

	float get_imu_value(int p_imu) const;

	float get_accelX_value() const;
	float get_accelY_value() const;
	float get_accelZ_value() const;
	float get_gyroX_value() const;
	float get_gyroY_value() const;
	float get_gyroZ_value() const;

//	virtual String as_text() const;

	InputEventJoypadIMU();
};
#endif //JOYIMU_H
