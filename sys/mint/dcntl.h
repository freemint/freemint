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
 * This file is free software; you can redistribute it and/or modify
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
 * Started: 1998-09-07
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * Description: Constants for Dcntl() calls.
 * 
 */

# ifndef _mint_dcntl_h
# define _mint_dcntl_h


/* 64 bit support */
# ifndef LLONG
# define LLONG
typedef long long		llong;
typedef unsigned long long	ullong;
# endif


/*
 * Minix-FS
 */

# define MFS_VERIFY		0x100		/* minixfs/docs/syscall.doc */
# define  MFS_MAGIC		0x18970431	/* Magic number from MFS_VERIFY */
# define MFS_SYNC		0x101		/* minixfs/docs/syscall.doc */
# define MFS_CINVALID		0x102		/* minixfs/docs/syscall.doc */
# define MFS_FINVALID		0x103		/* minixfs/docs/syscall.doc */
# define MFS_INFO		0x104		/* minixfs/docs/syscall.doc */
# define MFS_USAGE		0x105		/* minixfs/minixfs.h */
# define MFS_IMODE		0x106		/* minixfs/docs/syscall.doc */
# define MFS_GTRANS		0x107		/* minixfs/docs/syscall.doc */
# define MFS_STRANS		0x108		/* minixfs/docs/syscall.doc */
# define MFS_PHYS		0x109		/* minixfs/minixfs.h */
# define MFS_IADDR		0x10a		/* minixfs/docs/syscall.doc */
# define MFS_UPDATE		0x10b		/* minixfs/docs/syscall.doc */
# define MFS_MOUNT		0x10c		/* minixfs/docs/syscall.doc */
# define MFS_UMOUNT		0x10d		/* minixfs/docs/syscall.doc */
# define MFS_LOPEN		0x10e		/* minixfs/docs/syscall.doc */
# define MFS_MKNOD		0x10f		/* minixfs/docs/syscall.doc */


/*
 * MagiC specific calls for filesystem installation (use group 'm'!)
 */

/*# define KER_INSTXFS		0x0200*/	/* mgx_dos.txt */
/*# define KER_SETWBACK		0x0300*/	/* mgx_dos.txt */
/*# define DFS_GETINFO		0x1100*/	/* mgx_dos.txt */
/*# define DFS_INSTDFS		0x1200*/	/* mgx_dos.txt */


/*
 * CD-ROMs
 */

# define CDROMREADOFFSET	(('C'<< 8) | 0)		/* cdromio.h */
# define CDROMPAUSE		(('C'<< 8) | 1)		/* cdromio.h */
# define CDROMRESUME		(('C'<< 8) | 2)		/* cdromio.h */
# define CDROMPLAYMSF		(('C'<< 8) | 3)		/* cdromio.h */
# define CDROMPLAYTRKIND	(('C'<< 8) | 4)		/* cdromio.h */
# define CDROMREADTOCHDR	(('C'<< 8) | 5)		/* cdromio.h */
# define CDROMREADTOCENTRY	(('C'<< 8) | 6)		/* cdromio.h */
# define CDROMSTOP		(('C'<< 8) | 7)		/* cdromio.h */
# define CDROMSTART		(('C'<< 8) | 8)		/* cdromio.h */
# define CDROMEJECT		(('C'<< 8) | 9)		/* cdromio.h */
# define CDROMVOLCTRL		(('C'<< 8) | 10)	/* cdromio.h */
# define CDROMSUBCHNL		(('C'<< 8) | 11)	/* cdromio.h */
# define CDROMREADMODE2		(('C'<< 8) | 12)	/* cdromio.h */
# define CDROMREADMODE1		(('C'<< 8) | 13)	/* cdromio.h */
# define CDROMPREVENTREMOVAL	(('C'<< 8) | 14)	/* cdromio.h */
# define CDROMALLOWTREMOVAL	(('C'<< 8) | 15)	/* cdromio.h */
# define CDROMAUDIOCTRL		(('C'<< 8) | 16)	/* cdromio.h */
# define CDROMREADDA		(('C'<< 8) | 17)	/* cdromio.h */

# define CDROMGETMCN		(('C'<< 8) | 19)	/* cdromio.h */
# define CDROMGETTISRC		(('C'<< 8) | 20)	/* cdromio.h */


/*
 * Big-DOS
 */

# define DL_SECSIZ	(('D'<< 8) | 0)		/* max. bpb->recsiz */
# define DL_MINFAT	(('D'<< 8) | 1)		/* min. Anzahl FATs */
# define DL_MAXFAT	(('D'<< 8) | 2)		/* max. Anzahl FATs */
# define DL_MINSPC	(('D'<< 8) | 3)		/* min. bpb->clsiz */
# define DL_MAXSPC	(('D'<< 8) | 4)		/* max. bpb->clsiz */
# define DL_CLUSTS	(('D'<< 8) | 5)		/* max. bpb->numcl FAT16 */
# define DL_MAXSEC	(('D'<< 8) | 6)		/* max. number of sectors */
# define DL_DRIVES	(('D'<< 8) | 7)		/* max. number of drives */
# define DL_CLSIZB	(('D'<< 8) | 8)		/* max. bpb->clsizb */
# define DL_RDLEN	(('D'<< 8) | 9)		/* max. (bpb->rdlen * bpb->recsiz / 32) */
# define DL_FSIZ	(('D'<< 8) | 10)	/* max. bpb->fsiz */
# define DL_FATREC	(('D'<< 8) | 11)	/* max. bpb->fatrec */
# define DL_CLUSTS12	(('D'<< 8) | 12)	/* max. bpb->numcl FAT12 */
# define DL_CLUSTS32	(('D'<< 8) | 13)	/* max. bpb->l_numcl FAT32 */
# define DL_BFLAGS	(('D'<< 8) | 14)	/* supported bits in bpb->bflags */
# define DL_FILESYS	(('D'<< 8) | 15)	/* cookie of the filesystem */
# define DL_VERSION	(('D'<< 8) | 16)	/* version of the filesystem */
# define DL_CHAR	(('D'<< 8) | 17)	/* pointer to a string with legal chars */
# define DL_SETCHAR	(('D'<< 8) | 18)	/* ask or set valid names */


/*
 * (V)FAT filesystem extensions (MiNT/MagiC)
 */
    
# define VFAT_CNFDFLN	(('V'<< 8) | 0)		/* MiNT/MagiC */
# define VFAT_CNFLN	(('V'<< 8) | 1)		/* MiNT/MagiC */
# define V_CNTR_SLNK	(('V'<< 8) | 2)		/* MiNT, works on FAT and VFAT */
# define V_CNTR_RES3	(('V'<< 8) | 3)		/* reserved, not used at the moment */
# define V_CNTR_MODE	(('V'<< 8) | 4)		/* MiNT, works on FAT */

/* name mode flags: */
# define GEMDOS		0
# define ISO		1
# define MSDOS		2

# define V_CNTR_FAT32	(('V'<< 8) | 10)	/* MiNT, FAT32 control */

struct control_FAT32
{
	unsigned short	mode;	/* 0 = fill out; 1 = set values */
	unsigned short	mirr;	/* FAT mirroring: 0 = enabled, otherwise active FAT */
	unsigned short	fats;	/* number of FATs */
	unsigned short	info;	/* status of additional info sector */
# define FAT32_info_exist	0x1
# define FAT32_info_active	0x2
# define FAT32_info_reset	0x4	/* in set mode: clear info sector */
};

# define V_CNTR_WP	(('V'<< 8) | 100)	/* MiNT, write protect control */
# define V_CNTR_WB	(('V'<< 8) | 101)	/* MiNT, write back control */


/*
 * MagiC opcodes (all group 'm' opcodes are reserved for MagiC)
 */

# define MX_KER_GETINFO		(('m'<< 8) | 0)		/* mgx_dos.txt */
# define MX_KER_DOSLIMITS	(('m'<< 8) | 1)		/* mgx_dos.txt */
# define MX_KER_INSTXFS		(('m'<< 8) | 2)		/* mgx_dos.txt */
# define MX_KER_DRVSTAT		(('m'<< 8) | 4)		/* mgx_dos.txt */
# define MX_KER_XFSNAME		(('m'<< 8) | 5)		/* mgx_dos.txt */
# define MX_DEV_INSTALL 	(('m'<< 8) | 0x20)	/* mgx_dos.txt */
# define MX_DFS_GETINFO		(('m'<< 8) | 0x40)	/* mgx_dos.txt */
# define MX_DFS_INSTDFS		(('m'<< 8) | 0x41)	/* mgx_dos.txt */


/*
 * Printers (whole range reserved)
 */

/*# define PNVDI		(('p'<< 8) | *)*/	/* NVDI programmer's manual */


/*
 * MagiC specific calls for device driver installation (use group 'm'!)
 */

/*# define PROC_CREATE		0xcc00*/		/* mgx_dos.txt */
/*# define DEV_M_INSTALL	0xcd00*/		/* mgx_dos.txt */


/*
 * device driver installation
 */

# define DEV_NEWTTY	0xde00
# define DEV_NEWBIOS	0xde01
# define DEV_INSTALL	0xde02
# define DEV_INSTALL2	0xde03


/*
 * filesystem installation
 */

# define FS_INSTALL	0xf001		/* let the kernel know about the file system */
# define FS_MOUNT	0xf002		/* make a new directory for a file system */
# define FS_UNMOUNT	0xf003		/* remove a directory for a file system */
# define FS_UNINSTALL	0xf004		/* remove a file system from the list */

/*
 * filesystem information
 */

# define FS_INFO	0xf100		/* xfs fill out the following struct */

struct fs_info
{
	char	name [32];	/* name of the xfs (same as MX_KER_XFSNAME but more space) */
	long	version;	/* upper word: major version; lower word: minor version */
	long	type;		/* upper word: major type; lower word: minor type */
	char	type_asc[32];	/* human readable version of type */
};

/* values of fs_info.type */
# define _MAJOR_OLDTOS	(00L << 16)
# define _MAJOR_FAT	(01L << 16)
# define _MAJOR_VFAT	(02L << 16)
# define _MAJOR_MINIX	(03L << 16)
# define _MAJOR_RAMFS	(04L << 16)
# define _MAJOR_EXT2 	(05L << 16)
# define _MAJOR_HSIERRA	(06L << 16)
# define _MAJOR_ISO9660	(07L << 16)
# define _MAJOR_JOLIET	(08L << 16)
# define _MAJOR_HFS	(09L << 16)
# define _MAJOR_CDRAW	(10L << 16)
# define _MAJOR_STONX	(11L << 16)
# define _MAJOR_NFS	(12L << 16)

# define _MINOR_FAT12	0
# define _MINOR_FAT16	1
# define _MINOR_FAT32	2

# define FS_OLDTOS	(_MAJOR_OLDTOS)			/* default/unknown */
# define FS_FAT12	(_MAJOR_FAT  | _MINOR_FAT12)	/* MiNT 1.15 */
# define FS_FAT16	(_MAJOR_FAT  | _MINOR_FAT16)	/* MiNT 1.15 */
# define FS_FAT32	(_MAJOR_FAT  | _MINOR_FAT32)	/* MiNT 1.15 */
# define FS_VFAT12	(_MAJOR_VFAT | _MINOR_FAT12)	/* MiNT 1.15 */
# define FS_VFAT16	(_MAJOR_VFAT | _MINOR_FAT16)	/* MiNT 1.15 */
# define FS_VFAT32	(_MAJOR_VFAT | _MINOR_FAT32)	/* MiNT 1.15 */
# define FS_MINIX	(_MAJOR_MINIX)			/* MinixFS */
# define FS_RAMFS	(_MAJOR_RAMFS)			/* fnramFS */
# define FS_EXT2	(_MAJOR_EXT2)			/* Ext2-FS */
# define FS_HSIERRA	(_MAJOR_HSIERRA)		/* Spin 0.35 */
# define FS_ISO9660	(_MAJOR_ISO9660)		/* Spin 0.35 */
# define FS_JOLIET	(_MAJOR_JOLIET)			/* Spin 0.35 */
# define FS_HFS		(_MAJOR_HFS)			/* Spin 0.35 */
# define FS_CDRAW	(_MAJOR_CDRAW)			/* Spin 0.35 */
# define FS_STONX	(_MAJOR_STONX)			/* STonXfs4MiNT */
# define FS_NFS2	(_MAJOR_NFS)			/* nfs 0.55 */


# define FS_USAGE	0xf101		/* xfs fill out the following struct */

struct fs_usage
{
	unsigned long blocksize;/* 32bit: size in bytes of a block */
	llong	blocks;		/* 64bit: number of blocks */
	llong	free_blocks;	/* 64bit: number of free blocks */
	llong	inodes;		/* 64bit: number of inodes or FS_UNLIMITED */
	llong	free_inodes;	/* 64bit: number of free inodes or FS_UNLIMITED */
# define FS_UNLIMITED	-1
};

# endif /* _mint_dcntl_h */
