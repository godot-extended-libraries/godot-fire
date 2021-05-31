#include "core/error/error_macros.h"
#include "modules/openxr/openxr_loader_windows/1.0.14/include/openxr/openxr.h"
#include "servers/rendering/rendering_device.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr_server.h"

#include "openxr/OpenXRApi.h"
#include <stdint.h>

#include "XRInterface.h"
#include "servers/rendering/renderer_compositor.h"

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

void OpenXRInterface::notification(int p_what) {
}

Vector<BlitToScreen> OpenXRInterface::commit_views(RID p_render_target, const Rect2 &p_screen_rect) {
	Vector<BlitToScreen> ret;
	return ret;
}
