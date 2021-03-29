/*************************************************************************/
/*  cmark_gfm.h                                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
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

#ifndef CMARK_GFM_H
#define CMARK_GFM_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "modules/cmark_gfm/config.h"
#include "modules/cmark_gfm/cmark-gfm_export.h"
#include "thirdparty/cmark-gfm/src/cmark-gfm.h"
#include "thirdparty/cmark-gfm/src/buffer.h"
#include "thirdparty/cmark-gfm/src/node.h"
#include "thirdparty/cmark-gfm/src/cmark-gfm-extension_api.h"
#include "thirdparty/cmark-gfm/src/syntax_extension.h"
#include "thirdparty/cmark-gfm/src/parser.h"
#include "thirdparty/cmark-gfm/src/registry.h"
#include "thirdparty/cmark-gfm/src/extensions/cmark-gfm-core-extensions.h"

#endif