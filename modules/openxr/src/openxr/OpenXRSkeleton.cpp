/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through skeleton (bones)

#include "core/object/object.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"
#include "servers/xr_server.h"

#include "OpenXRSkeleton.h"



void OpenXRSkeleton::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_ready"), &OpenXRSkeleton::_ready);
	ClassDB::bind_method(D_METHOD("_physics_process"), &OpenXRSkeleton::_physics_process);

	ClassDB::bind_method(D_METHOD("get_hand"), &OpenXRSkeleton::get_hand);
	ClassDB::bind_method(D_METHOD("set_hand"), &OpenXRSkeleton::set_hand);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "hand", PROPERTY_HINT_ENUM, "Left,Right"), "set_hand", "get_hand");
}

OpenXRSkeleton::OpenXRSkeleton() {
	hand = 0;
	openxr_api = OpenXRApi::openxr_get_api();

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		bones[i] = -1;
	}
}

OpenXRSkeleton::~OpenXRSkeleton() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRSkeleton::_init() {
	// nothing to do here
}

void OpenXRSkeleton::_ready() {
	const char *bone_names[XR_HAND_JOINT_COUNT_EXT] = {
		"Palm",
		"Wrist",
		"Thumb_Metacarpal",
		"Thumb_Proximal",
		"Thumb_Distal",
		"Thumb_Tip",
		"Index_Metacarpal",
		"Index_Proximal",
		"Index_Intermediate",
		"Index_Distal",
		"Index_Tip",
		"Middle_Metacarpal",
		"Middle_Proximal",
		"Middle_Intermediate",
		"Middle_Distal",
		"Middle_Tip",
		"Ring_Metacarpal",
		"Ring_Proximal",
		"Ring_Intermediate",
		"Ring_Distal",
		"Ring_Tip",
		"Little_Metacarpal",
		"Little_Proximal",
		"Little_Intermediate",
		"Little_Distal",
		"Little_Tip",
	};

	// We cast to spatials which should allow us to use any subclass of that.
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		char bone_name[250];
		if (hand == 0) {
			sprintf(bone_name, "%s_L", bone_names[i]);
		} else {
			sprintf(bone_name, "%s_R", bone_names[i]);
		}

		bones[i] = find_bone(bone_name);
		if (bones[i] == -1) {
			print_line(vformat("Couldn't obtain bone for %s", bone_name));
		}
	}
}

void OpenXRSkeleton::_physics_process(float delta) {
	if (openxr_api == NULL) {
		return;
	} else if (!openxr_api->is_initialised()) {
		return;
	}

	// we cache our transforms so we can quickly calculate local transforms
	Transform transforms[XR_HAND_JOINT_COUNT_EXT];
	Transform inv_transforms[XR_HAND_JOINT_COUNT_EXT];

	const HandTracker *hand_tracker = openxr_api->get_hand_tracker(hand);
	const float ws = XRServer::get_singleton()->get_world_scale();

	if (hand_tracker->is_initialised && hand_tracker->locations.isActive) {
		// get our transforms
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			const XrPosef &pose = hand_tracker->joint_locations[i].pose;
			transforms[i] = openxr_api->transform_from_pose(pose, ws);
			inv_transforms[i] = transforms[i].inverse();
		}

		// now update our skeleton
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			if (bones[i] != -1) {
				int bone = bones[i];
				int parent = get_bone_parent(bone);

				Transform t = transforms[i];

				// get local translation, parent should already be processed
				if (parent == -1) {
					// use our palm location here, that is what we are tracking
					t = inv_transforms[XR_HAND_JOINT_PALM_EXT] * t;
				} else {
					int found = false;
					for (int b = 0; b < XR_HAND_JOINT_COUNT_EXT && !found; b++) {
						if (bones[b] == parent) {
							t = inv_transforms[b] * t;
							found = true;
						}
					}
				}

				// get difference with rest
				Transform rest = get_bone_rest(bones[i]);
				t = rest.inverse() * t;

				// and set our pose
				set_bone_pose(bones[i], t);
			}
		}

		// show it
		set_visible(true);
	} else {
		// hide it
		set_visible(false);
	}
}

int OpenXRSkeleton::get_hand() const {
	return hand;
}

void OpenXRSkeleton::set_hand(int p_hand) {
	hand = p_hand == 1 ? 1 : 0;
}
