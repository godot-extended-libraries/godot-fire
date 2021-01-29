/*************************************************************************/
/*  audio_consumer.cpp                                                   */
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

#include "scene/audio/audio_consumer.h"

void AudioConsumer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_buffer", "len"), &AudioConsumer::get_buffer);
	ClassDB::bind_method(D_METHOD("initialize", "capture_effect", "buffer_size_ms"), &AudioConsumer::initialize);
	ClassDB::bind_method(D_METHOD("clear_buffer"), &AudioConsumer::clear_buffer);
}

AudioConsumer::AudioConsumer() {
}

AudioConsumer::~AudioConsumer() {
	clear_buffer();
}
PackedVector2Array AudioConsumer::get_buffer(int p_frames) {
	ERR_FAIL_COND_V(ring_buffer.is_null(), PackedVector2Array());
	ERR_FAIL_COND_V(p_frames >= ring_buffer->get().size(), PackedVector2Array());
	int data_left = ring_buffer->get().data_left();
	if (data_left < p_frames || !data_left) {
		return PackedVector2Array();
	}

	PackedVector2Array ret;
	ret.resize(p_frames);

	Vector<AudioFrame> streaming_data;
	streaming_data.resize(p_frames);
	ring_buffer->get().read(streaming_data.ptrw(), p_frames);
	for (int32_t i = 0; i < p_frames; i++) {
		ret.write[i] = Vector2(streaming_data[i].l, streaming_data[i].r);
	}
	return ret;
}

void AudioConsumer::initialize(Ref<AudioEffectCapture> p_capture_effect, float p_buffer_size_seconds) {
	if (p_capture_effect.is_null()) {
		return;
	}
	int64_t ring_buffer_max_size = (int64_t)(1000.0f * p_buffer_size_seconds);
	ring_buffer_max_size *= AudioServer::get_singleton()->get_mix_rate();
	ring_buffer_max_size /= 1000; //convert to seconds
	if (ring_buffer_max_size <= 0 || ring_buffer_max_size >= (1 << 29)) {
		return;
	}
	ring_buffer = p_capture_effect->instance_ring_buffer((int32_t)ring_buffer_max_size);
}

void AudioConsumer::clear_buffer() {
	if (ring_buffer.is_null()) {
		return;
	}
	const int32_t data_left = ring_buffer->get().data_left();
	ring_buffer->get().advance_read(data_left);
}
