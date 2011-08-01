/*
 * "builtin" memcpy, memset
 */
typedef void *	_cdecl XMEMCPY		(void *dst, const void *src, unsigned long nbytes);
typedef void *	_cdecl XMEMSET		(void *dst, int ucharfill, unsigned long size);
extern XMEMCPY *xmemcpy;
extern XMEMSET *xmemset;

