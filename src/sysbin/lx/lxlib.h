/*
** Lua eXtended libraries
*/
#pragma once

#include <lua/lua.h>


#define LX_SYSLIBNAME "lx.sys"
LUAMOD_API int (lx_open_sys) (lua_State *L);

#define LX_SYSLIBNAME "lx.sys"
LUAMOD_API int (lx_open_sys) (lua_State *L);

#define LX_MSGLIBNAME "lx.msg"
LUAMOD_API int (luaopen_cmsgpack) (lua_State *L);
#define lx_open_msg luaopen_cmsgpack

/* open all previous libraries */
LUALIB_API void (lx_openlibs) (lua_State *L);

