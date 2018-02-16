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

#ifndef _trnfm_h
#define _trnfm_h

#include "global.h"
#include "xa_types.h"

void create_gradient(XAMFDB *pm, struct rgb_1000 *c, short method, short n_steps, short *steps, short w, short h );

//void depack_img(char *name, XA_XIMG_HEAD *pic);
void load_image(char *name, XAMFDB *mimg);

//void remap_bitmap_colindexes(MFDB *map, unsigned char *cref);
//void build_pal_xref(struct rgb_1000 *src_palette, struct rgb_1000 *dst_palette, unsigned char *cref, int pens);

bool transform_bitmap(short vdih, MFDB *src, MFDB *dst, struct rgb_1000 *src_pal, struct rgb_1000 *sys_pal);
bool transform_gem_bitmap_data(short vdih, MFDB msrc, MFDB mdest, int src_planes, int dst_planes);
bool transform_gem_bitmap(short vdih, MFDB msrc, MFDB mdest, short planes, struct rgb_1000 *src_pal, struct rgb_1000 *sys_pal);
void fix_rsc_palette(struct xa_rsc_rgb *palette);
int rw_syspalette( int md, struct rgb_1000 *palette, char *path, char *f );
void set_syspalette(short vdih, struct rgb_1000 *palette);
void get_syspalette(short vdih, struct rgb_1000 *palette);
void set_defaultpalette(short vdih);
void set_syscolor(void);
short detect_pixel_format(struct xa_vdi_settings *v);

#endif /* _trnfm_h */
