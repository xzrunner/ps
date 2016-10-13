#include "ps_3d.h"
#include "ps_3d_sprite.h"
#include "ps_3d_buffer.h"
#include "sprite.h"
#include "spritepack.h"
#include "ej_ps.h"

#include <dtex_package.h>

#include <lua.h>
#include <lauxlib.h>

#include <assert.h>

static inline void
_traverse_p3d(struct sprite* spr, void (*func)(struct sprite*, void*), void* ud) {
	if (spr->type == TYPE_P3D_SPR) {
		func(spr, ud);
	} else if (spr->type == TYPE_ANIMATION) {
		if (spr->total_frame <= 0) {
			return;
		}
		int frame = spr->frame % spr->total_frame;
		if (frame < 0) {
			frame += spr->total_frame;
		}
		frame += spr->start_frame;
		if (frame < 0) {
			return;
		}
		struct pack_animation* ani = spr->s.ani;
		struct pack_frame* pf = &ani->frame[frame];
		for (int i = 0; i < pf->n; ++i) {
			struct pack_part* pp = &pf->part[i];
			int index = pp->component_id;
			struct sprite* child = spr->data.children[index];
			_traverse_p3d(child, func, ud);
		}
	}
}

static int
lp3d_pause(lua_State* L) {
	ej_ps_pause();
	return 0;
}

static int
lp3d_resume(lua_State* L) {
	ej_ps_resume();
	return 0;
}

static int
lp3d_clear(lua_State* L) {
	p3d_buffer_clear();
	p3d_sprite_clear();
	p3d_clear();
	return 0;
}

static void inline
_emitter_start(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	if (spr->data_ext.p3d && spr->data_ext.p3d->et) {
		p3d_emitter_start(spr->data_ext.p3d->et);
	}
}

static int
lp3d_emitter_start(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	_traverse_p3d(spr, _emitter_start, NULL);
	return 0;
}

static void inline
_emitter_release(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	struct p3d_sprite* p3d = spr->data_ext.p3d;
	if (!p3d) {
		return;
	}
	// already release from buffer
	if (p3d->et) {
		p3d_buffer_remove(p3d);
		p3d_sprite_release(p3d);
	}
	spr->data_ext.p3d = NULL;
}

static int
lp3d_emitter_release(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	_traverse_p3d(spr, _emitter_release, NULL);
	return 0;
}

static void inline
_emitter_clear_time(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	if (spr->data_ext.p3d && spr->data_ext.p3d->et){
		spr->data_ext.p3d->et->time = 0;
	}
}

static int
lp3d_emitter_clear_time(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	_traverse_p3d(spr, _emitter_clear_time, NULL);
	return 0;
}

static void inline
_emitter_update(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	if (spr->data_ext.p3d && spr->data_ext.p3d->et) {
		float dt = *(float*)ud;
		p3d_emitter_update(spr->data_ext.p3d->et, dt, NULL);
		spr->data_ext.p3d->et->time += dt;
	}
}

static int
lp3d_emitter_update(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	float dt = luaL_optnumber(L, 2, 0.033f);
	_traverse_p3d(spr, _emitter_update, &dt);
	return 0;
}

static void inline
_emitter_set_loop(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	bool loop = *(bool*)ud;
	if (spr->data_ext.p3d && spr->data_ext.p3d->et) {
		spr->data_ext.p3d->et->loop = loop;
	}
}

static int
lp3d_emitter_set_loop(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	bool loop = lua_toboolean(L, 2);
	_traverse_p3d(spr, _emitter_set_loop, &loop);
	return 0;
}

static void inline
_emitter_is_finished(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	if (spr->data_ext.p3d && spr->data_ext.p3d->et) {
		bool finishded = p3d_emitter_is_finished(spr->data_ext.p3d->et);
		if (!finishded) {
			*(bool*)ud = false;
		}
	}
}

static int
lp3d_emitter_is_finished(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	bool finished = true;
	_traverse_p3d(spr, _emitter_is_finished, &finished);
	lua_pushboolean(L, finished);
	return 1;
}

static void inline
_sprite_set_local(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	bool local = *(bool*)ud;
	spr->data_ext.p3d->local_mode_draw = local;
}

static int
lp3d_sprite_set_local(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	bool local = lua_toboolean(L, 2);
	_traverse_p3d(spr, _sprite_set_local, &local);
	return 0;
}

static void inline
_sprite_set_alone(struct sprite* spr, void* ud) {
	assert(spr->type == TYPE_P3D_SPR);
	bool alone = *(bool*)ud;
	if (alone == spr->s.p3d_spr->alone || !spr->data_ext.p3d) {
		return;
	}
	if (alone) {
		spr->data_ext.p3d->ref_id = ej_sprite_sprite_ref(1);
	}
	spr->s.p3d_spr->alone = alone;
}

static int
lp3d_sprite_set_alone(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	bool alone = lua_toboolean(L, 2);
	_traverse_p3d(spr, _sprite_set_alone, &alone);
	return 0;
}

static int
lp3d_buffer_draw(lua_State* L) {
	double x = luaL_optnumber(L, 1, 0);
	double y = luaL_optnumber(L, 2, 0);
	double s = luaL_optnumber(L, 3, 1);
	p3d_buffer_draw(x, y, s);
	return 0;
}

int
luaopen_ps_c(lua_State* L) {
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{ "p3d_pause", lp3d_pause },
		{ "p3d_resume", lp3d_resume },
		{ "p3d_clear", lp3d_clear },

		{ "p3d_emitter_start", lp3d_emitter_start },
		{ "p3d_emitter_release", lp3d_emitter_release },
		{ "p3d_emitter_clear_time", lp3d_emitter_clear_time },
		{ "p3d_emitter_update", lp3d_emitter_update },
		{ "p3d_emitter_set_loop", lp3d_emitter_set_loop },
		{ "p3d_emitter_is_finished", lp3d_emitter_is_finished },

		{ "p3d_sprite_set_local", lp3d_sprite_set_local },
		{ "p3d_sprite_set_alone", lp3d_sprite_set_alone },

		{ "p3d_buffer_draw", lp3d_buffer_draw },

		{ NULL, NULL },
	};

	luaL_newlib(L, l);
	return 1;
}