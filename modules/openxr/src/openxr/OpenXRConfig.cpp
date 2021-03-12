/////////////////////////////////////////////////////////////////////////////////////
// Our OpenXR config GDNative object

#include "OpenXRConfig.h"
#include "core/variant/dictionary.h"

void OpenXRConfig::_bind_methods() {
	ClassDB::bind_method(D_METHOD("keep_3d_linear"), &OpenXRConfig::keep_3d_linear);

	ClassDB::bind_method(D_METHOD("get_form_factor"), &OpenXRConfig::get_form_factor);
	ClassDB::bind_method(D_METHOD("set_form_factor"), &OpenXRConfig::set_form_factor);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "form_factor", PROPERTY_HINT_ENUM, "Not set,HMD,Hand Held"), "set_form_factor", "get_form_factor");

	ClassDB::bind_method(D_METHOD("get_action_sets"), &OpenXRConfig::get_action_sets);
	ClassDB::bind_method(D_METHOD("set_action_sets"), &OpenXRConfig::set_action_sets);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "action_sets", PROPERTY_HINT_MULTILINE_TEXT, "Not set,HMD,Hand Held"), "set_action_sets", "get_form_factor");

	ClassDB::bind_method(D_METHOD("get_interaction_profiles"), &OpenXRConfig::get_interaction_profiles);
	ClassDB::bind_method(D_METHOD("set_interaction_profiles"), &OpenXRConfig::set_interaction_profiles);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "interaction_profiles", PROPERTY_HINT_MULTILINE_TEXT, "Not set,HMD,Hand Held"), "set_interaction_profiles", "get_interaction_profiles");
}

OpenXRConfig::OpenXRConfig() {
	openxr_api = OpenXRApi::openxr_get_api();
}

OpenXRConfig::~OpenXRConfig() {
	if (openxr_api != NULL) {
		OpenXRApi::openxr_release_api();
	}
}

void OpenXRConfig::_init() {
	// nothing to do here
}

bool OpenXRConfig::keep_3d_linear() const {
	if (openxr_api == NULL) {
		return false;
	} else {
		return openxr_api->get_keep_3d_linear();
	}
}

int OpenXRConfig::get_form_factor() const {
	if (openxr_api == NULL) {
		return 0;
	} else {
		return (int)openxr_api->get_form_factor();
	}
}

void OpenXRConfig::set_form_factor(int p_form_factor) {
	if (openxr_api == NULL) {
		print_line("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_form_factor((XrFormFactor)p_form_factor);
	}
}

String OpenXRConfig::get_action_sets() const {
	if (openxr_api == NULL) {
		return String();
	} else {
		return openxr_api->get_action_sets_json();
	}
}

void OpenXRConfig::set_action_sets(const String p_action_sets) {
	if (openxr_api == NULL) {
		print_line("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_action_sets_json(p_action_sets);
	}
}

String OpenXRConfig::get_interaction_profiles() const {
	if (openxr_api == NULL) {
		return String();
	} else {
		return openxr_api->get_interaction_profiles_json();
	}
}

void OpenXRConfig::set_interaction_profiles(const String p_interaction_profiles) {
	if (openxr_api == NULL) {
		print_line("OpenXR object wasn't constructed.");
	} else {
		openxr_api->set_interaction_profiles_json(p_interaction_profiles);
	}
}
