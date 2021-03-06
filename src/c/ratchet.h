#ifndef __RATCHET_H
#define __RATCHET_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

const char *ratchet_version (void);

int luaopen_ratchet (lua_State *L);
int luaopen_ratchet_error (lua_State *L);
int luaopen_ratchet_socket (lua_State *L);
int luaopen_ratchet_ssl (lua_State *L);
int luaopen_ratchet_timerfd (lua_State *L);
int luaopen_ratchet_serial (lua_State *L);
int luaopen_ratchet_zmqsocket (lua_State *L);
int luaopen_ratchet_dns (lua_State *L);
int luaopen_ratchet_dns_hosts (lua_State *L);
int luaopen_ratchet_dns_resolv_conf (lua_State *L);
int luaopen_ratchet_exec (lua_State *L);

/* Error handling convenience functions. */
#define ratchet_error_errno(L, f, s) ratchet_error_errno_ln (L, f, s, __FILE__, __LINE__)
#define ratchet_error_top(L, f, c) ratchet_error_top_ln (L, f, c, __FILE__, __LINE__)
#define ratchet_error_str(L, f, c, ...) ratchet_error_str_ln (L, f, c, __FILE__, __LINE__, __VA_ARGS__)

void ratchet_error_push_constructor (lua_State *L);
void ratchet_error_push_code (lua_State *L, int e);
int ratchet_error_errno_ln (lua_State *L, const char *function, const char *syscall, const char *file, int line);
int ratchet_error_top_ln (lua_State *L, const char *function, const char *code, const char *file, int line);
int ratchet_error_str_ln (lua_State *L, const char *function, const char *code, const char *file, int line, const char *description, ...);

#define RATCHET_YIELD_GET ((void *) 1)
#define RATCHET_YIELD_WRITE ((void *) 2)
#define RATCHET_YIELD_READ ((void *) 3)
#define RATCHET_YIELD_TIMEOUT ((void *) 4)
#define RATCHET_YIELD_WAITALL ((void *) 5)
#define RATCHET_YIELD_MULTIRW ((void *) 6)
#define RATCHET_YIELD_PAUSE ((void *) 7)
#define RATCHET_YIELD_SIGNAL ((void *) 8)

#endif
// vim:fdm=marker:ai:ts=4:sw=4:noet:
