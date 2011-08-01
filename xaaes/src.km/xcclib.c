/*
 * "builtin" memcpy, memset
 */

#include "mint/kernel.h"
#include "libkern/libkern.h"
#include "xcclib.h"
#undef memcpy
#undef memset

XMEMCPY *xmemcpy;
XMEMSET *xmemset;
void *	_cdecl memcpy		(void *dst, const void *src, unsigned long nbytes);
void *	_cdecl memset		(void *dst, int ucharfill, unsigned long size);

void *	_cdecl memcpy		(void *dst, const void *src, unsigned long nbytes)
{
	return xmemcpy( dst, src, nbytes);
}
void *	_cdecl memset		(void *dst, int ucharfill, unsigned long size)
{
	return xmemset( dst, ucharfill, size);
}

