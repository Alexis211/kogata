/*
** Lua eXtended libraries
*/
#pragma once

#include <lua/lua.h>


#define LX_SYSLIBNAME "lx.sys"
LUAMOD_API int (lx_open_sys) (lua_State *L);


/* open all previous libraries */
LUALIB_API void (lx_openlibs) (lua_State *L);

