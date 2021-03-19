#include "core/error/error_macros.h"
#include "modules/openxr/openxr_loader_windows/1.0.14/include/openxr/openxr.h"
#include "servers/rendering/rendering_device.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr_server.h"

#include "openxr/OpenXRApi.h"
#include <stdint.h>

#include "XRInterface.h"

StringName OpenXRInterface::get_name() const {
	StringName ret = "OpenXR";
	return ret;
}

int OpenXRInterface::get_capabilities() const {
	int ret;
	ret = 2 + 8; // 2 = ARVR_STEREO, 8 = ARVR_EXTERNAL
	return ret;
}

bool OpenXRInterface::get_anchor_detection_is_enabled() const {
	bool ret = false; // does not apply here

	return ret;
}

void OpenXRInterface::set_anchor_detection_is_enabled(bool p_enable) {
	// we ignore this, not supported in this interface!
}

bool OpenXRInterface::is_stereo() {
	bool ret = true;

	// TODO we should check our configuration and see if we are setup for stereo (hmd) or mono output (tablet)

	return ret;
}

bool OpenXRInterface::is_initialized() const {
	bool ret = false;
	if (!arvr_data.openxr_api) {
		return ret;
	} 
	return arvr_data.openxr_api->is_initialised();
}

bool OpenXRInterface::initialize() {
	bool ret = false;

	// Doesn't yet exist? create our OpenXR API instance
	if (arvr_data.openxr_api == NULL) {
		arvr_data.openxr_api = OpenXRApi::openxr_get_api();
	}

	// We (already) have our API instance? cool!
	if (arvr_data.openxr_api != NULL) {
		// not initialise
		arvr_data.openxr_api->initialize();

		// Are we good ?
		ret = arvr_data.openxr_api->is_initialised();
	}

	// and return our result
	return ret;
}

void OpenXRInterface::uninitialize() {
	if (arvr_data.openxr_api != NULL) {
		// cleanup
		arvr_data.openxr_api->uninitialize();

		// and release
		OpenXRApi::openxr_release_api();
		arvr_data.openxr_api = NULL;
	}
}

Vector2 OpenXRInterface::get_render_targetsize() {
	Vector2 size;

	if (arvr_data.openxr_api != NULL) {
		uint32_t width, height;

		arvr_data.openxr_api->recommended_rendertarget_size(width, height);
		size.width = width;
		size.height = height;
	} else {
		size.width = 500;
		size.height = 500;
	}

	return size;
}

void OpenXRInterface::set_default_pos(Transform *p_transform, float p_world_scale, int p_eye) {
	ERR_FAIL_COND(!p_transform);
	Transform t;
	t = *p_transform;

	// if we're not tracking, don't put our head on the floor...
	t.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		t.origin.x = 0.03 * p_world_scale;
	} else if (p_eye == 2) {
		t.origin.x = -0.03 * p_world_scale;
	}
}

Transform OpenXRInterface::get_transform_for_eye(XRInterface::Eyes p_eye, const Transform &p_cam_transform) {
	Transform transform_for_eye;
	Transform reference_frame = XRServer::get_singleton()->get_reference_frame();
	Transform ret;
	float world_scale = XRServer::get_singleton()->get_world_scale();

	if (arvr_data.openxr_api != NULL) {
		if (p_eye == 0) {
			// this is used for head positioning, it should return the position center between the eyes
			if (!arvr_data.openxr_api->get_head_center(world_scale, transform_for_eye)) {
				set_default_pos(&transform_for_eye, world_scale, p_eye);
			}
		} else {
			// printf("Get view matrix for eye %d\n", p_eye);
			if (p_eye == 1) {
				if (!arvr_data.openxr_api->get_view_transform(0, world_scale, transform_for_eye)) {
					set_default_pos(&transform_for_eye, world_scale, p_eye);
				}
			} else if (p_eye == 2) {
				if (!arvr_data.openxr_api->get_view_transform(1, world_scale, transform_for_eye)) {
					set_default_pos(&transform_for_eye, world_scale, p_eye);
				}
			} else {
				// TODO does this ever happen?
				set_default_pos(&transform_for_eye, world_scale, p_eye);
			}
		}
	} else {
		set_default_pos(&transform_for_eye, world_scale, p_eye);
	}

	// Now construct our full Transform, the order may be in reverse, have
	// to test
	// :)
	ret *= p_cam_transform;
	ret *= reference_frame;
	ret *= transform_for_eye;
	return ret;
}

void OpenXRInterface::fill_projection_for_eye(float *p_projection, int p_eye, float p_aspect, float p_z_near, float p_z_far) {
	if (arvr_data.openxr_api != NULL) {
		// printf("fill projection for eye %d\n", p_eye);
		if (p_eye == 1) {
			arvr_data.openxr_api->fill_projection_matrix(0, p_z_near, p_z_far, p_projection);
		} else {
			arvr_data.openxr_api->fill_projection_matrix(1, p_z_near, p_z_far, p_projection);
		}
		// ???

		// printf("\n");
	} else {
		// uhm, should do something here really..
	}
}

void OpenXRInterface::process() {
	// this method gets called before every frame is rendered, here is where
	// you should update tracking data, update controllers, etc.
	if (arvr_data.openxr_api != NULL) {
		arvr_data.openxr_api->process_openxr();
	}
}

void OpenXRInterface::get_external_texture_for_eye(XRInterface::Eyes p_eye, RID r_texture) {
	// this only gets called from Godot 3.2 and newer, allows us to use
	// OpenXR swapchain directly.

	RenderingDevice::get_singleton()->submit_vr_texture(r_texture, p_eye);
}

CameraMatrix OpenXRInterface::get_projection_for_eye(XRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) {
	_THREAD_SAFE_METHOD_

	CameraMatrix eye;

	if (p_eye == XRInterface::EYE_MONO) {
		///@TODO for now hardcode some of this, what is really needed here is that this needs to be in sync with the real cameras properties
		// which probably means implementing a specific class for iOS and Android. For now this is purely here as an example.
		// Note also that if you use a normal viewport with AR/VR turned off you can still use the tracker output of this interface
		// to position a stock standard Godot camera and have control over this.
		// This will make more sense when we implement ARkit on iOS (probably a separate interface).
		eye.set_perspective(60.0, p_aspect, p_z_near, p_z_far, false);
	} else {
		arvr_data.openxr_api->fill_projection_matrix(p_eye, p_z_near, p_z_far, (float *)eye.matrix);
	};

	return eye;
};
void OpenXRInterface::notification(int p_what) {
}