#include "jsl.h"

//#include "core/input_map.h"

int JSL::jsl_set_device(int p_device) {
    jslDeviceListSize = JslConnectDevices();
    JslGetConnectedDeviceHandles(jslDeviceList, jslDeviceListSize); 
    jslDevice = jslDeviceList[p_device];
    return jslDevice;
}

MOTION_STATE JSL::jsl_get_joy_state(int p_device){
    jsl_set_device(p_device);
    joy_state = JslGetMotionState(jslDevice);
    return joy_state;
};

IMU_STATE JSL::jsl_get_imu_state(int p_device){
    jsl_set_device(p_device);
    imu_state = JslGetIMUState(jslDevice);
    return imu_state;
};

///////////////////////////////////////////////////////

Quat JSL::jsl_get_joy_rotation(int p_device){
    MOTION_STATE p_state = jsl_get_joy_state(p_device);
    joy_rotation.set(p_state.quatX, p_state.quatY, p_state.quatZ, p_state.quatW);
    return joy_rotation;
};

Vector3 JSL::jsl_get_joy_gravity(int p_device){
    MOTION_STATE p_state = jsl_get_joy_state(p_device);
    joy_gravity = Vector3(p_state.gravX, p_state.gravY, p_state.gravZ);
    return joy_gravity;
};

Vector3 JSL::jsl_get_joy_accel(int p_device){
    MOTION_STATE p_state = jsl_get_joy_state(p_device);
    joy_accel = Vector3(p_state.accelX, p_state.accelY, p_state.accelZ);
    return joy_accel;
};

///////////////////////////////////////////////////////

Vector3 JSL::jsl_get_imu_accel(int p_device){
    IMU_STATE p_state = jsl_get_imu_state(p_device);
    accel_value = Vector3(p_state.accelX, p_state.accelY, p_state.accelZ);
    return accel_value;
};

Vector3 JSL::jsl_get_imu_gyro(int p_device){
    IMU_STATE p_state = jsl_get_imu_state(p_device);
    gyro_value = Vector3(p_state.gyroX, p_state.gyroY, p_state.gyroZ);
    return gyro_value;
};

///////////////////////////////////////////////////////

float JSL::jsl_get_imu_value(int p_device, int p_imu){
    jsl_set_device(p_device);
    switch (p_imu){
        case 0:
            return JslGetAccelX(jslDevice);
        case 1:
            return JslGetAccelY(jslDevice);
        case 2:
            return JslGetAccelZ(jslDevice);
        case 3:
            return JslGetGyroX(jslDevice);
        case 4:
            return JslGetGyroY(jslDevice);
        case 5:
            return JslGetGyroZ(jslDevice);
        default:
            return 0;
    }
};

void JSL::_bind_methods() {
    ClassDB::bind_method(D_METHOD("jsl_get_joy_rotation", "deviceID"), &JSL::jsl_get_joy_rotation);
    ClassDB::bind_method(D_METHOD("jsl_get_joy_gravity", "deviceID"), &JSL::jsl_get_joy_gravity);
    ClassDB::bind_method(D_METHOD("jsl_get_joy_accel", "deviceID"), &JSL::jsl_get_joy_accel);

    ClassDB::bind_method(D_METHOD("jsl_get_imu_accel", "deviceID"), &JSL::jsl_get_imu_accel);
    ClassDB::bind_method(D_METHOD("jsl_get_imu_gyro", "deviceID"), &JSL::jsl_get_imu_gyro);

    ClassDB::bind_method(D_METHOD("jsl_get_imu_value", "deviceID", "imuID"), &JSL::jsl_get_imu_value);
};

JSL::JSL() {
    jslDevice = 0;
    for (int i = 0; i <= 32; i++){
        jslDeviceList[i] = 0;
    }
    jslDeviceListSize = 0;
    jslDevice = 0;

    joy_state.quatX = 0;
    joy_state.quatY = 0;
    joy_state.quatZ = 0;
    joy_state.quatW = 0;
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

    joy_rotation.set(0,0,0,1);
    joy_gravity.set_all(0);
    joy_accel.set_all(0);
    accel_value.set_all(0);
    gyro_value.set_all(0);
};
