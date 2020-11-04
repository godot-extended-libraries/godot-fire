/*************************************************************************/
/*  opensubdiv_subdivision.cpp                                           */
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

#include "opensubdiv_subdivision.h"

#include "far/primvarRefiner.h"
#include "far/topologyDescriptor.h"

#include "scene/resources/surface_tool.h"

using namespace OpenSubdiv;

typedef Far::TopologyDescriptor Descriptor;

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

OpenSubdivMeshSubdivision::OpenSubdivMeshSubdivision() {
	subdiv_mesh = VisualServer::get_singleton()->mesh_create();
	subdiv_vertex_count = 0;
	refiner = NULL;
}

OpenSubdivMeshSubdivision ::~OpenSubdivMeshSubdivision() {
	VisualServer::get_singleton()->free(subdiv_mesh);
	subdiv_mesh = RID();
	source_mesh = RID();

	surface_data.clear();
	subdiv_vertex_count = 0;

	if (refiner) {
		delete refiner;
		refiner = NULL;
	}
}

void OpenSubdivMeshSubdivision::update_subdivision(Ref<Mesh> p_mesh, int p_level) {
	VisualServer::get_singleton()->mesh_clear(subdiv_mesh);

	surface_data.clear();
	subdiv_vertex_count = 0;
	subdiv_uv_count = 0;
	bool has_bones = false;

	if (refiner) {
		delete refiner;
		refiner = NULL;
	}

	ERR_FAIL_COND(p_level <= 0);

	ERR_FAIL_COND(p_mesh.is_null());
	for (int32_t surface_i = 0; surface_i < p_mesh->get_surface_count(); surface_i++) {
		Ref<ArrayMesh> array_mesh = p_mesh;
		if (array_mesh.is_valid() && (array_mesh->surface_get_format(surface_i) & Mesh::ARRAY_FORMAT_TEX_UV) == 0) {
			array_mesh->mesh_unwrap(Transform(), 1.0f);
		}
		if (array_mesh.is_valid() && (array_mesh->surface_get_format(surface_i) & Mesh::ARRAY_FORMAT_BONES)) {
			has_bones = true;
		}
	}
	source_mesh = p_mesh->get_rid();
	ERR_FAIL_COND(!source_mesh.is_valid());

	if (p_mesh->get_blend_shape_count() > 0) {
		ERR_PRINT("Blend shapes are not supported for mesh subdivision.");
	}

	VisualServer *visual_server = VisualServer::get_singleton();

	int surface_count = p_mesh->get_surface_count();
	surface_data.resize(surface_count);

	int subdiv_index_count = 0;

	PoolVector3Array subdiv_vertex_array;
	PoolVector2Array subdiv_uv_array;
	PoolIntArray subdiv_index_array;

	int subdiv_face_count = 0;
	Vector<int> face_to_surface_index_map;

	Map<Vector3, int> vertex_map;

	// Gather all vertices and faces from surfaces
	for (int surface_index = 0; surface_index < surface_count; ++surface_index) {
		SurfaceData &surface = surface_data.write[surface_index];

		ERR_FAIL_COND(Mesh::PRIMITIVE_TRIANGLES != p_mesh->surface_get_primitive_type(surface_index));

		Array mesh_arrays = p_mesh->surface_get_arrays(surface_index);
		PoolVector3Array vertex_array = mesh_arrays[Mesh::ARRAY_VERTEX];
		PoolIntArray index_array = mesh_arrays[Mesh::ARRAY_INDEX];
		PoolVector2Array uv_array = mesh_arrays[Mesh::ARRAY_TEX_UV];

		int index_count = index_array.size();

		// Remove duplicated vertices to link faces properly
		{
			int vertex_source_count = vertex_array.size();

			surface.mesh_to_subdiv_index_map.resize(vertex_source_count);
			subdiv_vertex_array.resize(subdiv_vertex_count + vertex_source_count);
			subdiv_uv_array.resize(subdiv_vertex_count + vertex_source_count);

			PoolVector3Array::Read vertex_array_read = vertex_array.read();
			PoolVector2Array::Read uv_array_read = uv_array.read();
			PoolVector3Array::Write subdiv_vertex_array_write = subdiv_vertex_array.write();
			PoolVector2Array::Write subdiv_uv_array_write = subdiv_uv_array.write();

			int vertex_index_out = 0;
			for (int vertex_index = 0; vertex_index < vertex_source_count; ++vertex_index) {
				const Vector3 &vertex = vertex_array_read[vertex_index];
				Map<Vector3, int>::Element *found_vertex = vertex_map.find(vertex);
				if (found_vertex) {
					surface.mesh_to_subdiv_index_map.write[vertex_index] = found_vertex->value();
				} else {
					int subdiv_vertex_index = subdiv_vertex_count + vertex_index_out;
					vertex_map[vertex] = subdiv_vertex_index;
					if (uv_array.size()) {
						Vector2 uv = uv_array_read[vertex_index];
						surface.mesh_to_subdiv_index_map.write[vertex_index] = subdiv_vertex_index;
						subdiv_uv_array_write[subdiv_vertex_index] = uv;
					}
					subdiv_vertex_array_write[subdiv_vertex_index] = vertex;
					++vertex_index_out;
				}
			}
			if (uv_array.size()) {
				subdiv_uv_count += vertex_index_out;
			}
			subdiv_vertex_count += vertex_index_out;
		}
		subdiv_vertex_array.resize(subdiv_vertex_count);

		// Add vertex indices
		{
			subdiv_index_array.resize(subdiv_index_count + index_count);

			PoolIntArray::Read index_array_read = index_array.read();
			PoolIntArray::Write subdiv_index_array_write = subdiv_index_array.write();
			for (int index = 0; index < index_count; ++index) {
				int subdiv_index = subdiv_index_count + index;
				subdiv_index_array_write[subdiv_index] = surface.mesh_to_subdiv_index_map[index_array_read[index]];
			}

			subdiv_index_count += index_count;

			int face_count = index_count / 3;

			face_to_surface_index_map.resize(subdiv_face_count + face_count);
			for (int face_index = 0; face_index < face_count; ++face_index) {
				face_to_surface_index_map.write[subdiv_face_count + face_index] = surface_index;
			}

			subdiv_face_count += face_count;
		}
	}

	// Generate subdivision data
	Vector<PoolIntArray> index_arrays_out;
	index_arrays_out.resize(surface_count);
	Vector<PoolVector3Array> normal_arrays_out;
	normal_arrays_out.resize(surface_count);
	{
		// Create per-face vertex count
		Vector<int> subdiv_face_vertex_count;
		{
			subdiv_face_vertex_count.resize(subdiv_face_count);
			for (int face_index = 0; face_index < subdiv_face_count; ++face_index) {
				subdiv_face_vertex_count.write[face_index] = 3;
			}
		}

		PoolIntArray::Read subdiv_index_array_read = subdiv_index_array.read();

		Far::TopologyDescriptor desc;
		desc.numVertices = subdiv_vertex_count;
		desc.numFaces = subdiv_face_count;
		desc.numVertsPerFace = subdiv_face_vertex_count.ptr();
		desc.vertIndicesPerFace = subdiv_index_array_read.ptr();
		const int num_channels = 1;
		const int channel_uv = 0;
		Descriptor::FVarChannel channels[num_channels];
		channels[channel_uv].numValues = subdiv_uv_count;
		channels[channel_uv].valueIndices = subdiv_index_array_read.ptr();

		desc.numFVarChannels = num_channels;
		desc.fvarChannels = channels;

		// Create topology refiner
		Sdc::SchemeType type = OpenSubdiv::Sdc::SCHEME_LOOP;

		Sdc::Options options;
		options.SetVtxBoundaryInterpolation(Sdc::Options::VTX_BOUNDARY_EDGE_ONLY);
		options.SetFVarLinearInterpolation(Sdc::Options::FVAR_LINEAR_CORNERS_ONLY);
		options.SetCreasingMethod(Sdc::Options::CREASE_UNIFORM);

		Far::TopologyRefinerFactory<Descriptor>::Options create_options(type, options);

		refiner = Far::TopologyRefinerFactory<Descriptor>::Create(desc, create_options);
		ERR_FAIL_COND(!refiner);

		Far::TopologyRefiner::UniformOptions refine_options(p_level);
		refine_options.fullTopologyInLastLevel = true;
		refiner->RefineUniform(refine_options);

		subdiv_vertex_count = refiner->GetNumVerticesTotal();
		subdiv_uv_count = refiner->GetNumFVarValuesTotal(channel_uv);

		// Create subdivision vertices
		{
			subdiv_vertex_array.resize(subdiv_vertex_count);
			subdiv_uv_array.resize(subdiv_uv_count);

			PoolVector3Array::Write subdiv_vertex_array_write = subdiv_vertex_array.write();
			PoolVector2Array::Write subdiv_uv_array_write = subdiv_uv_array.write();

			// Interpolate vertex primvar data
			Far::PrimvarRefiner primvar_refiner(*refiner);

			Vertex *src = (Vertex *)subdiv_vertex_array_write.ptr();
			VertexUV *src_uv = (VertexUV *)subdiv_uv_array_write.ptr();
			for (int level = 0; level < p_level; ++level) {
				Vertex *dst = src + refiner->GetLevel(level).GetNumVertices();
				primvar_refiner.Interpolate(level + 1, src, dst);
				src = dst;
				VertexUV *dst_uv = src_uv + refiner->GetLevel(level).GetNumFVarValues(channel_uv);
				primvar_refiner.InterpolateFaceVarying(level + 1, src_uv, dst_uv, channel_uv);
				src_uv = dst_uv;
			}
		}

		// Create subdivision faces
		{
			Far::TopologyLevel const &last_level = refiner->GetLevel(p_level);
			int face_count_out = last_level.GetNumFaces();

			int vertex_index_offset = subdiv_vertex_count - last_level.GetNumVertices();

			for (int face_index = 0; face_index < face_count_out; ++face_index) {
				int parent_face_index = last_level.GetFaceParentFace(face_index);
				for (int level_index = p_level - 1; level_index > 0; --level_index) {
					Far::TopologyLevel const &prev_level = refiner->GetLevel(level_index);
					parent_face_index = prev_level.GetFaceParentFace(parent_face_index);
				}

				int surface_index = face_to_surface_index_map[parent_face_index];
				PoolIntArray &index_array_surface_out = index_arrays_out.write[surface_index];
				Far::ConstIndexArray face_vertices = last_level.GetFaceVertices(face_index);
				ERR_FAIL_COND(face_vertices.size() != 3);

				index_array_surface_out.push_back(vertex_index_offset + face_vertices[0]);
				index_array_surface_out.push_back(vertex_index_offset + face_vertices[1]);
				index_array_surface_out.push_back(vertex_index_offset + face_vertices[2]);
			}
		}
	}

	// Create all subdivision surfaces
	for (int surface_index = 0; surface_index < surface_count; ++surface_index) {
		const PoolIntArray &index_array_out = index_arrays_out[surface_index];

		Array subdiv_mesh_arrays;
		subdiv_mesh_arrays.resize(Mesh::ARRAY_MAX);
		subdiv_mesh_arrays[Mesh::ARRAY_VERTEX] = subdiv_vertex_array;
		subdiv_mesh_arrays[Mesh::ARRAY_INDEX] = index_array_out;
		if (has_bones) {
			uint32_t surface_format = p_mesh->surface_get_format(surface_index);
			surface_format &= ~Mesh::ARRAY_COMPRESS_VERTEX;
			surface_format |= Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE;
			visual_server->mesh_add_surface_from_arrays(subdiv_mesh, VisualServer::PRIMITIVE_TRIANGLES, subdiv_mesh_arrays, Array(), surface_format);
			Ref<Material> material = p_mesh->surface_get_material(surface_index);
			visual_server->mesh_surface_set_material(subdiv_mesh, surface_index, material.is_null() ? RID() : material->get_rid());
			return;
		}
		PoolVector2Array uv_array_out;
		uv_array_out.resize(subdiv_uv_count);
		PoolVector2Array::Read subdiv_uv_array_read = subdiv_uv_array.read();
		PoolVector2Array::Write uv_array_out_write = uv_array_out.write();
		for (int uv_index = 0; uv_index < subdiv_uv_count; uv_index++) {
			uv_array_out_write[uv_index] = subdiv_uv_array_read[uv_index];
		}
		subdiv_mesh_arrays[Mesh::ARRAY_TEX_UV] = uv_array_out;

		Ref<SurfaceTool> st;
		st.instance();
		st->begin(Mesh::PRIMITIVE_TRIANGLES);
		st->create_from_triangle_arrays(subdiv_mesh_arrays);
		st->generate_smooth_normals();
		st->generate_tangents();

		uint32_t surface_format = p_mesh->surface_get_format(surface_index);
		surface_format &= ~Mesh::ARRAY_COMPRESS_VERTEX;
		surface_format |= Mesh::ARRAY_FORMAT_NORMAL;
		surface_format |= Mesh::ARRAY_FORMAT_TANGENT;
		surface_format |= Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE;

		// TODO: use mesh_add_surface to share vertex array
		visual_server->mesh_add_surface_from_arrays(subdiv_mesh, VisualServer::PRIMITIVE_TRIANGLES, st->commit_to_arrays(), Array(), surface_format);

		Ref<Material> material = p_mesh->surface_get_material(surface_index);
		visual_server->mesh_surface_set_material(subdiv_mesh, surface_index, material.is_null() ? RID() : material->get_rid());
	}
}

MeshSubdivision *OpenSubdivSubdivisionSystem::create_mesh_subdivision(Ref<Mesh> p_mesh, int p_level) {
	OpenSubdivMeshSubdivision *mesh_subdivision = memnew(OpenSubdivMeshSubdivision);
	mesh_subdivision->update_subdivision(p_mesh, p_level);

	return mesh_subdivision;
}

void OpenSubdivSubdivisionSystem::destroy_mesh_subdivision(MeshSubdivision *p_mesh_subdivision) {
	memdelete(p_mesh_subdivision);
}
