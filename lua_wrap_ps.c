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
	if (spr->type == TYPE_P3D_SPR) {
		struct p3d_sprite* p3d = spr->data_ext.p3d;
		// already release from buffer
		if (p3d->et) {
			p3d_buffer_remove(p3d);
		}
		spr->data_ext.p3d = NULL;
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 0;
}

static int
lp3d_emitter_clear_time(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_P3D_SPR) {
		spr->data_ext.p3d->et->time = 0;
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 0;
}

static int
lp3d_emitter_update(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_P3D_SPR) {
		float dt = luaL_optnumber(L, 2, 0.033f);
		p3d_emitter_update(spr->data_ext.p3d->et, dt, NULL);
		spr->data_ext.p3d->et->time += dt;
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 0;
}

static int
lp3d_emitter_set_loop(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_P3D_SPR) {
		spr->data_ext.p3d->et->loop = lua_toboolean(L, 2);
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 0;
}

static int
lp3d_emitter_is_finished(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_P3D_SPR) {
		bool finishded = p3d_emitter_is_finished(spr->data_ext.p3d->et);
		lua_pushboolean(L, finishded);
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 1;
}

static int
lp3d_sprite_set_local(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_P3D_SPR) {
		spr->data_ext.p3d->local_mode_draw = lua_toboolean(L, 2);
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 0;
}

static int
lp3d_sprite_set_alone(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	if (spr->type == TYPE_P3D_SPR) {
		bool alone = lua_toboolean(L, 2);
		spr->pkg->ej_pkg;
		struct pack_p3d_spr* spr_cfg = (struct pack_p3d_spr*)(spr->pkg->ej_pkg->data[spr->id]);
		if (spr_cfg->alone != alone) {
			spr_cfg->alone = alone;
			if (alone) {
				p3d_buffer_insert(spr->data_ext.p3d);
			}
		}
	} else if (spr->type == TYPE_P3D_SPR) {
		luaL_error(L, "Use p3d sym.");
	}
	return 0;
}

static int
lp3d_buffer_draw(lua_State* L) {
	p3d_buffer_draw();
	return 0;
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

		{ "p3d_sprite_set_local", lp3d_sprite_set_local },
		{ "p3d_sprite_set_alone", lp3d_sprite_set_alone },

		{ "p3d_buffer_draw", lp3d_buffer_draw },

		{ NULL, NULL },
	};

	luaL_newlib(L, l);
	return 1;
}