/*************************************************************************/
/*  effector_editor.h													 */
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

#ifndef SKELETON_IK_CMDD_EDITOR_PLUGIN_H
#define SKELETON_IK_CMDD_EDITOR_PLUGIN_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"
#include "editor/editor_properties.h"
#include "scene/3d/skeleton.h"

class EffectorTargetTransformEditor : public VBoxContainer {
	GDCLASS(EffectorTargetTransformEditor, VBoxContainer);

	static const int32_t TRANSLATION_COMPONENTS = 3;
	static const int32_t ROTATION_DEGREES_COMPONENTS = 3;
	static const int32_t SCALE_COMPONENTS = 3;
	static const int32_t BASIS_COMPONENTS = 9;
	static const int32_t BASIS_SPLIT_COMPONENTS = 3;
	static const int32_t TRANSFORM_COMPONENTS = 12;
	static const int32_t TRANSFORM_SPLIT_COMPONENTS = 3;
	static const int32_t TRANSFORM_CONTROL_COMPONENTS = 3;

	EditorInspectorSection *section = nullptr;

	GridContainer *translation_grid = nullptr;
	GridContainer *rotation_grid = nullptr;
	GridContainer *scale_grid = nullptr;
	GridContainer *transform_grid = nullptr;

	EditorSpinSlider *translation_slider[TRANSLATION_COMPONENTS];
	EditorSpinSlider *rotation_slider[ROTATION_DEGREES_COMPONENTS];
	EditorSpinSlider *scale_slider[SCALE_COMPONENTS];
	EditorSpinSlider *transform_slider[TRANSFORM_COMPONENTS];

	Rect2 background_rects[5];

	Skeleton *ik = nullptr;
	String property = "";

	UndoRedo *undo_redo = nullptr;

	String label = "";

	void create_editors();
	void setup_spinner(EditorSpinSlider *spinner, const bool is_transform_spinner);

	void _value_changed(const double p_value, const bool p_from_transform);

	Transform compute_transform(const bool p_from_transform) const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	EffectorTargetTransformEditor(Skeleton *p_ik);

	// Which transform target to modify
	void set_target(const String &p_prop);
	void set_label(const String &p_label);

	void _update_properties();
	void _update_target_transform_properties();
	void _update_transform_properties(Transform p_transform);
};

#endif // SKELETON_IK_CMDD_EDITOR_PLUGIN_H
