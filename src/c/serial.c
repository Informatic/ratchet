/* Copyright (c) 2010 Ian C. Good
 * Copyright (c) 2013 Piotr Dobrowolski
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
#include <termios.h>
#include <string.h>
#include <errno.h>

#include "ratchet.h"
#include "misc.h"
#include "baudtable.h"

#define serial_fd(L, i) (int *) luaL_checkudata (L, i, "ratchet_serial_meta")

struct baudentry *
baudtable_lookup(int value)
{
	struct baudentry *ptr;
	for(ptr = baudtable; ptr->speed != -1; ptr++)
		if(ptr->speed == value || ptr->symbol == value)
			return ptr;

	return NULL;
}

/* ---- Namespace Functions ------------------------------------------------- */

/* {{{ rserial_new() */
static int rserial_new (lua_State *L)
{
	const char *devname = luaL_checkstring(L, 1);
	int *fd = (int *) lua_newuserdata (L, sizeof (int));
	*fd = open(devname, O_RDWR | O_NONBLOCK);
	if (*fd < 0)
		return ratchet_error_errno (L, "ratchet.serial.new()", "open");

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
	{
		tcflush (fd, TCIOFLUSH);
		close (fd);
	}

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

	int ret = tcflush (*fd, TCIOFLUSH);
	if (ret == -1)
		return ratchet_error_errno (L, "ratchet.serial.close()", "tcflush");
	
	ret = close (*fd);
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
	int fd = *serial_fd (L, 1);
	luaL_Buffer buffer;
	ssize_t ret;

	lua_settop (L, 2);

	luaL_buffinit (L, &buffer);
	char *prepped = luaL_prepbuffer (&buffer);

	size_t len = (size_t) luaL_optunsigned (L, 2, (lua_Unsigned) LUAL_BUFFERSIZE);
	if (len > LUAL_BUFFERSIZE)
		return luaL_error (L, "Cannot read more than %u bytes, %u requested", (unsigned) LUAL_BUFFERSIZE, (unsigned) len);

	ret = read (fd, prepped, len);
	if (ret == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			lua_pushlightuserdata (L, RATCHET_YIELD_READ);
			lua_pushvalue (L, 1);
			return lua_yieldk (L, 2, 1, rserial_read);
		}
		else
			return ratchet_error_errno (L, "ratchet.serial.read()", "read");
	}

	luaL_addsize (&buffer, (size_t) ret);
	luaL_pushresult (&buffer);

	return 1;
}
/* }}} */

/* {{{ rserial_write() */
static int rserial_write (lua_State *L)
{
	int fd = *serial_fd (L, 1);
	size_t data_len, remaining;
	const char *data = luaL_checklstring (L, 2, &data_len);
	ssize_t ret;

	lua_settop (L, 2);

	ret = write (fd, data, data_len);

	if (ret == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			lua_pushlightuserdata (L, RATCHET_YIELD_WRITE);
			lua_pushvalue (L, 1);
			return lua_yieldk (L, 2, 1, rserial_write);
		}
		else
			return ratchet_error_errno (L, "ratchet.serial.write()", "write");
	}

	if ((size_t) ret < data_len)
	{
		remaining = data_len - (size_t) ret;
		lua_pushlstring (L, data+ret, remaining);
		return 1;
	}
	else
		return 0;
}
/* }}} */

// @TODO: proper error handling
/* {{{ rserial_set_speed() */
static int rserial_set_speed (lua_State *L)
{
	lua_settop(L, 2);
	int fd = *serial_fd (L, 1);
	int baudrate = luaL_checkint(L, 2);
	struct baudentry* entry = baudtable_lookup(baudrate);
	if(entry == NULL)
	{
		return luaL_error (L, "Invalid requested baudrate %d", baudrate);
	}
	struct termios tio;
	tcgetattr(fd, &tio);
	cfsetispeed(&tio, entry->symbol);
	cfsetospeed(&tio, entry->symbol);
	tcsetattr(fd, TCSANOW, &tio);

	return 0;
}
/* }}} */

/* ---- Public Functions ---------------------------------------------------- */

/* {{{ luaopen_ratchet_serial() */
int luaopen_ratchet_serial (lua_State *L)
{
	/* Static functions in the ratchet.serial namespace. */
	const luaL_Reg funcs[] = {
		{"new", rserial_new},
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
		{"set_speed", rserial_set_speed},
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
