/*
 * "builtin" memcpy, memset
 *
 * for gcc < 4 use .s-output from gcc4
 *
 */
#if __GNUC__ >= 4
#include "mint/kernel.h"
#include "libkern/libkern.h"
#include "xcclib.h"
#undef memcpy
#undef memset

XMEMCPY *xmemcpy;	// set in init.c
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

#else
	__asm__  (
"	.text\n\t"
"	.even\n\t"
"	.globl	_memcpy\n\t"
"_memcpy:\n\t"
"	move.l _xmemcpy,%a0\n\t"
"	jmp (%a0)\n\t"
"	.even\n\t"
"	.globl	_memset\n\t"
"_memset:\n\t"
"	move.l _xmemset,%a0\n\t"
"	jmp (%a0)\n\t"
".comm _xmemcpy,4\n\t"
".comm _xmemset,4\n\t"
);
#endif
