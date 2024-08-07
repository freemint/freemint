/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This code is from Mario Becroft.  Thanks a lot Mario!
 *
 * modifications:
 * - use C.vh
 * - call kmalloc for temps
 * - kept control over allocated permanent areas outside
 * - cast void *fd_addr to char *
 * - just return true or false
 * - source level optimizations for bit handling (partly faster, partly smaller)
 */

#include "xa_types.h"
#include "xa_global.h"

/* define this 1 if vs_color works */
/* vs_color in libgem broken? */
#define  HAVE_VS_COLOR	1

#include "trnfm.h"
#include "util.h"
#if INCLUDE_UNUSED
static short systempalette[] =
{
0x03e8, 0x03e8, 0x03e8, 0x0000, 0x0000, 0x0000, 0x03e8, 0x0000,
0x0000, 0x0000, 0x03e8, 0x0000, 0x0000, 0x0000, 0x03e8, 0x0066,
0x031c, 0x0301, 0x03e8, 0x03e8, 0x0000, 0x0386, 0x00c8, 0x0258,
0x0310, 0x0310, 0x0310, 0x01ee, 0x01ee, 0x01ee, 0x01f6, 0x0000,
0x0000, 0x0000, 0x01f6, 0x0000, 0x0000, 0x0000, 0x01f6, 0x0000,
0x01f6, 0x01f6, 0x02ca, 0x027b, 0x00e0, 0x01f6, 0x0000, 0x01f6,
0x0000, 0x0000, 0x00c8, 0x0000, 0x0000, 0x0190, 0x0000, 0x0000,
0x0258, 0x0000, 0x0000, 0x0320, 0x0000, 0x0000, 0x03e8, 0x0000,
0x00c8, 0x0000, 0x0000, 0x00c8, 0x00c8, 0x0000, 0x00c8, 0x0190,
0x0000, 0x00c8, 0x0258, 0x0000, 0x00c8, 0x0320, 0x0000, 0x00c8,
0x03e8, 0x0000, 0x0190, 0x0000, 0x0000, 0x0190, 0x00c8, 0x0000,
0x0190, 0x0190, 0x0000, 0x0190, 0x0258, 0x0000, 0x0190, 0x0320,
0x0000, 0x0190, 0x03e8, 0x0000, 0x0258, 0x0000, 0x0000, 0x0258,
0x00c8, 0x0000, 0x0258, 0x0190, 0x0000, 0x0258, 0x0258, 0x0000,
0x0258, 0x0320, 0x0000, 0x0258, 0x03e8, 0x0000, 0x0320, 0x0000,
0x0000, 0x0320, 0x00c8, 0x0000, 0x0320, 0x0190, 0x0000, 0x0320,
0x0258, 0x0000, 0x0320, 0x0320, 0x0000, 0x0320, 0x03e8, 0x0000,
0x03e8, 0x0000, 0x0000, 0x03e8, 0x00c8, 0x0000, 0x03e8, 0x0190,
0x0000, 0x03e8, 0x0258, 0x0000, 0x03e8, 0x0320, 0x0000, 0x03e8,
0x03e8, 0x00c8, 0x0000, 0x0000, 0x00c8, 0x0000, 0x00c8, 0x00c8,
0x0000, 0x0190, 0x00c8, 0x0000, 0x0258, 0x00c8, 0x0000, 0x0320,
0x00c8, 0x0000, 0x03e8, 0x00c8, 0x00c8, 0x0000, 0x00c8, 0x00c8,
0x00c8, 0x00c8, 0x00c8, 0x0190, 0x0000, 0x0000, 0x02f1, 0x00c8,
0x00c8, 0x0320, 0x00c8, 0x00c8, 0x03e8, 0x00c8, 0x0190, 0x0000,
0x00c8, 0x0190, 0x00c8, 0x00c8, 0x0190, 0x0190, 0x00c8, 0x0190,
0x0258, 0x00c8, 0x0190, 0x0320, 0x00c8, 0x0190, 0x03e8, 0x00c8,
0x0258, 0x0000, 0x00c8, 0x0258, 0x00c8, 0x00c8, 0x0258, 0x0190,
0x00c8, 0x0258, 0x0258, 0x00c8, 0x0258, 0x0320, 0x00c8, 0x0258,
0x03e8, 0x00c8, 0x0320, 0x0000, 0x00c8, 0x0320, 0x00c8, 0x00c8,
0x0320, 0x0190, 0x00c8, 0x0320, 0x0258, 0x00c8, 0x0320, 0x0320,
0x00c8, 0x0320, 0x03e8, 0x00c8, 0x03e8, 0x0000, 0x00c8, 0x03e8,
0x00c8, 0x00c8, 0x03e8, 0x0190, 0x00c8, 0x03e8, 0x0258, 0x00c8,
0x03e8, 0x0320, 0x00c8, 0x03e8, 0x03e8, 0x0190, 0x0000, 0x0000,
0x0190, 0x0000, 0x00c8, 0x0190, 0x0000, 0x0190, 0x0190, 0x0000,
0x0258, 0x0190, 0x0000, 0x0320, 0x0190, 0x0000, 0x03e8, 0x0190,
0x00c8, 0x0000, 0x0190, 0x00c8, 0x00c8, 0x0190, 0x00c8, 0x0190,
0x0190, 0x00c8, 0x0258, 0x0190, 0x00c8, 0x0320, 0x0190, 0x00c8,
0x03e8, 0x0190, 0x0190, 0x0000, 0x0190, 0x0190, 0x00c8, 0x0190,
0x0190, 0x0190, 0x0190, 0x0190, 0x0258, 0x0190, 0x0190, 0x0320,
0x0190, 0x0190, 0x03e8, 0x0190, 0x0258, 0x0000, 0x0190, 0x0258,
0x00c8, 0x0190, 0x0258, 0x0190, 0x0190, 0x0258, 0x0258, 0x0190,
0x0258, 0x0320, 0x0190, 0x0258, 0x03e8, 0x0190, 0x0320, 0x0000,
0x0190, 0x0320, 0x00c8, 0x0190, 0x0320, 0x0190, 0x0190, 0x0320,
0x0258, 0x0190, 0x0320, 0x0320, 0x0190, 0x0320, 0x03e8, 0x0190,
0x03e8, 0x0000, 0x0190, 0x03e8, 0x00c8, 0x0190, 0x03e8, 0x0190,
0x0190, 0x03e8, 0x0258, 0x0190, 0x03e8, 0x0320, 0x0190, 0x03e8,
0x03e8, 0x0258, 0x0000, 0x0000, 0x0258, 0x0000, 0x00c8, 0x0258,
0x0000, 0x0190, 0x0258, 0x0000, 0x0258, 0x0258, 0x0000, 0x0320,
0x0258, 0x0000, 0x03e8, 0x0258, 0x00c8, 0x0000, 0x0258, 0x00c8,
0x00c8, 0x0258, 0x00c8, 0x0190, 0x0258, 0x00c8, 0x0258, 0x0258,
0x00c8, 0x0320, 0x0258, 0x00c8, 0x03e8, 0x0258, 0x0190, 0x0000,
0x0258, 0x0190, 0x00c8, 0x0258, 0x0190, 0x0190, 0x0258, 0x0190,
0x0258, 0x0258, 0x0190, 0x0320, 0x0258, 0x0190, 0x03e8, 0x0258,
0x0258, 0x0000, 0x0258, 0x0258, 0x00c8, 0x0258, 0x0258, 0x0190,
0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0320, 0x0258, 0x0258,
0x03e8, 0x0258, 0x0320, 0x0000, 0x0258, 0x0320, 0x00c8, 0x0258,
0x0320, 0x0190, 0x0258, 0x0320, 0x0258, 0x0258, 0x0320, 0x0320,
0x0258, 0x0320, 0x03e8, 0x0258, 0x03e8, 0x0000, 0x0258, 0x03e8,
0x00c8, 0x0258, 0x03e8, 0x0190, 0x0258, 0x03e8, 0x0258, 0x0258,
0x03e8, 0x0320, 0x0258, 0x03e8, 0x03e8, 0x0320, 0x0000, 0x0000,
0x0320, 0x0000, 0x00c8, 0x0320, 0x0000, 0x0190, 0x0320, 0x0000,
0x0258, 0x0320, 0x0000, 0x0320, 0x0320, 0x0000, 0x03e8, 0x0320,
0x00c8, 0x0000, 0x0320, 0x00c8, 0x00c8, 0x0320, 0x00c8, 0x0190,
0x0320, 0x00c8, 0x0258, 0x0320, 0x00c8, 0x0320, 0x0320, 0x00c8,
0x03e8, 0x0320, 0x0190, 0x0000, 0x0320, 0x0190, 0x00c8, 0x0320,
0x0190, 0x0190, 0x0320, 0x0190, 0x0258, 0x0320, 0x0190, 0x0320,
0x0320, 0x0190, 0x03e8, 0x0320, 0x0258, 0x0000, 0x0320, 0x0258,
0x00c8, 0x0320, 0x0258, 0x0190, 0x0320, 0x0258, 0x0258, 0x0320,
0x0258, 0x0320, 0x0320, 0x0258, 0x03e8, 0x0320, 0x0320, 0x0000,
0x0320, 0x0320, 0x00c8, 0x0320, 0x0320, 0x0190, 0x0320, 0x0320,
0x0258, 0x0320, 0x0320, 0x0320, 0x0320, 0x0320, 0x03e8, 0x0320,
0x03e8, 0x0000, 0x0320, 0x03e8, 0x00c8, 0x0320, 0x03e8, 0x0190,
0x0320, 0x03e8, 0x0258, 0x0320, 0x03e8, 0x0320, 0x0320, 0x03e8,
0x03e8, 0x03e8, 0x0000, 0x0000, 0x03e8, 0x0000, 0x00c8, 0x03e8,
0x0000, 0x0190, 0x03e8, 0x0000, 0x0258, 0x03e8, 0x0000, 0x0320,
0x03e8, 0x0000, 0x03e8, 0x03e8, 0x00c8, 0x0000, 0x03e8, 0x00c8,
0x00c8, 0x03e8, 0x00c8, 0x0190, 0x03e8, 0x00c8, 0x0258, 0x03e8,
0x00c8, 0x0320, 0x03e8, 0x00c8, 0x03e8, 0x03e8, 0x0190, 0x0000,
0x03e8, 0x0190, 0x00c8, 0x03e8, 0x0190, 0x0190, 0x03e8, 0x0190,
0x0258, 0x03e8, 0x0190, 0x0320, 0x03e8, 0x0190, 0x03e8, 0x03e8,
0x0258, 0x0000, 0x03e8, 0x0258, 0x00c8, 0x03e8, 0x0258, 0x0190,
0x03e8, 0x0258, 0x0258, 0x03e8, 0x0258, 0x0320, 0x03e8, 0x0258,
0x03e8, 0x03e8, 0x0320, 0x0000, 0x03e8, 0x0320, 0x00c8, 0x03e8,
0x0320, 0x0190, 0x03e8, 0x0320, 0x0258, 0x03e8, 0x0320, 0x0320,
0x03e8, 0x0320, 0x03e8, 0x03e8, 0x03e8, 0x0000, 0x03e8, 0x03e8,
0x00c8, 0x03e8, 0x03e8, 0x0190, 0x03e8, 0x03e8, 0x0258, 0x03e8,
0x03e8, 0x0320, 0x03b5, 0x0000, 0x0000, 0x0386, 0x0000, 0x0000,
0x02f1, 0x0000, 0x0000, 0x02be, 0x0000, 0x0000, 0x01f6, 0x0000,
0x0000, 0x012e, 0x0000, 0x0000, 0x0066, 0x0000, 0x0000, 0x0000,
0x03b5, 0x0000, 0x0000, 0x0386, 0x0000, 0x0000, 0x02f1, 0x0000,
0x0000, 0x02be, 0x0000, 0x0000, 0x01f6, 0x0000, 0x0000, 0x012e,
0x0000, 0x0000, 0x0066, 0x0000, 0x0000, 0x0000, 0x0066, 0x0000,
0x0000, 0x012e, 0x0000, 0x0000, 0x01f6, 0x0000, 0x0000, 0x02be,
0x00c8, 0x00c8, 0x0258, 0x0000, 0x0000, 0x0386, 0x03b5, 0x03b5,
0x03b5, 0x0386, 0x0386, 0x0386, 0x02f1, 0x02f1, 0x02f1, 0x02be,
0x02be, 0x02be, 0x012e, 0x012e, 0x012e, 0x0066, 0x0066, 0x0066,
};
#endif

static const short devtovdi8[256] =
{
#if 0
0, 2, 3, 6, 4, 7, 5, 8, 9, 10,11,14,12,15,13,255,
#endif
#if 0
0  1  2  3  4  5  6  7  8	 9   10 11 12 13 14 15
#endif
  0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11,14,12,15,13,255,
 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
 48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
 80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
 113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,
 141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,
 169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,
 197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,
 225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,1
};
#if 0
#define G_WHITE			0
#define G_BLACK			1
#define G_RED			2
#define G_GREEN			3
#define G_BLUE			4
#define G_CYAN			5
#define G_YELLOW		6
#define G_MAGENTA		7
#define G_LWHITE		8
#define G_LBLACK		9
#define G_LRED			10
#define G_LGREEN		11
#define G_LBLUE			12
#define G_LCYAN			13
#define G_LYELLOW		14
#define G_LMAGENTA		15
#endif
/*                                                                         1   1   1   1   1   1 */
/*                                 0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5 */
static const short devtovdi4[] = { 0,  2,  3,  6,  4,  7,  5,  8,  9, 10, 11, 14, 12, 15, 13,  1 };

/*                                                                         1   1   1   1   1   1 */
/*                                 0   1   2   3   4   5   6   7   8   9   0   1   2   3   4   5 */
static const short vditodev4[] = { 0,255,  1,  2,  4,  6,  3,  5,  7,  8,  9, 10, 12, 14, 11, 13 };

#if 0
static const short vditodev4[] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  8,  9, 12, 14, 11, 13 };
#endif

static const short devtovdi2[] = { 0, 2, 3, 1 };

static const short devtovdi1[] = { 0, 1 };

static const short *tovdilut[] = { 0, devtovdi1, devtovdi2, 0, devtovdi4, 0, 0, 0, devtovdi8 };

static unsigned long
get_coldist(struct rgb_1000 *src, struct rgb_1000 *dst)
{
	long r, g, b;
	int src_grey = 0, dst_grey = 0;

	r = dst->red;
	r -= src->red;
	r *= r;

	if( src->red == src->green && src->green == src->blue )
		src_grey = 1;
	if( dst->red == dst->green && dst->green == dst->blue )
		dst_grey = 1;
	if( src_grey && dst_grey )	/* prefer if both grey */
	{
		return r / 8;
	}
	if( src_grey != dst_grey )
		r *= 4;	/* bad if only one grey */

	g = dst->green;
	g -= src->green;
	g *= g;

	b = dst->blue;
	b -= src->blue;
	b *= b;

	r += (g + b);

	return r;
}

static char
fgetc(struct file *fp, long *count)
{
	long r;
	char ret;

	r = kernel_read(fp, &ret, 1);
	if (r == 1)
		*count += 1;

	return (volatile char)ret;
}

#if 0
static unsigned short
fgetw(struct file *fp)
{
	unsigned short ret;
	kernel_read(fp, &ret, 2);
	return (volatile unsigned short)ret;
}
#endif
#ifndef ST_ONLY
/* pixelformat E07F */
static void *
toI15b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned short *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 5;		/* 32 */
	r += 500;
	r /= 1000;
	if (r > 31)
		r = 31;

	g = pal->green;
	g <<= 5;
	g += 500;
	g /= 1000;
	if (g > 31)
		g = 31;

	b = pal->blue;
	b <<= 5;
	b += 500;
	b /= 1000;
	if (b > 31)
		b = 31;

/* gggbbbbb.0rrrrrgg */
	r <<= 2;
	r |= (g >> 3);
	r |= (g << 13);
	r |= (b << 8);

	*img++ = r;
	return img;
}
static void *
toF16b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned short *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 5;		/* 32 */
	r += 500;
	r /= 1000;
	if (r > 31)
		r = 31;

	g = pal->green;
	g <<= 5;
	g += 500;
	g /= 1000;
	if (g > 31)
		g = 31;

	b = pal->blue;
	b <<= 5;
	b += 500;
	b /= 1000;
	if (b > 31)
		b = 31;

/* rrrrrggg.ggobbbbb */
	r <<= 5;
	r |= g;
	r <<= 6;
	r |= b;

	*img++ = r;
	return img;
}

static void *
toM16b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned short *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 5;
	r += 500;
	r /= 1000;
	if (r > 31)
		r = 31;

	g = pal->green;
	g <<= 6;
	g += 500;
	g /= 1000;
	if (g > 63)
		g = 63;

	b = pal->blue;
	b <<= 5;
	b += 500;
	b /= 1000;
	if (b > 31)
		b = 31;

/* rrrrrggg.gggbbbbb */
	r <<= 3;
	r |= (g >> 3);
	r <<= 8;

	r |= ((g & 7) << 5);
	r |= b;

	*img++ = r;
	return img;

}
static void *
toI16b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned short *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 5;
	r += 500;
	r /= 1000;
	if (r > 31)
		r = 31;

	g = pal->green;
	g <<= 6;
	g += 500;
	g /= 1000;
	if (g > 63)
		g = 63;

	b = pal->blue;
	b <<= 5;
	b += 500;
	b /= 1000;
	if (b > 31)
		b = 31;

/* gggbbbbb.rrrrrggg */

	r <<= 3;
	r |= (g >> 3);

	r |= (((g & 7) << 5) | b) << 8;

	*img++ = r;
	return img;
};

static void *
toI24b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned char *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 8;
	r += 500;
	r /= 1000;
	if (r > 255)
		r = 255;

	g = pal->green;
	g <<= 8;
	g += 500;
	g /= 1000;
	if (g > 255)
		g = 255;

	b = pal->blue;
	b <<= 8;
	b += 500;
	b /= 1000;
	if (b > 255)
		b = 255;

/* bbbbbbbb.gggggggg.rrrrrrrr */
	*img++ = b;
	*img++ = g;
	*img++ = r;
	return img;
};
static void *
toM24b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned char *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 8;
	r += 500;
	r /= 1000;
	if (r > 255)
		r = 255;

	g = pal->green;
	g <<= 8;
	g += 500;
	g /= 1000;
	if (g > 255)
		g = 255;

	b = pal->blue;
	b <<= 8;
	b += 500;
	b /= 1000;
	if (b > 255)
		b = 255;

/* bbbbbbbb.gggggggg.rrrrrrrr */
	*img++ = r;
	*img++ = g;
	*img++ = b;
	return img;
};

static void *
toI32b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned long *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 8;
	r += 500;
	r /= 1000;
	if (r > 255)
		r = 255;

	g = pal->green;
	g <<= 8;
	g += 500;
	g /= 1000;
	if (g > 255)
		g = 255;

	b = pal->blue;
	b <<= 8;
	b += 500;
	b /= 1000;
	if (b > 255)
		b = 255;

/* rrrrrrrr.gggggggg.bbbbbbbb.00000000 */
	r <<= 8;
	r |= g;
	r <<= 8;
	r |= b;
	r <<= 8;

	*img++ = r;
	return img;
};

static void *
toM32b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned long *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 8;
	r += 500;
	r /= 1000;
	if (r > 255)
		r = 255;

	g = pal->green;
	g <<= 8;
	g += 500;
	g /= 1000;
	if (g > 255)
		g = 255;

	b = pal->blue;
	b <<= 8;
	b += 500;
	b /= 1000;
	if (b > 255)
		b = 255;

/* 00000000.rrrrrrrr.gggggggg.bbbbbbbb */
	b |= (g << 8);
	b |= (r << 16);

	*img++ = b;
	return img;
};

static void *
toIbs32b(struct rgb_1000 *pal, void *img_ptr)
{
	unsigned long *img = img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 8;
	r += 500;
	r /= 1000;
	if (r > 255)
		r = 255;

	g = pal->green;
	g <<= 8;
	g += 500;
	g /= 1000;
	if (g > 255)
		g = 255;

	b = pal->blue;
	b <<= 8;
	b += 500;
	b /= 1000;
	if (b > 255)
		b = 255;

/* 00000000.bbbbbbbb.gggggggg.rrrrrrrr */
	r |= (g << 8);
	r |= (b << 16);

	*img++ = r;
	return img;
};
#endif
/*
 * build_pal_xref will build a table of cross-reference palette indexes.
 * This means that for each entry in the source palette, cref contains the
 * index into the destination palette where the color closest to the one
 * in the src palette was found.
 */
static void
build_pal_xref(struct rgb_1000 *src_palette, struct rgb_1000 *dst_palette, unsigned char *cref, int pens)
{
	struct rgb_1000 *dst, *src;
	unsigned long closest, this;
	int i, j, c;
	short tpens = 256;

	if( screen.planes < 8 )
		tpens = 1 << screen.planes;

	if (pens > 256)
		pens = 256;

	cref[0] = 0;
	for (i = 1, src = src_palette; i < pens; i++)
	{
		dst = dst_palette + 1;
		closest = 0xffffffffL;

		c = 0;
		for (j = 1; j < tpens; j++, dst++)
		{
			if (!(this = get_coldist(src+i, dst)))
			{
				closest = this;
				c = j;
				break;
			}
			else if (this < closest)
			{
				closest = this;
				c = j;
			}
		}
		if (c < 16 && screen.planes == 8)
		{
			/* We need to convert from vdi to dev here so vr_trnfm() converts
			 * back to what we already know here...
			 */
			c = vditodev4[c];
		}

		cref[i] = c;

	}
}
/*
 * Remap the bitmap palette referances.
 * md == 1: bitmap, else img
 */
static void
remap_bitmap_colindexes(MFDB *map, unsigned char *cref, int md)
{
	int planes, psize;
	int i, j, k, bit;
	unsigned short *data, *d_ptr;
	unsigned short d[8];
	unsigned short cref_ind, mask;
	short const *lut = 0;

	planes = map->fd_nplanes;
	psize = map->fd_wdwidth * map->fd_h;
	data = map->fd_addr;

	DIAGS(("remap_bitmap_colindex: planes = %d, psize = %d, data = %lx", planes, psize, (unsigned long)data));

	if( md && planes >= 4 )
		lut = tovdilut[planes];

	for (k = 0; k < psize; k++, data++)
	{
		for (i = 0; i < 8; i++)
			d[i] = 0;

		mask = 0x8000;

		for (bit = 15, i = 0; i < 16; i++, bit--)
		{
			cref_ind = 0;
			d_ptr = data;
			for (j = 0; j < planes; j++)
			{
				cref_ind >>= 1;
				cref_ind |= *d_ptr & mask;
				*d_ptr <<= 1;
				d_ptr += psize;
			}
			cref_ind >>= (16 - planes);

			if( cref_ind >= md )
			{
				if( lut )
					cref_ind = lut[cref_ind];
				cref_ind = cref[cref_ind];
			}

			for (j = 0; j < planes; j++)
			{
				d[j] |= (cref_ind & 1) << bit;
				cref_ind >>= 1;
			}

		}
		d_ptr = data;
		for (i = 0; i < planes; i++)
		{
			*d_ptr = d[i];
			d_ptr += psize;
		}
	}
}

/* Vertical replication codes are only allowed in certain places */
static int
read_gem_line( struct file *fp, unsigned char *line, unsigned long scan, int patlen, unsigned long *vrep, bool allow_vrep, bool planes, long *rcnt)
{
	unsigned char *l = line, *end = line + scan + 1;
	unsigned char *endline = line + scan;

	do
	{
		short b1 = fgetc(fp, rcnt);

		b1 &= 0xff;

		if ( b1 == 0x80 )
		/* Literal run */
		{
			short len = fgetc(fp, rcnt) & 0x00ff;

			if (len > 0)
			{
				scan -= len;
				for (; len > 0; len--)
				{
					if (line > end || line < l)
					{
						return 0;
					}
					*line++ = (unsigned char)fgetc(fp, rcnt);
				}
			}
		}
		else if ( b1 == 0x00 )
		/* Pattern code */
		{
			short rep = fgetc(fp, rcnt) & 0x00ff;
			if ( rep == 0 && allow_vrep )
			/* SCANREPEAT Is actually a vertical replication */
			{
				if ((unsigned char)fgetc(fp, rcnt) != 0xff)
				{
					return 0;
				}
				*vrep = (unsigned long)((unsigned char)fgetc(fp, rcnt) - 1);
			}
			else if (rep)/* PATTERN REPEAT */
			{
				int i;

				scan -= patlen * rep;
				for ( i = 0; i < patlen; i++ )
				{
					if (line > end || line < l)
					{
						return 0;
					}
					*line++ = (unsigned char)fgetc(fp, rcnt);
				}
				while ( rep > 1 )
				{
					for ( i = 0; i < patlen; i++, line++ )
					{
						*line = line[-patlen];
						if (line > end || line < l)
						{
							return 0;
						}
					}
					rep--;
				}
			}
		}
		else
		/* Is a black/white (=0xff's/0x00's) run code */
		{
			unsigned char store = (unsigned char)((signed char)b1 >> 7 );
			short am;

			am = b1 & 0x7f;
			if (line + am > end || line < l)
			{
				return 0;
			}
			scan -= am;
			while (am)
				*line++ = store, am--;
		}
		allow_vrep = false;
	}
	while (line < endline);

	if (line != endline)
	{
		/* display("ERROR4: past buffer!"); */
	}

	return 1;
}

static long
gem_rdata(struct file *fp, XA_XIMG_HEAD *pic, bool disp, long *rcnt)
{
	unsigned long scan = (pic->ximg.img_w + 7) / 8;
	unsigned long wscan = ((pic->ximg.img_w + 15) >> 4) << 1;
	long ret = 0, stride = ((pic->ximg.img_w * pic->ximg.planes + 31) / 32) * 4;
	unsigned char *data, *s, *e;

	data = pic->addr;

	s = data;
	UNUSED(s);
	e = data + (pic->ximg.img_h * stride);
	UNUSED(e);

	switch (pic->ximg.planes)
	{
		case 1:
		{
			int y;
			unsigned long vrep = 0L;
			unsigned char *dst;

			for (y = 0; y < pic->ximg.img_h; y++)
			{
				dst = data + (y * wscan);
				if (vrep)
				{
					memcpy(dst - wscan, dst, wscan);
					vrep--;
				}
				else
				{
					if (!(read_gem_line(fp, dst, scan, pic->ximg.pat_len, &vrep, true, true, rcnt)))
					{
						ret = -1;
						break;
					}
				}
			}
			break;
		}
		case 2 ... 8:
		{
			int i, y;
			unsigned long vrep = 0L;
			unsigned char *dst = data;
			long readcount;

			for (y = 0; ret == 0 && y < pic->ximg.img_h; y += (vrep + 1))
			{
				vrep = 0;
				for (i = 0; i < pic->ximg.planes; i++)
				{
					dst = data + (long)((long)y + ((long)i * pic->ximg.img_h)) * wscan;

					readcount = 0;
					if (!(read_gem_line(fp, dst, scan, pic->ximg.pat_len, &vrep, true, true, &readcount)))
					{
						*rcnt += readcount;
						ret = -1;
						break;
					}
					*rcnt += readcount;
					if (vrep)
					{
						long a;
						if (vrep + y > pic->ximg.img_h)
							vrep = pic->ximg.img_h - y;
						for (a = 0; a < vrep; a++)
						{
							memcpy(dst + wscan, dst, wscan);
							dst += wscan;
						}
					}
				}
			}
			break;
		}
#ifndef ST_ONLY
		case 24:
		/* 24bpp data is strange in that it isn't bit planed at all like
		   the others are. This makes decoding quicker, and most 24bpp
		   hardware is not bit planar, and this may explain why.
		   Of course, my guesswork says that the order is R,G,B and as
		   GBM used B,G,R, i'll have to reverse the order. */
		{
			long y, sl, rsl;
			unsigned long vrep = 0L;
			unsigned char *dst;
			sl = (long)((pic->ximg.img_w + 15) & ~15) * 3;
			rsl = (long)((pic->ximg.img_w + 7) & ~7) * 3;

			for (y = 0, dst = data; y < pic->ximg.img_h; y++, dst += sl)
			{
				if (vrep)
				{
					memcpy(dst - sl, dst, sl);
					vrep--;
				}
				else
				{
					if (!(read_gem_line(fp, dst, rsl, pic->ximg.pat_len, &vrep, true, false, rcnt)))
					{
						ret = -1;
						break;
					}
				}
			}
			break;
		}
#endif
		default:
		{
			ret = -1;
			break;
		}
	}

	return ret;
}

/* Loads & depacks IMG (0 if succeded, else error). */
/* Bitplanes are one after another in address IMG_HEADER.addr. */
static void
depack_img(char *name, XA_XIMG_HEAD *pic)
{
	int /*width, */word_aligned, pal_size;
	long size, err;
	struct file *fp = NULL;
	bool disp = false;
	long rcnt;

	pic->addr = NULL;
	pic->palette = NULL;
#if 0
	{
		int slen = strlen(name);
		char *fname = name + slen;

		while (*fname != '/' && *fname != '\\')
			fname--;
		fname++;
		display(" --?? %s", fname);
		if (!strcmp("wtitle.img", fname))
		{
			display("Yeahhah!");
			disp = true;
		}
	}
#endif
	fp = kernel_open(name, O_RDONLY, &err, NULL);

	if (fp)
	{
		/* read header info (bw & ximg) into image structure */
		if ((kernel_read(fp, (char *)&pic->ximg.version, sizeof(XIMG_header)) != sizeof(XIMG_header)))
		{
			goto end_depack;
		}
#ifndef ST_ONLY
		if (pic->ximg.length > 7 && pic->ximg.planes >= 1 && pic->ximg.planes <= 32 && pic->ximg.img_w > 0 && pic->ximg.img_h > 0)
#else
		if (pic->ximg.length > 7 && pic->ximg.planes >= 1 && pic->ximg.planes <= 8 && pic->ximg.img_w > 0 && pic->ximg.img_h > 0)
#endif
		{
			word_aligned = (pic->ximg.img_w + 15) >> 4;
			word_aligned <<= 1;

			/* width byte aligned */
			size = (long)((long)word_aligned * pic->ximg.img_h * pic->ximg.planes);

			/* if XIMG, read info */
			if (pic->ximg.planes <= 8 && pic->ximg.length >= ((sizeof(XIMG_header) >> 1) + (3 * (1 << (pic->ximg.planes)))) &&
			    pic->ximg.magic == XIMG && pic->ximg.paltype == 0)
			{
				pal_size = (1 << pic->ximg.planes) * 3 * 2;
				if ((pic->palette = kmalloc(pal_size)))
				{
					if (!(pal_size == kernel_read(fp, pic->palette, pal_size)))
					{
						kfree(pic->palette);
						pic->palette = NULL;
					}
				}
			}

			if (!(pic->addr = kmalloc(size)))
				goto end_depack;

			rcnt = 2L * pic->ximg.length;
			kernel_lseek(fp, rcnt, SEEK_SET);
#if 0
			if (disp)
			{
				struct ximg_header *ximg = &pic->ximg;
				display("version %d\r\n hlen    %d\r\n planes  %d\r\n pat_len %d\r\n pix_w   %d\r\n pix_h   %d\r\n img_w   %d\r\n img_h   %d\r\n magic   %lx\r\n paltype %d",
					ximg->version, ximg->length, ximg->planes, ximg->pat_len, ximg->pix_w, ximg->pix_h, ximg->img_w, ximg->img_h,
					ximg->magic, ximg->paltype);
			}
#endif
			if (gem_rdata(fp, pic, disp, &rcnt) == -1L)
			{
				goto end_depack;
			}
		}
	}
	else
	{
end_depack:
		if (pic->palette)
		{
			kfree(pic->palette);
			pic->palette = NULL;
		}
		if (pic->addr)
		{
			kfree(pic->addr);
			pic->addr = NULL;
		}
	}

	if (fp)
		kernel_close(fp);
}

struct TrNfo
{
	short	shift;
	short	half;
	short	full;
};
#ifndef ST_ONLY
static struct TrNfo from1_8[] =
{
	{0,0,0},
	{8 + 7,   0,   1},	/* 1 */
	{8 + 6,   1,   3},	/* 2 */
	{8 + 5,   3,   7},	/* 3 */
	{8 + 4,   7,  15},	/* 4 */
	{8 + 3,  15,  31},	/* 5 */
	{8 + 2,  31,  63},	/* 6 */
	{8 + 1,  63, 127},	/* 7 */
	{8 + 0, 127, 255},	/* 8 */
};

static void
from8b(void *(*to)(struct rgb_1000 *, void *), struct rgb_1000 *pal, MFDB *src, MFDB *dst)
{
	int i, j, k, psize, planes;
	unsigned int palidx;
	unsigned long val;
	unsigned short *s_ptr, *src_ptr;
	void *p, **d_ptr;
	struct rgb_1000 *col;

	d_ptr = &p;
	*d_ptr = dst->fd_addr;

	psize = src->fd_wdwidth * src->fd_h;
	src_ptr = src->fd_addr;
	planes = src->fd_nplanes;

	for (k = 0; k < psize; k++, src_ptr++)
	{
		for (i = 0; i < 16; i++)
		{
			palidx = 0;
			s_ptr = src_ptr;
			for (j = 0; j < planes; j++)
			{
				palidx >>= 1;
				palidx |= (*s_ptr & 0x8000);
				*s_ptr <<= 1;
				s_ptr += psize;
			}
			palidx >>= from1_8[planes].shift;
			if (!pal)
			{
				struct rgb_1000 gp;
				val = from1_8[planes].full - palidx;
				val *= 1000;
				val += from1_8[planes].half;
				val /= from1_8[planes].full + 1;
				gp.red = gp.green = gp.blue = (val > 1000) ? 1000 : val;
				*d_ptr = (*to)(&gp, *d_ptr);
			}
			else
			{
				if (src->fd_r1 && palidx < 15)
					palidx = devtovdi8[palidx];

				col = pal + palidx;
				*d_ptr = (*to)(col, *d_ptr);
			}
		}
	}
}

static void
from24b(void *(*to)(struct rgb_1000 *, void *), struct rgb_1000 *pal, MFDB *src, MFDB *dst)
{
	int i, j;
	unsigned long val;
	unsigned char *src_ptr;
	void *p, **d_ptr;
	struct rgb_1000 ppal;

	d_ptr = &p;
	*d_ptr = dst->fd_addr;
	src_ptr = src->fd_addr;

	for (i = 0; i < src->fd_h; i++)
	{
		for (j = 0; j < src->fd_w; j++)
		{
			val = *(unsigned char *)src_ptr++;
			val *= 1000;
			val += 127;
			val >>= 8;
			ppal.red = val > 1000 ? 1000 : val;

			val = *(unsigned char *)src_ptr++;
			val *= 1000;
			val += 127;
			val >>= 8;
			ppal.green = val > 1000 ? 1000 : val;

			val = *(unsigned char *)src_ptr++;
			val *= 1000;
			val += 127;
			val >>= 8;
			ppal.blue = val > 1000 ? 1000 : val;

			*d_ptr = (*to)(&ppal, *d_ptr);
		}
	}
}

static void
from16b(void *(*to)(struct rgb_1000 *, void *), struct rgb_1000 *pal, MFDB *src, MFDB *dst)
{
	int i, j;
	unsigned long val;
	unsigned short *src_ptr, pix;
	void *p, **d_ptr;
	struct rgb_1000 ppal;

	d_ptr = &p;
	*d_ptr = dst->fd_addr;
	src_ptr = src->fd_addr;

	for (i = 0; i < src->fd_h; i++)
	{
		for (j = 0; j < src->fd_w; j++)
		{
			pix = *src_ptr++;
			val = (pix >> 11) & 31;
			val *= 1000;
			val += 127;
			val >>= 8;
			ppal.red = val > 1000 ? 1000 : val;

			val = (pix >> 6) & 63;
			val *= 1000;
			val += 127;
			val >>= 8;
			ppal.green = val > 1000 ? 1000 : val;

			val = pix & 31;
			val *= 1000;
			val += 127;
			val >>= 8;
			ppal.blue = val > 1000 ? 1000 : val;

			*d_ptr = (*to)(&ppal, *d_ptr);
		}
	}
}
#endif

typedef void * to_x_bit(struct rgb_1000 *pal, void *imgdata);
#if 0
		15	moto,
			intel,
		16	moto,
			intel,
			moto 15b,
			intel 15b,
		24	moto,
			intel,
		32	moto,
			intel,
			intel byteswapped,
#endif
#ifndef ST_ONLY
static to_x_bit *f_to15[] =
{
	toF16b,
	toI15b,
};
static to_x_bit *f_to16[] =
{
	toM16b,
	toI16b,
	toF16b,
	toI15b,
};
static to_x_bit *f_to24[] =
{
	toM24b,
	toI24b,
};
static to_x_bit *f_to32[] =
{
	toM32b,
	toI32b,
	toIbs32b
};
#endif

#ifndef ST_ONLY
#if WITH_GRADIENTS
static void
repeat_16bpixel(void *_pixel, void *_dest, int count)
{
	register unsigned short *dest = _dest;
	register unsigned short pixel = *(unsigned short *)_pixel;

	while (count--)
		*dest++ = (unsigned short)pixel;
}

static void
repeat_24bpixel(void *_pixel, void *_dest, int count)
{
	register union { unsigned char b[4]; unsigned short s[2]; unsigned long p;} pxl;
	register union { unsigned char *bp; unsigned short *sp; long lng;} pptr;
	register unsigned long pixel = *(unsigned long *)_pixel;

	pixel >>= 8;

	pxl.p = pixel;
	pptr.bp = _dest;

	pixel >>= 8;

	if (pptr.lng & 1)
	{
		while (count > 1)
		{
			*pptr.bp++ = pxl.b[1];
			*pptr.sp++ = pxl.s[1];
			*pptr.sp++ = pixel;
			*pptr.bp++ = pxl.b[3];
			count -= 2;
		}
		if (count)
		{
			*pptr.bp++ = pxl.b[1];
			*pptr.sp = pxl.s[1];
		}
	}
	else
	{
		while (count > 1)
		{
			*pptr.sp++ = pixel;
			*pptr.bp++ = pxl.b[3];
			*pptr.bp++ = pxl.b[1];
			*pptr.sp++ = pxl.s[1];
			count -= 2;
		}
		if (count)
		{
			*pptr.sp++ = pixel;
			*pptr.bp = pxl.b[3];
		}
	}
}
static void
repeat_32bpixel(void *_pixel, void *_dest, int count)
{
	register unsigned long *dest = _dest;
	register unsigned long pixel = *(unsigned long *)_pixel;

	while (count--)
		*dest++ = pixel;
}

void _cdecl
create_gradient(XAMFDB *pm, struct rgb_1000 *c, short method, short n_steps, short *steps, short w, short h )
{
	pm->mfdb.fd_addr = NULL;

	if (screen.pixel_fmt >= 0)
	{
		if (screen.planes > 8)
		{
			int i, j, pixelsize;
			char *data;
			char *d;
			short wdwidth;
			long pixel, size, scanlen, red, green, blue, ired=0, igreen=0, iblue=0;
			void * (*to)(struct rgb_1000 *pal, void *imgdata);
			void (*rp)(void *, void *, int);
			struct rgb_1000 col;

			wdwidth = (w + 15) >> 4;
			switch (screen.planes)
			{
				case 15: to = f_to15[screen.pixel_fmt]; pixelsize = 2; rp = repeat_16bpixel; break;
				case 16: to = f_to16[screen.pixel_fmt]; pixelsize = 2; rp = repeat_16bpixel; break;
				case 24: to = f_to24[screen.pixel_fmt]; pixelsize = 3; rp = repeat_24bpixel; break;
				case 32: to = f_to32[screen.pixel_fmt]; pixelsize = 4; rp = repeat_32bpixel; break;
				default: to = NULL; pixelsize = 0; rp = NULL; break;
			}
			scanlen = (long)pixelsize * ((w + 15) & ~15);
			size = (long)scanlen * h;

			if (!to || !(data = kmalloc(size)))
				return;

			pm->d_w = w;
			pm->d_h = h;
			pm->mfdb.fd_addr = data;
			pm->mfdb.fd_w = (w + 15) & ~15;
			pm->mfdb.fd_h = h;
			pm->mfdb.fd_wdwidth = wdwidth;
			pm->mfdb.fd_stand = 0;
			pm->mfdb.fd_nplanes = screen.planes == 15 ? 16 : screen.planes;
			pm->mfdb.fd_r1 = pm->mfdb.fd_r2 = pm->mfdb.fd_r3 = 0;

			scanlen = pm->mfdb.fd_w * pixelsize;


			data = pm->mfdb.fd_addr;

			switch (method)
			{

			case 0:
			{
				if( h )
				{
					ired = ((long)(c[1].red - c[0].red) << 16) / h;
					igreen = ((long)(c[1].green - c[0].green) << 16) / h;
					iblue = ((long)(c[1].blue - c[0].blue) << 16) / h;
				}

				red = (long)c[0].red << 16;
				green = (long)c[0].green << 16;
				blue = (long)c[0].blue << 16;

				for (i = 0; i < h; i++)
				{
					col.red = red >> 16;
					col.green = green >> 16;
					col.blue = blue >> 16;
				#if 1
					(*to)(&col, &pixel);
					(*rp)(&pixel, data, w);
				#else
					for (j = 0; j < w; j++)
					{
						d = (*to)(&col, d);
					}
				#endif
					red += ired;
					green += igreen;
					blue += iblue;

					data += scanlen;
				}
				break;
			}
			case 1:
			{
				long rgb[3];

				if( w )
				{
					ired = ((long)(c[1].red - c[0].red) << 16) / w;
					igreen = ((long)(c[1].green - c[0].green) << 16) / w;
					iblue = ((long)(c[1].blue - c[0].blue) << 16) / w;
				}
				red = (long)c[0].red << 16;
				green = (long)c[0].green << 16;
				blue = (long)c[0].blue << 16;

				rgb[0] = red;
				rgb[1] = green;
				rgb[2] = blue;
				for (i = 0; i < h; i++)
				{
					d = data;
					for (j = 0; j < w; j++)
					{
						col.red = red >> 16;
						col.green = green >> 16;
						col.blue = blue >> 16;
						d = (*to)(&col, d);

						red += ired;
						green += igreen;
						blue += iblue;
					}
					red = rgb[0];
					green = rgb[1];
					blue = rgb[2];
					col = c[0];

					data += scanlen;
				}
				break;
			}
			case 2:
			/* c[2], c[3]: chaos-parameters */
			{
				struct rgb_1000 strt;
				long rgb[3];
				long yred, ygreen, yblue;

				yred = ((long)(c[1].red - c[0].red) << 16)  / h;
				ygreen = ((long)(c[1].green - c[0].green) << 16) / h;
				yblue = ((long)(c[1].blue - c[0].blue) << 16) / h;


				col = strt = c[0];
				rgb[0] = (long)strt.red << 16;
				rgb[1] = (long)strt.green << 16;
				rgb[2] = (long)strt.blue << 16;

				for (i = 0; i < h; i++)
				{
					d = data;

					if( w )
					{
						ired = ((long)(c[1].red - strt.red) << 16) / w;
						igreen = ((long)(c[1].green - strt.green) << 16) / w;
						iblue = ((long)(c[1].blue - strt.blue) << 16) / w;
					}
					if( c[3].red )ired += c[3].red;	/* chaos! */
					if( c[3].green )igreen += c[3].green;
					if( c[3].blue )iblue += c[3].blue;

					red = (long)strt.red << 16;
					green = (long)strt.green << 16;
					blue = (long)strt.blue << 16;

					for (j = 0; j < w; j++)
					{
						col.red = red >> 16;
						if( c[2].red )col.red += ((col.green) % c[2].red);	/* chaos! */
						col.green = green >> 16;
						if( c[2].green )col.green += ((col.blue) % c[2].green);
						col.blue = blue >> 16;
						if( c[2].blue )col.blue += ((col.red) % c[2].blue);
						d = (*to)(&col, d);

						red += ired;
						green += igreen;
						blue += iblue;
					}

					rgb[0] += yred;
					rgb[1] += ygreen;
					rgb[2] += yblue;

					strt.red = rgb[0] >> 16;
					strt.green = rgb[1] >> 16;
					strt.blue = rgb[2] >> 16;

					data += scanlen;
				}
				break;
			}
			case 3:
			/* c[3]: chaos-parameters */
			{
				short h1, h2, step = steps[0];

				if (step < 0)
				{
					step = -step;
					if (step > 100)
						step = 100;
					h1 = ((long)h * step) / 100;
					h2 = h - h1;
				}
				else
				{
					h1 = step;
					h2 = h - step;
				}

				if( h1 )
				{
					ired = ((long)(c[1].red - c[0].red) << 16) / h1;
					igreen = ((long)(c[1].green - c[0].green) << 16) / h1;
					iblue = ((long)(c[1].blue - c[0].blue) << 16) / h1;
				}

				red = (long)c[0].red << 16;
				green = (long)c[0].green << 16;
				blue = (long)c[0].blue << 16;

				col.red = red >> 16;
				col.green = green >> 16;
				col.blue = blue >> 16;

				for (i = 0; i < h1; i++)
				{

					(*to)(&col, &pixel);
					(*rp)(&pixel, data, w);

					red += ired;
					green += igreen;
					blue += iblue;
					col.red = red >> 16;
					if( c[3].red )col.red -= ((col.green) % c[3].red);
					col.green = green >> 16;
					if( c[3].green )col.green -= ((col.blue) % c[3].green);
					col.blue = blue >> 16;
					if( c[3].blue )col.blue -= ((col.red) % c[3].blue);

					data += scanlen;
				}

				if( h2 )
				{
					ired = ((long)(c[2].red - col.red) << 16) / h2;
					igreen = ((long)(c[2].green - col.green) << 16) / h2;
					iblue = ((long)(c[2].blue - col.blue) << 16) / h2;
				}

				red = (long)col.red << 16;
				green = (long)col.green << 16;
				blue = (long)col.blue << 16;
				col.red = red >> 16;
				col.green = green >> 16;
				col.blue = blue >> 16;

				for (i = 0; i < h2; i++)
				{
					(*to)(&col, &pixel);
					(*rp)(&pixel, data, w);
					red += ired;
					green += igreen;
					blue += iblue;
					col.red = red >> 16;
					if( c[3].red )col.red += ((col.green) % c[3].red);
					col.green = green >> 16;
					if( c[3].green )col.green += ((col.blue) % c[3].green);
					col.blue = blue >> 16;
					if( c[3].blue )col.blue += ((col.red) % c[3].blue);

					data += scanlen;
				}
				break;
			}
			case 4:
			{
				short w1, w2, step = steps[0];
				if (step < 0)
				{
					step = -step;
					if (step > 100)
						step = 100;
					w1 = ((long)w * step) / 100;
					w2 = w - w1;
				}
				else
				{
					w1 = step;
					w2 = w - step;
				}

				for (i = 0; i < h; i++)
				{
					d = data;

					if( w1 )
					{
						ired =   ((long)(c[1].red - c[0].red) << 16) / w1;
						igreen = ((long)(c[1].green - c[0].green) << 16) / w1;
						iblue =  ((long)(c[1].blue - c[0].blue) << 16) / w1;
					}

					red =	(long)c[0].red << 16;
					green =	(long)c[0].green << 16;
					blue =	(long)c[0].blue << 16;
					for (j = 0; j < w1; j++)
					{
						col.red = red >> 16;
						col.green = green >> 16;
						col.blue = blue >> 16;
						d = (*to)(&col, d);
						red += ired;
						green += igreen;
						blue += iblue;
					}

					if( w2 )
					{
						ired =   ((long)(c[2].red - col.red) << 16) / w2;
						igreen = ((long)(c[2].green - col.green) << 16) / w2;
						iblue =  ((long)(c[2].blue - col.blue) << 16) / w2;
					}

					red =   (long)col.red << 16;
					green = (long)col.green << 16;
					blue =  (long)col.blue << 16;
					for (j = 0; j < w2; j++)
					{
						col.red = red >> 16;
						col.green = green >> 16;
						col.blue = blue >> 16;
						d = (*to)(&col, d);
						red += ired;
						green += igreen;
						blue += iblue;
					}
					data += scanlen;
				}
				break;
				default:;
				}
			}
		}
	}
}
#endif	/* ST_ONLY */
#endif	/* WITH_GRADIENTS */

static void
image_cache_get(char *name, XAMFDB *mimg)
{
	struct file *f;

	mimg->mfdb.fd_addr = NULL;
	f = kernel_open(name, O_RDONLY, NULL, NULL);
	if (f)
	{
		long bmread, bmsize;
		bmread = kernel_read(f, mimg, sizeof(XAMFDB));
		bmsize = (long)((long)mimg->mfdb.fd_wdwidth * (mimg->mfdb.fd_nplanes == 15 ? 16 : mimg->mfdb.fd_nplanes) * mimg->mfdb.fd_h) << 1;
		if ((mimg->mfdb.fd_addr = kmalloc(bmsize)))
			bmread += kernel_read(f, mimg->mfdb.fd_addr, bmsize);
		else
			BLOG((0,"Not enough memory for %s (%ld bytes)", name, bmsize));

		kernel_close(f);
		if (bmread != (sizeof(XAMFDB)+bmsize))
		{
			BLOG((0,"Read error for %s: read %ld bytes (must be %ld)", name, bmread, sizeof(XAMFDB)+bmsize));
			if (mimg->mfdb.fd_addr)
			{
				kfree(mimg->mfdb.fd_addr);
				mimg->mfdb.fd_addr = NULL;
			}
			_f_delete(name);
		}
	}
	else
	{
		BLOG((0,"File %s not found", name));
	}
}

static void
image_cache_put(char *name, XAMFDB *mimg)
{
	struct file *f;

	f = kernel_open(name, O_RDWR|O_CREAT|O_TRUNC, NULL, NULL);
	if (!f)
	{
		char path[PATH_MAX];
		long err;

		sprintf(path, PATH_MAX-1, "%scache", C.Aes->home_path);
		err = _d_create( path );
		if (err)	/* could not mkdir */
		{
			BLOG((0,"image_cache_put: could not create %s", path ));
			return;
		}
		/* Retry */
		f = kernel_open(name, O_RDWR|O_CREAT|O_TRUNC, NULL, NULL);
	}
	if (f)
	{
		long bmsize = (long)((long)mimg->mfdb.fd_wdwidth * (mimg->mfdb.fd_nplanes == 15 ? 16 : mimg->mfdb.fd_nplanes) * mimg->mfdb.fd_h) << 1;
		long bmwrite = kernel_write(f, mimg, sizeof(XAMFDB));
		bmwrite += kernel_write(f, mimg->mfdb.fd_addr, bmsize);
		kernel_close(f);
		if (bmwrite != (sizeof(XAMFDB)+bmsize))
		{
			BLOG((0,"Write error for %s: write %ld bytes (must be %ld)", name, bmwrite, sizeof(XAMFDB)+bmsize));
			_f_delete(name);
		}
	}
}

void
load_image(char *name, XAMFDB *mimg)
{
	XA_XIMG_HEAD xa_img;
	struct ximg_header *ximg = &xa_img.ximg;
	long bmsize;
	char cache_path[PATH_MAX];

	if (cfg.textures_cache)
	{
		sprintf(cache_path, PATH_MAX-1, "%scache\\", C.Aes->home_path);
		strip_fname(name, 0, cache_path+strlen(cache_path));
		sprintf(strrchr(cache_path, '.')+1, 4, "%d", screen.planes);
		image_cache_get(cache_path, mimg);
		if (mimg->mfdb.fd_addr)                      /* Cache hit! */
			return;
	}

	depack_img(name, &xa_img);
	mimg->mfdb.fd_addr = NULL;

	if (xa_img.addr)
	{
		XAMFDB msrc;

		msrc.mfdb.fd_addr	= xa_img.addr;
		msrc.mfdb.fd_w		= (ximg->img_w + 15) & ~15;
		msrc.mfdb.fd_h		= ximg->img_h;
		msrc.mfdb.fd_wdwidth	= (ximg->img_w + 15) >> 4;
		msrc.mfdb.fd_stand	= 1;
		msrc.mfdb.fd_nplanes	= ximg->planes;
		msrc.mfdb.fd_r1 = msrc.mfdb.fd_r2 = msrc.mfdb.fd_r3 = 0;
		msrc.d_w = ximg->img_w;
		msrc.d_h = ximg->img_h;

		if (xa_img.palette)
		{
			if (ximg->planes <= 8 && screen.planes == 8)
			{
				unsigned char cref[256];
				build_pal_xref((struct rgb_1000 *)xa_img.palette, screen.palette, (unsigned char *)&cref, (1 << msrc.mfdb.fd_nplanes));
				remap_bitmap_colindexes(&msrc.mfdb, (unsigned char *)&cref, 0);
			}
		}

		if (ximg->planes < 8 && screen.planes == 8)
		{
			long newsize, oldsize;
			unsigned char *newdata;

			newsize = (long)(((msrc.mfdb.fd_w + 15) >> 4) << 1) * msrc.mfdb.fd_h * 8;
			oldsize = (long)(((msrc.mfdb.fd_w + 15) >> 4) << 1) * msrc.mfdb.fd_h * msrc.mfdb.fd_nplanes;
			if ((newdata = kmalloc(newsize)))
			{
				bzero(newdata, newsize);
				memcpy(newdata, msrc.mfdb.fd_addr, oldsize);
				kfree(msrc.mfdb.fd_addr);
				msrc.mfdb.fd_addr = xa_img.addr = newdata;
				msrc.mfdb.fd_nplanes = 8;
			}
			else
			{
				kfree(xa_img.addr);
				mimg->mfdb.fd_addr = NULL;
				return;
			}
		}

		*mimg = msrc;
		mimg->mfdb.fd_nplanes = screen.planes;
		bmsize = (long)((long)mimg->mfdb.fd_wdwidth * (mimg->mfdb.fd_nplanes == 15 ? 16 : mimg->mfdb.fd_nplanes) * mimg->mfdb.fd_h) << 1;

		if ((mimg->mfdb.fd_addr = kmalloc(bmsize)))
		{
#ifndef ST_ONLY
			if (screen.planes > 8)
			{
				bool fail = true;
				void * (*to)(struct rgb_1000 *pal, void *imgdata);
				void (*from)(void *(*)(struct rgb_1000 *, void *), struct rgb_1000 *pal, MFDB *src, MFDB *dst);

				to = (void *)-1L;
				from = (void *)-1L;

				BLOG((0,"\r\npixel format:screen:planes=%d pixel-format=%d src:%d", screen.planes, screen.pixel_fmt, msrc.mfdb.fd_nplanes));
				if (screen.pixel_fmt >= 0)
				{
					switch (screen.planes)
					{
						case 15: to = f_to15[screen.pixel_fmt]; break;
						case 16: to = f_to16[screen.pixel_fmt]; break;
						case 24: to = f_to24[screen.pixel_fmt]; break;
						case 32: to = f_to32[screen.pixel_fmt]; break;
						default: to = NULL; break;
					}

					switch (msrc.mfdb.fd_nplanes)
					{
						case 1 ... 8: from = from8b;  break;
#if 0
						case 15: from = from15b; break;
#endif
						case 16: from = from16b; break;
						case 24: from = from24b; break;
						default: from = NULL;    break;
					}
					if (from && to)
					{
						(*from)(to, (struct rgb_1000 *)xa_img.palette, &msrc.mfdb, &mimg->mfdb);
						fail = false;
					}
				}
				if (fail)
				{
					if (!from)
						display("\r\n  cannot handle %d bpp images!", msrc.mfdb.fd_nplanes);
					if (!to)
						display("\r\n  cannot handle %d bpp screen modes!", screen.planes);
					if (screen.pixel_fmt < 0)
						display("\r\n unknown pixel format:screen:planes=%d pixel-format=%d src:%d- cannot transform from=%lx to=%lx",
							screen.planes, screen.pixel_fmt, msrc.mfdb.fd_nplanes, (unsigned long)from, (unsigned long)to);
					kfree(mimg->mfdb.fd_addr);
					mimg->mfdb.fd_addr = NULL;
				}
			}
			else
#endif
			{
				vr_trnfm(C.P_handle, &msrc.mfdb, &mimg->mfdb);
			}
			kfree(xa_img.addr);
		}
		else
		{
			mimg->mfdb.fd_addr = msrc.mfdb.fd_addr;
			vr_trnfm(C.P_handle, &msrc.mfdb, &mimg->mfdb);
		}
		mimg->mfdb.fd_stand = 0;

		if (xa_img.palette)
			kfree(xa_img.palette);

		/* Cache system for textures/images
		 * - If cache is ON and .img successfully loaded, write file
		 */
		if (cfg.textures_cache && mimg->mfdb.fd_addr)
			image_cache_put(cache_path, mimg);
	}
}

static long
map_size(MFDB *m, int i)
{
	long l = 2L * m->fd_wdwidth * m->fd_h * m->fd_nplanes;

	/* DIAGS(("[%d]map_size: wd %d, h %d, planes %d --> %ld",
	 * 	  i, m->fd_wdwidth, m->fd_h, m->fd_nplanes, l)); */

	return l;
}
#if 0
bool
transform_bitmap(short vdih, MFDB *src, MFDB *dst, struct rgb_1000 *src_pal, struct rgb_1000 *sys_pal)
{
	bool ret = false;
	char *newdata = NULL;

	if (src->fd_nplanes > dst->fd_nplanes)
		return false;

	if (src->fd_nplanes <= 8 && dst->fd_nplanes <= 8)
	{
		if (src->fd_nplanes <= 8 && dst->fd_nplanes >= 2)
		{
			long newsize, oldsize;
			if (src_pal && sys_pal)
			{
				unsigned char cref[256];
				build_pal_xref(src_pal, sys_pal, cref, (1 << src->fd_nplanes));
				remap_bitmap_colindexes(src, cref, 16);
			}
			newsize = (long)(((src->fd_w + 15) >> 4) << 1) * src->fd_h * dst->fd_nplanes;
			oldsize = (long)(((src->fd_w + 15) >> 4) << 1) * src->fd_h * src->fd_nplanes;
			if ((newdata = kmalloc(newsize)))
			{
				bzero(newdata, newsize);
				memcpy(newdata, src->fd_addr, oldsize);
				src->fd_addr = newdata;
				src->fd_nplanes = dst->fd_nplanes;
			}
			else
			{
				return false;
			}
		}
		if ((src->fd_nplanes | dst->fd_nplanes) != 1)
			vr_trnfm(vdih, src, dst);
		else
		{
			short i, *s, *d;
			s = src->fd_addr;
			d = dst->fd_addr;
			for (i = src->fd_wdwidth * src->fd_h; i > 0; i--)
				*d++ = *s++;
		}
		ret = true;
	}
	else
	{
/* ************************************************* */
		if (dst->fd_nplanes > 8)
		{
			void (*to)(struct rgb_1000 *, void **);
			void (*from)(void (*)(struct rgb_1000 *, void **), struct rgb_1000 *, MFDB *, MFDB *);

			to = (void *)-1L;
			from = (void *)-1L;
			if (src->fd_nplanes <= 8 && src_pal && sys_pal)
			{
				unsigned char cref[256];
				build_pal_xref(src_pal, sys_pal, cref, (1 << src->fd_nplanes));
				remap_bitmap_colindexes(src, cref, 16);
			}
			if (screen.pixel_fmt >= 0)
			{
				switch (dst->fd_nplanes)
				{
					case 15: to = f_to15[screen.pixel_fmt]; break;
					case 16: to = f_to16[screen.pixel_fmt]; break;
					case 24: to = f_to24[screen.pixel_fmt]; break;
					case 32: to = f_to32[screen.pixel_fmt]; break;
					default: to = NULL; break;
				}

				switch (src->fd_nplanes)
				{
					case 1 ... 8: from = from8b;  break;
					case 16: from = from16b; break;
					case 24: from = from24b; break;
					default: from = NULL;    break;
				}
				if (from && to)
				{
					src->fd_r1 = 1;
					(*from)(to, src_pal ? src_pal : sys_pal, src, dst);
					src->fd_r1 = 0;
					ret = true;
				}
			}
		}
	}
	if (newdata)
		kfree(newdata);

	return ret;
/* ************************************************* */
}
#else
#define DBG_hex dump_hex

#define CLUT_NI 12312
#define CREF_NI 0
#define RECALC_REMAP_LUT 127

bool
transform_gem_bitmap(short vdih, MFDB msrc, MFDB mdest, short planes, struct rgb_1000 *src_pal, struct rgb_1000 *sys_pal)
{
#ifndef ST_ONLY
	static short clut8[256] = {CLUT_NI}, clut4[256] = {CLUT_NI};
	short *clp;
#endif
	short src_planes, dst_planes;

	src_planes = planes;
	dst_planes = mdest.fd_nplanes;

	if (src_planes <= 8 && dst_planes <= 8)
	{
#ifndef ST_ONLY
		if(dst_planes == 8)	/* 4 does not work yet */
		if (src_pal && sys_pal)
		{
			static uchar cref8[256] = {CREF_NI}, cref4[256] = {CREF_NI};
			uchar *crp;
			switch (src_planes)
			{
			case 4:
				crp = cref4;
			goto case_88;
			case 8:
				crp = cref8;
case_88:
				if( C.is_init_icn_pal & src_planes )
				{
					short p;
					p = 1 << src_planes;
					if( dst_planes == 8 && cref4[255] == 0 )
						build_pal_xref(src_pal, sys_pal, cref4, 256);
					build_pal_xref(src_pal, sys_pal, crp, p );
					C.is_init_icn_pal &= ~src_planes;
				}

				remap_bitmap_colindexes(&msrc, crp, 4);
			}
		}
#endif
		if ((src_planes | dst_planes) != 1)
		{
			vr_trnfm(vdih, &msrc, &mdest);
		}
		else
		{
			short i, *s, *d;
			s = msrc.fd_addr;
			d = mdest.fd_addr;
			for (i = msrc.fd_wdwidth * msrc.fd_h; i > 0; i--)
				*d++ = *s++;
		}
	}
	else if (src_planes <= 8)
	{
		short i;
		short x, y, pxy[8], colours[2];
		long plane_int_size = msrc.fd_wdwidth * msrc.fd_h;
		short const *colour_lut;
		short words[8], mask;
		char used_colours[256];
		MFDB tmp, tmp2;


		DIAGS(("dst.planes %d src.fd_stand %d", dst_planes, msrc.fd_stand));
		tmp = msrc;
		tmp.fd_nplanes = 1;
		tmp.fd_h = 1 << (src_planes);
		tmp.fd_addr = kmalloc(map_size(&tmp, 1));
		if (!tmp.fd_addr)
			return false;

		tmp2 = tmp;
		tmp2.fd_stand = 0;
		tmp2.fd_addr = kmalloc(map_size(&tmp2, 2));
		if (!tmp2.fd_addr)
			tmp2.fd_addr = tmp.fd_addr;

		mask = (1 << src_planes) - 1;

		colours[1] = 0;
		pxy[4] = pxy[0] = 0;
		pxy[5] = pxy[1] = 0;
		pxy[6] = pxy[2] = mdest.fd_w - 1;
		pxy[7] = pxy[3] = mdest.fd_h - 1;
		/* vro_cpyfm(vdih, ALL_WHITE, pxy, &mdest, &mdest);*/
		/* The above line crashes in Falcon TC (w/o NVDI) for some reason ?! */

		DIAGS(("mdest planes %d", mdest.fd_nplanes));
		bzero(mdest.fd_addr, map_size(&mdest,3));

		switch (src_planes)
		{
#ifndef ST_ONLY
			case 4:
				clp = clut4;
			goto case_8;
			case 8:
				clp = clut8;
case_8:
				colour_lut = tovdilut[src_planes];
				/* convert 8bit-icon-palette to system-palette */
				if( src_pal )
				{
					if( C.is_init_icn_pal & src_planes )
					{
						uchar pal_cref[256];
						short lut[256];
						short p = 256;
						build_pal_xref(src_pal, sys_pal, pal_cref, p);
						memcpy( lut, colour_lut, p* sizeof(short) );
						for( i = 0; i < p; i++ )
							clp[i] = pal_cref[lut[i]];
						C.is_init_icn_pal &= ~src_planes;
					}
					colour_lut = clp;
				}
#endif
			break;
			case 2:	colour_lut = devtovdi2;	break;
			case 1: colour_lut = devtovdi1;	break;
			default:
				/* This should never happen */
				kfree(tmp.fd_addr);
				if (tmp2.fd_addr != tmp.fd_addr)
					kfree(tmp2.fd_addr);
				return false;
		}

		/* foreach src image line */
		for (y = 0; y < msrc.fd_h; y++)
		{
			pxy[4] = pxy[0] = 0;
			pxy[5] = pxy[1] = 0;
			pxy[6] = pxy[2] = tmp.fd_w - 1;
			pxy[7] = pxy[3] = tmp.fd_h - 1;
			/* vro_cpyfm(vdih, ALL_WHITE, pxy, &tmp, &tmp);*/
			/* The above line crashes in Falcon TC (w/o NVDI) for some reason ?! */

			bzero(tmp.fd_addr, map_size(&tmp, 4));
			bzero(used_colours, sizeof(used_colours));
			for (x = 0; x < msrc.fd_wdwidth; x++)
			{
				short *ptr = (short *)((char *)msrc.fd_addr + (y * msrc.fd_wdwidth * 2) + (x << 1));
				/* Get the 16 bit from each plane in words[8] */
				i = 0;
				while (i < src_planes)
				{
					words[i++] = *ptr;
					ptr += plane_int_size;
				}

				/* Now built the colour index number from them
				 * by taking 1 bit from each words[j]. */
				for (i = 0; i < 16; i++)
				{
					/* interleaved to chunky conversion */
					short j;
					unsigned short pixcol = 0;
					for (j = src_planes - 1; j >= 0; j--)
					{
						pixcol <<= 1;
						pixcol |= (words[j] >> i) & 1;
					}

					pixcol &= mask;

					ptr = (short *)((char *)tmp.fd_addr + (pixcol * msrc.fd_wdwidth * 2) + (x << 1));
					*ptr |= 1 << i;
					used_colours[pixcol] = 1;
				}
			}

			/* src fd_stand to device dependent format translation
			 * (for the current line) */
			pxy[5] = pxy[7] = y;
			vr_trnfm( vdih, &tmp, &tmp2 );

			/* for each color in src MFDB color depth */
			for (i = 0; i < tmp.fd_h; i++)
			{
				/* only for the used ones */
				if (used_colours[i])
				{
					/* the system palette is a bit complicated */
					colours[0] = colour_lut[i];

					pxy[1] = pxy[3] = i;
					/* project transparently to the mdest */
					vrt_cpyfm( vdih, MD_TRANS, pxy, &tmp2, &mdest, colours );
				}
			}
		}

		if (tmp2.fd_addr != tmp.fd_addr)
			kfree(tmp2.fd_addr);

		kfree(tmp.fd_addr);
	}
	else
		return false;

	return true;
}
#endif
#if 0
bool
transform_gem_bitmap_data(short vdih, MFDB msrc, MFDB mdest, int src_planes, int dst_planes)
{

	if (src_planes <= 8 && dst_planes <= 8)
	{
		vr_trnfm(vdih, &msrc, &mdest);
	}
	else if (src_planes <= 8)
	{
		int i;
		short x, y, pxy[8], colours[2];
		long plane_int_size = msrc.fd_wdwidth * msrc.fd_h;
		short *colour_lut, words[8], mask;
		char used_colours[256];
		MFDB tmp, tmp2;

		DIAGS(("dst.planes %d src.fd_stand %d", dst_planes, msrc.fd_stand));
		tmp = msrc;
		tmp.fd_nplanes = 1;
		tmp.fd_h = 1 << (src_planes);
		tmp.fd_addr = kmalloc(map_size(&tmp, 1));
		if (!tmp.fd_addr)
			return false;

		tmp2 = tmp;
		tmp2.fd_stand = 0;
		tmp2.fd_addr = kmalloc(map_size(&tmp2, 2));
		if (!tmp2.fd_addr)
			tmp2.fd_addr = tmp.fd_addr;

		mask = (1 << src_planes) - 1;

		colours[1] = 0;
		pxy[4] = pxy[0] = 0;
		pxy[5] = pxy[1] = 0;
		pxy[6] = pxy[2] = mdest.fd_w - 1;
		pxy[7] = pxy[3] = mdest.fd_h - 1;
		/* vro_cpyfm(vdih, ALL_WHITE, pxy, &mdest, &mdest);*/
		/* The above line crashes in Falcon TC (w/o NVDI) for some reason ?! */

		DIAGS(("mdest planes %d", mdest.fd_nplanes));
		bzero(mdest.fd_addr, map_size(&mdest,3));

		switch (src_planes)
		{
			case 8:	colour_lut = devtovdi8;	break;
			case 4:	colour_lut = devtovdi4;	break;
			case 2:	colour_lut = devtovdi2;	break;
			case 1: colour_lut = devtovdi1;	break;
			default:
				/* This should never happen */
				kfree(tmp.fd_addr);
				if (tmp2.fd_addr != tmp.fd_addr)
					kfree(tmp2.fd_addr);
				return false;
		}

		/* foreach src image line */
		for (y = 0; y < msrc.fd_h; y++)
		{
			pxy[4] = pxy[0] = 0;
			pxy[5] = pxy[1] = 0;
			pxy[6] = pxy[2] = tmp.fd_w - 1;
			pxy[7] = pxy[3] = tmp.fd_h - 1;
			/* vro_cpyfm(vdih, ALL_WHITE, pxy, &tmp, &tmp);*/
			/* The above line crashes in Falcon TC (w/o NVDI) for some reason ?! */

			bzero(tmp.fd_addr, map_size(&tmp, 4));
			bzero(used_colours, sizeof(used_colours));
			for (x = 0; x < msrc.fd_wdwidth; x++)
			{
				short *ptr = (short *)((char *)msrc.fd_addr + (y * msrc.fd_wdwidth * 2) + (x << 1));
				/* Get the 16 bit from each plane in words[8] */
				i = 0;
				while (i < src_planes)
					words[i++] = *ptr,
					ptr += plane_int_size;

				/* Now built the colour index number from them
				 * by taking 1 bit from each words[j]. */
				for (i = 0; i < 16; i++)
				{
					/* interleaved to chunky conversion */
					int j;
					unsigned short pixcol = 0;
					for (j = src_planes - 1; j >= 0; j--)
					{
						pixcol <<= 1;
						pixcol |= (words[j] >> i) & 1;
					}

					pixcol &= mask;

					ptr = (short *)((char *)tmp.fd_addr + (pixcol * msrc.fd_wdwidth * 2) + (x << 1));
					*ptr |= 1 << i;
					used_colours[pixcol] = 1;
				}
			}

			/* src fd_stand to device dependent format translation
			 * (for the current line) */
			pxy[5] = pxy[7] = y;
			vr_trnfm( vdih, &tmp, &tmp2 );

			/* for each color in src MFDB color depth */
			for (i = 0; i < tmp.fd_h; i++)
			{
				/* only for the used ones */
				if (used_colours[i])
				{
					/* the system palette is a bit complicated */
					colours[0] = colour_lut[i];

					pxy[1] = pxy[3] = i;
					/* project transparently to the mdest */
					vrt_cpyfm( vdih, MD_TRANS, pxy, &tmp2, &mdest, colours );
				}
			}
		}

		if (tmp2.fd_addr != tmp.fd_addr)
			kfree(tmp2.fd_addr);

		kfree(tmp.fd_addr);
	}
	else
		return false;

	return true;
}
#endif

void
fix_rsc_palette(struct xa_rsc_rgb *palette)
{
	int i;
	long palen;
	struct rgb_1000 *new;

	palen = sizeof(*new) * 256;
	new = kmalloc(palen);
	assert(new);

	for (i = 0; i < 256; i++)
	{
		short p = palette[i].pen;
		if (p >= 0 && p < 256)
		{
			new[p].red	= palette[i].red;
			new[p].green	= palette[i].green;
			new[p].blue	= palette[i].blue;
		}
	}
	memcpy(palette, new, palen);
	kfree(new);
}

/*
 * md = READ: read palette-file and copy palette into palette
 * md = WRITE: unimplemented
 * if path=0 use f else path/f.pal
 * return 0 for success
 */
int rw_syspalette( int md, struct rgb_1000 *palette, char *path, char *f )
{
	short buf[1024], palsz, pens;
	char fn[PATH_MAX];
	long err;
	struct file *fp;

	if (screen.planes > 8)
		pens = 256;
	else
		pens = 1 << screen.planes;

	palsz = pens * 3 * 2 + 4;
	if( path )
		sprintf( fn, sizeof(fn), "%spal/%s.pal", path, f );
	else
		strcpy( fn, f );

	BLOG((0,"loading palette:'%s'", fn));
	fp = kernel_open( fn, O_RDONLY, &err, NULL );
	if( !fp )
	{
		BLOG((0,"palette not found:%ld", err));
		return 1;
	}
	err = kernel_read( fp, buf, sizeof(buf)-1 );
	kernel_close( fp );
	if( err != palsz )
	{
		BLOG((0,"palette-size wrong:%ld, should be %d", err, palsz));
		/* return 2; */
	}
	if( memcmp( (char*)buf, "PA01", 4 ) )
	{
		BLOG((0,"%s:no palette-file", fn ));
		return 3;
	}

	palsz -= 4;
	memcpy( palette, &buf[2], palsz );
	return 0;
}

void
set_syspalette(short vdih, struct rgb_1000 *palette)
{
	short i, pens;

	if (screen.planes > 8)
		pens = 256;
	else
		pens = 1 << screen.planes;

	for (i = 0; i < pens; i++)
	{
		vs_color(vdih, i, (short *)&(palette[i]));
	}
}

void
get_syspalette(short vdih, struct rgb_1000 *palette)
{
	int i, pens;

	switch (screen.planes)
	{
		case 1:	pens =  2; break;
		case 2: pens =  4; break;
		case 4: pens = 16; break;
		case 0: case 16: case 24: case 32: return;
		default: pens = 256; break;
	}
	for (i = 0; i < pens; i++)
	{
		union { short rgb[3]; struct rgb_1000 rgb_1000; } rgbunion;
		short ind;

		ind = vq_color(vdih, i, 0, rgbunion.rgb);
		if (ind >= 0)
		{
			palette[i] = rgbunion.rgb_1000;
		}
	}
}
#if INCLUDE_UNUSED
void
set_defaultpalette(short vdih)
{
	union { short *sp; struct rgb_1000 *rgb;} ptrs;

	ptrs.sp = systempalette;
	set_syspalette(vdih, ptrs.rgb);
}
#endif


#if INCLUDE_UNUSED
static COLOR_TAB256 syscol;

void
set_syscolor(void)
{
	if (vq_ctab(C.P_handle, sizeof(COLOR_TAB256), (COLOR_TAB *)&syscol))
	{
		vq_dflt_ctab(C.P_handle, sizeof(COLOR_TAB256), (COLOR_TAB *)&syscol);
		syscol.tab.map_id = vq_ctab(C.P_handle, sizeof(COLOR_TAB256), (COLOR_TAB *)&syscol);
		vs_ctab(C.P_handle, (COLOR_TAB *)&syscol);
	}
}
#endif

/*
 * Ozk: Attempt to detect pixel format in 15 bpp and above
 */
short
detect_pixel_format(struct xa_vdi_settings *v)
{
	short ret = 8;	/* generic */

	if (screen.planes > 8)
	{
		MFDB scr, dst;
#if HAVE_VS_COLOR
		struct rgb_1000 srgb, rgb;
#endif
		short pxy[8];
		union { unsigned short w[64]; unsigned long l[32];} b;

		(*v->api->wr_mode)(v, MD_REPLACE);
		(*v->api->l_type)(v, 1);
		(*v->api->l_ends)(v, 0, 0);
		(*v->api->l_width)(v, 1);
#if HAVE_VS_COLOR
		vq_color(v->handle, 0, 1, (short *)&srgb);
		rgb.red = 1000;
		rgb.green = 1000;
		rgb.blue = 0;
		vs_color(v->handle, 0, (short *)&rgb);
		(*v->api->line)(v, 0, 0, 0, 0, 0);
#else
		{
		GRECT r = {0,0,v->screen.w,v->screen.h};
		(*v->api->f_color)(v, 6 );	/* yellow */
		(*v->api->gbar)( v, 0, &r );
		}
#endif
		scr.fd_addr = NULL;

		dst.fd_addr = &b;
		dst.fd_w = 1;
		dst.fd_h = 1;
		dst.fd_wdwidth = 1;
		dst.fd_stand = 1;
		dst.fd_nplanes = screen.planes;
		dst.fd_r1 = dst.fd_r2 = dst.fd_r3 = 0;

		pxy[0] = 0;
		pxy[1] = 0;
		pxy[2] = 0;
		pxy[3] = 0;
		pxy[4] = 0;
		pxy[5] = 0;
		pxy[6] = 0;
		pxy[7] = 0;

		vro_cpyfm(v->handle, S_ONLY, pxy, &scr, &dst);

#if HAVE_VS_COLOR
		vs_color(v->handle, 0, (short *)&srgb);
#endif
		switch (screen.planes)
		{
	/* pixelformat E07F */

						/* 12345678.12345678 */
			case 15:
			{
				unsigned short pix = b.w[0];
				if (pix == ((31 << 2) | (7 << 13) | 3))		/* gggbbbbb.0rrrrrgg */
				{
					ret = 1;
				}
				else if (pix == ((31 << 11) | (31 << 6))) 	/* rrrrrggg.ggobbbbb */
				{
					ret = 0;
				}
				else
				{
					ret = -1;
				}
				break;
			}
			case 16:
			{
				unsigned short pix = b.w[0];
				if (pix == ((31 << 3) | (7 << 13) | 7))		/* gggbbbbb.rrrrrggg */
				{
					ret = 1;
				}
				else if (pix == ((31 << 11) | (63 << 5)))	/* rrrrrggg.gggbbbbb */
				{
					ret = 0;
				}
				else if (pix == ((31 << 11) | (31 << 6)))	/* rrrrrggg.ggobbbbb */
				{
					ret = 2;
				}
				else if (pix == ((31 << 2) | (7 << 13) | 3))	/* gggbbbbb.0rrrrrgg */
				{
					ret = 3;
				}
				else
				{
					ret = -1;
				}
				break;
			}
			case 24:
			{
				unsigned long pix = b.l[0];
				pix >>= 8;
				if (pix == 0xffff00L)			/* rrrrrrrr.gggggggg.bbbbbbbb  Moto */
				{
					ret = 0;
				}
				else if (pix == 0xffffL)		/* gggggggg.rrrrrrrrbbbbbbbb */
				{
					ret = 1;
				}
				else
				{
					ret = -1;
				}
				break;
			}
			case 32:
			{
				unsigned long pix = b.l[0];
				if (pix == 0xffff00L)			/* 00000000.rrrrrrrr.gggggggg.bbbbbbbb */
				{
					ret = 0;
				}
				else if (pix == 0xffff0000L)		/* rrrrrrrr.gggggggg.bbbbbbbb.00000000 */
				{
					ret = 1;
				}
				else if (pix == 0x0000ffffL)		/* 00000000.bbbbbbbb.gggggggg.rrrrrrrr */
				{
					ret = 2;
				}
				else
				{
					ret = -1;
				}
				break;
			}
			default:
			{
				ret = -2;
				break;
			}
		}
	}
	return ret;
}
