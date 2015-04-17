/*
 * $Id$
 * 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * @(#)XHDI/xhdi.h
 * 
 * Julian F. Reschke, Rainer Seitel, 1997-07-28
 * 
 * Bindings for the XHDI functions
 * --- NOT FULLY TESTED, USE AT YOUR OWN RISK ---
 * 
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 1999-01-01
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */


# ifndef _xhdi_h
# define _xhdi_h

# include "mint/mint.h"
# include "mint/kerinfo.h"


/*
 * BIOS parameter block (osbind.h is buggy)
 */

typedef struct bpb
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

# define XH_DL_SECSIZ		0	/* maximal sector size (BIOS level) */
# define XH_DL_MINFAT		1	/* minimal number of FATs */
# define XH_DL_MAXFAT		2	/* maximal number of FATs */
# define XH_DL_MINSPC		3	/* sectors per cluster minimal */
# define XH_DL_MAXSPC		4	/* sectors per cluster maximal */
# define XH_DL_CLUSTS		5	/* maximal number of clusters of a 16 bit FAT */
# define XH_DL_MAXSEC		6	/* maximal number of sectors */
# define XH_DL_DRIVES		7	/* maximal number of BIOS drives supported by the DOS */
# define XH_DL_CLSIZB		8	/* maximal clustersize */
# define XH_DL_RDLEN		9	/* max. (bpb->rdlen * bpb->recsiz / 32) */
# define XH_DL_CLUSTS12		12	/* max. number of clusters of a 12 bit FAT */
# define XH_DL_CLUSTS32		13	/* max. number of clusters of a 32 bit FAT */
# define XH_DL_BFLAGS		14	/* supported bits in bpb->bflags */


extern ushort XHDI_installed;

long	XHDI_init	(void);

long	sys_xhdi	(ushort op, long a1, long a2, long a3, long a4, long a5, long a6, long a7);

long	XHGetVersion	(void);
long	XHInqTarget	(ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name);
long	XHReserve	(ushort major, ushort minor, ushort do_reserve, ushort key);
long	XHLock		(ushort major, ushort minor, ushort do_lock, ushort key);
long	XHStop		(ushort major, ushort minor, ushort do_stop, ushort key);
long	XHEject		(ushort major, ushort minor, ushort do_eject, ushort key);
long	XHDrvMap	(void);
long	XHInqDev	(ushort bios_device, ushort *major, ushort *minor, ulong *start_sector, __BPB *bpb);
long	XHInqDriver	(ushort bios_device, char *name, char *ver, char *company, ushort *ahdi_version, ushort *maxIPL);
long	XHNewCookie	(void *newcookie);
long	XHReadWrite	(ushort major, ushort minor, ushort rwflag, ulong recno, ushort count, void *buf);
long	XHInqTarget2	(ushort major, ushort minor, ulong *block_size, ulong *device_flags, char *product_name, ushort stringlen);
long	XHInqDev2	(ushort bios_device, ushort *major, ushort *minor, ulong *start_sector, __BPB *bpb, ulong *blocks, char *partid);
long	XHDriverSpecial	(ulong key1, ulong key2, ushort subopcode, void *data);
long	XHGetCapacity	(ushort major, ushort minor, ulong *blocks, ulong *bs);
long	XHMediumChanged	(ushort major, ushort minor);
long	XHMiNTInfo	(ushort opcode, struct kerinfo *data);
long	XHDOSLimits	(ushort which, ulong limit);
long	XHLastAccess	(ushort major, ushort minor, ulong *ms);
long	XHReaccess	(ushort major, ushort minor);

# endif /* _xhdi_h */
