/*************************************************************************/
/*  multi_constraint_ik_editor.h                                         */
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

#ifndef MULTI_CONSTRAINT_IK_EDITOR_H
#define MULTI_CONSTRAINT_IK_EDITOR_H
#include "scene/gui/box_container.h"
#include "skeleton_editor_plugin.h"

class EditorInspectorPluginSkeleton;

class SkeletonIKMultiConstraintEditor : public VBoxContainer {
	GDCLASS(SkeletonIKMultiConstraintEditor, VBoxContainer);
	friend class SkeletonIKMultiConstraintEditorPlugin;
	enum Menu {
		MENU_OPTION_CREATE_PHYSICAL_SKELETON
	};
	struct BoneInfo {
		PhysicalBone *physical_bone = nullptr;
		Transform relative_rest = Transform(); // Relative to skeleton node
		BoneInfo();
	};
	EditorNode *editor = nullptr;
	EditorInspectorPluginSkeleton *editor_plugin = nullptr;
	Skeleton *skeleton = nullptr;
	Tree *joint_tree = nullptr;
	Button *create_effector_button = nullptr;
	Button *joints_remove_effector_button = nullptr;
	Button *effectors_remove_effector_button = nullptr;
	Tree *effector_tree = nullptr;
	EffectorTargetTransformEditor *effector_target_transform_editor = nullptr;
	HBoxContainer *effector_target_node_editor = nullptr;
	Label *target_node_label = nullptr;
	Button *assign = nullptr;
	Button *clear = nullptr;
	SceneTreeDialog *scene_tree = nullptr;
	NodePath base_hint = NodePath();
	String property = "";
	EditorFileDialog *file_dialog = nullptr;
	UndoRedo *undo_redo = nullptr;
	EditorFileDialog *file_export_lib = nullptr;
	void _multi_constraint_file_selected(const String &p_file);
	void update_joint_tree();
	void update_effector_tree();
	void update_editors();
	void create_editors();

protected:
	void _notification(int p_what);
	void _node_removed(Node *p_node);
	static void _bind_methods();

public:
	void _update_effector_node_node_assign();
	void _update_effector_node(String p_path);
	void _update_effector_target_transform(int32_t p_effector_index, Transform p_value);
	void _create_effector_button_pressed();
	void _effectors_remove_effector_button_pressed();
	void _joints_remove_effector_button_pressed();
	void set_target(const String &p_prop);
	Skeleton *get_skeleton() const;
	const Vector<int> &get_process_order();
	void _joint_tree_selection_changed();
	void _joint_tree_rmb_select(const Vector2 &p_pos);
	void _effector_tree_selection_changed();
	void _update_properties();
	SkeletonIKMultiConstraintEditor(EditorInspectorPluginSkeleton *e_plugin, EditorNode *p_editor, Skeleton *skeleton);
	~SkeletonIKMultiConstraintEditor();
};

#endif
