/*
** Lua eXtended keyboard library
*/

#define lxkbdlib_c
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

#include <kogata/keyboard.h>

#include "lxlib.h"


typedef struct {
  keyboard_t *kbd;
} kbdlib_t;

#define KEYBOARD "lx.kbd.keyboard"

// ====================================================================
//                            FONTS

static int kbd_init(lua_State *L) {
  keyboard_t *kbd = init_keyboard();
  if (kbd == NULL)
    luaL_error(L, "Could not initialize keyboard_t");

  kbdlib_t *k = (kbdlib_t*)lua_newuserdata(L, sizeof(kbdlib_t));
  luaL_getmetatable(L, KEYBOARD);
  lua_setmetatable(L, -2);

  k->kbd = kbd;

  return 1;
}

static int kbd_gc(lua_State *L) {
  kbdlib_t *k = (kbdlib_t*)luaL_checkudata(L, 1, KEYBOARD);

  if(k->kbd == NULL) {
    luaL_error(L, "k->kbd == NULL ?");
  }
  free_keyboard(k->kbd);
  k->kbd = NULL;

  return 0;
}

static int kbd_load_keymap(lua_State *L) {
  kbdlib_t *k = (kbdlib_t*)luaL_checkudata(L, 1, KEYBOARD);
  const char* name = luaL_checkstring(L, 2);

  bool ret = load_keymap(k->kbd, name);
  lua_pushboolean(L, ret);

  return 1;
}

static int kbd_press(lua_State *L) {
  kbdlib_t *k = (kbdlib_t*)luaL_checkudata(L, 1, KEYBOARD);
  int scancode = luaL_checkinteger(L, 2);

  key_t key = keyboard_press(k->kbd, scancode);

  lua_createtable(L, 0, 2);
  setintfield(L, "flags", key.flags);
  if (key.flags & KBD_CHAR)
    setintfield(L, "chr", key.chr);
  else
    setintfield(L, "key", key.key);

  return 1;
}

static int kbd_release(lua_State *L) {
  kbdlib_t *k = (kbdlib_t*)luaL_checkudata(L, 1, KEYBOARD);
  int scancode = luaL_checkinteger(L, 2);

  key_t key = keyboard_release(k->kbd, scancode);

  lua_createtable(L, 0, 2);
  setintfield(L, "flags", key.flags);
  if (key.flags & KBD_CHAR)
    setintfield(L, "chr", key.chr);
  else
    setintfield(L, "key", key.key);

  return 1;
}


/* }====================================================== */

static const luaL_Reg kbdlib[] = {
  {"init",         kbd_init},
  {NULL, NULL}
};


static const luaL_Reg keyboard_meta[] = {
  {"load_keymap", kbd_load_keymap},
  {"press",       kbd_press},
  {"release",     kbd_release},
  {"__gc",        kbd_gc},
  {NULL, NULL}
};


LUAMOD_API int lx_open_kbd (lua_State *L) {
  luaL_newlib(L, kbdlib);

  luaL_newmetatable(L, KEYBOARD);
  luaL_setfuncs(L, keyboard_meta, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  return 1;
}


/* vim: set sts=2 ts=2 sw=2 tw=0 et :*/
