////////////////////////////////////////////////////////////////////////////////////////////////
// Wrapper class for interacting with OpenXR
//
// Initial implementation thanks Christoph Haag

#ifndef OPENXR_API_H
#define OPENXR_API_H

#ifdef WIN32
#include <windows.h>
#endif
#include "core/typedefs.h"
#include "core/error/error_macros.h"
#include "core/math/transform.h"
#include "core/math/vector2.h"
#include "core/string/print_string.h"
#include "core/variant/variant.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "xrmath.h"
#include "modules/openxr/openxr_loader_windows/1.0.12/include/openxr/openxr.h"

#include <vulkan/vulkan.h>

#define XR_MND_BALL_ON_STICK_EXTENSION_NAME "XR_MNDX_ball_on_a_stick_controller"

#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#else
#define XR_USE_PLATFORM_XLIB
#endif
#define XR_USE_GRAPHICS_API_VULKAN

#include "modules/openxr/openxr_loader_windows/1.0.12/include/openxr/openxr_platform.h"

// forward declare this
class OpenXRApi;
#include "modules/openxr/src/openxr/actions/action.h"
#include "modules/openxr/src/openxr/actions/actionset.h"

#include "drivers/vulkan/rendering_device_vulkan.h"

// TODO move hand tracker logic into it's own source files, I'll do a separate PR for that in due time.
class HandTracker {
public:
	bool is_initialised = false;

	XrHandTrackerEXT hand_tracker;
	XrHandJointLocationEXT joint_locations[XR_HAND_JOINT_COUNT_EXT];
	XrHandJointVelocityEXT joint_velocities[XR_HAND_JOINT_COUNT_EXT];

	XrHandJointVelocitiesEXT velocities;
	XrHandJointLocationsEXT locations;
};

#define USER_INPUT_MAX 2

class OpenXRApi {
	friend class Action;
	friend class ActionSet;
	friend class RenderingDeviceVulkan;

private:
	OpenXRApi *xr_api = nullptr;

public:
	// These are hardcoded and meant for our backwards compatibility layer
	// If not configured in our action sets they will be defunct

	struct InputMap {
		const char *name;
		XrPath toplevel_path;
		int32_t godot_controller;
		XrPath active_profile; // note, this can be a profile added in the OpenXR runtime unknown to our default mappings
	};

	InputMap inputmaps[USER_INPUT_MAX] = {
		{ "/user/hand/left", XR_NULL_PATH, -1, XR_NULL_PATH },
		{ "/user/hand/right", XR_NULL_PATH, -1, XR_NULL_PATH },
		// gamepad is already supported in Godots own joystick handling, head we're using directly
		// { "/user/foot/left", XR_NULL_PATH, -1, XR_NULL_PATH },
		// { "/user/foot/right", XR_NULL_PATH, -1, XR_NULL_PATH },
		// { "/user/treadmill", XR_NULL_PATH, -1, XR_NULL_PATH },
	};

	// Default actions we support so we can mimic our old ARVRController handling
	enum DefaultActions {
		// Poses
		ACTION_AIM_POSE, // we are not using this ourselves
		ACTION_GRIP_POSE,

		// Analog
		ACTION_FRONT_TRIGGER, // front trigger (Axis 2)
		ACTION_SIDE_TRIGGER, // side trigger/grip (Axis 4)
		ACTION_JOYSTICK, // primary joystick (Axis 0/1)

		// Buttons
		ACTION_AX_BUTTON, // A/X button (button 7)
		ACTION_BYM_BUTTON, // B/Y/Menu button (button 1)
		ACTION_FRONT_BUTTON, // front trigger as button (button 15)
		ACTION_SIDE_BUTTON, // side trigger/grip as button (button 2)

		// Output
		ACTION_HAPTIC, // Haptic output

		ACTION_MAX
	};

	struct DefaultAction {
		const char *name;
		const XrActionType type;
		Action *action;
	};

	DefaultAction default_actions[ACTION_MAX] = {
		{ "aim_pose", XR_ACTION_TYPE_POSE_INPUT, NULL },
		{ "grip_pose", XR_ACTION_TYPE_POSE_INPUT, NULL },
		{ "front_trigger", XR_ACTION_TYPE_FLOAT_INPUT, NULL },
		{ "side_trigger", XR_ACTION_TYPE_FLOAT_INPUT, NULL },
		{ "joystick", XR_ACTION_TYPE_VECTOR2F_INPUT, NULL },
		{ "ax_buttons", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "bym_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "front_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "side_button", XR_ACTION_TYPE_BOOLEAN_INPUT, NULL },
		{ "haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT, NULL },
	};

private:
	static OpenXRApi *singleton;
	bool initialised = false;
	bool running = false;
	int use_count = 1;

	// extensions
	bool hand_tracking_ext = false;
	bool monado_stick_on_ball_ext = false;
	bool hand_tracking_supported = false;

	XrInstance instance = XR_NULL_HANDLE;
	XrSystemId systemId;
	XrSession session = XR_NULL_HANDLE;
	XrSessionState state = XR_SESSION_STATE_UNKNOWN;
	XrVulkanInstanceCreateInfoKHR graphics_binding_vulkan;

	bool keep_3d_linear = true;
	XrVulkanSwapchainFormatListCreateInfoKHR **images = NULL;
	XrSwapchain *swapchains = NULL;
	uint32_t view_count;
	XrViewConfigurationView *configuration_views = NULL;

	XrCompositionLayerProjection *projectionLayer = NULL;
	XrFrameState frameState = {};

	uint32_t *buffer_index = NULL;

	XrView *views = NULL;
	XrCompositionLayerProjectionView *projection_views = NULL;
	XrSpace play_space = XR_NULL_HANDLE;
	XrSpace view_space = XR_NULL_HANDLE;
	bool view_pose_valid = false;
	bool head_pose_valid = false;

	HandTracker hand_trackers[2]; // Fixed for left and right hand

	// config
	/*
	 * XR_REFERENCE_SPACE_TYPE_LOCAL: head pose on startup/recenter is coordinate system origin.
	 * XR_REFERENCE_SPACE_TYPE_STAGE: origin is externally calibrated to be on play space floor.
	 *
	 * Note that Godot has it's own implementation to support localise the headset, but we could expose this through our config
	 */
	XrReferenceSpaceType play_space_type = XR_REFERENCE_SPACE_TYPE_STAGE;

	/*
	 * XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY - wearable displays, usually stereoscopic
	 * XR_FORM_FACTOR_HANDHELD_DISPLAY - handheld devices, phones, tablets, etc.
	 */
	XrFormFactor form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

	String action_sets_json;
	String interaction_profiles_json;

	std::vector<ActionSet *> action_sets;

	bool isExtensionSupported(const char *extensionName, XrExtensionProperties *instanceExtensionProperties, uint32_t instanceExtensionCount);
	bool isViewConfigSupported(XrViewConfigurationType type, XrSystemId systemId);
	bool isReferenceSpaceSupported(XrReferenceSpaceType type);
	bool initialiseInstance();
	bool initialiseExtensions();
	bool initialiseSession();
	bool initialiseSpaces();
	bool initialiseSwapChains();
	bool initialiseActionSets();
	bool initialiseHandTracking();
	bool check_graphics_requirements_gl(XrSystemId system_id);
	XrResult acquire_image(int eye);
	void update_actions();
	void update_handtracking();
	void transform_from_matrix(Transform &r_dest, XrMatrix4x4f *matrix, float p_world_scale);

	bool parse_action_sets(const String &p_json);
	bool parse_interaction_profiles(const String &p_json);

public:
	static OpenXRApi *openxr_get_api();
	static void openxr_release_api();

	OpenXRApi();
	~OpenXRApi();

	bool is_initialised();
	bool initialize();
	void uninitialize();

	XrInstance get_instance() { return instance; };
	XrSession get_session() { return session; };
	bool get_keep_3d_linear() { return keep_3d_linear; };

	template <class... Args>
	bool xr_result(XrResult result, const char *format, Args... values) {
		if (XR_SUCCEEDED(result))
			return true;

		char resultString[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(instance, result, resultString);

		ERR_PRINT(String("OpenXR ") + vformat(format, values...) + String(" [") + String(resultString) + String("]"));
		return false;
	};

	const HandTracker *get_hand_tracker(int p_hand) { return &hand_trackers[p_hand]; };

	// config
	XrFormFactor get_form_factor() const;
	void set_form_factor(XrFormFactor p_form_factor);

	static const char *default_action_sets_json;
	String get_action_sets_json() const;
	void set_action_sets_json(const String &p_action_sets_json);

	static const char *default_interaction_profiles_json;
	String get_interaction_profiles_json() const;
	void set_interaction_profiles_json(const String &p_interaction_profiles_json);

	bool has_action_sets() { return action_sets.size() > 0; };
	ActionSet *get_action_set(const String &p_name);
	Action *get_action(const char *p_name);

	// fill_projection_matrix() should be called after process_openxr()
	void fill_projection_matrix(int eye, float p_z_near, float p_z_far, float *p_projection);

	// recommended_rendertarget_size() returns required size of our image buffers
	void recommended_rendertarget_size(uint32_t &width, uint32_t &height);

	// get_view_transform() should be called after fill_projection_matrix()
	bool get_view_transform(int eye, float world_scale, Transform &transform_for_eye);

	// get_head_center() can be called at any time after init
	bool get_head_center(float world_scale, Transform &transform);

	// get_external_texture_for_eye() acquires images and sets has_support to true
	int get_external_texture_for_eye(int eye, bool &has_support);

	// process_openxr() should be called FIRST in the frame loop
	void process_openxr();

	// helper method to get a Transform from an openxr pose
	Transform transform_from_pose(const XrPosef &p_pose, float p_world_scale);
};

#endif /* !OPENXR_API_H */
