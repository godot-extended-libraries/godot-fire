/*************************************************************************/
/*  bone_attachment.cpp                                                  */
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

#include "bone_attachment.h"

void BoneAttachment::_validate_property(PropertyInfo &property) const {
	if (property.name == "bone_name") {
		Skeleton *parent = _get_Skeleton();

		if (parent) {
			String names;
			for (int i = 0; i < parent->get_bone_count(); i++) {
				if (i > 0) {
					names += ",";
				}
				names += parent->get_bone_name(i);
			}

			property.hint = PROPERTY_HINT_ENUM;
			property.hint_string = names;
		} else {
			property.hint = PROPERTY_HINT_NONE;
			property.hint_string = "";
		}
	}
}

bool BoneAttachment::_set(const StringName &p_path, const Variant &p_value) {
	if (p_path == "override_pose") {
		set_override_pose(p_value);
	} else if (p_path == "override_mode") {
		set_override_mode(p_value);
	} else if (p_path == "use_external_skeleton") {
		set_use_external_skeleton(p_value);
	} else if (p_path == "external_skeleton") {
		set_external_skeleton(p_value);
	}

	return true;
}

bool BoneAttachment::_get(const StringName &p_path, Variant &r_ret) const {
	if (p_path == "override_pose") {
		r_ret = get_override_pose();
	} else if (p_path == "override_mode") {
		r_ret = get_override_mode();
	} else if (p_path == "use_external_skeleton") {
		r_ret = get_use_external_skeleton();
	} else if (p_path == "external_skeleton") {
		r_ret = get_external_skeleton();
	}

	return true;
}

void BoneAttachment::_get_property_list(List<PropertyInfo> *p_list) const {
	p_list->push_back(PropertyInfo(Variant::BOOL, "override_pose", PROPERTY_HINT_NONE, ""));
	if (override_pose) {
		p_list->push_back(PropertyInfo(Variant::INT, "override_mode", PROPERTY_HINT_ENUM, "Global Pose Override, Local Pose Override, Custom Pose"));
	}

	p_list->push_back(PropertyInfo(Variant::BOOL, "use_external_skeleton", PROPERTY_HINT_NONE, ""));
	if (use_external_skeleton) {
		p_list->push_back(PropertyInfo(Variant::NODE_PATH, "external_skeleton", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Skeleton"));
	}
}

void BoneAttachment::_update_external_skeleton_cache() {
	external_skeleton_node_cache = ObjectID();
	if (has_node(external_skeleton_node)) {
		Node *node = get_node(external_skeleton_node);
		ERR_FAIL_COND_MSG(!node, "Cannot update external skeleton cache: Node cannot be found!");

		// Make sure it's a Skeleton
		Skeleton *sk = Object::cast_to<Skeleton>(node);
		ERR_FAIL_COND_MSG(!sk, "Cannot update external skeleton cache: Nodepath does not point to a Skeleton node!");

		external_skeleton_node_cache = node->get_instance_id();
	}
}

void BoneAttachment::_check_bind() {
	Skeleton *sk = _get_Skeleton();

	if (sk && !bound) {
		if (bone_idx <= -1) {
			bone_idx = sk->find_bone(bone_name);
		}
		if (bone_idx != -1) {
			sk->connect("bone_pose_changed", this, "on_bone_pose_override");
			if (use_external_skeleton) {
				call_deferred("set_global_transform", sk->global_pose_to_world_transform(sk->get_bone_global_pose(bone_idx)));
			} else {
				call_deferred("set_transform", sk->get_bone_global_pose(bone_idx));
			}

			bound = true;
		}
	}
}

Skeleton *BoneAttachment::_get_Skeleton() const {
	if (use_external_skeleton) {
		if (external_skeleton_node_cache) {
			return Object::cast_to<Skeleton>(ObjectDB::get_instance(external_skeleton_node_cache));
		}
	} else {
		return Object::cast_to<Skeleton>(get_parent());
	}
	return nullptr;
}

void BoneAttachment::_check_unbind() {
	if (bound) {
		Skeleton *sk = _get_Skeleton();

		if (sk) {
			sk->disconnect("bone_pose_changed", this, "on_bone_pose_override");
		}
		bound = false;
	}
}

void BoneAttachment::_transform_changed() {
	if (override_pose) {
		Skeleton *sk = _get_Skeleton();

		ERR_FAIL_COND_MSG(!sk, "Cannot override pose: Skeleton not found!");
		ERR_FAIL_INDEX_MSG(bone_idx, sk->get_bone_count(), "Cannot override pose: Bone index is out of range!");

		Transform our_trans = get_transform();
		if (use_external_skeleton) {
			our_trans = sk->world_transform_to_global_pose(get_global_transform());
		}

		if (override_mode == OVERRIDE_MODES::MODE_GLOBAL_POSE) {
			sk->set_bone_global_pose_override(bone_idx, our_trans, 1.0, true);
		} else if (override_mode == OVERRIDE_MODES::MODE_LOCAL_POSE) {
			sk->set_bone_local_pose_override(bone_idx, sk->global_pose_to_local_pose(bone_idx, our_trans), 1.0, true);
		} else if (override_mode == OVERRIDE_MODES::MODE_CUSTOM_POSE) {
			sk->set_bone_custom_pose(bone_idx, sk->global_pose_to_local_pose(bone_idx, our_trans));
		}
	}
}

void BoneAttachment::set_bone_name(const String &p_name) {
	bone_name = p_name;
	Skeleton *sk = _get_Skeleton();
	if (sk) {
		set_bone_idx(sk->find_bone(bone_name));
	}
}

String BoneAttachment::get_bone_name() const {
	return bone_name;
}

void BoneAttachment::set_bone_idx(const int &p_idx) {
	if (is_inside_tree()) {
		_check_unbind();
	}

	bone_idx = p_idx;

	Skeleton *sk = _get_Skeleton();
	if (sk) {
		if (bone_idx <= -1 || bone_idx >= sk->get_bone_count()) {
			WARN_PRINT("Bone index out of range! Cannot connect BoneAttachment to node!");
			bone_idx = -1;
		} else {
			bone_name = sk->get_bone_name(bone_idx);
		}
	}

	if (is_inside_tree()) {
		_check_bind();
	}

	_change_notify();
}

int BoneAttachment::get_bone_idx() const {
	return bone_idx;
}

void BoneAttachment::set_override_pose(bool p_override) {
	override_pose = p_override;
	set_notify_local_transform(override_pose);

	if (!override_pose) {
		Skeleton *sk = _get_Skeleton();
		if (sk) {
			if (override_mode == OVERRIDE_MODES::MODE_GLOBAL_POSE) {
				sk->set_bone_global_pose_override(bone_idx, Transform(), 0.0, false);
			} else if (override_mode == OVERRIDE_MODES::MODE_LOCAL_POSE) {
				sk->set_bone_local_pose_override(bone_idx, Transform(), 0.0, false);
			} else if (override_mode == OVERRIDE_MODES::MODE_CUSTOM_POSE) {
				sk->set_bone_custom_pose(bone_idx, Transform());
			}
		}
		_transform_changed();
	}
	_change_notify();
}

bool BoneAttachment::get_override_pose() const {
	return override_pose;
}

void BoneAttachment::set_override_mode(int p_mode) {
	if (override_pose) {
		Skeleton *sk = _get_Skeleton();
		if (sk) {
			if (override_mode == OVERRIDE_MODES::MODE_GLOBAL_POSE) {
				sk->set_bone_global_pose_override(bone_idx, Transform(), 0.0, false);
			} else if (override_mode == OVERRIDE_MODES::MODE_LOCAL_POSE) {
				sk->set_bone_local_pose_override(bone_idx, Transform(), 0.0, false);
			} else if (override_mode == OVERRIDE_MODES::MODE_CUSTOM_POSE) {
				sk->set_bone_custom_pose(bone_idx, Transform());
			}
		}
		override_mode = p_mode;
		_transform_changed();
		return;
	}
	override_mode = p_mode;
}

int BoneAttachment::get_override_mode() const {
	return override_mode;
}

void BoneAttachment::set_use_external_skeleton(bool p_use_external) {
	use_external_skeleton = p_use_external;
	_change_notify();
}

bool BoneAttachment::get_use_external_skeleton() const {
	return use_external_skeleton;
}

void BoneAttachment::set_external_skeleton(NodePath p_path) {
	external_skeleton_node = p_path;
	_update_external_skeleton_cache();
}

NodePath BoneAttachment::get_external_skeleton() const {
	return external_skeleton_node;
}

void BoneAttachment::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (use_external_skeleton) {
				_update_external_skeleton_cache();
			}
			_check_bind();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_check_unbind();
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			_transform_changed();
		} break;
	}
}

void BoneAttachment::on_bone_pose_update(int p_bone_index) {
	if (bone_idx == p_bone_index) {
		Skeleton *sk = _get_Skeleton();
		if (sk) {
			if (!override_pose) {
				if (use_external_skeleton) {
					set_global_transform(sk->global_pose_to_world_transform(sk->get_bone_global_pose(bone_idx)));
				} else {
					set_transform(sk->get_bone_global_pose(bone_idx));
				}
			} else {
				set_notify_local_transform(false);
				if (override_mode == OVERRIDE_MODES::MODE_LOCAL_POSE) {
					if (use_external_skeleton) {
						set_global_transform(sk->global_pose_to_world_transform(sk->local_pose_to_global_pose(bone_idx, sk->get_bone_local_pose_override(bone_idx))));
					} else {
						set_transform(sk->local_pose_to_global_pose(bone_idx, sk->get_bone_local_pose_override(bone_idx)));
					}
				} else if (override_mode == OVERRIDE_MODES::MODE_CUSTOM_POSE) {
					if (use_external_skeleton) {
						set_global_transform(sk->global_pose_to_world_transform(sk->local_pose_to_global_pose(bone_idx, sk->get_bone_custom_pose(bone_idx))));
					} else {
						set_transform(sk->local_pose_to_global_pose(bone_idx, sk->get_bone_custom_pose(bone_idx)));
					}
				}

				set_notify_local_transform(true);
			}
		}
	}
}

BoneAttachment::BoneAttachment() {
	bound = false;
}

void BoneAttachment::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_bone_name", "bone_name"), &BoneAttachment::set_bone_name);
	ClassDB::bind_method(D_METHOD("get_bone_name"), &BoneAttachment::get_bone_name);

	ClassDB::bind_method(D_METHOD("set_bone_idx", "bone_idx"), &BoneAttachment::set_bone_idx);
	ClassDB::bind_method(D_METHOD("get_bone_idx"), &BoneAttachment::get_bone_idx);

	ClassDB::bind_method(D_METHOD("on_bone_pose_update"), &BoneAttachment::on_bone_pose_update);

	ClassDB::bind_method(D_METHOD("set_override_pose", "override_pose"), &BoneAttachment::set_override_pose);
	ClassDB::bind_method(D_METHOD("get_override_pose"), &BoneAttachment::get_override_pose);
	ClassDB::bind_method(D_METHOD("set_override_mode", "override_mode"), &BoneAttachment::set_override_mode);
	ClassDB::bind_method(D_METHOD("get_override_mode"), &BoneAttachment::get_override_mode);

	ClassDB::bind_method(D_METHOD("set_use_external_skeleton", "use_external_skeleton"), &BoneAttachment::set_use_external_skeleton);
	ClassDB::bind_method(D_METHOD("get_use_external_skeleton"), &BoneAttachment::get_use_external_skeleton);
	ClassDB::bind_method(D_METHOD("set_external_skeleton", "external_skeleton"), &BoneAttachment::set_external_skeleton);
	ClassDB::bind_method(D_METHOD("get_external_skeleton"), &BoneAttachment::get_external_skeleton);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "bone_name"), "set_bone_name", "get_bone_name");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bone_idx"), "set_bone_idx", "get_bone_idx");
}
