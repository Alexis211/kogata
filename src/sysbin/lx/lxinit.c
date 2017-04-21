

#define lxinit_c
#define LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, "_PRELOAD");
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove _PRELOAD table
*/

#include <lua/lprefix.h>


#include <stddef.h>

#include <lua/lua.h>

#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lxlib.h"


static const luaL_Reg loadedlibs[] = {
  {LX_SYSLIBNAME, lx_open_sys},
  {LX_MSGLIBNAME, lx_open_msg},
  {LX_IOCTLLIBNAME, lx_open_ioctl},
  {LX_DRAWLIBNAME, lx_open_draw},
  {NULL, NULL}
};

LUALIB_API void lx_openlibs (lua_State *L) {
  const luaL_Reg *lib;
  /* "require" functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    luaL_requiref(L, lib->name, lib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }
}

