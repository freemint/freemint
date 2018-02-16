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

#ifndef _mvdi_h
#define _mvdi_h

#define MI_MAGIC 0x4D49

#define CMD_GETMODE  0
#define CMD_SETMODE  1
#define CMD_GETINFO  2
#define CMD_ALLOCPAGE 3
#define CMD_FREEPAGE 4
#define CMD_FLIPPAGE 5
#define CMD_ALLOCMEM 6
#define CMD_FREEMEM  7
#define CMD_SETADR  8
#define CMD_ENUMMODES  9
#define  ENUMMODE_EXIT 0
#define  ENUMMODE_CONT 1

#define BLK_ERR 0
#define BLK_OK 1
#define BLK_CLEARED 2

typedef struct _scrblk
{
	long size;		/* size of strukture  */
	long blk_status;	/* status bits of blk */
	long blk_start;		/* Start Adress   */
	long blk_len;		/* length of memblk  */
	long blk_x;		/* x pos in total screen*/
	long blk_y;		/* y pos in total screen */
	long blk_w;		/* width     */
	long blk_h;		/* height    */
	long blk_wrap;		/* width in bytes  */
} SCRMEMBLK;

/* scrFlags */
#define SCRINFO_OK 1

/* scrClut */
#define NO_CLUT 0
#define HARD_CLUT 1
#define SOFT_CLUT 2

/* scrFormat */
#define INTERLEAVE_PLANES 0
#define STANDARD_PLANES  1
#define PACKEDPIX_PLANES 2

/* bitFlags */
#define STANDARD_BITS  1
#define FALCON_BITS   2
#define INTEL_BITS   8

typedef struct screeninfo
{
	long size;		/* Size of structur   */
	long devID;		/* device id number   */
	char name[64];		/* Friendly name of Screen */
	long scrFlags;		/* some Flags    */
	long frameadr;		/* Adress of framebuffer */
	long scrHeight;		/* visible X res   */
	long scrWidth;		/* visible Y res   */
	long virtHeight;	/* virtual X res   */
	long virtWidth;		/* virtual Y res   */
	long scrPlanes;		/* color Planes    */
	long scrColors;		/* # of colors    */
	long lineWrap;		/* # of Bytes to next line  */
	long planeWarp;		/* # of Bytes to next plane */
	long scrFormat;		/* screen Format    */
	long scrClut;		/* type of clut    */
	long redBits;		/* Mask of Red Bits   */
	long greenBits;		/* Mask of Green Bits  */
	long blueBits;		/* Mask of Blue Bits  */
	long alphaBits;		/* Mask of Alpha Bits  */
	long genlockBits;	/* Mask of Genlock Bits  */
	long unusedBits;	/* Mask of unused Bits  */
	long bitFlags;		/* Bits organisation flags */
	long maxmem;		/* max. memory in this mode */
	long pagemem;		/* needed memory for one page */
	long max_x;		/* max. possible width   */
	long max_y;		/* max. possible heigth  */
}SCREENINFO;

/*
 * size of this struct must be 106 bytes
*/
struct videodef
{
	char name[64];
	short devid;
	short planes;
	short res_x;
	short res_y;
	short vres_x;
	short vres_y;
	char dontknow[106 - 76];
#if 0
	char name[64];
	short devid;
	short planes;
	char dontknow3[106 - 68];
#endif
};


/*
 * mVDI Driver Dispatcher Values
 *
 * These are 'dispather ID' values which are loaded into the
 * low word of D0 to tell which function 'area' we want. The
 * function number within a 'function area' is found in the
 * high word of D0. What a mess!
 */
#define DRIVER_ACCEL	1	/* graphic accelerator */
#define DRIVER_DEVICE	2	/* graphic device routines */
#define DRIVER_BIOS	3	/* Bios/XBios routines */

/* mVDI Device Dispatcher Values  */
/* These are put into the high word of CPU reg D0 when
 * calling out the dispatcher. The low word of D0 contains
 * the "dispather ID"
 */
#define DEVICE_GETDEVICELIST	0001
#define DEVICE_GETDEVICE	0002
#define DEVICE_SETDEVICE	0003
struct cookie_mvdi
{
	short	version;
	long	subcookie;
	unsigned long flags;		/* +06 -- flags */
	void	(*dispatch)(void);	/* +10 -- dispatcher */
};

long mvdi_device(long d1, long a0, short cmd, long *ret);
short vcheckmode(short mode);
short vsetmode(short mode);
void mSetscreen(unsigned long p1, unsigned long p2, short cmd);


#endif	/* _mvdi_h */
