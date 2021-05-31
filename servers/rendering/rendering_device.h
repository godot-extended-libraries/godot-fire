/*************************************************************************/
/*  rendering_device.h                                                   */
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

#ifndef RENDERING_DEVICE_H
#define RENDERING_DEVICE_H

#include "core/object/class_db.h"
#include "core/variant/typed_array.h"
#include "servers/display_server.h"

class RDTextureFormat;
class RDTextureView;
class RDAttachmentFormat;
class RDSamplerState;
class RDVertexAttribute;
class RDShaderSource;
class RDShaderBytecode;
class RDUniforms;
class RDPipelineRasterizationState;
class RDPipelineMultisampleState;
class RDPipelineDepthStencilState;
class RDPipelineColorBlendState;

class RenderingDevice : public Object {
	GDCLASS(RenderingDevice, Object)
public:
	enum DeviceFamily {
		DEVICE_UNKNOWN,
		DEVICE_OPENGL,
		DEVICE_VULKAN,
		DEVICE_DIRECTX
	};

	enum ShaderStage {
		SHADER_STAGE_VERTEX,
		SHADER_STAGE_FRAGMENT,
		SHADER_STAGE_TESSELATION_CONTROL,
		SHADER_STAGE_TESSELATION_EVALUATION,
		SHADER_STAGE_COMPUTE,
		SHADER_STAGE_MAX,
		SHADER_STAGE_VERTEX_BIT = (1 << SHADER_STAGE_VERTEX),
		SHADER_STAGE_FRAGMENT_BIT = (1 << SHADER_STAGE_FRAGMENT),
		SHADER_STAGE_TESSELATION_CONTROL_BIT = (1 << SHADER_STAGE_TESSELATION_CONTROL),
		SHADER_STAGE_TESSELATION_EVALUATION_BIT = (1 << SHADER_STAGE_TESSELATION_EVALUATION),
		SHADER_STAGE_COMPUTE_BIT = (1 << SHADER_STAGE_COMPUTE),
	};

	enum ShaderLanguage {
		SHADER_LANGUAGE_GLSL,
		SHADER_LANGUAGE_HLSL
	};

	enum SubgroupOperations {
		SUBGROUP_BASIC_BIT = 1,
		SUBGROUP_VOTE_BIT = 2,
		SUBGROUP_ARITHMETIC_BIT = 4,
		SUBGROUP_BALLOT_BIT = 8,
		SUBGROUP_SHUFFLE_BIT = 16,
		SUBGROUP_SHUFFLE_RELATIVE_BIT = 32,
		SUBGROUP_CLUSTERED_BIT = 64,
		SUBGROUP_QUAD_BIT = 128,
	};

	struct Capabilities {
		// main device info
		DeviceFamily device_family = DEVICE_UNKNOWN;
		uint32_t version_major = 1.0;
		uint32_t version_minor = 0.0;

		// subgroup capabilities
		uint32_t subgroup_size = 0;
		uint32_t subgroup_in_shaders = 0; // Set flags using SHADER_STAGE_VERTEX_BIT, SHADER_STAGE_FRAGMENT_BIT, etc.
		uint32_t subgroup_operations = 0; // Set flags, using SubgroupOperations

		// features
		bool supports_multiview = false; // If true this device supports multiview options
	};

	typedef String (*ShaderGetCacheKeyFunction)(const Capabilities *p_capabilities);
	typedef Vector<uint8_t> (*ShaderCompileFunction)(ShaderStage p_stage, const String &p_source_code, ShaderLanguage p_language, String *r_error, const Capabilities *p_capabilities);
	typedef Vector<uint8_t> (*ShaderCacheFunction)(ShaderStage p_stage, const String &p_source_code, ShaderLanguage p_language);

private:
	static ShaderCompileFunction compile_function;
	static ShaderCacheFunction cache_function;
	static ShaderGetCacheKeyFunction get_cache_key_function;

	static RenderingDevice *singleton;

protected:
	static void _bind_methods();

	Capabilities device_capabilities;

public:
	//base numeric ID for all types
	enum {
		INVALID_ID = -1,
		INVALID_FORMAT_ID = -1
	};

	/*****************/
	/**** GENERIC ****/
	/*****************/

	enum CompareOperator {
		COMPARE_OP_NEVER,
		COMPARE_OP_LESS,
		COMPARE_OP_EQUAL,
		COMPARE_OP_LESS_OR_EQUAL,
		COMPARE_OP_GREATER,
		COMPARE_OP_NOT_EQUAL,
		COMPARE_OP_GREATER_OR_EQUAL,
		COMPARE_OP_ALWAYS,
		COMPARE_OP_MAX //not an actual operator, just the amount of operators :D
	};

	enum DataFormat {
		DATA_FORMAT_R4G4_UNORM_PACK8,
		DATA_FORMAT_R4G4B4A4_UNORM_PACK16,
		DATA_FORMAT_B4G4R4A4_UNORM_PACK16,
		DATA_FORMAT_R5G6B5_UNORM_PACK16,
		DATA_FORMAT_B5G6R5_UNORM_PACK16,
		DATA_FORMAT_R5G5B5A1_UNORM_PACK16,
		DATA_FORMAT_B5G5R5A1_UNORM_PACK16,
		DATA_FORMAT_A1R5G5B5_UNORM_PACK16,
		DATA_FORMAT_R8_UNORM,
		DATA_FORMAT_R8_SNORM,
		DATA_FORMAT_R8_USCALED,
		DATA_FORMAT_R8_SSCALED,
		DATA_FORMAT_R8_UINT,
		DATA_FORMAT_R8_SINT,
		DATA_FORMAT_R8_SRGB,
		DATA_FORMAT_R8G8_UNORM,
		DATA_FORMAT_R8G8_SNORM,
		DATA_FORMAT_R8G8_USCALED,
		DATA_FORMAT_R8G8_SSCALED,
		DATA_FORMAT_R8G8_UINT,
		DATA_FORMAT_R8G8_SINT,
		DATA_FORMAT_R8G8_SRGB,
		DATA_FORMAT_R8G8B8_UNORM,
		DATA_FORMAT_R8G8B8_SNORM,
		DATA_FORMAT_R8G8B8_USCALED,
		DATA_FORMAT_R8G8B8_SSCALED,
		DATA_FORMAT_R8G8B8_UINT,
		DATA_FORMAT_R8G8B8_SINT,
		DATA_FORMAT_R8G8B8_SRGB,
		DATA_FORMAT_B8G8R8_UNORM,
		DATA_FORMAT_B8G8R8_SNORM,
		DATA_FORMAT_B8G8R8_USCALED,
		DATA_FORMAT_B8G8R8_SSCALED,
		DATA_FORMAT_B8G8R8_UINT,
		DATA_FORMAT_B8G8R8_SINT,
		DATA_FORMAT_B8G8R8_SRGB,
		DATA_FORMAT_R8G8B8A8_UNORM,
		DATA_FORMAT_R8G8B8A8_SNORM,
		DATA_FORMAT_R8G8B8A8_USCALED,
		DATA_FORMAT_R8G8B8A8_SSCALED,
		DATA_FORMAT_R8G8B8A8_UINT,
		DATA_FORMAT_R8G8B8A8_SINT,
		DATA_FORMAT_R8G8B8A8_SRGB,
		DATA_FORMAT_B8G8R8A8_UNORM,
		DATA_FORMAT_B8G8R8A8_SNORM,
		DATA_FORMAT_B8G8R8A8_USCALED,
		DATA_FORMAT_B8G8R8A8_SSCALED,
		DATA_FORMAT_B8G8R8A8_UINT,
		DATA_FORMAT_B8G8R8A8_SINT,
		DATA_FORMAT_B8G8R8A8_SRGB,
		DATA_FORMAT_A8B8G8R8_UNORM_PACK32,
		DATA_FORMAT_A8B8G8R8_SNORM_PACK32,
		DATA_FORMAT_A8B8G8R8_USCALED_PACK32,
		DATA_FORMAT_A8B8G8R8_SSCALED_PACK32,
		DATA_FORMAT_A8B8G8R8_UINT_PACK32,
		DATA_FORMAT_A8B8G8R8_SINT_PACK32,
		DATA_FORMAT_A8B8G8R8_SRGB_PACK32,
		DATA_FORMAT_A2R10G10B10_UNORM_PACK32,
		DATA_FORMAT_A2R10G10B10_SNORM_PACK32,
		DATA_FORMAT_A2R10G10B10_USCALED_PACK32,
		DATA_FORMAT_A2R10G10B10_SSCALED_PACK32,
		DATA_FORMAT_A2R10G10B10_UINT_PACK32,
		DATA_FORMAT_A2R10G10B10_SINT_PACK32,
		DATA_FORMAT_A2B10G10R10_UNORM_PACK32,
		DATA_FORMAT_A2B10G10R10_SNORM_PACK32,
		DATA_FORMAT_A2B10G10R10_USCALED_PACK32,
		DATA_FORMAT_A2B10G10R10_SSCALED_PACK32,
		DATA_FORMAT_A2B10G10R10_UINT_PACK32,
		DATA_FORMAT_A2B10G10R10_SINT_PACK32,
		DATA_FORMAT_R16_UNORM,
		DATA_FORMAT_R16_SNORM,
		DATA_FORMAT_R16_USCALED,
		DATA_FORMAT_R16_SSCALED,
		DATA_FORMAT_R16_UINT,
		DATA_FORMAT_R16_SINT,
		DATA_FORMAT_R16_SFLOAT,
		DATA_FORMAT_R16G16_UNORM,
		DATA_FORMAT_R16G16_SNORM,
		DATA_FORMAT_R16G16_USCALED,
		DATA_FORMAT_R16G16_SSCALED,
		DATA_FORMAT_R16G16_UINT,
		DATA_FORMAT_R16G16_SINT,
		DATA_FORMAT_R16G16_SFLOAT,
		DATA_FORMAT_R16G16B16_UNORM,
		DATA_FORMAT_R16G16B16_SNORM,
		DATA_FORMAT_R16G16B16_USCALED,
		DATA_FORMAT_R16G16B16_SSCALED,
		DATA_FORMAT_R16G16B16_UINT,
		DATA_FORMAT_R16G16B16_SINT,
		DATA_FORMAT_R16G16B16_SFLOAT,
		DATA_FORMAT_R16G16B16A16_UNORM,
		DATA_FORMAT_R16G16B16A16_SNORM,
		DATA_FORMAT_R16G16B16A16_USCALED,
		DATA_FORMAT_R16G16B16A16_SSCALED,
		DATA_FORMAT_R16G16B16A16_UINT,
		DATA_FORMAT_R16G16B16A16_SINT,
		DATA_FORMAT_R16G16B16A16_SFLOAT,
		DATA_FORMAT_R32_UINT,
		DATA_FORMAT_R32_SINT,
		DATA_FORMAT_R32_SFLOAT,
		DATA_FORMAT_R32G32_UINT,
		DATA_FORMAT_R32G32_SINT,
		DATA_FORMAT_R32G32_SFLOAT,
		DATA_FORMAT_R32G32B32_UINT,
		DATA_FORMAT_R32G32B32_SINT,
		DATA_FORMAT_R32G32B32_SFLOAT,
		DATA_FORMAT_R32G32B32A32_UINT,
		DATA_FORMAT_R32G32B32A32_SINT,
		DATA_FORMAT_R32G32B32A32_SFLOAT,
		DATA_FORMAT_R64_UINT,
		DATA_FORMAT_R64_SINT,
		DATA_FORMAT_R64_SFLOAT,
		DATA_FORMAT_R64G64_UINT,
		DATA_FORMAT_R64G64_SINT,
		DATA_FORMAT_R64G64_SFLOAT,
		DATA_FORMAT_R64G64B64_UINT,
		DATA_FORMAT_R64G64B64_SINT,
		DATA_FORMAT_R64G64B64_SFLOAT,
		DATA_FORMAT_R64G64B64A64_UINT,
		DATA_FORMAT_R64G64B64A64_SINT,
		DATA_FORMAT_R64G64B64A64_SFLOAT,
		DATA_FORMAT_B10G11R11_UFLOAT_PACK32,
		DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32,
		DATA_FORMAT_D16_UNORM,
		DATA_FORMAT_X8_D24_UNORM_PACK32,
		DATA_FORMAT_D32_SFLOAT,
		DATA_FORMAT_S8_UINT,
		DATA_FORMAT_D16_UNORM_S8_UINT,
		DATA_FORMAT_D24_UNORM_S8_UINT,
		DATA_FORMAT_D32_SFLOAT_S8_UINT,
		DATA_FORMAT_BC1_RGB_UNORM_BLOCK,
		DATA_FORMAT_BC1_RGB_SRGB_BLOCK,
		DATA_FORMAT_BC1_RGBA_UNORM_BLOCK,
		DATA_FORMAT_BC1_RGBA_SRGB_BLOCK,
		DATA_FORMAT_BC2_UNORM_BLOCK,
		DATA_FORMAT_BC2_SRGB_BLOCK,
		DATA_FORMAT_BC3_UNORM_BLOCK,
		DATA_FORMAT_BC3_SRGB_BLOCK,
		DATA_FORMAT_BC4_UNORM_BLOCK,
		DATA_FORMAT_BC4_SNORM_BLOCK,
		DATA_FORMAT_BC5_UNORM_BLOCK,
		DATA_FORMAT_BC5_SNORM_BLOCK,
		DATA_FORMAT_BC6H_UFLOAT_BLOCK,
		DATA_FORMAT_BC6H_SFLOAT_BLOCK,
		DATA_FORMAT_BC7_UNORM_BLOCK,
		DATA_FORMAT_BC7_SRGB_BLOCK,
		DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
		DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
		DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
		DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
		DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
		DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
		DATA_FORMAT_EAC_R11_UNORM_BLOCK,
		DATA_FORMAT_EAC_R11_SNORM_BLOCK,
		DATA_FORMAT_EAC_R11G11_UNORM_BLOCK,
		DATA_FORMAT_EAC_R11G11_SNORM_BLOCK,
		DATA_FORMAT_ASTC_4x4_UNORM_BLOCK,
		DATA_FORMAT_ASTC_4x4_SRGB_BLOCK,
		DATA_FORMAT_ASTC_5x4_UNORM_BLOCK,
		DATA_FORMAT_ASTC_5x4_SRGB_BLOCK,
		DATA_FORMAT_ASTC_5x5_UNORM_BLOCK,
		DATA_FORMAT_ASTC_5x5_SRGB_BLOCK,
		DATA_FORMAT_ASTC_6x5_UNORM_BLOCK,
		DATA_FORMAT_ASTC_6x5_SRGB_BLOCK,
		DATA_FORMAT_ASTC_6x6_UNORM_BLOCK,
		DATA_FORMAT_ASTC_6x6_SRGB_BLOCK,
		DATA_FORMAT_ASTC_8x5_UNORM_BLOCK,
		DATA_FORMAT_ASTC_8x5_SRGB_BLOCK,
		DATA_FORMAT_ASTC_8x6_UNORM_BLOCK,
		DATA_FORMAT_ASTC_8x6_SRGB_BLOCK,
		DATA_FORMAT_ASTC_8x8_UNORM_BLOCK,
		DATA_FORMAT_ASTC_8x8_SRGB_BLOCK,
		DATA_FORMAT_ASTC_10x5_UNORM_BLOCK,
		DATA_FORMAT_ASTC_10x5_SRGB_BLOCK,
		DATA_FORMAT_ASTC_10x6_UNORM_BLOCK,
		DATA_FORMAT_ASTC_10x6_SRGB_BLOCK,
		DATA_FORMAT_ASTC_10x8_UNORM_BLOCK,
		DATA_FORMAT_ASTC_10x8_SRGB_BLOCK,
		DATA_FORMAT_ASTC_10x10_UNORM_BLOCK,
		DATA_FORMAT_ASTC_10x10_SRGB_BLOCK,
		DATA_FORMAT_ASTC_12x10_UNORM_BLOCK,
		DATA_FORMAT_ASTC_12x10_SRGB_BLOCK,
		DATA_FORMAT_ASTC_12x12_UNORM_BLOCK,
		DATA_FORMAT_ASTC_12x12_SRGB_BLOCK,
		DATA_FORMAT_G8B8G8R8_422_UNORM,
		DATA_FORMAT_B8G8R8G8_422_UNORM,
		DATA_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
		DATA_FORMAT_G8_B8R8_2PLANE_420_UNORM,
		DATA_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
		DATA_FORMAT_G8_B8R8_2PLANE_422_UNORM,
		DATA_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
		DATA_FORMAT_R10X6_UNORM_PACK16,
		DATA_FORMAT_R10X6G10X6_UNORM_2PACK16,
		DATA_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
		DATA_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
		DATA_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
		DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
		DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
		DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
		DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
		DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
		DATA_FORMAT_R12X4_UNORM_PACK16,
		DATA_FORMAT_R12X4G12X4_UNORM_2PACK16,
		DATA_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
		DATA_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
		DATA_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
		DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
		DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
		DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
		DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
		DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
		DATA_FORMAT_G16B16G16R16_422_UNORM,
		DATA_FORMAT_B16G16R16G16_422_UNORM,
		DATA_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
		DATA_FORMAT_G16_B16R16_2PLANE_420_UNORM,
		DATA_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
		DATA_FORMAT_G16_B16R16_2PLANE_422_UNORM,
		DATA_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
		DATA_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
		DATA_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,
		DATA_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,
		DATA_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,
		DATA_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,
		DATA_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
		DATA_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,
		DATA_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,
		DATA_FORMAT_MAX
	};

	/*****************/
	/**** BARRIER ****/
	/*****************/

	enum BarrierMask {
		BARRIER_MASK_RASTER = 1,
		BARRIER_MASK_COMPUTE = 2,
		BARRIER_MASK_TRANSFER = 4,
		BARRIER_MASK_NO_BARRIER = 8,
		BARRIER_MASK_ALL = BARRIER_MASK_RASTER | BARRIER_MASK_COMPUTE | BARRIER_MASK_TRANSFER
	};

	/*****************/
	/**** TEXTURE ****/
	/*****************/

	enum TextureType {
		TEXTURE_TYPE_1D,
		TEXTURE_TYPE_2D,
		TEXTURE_TYPE_3D,
		TEXTURE_TYPE_CUBE,
		TEXTURE_TYPE_1D_ARRAY,
		TEXTURE_TYPE_2D_ARRAY,
		TEXTURE_TYPE_CUBE_ARRAY,
		TEXTURE_TYPE_MAX
	};

	enum TextureSamples {
		TEXTURE_SAMPLES_1,
		TEXTURE_SAMPLES_2,
		TEXTURE_SAMPLES_4,
		TEXTURE_SAMPLES_8,
		TEXTURE_SAMPLES_16,
		TEXTURE_SAMPLES_32,
		TEXTURE_SAMPLES_64,
		TEXTURE_SAMPLES_MAX
	};

	enum TextureUsageBits {
		TEXTURE_USAGE_SAMPLING_BIT = (1 << 0),
		TEXTURE_USAGE_COLOR_ATTACHMENT_BIT = (1 << 1),
		TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = (1 << 2),
		TEXTURE_USAGE_STORAGE_BIT = (1 << 3),
		TEXTURE_USAGE_STORAGE_ATOMIC_BIT = (1 << 4),
		TEXTURE_USAGE_CPU_READ_BIT = (1 << 5),
		TEXTURE_USAGE_CAN_UPDATE_BIT = (1 << 6),
		TEXTURE_USAGE_CAN_COPY_FROM_BIT = (1 << 7),
		TEXTURE_USAGE_CAN_COPY_TO_BIT = (1 << 8),
		TEXTURE_USAGE_RESOLVE_ATTACHMENT_BIT = (1 << 9),
	};

	enum TextureSwizzle {
		TEXTURE_SWIZZLE_IDENTITY,
		TEXTURE_SWIZZLE_ZERO,
		TEXTURE_SWIZZLE_ONE,
		TEXTURE_SWIZZLE_R,
		TEXTURE_SWIZZLE_G,
		TEXTURE_SWIZZLE_B,
		TEXTURE_SWIZZLE_A,
		TEXTURE_SWIZZLE_MAX
	};

	struct TextureFormat {
		DataFormat format;
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t array_layers;
		uint32_t mipmaps;
		TextureType texture_type;
		TextureSamples samples;
		uint32_t usage_bits;
		Vector<DataFormat> shareable_formats;

		TextureFormat() {
			format = DATA_FORMAT_R8_UNORM;
			width = 1;
			height = 1;
			depth = 1;
			array_layers = 1;
			mipmaps = 1;
			texture_type = TEXTURE_TYPE_2D;
			samples = TEXTURE_SAMPLES_1;
			usage_bits = 0;
		}
	};

	struct TextureView {
		DataFormat format_override;
		TextureSwizzle swizzle_r;
		TextureSwizzle swizzle_g;
		TextureSwizzle swizzle_b;
		TextureSwizzle swizzle_a;

		TextureView() {
			format_override = DATA_FORMAT_MAX; //means, use same as format
			swizzle_r = TEXTURE_SWIZZLE_R;
			swizzle_g = TEXTURE_SWIZZLE_G;
			swizzle_b = TEXTURE_SWIZZLE_B;
			swizzle_a = TEXTURE_SWIZZLE_A;
		}
	};

	virtual RID texture_create(const TextureFormat &p_format, const TextureView &p_view, const Vector<Vector<uint8_t>> &p_data = Vector<Vector<uint8_t>>()) = 0;
	virtual RID texture_create_shared(const TextureView &p_view, RID p_with_texture) = 0;

	enum TextureSliceType {
		TEXTURE_SLICE_2D,
		TEXTURE_SLICE_CUBEMAP,
		TEXTURE_SLICE_3D,
		TEXTURE_SLICE_2D_ARRAY,
	};

	virtual RID texture_create_shared_from_slice(const TextureView &p_view, RID p_with_texture, uint32_t p_layer, uint32_t p_mipmap, TextureSliceType p_slice_type = TEXTURE_SLICE_2D) = 0;

	virtual Error texture_update(RID p_texture, uint32_t p_layer, const Vector<uint8_t> &p_data, uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;
	virtual Vector<uint8_t> texture_get_data(RID p_texture, uint32_t p_layer) = 0; // CPU textures will return immediately, while GPU textures will most likely force a flush

	virtual bool texture_is_format_supported_for_usage(DataFormat p_format, uint32_t p_usage) const = 0;
	virtual bool texture_is_shared(RID p_texture) = 0;
	virtual bool texture_is_valid(RID p_texture) = 0;

	virtual Error texture_copy(RID p_from_texture, RID p_to_texture, const Vector3 &p_from, const Vector3 &p_to, const Vector3 &p_size, uint32_t p_src_mipmap, uint32_t p_dst_mipmap, uint32_t p_src_layer, uint32_t p_dst_layer, uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;
	virtual Error texture_clear(RID p_texture, const Color &p_color, uint32_t p_base_mipmap, uint32_t p_mipmaps, uint32_t p_base_layer, uint32_t p_layers, uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;
	virtual Error texture_resolve_multisample(RID p_from_texture, RID p_to_texture, uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;

	/*********************/
	/**** FRAMEBUFFER ****/
	/*********************/

	struct AttachmentFormat {
		DataFormat format;
		TextureSamples samples;
		uint32_t usage_flags;
		AttachmentFormat() {
			format = DATA_FORMAT_R8G8B8A8_UNORM;
			samples = TEXTURE_SAMPLES_1;
			usage_flags = 0;
		}
	};

	typedef int64_t FramebufferFormatID;

	// This ID is warranted to be unique for the same formats, does not need to be freed
	virtual FramebufferFormatID framebuffer_format_create(const Vector<AttachmentFormat> &p_format, uint32_t p_view_count = 1) = 0;
	virtual FramebufferFormatID framebuffer_format_create_empty(TextureSamples p_samples = TEXTURE_SAMPLES_1) = 0;
	virtual TextureSamples framebuffer_format_get_texture_samples(FramebufferFormatID p_format) = 0;

	virtual RID framebuffer_create(const Vector<RID> &p_texture_attachments, FramebufferFormatID p_format_check = INVALID_ID, uint32_t p_view_count = 1) = 0;
	virtual RID framebuffer_create_empty(const Size2i &p_size, TextureSamples p_samples = TEXTURE_SAMPLES_1, FramebufferFormatID p_format_check = INVALID_ID) = 0;

	virtual FramebufferFormatID framebuffer_get_format(RID p_framebuffer) = 0;

	/*****************/
	/**** SAMPLER ****/
	/*****************/

	enum SamplerFilter {
		SAMPLER_FILTER_NEAREST,
		SAMPLER_FILTER_LINEAR,
	};

	enum SamplerRepeatMode {
		SAMPLER_REPEAT_MODE_REPEAT,
		SAMPLER_REPEAT_MODE_MIRRORED_REPEAT,
		SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE,
		SAMPLER_REPEAT_MODE_CLAMP_TO_BORDER,
		SAMPLER_REPEAT_MODE_MIRROR_CLAMP_TO_EDGE,
		SAMPLER_REPEAT_MODE_MAX
	};

	enum SamplerBorderColor {
		SAMPLER_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
		SAMPLER_BORDER_COLOR_INT_TRANSPARENT_BLACK,
		SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		SAMPLER_BORDER_COLOR_INT_OPAQUE_BLACK,
		SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		SAMPLER_BORDER_COLOR_INT_OPAQUE_WHITE,
		SAMPLER_BORDER_COLOR_MAX
	};

	struct SamplerState {
		SamplerFilter mag_filter;
		SamplerFilter min_filter;
		SamplerFilter mip_filter;
		SamplerRepeatMode repeat_u;
		SamplerRepeatMode repeat_v;
		SamplerRepeatMode repeat_w;
		float lod_bias;
		bool use_anisotropy;
		float anisotropy_max;
		bool enable_compare;
		CompareOperator compare_op;
		float min_lod;
		float max_lod;
		SamplerBorderColor border_color;
		bool unnormalized_uvw;

		SamplerState() {
			mag_filter = SAMPLER_FILTER_NEAREST;
			min_filter = SAMPLER_FILTER_NEAREST;
			mip_filter = SAMPLER_FILTER_NEAREST;
			repeat_u = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
			repeat_v = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
			repeat_w = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
			lod_bias = 0;
			use_anisotropy = false;
			anisotropy_max = 1.0;
			enable_compare = false;
			compare_op = COMPARE_OP_ALWAYS;
			min_lod = 0;
			max_lod = 1e20; //something very large should do
			border_color = SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			unnormalized_uvw = false;
		}
	};

	virtual RID sampler_create(const SamplerState &p_state) = 0;

	/**********************/
	/**** VERTEX ARRAY ****/
	/**********************/

	enum VertexFrequency {
		VERTEX_FREQUENCY_VERTEX,
		VERTEX_FREQUENCY_INSTANCE,
	};

	struct VertexAttribute {
		uint32_t location; //shader location
		uint32_t offset;
		DataFormat format;
		uint32_t stride;
		VertexFrequency frequency;
		VertexAttribute() {
			location = 0;
			offset = 0;
			stride = 0;
			format = DATA_FORMAT_MAX;
			frequency = VERTEX_FREQUENCY_VERTEX;
		}
	};
	virtual RID vertex_buffer_create(uint32_t p_size_bytes, const Vector<uint8_t> &p_data = Vector<uint8_t>(), bool p_use_as_storage = false) = 0;

	typedef int64_t VertexFormatID;

	// This ID is warranted to be unique for the same formats, does not need to be freed
	virtual VertexFormatID vertex_format_create(const Vector<VertexAttribute> &p_vertex_formats) = 0;
	virtual RID vertex_array_create(uint32_t p_vertex_count, VertexFormatID p_vertex_format, const Vector<RID> &p_src_buffers) = 0;

	enum IndexBufferFormat {
		INDEX_BUFFER_FORMAT_UINT16,
		INDEX_BUFFER_FORMAT_UINT32,
	};

	virtual RID index_buffer_create(uint32_t p_size_indices, IndexBufferFormat p_format, const Vector<uint8_t> &p_data = Vector<uint8_t>(), bool p_use_restart_indices = false) = 0;
	virtual RID index_array_create(RID p_index_buffer, uint32_t p_index_offset, uint32_t p_index_count) = 0;

	/****************/
	/**** SHADER ****/
	/****************/

	const Capabilities *get_device_capabilities() const { return &device_capabilities; };

	virtual Vector<uint8_t> shader_compile_from_source(ShaderStage p_stage, const String &p_source_code, ShaderLanguage p_language = SHADER_LANGUAGE_GLSL, String *r_error = nullptr, bool p_allow_cache = true);
	virtual String shader_get_cache_key() const;

	static void shader_set_compile_function(ShaderCompileFunction p_function);
	static void shader_set_cache_function(ShaderCacheFunction p_function);
	static void shader_set_get_cache_key_function(ShaderGetCacheKeyFunction p_function);

	struct ShaderStageData {
		ShaderStage shader_stage;
		Vector<uint8_t> spir_v;

		ShaderStageData() {
			shader_stage = SHADER_STAGE_VERTEX;
		}
	};

	RID shader_create_from_bytecode(const Ref<RDShaderBytecode> &p_bytecode);
	virtual RID shader_create(const Vector<ShaderStageData> &p_stages) = 0;
	virtual uint32_t shader_get_vertex_input_attribute_mask(RID p_shader) = 0;

	/******************/
	/**** UNIFORMS ****/
	/******************/

	enum UniformType {
		UNIFORM_TYPE_SAMPLER, //for sampling only (sampler GLSL type)
		UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, // for sampling only, but includes a texture, (samplerXX GLSL type), first a sampler then a texture
		UNIFORM_TYPE_TEXTURE, //only texture, (textureXX GLSL type)
		UNIFORM_TYPE_IMAGE, // storage image (imageXX GLSL type), for compute mostly
		UNIFORM_TYPE_TEXTURE_BUFFER, // buffer texture (or TBO, textureBuffer type)
		UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER, // buffer texture with a sampler(or TBO, samplerBuffer type)
		UNIFORM_TYPE_IMAGE_BUFFER, //texel buffer, (imageBuffer type), for compute mostly
		UNIFORM_TYPE_UNIFORM_BUFFER, //regular uniform buffer (or UBO).
		UNIFORM_TYPE_STORAGE_BUFFER, //storage buffer ("buffer" qualifier) like UBO, but supports storage, for compute mostly
		UNIFORM_TYPE_INPUT_ATTACHMENT, //used for sub-pass read/write, for mobile mostly
		UNIFORM_TYPE_MAX
	};

	enum StorageBufferUsage {
		STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT = 1
	};

	virtual RID uniform_buffer_create(uint32_t p_size_bytes, const Vector<uint8_t> &p_data = Vector<uint8_t>()) = 0;
	virtual RID storage_buffer_create(uint32_t p_size, const Vector<uint8_t> &p_data = Vector<uint8_t>(), uint32_t p_usage = 0) = 0;
	virtual RID texture_buffer_create(uint32_t p_size_elements, DataFormat p_format, const Vector<uint8_t> &p_data = Vector<uint8_t>()) = 0;

	struct Uniform {
		UniformType uniform_type;
		int binding; //binding index as specified in shader

		//for single items, provide one ID, for
		//multiple items (declared as arrays in shader),
		//provide more
		//for sampler with texture, supply two IDs for each.
		//accepted IDs are: Sampler, Texture, Uniform Buffer and Texture Buffer
		Vector<RID> ids;

		Uniform() {
			uniform_type = UNIFORM_TYPE_IMAGE;
			binding = 0;
		}
	};

	virtual RID uniform_set_create(const Vector<Uniform> &p_uniforms, RID p_shader, uint32_t p_shader_set) = 0;
	virtual bool uniform_set_is_valid(RID p_uniform_set) = 0;

	virtual Error buffer_update(RID p_buffer, uint32_t p_offset, uint32_t p_size, const void *p_data, uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;
	virtual Error buffer_clear(RID p_buffer, uint32_t p_offset, uint32_t p_size, uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;
	virtual Vector<uint8_t> buffer_get_data(RID p_buffer) = 0; //this causes stall, only use to retrieve large buffers for saving

	/*************************/
	/**** RENDER PIPELINE ****/
	/*************************/

	enum RenderPrimitive {
		RENDER_PRIMITIVE_POINTS,
		RENDER_PRIMITIVE_LINES,
		RENDER_PRIMITIVE_LINES_WITH_ADJACENCY,
		RENDER_PRIMITIVE_LINESTRIPS,
		RENDER_PRIMITIVE_LINESTRIPS_WITH_ADJACENCY,
		RENDER_PRIMITIVE_TRIANGLES,
		RENDER_PRIMITIVE_TRIANGLES_WITH_ADJACENCY,
		RENDER_PRIMITIVE_TRIANGLE_STRIPS,
		RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_AJACENCY,
		RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_RESTART_INDEX,
		RENDER_PRIMITIVE_TESSELATION_PATCH,
		RENDER_PRIMITIVE_MAX
	};

	//disable optimization, tessellate control points

	enum PolygonCullMode {
		POLYGON_CULL_DISABLED,
		POLYGON_CULL_FRONT,
		POLYGON_CULL_BACK,
	};

	enum PolygonFrontFace {
		POLYGON_FRONT_FACE_CLOCKWISE,
		POLYGON_FRONT_FACE_COUNTER_CLOCKWISE,
	};

	enum StencilOperation {
		STENCIL_OP_KEEP,
		STENCIL_OP_ZERO,
		STENCIL_OP_REPLACE,
		STENCIL_OP_INCREMENT_AND_CLAMP,
		STENCIL_OP_DECREMENT_AND_CLAMP,
		STENCIL_OP_INVERT,
		STENCIL_OP_INCREMENT_AND_WRAP,
		STENCIL_OP_DECREMENT_AND_WRAP,
		STENCIL_OP_MAX //not an actual operator, just the amount of operators :D
	};

	enum LogicOperation {
		LOGIC_OP_CLEAR,
		LOGIC_OP_AND,
		LOGIC_OP_AND_REVERSE,
		LOGIC_OP_COPY,
		LOGIC_OP_AND_INVERTED,
		LOGIC_OP_NO_OP,
		LOGIC_OP_XOR,
		LOGIC_OP_OR,
		LOGIC_OP_NOR,
		LOGIC_OP_EQUIVALENT,
		LOGIC_OP_INVERT,
		LOGIC_OP_OR_REVERSE,
		LOGIC_OP_COPY_INVERTED,
		LOGIC_OP_OR_INVERTED,
		LOGIC_OP_NAND,
		LOGIC_OP_SET,
		LOGIC_OP_MAX //not an actual operator, just the amount of operators :D
	};

	enum BlendFactor {
		BLEND_FACTOR_ZERO,
		BLEND_FACTOR_ONE,
		BLEND_FACTOR_SRC_COLOR,
		BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		BLEND_FACTOR_DST_COLOR,
		BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		BLEND_FACTOR_SRC_ALPHA,
		BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		BLEND_FACTOR_DST_ALPHA,
		BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		BLEND_FACTOR_CONSTANT_COLOR,
		BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		BLEND_FACTOR_CONSTANT_ALPHA,
		BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
		BLEND_FACTOR_SRC_ALPHA_SATURATE,
		BLEND_FACTOR_SRC1_COLOR,
		BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
		BLEND_FACTOR_SRC1_ALPHA,
		BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
		BLEND_FACTOR_MAX
	};

	enum BlendOperation {
		BLEND_OP_ADD,
		BLEND_OP_SUBTRACT,
		BLEND_OP_REVERSE_SUBTRACT,
		BLEND_OP_MINIMUM,
		BLEND_OP_MAXIMUM, //yes this one is an actual operator
		BLEND_OP_MAX //not an actual operator, just the amount of operators :D
	};

	struct PipelineRasterizationState {
		bool enable_depth_clamp;
		bool discard_primitives;
		bool wireframe;
		PolygonCullMode cull_mode;
		PolygonFrontFace front_face;
		bool depth_bias_enable;
		float depth_bias_constant_factor;
		float depth_bias_clamp;
		float depth_bias_slope_factor;
		float line_width;
		uint32_t patch_control_points;
		PipelineRasterizationState() {
			enable_depth_clamp = false;
			discard_primitives = false;
			wireframe = false;
			cull_mode = POLYGON_CULL_DISABLED;
			front_face = POLYGON_FRONT_FACE_CLOCKWISE;
			depth_bias_enable = false;
			depth_bias_constant_factor = 0;
			depth_bias_clamp = 0;
			depth_bias_slope_factor = 0;
			line_width = 1.0;
			patch_control_points = 1;
		}
	};

	struct PipelineMultisampleState {
		TextureSamples sample_count;
		bool enable_sample_shading;
		float min_sample_shading;
		Vector<uint32_t> sample_mask;
		bool enable_alpha_to_coverage;
		bool enable_alpha_to_one;

		PipelineMultisampleState() {
			sample_count = TEXTURE_SAMPLES_1;
			enable_sample_shading = false;
			min_sample_shading = 0;
			enable_alpha_to_coverage = false;
			enable_alpha_to_one = false;
		}
	};

	struct PipelineDepthStencilState {
		bool enable_depth_test;
		bool enable_depth_write;
		CompareOperator depth_compare_operator;
		bool enable_depth_range;
		float depth_range_min;
		float depth_range_max;
		bool enable_stencil;

		struct StencilOperationState {
			StencilOperation fail;
			StencilOperation pass;
			StencilOperation depth_fail;
			CompareOperator compare;
			uint32_t compare_mask;
			uint32_t write_mask;
			uint32_t reference;

			StencilOperationState() {
				fail = STENCIL_OP_ZERO;
				pass = STENCIL_OP_ZERO;
				depth_fail = STENCIL_OP_ZERO;
				compare = COMPARE_OP_ALWAYS;
				compare_mask = 0;
				write_mask = 0;
				reference = 0;
			}
		};

		StencilOperationState front_op;
		StencilOperationState back_op;

		PipelineDepthStencilState() {
			enable_depth_test = false;
			enable_depth_write = false;
			depth_compare_operator = COMPARE_OP_ALWAYS;
			enable_depth_range = false;
			depth_range_min = 0;
			depth_range_max = 0;
			enable_stencil = false;
		}
	};

	struct PipelineColorBlendState {
		bool enable_logic_op;
		LogicOperation logic_op;
		struct Attachment {
			bool enable_blend;
			BlendFactor src_color_blend_factor;
			BlendFactor dst_color_blend_factor;
			BlendOperation color_blend_op;
			BlendFactor src_alpha_blend_factor;
			BlendFactor dst_alpha_blend_factor;
			BlendOperation alpha_blend_op;
			bool write_r;
			bool write_g;
			bool write_b;
			bool write_a;
			Attachment() {
				enable_blend = false;
				src_color_blend_factor = BLEND_FACTOR_ZERO;
				dst_color_blend_factor = BLEND_FACTOR_ZERO;
				color_blend_op = BLEND_OP_ADD;
				src_alpha_blend_factor = BLEND_FACTOR_ZERO;
				dst_alpha_blend_factor = BLEND_FACTOR_ZERO;
				alpha_blend_op = BLEND_OP_ADD;
				write_r = true;
				write_g = true;
				write_b = true;
				write_a = true;
			}
		};

		static PipelineColorBlendState create_disabled(int p_attachments = 1) {
			PipelineColorBlendState bs;
			for (int i = 0; i < p_attachments; i++) {
				bs.attachments.push_back(Attachment());
			}
			return bs;
		}

		static PipelineColorBlendState create_blend(int p_attachments = 1) {
			PipelineColorBlendState bs;
			for (int i = 0; i < p_attachments; i++) {
				Attachment ba;
				ba.enable_blend = true;
				ba.src_color_blend_factor = BLEND_FACTOR_SRC_ALPHA;
				ba.dst_color_blend_factor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				ba.src_alpha_blend_factor = BLEND_FACTOR_SRC_ALPHA;
				ba.dst_alpha_blend_factor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

				bs.attachments.push_back(ba);
			}
			return bs;
		}

		Vector<Attachment> attachments; //one per render target texture
		Color blend_constant;

		PipelineColorBlendState() {
			enable_logic_op = false;
			logic_op = LOGIC_OP_CLEAR;
		}
	};

	enum PipelineDynamicStateFlags {
		DYNAMIC_STATE_LINE_WIDTH = (1 << 0),
		DYNAMIC_STATE_DEPTH_BIAS = (1 << 1),
		DYNAMIC_STATE_BLEND_CONSTANTS = (1 << 2),
		DYNAMIC_STATE_DEPTH_BOUNDS = (1 << 3),
		DYNAMIC_STATE_STENCIL_COMPARE_MASK = (1 << 4),
		DYNAMIC_STATE_STENCIL_WRITE_MASK = (1 << 5),
		DYNAMIC_STATE_STENCIL_REFERENCE = (1 << 6),
	};

	virtual bool render_pipeline_is_valid(RID p_pipeline) = 0;
	virtual RID render_pipeline_create(RID p_shader, FramebufferFormatID p_framebuffer_format, VertexFormatID p_vertex_format, RenderPrimitive p_render_primitive, const PipelineRasterizationState &p_rasterization_state, const PipelineMultisampleState &p_multisample_state, const PipelineDepthStencilState &p_depth_stencil_state, const PipelineColorBlendState &p_blend_state, int p_dynamic_state_flags = 0) = 0;

	/**************************/
	/**** COMPUTE PIPELINE ****/
	/**************************/

	virtual RID compute_pipeline_create(RID p_shader) = 0;
	virtual bool compute_pipeline_is_valid(RID p_pipeline) = 0;

	/****************/
	/**** SCREEN ****/
	/****************/

	virtual int screen_get_width(DisplayServer::WindowID p_screen = 0) const = 0;
	virtual int screen_get_height(DisplayServer::WindowID p_screen = 0) const = 0;
	virtual FramebufferFormatID screen_get_framebuffer_format() const = 0;

	/********************/
	/**** DRAW LISTS ****/
	/********************/

	enum InitialAction {
		INITIAL_ACTION_CLEAR, //start rendering and clear the whole framebuffer (region or not) (supply params)
		INITIAL_ACTION_CLEAR_REGION, //start rendering and clear the framebuffer in the specified region (supply params)
		INITIAL_ACTION_CLEAR_REGION_CONTINUE, //continue rendering and clear the framebuffer in the specified region (supply params)
		INITIAL_ACTION_KEEP, //start rendering, but keep attached color texture contents (depth will be cleared)
		INITIAL_ACTION_DROP, //start rendering, ignore what is there, just write above it
		INITIAL_ACTION_CONTINUE, //continue rendering (framebuffer must have been left in "continue" state as final action previously)
		INITIAL_ACTION_MAX
	};

	enum FinalAction {
		FINAL_ACTION_READ, //will no longer render to it, allows attached textures to be read again, but depth buffer contents will be dropped (Can't be read from)
		FINAL_ACTION_DISCARD, // discard contents after rendering
		FINAL_ACTION_CONTINUE, //will continue rendering later, attached textures can't be read until re-bound with "finish"
		FINAL_ACTION_MAX
	};

	typedef int64_t DrawListID;

	virtual DrawListID draw_list_begin_for_screen(DisplayServer::WindowID p_screen = 0, const Color &p_clear_color = Color()) = 0;
	virtual DrawListID draw_list_begin(RID p_framebuffer, InitialAction p_initial_color_action, FinalAction p_final_color_action, InitialAction p_initial_depth_action, FinalAction p_final_depth_action, const Vector<Color> &p_clear_color_values = Vector<Color>(), float p_clear_depth = 1.0, uint32_t p_clear_stencil = 0, const Rect2 &p_region = Rect2(), const Vector<RID> &p_storage_textures = Vector<RID>()) = 0;
	virtual Error draw_list_begin_split(RID p_framebuffer, uint32_t p_splits, DrawListID *r_split_ids, InitialAction p_initial_color_action, FinalAction p_final_color_action, InitialAction p_initial_depth_action, FinalAction p_final_depth_action, const Vector<Color> &p_clear_color_values = Vector<Color>(), float p_clear_depth = 1.0, uint32_t p_clear_stencil = 0, const Rect2 &p_region = Rect2(), const Vector<RID> &p_storage_textures = Vector<RID>()) = 0;

	virtual void draw_list_bind_render_pipeline(DrawListID p_list, RID p_render_pipeline) = 0;
	virtual void draw_list_bind_uniform_set(DrawListID p_list, RID p_uniform_set, uint32_t p_index) = 0;
	virtual void draw_list_bind_vertex_array(DrawListID p_list, RID p_vertex_array) = 0;
	virtual void draw_list_bind_index_array(DrawListID p_list, RID p_index_array) = 0;
	virtual void draw_list_set_line_width(DrawListID p_list, float p_width) = 0;
	virtual void draw_list_set_push_constant(DrawListID p_list, const void *p_data, uint32_t p_data_size) = 0;

	virtual void draw_list_draw(DrawListID p_list, bool p_use_indices, uint32_t p_instances = 1, uint32_t p_procedural_vertices = 0) = 0;

	virtual void draw_list_enable_scissor(DrawListID p_list, const Rect2 &p_rect) = 0;
	virtual void draw_list_disable_scissor(DrawListID p_list) = 0;

	virtual void draw_list_end(uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;

	/***********************/
	/**** COMPUTE LISTS ****/
	/***********************/

	typedef int64_t ComputeListID;

	virtual ComputeListID compute_list_begin(bool p_allow_draw_overlap = false) = 0;
	virtual void compute_list_bind_compute_pipeline(ComputeListID p_list, RID p_compute_pipeline) = 0;
	virtual void compute_list_bind_uniform_set(ComputeListID p_list, RID p_uniform_set, uint32_t p_index) = 0;
	virtual void compute_list_set_push_constant(ComputeListID p_list, const void *p_data, uint32_t p_data_size) = 0;
	virtual void compute_list_dispatch(ComputeListID p_list, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) = 0;
	virtual void compute_list_dispatch_threads(ComputeListID p_list, uint32_t p_x_threads, uint32_t p_y_threads, uint32_t p_z_threads) = 0;
	virtual void compute_list_dispatch_indirect(ComputeListID p_list, RID p_buffer, uint32_t p_offset) = 0;
	virtual void compute_list_add_barrier(ComputeListID p_list) = 0;

	virtual void compute_list_end(uint32_t p_post_barrier = BARRIER_MASK_ALL) = 0;

	virtual void barrier(uint32_t p_from = BARRIER_MASK_ALL, uint32_t p_to = BARRIER_MASK_ALL) = 0;
	virtual void full_barrier() = 0;

	/***************/
	/**** FREE! ****/
	/***************/

	virtual void free(RID p_id) = 0;

	/****************/
	/**** Timing ****/
	/****************/

	virtual void capture_timestamp(const String &p_name) = 0;
	virtual uint32_t get_captured_timestamps_count() const = 0;
	virtual uint64_t get_captured_timestamps_frame() const = 0;
	virtual uint64_t get_captured_timestamp_gpu_time(uint32_t p_index) const = 0;
	virtual uint64_t get_captured_timestamp_cpu_time(uint32_t p_index) const = 0;
	virtual String get_captured_timestamp_name(uint32_t p_index) const = 0;

	/****************/
	/**** LIMITS ****/
	/****************/

	enum Limit {
		LIMIT_MAX_BOUND_UNIFORM_SETS,
		LIMIT_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS,
		LIMIT_MAX_TEXTURES_PER_UNIFORM_SET,
		LIMIT_MAX_SAMPLERS_PER_UNIFORM_SET,
		LIMIT_MAX_STORAGE_BUFFERS_PER_UNIFORM_SET,
		LIMIT_MAX_STORAGE_IMAGES_PER_UNIFORM_SET,
		LIMIT_MAX_UNIFORM_BUFFERS_PER_UNIFORM_SET,
		LIMIT_MAX_DRAW_INDEXED_INDEX,
		LIMIT_MAX_FRAMEBUFFER_HEIGHT,
		LIMIT_MAX_FRAMEBUFFER_WIDTH,
		LIMIT_MAX_TEXTURE_ARRAY_LAYERS,
		LIMIT_MAX_TEXTURE_SIZE_1D,
		LIMIT_MAX_TEXTURE_SIZE_2D,
		LIMIT_MAX_TEXTURE_SIZE_3D,
		LIMIT_MAX_TEXTURE_SIZE_CUBE,
		LIMIT_MAX_TEXTURES_PER_SHADER_STAGE,
		LIMIT_MAX_SAMPLERS_PER_SHADER_STAGE,
		LIMIT_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE,
		LIMIT_MAX_STORAGE_IMAGES_PER_SHADER_STAGE,
		LIMIT_MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE,
		LIMIT_MAX_PUSH_CONSTANT_SIZE,
		LIMIT_MAX_UNIFORM_BUFFER_SIZE,
		LIMIT_MAX_VERTEX_INPUT_ATTRIBUTE_OFFSET,
		LIMIT_MAX_VERTEX_INPUT_ATTRIBUTES,
		LIMIT_MAX_VERTEX_INPUT_BINDINGS,
		LIMIT_MAX_VERTEX_INPUT_BINDING_STRIDE,
		LIMIT_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
		LIMIT_MAX_COMPUTE_SHARED_MEMORY_SIZE,
		LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X,
		LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y,
		LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z,
		LIMIT_MAX_COMPUTE_WORKGROUP_INVOCATIONS,
		LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_X,
		LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Y,
		LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Z,
	};

	virtual int limit_get(Limit p_limit) = 0;

	//methods below not exposed, used by RenderingDeviceRD
	virtual void prepare_screen_for_drawing() = 0;

	virtual void swap_buffers() = 0;

	virtual uint32_t get_frame_delay() const = 0;

	virtual void submit() = 0;
	virtual void sync() = 0;

	virtual uint64_t get_memory_usage() const = 0;

	virtual RenderingDevice *create_local_device() = 0;

	virtual void set_resource_name(RID p_id, const String p_name) = 0;

	virtual void draw_command_begin_label(String p_label_name, const Color p_color = Color(1, 1, 1, 1)) = 0;
	virtual void draw_command_insert_label(String p_label_name, const Color p_color = Color(1, 1, 1, 1)) = 0;
	virtual void draw_command_end_label() = 0;

	virtual String get_device_vendor_name() const = 0;
	virtual String get_device_name() const = 0;
	virtual String get_device_pipeline_cache_uuid() const = 0;

	static RenderingDevice *get_singleton();
	RenderingDevice();

protected:
	//binders to script API
	RID _texture_create(const Ref<RDTextureFormat> &p_format, const Ref<RDTextureView> &p_view, const TypedArray<PackedByteArray> &p_data = Array());
	RID _texture_create_shared(const Ref<RDTextureView> &p_view, RID p_with_texture);
	RID _texture_create_shared_from_slice(const Ref<RDTextureView> &p_view, RID p_with_texture, uint32_t p_layer, uint32_t p_mipmap, TextureSliceType p_slice_type = TEXTURE_SLICE_2D);

	FramebufferFormatID _framebuffer_format_create(const TypedArray<RDAttachmentFormat> &p_attachments);
	RID _framebuffer_create(const Array &p_textures, FramebufferFormatID p_format_check = INVALID_ID);
	RID _sampler_create(const Ref<RDSamplerState> &p_state);
	VertexFormatID _vertex_format_create(const TypedArray<RDVertexAttribute> &p_vertex_formats);
	RID _vertex_array_create(uint32_t p_vertex_count, VertexFormatID p_vertex_format, const TypedArray<RID> &p_src_buffers);

	Ref<RDShaderBytecode> _shader_compile_from_source(const Ref<RDShaderSource> &p_source, bool p_allow_cache = true);

	RID _uniform_set_create(const Array &p_uniforms, RID p_shader, uint32_t p_shader_set);

	Error _buffer_update(RID p_buffer, uint32_t p_offset, uint32_t p_size, const Vector<uint8_t> &p_data, uint32_t p_post_barrier = BARRIER_MASK_ALL);

	RID _render_pipeline_create(RID p_shader, FramebufferFormatID p_framebuffer_format, VertexFormatID p_vertex_format, RenderPrimitive p_render_primitive, const Ref<RDPipelineRasterizationState> &p_rasterization_state, const Ref<RDPipelineMultisampleState> &p_multisample_state, const Ref<RDPipelineDepthStencilState> &p_depth_stencil_state, const Ref<RDPipelineColorBlendState> &p_blend_state, int p_dynamic_state_flags = 0);

	Vector<int64_t> _draw_list_begin_split(RID p_framebuffer, uint32_t p_splits, InitialAction p_initial_color_action, FinalAction p_final_color_action, InitialAction p_initial_depth_action, FinalAction p_final_depth_action, const Vector<Color> &p_clear_color_values = Vector<Color>(), float p_clear_depth = 1.0, uint32_t p_clear_stencil = 0, const Rect2 &p_region = Rect2(), const TypedArray<RID> &p_storage_textures = TypedArray<RID>());
	void _draw_list_set_push_constant(DrawListID p_list, const Vector<uint8_t> &p_data, uint32_t p_data_size);
	void _compute_list_set_push_constant(ComputeListID p_list, const Vector<uint8_t> &p_data, uint32_t p_data_size);
};

VARIANT_ENUM_CAST(RenderingDevice::ShaderStage)
VARIANT_ENUM_CAST(RenderingDevice::ShaderLanguage)
VARIANT_ENUM_CAST(RenderingDevice::CompareOperator)
VARIANT_ENUM_CAST(RenderingDevice::DataFormat)
VARIANT_ENUM_CAST(RenderingDevice::TextureType)
VARIANT_ENUM_CAST(RenderingDevice::TextureSamples)
VARIANT_ENUM_CAST(RenderingDevice::TextureUsageBits)
VARIANT_ENUM_CAST(RenderingDevice::TextureSwizzle)
VARIANT_ENUM_CAST(RenderingDevice::TextureSliceType)
VARIANT_ENUM_CAST(RenderingDevice::SamplerFilter)
VARIANT_ENUM_CAST(RenderingDevice::SamplerRepeatMode)
VARIANT_ENUM_CAST(RenderingDevice::SamplerBorderColor)
VARIANT_ENUM_CAST(RenderingDevice::VertexFrequency)
VARIANT_ENUM_CAST(RenderingDevice::IndexBufferFormat)
VARIANT_ENUM_CAST(RenderingDevice::StorageBufferUsage)
VARIANT_ENUM_CAST(RenderingDevice::UniformType)
VARIANT_ENUM_CAST(RenderingDevice::RenderPrimitive)
VARIANT_ENUM_CAST(RenderingDevice::PolygonCullMode)
VARIANT_ENUM_CAST(RenderingDevice::PolygonFrontFace)
VARIANT_ENUM_CAST(RenderingDevice::StencilOperation)
VARIANT_ENUM_CAST(RenderingDevice::LogicOperation)
VARIANT_ENUM_CAST(RenderingDevice::BlendFactor)
VARIANT_ENUM_CAST(RenderingDevice::BlendOperation)
VARIANT_ENUM_CAST(RenderingDevice::PipelineDynamicStateFlags)
VARIANT_ENUM_CAST(RenderingDevice::InitialAction)
VARIANT_ENUM_CAST(RenderingDevice::FinalAction)
VARIANT_ENUM_CAST(RenderingDevice::Limit)

typedef RenderingDevice RD;

#endif // RENDERING_DEVICE_H
