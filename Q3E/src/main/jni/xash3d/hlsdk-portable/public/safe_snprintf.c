#include "safe_snprintf.h"
#if XASH_WIN32
#include <stdarg.h>
#include <stdio.h>
// Microsoft's _snprintf works doesn't like C99's snprintf
// and behaviour of this function is similar to strncpy.
// For more information read _snprintf article on MSDN.
int safe_snprintf(char *buffer, int buffersize, const char *format, ...)
{
	va_list	args;
	int	result;

	if( buffersize <= 0 )
		return -1;

	va_start( args, format );
	result = _vsnprintf( buffer, buffersize, format, args );
	va_end( args );

	if( result >= buffersize )
	{
		buffer[buffersize - 1] = '\0';
		return -1;
	}

	return result;
}
#endif // XASH_WIN32
