/*************************************************************************/
/*  text_server_gdnative.h                                               */
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

#ifndef TEXT_SERVER_GDNATIVE_H
#define TEXT_SERVER_GDNATIVE_H

#include "modules/gdnative/gdnative.h"

#include "servers/text_server.h"

class TextServerGDNative : public TextServer {
	GDCLASS(TextServerGDNative, TextServer);

	const godot_text_interface_gdnative *interface = nullptr;
	void *data = nullptr;

protected:
	static void _bind_methods(){};

public:
	virtual bool has_feature(Feature p_feature) override;
	virtual String get_name() const override;

	virtual void free(RID p_rid) override;
	virtual bool has(RID p_rid) override;
	virtual bool load_support_data(const String &p_filename) override;

#ifdef TOOLS_ENABLED
	virtual String get_support_data_filename() override;
	virtual String get_support_data_info() override;
	virtual bool save_support_data(const String &p_filename) override;
#endif

	virtual bool is_locale_right_to_left(const String &p_locale) override;

	/* Font interface */
	virtual RID create_font_system(const String &p_name, int p_base_size = 16) override;
	virtual RID create_font_resource(const String &p_filename, int p_base_size = 16) override;
	virtual RID create_font_memory(const uint8_t *p_data, size_t p_size, int p_base_size = 16) override;

	// Cache.
	virtual Error font_save_cache(RID p_font, const String &p_path, uint8_t p_flags, List<String> *r_gen_files) const override;
	virtual void font_add_to_cache(RID p_font, const Map<int32_t, double> &p_var_id, int p_size, int p_outline_size) override;
	virtual void font_clear_cache(RID p_font) override;
	virtual RID create_font_bitmap(float p_height, float p_ascent, int p_base_size = 16) override;

	virtual void font_bitmap_add_texture(RID p_font, const Ref<Texture2D> &p_texture) override;
	virtual void font_bitmap_add_char(RID p_font, char32_t p_char, int p_texture_idx, const Rect2 &p_rect, const Size2 &p_align, float p_advance) override;
	virtual void font_bitmap_add_kerning_pair(RID p_font, char32_t p_A, char32_t p_B, int p_kerning) override;

	virtual void font_get_recognized_extensions(List<String> *p_extensions) const override;

	// Preload.
	virtual void font_preload_range(RID p_font, uint32_t p_start, uint32_t p_end, bool p_glyphs) override;

	virtual float font_get_height(RID p_font, int p_size) const override;
	virtual float font_get_ascent(RID p_font, int p_size) const override;
	virtual float font_get_descent(RID p_font, int p_size) const override;

	virtual float font_get_underline_position(RID p_font, int p_size) const override;
	virtual float font_get_underline_thickness(RID p_font, int p_size) const override;

	virtual int font_get_spacing_space(RID p_font) const override;
	virtual void font_set_spacing_space(RID p_font, int p_value) override;

	virtual int font_get_spacing_glyph(RID p_font) const override;
	virtual void font_set_spacing_glyph(RID p_font, int p_value) override;

	virtual float font_get_oversampling(RID p_font) const override;
	virtual void font_set_oversampling(RID p_font, float p_value) override;

	virtual void font_set_antialiased(RID p_font, bool p_antialiased) override;
	virtual bool font_get_antialiased(RID p_font) const override;

	virtual Dictionary font_get_feature_list(RID p_font) const override;
	virtual Dictionary font_get_variation_list(RID p_font) const override;

	virtual void font_set_variation(RID p_font, const String &p_name, double p_value) override;
	virtual double font_get_variation(RID p_font, const String &p_name) const override;

	virtual void font_set_hinting(RID p_font, Hinting p_hinting) override;
	virtual Hinting font_get_hinting(RID p_font) const override;

	virtual void font_set_distance_field_hint(RID p_font, bool p_distance_field) override;
	virtual bool font_get_distance_field_hint(RID p_font) const override;

	virtual void font_set_disable_distance_field_shader(RID p_font, bool p_disable) override;
	virtual bool font_get_disable_distance_field_shader(RID p_font) const override;

	virtual void font_set_force_autohinter(RID p_font, bool p_enabeld) override;
	virtual bool font_get_force_autohinter(RID p_font) const override;

	virtual void font_set_msdf_px_range(RID p_font, double p_range) override;
	virtual double font_get_msdf_px_range(RID p_font) const override;

	virtual bool font_has_char(RID p_font, char32_t p_char) const override;
	virtual String font_get_supported_chars(RID p_font) const override;

	virtual bool font_has_outline(RID p_font) const override;
	virtual float font_get_base_size(RID p_font) const override;

	virtual bool font_is_language_supported(RID p_font, const String &p_language) const override;
	virtual void font_set_language_support_override(RID p_font, const String &p_language, bool p_supported) override;
	virtual bool font_get_language_support_override(RID p_font, const String &p_language) override;
	virtual void font_remove_language_support_override(RID p_font, const String &p_language) override;
	Vector<String> font_get_language_support_overrides(RID p_font) override;

	virtual bool font_is_script_supported(RID p_font, const String &p_script) const override;
	virtual void font_set_script_support_override(RID p_font, const String &p_script, bool p_supported) override;
	virtual bool font_get_script_support_override(RID p_font, const String &p_script) override;
	virtual void font_remove_script_support_override(RID p_font, const String &p_script) override;
	Vector<String> font_get_script_support_overrides(RID p_font) override;

	virtual uint32_t font_get_glyph_index(RID p_font, char32_t p_char, char32_t p_variation_selector = 0x0000) const override;
	virtual Vector2 font_get_glyph_advance(RID p_font, uint32_t p_index, int p_size) const override;
	virtual Vector2 font_get_glyph_size(RID p_font, uint32_t p_index, int p_size) const override;
	virtual Vector2 font_get_glyph_kerning(RID p_font, uint32_t p_index_a, uint32_t p_index_b, int p_size) const override;

	virtual void font_draw_glyph(RID p_font, RID p_canvas, int p_size, const Vector2 &p_pos, uint32_t p_index, const Color &p_color = Color(1, 1, 1)) const override;
	virtual void font_draw_glyph_outline(RID p_font, RID p_canvas, int p_size, int p_outline_size, const Vector2 &p_pos, uint32_t p_index, const Color &p_color = Color(1, 1, 1)) const override;

	virtual bool font_get_glyph_contours(RID p_font, int p_size, uint32_t p_index, Vector<Vector3> &r_points, Vector<int32_t> &r_contours, bool &r_orientation) const override;

	virtual float font_get_global_oversampling() const override;
	virtual void font_set_global_oversampling(float p_oversampling) override;

	virtual Vector<String> get_system_fonts() const override;

	/* Shaped text buffer interface */

	virtual RID create_shaped_text(Direction p_direction = DIRECTION_AUTO, Orientation p_orientation = ORIENTATION_HORIZONTAL) override;

	virtual void shaped_text_clear(RID p_shaped) override;

	virtual void shaped_text_set_direction(RID p_shaped, Direction p_direction = DIRECTION_AUTO) override;
	virtual Direction shaped_text_get_direction(RID p_shaped) const override;

	virtual void shaped_text_set_bidi_override(RID p_shaped, const Vector<Vector2i> &p_override) override;

	virtual void shaped_text_set_orientation(RID p_shaped, Orientation p_orientation = ORIENTATION_HORIZONTAL) override;
	virtual Orientation shaped_text_get_orientation(RID p_shaped) const override;

	virtual void shaped_text_set_preserve_invalid(RID p_shaped, bool p_enabled) override;
	virtual bool shaped_text_get_preserve_invalid(RID p_shaped) const override;

	virtual void shaped_text_set_preserve_control(RID p_shaped, bool p_enabled) override;
	virtual bool shaped_text_get_preserve_control(RID p_shaped) const override;

	virtual bool shaped_text_add_string(RID p_shaped, const String &p_text, const Vector<RID> &p_fonts, int p_size, const Dictionary &p_opentype_features = Dictionary(), const String &p_language = "") override;
	virtual bool shaped_text_add_object(RID p_shaped, Variant p_key, const Size2 &p_size, VAlign p_inline_align = VALIGN_CENTER, int p_length = 1) override;
	virtual bool shaped_text_resize_object(RID p_shaped, Variant p_key, const Size2 &p_size, VAlign p_inline_align = VALIGN_CENTER) override;

	virtual RID shaped_text_substr(RID p_shaped, int p_start, int p_length) const override;
	virtual RID shaped_text_get_parent(RID p_shaped) const override;

	virtual float shaped_text_fit_to_width(RID p_shaped, float p_width, uint8_t /*JustificationFlag*/ p_jst_flags = JUSTIFICATION_WORD_BOUND | JUSTIFICATION_KASHIDA) override;
	virtual float shaped_text_tab_align(RID p_shaped, const Vector<float> &p_tab_stops) override;

	virtual bool shaped_text_shape(RID p_shaped) override;
	virtual bool shaped_text_update_breaks(RID p_shaped) override;
	virtual bool shaped_text_update_justification_ops(RID p_shaped) override;

	virtual bool shaped_text_is_ready(RID p_shaped) const override;

	virtual Vector<Glyph> shaped_text_get_glyphs(RID p_shaped) const override;

	virtual Vector2i shaped_text_get_range(RID p_shaped) const override;

	virtual Vector<Glyph> shaped_text_sort_logical(RID p_shaped) override;
	virtual Vector<Vector2i> shaped_text_get_line_breaks_adv(RID p_shaped, const Vector<float> &p_width, int p_start = 0, bool p_once = true, uint8_t /*TextBreakFlag*/ p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const override;
	virtual Vector<Vector2i> shaped_text_get_line_breaks(RID p_shaped, float p_width, int p_start = 0, uint8_t p_break_flags = BREAK_MANDATORY | BREAK_WORD_BOUND) const override;
	virtual Vector<Vector2i> shaped_text_get_word_breaks(RID p_shaped) const override;
	virtual Array shaped_text_get_objects(RID p_shaped) const override;
	virtual Rect2 shaped_text_get_object_rect(RID p_shaped, Variant p_key) const override;

	virtual Size2 shaped_text_get_size(RID p_shaped) const override;
	virtual float shaped_text_get_ascent(RID p_shaped) const override;
	virtual float shaped_text_get_descent(RID p_shaped) const override;
	virtual float shaped_text_get_width(RID p_shaped) const override;
	virtual float shaped_text_get_underline_position(RID p_shaped) const override;
	virtual float shaped_text_get_underline_thickness(RID p_shaped) const override;

	virtual String format_number(const String &p_string, const String &p_language = "") const override;
	virtual String parse_number(const String &p_string, const String &p_language = "") const override;
	virtual String percent_sign(const String &p_language = "") const override;

	static TextServer *create_func(Error &r_error, void *p_user_data);

	TextServerGDNative();
	~TextServerGDNative();
};

#endif // TEXT_SERVER_GDNATIVE_H
