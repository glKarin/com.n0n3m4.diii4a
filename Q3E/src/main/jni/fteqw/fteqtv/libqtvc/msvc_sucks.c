/*
Copyright (C) 2007 Mark Olsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (LICENSE) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#if _MSC_VER >= 1300
	#define vsnprintf q_vsnprintf /*msvc doesn't null terminate. its insecute and thus useless*/
#endif

static int vsnprintf_calcsize(const char *fmt, va_list va)
{
	void *mem;
	unsigned int memsize;
	int ret;

	memsize = 1024;

	do
	{
		mem = malloc(memsize);

		ret = _vsnprintf(mem, memsize-1, fmt,va);
		if (ret == -1)
			memsize*= 2;

		free(mem);
	} while(ret == -1 && memsize);

	return ret;
}

int vsnprintf(char *buf, size_t buflen, const char *fmt, va_list va)
{
	int ret;

	if (buflen == 0)
		return vsnprintf_calcsize(fmt, va);
	
	ret = _vsnprintf(buf, buflen-1, fmt, va);
	buf[buflen-1] = 0;

	if (ret == -1)
		return vsnprintf_calcsize(fmt, va);
	
	return ret;
}

int snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
	int ret;
	va_list va;

	va_start(va, fmt);
	ret = vsnprintf(buf, buflen, fmt, va);
	va_end(va);

	return ret;
}

#endif

