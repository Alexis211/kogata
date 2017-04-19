/*
** Lua eXtended system library
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



static void setintfield (lua_State *L, const char *key, int value) {
  lua_pushinteger(L, value);
  lua_setfield(L, -2, key);
}

static void setstrfield (lua_State *L, const char *key, const char* value) {
  lua_pushstring(L, value);
  lua_setfield(L, -2, key);
}


static int sys_dbg_print(lua_State *L) {
  const char *str = luaL_checkstring(L, 1);
  sc_dbg_print(str);
  return 0;
}

static int sys_yield(lua_State *L) {
  sc_yield();
  return 0;
}

static int sys_exit(lua_State *L) {
  int code = luaL_checkinteger(L, 1);
  sc_exit(code);
  return 0;
}

static int sys_usleep(lua_State *L) {
  int usecs = luaL_checkinteger(L, 1);
  sc_usleep(usecs);
  return 0;
}

/*
    Missing syscalls :
    - sys_new_thread, exit_thread -> into specific threading library
    - mmap, mmap_file, mchmap, munmap -> into specific memory-related library ??
*/

static int sys_create(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  int type = luaL_checkinteger(L, 2);
  lua_pushboolean(L, sc_create(name, type));
  return 1;
}

static int sys_delete(lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_pushboolean(L, sc_delete(name));
  return 1;
}

static int sys_move(lua_State *L) {
  const char *oldname = luaL_checkstring(L, 1);
  const char *newname = luaL_checkstring(L, 2);
  lua_pushboolean(L, sc_move(oldname, newname));
  return 1;
}

static int sys_stat(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  stat_t s;
  if (sc_stat(name, &s)) {
    lua_createtable(L, 0, 3);
    setintfield(L, "type", s.type);
    setintfield(L, "access", s.access);
    setintfield(L, "size", s.size);
  } else {
    lua_pushnil(L);
  }
  return 1;
}


static int sys_open(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  int mode = luaL_checkinteger(L, 2);
  fd_t fh = sc_open(name, mode);
  if (fh) {
    lua_pushinteger(L, fh);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_close(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  sc_close(fd);
  return 0;
}

static int sys_read(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int offset = luaL_checkinteger(L, 2);
  int len = luaL_checkinteger(L, 3);

  luaL_Buffer b;
  char* bufptr = luaL_buffinitsize(L, &b, len);
  int ret = sc_read(fd, offset, len, bufptr);

  luaL_pushresultsize(&b, ret);
  lua_pushinteger(L, ret);
  return 2;
}

static int sys_write(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int offset = luaL_checkinteger(L, 2);
  const char* str = luaL_checkstring(L, 3);
  int len = luaL_len(L, 3);

  int ret = sc_write(fd, offset, len, str);
  lua_pushinteger(L, ret);
  return 1;
}

static int sys_readdir(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  int ent_no = luaL_checkinteger(L, 2);

  dirent_t d;
  bool ret = sc_readdir(fd, ent_no, &d);
  if (ret) {
    lua_createtable(L, 0, 4);
    setstrfield(L, "name", d.name);
    setintfield(L, "type", d.st.type);
    setintfield(L, "access", d.st.access);
    setintfield(L, "size", d.st.size);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_stat_open(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  stat_t s;
  if (sc_stat_open(fd, &s)) {
    lua_createtable(L, 0, 3);
    setintfield(L, "type", s.type);
    setintfield(L, "access", s.access);
    setintfield(L, "size", s.size);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_ioctl(lua_State *L) {
  // TODO
  return 0;
}

static int sys_fctl(lua_State *L) {
  // TODO
  return 0;
}

static int sys_select(lua_State *L) {
  // TODO
  return 0;
}


static const luaL_Reg syslib[] = {
  {"dbg_print", sys_dbg_print},
  {"yield",     sys_yield},
  {"exit",      sys_exit},
  {"usleep",    sys_usleep},
  {"create",    sys_create},
  {"delete",    sys_delete},
  {"move",      sys_move},
  {"stat",      sys_stat},
  {"open",      sys_open},
  {"close",     sys_close},
  {"read",      sys_read},
  {"write",     sys_write},
  {"readdir",   sys_readdir},
  {"stat_open", sys_stat_open},

  {NULL, NULL}
};

/* }====================================================== */



LUAMOD_API int lx_open_sys (lua_State *L) {
  luaL_newlib(L, syslib);
  return 1;
}


/* vim: set sts=2 ts=2 sw=2 tw=0 et :*/
