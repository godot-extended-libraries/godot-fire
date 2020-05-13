/*************************************************************************/
/*  multi_constraint_ik_editor.cpp                                       */
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
#include "skeleton_editor_plugin.h"

void SkeletonIKMultiConstraintEditor::_update_effector_node_node_assign() {
	if (!scene_tree) {
		scene_tree = memnew(SceneTreeDialog);
		scene_tree->get_scene_tree()->set_show_enabled_subscene(true);
		add_child(scene_tree);
		scene_tree->connect("selected", this, "_update_effector_node");
	}
	scene_tree->popup_centered_ratio();
}

void SkeletonIKMultiConstraintEditor::_update_effector_node(String p_path) {
	const BoneId bone_id = property.get_slicec('/', 1).to_int();
	const String name = skeleton->get_bone_name(bone_id);
	if (name.empty()) {
		return;
	}
	ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
	Ref<MultiConstraint> constraint = skeleton->get_multi_constraint();
	int32_t effector_index = constraint->find_effector(name);
	ERR_FAIL_COND(effector_index == -1);
	ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
	ERR_FAIL_INDEX(effector_index, skeleton->get_multi_constraint()->get_effector_count());
	Ref<BoneEffector> effector = skeleton->get_multi_constraint()->get_effector(effector_index);
	if (effector.is_null()) {
		return;
	}
	if (p_path.find("/root/EditorNode/") != -1) {
		Node *to_node = get_node(p_path);
		ERR_FAIL_COND(!to_node);
		p_path = skeleton->get_path_to(to_node);
	}
	effector->set_target_node(p_path);
	assign->set_text(p_path);
	if (p_path.empty()) {
		assign->set_text(TTR("Assign..."));
	}
	skeleton->get_multi_constraint()->set_constraint(effector_index, effector);
	_update_properties();
}

void SkeletonIKMultiConstraintEditor::_update_effector_target_transform(int32_t p_effector_index, Transform p_value) {
	ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
	ERR_FAIL_INDEX(p_effector_index, skeleton->get_multi_constraint()->get_effector_count());
	Ref<BoneEffector> effector = skeleton->get_multi_constraint()->get_effector(p_effector_index);
	if (effector.is_null()) {
		return;
	}
	effector->set_target_transform(p_value);
	skeleton->get_multi_constraint()->set_constraint(p_effector_index, effector);
	_update_properties();
}

void SkeletonIKMultiConstraintEditor::_create_effector_button_pressed() {
	if (skeleton == nullptr)
		return;

	const BoneId bone_id = property.get_slicec('/', 1).to_int();
	const String name = skeleton->get_bone_name(bone_id);

	if (name.empty())
		return;
	ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
	if (skeleton->get_multi_constraint()->find_effector(name) != -1) {
		return;
	}
	Ref<MultiConstraint> constraint = skeleton->get_multi_constraint();
	constraint->add_effector(name);
	create_effector_button->set_visible(false);
	effectors_remove_effector_button->set_visible(false);
	joints_remove_effector_button->set_visible(true);
	_update_properties();
}

void SkeletonIKMultiConstraintEditor::_effectors_remove_effector_button_pressed() {
	if (skeleton == nullptr) {
		return;
	}
	const String what = property.get_slicec('/', 0);
	ERR_FAIL_COND(what != "effectors");
	const BoneId effector_id = property.get_slicec('/', 1).to_int();
	ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
	Ref<MultiConstraint> constraint = skeleton->get_multi_constraint();
	ERR_FAIL_COND(effector_id == -1);
	constraint->remove_effector(effector_id);
	create_effector_button->set_visible(true);
	effectors_remove_effector_button->set_visible(false);
	joints_remove_effector_button->set_visible(false);
	effector_target_transform_editor->set_visible(false);
	effector_target_node_editor->set_visible(false);
	_update_properties();
}

void SkeletonIKMultiConstraintEditor::_joints_remove_effector_button_pressed() {
	if (skeleton == nullptr)
		return;

	const String what = property.get_slicec('/', 0);
	ERR_FAIL_COND(what != "bones");
	const BoneId bone_id = property.get_slicec('/', 1).to_int();
	const String name = skeleton->get_bone_name(bone_id);

	if (name.empty())
		return;
	ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());

	Ref<MultiConstraint> constraint = skeleton->get_multi_constraint();
	int32_t effector_index = constraint->find_effector(name);
	ERR_FAIL_COND(effector_index == -1);
	constraint->remove_effector(effector_index);
	create_effector_button->set_visible(true);
	joints_remove_effector_button->set_visible(false);
	_update_properties();
}

void SkeletonIKMultiConstraintEditor::set_target(const String &p_prop) {
	property = p_prop;
}

Skeleton *SkeletonIKMultiConstraintEditor::get_skeleton() const {
	return skeleton;
}

void SkeletonIKMultiConstraintEditor::_joint_tree_selection_changed() {
	create_effector_button->set_visible(false);
	joints_remove_effector_button->set_visible(false);
	TreeItem *selected = joint_tree->get_selected();
	const String path = selected->get_metadata(0);
	set_target(path);
	if (path.begins_with("bones/")) {
		const int b_idx = path.get_slicec('/', 1).to_int();
		ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
		String bone_name = skeleton->get_bone_name(b_idx);
		if (skeleton->get_multi_constraint()->find_effector(bone_name) == -1) {
			create_effector_button->set_visible(true);
			joints_remove_effector_button->set_visible(false);
		} else {
			create_effector_button->set_visible(false);
			joints_remove_effector_button->set_visible(true);
		}
		effector_target_node_editor->set_visible(false);
		effector_target_transform_editor->set_visible(false);
		effectors_remove_effector_button->set_visible(false);
		effector_tree->deselect_all();
	}
}

void SkeletonIKMultiConstraintEditor::_effector_tree_selection_changed() {
	effector_target_node_editor->set_visible(false);
	effector_target_transform_editor->set_visible(false);
	TreeItem *selected = effector_tree->get_selected();
	const String path = selected->get_metadata(0);
	set_target(path);
	if (path.begins_with("effectors/")) {
		const int b_idx = path.get_slicec('/', 1).to_int();
		ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
		Ref<BoneEffector> effector = skeleton->get_multi_constraint()->get_effector(b_idx);
		ERR_FAIL_COND(effector.is_null());
		const String bone_name = effector->get_name();
		ERR_FAIL_COND(bone_name.empty());
		const String effector_path = "effectors/" + itos(b_idx) + "/";
		if (skeleton->get_multi_constraint()->find_effector(bone_name) == -1) {
			effectors_remove_effector_button->set_visible(false);
		} else {
			effectors_remove_effector_button->set_visible(true);
		}
		effector_target_node_editor->set_visible(true);
		effector_target_transform_editor->set_target(effector_path + "target_transform");
		effector_target_transform_editor->set_visible(true);
		joint_tree->deselect_all();
	}
}

void SkeletonIKMultiConstraintEditor::_joint_tree_rmb_select(const Vector2 &p_pos) {
}

void SkeletonIKMultiConstraintEditor::update_joint_tree() {
	joint_tree->clear();
	TreeItem *root = joint_tree->create_item();

	Map<int, TreeItem *> items;

	items.insert(-1, root);

	const Vector<int> &joint_porder = skeleton->get_bone_process_order();

	Ref<Texture> bone_icon = get_icon("Bone", "EditorIcons");

	for (int i = 0; i < joint_porder.size(); ++i) {
		const int b_idx = joint_porder[i];

		const int p_idx = skeleton->get_bone_parent(b_idx);
		TreeItem *p_item = items.find(p_idx)->get();
		ERR_CONTINUE(!p_item);
		TreeItem *joint_item = joint_tree->create_item(p_item);
		items.insert(b_idx, joint_item);

		joint_item->set_text(0, skeleton->get_bone_name(b_idx));
		joint_item->set_icon(0, bone_icon);
		joint_item->set_selectable(0, true);
		joint_item->set_metadata(0, "bones/" + itos(b_idx));
	}
}

void SkeletonIKMultiConstraintEditor::update_effector_tree() {
	effector_tree->clear();

	TreeItem *root = effector_tree->create_item();
	Ref<Texture> effector_icon = get_icon("Position3D", "EditorIcons");
	Ref<MultiConstraint> constraints = skeleton->get_multi_constraint();
	ERR_FAIL_COND(constraints.is_null());
	Vector<Ref<BoneEffector> > effectors = constraints->get_bone_effectors();
	int32_t size = effectors.size();
	for (int i = 0; i < size; i++) {
		if (effectors[size - 1 - i].is_null()) {
			effectors.remove(size - 1 - i);
			continue;
		}
		TreeItem *effector_item = effector_tree->create_item(root);
		ERR_CONTINUE(!effector_item);
		String name = effectors[size - 1 - i]->get_name();
		if (name.empty()) {
			continue;
		}
		int32_t bone = skeleton->find_bone(name);
		if (bone == -1) {
			continue;
		}
		effector_item->set_text(0, name);
		effector_item->set_icon(0, effector_icon);
		effector_item->set_selectable(0, true);
		effector_item->set_metadata(0, "effectors/" + itos(size - 1 - i) + "/" + name);
	}
}

void SkeletonIKMultiConstraintEditor::update_editors() {
}

void SkeletonIKMultiConstraintEditor::create_editors() {
	const Color section_color = get_color("prop_subsection", "Editor");

	EditorInspectorSection *effector_section = memnew(EditorInspectorSection);
	effector_section->setup("effectors", TTR("Effectors"), skeleton, section_color, true);
	add_child(effector_section);

	ScrollContainer *e_s_con = memnew(ScrollContainer);
	e_s_con->set_custom_minimum_size(Size2(1, 175) * EDSCALE);
	effector_section->get_vbox()->add_child(e_s_con);

	effector_tree = memnew(Tree);
	effector_tree->set_columns(1);
	effector_tree->set_select_mode(Tree::SELECT_SINGLE);
	effector_tree->set_hide_root(true);
	effector_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	effector_tree->set_h_size_flags(SIZE_EXPAND_FILL);
	effector_tree->set_allow_rmb_select(true);
	effector_tree->set_drag_forwarding(this);
	effector_tree->set_focus_mode(Control::FocusMode::FOCUS_NONE);
	e_s_con->add_child(effector_tree);

	effectors_remove_effector_button = memnew(Button);
	effectors_remove_effector_button->set_text(TTR("Remove Effector"));
	effectors_remove_effector_button->set_visible(false);
	effectors_remove_effector_button->set_icon(get_icon("Position3D", "EditorIcons"));
	effectors_remove_effector_button->set_flat(true);
	effector_section->get_vbox()->add_child(effectors_remove_effector_button);

	effector_target_node_editor = memnew(HBoxContainer);
	effector_target_node_editor->set_visible(false);
	effector_section->get_vbox()->add_child(effector_target_node_editor);
	effector_section->unfold();
	target_node_label = memnew(Label);
	target_node_label->set_text(TTR("Target Node"));
	effector_target_node_editor->add_child(target_node_label);

	assign = memnew(Button);
	assign->set_text(TTR("Assign..."));
	assign->set_flat(true);
	assign->set_h_size_flags(SIZE_EXPAND_FILL);
	assign->set_clip_text(true);
	assign->connect("pressed", this, "_update_effector_node_node_assign");
	effector_target_node_editor->add_child(assign);

	clear = memnew(Button);
	clear->set_text(TTR("Clear"));
	clear->set_flat(true);
	clear->connect("pressed", this, "_update_effector_node", varray(NodePath()));
	effector_target_node_editor->add_child(clear);
	scene_tree = NULL;

	effector_target_transform_editor = memnew(EffectorTargetTransformEditor(skeleton));
	effector_target_transform_editor->set_label(TTR("Target Transform"));
	effector_target_transform_editor->set_visible(false);
	effector_section->get_vbox()->add_child(effector_target_transform_editor);

	EditorInspectorSection *bones_section = memnew(EditorInspectorSection);
	bones_section->setup("bones", TTR("Bones"), skeleton, section_color, true);
	add_child(bones_section);
	bones_section->unfold();

	ScrollContainer *s_con = memnew(ScrollContainer);
	s_con->set_focus_mode(Control::FocusMode::FOCUS_NONE);
	s_con->set_custom_minimum_size(Size2(1, 350) * EDSCALE);
	s_con->set_block_minimum_size_adjust(true);
	bones_section->get_vbox()->add_child(s_con);

	joint_tree = memnew(Tree);
	joint_tree->set_columns(1);
	joint_tree->set_select_mode(Tree::SELECT_SINGLE);
	joint_tree->set_hide_root(true);
	joint_tree->set_v_size_flags(SIZE_EXPAND_FILL);
	joint_tree->set_h_size_flags(SIZE_EXPAND_FILL);
	joint_tree->set_allow_rmb_select(true);
	joint_tree->set_drag_forwarding(this);
	joint_tree->set_focus_mode(Control::FocusMode::FOCUS_NONE);
	s_con->add_child(joint_tree);

	create_effector_button = memnew(Button);
	create_effector_button->set_text(TTR("Create Effector"));
	create_effector_button->set_visible(false);
	create_effector_button->set_icon(get_icon("Position3D", "EditorIcons"));
	create_effector_button->set_flat(true);
	bones_section->get_vbox()->add_child(create_effector_button);

	joints_remove_effector_button = memnew(Button);
	joints_remove_effector_button->set_text(TTR("Remove Effector"));
	joints_remove_effector_button->set_visible(false);
	joints_remove_effector_button->set_icon(get_icon("Position3D", "EditorIcons"));
	joints_remove_effector_button->set_flat(true);
	bones_section->get_vbox()->add_child(joints_remove_effector_button);
}

void SkeletonIKMultiConstraintEditor::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			create_editors();
			_update_properties();
			joint_tree->connect("item_selected", this, "_joint_tree_selection_changed");
			joint_tree->connect("item_rmb_selected", this, "_joint_tree_rmb_select");
			effector_tree->connect("item_selected", this, "_effector_tree_selection_changed");
			skeleton->connect("ik_changed", this, "_update_properties");
			ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
			skeleton->get_multi_constraint()->connect("ik_changed", this, "_update_properties");
			create_effector_button->connect("pressed", this, "_create_effector_button_pressed");
			effectors_remove_effector_button->connect("pressed", this, "_effectors_remove_effector_button_pressed");
			joints_remove_effector_button->connect("pressed", this, "_joints_remove_effector_button_pressed");
			effector_target_transform_editor->connect("effector_property_changed", this, "_update_effector_target_transform");
			break;
		}
		case NOTIFICATION_EXIT_TREE: {
			joint_tree->disconnect("item_selected", this, "_joint_tree_selection_changed");
			joint_tree->disconnect("item_rmb_selected", this, "_joint_tree_rmb_select");
			effector_tree->disconnect("item_selected", this, "_effector_tree_selection_changed");
			skeleton->disconnect("ik_changed", this, "_update_properties");
			ERR_FAIL_COND(skeleton->get_multi_constraint().is_null());
			skeleton->get_multi_constraint()->disconnect("ik_changed", this, "_update_properties");
			create_effector_button->disconnect("pressed", this, "_create_effector_button_pressed");
			effectors_remove_effector_button->disconnect("pressed", this, "_effectors_remove_effector_button_pressed");
			joints_remove_effector_button->disconnect("pressed", this, "_joints_remove_effector_button_pressed");
			effector_target_transform_editor->disconnect("effector_property_changed", this, "_update_effector_target_transform");
			break;
		}
	}
}

void SkeletonIKMultiConstraintEditor::_update_properties() {
	update_joint_tree();
	update_effector_tree();
	update_editors();
}

void SkeletonIKMultiConstraintEditor::_node_removed(Node *p_node) {
	if (skeleton && p_node == skeleton) {
		skeleton = nullptr;
	}
}

void SkeletonIKMultiConstraintEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_update_effector_node_node_assign"), &SkeletonIKMultiConstraintEditor::_update_effector_node_node_assign);
	ClassDB::bind_method(D_METHOD("_update_effector_node", "path"), &SkeletonIKMultiConstraintEditor::_update_effector_node);
	ClassDB::bind_method(D_METHOD("_node_removed"), &SkeletonIKMultiConstraintEditor::_node_removed);
	ClassDB::bind_method(D_METHOD("_joint_tree_selection_changed"), &SkeletonIKMultiConstraintEditor::_joint_tree_selection_changed);
	ClassDB::bind_method(D_METHOD("_joint_tree_rmb_select"), &SkeletonIKMultiConstraintEditor::_joint_tree_rmb_select);
	ClassDB::bind_method(D_METHOD("_update_properties"), &SkeletonIKMultiConstraintEditor::_update_properties);
	ClassDB::bind_method(D_METHOD("_effector_tree_selection_changed"), &SkeletonIKMultiConstraintEditor::_effector_tree_selection_changed);
	ClassDB::bind_method(D_METHOD("_create_effector_button_pressed"), &SkeletonIKMultiConstraintEditor::_create_effector_button_pressed);
	ClassDB::bind_method(D_METHOD("_joints_remove_effector_button_pressed"), &SkeletonIKMultiConstraintEditor::_joints_remove_effector_button_pressed);
	ClassDB::bind_method(D_METHOD("_effectors_remove_effector_button_pressed"), &SkeletonIKMultiConstraintEditor::_effectors_remove_effector_button_pressed);
	ClassDB::bind_method(D_METHOD("_update_effector_target_transform", "index", "transform"), &SkeletonIKMultiConstraintEditor::_update_effector_target_transform);
}

SkeletonIKMultiConstraintEditor::SkeletonIKMultiConstraintEditor(EditorInspectorPluginSkeleton *e_plugin, EditorNode *p_editor, Skeleton *p_ik) :
		editor(p_editor),
		editor_plugin(e_plugin),
		skeleton(p_ik) {
}

SkeletonIKMultiConstraintEditor::~SkeletonIKMultiConstraintEditor() {
}
