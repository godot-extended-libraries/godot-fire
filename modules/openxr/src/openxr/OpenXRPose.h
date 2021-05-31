/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR pose GDNative object, this exposes specific locations we track

#ifndef OPENXR_POSE_H
#define OPENXR_POSE_H

#include "OpenXRApi.h"
#include "scene/3d/node_3d.h"

class OpenXRPose : public Node3D {
	GDCLASS(OpenXRPose, Node3D);

private:
	OpenXRApi *openxr_api;
	bool invisible_if_inactive = true;
	String action;
	String path;

	// cache action and path
	bool fail_cache = false;
	Action *_action;
	XrPath _path;
	bool check_action_and_path();
protected:

	static void _bind_methods();
public:

	void _init();
	void _physics_process(float delta);

	OpenXRPose();
	~OpenXRPose();

	bool is_active();

	bool get_invisible_if_inactive() const;
	void set_invisible_if_inactive(bool hide);

	String get_action() const;
	void set_action(const String p_action);

	String get_path() const;
	void set_path(const String p_path);
};

#endif // !OPENXR_POSE_H
