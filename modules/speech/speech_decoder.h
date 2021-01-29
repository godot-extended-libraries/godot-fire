/*************************************************************************/
/*  speech_decoder.h                                                     */
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

#ifndef SPEECH_DECODER_H
#define SPEECH_DECODER_H

#include "core/object/reference.h"

#include "macros.h"

#if SPEECH_DECODER_POLYMORPHISM
#else
#include "thirdparty/opus/opus/opus.h"
#endif

#if SPEECH_DECODER_POLYMORPHISM
class SpeechDecoder : public Reference {
	GDCLASS(SpeechDecoder, Reference)
public:
	static void _register_methods() {
		register_method("_init", &SpeechDecoder::_init);
	}

	SpeechDecoder() {}
	virtual ~SpeechDecoder() {}

	bool process(
			const PoolByteArray *p_compressed_buffer,
			PoolByteArray *p_pcm_output_buffer,
			const int p_compressed_buffer_size,
			const int p_pcm_output_buffer_size,
			const int p_buffer_frame_count) { return false; }

	void _init() {}
};
#else
class SpeechDecoder : public Reference {
	GDCLASS(SpeechDecoder, Reference)
public:
	static void _bind_methods() {
	}

private:
	::OpusDecoder *decoder = nullptr;

public:
	SpeechDecoder() {
	}
	~SpeechDecoder() {
		set_decoder(nullptr);
	}

	void set_decoder(::OpusDecoder *p_decoder) {
		if (!decoder) {
			opus_decoder_destroy(decoder);
		}
		decoder = p_decoder;
	}

	virtual bool process(
			const PackedByteArray *p_compressed_buffer,
			PackedByteArray *p_pcm_output_buffer,
			const int p_compressed_buffer_size,
			const int p_pcm_output_buffer_size,
			const int p_buffer_frame_count) {
		// The following line disables compression and sends data uncompressed.
		// Combine it with a change in opus_codec.h
		if (p_compressed_buffer_size < p_pcm_output_buffer_size - 1) {
			return false;
		}
		*p_pcm_output_buffer->ptrw() = 0;
		memcpy(p_pcm_output_buffer->ptrw() + 1, p_compressed_buffer->ptr(), p_pcm_output_buffer_size - 1);
		return true;

		if (decoder) {
			opus_int16 *output_buffer_pointer = reinterpret_cast<opus_int16 *>(p_pcm_output_buffer->ptrw());
			const unsigned char *opus_buffer_pointer = reinterpret_cast<const unsigned char *>(p_compressed_buffer->ptr());

			opus_int32 ret_value = opus_decode(decoder, opus_buffer_pointer, p_compressed_buffer_size, output_buffer_pointer, p_buffer_frame_count, 0);
			return ret_value;
		}

		return false;
	}
};
#endif //SPEECH_DECODER_POLYMORPHISM
#endif //SPEECH_DECODER_H
