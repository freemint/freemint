/*
 * sample format converters for /dev/audio
 *
 * 11/03/95, Kay Roemer.
 *
 * 9705, John Blakeley - added u16copy
 */

# include "global.h"

# include "afmts.h"


static const char ulaw_s8[] = {
	-128, -128, -128, -128, -128, -128, -128, -128, 
	-128, -128, -128, -128, -128, -128, -128, -128, 
	-128, -128, -128, -128, -128, -128, -128, -128, 
	-128, -128, -128, -128, -128, -128, -128, -126, 
	-123, -119, -115, -111, -107, -103,  -99,  -95, 
	 -91,  -87,  -83,  -79,  -75,  -71,  -67,  -63, 
	 -60,  -58,  -56,  -54,  -52,  -50,  -48,  -46, 
	 -44,  -42,  -40,  -38,  -36,  -34,  -32,  -30, 
	 -28,  -27,  -26,  -25,  -24,  -23,  -22,  -21, 
	 -20,  -19,  -18,  -17,  -16,  -15,  -14,  -13, 
	 -13,  -12,  -12,  -11,  -11,  -10,  -10,   -9, 
	  -9,   -8,   -8,   -7,   -7,   -6,   -6,   -5, 
	  -5,   -5,   -4,   -4,   -4,   -4,   -3,   -3, 
	  -3,   -3,   -2,   -2,   -2,   -2,   -1,   -1, 
	  -1,   -1,   -1,   -1,    0,    0,    0,    0, 
	   0,    0,    0,    0,    0,    0,    0,    0, 
	 127,  127,  127,  127,  127,  127,  127,  127, 
	 127,  127,  127,  127,  127,  127,  127,  127, 
	 127,  127,  127,  127,  127,  127,  127,  127, 
	 127,  127,  127,  127,  127,  127,  127,  127, 
	 124,  120,  116,  112,  108,  104,  100,   96, 
	  92,   88,   84,   80,   76,   72,   68,   64, 
	  61,   59,   57,   55,   53,   51,   49,   47, 
	  45,   43,   41,   39,   37,   35,   33,   31, 
	  29,   28,   27,   26,   25,   24,   23,   22, 
	  21,   20,   19,   18,   17,   16,   15,   14, 
	  14,   13,   13,   12,   12,   11,   11,   10, 
	  10,    9,    9,    8,    8,    7,    7,    6, 
	   6,    6,    5,    5,    5,    5,    4,    4, 
	   4,    4,    3,    3,    3,    3,    2,    2, 
	   2,    2,    2,    2,    1,    1,    1,    1, 
	   1,    1,    1,    1,    0,    0,    0,    0, 
};

void *
u8copy (void *dst, const void *src, long len)
{
	uchar *_dst = dst;
	const uchar *_src = src;

	if ((((long)dst ^ (long)src) & 1) == 0) {
		/*
		 * both pointers are either aligned or unaligned,
		 * so we can use long copy's
		 */
		if ((long)dst & 1) {
			*_dst++ = *_src++ ^ 0x80;
			--len;
		}
		for ( ; len >= 4; len -= 4) {
			*((ulong *)_dst)++ =
				*((const ulong *)_src)++ ^ 0x80808080L;
		}
	}
	while (--len >= 0)
		*_dst++ = *_src++ ^ 0x80;
	return dst;
}

void *
ulawcopy (void *dst, const void *src, long len)
{
	char *_dst = dst;
	const uchar *_src = src;

	for ( ; len & 7; --len)
		*_dst++ = ulaw_s8[*_src++];
	while ((len -= 8) >= 0) {
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
		*_dst++ = ulaw_s8[*_src++];
	}
	return dst;
}

void *
u16copy (void *dst, const void *src, long len)
{
	ushort *_dst = dst;
	const ushort *_src = src;

	if ((((long)dst ^ (long)src) & 1) == 0) {
		/*
		 * both pointers are either aligned or unaligned,
		 * so we can use long copy's
		 */
		if ((long)dst & 1) {
			*_dst++ = *_src++ ^ 0x8000;
			len -= 2;
		}
		for ( ; len >= 4; len -= 4) {
			*((ulong *)_dst)++ =
				*((const ulong *)_src)++ ^ 0x80008000L;
		}
	}
	while ((len -= 2) >= 0)
		*_dst++ = *_src++ ^ 0x8000;
	return dst;
}

void *
mu16copy (void *dst, const void *src, long len)
{
	ushort *_dst = dst;
	const ushort *_src = src;

	if ((((long)dst ^ (long)src) & 1) == 0) {
		if ((long)dst & 1) {
			*_dst++ = *_src ^ 0x8000;
			*_dst++ = *_src++ ^ 0x8000;
			len -= 4;
		}
		for ( ; len >= 4; len -= 4) {
			*_dst++ = *_src ^ 0x8000;
			*_dst++ = *_src++ ^ 0x8000;
		}
	}
	while ((len -= 4) >= 0) {
		*_dst++ = *_src ^ 0x8000;
		*_dst++ = *_src++ ^ 0x8000;
	}
	return dst;
}

void *
m16copy (void *dst, const void *src, long len)
{
	int *_dst = dst;
	const short *_src = src;

	if ((((long)dst ^ (long)src) & 1) == 0) {
		if ((long)dst & 1) {
			*_dst++ = *_src;
			*_dst++ = *_src++;
			len -= 4;
		}
		for ( ; len >= 4; len -= 4) {
			*_dst++ = *_src;
			*_dst++ = *_src++;
		}
	}
	while ((len -= 4) >= 0) {
		*_dst++ = *_src;
		*_dst++ = *_src++;
	}
	return dst;
}

