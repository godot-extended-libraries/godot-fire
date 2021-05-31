#pragma once

#include "core/error/error_macros.h"
#include "modules/openxr/openxr_loader_windows/1.0.14/include/openxr/openxr.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr_server.h"

#include "openxr/OpenXRApi.h"
#include <stdint.h>

class OpenXRInterface : public XRInterface {
	GDCLASS(OpenXRInterface, XRInterface);
	friend class RenderingDeviceVulkan;
	friend class RendererCompositorRD;

	struct arvr_data_struct {
		OpenXRApi *openxr_api = nullptr;

		bool has_external_texture_support = true;
	} arvr_data;

	// Just set some defaults for these. At some point we need to look at adding a lookup table for common device + headset combos and/or support reading cardboard QR codes
	float eye_height = 1.85;
	uint64_t last_ticks = 0;

	real_t intraocular_dist = 6.0;
	real_t display_width = 14.5;
	real_t display_to_lens = 4.0;
	real_t oversample = 1.5;

	real_t k1 = 0.215;
	real_t k2 = 0.215;
	real_t aspect = 1.0;

public:
	virtual Transform get_camera_transform() override { return Transform(); }
	virtual uint32_t get_view_count() override {
		return 2;
	}
	virtual CameraMatrix get_projection_for_view(uint32_t p_view, real_t p_aspect, real_t p_z_near, real_t p_z_far) override {
		return CameraMatrix();
	}
	virtual Transform get_transform_for_view(uint32_t p_view, const Transform &p_cam_transform) override {
		return Transform();
	}
	virtual Vector<BlitToScreen> commit_views(RID p_render_target, const Rect2 &p_screen_rect) override;

	StringName get_name() const override;

	int get_capabilities() const override;

	bool get_anchor_detection_is_enabled() const override;

	void set_anchor_detection_is_enabled(bool p_enable) override;

	bool is_initialized() const override;

	bool initialize() override;

	void uninitialize() override;
	Vector2 get_render_targetsize() override;

	void set_default_pos(Transform *p_transform, float p_world_scale, int p_eye);

	void fill_projection_for_eye(float *p_projection, int p_eye, float p_aspect, float p_z_near, float p_z_far);

	void process() override;

	void notification(int p_what) override;
};