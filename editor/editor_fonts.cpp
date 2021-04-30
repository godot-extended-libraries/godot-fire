/*************************************************************************/
/*  editor_fonts.cpp                                                     */
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

#include "editor_fonts.h"

#include "builtin_fonts.gen.h"

#include "core/crypto/crypto_core.h"
#include "core/os/dir_access.h"
#include "editor_scale.h"
#include "editor_settings.h"
#include "scene/resources/default_theme/default_theme.h"
#include "scene/resources/font.h"

#define MAKE_FALLBACKS(m_name)       \
	m_name->add_data(FontArabic);    \
	m_name->add_data(FontBengali);   \
	m_name->add_data(FontGeorgian);  \
	m_name->add_data(FontMalayalam); \
	m_name->add_data(FontOriya);     \
	m_name->add_data(FontSinhala);   \
	m_name->add_data(FontTamil);     \
	m_name->add_data(FontTelugu);    \
	m_name->add_data(FontHebrew);    \
	m_name->add_data(FontThai);      \
	m_name->add_data(FontHindi);     \
	m_name->add_data(FontJapanese);  \
	m_name->add_data(FontFallback);

// the custom spacings might only work with Noto Sans
#define MAKE_DEFAULT_FONT(m_name)                        \
	Ref<Font> m_name;                                    \
	m_name.instance();                                   \
	if (CustomFont.is_valid()) {                         \
		m_name->add_data(CustomFont);                    \
		m_name->add_data(DefaultFont);                   \
	} else {                                             \
		m_name->add_data(DefaultFont);                   \
	}                                                    \
	m_name->set_spacing(Font::SPACING_TOP, -EDSCALE);    \
	m_name->set_spacing(Font::SPACING_BOTTOM, -EDSCALE); \
	MAKE_FALLBACKS(m_name);

#define MAKE_BOLD_FONT(m_name)                           \
	Ref<Font> m_name;                                    \
	m_name.instance();                                   \
	if (CustomFontBold.is_valid()) {                     \
		m_name->add_data(CustomFontBold);                \
		m_name->add_data(DefaultFontBold);               \
	} else {                                             \
		m_name->add_data(DefaultFontBold);               \
	}                                                    \
	m_name->set_spacing(Font::SPACING_TOP, -EDSCALE);    \
	m_name->set_spacing(Font::SPACING_BOTTOM, -EDSCALE); \
	MAKE_FALLBACKS(m_name);

#define MAKE_SOURCE_FONT(m_name)                         \
	Ref<Font> m_name;                                    \
	m_name.instance();                                   \
	if (CustomFontSource.is_valid()) {                   \
		m_name->add_data(CustomFontSource);              \
		m_name->add_data(dfmono);                        \
	} else {                                             \
		m_name->add_data(dfmono);                        \
	}                                                    \
	m_name->set_spacing(Font::SPACING_TOP, -EDSCALE);    \
	m_name->set_spacing(Font::SPACING_BOTTOM, -EDSCALE); \
	MAKE_FALLBACKS(m_name);

Ref<FontData> load_cached_external_font(const String &p_path, const String &p_variations, int p_base_size, TextServer::Hinting p_hinting, bool p_aa, bool p_autohint) {
	Ref<FontData> font;
	font.instance();

	font->load_resource(p_path, p_base_size);
	font->set_distance_field_hint(false);
	font->set_antialiased(p_aa);
	font->set_hinting(p_hinting);
	font->set_force_autohinter(p_autohint);
	if (p_variations != String()) {
		Vector<String> variation_tags = p_variations.split(",");
		for (int i = 0; i < variation_tags.size(); i++) {
			Vector<String> tokens = variation_tags[i].split("=");
			if (tokens.size() == 2) {
				font->set_variation(tokens[0], tokens[1].to_float());
			}
		}
	}

	return font;
}

Ref<FontData> load_cached_internal_font(const uint8_t *p_data, size_t p_size, const String &p_variations, int p_base_size, TextServer::Hinting p_hinting, bool p_aa, bool p_autohint) {
	Ref<FontData> font;
	font.instance();

	font->load_memory(p_data, p_size, p_base_size);
	font->set_distance_field_hint(false);
	font->set_antialiased(p_aa);
	font->set_hinting(p_hinting);
	font->set_force_autohinter(p_autohint);
	if (p_variations != String()) {
		Vector<String> variation_tags = p_variations.split(",");
		for (int i = 0; i < variation_tags.size(); i++) {
			Vector<String> tokens = variation_tags[i].split("=");
			if (tokens.size() == 2) {
				font->set_variation(tokens[0], tokens[1].to_float());
			}
		}
	}

	return font;
}

void editor_register_fonts(Ref<Theme> p_theme) {
	DirAccess *dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	/* Custom font */

	bool font_antialiased = (bool)EditorSettings::get_singleton()->get("interface/editor/font_antialiased");
	int font_hinting_setting = (int)EditorSettings::get_singleton()->get("interface/editor/font_hinting");

	TextServer::Hinting font_hinting;
	switch (font_hinting_setting) {
		case 0:
			// The "Auto" setting uses the setting that best matches the OS' font rendering:
			// - macOS doesn't use font hinting.
			// - Windows uses ClearType, which is in between "Light" and "Normal" hinting.
			// - Linux has configurable font hinting, but most distributions including Ubuntu default to "Light".
#ifdef OSX_ENABLED
			font_hinting = TextServer::HINTING_NONE;
#else
			font_hinting = TextServer::HINTING_LIGHT;
#endif
			break;
		case 1:
			font_hinting = TextServer::HINTING_NONE;
			break;
		case 2:
			font_hinting = TextServer::HINTING_LIGHT;
			break;
		default:
			font_hinting = TextServer::HINTING_NORMAL;
			break;
	}

	int default_font_size = int(EDITOR_GET("interface/editor/main_font_size")) * EDSCALE;

	String custom_font_path = EditorSettings::get_singleton()->get("interface/editor/main_font");
	Ref<FontData> CustomFont;
	if (custom_font_path.length() > 0 && dir->file_exists(custom_font_path)) {
		CustomFont = load_cached_external_font(custom_font_path, "", default_font_size, font_hinting, font_antialiased, true);
	} else {
		EditorSettings::get_singleton()->set_manually("interface/editor/main_font", "");
	}

	/* Custom Bold font */

	String custom_font_path_bold = EditorSettings::get_singleton()->get("interface/editor/main_font_bold");
	Ref<FontData> CustomFontBold;
	if (custom_font_path_bold.length() > 0 && dir->file_exists(custom_font_path_bold)) {
		CustomFontBold = load_cached_external_font(custom_font_path_bold, "", default_font_size, font_hinting, font_antialiased, true);
	} else {
		EditorSettings::get_singleton()->set_manually("interface/editor/main_font_bold", "");
	}

	/* Custom source code font */

	String custom_font_path_source = EditorSettings::get_singleton()->get("interface/editor/code_font");
	Ref<FontData> CustomFontSource;
	if (custom_font_path_source.length() > 0 && dir->file_exists(custom_font_path_source)) {
		CustomFontSource = load_cached_external_font(custom_font_path_source, EditorSettings::get_singleton()->get("interface/editor/code_font_custom_variations"), default_font_size, font_hinting, font_antialiased, true);
	} else {
		EditorSettings::get_singleton()->set_manually("interface/editor/code_font", "");
	}

	memdelete(dir);

	/* Droid Sans */

	Ref<FontData> DefaultFont = load_cached_internal_font(_font_NotoSansUI_Regular, _font_NotoSansUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> DefaultFontBold = load_cached_internal_font(_font_NotoSansUI_Bold, _font_NotoSansUI_Bold_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontFallback = load_cached_internal_font(_font_DroidSansFallback, _font_DroidSansFallback_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontJapanese = load_cached_internal_font(_font_DroidSansJapanese, _font_DroidSansJapanese_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontArabic = load_cached_internal_font(_font_NotoNaskhArabicUI_Regular, _font_NotoNaskhArabicUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontBengali = load_cached_internal_font(_font_NotoSansBengali_Regular, _font_NotoSansBengali_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontGeorgian = load_cached_internal_font(_font_NotoSansGeorgian_Regular, _font_NotoSansGeorgian_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontHebrew = load_cached_internal_font(_font_NotoSansHebrew_Regular, _font_NotoSansHebrew_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontMalayalam = load_cached_internal_font(_font_NotoSansMalayalamUI_Regular, _font_NotoSansMalayalamUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontOriya = load_cached_internal_font(_font_NotoSansOriyaUI_Regular, _font_NotoSansOriyaUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontSinhala = load_cached_internal_font(_font_NotoSansSinhalaUI_Regular, _font_NotoSansSinhalaUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontTamil = load_cached_internal_font(_font_NotoSansTamilUI_Regular, _font_NotoSansTamilUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontTelugu = load_cached_internal_font(_font_NotoSansTeluguUI_Regular, _font_NotoSansTeluguUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontThai = load_cached_internal_font(_font_NotoSansThaiUI_Regular, _font_NotoSansThaiUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);
	Ref<FontData> FontHindi = load_cached_internal_font(_font_NotoSansDevanagariUI_Regular, _font_NotoSansDevanagariUI_Regular_size, "", default_font_size, font_hinting, font_antialiased, true);

	/* Hack */

	Ref<FontData> dfmono = load_cached_internal_font(_font_Hack_Regular, _font_Hack_Regular_size, EditorSettings::get_singleton()->get("interface/editor/code_font_custom_variations"), default_font_size, font_hinting, font_antialiased, true);

	Vector<String> subtag = String(EditorSettings::get_singleton()->get("interface/editor/code_font_custom_variations")).split(",");
	Dictionary ftrs;
	for (int i = 0; i < subtag.size(); i++) {
		Vector<String> subtag_a = subtag[i].split("=");
		if (subtag_a.size() == 2) {
			dfmono->set_variation(subtag_a[0], subtag_a[1].to_float());
		}
	}

	// Default font
	MAKE_DEFAULT_FONT(df);
	p_theme->set_default_theme_font(df); // Default theme font
	p_theme->set_default_theme_font_size(default_font_size);

	p_theme->set_font_size("main_size", "EditorFonts", default_font_size);
	p_theme->set_font("main", "EditorFonts", df);

	// Bold font
	MAKE_BOLD_FONT(df_bold);
	p_theme->set_font_size("bold_size", "EditorFonts", default_font_size);
	p_theme->set_font("bold", "EditorFonts", df_bold);

	// Title font
	p_theme->set_font_size("title_size", "EditorFonts", default_font_size + 2 * EDSCALE);
	p_theme->set_font("title", "EditorFonts", df_bold);

	// Documentation fonts
	MAKE_SOURCE_FONT(df_code);
	p_theme->set_font_size("doc_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_font_size")) * EDSCALE);
	p_theme->set_font("doc", "EditorFonts", df);
	p_theme->set_font_size("doc_bold_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_font_size")) * EDSCALE);
	p_theme->set_font("doc_bold", "EditorFonts", df_bold);
	p_theme->set_font_size("doc_title_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_title_font_size")) * EDSCALE);
	p_theme->set_font("doc_title", "EditorFonts", df_bold);
	p_theme->set_font_size("doc_source_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_source_font_size")) * EDSCALE);
	p_theme->set_font("doc_source", "EditorFonts", df_code);
	p_theme->set_font_size("doc_keyboard_size", "EditorFonts", (int(EDITOR_GET("text_editor/help/help_source_font_size")) - 1) * EDSCALE);
	p_theme->set_font("doc_keyboard", "EditorFonts", df_code);

	// Ruler font
	p_theme->set_font_size("rulers_size", "EditorFonts", 8 * EDSCALE);
	p_theme->set_font("rulers", "EditorFonts", df);

	// Rotation widget font
	p_theme->set_font_size("rotation_control_size", "EditorFonts", 14 * EDSCALE);
	p_theme->set_font("rotation_control", "EditorFonts", df);

	// Code font
	p_theme->set_font_size("source_size", "EditorFonts", int(EDITOR_GET("interface/editor/code_font_size")) * EDSCALE);
	p_theme->set_font("source", "EditorFonts", df_code);

	p_theme->set_font_size("expression_size", "EditorFonts", (int(EDITOR_GET("interface/editor/code_font_size")) - 1) * EDSCALE);
	p_theme->set_font("expression", "EditorFonts", df_code);

	p_theme->set_font_size("output_source_size", "EditorFonts", int(EDITOR_GET("run/output/font_size")) * EDSCALE);
	p_theme->set_font("output_source", "EditorFonts", df_code);

	p_theme->set_font_size("status_source_size", "EditorFonts", default_font_size);
	p_theme->set_font("status_source", "EditorFonts", df_code);
}
