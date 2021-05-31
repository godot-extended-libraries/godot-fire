/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR pose GDNative object, this exposes specific locations we track

#include "core/object/object.h"
#include "core/variant/variant.h"
#include "servers/xr_server.h"

#include "OpenXRPose.h"



void OpenXRPose::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_physics_process"), &OpenXRPose::_physics_process);

	ClassDB::bind_method(D_METHOD("get_invisible_if_inactive"), &OpenXRPose::get_invisible_if_inactive);
	ClassDB::bind_method(D_METHOD("set_invisible_if_inactive"), &OpenXRPose::set_invisible_if_inactive);
	
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "invisible_if_inactive"), "set_invisible_if_inactive", "get_invisible_if_inactive");

	// For now these are hard coded based on our actions
	// As our actions JSON is parsed after initialisation we can't really present the dropdown (yet)
	// For now this will do
	// Note that SkeletonBase is a special value for our hand skeleton support.
	ClassDB::bind_method(D_METHOD("get_action"), &OpenXRPose::get_action);
	ClassDB::bind_method(D_METHOD("set_action"), &OpenXRPose::set_action);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "action", PROPERTY_HINT_ENUM, "SkeletonBase,godot/aim_pose,godot/grip_pose"), "set_action", "get_action");

	// For now this is hard coded, these are fixed entries based on the OpenXR spec
	ClassDB::bind_method(D_METHOD("get_path"), &OpenXRPose::get_path);
	ClassDB::bind_method(D_METHOD("set_path"), &OpenXRPose::set_path);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "controlller_path", PROPERTY_HINT_ENUM, "/user/hand/left,/user/hand/right,/user/treadmill"), "set_path", "get_path");
	ClassDB::bind_method(D_METHOD("is_active"), &OpenXRPose::is_active);
}

OpenXRPose::OpenXRPose() {
	invisible_if_inactive = true;
	action = String("SkeletonBase");
	_action = NULL;
	path = String("/user/hand/left");
	_path = XR_NULL_PATH;
	openxr_api = OpenXRApi::openxr_get_api();
}

OpenXRPose::~OpenXRPose() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRPose::_init() {
	// nothing to do here
}

bool OpenXRPose::check_action_and_path() {
	// not yet ready?
	if (!openxr_api->has_action_sets()) {
		return false;
	}

	// don't keep trying this over and over and over again if we fail
	if (fail_cache) {
		return false;
	}

	if (_action == NULL) {
		Vector<String> split = action.split("/");
		if (split.size() != 2) {
			print_line(vformat("Incorrect action string %s", action));
			fail_cache = true;
			return false;
		}

		ActionSet *aset = openxr_api->get_action_set(split[0]);
		if (aset == NULL) {
			print_line(vformat("Couldn't find action set %s", split[0]));
			fail_cache = true;
			return false;
		}

		_action = aset->get_action(split[1]);
		if (_action == NULL) {
			print_line(vformat("Couldn't find action %s", split[1]));
			fail_cache = true;
			return false;
		}
	}

	if (_path == XR_NULL_PATH) {
		XrResult res = xrStringToPath(openxr_api->get_instance(), path.utf8().get_data(), &_path);
		if (!openxr_api->xr_result(res, "Couldn't obtain path {0}", path)) {
			fail_cache = true;
			return false;
		}
	}

	return true;
}

void OpenXRPose::_physics_process(float delta) {
	if (openxr_api == NULL) {
		return;
	} else if (!openxr_api->is_initialised()) {
		return;
	}

	if (invisible_if_inactive) {
		set_visible(is_active());
	}

	XRServer *server = XRServer::get_singleton();
	const float ws = server->get_world_scale();
	Transform reference_frame = server->get_reference_frame();

	if (action == "SkeletonBase") {
		if (path == "/user/hand/left") {
			const HandTracker *hand_tracker = openxr_api->get_hand_tracker(0);
			const XrPosef &pose = hand_tracker->joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
			set_transform(reference_frame * openxr_api->transform_from_pose(pose, ws));
		} else if (path == "/user/hand/right") {
			const HandTracker *hand_tracker = openxr_api->get_hand_tracker(1);
			const XrPosef &pose = hand_tracker->joint_locations[XR_HAND_JOINT_PALM_EXT].pose;
			set_transform(reference_frame * openxr_api->transform_from_pose(pose, ws));
		}
	} else if (check_action_and_path()) {
		set_transform(reference_frame * _action->get_as_pose(_path, ws));
	}
}

bool OpenXRPose::is_active() {
	if (openxr_api == NULL) {
		return false;
	} else if (!openxr_api->is_initialised()) {
		return false;
	}

	if (action == "SkeletonBase") {
		if (path == "/user/hand/left") {
			const HandTracker *hand_tracker = openxr_api->get_hand_tracker(0);

			return (hand_tracker->is_initialised && hand_tracker->locations.isActive);
		} else if (path == "/user/hand/right") {
			const HandTracker *hand_tracker = openxr_api->get_hand_tracker(1);

			return (hand_tracker->is_initialised && hand_tracker->locations.isActive);
		}
	} else if (check_action_and_path()) {
		return _action->is_pose_active(_path);
	}

	return false;
}

bool OpenXRPose::get_invisible_if_inactive() const {
	return invisible_if_inactive;
}

void OpenXRPose::set_invisible_if_inactive(bool hide) {
	invisible_if_inactive = hide;
}

String OpenXRPose::get_action() const {
	return action;
}

void OpenXRPose::set_action(const String p_action) {
	action = p_action;
	_action = NULL;
	fail_cache = false;
}

String OpenXRPose::get_path() const {
	return path;
}

void OpenXRPose::set_path(const String p_path) {
	path = p_path;
	_path = XR_NULL_PATH;
	fail_cache = false;
}
