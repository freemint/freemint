/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-05-02
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _xhdi_h
# define _xhdi_h

# include "mytypes.h"


/*
 * BIOS parameter block (osbind.h is buggy)
 */

typedef struct
{
	ushort	recsiz;		/* bytes per sector */
	short	clsiz;		/* sectors per cluster */
	ushort	clsizb;		/* bytes per cluster */
	short	rdlen;		/* root directory size */
	short	fsiz;		/* size of FAT */
	short	fatrec;		/* startsector of second FAT */
	short	datrec;		/* first data sector */
	ushort	numcl;		/* total number of clusters */
	short	bflags;		/* some flags */
	
} __BPB;


/*
 * Extended BIOS Parameter Block (XHDI)
 */

typedef struct
{
	ushort	recsiz;		/* bytes per sector */
	short	clsiz;		/* sectors per cluster */
	ushort	clsizb;		/* bytes per cluster */
	short	rdlen;		/* root directory size or 0 if FAT32 */
	short	fsiz;		/* size of FAT or 0 if FAT32 */
	short	fatrec;		/* startsector of second FAT or 0 if FAT32 */
	short	datrec;		/* first data sector or 0 if FAT32 */
	ushort	numcl;		/* total number of clusters or 0 if FAT32 */
	short	bflags;		/* bit 0: 0 = FAT12, 1 = FAT16
				 * bit 1: 0 = 2 FATs, 1 = 1 FAT
				 * bit 2: 0 = BPB, 1 = EXTENDED_BPB
				 */
	
	/* Ab hier undokumentiert, nur A: und B:! */
	short	ntracks;	/* Anzahl Spuren */
	short	nsides;		/* Anzahl Seiten */
	short	spc;		/* Sektoren pro Zylinder */
	short	spt;		/* Sektoren pro Spur */
	ushort	nhid;		/* Anzahl versteckte Sektoren */
	uchar	ser[3];		/* Seriennummer */
	uchar	serms[4];	/* ab TOS 2.06: MS-DOS-4.0-Seriennummer */
	char	unused;
	
	/* if bit 2 of bflags are set */
	long	l_recsiz;	/* bytes per sector */
	long	l_clsiz;	/* sectors per cluster */
	long	l_clsizb;	/* bytes per cluster */
	long	l_rdlen;	/* root directory size */
	long	l_fsiz;		/* size of FAT */
	long	l_fatrec;	/* startsector of second FAT */
	long	l_datrec;	/* first data sector */
	long	l_numcl;	/* total number of clusters */
	long	l_rdstcl;	/* if FAT32: startcluster of root directory
				 * otherwise 0
				 */
} __xhdi_BPB;


# define XH_TARGET_STOPPABLE	0x00000001L
# define XH_TARGET_REMOVABLE	0x00000002L
# define XH_TARGET_LOCKABLE	0x00000004L
# define XH_TARGET_EJECTABLE	0x00000008L
# define XH_TARGET_LOCKED	0x20000000L
# define XH_TARGET_STOPPED	0x40000000L
# define XH_TARGET_RESERVED	0x80000000L

# define XH_MI_SETKERINFO	0
# define XH_MI_GETKERINFO	1

# define XH_DL_SECSIZ		0
# define XH_DL_MINFAT		1
# define XH_DL_MAXFAT		2
# define XH_DL_MINSPC		3
# define XH_DL_MAXSPC		4
# define XH_DL_CLUSTS		5
# define XH_DL_MAXSEC		6
# define XH_DL_DRIVES		7
# define XH_DL_CLSIZB		8
# define XH_DL_RDLEN		9
# define XH_DL_CLUSTS12		12
# define XH_DL_CLUSTS32		13
# define XH_DL_BFLAGS		14


extern ushort XHDI_installed;

long	init_XHDI	(void);

long	XHGetVersion	(void);
long	XHInqTarget	(ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name);
long	XHReserve	(ushort major, ushort minor, ushort do_reserve, ushort key);
long	XHLock		(ushort major, ushort minor, ushort do_lock, ushort key);
long	XHStop		(ushort major, ushort minor, ushort do_stop, ushort key);
long	XHEject		(ushort major, ushort minor, ushort do_eject, ushort key);
long	XHDrvMap	(void);
long	XHInqDev	(ushort bios, ushort *major, ushort *minor, ulong *start, __BPB *bpb);
long	XHInqDriver	(ushort bios, char *name, char *version, char *company, ushort *ahdi_version, ushort *maxIPL);
long	XHNewCookie	(void *newcookie);
long	XHReadWrite	(ushort major, ushort minor, ushort rwflag, ulong recno, ushort count, void *buf);
long	XHInqTarget2	(ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name, ushort stringlen);
long	XHInqDev2	(ushort bios, ushort *major, ushort *minor, ulong *start, __BPB *bpb, ulong *blocks, char *partid);
long	XHDriverSpecial	(ulong key1, ulong key2, ushort subopcode, void *data);
long	XHGetCapacity	(ushort major, ushort minor, ulong *blocks, ulong *bs);
long	XHMediumChanged	(ushort major, ushort minor);
long	XHMiNTInfo	(ushort op, void *data);
long	XHDOSLimits	(ushort which, ulong limit);
long	XHLastAccess	(ushort major, ushort minor, ulong *ms);
long	XHReaccess	(ushort major, ushort minor);


# endif /* _xhdi_h */
