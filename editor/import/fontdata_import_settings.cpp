/*************************************************************************/
/*  fontdata_import_settings.cpp                                         */
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

#include "fontdata_import_settings.h"

#include "resource_importer_fontdata.h"

#include "editor/editor_node.h"
#include "editor/editor_scale.h"

class FontDataImportSettingsData : public Reference {
	GDCLASS(FontDataImportSettingsData, Reference)
	friend class FontDataImportSettings;

	Map<StringName, Variant> settings;
	Map<StringName, Variant> defaults;
	List<ResourceImporter::ImportOption> options;

	bool _set(const StringName &p_name, const Variant &p_value) {
		if (defaults.has(p_name) && defaults[p_name] == p_value) {
			settings.erase(p_name);
		} else {
			settings[p_name] = p_value;
		}
		return true;
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		if (settings.has(p_name)) {
			r_ret = settings[p_name];
			return true;
		}
		if (defaults.has(p_name)) {
			r_ret = defaults[p_name];
			return true;
		}
		return false;
	}

	void _get_property_list(List<PropertyInfo> *p_list) const {
		for (const List<ResourceImporter::ImportOption>::Element *E = options.front(); E; E = E->next()) {
			p_list->push_back(E->get().option);
		}
	}
};

/*************************************************************************/
/* Glyph ranges                                                          */
/*************************************************************************/

struct UniRange {
	uint32_t start;
	uint32_t end;
	String name;
};

static UniRange unicode_ranges[] = {
	{ 0x0000, 0x007F, U"Basic Latin" },
	{ 0x0080, 0x00FF, U"Latin-1 Supplement" },
	{ 0x0100, 0x017F, U"Latin Extended-A" },
	{ 0x0180, 0x024F, U"Latin Extended-B" },
	{ 0x0250, 0x02AF, U"IPA Extensions" },
	{ 0x02B0, 0x02FF, U"Spacing Modifier Letters" },
	{ 0x0300, 0x036F, U"Combining Diacritical Marks" },
	{ 0x0370, 0x03FF, U"Greek and Coptic" },
	{ 0x0400, 0x04FF, U"Cyrillic" },
	{ 0x0500, 0x052F, U"Cyrillic Supplement" },
	{ 0x0530, 0x058F, U"Armenian" },
	{ 0x0590, 0x05FF, U"Hebrew" },
	{ 0x0600, 0x06FF, U"Arabic" },
	{ 0x0700, 0x074F, U"Syriac" },
	{ 0x0750, 0x077F, U"Arabic Supplement" },
	{ 0x0780, 0x07BF, U"Thaana" },
	{ 0x07C0, 0x07FF, U"N'Ko" },
	{ 0x0800, 0x083F, U"Samaritan" },
	{ 0x0840, 0x085F, U"Mandaic" },
	{ 0x0860, 0x086F, U"Syriac Supplement" },
	{ 0x08A0, 0x08FF, U"Arabic Extended-A" },
	{ 0x0900, 0x097F, U"Devanagari" },
	{ 0x0980, 0x09FF, U"Bengali" },
	{ 0x0A00, 0x0A7F, U"Gurmukhi" },
	{ 0x0A80, 0x0AFF, U"Gujarati" },
	{ 0x0B00, 0x0B7F, U"Oriya" },
	{ 0x0B80, 0x0BFF, U"Tamil" },
	{ 0x0C00, 0x0C7F, U"Telugu" },
	{ 0x0C80, 0x0CFF, U"Kannada" },
	{ 0x0D00, 0x0D7F, U"Malayalam" },
	{ 0x0D80, 0x0DFF, U"Sinhala" },
	{ 0x0E00, 0x0E7F, U"Thai" },
	{ 0x0E80, 0x0EFF, U"Lao" },
	{ 0x0F00, 0x0FFF, U"Tibetan" },
	{ 0x1000, 0x109F, U"Myanmar" },
	{ 0x10A0, 0x10FF, U"Georgian" },
	{ 0x1100, 0x11FF, U"Hangul Jamo" },
	{ 0x1200, 0x137F, U"Ethiopic" },
	{ 0x1380, 0x139F, U"Ethiopic Supplement" },
	{ 0x13A0, 0x13FF, U"Cherokee" },
	{ 0x1400, 0x167F, U"Unified Canadian Aboriginal Syllabics" },
	{ 0x1680, 0x169F, U"Ogham" },
	{ 0x16A0, 0x16FF, U"Runic" },
	{ 0x1700, 0x171F, U"Tagalog" },
	{ 0x1720, 0x173F, U"Hanunoo" },
	{ 0x1740, 0x175F, U"Buhid" },
	{ 0x1760, 0x177F, U"Tagbanwa" },
	{ 0x1780, 0x17FF, U"Khmer" },
	{ 0x1800, 0x18AF, U"Mongolian" },
	{ 0x18B0, 0x18FF, U"Unified Canadian Aboriginal Syllabics Extended" },
	{ 0x1900, 0x194F, U"Limbu" },
	{ 0x1950, 0x197F, U"Tai Le" },
	{ 0x1980, 0x19DF, U"New Tai Lue" },
	{ 0x19E0, 0x19FF, U"Khmer Symbols" },
	{ 0x1A00, 0x1A1F, U"Buginese" },
	{ 0x1A20, 0x1AAF, U"Tai Tham" },
	{ 0x1AB0, 0x1AFF, U"Combining Diacritical Marks Extended" },
	{ 0x1B00, 0x1B7F, U"Balinese" },
	{ 0x1B80, 0x1BBF, U"Sundanese" },
	{ 0x1BC0, 0x1BFF, U"Batak" },
	{ 0x1C00, 0x1C4F, U"Lepcha" },
	{ 0x1C50, 0x1C7F, U"Ol Chiki" },
	{ 0x1C80, 0x1C8F, U"Cyrillic Extended-C" },
	{ 0x1C90, 0x1CBF, U"Georgian Extended" },
	{ 0x1CC0, 0x1CCF, U"Sundanese Supplement" },
	{ 0x1CD0, 0x1CFF, U"Vedic Extensions" },
	{ 0x1D00, 0x1D7F, U"Phonetic Extensions" },
	{ 0x1D80, 0x1DBF, U"Phonetic Extensions Supplement" },
	{ 0x1DC0, 0x1DFF, U"Combining Diacritical Marks Supplement" },
	{ 0x1E00, 0x1EFF, U"Latin Extended Additional" },
	{ 0x1F00, 0x1FFF, U"Greek Extended" },
	{ 0x2000, 0x206F, U"General Punctuation" },
	{ 0x2070, 0x209F, U"Superscripts and Subscripts" },
	{ 0x20A0, 0x20CF, U"Currency Symbols" },
	{ 0x20D0, 0x20FF, U"Combining Diacritical Marks for Symbols" },
	{ 0x2100, 0x214F, U"Letterlike Symbols" },
	{ 0x2150, 0x218F, U"Number Forms" },
	{ 0x2190, 0x21FF, U"Arrows" },
	{ 0x2200, 0x22FF, U"Mathematical Operators" },
	{ 0x2300, 0x23FF, U"Miscellaneous Technical" },
	{ 0x2400, 0x243F, U"Control Pictures" },
	{ 0x2440, 0x245F, U"Optical Character Recognition" },
	{ 0x2460, 0x24FF, U"Enclosed Alphanumerics" },
	{ 0x2500, 0x257F, U"Box Drawing" },
	{ 0x2580, 0x259F, U"Block Elements" },
	{ 0x25A0, 0x25FF, U"Geometric Shapes" },
	{ 0x2600, 0x26FF, U"Miscellaneous Symbols" },
	{ 0x2700, 0x27BF, U"Dingbats" },
	{ 0x27C0, 0x27EF, U"Miscellaneous Mathematical Symbols-A" },
	{ 0x27F0, 0x27FF, U"Supplemental Arrows-A" },
	{ 0x2800, 0x28FF, U"Braille Patterns" },
	{ 0x2900, 0x297F, U"Supplemental Arrows-B" },
	{ 0x2980, 0x29FF, U"Miscellaneous Mathematical Symbols-B" },
	{ 0x2A00, 0x2AFF, U"Supplemental Mathematical Operators" },
	{ 0x2B00, 0x2BFF, U"Miscellaneous Symbols and Arrows" },
	{ 0x2C00, 0x2C5F, U"Glagolitic" },
	{ 0x2C60, 0x2C7F, U"Latin Extended-C" },
	{ 0x2C80, 0x2CFF, U"Coptic" },
	{ 0x2D00, 0x2D2F, U"Georgian Supplement" },
	{ 0x2D30, 0x2D7F, U"Tifinagh" },
	{ 0x2D80, 0x2DDF, U"Ethiopic Extended" },
	{ 0x2DE0, 0x2DFF, U"Cyrillic Extended-A" },
	{ 0x2E00, 0x2E7F, U"Supplemental Punctuation" },
	{ 0x2E80, 0x2EFF, U"CJK Radicals Supplement" },
	{ 0x2F00, 0x2FDF, U"Kangxi Radicals" },
	{ 0x2FF0, 0x2FFF, U"Ideographic Description Characters" },
	{ 0x3000, 0x303F, U"CJK Symbols and Punctuation" },
	{ 0x3040, 0x309F, U"Hiragana" },
	{ 0x30A0, 0x30FF, U"Katakana" },
	{ 0x3100, 0x312F, U"Bopomofo" },
	{ 0x3130, 0x318F, U"Hangul Compatibility Jamo" },
	{ 0x3190, 0x319F, U"Kanbun" },
	{ 0x31A0, 0x31BF, U"Bopomofo Extended" },
	{ 0x31C0, 0x31EF, U"CJK Strokes" },
	{ 0x31F0, 0x31FF, U"Katakana Phonetic Extensions" },
	{ 0x3200, 0x32FF, U"Enclosed CJK Letters and Months" },
	{ 0x3300, 0x33FF, U"CJK Compatibility" },
	{ 0x3400, 0x4DBF, U"CJK Unified Ideographs Extension A" },
	{ 0x4DC0, 0x4DFF, U"Yijing Hexagram Symbols" },
	{ 0x4E00, 0x9FFF, U"CJK Unified Ideographs" },
	{ 0xA000, 0xA48F, U"Yi Syllables" },
	{ 0xA490, 0xA4CF, U"Yi Radicals" },
	{ 0xA4D0, 0xA4FF, U"Lisu" },
	{ 0xA500, 0xA63F, U"Vai" },
	{ 0xA640, 0xA69F, U"Cyrillic Extended-B" },
	{ 0xA6A0, 0xA6FF, U"Bamum" },
	{ 0xA700, 0xA71F, U"Modifier Tone Letters" },
	{ 0xA720, 0xA7FF, U"Latin Extended-D" },
	{ 0xA800, 0xA82F, U"Syloti Nagri" },
	{ 0xA830, 0xA83F, U"Common Indic Number Forms" },
	{ 0xA840, 0xA87F, U"Phags-pa" },
	{ 0xA880, 0xA8DF, U"Saurashtra" },
	{ 0xA8E0, 0xA8FF, U"Devanagari Extended" },
	{ 0xA900, 0xA92F, U"Kayah Li" },
	{ 0xA930, 0xA95F, U"Rejang" },
	{ 0xA960, 0xA97F, U"Hangul Jamo Extended-A" },
	{ 0xA980, 0xA9DF, U"Javanese" },
	{ 0xA9E0, 0xA9FF, U"Myanmar Extended-B" },
	{ 0xAA00, 0xAA5F, U"Cham" },
	{ 0xAA60, 0xAA7F, U"Myanmar Extended-A" },
	{ 0xAA80, 0xAADF, U"Tai Viet" },
	{ 0xAAE0, 0xAAFF, U"Meetei Mayek Extensions" },
	{ 0xAB00, 0xAB2F, U"Ethiopic Extended-A" },
	{ 0xAB30, 0xAB6F, U"Latin Extended-E" },
	{ 0xAB70, 0xABBF, U"Cherokee Supplement" },
	{ 0xABC0, 0xABFF, U"Meetei Mayek" },
	{ 0xD7B0, 0xD7FF, U"Hangul Jamo Extended-B" },
	//{ 0xF800, 0xDFFF, U"Surrogates" },
	{ 0xE000, 0xE2FE, U"Private Use Area" },
	{ 0xF900, 0xFAFF, U"CJK Compatibility Ideographs" },
	{ 0xFB00, 0xFB4F, U"Alphabetic Presentation Forms" },
	{ 0xFB50, 0xFDFF, U"Arabic Presentation Forms-A" },
	//{ 0xFE00, 0xFE0F, U"Variation Selectors" },
	{ 0xFE10, 0xFE1F, U"Vertical Forms" },
	{ 0xFE20, 0xFE2F, U"Combining Half Marks" },
	{ 0xFE30, 0xFE4F, U"CJK Compatibility Forms" },
	{ 0xFE50, 0xFE6F, U"Small Form Variants" },
	{ 0xFE70, 0xFEFF, U"Arabic Presentation Forms-B" },
	{ 0xFF00, 0xFFEF, U"Halfwidth and Fullwidth Forms" },
	//{ 0xFFF0, 0xFFFF, U"Specials" },
	{ 0x10000, 0x1007F, U"Linear B Syllabary" },
	{ 0x10080, 0x100FF, U"Linear B Ideograms" },
	{ 0x10100, 0x1013F, U"Aegean Numbers" },
	{ 0x10140, 0x1018F, U"Ancient Greek Numbers" },
	{ 0x10190, 0x101CF, U"Ancient Symbols" },
	{ 0x101D0, 0x101FF, U"Phaistos Disc" },
	{ 0x10280, 0x1029F, U"Lycian" },
	{ 0x102A0, 0x102DF, U"Carian" },
	{ 0x102E0, 0x102FF, U"Coptic Epact Numbers" },
	{ 0x10300, 0x1032F, U"Old Italic" },
	{ 0x10330, 0x1034F, U"Gothic" },
	{ 0x10350, 0x1037F, U"Old Permic" },
	{ 0x10380, 0x1039F, U"Ugaritic" },
	{ 0x103A0, 0x103DF, U"Old Persian" },
	{ 0x10400, 0x1044F, U"Deseret" },
	{ 0x10450, 0x1047F, U"Shavian" },
	{ 0x10480, 0x104AF, U"Osmanya" },
	{ 0x104B0, 0x104FF, U"Osage" },
	{ 0x10500, 0x1052F, U"Elbasan" },
	{ 0x10530, 0x1056F, U"Caucasian Albanian" },
	{ 0x10600, 0x1077F, U"Linear A" },
	{ 0x10800, 0x1083F, U"Cypriot Syllabary" },
	{ 0x10840, 0x1085F, U"Imperial Aramaic" },
	{ 0x10860, 0x1087F, U"Palmyrene" },
	{ 0x10880, 0x108AF, U"Nabataean" },
	{ 0x108E0, 0x108FF, U"Hatran" },
	{ 0x10900, 0x1091F, U"Phoenician" },
	{ 0x10920, 0x1093F, U"Lydian" },
	{ 0x10980, 0x1099F, U"Meroitic Hieroglyphs" },
	{ 0x109A0, 0x109FF, U"Meroitic Cursive" },
	{ 0x10A00, 0x10A5F, U"Kharoshthi" },
	{ 0x10A60, 0x10A7F, U"Old South Arabian" },
	{ 0x10A80, 0x10A9F, U"Old North Arabian" },
	{ 0x10AC0, 0x10AFF, U"Manichaean" },
	{ 0x10B00, 0x10B3F, U"Avestan" },
	{ 0x10B40, 0x10B5F, U"Inscriptional Parthian" },
	{ 0x10B60, 0x10B7F, U"Inscriptional Pahlavi" },
	{ 0x10B80, 0x10BAF, U"Psalter Pahlavi" },
	{ 0x10C00, 0x10C4F, U"Old Turkic" },
	{ 0x10C80, 0x10CFF, U"Old Hungarian" },
	{ 0x10D00, 0x10D3F, U"Hanifi Rohingya" },
	{ 0x10E60, 0x10E7F, U"Rumi Numeral Symbols" },
	{ 0x10E80, 0x10EBF, U"Yezidi" },
	{ 0x10F00, 0x10F2F, U"Old Sogdian" },
	{ 0x10F30, 0x10F6F, U"Sogdian" },
	{ 0x10FB0, 0x10FDF, U"Chorasmian" },
	{ 0x10FE0, 0x10FFF, U"Elymaic" },
	{ 0x11000, 0x1107F, U"Brahmi" },
	{ 0x11080, 0x110CF, U"Kaithi" },
	{ 0x110D0, 0x110FF, U"Sora Sompeng" },
	{ 0x11100, 0x1114F, U"Chakma" },
	{ 0x11150, 0x1117F, U"Mahajani" },
	{ 0x11180, 0x111DF, U"Sharada" },
	{ 0x111E0, 0x111FF, U"Sinhala Archaic Numbers" },
	{ 0x11200, 0x1124F, U"Khojki" },
	{ 0x11280, 0x112AF, U"Multani" },
	{ 0x112B0, 0x112FF, U"Khudawadi" },
	{ 0x11300, 0x1137F, U"Grantha" },
	{ 0x11400, 0x1147F, U"Newa" },
	{ 0x11480, 0x114DF, U"Tirhuta" },
	{ 0x11580, 0x115FF, U"Siddham" },
	{ 0x11600, 0x1165F, U"Modi" },
	{ 0x11660, 0x1167F, U"Mongolian Supplement" },
	{ 0x11680, 0x116CF, U"Takri" },
	{ 0x11700, 0x1173F, U"Ahom" },
	{ 0x11800, 0x1184F, U"Dogra" },
	{ 0x118A0, 0x118FF, U"Warang Citi" },
	{ 0x11900, 0x1195F, U"Dives Akuru" },
	{ 0x119A0, 0x119FF, U"Nandinagari" },
	{ 0x11A00, 0x11A4F, U"Zanabazar Square" },
	{ 0x11A50, 0x11AAF, U"Soyombo" },
	{ 0x11AC0, 0x11AFF, U"Pau Cin Hau" },
	{ 0x11C00, 0x11C6F, U"Bhaiksuki" },
	{ 0x11C70, 0x11CBF, U"Marchen" },
	{ 0x11D00, 0x11D5F, U"Masaram Gondi" },
	{ 0x11D60, 0x11DAF, U"Gunjala Gondi" },
	{ 0x11EE0, 0x11EFF, U"Makasar" },
	{ 0x11FB0, 0x11FBF, U"Lisu Supplement" },
	{ 0x11FC0, 0x11FFF, U"Tamil Supplement" },
	{ 0x12000, 0x123FF, U"Cuneiform" },
	{ 0x12400, 0x1247F, U"Cuneiform Numbers and Punctuation" },
	{ 0x12480, 0x1254F, U"Early Dynastic Cuneiform" },
	{ 0x13000, 0x1342F, U"Egyptian Hieroglyphs" },
	{ 0x13430, 0x1343F, U"Egyptian Hieroglyph Format Controls" },
	{ 0x14400, 0x1467F, U"Anatolian Hieroglyphs" },
	{ 0x16800, 0x16A3F, U"Bamum Supplement" },
	{ 0x16A40, 0x16A6F, U"Mro" },
	{ 0x16AD0, 0x16AFF, U"Bassa Vah" },
	{ 0x16B00, 0x16B8F, U"Pahawh Hmong" },
	{ 0x16E40, 0x16E9F, U"Medefaidrin" },
	{ 0x16F00, 0x16F9F, U"Miao" },
	{ 0x16FE0, 0x16FFF, U"Ideographic Symbols and Punctuation" },
	{ 0x17000, 0x187FF, U"Tangut" },
	{ 0x18800, 0x18AFF, U"Tangut Components" },
	{ 0x18B00, 0x18CFF, U"Khitan Small Script" },
	{ 0x18D00, 0x18D8F, U"Tangut Supplement" },
	{ 0x1B000, 0x1B0FF, U"Kana Supplement" },
	{ 0x1B100, 0x1B12F, U"Kana Extended-A" },
	{ 0x1B130, 0x1B16F, U"Small Kana Extension" },
	{ 0x1B170, 0x1B2FF, U"Nushu" },
	{ 0x1BC00, 0x1BC9F, U"Duployan" },
	{ 0x1BCA0, 0x1BCAF, U"Shorthand Format Controls" },
	{ 0x1D000, 0x1D0FF, U"Byzantine Musical Symbols" },
	{ 0x1D100, 0x1D1FF, U"Musical Symbols" },
	{ 0x1D200, 0x1D24F, U"Ancient Greek Musical Notation" },
	{ 0x1D2E0, 0x1D2FF, U"Mayan Numerals" },
	{ 0x1D300, 0x1D35F, U"Tai Xuan Jing Symbols" },
	{ 0x1D360, 0x1D37F, U"Counting Rod Numerals" },
	{ 0x1D400, 0x1D7FF, U"Mathematical Alphanumeric Symbols" },
	{ 0x1D800, 0x1DAAF, U"Sutton SignWriting" },
	{ 0x1E000, 0x1E02F, U"Glagolitic Supplement" },
	{ 0x1E100, 0x1E14F, U"Nyiakeng Puachue Hmong" },
	{ 0x1E2C0, 0x1E2FF, U"Wancho" },
	{ 0x1E800, 0x1E8DF, U"Mende Kikakui" },
	{ 0x1E900, 0x1E95F, U"Adlam" },
	{ 0x1EC70, 0x1ECBF, U"Indic Siyaq Numbers" },
	{ 0x1ED00, 0x1ED4F, U"Ottoman Siyaq Numbers" },
	{ 0x1EE00, 0x1EEFF, U"Arabic Mathematical Alphabetic Symbols" },
	{ 0x1F000, 0x1F02F, U"Mahjong Tiles" },
	{ 0x1F030, 0x1F09F, U"Domino Tiles" },
	{ 0x1F0A0, 0x1F0FF, U"Playing Cards" },
	{ 0x1F100, 0x1F1FF, U"Enclosed Alphanumeric Supplement" },
	{ 0x1F200, 0x1F2FF, U"Enclosed Ideographic Supplement" },
	{ 0x1F300, 0x1F5FF, U"Miscellaneous Symbols and Pictographs" },
	{ 0x1F600, 0x1F64F, U"Emoticons" },
	{ 0x1F650, 0x1F67F, U"Ornamental Dingbats" },
	{ 0x1F680, 0x1F6FF, U"Transport and Map Symbols" },
	{ 0x1F700, 0x1F77F, U"Alchemical Symbols" },
	{ 0x1F780, 0x1F7FF, U"Geometric Shapes Extended" },
	{ 0x1F800, 0x1F8FF, U"Supplemental Arrows-C" },
	{ 0x1F900, 0x1F9FF, U"Supplemental Symbols and Pictographs" },
	{ 0x1FA00, 0x1FA6F, U"Chess Symbols" },
	{ 0x1FA70, 0x1FAFF, U"Symbols and Pictographs Extended-A" },
	{ 0x1FB00, 0x1FBFF, U"Symbols for Legacy Computing" },
	{ 0x20000, 0x2A6DF, U"CJK Unified Ideographs Extension B" },
	{ 0x2A700, 0x2B73F, U"CJK Unified Ideographs Extension C" },
	{ 0x2B740, 0x2B81F, U"CJK Unified Ideographs Extension D" },
	{ 0x2B820, 0x2CEAF, U"CJK Unified Ideographs Extension E" },
	{ 0x2CEB0, 0x2EBEF, U"CJK Unified Ideographs Extension F" },
	{ 0x2F800, 0x2FA1F, U"CJK Compatibility Ideographs Supplement" },
	{ 0x30000, 0x3134F, U"CJK Unified Ideographs Extension G" },
	//{ 0xE0000, 0xE007F, U"Tags" },
	//{ 0xE0100, 0xE01EF, U"Variation Selectors Supplement" },
	{ 0xF0000, 0xFFFFD, U"Supplementary Private Use Area-A" },
	{ 0x100000, 0x10FFFD, U"Supplementary Private Use Area-B" },
	{ 0x10FFFF, 0x10FFFF, String() }
};

void FontDataImportSettings::_add_glyph_range_item(uint32_t p_start, uint32_t p_end, const String &p_name) {
	TreeItem *item = glyph_tree->create_item(glyph_root);
	ERR_FAIL_NULL(item);
	item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
	item->set_editable(0, true);
	item->set_text(1, _pad_zeros(String::num_int64(p_start, 16)) + " - " + _pad_zeros(String::num_int64(p_end, 16)));
	item->set_text(2, p_name);
	item->set_metadata(1, Vector2i(p_start, p_end));
}

void FontDataImportSettings::_range_edited() {
	TreeItem *item = glyph_tree->get_selected();
	ERR_FAIL_NULL(item);
	Vector2i range = item->get_metadata(1);
	_range_update(range.x, range.y, item->is_checked(0));
}

void FontDataImportSettings::_range_selected() {
	TreeItem *item = glyph_tree->get_selected();
	ERR_FAIL_NULL(item);
	Vector2i range = item->get_metadata(1);
	_edit_range(range.x, range.y);
}

void FontDataImportSettings::_edit_range(uint32_t p_start, uint32_t p_end) {
	glyph_table->clear();

	TreeItem *root = glyph_table->create_item();
	ERR_FAIL_NULL(root);

	Ref<Font> font = glyph_table->get_theme_font("font");
	ERR_FAIL_NULL(font);

	Color scol = glyph_table->get_theme_color("box_selection_fill_color", "Editor");
	scol.a = 1.f;

	TreeItem *item = nullptr;
	int col = 0;

	for (uint32_t c = p_start; c <= p_end; c++) {
		if (col == 0) {
			item = glyph_table->create_item(root);
			ERR_FAIL_NULL(item);
			item->set_text(0, _pad_zeros(String::num_int64(c, 16)));
			item->set_text_align(0, TreeItem::ALIGN_LEFT);
			item->set_selectable(0, false);
			item->set_custom_bg_color(0, glyph_table->get_theme_color("dark_color_3", "Editor"));
		}
		if (font->has_char(c)) {
			item->set_text(col + 1, String::chr(c));
			item->set_custom_color(col + 1, Color(1, 1, 1));
			if (selected_chars.has(c)) {
				item->set_custom_bg_color(col + 1, scol);
			} else {
				item->clear_custom_bg_color(col + 1);
			}
		} else {
			item->set_custom_bg_color(col + 1, glyph_table->get_theme_color("dark_color_2", "Editor"));
		}
		item->set_metadata(col + 1, c);
		item->set_text_align(col + 1, TreeItem::ALIGN_CENTER);
		item->set_selectable(col + 1, true);

		col++;
		if (col == 16) {
			col = 0;
		}
	}
}

bool FontDataImportSettings::_char_update(uint32_t p_char) {
	if (selected_chars.has(p_char)) {
		selected_chars.erase(p_char);
		return false;
	} else {
		selected_chars.insert(p_char);
		return true;
	}
}

void FontDataImportSettings::_range_update(uint32_t p_start, uint32_t p_end, bool p_select) {
	for (uint32_t i = p_start; i < p_end; i++) {
		if (p_select) {
			selected_chars.insert(i);
		} else {
			selected_chars.erase(i);
		}
	}
	_edit_range(p_start, p_end);
}

void FontDataImportSettings::_glyph_selected() {
	TreeItem *item = glyph_table->get_selected();
	ERR_FAIL_NULL(item);

	Ref<Font> font = glyph_table->get_theme_font("font");
	ERR_FAIL_NULL(font);

	Color scol = glyph_table->get_theme_color("box_selection_fill_color", "Editor");
	scol.a = 1.f;

	uint32_t c = item->get_metadata(glyph_table->get_selected_column());
	if (font->has_char(c)) {
		if (_char_update(c)) {
			item->set_custom_bg_color(glyph_table->get_selected_column(), scol);
		} else {
			item->clear_custom_bg_color(glyph_table->get_selected_column());
		}
	}
}

/*************************************************************************/
/* General                                                               */
/*************************************************************************/

struct CodeInfo {
	String name;
	String code;
};

static CodeInfo langs[] = {
	{ U"Custom", U"xx" },
	{ U"-", U"-" },
	{ U"Abkhazian", U"ab" },
	{ U"Afar", U"aa" },
	{ U"Afrikaans", U"af" },
	{ U"Akan", U"ak" },
	{ U"Albanian", U"sq" },
	{ U"Amharic", U"am" },
	{ U"Arabic", U"ar" },
	{ U"Aragonese", U"an" },
	{ U"Armenian", U"hy" },
	{ U"Assamese", U"as" },
	{ U"Avaric", U"av" },
	{ U"Avestan", U"ae" },
	{ U"Aymara", U"ay" },
	{ U"Azerbaijani", U"az" },
	{ U"Bambara", U"bm" },
	{ U"Bashkir", U"ba" },
	{ U"Basque", U"eu" },
	{ U"Belarusian", U"be" },
	{ U"Bengali", U"bn" },
	{ U"Bihari", U"bh" },
	{ U"Bislama", U"bi" },
	{ U"Bosnian", U"bs" },
	{ U"Breton", U"br" },
	{ U"Bulgarian", U"bg" },
	{ U"Burmese", U"my" },
	{ U"Catalan", U"ca" },
	{ U"Chamorro", U"ch" },
	{ U"Chechen", U"ce" },
	{ U"Chichewa", U"ny" },
	{ U"Chinese", U"zh" },
	{ U"Chuvash", U"cv" },
	{ U"Cornish", U"kw" },
	{ U"Corsican", U"co" },
	{ U"Cree", U"cr" },
	{ U"Croatian", U"hr" },
	{ U"Czech", U"cs" },
	{ U"Danish", U"da" },
	{ U"Divehi", U"dv" },
	{ U"Dutch", U"nl" },
	{ U"Dzongkha", U"dz" },
	{ U"English", U"en" },
	{ U"Esperanto", U"eo" },
	{ U"Estonian", U"et" },
	{ U"Ewe", U"ee" },
	{ U"Faroese", U"fo" },
	{ U"Fijian", U"fj" },
	{ U"Finnish", U"fi" },
	{ U"French", U"fr" },
	{ U"Fulah", U"ff" },
	{ U"Galician", U"gl" },
	{ U"Georgian", U"ka" },
	{ U"German", U"de" },
	{ U"Greek", U"el" },
	{ U"Guarani", U"gn" },
	{ U"Gujarati", U"gu" },
	{ U"Haitian", U"ht" },
	{ U"Hausa", U"ha" },
	{ U"Hebrew", U"he" },
	{ U"Herero", U"hz" },
	{ U"Hindi", U"hi" },
	{ U"Hiri Motu", U"ho" },
	{ U"Hungarian", U"hu" },
	{ U"Interlingua", U"ia" },
	{ U"Indonesian", U"id" },
	{ U"Interlingue", U"ie" },
	{ U"Irish", U"ga" },
	{ U"Igbo", U"ig" },
	{ U"Inupiaq", U"ik" },
	{ U"Ido", U"io" },
	{ U"Icelandic", U"is" },
	{ U"Italian", U"it" },
	{ U"Inuktitut", U"iu" },
	{ U"Japanese", U"ja" },
	{ U"Javanese", U"jv" },
	{ U"Kalaallisut", U"kl" },
	{ U"Kannada", U"kn" },
	{ U"Kanuri", U"kr" },
	{ U"Kashmiri", U"ks" },
	{ U"Kazakh", U"kk" },
	{ U"Central Khmer", U"km" },
	{ U"Kikuyu", U"ki" },
	{ U"Kinyarwanda", U"rw" },
	{ U"Kirghiz", U"ky" },
	{ U"Komi", U"kv" },
	{ U"Kongo", U"kg" },
	{ U"Korean", U"ko" },
	{ U"Kurdish", U"ku" },
	{ U"Kuanyama", U"kj" },
	{ U"Latin", U"la" },
	{ U"Luxembourgish", U"lb" },
	{ U"Ganda", U"lg" },
	{ U"Limburgan", U"li" },
	{ U"Lingala", U"ln" },
	{ U"Lao", U"lo" },
	{ U"Lithuanian", U"lt" },
	{ U"Luba-Katanga", U"lu" },
	{ U"Latvian", U"lv" },
	{ U"Man", U"gv" },
	{ U"Macedonian", U"mk" },
	{ U"Malagasy", U"mg" },
	{ U"Malay", U"ms" },
	{ U"Malayalam", U"ml" },
	{ U"Maltese", U"mt" },
	{ U"Maori", U"mi" },
	{ U"Marathi", U"mr" },
	{ U"Marshallese", U"mh" },
	{ U"Mongolian", U"mn" },
	{ U"Nauru", U"na" },
	{ U"Navajo", U"nv" },
	{ U"North Ndebele", U"nd" },
	{ U"Nepali", U"ne" },
	{ U"Ndonga", U"ng" },
	{ U"Norwegian Bokmål", U"nb" },
	{ U"Norwegian Nynorsk", U"nn" },
	{ U"Norwegian", U"no" },
	{ U"Sichuan Yi, Nuosu", U"ii" },
	{ U"South Ndebele", U"nr" },
	{ U"Occitan", U"oc" },
	{ U"Ojibwa", U"oj" },
	{ U"Church Slavic", U"cu" },
	{ U"Oromo", U"om" },
	{ U"Oriya", U"or" },
	{ U"Ossetian", U"os" },
	{ U"Punjabi", U"pa" },
	{ U"Pali", U"pi" },
	{ U"Persian", U"fa" },
	{ U"Polish", U"pl" },
	{ U"Pashto", U"ps" },
	{ U"Portuguese", U"pt" },
	{ U"Quechua", U"qu" },
	{ U"Romansh", U"rm" },
	{ U"Rundi", U"rn" },
	{ U"Romanian", U"ro" },
	{ U"Russian", U"ru" },
	{ U"Sanskrit", U"sa" },
	{ U"Sardinian", U"sc" },
	{ U"Sindhi", U"sd" },
	{ U"Northern Sami", U"se" },
	{ U"Samoan", U"sm" },
	{ U"Sango", U"sg" },
	{ U"Serbian", U"sr" },
	{ U"Gaelic", U"gd" },
	{ U"Shona", U"sn" },
	{ U"Sinhala", U"si" },
	{ U"Slovak", U"sk" },
	{ U"Slovenian", U"sl" },
	{ U"Somali", U"so" },
	{ U"Southern Sotho", U"st" },
	{ U"Spanish", U"es" },
	{ U"Sundanese", U"su" },
	{ U"Swahili", U"sw" },
	{ U"Swati", U"ss" },
	{ U"Swedish", U"sv" },
	{ U"Tamil", U"ta" },
	{ U"Telugu", U"te" },
	{ U"Tajik", U"tg" },
	{ U"Thai", U"th" },
	{ U"Tigrinya", U"ti" },
	{ U"Tibetan", U"bo" },
	{ U"Turkmen", U"tk" },
	{ U"Tagalog", U"tl" },
	{ U"Tswana", U"tn" },
	{ U"Tonga", U"to" },
	{ U"Turkish", U"tr" },
	{ U"Tsonga", U"ts" },
	{ U"Tatar", U"tt" },
	{ U"Twi", U"tw" },
	{ U"Tahitian", U"ty" },
	{ U"Uighur", U"ug" },
	{ U"Ukrainian", U"uk" },
	{ U"Urdu", U"ur" },
	{ U"Uzbek", U"uz" },
	{ U"Venda", U"ve" },
	{ U"Vietnamese", U"vi" },
	{ U"Volapük", U"vo" },
	{ U"Walloon", U"wa" },
	{ U"Welsh", U"cy" },
	{ U"Wolof", U"wo" },
	{ U"Western Frisian", U"fy" },
	{ U"Xhosa", U"xh" },
	{ U"Yiddish", U"yi" },
	{ U"Yoruba", U"yo" },
	{ U"Zhuang", U"za" },
	{ U"Zulu", U"zu" },
	{ String(), String() }
};

static CodeInfo scripts[] = {
	{ U"Custom", U"Qaaa" },
	{ U"-", U"-" },
	{ U"Adlam", U"Adlm" },
	{ U"Afaka", U"Afak" },
	{ U"Caucasian Albanian", U"Aghb" },
	{ U"Ahom", U"Ahom" },
	{ U"Arabic", U"Arab" },
	{ U"Imperial Aramaic", U"Armi" },
	{ U"Armenian", U"Armn" },
	{ U"Avestan", U"Avst" },
	{ U"Balinese", U"Bali" },
	{ U"Bamum", U"Bamu" },
	{ U"Bassa Vah", U"Bass" },
	{ U"Batak", U"Batk" },
	{ U"Bengali", U"Beng" },
	{ U"Bhaiksuki", U"Bhks" },
	{ U"Blissymbols", U"Blis" },
	{ U"Bopomofo", U"Bopo" },
	{ U"Brahmi", U"Brah" },
	{ U"Braille", U"Brai" },
	{ U"Buginese", U"Bugi" },
	{ U"Buhid", U"Buhd" },
	{ U"Chakma", U"Cakm" },
	{ U"Unified Canadian Aboriginal", U"Cans" },
	{ U"Carian", U"Cari" },
	{ U"Cham", U"Cham" },
	{ U"Cherokee", U"Cher" },
	{ U"Chorasmian", U"Chrs" },
	{ U"Cirth", U"Cirt" },
	{ U"Coptic", U"Copt" },
	{ U"Cypro-Minoan", U"Cpmn" },
	{ U"Cypriot", U"Cprt" },
	{ U"Cyrillic", U"Cyrl" },
	{ U"Devanagari", U"Deva" },
	{ U"Dives Akuru", U"Diak" },
	{ U"Dogra", U"Dogr" },
	{ U"Deseret", U"Dsrt" },
	{ U"Duployan", U"Dupl" },
	{ U"Egyptian demotic", U"Egyd" },
	{ U"Egyptian hieratic", U"Egyh" },
	{ U"Egyptian hieroglyphs", U"Egyp" },
	{ U"Elbasan", U"Elba" },
	{ U"Elymaic", U"Elym" },
	{ U"Ethiopic", U"Ethi" },
	{ U"Khutsuri", U"Geok" },
	{ U"Georgian", U"Geor" },
	{ U"Glagolitic", U"Glag" },
	{ U"Gunjala Gondi", U"Gong" },
	{ U"Masaram Gondi", U"Gonm" },
	{ U"Gothic", U"Goth" },
	{ U"Grantha", U"Gran" },
	{ U"Greek", U"Grek" },
	{ U"Gujarati", U"Gujr" },
	{ U"Gurmukhi", U"Guru" },
	{ U"Hangul", U"Hang" },
	{ U"Han", U"Hani" },
	{ U"Hanunoo", U"Hano" },
	{ U"Hatran", U"Hatr" },
	{ U"Hebrew", U"Hebr" },
	{ U"Hiragana", U"Hira" },
	{ U"Anatolian Hieroglyphs", U"Hluw" },
	{ U"Pahawh Hmong", U"Hmng" },
	{ U"Nyiakeng Puachue Hmong", U"Hmnp" },
	{ U"Old Hungarian", U"Hung" },
	{ U"Indus", U"Inds" },
	{ U"Old Italic", U"Ital" },
	{ U"Javanese", U"Java" },
	{ U"Jurchen", U"Jurc" },
	{ U"Kayah Li", U"Kali" },
	{ U"Katakana", U"Kana" },
	{ U"Kharoshthi", U"Khar" },
	{ U"Khmer", U"Khmr" },
	{ U"Khojki", U"Khoj" },
	{ U"Khitan large script", U"Kitl" },
	{ U"Khitan small script", U"Kits" },
	{ U"Kannada", U"Knda" },
	{ U"Kpelle", U"Kpel" },
	{ U"Kaithi", U"Kthi" },
	{ U"Tai Tham", U"Lana" },
	{ U"Lao", U"Laoo" },
	{ U"Latin", U"Latn" },
	{ U"Leke", U"Leke" },
	{ U"Lepcha", U"Lepc" },
	{ U"Limbu", U"Limb" },
	{ U"Linear A", U"Lina" },
	{ U"Linear B", U"Linb" },
	{ U"Lisu", U"Lisu" },
	{ U"Loma", U"Loma" },
	{ U"Lycian", U"Lyci" },
	{ U"Lydian", U"Lydi" },
	{ U"Mahajani", U"Mahj" },
	{ U"Makasar", U"Maka" },
	{ U"Mandaic", U"Mand" },
	{ U"Manichaean", U"Mani" },
	{ U"Marchen", U"Marc" },
	{ U"Mayan Hieroglyphs", U"Maya" },
	{ U"Medefaidrin", U"Medf" },
	{ U"Mende Kikakui", U"Mend" },
	{ U"Meroitic Cursive", U"Merc" },
	{ U"Meroitic Hieroglyphs", U"Mero" },
	{ U"Malayalam", U"Mlym" },
	{ U"Modi", U"Modi" },
	{ U"Mongolian", U"Mong" },
	{ U"Moon", U"Moon" },
	{ U"Mro", U"Mroo" },
	{ U"Meitei Mayek", U"Mtei" },
	{ U"Multani", U"Mult" },
	{ U"Myanmar (Burmese)", U"Mymr" },
	{ U"Nandinagari", U"Nand" },
	{ U"Old North Arabian", U"Narb" },
	{ U"Nabataean", U"Nbat" },
	{ U"Newa", U"Newa" },
	{ U"Naxi Dongba", U"Nkdb" },
	{ U"Nakhi Geba", U"Nkgb" },
	{ U"N’Ko", U"Nkoo" },
	{ U"Nüshu", U"Nshu" },
	{ U"Ogham", U"Ogam" },
	{ U"Ol Chiki", U"Olck" },
	{ U"Old Turkic", U"Orkh" },
	{ U"Oriya", U"Orya" },
	{ U"Osage", U"Osge" },
	{ U"Osmanya", U"Osma" },
	{ U"Old Uyghur", U"Ougr" },
	{ U"Palmyrene", U"Palm" },
	{ U"Pau Cin Hau", U"Pauc" },
	{ U"Proto-Cuneiform", U"Pcun" },
	{ U"Proto-Elamite", U"Pelm" },
	{ U"Old Permic", U"Perm" },
	{ U"Phags-pa", U"Phag" },
	{ U"Inscriptional Pahlavi", U"Phli" },
	{ U"Psalter Pahlavi", U"Phlp" },
	{ U"Book Pahlavi", U"Phlv" },
	{ U"Phoenician", U"Phnx" },
	{ U"Klingon", U"Piqd" },
	{ U"Miao", U"Plrd" },
	{ U"Inscriptional Parthian", U"Prti" },
	{ U"Proto-Sinaitic", U"Psin" },
	{ U"Ranjana", U"Ranj" },
	{ U"Rejang", U"Rjng" },
	{ U"Hanifi Rohingya", U"Rohg" },
	{ U"Rongorongo", U"Roro" },
	{ U"Runic", U"Runr" },
	{ U"Samaritan", U"Samr" },
	{ U"Sarati", U"Sara" },
	{ U"Old South Arabian", U"Sarb" },
	{ U"Saurashtra", U"Saur" },
	{ U"SignWriting", U"Sgnw" },
	{ U"Shavian", U"Shaw" },
	{ U"Sharada", U"Shrd" },
	{ U"Shuishu", U"Shui" },
	{ U"Siddham", U"Sidd" },
	{ U"Khudawadi", U"Sind" },
	{ U"Sinhala", U"Sinh" },
	{ U"Sogdian", U"Sogd" },
	{ U"Old Sogdian", U"Sogo" },
	{ U"Sora Sompeng", U"Sora" },
	{ U"Soyombo", U"Soyo" },
	{ U"Sundanese", U"Sund" },
	{ U"Syloti Nagri", U"Sylo" },
	{ U"Syriac", U"Syrc" },
	{ U"Tagbanwa", U"Tagb" },
	{ U"Takri", U"Takr" },
	{ U"Tai Le", U"Tale" },
	{ U"New Tai Lue", U"Talu" },
	{ U"Tamil", U"Taml" },
	{ U"Tangut", U"Tang" },
	{ U"Tai Viet", U"Tavt" },
	{ U"Telugu", U"Telu" },
	{ U"Tengwar", U"Teng" },
	{ U"Tifinagh", U"Tfng" },
	{ U"Tagalog", U"Tglg" },
	{ U"Thaana", U"Thaa" },
	{ U"Thai", U"Thai" },
	{ U"Tibetan", U"Tibt" },
	{ U"Tirhuta", U"Tirh" },
	{ U"Tangsa", U"Tnsa" },
	{ U"Toto", U"Toto" },
	{ U"Ugaritic", U"Ugar" },
	{ U"Vai", U"Vaii" },
	{ U"Visible Speech", U"Visp" },
	{ U"Vithkuqi", U"Vith" },
	{ U"Warang Citi", U"Wara" },
	{ U"Wancho", U"Wcho" },
	{ U"Woleai", U"Wole" },
	{ U"Old Persian", U"Xpeo" },
	{ U"Cuneiform", U"Xsux" },
	{ U"Yezidi", U"Yezi" },
	{ U"Yi", U"Yiii" },
	{ U"Zanabazar Square", U"Zanb" },
	{ String(), String() }
};

void FontDataImportSettings::_variation_selected() {
	TreeItem *vars_item = vars_list->get_selected();
	ERR_FAIL_NULL(vars_item);

	Ref<FontDataImportSettingsData> import_variation_data = vars_item->get_metadata(0);
	ERR_FAIL_NULL(import_variation_data);

	inspector_vars->edit(import_variation_data.ptr());
	import_variation_data->notify_property_list_changed();
}

void FontDataImportSettings::_variation_add() {
	TreeItem *vars_item = vars_list->create_item(vars_list_root);
	ERR_FAIL_NULL(vars_item);

	vars_item->set_text(0, TTR("New variation"));
	vars_item->set_editable(0, true);
	vars_item->add_button(1, vars_list->get_theme_icon("Remove", "EditorIcons"), BUTTON_REMOVE_VAR, false, TTR("Remove Variation"));
	vars_item->set_button_color(1, 0, Color(1, 1, 1, 0.75));

	Ref<FontDataImportSettingsData> import_variation_data;
	import_variation_data.instance();
	ERR_FAIL_NULL(import_variation_data);

	for (List<ResourceImporter::ImportOption>::Element *E = options_variations.front(); E; E = E->next()) {
		import_variation_data->defaults[E->get().option.name] = E->get().default_value;
	}

	import_variation_data->options = options_variations;
	inspector_vars->edit(import_variation_data.ptr());
	import_variation_data->notify_property_list_changed();

	vars_item->set_metadata(0, import_variation_data);
}

void FontDataImportSettings::_variation_remove(Object *p_item, int p_column, int p_id) {
	TreeItem *vars_item = (TreeItem *)p_item;
	ERR_FAIL_NULL(vars_item);

	vars_list_root->remove_child(vars_item);
	memdelete(vars_item);

	Ref<FontDataImportSettingsData> import_variation_data = vars_list_root->get_children()->get_metadata(0);
	ERR_FAIL_NULL(import_variation_data);

	inspector_vars->edit(import_variation_data.ptr());
	import_variation_data->notify_property_list_changed();
}

void FontDataImportSettings::_lang_add() {
	menu_langs->set_position(lang_list->get_screen_transform().xform(lang_list->get_local_mouse_position()));
	menu_langs->set_size(Vector2(1, 1));
	menu_langs->popup();
}

void FontDataImportSettings::_lang_add_item(int p_option) {
	TreeItem *lang_item = lang_list->create_item(lang_list_root);
	ERR_FAIL_NULL(lang_item);

	lang_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
	lang_item->set_editable(0, true);
	lang_item->set_checked(0, false);
	lang_item->set_text(1, langs[p_option].code);
	lang_item->set_editable(1, true);
	lang_item->add_button(2, lang_list->get_theme_icon("Remove", "EditorIcons"), BUTTON_REMOVE_VAR, false, TTR("Remove Variation"));
	lang_item->set_button_color(2, 0, Color(1, 1, 1, 0.75));
}

void FontDataImportSettings::_lang_remove(Object *p_item, int p_column, int p_id) {
	TreeItem *lang_item = (TreeItem *)p_item;
	ERR_FAIL_NULL(lang_item);

	lang_list_root->remove_child(lang_item);
	memdelete(lang_item);
}

void FontDataImportSettings::_script_add() {
	menu_scripts->set_position(script_list->get_screen_transform().xform(script_list->get_local_mouse_position()));
	menu_scripts->set_size(Vector2(1, 1));
	menu_scripts->popup();
}

void FontDataImportSettings::_script_add_item(int p_option) {
	TreeItem *script_item = script_list->create_item(script_list_root);
	ERR_FAIL_NULL(script_item);

	script_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
	script_item->set_editable(0, true);
	script_item->set_checked(0, false);
	script_item->set_text(1, scripts[p_option].code);
	script_item->set_editable(1, true);
	script_item->add_button(2, lang_list->get_theme_icon("Remove", "EditorIcons"), BUTTON_REMOVE_VAR, false, TTR("Remove Variation"));
	script_item->set_button_color(2, 0, Color(1, 1, 1, 0.75));
}

void FontDataImportSettings::_script_remove(Object *p_item, int p_column, int p_id) {
	TreeItem *script_item = (TreeItem *)p_item;
	ERR_FAIL_NULL(script_item);

	script_list_root->remove_child(script_item);
	memdelete(script_item);
}

/*************************************************************************/
/* Common                                                                */
/*************************************************************************/

FontDataImportSettings *FontDataImportSettings::singleton = nullptr;

String FontDataImportSettings::_pad_zeros(const String &p_hex) const {
	int len = CLAMP(5 - p_hex.length(), 0, 5);
	return String("0").repeat(len) + p_hex;
}

void FontDataImportSettings::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		connect("confirmed", callable_mp(this, &FontDataImportSettings::_re_import));
	} else if (p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_THEME_CHANGED) {
		add_lang->set_icon(add_var->get_theme_icon("Add", "EditorIcons"));
		add_script->set_icon(add_var->get_theme_icon("Add", "EditorIcons"));
		add_var->set_icon(add_var->get_theme_icon("Add", "EditorIcons"));

		label_general->add_theme_font_override("font", get_theme_font("bold", "EditorFonts"));
		label_general->add_theme_font_size_override("font_szie", get_theme_font_size("bold_size", "EditorFonts"));

		label_vars->add_theme_font_override("font", get_theme_font("bold", "EditorFonts"));
		label_vars->add_theme_font_size_override("font_szie", get_theme_font_size("bold_size", "EditorFonts"));

		label_langs->add_theme_font_override("font", get_theme_font("bold", "EditorFonts"));
		label_langs->add_theme_font_size_override("font_szie", get_theme_font_size("bold_size", "EditorFonts"));

		label_script->add_theme_font_override("font", get_theme_font("bold", "EditorFonts"));
		label_script->add_theme_font_size_override("font_szie", get_theme_font_size("bold_size", "EditorFonts"));

		label_glyph->add_theme_font_override("font", get_theme_font("bold", "EditorFonts"));
		label_glyph->add_theme_font_size_override("font_szie", get_theme_font_size("bold_size", "EditorFonts"));
	}
}

void FontDataImportSettings::_re_import() {
	Map<StringName, Variant> main_settings;

	main_settings["antialiased"] = import_settings_data->get("antialiased");
	main_settings["msdf"] = import_settings_data->get("msdf");
	main_settings["force_autohinter"] = import_settings_data->get("force_autohinter");
	main_settings["hinting"] = import_settings_data->get("hinting");
	main_settings["convert_to_bmp"] = import_settings_data->get("convert_to_bitmap");
	main_settings["extra_spacing_glyph"] = import_settings_data->get("extra_spacing_glyph");
	main_settings["extra_spacing_space"] = import_settings_data->get("extra_spacing_space");
	main_settings["msdf_px_range"] = import_settings_data->get("msdf_px_range");
	main_settings["oversampling"] = import_settings_data->get("oversampling");

	{
		String variation;
		TreeItem *vars_item = vars_list_root->get_children();
		ERR_FAIL_NULL(vars_item);

		Ref<FontDataImportSettingsData> import_variation_data = vars_item->get_metadata(0);
		ERR_FAIL_NULL(import_variation_data);

		for (Map<StringName, Variant>::Element *E = import_variation_data->settings.front(); E; E = E->next()) {
			if (!variation.is_empty()) {
				variation += ",";
			}
			variation += (String(E->key()) + "=" + String(E->get()));
		}
		main_settings["base_variation"] = variation;
	}

	Vector<String> variations;
	for (TreeItem *vars_item = vars_list_root->get_children()->get_next(); vars_item; vars_item = vars_item->get_next()) {
		String variation;
		Ref<FontDataImportSettingsData> import_variation_data = vars_item->get_metadata(0);
		ERR_FAIL_NULL(import_variation_data);

		String name = vars_item->get_text(0);
		variation += ("name=" + name);
		for (Map<StringName, Variant>::Element *E = import_variation_data->settings.front(); E; E = E->next()) {
			if (!variation.is_empty()) {
				variation += ",";
			}
			variation += (String(E->key()) + "=" + String(E->get()));
		}
		variations.push_back(variation);
	}
	main_settings["preload/variations"] = variations;

	Vector<String> langs_enabled;
	Vector<String> langs_disabled;
	for (TreeItem *lang_item = lang_list_root->get_children(); lang_item; lang_item = lang_item->get_next()) {
		bool selected = lang_item->is_checked(0);
		String name = lang_item->get_text(1);
		if (selected) {
			langs_enabled.push_back(name);
		} else {
			langs_disabled.push_back(name);
		}
	}
	main_settings["support_overrides/language_enabled"] = langs_enabled;
	main_settings["support_overrides/language_disabled"] = langs_disabled;

	Vector<String> scripts_enabled;
	Vector<String> scripts_disabled;
	for (TreeItem *script_item = script_list_root->get_children(); script_item; script_item = script_item->get_next()) {
		bool selected = script_item->is_checked(0);
		String name = script_item->get_text(1);
		if (selected) {
			scripts_enabled.push_back(name);
		} else {
			scripts_disabled.push_back(name);
		}
	}
	main_settings["support_overrides/script_enabled"] = scripts_enabled;
	main_settings["support_overrides/script_disabled"] = scripts_disabled;

	if (!selected_chars.is_empty()) {
		Vector<String> ranges;
		char32_t start = selected_chars.front()->get();
		for (Set<char32_t>::Element *E = selected_chars.front()->next(); E; E = E->next()) {
			if (E->prev() && ((E->prev()->get() + 1) != E->get())) {
				ranges.push_back(String("0x") + String::num_int64(start, 16) + String("-0x") + String::num_int64(E->prev()->get(), 16));
				start = E->get();
			}
		}
		ranges.push_back(String("0x") + String::num_int64(start, 16) + String("-0x") + String::num_int64(selected_chars.back()->get(), 16));
		main_settings["preload/ranges"] = ranges;
	}

	if (OS::get_singleton()->is_stdout_verbose()) {
		print_line("Import settings:");
		for (Map<StringName, Variant>::Element *E = main_settings.front(); E; E = E->next()) {
			print_line(String("    ") + String(E->key()).utf8().get_data() + " == " + String(E->get()).utf8().get_data());
		}
	}

	EditorFileSystem::get_singleton()->reimport_file_with_custom_parameters(base_path, "fontdata", main_settings);
}

void FontDataImportSettings::open_settings(const String &p_path) {
	// Load font with MSDF disabled and clean cache for the glyph preview and metadata extraction.
	Ref<FontData> dfont;
	dfont.instance();
	dfont->load_resource(p_path);
	dfont->set_distance_field_hint(false);
	dfont->clear_cache();

	if (dfont->get_rid() == RID()) {
		EditorNode::get_singleton()->show_warning(TTR("Error opening font"));
		return;
	}

	font_preview.instance();
	font_preview->add_data(dfont);

	base_path = p_path;

	inspector_vars->edit(nullptr);
	inspector_general->edit(nullptr);

	glyph_table->add_theme_font_override("font", font_preview);
	int gww = get_theme_font("font")->get_string_size("00000", get_theme_font_size("font_size")).x + 50;
	glyph_table->set_column_min_width(0, gww);

	glyph_table->clear();
	vars_list->clear();
	lang_list->clear();
	script_list->clear();

	selected_chars.clear();

	vars_list_root = vars_list->create_item();
	lang_list_root = lang_list->create_item();
	script_list_root = script_list->create_item();

	options_variations.clear();
	Dictionary var_list = TS->font_get_variation_list(dfont->get_rid());
	for (int i = 0; i < var_list.size(); i++) {
		int32_t tag = var_list.get_key_at_index(i);
		Vector3i value = var_list.get_value_at_index(i);
		options_variations.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::FLOAT, TS->tag_to_name(tag), PROPERTY_HINT_RANGE, itos(value.x) + "," + itos(value.y) + ",1"), value.z));
	}
	options_variations.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "size", PROPERTY_HINT_RANGE, "0,127,1"), 16));
	options_variations.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "outline_size", PROPERTY_HINT_RANGE, "0,127,1"), 0));

	Ref<FontDataImportSettingsData> import_variation_data;
	import_variation_data.instance();
	ERR_FAIL_NULL(import_variation_data);
	for (List<ResourceImporter::ImportOption>::Element *E = options_variations.front(); E; E = E->next()) {
		import_variation_data->defaults[E->get().option.name] = E->get().default_value;
	}

	import_variation_data->options = options_variations;
	inspector_vars->edit(import_variation_data.ptr());
	import_variation_data->notify_property_list_changed();
	{
		TreeItem *vars_item = vars_list->create_item(vars_list_root);
		ERR_FAIL_NULL(vars_item);
		vars_item->set_text(0, TTR("Base variation"));
		vars_item->set_editable(0, false);
		vars_item->set_metadata(0, import_variation_data);
	}

	import_settings_data->defaults.clear();
	for (List<ResourceImporter::ImportOption>::Element *E = options_general.front(); E; E = E->next()) {
		import_settings_data->defaults[E->get().option.name] = E->get().default_value;
	}

	Ref<ConfigFile> config;
	config.instance();
	ERR_FAIL_NULL(config);

	Error err = config->load(p_path + ".import");
	print_verbose("Loading import settings:");
	if (err == OK) {
		List<String> keys;
		config->get_section_keys("params", &keys);
		for (List<String>::Element *E = keys.front(); E; E = E->next()) {
			String key = E->get();
			print_verbose(String("    ") + key + " == " + String(config->get_value("params", key)));
			if (key == "base_variation") {
				String var_str = config->get_value("params", key);
				Vector<String> variation_tags = var_str.split(",");
				for (int j = 0; j < variation_tags.size(); j++) {
					Vector<String> tokens = variation_tags[j].split("=");
					if (tokens[0] == "size" || tokens[0] == "outline_size") {
						import_variation_data->set(tokens[0], tokens[1].to_int());
					} else if (tokens[0] != "name") {
						import_variation_data->set(tokens[0], tokens[1].to_float());
					}
				}
			} else if (key == "preload/ranges") {
				Vector<String> ranges = config->get_value("params", key);
				for (int i = 0; i < ranges.size(); i++) {
					uint32_t start, end;
					Vector<String> tokens = ranges[i].split("-");
					if (tokens.size() == 2) {
						if (!ResourceImporterFontData::_decode_range(tokens[0], &start) || !ResourceImporterFontData::_decode_range(tokens[1], &end)) {
							WARN_PRINT("Invalid range: \"" + ranges[i] + "\"");
							continue;
						}
					} else if (tokens.size() == 1) {
						if (!ResourceImporterFontData::_decode_range(tokens[0], &start)) {
							WARN_PRINT("Invalid range: \"" + ranges[i] + "\"");
							continue;
						}
						end = start;
					} else {
						WARN_PRINT("Invalid range: \"" + ranges[i] + "\"");
						continue;
					}
					for (uint32_t j = start; j <= end; j++) {
						selected_chars.insert(j);
					}
				}
			} else if (key == "preload/variations") {
				Vector<String> variations = config->get_value("params", key);
				for (int i = 0; i < variations.size(); i++) {
					TreeItem *vars_item = vars_list->create_item(vars_list_root);
					ERR_FAIL_NULL(vars_item);

					vars_item->set_text(0, TTR("New variation"));
					vars_item->set_editable(0, true);
					vars_item->add_button(1, vars_list->get_theme_icon("Remove", "EditorIcons"), BUTTON_REMOVE_VAR, false, TTR("Remove Variation"));
					vars_item->set_button_color(1, 0, Color(1, 1, 1, 0.75));

					Ref<FontDataImportSettingsData> import_variation_data_custom;
					import_variation_data_custom.instance();
					ERR_FAIL_NULL(import_variation_data_custom);

					for (List<ResourceImporter::ImportOption>::Element *F = options_variations.front(); F; F = F->next()) {
						import_variation_data_custom->defaults[F->get().option.name] = F->get().default_value;
					}

					import_variation_data_custom->options = options_variations;

					vars_item->set_metadata(0, import_variation_data_custom);
					Vector<String> variation_tags = variations[i].split(",");
					for (int j = 0; j < variation_tags.size(); j++) {
						Vector<String> tokens = variation_tags[j].split("=");
						if (tokens[0] == "name") {
							vars_item->set_text(0, tokens[1]);
						} else if (tokens[0] == "size" || tokens[0] == "outline_size") {
							import_variation_data_custom->set(tokens[0], tokens[1].to_int());
						} else {
							import_variation_data_custom->set(tokens[0], tokens[1].to_float());
						}
					}
				}
			} else if (key == "support_overrides/language_enabled") {
				PackedStringArray _langs = config->get_value("params", key);
				for (int i = 0; i < _langs.size(); i++) {
					TreeItem *lang_item = lang_list->create_item(lang_list_root);
					ERR_FAIL_NULL(lang_item);

					lang_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
					lang_item->set_editable(0, true);
					lang_item->set_checked(0, true);
					lang_item->set_text(1, _langs[i]);
					lang_item->set_editable(1, true);
				}
			} else if (key == "support_overrides/language_disabled") {
				PackedStringArray _langs = config->get_value("params", key);
				for (int i = 0; i < _langs.size(); i++) {
					TreeItem *lang_item = lang_list->create_item(lang_list_root);
					ERR_FAIL_NULL(lang_item);

					lang_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
					lang_item->set_editable(0, true);
					lang_item->set_checked(0, false);
					lang_item->set_text(1, _langs[i]);
					lang_item->set_editable(1, true);
				}
			} else if (key == "support_overrides/script_enabled") {
				PackedStringArray _scripts = config->get_value("params", key);
				for (int i = 0; i < _scripts.size(); i++) {
					TreeItem *script_item = script_list->create_item(script_list_root);
					ERR_FAIL_NULL(script_item);

					script_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
					script_item->set_editable(0, true);
					script_item->set_checked(0, true);
					script_item->set_text(1, _scripts[i]);
					script_item->set_editable(1, true);
				}
			} else if (key == "support_overrides/script_disabled") {
				PackedStringArray _scripts = config->get_value("params", key);
				for (int i = 0; i < _scripts.size(); i++) {
					TreeItem *script_item = script_list->create_item(script_list_root);
					ERR_FAIL_NULL(script_item);

					script_item->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
					script_item->set_editable(0, true);
					script_item->set_checked(0, false);
					script_item->set_text(1, _scripts[i]);
					script_item->set_editable(1, true);
				}
			} else {
				Variant value = config->get_value("params", key);
				import_settings_data->defaults[key] = value;
			}
		}
	}

	import_settings_data->options = options_general;
	inspector_general->edit(import_settings_data.ptr());
	import_settings_data->notify_property_list_changed();

	popup_centered_ratio();

	set_title(vformat(TTR("Advanced Import Settings for '%s'"), base_path.get_file()));
}

FontDataImportSettings *FontDataImportSettings::get_singleton() {
	return singleton;
}

FontDataImportSettings::FontDataImportSettings() {
	singleton = this;

	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "antialiased"), true));
	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "force_autohinter"), false));
	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "msdf", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), true));
	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::FLOAT, "oversampling", PROPERTY_HINT_RANGE, "0,100,0.1"), 0.0));
	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::FLOAT, "msdf_px_range", PROPERTY_HINT_RANGE, "1,100,0.1"), 8.0));
	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "hinting", PROPERTY_HINT_ENUM, "None,Light,Normal"), 1));

	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::BOOL, "convert_to_bitmap", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), false));

	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "extra_spacing_glyph"), 0));
	options_general.push_back(ResourceImporter::ImportOption(PropertyInfo(Variant::INT, "extra_spacing_space"), 0));

	menu_langs = memnew(PopupMenu);
	menu_langs->set_name("Lang");
	for (int i = 0; langs[i].name != String(); i++) {
		if (langs[i].name == "-") {
			menu_langs->add_separator();
		} else {
			menu_langs->add_item(langs[i].name + " (" + langs[i].code + ")", i);
		}
	}
	add_child(menu_langs);
	menu_langs->connect("id_pressed", callable_mp(this, &FontDataImportSettings::_lang_add_item));

	menu_scripts = memnew(PopupMenu);
	menu_scripts->set_name("Script");
	for (int i = 0; scripts[i].name != String(); i++) {
		if (scripts[i].name == "-") {
			menu_scripts->add_separator();
		} else {
			menu_scripts->add_item(scripts[i].name + " (" + scripts[i].code + ")", i);
		}
	}
	add_child(menu_scripts);
	menu_scripts->connect("id_pressed", callable_mp(this, &FontDataImportSettings::_script_add_item));

	HSplitContainer *main_hb = memnew(HSplitContainer);
	main_hb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(main_hb);

	VBoxContainer *side_vb = memnew(VBoxContainer);
	main_hb->add_child(side_vb);

	label_general = memnew(Label);
	label_general->set_align(Label::ALIGN_CENTER);
	label_general->set_text(TTR("General"));
	side_vb->add_child(label_general);

	inspector_general = memnew(EditorInspector);
	inspector_general->set_custom_minimum_size(Size2(300 * EDSCALE, 350 * EDSCALE));
	inspector_general->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	side_vb->add_child(inspector_general);

	HBoxContainer *hb_vars = memnew(HBoxContainer);
	side_vb->add_child(hb_vars);

	label_vars = memnew(Label);
	hb_vars->add_child(label_vars);
	label_vars->set_align(Label::ALIGN_CENTER);
	label_vars->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	label_vars->set_text(TTR("Variations and Sizes"));

	add_var = memnew(Button);
	hb_vars->add_child(add_var);
	add_var->set_tooltip(TTR("Add variation"));
	add_var->set_icon(add_var->get_theme_icon("Add", "EditorIcons"));
	add_var->connect("pressed", callable_mp(this, &FontDataImportSettings::_variation_add));

	vars_list = memnew(Tree);
	side_vb->add_child(vars_list);
	vars_list->set_hide_root(true);
	vars_list->set_columns(2);
	vars_list->set_column_expand(0, true);
	vars_list->set_column_min_width(0, 80 * EDSCALE);
	vars_list->set_column_expand(1, false);
	vars_list->set_column_min_width(1, 50 * EDSCALE);
	vars_list->connect("item_selected", callable_mp(this, &FontDataImportSettings::_variation_selected));
	vars_list->connect("button_pressed", callable_mp(this, &FontDataImportSettings::_variation_remove));
	vars_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	inspector_vars = memnew(EditorInspector);
	inspector_vars->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	inspector_vars->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	side_vb->add_child(inspector_vars);

	HBoxContainer *hb_lang = memnew(HBoxContainer);
	side_vb->add_child(hb_lang);

	label_langs = memnew(Label);
	label_langs->set_align(Label::ALIGN_CENTER);
	label_langs->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	label_langs->set_text(TTR("Supported Languages"));
	hb_lang->add_child(label_langs);

	add_lang = memnew(Button);
	hb_lang->add_child(add_lang);
	add_lang->set_tooltip(TTR("Add language override"));
	add_lang->set_icon(add_var->get_theme_icon("Add", "EditorIcons"));
	add_lang->connect("pressed", callable_mp(this, &FontDataImportSettings::_lang_add));

	lang_list = memnew(Tree);
	side_vb->add_child(lang_list);
	lang_list->set_hide_root(true);
	lang_list->set_columns(3);
	lang_list->set_column_expand(0, false); // Check
	lang_list->set_column_min_width(0, 50 * EDSCALE);
	lang_list->set_column_expand(1, true);
	lang_list->set_column_min_width(1, 80 * EDSCALE);
	lang_list->set_column_expand(2, false);
	lang_list->set_column_min_width(2, 50 * EDSCALE);
	lang_list->connect("button_pressed", callable_mp(this, &FontDataImportSettings::_lang_remove));
	lang_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	HBoxContainer *hb_script = memnew(HBoxContainer);
	side_vb->add_child(hb_script);

	label_script = memnew(Label);
	label_script->set_align(Label::ALIGN_CENTER);
	label_script->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	label_script->set_text(TTR("Supported Scripts"));
	hb_script->add_child(label_script);

	add_script = memnew(Button);
	hb_script->add_child(add_script);
	add_script->set_tooltip(TTR("Add script override"));
	add_script->set_icon(add_var->get_theme_icon("Add", "EditorIcons"));
	add_script->connect("pressed", callable_mp(this, &FontDataImportSettings::_script_add));

	script_list = memnew(Tree);
	side_vb->add_child(script_list);
	script_list->set_hide_root(true);
	script_list->set_columns(3);
	script_list->set_column_expand(0, false); // Check
	script_list->set_column_min_width(0, 50 * EDSCALE);
	script_list->set_column_expand(1, true);
	script_list->set_column_min_width(1, 80 * EDSCALE);
	script_list->set_column_expand(2, false);
	script_list->set_column_min_width(2, 50 * EDSCALE);
	script_list->connect("button_pressed", callable_mp(this, &FontDataImportSettings::_script_remove));
	script_list->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	VBoxContainer *main_vb = memnew(VBoxContainer);
	main_hb->add_child(main_vb);
	main_vb->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	main_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	label_glyph = memnew(Label);
	label_glyph->set_align(Label::ALIGN_CENTER);
	label_glyph->set_text(TTR("Preloaded Glyphs"));
	main_vb->add_child(label_glyph);

	HSplitContainer *glyphs_split = memnew(HSplitContainer);
	glyphs_split->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	glyphs_split->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	main_vb->add_child(glyphs_split);

	glyph_table = memnew(Tree);
	glyphs_split->add_child(glyph_table);
	glyph_table->set_custom_minimum_size(Size2((30 * 16 + 100) * EDSCALE, 0));
	glyph_table->set_columns(17);
	glyph_table->set_column_expand(0, false);
	glyph_table->set_hide_root(true);
	glyph_table->set_allow_reselect(true);
	glyph_table->set_select_mode(Tree::SELECT_SINGLE);
	glyph_table->connect("item_activated", callable_mp(this, &FontDataImportSettings::_glyph_selected));
	glyph_table->set_column_titles_visible(true);
	for (int i = 0; i < 16; i++) {
		glyph_table->set_column_title(i + 1, String::num_int64(i, 16));
	}
	glyph_table->add_theme_style_override("selected", glyph_table->get_theme_stylebox("bg"));
	glyph_table->add_theme_style_override("selected_focus", glyph_table->get_theme_stylebox("bg"));
	glyph_table->add_theme_constant_override("hseparation", 0);
	glyph_table->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	glyph_table->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	glyph_tree = memnew(Tree);
	glyphs_split->add_child(glyph_tree);
	glyph_tree->set_custom_minimum_size(Size2(300 * EDSCALE, 0));
	glyph_tree->set_columns(3);
	glyph_tree->set_hide_root(true);
	glyph_tree->set_column_expand(0, false);
	glyph_tree->set_column_expand(1, false);
	glyph_tree->set_column_expand(2, true);
	glyph_tree->set_column_min_width(0, 50 * EDSCALE);
	glyph_tree->set_column_min_width(1, 120 * EDSCALE);
	glyph_tree->connect("item_edited", callable_mp(this, &FontDataImportSettings::_range_edited));
	glyph_tree->connect("item_selected", callable_mp(this, &FontDataImportSettings::_range_selected));
	glyph_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	glyph_root = glyph_tree->create_item();
	for (int i = 0; unicode_ranges[i].name != String(); i++) {
		_add_glyph_range_item(unicode_ranges[i].start, unicode_ranges[i].end, unicode_ranges[i].name);
	}

	import_settings_data.instance();

	get_ok_button()->set_text(TTR("Reimport"));
	get_cancel_button()->set_text(TTR("Close"));
}
