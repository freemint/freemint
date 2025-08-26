/*
 * Filename:     mkfatfs.c
 * Author:       Frank Naumann
 * Started:      1998-08-01
 * Target O/S:   TOS
 * Description:  Utility to allow an FAT filesystem to be created.
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@).
 * 
 * This program is a mostly rewritten version of
 * David Hudson's "mkdosfs".
 * 
 * Copying: Copyright 1998 Frank Naumann <fnaumann@freemint.de>
 * 
 * Portions copyright 1993, 1994 David Hudson (dave@humbug.demon.co.uk)
 * Portions copyright 1992, 1993 Remy Card (card@masi.ibp.fr)
 * and 1991 Linus Torvalds (torvalds@klaava.helsinki.fi)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * last changes:
 * 
 * 2000-06-14:	(v0.24)
 * 
 * - fix: for DOS partitions use fixed Adaptec mapping
 * 
 * 2000-06-14:	(v0.23)
 * 
 * - new: replaced old XHDI wrapper with new 32bit clean version
 *        -> removed mshort compile time switch
 * - fix: for DOS partition type detection, added more supported types
 * - new: adaptions for new MiNTLib
 * 
 * 1999-03-03:	(v0.22b)
 * 
 * - fix: better partition informations
 * 
 * 1998-11-04:	(v0.21b)
 * 
 * - fix: bug in FAT32 default cluster size
 * 
 * 1998-09-08:	(v0.2b)
 * 
 * - new: check partition ID, error message if partition ID is wrong
 * - new: works with logical sectors for GEMDOS compatible partitions
 * 
 * 1998-08-07:	(v0.1a)
 * 
 * - initial release
 * 
 * 
 * known bugs:
 * 
 * - nothing
 * 
 * todo:
 * 
 * - read/write sector test
 * - some more informations and warnings
 * 
 */

# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
# include <string.h>
# include <ctype.h>
# include <time.h>
# include <signal.h>
# include <getopt.h>
# include <unistd.h>
# include <mintbind.h>

# include "xhdi.h"
# include "mytypes.h"
# include "bswap.h"


# define VERSION	"0.26"
# define DEBUG(x)	printf x
# define INLINE		static inline

# define NO		0
# define YES		1

#ifndef NO_CONST
#  ifdef __GNUC__
#	 define NO_CONST(p) __extension__({ union { const void *cs; void *s; } _x; _x.cs = p; _x.s; })
#  else
#	 define NO_CONST(p) ((void *)(p))
#  endif
#endif

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')
#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)

/****************************************************************************/
/* BEGIN definition part */

/*
 * internal global data definitions
 */

# define BOOT_SIGN	0xAA55	/* Boot sector magic number */

/* FAT boot sector */
typedef struct
{
	uchar	boot_jump[3];	/* Boot strap short or near jump */
	char	system_id[8];	/* Name - can be used to special case partition manager volumes */
	
	uchar	sector_size[2];	/* bytes per logical sector */
	uchar	cluster_size;	/* sectors/cluster */
	ushort	reserved;	/* reserved sectors */
	uchar	fats;		/* number of FATs */
	uchar	dir_entries[2];	/* root directory entries */
	uchar	sectors[2];	/* number of sectors */
	uchar	media;		/* media code (unused) */
	ushort	fat_length;	/* sectors/FAT */
	ushort	secs_track;	/* sectors per track */
	ushort	heads;		/* number of heads */
	ulong	hidden;		/* hidden sectors (unused) */
	ulong	total_sect;	/* number of sectors (if sectors == 0) */
	
} _F_BS;

/* FAT volume info */
typedef struct
{
	uchar	drive_number;	/* BIOS drive number */
	uchar	RESERVED;	/* Unused */
	uchar	ext_boot_sign;	/* 0x29 if fields below exist (DOS 3.3+) */
	
# define EXT_INFO		0x29
	
	uchar	vol_id[4];	/* Volume ID number */
	char	vol_label[11];	/* Volume label */
	char	fs_type[8];	/* Typically FAT12, FAT16, or FAT32 */
	
# define FAT12_SIGN "FAT12   "	/* FAT12 filesystem signature */
# define FAT16_SIGN "FAT16   "	/* FAT16 filesystem signature */
# define FAT32_SIGN "FAT32   "	/* FAT32 filesystem signature */
	
} _F_VI;

/* FAT32 boot sector */
typedef struct
{
	_F_BS	fbs;		/* normal FAT boot sector */
	
	ulong	fat32_length;	/* sectors/FAT */
	ushort	flags;		/* bit 8: fat mirroring, low 4: active fat */
	
# define FAT32_ActiveFAT_Mask	0x0f
# define FAT32_NoFAT_Mirror	0x80
	
	uchar	version[2];	/* major, minor filesystem version */
	ulong	root_cluster;	/* first cluster in root directory */
	ushort	info_sector;	/* filesystem info sector */
	ushort	backup_boot;	/* backup boot sector */
	
# define INVALID_SECTOR		0xffff
	
	ushort	RESERVED2[6];	/* Unused */
	
} _F32_BS;

/* FAT32 boot fsinfo */
typedef struct
{
	ulong	reserved1;	/* Nothing as far as I can tell */
	ulong	signature;	/* 0x61417272L */
	
# define FAT32_FSINFOSIG	0x61417272L
	
	ulong	free_clusters;	/* Free cluster count.  -1 if unknown */
	ulong	next_cluster;	/* Most recently allocated cluster. Unused under Linux. */
	ulong	RESERVED2[4];	/* Unused */
	
} _F32_INF;

/* fat entry structure */
typedef struct
{
	char	name[11];	/* short name */
	char	attr;		/* file attribut */
	uchar	lcase;		/* used by Windows NT and Linux */
	uchar	ctime_ms;	/* creation time milliseconds */
	ushort	ctime;		/* creation time */
	ushort	cdate;		/* creation date */
	ushort	adate;		/* last access date */
	ushort	stcl_fat32;	/* the 12 upper bits for the stcl */
	ushort	time;		/* last modification time */
	ushort	date;		/* last modification date */
	ushort	stcl;		/* start cluster */
	ulong	flen;		/* file len */
	
} _DIR; /* 32 byte */

/* vfat entry structure */
typedef struct
{
	uchar	head;		/* bit 0..4: number of slot, bit 6: endofname */
	uchar	name0_4[10];	/* 5 unicode character */
	char	attr;		/* attribut (0x0f) */
	uchar	unused;		/* not used, reserved, (= 0) */
	uchar	chksum;		/* checksum short name */
	uchar	name5_10[12];	/* 6 unicode character */
	ushort	stcl;		/* start cluster (must be 0) */
	uchar	name11_12[4];	/* 2 unicode character */
	
} LDIR; /* 32 byte */

/*
 * file attributes
 */

# define FA_RDONLY	0x01	/* write protected */
# define FA_HIDDEN	0x02	/* hidden */
# define FA_SYSTEM	0x04	/* system */
# define FA_LABEL	0x08	/* label */
# define FA_DIR		0x10	/* subdirectory */
# define FA_CHANGED	0x20	/* archiv bit */
# define FA_VFAT	0x0f	/* internal: VFAT entry */
# define FA_SYMLINK	0x40	/* internal: symbolic link (MagiC like) */

# define FA_TOSVALID	(FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_DIR | FA_CHANGED)

/*
 * The "boot code" we put into the filesystem... it writes a message and
 * tells the user to try again. The last two bytes are the boot sign
 * bytes. On FAT32, it is less than 448 chars
 */

static char dummy_boot_jump[3] = { 0xeb, 0x3c, 0x90 };

static char dummy_boot_code[450] =
	"\x0e"			/* push cs */
	"\x1f"			/* pop ds */
	"\xbe\x5b\x7c"		/* mov si, offset message_txt */
				/* write_msg: */
	"\xac"			/* lodsb */
	"\x22\xc0"		/* and al, al */
	"\x74\x0b"		/* jz key_press */
	"\x56"			/* push si */
	"\xb4\x0e"		/* mov ah, 0eh */
	"\xbb\x07\x00"		/* mov bx, 0007h */
	"\xcd\x10"		/* int 10h */
	"\x5e"			/* pop si */
	"\xeb\xf0"		/* jmp write_msg */
				/* key_press: */
	"\x32\xe4"		/* xor ah, ah */
	"\xcd\x16"		/* int 16h */
	"\xcd\x19"		/* int 19h */
	"\xeb\xfe"		/* foo: jmp foo */
	
	/* message_txt: */
	"This is not a bootable disk.  Please insert a bootable floppy and\r\n"
	"press any key to try again ... \r\n"
;

# define MESSAGE_OFFSET 29	/* Offset of message in above code */

/*
 * internal global functions
 */

/* tools */

INLINE long	C2S	(long cluster);
INLINE long	S2C	(long sector);
INLINE long	cdiv	(long a, long b);


/* low level FAT access */

static void	mark_FAT	(long cluster, ulong value);
static void	mark_bad	(ulong sector);


/* block check */

static long	do_check	(char *buffer, long try, ulong current_block);
static void	alarm_intr	(long alnum);
static void	check_blocks	(void);

static void	get_bad_list	(char *filename);


/*  */

static void	get_geometry	(void);
static void	setup_tables	(void);
static void	write_tables	(void);

static void	usage		(void);
static int	verify_user	(void);

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

static int	check	= NO;	/* Default to no readability checking */
static int	always	= NO;	/* Default check partition IDs */
static int	verbose	= YES;	/* Default to verbose mode on */
static int	only_bs	= NO;	/* Write only the bootsector (dangerous option) */
static const char *program = "mkfatfs";	/* Name of the program */

static _F32_BS	f32bs;		/* Boot sector data (FAT and FAT32) */
static uchar *	fsinfo;		/* FAT32 signature sector */
static _F_VI	vi;		/* volume info */

static ulong	clsiz	= 0;	/* sectors per cluster */
static ulong	datrec	= 0;	/* first data sector */

static _DIR *	root	= NULL;	/* root directory */
static ulong	rentry	= 0;	/* root directory size in entrys */
static ulong	rsize	= 0;	/* root directory size in bytes */

static uchar *	fat	= NULL;	/* File allocation table */
static ulong	fsiz	= 0;	/* size of a FAT (in sectors) */
static short	fats	= 2;	/* number of FAT's */
static short	ftype	= 0;	/* type of FAT */

/* ftype: */
# define FAT_INVALID	0
# define FAT_TYPE_12	12
# define FAT_TYPE_16	16
# define FAT_TYPE_32	32

static char	id [4];		/* partition ID */
static short	mode	= 0;	/* partition mode */

# define MODE_BGM	0x1
# define MODE_F32	0x2
# define MODE_DOS	0x4
# define MODE_DOS_F32	0x8

static long	vol_id	= 0;	/* Volume ID number */
static char *	vol	= NULL;	/* Volume name */
static time_t	cr_time	= 0;	/* Creation time */

static ushort	drv	= 0;
static ushort	major	= 0;
static ushort	minor	= 0;
static ulong	start	= 0;
static ulong	psectors= 0;	/* Number of physical sectors in filesystem */
static ulong	lsectors= 0;	/* Number of logical sectors in filesystem */
static ulong	pssize	= 512;	/* physical sectorsize */
static ulong	lssize	= 512;	/* logical sectorsize */
static short    quiet;

static long	bad	= 0;	/* Number of bad sectors in the filesystam */
static ulong	test	= 0;	/* Block currently being tested (if autodetect bad sectors) */
static long	chunks	= 32;


# define CHECK		check
# define ALWAYS		always
# define VERBOSE	verbose
# define ONLY_BS	only_bs

# define BOOT		f32bs.fbs
# define BOOT32		f32bs
# define F32_INFO	fsinfo
# define VOL_INFO	vi

# define CLSIZE		clsiz
# define CLFIRST	datrec

# define ROOT		root
# define ROOTSIZE	rentry
# define ROOTBYTES	rsize

# define FAT		fat
# define FATSIZE	fsiz
# define FATS		fats
# define FTYPE		ftype

# define ID		id
# define MODE		mode
# define GEMDOS		(MODE & MODE_BGM)

# define DOFFSET	(CLFIRST - 2 * CLSIZE)

# define VOL_ID		vol_id
# define VOL_NAME	vol
# define CTIME		cr_time

# define DRV		drv
# define MAJOR		major
# define MINOR		minor
# define START		start
# define PSECS		psectors
# define SECS		lsectors
# define PSECSIZE	pssize
# define SECSIZE	lssize

# define BAD_SECS	bad
# define CURRENT	test
# define CHUNKS		chunks

/* END global data definition & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN tools */

INLINE long
C2S (long cluster)
{
	return ((cluster * CLSIZE) + DOFFSET);
}

INLINE long
S2C (long sector)
{
	return ((sector - DOFFSET) / CLSIZE);
}

/* Compute ceil (a/b) */
INLINE long
cdiv (long a, long b)
{
	return (a + b - 1) / b;
}

/* END tools */
/****************************************************************************/

/****************************************************************************/
/* BEGIN error handling */

static void
fatal_ (const char *fmt_string)
{
	printf (fmt_string, program, DriveToLetter(drv));
	
	exit (1);
}

# define fatal(str)	fatal_ ("%s: " str "\n")

/* END error handling */
/****************************************************************************/

/****************************************************************************/
/* BEGIN low level access */

static long
rwabs_xhdi (ushort rw, void *buf, ulong size, ulong recno)
{
	register ulong n = size / PSECSIZE;
	recno *= (SECSIZE / PSECSIZE);
	if (!n || (recno + n) > PSECS)
	{
		fatal ("rwabs_xhdi: access outside partition (drv = %c:)");
	}
	if (n > 65535UL)
	{
		fatal ("rwabs_xhdi: n to large (drv = %c)");
	}
	return XHReadWrite (MAJOR, MINOR, rw, START + recno, n, buf);
}

/* END low level access */
/****************************************************************************/

/****************************************************************************/
/* BEGIN low level FAT access */

static void
mark_FAT (long cluster, ulong value)
{
	if (FTYPE == FAT_TYPE_16)
	{
		value &= 0xffff;
		FAT [(2 * cluster)    ] = (uchar) (value & 0x00ff);
		FAT [(2 * cluster) + 1] = (uchar) (value >> 8);
	}
	else if (FTYPE == FAT_TYPE_32)
	{
		value &= 0x0fffffff;
		FAT [(4 * cluster)    ] = (uchar)  (value & 0x000000ff);
		FAT [(4 * cluster) + 1] = (uchar) ((value & 0x0000ff00) >> 8);
		FAT [(4 * cluster) + 2] = (uchar) ((value & 0x00ff0000) >> 16);
		FAT [(4 * cluster) + 3] = (uchar) ((value & 0xff000000) >> 24);
	}
	else if (FTYPE == FAT_TYPE_12)
	{
		value &= 0x0fff;
		if (((cluster * 3) & 0x1) == 0)
		{
			FAT [(3 * cluster / 2)    ] = (uchar) (value & 0x00ff);
			FAT [(3 * cluster / 2) + 1] = (uchar) ((FAT [(3 * cluster / 2) + 1] & 0x00f0) | ((value & 0x0f00) >> 8));
		}
		else
		{
			FAT [(3 * cluster / 2)    ] = (uchar) ((FAT [3 * cluster / 2] & 0x000f) | ((value & 0x000f) << 4));
			FAT [(3 * cluster / 2) + 1] = (uchar) ((value & 0x0ff0) >> 4);
		}
	}
	else
		fatal ("internal error");
}

/*
 * Mark a cluster in the FAT as bad
 */

static void
mark_bad (ulong sector)
{
	if (FTYPE == 16)
		mark_FAT (S2C (sector), 0xfff7);
	else if (FTYPE == 32)
		mark_FAT (S2C (sector), 0xffffff7);
	else if (FTYPE == 12)
		mark_FAT (S2C (sector), 0xff7);
	else
		fatal ("internal error");
}

/* END low level FAT access */
/****************************************************************************/

/****************************************************************************/
/* BEGIN block check */

/*
 * Perform a test on a number of sectors.
 * Return the number of sectors that could be read successfully
 */

static long
do_check (char *buffer, long try, ulong sector)
{
	long i = try;
	long r;
	
	r = XHReadWrite (MAJOR, MINOR, 0, START + sector, i, buffer);
	if (r)
	{
		i = 0;
		while (r == 0 && i < try)
		{
			r = XHReadWrite (MAJOR, MINOR, 0, START + sector + i, 1, buffer);
			i++;
		}
	}
	
	return i;
}


/*
 * Alarm clock handler - display the status of the quest for bad sectors!
 * Then retrigger the alarm for five senconds
 * later (so we can come here again)
 */

static void
alarm_intr (long alnum)
{
	if (CURRENT >= SECS)
		return;
	
	signal (SIGALRM, (__sighandler_t) alarm_intr);
	alarm (3);
	
	if (!CURRENT)
		return;
	
	printf ("%ld ... ", CURRENT);
	fflush (stdout);
}


static void
check_blocks (void)
{
	char *buf;
	long try, got;
	
	buf = malloc (CHUNKS * SECSIZE);
	if (buf == NULL)
		fatal ("out of memory");
	
	printf ("Searching for bad sectors (%ld todo):\n", SECS);
	
	CURRENT = 0;
	
	signal (SIGALRM, (__sighandler_t) alarm_intr);
	alarm (3);
	
	try = CHUNKS;
	while (CURRENT < SECS)
	{
		if (CURRENT + try > SECS)
			try = SECS - CURRENT;
		
		got = do_check (buf, try, CURRENT);
		CURRENT += got;
		if (got == try)
		{
			try = CHUNKS;
			continue;
		}
		else
			try = 1;
		
		if (CURRENT < CLFIRST)
			fatal ("bad sectors before data-area: cannot make fs");
		
		mark_bad (CURRENT);
		
		BAD_SECS++;
		CURRENT++;
	}
	
	printf ("\n");
	
	if (BAD_SECS)
		printf ("%ld bad block%s\n", BAD_SECS, (BAD_SECS > 1) ? "s" : "");
	
	free (buf);
}

static void
get_bad_list (char *filename)
{
	FILE *listfile;
	ulong sector;
	
	listfile = fopen (filename, "r");
	if (listfile == NULL)
		fatal ("Can't open file of bad sectors");
	
	while (!feof (listfile))
	{
		fscanf (listfile, "%ld\n", &sector);
		mark_bad (sector);
		BAD_SECS++;
	}
	fclose (listfile);
	
	if (BAD_SECS)
		printf ("%ld bad block%s\n", BAD_SECS, (BAD_SECS > 1) ? "s" : "");
}

/* END block check */
/****************************************************************************/

/****************************************************************************/
/* BEGIN  */

/*
 * Establish the geometry and media parameters for the device
 */

static void
get_geometry (void)
{
	long r;
	
	r = XHInqDev2 (drv, &MAJOR, &MINOR, &START, NULL, &PSECS, ID);
	if (r == 0)
	{
		r = XHInqTarget2 (MAJOR, MINOR, &PSECSIZE, NULL, NULL, 0);
	}
	if (r != 0)
	{
		perror ("get_geometry");
		fatal ("unable to get geometry for '%c'");
	}
	
	ID [3] = '\0';
	
	printf ("Physical informations about partition %c:\n", DriveToLetter(drv));
	printf ("----------------------------------------\n");
	printf ("XHDI major number    : %d\n", MAJOR);
	printf ("XHDI minor number    : %d\n", MINOR);
	printf ("partition start      : %ld\n", START);
	printf ("partition length     : %ld sectors\n", PSECS);
	printf ("phyiscal sector size : %ld bytes\n", PSECSIZE);
	
	MODE = 0;
	
	if (ID [0])
	{
		printf ("partition type       : TOS\n");
		printf ("partition ID         : %s\n", ID);
		
		if ((strcmp (ID, "GEM") == 0) || (strcmp (ID, "BGM") == 0))
		{
			MODE |= MODE_BGM;
			if (!FTYPE) FTYPE = 16;
		}
		else if (strcmp (ID, "F32") == 0)
		{
			MODE |= MODE_F32;
			if (!FTYPE) FTYPE = 32;
		}
	}
	else if (ID [1] == 'D')
	{
		int sys_type = ID [2];
		
		printf ("partition type       : DOS\n");
		printf ("partition ID         : 0x%x\n", sys_type);
		
		switch (sys_type)
		{
			case 0x4:
			case 0x6:
			case 0xe:
			{
				MODE |= MODE_DOS;
				if (!FTYPE) FTYPE = 16;
				break;
			}
			case 0xb:
			case 0xc:
			{
				MODE |= MODE_DOS_F32;
				if (!FTYPE) FTYPE = 32;
				break;
			}
		}
	}
	else
	{
		printf ("partition type       : UNKNOWN\n");
		printf ("partition ID         : %s\n", ID);
	}
	
	printf ("\n");
	
	/* Set up the geometry information */
	BOOT.secs_track = 63; /* geometry.sectors; */
	BOOT.heads = 255; /* geometry.heads; */
	
	/* Set up the media descriptor for a hard drive */
	BOOT.media = (char) 0xf8;
	
	/*
	 * Default to 512 entries
	 * No limits on FAT32 root directory
	 */
	BOOT.dir_entries[0] = (char) 0;
	BOOT.dir_entries[1] = (char) ((FTYPE == 32) ? 0 : 2);
	
	if (FTYPE == 32)
	{
		/*
		 * This follows what Microsoft's
		 * format command does for FAT32
		 */
		BOOT.cluster_size = (PSECS >= 524288) ? 8 : 1;
	}
	else
	{
		/* Start at 2 sectors per cluster */
		BOOT.cluster_size = 2;
	}
	
	if (MODE & MODE_BGM)
	{
		/* in GEMDOS mode fix clsiz to 2 */
		CLSIZE = 2;
	}
	
	SECS = PSECS;
	SECSIZE = PSECSIZE;
}


/* Create the filesystem data tables */

static void
setup_tables (void)
{
	ulong fatlength12, fatlength16, fatlength32;
	ulong maxclust12, maxclust16, maxclust32;
	ulong clust12, clust16, clust32;
	
	long fatdata; /* Sectors for FATs + data area */
	long maxclustsize;
	long maxsecsize = 16384; /* depends on TOS version? */
	
	long cluster_count = 0;
	long i;
	
	struct tm *create_time;
	
	
	strcpy (BOOT.system_id, "mkdosfs");
	
	if (!ROOTSIZE)
	{
		ROOTSIZE = BOOT.dir_entries[0] + (BOOT.dir_entries[1] << 8);
	}
	
	VOL_INFO.vol_id[0] = (uchar)  (VOL_ID & 0x000000ff);
	VOL_INFO.vol_id[1] = (uchar) ((VOL_ID & 0x0000ff00) >> 8);
	VOL_INFO.vol_id[2] = (uchar) ((VOL_ID & 0x00ff0000) >> 16);
	VOL_INFO.vol_id[3] = (uchar) ((VOL_ID & 0xff000000) >> 24);
	
	memcpy (VOL_INFO.vol_label, VOL_NAME, 11);
	memcpy (BOOT.boot_jump, dummy_boot_jump, 3);
	
	i = SECSIZE - sizeof (VOL_INFO);
	if (FTYPE == 32)
		i -= sizeof (BOOT32);
	else
		i -= sizeof (BOOT);
	
	dummy_boot_code[i-2] = BOOT_SIGN & 0xff;
	dummy_boot_code[i-1] = BOOT_SIGN >> 8;
	
	BOOT.reserved = (FTYPE == 32) ? 32 : 1;
	BOOT.fats = (char) FATS;
	BOOT.hidden = 0;
	
	fatdata = SECS - BOOT.reserved;
	/*
	 * XXX: If we ever change to automatically create FAT32
	 * partitions without forcing the user to specify it,
	 * this will have to change
	 */
	if (FTYPE != 32)
	{
		long entries = (((ROOTSIZE * 32 + SECSIZE - 1) / SECSIZE) * SECSIZE) / 32;
		
		fatdata -= cdiv (entries * 32, SECSIZE);
	}
	
	
	if (CLSIZE)
	{
		BOOT.cluster_size = (char) CLSIZE;
		maxclustsize = CLSIZE;
	}
	else
	{
		/*
		 * An initial guess for BOOT.cluster_size
		 * should already be set
		 */
		maxclustsize = 128;
	}
	
	do {
		clust12 = (long long) fatdata * SECSIZE / ((long) BOOT.cluster_size * SECSIZE + 3);
		fatlength12 = cdiv ((clust12 * 3 + 1) >> 1, SECSIZE);
		maxclust12 = (fatlength12 * 2 * SECSIZE) / 3;
		if (maxclust12 > 4086)
			maxclust12 = 4086;
		if (clust12 > maxclust12)
			clust12 = 0;
		
		clust16 = (long long) fatdata * SECSIZE / ((long) BOOT.cluster_size * SECSIZE + 4);
		fatlength16 = cdiv (clust16 * 2, SECSIZE);
		maxclust16 = (fatlength16 * SECSIZE) / 2;
		if (maxclust16 > (GEMDOS ? 32766 : 65526))
			maxclust16 = GEMDOS ? 32766 : 65526;
		if (clust16 > maxclust16)
			clust16 = 0;
		
		clust32 = (long long) fatdata * SECSIZE / ((long) BOOT.cluster_size * SECSIZE + 4);
		fatlength32 = cdiv (clust32 * 4, SECSIZE);
		maxclust32 = (fatlength32 * SECSIZE) / 4;
		if (maxclust32 > 0xffffff6)
			maxclust32 = 0xffffff6;
		if (clust32 > maxclust32)
			clust32 = 0;
		
		if ((clust12 && (FTYPE == 12 || FTYPE == 0))
			|| (clust16 && (FTYPE == 16 || FTYPE == 0))
			|| (clust32 && (FTYPE == 32)))
		{
			break;
		}
		
		if (MODE & MODE_BGM)
		{
			long entries;
			
			SECSIZE <<= 1;
			SECS >>= 1;
			
			entries = (((ROOTSIZE * 32 + SECSIZE - 1) / SECSIZE) * SECSIZE) / 32;
			
			fatdata = SECS - BOOT.reserved;
			fatdata -= cdiv (entries * 32, SECSIZE);
		}
		else
		{
			BOOT.cluster_size <<= 1;
		}
	}
	while (BOOT.cluster_size
		&& BOOT.cluster_size <= maxclustsize
		&& SECSIZE <= maxsecsize);
	
	
	if (FTYPE != 32)
	{
		ROOTSIZE = (((ROOTSIZE * 32 + SECSIZE - 1) / SECSIZE) * SECSIZE) / 32;
		
		BOOT.dir_entries[0] = (char)  (ROOTSIZE & 0x00ff);
		BOOT.dir_entries[1] = (char) ((ROOTSIZE & 0xff00) >> 8);
	}
	
	BOOT.sector_size[0] = (char)  (SECSIZE & 0x00ff);
	BOOT.sector_size[1] = (char) ((SECSIZE & 0xff00) >> 8);
	
	if (SECS >= 65536)
	{
		BOOT.sectors[0] = (char) 0;
		BOOT.sectors[1] = (char) 0;
		BOOT.total_sect = SECS;
	}
	else
	{
		BOOT.sectors[0] = (char)  (SECS & 0x00ff);
		BOOT.sectors[1] = (char) ((SECS & 0xff00) >> 8);
		BOOT.total_sect = 0;
	}
	
	
	/* Use the optimal FAT size */
	/* XXX: Force use of clust32 if specified on command line */
	if (!FTYPE)
	{
		FTYPE = (clust16 > clust12) ? 16 : 12;
	}
	
	switch (FTYPE)
	{
		case 12:
		{
			cluster_count = clust12;
			FATSIZE = BOOT.fat_length = fatlength12;
			memcpy (VOL_INFO.fs_type, FAT12_SIGN, 8);
			
			break;
		}
		case 16:
		{
			cluster_count = clust16;
			FATSIZE = BOOT.fat_length = fatlength16;
			memcpy (VOL_INFO.fs_type, FAT16_SIGN, 8);
			
			break;
		}
		case 32:
		{
			cluster_count = clust32;
			BOOT.fat_length = 0;
			FATSIZE = BOOT32.fat32_length = fatlength32;
			BOOT32.flags = 0;
			BOOT32.version[0] = 0;
			BOOT32.version[1] = 0;
			BOOT32.root_cluster = 2;
			BOOT32.info_sector = 1;
			BOOT32.backup_boot = 6;
			memset (&BOOT32.RESERVED2, 0, sizeof (BOOT32.RESERVED2));
			memcpy (VOL_INFO.fs_type, FAT32_SIGN, 8);
			
			break;
		}
		default:
		{
			fatal ("FAT not 12, 16, or 32 bits!\n");
		}
	}		
	VOL_INFO.ext_boot_sign = EXT_INFO;
	
	if (!cluster_count)
	{
		if (CLSIZE)
		{
			fatal ("Too many clusters for file system - try more sectors per cluster");
		}
		else
		{
			fatal ("Attempting to create too large a file system");
		}
	}
	
	CLFIRST = (long) (BOOT.reserved) + FATS * FATSIZE;
	
	if (VERBOSE)
	{
		printf ("Logical informations about partition %c:\n", DriveToLetter(drv));
		printf ("---------------------------------------\n");
		printf ("Media descriptor    : 0x%02x\n", (unsigned int)(BOOT.media));
		printf ("logical sector size : %ld\n", SECSIZE);
		printf ("sectors per cluster : %ld\n", (long)(BOOT.cluster_size));
		printf ("clustersize         : %ld\n", (long)(BOOT.cluster_size) * SECSIZE);
		printf ("number of FATs      : %ld\t\t(%ld bit wide)\n", (long)(BOOT.fats), (long)FTYPE);
		printf ("sectors per FAT     : %ld\t(%8qd kb)\n", FATSIZE, ((long long)FATSIZE * SECSIZE) / 1024);
		printf ("number of cluster   : %ld\t(%8qd kb)\n", cluster_count, ((long long)cluster_count * BOOT.cluster_size * SECSIZE) / 1024);
		printf ("Volume ID           : %08lx\n", vol_id);
		
		if (memcmp (VOL_NAME, "           ", 11) != 0)
			printf ("Volume label        : %s\n", VOL_NAME);
		else
			printf ("No volume label.\n");
		
		printf ("\n");
	}
	
	/* Make the file allocation tables! */
	FAT = malloc (FATSIZE * SECSIZE);
	if (FAT == NULL)
	{
		fatal ("out of memory");
	}
	
	memset (FAT, 0, FATSIZE * SECSIZE);
	
	/* Initial fat entries */
	mark_FAT (0, 0xfffffff);
	mark_FAT (1, 0xfffffff);
	
	/* XXX: Not sure about this one */
	if (FTYPE == 32)
	{
		mark_FAT (2, 0xffffff8);
	}
	/* Put media type in first byte! */
	FAT[0] = (uchar) BOOT.media;
	
	/* Make the root directory entries */
	if (FTYPE == 32)
	{
		ROOTBYTES = BOOT.cluster_size * SECSIZE;
	}
	else
	{
		ROOTBYTES = ROOTSIZE * 32;
	}
	
	ROOT = malloc (ROOTBYTES);
	if (ROOT == NULL)
	{
		free (FAT);
		fatal ("out of memory");
	}
	
	memset (ROOT, 0, ROOTBYTES);
	
	if (memcmp (VOL_NAME, "           ", 11) != 0)
	{
		memcpy (ROOT[0].name, VOL_NAME, 11);
		ROOT[0].attr = FA_LABEL;
		create_time = localtime (&CTIME);;
		
		ROOT[0].time = (ushort) ((create_time->tm_sec >> 1)
					+ (create_time->tm_min << 5)
					+ (create_time->tm_hour << 11));
		ROOT[0].date = (ushort) (create_time->tm_mday
					+ ((create_time->tm_mon+1) << 5)
					+ ((create_time->tm_year-80) << 9));
	}
	
	if (FTYPE == 32)
	{
		ulong clusters;
		ulong fat_clusters;
		
		F32_INFO = malloc (SECSIZE);
		if (F32_INFO == NULL) 
		{
			fatal ("out of memory");
		}
		memset (F32_INFO, 0, SECSIZE);
		
		F32_INFO[0] = 'R';
		F32_INFO[1] = 'R';
		F32_INFO[2] = 'a';
		F32_INFO[3] = 'A';
		
		F32_INFO[0x1e4] = 'r';
		F32_INFO[0x1e5] = 'r';
		F32_INFO[0x1e6] = 'A';
		F32_INFO[0x1e7] = 'a';
		
		clusters = (SECS - CLFIRST) / BOOT.cluster_size;
		fat_clusters = FATSIZE * SECSIZE * 8 / 32;
		
		if (clusters + 2 > fat_clusters)
			clusters = fat_clusters - 2;
		
		F32_INFO[0x1e8] = (clusters & 0x000000ff);
		F32_INFO[0x1e9] = (clusters & 0x0000ff00) >> 8;
		F32_INFO[0x1ea] = (clusters & 0x00ff0000) >> 16;
		F32_INFO[0x1eb] = (clusters & 0xff000000) >> 24;
		F32_INFO[0x1ec] = 0x02;
		F32_INFO[0x1ed] = 0x00;
		F32_INFO[0x1ee] = 0x00;
		F32_INFO[0x1ef] = 0x00;
		
		F32_INFO[0x1fe] = BOOT_SIGN & 0xff;
		F32_INFO[0x1ff] = BOOT_SIGN >> 8;
	}
	
	
	if (ALWAYS)
	{
		printf ("WARNING: no partition ID checking!\n\n");
	}
	else
	{
		/* verify with partition type */
		
		if (!MODE)
		{
			printf ("Can't create filesystem - unknown partition ID!\n");
			fatal ("aborted");
		}
		
		if (FTYPE == 32)
		{
			if (!((MODE & MODE_F32) || (MODE & MODE_DOS_F32)))
			{
				printf ("Can't create FAT32 filesystem - wrong partition ID!\n");
				printf ("Please change first partition ID to 'F32' (TOS) or '0xb' (DOS).\n");
				printf ("See in documentation for more details.\n");
				printf ("\n");
				fatal ("aborted.");
			}
		}
		else
		{
			if (!((MODE & MODE_BGM) || (MODE & MODE_DOS)))
			{
				printf ("Can't create FAT filesystem - wrong partition ID!\n");
				printf ("Please change first partition ID to 'BGM' (TOS) or '0x6' (DOS).\n");
				printf ("See in documentation for more details.\n");
				printf ("\n");
				fatal ("aborted.");
			}
		}
	}
}


/*
 * Write the new filesystem's data tables to disk
 */

static void
write_tables (void)
{
	char *boot, *blank;
	long actual;
	long i, j, r;
	
	
	/* byte order correction */
	
	BOOT.reserved		= BSWAP16 (BOOT.reserved);
	BOOT.fat_length		= BSWAP16 (BOOT.fat_length);
	BOOT.secs_track		= BSWAP16 (BOOT.secs_track);
	BOOT.heads		= BSWAP16 (BOOT.heads);
	BOOT.hidden		= BSWAP32 (BOOT.hidden);
	BOOT.total_sect		= BSWAP32 (BOOT.total_sect);
	
	BOOT32.fat32_length	= BSWAP32 (BOOT32.fat32_length);
	BOOT32.flags		= BSWAP16 (BOOT32.flags);
	BOOT32.root_cluster	= BSWAP32 (BOOT32.root_cluster);
	BOOT32.info_sector	= BSWAP16 (BOOT32.info_sector);
	BOOT32.backup_boot	= BSWAP16 (BOOT32.backup_boot);
	
	ROOT[0].time		= BSWAP16 (ROOT[0].time);
	ROOT[0].date		= BSWAP16 (ROOT[0].date);
	
	
	/* build boot and blank sector */
	
	boot = malloc (SECSIZE);
	blank = malloc (SECSIZE);
	
	if (!boot || !blank)
		fatal ("out of memory");
	
	memset (boot, 0, SECSIZE);
	memset (blank, 0, SECSIZE);
	
	if (FTYPE == 32)
		i = sizeof (BOOT32);
	else
		i = sizeof (BOOT);
	
	memcpy (boot, &BOOT32, i);
	memcpy (boot + i, &VOL_INFO, sizeof (VOL_INFO));
	
	i += sizeof (VOL_INFO);
	j = 512 - i;
	
	memcpy (boot + i, dummy_boot_code, j);
	
# if 1
# define FEEDBACK(x)	printf ("."); fflush (stdout)
# define FEEDBACKSTART	printf ("Writing tables ")
# define FEEDBACKEND	printf (" done\n")
# else
# define FEEDBACK(x)	printf x; fflush (stdout)
# define FEEDBACKSTART	printf ("Initialising partition ...\n")
# define FEEDBACKEND	printf ("done\n")
# endif
	
	/* write out the bootsector (and backup if FAT32) */
	
	FEEDBACKSTART;
	
	actual = 0;
	for (i = 0; i < ((FTYPE == 32) ? 2 : 1); i++)
	{
		FEEDBACK (("Write bootblock: sector = %ld, count = %ld\n", actual, 1L));
		r = rwabs_xhdi (1, boot, SECSIZE, actual++);
		if (r) fatal ("failed whilst writing boot sector");
		
		if (ONLY_BS == YES)
		{
			FEEDBACKEND;
			printf("Only bootsector has been written!\n");
			return;
		}
		
		if (FTYPE == 32)
		{
			long max;
			
			FEEDBACK (("Write boot backup: sector = %ld, count = %ld\n", actual, 1L));
			r = rwabs_xhdi (1, F32_INFO, SECSIZE, actual++);
			if (r) fatal ("failed whilst writing boot sector");
			
			/* So far, two sectors have gone out. Write out blanks */
			max = BSWAP16 (BOOT32.backup_boot);
			for (j = 2; j < max; j++)
			{
				FEEDBACK (("Write blank: sector = %ld, count = %ld\n", actual, 1L));
				r = rwabs_xhdi (1, blank, SECSIZE, actual++);
				if (r) fatal ("failed whilst writing boot sector");
			}
		}
	}
	
	actual = BSWAP16 (BOOT.reserved);
	
	
	/* write out FATs */
	
	for (i = 0; i < FATS; i++)
	{
		FEEDBACK (("Write FATs: sector = %ld, count = %ld\n", actual, FATSIZE));
		r = rwabs_xhdi (1, FAT, FATSIZE * SECSIZE, actual);
		if (r) fatal ("failed whilst writing FAT");
		actual += FATSIZE;
	}
	
	
	/* write out root directory */
	
	FEEDBACK (("Write rootdir: sector = %ld, count = %ld\n", actual, ROOTBYTES / SECSIZE));
	r = rwabs_xhdi (1, ROOT, ROOTBYTES, actual);
	if (r) fatal ("failed whilst writing root directory");
	
	FEEDBACKEND;
}

/* END  */
/****************************************************************************/

/****************************************************************************/
/* BEGIN main */

static void
usage (void)
{
	fatal_ (
		"\n"
		"Usage: %s [OPTIONS] drv:\n"
		"  -c            Check filesystem as is gets built\n"
		"  -f <num>      Number of FATs (between 1 and 4)\n"
		"  -F <num>      FAT size (12, 16, or 32)\n"
		"  -i <id>       Volume ID as hexadecimal number (i.e. 0x0410)\n"
		"  -l <filename> Bad block filename\n"
		"  -m <filename> Boot message filename--text is used as a boot message\n"
		"  -n <name>     Volume name\n"
		"  -r <num>      Root directory entries (not applicable to FAT32)\n"
		"  -s <num>      Sectors per cluster (2, 4, 8, 16, 32, 64, or 128)\n"
		"  -a            Create filesystem always (disables partition ID checking)\n"
		"\n"
		"Expert options:\n"
		"  -B            Write boot sector only (attention, be sure you know\n"
		"                what you do!)\n"
	);
}

static int
verify_user (void)
{
	char c;
	
	if (ONLY_BS == YES)
	{
		printf ("WARNING: THIS WILL OVERWRITE YOUR BOOTSECTOR ON %c:\n", DriveToLetter(drv));
		if (!quiet)
			printf ("Are you ABSOLUTELY SURE you want to do this? (y/n) ");
	}
	else
	{
		printf ("WARNING: THIS WILL TOTALLY DESTROY ANY DATA ON %c:\n", DriveToLetter(drv));
		if (!quiet)
			printf ("Are you ABSOLUTELY SURE you want to do this? (y/n) ");
	}
	if (quiet)
		return 1;
	scanf ("%c", &c);
	printf ("\n");
	
	if (c == 'y' || c == 'Y')
		return 1;
	
	return 0;
}

int
main (int argc, char **argv)
{
	char *listfile = NULL;
	char *tmp;
	char c;
	long i;
	
	VOL_NAME = strdup ("           ");
	
	if (argc && *argv && **argv)
	{
		char *p, *p2;

		program = argv[0];
		p = strrchr(argv[0], '/');
		p2 = strrchr(argv[0], '\\');
		if (p == NULL || p2 > p)
			p = p2;
		if (p)
			program = p + 1;
	}
	printf ("%s " VERSION ", 2002-09-19 for TOS and DOS FAT/FAT32-FS\n", program);
	
	/* check and initalize XHDI */
	if (init_XHDI ())
		fatal ("No XHDI installed (or to old)");
	
	printf ("Found XHDI level %x.%x.\n\n", (XHDI_installed >> 8), (XHDI_installed & 0x00ff));
	
	/* Default volume ID = creation time */
	time (&CTIME);
	VOL_ID = (long) CTIME;
	
	while ((c = getopt (argc, argv, "cf:F:i:l:m:n:qr:s:avB")) != EOF)
	{
		/* Scan the command line for options */
		switch (c)
		{
			/* Check FS as we build it */
			case 'c':
			{
				CHECK = YES;
				break;
			}
			/* Choose number of FATs */
			case 'f':
			{
				FATS = (long) strtol (optarg, &tmp, 0);
				if (*tmp || FATS < 1 || FATS > 4)
				{
					printf ("Bad number of FATs: %s\n", optarg);
					usage ();
				}
				break;
			}
			/* Choose FAT size */
			case 'F':
			{
				FTYPE = (long) strtol (optarg, &tmp, 0);
				if (*tmp || (FTYPE != 12 && FTYPE != 16 && FTYPE != 32))
				{
					printf ("Bad FAT type: %s\n", optarg);
					usage ();
				}
				break;
			}
			/* specify volume ID */
			case 'i':
			{
				VOL_ID = strtol (optarg, &tmp, 16);
				if (*tmp)
				{
					printf ("Volume ID must be a hexadecimal number\n");
					usage ();
				}
				break;
			}
			/* Bad block filename */
			case 'l':
			{
				listfile = optarg;
				break;
			}
			/* Set boot message */
			case 'm':
			{
				printf ("-m not supported\n");
				break;
			}
			/* Volume name */
			case 'n':
			{		
				sprintf (VOL_NAME, "%-11.11s", optarg);
				for (i = 0; i < 11; i++)
				{
					VOL_NAME[i] = toupper (VOL_NAME[i]);
				}
				printf ("%s\n", VOL_NAME);
				break;
			}
			/* Root directory entries */
			case 'r':
			{
				ROOTSIZE = (long) strtol (optarg, &tmp, 0);
				if (*tmp || ROOTSIZE < 16 || ROOTSIZE > 32768)
				{
					printf ("Bad number of root directory entries: %s\n", optarg);
					usage ();
				}
				break;
			}
			/* Sectors per cluster */
			case 's':
			{		
				CLSIZE = (long) strtol (optarg, &tmp, 0);
				if (*tmp
					|| (CLSIZE != 1
						&& CLSIZE != 2
						&& CLSIZE != 4
						&& CLSIZE != 8
						&& CLSIZE != 16
						&& CLSIZE != 32
						&& CLSIZE != 64
						&& CLSIZE != 128
					))
				{
					printf ("Bad number of sectors per cluster: %s\n", optarg);
					usage ();
				}
				break;
			}
			/* disbale partition ID checking */
			case 'a':
			{
				ALWAYS = YES;
				break;
			}
			/* write boot sector only */
			case 'B':
			{
				ONLY_BS = YES;
				break;
			}
			case 'q':
				quiet = 1;
				break;
			default:
			{
				usage ();
			}
		}
	}
	
	if (argv[optind])
	{
		c = *(argv[optind]);
	}
	else
	{
		printf ("No drive specified.\n");
		usage ();
	}
	
	drv = DriveFromLetter(c);
	if (drv < 0)
	{
		fatal ("invalid drive");
	}

	/*
	 * Auto and specified bad sector handling are mutually
	 * exclusive of each other!
	 */
	if (CHECK && listfile)
	{
		fatal ("-c and -l are incompatible");
	}
	
	/* lock the drv */
	if (Dlock (1, drv))
	{
		fatal ("Can't lock %c:");
	}
	
	/* Establish the media parameters */
	get_geometry ();
	
	/* Establish the file system tables */
	setup_tables ();
	
	/* Determine any bad block locations and mark them */
	if (CHECK)
	{
		check_blocks ();
	}
	else
	{
		if (listfile)
		{
			get_bad_list (listfile);
		}
	}
	
	if (verify_user ())
	{
		/* Write the file system tables away! */
		write_tables ();
	}
	
	(void) Dlock (0, drv);
	
	/* Terminate with no errors! */
	return EXIT_SUCCESS;
}

/* END main */
/****************************************************************************/
