/*************************************************************************/
/*  gd_navigation_server.cpp                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md)    */
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

#include "gd_navigation_server.h"

#include "core/os/mutex.h"

#ifndef _3D_DISABLED
#include "navigation_mesh_generator.h"
#endif

/**
	@author AndreaCatania
*/

/// Creates a struct for each function and a function that once called creates
/// an instance of that struct with the submited parameters.
/// Then, that struct is stored in an array; the `sync` function consume that array.

#define COMMAND_1(F_NAME, T_0, D_0)                      \
	struct MERGE(F_NAME, _command) : public SetCommand { \
		T_0 d_0;                                         \
		MERGE(F_NAME, _command)                          \
		(T_0 p_d_0) :                                    \
				d_0(p_d_0) {}                            \
		virtual void exec(GdNavigationServer *server) {  \
			server->MERGE(_cmd_, F_NAME)(d_0);           \
		}                                                \
	};                                                   \
	void GdNavigationServer::F_NAME(T_0 D_0) const {     \
		auto cmd = memnew(MERGE(F_NAME, _command)(       \
				D_0));                                   \
		add_command(cmd);                                \
	}                                                    \
	void GdNavigationServer::MERGE(_cmd_, F_NAME)(T_0 D_0)

#define COMMAND_2(F_NAME, T_0, D_0, T_1, D_1)                 \
	struct MERGE(F_NAME, _command) : public SetCommand {      \
		T_0 d_0;                                              \
		T_1 d_1;                                              \
		MERGE(F_NAME, _command)                               \
		(                                                     \
				T_0 p_d_0,                                    \
				T_1 p_d_1) :                                  \
				d_0(p_d_0),                                   \
				d_1(p_d_1) {}                                 \
		virtual void exec(GdNavigationServer *server) {       \
			server->MERGE(_cmd_, F_NAME)(d_0, d_1);           \
		}                                                     \
	};                                                        \
	void GdNavigationServer::F_NAME(T_0 D_0, T_1 D_1) const { \
		auto cmd = memnew(MERGE(F_NAME, _command)(            \
				D_0,                                          \
				D_1));                                        \
		add_command(cmd);                                     \
	}                                                         \
	void GdNavigationServer::MERGE(_cmd_, F_NAME)(T_0 D_0, T_1 D_1)

#define COMMAND_4(F_NAME, T_0, D_0, T_1, D_1, T_2, D_2, T_3, D_3)               \
	struct MERGE(F_NAME, _command) : public SetCommand {                        \
		T_0 d_0;                                                                \
		T_1 d_1;                                                                \
		T_2 d_2;                                                                \
		T_3 d_3;                                                                \
		MERGE(F_NAME, _command)                                                 \
		(                                                                       \
				T_0 p_d_0,                                                      \
				T_1 p_d_1,                                                      \
				T_2 p_d_2,                                                      \
				T_3 p_d_3) :                                                    \
				d_0(p_d_0),                                                     \
				d_1(p_d_1),                                                     \
				d_2(p_d_2),                                                     \
				d_3(p_d_3) {}                                                   \
		virtual void exec(GdNavigationServer *server) {                         \
			server->MERGE(_cmd_, F_NAME)(d_0, d_1, d_2, d_3);                   \
		}                                                                       \
	};                                                                          \
	void GdNavigationServer::F_NAME(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3) const { \
		auto cmd = memnew(MERGE(F_NAME, _command)(                              \
				D_0,                                                            \
				D_1,                                                            \
				D_2,                                                            \
				D_3));                                                          \
		add_command(cmd);                                                       \
	}                                                                           \
	void GdNavigationServer::MERGE(_cmd_, F_NAME)(T_0 D_0, T_1 D_1, T_2 D_2, T_3 D_3)

GdNavigationServer::GdNavigationServer() :
		NavigationServer(),
		active(true) {
	commands_mutex = Mutex::create();
	operations_mutex = Mutex::create();
}

GdNavigationServer::~GdNavigationServer() {}

void GdNavigationServer::add_command(SetCommand *command) const {
	auto mut_this = const_cast<GdNavigationServer *>(this);
	commands_mutex->lock();
	mut_this->commands.push_back(command);
	commands_mutex->unlock();
}

RID GdNavigationServer::map_create() const {
	auto mut_this = const_cast<GdNavigationServer *>(this);
	mut_this->operations_mutex->lock();
	NavMap *space = memnew(NavMap);
	RID rid = map_owner.make_rid(space);
	space->set_self(rid);
	mut_this->operations_mutex->unlock();
	return rid;
}

COMMAND_2(map_set_active, RID, p_map, bool, p_active) {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND(map == NULL);

	if (p_active) {
		if (!map_is_active(p_map)) {
			active_maps.push_back(map);
		}
	} else {
		active_maps.erase(map);
	}
}

bool GdNavigationServer::map_is_active(RID p_map) const {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND_V(map == NULL, false);

	return active_maps.find(map) >= 0;
}

COMMAND_2(map_set_up, RID, p_map, Vector3, p_up) {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND(map == NULL);

	map->set_up(p_up);
}

Vector3 GdNavigationServer::map_get_up(RID p_map) const {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND_V(map == NULL, Vector3());

	return map->get_up();
}

COMMAND_2(map_set_cell_size, RID, p_map, real_t, p_cell_size) {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND(map == NULL);

	map->set_cell_size(p_cell_size);
}

real_t GdNavigationServer::map_get_cell_size(RID p_map) const {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND_V(map == NULL, 0);

	return map->get_cell_size();
}

COMMAND_2(map_set_edge_connection_margin, RID, p_map, real_t, p_connection_margin) {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND(map == NULL);

	map->set_edge_connection_margin(p_connection_margin);
}

real_t GdNavigationServer::map_get_edge_connection_margin(RID p_map) const {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND_V(map == NULL, 0);

	return map->get_edge_connection_margin();
}

Vector<Vector3> GdNavigationServer::map_get_path(RID p_map, Vector3 p_origin, Vector3 p_destination, bool p_optimize) const {
	NavMap *map = map_owner.get(p_map);
	ERR_FAIL_COND_V(map == NULL, Vector<Vector3>());

	return map->get_path(p_origin, p_destination, p_optimize);
}

RID GdNavigationServer::region_create() const {
	auto mut_this = const_cast<GdNavigationServer *>(this);
	mut_this->operations_mutex->lock();
	NavRegion *reg = memnew(NavRegion);
	RID rid = region_owner.make_rid(reg);
	reg->set_self(rid);
	mut_this->operations_mutex->unlock();
	return rid;
}

COMMAND_2(region_set_map, RID, p_region, RID, p_map) {
	NavRegion *region = region_owner.get(p_region);
	ERR_FAIL_COND(region == NULL);

	if (region->get_map() != NULL) {

		if (region->get_map()->get_self() == p_map)
			return; // Pointless

		region->get_map()->remove_region(region);
		region->set_map(NULL);
	}

	if (p_map.is_valid()) {
		NavMap *map = map_owner.get(p_map);
		ERR_FAIL_COND(map == NULL);

		map->add_region(region);
		region->set_map(map);
	}
}

COMMAND_2(region_set_transform, RID, p_region, Transform, p_transform) {
	NavRegion *region = region_owner.get(p_region);
	ERR_FAIL_COND(region == NULL);

	region->set_transform(p_transform);
}

COMMAND_2(region_set_navmesh, RID, p_region, Ref<NavigationMesh>, p_nav_mesh) {
	NavRegion *region = region_owner.get(p_region);
	ERR_FAIL_COND(region == NULL);

	region->set_mesh(p_nav_mesh);
}

void GdNavigationServer::region_bake_navmesh(Ref<NavigationMesh> r_mesh, Node *p_node) const {
	ERR_FAIL_COND(r_mesh.is_null());
	ERR_FAIL_COND(p_node == NULL);

#ifndef _3D_DISABLED
	NavigationMeshGenerator::get_singleton()->clear(r_mesh);
	NavigationMeshGenerator::get_singleton()->bake(r_mesh, p_node);
#endif
}

RID GdNavigationServer::agent_create() const {
	auto mut_this = const_cast<GdNavigationServer *>(this);
	mut_this->operations_mutex->lock();
	RvoAgent *agent = memnew(RvoAgent());
	RID rid = agent_owner.make_rid(agent);
	agent->set_self(rid);
	mut_this->operations_mutex->unlock();
	return rid;
}

COMMAND_2(agent_set_map, RID, p_agent, RID, p_map) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	if (agent->get_map()) {
		if (agent->get_map()->get_self() == p_map)
			return; // Pointless

		agent->get_map()->remove_agent(agent);
	}

	agent->set_map(NULL);

	if (p_map.is_valid()) {
		NavMap *map = map_owner.get(p_map);
		ERR_FAIL_COND(map == NULL);

		agent->set_map(map);
		map->add_agent(agent);

		if (agent->has_callback()) {
			map->set_agent_as_controlled(agent);
		}
	}
}

COMMAND_2(agent_set_neighbor_dist, RID, p_agent, real_t, p_dist) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->neighborDist_ = p_dist;
}

COMMAND_2(agent_set_max_neighbors, RID, p_agent, int, p_count) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->maxNeighbors_ = p_count;
}

COMMAND_2(agent_set_time_horizon, RID, p_agent, real_t, p_time) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->timeHorizon_ = p_time;
}

COMMAND_2(agent_set_radius, RID, p_agent, real_t, p_radius) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->radius_ = p_radius;
}

COMMAND_2(agent_set_max_speed, RID, p_agent, real_t, p_max_speed) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->maxSpeed_ = p_max_speed;
}

COMMAND_2(agent_set_velocity, RID, p_agent, Vector3, p_velocity) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->velocity_ = RVO::Vector3(p_velocity.x, p_velocity.y, p_velocity.z);
}

COMMAND_2(agent_set_target_velocity, RID, p_agent, Vector3, p_velocity) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->prefVelocity_ = RVO::Vector3(p_velocity.x, p_velocity.y, p_velocity.z);
}

COMMAND_2(agent_set_position, RID, p_agent, Vector3, p_position) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->position_ = RVO::Vector3(p_position.x, p_position.y, p_position.z);
}

COMMAND_2(agent_set_ignore_y, RID, p_agent, bool, p_ignore) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->get_agent()->ignore_y_ = p_ignore;
}

bool GdNavigationServer::agent_is_map_changed(RID p_agent) const {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND_V(agent == NULL, false);

	return agent->is_map_changed();
}

COMMAND_4(agent_set_callback, RID, p_agent, Object *, p_receiver, StringName, p_method, Variant, p_udata) {
	RvoAgent *agent = agent_owner.get(p_agent);
	ERR_FAIL_COND(agent == NULL);

	agent->set_callback(p_receiver == NULL ? 0 : p_receiver->get_instance_id(), p_method, p_udata);

	if (agent->get_map()) {
		if (p_receiver == NULL) {
			agent->get_map()->remove_agent_as_controlled(agent);
		} else {
			agent->get_map()->set_agent_as_controlled(agent);
		}
	}
}

COMMAND_1(free, RID, p_object) {
	if (map_owner.owns(p_object)) {
		NavMap *map = map_owner.get(p_object);

		// Removes any assigned region
		std::vector<NavRegion *> regions = map->get_regions();
		for (size_t i(0); i < regions.size(); i++) {
			map->remove_region(regions[i]);
			regions[i]->set_map(NULL);
		}

		// Remove any assigned agent
		std::vector<RvoAgent *> agents = map->get_agents();
		for (size_t i(0); i < agents.size(); i++) {
			map->remove_agent(agents[i]);
			agents[i]->set_map(NULL);
		}

		active_maps.erase(map);
		map_owner.free(p_object);
		memdelete(map);

	} else if (region_owner.owns(p_object)) {
		NavRegion *region = region_owner.get(p_object);

		// Removes this region from the map if assigned
		if (region->get_map() != NULL) {
			region->get_map()->remove_region(region);
			region->set_map(NULL);
		}

		region_owner.free(p_object);
		memdelete(region);

	} else if (agent_owner.owns(p_object)) {
		RvoAgent *agent = agent_owner.get(p_object);

		// Removes this agent from the map if assigned
		if (agent->get_map() != NULL) {
			agent->get_map()->remove_agent(agent);
			agent->set_map(NULL);
		}

		agent_owner.free(p_object);
		memdelete(agent);

	} else {
		ERR_FAIL_COND("Invalid ID.");
	}
}

void GdNavigationServer::set_active(bool p_active) const {
	auto mut_this = const_cast<GdNavigationServer *>(this);
	mut_this->operations_mutex->lock();
	mut_this->active = p_active;
	mut_this->operations_mutex->unlock();
}

void GdNavigationServer::step(real_t p_delta_time) {
	if (!active) {
		return;
	}

	// With c++ we can't be 100% sure this is called in single thread so use the mutex.
	commands_mutex->lock();
	operations_mutex->lock();
	for (size_t i(0); i < commands.size(); i++) {
		commands[i]->exec(this);
		memdelete(commands[i]);
	}
	commands.clear();
	operations_mutex->unlock();
	commands_mutex->unlock();

	// These are internal operations so don't need to be shielded.
	for (int i(0); i < active_maps.size(); i++) {
		active_maps[i]->sync();
		active_maps[i]->step(p_delta_time);
		active_maps[i]->dispatch_callbacks();
	}
}

#undef COMMAND_1
#undef COMMAND_2
#undef COMMAND_4
