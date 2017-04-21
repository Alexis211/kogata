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
    - ioctl, fctl -> into ioctl library
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

static int sys_select(lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  int nfds = luaL_len(L, 1);
  int timeout = luaL_checkinteger(L, 2);

  sel_fd_t *fds = (sel_fd_t*)malloc(nfds*sizeof(sel_fd_t));
  if (fds == NULL)
    luaL_error(L, "Out of memory, could not allocate %d sel_fd_t", nfds);

  for (int i = 0; i < nfds; i++) {
    lua_rawgeti(L, 1, i+1);
    luaL_checktype(L, -1, LUA_TTABLE);

    lua_rawgeti(L, -1, 1);
    fds[i].fd = luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    
    lua_rawgeti(L, -1, 2);
    fds[i].req_flags = luaL_checkinteger(L, -1);
    lua_pop(L, 2);
  }

  bool result = sc_select(fds, nfds, timeout);

  for (int i = 0; i < nfds; i++) {
    lua_rawgeti(L, 1, i+1);
    lua_pushinteger(L, fds[i].got_flags);
    lua_rawseti(L, -2, 3);
    lua_pop(L, 1);
  }

  free(fds);

  lua_pushboolean(L, result);
  return 1;
}

static int sys_make_channel(lua_State *L) {
  bool blocking = lx_checkboolean(L, 1);
  fd_pair_t pair = sc_make_channel(blocking);
  lua_pushinteger(L, pair.a);
  lua_pushinteger(L, pair.b);
  return 2;
}

static int sys_make_shm(lua_State *L) {
  int size = luaL_checkinteger(L, 1);
  int fd = sc_make_shm(size);
  if (fd) {
    lua_pushinteger(L, fd);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_gen_token(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  token_t t;
  if (sc_gen_token(fd, &t)) {
    lua_pushlstring(L, t.bytes, TOKEN_LENGTH);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_use_token(lua_State *L) {
  const char* tok = luaL_checkstring(L, 1);
  if (!(luaL_len(L, 1) == TOKEN_LENGTH)) {
    luaL_argerror(L, 1, "invalid token length");
  }
  token_t t;
  memcpy(t.bytes, tok, TOKEN_LENGTH);
  fd_t f = sc_use_token(&t);
  if (f) {
    lua_pushinteger(L, f);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_make_fs(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  const char* driver = luaL_checkstring(L, 2);
  int source = luaL_checkinteger(L, 3);
  const char* options = luaL_checkstring(L, 4);
  lua_pushboolean(L, sc_make_fs(name, driver, source, options));
  return 1;
}

static int sys_fs_add_source(lua_State *L) {
  const char* fs = luaL_checkstring(L, 1);
  int source = luaL_checkinteger(L, 2);
  const char* options = luaL_checkstring(L, 3);
  lua_pushboolean(L, sc_fs_add_source(fs, source, options));
  return 1;
}

static int sys_fs_subfs(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  const char* orig_fs = luaL_checkstring(L, 2);
  const char* root = luaL_checkstring(L, 3);
  int ok_modes = luaL_checkinteger(L, 4);
  lua_pushboolean(L, sc_fs_subfs(name, orig_fs, root, ok_modes));
  return 1;
}

static int sys_fs_remove(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  sc_fs_remove(name);
  return 0;
}

static int sys_new_proc(lua_State *L) {
  lua_pushinteger(L, sc_new_proc());
  return 1;
}

static int sys_bind_fs(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  const char* new_name = luaL_checkstring(L, 2);
  const char* name = luaL_checkstring(L, 3);
  lua_pushboolean(L, sc_bind_fs(pid, new_name, name));
  return 1;
}

static int sys_bind_subfs(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  const char* new_name = luaL_checkstring(L, 2);
  const char* fs = luaL_checkstring(L, 3);
  const char* root = luaL_checkstring(L, 4);
  int ok_modes = luaL_checkinteger(L, 5);
  lua_pushboolean(L, sc_bind_subfs(pid, new_name, fs, root, ok_modes));
  return 1;
}

static int sys_bind_make_fs(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  const char* new_name = luaL_checkstring(L, 2);
  const char* driver = luaL_checkstring(L, 3);
  fd_t source = luaL_checkinteger(L, 4);
  const char* options = luaL_checkstring(L, 5);
  lua_pushboolean(L, sc_bind_make_fs(pid, new_name, driver, source, options));
  return 1;
}

static int sys_bind_fd(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  int new_fd = luaL_checkinteger(L, 2);
  int fd = luaL_checkinteger(L, 3);
  lua_pushboolean(L, sc_bind_fd(pid, new_fd, fd));
  return 1;
}

static int sys_proc_exec(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  const char *name = luaL_checkstring(L, 2);
  lua_pushboolean(L, sc_proc_exec(pid, name));
  return 1;
}

static int sys_proc_status(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  proc_status_t s;
  if (sc_proc_status(pid, &s)) {
    lua_createtable(L, 0, 3);
    setintfield(L, "pid", s.pid);
    setintfield(L, "status",s.status);
    setintfield(L, "exit_code", s.exit_code);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_proc_kill(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  proc_status_t s;
  if (sc_proc_kill(pid, &s)) {
    lua_createtable(L, 0, 3);
    setintfield(L, "pid", s.pid);
    setintfield(L, "status",s.status);
    setintfield(L, "exit_code", s.exit_code);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int sys_proc_wait(lua_State *L) {
  int pid = luaL_checkinteger(L, 1);
  bool wait = lx_checkboolean(L, 2);
  proc_status_t s;
  sc_proc_wait(pid, wait, &s);
  lua_createtable(L, 0, 3);
  setintfield(L, "pid", s.pid);
  setintfield(L, "status",s.status);
  setintfield(L, "exit_code", s.exit_code);
  return 1;
}

/* }====================================================== */

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
  {"select",    sys_select},
  {"make_channel",sys_make_channel},
  {"make_shm",  sys_make_shm},
  {"gen_token", sys_gen_token},
  {"use_token", sys_use_token},
  {"make_fs",   sys_make_fs},
  {"fs_add_source",sys_fs_add_source},
  {"fs_subfs",  sys_fs_subfs},
  {"fs_remove", sys_fs_remove},
  {"new_proc",  sys_new_proc},
  {"bind_fs",   sys_bind_fs},
  {"bind_subfs",sys_bind_subfs},
  {"bind_make_fs",sys_bind_make_fs},
  {"bind_fd",   sys_bind_fd},
  {"proc_exec", sys_proc_exec},
  {"proc_status",sys_proc_status},
  {"proc_kill", sys_proc_kill},
  {"proc_wait", sys_proc_wait},
  
  {NULL, NULL}
};


LUAMOD_API int lx_open_sys (lua_State *L) {
  luaL_newlib(L, syslib);
  return 1;
}


/* vim: set sts=2 ts=2 sw=2 tw=0 et :*/
