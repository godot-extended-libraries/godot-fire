/*************************************************************************/
/*  speech_processor.h                                                   */
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

#ifndef SPEECH_PROCESSOR_H
#define SPEECH_PROCESSOR_H

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/mutex.h"
#include "scene/main/node.h"
#include "servers/audio_server.h"

#include "servers/audio/effects/audio_effect_capture.h"

#include "scene/audio/audio_stream_player.h"
#include "servers/audio/audio_stream.h"

#include <stdlib.h>
#include <functional>

#include "opus_codec.h"
#include "thirdparty/libsamplerate/src/samplerate.h"

#include "speech_decoder.h"

class SpeechDecoder;
class SpeechProcessor : public Node {
	GDCLASS(SpeechProcessor, Node)
	Mutex mutex;

public:
	static const uint32_t VOICE_SAMPLE_RATE = 48000;
	static const uint32_t CHANNEL_COUNT = 1;
	//static const uint32_t MILLISECONDS_PER_PACKET = 100;
	static const uint32_t BUFFER_FRAME_COUNT = VOICE_SAMPLE_RATE / 100; //MILLISECONDS_PER_PACKET;
	static const uint32_t BUFFER_BYTE_COUNT = sizeof(uint16_t);
	static const uint32_t PCM_BUFFER_SIZE = BUFFER_FRAME_COUNT * BUFFER_BYTE_COUNT * CHANNEL_COUNT;

private:
	OpusCodec<VOICE_SAMPLE_RATE, CHANNEL_COUNT> *opus_codec;

private:
	uint32_t record_mix_frames_processed = 0;

	AudioServer *audio_server = NULL;
	AudioStreamPlayer *audio_input_stream_player = NULL;
	Ref<AudioEffectCapture> audio_effect_capture = NULL;

	uint32_t mix_rate;
	PackedByteArray mix_byte_array;

	PackedFloat32Array mono_real_array;
	PackedFloat32Array resampled_real_array;
	uint32_t resampled_real_array_offset = 0;

	PackedByteArray pcm_byte_array_cache;

	// LibResample
	SRC_STATE *libresample_state;
	int libresample_error;

	int64_t capture_discarded_frames;
	int64_t capture_pushed_frames;
	int32_t capture_ring_limit;
	int32_t capture_ring_current_size;
	int32_t capture_ring_max_size;
	int64_t capture_ring_size_sum;
	int32_t capture_get_calls;
	int64_t capture_get_frames;

public:
	struct SpeechInput {
		PackedByteArray *pcm_byte_array = NULL;
		float volume = 0.0;
	};

	struct CompressedSpeechBuffer {
		PackedByteArray *compressed_byte_array = NULL;
		int buffer_size = 0;
	};

	std::function<void(SpeechInput *)> speech_processed;
	void register_speech_processed(const std::function<void(SpeechInput *)> &callback) {
		speech_processed = callback;
	}

	static void _bind_methods();

	uint32_t _resample_audio_buffer(const float *p_src,
			const uint32_t p_src_frame_count,
			const uint32_t p_src_samplerate,
			const uint32_t p_target_samplerate,
			float *p_dst);

	void start();
	void stop();

	static void _get_capture_block(
			AudioServer *p_audio_server,
			const uint32_t &p_mix_frame_count,
			const Vector2 *p_process_buffer_in,
			float *p_process_buffer_out);

	void _mix_audio(const Vector2 *p_process_buffer_in);

	static bool _16_pcm_mono_to_real_stereo(const PackedByteArray *p_src_buffer, PackedVector2Array *p_dst_buffer);

	virtual bool compress_buffer_internal(const PackedByteArray *p_pcm_byte_array, CompressedSpeechBuffer *p_output_buffer) {
		p_output_buffer->buffer_size = opus_codec->encode_buffer(p_pcm_byte_array, p_output_buffer->compressed_byte_array);
		if (p_output_buffer->buffer_size != -1) {
			return true;
		}

		return false;
	}

	virtual bool decompress_buffer_internal(
			SpeechDecoder *speech_decoder,
			const PackedByteArray *p_read_byte_array,
			const int p_read_size,
			PackedVector2Array *p_write_vec2_array) {
		if (opus_codec->decode_buffer(speech_decoder, p_read_byte_array, &pcm_byte_array_cache, p_read_size, PCM_BUFFER_SIZE)) {
			if (_16_pcm_mono_to_real_stereo(&pcm_byte_array_cache, p_write_vec2_array)) {
				return true;
			}
		}
		return true;
	}

	virtual Dictionary compress_buffer(
			const PackedByteArray &p_pcm_byte_array,
			Dictionary p_output_buffer);

	virtual PackedVector2Array decompress_buffer(
			Ref<SpeechDecoder> p_speech_decoder,
			const PackedByteArray &p_read_byte_array,
			const int p_read_size,
			PackedVector2Array p_write_vec2_array);

	Ref<SpeechDecoder> get_speech_decoder() {
		if (opus_codec) {
			return opus_codec->get_speech_decoder();
		} else {
			return NULL;
		}
	}

	void set_streaming_bus(const String &p_name);
	bool set_audio_input_stream_player(Node *p_audio_input_stream_player);

	void set_process_all(bool p_active);

	Dictionary get_stats() const;

	void _setup();
	void _update_stats();

	void _notification(int p_what);

	SpeechProcessor();
	~SpeechProcessor();
};

#endif // SPEECH_PROCESSOR_H
