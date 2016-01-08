#include "ps_3d.h"
#include "sprite.h"
#include "spritepack.h"
#include "ej_ps.h"

#include "lua.h"
#include "lauxlib.h"

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
lp3d_emitter_release(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_PARTICLE3D) {
		p3d_emitter_release(spr->ext.p3d);
		spr->ext.p3d = NULL;
	}
	return 0;
}

static int
lp3d_emitter_clear_time(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_PARTICLE3D) {
		spr->ext.p3d->time = 0;
	}
	return 0;
}

static int
lp3d_emitter_update(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_PARTICLE3D) {
		float dt = luaL_optnumber(L, 2, 0.033f);
		p3d_emitter_update(spr->ext.p3d, dt, NULL);
		spr->ext.p3d->time += dt;
	}
	return 0;
}

static int
lp3d_emitter_set_loop(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_PARTICLE3D) {
		spr->ext.p3d->loop = lua_toboolean(L, 2);
	}
	return 0;
}

static int
lp3d_emitter_is_finished(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_PARTICLE3D) {
		bool finishded = p3d_emitter_is_finished(spr->ext.p3d);
		lua_pushboolean(L, finishded);
	}
	return 1;
}

int
luaopen_ps_c(lua_State* L) {
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{ "p3d_pause", lp3d_pause },
		{ "p3d_resume", lp3d_resume },

		{ "p3d_emitter_release", lp3d_emitter_release },
		{ "p3d_emitter_clear_time", lp3d_emitter_clear_time },
		{ "p3d_emitter_update", lp3d_emitter_update },
		{ "p3d_emitter_set_loop", lp3d_emitter_set_loop },
		{ "p3d_emitter_is_finished", lp3d_emitter_is_finished },

		{ NULL, NULL },
	};

	luaL_newlib(L, l);
	return 1;
}