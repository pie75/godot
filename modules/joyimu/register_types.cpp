#include "register_types.h"

#include "core/class_db.h"
#include "core/project_settings.h"
#include "joyimu.h"

void register_joyimu_types() {
	ClassDB::register_class<InputEventJoypadMotion>();
}

void unregister_joyimu_types() {
	// Nothing to do here in this example.
}
