/*
** Lua eXtended draw library
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
  font_t *font;
} drawlib_font;

#define FONT "lx.draw.font"

typedef struct {
  fb_t *fb;
  fb_region_t subregion;
  bool has_subregion;
} drawlib_surface;

#define SURFACE "lx.draw.surface"

// ====================================================================
//                            FONTS

static int draw_load_bitmap_font(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  font_t *f = g_load_ascii_bitmap_font(name);
  if (f == NULL) {
    lua_pushnil(L);
    return 1;
  }

  drawlib_font *ff = (drawlib_font*)lua_newuserdata(L, sizeof(drawlib_font));
  luaL_getmetatable(L, FONT);
  lua_setmetatable(L, -2);

  ff->font = f;

  return 1;
}

static int draw_load_ttf_font(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  font_t *f = g_load_ttf_font(name);
  if (f == NULL) {
    lua_pushnil(L);
    return 1;
  }

  drawlib_font *ff = (drawlib_font*)lua_newuserdata(L, sizeof(drawlib_font));
  luaL_getmetatable(L, FONT);
  lua_setmetatable(L, -2);

  ff->font = f;

  return 1;
}

static int font_gc(lua_State *L) {
  drawlib_font *f = (drawlib_font*)luaL_checkudata(L, 1, FONT);

  if(f->font == NULL) {
    luaL_error(L, "f->font == NULL ?");
  }
  g_decref_font(f->font);
  f->font = NULL;

  return 0;
}

static int font_text_width(lua_State *L) {
  drawlib_font *f = (drawlib_font*)luaL_checkudata(L, 1, FONT);
  const char* txt = luaL_checkstring(L, 2);

  lua_pushinteger(L, g_text_width(f->font, txt));
  return 1;
}

static int font_text_height(lua_State *L) {
  drawlib_font *f = (drawlib_font*)luaL_checkudata(L, 1, FONT);
  const char* txt = luaL_checkstring(L, 2);

  lua_pushinteger(L, g_text_height(f->font, txt));
  return 1;
}

// ====================================================================
//                            SURFACES

static int draw_surface_from_fd(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);

  fb_info_t geom;
  geom.width = getintfield(L, 2, "width");
  geom.height = getintfield(L, 2, "height");
  geom.bpp = getintfield(L, 2, "bpp");
  geom.pitch = getintfield(L, 2, "pitch");
  geom.memory_model = getintfield(L, 2, "memory_model");

  fb_t *fb = g_fb_from_file(fd, &geom);
  if (fb == NULL) {
    lua_pushnil(L);
    return 1;
  }

  drawlib_surface *s = (drawlib_surface*)lua_newuserdata(L, sizeof(drawlib_surface));
  luaL_getmetatable(L, SURFACE);
  lua_setmetatable(L, -2);

  s->fb = fb;
  s->subregion.x = s->subregion.y = 0;
  s->subregion.w = geom.width;
  s->subregion.h = geom.height;
  s->has_subregion = false;

  return 1;
}

static int draw_new_surface(lua_State *L) {
  int w = luaL_checkinteger(L, 1);
  int h = luaL_checkinteger(L, 2);
  int bpp = luaL_checkinteger(L, 3);
  bool alpha = lx_checkboolean(L, 4);

  if (bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) {
    luaL_error(L, "Invalid BPP value %d, must be 8 16 24 or 32.", bpp);
  }

  if (alpha && (bpp == 8 || bpp == 24)) {
    luaL_error(L, "Invalid BPP value %d with alpha, must be 16 or 32.", bpp);
  }

  if (w < 0 || h < 0) {
    luaL_error(L, "Invalid dimensions (%d %d)", w, h);
  }

  fb_info_t geom;
  geom.width = w;
  geom.height =  h;
  geom.bpp = bpp;
  geom.pitch = w * bpp / 8;
  if (bpp == 8) geom.memory_model = FB_MM_GREY8;
  if (bpp == 16 && alpha) geom.memory_model = FB_MM_GA16;
  if (bpp == 16 && !alpha) geom.memory_model = FB_MM_RGB16;
  if (bpp == 24) geom.memory_model = FB_MM_RGB24;
  if (bpp == 32 && alpha) geom.memory_model = FB_MM_RGBA32;
  if (bpp == 32 && !alpha) geom.memory_model = FB_MM_RGB32;

  uint8_t *buf = (uint8_t*)malloc(geom.height * geom.pitch);
  if (buf == NULL) {
    luaL_error(L, "Buffer allocation failed");
  }
  memset(buf, 0, geom.height * geom.pitch);

  fb_t *fb = g_fb_from_mem(buf, &geom, true);
  if (fb == NULL) {
    luaL_error(L, "fb_t allocation failed");
  }

  drawlib_surface *s = (drawlib_surface*)lua_newuserdata(L, sizeof(drawlib_surface));
  luaL_getmetatable(L, SURFACE);
  lua_setmetatable(L, -2);

  s->fb = fb;
  s->subregion.x = s->subregion.y = 0;
  s->subregion.w = geom.width;
  s->subregion.h = geom.height;
  s->has_subregion = false;

  return 1;
}

static int draw_load_image(lua_State *L) {
  const char* filename = luaL_checkstring(L, 1);

  fb_t *img = g_load_image(filename);
  if (img == NULL) {
    lua_pushnil(L);
    return 1;
  }

  drawlib_surface *s = (drawlib_surface*)lua_newuserdata(L, sizeof(drawlib_surface));
  luaL_getmetatable(L, SURFACE);
  lua_setmetatable(L, -2);

  s->fb = img;
  s->subregion.x = s->subregion.y = 0;
  s->subregion.w = img->geom.width;
  s->subregion.h = img->geom.height;
  s->has_subregion = false;

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
  if (x > s->subregion.w || y > s->subregion.h)
    luaL_error(L, "out of bounds");

  if (x+w > (int)s->subregion.w) w = s->subregion.w - x;
  if (y+h > (int)s->subregion.h) h = s->subregion.h - y;

  drawlib_surface *s2 = (drawlib_surface*)lua_newuserdata(L, sizeof(drawlib_surface));
  luaL_getmetatable(L, SURFACE);
  lua_setmetatable(L, -2);

  s2->fb = s->fb;
  g_incref_fb(s->fb);
  s2->has_subregion = true;
  s2->subregion.x = s->subregion.x + x;
  s2->subregion.y = s->subregion.y + y;
  s2->subregion.w = w;
  s2->subregion.h = h;

  return 1;
}

static int surface_width(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  lua_pushinteger(L, s->subregion.w);
  return 1;
}

static int surface_height(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  lua_pushinteger(L, s->subregion.h);
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

  // TODO: relative to subregion!
  g_plot(s->fb, x, y, c);
  return 0;
}

static int surface_hline(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int w = luaL_checkinteger(L, 4);
  color_t c = (color_t)lx_checklightudata(L, 5);

  // TODO: relative to subregion!
  g_hline(s->fb, x, y, w, c);
  return 0;
}

static int surface_vline(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int h = luaL_checkinteger(L, 4);
  color_t c = (color_t)lx_checklightudata(L, 5);

  // TODO: relative to subregion!
  g_vline(s->fb, x, y, h, c);
  return 0;
}

static int surface_line(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int x2 = luaL_checkinteger(L, 4);
  int y2 = luaL_checkinteger(L, 5);
  color_t c = (color_t)lx_checklightudata(L, 6);

  // TODO: relative to subregion!
  g_line(s->fb, x, y, x2, y2, c);
  return 0;
}

static int surface_rect(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int w = luaL_checkinteger(L, 4);
  int h = luaL_checkinteger(L, 5);
  color_t c = (color_t)lx_checklightudata(L, 6);

  // TODO: relative to subregion!
  g_rect(s->fb, x, y, w, h, c);
  return 0;
}

static int surface_fillrect(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int w = luaL_checkinteger(L, 4);
  int h = luaL_checkinteger(L, 5);
  color_t c = (color_t)lx_checklightudata(L, 6);

  // TODO: relative to subregion!
  g_fillrect(s->fb, x, y, w, h, c);
  return 0;
}

static int surface_circle(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int r = luaL_checkinteger(L, 4);
  color_t c = (color_t)lx_checklightudata(L, 5);

  // TODO: relative to subregion!
  g_circle(s->fb, x, y, r, c);
  return 0;
}

static int surface_fillcircle(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  int r = luaL_checkinteger(L, 4);
  color_t c = (color_t)lx_checklightudata(L, 5);

  // TODO: relative to subregion!
  g_fillcircle(s->fb, x, y, r, c);
  return 0;
}

static int surface_blit(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  drawlib_surface *s2 = (drawlib_surface*)luaL_checkudata(L, 4, SURFACE);

  // TODO: relative to subregion!
  g_blit_region(s->fb, x, y, s2->fb, s2->subregion);
  return 0;
}

static int surface_write(lua_State *L) {
  drawlib_surface *s = (drawlib_surface*)luaL_checkudata(L, 1, SURFACE);
  int x = luaL_checkinteger(L, 2);
  int y = luaL_checkinteger(L, 3);
  const char *text = luaL_checkstring(L, 4);
  drawlib_font *f = (drawlib_font*)luaL_checkudata(L, 5, FONT);
  int size = luaL_checkinteger(L, 6);
  color_t c = (color_t)lx_checklightudata(L, 7);

  // TODO: relative to subregion!
  g_write(s->fb, x, y, text, f->font, size, c);
  return 0;
}

/* }====================================================== */

static const luaL_Reg drawlib[] = {
  {"surface_from_fd",draw_surface_from_fd},
  {"new_surface",  draw_new_surface},
  {"load_image",   draw_load_image},
  {"load_bitmap_font",draw_load_bitmap_font},
  {"load_ttf_font",draw_load_ttf_font},
  {NULL, NULL}
};

static const luaL_Reg surface_meta[] = {
  {"sub",         surface_sub},
  {"width",       surface_width},
  {"height",      surface_height},
  {"rgb",         surface_rgb},
  {"plot",        surface_plot},
  {"hline",       surface_hline},
  {"vline",       surface_vline},
  {"line",        surface_line},
  {"rect",        surface_rect},
  {"fillrect",    surface_fillrect},
  {"circle",      surface_circle},
  {"fillcircle",  surface_fillcircle},
  {"blit",        surface_blit},
  {"write",       surface_write},
  {"__gc",        surface_gc},
  {NULL, NULL}
};

static const luaL_Reg font_meta[] = {
  {"text_width",  font_text_width},
  {"text_height", font_text_height},
  {"__gc",        font_gc},
  {NULL, NULL}
};


LUAMOD_API int lx_open_draw (lua_State *L) {
  luaL_newlib(L, drawlib);

  luaL_newmetatable(L, SURFACE);
  luaL_setfuncs(L, surface_meta, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, FONT);
  luaL_setfuncs(L, font_meta, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  return 1;
}


/* vim: set sts=2 ts=2 sw=2 tw=0 et :*/
