/*************************************************************************/
/*  audio_effect_capture.cpp                                             */
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

#include "audio_effect_capture.h"

void AudioEffectCapture::_bind_methods() {
	ClassDB::bind_method(D_METHOD("instance_ring_buffer"), &AudioEffectCapture::instance_ring_buffer);
	ClassDB::bind_method(D_METHOD("get_discarded_frames"), &AudioEffectCapture::get_discarded_frames);
	ClassDB::bind_method(D_METHOD("get_ring_data_left"), &AudioEffectCapture::get_ring_data_left);
	ClassDB::bind_method(D_METHOD("get_ring_size"), &AudioEffectCapture::get_ring_size);
	ClassDB::bind_method(D_METHOD("get_pushed_frames"), &AudioEffectCapture::get_pushed_frames);
}

Ref<AudioEffectInstance> AudioEffectCapture::instance() {
	Ref<AudioEffectCaptureInstance> ins;
	ins.instance();
	ins->base = Ref<AudioEffectCapture>(this);

	return ins;
}

Ref<RingBufferAudioFrame> AudioEffectCapture::instance_ring_buffer(int32_t p_ring_buffer_max_size) {
	Ref<RingBufferAudioFrame> new_ring_buffer;
	new_ring_buffer.instance();
	int32_t higher_power = (int32_t)Math::ceil(Math::log(double(p_ring_buffer_max_size)) / Math::log(2.0));
	new_ring_buffer->get().resize(higher_power);
	output_ring_buffer = new_ring_buffer;
	return new_ring_buffer;
}

uint64_t AudioEffectCapture::get_discarded_frames() const {
	return discarded_frames;
}

int32_t AudioEffectCapture::get_ring_data_left() const {
	Ref<RingBufferAudioFrame> ring_buffer = output_ring_buffer;
	ERR_FAIL_NULL_V(ring_buffer, 0);
	return ring_buffer->get().data_left();
}

int32_t AudioEffectCapture::get_ring_size() const {
	Ref<RingBufferAudioFrame> ring_buffer = output_ring_buffer;
	ERR_FAIL_NULL_V(ring_buffer, 0);
	return ring_buffer->get().size();
}

uint64_t AudioEffectCapture::get_pushed_frames() const {
	return pushed_frames;
}

AudioEffectCapture::AudioEffectCapture() {
}

AudioEffectCapture::~AudioEffectCapture() {
	output_ring_buffer.unref();
}

void AudioEffectCaptureInstance::init() {
}

void AudioEffectCaptureInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	Ref<RingBufferAudioFrame> ring_buffer = base->output_ring_buffer;

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
	}

	if (ring_buffer.is_null() || !ring_buffer->get().size()) {
		return;
	}
	if (ring_buffer->get().space_left() >= p_frame_count) {
		//Add incoming audio frames to the IO ring buffer
		int32_t ret = ring_buffer->get().write(p_src_frames, p_frame_count);
		ERR_FAIL_COND_MSG(ret != p_frame_count, "Failed to add data to effect capture ring buffer despite sufficient space.");
		atomic_add(&base->pushed_frames, p_frame_count);
	} else {
		atomic_add(&base->discarded_frames, p_frame_count);
	}
}

bool AudioEffectCaptureInstance::process_silence() const {
	return true;
}

AudioEffectCaptureInstance::AudioEffectCaptureInstance() {
}

AudioEffectCaptureInstance::~AudioEffectCaptureInstance() {
}
