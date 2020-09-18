#include "godot.h"

static OS_Server os;

UNIFEX_TERM init(UnifexEnv *env, MyState *state, char **in_strings, unsigned int list_length) {
	int err = OK;
	if (state) {
		return init_result_fail(env, state, "Godot is already initialized.");
	}
	state = unifex_alloc_state(env);
	err = Main::setup(in_strings[0], list_length - 1, &in_strings[1]);
	if (err != OK) {
		return init_result_fail(env, state, "Godot can't be setup.");
	}
	if (!Main::start()) {
		return init_result_fail(env, state, "Godot can't start.");
	}	
	return init_result_ok(env, state, err);
}

UNIFEX_TERM iteration(UnifexEnv *env, MyState *state, int delta) {
	// delta should be a float
	if (!state) {
		return iteration_result_fail(env, state, "Godot is not initalized.");
	}
	bool err = os.get_main_loop()->iteration(delta);
	if (err != OK) {
		return iteration_result_fail(env, state, "Godot can't iterate.");
	}
	return iteration_result_ok(env, state, err);
}

UNIFEX_TERM call(UnifexEnv *env, MyState *state, char *method) {
	if (!state) {
        return call_result_fail(env, state, "Godot is not initalized.");
	}
	if (!os.get_main_loop()->get_script_instance()) {
        return call_result_fail(env, state, "Godot does not have a script instance.");
	}
	Variant::CallError call_error;
	Variant res = os.get_main_loop()->call(method, nullptr, 0, call_error);
	const CharString res_char_string = String(res).utf8();
	const char *res_string = res_char_string.get_data();
	return call_result_ok(env, state, res.get_type(), res_string);
}

void handle_destroy_state(UnifexEnv *env, MyState *state) {
    UNIFEX_UNUSED(env);
    UNIFEX_UNUSED(state);
	os.get_main_loop()->finish();
	Main::cleanup();
}
