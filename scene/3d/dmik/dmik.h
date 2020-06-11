/*************************************************************************/
/*  cmdd_inverse_kinematics.h                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
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

#ifndef GODOT_DMIK_H
#define GODOT_DMIK_H

#include "core/class_db.h"
#include "core/map.h"
#include "core/math/math_funcs.h"
#include "core/os/memory.h"
#include "core/reference.h"
#include "core/rid.h"
#include "ik_axes.h"
#include "direction_constraint.h"
#include "qcp.h"
#include "scene/3d/physics_body.h"
#include "multi_constraint.h"
#include "scene/3d/physics_body.h"

class Axes : public Reference {
public:
	Transform local_transform;
	Transform global_transform;
	bool dirty = false;
	Ref<Axes> parent;
	Vector<Ref<Axes> > children;
	void mark_dirty() {
		dirty = true;
		for (int32_t i = 0; i < children.size(); i++) {
			children.write[i]->mark_dirty();
		}
	}
	bool is_dirty() const {
		return dirty;
	}
	void update_global_transform() {
		if (parent.is_valid()) {
			global_transform = parent->get_global_transform() * local_transform;
		} else {
			global_transform = local_transform;
		}
	}
	Transform get_global_transform() {
		if (is_dirty()) {
			update_global_transform();
		}
		return global_transform;
	}
};

class BoneChain;
class PhysicalBone;
class BoneChainItem : public Reference {

	GDCLASS(BoneChainItem, Reference);

public:
	Ref<BoneChain> parent_armature = nullptr;
	Vector<Ref<BoneChainItem> > children;
	Ref<BoneChainItem> parent_item = nullptr;
	// Bone info
	int bone = -1;
	PhysicalBone *pb = nullptr;
	bool springy = false;
	float cos_half_dampen = 0.0f;
	Vector<real_t> cos_half_returnful_dampened;
	Vector<real_t> half_returnful_dampened;
	bool ik_orientation_lock = false;
	float stiffness_scalar = 0.0f;
	float bone_height = 0.0f;
	float length = 0.0f;

	Ref<Axes> axes = nullptr;

	Ref<KusudamaConstraint> constraint = nullptr;

	BoneChainItem() { axes.instance(); }

	Transform get_global_transform() const {
		Transform xform;
		Ref<BoneChainItem> item = parent_item;
		while (item.is_valid()) {
			xform = item->axes->local_transform * xform;
			item = item->parent_item;
		}
		return xform;
	}

	float get_bone_height() const;

	void set_bone_height(const float p_bone_height);

	Ref<BoneChainItem> find_child(const int p_bone_id);

	Ref<BoneChainItem> add_child(const int p_bone_id);

	void set_stiffness(float p_stiffness);

	float get_stiffness() const;

	void update_cos_dampening();

	void
	set_axes_to_returned(IKAxes p_global, IKAxes p_to_set, IKAxes p_limiting_axes, float p_cos_half_angle_dampen,
			float p_angle_dampen);

	void set_axes_to_be_snapped(IKAxes p_to_set, IKAxes p_limiting_axes, float p_cos_half_angle_dampen);

	void populate_return_dampening_iteration_array(Ref<KusudamaConstraint> k);
	void rootwardly_update_falloff_cache_from(Ref<BoneChainItem> p_current);
};

class BoneChainTarget;
class MultiConstraint;
class BoneChain : public Reference {
	GDCLASS(BoneChain, Reference);

public:
	Ref<BoneChainItem> chain_root = memnew(BoneChainItem);
	Ref<BoneChainItem> middle_chain_item = nullptr;
	Vector<Ref<BoneChainTarget> > targets;
	Vector3 magnet_position;
	Vector<Vector3> localized_target_headings;
	Vector<Vector3> localized_effector_headings;
	Vector<real_t> weights;
	Ref<MultiConstraint> constraints = nullptr;
	float dampening = Math::deg2rad(5.0f);
	Map<int, Ref<BoneChainItem> > bone_segment_map;
	int ik_iterations = 15;

	int get_default_iterations() const;
	void recursively_create_penalty_array(Ref<BoneChain> from, Vector<Vector<real_t> > &r_weight_array, Vector<Ref<BoneChainItem> > pin_sequence, float current_falloff);
	void create_headings_arrays();
};

class BoneEndEffector : public Reference {
	GDCLASS(BoneEndEffector, Reference);

public:
	int effector_bone;
	Transform goal_transform;
};
class BoneChainTarget : public Reference {
	GDCLASS(BoneChainTarget, Reference);

public:
	Ref<BoneChainItem> chain_item = nullptr;
	Ref<BoneEndEffector> end_effector = nullptr;

	BoneChainTarget() :
			chain_item(NULL),
			end_effector(NULL) {}

	BoneChainTarget(Ref<BoneChainItem> p_chain_item, const Ref<BoneEndEffector> p_end_effector) :
			chain_item(p_chain_item),
			end_effector(p_end_effector) {}

	BoneChainTarget(const Ref<BoneChainTarget> p_other_ct) :
			chain_item(p_other_ct->chain_item),
			end_effector(p_other_ct->end_effector) {}

	BoneChainTarget(Ref<BoneChainItem> p_chain_item, const Ref<BoneEndEffector> p_end_effector, bool p_enabled) {
		enabled = p_enabled;
		set_target_priorities(x_priority, y_priority, z_priority);
	}

protected:
	bool enabled;
	BoneChainTarget *parent_target = nullptr;
	Vector<BoneChainTarget *> child_targets;
	float target_weight = 1;
	uint8_t mode_code = 7;
	int sub_target_count = 4;
	float x_priority = 1.0f, y_priority = 1.0f, z_priority = 1.0f;
	float depthFalloff = 0.0f;

public:
	static const short XDir = 1, YDir = 2, ZDir = 4;

	bool is_enabled() const;

	void toggle();

	void enable();

	void disable();

	/**
     * Targets can be ultimate targets, or intermediary targets.
     * By default, each target is treated as an ultimate target, meaning
     * any bones which are ancestors to that target's end-effector
     * are not aware of any target wich are target of bones descending from that end effector.
     *
     * Changing this value makes ancestor bones aware, and also determines how much less
     * they care with each level down.
     *
     * Presuming all descendants of this target have a falloff of 1, then:
     * A target falloff of 0 on this target means only this target is reported to ancestors.
     * A target falloff of 1 on this target means ancestors care about all descendant target equally (after accounting for their pinWeight),
     * regardless of how many levels down they are.
     * A target falloff of 0.5 means each descendant target is cared about half as much as its ancestor.
     *
     * With each level, the target falloff of a descendant is taken account for each level.
     *  Meaning, if this target has a falloff of 1, and its descendent has a falloff of 0.5
     *  then this target will be reported with full weight,
     *  it descendant will be reported with full weight,
     *  the descendant of that target will be reported with half weight.
     *  the desecendant of that one's descendant will be reported with quarter weight.
     *
     * @param p_depth
     */
	void set_depth_falloff(float p_depth);

	float get_depth_falloff() const;

	/**
     * Sets  the priority of the orientation bases which effectors reaching for this target will and won't align with.
     * If all are set to 0, then the target is treated as a simple position target.
     * It's usually better to set at least on of these three values to 0, as giving a nonzero value to all three is most often redundant.
     *
     *  This values this function sets are only considered by the orientation aware solver.
     *
     * @param position
     * @param p_x_priority set to a positive value (recommended between 0 and 1) if you want the bone's x basis to point in the same direction as this target's x basis (by this library's convention the x basis corresponds to a limb's twist)
     * @param p_y_priority set to a positive value (recommended between 0 and 1)  if you want the bone's y basis to point in the same direction as this target's y basis (by this library's convention the y basis corresponds to a limb's direction)
     * @param p_z_priority set to a positive value (recommended between 0 and 1)  if you want the bone's z basis to point in the same direction as this target's z basis (by this library's convention the z basis corresponds to a limb's twist)
     */
	void set_target_priorities(float p_x_priority, float p_y_priority, float p_z_priority);

	/**
     * @return the number of bases an effector to this target will attempt to align on.
     */
	int get_subtarget_count();

	uint8_t get_mode_code() const;

	/**
     * @return the priority of this target's x axis;
     */
	float get_x_priority() const;

	/**
     * @return the priority of this target's y axis;
     */
	float get_y_priority() const;

	/**
     * @return the priority of this target's z axis;
     */
	float get_z_priority() const;

	IKAxes get_axes() const;

	/**
     * translates and rotates the target to match the position
     * and orientation of the input Axes. The orientation
     * is only relevant for orientation aware solvers.
     * @param inAxes
     */
	void align_to_axes(IKAxes inAxes);

	/**
     * translates the pin to the location specified in global coordinates
     * @param location
     */
	void translate_global(Vector3 location);

	/**
     * translates the pin to the location specified in local coordinates
     * (relative to any other Axes objects the pin may be parented to)
     * @param location
     */
	void translate(Vector3 location);

	/**
     * @return the target location in global coordinates
     */
	Vector3 get_location();

	Ref<BoneChainItem> for_bone();

	/**
     * called when this target is being removed entirely from the Armature. (as opposed to just being disabled)
     */
	void removal_notification();

	void set_parent_target(BoneChainTarget *parent);

	void remove_child_target(BoneChainTarget *child);

	void add_child_target(BoneChainTarget *new_child);

	BoneChainTarget *get_parent_target();

	bool is_ancestor_of(BoneChainTarget *potential_descendent);

	float get_target_weight();
};

class Skeleton;
struct DMIKTask {
	Skeleton *skeleton = nullptr;

	Ref<BoneChain> chain = memnew(BoneChain);

	// Settings
	float min_distance = 0.01f;
	int iterations = 4;
	int max_iterations = 1.0f;
	// dampening dampening angle in radians.
	// Set this to -1 if you want to use the armature's default.
	float dampening = 0.05f;
	// stabilizing_passes number of stabilization passes to run.
	// Set this to -1 if you want to use the armature's default.
	int stabilizing_passes = -1;

	// Bone data
	int root_bone = -1;
	Vector<Ref<BoneEndEffector> > end_effectors;
};

class DMIK : public Reference {
private:
	/**
     * The default maximum number of radians a bone is allowed to rotate per solver iteration.
     * The lower this value, the more natural the pose results. However, this will  the number of iterations
     * the solver requires to converge.
     *
     * !!THIS IS AN EXPENSIVE OPERATION.
     * This updates the entire armature's cache of precomputed quadrance angles.
     * The cache makes things faster in general, but if you need to dynamically change the dampening during a call to IKSolver, use
     * the IKSolver(bone, dampening, iterations, stabilizationPasses) function, which clamps rotations on the fly.
     * @param damp
     */
	static void set_default_dampening(Ref<BoneChain> r_chain, float p_damp);

	static void update_armature_segments(Ref<BoneChain> r_chain);

	static void update_optimal_rotation_to_target_descendants(
			Ref<BoneChainItem> p_chain_item,
			float p_dampening,
			bool p_is_translate,
			Vector<Vector3> p_localized_tip_headings,
			Vector<Vector3> p_localized_target_headings,
			Vector<real_t> p_weights,
			Ref<QCP> p_qcp_orientation_aligner,
			int p_iteration,
			float p_total_iterations);

	static void recursively_update_bone_segment_map_from(Ref<BoneChain> r_chain, Ref<BoneChainItem> p_start_from);

	static void QCPSolver(
			Ref<BoneChain> p_chain,
			float p_dampening,
			bool p_inverse_weighting,
			int p_stabilization_passes,
			int p_iteration,
			float p_total_iterations);

	static bool build_chain(DMIKTask *p_task);

	static void update_chain(const Skeleton *p_sk, Ref<BoneChainItem> p_chain_item);

	static void solve_simple(DMIKTask *p_task, bool p_solve_magnet);

public:
	static const int32_t x_axis = 0;
	static const int32_t y_axis = 1;
	static const int32_t z_axis = 2;

	/**
     *
     * @param for_bone
     * @param dampening
     * @param translate set to true if you wish to allow translation in addition to rotation of the bone (should only be used for unpinned root bones)
     * @param stabilization_passes If you know that your armature isn't likely to succumb to instability in unsolvable configurations, leave this value set to 0.
     * If you value stability in extreme situations more than computational speed, then increase this value. A value of 1 will be completely stable, and just as fast
     * as a value of 0, however, it might result in small levels of robotic looking jerk. The higher the value, the less jerk there will be (but at potentially significant computation cost).
     */
	static void update_optimal_rotation_to_target_descendants(
			Ref<BoneChain> r_chain,
			Ref<BoneChainItem> p_for_bone,
			float p_dampening,
			bool p_translate,
			int p_stabilization_passes,
			int p_iteration,
			int p_total_iterations);

	static float
	get_manual_msd(Vector<Vector3> &r_localized_effector_headings, Vector<Vector3> &r_localized_target_headings,
			const Vector<real_t> &p_weights);

	static void update_target_headings(Ref<BoneChain> r_chain, Vector<Vector3> &r_localized_target_headings,
			Vector<real_t> p_weights, Transform p_bone_xform);

	static void update_effector_headings(Ref<BoneChain> r_chain, Vector<Vector3> &r_localized_effector_headings,
			Transform p_bone_xform);

	static DMIKTask * create_simple_task(Skeleton *p_sk, const Transform &goal_transform,
                                         float p_dampening = -1, int p_stabilizing_passes = -1,
                                         Ref<MultiConstraint> p_constraints = NULL);

	static void free_task(DMIKTask *p_task);

	static void make_goal(DMIKTask *p_task, const Transform &p_inverse_transf, float blending_delta);

	static void solve(DMIKTask *p_task, float blending_delta, bool override_effector_basis, bool p_use_magnet,
                      const Vector3 &p_magnet_position);
};

#include "kusudama_constraint.h"

#endif //GODOT_DMIK_H
