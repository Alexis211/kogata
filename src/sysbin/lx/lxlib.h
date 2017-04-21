/*
** Lua eXtended libraries
*/
#pragma once

#include <lua/lua.h>


// Helper for libraries
bool lx_checkboolean(lua_State *L, int arg);
void setintfield (lua_State *L, const char *key, int value);
void setstrfield (lua_State *L, const char *key, const char* value);


#define LX_SYSLIBNAME "lx.sys"
LUAMOD_API int (lx_open_sys) (lua_State *L);

#define LX_IOCTLLIBNAME "lx.ioctl"
LUAMOD_API int (lx_open_ioctl) (lua_State *L);

#define LX_MSGLIBNAME "lx.msg"
LUAMOD_API int (luaopen_cmsgpack) (lua_State *L);
#define lx_open_msg luaopen_cmsgpack

/* open all previous libraries */
LUALIB_API void (lx_openlibs) (lua_State *L);

