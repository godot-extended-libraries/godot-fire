/*************************************************************************/
/*  popup_menu.cpp                                                       */
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

#include "popup_menu.h"

#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/print_string.h"
#include "core/translation.h"

String PopupMenu::_get_accel_text(int p_item) const {
	ERR_FAIL_INDEX_V(p_item, items.size(), String());

	if (items[p_item].shortcut.is_valid()) {
		return items[p_item].shortcut->get_as_text();
	} else if (items[p_item].accel) {
		return keycode_get_string(items[p_item].accel);
	}
	return String();
}

Size2 PopupMenu::_get_contents_minimum_size() const {
	int vseparation = get_theme_constant("vseparation");
	int hseparation = get_theme_constant("hseparation");

	Size2 minsize = get_theme_stylebox("panel")->get_minimum_size(); // Accounts for margin in the margin container
	minsize.x += scroll_container->get_v_scrollbar()->get_size().width * 2; // Adds a buffer so that the scrollbar does not render over the top of content
	Ref<Font> font = get_theme_font("font");

	float max_w = 0;
	float icon_w = 0;
	int font_h = font->get_height();
	int check_w = MAX(get_theme_icon("checked")->get_width(), get_theme_icon("radio_checked")->get_width()) + hseparation;
	int accel_max_w = 0;
	bool has_check = false;

	for (int i = 0; i < items.size(); i++) {
		Size2 size;

		Size2 icon_size = items[i].get_icon_size();
		size.height = MAX(icon_size.height, font_h);
		icon_w = MAX(icon_size.width, icon_w);

		size.width += items[i].h_ofs;

		if (items[i].checkable_type) {
			has_check = true;
		}

		String text = items[i].xl_text;
		size.width += font->get_string_size(text).width;
		if (i > 0) {
			size.height += vseparation;
		}

		if (items[i].accel || (items[i].shortcut.is_valid() && items[i].shortcut->is_valid())) {
			int accel_w = hseparation * 2;
			accel_w += font->get_string_size(_get_accel_text(i)).width;
			accel_max_w = MAX(accel_w, accel_max_w);
		}

		if (items[i].submenu != "") {
			size.width += get_theme_icon("submenu")->get_width();
		}

		max_w = MAX(max_w, size.width);

		minsize.height += size.height;
	}

	minsize.width += max_w + icon_w + accel_max_w;
	if (has_check) {
		minsize.width += check_w;
	}

	if (is_inside_tree()) {
		int height_limit = get_usable_parent_rect().size.height;
		if (minsize.height > height_limit) {
			minsize.height = height_limit;
		}
	}

	return minsize;
}

int PopupMenu::_get_items_total_height() const {
	int font_height = get_theme_font("font")->get_height();
	int vsep = get_theme_constant("vseparation");

	// Get total height of all items by taking max of icon height and font height
	int items_total_height = 0;
	for (int i = 0; i < items.size(); i++) {
		items_total_height += MAX(items[i].get_icon_size().height, font_height) + vsep;
	}

	// Subtract a separator which is not needed for the last item.
	return items_total_height - vsep;
}

void PopupMenu::_scroll_to_item(int p_item) {
	ERR_FAIL_INDEX(p_item, items.size());
	ERR_FAIL_COND(p_item < 0);

	// Scroll item into view (upwards)
	if (items[p_item]._ofs_cache < -control->get_position().y) {
		int amnt_over = items[p_item]._ofs_cache + control->get_position().y;
		scroll_container->set_v_scroll(scroll_container->get_v_scroll() + amnt_over);
	}

	// Scroll item into view (downwards)
	if (items[p_item]._ofs_cache + items[p_item]._height_cache > -control->get_position().y + scroll_container->get_size().height) {
		int amnt_over = items[p_item]._ofs_cache + items[p_item]._height_cache + control->get_position().y - scroll_container->get_size().height;
		scroll_container->set_v_scroll(scroll_container->get_v_scroll() + amnt_over);
	}
}

int PopupMenu::_get_mouse_over(const Point2 &p_over) const {
	if (p_over.x < 0 || p_over.x >= get_size().width) {
		return -1;
	}

	Ref<StyleBox> style = get_theme_stylebox("panel"); // Accounts for margin in the margin container

	int vseparation = get_theme_constant("vseparation");
	float font_h = get_theme_font("font")->get_height();

	Point2 ofs = style->get_offset() + Point2(0, vseparation / 2);

	if (ofs.y > p_over.y) {
		return -1;
	}

	for (int i = 0; i < items.size(); i++) {
		if (i > 0) {
			ofs.y += vseparation;
		}

		ofs.y += MAX(items[i].get_icon_size().height, font_h);

		if (p_over.y - control->get_position().y < ofs.y) {
			return i;
		}
	}

	return -1;
}

void PopupMenu::_activate_submenu(int over) {
	Node *n = get_node(items[over].submenu);
	ERR_FAIL_COND_MSG(!n, "Item subnode does not exist: " + items[over].submenu + ".");
	Popup *submenu_popup = Object::cast_to<Popup>(n);
	ERR_FAIL_COND_MSG(!submenu_popup, "Item subnode is not a Popup: " + items[over].submenu + ".");
	if (submenu_popup->is_visible()) {
		return; //already visible!
	}

	Ref<StyleBox> style = get_theme_stylebox("panel");
	int vsep = get_theme_constant("vseparation");

	Point2 this_pos = get_position();
	Rect2 this_rect(this_pos, get_size());

	float scroll_offset = control->get_position().y;

	Point2 submenu_pos = this_pos + Point2(this_rect.size.width, items[over]._ofs_cache + scroll_offset);
	Size2 submenu_size = submenu_popup->get_size();

	// Fix pos if going outside parent rect
	if (submenu_pos.x + submenu_size.width > get_parent_rect().size.width) {
		submenu_pos.x = this_pos.x - submenu_size.width;
	}

	submenu_popup->set_position(submenu_pos);
	submenu_popup->set_as_minsize(); // Shrink the popup size to it's contents.
	submenu_popup->popup();

	// Set autohide areas
	PopupMenu *submenu_pum = Object::cast_to<PopupMenu>(submenu_popup);
	if (submenu_pum) {
		// Make the position of the parent popup relative to submenu popup
		this_rect.position = this_rect.position - submenu_pum->get_position();

		// Autohide area above the submenu item
		submenu_pum->clear_autohide_areas();
		submenu_pum->add_autohide_area(Rect2(this_rect.position.x, this_rect.position.y, this_rect.size.x, items[over]._ofs_cache + scroll_offset + style->get_offset().height - vsep / 2));

		// If there is an area below the submenu item, add an autohide area there.
		if (items[over]._ofs_cache + items[over]._height_cache + scroll_offset <= control->get_size().height) {
			int from = items[over]._ofs_cache + items[over]._height_cache + scroll_offset + vsep / 2 + style->get_offset().height;
			submenu_pum->add_autohide_area(Rect2(this_rect.position.x, this_rect.position.y + from, this_rect.size.x, this_rect.size.y - from));
		}
	}
}

void PopupMenu::_submenu_timeout() {
	if (mouse_over == submenu_over) {
		_activate_submenu(mouse_over);
	}

	submenu_over = -1;
}

void PopupMenu::_gui_input(const Ref<InputEvent> &p_event) {
	if (p_event->is_action("ui_down") && p_event->is_pressed() && mouse_over != items.size() - 1) {
		int search_from = mouse_over + 1;
		if (search_from >= items.size()) {
			search_from = 0;
		}

		for (int i = search_from; i < items.size(); i++) {
			if (!items[i].separator && !items[i].disabled) {
				mouse_over = i;
				emit_signal("id_focused", i);
				_scroll_to_item(i);
				control->update();
				set_input_as_handled();
				break;
			}
		}
	} else if (p_event->is_action("ui_up") && p_event->is_pressed() && mouse_over != 0) {
		int search_from = mouse_over - 1;
		if (search_from < 0) {
			search_from = items.size() - 1;
		}

		for (int i = search_from; i >= 0; i--) {
			if (!items[i].separator && !items[i].disabled) {
				mouse_over = i;
				emit_signal("id_focused", i);
				_scroll_to_item(i);
				control->update();
				set_input_as_handled();
				break;
			}
		}
	} else if (p_event->is_action("ui_left") && p_event->is_pressed()) {
		Node *n = get_parent();
		if (n && Object::cast_to<PopupMenu>(n)) {
			hide();
			set_input_as_handled();
		}
	} else if (p_event->is_action("ui_right") && p_event->is_pressed()) {
		if (mouse_over >= 0 && mouse_over < items.size() && !items[mouse_over].separator && items[mouse_over].submenu != "" && submenu_over != mouse_over) {
			_activate_submenu(mouse_over);
			set_input_as_handled();
		}
	} else if (p_event->is_action("ui_accept") && p_event->is_pressed()) {
		if (mouse_over >= 0 && mouse_over < items.size() && !items[mouse_over].separator) {
			if (items[mouse_over].submenu != "" && submenu_over != mouse_over) {
				_activate_submenu(mouse_over);
			} else {
				activate_item(mouse_over);
			}
			set_input_as_handled();
		}
	}

	// Make an area which does not include v scrollbar, so that items are not activated when dragging scrollbar.
	Rect2 item_clickable_area = scroll_container->get_rect();
	if (scroll_container->get_v_scrollbar()->is_visible_in_tree()) {
		item_clickable_area.size.width -= scroll_container->get_v_scrollbar()->get_size().width;
	}

	Ref<InputEventMouseButton> b = p_event;

	if (b.is_valid()) {
		if (!item_clickable_area.has_point(b->get_position())) {
			return;
		}

		int button_idx = b->get_button_index();
		if (b->is_pressed() || (!b->is_pressed() && during_grabbed_click)) {
			// Allow activating item by releasing the LMB or any that was down when the popup appeared.
			// However, if button was not held when opening menu, do not allow release to activate item.
			if (button_idx == BUTTON_LEFT || (initial_button_mask & (1 << (button_idx - 1)))) {
				bool was_during_grabbed_click = during_grabbed_click;
				during_grabbed_click = false;
				initial_button_mask = 0;

				// Disable clicks under a time threshold to avoid selection right when opening the popup.
				uint64_t now = OS::get_singleton()->get_ticks_msec();
				uint64_t diff = now - popup_time_msec;
				if (diff < 100) {
					return;
				}

				int over = _get_mouse_over(b->get_position());
				if (over < 0) {
					if (!was_during_grabbed_click) {
						hide();
					}
					return;
				}

				if (items[over].separator || items[over].disabled) {
					return;
				}

				if (items[over].submenu != "") {
					_activate_submenu(over);
					return;
				}
				activate_item(over);
			}
		}
	}

	Ref<InputEventMouseMotion> m = p_event;

	if (m.is_valid()) {
		if (!item_clickable_area.has_point(m->get_position())) {
			return;
		}

		for (List<Rect2>::Element *E = autohide_areas.front(); E; E = E->next()) {
			if (!Rect2(Point2(), get_size()).has_point(m->get_position()) && E->get().has_point(m->get_position())) {
				_close_pressed();
				return;
			}
		}

		int over = _get_mouse_over(m->get_position());
		int id = (over < 0 || items[over].separator || items[over].disabled) ? -1 : (items[over].id >= 0 ? items[over].id : over);

		if (id < 0) {
			mouse_over = -1;
			control->update();
			return;
		}

		if (items[over].submenu != "" && submenu_over != over) {
			submenu_over = over;
			submenu_timer->start();
		}

		if (over != mouse_over) {
			mouse_over = over;
			control->update();
		}
	}

	Ref<InputEventKey> k = p_event;

	if (allow_search && k.is_valid() && k->get_unicode() && k->is_pressed()) {
		uint64_t now = OS::get_singleton()->get_ticks_msec();
		uint64_t diff = now - search_time_msec;
		uint64_t max_interval = uint64_t(GLOBAL_DEF("gui/timers/incremental_search_max_interval_msec", 2000));
		search_time_msec = now;

		if (diff > max_interval) {
			search_string = "";
		}

		if (String::chr(k->get_unicode()) != search_string) {
			search_string += String::chr(k->get_unicode());
		}

		for (int i = mouse_over + 1; i <= items.size(); i++) {
			if (i == items.size()) {
				if (mouse_over <= 0) {
					break;
				} else {
					i = 0;
				}
			}

			if (i == mouse_over) {
				break;
			}

			if (items[i].text.findn(search_string) == 0) {
				mouse_over = i;
				emit_signal("id_focused", i);
				_scroll_to_item(i);
				control->update();
				set_input_as_handled();
				break;
			}
		}
	}
}

void PopupMenu::_draw_items() {
	control->set_custom_minimum_size(Size2(0, _get_items_total_height()));
	RID ci = control->get_canvas_item();

	Size2 margin_size;
	margin_size.width = margin_container->get_theme_constant("margin_right") + margin_container->get_theme_constant("margin_left");
	margin_size.height = margin_container->get_theme_constant("margin_top") + margin_container->get_theme_constant("margin_bottom");

	Ref<StyleBox> style = get_theme_stylebox("panel");
	Ref<StyleBox> hover = get_theme_stylebox("hover");
	Ref<Font> font = get_theme_font("font");
	// In Item::checkable_type enum order (less the non-checkable member)
	Ref<Texture2D> check[] = { get_theme_icon("checked"), get_theme_icon("radio_checked") };
	Ref<Texture2D> uncheck[] = { get_theme_icon("unchecked"), get_theme_icon("radio_unchecked") };
	Ref<Texture2D> submenu = get_theme_icon("submenu");
	Ref<StyleBox> separator = get_theme_stylebox("separator");
	Ref<StyleBox> labeled_separator_left = get_theme_stylebox("labeled_separator_left");
	Ref<StyleBox> labeled_separator_right = get_theme_stylebox("labeled_separator_right");

	int vseparation = get_theme_constant("vseparation");
	int hseparation = get_theme_constant("hseparation");
	Color font_color = get_theme_color("font_color");
	Color font_color_disabled = get_theme_color("font_color_disabled");
	Color font_color_accel = get_theme_color("font_color_accel");
	Color font_color_hover = get_theme_color("font_color_hover");
	float font_h = font->get_height();

	float scroll_width = scroll_container->get_v_scrollbar()->is_visible_in_tree() ? scroll_container->get_v_scrollbar()->get_size().width : 0;
	float display_width = control->get_size().width - scroll_width;

	// Find the widest icon and whether any items have a checkbox, and store the offsets for each.
	float icon_ofs = 0.0;
	bool has_check = false;
	for (int i = 0; i < items.size(); i++) {
		icon_ofs = MAX(items[i].get_icon_size().width, icon_ofs);

		if (items[i].checkable_type) {
			has_check = true;
		}
	}
	if (icon_ofs > 0.0) {
		icon_ofs += hseparation;
	}

	float check_ofs = 0.0;
	if (has_check) {
		check_ofs = MAX(get_theme_icon("checked")->get_width(), get_theme_icon("radio_checked")->get_width()) + hseparation;
	}

	Point2 ofs = Point2();

	// Loop through all items and draw each.
	for (int i = 0; i < items.size(); i++) {
		// If not the first item, add the separation space between items.
		if (i > 0) {
			ofs.y += vseparation;
		}

		Point2 item_ofs = ofs;
		Size2 icon_size = items[i].get_icon_size();
		float h = MAX(icon_size.height, font_h);

		if (i == mouse_over) {
			hover->draw(ci, Rect2(item_ofs + Point2(-hseparation, -vseparation / 2), Size2(display_width + hseparation * 2, h + vseparation)));
		}

		String text = items[i].xl_text;

		// Separator
		item_ofs.x += items[i].h_ofs;
		if (items[i].separator) {
			int sep_h = separator->get_center_size().height + separator->get_minimum_size().height;
			if (text != String()) {
				int text_size = font->get_string_size(text).width;
				int text_center = display_width / 2;
				int text_left = text_center - text_size / 2;
				int text_right = text_center + text_size / 2;
				if (text_left > item_ofs.x) {
					labeled_separator_left->draw(ci, Rect2(item_ofs + Point2(0, Math::floor((h - sep_h) / 2.0)), Size2(MAX(0, text_left - item_ofs.x), sep_h)));
				}
				if (text_right < display_width) {
					labeled_separator_right->draw(ci, Rect2(Point2(text_right, item_ofs.y + Math::floor((h - sep_h) / 2.0)), Size2(MAX(0, display_width - text_right), sep_h)));
				}
			} else {
				separator->draw(ci, Rect2(item_ofs + Point2(0, Math::floor((h - sep_h) / 2.0)), Size2(display_width, sep_h)));
			}
		}

		Color icon_color(1, 1, 1, items[i].disabled ? 0.5 : 1);

		// Checkboxes
		if (items[i].checkable_type) {
			Texture2D *icon = (items[i].checked ? check[items[i].checkable_type - 1] : uncheck[items[i].checkable_type - 1]).ptr();
			icon->draw(ci, item_ofs + Point2(0, Math::floor((h - icon->get_height()) / 2.0)), icon_color);
		}

		// Icon
		if (!items[i].icon.is_null()) {
			items[i].icon->draw(ci, item_ofs + Size2(check_ofs, 0) + Point2(0, Math::floor((h - icon_size.height) / 2.0)), icon_color);
		}

		// Submenu arrow on right hand side
		if (items[i].submenu != "") {
			submenu->draw(ci, Point2(display_width - submenu->get_width(), item_ofs.y + Math::floor(h - submenu->get_height()) / 2), icon_color);
		}

		// Text
		item_ofs.y += font->get_ascent();
		if (items[i].separator) {
			if (text != String()) {
				int center = (display_width - font->get_string_size(text).width) / 2;
				font->draw(ci, Point2(center, item_ofs.y + Math::floor((h - font_h) / 2.0)), text, font_color_disabled);
			}
		} else {
			item_ofs.x += icon_ofs + check_ofs;
			font->draw(ci, item_ofs + Point2(0, Math::floor((h - font_h) / 2.0)), text, items[i].disabled ? font_color_disabled : (i == mouse_over ? font_color_hover : font_color));
		}

		// Accelerator / Shortcut
		if (items[i].accel || (items[i].shortcut.is_valid() && items[i].shortcut->is_valid())) {
			String sc_text = _get_accel_text(i);
			item_ofs.x = display_width - font->get_string_size(sc_text).width;
			font->draw(ci, item_ofs + Point2(0, Math::floor((h - font_h) / 2.0)), sc_text, i == mouse_over ? font_color_hover : font_color_accel);
		}

		// Cache the item vertical offset from the first item and the height
		items.write[i]._ofs_cache = ofs.y;
		items.write[i]._height_cache = h;

		ofs.y += h;
	}
}

void PopupMenu::_draw_background() {
	Ref<StyleBox> style = get_theme_stylebox("panel");
	RID ci2 = margin_container->get_canvas_item();
	style->draw(ci2, Rect2(Point2(), margin_container->get_size()));
}

void PopupMenu::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			PopupMenu *pm = Object::cast_to<PopupMenu>(get_parent());
			if (pm) {
				// Inherit submenu's popup delay time from parent menu
				float pm_delay = pm->get_submenu_popup_delay();
				set_submenu_popup_delay(pm_delay);
			}
		} break;
		case NOTIFICATION_TRANSLATION_CHANGED: {
			for (int i = 0; i < items.size(); i++) {
				items.write[i].xl_text = tr(items[i].text);
			}

			child_controls_changed();
			control->update();
		} break;
		case NOTIFICATION_WM_MOUSE_ENTER: {
			//grab_focus();
		} break;
		case NOTIFICATION_WM_MOUSE_EXIT: {
			if (mouse_over >= 0 && (items[mouse_over].submenu == "" || submenu_over != -1)) {
				mouse_over = -1;
				control->update();
			}
		} break;
		case NOTIFICATION_POST_POPUP: {
			initial_button_mask = Input::get_singleton()->get_mouse_button_mask();
			during_grabbed_click = (bool)initial_button_mask;
		} break;
		case NOTIFICATION_WM_SIZE_CHANGED: {
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			//only used when using operating system windows
			if (get_window_id() != DisplayServer::INVALID_WINDOW_ID && autohide_areas.size()) {
				Point2 mouse_pos = DisplayServer::get_singleton()->mouse_get_position();
				mouse_pos -= get_position();

				for (List<Rect2>::Element *E = autohide_areas.front(); E; E = E->next()) {
					if (!Rect2(Point2(), get_size()).has_point(mouse_pos) && E->get().has_point(mouse_pos)) {
						_close_pressed();
						return;
					}
				}
			}
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (!is_visible()) {
				if (mouse_over >= 0) {
					mouse_over = -1;
					control->update();
				}

				for (int i = 0; i < items.size(); i++) {
					if (items[i].submenu == "") {
						continue;
					}

					Node *n = get_node(items[i].submenu);
					if (!n) {
						continue;
					}

					PopupMenu *pm = Object::cast_to<PopupMenu>(n);
					if (!pm || !pm->is_visible()) {
						continue;
					}

					pm->hide();
				}

				set_process_internal(false);
			} else {
				if (get_window_id() != DisplayServer::INVALID_WINDOW_ID) {
					set_process_internal(true);
				}

				// Set margin on the margin container
				Ref<StyleBox> panel_style = get_theme_stylebox("panel");
				margin_container->add_theme_constant_override("margin_top", panel_style->get_margin(Margin::MARGIN_TOP));
				margin_container->add_theme_constant_override("margin_bottom", panel_style->get_margin(Margin::MARGIN_BOTTOM));
				margin_container->add_theme_constant_override("margin_left", panel_style->get_margin(Margin::MARGIN_LEFT));
				margin_container->add_theme_constant_override("margin_right", panel_style->get_margin(Margin::MARGIN_RIGHT));
			}
		} break;
	}
}

/* Methods to add items with or without icon, checkbox, shortcut.
 * Be sure to keep them in sync when adding new properties in the Item struct.
 */

#define ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel) \
	item.text = p_label;                              \
	item.xl_text = tr(p_label);                       \
	item.id = p_id == -1 ? items.size() : p_id;       \
	item.accel = p_accel;

void PopupMenu::add_item(const String &p_label, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_icon_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	item.icon = p_icon;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_check_item(const String &p_label, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	item.checkable_type = Item::CHECKABLE_TYPE_CHECK_BOX;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_icon_check_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	item.icon = p_icon;
	item.checkable_type = Item::CHECKABLE_TYPE_CHECK_BOX;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_radio_check_item(const String &p_label, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	item.checkable_type = Item::CHECKABLE_TYPE_RADIO_BUTTON;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_icon_radio_check_item(const Ref<Texture2D> &p_icon, const String &p_label, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	item.icon = p_icon;
	item.checkable_type = Item::CHECKABLE_TYPE_RADIO_BUTTON;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_multistate_item(const String &p_label, int p_max_states, int p_default_state, int p_id, uint32_t p_accel) {
	Item item;
	ITEM_SETUP_WITH_ACCEL(p_label, p_id, p_accel);
	item.max_states = p_max_states;
	item.state = p_default_state;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

#define ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global)                           \
	ERR_FAIL_COND_MSG(p_shortcut.is_null(), "Cannot add item with invalid Shortcut."); \
	_ref_shortcut(p_shortcut);                                                         \
	item.text = p_shortcut->get_name();                                                \
	item.xl_text = tr(item.text);                                                      \
	item.id = p_id == -1 ? items.size() : p_id;                                        \
	item.shortcut = p_shortcut;                                                        \
	item.shortcut_is_global = p_global;

void PopupMenu::add_shortcut(const Ref<Shortcut> &p_shortcut, int p_id, bool p_global) {
	Item item;
	ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global);
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_icon_shortcut(const Ref<Texture2D> &p_icon, const Ref<Shortcut> &p_shortcut, int p_id, bool p_global) {
	Item item;
	ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global);
	item.icon = p_icon;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_check_shortcut(const Ref<Shortcut> &p_shortcut, int p_id, bool p_global) {
	Item item;
	ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global);
	item.checkable_type = Item::CHECKABLE_TYPE_CHECK_BOX;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_icon_check_shortcut(const Ref<Texture2D> &p_icon, const Ref<Shortcut> &p_shortcut, int p_id, bool p_global) {
	Item item;
	ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global);
	item.icon = p_icon;
	item.checkable_type = Item::CHECKABLE_TYPE_CHECK_BOX;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_radio_check_shortcut(const Ref<Shortcut> &p_shortcut, int p_id, bool p_global) {
	Item item;
	ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global);
	item.checkable_type = Item::CHECKABLE_TYPE_RADIO_BUTTON;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_icon_radio_check_shortcut(const Ref<Texture2D> &p_icon, const Ref<Shortcut> &p_shortcut, int p_id, bool p_global) {
	Item item;
	ITEM_SETUP_WITH_SHORTCUT(p_shortcut, p_id, p_global);
	item.icon = p_icon;
	item.checkable_type = Item::CHECKABLE_TYPE_RADIO_BUTTON;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_submenu_item(const String &p_label, const String &p_submenu, int p_id) {
	Item item;
	item.text = p_label;
	item.xl_text = tr(p_label);
	item.id = p_id == -1 ? items.size() : p_id;
	item.submenu = p_submenu;
	items.push_back(item);
	control->update();
	child_controls_changed();
}

#undef ITEM_SETUP_WITH_ACCEL
#undef ITEM_SETUP_WITH_SHORTCUT

/* Methods to modify existing items. */

void PopupMenu::set_item_text(int p_idx, const String &p_text) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].text = p_text;
	items.write[p_idx].xl_text = tr(p_text);

	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_icon(int p_idx, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].icon = p_icon;

	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_checked(int p_idx, bool p_checked) {
	ERR_FAIL_INDEX(p_idx, items.size());

	items.write[p_idx].checked = p_checked;

	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_id(int p_idx, int p_id) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].id = p_id;

	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_accelerator(int p_idx, uint32_t p_accel) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].accel = p_accel;

	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_metadata(int p_idx, const Variant &p_meta) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].metadata = p_meta;
	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_disabled(int p_idx, bool p_disabled) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].disabled = p_disabled;
	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_submenu(int p_idx, const String &p_submenu) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].submenu = p_submenu;
	control->update();
	child_controls_changed();
}

void PopupMenu::toggle_item_checked(int p_idx) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].checked = !items[p_idx].checked;
	control->update();
	child_controls_changed();
}

String PopupMenu::get_item_text(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].text;
}

int PopupMenu::get_item_idx_from_text(const String &text) const {
	for (int idx = 0; idx < items.size(); idx++) {
		if (items[idx].text == text) {
			return idx;
		}
	}

	return -1;
}

Ref<Texture2D> PopupMenu::get_item_icon(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Texture2D>());
	return items[p_idx].icon;
}

uint32_t PopupMenu::get_item_accelerator(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].accel;
}

Variant PopupMenu::get_item_metadata(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), Variant());
	return items[p_idx].metadata;
}

bool PopupMenu::is_item_disabled(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].disabled;
}

bool PopupMenu::is_item_checked(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checked;
}

int PopupMenu::get_item_id(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), 0);
	return items[p_idx].id;
}

int PopupMenu::get_item_index(int p_id) const {
	for (int i = 0; i < items.size(); i++) {
		if (items[i].id == p_id) {
			return i;
		}
	}

	return -1;
}

String PopupMenu::get_item_submenu(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].submenu;
}

String PopupMenu::get_item_tooltip(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), "");
	return items[p_idx].tooltip;
}

Ref<Shortcut> PopupMenu::get_item_shortcut(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), Ref<Shortcut>());
	return items[p_idx].shortcut;
}

int PopupMenu::get_item_state(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), -1);
	return items[p_idx].state;
}

void PopupMenu::set_item_as_separator(int p_idx, bool p_separator) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].separator = p_separator;
	control->update();
}

bool PopupMenu::is_item_separator(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].separator;
}

void PopupMenu::set_item_as_checkable(int p_idx, bool p_checkable) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].checkable_type = p_checkable ? Item::CHECKABLE_TYPE_CHECK_BOX : Item::CHECKABLE_TYPE_NONE;
	control->update();
}

void PopupMenu::set_item_as_radio_checkable(int p_idx, bool p_radio_checkable) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].checkable_type = p_radio_checkable ? Item::CHECKABLE_TYPE_RADIO_BUTTON : Item::CHECKABLE_TYPE_NONE;
	control->update();
}

void PopupMenu::set_item_tooltip(int p_idx, const String &p_tooltip) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].tooltip = p_tooltip;
	control->update();
}

void PopupMenu::set_item_shortcut(int p_idx, const Ref<Shortcut> &p_shortcut, bool p_global) {
	ERR_FAIL_INDEX(p_idx, items.size());
	if (items[p_idx].shortcut.is_valid()) {
		_unref_shortcut(items[p_idx].shortcut);
	}
	items.write[p_idx].shortcut = p_shortcut;
	items.write[p_idx].shortcut_is_global = p_global;

	if (items[p_idx].shortcut.is_valid()) {
		_ref_shortcut(items[p_idx].shortcut);
	}

	control->update();
}

void PopupMenu::set_item_h_offset(int p_idx, int p_offset) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].h_ofs = p_offset;
	control->update();
	child_controls_changed();
}

void PopupMenu::set_item_multistate(int p_idx, int p_state) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].state = p_state;
	control->update();
}

void PopupMenu::set_item_shortcut_disabled(int p_idx, bool p_disabled) {
	ERR_FAIL_INDEX(p_idx, items.size());
	items.write[p_idx].shortcut_is_disabled = p_disabled;
	control->update();
}

void PopupMenu::toggle_item_multistate(int p_idx) {
	ERR_FAIL_INDEX(p_idx, items.size());
	if (0 >= items[p_idx].max_states) {
		return;
	}

	++items.write[p_idx].state;
	if (items.write[p_idx].max_states <= items[p_idx].state) {
		items.write[p_idx].state = 0;
	}

	control->update();
}

bool PopupMenu::is_item_checkable(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checkable_type;
}

bool PopupMenu::is_item_radio_checkable(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].checkable_type == Item::CHECKABLE_TYPE_RADIO_BUTTON;
}

bool PopupMenu::is_item_shortcut_disabled(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, items.size(), false);
	return items[p_idx].shortcut_is_disabled;
}

int PopupMenu::get_current_index() const {
	return mouse_over;
}

int PopupMenu::get_item_count() const {
	return items.size();
}

bool PopupMenu::activate_item_by_event(const Ref<InputEvent> &p_event, bool p_for_global_only) {
	uint32_t code = 0;
	Ref<InputEventKey> k = p_event;

	if (k.is_valid()) {
		code = k->get_keycode();
		if (code == 0) {
			code = k->get_unicode();
		}
		if (k->get_control()) {
			code |= KEY_MASK_CTRL;
		}
		if (k->get_alt()) {
			code |= KEY_MASK_ALT;
		}
		if (k->get_metakey()) {
			code |= KEY_MASK_META;
		}
		if (k->get_shift()) {
			code |= KEY_MASK_SHIFT;
		}
	}

	for (int i = 0; i < items.size(); i++) {
		if (is_item_disabled(i) || items[i].shortcut_is_disabled) {
			continue;
		}

		if (items[i].shortcut.is_valid() && items[i].shortcut->is_shortcut(p_event) && (items[i].shortcut_is_global || !p_for_global_only)) {
			activate_item(i);
			return true;
		}

		if (code != 0 && items[i].accel == code) {
			activate_item(i);
			return true;
		}

		if (items[i].submenu != "") {
			Node *n = get_node(items[i].submenu);
			if (!n) {
				continue;
			}

			PopupMenu *pm = Object::cast_to<PopupMenu>(n);
			if (!pm) {
				continue;
			}

			if (pm->activate_item_by_event(p_event, p_for_global_only)) {
				return true;
			}
		}
	}
	return false;
}

void PopupMenu::activate_item(int p_item) {
	ERR_FAIL_INDEX(p_item, items.size());
	ERR_FAIL_COND(items[p_item].separator);
	int id = items[p_item].id >= 0 ? items[p_item].id : p_item;

	//hide all parent PopupMenus
	Node *next = get_parent();
	PopupMenu *pop = Object::cast_to<PopupMenu>(next);
	while (pop) {
		// We close all parents that are chained together,
		// with hide_on_item_selection enabled

		if (items[p_item].checkable_type) {
			if (!hide_on_checkable_item_selection || !pop->is_hide_on_checkable_item_selection()) {
				break;
			}
		} else if (0 < items[p_item].max_states) {
			if (!hide_on_multistate_item_selection || !pop->is_hide_on_multistate_item_selection()) {
				break;
			}
		} else if (!hide_on_item_selection || !pop->is_hide_on_item_selection()) {
			break;
		}

		pop->hide();
		next = next->get_parent();
		pop = Object::cast_to<PopupMenu>(next);
	}

	// Hides popup by default; unless otherwise specified
	// by using set_hide_on_item_selection and set_hide_on_checkable_item_selection

	bool need_hide = true;

	if (items[p_item].checkable_type) {
		if (!hide_on_checkable_item_selection) {
			need_hide = false;
		}
	} else if (0 < items[p_item].max_states) {
		if (!hide_on_multistate_item_selection) {
			need_hide = false;
		}
	} else if (!hide_on_item_selection) {
		need_hide = false;
	}

	emit_signal("id_pressed", id);
	emit_signal("index_pressed", p_item);

	if (need_hide) {
		hide();
	}
}

void PopupMenu::remove_item(int p_idx) {
	ERR_FAIL_INDEX(p_idx, items.size());

	if (items[p_idx].shortcut.is_valid()) {
		_unref_shortcut(items[p_idx].shortcut);
	}

	items.remove(p_idx);
	control->update();
	child_controls_changed();
}

void PopupMenu::add_separator(const String &p_text) {
	Item sep;
	sep.separator = true;
	sep.id = -1;
	if (p_text != String()) {
		sep.text = p_text;
		sep.xl_text = tr(p_text);
	}
	items.push_back(sep);
	control->update();
}

void PopupMenu::clear() {
	for (int i = 0; i < items.size(); i++) {
		if (items[i].shortcut.is_valid()) {
			_unref_shortcut(items[i].shortcut);
		}
	}
	items.clear();
	mouse_over = -1;
	control->update();
	child_controls_changed();
}

Array PopupMenu::_get_items() const {
	Array items;
	for (int i = 0; i < get_item_count(); i++) {
		items.push_back(get_item_text(i));
		items.push_back(get_item_icon(i));
		// For compatibility, use false/true for no/checkbox and integers for other values
		int ct = this->items[i].checkable_type;
		items.push_back(Variant(ct <= Item::CHECKABLE_TYPE_CHECK_BOX ? is_item_checkable(i) : ct));
		items.push_back(is_item_checked(i));
		items.push_back(is_item_disabled(i));

		items.push_back(get_item_id(i));
		items.push_back(get_item_accelerator(i));
		items.push_back(get_item_metadata(i));
		items.push_back(get_item_submenu(i));
		items.push_back(is_item_separator(i));
	}

	return items;
}

void PopupMenu::_ref_shortcut(Ref<Shortcut> p_sc) {
	if (!shortcut_refcount.has(p_sc)) {
		shortcut_refcount[p_sc] = 1;
		p_sc->connect("changed", callable_mp((CanvasItem *)this, &CanvasItem::update));
	} else {
		shortcut_refcount[p_sc] += 1;
	}
}

void PopupMenu::_unref_shortcut(Ref<Shortcut> p_sc) {
	ERR_FAIL_COND(!shortcut_refcount.has(p_sc));
	shortcut_refcount[p_sc]--;
	if (shortcut_refcount[p_sc] == 0) {
		p_sc->disconnect("changed", callable_mp((CanvasItem *)this, &CanvasItem::update));
		shortcut_refcount.erase(p_sc);
	}
}

void PopupMenu::_set_items(const Array &p_items) {
	ERR_FAIL_COND(p_items.size() % 10);
	clear();

	for (int i = 0; i < p_items.size(); i += 10) {
		String text = p_items[i + 0];
		Ref<Texture2D> icon = p_items[i + 1];
		// For compatibility, use false/true for no/checkbox and integers for other values
		bool checkable = p_items[i + 2];
		bool radio_checkable = (int)p_items[i + 2] == Item::CHECKABLE_TYPE_RADIO_BUTTON;
		bool checked = p_items[i + 3];
		bool disabled = p_items[i + 4];

		int id = p_items[i + 5];
		int accel = p_items[i + 6];
		Variant meta = p_items[i + 7];
		String subm = p_items[i + 8];
		bool sep = p_items[i + 9];

		int idx = get_item_count();
		add_item(text, id);
		set_item_icon(idx, icon);
		if (checkable) {
			if (radio_checkable) {
				set_item_as_radio_checkable(idx, true);
			} else {
				set_item_as_checkable(idx, true);
			}
		}
		set_item_checked(idx, checked);
		set_item_disabled(idx, disabled);
		set_item_id(idx, id);
		set_item_metadata(idx, meta);
		set_item_as_separator(idx, sep);
		set_item_accelerator(idx, accel);
		set_item_submenu(idx, subm);
	}
}

// Hide on item selection determines whether or not the popup will close after item selection
void PopupMenu::set_hide_on_item_selection(bool p_enabled) {
	hide_on_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_item_selection() const {
	return hide_on_item_selection;
}

void PopupMenu::set_hide_on_checkable_item_selection(bool p_enabled) {
	hide_on_checkable_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_checkable_item_selection() const {
	return hide_on_checkable_item_selection;
}

void PopupMenu::set_hide_on_multistate_item_selection(bool p_enabled) {
	hide_on_multistate_item_selection = p_enabled;
}

bool PopupMenu::is_hide_on_multistate_item_selection() const {
	return hide_on_multistate_item_selection;
}

void PopupMenu::set_submenu_popup_delay(float p_time) {
	if (p_time <= 0) {
		p_time = 0.01;
	}

	submenu_timer->set_wait_time(p_time);
}

float PopupMenu::get_submenu_popup_delay() const {
	return submenu_timer->get_wait_time();
}

void PopupMenu::set_allow_search(bool p_allow) {
	allow_search = p_allow;
}

bool PopupMenu::get_allow_search() const {
	return allow_search;
}

String PopupMenu::get_tooltip(const Point2 &p_pos) const {
	int over = _get_mouse_over(p_pos);
	if (over < 0 || over >= items.size()) {
		return "";
	}
	return items[over].tooltip;
}

void PopupMenu::set_parent_rect(const Rect2 &p_rect) {
	parent_rect = p_rect;
}

void PopupMenu::get_translatable_strings(List<String> *p_strings) const {
	for (int i = 0; i < items.size(); i++) {
		if (items[i].xl_text != "") {
			p_strings->push_back(items[i].xl_text);
		}
	}
}

void PopupMenu::add_autohide_area(const Rect2 &p_area) {
	autohide_areas.push_back(p_area);
}

void PopupMenu::clear_autohide_areas() {
	autohide_areas.clear();
}

void PopupMenu::take_mouse_focus() {
	ERR_FAIL_COND(!is_inside_tree());

	if (get_parent()) {
		get_parent()->get_viewport()->pass_mouse_focus_to(this, control);
	}
}

void PopupMenu::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_gui_input"), &PopupMenu::_gui_input);

	ClassDB::bind_method(D_METHOD("add_item", "label", "id", "accel"), &PopupMenu::add_item, DEFVAL(-1), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("add_icon_item", "texture", "label", "id", "accel"), &PopupMenu::add_icon_item, DEFVAL(-1), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("add_check_item", "label", "id", "accel"), &PopupMenu::add_check_item, DEFVAL(-1), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("add_icon_check_item", "texture", "label", "id", "accel"), &PopupMenu::add_icon_check_item, DEFVAL(-1), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("add_radio_check_item", "label", "id", "accel"), &PopupMenu::add_radio_check_item, DEFVAL(-1), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("add_icon_radio_check_item", "texture", "label", "id", "accel"), &PopupMenu::add_icon_radio_check_item, DEFVAL(-1), DEFVAL(0));

	ClassDB::bind_method(D_METHOD("add_multistate_item", "label", "max_states", "default_state", "id", "accel"), &PopupMenu::add_multistate_item, DEFVAL(0), DEFVAL(-1), DEFVAL(0));

	ClassDB::bind_method(D_METHOD("add_shortcut", "shortcut", "id", "global"), &PopupMenu::add_shortcut, DEFVAL(-1), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("add_icon_shortcut", "texture", "shortcut", "id", "global"), &PopupMenu::add_icon_shortcut, DEFVAL(-1), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("add_check_shortcut", "shortcut", "id", "global"), &PopupMenu::add_check_shortcut, DEFVAL(-1), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("add_icon_check_shortcut", "texture", "shortcut", "id", "global"), &PopupMenu::add_icon_check_shortcut, DEFVAL(-1), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("add_radio_check_shortcut", "shortcut", "id", "global"), &PopupMenu::add_radio_check_shortcut, DEFVAL(-1), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("add_icon_radio_check_shortcut", "texture", "shortcut", "id", "global"), &PopupMenu::add_icon_radio_check_shortcut, DEFVAL(-1), DEFVAL(false));

	ClassDB::bind_method(D_METHOD("add_submenu_item", "label", "submenu", "id"), &PopupMenu::add_submenu_item, DEFVAL(-1));

	ClassDB::bind_method(D_METHOD("set_item_text", "idx", "text"), &PopupMenu::set_item_text);
	ClassDB::bind_method(D_METHOD("set_item_icon", "idx", "icon"), &PopupMenu::set_item_icon);
	ClassDB::bind_method(D_METHOD("set_item_checked", "idx", "checked"), &PopupMenu::set_item_checked);
	ClassDB::bind_method(D_METHOD("set_item_id", "idx", "id"), &PopupMenu::set_item_id);
	ClassDB::bind_method(D_METHOD("set_item_accelerator", "idx", "accel"), &PopupMenu::set_item_accelerator);
	ClassDB::bind_method(D_METHOD("set_item_metadata", "idx", "metadata"), &PopupMenu::set_item_metadata);
	ClassDB::bind_method(D_METHOD("set_item_disabled", "idx", "disabled"), &PopupMenu::set_item_disabled);
	ClassDB::bind_method(D_METHOD("set_item_submenu", "idx", "submenu"), &PopupMenu::set_item_submenu);
	ClassDB::bind_method(D_METHOD("set_item_as_separator", "idx", "enable"), &PopupMenu::set_item_as_separator);
	ClassDB::bind_method(D_METHOD("set_item_as_checkable", "idx", "enable"), &PopupMenu::set_item_as_checkable);
	ClassDB::bind_method(D_METHOD("set_item_as_radio_checkable", "idx", "enable"), &PopupMenu::set_item_as_radio_checkable);
	ClassDB::bind_method(D_METHOD("set_item_tooltip", "idx", "tooltip"), &PopupMenu::set_item_tooltip);
	ClassDB::bind_method(D_METHOD("set_item_shortcut", "idx", "shortcut", "global"), &PopupMenu::set_item_shortcut, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_item_multistate", "idx", "state"), &PopupMenu::set_item_multistate);
	ClassDB::bind_method(D_METHOD("set_item_shortcut_disabled", "idx", "disabled"), &PopupMenu::set_item_shortcut_disabled);

	ClassDB::bind_method(D_METHOD("toggle_item_checked", "idx"), &PopupMenu::toggle_item_checked);
	ClassDB::bind_method(D_METHOD("toggle_item_multistate", "idx"), &PopupMenu::toggle_item_multistate);

	ClassDB::bind_method(D_METHOD("get_item_text", "idx"), &PopupMenu::get_item_text);
	ClassDB::bind_method(D_METHOD("get_item_icon", "idx"), &PopupMenu::get_item_icon);
	ClassDB::bind_method(D_METHOD("is_item_checked", "idx"), &PopupMenu::is_item_checked);
	ClassDB::bind_method(D_METHOD("get_item_id", "idx"), &PopupMenu::get_item_id);
	ClassDB::bind_method(D_METHOD("get_item_index", "id"), &PopupMenu::get_item_index);
	ClassDB::bind_method(D_METHOD("get_item_accelerator", "idx"), &PopupMenu::get_item_accelerator);
	ClassDB::bind_method(D_METHOD("get_item_metadata", "idx"), &PopupMenu::get_item_metadata);
	ClassDB::bind_method(D_METHOD("is_item_disabled", "idx"), &PopupMenu::is_item_disabled);
	ClassDB::bind_method(D_METHOD("get_item_submenu", "idx"), &PopupMenu::get_item_submenu);
	ClassDB::bind_method(D_METHOD("is_item_separator", "idx"), &PopupMenu::is_item_separator);
	ClassDB::bind_method(D_METHOD("is_item_checkable", "idx"), &PopupMenu::is_item_checkable);
	ClassDB::bind_method(D_METHOD("is_item_radio_checkable", "idx"), &PopupMenu::is_item_radio_checkable);
	ClassDB::bind_method(D_METHOD("is_item_shortcut_disabled", "idx"), &PopupMenu::is_item_shortcut_disabled);
	ClassDB::bind_method(D_METHOD("get_item_tooltip", "idx"), &PopupMenu::get_item_tooltip);
	ClassDB::bind_method(D_METHOD("get_item_shortcut", "idx"), &PopupMenu::get_item_shortcut);

	ClassDB::bind_method(D_METHOD("get_current_index"), &PopupMenu::get_current_index);
	ClassDB::bind_method(D_METHOD("get_item_count"), &PopupMenu::get_item_count);

	ClassDB::bind_method(D_METHOD("remove_item", "idx"), &PopupMenu::remove_item);

	ClassDB::bind_method(D_METHOD("add_separator", "label"), &PopupMenu::add_separator, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("clear"), &PopupMenu::clear);

	ClassDB::bind_method(D_METHOD("_set_items"), &PopupMenu::_set_items);
	ClassDB::bind_method(D_METHOD("_get_items"), &PopupMenu::_get_items);

	ClassDB::bind_method(D_METHOD("set_hide_on_item_selection", "enable"), &PopupMenu::set_hide_on_item_selection);
	ClassDB::bind_method(D_METHOD("is_hide_on_item_selection"), &PopupMenu::is_hide_on_item_selection);

	ClassDB::bind_method(D_METHOD("set_hide_on_checkable_item_selection", "enable"), &PopupMenu::set_hide_on_checkable_item_selection);
	ClassDB::bind_method(D_METHOD("is_hide_on_checkable_item_selection"), &PopupMenu::is_hide_on_checkable_item_selection);

	ClassDB::bind_method(D_METHOD("set_hide_on_state_item_selection", "enable"), &PopupMenu::set_hide_on_multistate_item_selection);
	ClassDB::bind_method(D_METHOD("is_hide_on_state_item_selection"), &PopupMenu::is_hide_on_multistate_item_selection);

	ClassDB::bind_method(D_METHOD("set_submenu_popup_delay", "seconds"), &PopupMenu::set_submenu_popup_delay);
	ClassDB::bind_method(D_METHOD("get_submenu_popup_delay"), &PopupMenu::get_submenu_popup_delay);

	ClassDB::bind_method(D_METHOD("set_allow_search", "allow"), &PopupMenu::set_allow_search);
	ClassDB::bind_method(D_METHOD("get_allow_search"), &PopupMenu::get_allow_search);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "items", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL), "_set_items", "_get_items");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_on_item_selection"), "set_hide_on_item_selection", "is_hide_on_item_selection");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_on_checkable_item_selection"), "set_hide_on_checkable_item_selection", "is_hide_on_checkable_item_selection");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hide_on_state_item_selection"), "set_hide_on_state_item_selection", "is_hide_on_state_item_selection");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "submenu_popup_delay"), "set_submenu_popup_delay", "get_submenu_popup_delay");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow_search"), "set_allow_search", "get_allow_search");

	ADD_SIGNAL(MethodInfo("id_pressed", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("id_focused", PropertyInfo(Variant::INT, "id")));
	ADD_SIGNAL(MethodInfo("index_pressed", PropertyInfo(Variant::INT, "index")));
}

void PopupMenu::popup(const Rect2 &p_bounds) {
	moved = Vector2();
	popup_time_msec = OS::get_singleton()->get_ticks_msec();
	set_as_minsize();
	Popup::popup(p_bounds);
}

PopupMenu::PopupMenu() {
	// Margin Container
	margin_container = memnew(MarginContainer);
	margin_container->set_anchors_and_margins_preset(Control::PRESET_WIDE);
	add_child(margin_container);
	margin_container->connect("draw", callable_mp(this, &PopupMenu::_draw_background));

	// Scroll Container
	scroll_container = memnew(ScrollContainer);
	scroll_container->set_clip_contents(true);
	margin_container->add_child(scroll_container);

	// The control which will display the items
	control = memnew(Control);
	control->set_clip_contents(false);
	control->set_anchors_and_margins_preset(Control::PRESET_WIDE);
	control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	control->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scroll_container->add_child(control);
	control->connect("draw", callable_mp(this, &PopupMenu::_draw_items));

	connect("window_input", callable_mp(this, &PopupMenu::_gui_input));

	mouse_over = -1;
	submenu_over = -1;
	initial_button_mask = 0;
	during_grabbed_click = false;

	allow_search = true;
	search_time_msec = 0;
	search_string = "";

	set_hide_on_item_selection(true);
	set_hide_on_checkable_item_selection(true);
	set_hide_on_multistate_item_selection(false);

	submenu_timer = memnew(Timer);
	submenu_timer->set_wait_time(0.3);
	submenu_timer->set_one_shot(true);
	submenu_timer->connect("timeout", callable_mp(this, &PopupMenu::_submenu_timeout));
	add_child(submenu_timer);
}

PopupMenu::~PopupMenu() {
}
