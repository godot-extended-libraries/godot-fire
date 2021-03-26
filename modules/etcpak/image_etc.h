/*************************************************************************/
/*  image_etc.h                                                          */
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

#pragma once

#include "core/io/image.h"
#include "core/os/copymem.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "image_etc.h"

enum class EtcpakType {
	ETCPAK_TYPE_ETC1,
	ETCPAK_TYPE_ETC2,
	ETCPAK_TYPE_ETC2_ALPHA,
	ETCPAK_TYPE_ETC2_RA_AS_RG,
	ETCPAK_TYPE_DXT1,
	ETCPAK_TYPE_DXT5,
	ETCPAK_TYPE_DXT5_RA_AS_RG,
};

void _compress_etcpak(EtcpakType p_compresstype, Image *p_img, float p_lossy_quality, bool force_etc1_format, Image::UsedChannels p_channels);
void _compress_etc1(Image *p_img, float p_lossy_quality);
void _compress_etc2(Image *p_img, float p_lossy_quality, Image::UsedChannels p_source);
void _compress_bc(Image *p_img, float p_lossy_quality, Image::UsedChannels p_source);
void _register_etc_compress_func();
