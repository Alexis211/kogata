/*
** Lua eXtended libraries
*/
#pragma once

#include <lua/lua.h>


// Helper for libraries
bool lx_checkboolean(lua_State *L, int arg);
void* lx_checklightudata(lua_State *L, int arg);
void setintfield (lua_State *L, const char *key, int value);
void setstrfield (lua_State *L, const char *key, const char* value);

int getintfield(lua_State *L, int arg, const char *key);


#define LX_SYSLIBNAME "lx.sys"
LUAMOD_API int (lx_open_sys) (lua_State *L);

#define LX_IOCTLLIBNAME "lx.ioctl"
LUAMOD_API int (lx_open_ioctl) (lua_State *L);

#define LX_MSGLIBNAME "lx.msg"
LUAMOD_API int (luaopen_cmsgpack) (lua_State *L);
#define lx_open_msg luaopen_cmsgpack

#define LX_DRAWLIBNAME "lx.draw"
LUAMOD_API int (lx_open_draw) (lua_State *L);

#define LX_KBDLIBNAME "lx.kbd"
LUAMOD_API int (lx_open_kbd) (lua_State *L);

/* open all previous libraries */
LUALIB_API void (lx_openlibs) (lua_State *L);

