/*************************************************************************/
/*  godot.cpp                                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

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

UNIFEX_TERM iteration(UnifexEnv *env, MyState *state, double delta) {
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
	switch (res.get_type()) {
		case Variant::NIL: {
			return call_result_fail(env, state, "Call is invalid.");
		} break;
		case Variant::BOOL: {
			int res_int = res;
			return call_result_ok_bool(env, state, res_int);
		} break;
		case Variant::INT: {
			int res_int = res;
			return call_result_ok_int(env, state, res_int);
		} break;
		case Variant::REAL: {
			const CharString res_char_string = String(res).utf8();
			const char *res_string = res_char_string.get_data();
			return call_result_ok_string(env, state, res_string);
			// return call_result_ok_float(env, state, res.get_type(), res_float);
		} break;
		case Variant::STRING: {
			const CharString res_char_string = String(res).utf8();
			const char *res_string = res_char_string.get_data();
			return call_result_ok_string(env, state, res_string);
		} break;
		default: {
			const CharString res_char_string = (String("Unsupported result: ") + String(res)).utf8();
			const char *res_string = res_char_string.get_data();
			return call_result_fail(env, state, res_string);
		} break;
	}
	return call_result_fail(env, state, "Call is invalid.");
}

void handle_destroy_state(UnifexEnv *env, MyState *state) {
	UNIFEX_UNUSED(env);
	UNIFEX_UNUSED(state);
	os.get_main_loop()->finish();
	Main::cleanup();
}
