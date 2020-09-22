#[vertex]

#version 450

VERSION_DEFINES

#ifdef USE_ATTRIBUTES
layout(location = 0) in vec2 vertex_attrib;
layout(location = 3) in vec4 color_attrib;
layout(location = 4) in vec2 uv_attrib;

layout(location = 6) in uvec4 bones_attrib;

#endif

#include "canvas_uniforms_inc.glsl"

layout(location = 0) out vec2 uv_interp;
layout(location = 1) out vec4 color_interp;
layout(location = 2) out vec2 vertex_interp;

#ifdef USE_NINEPATCH

layout(location = 3) out vec2 pixel_size_interp;

#endif

#ifdef USE_MATERIAL_UNIFORMS
layout(set = 1, binding = 1, std140) uniform MaterialUniforms{
	/* clang-format off */
MATERIAL_UNIFORMS
	/* clang-format on */
} material;
#endif

/* clang-format off */
VERTEX_SHADER_GLOBALS
/* clang-format on */

void main() {
	vec4 instance_custom = vec4(0.0);
#ifdef USE_PRIMITIVE

	//weird bug,
	//this works
	vec2 vertex;
	vec2 uv;
	vec4 color;

	if (gl_VertexIndex == 0) {
		vertex = draw_data.points[0];
		uv = draw_data.uvs[0];
		color = vec4(unpackHalf2x16(draw_data.colors[0]), unpackHalf2x16(draw_data.colors[1]));
	} else if (gl_VertexIndex == 1) {
		vertex = draw_data.points[1];
		uv = draw_data.uvs[1];
		color = vec4(unpackHalf2x16(draw_data.colors[2]), unpackHalf2x16(draw_data.colors[3]));
	} else {
		vertex = draw_data.points[2];
		uv = draw_data.uvs[2];
		color = vec4(unpackHalf2x16(draw_data.colors[4]), unpackHalf2x16(draw_data.colors[5]));
	}
	uvec4 bones = uvec4(0, 0, 0, 0);

#elif defined(USE_ATTRIBUTES)

	vec2 vertex = vertex_attrib;
	vec4 color = color_attrib;
	vec2 uv = uv_attrib;

	uvec4 bones = bones_attrib;
#else

	vec2 vertex_base_arr[4] = vec2[](vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 0.0));
	vec2 vertex_base = vertex_base_arr[gl_VertexIndex];

	vec2 uv = draw_data.src_rect.xy + abs(draw_data.src_rect.zw) * ((draw_data.flags & FLAGS_TRANSPOSE_RECT) != 0 ? vertex_base.yx : vertex_base.xy);
	vec4 color = draw_data.modulation;
	vec2 vertex = draw_data.dst_rect.xy + abs(draw_data.dst_rect.zw) * mix(vertex_base, vec2(1.0, 1.0) - vertex_base, lessThan(draw_data.src_rect.zw, vec2(0.0, 0.0)));
	uvec4 bones = uvec4(0, 0, 0, 0);

#endif

	mat4 world_matrix = mat4(vec4(draw_data.world_x, 0.0, 0.0), vec4(draw_data.world_y, 0.0, 0.0), vec4(0.0, 0.0, 1.0, 0.0), vec4(draw_data.world_ofs, 0.0, 1.0));

#if 0
	if (draw_data.flags & FLAGS_INSTANCING_ENABLED) {
		uint offset = draw_data.flags & FLAGS_INSTANCING_STRIDE_MASK;
		offset *= gl_InstanceIndex;
		mat4 instance_xform = mat4(
				vec4(texelFetch(instancing_buffer, offset + 0), texelFetch(instancing_buffer, offset + 1), 0.0, texelFetch(instancing_buffer, offset + 3)),
				vec4(texelFetch(instancing_buffer, offset + 4), texelFetch(instancing_buffer, offset + 5), 0.0, texelFetch(instancing_buffer, offset + 7)),
				vec4(0.0, 0.0, 1.0, 0.0),
				vec4(0.0, 0.0, 0.0, 1.0));
		offset += 8;
		if (draw_data.flags & FLAGS_INSTANCING_HAS_COLORS) {
			vec4 instance_color;
			if (draw_data.flags & FLAGS_INSTANCING_COLOR_8_BIT) {
				uint bits = floatBitsToUint(texelFetch(instancing_buffer, offset));
				instance_color = unpackUnorm4x8(bits);
				offset += 1;
			} else {
				instance_color = vec4(texelFetch(instancing_buffer, offset + 0), texelFetch(instancing_buffer, offset + 1), texelFetch(instancing_buffer, offset + 2), texelFetch(instancing_buffer, offset + 3));
				offset += 4;
			}

			color *= instance_color;
		}
		if (draw_data.flags & FLAGS_INSTANCING_HAS_CUSTOM_DATA) {
			if (draw_data.flags & FLAGS_INSTANCING_CUSTOM_DATA_8_BIT) {
				uint bits = floatBitsToUint(texelFetch(instancing_buffer, offset));
				instance_custom = unpackUnorm4x8(bits);
			} else {
				instance_custom = vec4(texelFetch(instancing_buffer, offset + 0), texelFetch(instancing_buffer, offset + 1), texelFetch(instancing_buffer, offset + 2), texelFetch(instancing_buffer, offset + 3));
			}
		}
	}

#endif

#if !defined(USE_ATTRIBUTES) && !defined(USE_PRIMITIVE)
	if (bool(draw_data.flags & FLAGS_USING_PARTICLES)) {
		//scale by texture size
		vertex /= draw_data.color_texture_pixel_size;
	}
#endif

#ifdef USE_POINT_SIZE
	float point_size = 1.0;
#endif
	{
		/* clang-format off */
VERTEX_SHADER_CODE
		/* clang-format on */
	}

#ifdef USE_NINEPATCH
	pixel_size_interp = abs(draw_data.dst_rect.zw) * vertex_base;
#endif

#if !defined(SKIP_TRANSFORM_USED)
	vertex = (world_matrix * vec4(vertex, 0.0, 1.0)).xy;
#endif

	color_interp = color;

	if (bool(draw_data.flags & FLAGS_USE_PIXEL_SNAP)) {
		vertex = floor(vertex + 0.5);
		// precision issue on some hardware creates artifacts within texture
		// offset uv by a small amount to avoid
		uv += 1e-5;
	}

#ifdef USE_ATTRIBUTES
#if 0
	if (bool(draw_data.flags & FLAGS_USE_SKELETON) && bone_weights != vec4(0.0)) { //must be a valid bone
		//skeleton transform
		ivec4 bone_indicesi = ivec4(bone_indices);

		uvec2 tex_ofs = bone_indicesi.x * 2;

		mat2x4 m;
		m = mat2x4(
					texelFetch(skeleton_buffer, tex_ofs + 0),
					texelFetch(skeleton_buffer, tex_ofs + 1)) *
			bone_weights.x;

		tex_ofs = bone_indicesi.y * 2;

		m += mat2x4(
					 texelFetch(skeleton_buffer, tex_ofs + 0),
					 texelFetch(skeleton_buffer, tex_ofs + 1)) *
			 bone_weights.y;

		tex_ofs = bone_indicesi.z * 2;

		m += mat2x4(
					 texelFetch(skeleton_buffer, tex_ofs + 0),
					 texelFetch(skeleton_buffer, tex_ofs + 1)) *
			 bone_weights.z;

		tex_ofs = bone_indicesi.w * 2;

		m += mat2x4(
					 texelFetch(skeleton_buffer, tex_ofs + 0),
					 texelFetch(skeleton_buffer, tex_ofs + 1)) *
			 bone_weights.w;

		mat4 bone_matrix = skeleton_data.skeleton_transform * transpose(mat4(m[0], m[1], vec4(0.0, 0.0, 1.0, 0.0), vec4(0.0, 0.0, 0.0, 1.0))) * skeleton_data.skeleton_transform_inverse;

		//outvec = bone_matrix * outvec;
	}
#endif
#endif

	vertex = (canvas_data.canvas_transform * vec4(vertex, 0.0, 1.0)).xy;

	vertex_interp = vertex;
	uv_interp = uv;

	gl_Position = canvas_data.screen_transform * vec4(vertex, 0.0, 1.0);

#ifdef USE_POINT_SIZE
	gl_PointSize = point_size;
#endif
}

#[fragment]

#version 450

VERSION_DEFINES

#include "canvas_uniforms_inc.glsl"

layout(location = 0) in vec2 uv_interp;
layout(location = 1) in vec4 color_interp;
layout(location = 2) in vec2 vertex_interp;

#ifdef USE_NINEPATCH

layout(location = 3) in vec2 pixel_size_interp;

#endif

layout(location = 0) out vec4 frag_color;

#ifdef USE_MATERIAL_UNIFORMS
layout(set = 1, binding = 1, std140) uniform MaterialUniforms{
	/* clang-format off */
MATERIAL_UNIFORMS
	/* clang-format on */
} material;
#endif

/* clang-format off */
FRAGMENT_SHADER_GLOBALS
/* clang-format on */

#ifdef LIGHT_SHADER_CODE_USED

vec4 light_compute(
		vec3 light_vertex,
		vec3 light_position,
		vec3 normal,
		vec4 light_color,
		float light_energy,
		vec4 specular_shininess,
		inout vec4 shadow_modulate,
		vec2 screen_uv,
		vec2 uv,
		vec4 color) {
	vec4 light = vec4(0.0);
	/* clang-format off */
LIGHT_SHADER_CODE
	/* clang-format on */
	return light;
}

#endif

#ifdef USE_NINEPATCH

float map_ninepatch_axis(float pixel, float draw_size, float tex_pixel_size, float margin_begin, float margin_end, int np_repeat, inout int draw_center) {
	float tex_size = 1.0 / tex_pixel_size;

	if (pixel < margin_begin) {
		return pixel * tex_pixel_size;
	} else if (pixel >= draw_size - margin_end) {
		return (tex_size - (draw_size - pixel)) * tex_pixel_size;
	} else {
		if (!bool(draw_data.flags & FLAGS_NINEPACH_DRAW_CENTER)) {
			draw_center--;
		}

		// np_repeat is passed as uniform using NinePatchRect::AxisStretchMode enum.
		if (np_repeat == 0) { // Stretch.
			// Convert to ratio.
			float ratio = (pixel - margin_begin) / (draw_size - margin_begin - margin_end);
			// Scale to source texture.
			return (margin_begin + ratio * (tex_size - margin_begin - margin_end)) * tex_pixel_size;
		} else if (np_repeat == 1) { // Tile.
			// Convert to offset.
			float ofs = mod((pixel - margin_begin), tex_size - margin_begin - margin_end);
			// Scale to source texture.
			return (margin_begin + ofs) * tex_pixel_size;
		} else if (np_repeat == 2) { // Tile Fit.
			// Calculate scale.
			float src_area = draw_size - margin_begin - margin_end;
			float dst_area = tex_size - margin_begin - margin_end;
			float scale = max(1.0, floor(src_area / max(dst_area, 0.0000001) + 0.5));
			// Convert to ratio.
			float ratio = (pixel - margin_begin) / src_area;
			ratio = mod(ratio * scale, 1.0);
			// Scale to source texture.
			return (margin_begin + ratio * dst_area) * tex_pixel_size;
		} else { // Shouldn't happen, but silences compiler warning.
			return 0.0;
		}
	}
}

#endif

void main() {
	vec4 color = color_interp;
	vec2 uv = uv_interp;
	vec2 vertex = vertex_interp;

#if !defined(USE_ATTRIBUTES) && !defined(USE_PRIMITIVE)

#ifdef USE_NINEPATCH

	int draw_center = 2;
	uv = vec2(
			map_ninepatch_axis(pixel_size_interp.x, abs(draw_data.dst_rect.z), draw_data.color_texture_pixel_size.x, draw_data.ninepatch_margins.x, draw_data.ninepatch_margins.z, int(draw_data.flags >> FLAGS_NINEPATCH_H_MODE_SHIFT) & 0x3, draw_center),
			map_ninepatch_axis(pixel_size_interp.y, abs(draw_data.dst_rect.w), draw_data.color_texture_pixel_size.y, draw_data.ninepatch_margins.y, draw_data.ninepatch_margins.w, int(draw_data.flags >> FLAGS_NINEPATCH_V_MODE_SHIFT) & 0x3, draw_center));

	if (draw_center == 0) {
		color.a = 0.0;
	}

	uv = uv * draw_data.src_rect.zw + draw_data.src_rect.xy; //apply region if needed

#endif
	if (bool(draw_data.flags & FLAGS_CLIP_RECT_UV)) {
		uv = clamp(uv, draw_data.src_rect.xy, draw_data.src_rect.xy + abs(draw_data.src_rect.zw));
	}

#endif

	color *= texture(sampler2D(color_texture, texture_sampler), uv);

	uint light_count = (draw_data.flags >> FLAGS_LIGHT_COUNT_SHIFT) & 0xF; //max 16 lights

	vec3 normal;

#if defined(NORMAL_USED)
	bool normal_used = true;
#else
	bool normal_used = false;
#endif

	if (normal_used || (light_count > 0 && bool(draw_data.flags & FLAGS_DEFAULT_NORMAL_MAP_USED))) {
		normal.xy = texture(sampler2D(normal_texture, texture_sampler), uv).xy * vec2(2.0, -2.0) - vec2(1.0, -1.0);
		normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
		normal_used = true;
	} else {
		normal = vec3(0.0, 0.0, 1.0);
	}

	vec4 specular_shininess;

#if defined(SPECULAR_SHININESS_USED)

	bool specular_shininess_used = true;
#else
	bool specular_shininess_used = false;
#endif

	if (specular_shininess_used || (light_count > 0 && normal_used && bool(draw_data.flags & FLAGS_DEFAULT_SPECULAR_MAP_USED))) {
		specular_shininess = texture(sampler2D(specular_texture, texture_sampler), uv);
		specular_shininess *= unpackUnorm4x8(draw_data.specular_shininess);
		specular_shininess_used = true;
	} else {
		specular_shininess = vec4(1.0);
	}

#if defined(SCREEN_UV_USED)
	vec2 screen_uv = gl_FragCoord.xy * canvas_data.screen_pixel_size;
#else
	vec2 screen_uv = vec2(0.0);
#endif

	vec3 light_vertex = vec3(vertex, 0.0);
	vec2 shadow_vertex = vertex;

	{
		float normal_depth = 1.0;

#if defined(NORMALMAP_USED)
		vec3 normal_map = vec3(0.0, 0.0, 1.0);
		normal_used = true;
#endif

		/* clang-format off */

FRAGMENT_SHADER_CODE

		/* clang-format on */

#if defined(NORMALMAP_USED)
		normal = mix(vec3(0.0, 0.0, 1.0), normal_map * vec3(2.0, -2.0, 1.0) - vec3(1.0, -1.0, 0.0), normal_depth);
#endif
	}

	if (normal_used) {
		//convert by item transform
		normal.xy = mat2(normalize(draw_data.world_x), normalize(draw_data.world_y)) * normal.xy;
		//convert by canvas transform
		normal = normalize((canvas_data.canvas_normal_transform * vec4(normal, 0.0)).xyz);
	}

	vec4 base_color = color;
	if (bool(draw_data.flags & FLAGS_USING_LIGHT_MASK)) {
		color = vec4(0.0); //invisible by default due to using light mask
	}

	color *= canvas_data.canvas_modulation;
#ifdef USE_LIGHTING
	for (uint i = 0; i < MAX_LIGHT_TEXTURES; i++) {
		if (i >= light_count) {
			break;
		}
		uint light_base;
		if (i < 8) {
			if (i < 4) {
				light_base = draw_data.lights[0];
			} else {
				light_base = draw_data.lights[1];
			}
		} else {
			if (i < 12) {
				light_base = draw_data.lights[2];
			} else {
				light_base = draw_data.lights[3];
			}
		}
		light_base >>= (i & 3) * 8;
		light_base &= 0xFF;

		vec2 tex_uv = (vec4(vertex, 0.0, 1.0) * mat4(light_array.data[light_base].texture_matrix[0], light_array.data[light_base].texture_matrix[1], vec4(0.0, 0.0, 1.0, 0.0), vec4(0.0, 0.0, 0.0, 1.0))).xy; //multiply inverse given its transposed. Optimizer removes useless operations.
		vec4 light_color = texture(sampler2D(light_textures[i], texture_sampler), tex_uv);
		vec4 light_base_color = light_array.data[light_base].color;

#ifdef LIGHT_SHADER_CODE_USED

		vec4 shadow_modulate = vec4(1.0);
		vec3 light_position = vec3(light_array.data[light_base].position, light_array.data[light_base].height);

		light_color.rgb *= light_base_color.rgb;
		light_color = light_compute(light_vertex, light_position, normal, light_color, light_base_color.a, specular_shininess, shadow_modulate, screen_uv, color, uv);
#else

		light_color.rgb *= light_base_color.rgb * light_base_color.a;

		if (normal_used) {
			vec3 light_pos = vec3(light_array.data[light_base].position, light_array.data[light_base].height);
			vec3 pos = light_vertex;
			vec3 light_vec = normalize(light_pos - pos);
			float cNdotL = max(0.0, dot(normal, light_vec));

			if (specular_shininess_used) {
				//blinn
				vec3 view = vec3(0.0, 0.0, 1.0); // not great but good enough
				vec3 half_vec = normalize(view + light_vec);

				float cNdotV = max(dot(normal, view), 0.0);
				float cNdotH = max(dot(normal, half_vec), 0.0);
				float cVdotH = max(dot(view, half_vec), 0.0);
				float cLdotH = max(dot(light_vec, half_vec), 0.0);
				float shininess = exp2(15.0 * specular_shininess.a + 1.0) * 0.25;
				float blinn = pow(cNdotH, shininess);
				blinn *= (shininess + 8.0) * (1.0 / (8.0 * M_PI));
				float s = (blinn) / max(4.0 * cNdotV * cNdotL, 0.75);

				light_color.rgb = specular_shininess.rgb * light_base_color.rgb * s + light_color.rgb * cNdotL;
			} else {
				light_color.rgb *= cNdotL;
			}
		}
#endif
		if (any(lessThan(tex_uv, vec2(0.0, 0.0))) || any(greaterThanEqual(tex_uv, vec2(1.0, 1.0)))) {
			//if outside the light texture, light color is zero
			light_color.a = 0.0;
		}

		if (bool(light_array.data[light_base].flags & LIGHT_FLAGS_HAS_SHADOW)) {
			vec2 shadow_pos = (vec4(shadow_vertex, 0.0, 1.0) * mat4(light_array.data[light_base].shadow_matrix[0], light_array.data[light_base].shadow_matrix[1], vec4(0.0, 0.0, 1.0, 0.0), vec4(0.0, 0.0, 0.0, 1.0))).xy; //multiply inverse given its transposed. Optimizer removes useless operations.

			vec2 pos_norm = normalize(shadow_pos);
			vec2 pos_abs = abs(pos_norm);
			vec2 pos_box = pos_norm / max(pos_abs.x, pos_abs.y);
			vec2 pos_rot = pos_norm * mat2(vec2(0.7071067811865476, -0.7071067811865476), vec2(0.7071067811865476, 0.7071067811865476)); //is there a faster way to 45 degrees rot?
			float tex_ofs;
			float distance;
			if (pos_rot.y > 0) {
				if (pos_rot.x > 0) {
					tex_ofs = pos_box.y * 0.125 + 0.125;
					distance = shadow_pos.x;
				} else {
					tex_ofs = pos_box.x * -0.125 + (0.25 + 0.125);
					distance = shadow_pos.y;
				}
			} else {
				if (pos_rot.x < 0) {
					tex_ofs = pos_box.y * -0.125 + (0.5 + 0.125);
					distance = -shadow_pos.x;
				} else {
					tex_ofs = pos_box.x * 0.125 + (0.75 + 0.125);
					distance = -shadow_pos.y;
				}
			}

			//float distance = length(shadow_pos);
			float shadow;
			uint shadow_mode = light_array.data[light_base].flags & LIGHT_FLAGS_FILTER_MASK;

			vec4 shadow_uv = vec4(tex_ofs, 0.0, distance, 1.0);

			if (shadow_mode == LIGHT_FLAGS_SHADOW_NEAREST) {
				shadow = textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv).x;
			} else if (shadow_mode == LIGHT_FLAGS_SHADOW_PCF5) {
				vec4 shadow_pixel_size = vec4(light_array.data[light_base].shadow_pixel_size, 0.0, 0.0, 0.0);
				shadow = 0.0;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size * 2.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size * 2.0).x;
				shadow /= 5.0;
			} else { //PCF13
				vec4 shadow_pixel_size = vec4(light_array.data[light_base].shadow_pixel_size, 0.0, 0.0, 0.0);
				shadow = 0.0;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size * 6.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size * 5.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size * 4.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size * 3.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size * 2.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv - shadow_pixel_size).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size * 2.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size * 3.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size * 4.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size * 5.0).x;
				shadow += textureProj(sampler2DShadow(shadow_textures[i], shadow_sampler), shadow_uv + shadow_pixel_size * 6.0).x;
				shadow /= 13.0;
			}

			vec4 shadow_color = light_array.data[light_base].shadow_color;
#ifdef LIGHT_SHADER_CODE_USED
			shadow_color *= shadow_modulate;
#endif
			light_color = mix(light_color, shadow_color, shadow);
		}

		uint blend_mode = light_array.data[light_base].flags & LIGHT_FLAGS_BLEND_MASK;

		switch (blend_mode) {
			case LIGHT_FLAGS_BLEND_MODE_ADD: {
				color.rgb += light_color.rgb * light_color.a;
			} break;
			case LIGHT_FLAGS_BLEND_MODE_SUB: {
				color.rgb -= light_color.rgb * light_color.a;
			} break;
			case LIGHT_FLAGS_BLEND_MODE_MIX: {
				color.rgb = mix(color.rgb, light_color.rgb, light_color.a);
			} break;
			case LIGHT_FLAGS_BLEND_MODE_MASK: {
				light_color.a *= base_color.a;
				color.rgb = mix(color.rgb, light_color.rgb, light_color.a);
			} break;
		}
	}
#endif

	frag_color = color;
}
