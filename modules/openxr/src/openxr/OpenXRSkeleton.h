/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through skeleton (bones)

#ifndef OPENXR_SKELETON_H
#define OPENXR_SKELETON_H

#include "OpenXRApi.h"
#include "core/object/reference.h"
#include "scene/3d/skeleton_3d.h"

class OpenXRSkeleton : public Skeleton3D {
	GDCLASS(OpenXRSkeleton, Skeleton3D);

private:
	OpenXRApi *openxr_api;
	int hand;

	int64_t bones[XR_HAND_JOINT_COUNT_EXT];
protected:
	static void _bind_methods();
public:

	void _init();
	void _ready();
	void _physics_process(float delta);

	OpenXRSkeleton();
	~OpenXRSkeleton();

	int get_hand() const;
	void set_hand(int p_hand);
};

#endif // !OPENXR_SKELETON_H
