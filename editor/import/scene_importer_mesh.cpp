/*************************************************************************/
/*  scene_importer_mesh.cpp                                              */
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

#include "scene_importer_mesh.h"
#include "thirdparty/opensubdiv/far/primvarRefiner.h"
#include "thirdparty/opensubdiv/far/topologyDescriptor.h"

#include "scene/resources/surface_tool.h"
#include "thirdparty/opensubdiv/far/topologyRefiner.h"
#include "thirdparty/opensubdiv/sdc/types.h"

typedef OpenSubdiv::v3_4_3::Far::TopologyDescriptor Descriptor;

void EditorSceneImporterMesh::add_blend_shape(const String &p_name) {
	ERR_FAIL_COND(surfaces.size() > 0);
	blend_shapes.push_back(p_name);
}

int EditorSceneImporterMesh::get_blend_shape_count() const {
	return blend_shapes.size();
}

String EditorSceneImporterMesh::get_blend_shape_name(int p_blend_shape) const {
	ERR_FAIL_INDEX_V(p_blend_shape, blend_shapes.size(), String());
	return blend_shapes[p_blend_shape];
}

void EditorSceneImporterMesh::set_blend_shape_mode(Mesh::BlendShapeMode p_blend_shape_mode) {
	blend_shape_mode = p_blend_shape_mode;
}

Mesh::BlendShapeMode EditorSceneImporterMesh::get_blend_shape_mode() const {
	return blend_shape_mode;
}

void EditorSceneImporterMesh::add_surface(Mesh::PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes, const Dictionary &p_lods, const Ref<Material> &p_material, const String &p_name) {
	ERR_FAIL_COND(p_blend_shapes.size() != blend_shapes.size());
	ERR_FAIL_COND(p_arrays.size() != Mesh::ARRAY_MAX);
	Surface s;
	s.primitive = p_primitive;
	s.arrays = p_arrays;
	s.name = p_name;

	Vector<Vector3> vertex_array = p_arrays[Mesh::ARRAY_VERTEX];
	int vertex_count = vertex_array.size();
	ERR_FAIL_COND(vertex_count == 0);

	for (int i = 0; i < blend_shapes.size(); i++) {
		Array bsdata = p_blend_shapes[i];
		ERR_FAIL_COND(bsdata.size() != Mesh::ARRAY_MAX);
		Vector<Vector3> vertex_data = bsdata[Mesh::ARRAY_VERTEX];
		ERR_FAIL_COND(vertex_data.size() != vertex_count);
		Surface::BlendShape bs;
		bs.arrays = bsdata;
		s.blend_shape_data.push_back(bs);
	}

	List<Variant> lods;
	p_lods.get_key_list(&lods);
	for (List<Variant>::Element *E = lods.front(); E; E = E->next()) {
		ERR_CONTINUE(!E->get().is_num());
		Surface::LOD lod;
		lod.distance = E->get();
		lod.indices = p_lods[E->get()];
		ERR_CONTINUE(lod.indices.size() == 0);
		s.lods.push_back(lod);
	}

	s.material = p_material;

	surfaces.push_back(s);
	mesh.unref();
}

int EditorSceneImporterMesh::get_surface_count() const {
	return surfaces.size();
}

Mesh::PrimitiveType EditorSceneImporterMesh::get_surface_primitive_type(int p_surface) {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Mesh::PRIMITIVE_MAX);
	return surfaces[p_surface].primitive;
}
Array EditorSceneImporterMesh::get_surface_arrays(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Array());
	return surfaces[p_surface].arrays;
}
String EditorSceneImporterMesh::get_surface_name(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}
Array EditorSceneImporterMesh::get_surface_blend_shape_arrays(int p_surface, int p_blend_shape) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Array());
	ERR_FAIL_INDEX_V(p_blend_shape, surfaces[p_surface].blend_shape_data.size(), Array());
	return surfaces[p_surface].blend_shape_data[p_blend_shape].arrays;
}
int EditorSceneImporterMesh::get_surface_lod_count(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	return surfaces[p_surface].lods.size();
}
Vector<int> EditorSceneImporterMesh::get_surface_lod_indices(int p_surface, int p_lod) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Vector<int>());
	ERR_FAIL_INDEX_V(p_lod, surfaces[p_surface].lods.size(), Vector<int>());

	return surfaces[p_surface].lods[p_lod].indices;
}

float EditorSceneImporterMesh::get_surface_lod_size(int p_surface, int p_lod) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	ERR_FAIL_INDEX_V(p_lod, surfaces[p_surface].lods.size(), 0);
	return surfaces[p_surface].lods[p_lod].distance;
}

Ref<Material> EditorSceneImporterMesh::get_surface_material(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Ref<Material>());
	return surfaces[p_surface].material;
}

void EditorSceneImporterMesh::set_surface_material(int p_surface, const Ref<Material> &p_material) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].material = p_material;
}

Basis EditorSceneImporterMesh::compute_rotation_matrix_from_ortho_6d(Vector3 x_raw, Vector3 y_raw) {
	Vector3 x = x_raw.normalized();
	Vector3 z = x.cross(y_raw);
	z = z.normalized();
	Vector3 y = z.cross(x);
	Basis basis;
	basis.set_axis(Vector3::AXIS_X, x);
	basis.set_axis(Vector3::AXIS_Y, y);
	basis.set_axis(Vector3::AXIS_Z, z);
	return basis;
}

void EditorSceneImporterMesh::generate_lods() {
	if (!SurfaceTool::simplify_func) {
		return;
	}
	if (!SurfaceTool::simplify_scale_func) {
		return;
	}
	if (!SurfaceTool::simplify_sloppy_func) {
		return;
	}
	if (!SurfaceTool::simplify_with_attrib_func) {
		return;
	}

	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].primitive != Mesh::PRIMITIVE_TRIANGLES) {
			continue;
		}
		Ref<SurfaceTool> st;
		st.instance();
		st->create_from_triangle_arrays(surfaces[i].arrays);
		st->deindex();
		st->index();
		surfaces.write[i].arrays = st->commit_to_arrays();
		surfaces.write[i].arrays = subdivide(surfaces[i].arrays, 1);

		surfaces.write[i].lods.clear();
		Vector<Vector3> vertices = surfaces[i].arrays[RS::ARRAY_VERTEX];
		Vector<int> indices = surfaces[i].arrays[RS::ARRAY_INDEX];
		if (indices.size() == 0) {
			continue; //no lods if no indices
		}
		Vector<Vector3> normals = surfaces[i].arrays[RS::ARRAY_NORMAL];
		uint32_t vertex_count = vertices.size();
		const Vector3 *vertices_ptr = vertices.ptr();
		Vector<float> basis_normals;
		int32_t attribute_count = 6;
		for (int32_t normal_i = 0; normal_i < normals.size(); normal_i++) {
			Basis basis;
			basis.set_euler(normals[normal_i]);
			Vector3 basis_x = basis.get_axis(0);
			Vector3 basis_y = basis.get_axis(1);
			basis = compute_rotation_matrix_from_ortho_6d(basis_x, basis_y);
			basis_x = basis.get_axis(0);
			basis_y = basis.get_axis(1);
			basis_normals.push_back(basis_x.x);
			basis_normals.push_back(basis_x.y);
			basis_normals.push_back(basis_x.z);
			basis_normals.push_back(basis_y.x);
			basis_normals.push_back(basis_y.y);
			basis_normals.push_back(basis_y.z);
		}
		Vector<float> normal_weights;
		normal_weights.resize(vertex_count);
		for (int32_t weight_i = 0; weight_i < normal_weights.size(); weight_i++) {
			normal_weights.write[weight_i] = 1.0;
		}
		int min_indices = 10;
		const float threshold = 80.0 / 100.0;
		int index_target = indices.size() * threshold;
		print_verbose(vformat("Total %s triangles", indices.size() / 3));
		const float mesh_scale = SurfaceTool::simplify_scale_func((const float *)vertices_ptr, vertex_count, sizeof(Vector3));
		const float thickness_of_clipping_clothing = 0.05;
		const float abs_target_meter_error = thickness_of_clipping_clothing / 4.0 * 1000.0;
		float rel_mesh_error = abs_target_meter_error / mesh_scale;
		do {
			Vector<int> new_indices;
			new_indices.resize(indices.size());
			size_t new_len = SurfaceTool::simplify_with_attrib_func((unsigned int *)new_indices.ptrw(), (const unsigned int *)indices.ptr(), indices.size(), (const float *)vertices_ptr, vertex_count, sizeof(Vector3), index_target, rel_mesh_error, &rel_mesh_error, (float *)basis_normals.ptrw(), normal_weights.ptrw(), attribute_count);
			Surface::LOD lod;
			lod.distance = rel_mesh_error;
			if (Math::is_equal_approx(rel_mesh_error, 0.0f)) {
				break;
			}
			index_target *= threshold;
			if (!new_len) {
				continue;
			}
			new_indices.resize(new_len);
			lod.indices = new_indices;
			print_line("Lod " + itos(surfaces.write[i].lods.size()) + " begin with " + itos(indices.size() / 3) + " triangles and shoot for " + itos(index_target / 3) + " triangles. Got " + itos(new_len / 3) + " triangles. Lod screen ratio " + rtos(lod.distance));
			surfaces.write[i].lods.push_back(lod);
			if ((int)new_len > (index_target * 120 / 100.0)) {
				break;
			}
		} while (index_target > min_indices);
	}
}

bool EditorSceneImporterMesh::has_mesh() const {
	return mesh.is_valid();
}

Ref<ArrayMesh> EditorSceneImporterMesh::get_mesh(const Ref<Mesh> &p_base) {
	ERR_FAIL_COND_V(surfaces.size() == 0, Ref<ArrayMesh>());

	if (mesh.is_null()) {
		if (p_base.is_valid()) {
			mesh = p_base;
		}
		if (mesh.is_null()) {
			mesh.instance();
		}
		mesh->set_name(get_name());
		if (has_meta("import_id")) {
			mesh->set_meta("import_id", get_meta("import_id"));
		}
		for (int i = 0; i < blend_shapes.size(); i++) {
			mesh->add_blend_shape(blend_shapes[i]);
		}
		mesh->set_blend_shape_mode(blend_shape_mode);
		for (int i = 0; i < surfaces.size(); i++) {
			Array bs_data;
			if (surfaces[i].blend_shape_data.size()) {
				for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
					bs_data.push_back(surfaces[i].blend_shape_data[j].arrays);
				}
			}
			Dictionary lods;
			if (surfaces[i].lods.size()) {
				for (int j = 0; j < surfaces[i].lods.size(); j++) {
					lods[surfaces[i].lods[j].distance] = surfaces[i].lods[j].indices;
				}
			}

			mesh->add_surface_from_arrays(surfaces[i].primitive, surfaces[i].arrays, bs_data, lods);
			if (surfaces[i].material.is_valid()) {
				mesh->surface_set_material(mesh->get_surface_count() - 1, surfaces[i].material);
			}
			if (surfaces[i].name != String()) {
				mesh->surface_set_name(mesh->get_surface_count() - 1, surfaces[i].name);
			}
		}

		mesh->set_lightmap_size_hint(lightmap_size_hint);

		if (shadow_mesh.is_valid()) {
			Ref<ArrayMesh> shadow = shadow_mesh->get_mesh();
			mesh->set_shadow_mesh(shadow);
		}
	}

	return mesh;
}

void EditorSceneImporterMesh::clear() {
	surfaces.clear();
	blend_shapes.clear();
	mesh.unref();
}

void EditorSceneImporterMesh::create_shadow_mesh() {
	if (shadow_mesh.is_valid()) {
		shadow_mesh.unref();
	}

	//no shadow mesh for blendshapes
	if (blend_shapes.size() > 0) {
		return;
	}
	//no shadow mesh for skeletons
	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].arrays[RS::ARRAY_BONES].get_type() != Variant::NIL) {
			return;
		}
		if (surfaces[i].arrays[RS::ARRAY_WEIGHTS].get_type() != Variant::NIL) {
			return;
		}
	}

	shadow_mesh.instance();

	for (int i = 0; i < surfaces.size(); i++) {
		LocalVector<int> vertex_remap;
		Vector<Vector3> new_vertices;
		Vector<Vector3> vertices = surfaces[i].arrays[RS::ARRAY_VERTEX];
		int vertex_count = vertices.size();
		{
			Map<Vector3, int> unique_vertices;
			const Vector3 *vptr = vertices.ptr();
			for (int j = 0; j < vertex_count; j++) {
				Vector3 v = vptr[j];

				Map<Vector3, int>::Element *E = unique_vertices.find(v);

				if (E) {
					vertex_remap.push_back(E->get());
				} else {
					int vcount = unique_vertices.size();
					unique_vertices[v] = vcount;
					vertex_remap.push_back(vcount);
					new_vertices.push_back(v);
				}
			}
		}

		Array new_surface;
		new_surface.resize(RS::ARRAY_MAX);
		Dictionary lods;

		//		print_line("original vertex count: " + itos(vertices.size()) + " new vertex count: " + itos(new_vertices.size()));

		new_surface[RS::ARRAY_VERTEX] = new_vertices;

		Vector<int> indices = surfaces[i].arrays[RS::ARRAY_INDEX];
		if (indices.size()) {
			int index_count = indices.size();
			const int *index_rptr = indices.ptr();
			Vector<int> new_indices;
			new_indices.resize(indices.size());
			int *index_wptr = new_indices.ptrw();

			for (int j = 0; j < index_count; j++) {
				int index = index_rptr[j];
				ERR_FAIL_INDEX(index, vertex_count);
				index_wptr[j] = vertex_remap[index];
			}

			new_surface[RS::ARRAY_INDEX] = new_indices;

			// Make sure the same LODs as the full version are used.
			// This makes it more coherent between rendered model and its shadows.
			for (int j = 0; j < surfaces[i].lods.size(); j++) {
				indices = surfaces[i].lods[j].indices;

				index_count = indices.size();
				index_rptr = indices.ptr();
				new_indices.resize(indices.size());
				index_wptr = new_indices.ptrw();

				for (int k = 0; k < index_count; k++) {
					int index = index_rptr[j];
					ERR_FAIL_INDEX(index, vertex_count);
					index_wptr[j] = vertex_remap[index];
				}

				lods[surfaces[i].lods[j].distance] = new_indices;
			}
		}

		shadow_mesh->add_surface(surfaces[i].primitive, new_surface, Array(), lods, Ref<Material>(), surfaces[i].name);
	}
}

Ref<EditorSceneImporterMesh> EditorSceneImporterMesh::get_shadow_mesh() const {
	return shadow_mesh;
}

void EditorSceneImporterMesh::_set_data(const Dictionary &p_data) {
	clear();
	if (p_data.has("blend_shape_names")) {
		blend_shapes = p_data["blend_shape_names"];
	}
	if (p_data.has("surfaces")) {
		Array surface_arr = p_data["surfaces"];
		for (int i = 0; i < surface_arr.size(); i++) {
			Dictionary s = surface_arr[i];
			ERR_CONTINUE(!s.has("primitive"));
			ERR_CONTINUE(!s.has("arrays"));
			Mesh::PrimitiveType prim = Mesh::PrimitiveType(int(s["primitive"]));
			ERR_CONTINUE(prim >= Mesh::PRIMITIVE_MAX);
			Array arr = s["arrays"];
			Dictionary lods;
			String name;
			if (s.has("name")) {
				name = s["name"];
			}
			if (s.has("lods")) {
				lods = s["lods"];
			}
			Array blend_shapes;
			if (s.has("blend_shapes")) {
				blend_shapes = s["blend_shapes"];
			}
			Ref<Material> material;
			if (s.has("material")) {
				material = s["material"];
			}
			add_surface(prim, arr, blend_shapes, lods, material, name);
		}
	}
}
Dictionary EditorSceneImporterMesh::_get_data() const {
	Dictionary data;
	if (blend_shapes.size()) {
		data["blend_shape_names"] = blend_shapes;
	}
	Array surface_arr;
	for (int i = 0; i < surfaces.size(); i++) {
		Dictionary d;
		d["primitive"] = surfaces[i].primitive;
		d["arrays"] = surfaces[i].arrays;
		if (surfaces[i].blend_shape_data.size()) {
			Array bs_data;
			for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
				bs_data.push_back(surfaces[i].blend_shape_data[j].arrays);
			}
			d["blend_shapes"] = bs_data;
		}
		if (surfaces[i].lods.size()) {
			Dictionary lods;
			for (int j = 0; j < surfaces[i].lods.size(); j++) {
				lods[surfaces[i].lods[j].distance] = surfaces[i].lods[j].indices;
			}
			d["lods"] = lods;
		}

		if (surfaces[i].material.is_valid()) {
			d["material"] = surfaces[i].material;
		}

		if (surfaces[i].name != String()) {
			d["name"] = surfaces[i].name;
		}

		surface_arr.push_back(d);
	}
	data["surfaces"] = surface_arr;
	return data;
}

Vector<Face3> EditorSceneImporterMesh::get_faces() const {
	Vector<Face3> faces;
	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].primitive == Mesh::PRIMITIVE_TRIANGLES) {
			Vector<Vector3> vertices = surfaces[i].arrays[Mesh::ARRAY_VERTEX];
			Vector<int> indices = surfaces[i].arrays[Mesh::ARRAY_INDEX];
			if (indices.size()) {
				for (int j = 0; j < indices.size(); j += 3) {
					Face3 f;
					f.vertex[0] = vertices[indices[j + 0]];
					f.vertex[1] = vertices[indices[j + 1]];
					f.vertex[2] = vertices[indices[j + 2]];
					faces.push_back(f);
				}
			} else {
				for (int j = 0; j < vertices.size(); j += 3) {
					Face3 f;
					f.vertex[0] = vertices[j + 0];
					f.vertex[1] = vertices[j + 1];
					f.vertex[2] = vertices[j + 2];
					faces.push_back(f);
				}
			}
		}
	}

	return faces;
}

Vector<Ref<Shape3D>> EditorSceneImporterMesh::convex_decompose() const {
	ERR_FAIL_COND_V(!Mesh::convex_composition_function, Vector<Ref<Shape3D>>());

	const Vector<Face3> faces = get_faces();

	Vector<Vector<Face3>> decomposed = Mesh::convex_composition_function(faces);

	Vector<Ref<Shape3D>> ret;

	for (int i = 0; i < decomposed.size(); i++) {
		Set<Vector3> points;
		for (int j = 0; j < decomposed[i].size(); j++) {
			points.insert(decomposed[i][j].vertex[0]);
			points.insert(decomposed[i][j].vertex[1]);
			points.insert(decomposed[i][j].vertex[2]);
		}

		Vector<Vector3> convex_points;
		convex_points.resize(points.size());
		{
			Vector3 *w = convex_points.ptrw();
			int idx = 0;
			for (Set<Vector3>::Element *E = points.front(); E; E = E->next()) {
				w[idx++] = E->get();
			}
		}

		Ref<ConvexPolygonShape3D> shape;
		shape.instance();
		shape->set_points(convex_points);
		ret.push_back(shape);
	}

	return ret;
}

Ref<Shape3D> EditorSceneImporterMesh::create_trimesh_shape() const {
	Vector<Face3> faces = get_faces();
	if (faces.size() == 0) {
		return Ref<Shape3D>();
	}

	Vector<Vector3> face_points;
	face_points.resize(faces.size() * 3);

	for (int i = 0; i < face_points.size(); i += 3) {
		Face3 f = faces.get(i / 3);
		face_points.set(i, f.vertex[0]);
		face_points.set(i + 1, f.vertex[1]);
		face_points.set(i + 2, f.vertex[2]);
	}

	Ref<ConcavePolygonShape3D> shape = memnew(ConcavePolygonShape3D);
	shape->set_faces(face_points);
	return shape;
}

Ref<NavigationMesh> EditorSceneImporterMesh::create_navigation_mesh() {
	Vector<Face3> faces = get_faces();
	if (faces.size() == 0) {
		return Ref<NavigationMesh>();
	}

	Map<Vector3, int> unique_vertices;
	LocalVector<int> face_indices;

	for (int i = 0; i < faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			Vector3 v = faces[i].vertex[j];
			int idx;
			if (unique_vertices.has(v)) {
				idx = unique_vertices[v];
			} else {
				idx = unique_vertices.size();
				unique_vertices[v] = idx;
			}
			face_indices.push_back(idx);
		}
	}

	Vector<Vector3> vertices;
	vertices.resize(unique_vertices.size());
	for (Map<Vector3, int>::Element *E = unique_vertices.front(); E; E = E->next()) {
		vertices.write[E->get()] = E->key();
	}

	Ref<NavigationMesh> nm;
	nm.instance();
	nm->set_vertices(vertices);

	Vector<int> v3;
	v3.resize(3);
	for (uint32_t i = 0; i < face_indices.size(); i += 3) {
		v3.write[0] = face_indices[i + 0];
		v3.write[1] = face_indices[i + 1];
		v3.write[2] = face_indices[i + 2];
		nm->add_polygon(v3);
	}

	return nm;
}

extern bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, int p_index_count, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y, int *&r_cache_data, unsigned int &r_cache_size, bool &r_used_cache);

struct EditorSceneImporterMeshLightmapSurface {
	Ref<Material> material;
	LocalVector<SurfaceTool::Vertex> vertices;
	Mesh::PrimitiveType primitive = Mesh::PrimitiveType::PRIMITIVE_MAX;
	uint32_t format = 0;
	String name;
};

Error EditorSceneImporterMesh::lightmap_unwrap_cached(int *&r_cache_data, unsigned int &r_cache_size, bool &r_used_cache, const Transform &p_base_transform, float p_texel_size) {
	ERR_FAIL_COND_V(!array_mesh_lightmap_unwrap_callback, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(blend_shapes.size() != 0, ERR_UNAVAILABLE, "Can't unwrap mesh with blend shapes.");

	Vector<float> vertices;
	Vector<float> normals;
	Vector<int> indices;
	Vector<float> uv;
	Vector<Pair<int, int>> uv_indices;

	Vector<EditorSceneImporterMeshLightmapSurface> lightmap_surfaces;

	// Keep only the scale
	Transform transform = p_base_transform;
	transform.origin = Vector3();
	transform.looking_at(Vector3(1, 0, 0), Vector3(0, 1, 0));

	Basis normal_basis = transform.basis.inverse().transposed();

	for (int i = 0; i < get_surface_count(); i++) {
		EditorSceneImporterMeshLightmapSurface s;
		s.primitive = get_surface_primitive_type(i);

		ERR_FAIL_COND_V_MSG(s.primitive != Mesh::PRIMITIVE_TRIANGLES, ERR_UNAVAILABLE, "Only triangles are supported for lightmap unwrap.");
		Array arrays = get_surface_arrays(i);
		s.material = get_surface_material(i);
		s.name = get_surface_name(i);

		SurfaceTool::create_vertex_array_from_triangle_arrays(arrays, s.vertices, &s.format);

		Vector<Vector3> rvertices = arrays[Mesh::ARRAY_VERTEX];
		int vc = rvertices.size();
		const Vector3 *r = rvertices.ptr();

		Vector<Vector3> rnormals = arrays[Mesh::ARRAY_NORMAL];

		ERR_FAIL_COND_V_MSG(rnormals.size() == 0, ERR_UNAVAILABLE, "Normals are required for lightmap unwrap.");

		const Vector3 *rn = rnormals.ptr();

		int vertex_ofs = vertices.size() / 3;

		vertices.resize((vertex_ofs + vc) * 3);
		normals.resize((vertex_ofs + vc) * 3);
		uv_indices.resize(vertex_ofs + vc);

		for (int j = 0; j < vc; j++) {
			Vector3 v = transform.xform(r[j]);
			Vector3 n = normal_basis.xform(rn[j]).normalized();

			vertices.write[(j + vertex_ofs) * 3 + 0] = v.x;
			vertices.write[(j + vertex_ofs) * 3 + 1] = v.y;
			vertices.write[(j + vertex_ofs) * 3 + 2] = v.z;
			normals.write[(j + vertex_ofs) * 3 + 0] = n.x;
			normals.write[(j + vertex_ofs) * 3 + 1] = n.y;
			normals.write[(j + vertex_ofs) * 3 + 2] = n.z;
			uv_indices.write[j + vertex_ofs] = Pair<int, int>(i, j);
		}

		Vector<int> rindices = arrays[Mesh::ARRAY_INDEX];
		int ic = rindices.size();

		if (ic == 0) {
			for (int j = 0; j < vc / 3; j++) {
				if (Face3(r[j * 3 + 0], r[j * 3 + 1], r[j * 3 + 2]).is_degenerate()) {
					continue;
				}

				indices.push_back(vertex_ofs + j * 3 + 0);
				indices.push_back(vertex_ofs + j * 3 + 1);
				indices.push_back(vertex_ofs + j * 3 + 2);
			}

		} else {
			const int *ri = rindices.ptr();

			for (int j = 0; j < ic / 3; j++) {
				if (Face3(r[ri[j * 3 + 0]], r[ri[j * 3 + 1]], r[ri[j * 3 + 2]]).is_degenerate()) {
					continue;
				}
				indices.push_back(vertex_ofs + ri[j * 3 + 0]);
				indices.push_back(vertex_ofs + ri[j * 3 + 1]);
				indices.push_back(vertex_ofs + ri[j * 3 + 2]);
			}
		}

		lightmap_surfaces.push_back(s);
	}

	//unwrap

	float *gen_uvs;
	int *gen_vertices;
	int *gen_indices;
	int gen_vertex_count;
	int gen_index_count;
	int size_x;
	int size_y;

	bool ok = array_mesh_lightmap_unwrap_callback(p_texel_size, vertices.ptr(), normals.ptr(), vertices.size() / 3, indices.ptr(), indices.size(), &gen_uvs, &gen_vertices, &gen_vertex_count, &gen_indices, &gen_index_count, &size_x, &size_y, r_cache_data, r_cache_size, r_used_cache);

	if (!ok) {
		return ERR_CANT_CREATE;
	}

	//remove surfaces
	clear();

	//create surfacetools for each surface..
	Vector<Ref<SurfaceTool>> surfaces_tools;

	for (int i = 0; i < lightmap_surfaces.size(); i++) {
		Ref<SurfaceTool> st;
		st.instance();
		st->begin(Mesh::PRIMITIVE_TRIANGLES);
		st->set_material(lightmap_surfaces[i].material);
		st->set_meta("name", lightmap_surfaces[i].name);
		surfaces_tools.push_back(st); //stay there
	}

	print_verbose("Mesh: Gen indices: " + itos(gen_index_count));
	//go through all indices
	for (int i = 0; i < gen_index_count; i += 3) {
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 0]], uv_indices.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 1]], uv_indices.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 2]], uv_indices.size(), ERR_BUG);

		ERR_FAIL_COND_V(uv_indices[gen_vertices[gen_indices[i + 0]]].first != uv_indices[gen_vertices[gen_indices[i + 1]]].first || uv_indices[gen_vertices[gen_indices[i + 0]]].first != uv_indices[gen_vertices[gen_indices[i + 2]]].first, ERR_BUG);

		int surface = uv_indices[gen_vertices[gen_indices[i + 0]]].first;

		for (int j = 0; j < 3; j++) {
			SurfaceTool::Vertex v = lightmap_surfaces[surface].vertices[uv_indices[gen_vertices[gen_indices[i + j]]].second];

			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_COLOR) {
				surfaces_tools.write[surface]->set_color(v.color);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_TEX_UV) {
				surfaces_tools.write[surface]->set_uv(v.uv);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_NORMAL) {
				surfaces_tools.write[surface]->set_normal(v.normal);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_TANGENT) {
				Plane t;
				t.normal = v.tangent;
				t.d = v.binormal.dot(v.normal.cross(v.tangent)) < 0 ? -1 : 1;
				surfaces_tools.write[surface]->set_tangent(t);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_BONES) {
				surfaces_tools.write[surface]->set_bones(v.bones);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_WEIGHTS) {
				surfaces_tools.write[surface]->set_weights(v.weights);
			}

			Vector2 uv2(gen_uvs[gen_indices[i + j] * 2 + 0], gen_uvs[gen_indices[i + j] * 2 + 1]);
			surfaces_tools.write[surface]->set_uv2(uv2);

			surfaces_tools.write[surface]->add_vertex(v.vertex);
		}
	}

	//generate surfaces

	for (int i = 0; i < surfaces_tools.size(); i++) {
		surfaces_tools.write[i]->index();
		Array arrays = surfaces_tools.write[i]->commit_to_arrays();
		add_surface(surfaces_tools.write[i]->get_primitive(), arrays, Array(), Dictionary(), surfaces_tools.write[i]->get_material(), surfaces_tools.write[i]->get_meta("name"));
	}

	set_lightmap_size_hint(Size2(size_x, size_y));

	if (!r_used_cache) {
		//free stuff
		::free(gen_vertices);
		::free(gen_indices);
		::free(gen_uvs);
	}

	return OK;
}

void EditorSceneImporterMesh::set_lightmap_size_hint(const Size2i &p_size) {
	lightmap_size_hint = p_size;
}

Size2i EditorSceneImporterMesh::get_lightmap_size_hint() const {
	return lightmap_size_hint;
}

void EditorSceneImporterMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_blend_shape", "name"), &EditorSceneImporterMesh::add_blend_shape);
	ClassDB::bind_method(D_METHOD("get_blend_shape_count"), &EditorSceneImporterMesh::get_blend_shape_count);
	ClassDB::bind_method(D_METHOD("get_blend_shape_name", "blend_shape_idx"), &EditorSceneImporterMesh::get_blend_shape_name);

	ClassDB::bind_method(D_METHOD("set_blend_shape_mode", "mode"), &EditorSceneImporterMesh::set_blend_shape_mode);
	ClassDB::bind_method(D_METHOD("get_blend_shape_mode"), &EditorSceneImporterMesh::get_blend_shape_mode);

	ClassDB::bind_method(D_METHOD("add_surface", "primitive", "arrays", "blend_shapes", "lods", "material", "name"), &EditorSceneImporterMesh::add_surface, DEFVAL(Array()), DEFVAL(Dictionary()), DEFVAL(Ref<Material>()), DEFVAL(String()));

	ClassDB::bind_method(D_METHOD("get_surface_count"), &EditorSceneImporterMesh::get_surface_count);
	ClassDB::bind_method(D_METHOD("get_surface_primitive_type", "surface_idx"), &EditorSceneImporterMesh::get_surface_primitive_type);
	ClassDB::bind_method(D_METHOD("get_surface_name", "surface_idx"), &EditorSceneImporterMesh::get_surface_name);
	ClassDB::bind_method(D_METHOD("get_surface_arrays", "surface_idx"), &EditorSceneImporterMesh::get_surface_arrays);
	ClassDB::bind_method(D_METHOD("get_surface_blend_shape_arrays", "surface_idx", "blend_shape_idx"), &EditorSceneImporterMesh::get_surface_blend_shape_arrays);
	ClassDB::bind_method(D_METHOD("get_surface_lod_count", "surface_idx"), &EditorSceneImporterMesh::get_surface_lod_count);
	ClassDB::bind_method(D_METHOD("get_surface_lod_size", "surface_idx", "lod_idx"), &EditorSceneImporterMesh::get_surface_lod_size);
	ClassDB::bind_method(D_METHOD("get_surface_lod_indices", "surface_idx", "lod_idx"), &EditorSceneImporterMesh::get_surface_lod_indices);
	ClassDB::bind_method(D_METHOD("get_surface_material", "surface_idx"), &EditorSceneImporterMesh::get_surface_material);

	ClassDB::bind_method(D_METHOD("get_mesh"), &EditorSceneImporterMesh::get_mesh);
	ClassDB::bind_method(D_METHOD("clear"), &EditorSceneImporterMesh::clear);

	ClassDB::bind_method(D_METHOD("_set_data", "data"), &EditorSceneImporterMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &EditorSceneImporterMesh::_get_data);

	ClassDB::bind_method(D_METHOD("set_lightmap_size_hint", "size"), &EditorSceneImporterMesh::set_lightmap_size_hint);
	ClassDB::bind_method(D_METHOD("get_lightmap_size_hint"), &EditorSceneImporterMesh::get_lightmap_size_hint);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "_set_data", "_get_data");
}

Array EditorSceneImporterMesh::subdivide(Array p_mesh_arrays, int p_level) {
	struct Vertex {
		void Clear() { x = y = z = 0; }
		void AddWithWeight(Vertex const &src, float weight) {
			x += weight * src.x;
			y += weight * src.y;
			z += weight * src.z;
		}
		float x, y, z;
	};

	struct VertexUV {
		void Clear() {
			u = v = 0.0f;
		}

		void AddWithWeight(VertexUV const &src, float weight) {
			u += weight * src.u;
			v += weight * src.v;
		}

		// Basic 'uv' layout channel
		float u, v;
	};

	struct VertexBones {
		void Clear() {
			b0 = b1 = b2 = b3 = 0;
		}

		void AddWithWeight(VertexBones const &src, float weight) {
			b0 = src.b0;
			b1 = src.b1;
			b2 = src.b2;
			b3 = src.b3;
		}

		int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
	};
	struct VertexWeights {
		void Clear() {
			w0 = w1 = w2 = w3 = 0.0f;
		}

		void AddWithWeight(VertexWeights const &src, float weight) {
			w0 += weight * src.w0;
			w1 += weight * src.w1;
			w2 += weight * src.w2;
			w3 += weight * src.w3;
		}

		float w0 = 0.0f, w1 = 0.0f, w2 = 0.0f, w3 = 0.f;
	};
	struct SurfaceData {
		Vector<int> mesh_to_subdiv_index_map;
	};
	Vector<SurfaceData> surface_data;
	Vector<int> mesh_to_subdiv_index_map;
	int subdiv_vertex_count = 0;
	int subdiv_index_count = 0;
	int subdiv_uv_count = 0;
	int subdiv_bone_count = 0;
	int subdiv_weight_count = 0;
	ERR_FAIL_COND_V(p_level <= 0, p_mesh_arrays);

	ERR_FAIL_COND_V(p_level <= 0, p_mesh_arrays);

	int surface_count = 1;
	surface_data.resize(surface_count);

	Vector<Vector3> subdiv_vertex_array;
	Vector<Vector2> subdiv_uv_array;

	Vector<VertexBones> subdiv_bones_array;
	Vector<VertexWeights> subdiv_weights_array;
	Vector<int> subdiv_index_array;

	int subdiv_face_count = 0;
	Vector<int> face_to_surface_index_map;

	Map<Vector3, int> vertex_map;

	// Gather all vertices and faces from surfaces
	SurfaceData &surface = surface_data.write[0];
	Vector<Vector3> vertex_array = p_mesh_arrays[Mesh::ARRAY_VERTEX];
	Vector<int> index_array = p_mesh_arrays[Mesh::ARRAY_INDEX];
	Vector<Vector2> uv_array = p_mesh_arrays[Mesh::ARRAY_TEX_UV];
	subdiv_uv_count = uv_array.size();
	Vector<int> bones_array = p_mesh_arrays[Mesh::ARRAY_BONES];
	subdiv_bone_count = bones_array.size() % 8 ? bones_array.size() / 8 : bones_array.size() / 4;
	Vector<float> weights_array = p_mesh_arrays[Mesh::ARRAY_WEIGHTS];
	subdiv_weight_count = weights_array.size() % 8 ? weights_array.size() / 8 : weights_array.size() / 4;

	int index_count = index_array.size();

	// Remove duplicated vertices to link faces properly
	{
		int vertex_source_count = vertex_array.size();

		surface.mesh_to_subdiv_index_map.resize(vertex_source_count);
		subdiv_vertex_array.resize(subdiv_vertex_count + vertex_source_count);
		subdiv_uv_array.resize(subdiv_vertex_count + vertex_source_count);
		subdiv_bones_array.resize(subdiv_vertex_count + vertex_source_count);
		subdiv_weights_array.resize(subdiv_vertex_count + vertex_source_count);

		int vertex_index_out = 0;
		for (int vertex_index = 0; vertex_index < vertex_source_count; ++vertex_index) {
			const Vector3 &vertex = vertex_array[vertex_index];
			Map<Vector3, int>::Element *found_vertex = vertex_map.find(vertex);
			if (found_vertex) {
				surface.mesh_to_subdiv_index_map.write[vertex_index] = found_vertex->value();
			} else {
				int subdiv_vertex_index = subdiv_vertex_count + vertex_index_out;
				vertex_map[vertex] = subdiv_vertex_index;
				if (uv_array.size()) {
					Vector2 uv = uv_array.write[vertex_index];
					surface.mesh_to_subdiv_index_map.write[vertex_index] = subdiv_vertex_index;
					subdiv_uv_array.write[subdiv_vertex_index] = uv;
				}
				if (bones_array.size()) {
					surface.mesh_to_subdiv_index_map.write[vertex_index] = subdiv_vertex_index;
					VertexBones bones;
					bones.b0 = bones_array.write[vertex_index * 4 + 0];
					bones.b1 = bones_array.write[vertex_index * 4 + 1];
					bones.b2 = bones_array.write[vertex_index * 4 + 2];
					bones.b3 = bones_array.write[vertex_index * 4 + 3];
					subdiv_bones_array.write[subdiv_vertex_index] = bones;
					VertexWeights weights;
					weights.w0 = weights_array.write[vertex_index * 4 + 0];
					weights.w1 = weights_array.write[vertex_index * 4 + 1];
					weights.w2 = weights_array.write[vertex_index * 4 + 2];
					weights.w3 = weights_array.write[vertex_index * 4 + 3];
					subdiv_weights_array.write[subdiv_vertex_index] = weights;
				}
				subdiv_vertex_array.write[subdiv_vertex_index] = vertex;
				++vertex_index_out;
			}
		}
		subdiv_vertex_count += vertex_index_out;
	}
	subdiv_vertex_array.resize(subdiv_vertex_count);

	// Add vertex indices
	{
		subdiv_index_array.resize(subdiv_index_count + index_count);
		for (int index = 0; index < index_count; ++index) {
			int subdiv_index = subdiv_index_count + index;
			subdiv_index_array.write[subdiv_index] = surface.mesh_to_subdiv_index_map[index_array[index]];
		}

		subdiv_index_count += index_count;

		int face_count = index_count / 3;

		face_to_surface_index_map.resize(subdiv_face_count + face_count);
		for (int face_index = 0; face_index < face_count; ++face_index) {
			face_to_surface_index_map.write[subdiv_face_count + face_index] = 0;
		}

		subdiv_face_count += face_count;
	}

	// Generate subdivision data
	Vector<int> index_arrays_out;
	Vector<Vector3> normal_arrays_out;
	{
		// Create per-face vertex count
		Vector<int> subdiv_face_vertex_count;
		{
			subdiv_face_vertex_count.resize(subdiv_face_count);
			for (int face_index = 0; face_index < subdiv_face_count; ++face_index) {
				subdiv_face_vertex_count.write[face_index] = 3;
			}
		}
		Vector<int> subdiv_uv_vertex_count;
		{
			subdiv_uv_vertex_count.resize(subdiv_uv_count);
			for (int uv_index = 0; uv_index < subdiv_uv_count; ++uv_index) {
				subdiv_uv_vertex_count.write[uv_index] = 3;
			}
		}
		Vector<int> subdiv_bone_vertex_count;
		{
			subdiv_bone_vertex_count.resize(subdiv_bone_count);
			for (int bone_index = 0; bone_index < subdiv_bone_count; ++bone_index) {
				subdiv_bone_vertex_count.write[bone_index] = 3;
			}
		}
		Vector<int> subdiv_weights_vertex_count;
		{
			subdiv_weights_vertex_count.resize(subdiv_weight_count);
			for (int weight_index = 0; weight_index < subdiv_weight_count; ++weight_index) {
				subdiv_weights_vertex_count.write[weight_index] = 3;
			}
		}
		OpenSubdiv::Far::TopologyDescriptor desc;
		desc.numVertices = subdiv_vertex_count;
		desc.numFaces = subdiv_face_count;
		desc.numVertsPerFace = subdiv_face_vertex_count.ptr();
		desc.vertIndicesPerFace = subdiv_index_array.ptr();
		// const int num_channels = 3;
		// const int channel_uv = 0;
		// const int channel_bones = 1;
		// const int channel_weights = 2;
		// Descriptor::FVarChannel channels[num_channels];
		// channels[channel_uv].numValues = subdiv_uv_count;
		// channels[channel_uv].valueIndices = subdiv_uv_vertex_count.ptr();
		// channels[channel_bones].numValues = subdiv_bone_count;
		// channels[channel_bones].valueIndices = subdiv_bone_vertex_count.ptr();
		// channels[channel_weights].numValues = subdiv_weight_count;
		// channels[channel_weights].valueIndices = subdiv_weights_vertex_count.ptr();

		// desc.numFVarChannels = num_channels;
		// desc.fvarChannels = channels;

		OpenSubdiv::Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_CATMARK;

		OpenSubdiv::Sdc::Options options;
		options.SetVtxBoundaryInterpolation(OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
		options.SetFVarLinearInterpolation(OpenSubdiv::Sdc::Options::FVAR_LINEAR_NONE);
		options.SetCreasingMethod(OpenSubdiv::Sdc::Options::CREASE_UNIFORM);
		options.SetTriangleSubdivision(OpenSubdiv::Sdc::Options::TRI_SUB_SMOOTH);

		OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Options create_options(type, options);

		OpenSubdiv::Far::TopologyRefiner *refiner = OpenSubdiv::Far::TopologyRefinerFactory<Descriptor>::Create(desc, create_options);
		ERR_FAIL_COND_V(!refiner, p_mesh_arrays);

		OpenSubdiv::Far::TopologyRefiner::UniformOptions refine_options(p_level);
		refine_options.fullTopologyInLastLevel = true;
		refiner->RefineUniform(refine_options);

		subdiv_vertex_count = refiner->GetNumVerticesTotal();
		// subdiv_uv_count = refiner->GetNumFVarValuesTotal(channel_uv);
		// subdiv_bone_count = refiner->GetNumFVarValuesTotal(channel_bones);
		// subdiv_weight_count = refiner->GetNumFVarValuesTotal(channel_weights);
		// Create subdivision vertices
		{
			subdiv_vertex_array.resize(subdiv_vertex_count);
			subdiv_uv_array.resize(subdiv_uv_count);
			// Interpolate vertex primvar data
			OpenSubdiv::Far::PrimvarRefiner primvar_refiner(*refiner);

			Vertex *src = (Vertex *)subdiv_vertex_array.ptrw();
			VertexUV *src_uv = (VertexUV *)subdiv_uv_array.ptrw();
			VertexBones *src_bones = (VertexBones *)subdiv_bones_array.ptrw();
			VertexWeights *src_weights = (VertexWeights *)subdiv_weights_array.ptrw();
			for (int level = 0; level < p_level; ++level) {
				Vertex *dst = src + refiner->GetLevel(level).GetNumVertices();
				primvar_refiner.Interpolate(level + 1, src, dst);
				src = dst;
				// VertexUV *dst_uv = src_uv + refiner->GetLevel(level).GetNumFVarValues(channel_uv);
				// primvar_refiner.InterpolateFaceVarying(level + 1, src_uv, dst_uv, channel_uv);
				// src_uv = dst_uv;
				// if (!bones_array.size()) {
				// 	continue;
				// }
				// VertexBones *dst_bones = src_bones + refiner->GetLevel(level).GetNumFVarValues(channel_bones);
				// primvar_refiner.InterpolateFaceVarying(level + 1, src_bones, dst_bones, channel_bones);
				// src_bones = dst_bones;
				// VertexWeights *dst_weights = src_weights + refiner->GetLevel(level).GetNumFVarValues(channel_weights);
				// primvar_refiner.InterpolateFaceVarying(level + 1, src_weights, dst_weights, channel_weights);
				// src_weights = dst_weights;
			}
		}

		// Create subdivision faces
		{
			OpenSubdiv::Far::TopologyLevel const &last_level = refiner->GetLevel(p_level);
			int face_count_out = last_level.GetNumFaces();

			int vertex_index_offset = subdiv_vertex_count - last_level.GetNumVertices();

			for (int face_index = 0; face_index < face_count_out; ++face_index) {
				int parent_face_index = last_level.GetFaceParentFace(face_index);
				for (int level_index = p_level - 1; level_index > 0; --level_index) {
					OpenSubdiv::Far::TopologyLevel const &prev_level = refiner->GetLevel(level_index);
					parent_face_index = prev_level.GetFaceParentFace(parent_face_index);
				}

				Vector<int> &index_array_surface_out = index_arrays_out;
				OpenSubdiv::Far::ConstIndexArray face_vertices = last_level.GetFaceVertices(face_index);
				ERR_FAIL_COND_V(face_vertices.size() != 4, p_mesh_arrays);

				index_array_surface_out.push_back(vertex_index_offset + face_vertices[0]);
				index_array_surface_out.push_back(vertex_index_offset + face_vertices[1]);
				index_array_surface_out.push_back(vertex_index_offset + face_vertices[2]);

				index_array_surface_out.push_back(vertex_index_offset + face_vertices[0]);
				index_array_surface_out.push_back(vertex_index_offset + face_vertices[2]);
				index_array_surface_out.push_back(vertex_index_offset + face_vertices[3]);
			}
		}
		delete refiner;
	}
	const Vector<int> &index_array_out = index_arrays_out;
	Array subdiv_mesh_arrays;
	subdiv_mesh_arrays.resize(Mesh::ARRAY_MAX);
	subdiv_mesh_arrays[Mesh::ARRAY_VERTEX] = subdiv_vertex_array;
	subdiv_mesh_arrays[Mesh::ARRAY_INDEX] = index_array_out;

	// if (bones_array.size()) {
	// 	Vector<int> bone_out;
	// 	int bone_influences = 4;
	// 	if ((subdiv_bones_array.size() / subdiv_vertex_count) % 8 == 0) {
	// 		bone_influences = 8;
	// 	}
	// 	bone_out.resize(subdiv_vertex_count * bone_influences);
	// 	memcpy(bone_out.ptrw(), subdiv_bones_array.ptrw(), subdiv_bone_count * bone_influences * sizeof(int));
	// 	subdiv_mesh_arrays[Mesh::ARRAY_BONES] = bone_out;
	// 	Vector<float> weight_out;
	// 	weight_out.resize(subdiv_vertex_count * bone_influences);
	// 	memcpy(weight_out.ptrw(), subdiv_weights_array.ptrw(), subdiv_weight_count * bone_influences * sizeof(float));
	// 	subdiv_mesh_arrays[Mesh::ARRAY_WEIGHTS] = weight_out;
	// }
	// subdiv_mesh_arrays[Mesh::ARRAY_TEX_UV] = subdiv_uv_array;

	Ref<SurfaceTool> st;
	st.instance();
	st->create_from_triangle_arrays(subdiv_mesh_arrays);
	// st->generate_normals();
	// st->generate_tangents();
	return st->commit_to_arrays();
}
