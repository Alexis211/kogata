/*
** Lua eXtended library helpers
*/

#define lxsyslib_c
#define LUA_LIB

#include <lua/lprefix.h>


#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lua/lua.h>

#include <lua/lauxlib.h>
#include <lua/lualib.h>

#include "lxlib.h"


bool lx_checkboolean(lua_State *L, int arg) {
  if (!lua_isboolean(L, arg)) {
    luaL_argerror(L, arg, "expected boolean");
  }
  return lua_toboolean(L, arg);
}


void setintfield (lua_State *L, const char *key, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, key);
}

void setstrfield (lua_State *L, const char *key, const char* value) {
  lua_pushstring(L, value);
  lua_setfield(L, -2, key);
}
