/* Copyright (c) 2013 Piotr Dobrowolski
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
//#include <sys/timerfd.h>
#include <string.h>
#include <errno.h>

#include "ratchet.h"
#include "misc.h"

#define serial_fd(L, i) (int *) luaL_checkudata (L, i, "ratchet_serial_meta")

/* ---- Namespace Functions ------------------------------------------------- */

/* {{{ rserial_open() */
static int rserial_open (lua_State *L)
{
	const char *devname = luaL_checkstring(L, 1);
	int *fd = (int *) lua_newuserdata (L, sizeof (int));
	*fd = open(devname, O_RDWR | O_NONBLOCK);
	if (*fd < 0)
		return ratchet_error_errno (L, "ratchet.serial.open()", "serial_create");

	luaL_getmetatable (L, "ratchet_serial_meta");
	lua_setmetatable (L, -2);

	return 1;
}
/* }}} */

/* ---- Member Functions ---------------------------------------------------- */

/* {{{ rserial_gc() */
static int rserial_gc (lua_State *L)
{
	int fd = *serial_fd (L, 1);
	if (fd >= 0)
		close (fd);

	return 0;
}
/* }}} */

/* {{{ rserial_get_fd() */
static int rserial_get_fd (lua_State *L)
{
	int fd = *serial_fd (L, 1);
	lua_pushinteger (L, fd);
	return 1;
}
/* }}} */

/* {{{ rserial_close() */
static int rserial_close (lua_State *L)
{
	int *fd = serial_fd (L, 1);
	if (*fd < 0)
		return 0;

	int ret = close (*fd);
	if (ret == -1)
		return ratchet_error_errno (L, "ratchet.serial.close()", "close");
	*fd = -1;

	lua_pushboolean (L, 1);
	return 1;
}
/* }}} */

/* {{{ rserial_read() */
static int rserial_read (lua_State *L)
{
	int tfd = *serial_fd (L, 1);
	size_t buflen = luaL_checkinteger(L, 2);
	ssize_t ret;

	char* buf = malloc(buflen);

	ret = read (tfd, buf, buflen);
	if (ret == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			free(buf);
			lua_pushlightuserdata (L, RATCHET_YIELD_READ);
			lua_pushvalue (L, 1);
			return lua_yieldk (L, 2, 1, rserial_read);
		}

		else
			return ratchet_error_errno (L, "ratchet.serial.read()", "read");
	}

	lua_pushlstring(L, buf, ret);
	//free(buf);

	return 1;
}
/* }}} */

/* {{{ rserial_write() */
static int rserial_write (lua_State *L)
{
	int tfd = *serial_fd (L, 1);
	luaL_checkstring(L, 2);

	size_t buflen;
	const char* buf = lua_tolstring(L, 2, &buflen);
	
	ssize_t ret;
	ret = write(tfd, buf, buflen);

	if (ret == -1)
	{
		return ratchet_error_errno (L, "ratchet.serial.write()", "write");
	}

	lua_pushinteger(L, ret);

	return 1;
}
/* }}} */

/* ---- Public Functions ---------------------------------------------------- */

/* {{{ luaopen_ratchet_serial() */
int luaopen_ratchet_serial (lua_State *L)
{
	/* Static functions in the ratchet.serial namespace. */
	const luaL_Reg funcs[] = {
		{"open", rserial_open},
		{NULL}
	};

	/* Meta-methods for ratchet.serial object metatables. */
	const luaL_Reg metameths[] = {
		{"__gc", rserial_gc},
		{NULL}
	};

	/* Methods in the ratchet.serial class. */
	const luaL_Reg meths[] = {
		/* Documented methods. */
		{"get_fd", rserial_get_fd},
		{"close",  rserial_close},
		{"read",   rserial_read},
		{"write",  rserial_write},
		/* Undocumented, helper methods. */
		{NULL}
	};

	/* Set up the ratchet.serial namespace functions. */
	luaL_newlib (L, funcs);
	lua_pushvalue (L, -1);
	lua_setfield (L, LUA_REGISTRYINDEX, "ratchet_serial_class");

	/* Set up the ratchet.serial class and metatables. */
	luaL_newmetatable (L, "ratchet_serial_meta");
	lua_newtable (L);
	luaL_setfuncs (L, meths, 0);
	lua_setfield (L, -2, "__index");
	luaL_setfuncs (L, metameths, 0);
	lua_pop (L, 1);

	return 1;
}
/* }}} */

// vim:fdm=marker:ai:ts=4:sw=4:noet:
