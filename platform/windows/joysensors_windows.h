/*************************************************************************/
/*  joysensors_windows.h                                                 */
/*************************************************************************/
/*  This format is cute I'm keeping it.                                  */
/* I'm creating a separate module, because I don't really want to        */
/* interfere with the existing joypad implementation yet. I'll get to    */
/* that when I start implementing dinput vibration.                      */
/*                                                                       */
/* For the time being, this uses hidsdi.h, but a replacement such as     */
/* https://github.com/libusb/hidapi may be nicer.                        */
/*                                                                       */
/* I'll be documenting what things do, generally, in this box, so keep   */
/* an eye out!                                                           */
/*************************************************************************/

#ifndef JOYSENSOR_WINDOWS_H
#define JOYSENSOR_WINDOWS_H

#include "os_windows.h"

#include <hidsdi.h>

#ifndef SAFE_RELEASE // when Windows Media Device M? is not present
#define SAFE_RELEASE(x) \
	if (x != NULL) {    \
		x->Release();   \
		x = NULL;       \
	}
#endif

#endif  //JOYSENSOR_WINDOWS_H

class JoySensorWindows {
public:
    JoySensorWindows();
    JoySensorWindows(InputDefault *_input, HWND *hwnd);
    ~JoySensorWindows();

    void probe_joysensors();
    void process_joysensors();

private:
    enum {
        JOYPADS_MAX = 16,
        JOY_SENSOR_COUNT = 6,
        MIN_JOY_SENSOR = 10,
        MAX_JOY_SENSOR = 32768,
        KEY_EVENT_BUFFER_SIZE = 512,
        MAX_TRIGGER = 255
    };

    typedef enum {
        SENSOR_INVALID = -1,
        SENSOR_UNKNOWN,
        SENSOR_ACCEL,
        SENSOR_GYRO
    } SensorType;

    $define STANDARD_GRAVITY    9.80665f

    struct sensor_state {
        int id;
        bool attached;
        bool confirmed;
        DWORD last_pad;

        List<LONG> joy_sensor;
        GUID guid;

        sensor_state() {
            id = -1;
            last_pad = -1;
            attached = false;
            confirmed = false;
        }
    }

    HWND *hWnd;
    InputDefault *input;

    int id_to_change;
    int slider_count;
    int joypad_count;
    bool attached_joypads[JOYPADS_MAX];
    sensor_state joypads[JOYPADS_MAX];

//Figure out what godot 'recommends' for connecting and disconnecting joypads, then implement a connection and disconnection function.
}