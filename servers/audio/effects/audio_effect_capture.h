/*************************************************************************/
/*  audio_effect_capture.h                                               */
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

#ifndef AUDIO_EFFECT_STREAM_OPUS_H
#define AUDIO_EFFECT_STREAM_OPUS_H

#include "core/config/engine.h"
#include "core/math/audio_frame.h"
#include "core/object/reference.h"
#include "core/templates/vector.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio_server.h"

class AudioEffectCapture;

class RingBufferAudioFrame : public Reference {
	GDCLASS(RingBufferAudioFrame, Reference);
	RingBuffer<AudioFrame> ring;

public:
	RingBuffer<AudioFrame> &get() { return ring; }
};

class AudioEffectCaptureInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectCaptureInstance, AudioEffectInstance);
	friend class AudioEffectCapture;
	Ref<AudioEffectCapture> base;

public:
	void init();
	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;
	virtual bool process_silence() const override;
	AudioEffectCaptureInstance();
	~AudioEffectCaptureInstance();
};

class AudioEffectCapture : public AudioEffect {
	GDCLASS(AudioEffectCapture, AudioEffect)
	friend class AudioEffectCaptureInstance;

	Ref<RingBufferAudioFrame> output_ring_buffer;
	uint64_t discarded_frames = 0;
	uint64_t pushed_frames = 0;

protected:
	static void _bind_methods();

public:
	virtual Ref<AudioEffectInstance> instance() override;

	Ref<RingBufferAudioFrame> instance_ring_buffer(int32_t p_ring_buffer_max_size);

	uint64_t get_discarded_frames() const;
	int32_t get_ring_data_left() const;
	int32_t get_ring_size() const;
	uint64_t get_pushed_frames() const;

	AudioEffectCapture();
	~AudioEffectCapture();
};

#endif // AUDIO_EFFECT_STREAM_OPUS_H
