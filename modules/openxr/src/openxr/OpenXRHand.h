/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR hand GDNative object implemented through meshes

#ifndef OPENXR_HAND_H
#define OPENXR_HAND_H

#include "OpenXRApi.h"
#include "core/object/reference.h"
#include "scene/3d/node_3d.h"

class OpenXRHand : public Node3D {
	GDCLASS(OpenXRHand, Node3D);

private:
	OpenXRApi *openxr_api;
	int hand;

	Node3D *joints[XR_HAND_JOINT_COUNT_EXT];
	
protected:
	static void _bind_methods();
public:

	void _init();
	void _ready();
	void _physics_process(float delta);

	OpenXRHand();
	~OpenXRHand();

	bool is_active() const;

	int get_hand() const;
	void set_hand(int p_hand);
};

#endif // !OPENXR_HAND_H
