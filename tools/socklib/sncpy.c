/*
 *	Almost like strncpy() except that it returns the length
 *	of the copied string, terminates dst always with exectly
 *	one '\0'.
 */

#include "stsocket.h"

int _sncpy(char *dst, const char *src, size_t len)
{
	int count = (int)len;

	if (count <= 0)
		return 0;
	while (--count >= 0 && (*dst++ = *src++) != 0)
		;
	if (count < 0)
	{
		dst[-1] = '\0';
		return (int)(len - 1);
	}
	return (int)(len - count - 1);
}
