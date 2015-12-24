#include "particle3d.h"
#include "sprite.h"
#include "spritepack.h"
#include "ej_ps.h"

#include "lua.h"
#include "lauxlib.h"

static int
lp3d_release(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	struct p3d_particle_system* ps = spr->s.p3d->spr.ps;
	p3d_release(ps);
	spr->s.p3d->spr.ps = NULL;
	return 0;
}

static int
lp3d_clear_time(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	spr->s.p3d->spr.ps_time = 0;
	return 0;
}

static int
lp3d_update(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	float dt = luaL_optnumber(L, 2, 0.033f);
	p3d_update(spr->s.p3d->spr.ps, dt);
	spr->s.p3d->spr.ps_time += dt;
	return 0;
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
lp3d_set_loop(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	struct p3d_particle_system* ps = spr->s.p3d->spr.ps;
	ps->loop = lua_toboolean(L, 2);
	return 0;
}

int
luaopen_ps_c(lua_State* L) {
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{ "p3d_release", lp3d_release },
		{ "p3d_clear_time", lp3d_clear_time },
		{ "p3d_update", lp3d_update },
		{ "p3d_pause", lp3d_pause },
		{ "p3d_resume", lp3d_resume },
		{ "p3d_set_loop", lp3d_set_loop },

		{ NULL, NULL },
	};

	luaL_newlib(L, l);
	return 1;
}