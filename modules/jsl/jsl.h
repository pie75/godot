#ifndef JSL_H
#define JSL_H

#include "core/reference.h"
#include "core/math/vector3.h"
#include "core/math/quat.h"
#include "thirdparty/jsl/JoyShocklibrary.h"
//#include "core/os/input.h"

class JSL : public Object {
    GDCLASS(JSL, Object);
    int jslDeviceList[32] {0};
    int jslDeviceListSize;
    int jslDevice;

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
    int jsl_set_device(int p_device);
    MOTION_STATE jsl_get_joy_state(int p_device);
    IMU_STATE jsl_get_imu_state(int p_device);

    Quat jsl_get_joy_rotation(int p_device);
    Vector3 jsl_get_joy_gravity(int p_device);
    Vector3 jsl_get_joy_accel(int p_device);

    Vector3 jsl_get_imu_accel(int p_device);
    Vector3 jsl_get_imu_gyro(int p_device);

    float jsl_get_imu_value(int p_device, int p_imu);

    JSL();
};
#endif //JSL_H
