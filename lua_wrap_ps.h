#ifdef __cplusplus
extern "C"
{
#endif

#ifndef lua_wrap_ps
#define lua_wrap_ps

#include "lua.h"
#include "lauxlib.h"

int luaopen_ps_c(lua_State* L);

#endif // lua_wrap_ps

#ifdef __cplusplus
}
#endif