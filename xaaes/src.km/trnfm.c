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
#if 0
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

static short devtovdi4[] = { 0,2  ,3,6,4,7,5,8,9,10,11,14,12,15,13,1  };
static short vditodev4[] = { 0,255,1,2,4,6,3,5,7, 8, 9,10,12,14,11,13 };

static short devtovdi2[] = { 0,2,3,1 };

static short devtovdi1[] = { 0,1 };

static long
get_coldist(struct rgb_1000 *src, struct rgb_1000 *dst)
{
	long r, g, b;

	r = (long)dst->red - src->red;
	r = r * r;

	g = (long)dst->green - src->green;
	g = g * g;
	r += g;

	b = (long)dst->blue - src->blue;
	b = b * b;
	r += b;

	return r;
}
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
	long closest, this;
	int i, j, c;
	
	/*
	 * Normally, We do not remap any referances to pens 0 - 15
	 */
	for (i = 0; i < pens && i < 16; i++)
		cref[i] = i;

	for (i = 16, src = src_palette + 16; i < pens; i++, src++)
	{
		dst = dst_palette;
		s = src;

		if (!(closest = get_coldist(src, dst)))
		{
			cref[i] = 0;
		}
		else
		{
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
				else if (closest > this)
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
		}
	}
}
/*
 * Remap the bitmap palette referances.
 */
void
remap_bitmap_colindexes(MFDB *map, unsigned char *cref)
{
	int planes, psize;
	int i, j, k;
	unsigned short *data, *d_ptr, plane;
	unsigned short d[8];
	unsigned short cref_ind;

	planes = map->fd_nplanes;
	psize = map->fd_wdwidth * map->fd_h;
	data = map->fd_addr;
	
	DIAGS(("remap_bitmap_colindex: planes = %d, psize = %d, data = %lx",
		planes, psize, data));

	for (k = 0; k < psize; k++, data++)
	{
		for (i = 0; i < 8; i++)
			d[i] = 0;

		for (i = 0; i < 16; i++)
		{
			cref_ind = 0;
			d_ptr = data;

			for (j = 0; j < planes; j++)
			{
				plane = (*d_ptr >> i) & 1;
				cref_ind |= plane << j;
				d_ptr += psize;
			}

			cref_ind = cref[cref_ind];
			
			for (j = 0; j < planes; j++)
			{
				d[j] |= (cref_ind & 1) << i;
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

/* Loads & depacks IMG (0 if succeded, else error). */
/* Bitplanes are one after another in address IMG_HEADER.addr. */
void
depack_img(char *name, XA_XIMG_HEAD *pic)
{
	unsigned char opcode, patt_repeat, scan_repeat, byte_repeat;
	int b, line, plane, width, word_aligned, patt_len, pal_size;
	char *pattern, *to, *endline, *puffer, sol_pat;
	long size, err;
	struct file *fp = NULL;

	pic->addr = NULL;
	pic->palette = NULL;

	fp = kernel_open(name, O_RDONLY, &err, NULL);

	if (fp)// = kernel_open(name, O_RDONLY, &err, NULL)))
	{
		/* read header info (bw & ximg) into image structure */
		if ((kernel_read(fp, (char *)&(pic->ximg.version), sizeof(XIMG_header)) != sizeof(XIMG_header)))
		{
			goto end_depack;
		}

		/* only 2-256 color imgs */
		if(pic->ximg.planes < 1 || pic->ximg.planes > 32)
		{
			goto end_depack;
		}

		/* if XIMG, read info */
		if(pic->ximg.magic == XIMG && pic->ximg.paltype == 0)
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

		/* width in bytes word aliged */
		word_aligned = (pic->ximg.img_w + 15) >> 4;
		word_aligned <<= 1;

		/* width byte aligned */
		width = (pic->ximg.img_w + 7) >> 3;

		size = (long)((long)word_aligned * (long)pic->ximg.img_h * (long)pic->ximg.planes);
		
		display("depack_img: size = %ld", size);
		
		/* check for header validity & malloc long... */
		if (pic->ximg.length > 7 && pic->ximg.planes < 33 && pic->ximg.img_w > 0 && pic->ximg.img_h > 0)
		{
			pic->addr = kmalloc(size);
		}
		
		if (!pic->addr)
		{
			goto end_depack;
		}
			
		patt_len = pic->ximg.pat_len;

		/* jump over the header and possible (XIMG) info */
		kernel_lseek(fp, pic->ximg.length * 2L, 0); //(fp, (long) pic->length * 2L, SEEK_SET);
		to = endline = pic->addr;
		for (line = 0, to = pic->addr; line < pic->ximg.img_h; line += (volatile unsigned char)scan_repeat)
		{	/* depack whole img */
			for (plane = 0, scan_repeat = 1; plane < pic->ximg.planes; plane++)
			{	/* depack one scan line */
				puffer = to = pic->addr + (long)(line + ((long)plane * pic->ximg.img_h)) * (long)word_aligned;
				endline = puffer + width;
				do
				{	/* depack one line in one bitplane */
					//kernel_read(fp, &opcode, 1);
					kernel_read(fp, &opcode, 1);
					switch ((volatile unsigned char)opcode)
					{
					case 0:	/* pattern or scan repeat */
					{
						//kernel_read(fp, &patt_repeat, 1);
						kernel_read(fp, &patt_repeat, 1);
						
						if ((volatile unsigned char)patt_repeat)
						{	/* repeat a pattern */
							kernel_read(fp, to, patt_len);
							pattern = to;
							to += patt_len;
							while (--patt_repeat)
							{	/* copy pattern */
								memcpy(to, pattern, patt_len);
								to += patt_len;
							}
						}
						else
						{
							kernel_read(fp, &scan_repeat, 1);
							/* repeat a line */
							if ( (volatile unsigned char)scan_repeat == 0xFF)
							{
								kernel_read(fp, &scan_repeat, 1);
							}
							else
							{
								goto end_depack;
							}
						}
						break;
					}
					case 0x80:	/* Literal */
					{
						kernel_read(fp, &byte_repeat, 1);
						kernel_read(fp, to, (unsigned long)((volatile unsigned char)byte_repeat));
						to += byte_repeat;
						break;
					}
					default:	/* Solid run */
					{
						byte_repeat = opcode & 0x7F;
						sol_pat = opcode & 0x80 ? 0xFF : 0x00;
						while (byte_repeat--)	*to++ = sol_pat;
					}
					}
				} while (to < endline);

				if (to == endline)
				{
					/* ensure that lines aren't repeated past the end of the img */
					if (line + (volatile unsigned char)scan_repeat > pic->ximg.img_h)
						scan_repeat = pic->ximg.img_h - line;
		
					/* copy line to image buffer */
					if (scan_repeat > 1)
					{
						/* calculate address of a current line in a current bitplane */
/*						to=pic->addr+(long)(line+1+plane*pic->img_h)*(long)word_aligned;*/
						for (b = scan_repeat - 1; b; --b)
						{
							memcpy(to, puffer, width);
							to += word_aligned;
						}
					}
				}
				else
				{
					goto end_depack;
				}
			}
		}
		display(" -.- size = %ld", (long)endline - (long)pic->addr);
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

void
load_image(char *name, MFDB *mimg)
{
	XA_XIMG_HEAD xa_img;
	struct ximg_header *ximg = &xa_img.ximg;
	long bmsize;

	display("load_img: file %s");
	depack_img(name, &xa_img);

	mimg->fd_addr = NULL;

	if (xa_img.addr)
	{
		MFDB msrc;
		
		display("version %d\r\n hlen    %d\r\n planes  %d\r\n pat_len %d\r\n pix_w   %d\r\n pix_h   %d\r\n img_w   %d\r\n img_h   %d\r\n magic   %lx\r\n paltype %d",
			ximg->version, ximg->length, ximg->planes, ximg->pat_len, ximg->pix_w, ximg->pix_h, ximg->img_w, ximg->img_h,
			ximg->magic, ximg->paltype);

		msrc.fd_addr	= xa_img.addr;
		msrc.fd_w	= ximg->img_w;
		msrc.fd_h	= ximg->img_h;
		msrc.fd_wdwidth = (ximg->img_w + 15) >> 4;
		msrc.fd_stand	= 1;
		msrc.fd_nplanes = ximg->planes;
		msrc.fd_r1	= msrc.fd_r2 = msrc.fd_r3 = 0;

		if (xa_img.palette)
		{
			display("img got palette");
			if (ximg->planes == 8 && screen.planes == 8)
			{
				unsigned char cref[256];
				display("remapping image bitmap...");
				build_pal_xref((struct rgb_1000 *)xa_img.palette, screen.palette, (unsigned char *)&cref, 256);
				remap_bitmap_colindexes(&msrc, (unsigned char *)&cref);
				display(".. done!");
			}
			kfree(xa_img.palette);
		}

		*mimg = msrc;
		bmsize = (long)((long)mimg->fd_wdwidth * (long)mimg->fd_nplanes * (long)mimg->fd_h) << 1;
		
		display("alloc %ld bytes for bitmap", bmsize);

		if ((mimg->fd_addr = kmalloc(bmsize)))
		{
			display(" transform into %lx", mimg->fd_addr);
			vr_trnfm(C.vh, &msrc, mimg);
			kfree(xa_img.addr);
		}
		else
		{
			display(" -- inline transformation at %lx", msrc.fd_addr);
			mimg->fd_addr = msrc.fd_addr;
			vr_trnfm(C.vh, &msrc, mimg);
		}
		mimg->fd_stand = 0;
	}
	else
		display("no image loaded");
}

static long
map_size(MFDB *m, int i)
{
	long l = m->fd_wdwidth * 2L * m->fd_h * m->fd_nplanes;

	/* DIAGS(("[%d]map_size: wd %d, h %d, planes %d --> %ld",
	 * 	  i, m->fd_wdwidth, m->fd_h, m->fd_nplanes, l)); */

	return l;
}

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

	for (i = 0; i < pens; i++)
	{
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
	for (i = 0; i < pens; i++)
	{
		ind = vq_color(vdih, i, 0, rgb);
		if (ind >= 0)
		{
			palette[i] = *(struct rgb_1000 *)&rgb;
		}
	}
}
