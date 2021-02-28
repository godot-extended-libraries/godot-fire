/*************************************************************************/
/*  renderer_scene_cull.cpp                                              */
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

#include "renderer_scene_cull.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "rendering_server_default.h"
#include "rendering_server_globals.h"

#include <new>

/* CAMERA API */

RID RendererSceneCull::camera_allocate() {
	return camera_owner.allocate_rid();
}
void RendererSceneCull::camera_initialize(RID p_rid) {
	camera_owner.initialize_rid(p_rid, memnew(Camera));
}

void RendererSceneCull::camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->type = Camera::PERSPECTIVE;
	camera->fov = p_fovy_degrees;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RendererSceneCull::camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->type = Camera::ORTHOGONAL;
	camera->size = p_size;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RendererSceneCull::camera_set_frustum(RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->type = Camera::FRUSTUM;
	camera->size = p_size;
	camera->offset = p_offset;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RendererSceneCull::camera_set_transform(RID p_camera, const Transform &p_transform) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->transform = p_transform.orthonormalized();
}

void RendererSceneCull::camera_set_cull_mask(RID p_camera, uint32_t p_layers) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);

	camera->visible_layers = p_layers;
}

void RendererSceneCull::camera_set_environment(RID p_camera, RID p_env) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->env = p_env;
}

void RendererSceneCull::camera_set_camera_effects(RID p_camera, RID p_fx) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->effects = p_fx;
}

void RendererSceneCull::camera_set_use_vertical_aspect(RID p_camera, bool p_enable) {
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);
	camera->vaspect = p_enable;
}

bool RendererSceneCull::is_camera(RID p_camera) const {
	return camera_owner.owns(p_camera);
}

/* SCENARIO API */

void RendererSceneCull::_instance_pair(Instance *p_A, Instance *p_B) {
	RendererSceneCull *self = (RendererSceneCull *)singleton;
	Instance *A = p_A;
	Instance *B = p_B;

	//instance indices are designed so greater always contains lesser
	if (A->base_type > B->base_type) {
		SWAP(A, B); //lesser always first
	}

	if (B->base_type == RS::INSTANCE_LIGHT && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightData *light = static_cast<InstanceLightData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->lights.insert(B);
		light->geometries.insert(A);

		if (geom->can_cast_shadows) {
			light->shadow_dirty = true;
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_LIGHTING_DIRTY;
		}

	} else if (self->geometry_instance_pair_mask & (1 << RS::INSTANCE_REFLECTION_PROBE) && B->base_type == RS::INSTANCE_REFLECTION_PROBE && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->reflection_probes.insert(B);
		reflection_probe->geometries.insert(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_REFLECTION_DIRTY;
		}

	} else if (self->geometry_instance_pair_mask & (1 << RS::INSTANCE_DECAL) && B->base_type == RS::INSTANCE_DECAL && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceDecalData *decal = static_cast<InstanceDecalData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->decals.insert(B);
		decal->geometries.insert(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_DECAL_DIRTY;
		}

	} else if (B->base_type == RS::INSTANCE_LIGHTMAP && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightmapData *lightmap_data = static_cast<InstanceLightmapData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		if (A->dynamic_gi) {
			geom->lightmap_captures.insert(A);
			lightmap_data->geometries.insert(B);

			if (A->scenario && A->array_index >= 0) {
				InstanceData &idata = A->scenario->instance_data[A->array_index];
				idata.flags |= InstanceData::FLAG_LIGHTMAP_CAPTURE;
			}
			((RendererSceneCull *)self)->_instance_queue_update(A, false, false); //need to update capture
		}

	} else if (self->geometry_instance_pair_mask & (1 << RS::INSTANCE_GI_PROBE) && B->base_type == RS::INSTANCE_GI_PROBE && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->gi_probes.insert(B);

		if (A->dynamic_gi) {
			gi_probe->dynamic_geometries.insert(A);
		} else {
			gi_probe->geometries.insert(A);
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_GI_PROBE_DIRTY;
		}

	} else if (B->base_type == RS::INSTANCE_GI_PROBE && A->base_type == RS::INSTANCE_LIGHT) {
		InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(B->base_data);
		gi_probe->lights.insert(A);
	} else if (B->base_type == RS::INSTANCE_PARTICLES_COLLISION && A->base_type == RS::INSTANCE_PARTICLES) {
		InstanceParticlesCollisionData *collision = static_cast<InstanceParticlesCollisionData *>(B->base_data);
		RSG::storage->particles_add_collision(A->base, collision->instance);
	}
}

void RendererSceneCull::_instance_unpair(Instance *p_A, Instance *p_B) {
	RendererSceneCull *self = (RendererSceneCull *)singleton;
	Instance *A = p_A;
	Instance *B = p_B;

	//instance indices are designed so greater always contains lesser
	if (A->base_type > B->base_type) {
		SWAP(A, B); //lesser always first
	}

	if (B->base_type == RS::INSTANCE_LIGHT && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightData *light = static_cast<InstanceLightData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->lights.erase(B);
		light->geometries.erase(A);

		if (geom->can_cast_shadows) {
			light->shadow_dirty = true;
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_LIGHTING_DIRTY;
		}

	} else if (self->geometry_instance_pair_mask & (1 << RS::INSTANCE_REFLECTION_PROBE) && B->base_type == RS::INSTANCE_REFLECTION_PROBE && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->reflection_probes.erase(B);
		reflection_probe->geometries.erase(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_REFLECTION_DIRTY;
		}

	} else if (self->geometry_instance_pair_mask & (1 << RS::INSTANCE_DECAL) && B->base_type == RS::INSTANCE_DECAL && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceDecalData *decal = static_cast<InstanceDecalData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->decals.erase(B);
		decal->geometries.erase(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_DECAL_DIRTY;
		}

	} else if (B->base_type == RS::INSTANCE_LIGHTMAP && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightmapData *lightmap_data = static_cast<InstanceLightmapData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);
		if (A->dynamic_gi) {
			geom->lightmap_captures.erase(B);

			if (geom->lightmap_captures.is_empty() && A->scenario && A->array_index >= 0) {
				InstanceData &idata = A->scenario->instance_data[A->array_index];
				idata.flags &= ~uint32_t(InstanceData::FLAG_LIGHTMAP_CAPTURE);
			}

			lightmap_data->geometries.erase(A);
			((RendererSceneCull *)self)->_instance_queue_update(A, false, false); //need to update capture
		}

	} else if (self->geometry_instance_pair_mask & (1 << RS::INSTANCE_GI_PROBE) && B->base_type == RS::INSTANCE_GI_PROBE && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		geom->gi_probes.erase(B);
		if (A->dynamic_gi) {
			gi_probe->dynamic_geometries.erase(A);
		} else {
			gi_probe->geometries.erase(A);
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData &idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_GI_PROBE_DIRTY;
		}

	} else if (B->base_type == RS::INSTANCE_GI_PROBE && A->base_type == RS::INSTANCE_LIGHT) {
		InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(B->base_data);
		gi_probe->lights.erase(A);
	} else if (B->base_type == RS::INSTANCE_PARTICLES_COLLISION && A->base_type == RS::INSTANCE_PARTICLES) {
		InstanceParticlesCollisionData *collision = static_cast<InstanceParticlesCollisionData *>(B->base_data);
		RSG::storage->particles_remove_collision(A->base, collision->instance);
	}
}

RID RendererSceneCull::scenario_allocate() {
	return scenario_owner.allocate_rid();
}
void RendererSceneCull::scenario_initialize(RID p_rid) {
	Scenario *scenario = memnew(Scenario);
	scenario->self = p_rid;

	scenario->reflection_probe_shadow_atlas = scene_render->shadow_atlas_create();
	scene_render->shadow_atlas_set_size(scenario->reflection_probe_shadow_atlas, 1024); //make enough shadows for close distance, don't bother with rest
	scene_render->shadow_atlas_set_quadrant_subdivision(scenario->reflection_probe_shadow_atlas, 0, 4);
	scene_render->shadow_atlas_set_quadrant_subdivision(scenario->reflection_probe_shadow_atlas, 1, 4);
	scene_render->shadow_atlas_set_quadrant_subdivision(scenario->reflection_probe_shadow_atlas, 2, 4);
	scene_render->shadow_atlas_set_quadrant_subdivision(scenario->reflection_probe_shadow_atlas, 3, 8);
	scenario->reflection_atlas = scene_render->reflection_atlas_create();

	scenario->instance_aabbs.set_page_pool(&instance_aabb_page_pool);
	scenario->instance_data.set_page_pool(&instance_data_page_pool);

	scenario_owner.initialize_rid(p_rid, scenario);
}

void RendererSceneCull::scenario_set_debug(RID p_scenario, RS::ScenarioDebugMode p_debug_mode) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->debug = p_debug_mode;
}

void RendererSceneCull::scenario_set_environment(RID p_scenario, RID p_environment) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->environment = p_environment;
}

void RendererSceneCull::scenario_set_camera_effects(RID p_scenario, RID p_camera_effects) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->camera_effects = p_camera_effects;
}

void RendererSceneCull::scenario_set_fallback_environment(RID p_scenario, RID p_environment) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->fallback_environment = p_environment;
}

void RendererSceneCull::scenario_set_reflection_atlas_size(RID p_scenario, int p_reflection_size, int p_reflection_count) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND(!scenario);
	scene_render->reflection_atlas_set_size(scenario->reflection_atlas, p_reflection_size, p_reflection_count);
}

bool RendererSceneCull::is_scenario(RID p_scenario) const {
	return scenario_owner.owns(p_scenario);
}

RID RendererSceneCull::scenario_get_environment(RID p_scenario) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND_V(!scenario, RID());
	return scenario->environment;
}

/* INSTANCING API */

void RendererSceneCull::_instance_queue_update(Instance *p_instance, bool p_update_aabb, bool p_update_dependencies) {
	if (p_update_aabb) {
		p_instance->update_aabb = true;
	}
	if (p_update_dependencies) {
		p_instance->update_dependencies = true;
	}

	if (p_instance->update_item.in_list()) {
		return;
	}

	_instance_update_list.add(&p_instance->update_item);
}

RID RendererSceneCull::instance_allocate() {
	return instance_owner.allocate_rid();
}
void RendererSceneCull::instance_initialize(RID p_rid) {
	Instance *instance = memnew(Instance);
	instance->self = p_rid;

	instance_owner.initialize_rid(p_rid, instance);
}

void RendererSceneCull::_instance_update_mesh_instance(Instance *p_instance) {
	bool needs_instance = RSG::storage->mesh_needs_instance(p_instance->base, p_instance->skeleton.is_valid());
	if (needs_instance != p_instance->mesh_instance.is_valid()) {
		if (needs_instance) {
			p_instance->mesh_instance = RSG::storage->mesh_instance_create(p_instance->base);

		} else {
			RSG::storage->free(p_instance->mesh_instance);
			p_instance->mesh_instance = RID();
		}

		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);
		scene_render->geometry_instance_set_mesh_instance(geom->geometry_instance, p_instance->mesh_instance);

		if (p_instance->scenario && p_instance->array_index >= 0) {
			InstanceData &idata = p_instance->scenario->instance_data[p_instance->array_index];
			if (p_instance->mesh_instance.is_valid()) {
				idata.flags |= InstanceData::FLAG_USES_MESH_INSTANCE;
			} else {
				idata.flags &= ~uint32_t(InstanceData::FLAG_USES_MESH_INSTANCE);
			}
		}
	}

	if (p_instance->mesh_instance.is_valid()) {
		RSG::storage->mesh_instance_set_skeleton(p_instance->mesh_instance, p_instance->skeleton);
	}
}

void RendererSceneCull::instance_set_base(RID p_instance, RID p_base) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	Scenario *scenario = instance->scenario;

	if (instance->base_type != RS::INSTANCE_NONE) {
		//free anything related to that base

		if (scenario && instance->indexer_id.is_valid()) {
			_unpair_instance(instance);
		}

		if (instance->mesh_instance.is_valid()) {
			RSG::storage->free(instance->mesh_instance);
			instance->mesh_instance = RID();
			// no need to set instance data flag here, as it was freed above
		}

		switch (instance->base_type) {
			case RS::INSTANCE_MESH:
			case RS::INSTANCE_MULTIMESH:
			case RS::INSTANCE_IMMEDIATE:
			case RS::INSTANCE_PARTICLES: {
				InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
				scene_render->geometry_instance_free(geom->geometry_instance);
			} break;
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);

				if (scenario && instance->visible && RSG::storage->light_get_type(instance->base) != RS::LIGHT_DIRECTIONAL && light->bake_mode == RS::LIGHT_BAKE_DYNAMIC) {
					scenario->dynamic_lights.erase(light->instance);
				}

#ifdef DEBUG_ENABLED
				if (light->geometries.size()) {
					ERR_PRINT("BUG, indexing did not unpair geometries from light.");
				}
#endif
				if (scenario && light->D) {
					scenario->directional_lights.erase(light->D);
					light->D = nullptr;
				}
				scene_render->free(light->instance);
			} break;
			case RS::INSTANCE_PARTICLES_COLLISION: {
				InstanceParticlesCollisionData *collision = static_cast<InstanceParticlesCollisionData *>(instance->base_data);
				RSG::storage->free(collision->instance);
			} break;
			case RS::INSTANCE_REFLECTION_PROBE: {
				InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(instance->base_data);
				scene_render->free(reflection_probe->instance);
				if (reflection_probe->update_list.in_list()) {
					reflection_probe_render_list.remove(&reflection_probe->update_list);
				}
			} break;
			case RS::INSTANCE_DECAL: {
				InstanceDecalData *decal = static_cast<InstanceDecalData *>(instance->base_data);
				scene_render->free(decal->instance);

			} break;
			case RS::INSTANCE_LIGHTMAP: {
				InstanceLightmapData *lightmap_data = static_cast<InstanceLightmapData *>(instance->base_data);
				//erase dependencies, since no longer a lightmap
				while (lightmap_data->users.front()) {
					instance_geometry_set_lightmap(lightmap_data->users.front()->get()->self, RID(), Rect2(), 0);
				}
				scene_render->free(lightmap_data->instance);
			} break;
			case RS::INSTANCE_GI_PROBE: {
				InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(instance->base_data);
#ifdef DEBUG_ENABLED
				if (gi_probe->geometries.size()) {
					ERR_PRINT("BUG, indexing did not unpair geometries from GIProbe.");
				}
#endif
#ifdef DEBUG_ENABLED
				if (gi_probe->lights.size()) {
					ERR_PRINT("BUG, indexing did not unpair lights from GIProbe.");
				}
#endif
				if (gi_probe->update_element.in_list()) {
					gi_probe_update_list.remove(&gi_probe->update_element);
				}

				scene_render->free(gi_probe->probe_instance);

			} break;
			default: {
			}
		}

		if (instance->base_data) {
			memdelete(instance->base_data);
			instance->base_data = nullptr;
		}

		instance->materials.clear();
	}

	instance->base_type = RS::INSTANCE_NONE;
	instance->base = RID();

	if (p_base.is_valid()) {
		instance->base_type = RSG::storage->get_base_type(p_base);
		ERR_FAIL_COND(instance->base_type == RS::INSTANCE_NONE);

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = memnew(InstanceLightData);

				if (scenario && RSG::storage->light_get_type(p_base) == RS::LIGHT_DIRECTIONAL) {
					light->D = scenario->directional_lights.push_back(instance);
				}

				light->instance = scene_render->light_instance_create(p_base);

				instance->base_data = light;
			} break;
			case RS::INSTANCE_MESH:
			case RS::INSTANCE_MULTIMESH:
			case RS::INSTANCE_IMMEDIATE:
			case RS::INSTANCE_PARTICLES: {
				InstanceGeometryData *geom = memnew(InstanceGeometryData);
				instance->base_data = geom;
				geom->geometry_instance = scene_render->geometry_instance_create(p_base);

				scene_render->geometry_instance_set_skeleton(geom->geometry_instance, instance->skeleton);
				scene_render->geometry_instance_set_material_override(geom->geometry_instance, instance->material_override);
				scene_render->geometry_instance_set_surface_materials(geom->geometry_instance, instance->materials);
				scene_render->geometry_instance_set_transform(geom->geometry_instance, instance->transform, instance->aabb, instance->transformed_aabb);
				scene_render->geometry_instance_set_layer_mask(geom->geometry_instance, instance->layer_mask);
				scene_render->geometry_instance_set_lod_bias(geom->geometry_instance, instance->lod_bias);
				scene_render->geometry_instance_set_use_baked_light(geom->geometry_instance, instance->baked_light);
				scene_render->geometry_instance_set_use_dynamic_gi(geom->geometry_instance, instance->dynamic_gi);
				scene_render->geometry_instance_set_cast_double_sided_shadows(geom->geometry_instance, instance->cast_shadows == RS::SHADOW_CASTING_SETTING_DOUBLE_SIDED);
				scene_render->geometry_instance_set_use_lightmap(geom->geometry_instance, RID(), instance->lightmap_uv_scale, instance->lightmap_slice_index);
				if (instance->lightmap_sh.size() == 9) {
					scene_render->geometry_instance_set_lightmap_capture(geom->geometry_instance, instance->lightmap_sh.ptr());
				}

			} break;
			case RS::INSTANCE_PARTICLES_COLLISION: {
				InstanceParticlesCollisionData *collision = memnew(InstanceParticlesCollisionData);
				collision->instance = RSG::storage->particles_collision_instance_create(p_base);
				RSG::storage->particles_collision_instance_set_active(collision->instance, instance->visible);
				instance->base_data = collision;
			} break;
			case RS::INSTANCE_REFLECTION_PROBE: {
				InstanceReflectionProbeData *reflection_probe = memnew(InstanceReflectionProbeData);
				reflection_probe->owner = instance;
				instance->base_data = reflection_probe;

				reflection_probe->instance = scene_render->reflection_probe_instance_create(p_base);
			} break;
			case RS::INSTANCE_DECAL: {
				InstanceDecalData *decal = memnew(InstanceDecalData);
				decal->owner = instance;
				instance->base_data = decal;

				decal->instance = scene_render->decal_instance_create(p_base);
			} break;
			case RS::INSTANCE_LIGHTMAP: {
				InstanceLightmapData *lightmap_data = memnew(InstanceLightmapData);
				instance->base_data = lightmap_data;
				lightmap_data->instance = scene_render->lightmap_instance_create(p_base);
			} break;
			case RS::INSTANCE_GI_PROBE: {
				InstanceGIProbeData *gi_probe = memnew(InstanceGIProbeData);
				instance->base_data = gi_probe;
				gi_probe->owner = instance;

				if (scenario && !gi_probe->update_element.in_list()) {
					gi_probe_update_list.add(&gi_probe->update_element);
				}

				gi_probe->probe_instance = scene_render->gi_probe_instance_create(p_base);

			} break;
			default: {
			}
		}

		instance->base = p_base;

		if (instance->base_type == RS::INSTANCE_MESH) {
			_instance_update_mesh_instance(instance);
		}

		//forcefully update the dependency now, so if for some reason it gets removed, we can immediately clear it
		RSG::storage->base_update_dependency(p_base, &instance->dependency_tracker);
	}

	_instance_queue_update(instance, true, true);
}

void RendererSceneCull::instance_set_scenario(RID p_instance, RID p_scenario) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->scenario) {
		instance->scenario->instances.remove(&instance->scenario_item);

		if (instance->indexer_id.is_valid()) {
			_unpair_instance(instance);
		}

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);
#ifdef DEBUG_ENABLED
				if (light->geometries.size()) {
					ERR_PRINT("BUG, indexing did not unpair geometries from light.");
				}
#endif
				if (light->D) {
					instance->scenario->directional_lights.erase(light->D);
					light->D = nullptr;
				}
			} break;
			case RS::INSTANCE_REFLECTION_PROBE: {
				InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(instance->base_data);
				scene_render->reflection_probe_release_atlas_index(reflection_probe->instance);

			} break;
			case RS::INSTANCE_PARTICLES_COLLISION: {
				heightfield_particle_colliders_update_list.erase(instance);
			} break;
			case RS::INSTANCE_GI_PROBE: {
				InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(instance->base_data);

#ifdef DEBUG_ENABLED
				if (gi_probe->geometries.size()) {
					ERR_PRINT("BUG, indexing did not unpair geometries from GIProbe.");
				}
#endif
#ifdef DEBUG_ENABLED
				if (gi_probe->lights.size()) {
					ERR_PRINT("BUG, indexing did not unpair lights from GIProbe.");
				}
#endif

				if (gi_probe->update_element.in_list()) {
					gi_probe_update_list.remove(&gi_probe->update_element);
				}
			} break;
			default: {
			}
		}

		instance->scenario = nullptr;
	}

	if (p_scenario.is_valid()) {
		Scenario *scenario = scenario_owner.getornull(p_scenario);
		ERR_FAIL_COND(!scenario);

		instance->scenario = scenario;

		scenario->instances.add(&instance->scenario_item);

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);

				if (RSG::storage->light_get_type(instance->base) == RS::LIGHT_DIRECTIONAL) {
					light->D = scenario->directional_lights.push_back(instance);
				}
			} break;
			case RS::INSTANCE_GI_PROBE: {
				InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(instance->base_data);
				if (!gi_probe->update_element.in_list()) {
					gi_probe_update_list.add(&gi_probe->update_element);
				}
			} break;
			default: {
			}
		}

		_instance_queue_update(instance, true, true);
	}
}

void RendererSceneCull::instance_set_layer_mask(RID p_instance, uint32_t p_mask) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	instance->layer_mask = p_mask;
	if (instance->scenario && instance->array_index >= 0) {
		instance->scenario->instance_data[instance->array_index].layer_mask = p_mask;
	}

	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
		scene_render->geometry_instance_set_layer_mask(geom->geometry_instance, p_mask);
	}
}

void RendererSceneCull::instance_set_transform(RID p_instance, const Transform &p_transform) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->transform == p_transform) {
		return; //must be checked to avoid worst evil
	}

#ifdef DEBUG_ENABLED

	for (int i = 0; i < 4; i++) {
		const Vector3 &v = i < 3 ? p_transform.basis.elements[i] : p_transform.origin;
		ERR_FAIL_COND(Math::is_inf(v.x));
		ERR_FAIL_COND(Math::is_nan(v.x));
		ERR_FAIL_COND(Math::is_inf(v.y));
		ERR_FAIL_COND(Math::is_nan(v.y));
		ERR_FAIL_COND(Math::is_inf(v.z));
		ERR_FAIL_COND(Math::is_nan(v.z));
	}

#endif
	instance->transform = p_transform;
	_instance_queue_update(instance, true);
}

void RendererSceneCull::instance_attach_object_instance_id(RID p_instance, ObjectID p_id) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	instance->object_id = p_id;
}

void RendererSceneCull::instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->update_item.in_list()) {
		_update_dirty_instance(instance);
	}

	if (instance->mesh_instance.is_valid()) {
		RSG::storage->mesh_instance_set_blend_shape_weight(instance->mesh_instance, p_shape, p_weight);
	}
}

void RendererSceneCull::instance_set_surface_material(RID p_instance, int p_surface, RID p_material) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->base_type == RS::INSTANCE_MESH) {
		//may not have been updated yet, may also have not been set yet. When updated will be correcte, worst case
		instance->materials.resize(MAX(p_surface + 1, RSG::storage->mesh_get_surface_count(instance->base)));
	}

	ERR_FAIL_INDEX(p_surface, instance->materials.size());

	instance->materials.write[p_surface] = p_material;

	_instance_queue_update(instance, false, true);
}

void RendererSceneCull::instance_set_visible(RID p_instance, bool p_visible) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->visible == p_visible) {
		return;
	}

	instance->visible = p_visible;

	if (p_visible) {
		if (instance->scenario != nullptr) {
			_instance_queue_update(instance, true, false);
		}
	} else if (instance->indexer_id.is_valid()) {
		_unpair_instance(instance);
	}

	if (instance->base_type == RS::INSTANCE_LIGHT) {
		InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);
		if (instance->scenario && RSG::storage->light_get_type(instance->base) != RS::LIGHT_DIRECTIONAL && light->bake_mode == RS::LIGHT_BAKE_DYNAMIC) {
			if (p_visible) {
				instance->scenario->dynamic_lights.push_back(light->instance);
			} else {
				instance->scenario->dynamic_lights.erase(light->instance);
			}
		}
	}

	if (instance->base_type == RS::INSTANCE_PARTICLES_COLLISION) {
		InstanceParticlesCollisionData *collision = static_cast<InstanceParticlesCollisionData *>(instance->base_data);
		RSG::storage->particles_collision_instance_set_active(collision->instance, p_visible);
	}
}

inline bool is_geometry_instance(RenderingServer::InstanceType p_type) {
	return p_type == RS::INSTANCE_MESH || p_type == RS::INSTANCE_MULTIMESH || p_type == RS::INSTANCE_PARTICLES || p_type == RS::INSTANCE_IMMEDIATE;
}

void RendererSceneCull::instance_set_custom_aabb(RID p_instance, AABB p_aabb) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);
	ERR_FAIL_COND(!is_geometry_instance(instance->base_type));

	if (p_aabb != AABB()) {
		// Set custom AABB
		if (instance->custom_aabb == nullptr) {
			instance->custom_aabb = memnew(AABB);
		}
		*instance->custom_aabb = p_aabb;

	} else {
		// Clear custom AABB
		if (instance->custom_aabb != nullptr) {
			memdelete(instance->custom_aabb);
			instance->custom_aabb = nullptr;
		}
	}

	if (instance->scenario) {
		_instance_queue_update(instance, true, false);
	}
}

void RendererSceneCull::instance_attach_skeleton(RID p_instance, RID p_skeleton) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->skeleton == p_skeleton) {
		return;
	}

	instance->skeleton = p_skeleton;

	if (p_skeleton.is_valid()) {
		//update the dependency now, so if cleared, we remove it
		RSG::storage->skeleton_update_dependency(p_skeleton, &instance->dependency_tracker);
	}

	_instance_queue_update(instance, true, true);

	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		_instance_update_mesh_instance(instance);

		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
		scene_render->geometry_instance_set_skeleton(geom->geometry_instance, p_skeleton);
	}
}

void RendererSceneCull::instance_set_exterior(RID p_instance, bool p_enabled) {
}

void RendererSceneCull::instance_set_extra_visibility_margin(RID p_instance, real_t p_margin) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	instance->extra_margin = p_margin;
	_instance_queue_update(instance, true, false);
}

Vector<ObjectID> RendererSceneCull::instances_cull_aabb(const AABB &p_aabb, RID p_scenario) const {
	Vector<ObjectID> instances;
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND_V(!scenario, instances);

	const_cast<RendererSceneCull *>(this)->update_dirty_instances(); // check dirty instances before culling

	struct CullAABB {
		Vector<ObjectID> instances;
		_FORCE_INLINE_ bool operator()(void *p_data) {
			Instance *p_instance = (Instance *)p_data;
			if (!p_instance->object_id.is_null()) {
				instances.push_back(p_instance->object_id);
			}
			return false;
		}
	};

	CullAABB cull_aabb;
	scenario->indexers[Scenario::INDEXER_GEOMETRY].aabb_query(p_aabb, cull_aabb);
	scenario->indexers[Scenario::INDEXER_VOLUMES].aabb_query(p_aabb, cull_aabb);
	return cull_aabb.instances;
}

Vector<ObjectID> RendererSceneCull::instances_cull_ray(const Vector3 &p_from, const Vector3 &p_to, RID p_scenario) const {
	Vector<ObjectID> instances;
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND_V(!scenario, instances);
	const_cast<RendererSceneCull *>(this)->update_dirty_instances(); // check dirty instances before culling

	struct CullRay {
		Vector<ObjectID> instances;
		_FORCE_INLINE_ bool operator()(void *p_data) {
			Instance *p_instance = (Instance *)p_data;
			if (!p_instance->object_id.is_null()) {
				instances.push_back(p_instance->object_id);
			}
			return false;
		}
	};

	CullRay cull_ray;
	scenario->indexers[Scenario::INDEXER_GEOMETRY].ray_query(p_from, p_to, cull_ray);
	scenario->indexers[Scenario::INDEXER_VOLUMES].ray_query(p_from, p_to, cull_ray);
	return cull_ray.instances;
}

Vector<ObjectID> RendererSceneCull::instances_cull_convex(const Vector<Plane> &p_convex, RID p_scenario) const {
	Vector<ObjectID> instances;
	Scenario *scenario = scenario_owner.getornull(p_scenario);
	ERR_FAIL_COND_V(!scenario, instances);
	const_cast<RendererSceneCull *>(this)->update_dirty_instances(); // check dirty instances before culling

	Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&p_convex[0], p_convex.size());

	struct CullConvex {
		Vector<ObjectID> instances;
		_FORCE_INLINE_ bool operator()(void *p_data) {
			Instance *p_instance = (Instance *)p_data;
			if (!p_instance->object_id.is_null()) {
				instances.push_back(p_instance->object_id);
			}
			return false;
		}
	};

	CullConvex cull_convex;
	scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(p_convex.ptr(), p_convex.size(), points.ptr(), points.size(), cull_convex);
	scenario->indexers[Scenario::INDEXER_VOLUMES].convex_query(p_convex.ptr(), p_convex.size(), points.ptr(), points.size(), cull_convex);
	return cull_convex.instances;
}

void RendererSceneCull::instance_geometry_set_flag(RID p_instance, RS::InstanceFlags p_flags, bool p_enabled) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	//ERR_FAIL_COND(((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK));

	switch (p_flags) {
		case RS::INSTANCE_FLAG_USE_BAKED_LIGHT: {
			instance->baked_light = p_enabled;

			if (instance->scenario && instance->array_index >= 0) {
				InstanceData &idata = instance->scenario->instance_data[instance->array_index];
				if (instance->baked_light) {
					idata.flags |= InstanceData::FLAG_USES_BAKED_LIGHT;
				} else {
					idata.flags &= ~uint32_t(InstanceData::FLAG_USES_BAKED_LIGHT);
				}
			}

			if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
				InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
				scene_render->geometry_instance_set_use_baked_light(geom->geometry_instance, p_enabled);
			}

		} break;
		case RS::INSTANCE_FLAG_USE_DYNAMIC_GI: {
			if (p_enabled == instance->dynamic_gi) {
				//bye, redundant
				return;
			}

			if (instance->indexer_id.is_valid()) {
				_unpair_instance(instance);
				_instance_queue_update(instance, true, true);
			}

			//once out of octree, can be changed
			instance->dynamic_gi = p_enabled;

			if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
				InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
				scene_render->geometry_instance_set_use_dynamic_gi(geom->geometry_instance, p_enabled);
			}

		} break;
		case RS::INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE: {
			instance->redraw_if_visible = p_enabled;

			if (instance->scenario && instance->array_index >= 0) {
				InstanceData &idata = instance->scenario->instance_data[instance->array_index];
				if (instance->redraw_if_visible) {
					idata.flags |= InstanceData::FLAG_REDRAW_IF_VISIBLE;
				} else {
					idata.flags &= ~uint32_t(InstanceData::FLAG_REDRAW_IF_VISIBLE);
				}
			}

		} break;
		default: {
		}
	}
}

void RendererSceneCull::instance_geometry_set_cast_shadows_setting(RID p_instance, RS::ShadowCastingSetting p_shadow_casting_setting) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	instance->cast_shadows = p_shadow_casting_setting;

	if (instance->scenario && instance->array_index >= 0) {
		InstanceData &idata = instance->scenario->instance_data[instance->array_index];

		if (instance->cast_shadows != RS::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS;
		} else {
			idata.flags &= ~uint32_t(InstanceData::FLAG_CAST_SHADOWS);
		}

		if (instance->cast_shadows == RS::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS_ONLY;
		} else {
			idata.flags &= ~uint32_t(InstanceData::FLAG_CAST_SHADOWS_ONLY);
		}
	}

	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
		scene_render->geometry_instance_set_cast_double_sided_shadows(geom->geometry_instance, instance->cast_shadows == RS::SHADOW_CASTING_SETTING_DOUBLE_SIDED);
	}

	_instance_queue_update(instance, false, true);
}

void RendererSceneCull::instance_geometry_set_material_override(RID p_instance, RID p_material) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	instance->material_override = p_material;
	_instance_queue_update(instance, false, true);

	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
		scene_render->geometry_instance_set_material_override(geom->geometry_instance, p_material);
	}
}

void RendererSceneCull::instance_geometry_set_draw_range(RID p_instance, float p_min, float p_max, float p_min_margin, float p_max_margin) {
}

void RendererSceneCull::instance_geometry_set_as_instance_lod(RID p_instance, RID p_as_lod_of_instance) {
}

void RendererSceneCull::instance_geometry_set_lightmap(RID p_instance, RID p_lightmap, const Rect2 &p_lightmap_uv_scale, int p_slice_index) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->lightmap) {
		InstanceLightmapData *lightmap_data = static_cast<InstanceLightmapData *>(((Instance *)instance->lightmap)->base_data);
		lightmap_data->users.erase(instance);
		instance->lightmap = nullptr;
	}

	Instance *lightmap_instance = instance_owner.getornull(p_lightmap);

	instance->lightmap = lightmap_instance;
	instance->lightmap_uv_scale = p_lightmap_uv_scale;
	instance->lightmap_slice_index = p_slice_index;

	RID lightmap_instance_rid;

	if (lightmap_instance) {
		InstanceLightmapData *lightmap_data = static_cast<InstanceLightmapData *>(lightmap_instance->base_data);
		lightmap_data->users.insert(instance);
		lightmap_instance_rid = lightmap_data->instance;
	}

	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
		scene_render->geometry_instance_set_use_lightmap(geom->geometry_instance, lightmap_instance_rid, p_lightmap_uv_scale, p_slice_index);
	}
}

void RendererSceneCull::instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	instance->lod_bias = p_lod_bias;

	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
		scene_render->geometry_instance_set_lod_bias(geom->geometry_instance, p_lod_bias);
	}
}

void RendererSceneCull::instance_geometry_set_shader_parameter(RID p_instance, const StringName &p_parameter, const Variant &p_value) {
	Instance *instance = instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	Map<StringName, Instance::InstanceShaderParameter>::Element *E = instance->instance_shader_parameters.find(p_parameter);

	if (!E) {
		Instance::InstanceShaderParameter isp;
		isp.index = -1;
		isp.info = PropertyInfo();
		isp.value = p_value;
		instance->instance_shader_parameters[p_parameter] = isp;
	} else {
		E->get().value = p_value;
		if (E->get().index >= 0 && instance->instance_allocated_shader_parameters) {
			//update directly
			RSG::storage->global_variables_instance_update(p_instance, E->get().index, p_value);
		}
	}
}

Variant RendererSceneCull::instance_geometry_get_shader_parameter(RID p_instance, const StringName &p_parameter) const {
	const Instance *instance = const_cast<RendererSceneCull *>(this)->instance_owner.getornull(p_instance);
	ERR_FAIL_COND_V(!instance, Variant());

	if (instance->instance_shader_parameters.has(p_parameter)) {
		return instance->instance_shader_parameters[p_parameter].value;
	}
	return Variant();
}

Variant RendererSceneCull::instance_geometry_get_shader_parameter_default_value(RID p_instance, const StringName &p_parameter) const {
	const Instance *instance = const_cast<RendererSceneCull *>(this)->instance_owner.getornull(p_instance);
	ERR_FAIL_COND_V(!instance, Variant());

	if (instance->instance_shader_parameters.has(p_parameter)) {
		return instance->instance_shader_parameters[p_parameter].default_value;
	}
	return Variant();
}

void RendererSceneCull::instance_geometry_get_shader_parameter_list(RID p_instance, List<PropertyInfo> *p_parameters) const {
	const Instance *instance = const_cast<RendererSceneCull *>(this)->instance_owner.getornull(p_instance);
	ERR_FAIL_COND(!instance);

	const_cast<RendererSceneCull *>(this)->update_dirty_instances();

	Vector<StringName> names;
	for (Map<StringName, Instance::InstanceShaderParameter>::Element *E = instance->instance_shader_parameters.front(); E; E = E->next()) {
		names.push_back(E->key());
	}
	names.sort_custom<StringName::AlphCompare>();
	for (int i = 0; i < names.size(); i++) {
		PropertyInfo pinfo = instance->instance_shader_parameters[names[i]].info;
		p_parameters->push_back(pinfo);
	}
}

void RendererSceneCull::_update_instance(Instance *p_instance) {
	p_instance->version++;

	if (p_instance->base_type == RS::INSTANCE_LIGHT) {
		InstanceLightData *light = static_cast<InstanceLightData *>(p_instance->base_data);

		scene_render->light_instance_set_transform(light->instance, p_instance->transform);
		scene_render->light_instance_set_aabb(light->instance, p_instance->transform.xform(p_instance->aabb));
		light->shadow_dirty = true;

		RS::LightBakeMode bake_mode = RSG::storage->light_get_bake_mode(p_instance->base);
		if (RSG::storage->light_get_type(p_instance->base) != RS::LIGHT_DIRECTIONAL && bake_mode != light->bake_mode) {
			if (p_instance->visible && p_instance->scenario && light->bake_mode == RS::LIGHT_BAKE_DYNAMIC) {
				p_instance->scenario->dynamic_lights.erase(light->instance);
			}

			light->bake_mode = bake_mode;

			if (p_instance->visible && p_instance->scenario && light->bake_mode == RS::LIGHT_BAKE_DYNAMIC) {
				p_instance->scenario->dynamic_lights.push_back(light->instance);
			}
		}

		uint32_t max_sdfgi_cascade = RSG::storage->light_get_max_sdfgi_cascade(p_instance->base);
		if (light->max_sdfgi_cascade != max_sdfgi_cascade) {
			light->max_sdfgi_cascade = max_sdfgi_cascade; //should most likely make sdfgi dirty in scenario
		}
	} else if (p_instance->base_type == RS::INSTANCE_REFLECTION_PROBE) {
		InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(p_instance->base_data);

		scene_render->reflection_probe_instance_set_transform(reflection_probe->instance, p_instance->transform);

		if (p_instance->scenario && p_instance->array_index >= 0) {
			InstanceData &idata = p_instance->scenario->instance_data[p_instance->array_index];
			idata.flags |= InstanceData::FLAG_REFLECTION_PROBE_DIRTY;
		}
	} else if (p_instance->base_type == RS::INSTANCE_DECAL) {
		InstanceDecalData *decal = static_cast<InstanceDecalData *>(p_instance->base_data);

		scene_render->decal_instance_set_transform(decal->instance, p_instance->transform);
	} else if (p_instance->base_type == RS::INSTANCE_LIGHTMAP) {
		InstanceLightmapData *lightmap = static_cast<InstanceLightmapData *>(p_instance->base_data);

		scene_render->lightmap_instance_set_transform(lightmap->instance, p_instance->transform);
	} else if (p_instance->base_type == RS::INSTANCE_GI_PROBE) {
		InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(p_instance->base_data);

		scene_render->gi_probe_instance_set_transform_to_data(gi_probe->probe_instance, p_instance->transform);
	} else if (p_instance->base_type == RS::INSTANCE_PARTICLES) {
		RSG::storage->particles_set_emission_transform(p_instance->base, p_instance->transform);
	} else if (p_instance->base_type == RS::INSTANCE_PARTICLES_COLLISION) {
		InstanceParticlesCollisionData *collision = static_cast<InstanceParticlesCollisionData *>(p_instance->base_data);

		//remove materials no longer used and un-own them
		if (RSG::storage->particles_collision_is_heightfield(p_instance->base)) {
			heightfield_particle_colliders_update_list.insert(p_instance);
		}
		RSG::storage->particles_collision_instance_set_transform(collision->instance, p_instance->transform);
	}

	if (p_instance->aabb.has_no_surface()) {
		return;
	}

	if (p_instance->base_type == RS::INSTANCE_LIGHTMAP) {
		//if this moved, update the captured objects
		InstanceLightmapData *lightmap_data = static_cast<InstanceLightmapData *>(p_instance->base_data);
		//erase dependencies, since no longer a lightmap

		for (Set<Instance *>::Element *E = lightmap_data->geometries.front(); E; E = E->next()) {
			Instance *geom = E->get();
			_instance_queue_update(geom, true, false);
		}
	}

	AABB new_aabb;
	new_aabb = p_instance->transform.xform(p_instance->aabb);
	p_instance->transformed_aabb = new_aabb;

	if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);
		//make sure lights are updated if it casts shadow

		if (geom->can_cast_shadows) {
			for (Set<Instance *>::Element *E = geom->lights.front(); E; E = E->next()) {
				InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
				light->shadow_dirty = true;
			}
		}

		if (!p_instance->lightmap && geom->lightmap_captures.size()) {
			//affected by lightmap captures, must update capture info!
			_update_instance_lightmap_captures(p_instance);
		} else {
			if (!p_instance->lightmap_sh.is_empty()) {
				p_instance->lightmap_sh.clear(); //don't need SH
				p_instance->lightmap_target_sh.clear(); //don't need SH
				scene_render->geometry_instance_set_lightmap_capture(geom->geometry_instance, nullptr);
			}
		}

		scene_render->geometry_instance_set_transform(geom->geometry_instance, p_instance->transform, p_instance->aabb, p_instance->transformed_aabb);
	}

	// note: we had to remove is equal approx check here, it meant that det == 0.000004 won't work, which is the case for some of our scenes.
	if (p_instance->scenario == nullptr || !p_instance->visible || p_instance->transform.basis.determinant() == 0) {
		p_instance->prev_transformed_aabb = p_instance->transformed_aabb;
		return;
	}

	//quantize to improve moving object performance
	AABB bvh_aabb = p_instance->transformed_aabb;

	if (p_instance->indexer_id.is_valid() && bvh_aabb != p_instance->prev_transformed_aabb) {
		//assume motion, see if bounds need to be quantized
		AABB motion_aabb = bvh_aabb.merge(p_instance->prev_transformed_aabb);
		float motion_longest_axis = motion_aabb.get_longest_axis_size();
		float longest_axis = p_instance->transformed_aabb.get_longest_axis_size();

		if (motion_longest_axis < longest_axis * 2) {
			//moved but not a lot, use motion aabb quantizing
			float quantize_size = Math::pow(2.0, Math::ceil(Math::log(motion_longest_axis) / Math::log(2.0))) * 0.5; //one fifth
			bvh_aabb.quantize(quantize_size);
		}
	}

	if (!p_instance->indexer_id.is_valid()) {
		if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
			p_instance->indexer_id = p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY].insert(bvh_aabb, p_instance);
		} else {
			p_instance->indexer_id = p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES].insert(bvh_aabb, p_instance);
		}

		p_instance->array_index = p_instance->scenario->instance_data.size();
		InstanceData idata;
		idata.instance = p_instance;
		idata.layer_mask = p_instance->layer_mask;
		idata.flags = p_instance->base_type; //changing it means de-indexing, so this never needs to be changed later
		idata.base_rid = p_instance->base;
		switch (p_instance->base_type) {
			case RS::INSTANCE_MESH:
			case RS::INSTANCE_MULTIMESH:
			case RS::INSTANCE_IMMEDIATE:
			case RS::INSTANCE_PARTICLES: {
				idata.instance_geometry = static_cast<InstanceGeometryData *>(p_instance->base_data)->geometry_instance;
			} break;
			case RS::INSTANCE_LIGHT: {
				idata.instance_data_rid = static_cast<InstanceLightData *>(p_instance->base_data)->instance.get_id();
			} break;
			case RS::INSTANCE_REFLECTION_PROBE: {
				idata.instance_data_rid = static_cast<InstanceReflectionProbeData *>(p_instance->base_data)->instance.get_id();
			} break;
			case RS::INSTANCE_DECAL: {
				idata.instance_data_rid = static_cast<InstanceDecalData *>(p_instance->base_data)->instance.get_id();
			} break;
			case RS::INSTANCE_LIGHTMAP: {
				idata.instance_data_rid = static_cast<InstanceLightmapData *>(p_instance->base_data)->instance.get_id();
			} break;
			case RS::INSTANCE_GI_PROBE: {
				idata.instance_data_rid = static_cast<InstanceGIProbeData *>(p_instance->base_data)->probe_instance.get_id();
			} break;
			default: {
			}
		}

		if (p_instance->base_type == RS::INSTANCE_REFLECTION_PROBE) {
			//always dirty when added
			idata.flags |= InstanceData::FLAG_REFLECTION_PROBE_DIRTY;
		}
		if (p_instance->cast_shadows != RS::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS;
		}
		if (p_instance->cast_shadows == RS::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS_ONLY;
		}
		if (p_instance->redraw_if_visible) {
			idata.flags |= InstanceData::FLAG_REDRAW_IF_VISIBLE;
		}
		// dirty flags should not be set here, since no pairing has happened
		if (p_instance->baked_light) {
			idata.flags |= InstanceData::FLAG_USES_BAKED_LIGHT;
		}
		if (p_instance->mesh_instance.is_valid()) {
			idata.flags |= InstanceData::FLAG_USES_MESH_INSTANCE;
		}

		p_instance->scenario->instance_data.push_back(idata);
		p_instance->scenario->instance_aabbs.push_back(InstanceBounds(p_instance->transformed_aabb));
	} else {
		if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
			p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY].update(p_instance->indexer_id, bvh_aabb);
		} else {
			p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES].update(p_instance->indexer_id, bvh_aabb);
		}
		p_instance->scenario->instance_aabbs[p_instance->array_index] = InstanceBounds(p_instance->transformed_aabb);
	}

	//move instance and repair
	pair_pass++;

	PairInstances pair;

	pair.instance = p_instance;
	pair.pair_allocator = &pair_allocator;
	pair.pair_pass = pair_pass;
	pair.pair_mask = 0;

	if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		pair.pair_mask |= 1 << RS::INSTANCE_LIGHT;
		pair.pair_mask |= 1 << RS::INSTANCE_GI_PROBE;
		pair.pair_mask |= 1 << RS::INSTANCE_LIGHTMAP;

		pair.pair_mask |= geometry_instance_pair_mask;

		pair.bvh2 = &p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES];
	} else if (p_instance->base_type == RS::INSTANCE_LIGHT) {
		pair.pair_mask |= RS::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];

		if (RSG::storage->light_get_bake_mode(p_instance->base) == RS::LIGHT_BAKE_DYNAMIC) {
			pair.pair_mask |= (1 << RS::INSTANCE_GI_PROBE);
			pair.bvh2 = &p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES];
		}
	} else if (geometry_instance_pair_mask & (1 << RS::INSTANCE_REFLECTION_PROBE) && (p_instance->base_type == RS::INSTANCE_REFLECTION_PROBE)) {
		pair.pair_mask = RS::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	} else if (geometry_instance_pair_mask & (1 << RS::INSTANCE_DECAL) && (p_instance->base_type == RS::INSTANCE_DECAL)) {
		pair.pair_mask = RS::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	} else if (p_instance->base_type == RS::INSTANCE_PARTICLES_COLLISION) {
		pair.pair_mask = (1 << RS::INSTANCE_PARTICLES);
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	} else if (p_instance->base_type == RS::INSTANCE_GI_PROBE) {
		//lights and geometries
		pair.pair_mask = RS::INSTANCE_GEOMETRY_MASK | (1 << RS::INSTANCE_LIGHT);
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
		pair.bvh2 = &p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES];
	}

	pair.pair();

	p_instance->prev_transformed_aabb = p_instance->transformed_aabb;
}

void RendererSceneCull::_unpair_instance(Instance *p_instance) {
	if (!p_instance->indexer_id.is_valid()) {
		return; //nothing to do
	}

	while (p_instance->pairs.first()) {
		InstancePair *pair = p_instance->pairs.first()->self();
		Instance *other_instance = p_instance == pair->a ? pair->b : pair->a;
		_instance_unpair(p_instance, other_instance);
		pair_allocator.free(pair);
	}

	if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY].remove(p_instance->indexer_id);
	} else {
		p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES].remove(p_instance->indexer_id);
	}

	p_instance->indexer_id = DynamicBVH::ID();

	//replace this by last
	int32_t swap_with_index = p_instance->scenario->instance_data.size() - 1;
	if (swap_with_index != p_instance->array_index) {
		p_instance->scenario->instance_data[swap_with_index].instance->array_index = p_instance->array_index; //swap
		p_instance->scenario->instance_data[p_instance->array_index] = p_instance->scenario->instance_data[swap_with_index];
		p_instance->scenario->instance_aabbs[p_instance->array_index] = p_instance->scenario->instance_aabbs[swap_with_index];
	}

	// pop last
	p_instance->scenario->instance_data.pop_back();
	p_instance->scenario->instance_aabbs.pop_back();

	//uninitialize
	p_instance->array_index = -1;
	if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		// Clear these now because the InstanceData containing the dirty flags is gone
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);

		scene_render->geometry_instance_pair_light_instances(geom->geometry_instance, nullptr, 0);
		scene_render->geometry_instance_pair_reflection_probe_instances(geom->geometry_instance, nullptr, 0);
		scene_render->geometry_instance_pair_decal_instances(geom->geometry_instance, nullptr, 0);
		scene_render->geometry_instance_pair_gi_probe_instances(geom->geometry_instance, nullptr, 0);
	}
}

void RendererSceneCull::_update_instance_aabb(Instance *p_instance) {
	AABB new_aabb;

	ERR_FAIL_COND(p_instance->base_type != RS::INSTANCE_NONE && !p_instance->base.is_valid());

	switch (p_instance->base_type) {
		case RenderingServer::INSTANCE_NONE: {
			// do nothing
		} break;
		case RenderingServer::INSTANCE_MESH: {
			if (p_instance->custom_aabb) {
				new_aabb = *p_instance->custom_aabb;
			} else {
				new_aabb = RSG::storage->mesh_get_aabb(p_instance->base, p_instance->skeleton);
			}

		} break;

		case RenderingServer::INSTANCE_MULTIMESH: {
			if (p_instance->custom_aabb) {
				new_aabb = *p_instance->custom_aabb;
			} else {
				new_aabb = RSG::storage->multimesh_get_aabb(p_instance->base);
			}

		} break;
		case RenderingServer::INSTANCE_IMMEDIATE: {
			if (p_instance->custom_aabb) {
				new_aabb = *p_instance->custom_aabb;
			} else {
				new_aabb = RSG::storage->immediate_get_aabb(p_instance->base);
			}

		} break;
		case RenderingServer::INSTANCE_PARTICLES: {
			if (p_instance->custom_aabb) {
				new_aabb = *p_instance->custom_aabb;
			} else {
				new_aabb = RSG::storage->particles_get_aabb(p_instance->base);
			}

		} break;
		case RenderingServer::INSTANCE_PARTICLES_COLLISION: {
			new_aabb = RSG::storage->particles_collision_get_aabb(p_instance->base);

		} break;
		case RenderingServer::INSTANCE_LIGHT: {
			new_aabb = RSG::storage->light_get_aabb(p_instance->base);

		} break;
		case RenderingServer::INSTANCE_REFLECTION_PROBE: {
			new_aabb = RSG::storage->reflection_probe_get_aabb(p_instance->base);

		} break;
		case RenderingServer::INSTANCE_DECAL: {
			new_aabb = RSG::storage->decal_get_aabb(p_instance->base);

		} break;
		case RenderingServer::INSTANCE_GI_PROBE: {
			new_aabb = RSG::storage->gi_probe_get_bounds(p_instance->base);

		} break;
		case RenderingServer::INSTANCE_LIGHTMAP: {
			new_aabb = RSG::storage->lightmap_get_aabb(p_instance->base);

		} break;
		default: {
		}
	}

	// <Zylann> This is why I didn't re-use Instance::aabb to implement custom AABBs
	if (p_instance->extra_margin) {
		new_aabb.grow_by(p_instance->extra_margin);
	}

	p_instance->aabb = new_aabb;
}

void RendererSceneCull::_update_instance_lightmap_captures(Instance *p_instance) {
	bool first_set = p_instance->lightmap_sh.size() == 0;
	p_instance->lightmap_sh.resize(9); //using SH
	p_instance->lightmap_target_sh.resize(9); //using SH
	Color *instance_sh = p_instance->lightmap_target_sh.ptrw();
	bool inside = false;
	Color accum_sh[9];
	float accum_blend = 0.0;

	InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);
	for (Set<Instance *>::Element *E = geom->lightmap_captures.front(); E; E = E->next()) {
		Instance *lightmap = E->get();

		bool interior = RSG::storage->lightmap_is_interior(lightmap->base);

		if (inside && !interior) {
			continue; //we are inside, ignore exteriors
		}

		Transform to_bounds = lightmap->transform.affine_inverse();
		Vector3 center = p_instance->transform.xform(p_instance->aabb.position + p_instance->aabb.size * 0.5); //use aabb center

		Vector3 lm_pos = to_bounds.xform(center);

		AABB bounds = RSG::storage->lightmap_get_aabb(lightmap->base);
		if (!bounds.has_point(lm_pos)) {
			continue; //not in this lightmap
		}

		Color sh[9];
		RSG::storage->lightmap_tap_sh_light(lightmap->base, lm_pos, sh);

		//rotate it
		Basis rot = lightmap->transform.basis.orthonormalized();
		for (int i = 0; i < 3; i++) {
			float csh[9];
			for (int j = 0; j < 9; j++) {
				csh[j] = sh[j][i];
			}
			rot.rotate_sh(csh);
			for (int j = 0; j < 9; j++) {
				sh[j][i] = csh[j];
			}
		}

		Vector3 inner_pos = ((lm_pos - bounds.position) / bounds.size) * 2.0 - Vector3(1.0, 1.0, 1.0);

		float blend = MAX(inner_pos.x, MAX(inner_pos.y, inner_pos.z));
		//make blend more rounded
		blend = Math::lerp(inner_pos.length(), blend, blend);
		blend *= blend;
		blend = MAX(0.0, 1.0 - blend);

		if (interior && !inside) {
			//do not blend, just replace
			for (int j = 0; j < 9; j++) {
				accum_sh[j] = sh[j] * blend;
			}
			accum_blend = blend;
			inside = true;
		} else {
			for (int j = 0; j < 9; j++) {
				accum_sh[j] += sh[j] * blend;
			}
			accum_blend += blend;
		}
	}

	if (accum_blend > 0.0) {
		for (int j = 0; j < 9; j++) {
			instance_sh[j] = accum_sh[j] / accum_blend;
			if (first_set) {
				p_instance->lightmap_sh.write[j] = instance_sh[j];
			}
		}
	}

	scene_render->geometry_instance_set_lightmap_capture(geom->geometry_instance, p_instance->lightmap_sh.ptr());
}

void RendererSceneCull::_light_instance_setup_directional_shadow(int p_shadow_index, Instance *p_instance, const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, bool p_cam_vaspect) {
	InstanceLightData *light = static_cast<InstanceLightData *>(p_instance->base_data);

	Transform light_transform = p_instance->transform;
	light_transform.orthonormalize(); //scale does not count on lights

	real_t max_distance = p_cam_projection.get_z_far();
	real_t shadow_max = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_SHADOW_MAX_DISTANCE);
	if (shadow_max > 0 && !p_cam_orthogonal) { //its impractical (and leads to unwanted behaviors) to set max distance in orthogonal camera
		max_distance = MIN(shadow_max, max_distance);
	}
	max_distance = MAX(max_distance, p_cam_projection.get_z_near() + 0.001);
	real_t min_distance = MIN(p_cam_projection.get_z_near(), max_distance);

	RS::LightDirectionalShadowDepthRangeMode depth_range_mode = RSG::storage->light_directional_get_shadow_depth_range_mode(p_instance->base);

	real_t pancake_size = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_SHADOW_PANCAKE_SIZE);

	real_t range = max_distance - min_distance;

	int splits = 0;
	switch (RSG::storage->light_directional_get_shadow_mode(p_instance->base)) {
		case RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL:
			splits = 1;
			break;
		case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS:
			splits = 2;
			break;
		case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS:
			splits = 4;
			break;
	}

	real_t distances[5];

	distances[0] = min_distance;
	for (int i = 0; i < splits; i++) {
		distances[i + 1] = min_distance + RSG::storage->light_get_param(p_instance->base, RS::LightParam(RS::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET + i)) * range;
	};

	distances[splits] = max_distance;

	real_t texture_size = scene_render->get_directional_light_shadow_size(light->instance);

	bool overlap = RSG::storage->light_directional_get_blend_splits(p_instance->base);

	real_t first_radius = 0.0;

	real_t min_distance_bias_scale = distances[1];

	cull.shadow_count = p_shadow_index + 1;
	cull.shadows[p_shadow_index].cascade_count = splits;
	cull.shadows[p_shadow_index].light_instance = light->instance;

	for (int i = 0; i < splits; i++) {
		RENDER_TIMESTAMP("Culling Directional Light split" + itos(i));

		// setup a camera matrix for that range!
		CameraMatrix camera_matrix;

		real_t aspect = p_cam_projection.get_aspect();

		if (p_cam_orthogonal) {
			Vector2 vp_he = p_cam_projection.get_viewport_half_extents();

			camera_matrix.set_orthogonal(vp_he.y * 2.0, aspect, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], false);
		} else {
			real_t fov = p_cam_projection.get_fov(); //this is actually yfov, because set aspect tries to keep it
			camera_matrix.set_perspective(fov, aspect, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], true);
		}

		//obtain the frustum endpoints

		Vector3 endpoints[8]; // frustum plane endpoints
		bool res = camera_matrix.get_endpoints(p_cam_transform, endpoints);
		ERR_CONTINUE(!res);

		// obtain the light frustum ranges (given endpoints)

		Transform transform = light_transform; //discard scale and stabilize light

		Vector3 x_vec = transform.basis.get_axis(Vector3::AXIS_X).normalized();
		Vector3 y_vec = transform.basis.get_axis(Vector3::AXIS_Y).normalized();
		Vector3 z_vec = transform.basis.get_axis(Vector3::AXIS_Z).normalized();
		//z_vec points against the camera, like in default opengl

		real_t x_min = 0.f, x_max = 0.f;
		real_t y_min = 0.f, y_max = 0.f;
		real_t z_min = 0.f, z_max = 0.f;

		// FIXME: z_max_cam is defined, computed, but not used below when setting up
		// ortho_camera. Commented out for now to fix warnings but should be investigated.
		real_t x_min_cam = 0.f, x_max_cam = 0.f;
		real_t y_min_cam = 0.f, y_max_cam = 0.f;
		real_t z_min_cam = 0.f;
		//real_t z_max_cam = 0.f;

		real_t bias_scale = 1.0;
		real_t aspect_bias_scale = 1.0;

		//used for culling

		for (int j = 0; j < 8; j++) {
			real_t d_x = x_vec.dot(endpoints[j]);
			real_t d_y = y_vec.dot(endpoints[j]);
			real_t d_z = z_vec.dot(endpoints[j]);

			if (j == 0 || d_x < x_min) {
				x_min = d_x;
			}
			if (j == 0 || d_x > x_max) {
				x_max = d_x;
			}

			if (j == 0 || d_y < y_min) {
				y_min = d_y;
			}
			if (j == 0 || d_y > y_max) {
				y_max = d_y;
			}

			if (j == 0 || d_z < z_min) {
				z_min = d_z;
			}
			if (j == 0 || d_z > z_max) {
				z_max = d_z;
			}
		}

		real_t radius = 0;
		real_t soft_shadow_expand = 0;
		Vector3 center;

		{
			//camera viewport stuff

			for (int j = 0; j < 8; j++) {
				center += endpoints[j];
			}
			center /= 8.0;

			//center=x_vec*(x_max-x_min)*0.5 + y_vec*(y_max-y_min)*0.5 + z_vec*(z_max-z_min)*0.5;

			for (int j = 0; j < 8; j++) {
				real_t d = center.distance_to(endpoints[j]);
				if (d > radius) {
					radius = d;
				}
			}

			radius *= texture_size / (texture_size - 2.0); //add a texel by each side

			if (i == 0) {
				first_radius = radius;
			} else {
				bias_scale = radius / first_radius;
			}

			z_min_cam = z_vec.dot(center) - radius;

			{
				float soft_shadow_angle = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_SIZE);

				if (soft_shadow_angle > 0.0) {
					float z_range = (z_vec.dot(center) + radius + pancake_size) - z_min_cam;
					soft_shadow_expand = Math::tan(Math::deg2rad(soft_shadow_angle)) * z_range;

					x_max += soft_shadow_expand;
					y_max += soft_shadow_expand;

					x_min -= soft_shadow_expand;
					y_min -= soft_shadow_expand;
				}
			}

			x_max_cam = x_vec.dot(center) + radius + soft_shadow_expand;
			x_min_cam = x_vec.dot(center) - radius - soft_shadow_expand;
			y_max_cam = y_vec.dot(center) + radius + soft_shadow_expand;
			y_min_cam = y_vec.dot(center) - radius - soft_shadow_expand;

			if (depth_range_mode == RS::LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_STABLE) {
				//this trick here is what stabilizes the shadow (make potential jaggies to not move)
				//at the cost of some wasted resolution. Still the quality increase is very well worth it

				real_t unit = radius * 2.0 / texture_size;

				x_max_cam = Math::snapped(x_max_cam, unit);
				x_min_cam = Math::snapped(x_min_cam, unit);
				y_max_cam = Math::snapped(y_max_cam, unit);
				y_min_cam = Math::snapped(y_min_cam, unit);
			}
		}

		//now that we know all ranges, we can proceed to make the light frustum planes, for culling octree

		Vector<Plane> light_frustum_planes;
		light_frustum_planes.resize(6);

		//right/left
		light_frustum_planes.write[0] = Plane(x_vec, x_max);
		light_frustum_planes.write[1] = Plane(-x_vec, -x_min);
		//top/bottom
		light_frustum_planes.write[2] = Plane(y_vec, y_max);
		light_frustum_planes.write[3] = Plane(-y_vec, -y_min);
		//near/far
		light_frustum_planes.write[4] = Plane(z_vec, z_max + 1e6);
		light_frustum_planes.write[5] = Plane(-z_vec, -z_min); // z_min is ok, since casters further than far-light plane are not needed

		// a pre pass will need to be needed to determine the actual z-near to be used

		if (pancake_size > 0) {
			z_max = z_vec.dot(center) + radius + pancake_size;
		}

		if (aspect != 1.0) {
			// if the aspect is different, then the radius will become larger.
			// if this happens, then bias needs to be adjusted too, as depth will increase
			// to do this, compare the depth of one that would have resulted from a square frustum

			CameraMatrix camera_matrix_square;
			if (p_cam_orthogonal) {
				Vector2 vp_he = camera_matrix.get_viewport_half_extents();
				if (p_cam_vaspect) {
					camera_matrix_square.set_orthogonal(vp_he.x * 2.0, 1.0, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], true);
				} else {
					camera_matrix_square.set_orthogonal(vp_he.y * 2.0, 1.0, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], false);
				}
			} else {
				Vector2 vp_he = camera_matrix.get_viewport_half_extents();
				if (p_cam_vaspect) {
					camera_matrix_square.set_frustum(vp_he.x * 2.0, 1.0, Vector2(), distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], true);
				} else {
					camera_matrix_square.set_frustum(vp_he.y * 2.0, 1.0, Vector2(), distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], false);
				}
			}

			Vector3 endpoints_square[8]; // frustum plane endpoints
			res = camera_matrix_square.get_endpoints(p_cam_transform, endpoints_square);
			ERR_CONTINUE(!res);
			Vector3 center_square;

			for (int j = 0; j < 8; j++) {
				center_square += endpoints_square[j];
			}

			center_square /= 8.0;

			real_t radius_square = 0;

			for (int j = 0; j < 8; j++) {
				real_t d = center_square.distance_to(endpoints_square[j]);
				if (d > radius_square) {
					radius_square = d;
				}
			}

			radius_square *= texture_size / (texture_size - 2.0); //add a texel by each side

			float z_max_square = z_vec.dot(center_square) + radius_square + pancake_size;

			real_t z_min_cam_square = z_vec.dot(center_square) - radius_square;

			aspect_bias_scale = (z_max - z_min_cam) / (z_max_square - z_min_cam_square);

			// this is not entirely perfect, because the cull-adjusted z-max may be different
			// but at least it's warranted that it results in a greater bias, so no acne should be present either way.
			// pancaking also helps with this.
		}

		{
			CameraMatrix ortho_camera;
			real_t half_x = (x_max_cam - x_min_cam) * 0.5;
			real_t half_y = (y_max_cam - y_min_cam) * 0.5;

			ortho_camera.set_orthogonal(-half_x, half_x, -half_y, half_y, 0, (z_max - z_min_cam));

			Vector2 uv_scale(1.0 / (x_max_cam - x_min_cam), 1.0 / (y_max_cam - y_min_cam));

			Transform ortho_transform;
			ortho_transform.basis = transform.basis;
			ortho_transform.origin = x_vec * (x_min_cam + half_x) + y_vec * (y_min_cam + half_y) + z_vec * z_max;

			cull.shadows[p_shadow_index].cascades[i].frustum = Frustum(light_frustum_planes);
			cull.shadows[p_shadow_index].cascades[i].projection = ortho_camera;
			cull.shadows[p_shadow_index].cascades[i].transform = ortho_transform;
			cull.shadows[p_shadow_index].cascades[i].zfar = z_max - z_min_cam;
			cull.shadows[p_shadow_index].cascades[i].split = distances[i + 1];
			cull.shadows[p_shadow_index].cascades[i].shadow_texel_size = radius * 2.0 / texture_size;
			cull.shadows[p_shadow_index].cascades[i].bias_scale = bias_scale * aspect_bias_scale * min_distance_bias_scale;
			cull.shadows[p_shadow_index].cascades[i].range_begin = z_max;
			cull.shadows[p_shadow_index].cascades[i].uv_scale = uv_scale;
		}
	}
}

bool RendererSceneCull::_light_instance_update_shadow(Instance *p_instance, const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, bool p_cam_vaspect, RID p_shadow_atlas, Scenario *p_scenario, float p_screen_lod_threshold) {
	InstanceLightData *light = static_cast<InstanceLightData *>(p_instance->base_data);

	Transform light_transform = p_instance->transform;
	light_transform.orthonormalize(); //scale does not count on lights

	bool animated_material_found = false;

	switch (RSG::storage->light_get_type(p_instance->base)) {
		case RS::LIGHT_DIRECTIONAL: {
		} break;
		case RS::LIGHT_OMNI: {
			RS::LightOmniShadowMode shadow_mode = RSG::storage->light_omni_get_shadow_mode(p_instance->base);

			if (shadow_mode == RS::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID || !scene_render->light_instances_can_render_shadow_cube()) {
				if (max_shadows_used + 2 > MAX_UPDATE_SHADOWS) {
					return true;
				}
				for (int i = 0; i < 2; i++) {
					//using this one ensures that raster deferred will have it
					RENDER_TIMESTAMP("Culling Shadow Paraboloid" + itos(i));

					real_t radius = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_RANGE);

					real_t z = i == 0 ? -1 : 1;
					Vector<Plane> planes;
					planes.resize(6);
					planes.write[0] = light_transform.xform(Plane(Vector3(0, 0, z), radius));
					planes.write[1] = light_transform.xform(Plane(Vector3(1, 0, z).normalized(), radius));
					planes.write[2] = light_transform.xform(Plane(Vector3(-1, 0, z).normalized(), radius));
					planes.write[3] = light_transform.xform(Plane(Vector3(0, 1, z).normalized(), radius));
					planes.write[4] = light_transform.xform(Plane(Vector3(0, -1, z).normalized(), radius));
					planes.write[5] = light_transform.xform(Plane(Vector3(0, 0, -z), 0));

					instance_shadow_cull_result.clear();

					Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

					struct CullConvex {
						PagedArray<Instance *> *result;
						_FORCE_INLINE_ bool operator()(void *p_data) {
							Instance *p_instance = (Instance *)p_data;
							result->push_back(p_instance);
							return false;
						}
					};

					CullConvex cull_convex;
					cull_convex.result = &instance_shadow_cull_result;

					p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

					Plane near_plane(light_transform.origin, light_transform.basis.get_axis(2) * z);

					RendererSceneRender::RenderShadowData &shadow_data = render_shadow_data[max_shadows_used++];

					for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
						Instance *instance = instance_shadow_cull_result[j];
						if (!instance->visible || !((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) || !static_cast<InstanceGeometryData *>(instance->base_data)->can_cast_shadows) {
							continue;
						} else {
							if (static_cast<InstanceGeometryData *>(instance->base_data)->material_is_animated) {
								animated_material_found = true;
							}

							if (instance->mesh_instance.is_valid()) {
								RSG::storage->mesh_instance_check_for_update(instance->mesh_instance);
							}
						}

						shadow_data.instances.push_back(static_cast<InstanceGeometryData *>(instance->base_data)->geometry_instance);
					}

					RSG::storage->update_mesh_instances();

					scene_render->light_instance_set_shadow_transform(light->instance, CameraMatrix(), light_transform, radius, 0, i, 0);
					shadow_data.light = light->instance;
					shadow_data.pass = i;
				}
			} else { //shadow cube

				if (max_shadows_used + 6 > MAX_UPDATE_SHADOWS) {
					return true;
				}

				real_t radius = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_RANGE);
				CameraMatrix cm;
				cm.set_perspective(90, 1, 0.01, radius);

				for (int i = 0; i < 6; i++) {
					RENDER_TIMESTAMP("Culling Shadow Cube side" + itos(i));
					//using this one ensures that raster deferred will have it

					static const Vector3 view_normals[6] = {
						Vector3(+1, 0, 0),
						Vector3(-1, 0, 0),
						Vector3(0, -1, 0),
						Vector3(0, +1, 0),
						Vector3(0, 0, +1),
						Vector3(0, 0, -1)
					};
					static const Vector3 view_up[6] = {
						Vector3(0, -1, 0),
						Vector3(0, -1, 0),
						Vector3(0, 0, -1),
						Vector3(0, 0, +1),
						Vector3(0, -1, 0),
						Vector3(0, -1, 0)
					};

					Transform xform = light_transform * Transform().looking_at(view_normals[i], view_up[i]);

					Vector<Plane> planes = cm.get_projection_planes(xform);

					instance_shadow_cull_result.clear();

					Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

					struct CullConvex {
						PagedArray<Instance *> *result;
						_FORCE_INLINE_ bool operator()(void *p_data) {
							Instance *p_instance = (Instance *)p_data;
							result->push_back(p_instance);
							return false;
						}
					};

					CullConvex cull_convex;
					cull_convex.result = &instance_shadow_cull_result;

					p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

					RendererSceneRender::RenderShadowData &shadow_data = render_shadow_data[max_shadows_used++];

					for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
						Instance *instance = instance_shadow_cull_result[j];
						if (!instance->visible || !((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) || !static_cast<InstanceGeometryData *>(instance->base_data)->can_cast_shadows) {
							continue;
						} else {
							if (static_cast<InstanceGeometryData *>(instance->base_data)->material_is_animated) {
								animated_material_found = true;
							}
							if (instance->mesh_instance.is_valid()) {
								RSG::storage->mesh_instance_check_for_update(instance->mesh_instance);
							}
						}

						shadow_data.instances.push_back(static_cast<InstanceGeometryData *>(instance->base_data)->geometry_instance);
					}

					RSG::storage->update_mesh_instances();
					scene_render->light_instance_set_shadow_transform(light->instance, cm, xform, radius, 0, i, 0);

					shadow_data.light = light->instance;
					shadow_data.pass = i;
				}

				//restore the regular DP matrix
				//scene_render->light_instance_set_shadow_transform(light->instance, CameraMatrix(), light_transform, radius, 0, 0, 0);
			}

		} break;
		case RS::LIGHT_SPOT: {
			RENDER_TIMESTAMP("Culling Spot Light");

			if (max_shadows_used + 1 > MAX_UPDATE_SHADOWS) {
				return true;
			}

			real_t radius = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_RANGE);
			real_t angle = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_SPOT_ANGLE);

			CameraMatrix cm;
			cm.set_perspective(angle * 2.0, 1.0, 0.01, radius);

			Vector<Plane> planes = cm.get_projection_planes(light_transform);

			instance_shadow_cull_result.clear();

			Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

			struct CullConvex {
				PagedArray<Instance *> *result;
				_FORCE_INLINE_ bool operator()(void *p_data) {
					Instance *p_instance = (Instance *)p_data;
					result->push_back(p_instance);
					return false;
				}
			};

			CullConvex cull_convex;
			cull_convex.result = &instance_shadow_cull_result;

			p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

			RendererSceneRender::RenderShadowData &shadow_data = render_shadow_data[max_shadows_used++];

			for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
				Instance *instance = instance_shadow_cull_result[j];
				if (!instance->visible || !((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) || !static_cast<InstanceGeometryData *>(instance->base_data)->can_cast_shadows) {
					continue;
				} else {
					if (static_cast<InstanceGeometryData *>(instance->base_data)->material_is_animated) {
						animated_material_found = true;
					}

					if (instance->mesh_instance.is_valid()) {
						RSG::storage->mesh_instance_check_for_update(instance->mesh_instance);
					}
				}
				shadow_data.instances.push_back(static_cast<InstanceGeometryData *>(instance->base_data)->geometry_instance);
			}

			RSG::storage->update_mesh_instances();

			scene_render->light_instance_set_shadow_transform(light->instance, cm, light_transform, radius, 0, 0, 0);
			shadow_data.light = light->instance;
			shadow_data.pass = 0;

		} break;
	}

	return animated_material_found;
}

void RendererSceneCull::render_camera(RID p_render_buffers, RID p_camera, RID p_scenario, Size2 p_viewport_size, float p_screen_lod_threshold, RID p_shadow_atlas) {
// render to mono camera
#ifndef _3D_DISABLED

	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);

	/* STEP 1 - SETUP CAMERA */
	CameraMatrix camera_matrix;
	bool ortho = false;

	switch (camera->type) {
		case Camera::ORTHOGONAL: {
			camera_matrix.set_orthogonal(
					camera->size,
					p_viewport_size.width / (float)p_viewport_size.height,
					camera->znear,
					camera->zfar,
					camera->vaspect);
			ortho = true;
		} break;
		case Camera::PERSPECTIVE: {
			camera_matrix.set_perspective(
					camera->fov,
					p_viewport_size.width / (float)p_viewport_size.height,
					camera->znear,
					camera->zfar,
					camera->vaspect);
			ortho = false;

		} break;
		case Camera::FRUSTUM: {
			camera_matrix.set_frustum(
					camera->size,
					p_viewport_size.width / (float)p_viewport_size.height,
					camera->offset,
					camera->znear,
					camera->zfar,
					camera->vaspect);
			ortho = false;
		} break;
	}

	RID environment = _render_get_environment(p_camera, p_scenario);
	_render_scene(camera->transform, camera_matrix, ortho, camera->vaspect, p_render_buffers, environment, camera->effects, camera->visible_layers, p_scenario, p_shadow_atlas, RID(), -1, p_screen_lod_threshold);
#endif
}

void RendererSceneCull::render_camera(RID p_render_buffers, Ref<XRInterface> &p_interface, XRInterface::Eyes p_eye, RID p_camera, RID p_scenario, Size2 p_viewport_size, float p_screen_lod_threshold, RID p_shadow_atlas) {
	// render for AR/VR interface
	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);

	/* SETUP CAMERA, we are ignoring type and FOV here */
	float aspect = p_viewport_size.width / (float)p_viewport_size.height;
	CameraMatrix camera_matrix = p_interface->get_projection_for_eye(p_eye, aspect, camera->znear, camera->zfar);

	// We also ignore our camera position, it will have been positioned with a slightly old tracking position.
	// Instead we take our origin point and have our ar/vr interface add fresh tracking data! Whoohoo!
	Transform world_origin = XRServer::get_singleton()->get_world_origin();
	Transform cam_transform = p_interface->get_transform_for_eye(p_eye, world_origin);

	RID environment = _render_get_environment(p_camera, p_scenario);

	// For stereo render we only prepare for our left eye and then reuse the outcome for our right eye
	if (p_eye == XRInterface::EYE_LEFT) {
		// Center our transform, we assume basis is equal.
		Transform mono_transform = cam_transform;
		Transform right_transform = p_interface->get_transform_for_eye(XRInterface::EYE_RIGHT, world_origin);
		mono_transform.origin += right_transform.origin;
		mono_transform.origin *= 0.5;

		// We need to combine our projection frustums for culling.
		// Ideally we should use our clipping planes for this and combine them,
		// however our shadow map logic uses our projection matrix.
		// Note: as our left and right frustums should be mirrored, we don't need our right projection matrix.

		// - get some base values we need
		float eye_dist = (mono_transform.origin - cam_transform.origin).length();
		float z_near = camera_matrix.get_z_near(); // get our near plane
		float z_far = camera_matrix.get_z_far(); // get our far plane
		float width = (2.0 * z_near) / camera_matrix.matrix[0][0];
		float x_shift = width * camera_matrix.matrix[2][0];
		float height = (2.0 * z_near) / camera_matrix.matrix[1][1];
		float y_shift = height * camera_matrix.matrix[2][1];

		// printf("Eye_dist = %f, Near = %f, Far = %f, Width = %f, Shift = %f\n", eye_dist, z_near, z_far, width, x_shift);

		// - calculate our near plane size (horizontal only, right_near is mirrored)
		float left_near = -eye_dist - ((width - x_shift) * 0.5);

		// - calculate our far plane size (horizontal only, right_far is mirrored)
		float left_far = -eye_dist - (z_far * (width - x_shift) * 0.5 / z_near);
		float left_far_right_eye = eye_dist - (z_far * (width + x_shift) * 0.5 / z_near);
		if (left_far > left_far_right_eye) {
			// on displays smaller then double our iod, the right eye far frustrum can overtake the left eyes.
			left_far = left_far_right_eye;
		}

		// - figure out required z-shift
		float slope = (left_far - left_near) / (z_far - z_near);
		float z_shift = (left_near / slope) - z_near;

		// - figure out new vertical near plane size (this will be slightly oversized thanks to our z-shift)
		float top_near = (height - y_shift) * 0.5;
		top_near += (top_near / z_near) * z_shift;
		float bottom_near = -(height + y_shift) * 0.5;
		bottom_near += (bottom_near / z_near) * z_shift;

		// printf("Left_near = %f, Left_far = %f, Top_near = %f, Bottom_near = %f, Z_shift = %f\n", left_near, left_far, top_near, bottom_near, z_shift);

		// - generate our frustum
		CameraMatrix combined_matrix;
		combined_matrix.set_frustum(left_near, -left_near, bottom_near, top_near, z_near + z_shift, z_far + z_shift);

		// and finally move our camera back
		Transform apply_z_shift;
		apply_z_shift.origin = Vector3(0.0, 0.0, z_shift); // z negative is forward so this moves it backwards
		mono_transform *= apply_z_shift;

		// now prepare our scene with our adjusted transform projection matrix
		_prepare_scene(mono_transform, combined_matrix, false, false, p_render_buffers, environment, camera->visible_layers, p_scenario, p_shadow_atlas, RID(), p_screen_lod_threshold, true);
	} else if (p_eye == XRInterface::EYE_MONO) {
		// For mono render, prepare as per usual
		_prepare_scene(cam_transform, camera_matrix, false, false, p_render_buffers, environment, camera->visible_layers, p_scenario, p_shadow_atlas, RID(), p_screen_lod_threshold, true);
	}
	
	// And render our scene...
	_render_scene(cam_transform, camera_matrix, false, camera->vaspect, p_render_buffers, environment, camera->effects, camera->visible_layers, p_scenario, p_shadow_atlas, RID(), -1, p_screen_lod_threshold, false);
}

void RendererSceneCull::_frustum_cull_threaded(uint32_t p_thread, FrustumCullData *cull_data) {
	uint32_t cull_total = cull_data->scenario->instance_data.size();
	uint32_t total_threads = RendererThreadPool::singleton->thread_work_pool.get_thread_count();
	uint32_t cull_from = p_thread * cull_total / total_threads;
	uint32_t cull_to = (p_thread + 1 == total_threads) ? cull_total : ((p_thread + 1) * cull_total / total_threads);

	_frustum_cull(*cull_data, frustum_cull_result_threads[p_thread], cull_from, cull_to);
}

void RendererSceneCull::_frustum_cull(FrustumCullData &cull_data, FrustumCullResult &cull_result, uint64_t p_from, uint64_t p_to) {
	uint64_t frame_number = RSG::rasterizer->get_frame_number();
	float lightmap_probe_update_speed = RSG::storage->lightmap_get_probe_capture_update_speed() * RSG::rasterizer->get_frame_delta_time();

	uint32_t sdfgi_last_light_index = 0xFFFFFFFF;
	uint32_t sdfgi_last_light_cascade = 0xFFFFFFFF;

	RID instance_pair_buffer[MAX_INSTANCE_PAIRS];

	for (uint64_t i = p_from; i < p_to; i++) {
		bool mesh_visible = false;

		if (cull_data.scenario->instance_aabbs[i].in_frustum(cull_data.cull->frustum)) {
			InstanceData &idata = cull_data.scenario->instance_data[i];
			uint32_t base_type = idata.flags & InstanceData::FLAG_BASE_TYPE_MASK;

			if ((cull_data.visible_layers & idata.layer_mask) == 0) {
				//failure
			} else if (base_type == RS::INSTANCE_LIGHT) {
				cull_result.lights.push_back(idata.instance);
				cull_result.light_instances.push_back(RID::from_uint64(idata.instance_data_rid));
				if (cull_data.shadow_atlas.is_valid() && RSG::storage->light_has_shadow(idata.base_rid)) {
					scene_render->light_instance_mark_visible(RID::from_uint64(idata.instance_data_rid)); //mark it visible for shadow allocation later
				}

			} else if (base_type == RS::INSTANCE_REFLECTION_PROBE) {
				if (cull_data.render_reflection_probe != idata.instance) {
					//avoid entering The Matrix

					if ((idata.flags & InstanceData::FLAG_REFLECTION_PROBE_DIRTY) || scene_render->reflection_probe_instance_needs_redraw(RID::from_uint64(idata.instance_data_rid))) {
						InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(idata.instance->base_data);
						cull_data.cull->lock.lock();
						if (!reflection_probe->update_list.in_list()) {
							reflection_probe->render_step = 0;
							reflection_probe_render_list.add_last(&reflection_probe->update_list);
						}
						cull_data.cull->lock.unlock();

						idata.flags &= ~uint32_t(InstanceData::FLAG_REFLECTION_PROBE_DIRTY);
					}

					if (scene_render->reflection_probe_instance_has_reflection(RID::from_uint64(idata.instance_data_rid))) {
						cull_result.reflections.push_back(RID::from_uint64(idata.instance_data_rid));
					}
				}
			} else if (base_type == RS::INSTANCE_DECAL) {
				cull_result.decals.push_back(RID::from_uint64(idata.instance_data_rid));

			} else if (base_type == RS::INSTANCE_GI_PROBE) {
				InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(idata.instance->base_data);
				cull_data.cull->lock.lock();
				if (!gi_probe->update_element.in_list()) {
					gi_probe_update_list.add(&gi_probe->update_element);
				}
				cull_data.cull->lock.unlock();
				cull_result.gi_probes.push_back(RID::from_uint64(idata.instance_data_rid));

			} else if (base_type == RS::INSTANCE_LIGHTMAP) {
				cull_result.gi_probes.push_back(RID::from_uint64(idata.instance_data_rid));
			} else if (((1 << base_type) & RS::INSTANCE_GEOMETRY_MASK) && !(idata.flags & InstanceData::FLAG_CAST_SHADOWS_ONLY)) {
				bool keep = true;

				if (idata.flags & InstanceData::FLAG_REDRAW_IF_VISIBLE) {
					RenderingServerDefault::redraw_request();
				}

				if (base_type == RS::INSTANCE_MESH) {
					mesh_visible = true;
				} else if (base_type == RS::INSTANCE_PARTICLES) {
					//particles visible? process them
					if (RSG::storage->particles_is_inactive(idata.base_rid)) {
						//but if nothing is going on, don't do it.
						keep = false;
					} else {
						cull_data.cull->lock.lock();
						RSG::storage->particles_request_process(idata.base_rid);
						cull_data.cull->lock.unlock();
						RSG::storage->particles_set_view_axis(idata.base_rid, -cull_data.cam_transform.basis.get_axis(2).normalized());
						//particles visible? request redraw
						RenderingServerDefault::redraw_request();
					}
				}

				if (geometry_instance_pair_mask & (1 << RS::INSTANCE_LIGHT) && (idata.flags & InstanceData::FLAG_GEOM_LIGHTING_DIRTY)) {
					InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(idata.instance->base_data);
					uint32_t idx = 0;

					for (Set<Instance *>::Element *E = geom->lights.front(); E; E = E->next()) {
						InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
						instance_pair_buffer[idx++] = light->instance;
						if (idx == MAX_INSTANCE_PAIRS) {
							break;
						}
					}

					scene_render->geometry_instance_pair_light_instances(geom->geometry_instance, instance_pair_buffer, idx);
					idata.flags &= ~uint32_t(InstanceData::FLAG_GEOM_LIGHTING_DIRTY);
				}

				if (geometry_instance_pair_mask & (1 << RS::INSTANCE_REFLECTION_PROBE) && (idata.flags & InstanceData::FLAG_GEOM_REFLECTION_DIRTY)) {
					InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(idata.instance->base_data);
					uint32_t idx = 0;

					for (Set<Instance *>::Element *E = geom->reflection_probes.front(); E; E = E->next()) {
						InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(E->get()->base_data);

						instance_pair_buffer[idx++] = reflection_probe->instance;
						if (idx == MAX_INSTANCE_PAIRS) {
							break;
						}
					}

					scene_render->geometry_instance_pair_reflection_probe_instances(geom->geometry_instance, instance_pair_buffer, idx);
					idata.flags &= ~uint32_t(InstanceData::FLAG_GEOM_REFLECTION_DIRTY);
				}

				if (geometry_instance_pair_mask & (1 << RS::INSTANCE_DECAL) && (idata.flags & InstanceData::FLAG_GEOM_DECAL_DIRTY)) {
					//InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(idata.instance->base_data);
					//todo for GLES3
					idata.flags &= ~uint32_t(InstanceData::FLAG_GEOM_DECAL_DIRTY);
					/*for (Set<Instance *>::Element *E = geom->dec.front(); E; E = E->next()) {
					InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(E->get()->base_data);

					instance_pair_buffer[idx++] = reflection_probe->instance;
					if (idx==MAX_INSTANCE_PAIRS) {
						break;
					}
				}*/
					//scene_render->geometry_instance_pair_decal_instances(geom->geometry_instance, light_instances, idx);
				}

				if (idata.flags & InstanceData::FLAG_GEOM_GI_PROBE_DIRTY) {
					InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(idata.instance->base_data);
					uint32_t idx = 0;
					for (Set<Instance *>::Element *E = geom->gi_probes.front(); E; E = E->next()) {
						InstanceGIProbeData *gi_probe = static_cast<InstanceGIProbeData *>(E->get()->base_data);

						instance_pair_buffer[idx++] = gi_probe->probe_instance;
						if (idx == MAX_INSTANCE_PAIRS) {
							break;
						}
					}

					scene_render->geometry_instance_pair_gi_probe_instances(geom->geometry_instance, instance_pair_buffer, idx);
					idata.flags &= ~uint32_t(InstanceData::FLAG_GEOM_GI_PROBE_DIRTY);
				}

				if ((idata.flags & InstanceData::FLAG_LIGHTMAP_CAPTURE) && idata.instance->last_frame_pass != frame_number && !idata.instance->lightmap_target_sh.is_empty() && !idata.instance->lightmap_sh.is_empty()) {
					InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(idata.instance->base_data);
					Color *sh = idata.instance->lightmap_sh.ptrw();
					const Color *target_sh = idata.instance->lightmap_target_sh.ptr();
					for (uint32_t j = 0; j < 9; j++) {
						sh[j] = sh[j].lerp(target_sh[j], MIN(1.0, lightmap_probe_update_speed));
					}
					scene_render->geometry_instance_set_lightmap_capture(geom->geometry_instance, sh);
					idata.instance->last_frame_pass = frame_number;
				}

				if (keep) {
					cull_result.geometry_instances.push_back(idata.instance_geometry);
				}
			}
		}

		for (uint32_t j = 0; j < cull_data.cull->shadow_count; j++) {
			for (uint32_t k = 0; k < cull_data.cull->shadows[j].cascade_count; k++) {
				if (cull_data.scenario->instance_aabbs[i].in_frustum(cull_data.cull->shadows[j].cascades[k].frustum)) {
					InstanceData &idata = cull_data.scenario->instance_data[i];
					uint32_t base_type = idata.flags & InstanceData::FLAG_BASE_TYPE_MASK;

					if (((1 << base_type) & RS::INSTANCE_GEOMETRY_MASK) && idata.flags & InstanceData::FLAG_CAST_SHADOWS) {
						cull_result.directional_shadows[j].cascade_geometry_instances[k].push_back(idata.instance_geometry);
						mesh_visible = true;
					}
				}
			}
		}

		for (uint32_t j = 0; j < cull_data.cull->sdfgi.region_count; j++) {
			if (cull_data.scenario->instance_aabbs[i].in_aabb(cull_data.cull->sdfgi.region_aabb[j])) {
				InstanceData &idata = cull_data.scenario->instance_data[i];
				uint32_t base_type = idata.flags & InstanceData::FLAG_BASE_TYPE_MASK;

				if (base_type == RS::INSTANCE_LIGHT) {
					InstanceLightData *instance_light = (InstanceLightData *)idata.instance->base_data;
					if (instance_light->bake_mode == RS::LIGHT_BAKE_STATIC && cull_data.cull->sdfgi.region_cascade[j] <= instance_light->max_sdfgi_cascade) {
						if (sdfgi_last_light_index != i || sdfgi_last_light_cascade != cull_data.cull->sdfgi.region_cascade[j]) {
							sdfgi_last_light_index = i;
							sdfgi_last_light_cascade = cull_data.cull->sdfgi.region_cascade[j];
							cull_result.sdfgi_cascade_lights[sdfgi_last_light_cascade].push_back(instance_light->instance);
						}
					}
				} else if ((1 << base_type) & RS::INSTANCE_GEOMETRY_MASK) {
					if (idata.flags & InstanceData::FLAG_USES_BAKED_LIGHT) {
						cull_result.sdfgi_region_geometry_instances[j].push_back(idata.instance_geometry);
						mesh_visible = true;
					}
				}
			}
		}

		if (mesh_visible && cull_data.scenario->instance_data[i].flags & InstanceData::FLAG_USES_MESH_INSTANCE) {
			cull_result.mesh_instances.push_back(cull_data.scenario->instance_data[i].instance->mesh_instance);
		}
	}
}

void RendererSceneCull::_render_scene(const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, bool p_cam_vaspect, RID p_render_buffers, RID p_environment, RID p_force_camera_effects, uint32_t p_visible_layers, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_lod_threshold, bool p_using_shadows) {
	// Note, in stereo rendering:
	// - p_cam_transform will be a transform in the middle of our two eyes
	// - p_cam_projection is a wider frustrum that encompasses both eyes

	Instance *render_reflection_probe = instance_owner.getornull(p_reflection_probe); //if null, not rendering to it

	Scenario *scenario = scenario_owner.getornull(p_scenario);

	render_pass++;

	scene_render->set_scene_pass(render_pass);

	if (p_render_buffers.is_valid()) {
		//no rendering code here, this is only to set up what needs to be done, request regions, etc.
		scene_render->sdfgi_update(p_render_buffers, p_environment, p_cam_transform.origin); //update conditions for SDFGI (whether its used or not)
	}

	RENDER_TIMESTAMP("Frustum Culling");

	//rasterizer->set_camera(camera->transform, camera_matrix,ortho);

	Vector<Plane> planes = p_cam_projection.get_projection_planes(p_cam_transform);

	Plane near_plane(p_cam_transform.origin, -p_cam_transform.basis.get_axis(2).normalized());

	/* STEP 2 - CULL */

	cull.frustum = Frustum(planes);

	Vector<RID> directional_lights;
	// directional lights
	{
		cull.shadow_count = 0;

		Vector<Instance *> lights_with_shadow;

		for (List<Instance *>::Element *E = scenario->directional_lights.front(); E; E = E->next()) {
			if (!E->get()->visible) {
				continue;
			}

			if (directional_lights.size() > RendererSceneRender::MAX_DIRECTIONAL_LIGHTS) {
				break;
			}

			InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);

			//check shadow..

			if (light) {
				if (p_using_shadows && p_shadow_atlas.is_valid() && RSG::storage->light_has_shadow(E->get()->base) && !(RSG::storage->light_get_type(E->get()->base) == RS::LIGHT_DIRECTIONAL && RSG::storage->light_directional_is_sky_only(E->get()->base))) {
					lights_with_shadow.push_back(E->get());
				}
				//add to list
				directional_lights.push_back(light->instance);
			}
		}

		scene_render->set_directional_shadow_count(lights_with_shadow.size());

		for (int i = 0; i < lights_with_shadow.size(); i++) {
			_light_instance_setup_directional_shadow(i, lights_with_shadow[i], p_cam_transform, p_cam_projection, p_cam_orthogonal, p_cam_vaspect);
		}
	}

	{ //sdfgi
		cull.sdfgi.region_count = 0;

		if (p_render_buffers.is_valid()) {
			cull.sdfgi.cascade_light_count = 0;

			uint32_t prev_cascade = 0xFFFFFFFF;
			uint32_t pending_region_count = scene_render->sdfgi_get_pending_region_count(p_render_buffers);

			for (uint32_t i = 0; i < pending_region_count; i++) {
				cull.sdfgi.region_aabb[i] = scene_render->sdfgi_get_pending_region_bounds(p_render_buffers, i);
				uint32_t region_cascade = scene_render->sdfgi_get_pending_region_cascade(p_render_buffers, i);
				cull.sdfgi.region_cascade[i] = region_cascade;

				if (region_cascade != prev_cascade) {
					cull.sdfgi.cascade_light_index[cull.sdfgi.cascade_light_count] = region_cascade;
					cull.sdfgi.cascade_light_count++;
					prev_cascade = region_cascade;
				}
			}

			cull.sdfgi.region_count = pending_region_count;
		}
	}

	frustum_cull_result.clear();

	{
		uint64_t cull_from = 0;
		uint64_t cull_to = scenario->instance_data.size();

		FrustumCullData cull_data;

		//prepare for eventual thread usage
		cull_data.cull = &cull;
		cull_data.scenario = scenario;
		cull_data.shadow_atlas = p_shadow_atlas;
		cull_data.cam_transform = p_cam_transform;
		cull_data.visible_layers = p_visible_layers;
		cull_data.render_reflection_probe = render_reflection_probe;
//#define DEBUG_CULL_TIME
#ifdef DEBUG_CULL_TIME
		uint64_t time_from = OS::get_singleton()->get_ticks_usec();
#endif
		if (cull_to > thread_cull_threshold) {
			//multiple threads
			for (uint32_t i = 0; i < frustum_cull_result_threads.size(); i++) {
				frustum_cull_result_threads[i].clear();
			}

			RendererThreadPool::singleton->thread_work_pool.do_work(frustum_cull_result_threads.size(), this, &RendererSceneCull::_frustum_cull_threaded, &cull_data);

			for (uint32_t i = 0; i < frustum_cull_result_threads.size(); i++) {
				frustum_cull_result.append_from(frustum_cull_result_threads[i]);
			}

		} else {
			//single threaded
			_frustum_cull(cull_data, frustum_cull_result, cull_from, cull_to);
		}

#ifdef DEBUG_CULL_TIME
		static float time_avg = 0;
		static uint32_t time_count = 0;
		time_avg += double(OS::get_singleton()->get_ticks_usec() - time_from) / 1000.0;
		time_count++;
		print_line("time taken: " + rtos(time_avg / time_count));
#endif

		if (frustum_cull_result.mesh_instances.size()) {
			for (uint64_t i = 0; i < frustum_cull_result.mesh_instances.size(); i++) {
				RSG::storage->mesh_instance_check_for_update(frustum_cull_result.mesh_instances[i]);
			}
			RSG::storage->update_mesh_instances();
		}
	}

	//render shadows

	max_shadows_used = 0;

	if (p_using_shadows) { //setup shadow maps

		// Directional Shadows

		for (uint32_t i = 0; i < cull.shadow_count; i++) {
			for (uint32_t j = 0; j < cull.shadows[i].cascade_count; j++) {
				const Cull::Shadow::Cascade &c = cull.shadows[i].cascades[j];
				//			print_line("shadow " + itos(i) + " cascade " + itos(j) + " elements: " + itos(c.cull_result.size()));
				scene_render->light_instance_set_shadow_transform(cull.shadows[i].light_instance, c.projection, c.transform, c.zfar, c.split, j, c.shadow_texel_size, c.bias_scale, c.range_begin, c.uv_scale);
				if (max_shadows_used == MAX_UPDATE_SHADOWS) {
					continue;
				}
				render_shadow_data[max_shadows_used].light = cull.shadows[i].light_instance;
				render_shadow_data[max_shadows_used].pass = j;
				render_shadow_data[max_shadows_used].instances.merge_unordered(frustum_cull_result.directional_shadows[i].cascade_geometry_instances[j]);
				max_shadows_used++;
			}
		}

		// Positional Shadowss
		for (uint32_t i = 0; i < (uint32_t)frustum_cull_result.lights.size(); i++) {
			Instance *ins = frustum_cull_result.lights[i];

			if (!p_shadow_atlas.is_valid() || !RSG::storage->light_has_shadow(ins->base)) {
				continue;
			}

			InstanceLightData *light = static_cast<InstanceLightData *>(ins->base_data);

			float coverage = 0.f;

			{ //compute coverage

				Transform cam_xf = p_cam_transform;
				float zn = p_cam_projection.get_z_near();
				Plane p(cam_xf.origin + cam_xf.basis.get_axis(2) * -zn, -cam_xf.basis.get_axis(2)); //camera near plane

				// near plane half width and height
				Vector2 vp_half_extents = p_cam_projection.get_viewport_half_extents();

				switch (RSG::storage->light_get_type(ins->base)) {
					case RS::LIGHT_OMNI: {
						float radius = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_RANGE);

						//get two points parallel to near plane
						Vector3 points[2] = {
							ins->transform.origin,
							ins->transform.origin + cam_xf.basis.get_axis(0) * radius
						};

						if (!p_cam_orthogonal) {
							//if using perspetive, map them to near plane
							for (int j = 0; j < 2; j++) {
								if (p.distance_to(points[j]) < 0) {
									points[j].z = -zn; //small hack to keep size constant when hitting the screen
								}

								p.intersects_segment(cam_xf.origin, points[j], &points[j]); //map to plane
							}
						}

						float screen_diameter = points[0].distance_to(points[1]) * 2;
						coverage = screen_diameter / (vp_half_extents.x + vp_half_extents.y);
					} break;
					case RS::LIGHT_SPOT: {
						float radius = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_RANGE);
						float angle = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_SPOT_ANGLE);

						float w = radius * Math::sin(Math::deg2rad(angle));
						float d = radius * Math::cos(Math::deg2rad(angle));

						Vector3 base = ins->transform.origin - ins->transform.basis.get_axis(2).normalized() * d;

						Vector3 points[2] = {
							base,
							base + cam_xf.basis.get_axis(0) * w
						};

						if (!p_cam_orthogonal) {
							//if using perspetive, map them to near plane
							for (int j = 0; j < 2; j++) {
								if (p.distance_to(points[j]) < 0) {
									points[j].z = -zn; //small hack to keep size constant when hitting the screen
								}

								p.intersects_segment(cam_xf.origin, points[j], &points[j]); //map to plane
							}
						}

						float screen_diameter = points[0].distance_to(points[1]) * 2;
						coverage = screen_diameter / (vp_half_extents.x + vp_half_extents.y);

					} break;
					default: {
						ERR_PRINT("Invalid Light Type");
					}
				}
			}

			if (light->shadow_dirty) {
				light->last_version++;
				light->shadow_dirty = false;
			}

			bool redraw = scene_render->shadow_atlas_update_light(p_shadow_atlas, light->instance, coverage, light->last_version);

			if (redraw && max_shadows_used < MAX_UPDATE_SHADOWS) {
				//must redraw!
				RENDER_TIMESTAMP(">Rendering Light " + itos(i));
				light->shadow_dirty = _light_instance_update_shadow(ins, p_cam_transform, p_cam_projection, p_cam_orthogonal, p_cam_vaspect, p_shadow_atlas, scenario, p_screen_lod_threshold);
				RENDER_TIMESTAMP("<Rendering Light " + itos(i));
			} else {
				light->shadow_dirty = redraw;
			}
		}
	}

	//render SDFGI

	{
		sdfgi_update_data.update_static = false;

		if (cull.sdfgi.region_count > 0) {
			//update regions
			for (uint32_t i = 0; i < cull.sdfgi.region_count; i++) {
				render_sdfgi_data[i].instances.merge_unordered(frustum_cull_result.sdfgi_region_geometry_instances[i]);
				render_sdfgi_data[i].region = i;
			}
			//check if static lights were culled
			bool static_lights_culled = false;
			for (uint32_t i = 0; i < cull.sdfgi.cascade_light_count; i++) {
				if (frustum_cull_result.sdfgi_cascade_lights[i].size()) {
					static_lights_culled = true;
					break;
				}
			}

			if (static_lights_culled) {
				sdfgi_update_data.static_cascade_count = cull.sdfgi.cascade_light_count;
				sdfgi_update_data.static_cascade_indices = cull.sdfgi.cascade_light_index;
				sdfgi_update_data.static_positional_lights = frustum_cull_result.sdfgi_cascade_lights;
				sdfgi_update_data.update_static = true;
			}
		}

		if (p_render_buffers.is_valid()) {
			sdfgi_update_data.directional_lights = &directional_lights;
			sdfgi_update_data.positional_light_instances = scenario->dynamic_lights.ptr();
			sdfgi_update_data.positional_light_count = scenario->dynamic_lights.size();
		}
	}

	//append the directional lights to the lights culled
	for (int i = 0; i < directional_lights.size(); i++) {
		frustum_cull_result.light_instances.push_back(directional_lights[i]);
	}

	RID camera_effects;
	if (p_force_camera_effects.is_valid()) {
		camera_effects = p_force_camera_effects;
	} else {
		camera_effects = scenario->camera_effects;
	}
	/* PROCESS GEOMETRY AND DRAW SCENE */

	RENDER_TIMESTAMP("Render Scene ");
	scene_render->render_scene(p_render_buffers, p_cam_transform, p_cam_projection, p_cam_orthogonal, frustum_cull_result.geometry_instances, frustum_cull_result.light_instances, frustum_cull_result.reflections, frustum_cull_result.gi_probes, frustum_cull_result.decals, frustum_cull_result.lightmaps, p_environment, camera_effects, p_shadow_atlas, p_reflection_probe.is_valid() ? RID() : scenario->reflection_atlas, p_reflection_probe, p_reflection_probe_pass, p_screen_lod_threshold, render_shadow_data, max_shadows_used, render_sdfgi_data, cull.sdfgi.region_count, &sdfgi_update_data);

	for (uint32_t i = 0; i < max_shadows_used; i++) {
		render_shadow_data[i].instances.clear();
	}
	max_shadows_used = 0;

	for (uint32_t i = 0; i < cull.sdfgi.region_count; i++) {
		render_sdfgi_data[i].instances.clear();
	}

	//	virtual void render_scene(RID p_render_buffers, const Transform &p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_ortogonal, const PagedArray<GeometryInstance *> &p_instances, const PagedArray<RID> &p_lights, const PagedArray<RID> &p_reflection_probes, const PagedArray<RID> &p_gi_probes, const PagedArray<RID> &p_decals, const PagedArray<RID> &p_lightmaps, RID p_environment, RID p_camera_effects, RID p_shadow_atlas, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_lod_threshold,const RenderShadowData *p_render_shadows,int p_render_shadow_count,const RenderSDFGIData *p_render_sdfgi_regions,int p_render_sdfgi_region_count,const RenderSDFGIStaticLightData *p_render_sdfgi_static_lights=nullptr) = 0;
}

RID RendererSceneCull::_render_get_environment(RID p_camera, RID p_scenario) {
	Camera *camera = camera_owner.getornull(p_camera);
	if (camera && scene_render->is_environment(camera->env)) {
		return camera->env;
	}

	Scenario *scenario = scenario_owner.getornull(p_scenario);
	if (!scenario) {
		return RID();
	}
	if (scene_render->is_environment(scenario->environment)) {
		return scenario->environment;
	}

	if (scene_render->is_environment(scenario->fallback_environment)) {
		return scenario->fallback_environment;
	}

	return RID();
}

void RendererSceneCull::render_empty_scene(RID p_render_buffers, RID p_scenario, RID p_shadow_atlas) {
#ifndef _3D_DISABLED

	Scenario *scenario = scenario_owner.getornull(p_scenario);

	RID environment;
	if (scenario->environment.is_valid()) {
		environment = scenario->environment;
	} else {
		environment = scenario->fallback_environment;
	}
	RENDER_TIMESTAMP("Render Empty Scene ");
	scene_render->render_scene(p_render_buffers, Transform(), CameraMatrix(), true, PagedArray<RendererSceneRender::GeometryInstance *>(), PagedArray<RID>(), PagedArray<RID>(), PagedArray<RID>(), PagedArray<RID>(), PagedArray<RID>(), RID(), RID(), p_shadow_atlas, scenario->reflection_atlas, RID(), 0, 0, nullptr, 0, nullptr, 0, nullptr);
#endif
}

bool RendererSceneCull::_render_reflection_probe_step(Instance *p_instance, int p_step) {
	InstanceReflectionProbeData *reflection_probe = static_cast<InstanceReflectionProbeData *>(p_instance->base_data);
	Scenario *scenario = p_instance->scenario;
	ERR_FAIL_COND_V(!scenario, true);

	RenderingServerDefault::redraw_request(); //update, so it updates in editor

	if (p_step == 0) {
		if (!scene_render->reflection_probe_instance_begin_render(reflection_probe->instance, scenario->reflection_atlas)) {
			return true; //all full
		}
	}

	if (p_step >= 0 && p_step < 6) {
		static const Vector3 view_normals[6] = {
			Vector3(+1, 0, 0),
			Vector3(-1, 0, 0),
			Vector3(0, +1, 0),
			Vector3(0, -1, 0),
			Vector3(0, 0, +1),
			Vector3(0, 0, -1)
		};
		static const Vector3 view_up[6] = {
			Vector3(0, -1, 0),
			Vector3(0, -1, 0),
			Vector3(0, 0, +1),
			Vector3(0, 0, -1),
			Vector3(0, -1, 0),
			Vector3(0, -1, 0)
		};

		Vector3 extents = RSG::storage->reflection_probe_get_extents(p_instance->base);
		Vector3 origin_offset = RSG::storage->reflection_probe_get_origin_offset(p_instance->base);
		float max_distance = RSG::storage->reflection_probe_get_origin_max_distance(p_instance->base);
		float size = scene_render->reflection_atlas_get_size(scenario->reflection_atlas);
		float lod_threshold = RSG::storage->reflection_probe_get_lod_threshold(p_instance->base) / size;

		Vector3 edge = view_normals[p_step] * extents;
		float distance = ABS(view_normals[p_step].dot(edge) - view_normals[p_step].dot(origin_offset)); //distance from origin offset to actual view distance limit

		max_distance = MAX(max_distance, distance);

		//render cubemap side
		CameraMatrix cm;
		cm.set_perspective(90, 1, 0.01, max_distance);

		Transform local_view;
		local_view.set_look_at(origin_offset, origin_offset + view_normals[p_step], view_up[p_step]);

		Transform xform = p_instance->transform * local_view;

		RID shadow_atlas;

		bool use_shadows = RSG::storage->reflection_probe_renders_shadows(p_instance->base);
		if (use_shadows) {
			shadow_atlas = scenario->reflection_probe_shadow_atlas;
		}

		RENDER_TIMESTAMP("Render Reflection Probe, Step " + itos(p_step));
		_render_scene(xform, cm, false, false, RID(), RID(), RID(), RSG::storage->reflection_probe_get_cull_mask(p_instance->base), p_instance->scenario->self, shadow_atlas, reflection_probe->instance, p_step, lod_threshold, use_shadows);

	} else {
		//do roughness postprocess step until it believes it's done
		RENDER_TIMESTAMP("Post-Process Reflection Probe, Step " + itos(p_step));
		return scene_render->reflection_probe_instance_postprocess_step(reflection_probe->instance);
	}

	return false;
}

void RendererSceneCull::render_probes() {
	/* REFLECTION PROBES */

	SelfList<InstanceReflectionProbeData> *ref_probe = reflection_probe_render_list.first();

	bool busy = false;

	while (ref_probe) {
		SelfList<InstanceReflectionProbeData> *next = ref_probe->next();
		RID base = ref_probe->self()->owner->base;

		switch (RSG::storage->reflection_probe_get_update_mode(base)) {
			case RS::REFLECTION_PROBE_UPDATE_ONCE: {
				if (busy) { //already rendering something
					break;
				}

				bool done = _render_reflection_probe_step(ref_probe->self()->owner, ref_probe->self()->render_step);
				if (done) {
					reflection_probe_render_list.remove(ref_probe);
				} else {
					ref_probe->self()->render_step++;
				}

				busy = true; //do not render another one of this kind
			} break;
			case RS::REFLECTION_PROBE_UPDATE_ALWAYS: {
				int step = 0;
				bool done = false;
				while (!done) {
					done = _render_reflection_probe_step(ref_probe->self()->owner, step);
					step++;
				}

				reflection_probe_render_list.remove(ref_probe);
			} break;
		}

		ref_probe = next;
	}

	/* GI PROBES */

	SelfList<InstanceGIProbeData> *gi_probe = gi_probe_update_list.first();

	if (gi_probe) {
		RENDER_TIMESTAMP("Render GI Probes");
	}

	while (gi_probe) {
		SelfList<InstanceGIProbeData> *next = gi_probe->next();

		InstanceGIProbeData *probe = gi_probe->self();
		//Instance *instance_probe = probe->owner;

		//check if probe must be setup, but don't do if on the lighting thread

		bool cache_dirty = false;
		int cache_count = 0;
		{
			int light_cache_size = probe->light_cache.size();
			const InstanceGIProbeData::LightCache *caches = probe->light_cache.ptr();
			const RID *instance_caches = probe->light_instances.ptr();

			int idx = 0; //must count visible lights
			for (Set<Instance *>::Element *E = probe->lights.front(); E; E = E->next()) {
				Instance *instance = E->get();
				InstanceLightData *instance_light = (InstanceLightData *)instance->base_data;
				if (!instance->visible) {
					continue;
				}
				if (cache_dirty) {
					//do nothing, since idx must count all visible lights anyway
				} else if (idx >= light_cache_size) {
					cache_dirty = true;
				} else {
					const InstanceGIProbeData::LightCache *cache = &caches[idx];

					if (
							instance_caches[idx] != instance_light->instance ||
							cache->has_shadow != RSG::storage->light_has_shadow(instance->base) ||
							cache->type != RSG::storage->light_get_type(instance->base) ||
							cache->transform != instance->transform ||
							cache->color != RSG::storage->light_get_color(instance->base) ||
							cache->energy != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ENERGY) ||
							cache->bake_energy != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_INDIRECT_ENERGY) ||
							cache->radius != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_RANGE) ||
							cache->attenuation != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ATTENUATION) ||
							cache->spot_angle != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ANGLE) ||
							cache->spot_attenuation != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ATTENUATION)) {
						cache_dirty = true;
					}
				}

				idx++;
			}

			for (List<Instance *>::Element *E = probe->owner->scenario->directional_lights.front(); E; E = E->next()) {
				Instance *instance = E->get();
				InstanceLightData *instance_light = (InstanceLightData *)instance->base_data;
				if (!instance->visible) {
					continue;
				}
				if (cache_dirty) {
					//do nothing, since idx must count all visible lights anyway
				} else if (idx >= light_cache_size) {
					cache_dirty = true;
				} else {
					const InstanceGIProbeData::LightCache *cache = &caches[idx];

					if (
							instance_caches[idx] != instance_light->instance ||
							cache->has_shadow != RSG::storage->light_has_shadow(instance->base) ||
							cache->type != RSG::storage->light_get_type(instance->base) ||
							cache->transform != instance->transform ||
							cache->color != RSG::storage->light_get_color(instance->base) ||
							cache->energy != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ENERGY) ||
							cache->bake_energy != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_INDIRECT_ENERGY) ||
							cache->radius != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_RANGE) ||
							cache->attenuation != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ATTENUATION) ||
							cache->spot_angle != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ANGLE) ||
							cache->spot_attenuation != RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ATTENUATION) ||
							cache->sky_only != RSG::storage->light_directional_is_sky_only(instance->base)) {
						cache_dirty = true;
					}
				}

				idx++;
			}

			if (idx != light_cache_size) {
				cache_dirty = true;
			}

			cache_count = idx;
		}

		bool update_lights = scene_render->gi_probe_needs_update(probe->probe_instance);

		if (cache_dirty) {
			probe->light_cache.resize(cache_count);
			probe->light_instances.resize(cache_count);

			if (cache_count) {
				InstanceGIProbeData::LightCache *caches = probe->light_cache.ptrw();
				RID *instance_caches = probe->light_instances.ptrw();

				int idx = 0; //must count visible lights
				for (Set<Instance *>::Element *E = probe->lights.front(); E; E = E->next()) {
					Instance *instance = E->get();
					InstanceLightData *instance_light = (InstanceLightData *)instance->base_data;
					if (!instance->visible) {
						continue;
					}

					InstanceGIProbeData::LightCache *cache = &caches[idx];

					instance_caches[idx] = instance_light->instance;
					cache->has_shadow = RSG::storage->light_has_shadow(instance->base);
					cache->type = RSG::storage->light_get_type(instance->base);
					cache->transform = instance->transform;
					cache->color = RSG::storage->light_get_color(instance->base);
					cache->energy = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ENERGY);
					cache->bake_energy = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_INDIRECT_ENERGY);
					cache->radius = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_RANGE);
					cache->attenuation = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ATTENUATION);
					cache->spot_angle = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ANGLE);
					cache->spot_attenuation = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ATTENUATION);

					idx++;
				}
				for (List<Instance *>::Element *E = probe->owner->scenario->directional_lights.front(); E; E = E->next()) {
					Instance *instance = E->get();
					InstanceLightData *instance_light = (InstanceLightData *)instance->base_data;
					if (!instance->visible) {
						continue;
					}

					InstanceGIProbeData::LightCache *cache = &caches[idx];

					instance_caches[idx] = instance_light->instance;
					cache->has_shadow = RSG::storage->light_has_shadow(instance->base);
					cache->type = RSG::storage->light_get_type(instance->base);
					cache->transform = instance->transform;
					cache->color = RSG::storage->light_get_color(instance->base);
					cache->energy = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ENERGY);
					cache->bake_energy = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_INDIRECT_ENERGY);
					cache->radius = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_RANGE);
					cache->attenuation = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_ATTENUATION);
					cache->spot_angle = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ANGLE);
					cache->spot_attenuation = RSG::storage->light_get_param(instance->base, RS::LIGHT_PARAM_SPOT_ATTENUATION);
					cache->sky_only = RSG::storage->light_directional_is_sky_only(instance->base);

					idx++;
				}
			}

			update_lights = true;
		}

		frustum_cull_result.geometry_instances.clear();

		RID instance_pair_buffer[MAX_INSTANCE_PAIRS];

		for (Set<Instance *>::Element *E = probe->dynamic_geometries.front(); E; E = E->next()) {
			Instance *ins = E->get();
			if (!ins->visible) {
				continue;
			}
			InstanceGeometryData *geom = (InstanceGeometryData *)ins->base_data;

			if (ins->scenario && ins->array_index >= 0 && (ins->scenario->instance_data[ins->array_index].flags & InstanceData::FLAG_GEOM_GI_PROBE_DIRTY)) {
				uint32_t idx = 0;
				for (Set<Instance *>::Element *F = geom->gi_probes.front(); F; F = F->next()) {
					InstanceGIProbeData *gi_probe2 = static_cast<InstanceGIProbeData *>(F->get()->base_data);

					instance_pair_buffer[idx++] = gi_probe2->probe_instance;
					if (idx == MAX_INSTANCE_PAIRS) {
						break;
					}
				}

				scene_render->geometry_instance_pair_gi_probe_instances(geom->geometry_instance, instance_pair_buffer, idx);

				ins->scenario->instance_data[ins->array_index].flags &= ~uint32_t(InstanceData::FLAG_GEOM_GI_PROBE_DIRTY);
			}

			frustum_cull_result.geometry_instances.push_back(geom->geometry_instance);
		}

		scene_render->gi_probe_update(probe->probe_instance, update_lights, probe->light_instances, frustum_cull_result.geometry_instances);

		gi_probe_update_list.remove(gi_probe);

		gi_probe = next;
	}
}

void RendererSceneCull::render_particle_colliders() {
	while (heightfield_particle_colliders_update_list.front()) {
		Instance *hfpc = heightfield_particle_colliders_update_list.front()->get();

		if (hfpc->scenario && hfpc->base_type == RS::INSTANCE_PARTICLES_COLLISION && RSG::storage->particles_collision_is_heightfield(hfpc->base)) {
			//update heightfield
			instance_cull_result.clear();
			frustum_cull_result.geometry_instances.clear();

			struct CullAABB {
				PagedArray<Instance *> *result;
				_FORCE_INLINE_ bool operator()(void *p_data) {
					Instance *p_instance = (Instance *)p_data;
					result->push_back(p_instance);
					return false;
				}
			};

			CullAABB cull_aabb;
			cull_aabb.result = &instance_cull_result;
			hfpc->scenario->indexers[Scenario::INDEXER_GEOMETRY].aabb_query(hfpc->transformed_aabb, cull_aabb);
			hfpc->scenario->indexers[Scenario::INDEXER_VOLUMES].aabb_query(hfpc->transformed_aabb, cull_aabb);

			for (int i = 0; i < (int)instance_cull_result.size(); i++) {
				Instance *instance = instance_cull_result[i];
				if (!instance || !((1 << instance->base_type) & (RS::INSTANCE_GEOMETRY_MASK & (~(1 << RS::INSTANCE_PARTICLES))))) { //all but particles to avoid self collision
					continue;
				}
				InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);
				frustum_cull_result.geometry_instances.push_back(geom->geometry_instance);
			}

			scene_render->render_particle_collider_heightfield(hfpc->base, hfpc->transform, frustum_cull_result.geometry_instances);
		}
		heightfield_particle_colliders_update_list.erase(heightfield_particle_colliders_update_list.front());
	}
}

void RendererSceneCull::_update_instance_shader_parameters_from_material(Map<StringName, Instance::InstanceShaderParameter> &isparams, const Map<StringName, Instance::InstanceShaderParameter> &existing_isparams, RID p_material) {
	List<RendererStorage::InstanceShaderParam> plist;
	RSG::storage->material_get_instance_shader_parameters(p_material, &plist);
	for (List<RendererStorage::InstanceShaderParam>::Element *E = plist.front(); E; E = E->next()) {
		StringName name = E->get().info.name;
		if (isparams.has(name)) {
			if (isparams[name].info.type != E->get().info.type) {
				WARN_PRINT("More than one material in instance export the same instance shader uniform '" + E->get().info.name + "', but they do it with different data types. Only the first one (in order) will display correctly.");
			}
			if (isparams[name].index != E->get().index) {
				WARN_PRINT("More than one material in instance export the same instance shader uniform '" + E->get().info.name + "', but they do it with different indices. Only the first one (in order) will display correctly.");
			}
			continue; //first one found always has priority
		}

		Instance::InstanceShaderParameter isp;
		isp.index = E->get().index;
		isp.info = E->get().info;
		isp.default_value = E->get().default_value;
		if (existing_isparams.has(name)) {
			isp.value = existing_isparams[name].value;
		} else {
			isp.value = E->get().default_value;
		}
		isparams[name] = isp;
	}
}

void RendererSceneCull::_update_dirty_instance(Instance *p_instance) {
	if (p_instance->update_aabb) {
		_update_instance_aabb(p_instance);
	}

	if (p_instance->update_dependencies) {
		p_instance->dependency_tracker.update_begin();

		if (p_instance->base.is_valid()) {
			RSG::storage->base_update_dependency(p_instance->base, &p_instance->dependency_tracker);
		}

		if (p_instance->material_override.is_valid()) {
			RSG::storage->material_update_dependency(p_instance->material_override, &p_instance->dependency_tracker);
		}

		if (p_instance->base_type == RS::INSTANCE_MESH) {
			//remove materials no longer used and un-own them

			int new_mat_count = RSG::storage->mesh_get_surface_count(p_instance->base);
			p_instance->materials.resize(new_mat_count);

			_instance_update_mesh_instance(p_instance);
		}

		if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
			InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);

			bool can_cast_shadows = true;
			bool is_animated = false;
			Map<StringName, Instance::InstanceShaderParameter> isparams;

			if (p_instance->cast_shadows == RS::SHADOW_CASTING_SETTING_OFF) {
				can_cast_shadows = false;
			}

			if (p_instance->material_override.is_valid()) {
				if (!RSG::storage->material_casts_shadows(p_instance->material_override)) {
					can_cast_shadows = false;
				}
				is_animated = RSG::storage->material_is_animated(p_instance->material_override);
				_update_instance_shader_parameters_from_material(isparams, p_instance->instance_shader_parameters, p_instance->material_override);
			} else {
				if (p_instance->base_type == RS::INSTANCE_MESH) {
					RID mesh = p_instance->base;

					if (mesh.is_valid()) {
						bool cast_shadows = false;

						for (int i = 0; i < p_instance->materials.size(); i++) {
							RID mat = p_instance->materials[i].is_valid() ? p_instance->materials[i] : RSG::storage->mesh_surface_get_material(mesh, i);

							if (!mat.is_valid()) {
								cast_shadows = true;
							} else {
								if (RSG::storage->material_casts_shadows(mat)) {
									cast_shadows = true;
								}

								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}

								_update_instance_shader_parameters_from_material(isparams, p_instance->instance_shader_parameters, mat);

								RSG::storage->material_update_dependency(mat, &p_instance->dependency_tracker);
							}
						}

						if (!cast_shadows) {
							can_cast_shadows = false;
						}
					}

				} else if (p_instance->base_type == RS::INSTANCE_MULTIMESH) {
					RID mesh = RSG::storage->multimesh_get_mesh(p_instance->base);
					if (mesh.is_valid()) {
						bool cast_shadows = false;

						int sc = RSG::storage->mesh_get_surface_count(mesh);
						for (int i = 0; i < sc; i++) {
							RID mat = RSG::storage->mesh_surface_get_material(mesh, i);

							if (!mat.is_valid()) {
								cast_shadows = true;

							} else {
								if (RSG::storage->material_casts_shadows(mat)) {
									cast_shadows = true;
								}
								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}

								_update_instance_shader_parameters_from_material(isparams, p_instance->instance_shader_parameters, mat);

								RSG::storage->material_update_dependency(mat, &p_instance->dependency_tracker);
							}
						}

						if (!cast_shadows) {
							can_cast_shadows = false;
						}

						RSG::storage->base_update_dependency(mesh, &p_instance->dependency_tracker);
					}
				} else if (p_instance->base_type == RS::INSTANCE_IMMEDIATE) {
					RID mat = RSG::storage->immediate_get_material(p_instance->base);

					if (!(!mat.is_valid() || RSG::storage->material_casts_shadows(mat))) {
						can_cast_shadows = false;
					}

					if (mat.is_valid() && RSG::storage->material_is_animated(mat)) {
						is_animated = true;
					}

					if (mat.is_valid()) {
						_update_instance_shader_parameters_from_material(isparams, p_instance->instance_shader_parameters, mat);
					}

					if (mat.is_valid()) {
						RSG::storage->material_update_dependency(mat, &p_instance->dependency_tracker);
					}

				} else if (p_instance->base_type == RS::INSTANCE_PARTICLES) {
					bool cast_shadows = false;

					int dp = RSG::storage->particles_get_draw_passes(p_instance->base);

					for (int i = 0; i < dp; i++) {
						RID mesh = RSG::storage->particles_get_draw_pass_mesh(p_instance->base, i);
						if (!mesh.is_valid()) {
							continue;
						}

						int sc = RSG::storage->mesh_get_surface_count(mesh);
						for (int j = 0; j < sc; j++) {
							RID mat = RSG::storage->mesh_surface_get_material(mesh, j);

							if (!mat.is_valid()) {
								cast_shadows = true;
							} else {
								if (RSG::storage->material_casts_shadows(mat)) {
									cast_shadows = true;
								}

								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}

								_update_instance_shader_parameters_from_material(isparams, p_instance->instance_shader_parameters, mat);

								RSG::storage->material_update_dependency(mat, &p_instance->dependency_tracker);
							}
						}
					}

					if (!cast_shadows) {
						can_cast_shadows = false;
					}
				}
			}

			if (can_cast_shadows != geom->can_cast_shadows) {
				//ability to cast shadows change, let lights now
				for (Set<Instance *>::Element *E = geom->lights.front(); E; E = E->next()) {
					InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
					light->shadow_dirty = true;
				}

				geom->can_cast_shadows = can_cast_shadows;
			}

			geom->material_is_animated = is_animated;
			p_instance->instance_shader_parameters = isparams;

			if (p_instance->instance_allocated_shader_parameters != (p_instance->instance_shader_parameters.size() > 0)) {
				p_instance->instance_allocated_shader_parameters = (p_instance->instance_shader_parameters.size() > 0);
				if (p_instance->instance_allocated_shader_parameters) {
					p_instance->instance_allocated_shader_parameters_offset = RSG::storage->global_variables_instance_allocate(p_instance->self);
					scene_render->geometry_instance_set_instance_shader_parameters_offset(geom->geometry_instance, p_instance->instance_allocated_shader_parameters_offset);

					for (Map<StringName, Instance::InstanceShaderParameter>::Element *E = p_instance->instance_shader_parameters.front(); E; E = E->next()) {
						if (E->get().value.get_type() != Variant::NIL) {
							RSG::storage->global_variables_instance_update(p_instance->self, E->get().index, E->get().value);
						}
					}
				} else {
					RSG::storage->global_variables_instance_free(p_instance->self);
					p_instance->instance_allocated_shader_parameters_offset = -1;
					scene_render->geometry_instance_set_instance_shader_parameters_offset(geom->geometry_instance, -1);
				}
			}
		}

		if (p_instance->skeleton.is_valid()) {
			RSG::storage->skeleton_update_dependency(p_instance->skeleton, &p_instance->dependency_tracker);
		}

		p_instance->dependency_tracker.update_end();

		if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
			InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);
			scene_render->geometry_instance_set_surface_materials(geom->geometry_instance, p_instance->materials);
		}
	}

	_instance_update_list.remove(&p_instance->update_item);

	_update_instance(p_instance);

	p_instance->update_aabb = false;
	p_instance->update_dependencies = false;
}

void RendererSceneCull::update_dirty_instances() {
	RSG::storage->update_dirty_resources();

	while (_instance_update_list.first()) {
		_update_dirty_instance(_instance_update_list.first()->self());
	}
}

void RendererSceneCull::update() {
	//optimize bvhs
	for (uint32_t i = 0; i < scenario_owner.get_rid_count(); i++) {
		Scenario *s = scenario_owner.get_ptr_by_index(i);
		s->indexers[Scenario::INDEXER_GEOMETRY].optimize_incremental(indexer_update_iterations);
		s->indexers[Scenario::INDEXER_VOLUMES].optimize_incremental(indexer_update_iterations);
	}
	scene_render->update();
	update_dirty_instances();
	render_particle_colliders();
}

bool RendererSceneCull::free(RID p_rid) {
	if (scene_render->free(p_rid)) {
		return true;
	}

	if (camera_owner.owns(p_rid)) {
		Camera *camera = camera_owner.getornull(p_rid);

		camera_owner.free(p_rid);
		memdelete(camera);

	} else if (scenario_owner.owns(p_rid)) {
		Scenario *scenario = scenario_owner.getornull(p_rid);

		while (scenario->instances.first()) {
			instance_set_scenario(scenario->instances.first()->self()->self, RID());
		}
		scenario->instance_aabbs.reset();
		scenario->instance_data.reset();

		scene_render->free(scenario->reflection_probe_shadow_atlas);
		scene_render->free(scenario->reflection_atlas);
		scenario_owner.free(p_rid);
		memdelete(scenario);

	} else if (instance_owner.owns(p_rid)) {
		// delete the instance

		update_dirty_instances();

		Instance *instance = instance_owner.getornull(p_rid);

		instance_geometry_set_lightmap(p_rid, RID(), Rect2(), 0);
		instance_set_scenario(p_rid, RID());
		instance_set_base(p_rid, RID());
		instance_geometry_set_material_override(p_rid, RID());
		instance_attach_skeleton(p_rid, RID());

		if (instance->instance_allocated_shader_parameters) {
			//free the used shader parameters
			RSG::storage->global_variables_instance_free(instance->self);
		}
		update_dirty_instances(); //in case something changed this

		instance_owner.free(p_rid);
		memdelete(instance);
	} else {
		return false;
	}

	return true;
}

TypedArray<Image> RendererSceneCull::bake_render_uv2(RID p_base, const Vector<RID> &p_material_overrides, const Size2i &p_image_size) {
	return scene_render->bake_render_uv2(p_base, p_material_overrides, p_image_size);
}

/*******************************/
/* Passthrough to Scene Render */
/*******************************/

/* ENVIRONMENT API */

RendererSceneCull *RendererSceneCull::singleton = nullptr;

void RendererSceneCull::set_scene_render(RendererSceneRender *p_scene_render) {
	scene_render = p_scene_render;
	geometry_instance_pair_mask = scene_render->geometry_instance_get_pair_mask();
}

RendererSceneCull::RendererSceneCull() {
	render_pass = 1;
	singleton = this;

	instance_cull_result.set_page_pool(&instance_cull_page_pool);
	instance_shadow_cull_result.set_page_pool(&instance_cull_page_pool);

	for (uint32_t i = 0; i < MAX_UPDATE_SHADOWS; i++) {
		render_shadow_data[i].instances.set_page_pool(&geometry_instance_cull_page_pool);
	}
	for (uint32_t i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
		render_sdfgi_data[i].instances.set_page_pool(&geometry_instance_cull_page_pool);
	}

	frustum_cull_result.init(&rid_cull_page_pool, &geometry_instance_cull_page_pool, &instance_cull_page_pool);
	frustum_cull_result_threads.resize(RendererThreadPool::singleton->thread_work_pool.get_thread_count());
	for (uint32_t i = 0; i < frustum_cull_result_threads.size(); i++) {
		frustum_cull_result_threads[i].init(&rid_cull_page_pool, &geometry_instance_cull_page_pool, &instance_cull_page_pool);
	}

	indexer_update_iterations = GLOBAL_GET("rendering/limits/spatial_indexer/update_iterations_per_frame");
	thread_cull_threshold = GLOBAL_GET("rendering/limits/spatial_indexer/threaded_cull_minimum_instances");
	thread_cull_threshold = MAX(thread_cull_threshold, (uint32_t)RendererThreadPool::singleton->thread_work_pool.get_thread_count()); //make sure there is at least one thread per CPU
}

RendererSceneCull::~RendererSceneCull() {
	instance_cull_result.reset();
	instance_shadow_cull_result.reset();

	for (uint32_t i = 0; i < MAX_UPDATE_SHADOWS; i++) {
		render_shadow_data[i].instances.reset();
	}
	for (uint32_t i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
		render_sdfgi_data[i].instances.reset();
	}

	frustum_cull_result.reset();
	for (uint32_t i = 0; i < frustum_cull_result_threads.size(); i++) {
		frustum_cull_result_threads[i].reset();
	}
	frustum_cull_result_threads.clear();
}
void RendererSceneCull::_prepare_scene(const Transform p_cam_transform, const CameraMatrix &p_cam_projection, bool p_cam_orthogonal, bool p_cam_vaspect, RID p_render_buffers, RID p_environment, uint32_t p_visible_layers, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, float p_screen_lod_threshold, bool p_using_shadows) {
	// Note, in stereo rendering:
	// - p_cam_transform will be a transform in the middle of our two eyes
	// - p_cam_projection is a wider frustrum that encompasses both eyes

	Instance *render_reflection_probe = instance_owner.getornull(p_reflection_probe); //if null, not rendering to it

	Scenario *scenario = scenario_owner.getornull(p_scenario);

	render_pass++;

	scene_render->set_scene_pass(render_pass);

	if (p_render_buffers.is_valid()) {
		scene_render->sdfgi_update(p_render_buffers, p_environment, p_cam_transform.origin); //update conditions for SDFGI (whether its used or not)
	}

	RENDER_TIMESTAMP("Frustum Culling");

	//rasterizer->set_camera(camera->transform, camera_matrix,ortho);

	Vector<Plane> planes = p_cam_projection.get_projection_planes(p_cam_transform);

	Plane near_plane(p_cam_transform.origin, -p_cam_transform.basis.get_axis(2).normalized());

	/* STEP 2 - CULL */

	cull.frustum = Frustum(planes);

	Vector<RID> directional_lights;
	// directional lights
	{
		cull.shadow_count = 0;

		Vector<Instance *> lights_with_shadow;

		for (List<Instance *>::Element *E = scenario->directional_lights.front(); E; E = E->next()) {
			if (!E->get()->visible) {
				continue;
			}

			if (directional_lights.size() > RendererSceneRender::MAX_DIRECTIONAL_LIGHTS) {
				break;
			}

			InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);

			//check shadow..

			if (light) {
				if (p_using_shadows && p_shadow_atlas.is_valid() && RSG::storage->light_has_shadow(E->get()->base) && !(RSG::storage->light_get_type(E->get()->base) == RS::LIGHT_DIRECTIONAL && RSG::storage->light_directional_is_sky_only(E->get()->base))) {
					lights_with_shadow.push_back(E->get());
				}
				//add to list
				directional_lights.push_back(light->instance);
			}
		}

		scene_render->set_directional_shadow_count(lights_with_shadow.size());

		for (int i = 0; i < lights_with_shadow.size(); i++) {
			_light_instance_setup_directional_shadow(i, lights_with_shadow[i], p_cam_transform, p_cam_projection, p_cam_orthogonal, p_cam_vaspect);
		}
	}

	{ //sdfgi
		cull.sdfgi.region_count = 0;

		if (p_render_buffers.is_valid()) {
			cull.sdfgi.cascade_light_count = 0;

			uint32_t prev_cascade = 0xFFFFFFFF;
			uint32_t pending_region_count = scene_render->sdfgi_get_pending_region_count(p_render_buffers);

			for (uint32_t i = 0; i < pending_region_count; i++) {
				cull.sdfgi.region_aabb[i] = scene_render->sdfgi_get_pending_region_bounds(p_render_buffers, i);
				uint32_t region_cascade = scene_render->sdfgi_get_pending_region_cascade(p_render_buffers, i);
				cull.sdfgi.region_cascade[i] = region_cascade;

				if (region_cascade != prev_cascade) {
					cull.sdfgi.cascade_light_index[cull.sdfgi.cascade_light_count] = region_cascade;
					cull.sdfgi.cascade_light_count++;
					prev_cascade = region_cascade;
				}
			}

			cull.sdfgi.region_count = pending_region_count;
		}
	}

	frustum_cull_result.clear();

	{
		uint64_t cull_from = 0;
		uint64_t cull_to = scenario->instance_data.size();

		FrustumCullData cull_data;

		//prepare for eventual thread usage
		cull_data.cull = &cull;
		cull_data.scenario = scenario;
		cull_data.shadow_atlas = p_shadow_atlas;
		cull_data.cam_transform = p_cam_transform;
		cull_data.visible_layers = p_visible_layers;
		cull_data.render_reflection_probe = render_reflection_probe;
//#define DEBUG_CULL_TIME
#ifdef DEBUG_CULL_TIME
		uint64_t time_from = OS::get_singleton()->get_ticks_usec();
#endif
		if (cull_to > thread_cull_threshold) {
			//multiple threads
			for (uint32_t i = 0; i < frustum_cull_result_threads.size(); i++) {
				frustum_cull_result_threads[i].clear();
			}

			RendererThreadPool::singleton->thread_work_pool.do_work(frustum_cull_result_threads.size(), this, &RendererSceneCull::_frustum_cull_threaded, &cull_data);

			for (uint32_t i = 0; i < frustum_cull_result_threads.size(); i++) {
				frustum_cull_result.append_from(frustum_cull_result_threads[i]);
			}

		} else {
			//single threaded
			_frustum_cull(cull_data, frustum_cull_result, cull_from, cull_to);
		}

#ifdef DEBUG_CULL_TIME
		static float time_avg = 0;
		static uint32_t time_count = 0;
		time_avg += double(OS::get_singleton()->get_ticks_usec() - time_from) / 1000.0;
		time_count++;
		print_line("time taken: " + rtos(time_avg / time_count));
#endif

		if (frustum_cull_result.mesh_instances.size()) {
			for (uint64_t i = 0; i < frustum_cull_result.mesh_instances.size(); i++) {
				RSG::storage->mesh_instance_check_for_update(frustum_cull_result.mesh_instances[i]);
			}
			RSG::storage->update_mesh_instances();
		}
	}

	//render shadows
	/* // TODO Render Shadows 2021-02-28 
	for (uint32_t i = 0; i < cull.shadow_count; i++) {
		for (uint32_t j = 0; j < cull.shadows[i].cascade_count; j++) {
			const Cull::Shadow::Cascade &c = cull.shadows[i].cascades[j];
			//			print_line("shadow " + itos(i) + " cascade " + itos(j) + " elements: " + itos(c.cull_result.size()));
			scene_render->light_instance_set_shadow_transform(cull.shadows[i].light_instance, c.projection, c.transform, c.zfar, c.split, j, c.shadow_texel_size, c.bias_scale, c.range_begin, c.uv_scale);
			scene_render->render_shadow(cull.shadows[i].light_instance, p_shadow_atlas, j, frustum_cull_result.directional_shadows[i].cascade_geometry_instances[j], near_plane, p_cam_projection.get_lod_multiplier(), p_screen_lod_threshold);
		}
	}
	*/

	//render SDFGI
	/* // TODO RESTORE FIRE 2021-02-28
	{
		if (cull.sdfgi.region_count > 0) {
			//update regions
			for (uint32_t i = 0; i < cull.sdfgi.region_count; i++) {
				scene_render->render_sdfgi(p_render_buffers, i, frustum_cull_result.sdfgi_region_geometry_instances[i]);
			}
			//check if static lights were culled
			bool static_lights_culled = false;
			for (uint32_t i = 0; i < cull.sdfgi.cascade_light_count; i++) {
				if (frustum_cull_result.sdfgi_cascade_lights[i].size()) {
					static_lights_culled = true;
					break;
				}
			}

			if (static_lights_culled) {
				scene_render->render_sdfgi_static_lights(p_render_buffers, cull.sdfgi.cascade_light_count, cull.sdfgi.cascade_light_index, frustum_cull_result.sdfgi_cascade_lights);
			}
		}

		if (p_render_buffers.is_valid()) {
			scene_render->sdfgi_update_probes(p_render_buffers, p_environment, directional_lights, scenario->dynamic_lights.ptr(), scenario->dynamic_lights.size());
		}
	}
	*/

	//light_samplers_culled=0;

	/*
	print_line("OT: "+rtos( (OS::get_singleton()->get_ticks_usec()-t)/1000.0));
	print_line("OTO: "+itos(p_scenario->octree.get_octant_count()));
	print_line("OTE: "+itos(p_scenario->octree.get_elem_count()));
	print_line("OTP: "+itos(p_scenario->octree.get_pair_count()));
	*/

	/* STEP 3 - PROCESS PORTALS, VALIDATE ROOMS */
	//removed, will replace with culling

	/* STEP 4 - REMOVE FURTHER CULLED OBJECTS, ADD LIGHTS */

	/* STEP 5 - PROCESS POSITIONAL LIGHTS */

	if (p_using_shadows) { //setup shadow maps

		//SortArray<Instance*,_InstanceLightsort> sorter;
		//sorter.sort(light_cull_result,light_cull_count);
		for (uint32_t i = 0; i < (uint32_t)frustum_cull_result.lights.size(); i++) {
			Instance *ins = frustum_cull_result.lights[i];

			if (!p_shadow_atlas.is_valid() || !RSG::storage->light_has_shadow(ins->base)) {
				continue;
			}

			InstanceLightData *light = static_cast<InstanceLightData *>(ins->base_data);

			float coverage = 0.f;

			{ //compute coverage

				Transform cam_xf = p_cam_transform;
				float zn = p_cam_projection.get_z_near();
				Plane p(cam_xf.origin + cam_xf.basis.get_axis(2) * -zn, -cam_xf.basis.get_axis(2)); //camera near plane

				// near plane half width and height
				Vector2 vp_half_extents = p_cam_projection.get_viewport_half_extents();

				switch (RSG::storage->light_get_type(ins->base)) {
					case RS::LIGHT_OMNI: {
						float radius = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_RANGE);

						//get two points parallel to near plane
						Vector3 points[2] = {
							ins->transform.origin,
							ins->transform.origin + cam_xf.basis.get_axis(0) * radius
						};

						if (!p_cam_orthogonal) {
							//if using perspetive, map them to near plane
							for (int j = 0; j < 2; j++) {
								if (p.distance_to(points[j]) < 0) {
									points[j].z = -zn; //small hack to keep size constant when hitting the screen
								}

								p.intersects_segment(cam_xf.origin, points[j], &points[j]); //map to plane
							}
						}

						float screen_diameter = points[0].distance_to(points[1]) * 2;
						coverage = screen_diameter / (vp_half_extents.x + vp_half_extents.y);
					} break;
					case RS::LIGHT_SPOT: {
						float radius = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_RANGE);
						float angle = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_SPOT_ANGLE);

						float w = radius * Math::sin(Math::deg2rad(angle));
						float d = radius * Math::cos(Math::deg2rad(angle));

						Vector3 base = ins->transform.origin - ins->transform.basis.get_axis(2).normalized() * d;

						Vector3 points[2] = {
							base,
							base + cam_xf.basis.get_axis(0) * w
						};

						if (!p_cam_orthogonal) {
							//if using perspetive, map them to near plane
							for (int j = 0; j < 2; j++) {
								if (p.distance_to(points[j]) < 0) {
									points[j].z = -zn; //small hack to keep size constant when hitting the screen
								}

								p.intersects_segment(cam_xf.origin, points[j], &points[j]); //map to plane
							}
						}

						float screen_diameter = points[0].distance_to(points[1]) * 2;
						coverage = screen_diameter / (vp_half_extents.x + vp_half_extents.y);

					} break;
					default: {
						ERR_PRINT("Invalid Light Type");
					}
				}
			}

			if (light->shadow_dirty) {
				light->last_version++;
				light->shadow_dirty = false;
			}

			bool redraw = scene_render->shadow_atlas_update_light(p_shadow_atlas, light->instance, coverage, light->last_version);

			if (redraw) {
				//must redraw!
				RENDER_TIMESTAMP(">Rendering Light " + itos(i));
				light->shadow_dirty = _light_instance_update_shadow(ins, p_cam_transform, p_cam_projection, p_cam_orthogonal, p_cam_vaspect, p_shadow_atlas, scenario, p_screen_lod_threshold);
				RENDER_TIMESTAMP("<Rendering Light " + itos(i));
			}
		}
	}

	//append the directional lights to the lights culled
	for (int i = 0; i < directional_lights.size(); i++) {
		frustum_cull_result.light_instances.push_back(directional_lights[i]);
	}
}
