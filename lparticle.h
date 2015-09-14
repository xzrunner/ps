#ifdef __cplusplus
extern "C"
{
#endif

#ifndef particle_lua
#define particle_lua

#include "lua.h"
#include "lauxlib.h"

int luaopen_ps_c(lua_State* L);

#endif // particle_lua

#ifdef __cplusplus
}
#endif