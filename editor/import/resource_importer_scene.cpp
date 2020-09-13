/*************************************************************************/
/*  resource_importer_scene.cpp                                          */
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

#include "resource_importer_scene.h"

#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/navigation_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/3d/vehicle_body_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/animation.h"
#include "scene/resources/box_shape_3d.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/ray_shape_3d.h"
#include "scene/resources/resource_format_text.h"
#include "scene/resources/sphere_shape_3d.h"
#include "scene/resources/world_margin_shape_3d.h"

uint32_t EditorSceneImporter::get_import_flags() const {
	if (get_script_instance()) {
		return get_script_instance()->call("_get_import_flags");
	}

	ERR_FAIL_V(0);
}

void EditorSceneImporter::get_extensions(List<String> *r_extensions) const {
	if (get_script_instance()) {
		Array arr = get_script_instance()->call("_get_extensions");
		for (int i = 0; i < arr.size(); i++) {
			r_extensions->push_back(arr[i]);
		}
		return;
	}

	ERR_FAIL();
}

Node *EditorSceneImporter::import_scene(const String &p_path, uint32_t p_flags, int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	if (get_script_instance()) {
		return get_script_instance()->call("_import_scene", p_path, p_flags, p_bake_fps);
	}

	ERR_FAIL_V(nullptr);
}

Ref<Animation> EditorSceneImporter::import_animation(const String &p_path, uint32_t p_flags, int p_bake_fps) {
	if (get_script_instance()) {
		return get_script_instance()->call("_import_animation", p_path, p_flags);
	}

	ERR_FAIL_V(nullptr);
}

//for documenters, these functions are useful when an importer calls an external conversion helper (like, fbx2gltf),
//and you want to load the resulting file

Node *EditorSceneImporter::import_scene_from_other_importer(const String &p_path, uint32_t p_flags, int p_bake_fps) {
	return ResourceImporterScene::get_singleton()->import_scene_from_other_importer(this, p_path, p_flags, p_bake_fps);
}

Ref<Animation> EditorSceneImporter::import_animation_from_other_importer(const String &p_path, uint32_t p_flags, int p_bake_fps) {
	return ResourceImporterScene::get_singleton()->import_animation_from_other_importer(this, p_path, p_flags, p_bake_fps);
}

void EditorSceneImporter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("import_scene_from_other_importer", "path", "flags", "bake_fps"), &EditorSceneImporter::import_scene_from_other_importer);
	ClassDB::bind_method(D_METHOD("import_animation_from_other_importer", "path", "flags", "bake_fps"), &EditorSceneImporter::import_animation_from_other_importer);

	BIND_VMETHOD(MethodInfo(Variant::INT, "_get_import_flags"));
	BIND_VMETHOD(MethodInfo(Variant::ARRAY, "_get_extensions"));

	MethodInfo mi = MethodInfo(Variant::OBJECT, "_import_scene", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::INT, "flags"), PropertyInfo(Variant::INT, "bake_fps"));
	mi.return_val.class_name = "Node";
	BIND_VMETHOD(mi);
	mi = MethodInfo(Variant::OBJECT, "_import_animation", PropertyInfo(Variant::STRING, "path"), PropertyInfo(Variant::INT, "flags"), PropertyInfo(Variant::INT, "bake_fps"));
	mi.return_val.class_name = "Animation";
	BIND_VMETHOD(mi);

	BIND_CONSTANT(IMPORT_SCENE);
	BIND_CONSTANT(IMPORT_ANIMATION);
	BIND_CONSTANT(IMPORT_ANIMATION_DETECT_LOOP);
	BIND_CONSTANT(IMPORT_ANIMATION_OPTIMIZE);
	BIND_CONSTANT(IMPORT_ANIMATION_FORCE_ALL_TRACKS_IN_ALL_CLIPS);
	BIND_CONSTANT(IMPORT_ANIMATION_KEEP_VALUE_TRACKS);
	BIND_CONSTANT(IMPORT_GENERATE_TANGENT_ARRAYS);
	BIND_CONSTANT(IMPORT_FAIL_ON_MISSING_DEPENDENCIES);
	BIND_CONSTANT(IMPORT_MATERIALS_IN_INSTANCES);
	BIND_CONSTANT(IMPORT_USE_COMPRESSION);
}

/////////////////////////////////
void EditorScenePostImport::_bind_methods() {
	BIND_VMETHOD(MethodInfo(Variant::OBJECT, "post_import", PropertyInfo(Variant::OBJECT, "scene")));
	ClassDB::bind_method(D_METHOD("get_source_folder"), &EditorScenePostImport::get_source_folder);
	ClassDB::bind_method(D_METHOD("get_source_file"), &EditorScenePostImport::get_source_file);
}

Node *EditorScenePostImport::post_import(Node *p_scene) {
	if (get_script_instance()) {
		return get_script_instance()->call("post_import", p_scene);
	}

	return p_scene;
}

String EditorScenePostImport::get_source_folder() const {
	return source_folder;
}

String EditorScenePostImport::get_source_file() const {
	return source_file;
}

void EditorScenePostImport::init(const String &p_source_folder, const String &p_source_file) {
	source_folder = p_source_folder;
	source_file = p_source_file;
}

EditorScenePostImport::EditorScenePostImport() {
}

String ResourceImporterScene::get_importer_name() const {
	return "scene";
}

String ResourceImporterScene::get_visible_name() const {
	return "Scene";
}

void ResourceImporterScene::get_recognized_extensions(List<String> *p_extensions) const {
	for (Set<Ref<EditorSceneImporter>>::Element *E = importers.front(); E; E = E->next()) {
		E->get()->get_extensions(p_extensions);
	}
}

String ResourceImporterScene::get_save_extension() const {
	return "scn";
}

String ResourceImporterScene::get_resource_type() const {
	return "PackedScene";
}

bool ResourceImporterScene::get_option_visibility(const String &p_option, const Map<StringName, Variant> &p_options) const {
	if (p_option.begins_with("animation/")) {
		if (p_option != "animation/import" && !bool(p_options["animation/import"])) {
			return false;
		}

		if (p_option == "animation/keep_custom_tracks" && int(p_options["animation/storage"]) == 0) {
			return false;
		}

		if (p_option.begins_with("animation/optimizer/") && p_option != "animation/optimizer/enabled" && !bool(p_options["animation/optimizer/enabled"])) {
			return false;
		}

		if (p_option.begins_with("animation/clip_")) {
			int max_clip = p_options["animation/clips/amount"];
			int clip = p_option.get_slice("/", 1).get_slice("_", 1).to_int() - 1;
			if (clip >= max_clip) {
				return false;
			}
		}
	}

	if (p_option == "materials/keep_on_reimport" && int(p_options["materials/storage"]) == 0) {
		return false;
	}

	if (p_option == "meshes/lightmap_texel_size" && int(p_options["meshes/light_baking"]) < 2) {
		return false;
	}

	return true;
}

int ResourceImporterScene::get_preset_count() const {
	return PRESET_MAX;
}

String ResourceImporterScene::get_preset_name(int p_idx) const {
	switch (p_idx) {
		case PRESET_SINGLE_SCENE:
			return TTR("Import as Single Scene");
		case PRESET_SEPARATE_ANIMATIONS:
			return TTR("Import with Separate Animations");
		case PRESET_SEPARATE_MATERIALS:
			return TTR("Import with Separate Materials");
		case PRESET_SEPARATE_MESHES:
			return TTR("Import with Separate Objects");
		case PRESET_SEPARATE_MESHES_AND_MATERIALS:
			return TTR("Import with Separate Objects+Materials");
		case PRESET_SEPARATE_MESHES_AND_ANIMATIONS:
			return TTR("Import with Separate Objects+Animations");
		case PRESET_SEPARATE_MATERIALS_AND_ANIMATIONS:
			return TTR("Import with Separate Materials+Animations");
		case PRESET_SEPARATE_MESHES_MATERIALS_AND_ANIMATIONS:
			return TTR("Import with Separate Objects+Materials+Animations");
		case PRESET_MULTIPLE_SCENES:
			return TTR("Import as Multiple Scenes");
		case PRESET_MULTIPLE_SCENES_AND_MATERIALS:
			return TTR("Import as Multiple Scenes+Materials");
	}

	return "";
}

static bool _teststr(const String &p_what, const String &p_str) {
	String what = p_what;

	//remove trailing spaces and numbers, some apps like blender add ".number" to duplicates so also compensate for this
	while (what.length() && ((what[what.length() - 1] >= '0' && what[what.length() - 1] <= '9') || what[what.length() - 1] <= 32 || what[what.length() - 1] == '.')) {
		what = what.substr(0, what.length() - 1);
	}

	if (what.findn("$" + p_str) != -1) { //blender and other stuff
		return true;
	}
	if (what.to_lower().ends_with("-" + p_str)) { //collada only supports "_" and "-" besides letters
		return true;
	}
	if (what.to_lower().ends_with("_" + p_str)) { //collada only supports "_" and "-" besides letters
		return true;
	}
	return false;
}

static String _fixstr(const String &p_what, const String &p_str) {
	String what = p_what;

	//remove trailing spaces and numbers, some apps like blender add ".number" to duplicates so also compensate for this
	while (what.length() && ((what[what.length() - 1] >= '0' && what[what.length() - 1] <= '9') || what[what.length() - 1] <= 32 || what[what.length() - 1] == '.')) {
		what = what.substr(0, what.length() - 1);
	}

	String end = p_what.substr(what.length(), p_what.length() - what.length());

	if (what.findn("$" + p_str) != -1) { //blender and other stuff
		return what.replace("$" + p_str, "") + end;
	}
	if (what.to_lower().ends_with("-" + p_str)) { //collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	if (what.to_lower().ends_with("_" + p_str)) { //collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	return what;
}

static void _gen_shape_list(const Ref<Mesh> &mesh, List<Ref<Shape3D>> &r_shape_list, bool p_convex) {
	if (!p_convex) {
		Ref<Shape3D> shape = mesh->create_trimesh_shape();
		r_shape_list.push_back(shape);
	} else {
		Vector<Ref<Shape3D>> cd = mesh->convex_decompose();
		if (cd.size()) {
			for (int i = 0; i < cd.size(); i++) {
				r_shape_list.push_back(cd[i]);
			}
		}
	}
}

Node *ResourceImporterScene::_fix_node(Node *p_node, Node *p_root, Map<Ref<Mesh>, List<Ref<Shape3D>>> &collision_map, LightBakeMode p_light_bake_mode) {
	// children first
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *r = _fix_node(p_node->get_child(i), p_root, collision_map, p_light_bake_mode);
		if (!r) {
			i--; //was erased
		}
	}

	String name = p_node->get_name();

	bool isroot = p_node == p_root;

	if (!isroot && _teststr(name, "noimp")) {
		memdelete(p_node);
		return nullptr;
	}

	if (Object::cast_to<MeshInstance3D>(p_node)) {
		MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);

		Ref<ArrayMesh> m = mi->get_mesh();

		if (m.is_valid()) {
			for (int i = 0; i < m->get_surface_count(); i++) {
				Ref<StandardMaterial3D> mat = m->surface_get_material(i);
				if (!mat.is_valid()) {
					continue;
				}

				if (_teststr(mat->get_name(), "alpha")) {
					mat->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
					mat->set_name(_fixstr(mat->get_name(), "alpha"));
				}
				if (_teststr(mat->get_name(), "vcol")) {
					mat->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
					mat->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
					mat->set_name(_fixstr(mat->get_name(), "vcol"));
				}
			}
		}

		if (p_light_bake_mode != LIGHT_BAKE_DISABLED) {
			mi->set_gi_mode(GeometryInstance3D::GI_MODE_BAKED);
		}
	}

	if (Object::cast_to<AnimationPlayer>(p_node)) {
		//remove animations referencing non-importable nodes
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(p_node);

		List<StringName> anims;
		ap->get_animation_list(&anims);
		for (List<StringName>::Element *E = anims.front(); E; E = E->next()) {
			Ref<Animation> anim = ap->get_animation(E->get());
			ERR_CONTINUE(anim.is_null());
			for (int i = 0; i < anim->get_track_count(); i++) {
				NodePath path = anim->track_get_path(i);

				for (int j = 0; j < path.get_name_count(); j++) {
					String node = path.get_name(j);
					if (_teststr(node, "noimp")) {
						anim->remove_track(i);
						i--;
						break;
					}
				}
			}
		}
	}

	if (_teststr(name, "colonly") || _teststr(name, "convcolonly")) {
		if (isroot) {
			return p_node;
		}
		MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);
		if (mi) {
			Ref<Mesh> mesh = mi->get_mesh();

			if (mesh.is_valid()) {
				List<Ref<Shape3D>> shapes;
				String fixed_name;
				if (collision_map.has(mesh)) {
					shapes = collision_map[mesh];
				} else if (_teststr(name, "colonly")) {
					_gen_shape_list(mesh, shapes, false);
					collision_map[mesh] = shapes;
				} else if (_teststr(name, "convcolonly")) {
					_gen_shape_list(mesh, shapes, true);
					collision_map[mesh] = shapes;
				}

				if (_teststr(name, "colonly")) {
					fixed_name = _fixstr(name, "colonly");
				} else if (_teststr(name, "convcolonly")) {
					fixed_name = _fixstr(name, "convcolonly");
				}

				ERR_FAIL_COND_V(fixed_name == String(), nullptr);

				if (shapes.size()) {
					StaticBody3D *col = memnew(StaticBody3D);
					col->set_transform(mi->get_transform());
					col->set_name(fixed_name);
					p_node->replace_by(col);
					memdelete(p_node);
					p_node = col;

					int idx = 0;
					for (List<Ref<Shape3D>>::Element *E = shapes.front(); E; E = E->next()) {
						CollisionShape3D *cshape = memnew(CollisionShape3D);
						cshape->set_shape(E->get());
						col->add_child(cshape);

						cshape->set_name("shape" + itos(idx));
						cshape->set_owner(col->get_owner());
						idx++;
					}
				}
			}

		} else if (p_node->has_meta("empty_draw_type")) {
			String empty_draw_type = String(p_node->get_meta("empty_draw_type"));
			StaticBody3D *sb = memnew(StaticBody3D);
			sb->set_name(_fixstr(name, "colonly"));
			Object::cast_to<Node3D>(sb)->set_transform(Object::cast_to<Node3D>(p_node)->get_transform());
			p_node->replace_by(sb);
			memdelete(p_node);
			p_node = nullptr;
			CollisionShape3D *colshape = memnew(CollisionShape3D);
			if (empty_draw_type == "CUBE") {
				BoxShape3D *boxShape = memnew(BoxShape3D);
				boxShape->set_extents(Vector3(1, 1, 1));
				colshape->set_shape(boxShape);
				colshape->set_name("BoxShape3D");
			} else if (empty_draw_type == "SINGLE_ARROW") {
				RayShape3D *rayShape = memnew(RayShape3D);
				rayShape->set_length(1);
				colshape->set_shape(rayShape);
				colshape->set_name("RayShape3D");
				Object::cast_to<Node3D>(sb)->rotate_x(Math_PI / 2);
			} else if (empty_draw_type == "IMAGE") {
				WorldMarginShape3D *world_margin_shape = memnew(WorldMarginShape3D);
				colshape->set_shape(world_margin_shape);
				colshape->set_name("WorldMarginShape3D");
			} else {
				SphereShape3D *sphereShape = memnew(SphereShape3D);
				sphereShape->set_radius(1);
				colshape->set_shape(sphereShape);
				colshape->set_name("SphereShape3D");
			}
			sb->add_child(colshape);
			colshape->set_owner(sb->get_owner());
		}

	} else if (_teststr(name, "rigid") && Object::cast_to<MeshInstance3D>(p_node)) {
		if (isroot) {
			return p_node;
		}

		MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);
		Ref<Mesh> mesh = mi->get_mesh();

		if (mesh.is_valid()) {
			List<Ref<Shape3D>> shapes;
			if (collision_map.has(mesh)) {
				shapes = collision_map[mesh];
			} else {
				_gen_shape_list(mesh, shapes, true);
			}

			RigidBody3D *rigid_body = memnew(RigidBody3D);
			rigid_body->set_name(_fixstr(name, "rigid"));
			p_node->replace_by(rigid_body);
			rigid_body->set_transform(mi->get_transform());
			p_node = rigid_body;
			mi->set_name("mesh");
			mi->set_transform(Transform());
			rigid_body->add_child(mi);
			mi->set_owner(rigid_body->get_owner());

			int idx = 0;
			for (List<Ref<Shape3D>>::Element *E = shapes.front(); E; E = E->next()) {
				CollisionShape3D *cshape = memnew(CollisionShape3D);
				cshape->set_shape(E->get());
				rigid_body->add_child(cshape);

				cshape->set_name("shape" + itos(idx));
				cshape->set_owner(p_node->get_owner());
				idx++;
			}
		}

	} else if ((_teststr(name, "col") || (_teststr(name, "convcol"))) && Object::cast_to<MeshInstance3D>(p_node)) {
		MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);

		Ref<Mesh> mesh = mi->get_mesh();

		if (mesh.is_valid()) {
			List<Ref<Shape3D>> shapes;
			String fixed_name;
			if (collision_map.has(mesh)) {
				shapes = collision_map[mesh];
			} else if (_teststr(name, "col")) {
				_gen_shape_list(mesh, shapes, false);
				collision_map[mesh] = shapes;
			} else if (_teststr(name, "convcol")) {
				_gen_shape_list(mesh, shapes, true);
				collision_map[mesh] = shapes;
			}

			if (_teststr(name, "col")) {
				fixed_name = _fixstr(name, "col");
			} else if (_teststr(name, "convcol")) {
				fixed_name = _fixstr(name, "convcol");
			}

			if (fixed_name != String()) {
				if (mi->get_parent() && !mi->get_parent()->has_node(fixed_name)) {
					mi->set_name(fixed_name);
				}
			}

			if (shapes.size()) {
				StaticBody3D *col = memnew(StaticBody3D);
				col->set_name("static_collision");
				mi->add_child(col);
				col->set_owner(mi->get_owner());

				int idx = 0;
				for (List<Ref<Shape3D>>::Element *E = shapes.front(); E; E = E->next()) {
					CollisionShape3D *cshape = memnew(CollisionShape3D);
					cshape->set_shape(E->get());
					col->add_child(cshape);

					cshape->set_name("shape" + itos(idx));
					cshape->set_owner(p_node->get_owner());

					idx++;
				}
			}
		}

	} else if (_teststr(name, "navmesh") && Object::cast_to<MeshInstance3D>(p_node)) {
		if (isroot) {
			return p_node;
		}

		MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);

		Ref<ArrayMesh> mesh = mi->get_mesh();
		ERR_FAIL_COND_V(mesh.is_null(), nullptr);
		NavigationRegion3D *nmi = memnew(NavigationRegion3D);

		nmi->set_name(_fixstr(name, "navmesh"));
		Ref<NavigationMesh> nmesh = memnew(NavigationMesh);
		nmesh->create_from_mesh(mesh);
		nmi->set_navigation_mesh(nmesh);
		Object::cast_to<Node3D>(nmi)->set_transform(mi->get_transform());
		p_node->replace_by(nmi);
		memdelete(p_node);
		p_node = nmi;
	} else if (_teststr(name, "vehicle")) {
		if (isroot) {
			return p_node;
		}

		Node *owner = p_node->get_owner();
		Node3D *s = Object::cast_to<Node3D>(p_node);
		VehicleBody3D *bv = memnew(VehicleBody3D);
		String n = _fixstr(p_node->get_name(), "vehicle");
		bv->set_name(n);
		p_node->replace_by(bv);
		p_node->set_name(n);
		bv->add_child(p_node);
		bv->set_owner(owner);
		p_node->set_owner(owner);
		bv->set_transform(s->get_transform());
		s->set_transform(Transform());

		p_node = bv;

	} else if (_teststr(name, "wheel")) {
		if (isroot) {
			return p_node;
		}

		Node *owner = p_node->get_owner();
		Node3D *s = Object::cast_to<Node3D>(p_node);
		VehicleWheel3D *bv = memnew(VehicleWheel3D);
		String n = _fixstr(p_node->get_name(), "wheel");
		bv->set_name(n);
		p_node->replace_by(bv);
		p_node->set_name(n);
		bv->add_child(p_node);
		bv->set_owner(owner);
		p_node->set_owner(owner);
		bv->set_transform(s->get_transform());
		s->set_transform(Transform());

		p_node = bv;

	} else if (Object::cast_to<MeshInstance3D>(p_node)) {
		//last attempt, maybe collision inside the mesh data

		MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);

		Ref<ArrayMesh> mesh = mi->get_mesh();
		if (!mesh.is_null()) {
			List<Ref<Shape3D>> shapes;
			if (collision_map.has(mesh)) {
				shapes = collision_map[mesh];
			} else if (_teststr(mesh->get_name(), "col")) {
				_gen_shape_list(mesh, shapes, false);
				collision_map[mesh] = shapes;
				mesh->set_name(_fixstr(mesh->get_name(), "col"));
			} else if (_teststr(mesh->get_name(), "convcol")) {
				_gen_shape_list(mesh, shapes, true);
				collision_map[mesh] = shapes;
				mesh->set_name(_fixstr(mesh->get_name(), "convcol"));
			}

			if (shapes.size()) {
				StaticBody3D *col = memnew(StaticBody3D);
				col->set_name("static_collision");
				p_node->add_child(col);
				col->set_owner(p_node->get_owner());

				int idx = 0;
				for (List<Ref<Shape3D>>::Element *E = shapes.front(); E; E = E->next()) {
					CollisionShape3D *cshape = memnew(CollisionShape3D);
					cshape->set_shape(E->get());
					col->add_child(cshape);

					cshape->set_name("shape" + itos(idx));
					cshape->set_owner(p_node->get_owner());
					idx++;
				}
			}
		}
	}

	return p_node;
}

void ResourceImporterScene::_create_clips(Node *scene, const Array &p_clips, bool p_bake_all) {
	if (!scene->has_node(String("AnimationPlayer"))) {
		return;
	}

	Node *n = scene->get_node(String("AnimationPlayer"));
	ERR_FAIL_COND(!n);
	AnimationPlayer *anim = Object::cast_to<AnimationPlayer>(n);
	ERR_FAIL_COND(!anim);

	if (!anim->has_animation("default")) {
		return;
	}

	Ref<Animation> default_anim = anim->get_animation("default");

	for (int i = 0; i < p_clips.size(); i += 4) {
		String name = p_clips[i];
		float from = p_clips[i + 1];
		float to = p_clips[i + 2];
		bool loop = p_clips[i + 3];
		if (from >= to) {
			continue;
		}

		Ref<Animation> new_anim = memnew(Animation);

		for (int j = 0; j < default_anim->get_track_count(); j++) {
			List<float> keys;
			int kc = default_anim->track_get_key_count(j);
			int dtrack = -1;
			for (int k = 0; k < kc; k++) {
				float kt = default_anim->track_get_key_time(j, k);
				if (kt >= from && kt < to) {
					//found a key within range, so create track
					if (dtrack == -1) {
						new_anim->add_track(default_anim->track_get_type(j));
						dtrack = new_anim->get_track_count() - 1;
						new_anim->track_set_path(dtrack, default_anim->track_get_path(j));

						if (kt > (from + 0.01) && k > 0) {
							if (default_anim->track_get_type(j) == Animation::TYPE_TRANSFORM) {
								Quat q;
								Vector3 p;
								Vector3 s;
								default_anim->transform_track_interpolate(j, from, &p, &q, &s);
								new_anim->transform_track_insert_key(dtrack, 0, p, q, s);
							}
							if (default_anim->track_get_type(j) == Animation::TYPE_VALUE) {
								Variant var = default_anim->value_track_interpolate(j, from);
								new_anim->track_insert_key(dtrack, 0, var);
							}
						}
					}

					if (default_anim->track_get_type(j) == Animation::TYPE_TRANSFORM) {
						Quat q;
						Vector3 p;
						Vector3 s;
						default_anim->transform_track_get_key(j, k, &p, &q, &s);
						new_anim->transform_track_insert_key(dtrack, kt - from, p, q, s);
					}
					if (default_anim->track_get_type(j) == Animation::TYPE_VALUE) {
						Variant var = default_anim->track_get_key_value(j, k);
						new_anim->track_insert_key(dtrack, kt - from, var);
					}
				}

				if (dtrack != -1 && kt >= to) {
					if (default_anim->track_get_type(j) == Animation::TYPE_TRANSFORM) {
						Quat q;
						Vector3 p;
						Vector3 s;
						default_anim->transform_track_interpolate(j, to, &p, &q, &s);
						new_anim->transform_track_insert_key(dtrack, to - from, p, q, s);
					}
					if (default_anim->track_get_type(j) == Animation::TYPE_VALUE) {
						Variant var = default_anim->value_track_interpolate(j, to);
						new_anim->track_insert_key(dtrack, to - from, var);
					}
				}
			}

			if (dtrack == -1 && p_bake_all) {
				new_anim->add_track(default_anim->track_get_type(j));
				dtrack = new_anim->get_track_count() - 1;
				new_anim->track_set_path(dtrack, default_anim->track_get_path(j));
				if (default_anim->track_get_type(j) == Animation::TYPE_TRANSFORM) {
					Quat q;
					Vector3 p;
					Vector3 s;
					default_anim->transform_track_interpolate(j, from, &p, &q, &s);
					new_anim->transform_track_insert_key(dtrack, 0, p, q, s);
					default_anim->transform_track_interpolate(j, to, &p, &q, &s);
					new_anim->transform_track_insert_key(dtrack, to - from, p, q, s);
				}
				if (default_anim->track_get_type(j) == Animation::TYPE_VALUE) {
					Variant var = default_anim->value_track_interpolate(j, from);
					new_anim->track_insert_key(dtrack, 0, var);
					Variant to_var = default_anim->value_track_interpolate(j, to);
					new_anim->track_insert_key(dtrack, to - from, to_var);
				}
			}
		}

		new_anim->set_loop(loop);
		new_anim->set_length(to - from);
		anim->add_animation(name, new_anim);
	}

	anim->remove_animation("default"); //remove default (no longer needed)
}

void ResourceImporterScene::_filter_anim_tracks(Ref<Animation> anim, Set<String> &keep) {
	Ref<Animation> a = anim;
	ERR_FAIL_COND(!a.is_valid());

	for (int j = 0; j < a->get_track_count(); j++) {
		String path = a->track_get_path(j);

		if (!keep.has(path)) {
			a->remove_track(j);
			j--;
		}
	}
}

void ResourceImporterScene::_filter_tracks(Node *scene, const String &p_text) {
	if (!scene->has_node(String("AnimationPlayer"))) {
		return;
	}
	Node *n = scene->get_node(String("AnimationPlayer"));
	ERR_FAIL_COND(!n);
	AnimationPlayer *anim = Object::cast_to<AnimationPlayer>(n);
	ERR_FAIL_COND(!anim);

	Vector<String> strings = p_text.split("\n");
	for (int i = 0; i < strings.size(); i++) {
		strings.write[i] = strings[i].strip_edges();
	}

	List<StringName> anim_names;
	anim->get_animation_list(&anim_names);
	for (List<StringName>::Element *E = anim_names.front(); E; E = E->next()) {
		String name = E->get();
		bool valid_for_this = false;
		bool valid = false;

		Set<String> keep;
		Set<String> keep_local;

		for (int i = 0; i < strings.size(); i++) {
			if (strings[i].begins_with("@")) {
				valid_for_this = false;
				for (Set<String>::Element *F = keep_local.front(); F; F = F->next()) {
					keep.insert(F->get());
				}
				keep_local.clear();

				Vector<String> filters = strings[i].substr(1, strings[i].length()).split(",");
				for (int j = 0; j < filters.size(); j++) {
					String fname = filters[j].strip_edges();
					if (fname == "") {
						continue;
					}
					int fc = fname[0];
					bool plus;
					if (fc == '+') {
						plus = true;
					} else if (fc == '-') {
						plus = false;
					} else {
						continue;
					}

					String filter = fname.substr(1, fname.length()).strip_edges();

					if (!name.matchn(filter)) {
						continue;
					}
					valid_for_this = plus;
				}

				if (valid_for_this) {
					valid = true;
				}

			} else if (valid_for_this) {
				Ref<Animation> a = anim->get_animation(name);
				if (!a.is_valid()) {
					continue;
				}

				for (int j = 0; j < a->get_track_count(); j++) {
					String path = a->track_get_path(j);

					String tname = strings[i];
					if (tname == "") {
						continue;
					}
					int fc = tname[0];
					bool plus;
					if (fc == '+') {
						plus = true;
					} else if (fc == '-') {
						plus = false;
					} else {
						continue;
					}

					String filter = tname.substr(1, tname.length()).strip_edges();

					if (!path.matchn(filter)) {
						continue;
					}

					if (plus) {
						keep_local.insert(path);
					} else if (!keep.has(path)) {
						keep_local.erase(path);
					}
				}
			}
		}

		if (valid) {
			for (Set<String>::Element *F = keep_local.front(); F; F = F->next()) {
				keep.insert(F->get());
			}
			_filter_anim_tracks(anim->get_animation(name), keep);
		}
	}
}

void ResourceImporterScene::_optimize_animations(Node *scene, float p_max_lin_error, float p_max_ang_error, float p_max_angle) {
	if (!scene->has_node(String("AnimationPlayer"))) {
		return;
	}
	Node *n = scene->get_node(String("AnimationPlayer"));
	ERR_FAIL_COND(!n);
	AnimationPlayer *anim = Object::cast_to<AnimationPlayer>(n);
	ERR_FAIL_COND(!anim);

	List<StringName> anim_names;
	anim->get_animation_list(&anim_names);
	for (List<StringName>::Element *E = anim_names.front(); E; E = E->next()) {
		Ref<Animation> a = anim->get_animation(E->get());
		a->optimize(p_max_lin_error, p_max_ang_error, Math::deg2rad(p_max_angle));
	}
}

static String _make_extname(const String &p_str) {
	String ext_name = p_str.replace(".", "_");
	ext_name = ext_name.replace(":", "_");
	ext_name = ext_name.replace("\"", "_");
	ext_name = ext_name.replace("<", "_");
	ext_name = ext_name.replace(">", "_");
	ext_name = ext_name.replace("/", "_");
	ext_name = ext_name.replace("|", "_");
	ext_name = ext_name.replace("\\", "_");
	ext_name = ext_name.replace("?", "_");
	ext_name = ext_name.replace("*", "_");

	return ext_name;
}

void ResourceImporterScene::_find_meshes(Node *p_node, Map<Ref<ArrayMesh>, Transform> &meshes) {
	List<PropertyInfo> pi;
	p_node->get_property_list(&pi);

	MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_node);

	if (mi) {
		Ref<ArrayMesh> mesh = mi->get_mesh();

		if (mesh.is_valid() && !meshes.has(mesh)) {
			Node3D *s = mi;
			Transform transform;
			while (s) {
				transform = transform * s->get_transform();
				s = Object::cast_to<Node3D>(s->get_parent());
			}

			meshes[mesh] = transform;
		}
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_find_meshes(p_node->get_child(i), meshes);
	}
}

void ResourceImporterScene::_make_external_resources(Node *p_node, const String &p_base_path, bool p_make_animations, bool p_animations_as_text, bool p_keep_animations, bool p_make_materials, bool p_materials_as_text, bool p_keep_materials, bool p_make_meshes, bool p_meshes_as_text, Map<Ref<Animation>, Ref<Animation>> &p_animations, Map<Ref<Material>, Ref<Material>> &p_materials, Map<Ref<ArrayMesh>, Ref<ArrayMesh>> &p_meshes) {
	List<PropertyInfo> pi;

	if (p_make_animations) {
		if (Object::cast_to<AnimationPlayer>(p_node)) {
			AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(p_node);

			List<StringName> anims;
			ap->get_animation_list(&anims);
			for (List<StringName>::Element *E = anims.front(); E; E = E->next()) {
				Ref<Animation> anim = ap->get_animation(E->get());
				ERR_CONTINUE(anim.is_null());

				if (!p_animations.has(anim)) {
					// Tracks from source file should be set as imported, anything else is a custom track.
					for (int i = 0; i < anim->get_track_count(); i++) {
						anim->track_set_imported(i, true);
					}

					String ext_name;

					if (p_animations_as_text) {
						ext_name = p_base_path.plus_file(_make_extname(E->get()) + ".tres");
					} else {
						ext_name = p_base_path.plus_file(_make_extname(E->get()) + ".anim");
					}

					if (FileAccess::exists(ext_name) && p_keep_animations) {
						// Copy custom animation tracks from previously imported files.
						Ref<Animation> old_anim = ResourceLoader::load(ext_name, "Animation", true);
						if (old_anim.is_valid()) {
							for (int i = 0; i < old_anim->get_track_count(); i++) {
								if (!old_anim->track_is_imported(i)) {
									old_anim->copy_track(i, anim);
								}
							}
							anim->set_loop(old_anim->has_loop());
						}
					}

					anim->set_path(ext_name, true); // Set path to save externally.
					ResourceSaver::save(ext_name, anim, ResourceSaver::FLAG_CHANGE_PATH);
					p_animations[anim] = anim;
				}
			}
		}
	}

	p_node->get_property_list(&pi);

	for (List<PropertyInfo>::Element *E = pi.front(); E; E = E->next()) {
		if (E->get().type == Variant::OBJECT) {
			Ref<Material> mat = p_node->get(E->get().name);

			if (p_make_materials && mat.is_valid() && mat->get_name() != "") {
				if (!p_materials.has(mat)) {
					String ext_name;

					if (p_materials_as_text) {
						ext_name = p_base_path.plus_file(_make_extname(mat->get_name()) + ".tres");
					} else {
						ext_name = p_base_path.plus_file(_make_extname(mat->get_name()) + ".material");
					}

					if (p_keep_materials && FileAccess::exists(ext_name)) {
						//if exists, use it
						p_materials[mat] = ResourceLoader::load(ext_name);
					} else {
						ResourceSaver::save(ext_name, mat, ResourceSaver::FLAG_CHANGE_PATH);
						p_materials[mat] = ResourceLoader::load(ext_name, "", true); // disable loading from the cache.
					}
				}

				if (p_materials[mat] != mat) {
					p_node->set(E->get().name, p_materials[mat]);
				}
			} else {
				Ref<ArrayMesh> mesh = p_node->get(E->get().name);

				if (mesh.is_valid()) {
					bool mesh_just_added = false;

					if (p_make_meshes) {
						if (!p_meshes.has(mesh)) {
							//meshes are always overwritten, keeping them is not practical
							String ext_name;

							if (p_meshes_as_text) {
								ext_name = p_base_path.plus_file(_make_extname(mesh->get_name()) + ".tres");
							} else {
								ext_name = p_base_path.plus_file(_make_extname(mesh->get_name()) + ".mesh");
							}

							ResourceSaver::save(ext_name, mesh, ResourceSaver::FLAG_CHANGE_PATH);
							p_meshes[mesh] = ResourceLoader::load(ext_name);
							p_node->set(E->get().name, p_meshes[mesh]);
							mesh_just_added = true;
						}
					}

					if (p_make_materials) {
						if (mesh_just_added || !p_meshes.has(mesh)) {
							for (int i = 0; i < mesh->get_surface_count(); i++) {
								mat = mesh->surface_get_material(i);

								if (!mat.is_valid()) {
									continue;
								}
								if (mat->get_name() == "") {
									continue;
								}

								if (!p_materials.has(mat)) {
									String ext_name;

									if (p_materials_as_text) {
										ext_name = p_base_path.plus_file(_make_extname(mat->get_name()) + ".tres");
									} else {
										ext_name = p_base_path.plus_file(_make_extname(mat->get_name()) + ".material");
									}

									if (p_keep_materials && FileAccess::exists(ext_name)) {
										//if exists, use it
										p_materials[mat] = ResourceLoader::load(ext_name);
									} else {
										ResourceSaver::save(ext_name, mat, ResourceSaver::FLAG_CHANGE_PATH);
										p_materials[mat] = ResourceLoader::load(ext_name, "", true); // disable loading from the cache.
									}
								}

								if (p_materials[mat] != mat) {
									mesh->surface_set_material(i, p_materials[mat]);

									//re-save the mesh since a material is now assigned
									if (p_make_meshes) {
										String ext_name;

										if (p_meshes_as_text) {
											ext_name = p_base_path.plus_file(_make_extname(mesh->get_name()) + ".tres");
										} else {
											ext_name = p_base_path.plus_file(_make_extname(mesh->get_name()) + ".mesh");
										}

										ResourceSaver::save(ext_name, mesh, ResourceSaver::FLAG_CHANGE_PATH);
										p_meshes[mesh] = ResourceLoader::load(ext_name);
									}
								}
							}

							if (!p_make_meshes) {
								p_meshes[mesh] = Ref<ArrayMesh>(); //save it anyway, so it won't be checked again
							}
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_make_external_resources(p_node->get_child(i), p_base_path, p_make_animations, p_animations_as_text, p_keep_animations, p_make_materials, p_materials_as_text, p_keep_materials, p_make_meshes, p_meshes_as_text, p_animations, p_materials, p_meshes);
	}
}

void ResourceImporterScene::get_import_options(List<ImportOption> *r_options, int p_preset) const {
	r_options->push_back(ImportOption(PropertyInfo(Variant::STRING, "nodes/root_type", PROPERTY_HINT_TYPE_STRING, "Node"), "Node3D"));
	r_options->push_back(ImportOption(PropertyInfo(Variant::STRING, "nodes/root_name"), "Scene Root"));

	List<String> script_extentions;
	ResourceLoader::get_recognized_extensions_for_type("Script", &script_extentions);

	String script_ext_hint;

	for (List<String>::Element *E = script_extentions.front(); E; E = E->next()) {
		if (script_ext_hint != "") {
			script_ext_hint += ",";
		}
		script_ext_hint += "*." + E->get();
	}

	bool materials_out = p_preset == PRESET_SEPARATE_MATERIALS || p_preset == PRESET_SEPARATE_MESHES_AND_MATERIALS || p_preset == PRESET_MULTIPLE_SCENES_AND_MATERIALS || p_preset == PRESET_SEPARATE_MATERIALS_AND_ANIMATIONS || p_preset == PRESET_SEPARATE_MESHES_MATERIALS_AND_ANIMATIONS;
	bool meshes_out = p_preset == PRESET_SEPARATE_MESHES || p_preset == PRESET_SEPARATE_MESHES_AND_MATERIALS || p_preset == PRESET_SEPARATE_MESHES_AND_ANIMATIONS || p_preset == PRESET_SEPARATE_MESHES_MATERIALS_AND_ANIMATIONS;
	bool scenes_out = p_preset == PRESET_MULTIPLE_SCENES || p_preset == PRESET_MULTIPLE_SCENES_AND_MATERIALS;
	bool animations_out = p_preset == PRESET_SEPARATE_ANIMATIONS || p_preset == PRESET_SEPARATE_MESHES_AND_ANIMATIONS || p_preset == PRESET_SEPARATE_MATERIALS_AND_ANIMATIONS || p_preset == PRESET_SEPARATE_MESHES_MATERIALS_AND_ANIMATIONS;

	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "nodes/root_scale", PROPERTY_HINT_RANGE, "0.001,1000,0.001"), 1.0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::STRING, "nodes/custom_script", PROPERTY_HINT_FILE, script_ext_hint), ""));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "nodes/storage", PROPERTY_HINT_ENUM, "Single Scene,Instanced Sub-Scenes"), scenes_out ? 1 : 0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "nodes/optimizer/remove_empty_spatials", PROPERTY_HINT_NONE), false));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "nodes/optimizer/simplify_scene_tree", PROPERTY_HINT_NONE), false));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "materials/location", PROPERTY_HINT_ENUM, "Node,Mesh"), (meshes_out || materials_out) ? 1 : 0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "materials/storage", PROPERTY_HINT_ENUM, "Built-In,Files (.material),Files (.tres)", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), materials_out ? 1 : 0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "materials/keep_on_reimport"), materials_out));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "meshes/compress"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "meshes/ensure_tangents"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "meshes/storage", PROPERTY_HINT_ENUM, "Built-In,Files (.mesh),Files (.tres)"), meshes_out ? 1 : 0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "meshes/light_baking", PROPERTY_HINT_ENUM, "Disabled,Enable,Gen Lightmaps", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), 0));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "meshes/lightmap_texel_size", PROPERTY_HINT_RANGE, "0.001,100,0.001"), 0.1));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "skins/use_named_skins"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "external_files/store_in_subdir"), false));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "animation/import", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "animation/fps", PROPERTY_HINT_RANGE, "1,120,1"), 15));
	r_options->push_back(ImportOption(PropertyInfo(Variant::STRING, "animation/filter_script", PROPERTY_HINT_MULTILINE_TEXT), ""));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "animation/storage", PROPERTY_HINT_ENUM, "Built-In,Files (.anim),Files (.tres)", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), animations_out));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "animation/keep_custom_tracks"), animations_out));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "animation/optimizer/enabled", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "animation/optimizer/max_linear_error"), 0.05));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "animation/optimizer/max_angular_error"), 0.01));
	r_options->push_back(ImportOption(PropertyInfo(Variant::FLOAT, "animation/optimizer/max_angle"), 22));
	r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "animation/optimizer/remove_unused_tracks"), true));
	r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "animation/clips/amount", PROPERTY_HINT_RANGE, "0,256,1", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), 0));
	for (int i = 0; i < 256; i++) {
		r_options->push_back(ImportOption(PropertyInfo(Variant::STRING, "animation/clip_" + itos(i + 1) + "/name"), ""));
		r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "animation/clip_" + itos(i + 1) + "/start_frame"), 0));
		r_options->push_back(ImportOption(PropertyInfo(Variant::INT, "animation/clip_" + itos(i + 1) + "/end_frame"), 0));
		r_options->push_back(ImportOption(PropertyInfo(Variant::BOOL, "animation/clip_" + itos(i + 1) + "/loops"), false));
	}
}

void ResourceImporterScene::_replace_owner(Node *p_node, Node *p_scene, Node *p_new_owner) {
	if (p_node != p_new_owner && p_node->get_owner() == p_scene) {
		p_node->set_owner(p_new_owner);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *n = p_node->get_child(i);
		_replace_owner(n, p_scene, p_new_owner);
	}
}

Node *ResourceImporterScene::import_scene_from_other_importer(EditorSceneImporter *p_exception, const String &p_path, uint32_t p_flags, int p_bake_fps) {
	Ref<EditorSceneImporter> importer;
	String ext = p_path.get_extension().to_lower();

	for (Set<Ref<EditorSceneImporter>>::Element *E = importers.front(); E; E = E->next()) {
		if (E->get().ptr() == p_exception) {
			continue;
		}
		List<String> extensions;
		E->get()->get_extensions(&extensions);

		for (List<String>::Element *F = extensions.front(); F; F = F->next()) {
			if (F->get().to_lower() == ext) {
				importer = E->get();
				break;
			}
		}

		if (importer.is_valid()) {
			break;
		}
	}

	ERR_FAIL_COND_V(!importer.is_valid(), nullptr);

	List<String> missing;
	Error err;
	return importer->import_scene(p_path, p_flags, p_bake_fps, &missing, &err);
}

Ref<Animation> ResourceImporterScene::import_animation_from_other_importer(EditorSceneImporter *p_exception, const String &p_path, uint32_t p_flags, int p_bake_fps) {
	Ref<EditorSceneImporter> importer;
	String ext = p_path.get_extension().to_lower();

	for (Set<Ref<EditorSceneImporter>>::Element *E = importers.front(); E; E = E->next()) {
		if (E->get().ptr() == p_exception) {
			continue;
		}
		List<String> extensions;
		E->get()->get_extensions(&extensions);

		for (List<String>::Element *F = extensions.front(); F; F = F->next()) {
			if (F->get().to_lower() == ext) {
				importer = E->get();
				break;
			}
		}

		if (importer.is_valid()) {
			break;
		}
	}

	ERR_FAIL_COND_V(!importer.is_valid(), nullptr);

	return importer->import_animation(p_path, p_flags, p_bake_fps);
}

Error ResourceImporterScene::_animation_player_move(Node *new_scene, const Node *scene, Map<MeshInstance3D *, Skeleton3D *> &r_moved_meshes) {
	for (int32_t i = 0; i < scene->get_child_count(); i++) {
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(scene->get_child(i));
		if (!ap) {
			continue;
		}
		List<StringName> animations;
		ap->get_animation_list(&animations);
		for (List<StringName>::Element *E = animations.front(); E; E = E->next()) {
			Ref<Animation> animation = ap->get_animation(E->get());
			for (int32_t k = 0; k < animation->get_track_count(); k++) {
				const NodePath path = animation->track_get_path(k);
				Node *node = scene->get_node_or_null(String(path).get_slicec(':', 0));
				ERR_CONTINUE(!node);
				if (node->get_class_name() == Node3D().get_class_name()) {
					return FAILED;
				}
				String property;
				String split_path = String(path).get_slicec(':', 0);
				if (String(path).get_slice_count(":") > 1) {
					property = String(path).trim_prefix(split_path + ":");
				}
				String name = node->get_name();
				MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(node);
				String track_path;
				Skeleton3D *skeleton = nullptr;
				if (mi) {
					String skeleton_path = mi->get_skeleton_path();
					if (!skeleton_path.empty()) {
						Node *skeleton_node = mi->get_node_or_null(skeleton_path);
						ERR_CONTINUE(!skeleton_node);
						skeleton = Object::cast_to<Skeleton3D>(skeleton_node);
						ERR_CONTINUE(!skeleton);
					}
				}
				if (mi && skeleton && property.find("blend_shapes/") != -1) {
					track_path = String(skeleton->get_name()) + "/" + String(name) + ":" + property;
				} else if (mi && !skeleton && property.find("blend_shapes/") != -1) {
					track_path = String(name) + ":" + property;
				} else if (node) {
					if (!property.empty()) {
						track_path = name + ":" + property;
					} else {
						track_path = name;
					}
				} else {
					continue;
				}
				animation->track_set_path(k, track_path);
			}
		}
		AnimationPlayer *new_ap = Object::cast_to<AnimationPlayer>(ap->duplicate());
		new_scene->add_child(new_ap);
		new_ap->set_owner(new_scene);
	}
	return OK;
}

void ResourceImporterScene::_move_nodes(Node *new_scene, const Map<MeshInstance3D *, Skeleton3D *> moved_meshes, const Map<BoneAttachment3D *, Skeleton3D *> moved_attachments) {
	Map<Skeleton3D *, Set<MeshInstance3D *>> new_meshes_location;
	for (Map<MeshInstance3D *, Skeleton3D *>::Element *moved_meshes_i = moved_meshes.front(); moved_meshes_i; moved_meshes_i = moved_meshes_i->next()) {
		Map<Skeleton3D *, Set<MeshInstance3D *>>::Element *mesh_location = new_meshes_location.find(moved_meshes_i->get());
		if (mesh_location) {
			Set<MeshInstance3D *> meshes = mesh_location->get();
			meshes.insert(moved_meshes_i->key());
			new_meshes_location[mesh_location->key()] = meshes;
		} else {
			Set<MeshInstance3D *> meshes;
			meshes.insert(moved_meshes_i->key());
			new_meshes_location.insert(moved_meshes_i->get(), meshes);
		}
	}

	for (Map<Skeleton3D *, Set<MeshInstance3D *>>::Element *new_mesh_i = new_meshes_location.front(); new_mesh_i; new_mesh_i = new_mesh_i->next()) {
		Skeleton3D *old_skel = new_mesh_i->key();
		if (old_skel) {
			Skeleton3D *skel = memnew(Skeleton3D);
			new_scene->add_child(skel);
			skel->set_owner(new_scene);
			skel->set_name(old_skel->get_name());
			for (int32_t i = 0; i < old_skel->get_bone_count(); i++) {
				skel->add_bone(old_skel->get_bone_name(i));
			}
			for (int32_t i = 0; i < old_skel->get_bone_count(); i++) {
				skel->set_bone_parent(i, old_skel->get_bone_parent(i));
				skel->set_bone_rest(i, old_skel->get_bone_rest(i));
			}
			Transform skeleton_global;
			{
				Node3D *current_node = old_skel;
				while (current_node) {
					skeleton_global = current_node->get_transform() * skeleton_global;
					current_node = Object::cast_to<Node3D>(current_node->get_parent());
				}
			}
			print_verbose("ResourceImporterScene skeleton transform " + skeleton_global);
			skel->set_transform(skeleton_global);
			for (Set<MeshInstance3D *>::Element *mesh_i = new_mesh_i->get().front(); mesh_i; mesh_i = mesh_i->next()) {
				MeshInstance3D *old_mi = mesh_i->get();
				MeshInstance3D *mi = memnew(MeshInstance3D);
				Transform mi_global;
				{
					Node3D *current_node = old_mi;
					while (current_node) {
						mi_global = current_node->get_transform() * mi_global;
						current_node = Object::cast_to<Node3D>(current_node->get_parent());
					}
				}
				mi->set_mesh(old_mi->get_mesh());
				mi->set_skin(old_mi->get_skin());
				mi->set_name(old_mi->get_name());
				mi->set_transform(skeleton_global.affine_inverse() * mi_global);
				skel->add_child(mi);
				mi->set_owner(new_scene);
				_duplicate_children(mi, old_mi, new_scene, mi_global);
				mi->set_skeleton_path(NodePath(".."));
			}
			for (Map<BoneAttachment3D *, Skeleton3D *>::Element *attachment_i = moved_attachments.front(); attachment_i; attachment_i = attachment_i->next()) {
				BoneAttachment3D *old_attachment = attachment_i->key();
				BoneAttachment3D *attachment = memnew(BoneAttachment3D);
				Transform attachment_global;
				{
					Node3D *current_node = old_attachment;
					while (current_node) {
						attachment_global = current_node->get_transform() * attachment_global;
						current_node = Object::cast_to<Node3D>(current_node->get_parent());
					}
				}
				attachment->set_name(old_attachment->get_name());
				attachment->set_bone_name(old_attachment->get_bone_name());
				skel->add_child(attachment);
				attachment->set_owner(new_scene);
				_duplicate_children(attachment, old_attachment, new_scene, attachment_global);
				attachment->set_transform(old_attachment->get_transform());
			}
		} else {
			for (Set<MeshInstance3D *>::Element *mesh_i = new_mesh_i->get().front(); mesh_i; mesh_i = mesh_i->next()) {
				MeshInstance3D *old_mi = mesh_i->get();
				MeshInstance3D *mi = memnew(MeshInstance3D);
				Transform mi_global;
				{
					Node3D *current_node = old_mi;
					while (current_node) {
						mi_global = current_node->get_transform() * mi_global;
						current_node = Object::cast_to<Node3D>(current_node->get_parent());
					}
				}
				mi->set_mesh(old_mi->get_mesh());
				mi->set_skin(old_mi->get_skin());
				mi->set_name(old_mi->get_name());
				mi->set_transform(mi_global);
				new_scene->add_child(mi);
				mi->set_owner(new_scene);
			}
		}
	}
}
void ResourceImporterScene::_duplicate_children(Node *current_node, Node *matching_node, Node *owner, Transform global_xform) {
	for (int32_t i = 0; i < matching_node->get_child_count(); i++) {
		Map<Node *, Node *> remap_nodes;
		remap_nodes[owner] = matching_node->get_child(i);
		Node *node = matching_node->get_child(i)->duplicate_and_reown(remap_nodes);
		current_node->add_child(node);
		node->set_owner(owner);
		Node3D *spatial = Object::cast_to<Node3D>(node);
		if (spatial) {
			spatial->set_transform(global_xform.affine_inverse() * spatial->get_transform());
		}
		_duplicate_children(node, current_node->get_child(i), owner, global_xform);
	}
}
void ResourceImporterScene::_moved_mesh_and_attachments(Node *p_current, Node *p_owner, Map<MeshInstance3D *, Skeleton3D *> &r_moved_meshes,
		Map<BoneAttachment3D *, Skeleton3D *> &r_moved_attachments) {
	MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(p_current);
	BoneAttachment3D *bone_attachment = Object::cast_to<BoneAttachment3D>(p_current);
	if (mi) {
		Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(mi->get_node_or_null(mi->get_skeleton_path()));
		if (skeleton) {
			r_moved_meshes.insert(mi, skeleton);
		} else {
			bool is_bone_attachment = false;
			Node *node = mi;
			while (node && node->get_class_name() != Skeleton3D().get_class_name()) {
				if (node->get_class_name() == BoneAttachment3D().get_class_name()) {
					is_bone_attachment = true;
					break;
				}
				node = node->get_parent();
			}
			if (!is_bone_attachment) {
				r_moved_meshes.insert(mi, nullptr);
			}
		}
	} else if (bone_attachment) {
		Node *current_node = bone_attachment->get_parent();
		while (current_node) {
			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(current_node);
			if (skeleton) {
				r_moved_attachments.insert(bone_attachment, skeleton);
				break;
			}
			current_node = bone_attachment->get_parent();
		}
	}

	for (int i = 0; i < p_current->get_child_count(); i++) {
		_moved_mesh_and_attachments(p_current->get_child(i), p_owner, r_moved_meshes, r_moved_attachments);
	}
}
Error ResourceImporterScene::import(const String &p_source_file, const String &p_save_path, const Map<StringName, Variant> &p_options, List<String> *r_platform_variants, List<String> *r_gen_files, Variant *r_metadata) {
	const String &src_path = p_source_file;

	Ref<EditorSceneImporter> importer;
	String ext = src_path.get_extension().to_lower();

	EditorProgress progress("import", TTR("Import Scene"), 104);
	progress.step(TTR("Importing Scene..."), 0);

	for (Set<Ref<EditorSceneImporter>>::Element *E = importers.front(); E; E = E->next()) {
		List<String> extensions;
		E->get()->get_extensions(&extensions);

		for (List<String>::Element *F = extensions.front(); F; F = F->next()) {
			if (F->get().to_lower() == ext) {
				importer = E->get();
				break;
			}
		}

		if (importer.is_valid()) {
			break;
		}
	}

	ERR_FAIL_COND_V(!importer.is_valid(), ERR_FILE_UNRECOGNIZED);

	float fps = p_options["animation/fps"];

	int import_flags = EditorSceneImporter::IMPORT_ANIMATION_DETECT_LOOP;
	if (!bool(p_options["animation/optimizer/remove_unused_tracks"])) {
		import_flags |= EditorSceneImporter::IMPORT_ANIMATION_FORCE_ALL_TRACKS_IN_ALL_CLIPS;
	}

	if (bool(p_options["animation/import"])) {
		import_flags |= EditorSceneImporter::IMPORT_ANIMATION;
	}

	if (int(p_options["meshes/compress"])) {
		import_flags |= EditorSceneImporter::IMPORT_USE_COMPRESSION;
	}

	if (bool(p_options["meshes/ensure_tangents"])) {
		import_flags |= EditorSceneImporter::IMPORT_GENERATE_TANGENT_ARRAYS;
	}

	if (int(p_options["materials/location"]) == 0) {
		import_flags |= EditorSceneImporter::IMPORT_MATERIALS_IN_INSTANCES;
	}

	if (bool(p_options["skins/use_named_skins"])) {
		import_flags |= EditorSceneImporter::IMPORT_USE_NAMED_SKIN_BINDS;
	}

	Error err = OK;
	List<String> missing_deps; // for now, not much will be done with this
	Node *scene = importer->import_scene(src_path, import_flags, fps, &missing_deps, &err);
	if (!scene || err != OK) {
		return err;
	}

	String root_type = p_options["nodes/root_type"];
	root_type = root_type.split(" ")[0]; // full root_type is "ClassName (filename.gd)" for a script global class.

	Ref<Script> root_script = nullptr;
	if (ScriptServer::is_global_class(root_type)) {
		root_script = ResourceLoader::load(ScriptServer::get_global_class_path(root_type));
		root_type = ScriptServer::get_global_class_base(root_type);
	}

	if (root_type != "Node3D") {
		Node *base_node = Object::cast_to<Node>(ClassDB::instance(root_type));

		if (base_node) {
			scene->replace_by(base_node);
			memdelete(scene);
			scene = base_node;
		}
	}

	if (root_script.is_valid()) {
		scene->set_script(Variant(root_script));
	}

	float root_scale = 1.0;
	if (Object::cast_to<Node3D>(scene)) {
		root_scale = p_options["nodes/root_scale"];
		Object::cast_to<Node3D>(scene)->scale(Vector3(root_scale, root_scale, root_scale));
	}

	if (p_options["nodes/root_name"] != "Scene Root") {
		scene->set_name(p_options["nodes/root_name"]);
	} else {
		scene->set_name(p_save_path.get_file().get_basename());
	}

	err = OK;

	String animation_filter = String(p_options["animation/filter_script"]).strip_edges();

	bool use_optimizer = p_options["animation/optimizer/enabled"];
	float anim_optimizer_linerr = p_options["animation/optimizer/max_linear_error"];
	float anim_optimizer_angerr = p_options["animation/optimizer/max_angular_error"];
	float anim_optimizer_maxang = p_options["animation/optimizer/max_angle"];
	int light_bake_mode = p_options["meshes/light_baking"];

	Map<Ref<Mesh>, List<Ref<Shape3D>>> collision_map;

	scene = _fix_node(scene, scene, collision_map, LightBakeMode(light_bake_mode));

	if (use_optimizer) {
		_optimize_animations(scene, anim_optimizer_linerr, anim_optimizer_angerr, anim_optimizer_maxang);
	}

	Array animation_clips;
	{
		int clip_count = p_options["animation/clips/amount"];

		for (int i = 0; i < clip_count; i++) {
			String name = p_options["animation/clip_" + itos(i + 1) + "/name"];
			int from_frame = p_options["animation/clip_" + itos(i + 1) + "/start_frame"];
			int end_frame = p_options["animation/clip_" + itos(i + 1) + "/end_frame"];
			bool loop = p_options["animation/clip_" + itos(i + 1) + "/loops"];

			animation_clips.push_back(name);
			animation_clips.push_back(from_frame / fps);
			animation_clips.push_back(end_frame / fps);
			animation_clips.push_back(loop);
		}
	}
	if (animation_clips.size()) {
		_create_clips(scene, animation_clips, !bool(p_options["animation/optimizer/remove_unused_tracks"]));
	}

	if (animation_filter != "") {
		_filter_tracks(scene, animation_filter);
	}

	bool external_animations = int(p_options["animation/storage"]) == 1 || int(p_options["animation/storage"]) == 2;
	bool external_animations_as_text = int(p_options["animation/storage"]) == 2;
	bool keep_custom_tracks = p_options["animation/keep_custom_tracks"];
	bool external_materials = int(p_options["materials/storage"]) == 1 || int(p_options["materials/storage"]) == 2;
	bool external_materials_as_text = int(p_options["materials/storage"]) == 2;
	bool external_meshes = int(p_options["meshes/storage"]) == 1 || int(p_options["meshes/storage"]) == 2;
	bool external_meshes_as_text = int(p_options["meshes/storage"]) == 2;
	bool external_scenes = int(p_options["nodes/storage"]) == 1;

	String base_path = p_source_file.get_base_dir();

	if (external_animations || external_materials || external_meshes || external_scenes) {
		if (bool(p_options["external_files/store_in_subdir"])) {
			String subdir_name = p_source_file.get_file().get_basename();
			DirAccess *da = DirAccess::open(base_path);
			Error err2 = da->make_dir(subdir_name);
			memdelete(da);
			ERR_FAIL_COND_V_MSG(err2 != OK && err2 != ERR_ALREADY_EXISTS, err2, "Cannot make directory '" + subdir_name + "'.");
			base_path = base_path.plus_file(subdir_name);
		}
	}

	if (light_bake_mode == 2 /* || generate LOD */) {
		Map<Ref<ArrayMesh>, Transform> meshes;
		_find_meshes(scene, meshes);

		String file_id = src_path.get_file();
		String cache_file_path = base_path.plus_file(file_id + ".unwrap_cache");

		Vector<unsigned char> cache_data;

		if (FileAccess::exists(cache_file_path)) {
			Error err2;
			FileAccess *file = FileAccess::open(cache_file_path, FileAccess::READ, &err2);

			if (err2) {
				if (file) {
					memdelete(file);
				}
			} else {
				int cache_size = file->get_len();
				cache_data.resize(cache_size);
				file->get_buffer(cache_data.ptrw(), cache_size);
			}
		}

		float texel_size = p_options["meshes/lightmap_texel_size"];
		texel_size = MAX(0.001, texel_size);

		Map<String, unsigned int> used_unwraps;

		EditorProgress progress2("gen_lightmaps", TTR("Generating Lightmaps"), meshes.size());
		int step = 0;
		for (Map<Ref<ArrayMesh>, Transform>::Element *E = meshes.front(); E; E = E->next()) {
			Ref<ArrayMesh> mesh = E->key();
			String name = mesh->get_name();
			if (name == "") { //should not happen but..
				name = "Mesh " + itos(step);
			}

			progress2.step(TTR("Generating for Mesh: ") + name + " (" + itos(step) + "/" + itos(meshes.size()) + ")", step);

			int *ret_cache_data = (int *)cache_data.ptrw();
			unsigned int ret_cache_size = cache_data.size();
			bool ret_used_cache = true; // Tell the unwrapper to use the cache
			Error err2 = mesh->lightmap_unwrap_cached(ret_cache_data, ret_cache_size, ret_used_cache, E->get(), texel_size);

			if (err2 != OK) {
				EditorNode::add_io_error("Mesh '" + name + "' failed lightmap generation. Please fix geometry.");
			} else {
				String hash = String::md5((unsigned char *)ret_cache_data);
				used_unwraps.insert(hash, ret_cache_size);

				if (!ret_used_cache) {
					// Cache was not used, add the generated entry to the current cache
					if (cache_data.empty()) {
						cache_data.resize(4 + ret_cache_size);
						int *data = (int *)cache_data.ptrw();
						data[0] = 1;
						memcpy(&data[1], ret_cache_data, ret_cache_size);
					} else {
						int current_size = cache_data.size();
						cache_data.resize(cache_data.size() + ret_cache_size);
						unsigned char *ptrw = cache_data.ptrw();
						memcpy(&ptrw[current_size], ret_cache_data, ret_cache_size);
						int *data = (int *)ptrw;
						data[0] += 1;
					}
				}
			}
			step++;
		}

		Error err2;
		FileAccess *file = FileAccess::open(cache_file_path, FileAccess::WRITE, &err2);

		if (err2) {
			if (file) {
				memdelete(file);
			}
		} else {
			// Store number of entries
			file->store_32(used_unwraps.size());

			// Store cache entries
			const int *cache = (int *)cache_data.ptr();
			unsigned int r_idx = 1;
			for (int i = 0; i < cache[0]; ++i) {
				unsigned char *entry_start = (unsigned char *)&cache[r_idx];
				String entry_hash = String::md5(entry_start);
				if (used_unwraps.has(entry_hash)) {
					unsigned int entry_size = used_unwraps[entry_hash];
					file->store_buffer(entry_start, entry_size);
				}

				r_idx += 4; // hash
				r_idx += 2; // size hint

				int vertex_count = cache[r_idx];
				r_idx += 1; // vertex count
				r_idx += vertex_count; // vertex
				r_idx += vertex_count * 2; // uvs

				int index_count = cache[r_idx];
				r_idx += 1; // index count
				r_idx += index_count; // indices
			}

			file->close();
		}
	}

	if (external_animations || external_materials || external_meshes) {
		Map<Ref<Animation>, Ref<Animation>> anim_map;
		Map<Ref<Material>, Ref<Material>> mat_map;
		Map<Ref<ArrayMesh>, Ref<ArrayMesh>> mesh_map;

		bool keep_materials = bool(p_options["materials/keep_on_reimport"]);

		_make_external_resources(scene, base_path, external_animations, external_animations_as_text, keep_custom_tracks, external_materials, external_materials_as_text, keep_materials, external_meshes, external_meshes_as_text, anim_map, mat_map, mesh_map);
	}

	bool move_skeleton_to_root_optimizer = p_options["nodes/optimizer/simplify_scene_tree"];
	if (move_skeleton_to_root_optimizer) {
		Map<MeshInstance3D *, Skeleton3D *> moved_meshes;
		Map<BoneAttachment3D *, Skeleton3D *> moved_attachments;

		_moved_mesh_and_attachments(scene, scene, moved_meshes, moved_attachments);
		Node *old_scene = scene;
		scene = memnew(Node3D);
		if (_animation_player_move(scene, old_scene, moved_meshes) == OK) {
			_move_nodes(scene, moved_meshes, moved_attachments);
			old_scene->queue_delete();
		} else {
			scene->queue_delete();
			scene = old_scene;
			print_error("Cannot simplify_scene_tree. Please remove animated spatials, only skeletons, meshes, and blend shapes can be animated.");
		}
	}

	bool scene_optimizer = p_options["nodes/optimizer/remove_empty_spatials"];
	if (scene_optimizer) {
		_remove_empty_spatials(scene);
	}

	progress.step(TTR("Running Custom Script..."), 2);

	String post_import_script_path = p_options["nodes/custom_script"];
	Ref<EditorScenePostImport> post_import_script;

	if (post_import_script_path != "") {
		Ref<Script> scr = ResourceLoader::load(post_import_script_path);
		if (!scr.is_valid()) {
			EditorNode::add_io_error(TTR("Couldn't load post-import script:") + " " + post_import_script_path);
		} else {
			post_import_script = Ref<EditorScenePostImport>(memnew(EditorScenePostImport));
			post_import_script->set_script(scr);
			if (!post_import_script->get_script_instance()) {
				EditorNode::add_io_error(TTR("Invalid/broken script for post-import (check console):") + " " + post_import_script_path);
				post_import_script.unref();
				return ERR_CANT_CREATE;
			}
		}
	}

	if (post_import_script.is_valid()) {
		post_import_script->init(base_path, p_source_file);
		scene = post_import_script->post_import(scene);
		if (!scene) {
			EditorNode::add_io_error(
					TTR("Error running post-import script:") + " " + post_import_script_path + "\n" +
					TTR("Did you return a Node-derived object in the `post_import()` method?"));
			return err;
		}
	}

	progress.step(TTR("Saving..."), 104);

	if (external_scenes) {
		//save sub-scenes as instances!
		for (int i = 0; i < scene->get_child_count(); i++) {
			Node *child = scene->get_child(i);
			if (child->get_owner() != scene) {
				continue; //not a real child probably created by scene type (ig, a scrollbar)
			}
			_replace_owner(child, scene, child);

			String cn = String(child->get_name()).strip_edges().replace(".", "_").replace(":", "_");
			if (cn == String()) {
				cn = "ChildNode" + itos(i);
			}
			String path = base_path.plus_file(cn + ".scn");
			child->set_filename(path);

			Ref<PackedScene> packer = memnew(PackedScene);
			packer->pack(child);
			err = ResourceSaver::save(path, packer); //do not take over, let the changed files reload themselves
			ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot save scene to file '" + path + "'.");
		}
	}

	Ref<PackedScene> packer = memnew(PackedScene);
	packer->pack(scene);
	print_verbose("Saving scene to: " + p_save_path + ".scn");
	err = ResourceSaver::save(p_save_path + ".scn", packer); //do not take over, let the changed files reload themselves
	ERR_FAIL_COND_V_MSG(err != OK, err, "Cannot save scene to file '" + p_save_path + ".scn'.");

	memdelete(scene);

	//this is not the time to reimport, wait until import process is done, import file is saved, etc.
	//EditorNode::get_singleton()->reload_scene(p_source_file);

	return OK;
}

Vector3 ResourceImporterScene::_get_perpendicular_vector(Vector3 v) {
	Vector3 perpendicular;
	if (v[0] != 0 && v[1] != 0) {
		perpendicular = Vector3(0, 0, 1).cross(v).normalized();
	} else {
		perpendicular = Vector3(1, 0, 0);
	}
	return perpendicular;
}

Quat ResourceImporterScene::_align_vectors(Vector3 a, Vector3 b) {
	a.normalize();
	b.normalize();
	if (a.length_squared() != 0 && b.length_squared() != 0) {
		//Find the axis perpendicular to both vectors and rotate along it by the angular difference
		Vector3 perpendicular = a.cross(b).normalized();
		float angleDiff = a.angle_to(b);
		if (perpendicular.length_squared() == 0) {
			perpendicular = _get_perpendicular_vector(a);
		}
		return Quat(perpendicular, angleDiff);
	} else {
		return Quat();
	}
}

void ResourceImporterScene::_fix_skeleton(Skeleton3D *p_skeleton, Map<int, ResourceImporterScene::RestBone> &r_rest_bones) {
	int bone_count = p_skeleton->get_bone_count();

	//First iterate through all the bones and create a RestBone for it with an empty centroid
	for (int j = 0; j < bone_count; j++) {
		RestBone rest_bone;

		String path = p_skeleton->get_name();
		Node *current_node = p_skeleton->get_parent();
		while (current_node && current_node != p_skeleton->get_owner()) {
			path = String(current_node->get_name()) + "/" + path;
			current_node = current_node->get_parent();
		}
		rest_bone.path = String(path) + String(":") + p_skeleton->get_bone_name(j);
		rest_bone.parent_index = p_skeleton->get_bone_parent(j);
		rest_bone.rest_local_before = p_skeleton->get_bone_rest(j);
		rest_bone.rest_local_after = rest_bone.rest_local_before;
		r_rest_bones.insert(j, rest_bone);
	}

	//We iterate through again, and add the child's position to the centroid of its parent.
	//These position are local to the parent which means (0, 0, 0) is right where the parent is.
	for (int i = 0; i < bone_count; i++) {
		int parent_bone = p_skeleton->get_bone_parent(i);
		if (parent_bone >= 0) {
			r_rest_bones[parent_bone].children_centroid_direction = r_rest_bones[parent_bone].children_centroid_direction + p_skeleton->get_bone_rest(i).origin;
			r_rest_bones[parent_bone].children.push_back(i);
		}
	}

	//Point leaf bones to parent
	for (int i = 0; i < bone_count; i++) {
		ResourceImporterScene::RestBone &leaf_bone = r_rest_bones[i];
		if (!leaf_bone.children.size()) {
			leaf_bone.children_centroid_direction = r_rest_bones[leaf_bone.parent_index].children_centroid_direction;
		}
	}

	//We iterate again to point each bone to the centroid
	//When we rotate a bone, we also have to move all of its children in the opposite direction
	for (int i = 0; i < bone_count; i++) {
		r_rest_bones[i].rest_delta = _align_vectors(Vector3(0, 1, 0), r_rest_bones[i].children_centroid_direction);
		r_rest_bones[i].rest_local_after.basis = r_rest_bones[i].rest_local_after.basis * r_rest_bones[i].rest_delta;

		//Iterate through the children and rotate them in the opposite direction.
		for (int j = 0; j < r_rest_bones[i].children.size(); j++) {
			int child_index = r_rest_bones[i].children[j];
			r_rest_bones[child_index].rest_local_after = Transform(r_rest_bones[i].rest_delta.inverse(), Vector3()) * r_rest_bones[child_index].rest_local_after;
		}
	}

	//One last iteration to apply the transforms we calculated
	for (int i = 0; i < bone_count; i++) {
		p_skeleton->set_bone_rest(i, r_rest_bones[i].rest_local_after);
	}
}

void ResourceImporterScene::_fix_meshes(Map<int, ResourceImporterScene::RestBone> &r_rest_bones, Vector<MeshInstance3D *> p_meshes) {

	for (int32_t mesh_i = 0; mesh_i < p_meshes.size(); mesh_i++) {
		MeshInstance3D *mi = p_meshes.write[mesh_i];
		Ref<Skin> skin = mi->get_skin();
		if (skin.is_null()) {
			continue;
		}
		skin = skin->duplicate();
		mi->set_skin(skin);
		NodePath skeleton_path = mi->get_skeleton_path();
		Node *node = mi->get_node_or_null(skeleton_path);
		Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
		ERR_CONTINUE(!skeleton);
		for (int32_t bind_i = 0; bind_i < skin->get_bind_count(); bind_i++) {
			String bind_name = skin->get_bind_name(bind_i);
			if (bind_name.empty()) {
				continue;
			}
			int32_t bone_index = skeleton->find_bone(bind_name);
			if (bone_index == -1) {
				continue;
			}
			RestBone rest_bone = r_rest_bones[bone_index];
			Transform pose = skin->get_bind_pose(bind_i);
			skin->set_bind_pose(bind_i, Transform(rest_bone.rest_delta.inverse()) * pose);
		}
	}
}
Transform ResourceImporterScene::get_bone_global_transform(int p_id, Skeleton3D *p_skeleton, Vector<Vector<Transform> > p_local_transform_array) {
	Transform return_transform;
	int parent_id = p_skeleton->get_bone_parent(p_id);
	if (parent_id != -1) {
		return_transform = get_bone_global_transform(parent_id, p_skeleton, p_local_transform_array);
	}
	for (int i = 0; i < p_local_transform_array.size(); i++) {
		return_transform *= p_local_transform_array[i][p_id];
	}
	return return_transform;
}

void ResourceImporterScene::_skeleton_point_to_children(Node *p_scene) {
	Map<int, RestBone> rest_bones;
	Vector<MeshInstance3D *> meshes;
	List<Node *> queue;
	queue.push_back(p_scene);

	while (!queue.empty()) {
		List<Node *>::Element *E = queue.front();
		ERR_FAIL_COND(!E);
		Node *node = E->get();
		if (node->get_class_name() == StringName("Skeleton3D")) {
			Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
			_fix_skeleton(skeleton, rest_bones);
		}
		if (node->get_class_name() == StringName("MeshInstance3D")) {
			MeshInstance3D *mi = Object::cast_to<MeshInstance3D>(node);
			if (mi) {
				NodePath path = mi->get_skeleton_path();
				if (!path.is_empty() && mi->get_node_or_null(path) && Object::cast_to<Skeleton3D>(mi->get_node_or_null(path))) {
					meshes.push_back(mi);
				}
			}
		}

		int child_count = node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			queue.push_back(node->get_child(i));
		}
		queue.pop_front();
	}
	_fix_meshes(rest_bones, meshes);
	_align_animations(p_scene, rest_bones);
}

void ResourceImporterScene::_align_animations(Node *scene, const Map<int, RestBone> &p_rest_bones) {

	if (!scene->has_node(String("AnimationPlayer")))
		return;
	Node *n = scene->get_node(String("AnimationPlayer"));
	ERR_FAIL_COND(!n);
	AnimationPlayer *anim = Object::cast_to<AnimationPlayer>(n);
	ERR_FAIL_COND(!anim);

	List<StringName> anim_names;
	anim->get_animation_list(&anim_names);
	for (List<StringName>::Element *anim_i = anim_names.front(); anim_i; anim_i = anim_i->next()) {
		Ref<Animation> a = anim->get_animation(anim_i->get());
		for (Map<int, RestBone>::Element *rest_bone_i = p_rest_bones.front(); rest_bone_i; rest_bone_i = rest_bone_i->next()) {
			int track = a->find_track(rest_bone_i->get().path);
			if (track == -1) {
				continue;
			}
			int new_track = a->add_track(Animation::TYPE_TRANSFORM);
			a->track_set_path(new_track, rest_bone_i->get().path);
			for (int key_i = 0; key_i < a->track_get_key_count(track); key_i++) {
				Vector3 loc;
				Quat rot;
				Vector3 scale;
				Error err = a->transform_track_get_key(track, key_i, &loc, &rot, &scale);
				ERR_FAIL_COND(err != OK);
				real_t time = a->track_get_key_time(track, key_i);
				RestBone rest_bone = rest_bone_i->get();
				Basis basis;
				basis.set_quat_scale(rot, scale);
				Node *node = scene->get_node_or_null(String(rest_bone_i->get().path).split(":")[0]);
				Skeleton3D *skeleton = Object::cast_to<Skeleton3D>(node);
				ERR_FAIL_COND(!skeleton);
				Vector3 axis;
				float angle;
				rot.get_axis_angle(axis, angle);
				axis = rest_bone.rest_delta.inverse().xform(axis);
				loc = rest_bone.rest_delta.inverse().xform(loc);
				rot = Quat(axis, angle);
				scale = Vector3(1, 1, 1) - rest_bone.rest_delta.inverse().xform(Vector3(1, 1, 1) - scale);
				a->transform_track_insert_key(new_track, time, loc, rot, scale);
			}
			a->remove_track(track);
		}
	}
}

ResourceImporterScene *ResourceImporterScene::singleton = NULL;

ResourceImporterScene::ResourceImporterScene() {
	singleton = this;
}

void ResourceImporterScene::_mark_nodes(Node *p_current, Node *p_owner, Vector<Node *> &r_nodes) {
	Array queue;
	queue.push_back(p_current);
	while (queue.size()) {
		Node *node = queue.pop_back();
		r_nodes.push_back(node);
		for (int32_t i = 0; i < node->get_child_count(); i++) {
			queue.push_back(node->get_child(i));
		}
	}
}

void ResourceImporterScene::_remove_empty_spatials(Node *scene) {
	Vector<Node *> nodes;
	_clean_animation_player(scene);
	_mark_nodes(scene, scene, nodes);
	nodes.invert();
	_remove_nodes(scene, nodes);
}

void ResourceImporterScene::_clean_animation_player(Node *scene) {
	for (int32_t i = 0; i < scene->get_child_count(); i++) {
		AnimationPlayer *ap = Object::cast_to<AnimationPlayer>(scene->get_child(i));
		if (!ap) {
			continue;
		}
		List<StringName> animations;
		ap->get_animation_list(&animations);
		for (List<StringName>::Element *E = animations.front(); E; E = E->next()) {
			Ref<Animation> animation = ap->get_animation(E->get());
			for (int32_t k = 0; k < animation->get_track_count(); k++) {
				NodePath path = animation->track_get_path(k);
				if (!scene->has_node(path)) {
					animation->remove_track(k);
				}
			}
		}
	}
}

void ResourceImporterScene::_remove_nodes(Node *scene, Vector<Node *> &r_nodes) {
	for (int32_t node_i = 0; node_i < r_nodes.size(); node_i++) {
		Node *node = r_nodes[node_i];
		bool is_root = node == scene;
		bool is_base_spatial = node->get_class_name() == Node3D().get_class_name();
		int32_t pending_deletion_count = 0;
		for (int32_t child_i = 0; child_i < node->get_child_count(); child_i++) {
			if (node->get_child(child_i)->is_queued_for_deletion()) {
				pending_deletion_count++;
			}
		}
		bool has_children = (node->get_child_count() - pending_deletion_count) > 0;
		if (!is_root && is_base_spatial && !has_children) {
			print_verbose("ResourceImporterScene extra node \"" + node->get_name() + "\" was removed");
			node->queue_delete();
		} else {
			print_verbose("ResourceImporterScene node \"" + node->get_name() + "\" was kept");
		}
	}
}

///////////////////////////////////////

uint32_t EditorSceneImporterESCN::get_import_flags() const {
	return IMPORT_SCENE;
}

void EditorSceneImporterESCN::get_extensions(List<String> *r_extensions) const {
	r_extensions->push_back("escn");
}

Node *EditorSceneImporterESCN::import_scene(const String &p_path, uint32_t p_flags, int p_bake_fps, List<String> *r_missing_deps, Error *r_err) {
	Error error;
	Ref<PackedScene> ps = ResourceFormatLoaderText::singleton->load(p_path, p_path, &error);
	ERR_FAIL_COND_V_MSG(!ps.is_valid(), nullptr, "Cannot load scene as text resource from path '" + p_path + "'.");

	Node *scene = ps->instance();
	ERR_FAIL_COND_V(!scene, nullptr);

	return scene;
}

Ref<Animation> EditorSceneImporterESCN::import_animation(const String &p_path, uint32_t p_flags, int p_bake_fps) {
	ERR_FAIL_V(Ref<Animation>());
}
