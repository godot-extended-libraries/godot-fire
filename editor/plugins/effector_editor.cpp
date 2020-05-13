/*************************************************************************/
/*  effector_editor.h						                             */
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

#include "editor/plugins/skeleton_editor_plugin.h"
#include "core/io/resource_saver.h"
#include "editor/editor_file_dialog.h"
#include "editor/editor_properties.h"
#include "editor/editor_scale.h"
#include "editor/plugins/animation_player_editor_plugin.h"
#include "editor/plugins/spatial_editor_plugin.h"
#include "scene/3d/collision_shape.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/physics_body.h"
#include "scene/3d/physics_joint.h"
#include "scene/resources/capsule_shape.h"
#include "scene/resources/sphere_shape.h"

void EffectorTargetTransformEditor::set_target(const String &p_prop) {
	property = p_prop;
	_update_target_transform_properties();
}

void EffectorTargetTransformEditor::set_label(const String &p_label) {
	label = p_label;
}

void EffectorTargetTransformEditor::_update_target_transform_properties() {
	if (!ik)
		return;
	if (ik->get_multi_constraint().is_null())
		return;

	Transform tform = ik->get_multi_constraint()->get_effector(property.to_int())->get_target_transform();
	_update_transform_properties(tform);
}

void EffectorTargetTransformEditor::_update_transform_properties(Transform tform) {
	const int matrix_row_count = 3;
	const int matrix_column_count = 3;

	for (int r = 0; r < matrix_row_count; r++) {
		for (int c = 0; c < matrix_column_count; c++) {
			transform_slider[(r * matrix_column_count) + c]->set_value(tform.get_basis()[r][c]);
		}
	}
}

void EffectorTargetTransformEditor::_update_properties() {
	if (ik == nullptr)
		return;

	ERR_FAIL_COND(ik->get_multi_constraint().is_null());
	Transform tform = ik->get_multi_constraint()->get(property);
	_update_transform_properties(tform);
}

Transform EffectorTargetTransformEditor::compute_transform(const bool p_from_transform) const {
	// Last modified was a raw transform column...
	if (p_from_transform) {
		Transform tform;

		for (int i = 0; i < BASIS_COMPONENTS; ++i) {
			tform.basis[i / BASIS_SPLIT_COMPONENTS][i % BASIS_SPLIT_COMPONENTS] = transform_slider[i]->get_value();
		}

		for (int i = 0; i < TRANSLATION_COMPONENTS; ++i) {
			tform.origin[i] = transform_slider[i + BASIS_COMPONENTS]->get_value();
		}

		return tform;
	}
	Quat rot;
	float x = Math::deg2rad(rotation_slider[0]->get_value());
	float y = Math::deg2rad(rotation_slider[1]->get_value());
	float z = Math::deg2rad(rotation_slider[2]->get_value());
	Vector3 rot_rad = Vector3(x, y, z);
	rot.set_euler(rot_rad);
	return Transform(
			Basis(rot.normalized(),
					Vector3(scale_slider[0]->get_value(), scale_slider[1]->get_value(), scale_slider[2]->get_value())),
			Vector3(translation_slider[0]->get_value(), translation_slider[1]->get_value(), translation_slider[2]->get_value()));
}

void EffectorTargetTransformEditor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			create_editors();

			// [[fallthrough]];
		}
		case NOTIFICATION_THEME_CHANGED: {
			const Color base = get_color("accent_color", "Editor");
			const Color bg_color = get_color("property_color", "Editor");
			const Color bg_lbl_color(bg_color.r, bg_color.g, bg_color.b, 0.5);

			for (int i = 0; i < TRANSLATION_COMPONENTS; i++) {
				Color c = base;
				c.set_hsv(float(i % TRANSLATION_COMPONENTS) / TRANSLATION_COMPONENTS + 0.05, c.get_s() * 0.75, c.get_v());
				if (!translation_slider[i]) {
					continue;
				}
				translation_slider[i]->set_custom_label_color(true, c);
			}

			for (int i = 0; i < ROTATION_DEGREES_COMPONENTS; i++) {
				Color c = base;
				c.set_hsv(float(i % ROTATION_DEGREES_COMPONENTS) / ROTATION_DEGREES_COMPONENTS + 0.05, c.get_s() * 0.75, c.get_v());
				if (!rotation_slider[i]) {
					continue;
				}
				rotation_slider[i]->set_custom_label_color(true, c);
			}

			for (int i = 0; i < SCALE_COMPONENTS; i++) {
				Color c = base;
				c.set_hsv(float(i % SCALE_COMPONENTS) / SCALE_COMPONENTS + 0.05, c.get_s() * 0.75, c.get_v());
				if (!scale_slider[i]) {
					continue;
				}
				scale_slider[i]->set_custom_label_color(true, c);
			}

			for (int i = 0; i < TRANSFORM_COMPONENTS; i++) {
				Color c = base;
				c.set_hsv(float(i % TRANSFORM_COMPONENTS) / TRANSFORM_COMPONENTS + 0.05, c.get_s() * 0.75, c.get_v());
				if (!transform_slider[i]) {
					continue;
				}
				transform_slider[i]->set_custom_label_color(true, c);
			}

			break;
		}
		case NOTIFICATION_SORT_CHILDREN: {
			const Ref<Font> font = get_font("font", "Tree");

			Point2 buffer;
			buffer.x += get_constant("inspector_margin", "Editor");
			buffer.y += font->get_height();
			buffer.y += get_constant("vseparation", "Tree");

			const float vector_height = translation_grid->get_size().y;
			const float transform_height = transform_grid->get_size().y;

			const float width = get_size().x - get_constant("inspector_margin", "Editor");
			Vector<Rect2> input_rects;

			if (section->get_vbox()->is_visible()) {
				input_rects.push_back(Rect2(translation_grid->get_position() + buffer, Size2(width, vector_height)));
				input_rects.push_back(Rect2(rotation_grid->get_position() + buffer, Size2(width, vector_height)));
				input_rects.push_back(Rect2(scale_grid->get_position() + buffer, Size2(width, vector_height)));
				input_rects.push_back(Rect2(transform_grid->get_position() + buffer, Size2(width, transform_height)));
			} else {
				const int32_t start = input_rects.size();
				const int32_t empty_input_rect_elements = 4;
				const int32_t end = start + empty_input_rect_elements;
				for (int i = start; i < end; ++i) {
					input_rects.push_back(Rect2(0, 0, 0, 0));
				}
			}

			for (int32_t i = 0; i < input_rects.size(); i++) {
				background_rects[i] = input_rects[i];
			}

			update();
			break;
		}
		case NOTIFICATION_DRAW: {
			const Color dark_color = get_color("dark_color_2", "Editor");

			for (int i = 0; i < 5; ++i) {
				draw_rect(background_rects[i], dark_color);
			}

			break;
		}
	}
}

void EffectorTargetTransformEditor::setup_spinner(EditorSpinSlider *spinner, const bool is_transform_spinner) {
	spinner->set_flat(true);
	spinner->set_min(-10000);
	spinner->set_max(10000);
	spinner->set_step(0.001f);
	spinner->set_hide_slider(true);
	spinner->set_allow_greater(true);
	spinner->set_allow_lesser(true);
	spinner->set_h_size_flags(SIZE_EXPAND_FILL);

	spinner->connect("value_changed", this, "_value_changed", varray(is_transform_spinner));
}

void EffectorTargetTransformEditor::_value_changed(const double p_value, const bool p_from_transform) {
	if (property.get_slicec('/', 0) == "effectors" && property.get_slicec('/', 2) == "target_transform") {
		ERR_FAIL_COND(ik->get_multi_constraint().is_null());
		int32_t effector_i = property.get_slicec('/', 1).to_int();
		ERR_FAIL_INDEX(effector_i, ik->get_multi_constraint()->get_effector_count());
		const Transform tform = compute_transform(p_from_transform);
		Ref<BoneEffector> effector = ik->get_multi_constraint()->get_effector(effector_i);
		undo_redo->create_action(TTR("Set Effector Target Transform"), UndoRedo::MERGE_ENDS);
		if (effector.is_null()) {
			ik->get_multi_constraint()->remove_effector(effector_i);
			ERR_FAIL();
		}
		int32_t index = property.get_slicec('/', 1).to_int();
		Ref<MultiConstraint> multi_constraint = ik->get_multi_constraint();
		ERR_FAIL_COND(multi_constraint.is_null());
		ERR_FAIL_INDEX(index, multi_constraint->get_effector_count());
		ERR_FAIL_COND(multi_constraint->get_effector(index).is_null());
		multi_constraint->get_effector(index)->set_target_transform(tform);
		multi_constraint->emit_signal("ik_changed");
		_update_transform_properties(tform);
		emit_signal("effector_property_changed", index, tform);
	}
}

void EffectorTargetTransformEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_value_changed", "value"), &EffectorTargetTransformEditor::_value_changed);
	ClassDB::bind_method(D_METHOD("_update_transform_properties", "transform"), &EffectorTargetTransformEditor::_update_transform_properties);

	ADD_SIGNAL(MethodInfo("effector_property_changed"));
}

EffectorTargetTransformEditor::EffectorTargetTransformEditor(Skeleton *p_ik) :
		translation_slider(),
		rotation_slider(),
		scale_slider(),
		transform_slider(),
		ik(p_ik) {
	undo_redo = EditorNode::get_undo_redo();
}

void EffectorTargetTransformEditor::create_editors() {
	const Color section_color = get_color("prop_subsection", "Editor");

	section = memnew(EditorInspectorSection);
	section->setup("trf_properties", label, this, section_color, true);
	add_child(section);

	Label *l1 = memnew(Label(TTR("Translation")));
	section->get_vbox()->add_child(l1);

	translation_grid = memnew(GridContainer());
	translation_grid->set_columns(TRANSLATION_COMPONENTS);
	section->get_vbox()->add_child(translation_grid);

	Label *l2 = memnew(Label(TTR("Rotation Degrees")));
	section->get_vbox()->add_child(l2);

	rotation_grid = memnew(GridContainer());
	rotation_grid->set_columns(ROTATION_DEGREES_COMPONENTS);
	section->get_vbox()->add_child(rotation_grid);

	Label *l3 = memnew(Label(TTR("Scale")));
	section->get_vbox()->add_child(l3);

	scale_grid = memnew(GridContainer());
	scale_grid->set_columns(SCALE_COMPONENTS);
	section->get_vbox()->add_child(scale_grid);

	Label *l4 = memnew(Label(TTR("Transform")));
	section->get_vbox()->add_child(l4);

	transform_grid = memnew(GridContainer());
	transform_grid->set_columns(TRANSFORM_CONTROL_COMPONENTS);
	section->get_vbox()->add_child(transform_grid);
	section->unfold();

	static const char *desc[TRANSFORM_COMPONENTS] = { "x", "y", "z", "x", "y", "z", "x", "y", "z", "x", "y", "z" };

	for (int i = 0; i < TRANSFORM_CONTROL_COMPONENTS; ++i) {
		translation_slider[i] = memnew(EditorSpinSlider());
		translation_slider[i]->set_label(desc[i]);
		setup_spinner(translation_slider[i], false);
		translation_grid->add_child(translation_slider[i]);

		rotation_slider[i] = memnew(EditorSpinSlider());
		rotation_slider[i]->set_label(desc[i]);
		setup_spinner(rotation_slider[i], false);
		rotation_slider[i]->set_step(0.1f);
		rotation_grid->add_child(rotation_slider[i]);

		scale_slider[i] = memnew(EditorSpinSlider());
		scale_slider[i]->set_label(desc[i]);
		setup_spinner(scale_slider[i], false);
		scale_slider[i]->set_value(1.0f);
		scale_grid->add_child(scale_slider[i]);
	}

	for (int i = 0; i < TRANSFORM_COMPONENTS; ++i) {
		transform_slider[i] = memnew(EditorSpinSlider());
		transform_slider[i]->set_label(desc[i]);
		setup_spinner(transform_slider[i], true);
		transform_grid->add_child(transform_slider[i]);
	}
}
