/*
 *	@(#)XHDI/xhdi.h
 *	
 *	Julian F. Reschke, Rainer Seitel, 1997-07-28
 *	
 *	Bindings for the XHDI functions
 *	--- NOT FULLY TESTED, USE AT YOUR OWN RISK ---
 *	
 *	some preprocessor changes for MiNT by frank naumann
 */


# ifndef _xhdi_h
# define _xhdi_h

# include <sys/types.h>


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
	u_char	ser[3];		/* Seriennummer */
	u_char	serms[4];	/* ab TOS 2.06: MS-DOS-4.0-Seriennummer */
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

long	XHDI_init	(void);

ushort	XHGetVersion	(void);
long	XHInqTarget	(ushort, ushort, ulong *, ulong *, char *);
long	XHReserve	(ushort, ushort, ushort, ushort);
long	XHLock		(ushort, ushort, ushort, ushort);
long	XHStop		(ushort, ushort, ushort, ushort);
long	XHEject		(ushort, ushort, ushort, ushort);
ulong	XHDrvMap	(void);
long	XHInqDev	(ushort, ushort *, ushort *, ulong *, __BPB *);
long	XHInqDriver	(ushort, char *, char *, char *, ushort *, ushort *);
long	XHNewCookie	(void *);
long	XHReadWrite	(ushort, ushort, ushort, ulong, ushort, void *);
long	XHInqTarget2	(ushort, ushort, ulong *, ulong *, char *, ushort);
long	XHInqDev2	(ushort, ushort *, ushort *, ulong *, __BPB *, ulong *, char *);
long	XHDriverSpecial	(ulong, ulong, ushort, void *);
long	XHGetCapacity	(ushort, ushort, ulong *, ulong *);
long	XHMediumChanged	(ushort, ushort);
long	XHMiNTInfo	(ushort, void *);
long	XHDOSLimits	(ushort, ulong);
long	XHLastAccess	(ushort, ushort, ulong *);
long	XHReaccess	(ushort, ushort);
long	XHInqDev3	(ushort, ushort *, ushort *, ulong *, __xhdi_BPB *, ulong *, u_char *, ushort);
void	XHMakeName	(ushort, ushort, ulong, char *);

# endif /* _xhdi_h */
