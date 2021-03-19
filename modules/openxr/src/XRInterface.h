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

public:
	StringName get_name() const override;

	int get_capabilities() const override;

	bool get_anchor_detection_is_enabled() const override;

	void set_anchor_detection_is_enabled(bool p_enable) override;
	bool is_stereo() override;

	bool is_initialized() const override;

	bool initialize() override;

	void uninitialize() override;
	Vector2 get_render_targetsize() override;

	void set_default_pos(Transform *p_transform, float p_world_scale, int p_eye);

	Transform get_transform_for_eye(XRInterface::Eyes p_eye, const Transform &p_cam_transform) override;

	void fill_projection_for_eye(float *p_projection, int p_eye, float p_aspect, float p_z_near, float p_z_far);

	void process() override;

	void get_external_texture_for_eye(XRInterface::Eyes p_eye, RID p_texture) override;

	CameraMatrix get_projection_for_eye(XRInterface::Eyes p_eye, real_t p_aspect, real_t p_z_near, real_t p_z_far) override;

	void notification(int p_what) override;
};