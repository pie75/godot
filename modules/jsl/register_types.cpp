#include "register_types.h"

#include "core/class_db.h"
//#include "core/project_settings.h"
#include "jsl.h"

void register_jsl_types() {
	ClassDB::register_class<JSL>();
}

void unregister_jsl_types() {
	// Nothing to do here in this example.
}