
#include "register_types.h"
#include "core/object/class_db.h"

#include "servers/xr/xr_interface.h"

#include "openxr/OpenXRConfig.h"
#include "openxr/OpenXRHand.h"
#include "openxr/OpenXRPose.h"
#include "openxr/OpenXRSkeleton.h"
#include "servers/xr_server.h"

#include "src/XRInterface.h"

Ref<OpenXRInterface> new_interface = nullptr;

void register_openxr_types() {
	ClassDB::register_class<OpenXRInterface>();
	new_interface.instance();
	XRServer::get_singleton()->add_interface(new_interface);
	if (Engine::get_singleton()->get_singleton()->is_editor_hint()) {
		ClassDB::register_class<OpenXRConfig>();
	}
	ClassDB::register_class<OpenXRHand>();
	ClassDB::register_class<OpenXRPose>();
	ClassDB::register_class<OpenXRSkeleton>();
}

void unregister_openxr_types() {
	if (new_interface.is_null()) {
		return;
	}
	XRServer::get_singleton()->remove_interface(new_interface);
	new_interface.unref();
}
