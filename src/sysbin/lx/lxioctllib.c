/*
** Lua eXtended ioctl library
*/

#define lxioctllib_c
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

#include <kogata/syscall.h>
#include <kogata/gip.h>

#include "lxlib.h"


static int ioctl_fb_get_info(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);

  fb_info_t mode;
  int r = sc_ioctl(fd, IOCTL_FB_GET_INFO, &mode);
  if (r == 1) {
    lua_createtable(L, 0, 5);
    setintfield(L, "width", mode.width);
    setintfield(L, "height", mode.height);
    setintfield(L, "pitch", mode.pitch);
    setintfield(L, "bpp", mode.bpp);
    setintfield(L, "memory_model", mode.memory_model);
  } else {
    lua_pushnil(L);
  }

	return 1;
}


/* }====================================================== */

static const luaL_Reg ioctllib[] = {
  {"fb_get_info",		ioctl_fb_get_info},
  {NULL, NULL}
};


LUAMOD_API int lx_open_ioctl (lua_State *L) {
  luaL_newlib(L, ioctllib);
  return 1;
}


/* vim: set sts=2 ts=2 sw=2 tw=0 et :*/
