/*
 * Copyright (c) 2012 David Galvez
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _XHDI_H
#define _XHDI_H

/* AHDI */

#define PUN_DEV           0x1F /* device number of HD */
#define PUN_UNIT          0x07 /* Unit number */
#define PUN_SCSI          0x08 /* 1=SCSI 0=ACSI */
#define PUN_IDE           0x10 /* Falcon IDE */
#define PUN_USB           0x20 /* USB */
#define PUN_REMOVABLE     0x40 /* Removable media */
#define PUN_VALID         0x80 /* zero if valid */

/* BIOS parameter block */

struct  bpb
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
};
typedef struct bpb BPB;

/* Extended pun struct for USB */

#ifdef TOSONLY
#define MAX_LOGICAL_DRIVE 16
#else
#define MAX_LOGICAL_DRIVE 32
#endif

struct pun_info
{
	ushort	puns;			/* Number of HD's */
	uchar	pun [MAX_LOGICAL_DRIVE];		/* AND with masks below: */
	long	partition_start [MAX_LOGICAL_DRIVE];
	long	cookie;			/* 'AHDI' if following valid */
	long	*cookie_ptr;		/* Points to 'cookie' */
	ushort	version_num;		/* AHDI version */
	ushort	max_sect_siz;		/* Max logical sec size */
	long	ptype[MAX_LOGICAL_DRIVE];
	long	psize[MAX_LOGICAL_DRIVE];
	short	flags[MAX_LOGICAL_DRIVE];		/* B15:swap, B7:change, B0:bootable */
	BPB	bpb[MAX_LOGICAL_DRIVE];
	uchar	dev_num[MAX_LOGICAL_DRIVE];

} __attribute__((packed));
typedef struct pun_info PUN_INFO;

/* XHDI opcodes */
#define XHGETVERSION    0
#define XHINQTARGET     1
#define XHRESERVE       2
#define XHLOCK          3
#define XHSTOP          4
#define XHEJECT         5
#define XHDRVMAP        6
#define XHINQDEV        7
#define XHINQDRIVER     8
#define XHNEWCOOKIE     9
#define XHREADWRITE     10
#define XHINQTARGET2    11
#define XHINQDEV2       12
#define XHDRIVERSPECIAL 13
#define XHGETCAPACITY   14
#define XHMEDIUMCHANGED 15
#define XHMINTINFO      16
#define XHDOSLIMITS     17
#define XHLASTACCESS    18
#define XHREACCESS      19

#endif /* _XHDI_H */
