#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "misc.h"
#include "makeclass.h"

/* {{{ luaH_callinitmethod() */
static int luaH_callinitmethod (lua_State *L, int nargs)
{
	int mytop;

	lua_getfield (L, 1, "init");
	lua_pushvalue (L, 1);
	mytop = lua_gettop (L) - nargs;
	if (!lua_isnil (L, -2))
	{
		lua_insert (L, mytop);
		lua_insert (L, mytop);
		lua_call (L, nargs, LUA_MULTRET);
		return lua_gettop (L) - mytop + 1;
	}
	else
	{
		lua_pop (L, 2 + nargs);
		return 0;
	}
}
/* }}} */

/* {{{ luaH_setgcmethod() */
static void luaH_setgcmethod (lua_State *L)
{
	lua_getfield (L, 1, "del");
	if (!lua_isnil (L, -1))
	{
		lua_newuserdata (L, sizeof (int));
		lua_newtable (L);
		lua_pushvalue (L, -3);
		lua_setfield (L, -2, "__gc");
		lua_pushvalue (L, 1);
		lua_setfield (L, -2, "__index");
		lua_setmetatable (L, -2);
		lua_setfield (L, 1, "__gcobj");
	}
	lua_pop (L, 1);
}
/* }}} */

/* {{{ luaH_isinstance() */
static int luaH_isinstance (lua_State *L)
{
	lua_getfield (L, 2, "prototype");
	lua_getmetatable (L, 1);
	int ret = lua_equal (L, -1, -2);
	lua_pop (L, 2);
	lua_pushboolean (L, ret);
	return 1;
}
/* }}} */

/* {{{ luaH_newclass_new() */
static int luaH_newclass_new (lua_State *L)
{
	int initrets;
	int nargs = lua_gettop (L);

	lua_newtable (L);
	lua_getfield (L, 1, "prototype");
	lua_setmetatable (L, -2);
	lua_replace (L, 1);

	luaH_setgcmethod (L);
	initrets = luaH_callinitmethod (L, nargs);

	return 1 + initrets;
}
/* }}} */

/* {{{ luaH_newclass_newindex() */
static int luaH_newclass_newindex (lua_State *L)
{
	lua_getfield (L, 1, "prototype");
	lua_insert (L, -3);
	lua_settable (L, -3);
	lua_pop (L, 1);
}
/* }}} */

/* {{{ luaH_newclass() */
void luaH_newclass (lua_State *L, const char *name, const luaL_Reg *meths)
{
	static const luaL_Reg nomeths[] = {{NULL}};

	if (!meths)
		meths = nomeths;
	if (!name)
		lua_newtable (L);
	luaL_register (L, name, meths);

	/* Set up the class prototype object. */
	lua_newtable (L);
	lua_pushvalue (L, -1);
	lua_setfield (L, -2, "__index");
	lua_pushcfunction (L, luaH_isinstance);
	lua_setfield (L, -2, "isinstance");
	lua_setfield (L, -2, "prototype");

	/* Set up the class object metatable. */
	lua_newtable (L);
	lua_pushcfunction (L, luaH_newclass_new);
	lua_setfield (L, -2, "__call");
	lua_pushcfunction (L, luaH_newclass_newindex);
	lua_setfield (L, -2, "__newindex");
	lua_setmetatable (L, -2);
}
/* }}} */

/* {{{ luaH_makeclass() */
int luaH_makeclass (lua_State *L)
{
	const char *name = lua_tostring (L, -1);
	luaH_newclass (L, name, NULL);
	return 1;
}
/* }}} */

// vim:foldmethod=marker:ai:ts=4:sw=4:
