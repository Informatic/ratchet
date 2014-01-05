#ifndef __RATCHET_BAUDTABLE_H
#define __RATCHET_BAUDTABLE_H

#include <termios.h>

struct baudentry {
	int speed, symbol;
};

static struct baudentry baudtable[] = {
#ifdef B230400
	{ 230400, B230400 },
#endif
#ifdef B115200
	{ 115200, B115200 },
#endif
#ifdef B57600
	{ 57600, B57600 },
#endif
#ifdef B38400
	{ 38400, B38400 },
#endif
#ifdef B19200
	{ 19200, B19200 },
#endif
#ifdef B9600
	{ 9600, B9600 },
#endif
#ifdef B4800
	{ 4800, B4800 },
#endif
#ifdef B2400
	{ 2400, B2400 },
#endif
#ifdef B1800
	{ 1800, B1800 },
#endif
#ifdef B1200
	{ 1200, B1200 },
#endif
#ifdef B600
	{ 600, B600 },
#endif
#ifdef B300
	{ 300, B300 },
#endif
#ifdef B200
	{ 200, B200 },
#endif
#ifdef B150
	{ 150, B150 },
#endif
#ifdef B134
	{ 134, B134 },
#endif
#ifdef B110
	{ 110, B110 },
#endif
#ifdef B75
	{ 75, B75 },
#endif
#ifdef B50
	{ 50, B50 },
#endif
#ifdef B0
	{ 0, B0 },
#endif
	{ -1, -1 }
};

#endif
