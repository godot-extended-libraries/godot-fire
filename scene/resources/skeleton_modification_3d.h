/*************************************************************************/
/*  skeleton_modification_3d.h                                           */
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

#ifndef SKELETONMODIFICATION3D_H
#define SKELETONMODIFICATION3D_H

#include "scene/3d/skeleton_3d.h"

///////////////////////////////////////
// SkeletonModificationStack3D
///////////////////////////////////////

class Skeleton3D;
class SkeletonModification3D;

class SkeletonModificationStack3D : public Resource {
	GDCLASS(SkeletonModificationStack3D, Resource);
	friend class Skeleton3D;
	friend class SkeletonModification3D;

protected:
	static void _bind_methods();
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool _set(const StringName &p_path, const Variant &p_value);
	bool _get(const StringName &p_path, Variant &r_ret) const;

public:
	Skeleton3D *skeleton;
	bool is_setup = false;
	bool enabled = true;
	float strength = 1.0;

	enum EXECUTION_MODE {
		execution_mode_process,
		execution_mode_physics_process,
	};

	Vector<Ref<SkeletonModification3D>> modifications;
	int modifications_count = 0;

	void setup();
	void execute(float delta, int execution_mode);

	void enable_all_modifications(bool p_enable);
	Ref<SkeletonModification3D> get_modification(int p_mod_idx) const;
	void add_modification(Ref<SkeletonModification3D> p_mod);
	void delete_modification(int p_mod_idx);
	void set_modification(int p_mod_idx, Ref<SkeletonModification3D> p_mod);

	void set_modification_count(int p_count);
	int get_modification_count() const;

	void set_skeleton(Skeleton3D *p_skeleton);
	Skeleton3D *get_skeleton() const;

	bool get_is_setup() const;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	void set_strength(float p_strength);
	float get_strength() const;

	SkeletonModificationStack3D();
};

///////////////////////////////////////
// SkeletonModification3D
///////////////////////////////////////

class SkeletonModification3D : public Resource {
	GDCLASS(SkeletonModification3D, Resource);
	friend class Skeleton3D;

protected:
	static void _bind_methods();

	SkeletonModificationStack3D *stack;
	int execution_mode = SkeletonModificationStack3D::EXECUTION_MODE::execution_mode_process;

	bool enabled = true;
	bool is_setup = false;
	bool execution_error_found = false;

	bool _print_execution_error(bool p_condition, String p_message);

public:
	virtual void execute(float delta);
	virtual void setup_modification(SkeletonModificationStack3D *p_stack);

	float clamp_angle(float angle, float min_bound, float max_bound, bool invert);

	void set_enabled(bool p_enabled);
	bool get_enabled();

	void set_execution_mode(int p_mode);
	int get_execution_mode() const;

	Ref<SkeletonModificationStack3D> get_modification_stack();

	void set_is_setup(bool p_setup);
	bool get_is_setup() const;

	SkeletonModification3D();
};

///////////////////////////////////////
// SkeletonModification3DStackHolder
///////////////////////////////////////

class SkeletonModification3DStackHolder : public SkeletonModification3D {
	GDCLASS(SkeletonModification3DStackHolder, SkeletonModification3D);

protected:
	static void _bind_methods();
	bool _get(const StringName &p_path, Variant &r_ret) const;
	bool _set(const StringName &p_path, const Variant &p_value);
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	Ref<SkeletonModificationStack3D> held_modification_stack;

	void execute(float delta) override;
	void setup_modification(SkeletonModificationStack3D *p_stack) override;

	void set_held_modification_stack(Ref<SkeletonModificationStack3D> p_held_stack);
	Ref<SkeletonModificationStack3D> get_held_modification_stack() const;

	SkeletonModification3DStackHolder();
	~SkeletonModification3DStackHolder();
};

#endif // SKELETONMODIFICATION3D_H
