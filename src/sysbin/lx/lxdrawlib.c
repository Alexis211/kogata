/*
** Lua eXtended ioctl library
*/

#define lxdrawlib_c
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

#include <kogata/draw.h>

#include "lxlib.h"


typedef struct {
  fb_t *fb;
  fb_region_t subregion;
  bool has_subregion;
} drawlib_surface;

#define SURFACE "lx.draw.surface"

static int draw_from_fd(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  fb_info_t geom;
  geom.width = getintfield(L, 2, "width");
  geom.height = getintfield(L, 2, "height");
  geom.bpp = getintfield(L, 2, "bpp");
  geom.pitch = getintfield(L, 2, "pitch");
  geom.memory_model = getintfield(L, 2, "memory_model");

  drawlib_surface *s = (drawlib_surface*)lua_newuserdata(L, sizeof(drawlib_surface));
  luaL_getmetatable(L, SURFACE);
  lua_setmetatable(L, -2);

  s->fb = g_fb_from_file(fd, &geom);
  if (s->fb == NULL) {
    lua_pop(L, 1);
    lua_pushnil(L);
  } else {
    s->subregion.x = s->subregion.y = 0;
    s->subregion.w = geom.width;
    s->subregion.h = geom.height;
    s->has_subregion = false;
  }
  return 1;
}

static int surface_gc(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);

  if(s->fb == NULL) {
    luaL_error(L, "s->fb == NULL ?");
  }
  g_decref_fb(s->fb);
  s->fb = NULL; 

  return 0;
}

static int surface_sub(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int w, h;
  if (lua_isinteger(L, 4)) {
    w = luaL_checkinteger(L, 4);
  } else {
    w = s->subregion.w - x;
  }
  if (lua_isinteger(L, 5)) {
    h = luaL_checkinteger(L, 5);
  } else {
    h = s->subregion.h - y;
  }

  if (x < 0 || y < 0 || w < 0 || h < 0)
    luaL_error(L, "negative argument is invalid");
  if (x+w > (int)s->subregion.w) luaL_error(L, "w too big");
  if (y+h > (int)s->subregion.h) luaL_error(L, "h too big");

  drawlib_surface *s2 = (drawlib_surface*)lua_newuserdata(L, sizeof(drawlib_surface));
  luaL_getmetatable(L, SURFACE);
  lua_setmetatable(L, -2);

  s2->fb = s->fb;
  s2->has_subregion = true;
  s2->subregion.x = s->subregion.x + x;
  s2->subregion.y = s->subregion.y + y;
  s2->subregion.w = w;
  s2->subregion.h = h;

  return 1;
}

static int surface_rgb(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int r = luaL_checkinteger(L, 2);
  int g = luaL_checkinteger(L, 3);
  int b = luaL_checkinteger(L, 4);

  color_t c = g_color_rgb(s->fb, r, g, b);
  lua_pushlightuserdata(L, (void*)c);
  return 1;
}

static int surface_plot(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  color_t c = (color_t)lx_checklightudata(L, 4);

  g_plot(s->fb, x, y, c);
  return 0;
}

/* }====================================================== */

static const luaL_Reg drawlib[] = {
  {"from_fd",     draw_from_fd},
  {NULL, NULL}
};

static const luaL_Reg surface_meta[] = {
  {"sub",         surface_sub},
  {"rgb",         surface_rgb},
  {"plot",        surface_plot},
  {"__gc",        surface_gc},
  {NULL, NULL}
};


LUAMOD_API int lx_open_draw (lua_State *L) {
  luaL_newlib(L, drawlib);

  luaL_newmetatable(L, SURFACE);
  luaL_setfuncs(L, surface_meta, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  return 1;
}


/* vim: set sts=2 ts=2 sw=2 tw=0 et :*/
