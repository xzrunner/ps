#include "particle3d.h"
#include "sprite.h"

#include "lua.h"
#include "lauxlib.h"

static int
lp3d_release(lua_State* L) {
	luaL_checktype(L, 1, LUA_TUSERDATA);
	struct sprite* spr = (struct sprite*)lua_touserdata(L, 1);
	struct p3d_particle_system* ps = spr->s.p3d->spr.ps;
	p3d_release(ps);
	return 0;
}

int
luaopen_ps_c(lua_State* L) {
	luaL_checkversion(L);

	luaL_Reg l[] = {
		{ "p3d_release", lp3d_release },
		{ NULL, NULL },
	};

	luaL_newlib(L, l);
	return 1;
}