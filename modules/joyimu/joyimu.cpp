#include "joyimu.h"

#include "core/input_map.h"
//#include "GamepadMotion.hpp"

void InputEventJoypadIMU::set_joy_state() {
	int p_device = get_device();
	MOTION_STATE p_state = JslGetMotionState(p_device);
	joy_rotation.set_axis_angle(Vector3(p_state.quatX, p_state.quatY, p_state.quatZ), p_state.quatW);
	joy_gravity = Vector3(p_state.gravX, p_state.gravY, p_state.gravZ);
	joy_accel = Vector3(p_state.accelX, p_state.accelY, p_state.accelZ);
}

Quat InputEventJoypadIMU::get_joy_rotation() const {
	return joy_rotation;
}

Vector3 InputEventJoypadIMU::get_joy_gravity() const {
	return joy_gravity;
}

Vector3 InputEventJoypadIMU::get_joy_accel() const {
	return joy_accel;
}

void InputEventJoypadIMU::set_imu_state() {
	int p_device = get_device();
	IMU_STATE p_state = JslGetIMUState(p_device);
	imu_state.accelX = p_state.accelX;
	imu_state.accelY = p_state.accelY;
	imu_state.accelZ = p_state.accelZ;
	imu_state.gyroX = p_state.gyroX;
	imu_state.gyroY = p_state.gyroY;
	imu_state.gyroZ = p_state.gyroZ;
	accel_value = Vector3(imu_state.accelX, imu_state.accelY, imu_state.accelZ);
	gyro_value = Vector3(imu_state.gyroX, imu_state.gyroY, imu_state.gyroZ);
}

Vector3 InputEventJoypadIMU::get_accel_state() const {
	return accel_value;
}

Vector3 InputEventJoypadIMU::get_gyro_state() const {
	return gyro_value;
}

float InputEventJoypadIMU::get_imu_value(int p_imu) const {
	switch (p_imu) {
		case 0:
			return imu_state.accelX;
			break;
		case 1:
			return imu_state.accelY;
			break;
		case 2:
			return imu_state.accelZ;
			break;
		case 3:
			return imu_state.gyroX;
			break;
		case 4:
			return imu_state.gyroY;
			break;
		case 5:
			return imu_state.gyroZ;
			break;
		default:
			return 0;
	}
}

float InputEventJoypadIMU::get_accelX_value() const {
	return accel_value.x;
}

float InputEventJoypadIMU::get_accelY_value() const {
	return accel_value.y;
}

float InputEventJoypadIMU::get_accelZ_value() const {
	return accel_value.z;
}

float InputEventJoypadIMU::get_gyroX_value() const {
	return gyro_value.x;
}

float InputEventJoypadIMU::get_gyroY_value() const {
	return gyro_value.y;
}

float InputEventJoypadIMU::get_gyroZ_value() const {
	return gyro_value.z;
}

//String InputEventJoypadIMU::as_text() const {};

void InputEventJoypadIMU::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_joy_motion"), &InputEventJoypadIMU::set_joy_state);
	ClassDB::bind_method(D_METHOD("get_joy_rotation"), &InputEventJoypadIMU::get_joy_rotation);
	ClassDB::bind_method(D_METHOD("get_joy_gravity"), &InputEventJoypadIMU::get_joy_gravity);
	ClassDB::bind_method(D_METHOD("get_joy_acceleration"), &InputEventJoypadIMU::get_joy_accel);
	ClassDB::bind_method(D_METHOD("set_joy_imu_state"), &InputEventJoypadIMU::set_imu_state);
	ClassDB::bind_method(D_METHOD("set_joy_accelerometer"), &InputEventJoypadIMU::set_imu_state);
	ClassDB::bind_method(D_METHOD("set_joy_gyroscope"), &InputEventJoypadIMU::set_imu_state);
	ClassDB::bind_method(D_METHOD("get_joy_imu_value", "sensor"), &InputEventJoypadIMU::get_imu_value);
	ClassDB::bind_method(D_METHOD("get_joy_accelerometer"), &InputEventJoypadIMU::get_accel_state);
	ClassDB::bind_method(D_METHOD("get_joy_gyroscope"), &InputEventJoypadIMU::get_gyro_state);
	ClassDB::bind_method(D_METHOD("get_joy_accelX"), &InputEventJoypadIMU::get_accelX_value);
	ClassDB::bind_method(D_METHOD("get_joy_gyroX"), &InputEventJoypadIMU::get_gyroX_value);
	ClassDB::bind_method(D_METHOD("get_joy_accelY"), &InputEventJoypadIMU::get_accelY_value);
	ClassDB::bind_method(D_METHOD("get_joy_gyroY"), &InputEventJoypadIMU::get_gyroY_value);
	ClassDB::bind_method(D_METHOD("get_joy_accelZ"), &InputEventJoypadIMU::get_accelZ_value);
	ClassDB::bind_method(D_METHOD("get_joy_gyroZ"), &InputEventJoypadIMU::get_gyroZ_value);

	ADD_PROPERTY(PropertyInfo(Variant::QUAT, "rotation"), "set_joy_motion", "get_joy_rotation");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gravity"), "set_joy_motion", "get_joy_gravity");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "acceleration"), "set_joy_motion", "get_joy_acceleration");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "sensor_value"), "set_joy_imu_state", "get_joy_imu_value");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "accelerometer"), "set_joy_imu_state", "get_joy_accelerometer");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "gyroscope"), "set_joy_imu_state", "get_joy_gyroscope");
}

InputEventJoypadIMU::InputEventJoypadIMU() {
	JSLdevice = 0;

	joy_state.quatW = 0;
	joy_state.quatX = 0;
	joy_state.quatY = 0;
	joy_state.quatZ = 0;
	joy_state.accelX = 0;
	joy_state.accelY = 0;
	joy_state.accelZ = 0;
	joy_state.gravX = 0;
	joy_state.gravY = 0;
	joy_state.gravZ = 0;

	imu_state.accelX = 0;
	imu_state.accelY = 0;
	imu_state.accelZ = 0;
	imu_state.gyroX = 0;
	imu_state.gyroY = 0;
	imu_state.gyroZ = 0;

	joy_rotation.set_axis_angle(Vector3(0, 0, 0), 1);
	joy_gravity.set_all(0);
	joy_accel.set_all(0);

	accel_value.set_all(0);
	gyro_value.set_all(0);
}
