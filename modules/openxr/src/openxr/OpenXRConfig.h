/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#ifndef OPENXR_CONFIG_H
#define OPENXR_CONFIG_H

#include "scene/main/node.h"

#include "OpenXRApi.h"

class OpenXRConfig : public Node {
	GDCLASS(OpenXRConfig, Node);

private:
	OpenXRApi *openxr_api;

protected:
	static void _bind_methods();

public:

	void _init();

	OpenXRConfig();
	~OpenXRConfig();

	bool keep_3d_linear() const;

	int get_form_factor() const;
	void set_form_factor(int p_form_factor);

	String get_action_sets() const;
	void set_action_sets(const String p_action_sets);

	String get_interaction_profiles() const;
	void set_interaction_profiles(const String p_interaction_profiles);
};

#endif // !OPENXR_CONFIG_H
