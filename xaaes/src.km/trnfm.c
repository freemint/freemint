/*
 * $Id$
 * 
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

#include "trnfm.h"
#if 1
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

static short devtovdi8[] =
{
 0,2,3,6,4,7,5,8,9,10,11,14,12,15,13,255,
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
/*                                                   1  1  1  1  1  1 */
/*                           0  1  2 3 4 5 6 7 8  9  0  1  2  3  4  5 */
static short devtovdi4[] = { 0, 2 ,3,6,4,7,5,8,9,10,11,14,12,15,13,1  };
static short vditodev4[] = { 0,255,1,2,4,6,3,5,7, 8, 9,10,12,14,11,13 };

static short devtovdi2[] = { 0,2,3,1 };

static short devtovdi1[] = { 0,1 };

static unsigned long
get_coldist(struct rgb_1000 *src, struct rgb_1000 *dst)
{
	long r, g, b;

	r = dst->red;
	r -= src->red;
	r *= r;

	g = dst->green;
	g -= src->green;
	g *= g;

	b = dst->blue;
	b -= src->blue;
	b *= b;

	r += (g + b);

	return r;
}

static unsigned char
fgetc(struct file *fp)
{
	unsigned char ret;
	kernel_read(fp, &ret, 1);
	return (volatile unsigned char)ret;
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

/* pixelformat E07F */
static void
toI15b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned short *img = *img_ptr;
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
	*img_ptr = img;
}
static void
toF16b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned short *img = *img_ptr;
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
	*img_ptr = img;
}

static void
toM16b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned short *img = *img_ptr;
	unsigned long register r, g, b;

	r = pal->red;
	r <<= 5;
	r += 500;
	r /= 1000;
	if (r > 31)
		r = 31;

	g = pal->green;
	g <<= 6; 		//*= 64;
	g += 500;
	g /= 1000;
	if (g > 63)
		g = 63;

	b = pal->blue;
	b <<= 5;		//*= 32;
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
	*img_ptr = img;

}
static void
toI16b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned short *img = *img_ptr;
	unsigned long register r, g, b;

	img = *img_ptr;
	
	r = pal->red;
	r <<= 5;		//*= 32;
	r += 500;
	r /= 1000;
	if (r > 31)
		r = 31;

	g = pal->green;
	g <<= 6;		// 64;
	g += 500;
	g /= 1000;
	if (g > 63)
		g = 63;

	b = pal->blue;
	b <<= 5;		//*= 32;
	b += 500;
	b /= 1000;
	if (b > 31)
		b = 31;

/* gggbbbbb.rrrrrggg */

	r <<= 3;
	r |= (g >> 3);
	
	r |= (((g & 7) << 5) | b) << 8;
	
	*img++ = r;
	*img_ptr = img;
};

static void
toI24b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned char *img = *img_ptr;
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
	*img_ptr = img;
};
static void
toM24b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned char *img = *img_ptr;
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
	*img_ptr = img;
};

static void
toI32b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned long *img = *img_ptr;
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
	*img_ptr = img;
};

static void
toM32b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned long *img = *img_ptr;
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
	*img_ptr = img;
};

static void
toIbs32b(struct rgb_1000 *pal, void **img_ptr)
{
	unsigned long *img = *img_ptr;
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
	*img_ptr = img;
};

/*
 * build_pal_xref will build a table of cross-reference palette indexes.
 * This means that for each entry in the source palette, cref contains the
 * index into the destination palette where the color closest to the one
 * in the src palette was found.
 */
void
build_pal_xref(struct rgb_1000 *src_palette, struct rgb_1000 *dst_palette, unsigned char *cref, int pens)
{
	struct rgb_1000 *dst, *src, *s, *d;
	unsigned long closest, this;
	int i, j, c;
	
	if (pens > 256)
		pens = 256;

	/*
	 * Normally, We do not remap any referances to pens 0 - 15
	 */
	for (i = 0; i < pens && i < 16; i++)
		cref[i] = i;

	for (i = 1, src = src_palette + 1; i < pens; i++, src++)
	{
		dst = dst_palette + 16;
		s = src;
		closest = 0xffffffffL;

		c = 0;
		d = NULL;
		/*
		 * I'm a bit unsure here -- would it be best to avoid remapping
		 * bitmap pen referances down to pens 0 - 16?
		 */
		for (j = 1, dst = dst_palette + 1; j < pens; j++, dst++)
		{
			if (!(this = get_coldist(src, dst)))
			{
				closest = this;
				d = dst;
				c = j;
				break;
			}
			else if (this < closest) // > this)
			{
				d = dst;
				closest = this;
				c = j;
			}
		}
		if (c < 16)
		{
			/* We need to convert from vdi to dev here so vr_trnfm() converts
			 * back to what we already know here...
			 */
			c = vditodev4[c];
		}
		cref[i] = c;
	
// 		if (D) display("src %03d, %03d, %03d, dst %03d, %03d, %03d",
// 			src->red, src->green, src->blue, dst->red, dst->green, dst->blue);
// 		if (D) display(" %04lx xref %d to %d(%d)", closest, i, c, j, cref[i]);
	}
}
/*
 * Remap the bitmap palette referances.
 */
void
remap_bitmap_colindexes(MFDB *map, unsigned char *cref)
{
	int planes, psize;
	int i, j, k, bit;
	unsigned short *data, *d_ptr;
	unsigned short d[8];
	unsigned short cref_ind, mask;

	planes = map->fd_nplanes;
	psize = map->fd_wdwidth * map->fd_h;
	data = map->fd_addr;
	
	DIAGS(("remap_bitmap_colindex: planes = %d, psize = %d, data = %lx",
		planes, psize, data));

	for (k = 0; k < psize; k++, data++)
	{
		for (i = 0; i < 8; i++)
			d[i] = 0;

		mask = 0x8000;
		bit = 15;
		for (i = 0; i < 16; i++)
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
			cref_ind = cref[cref_ind >> (16 - planes)];			
			for (j = 0; j < planes; j++)
			{
				d[j] |= (cref_ind & 1) << bit;
				cref_ind >>= 1;
			}
			bit--;
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
read_gem_line( struct file *fp, unsigned char *line, unsigned long scan, int patlen, unsigned long *vrep, bool allow_vrep, bool d)
{
	unsigned char *l = line, *end = line + scan + 1;

// 	if (d) display("read gem line = %lx, scan = %ld, patlen = %d, vrep = %ld",
// 		line, scan, patlen, *vrep);

	while (scan)
	{
		unsigned short b1 = fgetc(fp);
// 		if (d) display("read %d", b1);

		if ( b1 == 0x80 )
		/* Literal run */
		{
			unsigned short len = fgetc(fp);
			scan -= len;
// 			if (d) display("litteral %d bytes (scan = %ld)", len, scan);
			while ( len-- > 0 )
			{
				if (line > end || line < l)
				{
					display("ERROR0 line=%lx, end=%lx", line, end);
					return 0;
				}
				*line++ = fgetc(fp);
			}
		}
		else if ( b1 == 0x00 )
		/* Pattern code */
		{
			unsigned short rep = fgetc(fp);
// 			if (d) display("pattern rep %d", rep);
			if ( rep == 0 && allow_vrep )
			/* SCANREPEAT Is actually a vertical replication */
			{
				if (fgetc(fp) != 0xff)
				{
					display("ERROR1 wrong scanrep");
					return 0;
				}
				*vrep = (unsigned long)fgetc(fp) - 1;
			}
			else /* PATTERN REPEAT */
			{
				int i;
				
// 				if (d) display("repeat pat_len %d %d times (%d bytes) scan %ld", patlen, rep, patlen * rep, scan);
				scan -= patlen * rep;
// 				if (d) display("scan %ld", scan);
				for ( i = 0; i < patlen; i++ )
				{
					if (line > end || line < l)
					{
						display("ERROR1 line=%lx, end=%lx", line, end);
						return 0;
					}
					*line++ = fgetc(fp);
				}
				while ( rep > 1 )
				{
					for ( i = 0; i < patlen; i++, line++ )
					{
						*line = line[-patlen];
						if (line > end || line < l)
						{
							display("ERROR2 line=%lx, end=%lx", line, end);
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
			unsigned short am;

			am = b1 & 0x7f;
// 			if (d) display("store %d bytes with %d val", am, store);
			if (line + am > end || line < l)
			{
				display("ERROR3 line=%lx, end=%lx", line, end);
				return 0;
			}
			scan -= am;
			while (am)
				*line++ = store, am--;
		}
// 		if (d) display("scan = %ld", scan);
		allow_vrep = false;
	}
// 	if (d) display(" -- scan = %ld line %lx, wrote %ld bytes, vrep = %ld",
// 		scan, line, line - l, *vrep);

	return 1;
}

static long
gem_rdata(struct file *fp, XA_XIMG_HEAD *pic)
{
	unsigned long scan = (pic->ximg.img_w + 7) / 8;
	unsigned long wscan = ((pic->ximg.img_w + 15) >> 4) << 1;
	long stride = ((pic->ximg.img_w * pic->ximg.planes + 31) / 32) * 4;
	unsigned char /**line,*/ *data, *s, *e;

// 	if (!(line = kmalloc(scan)))
// 	{
// 		return -1;
// 	}

	data = pic->addr;

	s = data;
	e = data + (pic->ximg.img_h * stride);
// 	display("scan %ld, wscan %ld, stride %ld", scan, wscan, stride);

// 	display("size of data should be %ld bytes", (pic->ximg.img_h * stride));

	switch (pic->ximg.planes)
	{
		case 1:
		{
			int y;
			unsigned long vrep = 0L;
			char *dst;
			
			for (y = 0; y < pic->ximg.img_h; y++)
			{
				dst = data + (y * wscan);
				if (vrep)
				{
					memcpy(dst - wscan, dst, wscan);
					vrep--;
				}
				else
					read_gem_line(fp, dst, scan, pic->ximg.pat_len, &vrep, true, false);
			}
			break;
		}
		case 2 ... 8:
		{
			int i, y;
			unsigned long vrep = 0L;
			unsigned char *dst = data;

			for (y = 0; y < pic->ximg.img_h; y++)
			{
				for (i = 0; i < pic->ximg.planes; i++)
				{
					dst = data + (y + (i * pic->ximg.img_h)) * wscan;
					if (vrep)
					{
						if (dst < s || dst + stride > e)
						{
							display("Er0");
							return -1;
						}
						memcpy(dst - wscan, dst, wscan);
						vrep--;
					}
					else
					{
						if (!(read_gem_line(fp, dst, scan, pic->ximg.pat_len, &vrep, true, false)))
							return -1;
					}
				}
			}
			break;
		}
		case 24:
		/* 24bpp data is strange in that it isn't bit planed at all like
		   the others are. This makes decoding quicker, and most 24bpp
		   hardware is not bit planar, and this may explain why.
		   Of course, my guesswork says that the order is R,G,B and as
		   GBM used B,G,R, i'll have to reverse the order. */
		{
			long y, sl;
			unsigned long vrep = 0L;
			unsigned char *dst;
// 			ndisplay("24b img: ");
			sl = (long)((pic->ximg.img_w + 15) & ~15) * 3;
			
// 			display("stride %ld, sl %ld, scan %ld, wscan %ld", stride, sl, scan, wscan);

			for (y = 0, dst = data; y < pic->ximg.img_h; y++, dst += sl)
			{
				if (vrep)
				{
					memcpy(dst - sl, data, sl);
					vrep--;
				}
				else
				{
					if (!(read_gem_line(fp, dst, sl, pic->ximg.pat_len, &vrep, true, true)))
					{
						display("24b err0");
						return -1;
					}
				}
			}
// 			display("start = %lx, end %lx(%lx) size = %ld(%ld)",
// 				s, dst, dst + sl, dst - s, (dst+sl) - s);
		}
		break;
	}

// 	kfree(line);

	return 0L;
}

/* Loads & depacks IMG (0 if succeded, else error). */
/* Bitplanes are one after another in address IMG_HEADER.addr. */
void
depack_img(char *name, XA_XIMG_HEAD *pic)
{
	int width, word_aligned, pal_size;
	long size, err;
	struct file *fp = NULL;

	pic->addr = NULL;
	pic->palette = NULL;

	fp = kernel_open(name, O_RDONLY, &err, NULL);

	if (fp)
	{
		/* read header info (bw & ximg) into image structure */
		if ((kernel_read(fp, (char *)&(pic->ximg.version), sizeof(XIMG_header)) != sizeof(XIMG_header)))
		{
			goto end_depack;
		}

		if (pic->ximg.length > 7 && pic->ximg.planes >= 1 && pic->ximg.planes < 33 && pic->ximg.img_w > 0 && pic->ximg.img_h > 0)
		{
			word_aligned = (pic->ximg.img_w + 15) >> 4;
			word_aligned <<= 1;
			
			/* width byte aligned */
			width = (pic->ximg.img_w + 7) >> 3;
			size = (long)((long)word_aligned * pic->ximg.img_h * pic->ximg.planes);

// 			display("depack_img: size = %ld, width=%d", size, width);

			/* if XIMG, read info */
			if (pic->ximg.planes <= 8 && pic->ximg.length >= ((sizeof(XIMG_header) >> 1) + (3 * (1 << (pic->ximg.planes)))) &&
			    pic->ximg.magic == XIMG && pic->ximg.paltype == 0)
			{
				pal_size = (1 << pic->ximg.planes) * 3 * 2;
				if((pic->palette = kmalloc(pal_size)))
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

			kernel_lseek(fp, 2L * pic->ximg.length, 0);

			if (gem_rdata(fp, pic) == -1L)
				goto end_depack;
			
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
static struct TrNfo from1_8[] =
{
	{0,0,0},
	{8 + 7, 0, 1},		/* 1 */
	{8 + 6, 1, 3},		/* 2 */
	{8 + 5, 3, 7},		/* 3 */
	{8 + 4, 7, 15},		/* 4 */
	{8 + 3, 15, 31},	/* 5 */
	{8 + 2, 31, 63},	/* 6 */
	{8 + 1, 63, 127},	/* 7 */
	{8 + 0, 127, 255},	/* 8 */
};
	
static void
from8b(void (*to)(struct rgb_1000 *, void **), struct rgb_1000 *pal, MFDB *src, MFDB *dst)
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

// 	display("psize = %d, src_ptr = %lx, d_ptr = %lx",
// 		psize, src_ptr, *d_ptr);

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
				(*to)(&gp, d_ptr);
			}
			else
			{
				if (src->fd_r1 && palidx < 15)
					palidx = devtovdi8[palidx];
					
				col = pal + palidx;
				(*to)(col, d_ptr);
			}
		}
	}
// 	display("dst_ptr start = %lx, dst_ptr end = %lx (size = %ld)",
// 		dst->fd_addr, *d_ptr, *(long*)d_ptr - (long)dst->fd_addr);
}
static void
from24b(void (*to)(struct rgb_1000 *, void **), struct rgb_1000 *pal, MFDB *src, MFDB *dst)
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

			(*to)(&ppal, d_ptr);
		}
	}
// 	display("dst_ptr start = %lx, dst_ptr end = %lx (size = %ld)",
// 		dst->fd_addr, *d_ptr, *(long*)d_ptr - (long)dst->fd_addr);
}
static void
from16b(void (*to)(struct rgb_1000 *, void **), struct rgb_1000 *pal, MFDB *src, MFDB *dst)
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

			(*to)(&ppal, d_ptr);
		}
	}
// 	display("dst_ptr start = %lx, dst_ptr end = %lx (size = %ld)",
// 		dst->fd_addr, *d_ptr, *(long*)d_ptr - (long)dst->fd_addr);
}

typedef void to_x_bit(struct rgb_1000 *pal, void **imgdata);
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

void
load_image(char *name, MFDB *mimg)
{
	XA_XIMG_HEAD xa_img;
	struct ximg_header *ximg = &xa_img.ximg;
	long bmsize;

	display("load_img: '%s'", name);

// 	display("load_img: file %s", name);

	ndisplay("  depacking...");
	depack_img(name, &xa_img);
	mimg->fd_addr = NULL;

	if (xa_img.addr)
	{
		MFDB msrc;
		
// 		display("version %d\r\n hlen    %d\r\n planes  %d\r\n pat_len %d\r\n pix_w   %d\r\n pix_h   %d\r\n img_w   %d\r\n img_h   %d\r\n magic   %lx\r\n paltype %d",
// 			ximg->version, ximg->length, ximg->planes, ximg->pat_len, ximg->pix_w, ximg->pix_h, ximg->img_w, ximg->img_h,
// 			ximg->magic, ximg->paltype);
		ndisplay("OK!");
		msrc.fd_addr	= xa_img.addr;
		msrc.fd_w	= ximg->img_w;
		msrc.fd_h	= ximg->img_h;
		msrc.fd_wdwidth = (ximg->img_w + 15) >> 4;
		msrc.fd_stand	= 1;
		msrc.fd_nplanes = ximg->planes;
		msrc.fd_r1	= msrc.fd_r2 = msrc.fd_r3 = 0;

		if (xa_img.palette)
		{

			ndisplay(", has palette");
			if (ximg->planes <= 8 && screen.planes == 8)
			{
				unsigned char cref[256];
				ndisplay(",remap bitmap...");
				build_pal_xref((struct rgb_1000 *)xa_img.palette, screen.palette, (unsigned char *)&cref, (1 << msrc.fd_nplanes));
				remap_bitmap_colindexes(&msrc, (unsigned char *)&cref);
				ndisplay("OK!");
			}
		}
		else
			ndisplay(", no palette");
		
		if (ximg->planes < 8 && screen.planes == 8)
		{
			long newsize, oldsize;
			char *newdata;

			newsize = (long)(((msrc.fd_w + 15) >> 4) << 1) * msrc.fd_h * 8;
			oldsize = (long)(((msrc.fd_w + 15) >> 4) << 1) * msrc.fd_h * msrc.fd_nplanes;
			display("oldsize = %ld, newsize = %ld", oldsize, newsize);
			if ((newdata = kmalloc(newsize)))
			{
				bzero(newdata, newsize);
				memcpy(newdata, msrc.fd_addr, oldsize);
				kfree(msrc.fd_addr);
				msrc.fd_addr = xa_img.addr = newdata;
				msrc.fd_nplanes = 8;
			}
			else
			{
				kfree(xa_img.addr);
				mimg->fd_addr = NULL;
				return;
			}
		}

		*mimg = msrc;
		mimg->fd_nplanes = screen.planes;
		bmsize = (long)((long)mimg->fd_wdwidth * (mimg->fd_nplanes == 15 ? 16 : mimg->fd_nplanes) * mimg->fd_h) << 1;

		if ((mimg->fd_addr = kmalloc(bmsize)))
		{
// 			display("alloc %ld bytes at %lx for bitmap", bmsize, mimg->fd_addr);
// 			display(" transform into %lx", mimg->fd_addr);
			if (screen.planes > 8)
			{
				bool fail = true;
				void (*to)(struct rgb_1000 *pal, void **imgdata);
				void (*from)(void (*)(struct rgb_1000 *, void **), struct rgb_1000 *pal, MFDB *src, MFDB *dst);

				(long)to = -1L;
				(long)from = -1L;			
				
				if (screen.pixel_fmt >= 0)
				{
					switch (screen.planes)
					{
						case 15: to = f_to15[screen.pixel_fmt]; break;
						case 16: to = f_to16[screen.pixel_fmt]; break;
						case 24: to = f_to24[screen.pixel_fmt]; break; //toI24b; break;
						case 32: to = f_to32[screen.pixel_fmt]; break; //toM32b; break;
						default: to = NULL; break;
					}
					
					switch (msrc.fd_nplanes)
					{
						case 1 ... 8: from = from8b;  break;
// 						case 15: from = from15b; break;
 						case 16: from = from16b; break;
						case 24: from = from24b; break;
						default: from = NULL;    break;
					}
					if (from && to)
					{
						ndisplay(", tranform %d -> %d bpp...", msrc.fd_nplanes, screen.planes);
						(*from)(to, (struct rgb_1000 *)xa_img.palette, &msrc, mimg);
						ndisplay("OK!");
						fail = false;
					}
				}
				if (fail)
				{
					if (!from)
						display("\r\n  cannot handle %d bpp images!", msrc.fd_nplanes);
					if (!to)
						display("\r\n  cannot handle %d bpp screen modes!", screen.planes);
					if (screen.pixel_fmt < 0)
						display("\r\n unknown pixel format - cannot transform");
					kfree(mimg->fd_addr);
					mimg->fd_addr = NULL;
				}
			}
			else
			{
				ndisplay(", vr_trnfm()..."); 
				vr_trnfm(C.P_handle, &msrc, mimg);
				ndisplay("OK!");
			}
			kfree(xa_img.addr);	
		}
		else
		{
			ndisplay(", inline vr_trnfm() at %lx...", msrc.fd_addr);
			mimg->fd_addr = msrc.fd_addr;
			vr_trnfm(C.P_handle, &msrc, mimg);
			ndisplay("OK!");
		}
		mimg->fd_stand = 0;
	
		if (xa_img.palette)
			kfree(xa_img.palette);
		
		display("");
// 		display("  %s!", name, mimg->fd_addr ?"OK":"NotOK");
	}
 	else
		display("no image loaded");

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
				build_pal_xref(src_pal, sys_pal, (unsigned char *)&cref, (1 << src->fd_nplanes));
				remap_bitmap_colindexes(src, (unsigned char *)&cref);
				yield();
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
// 			display("copy %d words from %lx to %lx", msrc.fd_wdwidth * msrc.fd_h, s, d);
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
// 			bool fail = true;
			void (*to)(struct rgb_1000 *, void **);
			void (*from)(void (*)(struct rgb_1000 *, void **), struct rgb_1000 *, MFDB *, MFDB *);

			(long)to = -1L;
			(long)from = -1L;			
#if 1			
			if (src->fd_nplanes <= 8 && src_pal && sys_pal)
			{
				unsigned char cref[256];
				build_pal_xref(src_pal, sys_pal, (unsigned char *)&cref, (1 << src->fd_nplanes));
				remap_bitmap_colindexes(src, (unsigned char *)&cref);
// 				yield();
			}
#endif			
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
// 					display("tranform %d -> %d bpp...", src->fd_nplanes, dst->fd_nplanes);
					(*from)(to, src_pal ? src_pal : sys_pal, src, dst);
// 					ndisplay("OK!");
					src->fd_r1 = 0;
					ret = true; //fail = false;
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
bool
transform_gem_bitmap(short vdih, MFDB msrc, MFDB mdest, short planes, struct rgb_1000 *src_pal, struct rgb_1000 *sys_pal)
{
	short src_planes, dst_planes;

	src_planes = planes; //msrc.fd_nplanes;
	dst_planes = mdest.fd_nplanes;

	if (src_planes <= 8 && dst_planes <= 8)
	{
		if (src_planes == 8 && dst_planes == 8 && src_pal && sys_pal)
		{
			unsigned char cref[256];
			build_pal_xref(src_pal, sys_pal, (unsigned char *)&cref, 256);
			remap_bitmap_colindexes(&msrc, (unsigned char *)&cref);
			yield();
		}
		if ((src_planes | dst_planes) != 1)
			vr_trnfm(vdih, &msrc, &mdest);
		else
		{
			short i, *s, *d;
			s = msrc.fd_addr;
			d = mdest.fd_addr;
// 			display("copy %d words from %lx to %lx", msrc.fd_wdwidth * msrc.fd_h, s, d);
			for (i = msrc.fd_wdwidth * msrc.fd_h; i > 0; i--)
				*d++ = *s++;
		}
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

void
set_syspalette(short vdih, struct rgb_1000 *palette)
{
	int i, pens;
	
	if (screen.planes > 8)
		pens = 256;
	else
		pens = 1 << screen.planes;

// 	display("set syspal - %d pens", pens);
	for (i = 0; i < pens; i++)
	{
// 		display("%03d - %04d, %04d, %04d", i, palette[i].red, palette[i].green, palette[i].blue);
		vs_color(vdih, i, (short *)&(palette[i]));
	}
}

void
get_syspalette(short vdih, struct rgb_1000 *palette)
{
	int i;
	short pens, ind;
	short rgb[3];
	
	switch (screen.planes)
	{
		case 1:	pens =  2; break;
		case 2: pens =  4; break;
		case 4: pens = 16; break;
		default: pens = 256; break;
	}
// 	display("get syspal - %d pens", pens);
	for (i = 0; i < pens; i++)
	{
		ind = vq_color(vdih, i, 0, rgb);
		if (ind >= 0)
		{
// 			display("idx %03d(%03d), %04d, %04d, %04d", i, ind, rgb[0], rgb[1], rgb[2]);
			palette[i] = *(struct rgb_1000 *)&rgb;
		}
	}
}
void
set_defaultpalette(short vdih)
{
	set_syspalette(vdih, (struct rgb_1000 *)systempalette);
}

struct color_tab256
{
	COLOR_TAB	tab;
	COLOR_ENTRY	colors[256];
};
typedef struct color_tab256 COLOR_TAB256;

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
/*
 * Ozk: Attempt to detect pixel format in 15 bpp and above
 */
short
detect_pixel_format(struct xa_vdi_settings *v)
{
	short ret = -1;

	if (screen.planes > 8)
	{
		MFDB scr, dst;
		struct rgb_1000 srgb, rgb;
		short pxy[8];
		unsigned long b[32];

		(*v->api->wr_mode)(v, MD_REPLACE);
		(*v->api->l_type)(v, 1);
		(*v->api->l_ends)(v, 0, 0);
		(*v->api->l_width)(v, 1);

		vq_color(v->handle, 0, 1, (short *)&srgb);
// 		display("saved %04d, %04d, %04d", srgb.red, srgb.green, srgb.blue);
		rgb.red = 1000;
		rgb.green = 1000;
		rgb.blue = 0;
		vs_color(v->handle, 0, (short *)&rgb);
		(*v->api->line)(v, 0, 0, 0, 0, 0);
	
	
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

		vs_color(v->handle, 0, (short *)&srgb);

		switch (screen.planes)
		{
	/* pixelformat E07F */

						/* 12345678.12345678 */			
			case 15:
			{
				unsigned short pix = *(unsigned short *)(&b[0]);
				ndisplay("%d bit pixel %x", screen.planes, pix);
				if (pix == ((31 << 2) | (7 << 13) | 3))		/* gggbbbbb.0rrrrrgg */
				{
					ret = 1;
					display(" is intel format");
				}
				else if (pix == ((31 << 11) | (31 << 6))) 	/* rrrrrggg.ggobbbbb */
				{
					ret = 0;
					display(" is moto format");
				}
				else
				{
					ret = -1;
					display(" unknown format");
				}
				break;
			}
			case 16:
			{
				unsigned short pix = *(unsigned short *)(&b[0]);
				ndisplay("%d bit pixel %x", screen.planes, pix);
				if (pix == ((31 << 3) | (7 << 13) | 7))		/* gggbbbbb.rrrrrggg */
				{
					ret = 1;
					display(" is intel format");
				}
				else if (pix == ((31 << 11) | (63 << 5)))	/* rrrrrggg.gggbbbbb */
				{
					ret = 0;
					display(" is moto format");
				}
				else if (pix == ((31 << 11) | (31 << 6)))	/* rrrrrggg.ggobbbbb */
				{
					ret = 2;
					display(" is falcon (motorola) 15 bit");
				}
				else if (pix == ((31 << 2) | (7 << 13) | 3))	/* gggbbbbb.0rrrrrgg */
				{
					ret = 3;
					display(" is a intel 15 bit");
				}
				else
				{
					ret = -1;
					display(" unknown format!");
				}
				break;
			}
			case 24:
			{
				unsigned long pix = b[0];
				ndisplay("%d bit pixel %lx", screen.planes, pix);
				pix >>= 8;
				if (pix == 0xffff00L)			/* rrrrrrrr.gggggggg.bbbbbbbb  Moto */
				{
					ret = 0;
					display(" is moto format");
				}
				else if (pix == 0xffffL)		/* bbbbbbbb.gggggggg.rrrrrrrr */
				{
					ret = 1;
					display(" is intel format");
				}
				else
				{
					ret = -1;
					display(" unknown format!");
				}
				break;
			}
			case 32:
			{
				unsigned long pix = b[0];
				ndisplay("%d bit pixel %lx", screen.planes, pix);
				if (pix == 0xffff00L)			/* 00000000.rrrrrrrr.gggggggg.bbbbbbbb */
				{
					ret = 0;
					display(" is moto format");
				}
				else if (pix == 0xffff0000L)		/* rrrrrrrr.gggggggg.bbbbbbbb.00000000 */
				{
					ret = 1;
					display(" is intel format");
				}
				else if (pix == 0x0000ffffL)		/* 00000000.bbbbbbbb.gggggggg.rrrrrrrr */
				{
					ret = 2;
					display(" is intel byteswapped format");
				}
				else
				{
					ret = -1;
					display(" unknown format!");
				}
				break;
			}
			default:
			{
				display("unsupported color depth!");
				ret = -1;
				break;
			}
		}
	}
	return ret;
}
