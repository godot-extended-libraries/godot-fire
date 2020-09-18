#include "godot.h"

static OS_Server os;

UNIFEX_TERM init(UnifexEnv *env, MyState *state, char **in_strings, unsigned int list_length) {
	int err = OK;
	if (state) {
		UNIFEX_TERM res = init_result_ok(env, state, FAILED);
		return res;
	}
	state = unifex_alloc_state(env);
	err = Main::setup(in_strings[0], list_length - 1, &in_strings[1]);
	if (err != OK) {
		return init_result_ok(env, state, 255);
	}
	if (!state) {
		return init_result_ok(env, state, 255);
	}
	if (!Main::start()) {
		return init_result_ok(env, state, 255);
	}
	UNIFEX_TERM res = init_result_ok(env, state, err);
	
	return res;
}

UNIFEX_TERM iteration(UnifexEnv *env, MyState *state, int delta) {
	// delta should be a float
	if (!state) {
		return iteration_result_ok(env, state, 255);
	}
	bool err = os.get_main_loop()->iteration(delta);
	if (err != OK) {
		return iteration_result_ok(env, state, 255);
	}
	UNIFEX_TERM res = iteration_result_ok(env, state, err);
	return res;
}

UNIFEX_TERM call(UnifexEnv *env, MyState *state, char *method) {
	if (!state) {
		return call_result_ok(env, state, Variant::NIL, "");
	}
	if (!os.get_main_loop()->get_script_instance()) {
		return call_result_ok(env, state, Variant::NIL, "");
	}
	Variant::CallError call_error;
	Variant res = os.get_main_loop()->call(method, nullptr, 0, call_error);
	if (call_error.error != Variant::CallError::CALL_OK) {
		return call_result_ok(env, state, Variant::NIL, "");
	}
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
