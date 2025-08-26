/*
 * minit - Minix INITializer.
 * Freely distributable Minix filesys creator.
 * 
 * Copyright S N Henson 1991, 1992, 1993, 1994.
 * Copyright Frank Naumann 1998, 1999.
 * 
 * Use entirely at your own risk! If it trashes your hard-drive then
 * it isn't my fault! 
 */

# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
# include <string.h>

# include <ctype.h>
# include <time.h>
# include <signal.h>
# include <unistd.h>
# define __KERNEL__
# include <errno.h>

# include <mintbind.h>

# include "xhdi.h"


# define VERSION	"0.32"

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')
#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)

/*
 * global data
 */

ushort	drv		= -1;
ushort	major		= 0;
ushort	minor		= 0;
ulong	start		= 0;
ulong	sectors		= 0;	/* number of physical sectors in filesystem */
ulong	ssize		= 512;	/* sector sectorsize */
char	id [4]		= { '\0', '\0', '\0', '\0' };
long	numblocks	= 0;
long	numinodes	= 0;
long	incr		= 4;	/* default to 62 characters per name */

/* various flags */
long sonly;
long tst;

/*
 * Structures we will need
 */

typedef struct
{
	ushort	s_ninodes;	/* # usable inodes on the minor device */
	ushort	s_nzones;	/* total device size, including bit maps etc */
	ushort	s_imap_blks;	/* # of blocks used by inode bit map */
	ushort	s_zmap_blks;	/* # of blocks used by zone bit map */
	ushort	s_firstdatazn;	/* number of first data zone */
	short	s_log_zsize;	/* log2 of blocks/zone */
	ulong	s_max_size;	/* maximum file size on this device */
	ushort	s_magic;	/* magic number to recognize super-blocks */
	ushort	s_state;	/* filesystem state */
	long	s_zones;	/* equivalent to 's_nzones' for V2 */
	
} super_block;

uchar block_buf[1024];
super_block *sblk = (super_block *) block_buf;

/* directory entry */
typedef struct
{
	ushort	d_inum;		/* inode number */
	char	d_name[14];	/* character string */
	
} dir_struct;

typedef struct
{
	ushort	i_mode;
	ushort	i_nlinks;
	ushort	i_uid;
	ushort	i_gid;
	ulong	i_size;
	ulong	i_atime;
	ulong	i_mtime;
	ulong	i_ctime;
	long	i_zone[10];
	
} d_inode2;


/****************************************************************************/
/* BEGIN low level access */

static long
rwabs_xhdi (ushort rw, void *buf, ulong size, ulong recno)
{
	register ulong n = size / ssize;
	
	if (ssize == 512)
	{
		recno <<= 1;
	}
	
	if (!n || (recno + n) > sectors)
	{
		fprintf (stderr, "rwabs_xhdi: access outside partition (drv = %c:)", DriveToLetter(drv));
		exit (2);
	}
	if (n > 65535UL)
	{
		fprintf (stderr, "rwabs_xhdi: n to large (drv = %c)", DriveToLetter(drv));
		exit (2);
	}
	
	return XHReadWrite (major, minor, rw, start + recno, n, buf);
}

# ifdef NOTUSED
static void
get_block (long num)
{
	long r;
	
	r = rwabs_xhdi (0, block_buf, 1024, num);
	if (r)
	{
		fprintf (stderr, "Fatal Read Error (%ld), block %ld\n", r, num);
		exit (2);
	}
}
# endif

static void
put_block (long num)
{
	long r;
	
	if (tst)
	{
		return;
	}
	
	r = rwabs_xhdi (1, block_buf, 1024, num);
	if (r)
	{
		fprintf (stderr, "Fatal Write Error (%ld), block %ld\n", r, num);
		exit (2);
	}
}

/* END low level access */
/****************************************************************************/



static void
warn (void)
{
	char c;
	
	fprintf (stderr, "\n");
	fprintf (stderr, "WARNING: THIS WILL TOTALLY DESTROY ANY DATA ON %c:\n", DriveToLetter(drv));
	fprintf (stderr, "Are you ABSOLUTELY SURE you want to do this? (y/n) ");
	scanf ("%c", &c);
	fprintf (stderr, "\n");
	
	if (c == 'y' || c == 'Y')
	{
		return;
	}
	
	fprintf (stderr, "Aborted\n");
	exit (1);
}

int
main (int argc, char **argv)
{
	long err = 0;
	long r;
	char c;
	
	int i;
	int j;
	ulong ioff;
	ulong zone1;
	
	super_block	csblk;
	d_inode2	*ripn	= (d_inode2 *) block_buf;
	dir_struct	*dir	= (dir_struct *) block_buf;
	ushort		*srt	= (ushort *) block_buf;
	long		icount;
	long		zcount;
	
	
	fprintf (stderr, "Minix-compatible filesystem initializer v" VERSION "\n");
	fprintf (stderr, "\n");
	fprintf (stderr, "Copyright S N Henson 1991, 1992, 1993, 1994.\n");
	fprintf (stderr, "Copyright Frank Naumann 1998, 1999.\n");
	fprintf (stderr, "\n");
		
	
	/*
	 * check and initalize XHDI
	 */
	
	if (init_XHDI ())
	{
		fprintf (stderr, "No XHDI installed (or to old)");
		exit (1);
	}
	
	fprintf (stderr, "Found XHDI level %x.%x.\n\n", (XHDI_installed >> 8), (XHDI_installed & 0x00ff));
	
	
	/*
	 * Parse command-line options
	 */
	
	opterr = 0;
	while ((c = getopt (argc, argv, "b:B:n:i:I:S:tepPzZrRV?")) != EOF)
	{
		switch (c)
		{
			case 'b':
			case 'B':
			{
				fprintf (stderr, "Option b/B no longer supported");
				fprintf (stderr, ", XHDI tell us always the number of blocks.\n");
				break;
			}
			case 'n':
			{
				incr = atol (optarg);
				break;
			}
			case 'i':
			case 'I':
			{
				numinodes = atol (optarg);
				break;
			}
			case 'S':
			{
				sonly = 1;
				break;
			}
			case 't':
			{
				tst=  1;
				break;
			}
			case 'e':
			{
				fprintf (stderr, "Option e no longer supported");
				fprintf (stderr, ", use always XHDI.\n");
				break;			
			}
			case '?':
			{
				err = 1;
				break;
			}
			case 'p':
			case 'P':
			case 'z':
			case 'Z':
			{
				fprintf (stderr, "Option p/P/z/Z no longer supported");
				fprintf (stderr, ", use always XHDI.\n");
				break;
			}
			case 'r':
			case 'R':
			{
				fprintf (stderr, "Option r/R no longer supported");
				fprintf (stderr, ", not needed in XHDI mode.\n");
				break;
			}
			case 'V':
			{
				fprintf (stderr, "Option V no longer supported");
				fprintf (stderr, ", create always a V2 filesystem.\n");
				break;
			}
		}
	}
	
	
	/*
	 * Sanity checking time
	 */
	
	if ((incr < 1) || (incr > 16) || ((incr) & (incr - 1)))
	{
		fprintf (stderr, "Dirincrement must be a power of two between\n");
		fprintf (stderr, "1 and 16 (inclusive).\n");
		err = 1;
	}
	
	if ((numinodes < 0) || (numinodes > 65535))
	{
		fprintf (stderr, "Need at least 1 and no more than 65535 inodes.\n");
		err = 1;
	}
	
	if (argv [optind])
	{
		drv = DriveFromLetter(argv[optind][0]);
		
		if (drv > 31)
		{
			fprintf (stderr, "drive out of range.\n");
			err = 1;
		}
	}
	else
	{
		fprintf (stderr, "no drive specified.\n");
		err = 1;
	}
	
	if (err || argc-optind != 1)
	{
		fprintf (stderr, "\n");
		fprintf (stderr, "Usage\t(auto)\t: minit drive\n");
		fprintf (stderr, "\t(manual): minit -i inodes drive\n");
		fprintf (stderr, "Also :\t-S only write out super-block\n");
		fprintf (stderr, "\t-n dirincrement (default is 4)\n");
		fprintf (stderr, "\t-t test mode (no writing)\n");
		fprintf (stderr, "\n");
		exit (1);
	}
	
	
	/*
	 * get drv geometry
	 */
	
	fprintf (stderr, "\n");
	
	r = XHInqDev2 (drv, &major, &minor, &start, NULL, &sectors, id);
	if (r == 0)
	{
		r = XHInqTarget2 (major, minor, &ssize, NULL, NULL, 0);
	}
	if (r != 0)
	{
		fprintf (stderr, "unable to get geometry for '%c'\n", DriveToLetter(drv));
		exit (1);
	}
	
	id [3] = '\0';
	
	fprintf (stderr, "Information about drive %c:\n", DriveToLetter(drv));
	fprintf (stderr, "--------------------------\n");
	fprintf (stderr, "XHDI major number    : %d\n", major);
	fprintf (stderr, "XHDI minor number    : %d\n", minor);
	fprintf (stderr, "partition start      : %ld\n", start);
	fprintf (stderr, "partition length     : %ld sectors\n", sectors);
	fprintf (stderr, "phyiscal sector size : %ld bytes\n", ssize);
	
	if (id [0])
	{
		fprintf (stderr, "partition type       : TOS\n");
		fprintf (stderr, "partition ID         : %s\n", id);
	}
	else if (id [1] == 'D')
	{
		fprintf (stderr, "partition type       : DOS\n");
		fprintf (stderr, "partition ID         : 0x%x\n", (int) id [2]);
	}
	else
	{
		fprintf (stderr, "partition type       : UNKNOWN\n");
		fprintf (stderr, "partition ID         : %s\n", id);
	}
	
	fprintf (stderr, "\n");
	
	if (!(id [0] == 'R' && id [1] == 'A' && id [2] == 'W')
		&& !(id [0] == 'M' && id [1] == 'I' && id [2] == 'X'))
	{
		fprintf (stderr, "Can't create Minix filesystem - wrong partition ID!\n");
		fprintf (stderr, "Please change first partition ID to 'MIX' or 'RAW'.\n");
		fprintf (stderr, "See in documentation for more details.\n");
		fprintf (stderr, "\n");
		exit (1);
	}
	
	
	/*
	 * validate and setup internal data
	 */
	
	if (ssize == 512)
	{
		numblocks = sectors / 2;
	}
	else if (ssize == 1024)
	{
		numblocks = sectors;
	}
	else
	{
		fprintf (stderr, "Only physical sector sizes 512 and 1024 are supported.");
		exit (1);
	}
		
	if (numinodes == 0)
	{
		numinodes = numblocks / 3;
		
		/* Round up inode number to nearest block */
		numinodes = (numinodes + 15) & ~15;
	}
	
	if (numinodes > 65535)
	{
		numinodes = 65535;
	}
	
	fprintf (stderr, "Drive will be initialised with %ld Blocks and %ld Inodes.\n", numblocks, numinodes);
	fprintf (stderr, "Names can be %ld characters long (dir increment = %ld).\n", (incr << 4L) - 2, incr);
	
	
	/*
	 * lock the drv
	 */
	
	i = Dlock (1, drv);
	if (i && i != ENOSYS)
	{
		fprintf (stderr, "Can't lock %c:, drive in use?\n\n", DriveToLetter(drv));
		exit (1);
	}
	
	
	/*
	 * verify user permission
	 */
	
	if (!tst)
	{
		warn ();
	}
	else
	{
		fprintf (stderr, "\n");
	}
	
	
	/*
	 * create superblock
	 */
	
	bzero (block_buf, 1024l);
	
	/* Super block */
       	sblk->s_ninodes		= numinodes;
       	sblk->s_zones		= numblocks;
       	sblk->s_imap_blks	= (numinodes + 8192) / 8192;
	sblk->s_zmap_blks	= (numblocks + 8191) / 8192;
	sblk->s_firstdatazn	= 2 + sblk->s_imap_blks + sblk->s_zmap_blks + ((numinodes + 15) / 16);
	sblk->s_log_zsize	= 0;
	sblk->s_max_size	= 0x7fffffffl;
	sblk->s_magic		= 0x2468;
	sblk->s_state		= 0x1;
	
	/* save super block */
	csblk = *sblk;
	
	fprintf (stderr, "Writing superblock ... ");
	put_block (1);
	if (tst) fprintf (stderr, "{ test mode }.\n");
	else fprintf (stderr, "done.\n");
	
	if (sonly)
	{
		fprintf (stderr, "Leave ok (-S option).\n");
		exit (0);
	}
	
	
	/*
	 * create bitmaps
	 */
	
	ioff = 2 + csblk.s_imap_blks + csblk.s_zmap_blks;
	zone1 = csblk.s_firstdatazn;
	
	
	/*
	 * Inode bitmaps
	 */
	
	bzero (block_buf, 1024l);
	
	fprintf (stderr, "Writing inode bitmaps ... ");
	
	icount = numinodes + 1;
	for (i = 2; i < (2 + csblk.s_imap_blks); i++)
	{
		if (i == 2)
		{
			srt[0] = 3;
		}
		
		/* Need to mark dead inodes as used */
		if (icount < 8192)
		{
			if (icount & 15)
			{
				srt[icount/16] = 0xffff << (icount & 15);
				icount += 16 - (icount & 15);
			}
			for (j = icount / 16; j < 512; j++)
			{
				srt[j] = 0xffff;
			}
		}
		
		put_block (i);
		
		if (i == 2)
		{
			srt[0] = 0;
		}
		
		icount -= 8192;
	}
	
	if (tst) fprintf (stderr, "{ test mode }.\n");
	else fprintf (stderr, "done.\n");
	
	
	/*
	 * Zone bitmaps
	 */
	
	bzero (block_buf, 1024l);
	
	fprintf (stderr, "Writing zone bitmaps ... ");
	
	zcount = numblocks + 1 - csblk.s_firstdatazn;
	for (i = 2 + csblk.s_imap_blks; i < ioff; i++)
	{
		if (i == 2 + csblk.s_imap_blks)
		{
			srt[0] = 3;
		}
		
		/* Need to mark dead zones as used */
		if (zcount < 8192)
		{
			if (zcount < 0) zcount = 0;
			if (zcount & 15)
			{
				srt[zcount / 16] = 0xffff << (zcount & 15);
				zcount += 16 - (zcount & 15);
			}
			for (j = zcount / 16; j < 512; j++)
			{
				srt[j] = 0xffff;
			}
		}
		
		put_block (i);
		
		if (i == 2 + csblk.s_imap_blks)
		{
			srt[0] = 0;
		}
		
		zcount -= 8192;
	}
	
	if (tst) fprintf (stderr, "{ test mode }.\n");
	else fprintf (stderr, "done.\n");
	
	
	/*
	 * Initialise inodes
	 */
	
	bzero (block_buf, 1024l);
	
	fprintf (stderr, "Writing inodes ... ");
	
	for (i = ioff; i < ioff + ((numinodes + 15) / 16); i++)
	{
		if (i == ioff) /* Root inode, initialise it properly */
		{
			ripn->i_mode	= 040777;	/* Directory */
			ripn->i_size	= 32 * incr;
			ripn->i_mtime	= time ((time_t *) 0);
			ripn->i_ctime	= ripn->i_mtime;
			ripn->i_atime	= ripn->i_mtime;
			ripn->i_nlinks	= 2;
			ripn->i_zone[0]	= zone1;
		}	
		
		put_block (i);
		
		if (i == ioff)
		{
			bzero (block_buf, 1024l);
		}
	}
	
	if (tst) fprintf (stderr, "{ test mode }.\n");
	else fprintf (stderr, "done.\n");
	
	
	/*
	 * And finally the root directory
	 */
	
	bzero (block_buf, 1024l);
	
	dir[0].d_inum = 1;
	strcpy (dir[0].d_name, ".");
	dir[incr].d_inum = 1;
	strcpy (dir[incr].d_name, "..");
	
	fprintf (stderr, "Writing root dir ... ");
	put_block (zone1);
	if (tst) fprintf (stderr, "{ test mode }.\n");
	else fprintf (stderr, "done.\n");
	
	
	/*
	 * done
	 */
	
	fprintf (stderr, "\n");
	
	if (!tst)
	{
		fprintf (stderr, "Drive %c: successfully initialised\n", DriveToLetter(drv));
	}
	
	(void) Dlock (0, drv);
	
	fprintf (stderr, "Leave ok.\n");
	exit (0);
}
