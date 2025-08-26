/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1995  Andries E. Brouwer <aeb@cwi.nl>
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
 * Started:      2000-06-13
 * 
 * Changes:
 * 
 * 2000-06-13:
 * 
 * - inital version
 * 
 */

# include <ctype.h>
# include <errno.h>
# include <fcntl.h>
# include <getopt.h>
# include <stdarg.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <unistd.h>

# include <sys/stat.h>

# include "mytypes.h"
# include "bswap.h"


# define le2cpu16(x)	BSWAP16 (x)
# define le2cpu32(x)	BSWAP32 (x)
# define le2cpu64(x)	xxx error xxx

# define be2cpu16(x)	((__u16) (x))
# define be2cpu32(x)	((__u32) (x))
# define be2cpu64(x)	((__u64) (x))


# define PROGNAME	"fdisk"
# define VERSION	"1.0"
# define DATE		"2000-06-16"


# define SIZE(a)	(sizeof (a) / sizeof (a[0]))

/*
 * Table of contents:
 *  A. About seeking
 *  B. About sectors
 *  C. About heads, sectors and cylinders
 *  D. About system Ids
 *  E. About partitions
 *  F. The standard input
 *  G. The command line
 *  H. Listing the current situation
 *  I. Writing the new situation
 */
static int exit_status = 0;

static int force = 0;		/* 1: do what I say, even if it is stupid ... */
static int quiet = 0;		/* 1: suppress all warnings */
static int dump = 0;		/* 1: list in a format suitable for later input */
static int verify = 0;		/* 1: check that listed partition is reasonable */
static int no_write = 0;	/* 1: do not actually write to disk */
#if 0
static int no_reread = 0;	/* 1: skip the BLKRRPART ioctl test at startup */
#endif
static int opt_list = 0;
static char *save_sector_file = NULL;
static char *restore_sector_file = NULL;

/****************************************************************************/
/* BEGIN  */

static void
warn (char *s, ...)
{
	va_list p;
	
	va_start (p, s);
	fflush (stdout);
	if (!quiet)
		vfprintf (stderr, s, p);
	va_end (p);
}

static void
error (char *s, ...)
{
	va_list p;
	
	va_start (p, s);
	fflush (stdout);
	fprintf (stderr, "\n" PROGNAME ": ");
	vfprintf (stderr, s, p);
	va_end (p);
}

static void
fatal (char *s, ...)
{
	va_list p;
	
	va_start (p, s);
	fflush (stdout);
	fprintf (stderr, "\n" PROGNAME ": ");
	vfprintf (stderr, s, p);
	va_end (p);
	
	exit (1);
}

/* END  */
/****************************************************************************/

/****************************************************************************/
/* BEGIN I/O mapper */

# include "xhdi.h"
# include <assert.h>
# include <mintbind.h>

# define BLKRRPART	2
# define BLKGETSIZE	3
# define BLKSSZGET	4

static int16_t xhdi_drv = -1;
static u_int16_t xhdi_major = 0;
static u_int16_t xhdi_minor = 0;
static ulong xhdi_sectors = 0;
static ulong xhdi_ssize = 0;

static int64_t xhdi_pos = 0;

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')
#define DriveFromLetter(d) \
	(((d) >= 'A' && (d) <= 'Z') ? ((d) - 'A') : \
	 ((d) >= 'a' && (d) <= 'z') ? ((d) - 'a') : \
	 ((d) >= '1' && (d) <= '6') ? ((d) - '1' + 26) : \
	 -1)

static long
rwabs_xhdi (ushort rw, void *buf, ulong size, ulong recno)
{
	register ulong n = size / xhdi_ssize;
	
	if (!n || n > xhdi_sectors)
	{
		printf ("rwabs_xhdi: access outside partition (drv = %c:)\n", DriveToLetter(xhdi_drv));
		exit (2);
	}
	
	if (n > 65535UL)
	{
		printf ("rwabs_xhdi: n to large (drv = %c)\n", DriveToLetter(xhdi_drv));
		exit (2);
	}
	
	return XHReadWrite (xhdi_major, xhdi_minor, rw, recno, n, buf);
}

static int
_my_open (const char *_dev, int iomode)
{
	int16_t dev = DriveFromLetter(_dev[0]);
	long r;
	
	init_XHDI ();
	
	if (dev < 0 || dev > 31)
	{
		printf ("invalid device!\n");
		return -1;
	}
	
	if (xhdi_drv != -1)
	{
		printf ("can only open one device at a time!\n");
		return -1;
	}
	
	r = Dlock (1, dev);
	if (r && r != -32)
	{
		printf ("Can't lock %c:, drive in use?\n", DriveToLetter(dev));
		return -1;
	}
	
	(void) Getbpb (dev);
	
	r = XHInqDev2 (dev, &xhdi_major, &xhdi_minor, NULL, NULL, NULL, NULL);
	if ((r == -1) && (errno == EBUSY)) r = 0; /* major and minor are ok */
	if (!r) r = XHGetCapacity (xhdi_major, xhdi_minor, &xhdi_sectors, &xhdi_ssize);
	
	if (r)
	{
		printf ("unable to get geometry for '%c' (%li)\n", DriveToLetter(dev), r);
		return -1;
	}
	
# if 0
	printf ("Information about %c:\n", DriveToLetter(dev));
	printf ("---------------------\n");
	printf ("XHDI major number : %d\n", xhdi_major);
	printf ("XHDI minor number : %d\n", xhdi_minor);
	printf ("length            : %lu sectors\n", xhdi_sectors);
	printf ("sector size       : %lu bytes\n", xhdi_ssize);
	printf ("\n");
# endif
	
	/* mark as open */
	xhdi_drv = dev;
	
# if 0
	{
		char buf [512];
		long r = rwabs_xhdi (0, buf, 512, 0);
		if (!r)
		{
			int fd;
			fd = open ("dump", O_CREAT|O_RDWR);
			if (fd > 0)
			{
				printf ("dumping rootsektor ... ");
				if (write (fd, buf, 512) != 512)
					printf ("error\n");
				else
					printf ("OK\n");
				
				close (fd);
			}
			else
				perror ("open");
		}
	}
# endif
	
	return xhdi_drv;
}

static int
_my_close (int fd)
{
	if (fd != xhdi_drv)
	{
		__set_errno (EBADF);
		return -1;
	}
	
	(void) Dlock (0, xhdi_drv);
	xhdi_drv = -1;
	
	return 0;
}

static int
_my_ioctl (int fd, int cmd, void *arg)
{
	if (fd != xhdi_drv)
	{
		__set_errno (EBADF);
		return -1;
	}
	
	switch (cmd)
	{
		case BLKRRPART:
		{
			//return XHMediumChanged (xhdi_major, xhdi_minor);
			__set_errno (EPERM);
			return -1;
		}
		case BLKGETSIZE:
		{
			*(ulong *) arg = xhdi_sectors;
			return 0;
		}
		case BLKSSZGET:
		{
			*(ulong *) arg = xhdi_ssize;
			return 0;
		}
	}
	
	__set_errno (ENOSYS);
	return -1;
}

#pragma GCC diagnostic ignored "-Wcast-qual"

static ssize_t
_my_write (int fd, const void *buf, size_t size)
{
	u_int32_t recno;
	int32_t ret;
	
	if (fd != xhdi_drv)
	{
		__set_errno (EBADF);
		return -1;
	}
	
	assert ((size % xhdi_ssize) == 0);
	assert ((xhdi_pos % xhdi_ssize) == 0);
	
	recno = xhdi_pos / xhdi_ssize;
	
	ret = rwabs_xhdi (1, (void *) buf, size, recno);
	if (!ret)
	{
		xhdi_pos += size;
		return size;
	}
	
	return -1;
}

static ssize_t
_my_read (int fd, void *buf, size_t size)
{
	u_int32_t recno;
	int32_t ret;
	
	if (fd != xhdi_drv)
	{
		__set_errno (EBADF);
		return -1;
	}
	
	assert ((size % xhdi_ssize) == 0);
	assert ((xhdi_pos % xhdi_ssize) == 0);
	
	recno = xhdi_pos / xhdi_ssize;
	
	ret = rwabs_xhdi (0, buf, size, recno);
	if (!ret)
	{
		xhdi_pos += size;
		return size;
	}
	
	return -1;
}

static int64_t
io_seek (int fd, int64_t where, int64_t *out, int whence)
{
	if (fd != xhdi_drv)
	{
		__set_errno (EBADF);
		return -1;
	}
	
	switch (whence)
	{
		case SEEK_SET:
			break;
		case SEEK_CUR:
			where += xhdi_pos;
			break;
		case SEEK_END:
			where += (int64_t) xhdi_sectors * xhdi_ssize;
			break;
		default:
		{
			__set_errno (EBADARG);
			return -1;
		}
	}
	
	if (where % xhdi_ssize)
	{
		__set_errno (EBADARG);
		return -1;
	}
	
	if (where > (int64_t) xhdi_sectors * xhdi_ssize)
	{
		__set_errno (EBADARG);
		return -1;
	}
	
	*out = xhdi_pos = where;
	return 0;
}

static int
sseek (char *dev, int fd, ulong s)
{
	int64_t in, out;
	in = ((int64_t) s << 9);
	out = 1;
	
	if (io_seek (fd, in, &out, SEEK_SET) != 0)
	{
		perror ("llseek");
		error ("seek error on %s - cannot seek to %lu\n", dev, s);
		return 0;
	}
	
	if (in != out)
	{
		error ("seek error: wanted 0x%08x%08x, got 0x%08x%08x\n",
			(uint)(in>>32), (uint)(in & 0xffffffff),
			(uint)(out>>32), (uint)(out & 0xffffffff));
		return 0;
	}
	
	return 1;
}

/* END I/O mapper */
/****************************************************************************/

/****************************************************************************/
/* BEGIN */

/*
 *  B. About sectors
 */

/*
 * We preserve all sectors read in a chain - some of these will
 * have to be modified and written back.
 */
static struct sector
{
	struct sector *next;
	ulong sectornumber;
	int to_be_written;
	char data[512];
} *sectorhead;

static void
free_sectors(void)
{
	struct sector *s;
	
	while (sectorhead)
	{
		s = sectorhead;
		sectorhead = s->next;
		free (s);
	}
}

static struct sector *
get_sector (char *dev, int fd, ulong sno)
{
	struct sector *s;
	
	for (s = sectorhead; s; s = s->next)
		if (s->sectornumber == sno)
			return s;
	
	if (!sseek (dev, fd, sno))
		return 0;
	
	s = malloc (sizeof (*s));
	if (!s)
		fatal ("out of memory - giving up\n");
	
	if (_my_read (fd, s->data, sizeof (s->data)) != sizeof (s->data))
	{
		perror ("read");
		error ("read error on %s - cannot read sector %lu\n", dev, sno);
		free (s);
		
		return 0;
	}
	
	s->next = sectorhead;
	sectorhead = s;
	s->sectornumber = sno;
	s->to_be_written = 0;
	
	return s;
}

static int
write_sectors (char *dev, int fd)
{
	struct sector *s;
	
	for (s = sectorhead; s; s = s->next)
	{
		if (s->to_be_written)
		{
			if (!sseek (dev, fd, s->sectornumber))
				return 0;
			
			if (_my_write (fd, s->data, sizeof (s->data)) != sizeof (s->data))
			{
				perror ("write");
				error ("write error on %s - cannot write sector %lu\n",
					dev, s->sectornumber);
				
				return 0;
			}
			
			s->to_be_written = 0;
		}
	}
	
	return 1;
}

static void
ulong_to_chars (ulong u, char *uu)
{
	int i;
	
	for(i = 0; i < 4; i++)
	{
		uu[i] = (u & 0xff);
		u >>= 8;
	}
}

static ulong
chars_to_ulong (const uchar *uu)
{
	int i;
	ulong u = 0;
	
	for(i = 3; i >= 0; i--)
		u = (u << 8) | uu[i];
	
	return u;
}

static int
save_sectors (char *dev, int fdin)
{
	struct sector *s;
	char ss[516];
	int fdout;
	
	fdout = open (save_sector_file, O_WRONLY | O_CREAT, 0444);
	if (fdout < 0)
	{
		perror (save_sector_file);
		error ("cannot open partition sector save file (%s)\n",
			save_sector_file);
		return 0;
	}
	
	for (s = sectorhead; s; s = s->next)
	{
		if (s->to_be_written)
		{
			ulong_to_chars (s->sectornumber, ss);
			
			if (!sseek (dev, fdin, s->sectornumber))
				return 0;
			
			if (_my_read (fdin, ss+4, 512) != 512)
			{
				perror ("read");
				error ("read error on %s - cannot read sector %lu\n",
					dev, s->sectornumber);
				return 0;
			}
			
			if (write (fdout, ss, sizeof (ss)) != sizeof (ss))
			{
				perror ("write");
				error ("write error on %s\n", save_sector_file);
				return 0;
			}
		}
	}
	
	return 1;
}

static void reread_disk_partition (char *dev, int fd);

static int
restore_sectors (char *dev)
{
	int fdin, fdout, ct;
	struct stat statbuf;
	uchar *ss0, *ss;
	ulong sno;
	
	if (stat (restore_sector_file, &statbuf) < 0)
	{
		perror (restore_sector_file);
		error ("cannot stat partition restore file (%s)\n",
			restore_sector_file);
		return 0;
	}
	
	if (statbuf.st_size % 516)
	{
		error ("partition restore file has wrong size - not restoring\n");
		return 0;
	}
	
	ss = (uchar *) malloc (statbuf.st_size);
	if (!ss)
	{
		error ("out of memory?\n");
		return 0;
	}
	
	fdin = open (restore_sector_file, O_RDONLY);
	if (fdin < 0)
	{
		perror (restore_sector_file);
		error ("cannot open partition restore file (%s)\n",
			restore_sector_file);
		return 0;
	}
	
	if (read (fdin, ss, statbuf.st_size) != statbuf.st_size)
	{
		perror ("read");
		error ("error reading %s\n", restore_sector_file);
		return 0;
	}
	
	fdout = _my_open (dev, O_WRONLY);
	if (fdout < 0)
	{
		perror (dev);
		error ("cannot open device %s for writing\n", dev);
		return 0;
	}
	
	ss0 = ss;
	ct = statbuf.st_size / 516;
	while (ct--)
	{
		sno = chars_to_ulong (ss);
		
		if (!sseek (dev, fdout, sno))
			return 0;
		
		if (_my_write (fdout, ss+4, 512) != 512)
		{
			perror (dev);
			error ("error writing sector %lu on %s\n", sno, dev);
			return 0;
		}
		
		ss += 516;
	}
	
	free (ss0);
	reread_disk_partition (dev, fdout);
	
	return 1;
}

/* END */
/****************************************************************************/

/****************************************************************************/
/* BEGIN */

/*
 *  C. About sectors
 */

static ulong sectorsize;
static ulong sectors;
static ulong specified_sectors;

static void
get_sectors (char *dev, int fd, int silent)
{
	int ioctl_ok = 0;
	ulong mysectors;
	ulong mysectorsize;
	
	sectorsize = 0;
	sectors = 0;
	
	if (!_my_ioctl (fd, BLKGETSIZE, &mysectors))
	{
		if (!_my_ioctl (fd, BLKSSZGET, &mysectorsize))
		{
			sectors = mysectors;
			sectorsize = mysectorsize;
			
			ioctl_ok = 1;
		}
	}
	
	if (specified_sectors)
		sectors = specified_sectors;
	
	if (ioctl_ok)
	{
		if (sectors != mysectors)
			warn ("Warning: BLKGETSIZE says that there are %d sectors\n",
				mysectors);
		if (sectorsize != mysectorsize)
			warn ("Warning: BLKSSZGET says that that the sectorsize is %d bytes\n",
				mysectorsize);
	}
	else if (!silent)
	{
		if (!sectors)
			printf ("Disk %s: cannot get geometry\n", dev);
	}
	
	if (!silent)
		printf ("\nDisk %s: %lu sectors\n", dev, sectors);
}

/* END */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definitions */

struct partition
{
	__u8	flg;			/* bit 0: valid; bit 7: bootable */
# define PART_VALID	0x1
# define PART_BOOTABLE	0x80
	__u8	id[3];			/* "GEM", "BGM", "XGM", or other */
	__u32	start;			/* start of partition */
	__u32	size;			/* length of partition */
}
__attribute__ ((packed));

struct rootsector
{
	__u8	unused[0x1C2];		/* room for boot code */
	__u32	hd_size;		/* size of disk in blocks */
	struct partition part[4];
	__u32	bsl_start;		/* start of bad sector list */
	__u32	bsl_cnt;		/* length of bad sector list */
	__u16	checksum;		/* checksum for bootable disks */
}
__attribute__ ((packed));

/* END definitions */
/****************************************************************************/

/****************************************************************************/
/* BEGIN */

/*
 *  D. About system Ids
 */

/*
 * List of system Id's
 */
static struct systypes
{
	uchar type [3];
	char *name;
}
sys_types [] =
{
	{ { 'G', 'E', 'M' },	"TOS 16-bit FAT <=16M" },
	{ { 'B', 'G', 'M' },	"TOS 16-bit FAT >16M" },
	{ { 'X', 'G', 'M' },	"TOS Extended" },
	{ { 'F', '3', '2' },	"TOS FAT32 (Win95 style)" },
	{ { 'R', 'A', 'W' },	"RAW partition" },
	{ { 'M', 'I', 'X' },	"Minix partition" },
	{ { 'L', 'N', 'X' },	"ext2 partition (Linux native)" },
	{ { 'U', 'N', 'X' },	"ASV (Atari System V R4)" },
	{ { 'S', 'W', 'P' },	"Swap" }
};

static const char *
sysname (struct partition *p)
{
	struct systypes *s;
	
	if (!(p->flg & PART_VALID))
		return "Empty";
	
	for (s = sys_types; s - sys_types < SIZE (sys_types); s++)
	{
		if (s->type[0] == p->id[0]
			&& s->type[1] == p->id[1]
			&& s->type[2] == p->id[2])
		{
			return s->name;
		}
	}
	
	return "Unknown";
}

static void
list_types (void)
{
	struct systypes *s;
	
	printf ("Id   Name\n\n");
	for (s = sys_types; s - sys_types < SIZE (sys_types); s++)
	{
		printf ("%c%c%c  %s\n",
			s->type[0],
			s->type[1],
			s->type[2],
			s->name);
	}
}

static void
print_id (__u8 *id)
{
	int i;
	
	for (i = 0; i < 3; i++)
	{
		if (id[i] < 32 || id[i] >= 128)
			putchar ('?');
		else
			putchar (id[i]);
	}
}

static int
is_xgm (__u8 *id)
{
	return (id[0] == 'X' && id[1] == 'G' && id[2] == 'M');
}

/* END */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definitions */

struct part_desc
{
	ulong start;
	ulong size;
	ulong sector, offset;		/* disk location of this info */
	struct partition p;
	struct part_desc *ep;		/* extended partition containing this one */
}
zero_part_desc;

#if 0
static struct part_desc *
outer_extended_partition (struct part_desc *p)
{
	while (p->ep)
		p = p->ep;
	
	return p;
}

static int
is_parent (struct part_desc *pp, struct part_desc *p)
{
	while (p)
	{
		if (pp == p)
			return 1;
		
		p = p->ep;
	}
	
	return 0;
}
#endif

struct disk_desc
{
    struct part_desc partitions[128];
    int partno;
}
oldp, newp;

#if 0

/* determine where on the disk this information goes */
static void
add_sector_and_offset (struct disk_desc *z)
{
	int pno;
	struct part_desc *p;
	
	for (pno = 0; pno < z->partno; pno++)
	{
		p = &(z->partitions[pno]);
		p->offset = 0x1be + (pno % 4) * sizeof (struct partition);
		p->sector = (p->ep ? p->ep->start : 0);
	}
}
#endif

/* tell the kernel to reread the partition tables */
static int
reread_ioctl (int fd)
{
	if (_my_ioctl (fd, BLKRRPART, NULL))
	{
		perror ("BLKRRPART");
		return -1;
	}
	
	return 0;
}

/* reread after writing */
static void
reread_disk_partition (char *dev, int fd)
{
	printf ("Re-reading the partition table ...\n");
	fflush (stdout);
	sync ();
	
	if (reread_ioctl (fd))
		printf ("The command to re-read the partition table failed\n"
			"Reboot your system now, before using mkfs\n");
	
	if (_my_close (fd))
	{
		perror (dev);
		printf ("Error closing %s\n", dev);
	}
	
	printf ("\n");
}

/* find Linux name of this partition, assuming that it will have a name */
static int
index_to_tos (int pno, struct disk_desc *z)
{
	int i, ct = 1;
	struct part_desc *p = &(z->partitions[0]);
	for (i = 0; i < pno; i++, p++)
		if (p->size > 0 && !is_xgm (p->p.id))
			ct++;
	return ct;
}

static int
tos_to_index (int lpno, struct disk_desc *z)
{
	int i, ct = 0;
	struct part_desc *p = &(z->partitions[0]);
	for (i = 0; i < z->partno && ct < lpno; i++, p++)
		if ((p->size > 0 && !is_xgm (p->p.id)) && ++ct == lpno)
			return i;
	return -1;
}

static int
asc_to_index (char *pnam, struct disk_desc *z)
{
	int pnum, pno;
	
	if (*pnam == '#')
		pno = atoi (pnam+1);
	else
	{
		pnum = atoi (pnam);
		pno = tos_to_index (pnum, z);
	}
	
	if (!(pno >= 0 && pno < z->partno))
		fatal ("%s: no such partition\n", pnam);
	
	return pno;
}

/*
 * List partitions - in terms of sectors, blocks or cylinders
 */
# define F_SECTOR   1
# define F_MEGABYTE 4

static int default_format = F_MEGABYTE;
static int specified_format = 0;
static int show_extended = 0;
static int one_only = 0;
#if 0
static int one_only_pno;
#endif
static int increment = 0;

static void
set_format (char c)
{
	switch(c)
	{
		default:
			printf ("unrecognized format - using sectors\n");
		case 'S':
			specified_format = F_SECTOR;
			break;
		case 'M':
			specified_format = F_MEGABYTE;
			break;
	}
}

static ulong
unitsize (int format)
{
	default_format = F_MEGABYTE;
	
	if (!format && !(format = specified_format))
		format = default_format;
	
	switch (format)
	{
		default:
		case F_SECTOR:
			return 1;
		case F_MEGABYTE:
			return 2048;
	}
}

static ulong
get_disksize (int format)
{
	ulong s = sectors;
	
	/* if (s && leave_last)
		ss--; */
	
	return (s * sectorsize) / unitsize (format);
}

static void
out_partition_header (char *dev, int format)
{
	if (dump)
	{
		printf ("# partition table of %s\n", dev);
		printf ("unit: sectors\n\n");
		return;
	}
	
	default_format = F_MEGABYTE;
	if (!format && !(format = specified_format))
		format = default_format;
	
	switch (format)
	{
		default:
			printf ("unimplemented format - using %s\n", "sectors");
		/* fall through */
		case F_SECTOR:
		{
			printf ("Units = sectors of 512 bytes, counting from %d\n\n", increment);
			printf ("   Device Boot    Start       End  #sectors  Id  System\n");
			break;
		}
		case F_MEGABYTE:
		{
			printf ("Units = megabytes of 1048576 bytes, blocks of 1024 bytes, counting from %d\n\n", increment);
			printf ("   Device Boot Start   End     MB   #blocks   Id  System\n");
			break;
		}
    }
}

static void
out_rounddown (int width, ulong n, ulong unit, int inc)
{
	printf ("%*lu", width, inc + n/unit);
	if (unit != 1)
		putchar ((n % unit) ? '+' : ' ');
	putchar (' ');
}

static void
out_roundup (int width, ulong n, ulong unit, int inc)
{
	if (n == (ulong)(-1))
		printf ("%*s", width, "-");
	else
		printf ("%*lu", width, inc + n/unit);
	if (unit != 1)
		putchar (((n+1) % unit) ? '-' : ' ');
	putchar (' ');
}

static void
out_roundup_size (int width, ulong n, ulong unit)
{
	printf ("%*lu", width, (n+unit-1)/unit);
	if (unit != 1)
		putchar ((n % unit) ? '-' : ' ');
	putchar (' ');
}

static void
out_partition (char *dev, int format, struct part_desc *p, struct disk_desc *z)
{
	ulong start, end, size;
	int pno, lpno;
	
	if (!format && !(format = specified_format))
		format = default_format;
	
	pno = p - &(z->partitions[0]);		/* our index */
	lpno = index_to_tos (pno, z);		/* name of next one that has a name */
	if (pno == tos_to_index (lpno, z))	/* was that us? */
		printf ("%8s%-2u", dev, lpno);	/* yes */
	else if (show_extended)
		printf ("    -     ");
	else
		return;
	
	putchar (dump ? ':' : ' ');
	
	start = p->start;
	end = p->start + p->size - 1;
	size = p->size;
	
	if (dump)
	{
		printf (" start=%9lu, size=%8lu ", start, size);
		print_id (p->p.id);
		
		if (p->p.flg & PART_BOOTABLE)
			printf (", bootable");
		
		printf ("\n");
	}
	else
	{
		if (p->p.flg & PART_BOOTABLE)
			printf (" * ");
		else
			printf ("   ");
		
		switch (format)
		{
			default:
			/* fall through */
			case F_SECTOR:
			{
				out_rounddown (9, start, 1, increment);
				out_roundup (9, end, 1, increment);
				out_rounddown (9, size, 1, 0);
				break;
			}
			case F_MEGABYTE:
			{
				out_rounddown (5, start, 2048, increment);
				out_roundup (5, end, 2048, increment);
				out_roundup_size (5, size, 2048);
				out_rounddown (8, size, 2, 0);
				break;
			}
		}
		
		putchar (' ');
		print_id (p->p.id);
		printf (" %s\n", sysname (&p->p));
	}
}

static void
out_partitions (char *dev, struct disk_desc *z)
{
	if (z->partno)
	{
		int pno, format = 0;
		
		out_partition_header (dev, format);
		for (pno = 0; pno < z->partno; pno++)
		{
			out_partition (dev, format, &(z->partitions[pno]), z);
			
			if (show_extended && is_xgm (z->partitions[pno].p.id))
				printf ("\n");
		}
	}
	else
		printf ("No partitions found\n");
}

static int
disj (struct part_desc *p, struct part_desc *q)
{
	return
		((p->start + p->size <= q->start)
			|| (is_xgm (p->p.id)
				&& q->start + q->size <= p->start + p->size));
}

static char *
pnumber (struct part_desc *p, struct disk_desc *z)
{
	static char buf[20];
	int this, next;
	struct part_desc *p0 = &(z->partitions[0]);
	
	this = index_to_tos (p-p0, z);
	next = index_to_tos (p-p0+1, z);
	
	if (next > this)
		sprintf (buf, "%d", this);
	else
		sprintf (buf, "[%d]", this);
	
	return buf;
}

static int
partitions_ok (struct disk_desc *z)
{
	struct part_desc *partitions = &(z->partitions[0]), *p, *q;
	int partno = z->partno;
	
# define PNO(p) pnumber(p, z)
	
	/* Have at least 4 partitions been defined? */
	if (partno < 4)
	{
        	if (!partno)
			fatal ("no partition table present.\n");
		else
			fatal ("strange, only %d partitions defined.\n", partno);
		
		return 0;
	}
	
	/* Are the partitions of size 0 marked empty?
	 * And do they have start = 0? And bootable = 0?
	 */
	for (p = partitions; p - partitions < partno; p++)
	{
		if (p->size == 0)
		{
			if (p->p.flg & PART_VALID)
				warn ("Warning: partition %s has size 0 but is marked valid\n", PNO (p));
			else if (p->p.flg & PART_BOOTABLE)
				warn ("Warning: partition %s has size 0 and is bootable\n", PNO (p));
			else if (p->p.start != 0)
				warn ("Warning: partition %s has size 0 and nonzero start\n", PNO (p));
			
			/* all this is probably harmless, no error return */
		}
	}
	
	/* Are the logical partitions contained in their extended partitions? */
	for (p = partitions+4; p < partitions+partno; p++)
	{
		if (p->size && !is_xgm (p->p.id))
		{
			q = p->ep;
			if (p->start < q->start || p->start + p->size > q->start + q->size)
			{
				warn ("Warning: partition %s ", PNO (p));
				warn ("is not contained in partition %s\n", PNO (q));
				return 0;
			}
		}
	}
	
	/* Are the data partitions mutually disjoint? */
	for (p = partitions; p < partitions+partno; p++)
	{
		if (p->size && !is_xgm (p->p.id))
		{
			for (q = p+1; q < partitions+partno; q++)
			{
				if (q->size && !is_xgm (q->p.id))
				{
					if (!((p->start > q-> start) ? disj (q,p) : disj (p,q)))
					{
						warn ("Warning: partitions %s ", PNO (p));
						warn ("and %s overlap\n", PNO (q));
						return 0;
					}
				}
			}
		}
	}
	
	/* Do they start past zero and end before end-of-disk? */
	{
		ulong ds = get_disksize (F_SECTOR);
		for (p = partitions; p < partitions+partno; p++)
		{
			if (p->size)
			{
				if (p->start == 0)
				{
					warn ("Warning: partition %s starts at sector 0\n", PNO (p));
					return 0;
				}
				
				if (p->size && p->start + p->size > ds)
				{
					warn ("Warning: partition %s extends past end of disk\n", PNO (p));
					return 0;
				}
			}
		}
	}
	
	/* Exact one chain of XGM extended partitions ?
	 */
	{
		int ect = 0;
		for (p = partitions; p < partitions+4; p++)
		{
			if (is_xgm (p->p.id))
				ect++;
			
			if (ect > 1)
			{
				warn ("Among the primary partitions, only one can be an xgm partition\n");
				return 0;
			}
		}
	}
	
	/*
	 */
	{
		int pno = -1;
		for (p = partitions; p < partitions+partno; p++)
		{
			if (p->p.flg & PART_BOOTABLE)
			{
				if (pno == -1)
					pno = p - partitions;
				else if (p - partitions < 4)
				{
					warn ("Warning: more than one primary partition is marked "
						"bootable (active)\n");
					break;
				}
				
				if (p - partitions >= 4)
				{
					warn ("Warning: a non-primary partition is marked as bootable.\n");
					break;
				}
			}
		}
		
		if (pno == -1 || pno >= 4)
			warn ("Warning: no primary partition is marked bootable (active)\n"
				"TOS will not boot from this disk.\n");
	}
	
	return 1;
	
# undef PNO
}

static void
xgm_partition (char *dev, int fd, struct part_desc *ep, struct disk_desc *z)
{
	struct part_desc *partitions = &(z->partitions[0]);
	ulong start, here, next;
	int pno = z->partno;
	int moretodo;
	
	here = start = ep->start;
	
	moretodo = 1;
	while (moretodo)
	{
		struct rootsector *root;
		struct partition *p;
		struct sector *s;
		int i;
		
		moretodo = 0;
		
		s = get_sector (dev, fd, here);
		if (!s) break;
		
		root = (struct rootsector *) s->data;
		p = (struct partition *) &root->part[0];
		
		if ((pno + 4) >= SIZE (z->partitions))
		{
			printf ("too many partitions - ignoring those past nr (%d)\n", pno-1);
			break;
		}
		
		next = 0;
		
		for (i = 0; i < 4; i++, p++)
		{
			if (!(p->flg & PART_VALID))
				continue;
			
			partitions[pno].sector = here;
			partitions[pno].offset = ((char *) p) - s->data;
			partitions[pno].ep = ep;
			
			if (is_xgm (p->id))
			{
				if (next)
					printf ("tree of partitions?\n");
				
				next = start + be2cpu32 (p->start);
				partitions[pno].start = next;
				
				moretodo = 1;
			}
			else
				partitions[pno].start = here + be2cpu32 (p->start);
			
			partitions[pno].size = be2cpu32 (p->size);
			partitions[pno++].p = *p;
        	}
        	
		here = next;
	}
	
	z->partno = pno;
}

static int
msdos_signature (struct sector *s)
{
	if (*(ushort *) (s->data + 0x1fe) != le2cpu16 (0xaa55))
		return 0;
	
	return 1;
}

static int
tos_partition (char *dev, int fd, ulong start, struct disk_desc *z)
{
	struct rootsector *root;
	struct partition *p;
	struct sector *s;
	struct part_desc *partitions = &(z->partitions[0]);
	int ret = 0;
	int xgm = 0;
	int pno;
	int i;
	
	if (start != 0)
		return 0;
	
	s = get_sector (dev, fd, start);	
	if (!s) return 0;
	
	if (msdos_signature (s))
		return 0;
	
	root = (struct rootsector *) s->data;
	p = (struct partition *) &root->part[0];
	
	for (pno = 0; pno < 4; pno++)
	{
		partitions[pno].sector = start;
		partitions[pno].offset = ((char *) p) - s->data;
		partitions[pno].start = start + be2cpu32 (p->start);
		partitions[pno].size = be2cpu32 (p->size);
		partitions[pno].ep = 0;
		partitions[pno].p = *p++;
		
		if (p->flg & PART_VALID)
			ret = 1;
	}
	
	z->partno = pno;
	
	for (i = 0; i < 4; i++)
	{
		if (is_xgm (partitions[i].p.id))
		{
			if (!xgm)
				xgm = 1;
			else
				printf ("strange..., more than one XGM partition chain?\n");
			
			if (!partitions[i].size)
			{
				printf ("strange..., an xgm partition of size 0?\n");
				continue;
			}
			
			xgm_partition (dev, fd, &partitions[i], z);
		}
	}
	
	return ret;
}

static void
get_partitions (char *dev, int fd, struct disk_desc *z)
{
	z->partno = 0;
	
	if (!tos_partition (dev, fd, 0, z))
	{
		printf (" %s: unrecognized partition\n", dev);
		return;
	}
}

static int
write_partitions (char *dev, int fd, struct disk_desc *z)
{
	struct part_desc *partitions = &(z->partitions[0]);
	struct part_desc *p;
	int pno = z->partno;
	
	if (no_write)
	{
		printf ("-n flag was given: Nothing changed\n");
		exit (0);
	}
	
	for (p = partitions; p < partitions + pno; p++)
	{
		struct sector *s;
		struct partition *part;
		
		s = get_sector (dev, fd, p->sector);
		if (!s) return 0;
		
		s->to_be_written = 1;
		part = (struct partition *)(&(s->data[p->offset]));
		*part = p->p;
	}
	
	if (save_sector_file)
	{
		if (!save_sectors (dev, fd))
		{
			fatal ("Failed saving the old sectors - aborting\n");
			return 0;
		}
	}
	
	if (!write_sectors (dev, fd))
	{
		error ("Failed writing the partition on %s\n", dev);
		return 0;
	}
	
	return 1;
}

/*
 *  F. The standard input
 */

/*
 * Input format:
 * <start> <size> <type> <bootable>
 * Fields are separated by whitespace or comma or semicolon possibly
 * followed by whitespace; initial and trailing whitespace is ignored.
 * Numbers can be octal, decimal or hexadecimal, decimal is default
 * Bootable is specified as [*|-], with as default not-bootable.
 * Type is given in ASCII.
 * The default value of start is the first nonassigned sector
 * The default value of size is as much as possible (until next
 * partition or end-of-disk).
 * 
 * On interactive input an empty line means: all defaults.
 * Otherwise empty lines are ignored.
 */

# if 0

static int eof, eob;

static struct dumpfld
{
    int fldno;
    char *fldname;
    int is_bool;
}
dumpflds[] =
{
	{ 0, "start", 0 },
	{ 1, "size", 0 },
	{ 2, "Id", 0 },
	{ 3, "bootable", 1 }
};

/*
 * Read a line, split it into fields
 *
 * (some primitive handwork, but a more elaborate parser seems
 *  unnecessary)
 */
# define RD_EOF	(-1)
# define RD_CMD	(-2)

static int
read_stdin (char **fields, char *line, int fieldssize, int linesize)
{
	char *lp, *ip;
	int c, fno;
	
	/* boolean true and empty string at start */
	line[0] = '*';
	line[1] = 0;
	for (fno = 0; fno < fieldssize; fno++)
		fields[fno] = line + 1;
	fno = 0;
	
	/* read a line from stdin */
	lp = fgets (line+2, linesize, stdin);
	if (!lp)
	{
		eof = 1;
		return RD_EOF;
	}
	
	lp = strchr (lp, '\n');
	if (!lp)
		fatal ("long or incomplete input line - quitting\n");
	
	*lp = 0;
	
	/* remove comments, if any */
	lp = strchr (line+2, '#');
	if (lp) *lp = 0;
	
	/* recognize a few commands - to be expanded */
	if (!strcmp (line+2, "unit: sectors"))
	{
		specified_format = F_SECTOR;
		return RD_CMD;
	}
	
	/* dump style? - then bad input is fatal */
	ip = strchr (line+2, ':');
	if (ip)
	{
		struct dumpfld *d;
		
nxtfld:
		ip++;
		while (isspace (*ip))
			ip++;
		
		if (*ip == 0)
			return fno;
		
		for (d = dumpflds; d-dumpflds < SIZE (dumpflds); d++)
		{
			if (!strncmp (ip, d->fldname, strlen (d->fldname)))
			{
				ip += strlen (d->fldname);
				
				while (isspace (*ip))
					ip++;
				
				if (d->is_bool)
					fields[d->fldno] = line;
				else if (*ip == '=')
				{
					while (isspace (*++ip))
						;
					
					fields[d->fldno] = ip;
					while (isalnum (*ip)) 	/* 0x07FF */
						ip++;
				}
				else
					fatal ("input error: `=' expected after %s field\n", d->fldname);
				
				if (fno <= d->fldno)
					fno = d->fldno + 1;
				if (*ip == 0)
					return fno;
				if (*ip != ',' && *ip != ';')
					fatal ("input error: unexpected character %c after %s field\n", *ip, d->fldname);
				*ip = 0;
				goto nxtfld;
			}
		}
		
		fatal ("unrecognized input: %s\n", ip);
	}
	
	/* split line into fields */
	lp = ip = line+2;
	fields[fno++] = lp;
	while ((c = *ip++) != 0)
	{
		if (!lp[-1] && (c == '\t' || c == ' '))
			;
		else if (c == '\t' || c == ' ' || c == ',' || c == ';')
		{
			*lp++ = 0;
			if (fno < fieldssize)
				fields[fno++] = lp;
			continue;
		}
		else
			*lp++ = c;
	}
	
	if (lp == fields[fno-1])
		fno--;
	
	return fno;
}

/* read a number, use default if absent */
static int
get_ul (char *u, ulong *up, ulong def, int base)
{
	char *nu;
	
	if (*u)
	{
		errno = 0;
		
		*up = strtoul (u, &nu, base);
		if (errno == ERANGE)
		{
			printf ("number too big\n");
			return -1;
		}
		
		if (*nu)
		{
			printf ("trailing junk after number\n");
			return -1;
		}
	}
	else
		*up = def;
	
	return 0;
}

/* There are two common ways to structure extended partitions:
   as nested boxes, and as a chain. Sometimes the partitions
   must be given in order. Sometimes all logical partitions
   must lie inside the outermost extended partition.
NESTED: every partition is contained in the surrounding partitions
   and is disjoint from all others.
CHAINED: every data partition is contained in the surrounding partitions
   and disjoint from all others, but extended partitions may lie outside
   (insofar as allowed by all_logicals_inside_outermost_extended).
ONESECTOR: all data partitions are mutually disjoint; extended partitions
   each use one sector only (except perhaps for the outermost one).
*/
int partitions_in_order = 0;
int all_logicals_inside_outermost_extended = 1;
enum { NESTED, CHAINED, ONESECTOR } boxes = NESTED;

/* find the default value for <start> - assuming entire units */
ulong
first_free(int pno, int is_xgm, struct part_desc *ep, int format,
	   ulong mid, struct disk_desc *z) {
    ulong ff, fff;
    ulong unit = unitsize(format);
    struct part_desc *partitions = &(z->partitions[0]), *pp = 0;

    /* if containing ep undefined, look at its container */
    if (ep && ep->p.sys_type == EMPTY_PARTITION)
      ep = ep->ep;

    if (ep) {
	if (boxes == NESTED || (boxes == CHAINED && !is_xgm))
	  pp = ep;
	else if (all_logicals_inside_outermost_extended)
	  pp = outer_extended_partition(ep);
    }
#if 0
    ff = pp ? (pp->start + unit - 1) / unit : 0;
#else
    /* rounding up wastes almost an entire cylinder - round down
       and leave it to compute_start_sect() to fix the difference */
    ff = pp ? pp->start / unit : 0;
#endif
    /* MBR and 1st sector of an extended partition are never free */
    if (unit == 1)
      ff++;

  again:
    for(pp = partitions; pp < partitions+pno; pp++) {
	if (!is_parent(pp, ep) && pp->size > 0) {
	    if ((partitions_in_order || pp->start / unit <= ff
		                     || (mid && pp->start / unit <= mid))
		&& (fff = (pp->start + pp->size + unit - 1) / unit) > ff) {
		ff = fff;
		goto again;
	    }
	}
    }

    return ff;
}

/* find the default value for <size> - assuming entire units */
static ulong
max_length (int pno, int is_xgm, struct part_desc *ep, int format,
		ulong start, struct disk_desc *z)
{
	ulong fu;
	ulong unit = unitsize(format);
	struct part_desc *partitions = &(z->partitions[0]), *pp = 0;
	
	/* if containing ep undefined, look at its container */
	if (ep && ep->p.sys_type == EMPTY_PARTITION)
		ep = ep->ep;
	
	if (ep)
	{
		if (boxes == NESTED || (boxes == CHAINED && !is_xgm))
			pp = ep;
		else if (all_logicals_inside_outermost_extended)
			pp = outer_extended_partition (ep);
	}
	
	fu = pp ? (pp->start + pp->size) / unit : get_disksize (format);
	for (pp = partitions; pp < partitions+pno; pp++)
	{
		if (!is_parent(pp, ep) && pp->size > 0
			&& pp->start / unit >= start && pp->start / unit < fu)
		{
			fu = pp->start / unit;
		}
	}
	
	return (fu > start) ? fu - start : 0;
}

/* compute starting sector of a partition inside an extended one */
/* ep is 0 or points to surrounding extended partition */
static int
compute_start_sect (struct part_desc *p, struct part_desc *ep)
{
	ulong base;
	int inc = (DOS && sectors) ? sectors : 1;
	int delta;
	
	if (ep && p->start + p->size >= ep->start + 1)
		delta = p->start - ep->start - inc;
	else if (p->start == 0 && p->size > 0)
		delta = -inc;
	else
		delta = 0;
	
	if (delta < 0)
	{
		p->start -= delta;
		p->size += delta;
		if (is_xgm (p->p.sys_type) && boxes == ONESECTOR)
			p->size = inc;
		else if ((int)(p->size) <= 0)
		{
			warn ("no room for partition descriptor\n");
			return 0;
		}
	}
	
	base = (!ep ? 0
	        : (is_xgm (p->p.id) ?
		   outer_extended_partition (ep) : ep)->start);
	
	p->ep = ep;
	if (p->p.sys_type == EMPTY_PARTITION && p->size == 0)
	{
        	p->p.start = 0;
	}
	else
	{
        	p->p.start = be2cpu32 (p->start - base);
	}
	
	p->p.size = be2cpu32 (p->size);
	return 1;
}    

/* build the extended partition surrounding a given logical partition */
int
build_surrounding_extended(struct part_desc *p, struct part_desc *ep,
			   struct disk_desc *z) {
    int inc = (DOS && sectors) ? sectors : 1;
    int format = F_SECTOR;
    struct part_desc *p0 = &(z->partitions[0]), *eep = ep->ep;

    if (boxes == NESTED) {
	ep->start = first_free(ep-p0, 1, eep, format, p->start, z);
	ep->size = max_length(ep-p0, 1, eep, format, ep->start, z);
	if (ep->start > p->start || ep->start + ep->size < p->start + p->size) {
	    warn("cannot build surrounding extended partition\n");
	    return 0;
	}
    } else {
	ep->start = p->start;
	if(boxes == CHAINED)
	  ep->size = p->size;
	else
	  ep->size = inc;
    }

    ep->p.nr_sects = be2cpu32 (ep->size);
    ep->p.bootable = 0;
    ep->p.sys_type = EXTENDED_PARTITION;
    if (!compute_start_sect(ep, eep) || !compute_start_sect(p, ep)) {
	ep->p.sys_type = EMPTY_PARTITION;
	ep->size = 0;
	return 0;
    }

    return 1;
}

static int
read_line (int pno, struct part_desc *ep, char *dev, int interactive,
		struct disk_desc *z)
{
	uchar line[1000];
	char *fields[11];
	int fno, pct = pno%4;
	struct part_desc p, *orig;
	ulong ff, ff1, ul, ml, ml1, def;
	int format, lpno, is_extd;
	
	if (eof || eob)
		return -1;
	
	lpno = index_to_tos(pno, z);
	
	if (interactive)
	{
		if (pct == 0 && (show_extended || pno == 0))
			warn ("\n");
		warn ("%8s%d: ", dev, lpno);
	}
	
	/* read input line - skip blank lines when reading from a file */
	do {
		fno = read_stdin (fields, line, SIZE (fields), SIZE (line));
	}
	while (fno == RD_CMD || (fno == 0 && !interactive));
	
	if (fno == RD_EOF)
	{
		return -1;
	}
	else if (fno > 10 && *(fields[10]) != 0)
	{
		printf ("too many input fields\n");
		return 0;
	}
	
	if (fno == 1 && !strcmp (fields[0], "."))
	{
		eob = 1;
		return -1;
	}
	
	format = 0;
	orig = (one_only ? &(oldp.partitions[pno]) : 0);
	
	p = zero_part_desc;
	p.ep = ep;
	
	/* first read the type - we need to know whether it is extended
	 * stop reading when input blank (defaults) and all is full
	 */
	is_extd = 0;
	if (fno == 0)
	{
		/* empty line */
		
		if (orig && is_xgm (orig->p.id))
			is_extd = 1;
		
		ff = first_free (pno, is_extd, ep, format, 0, z);
		ml = max_length (pno, is_extd, ep, format, ff, z);
		if (ml == 0 && is_extd == 0)
		{
			is_extd = 1;
			ff = first_free (pno, is_extd, ep, format, 0, z);
			ml = max_length (pno, is_extd, ep, format, ff, z);
		}
		
		if (ml == 0 && pno >= 4)
		{
			/* no free blocks left - don't read any further */
			warn ("No room for more\n");
			return -1;
		}
	}
	
	if (fno < 3 || !*(fields[2]))
		ul = orig ? orig->p.sys_type :
			(is_extd || (pno > 3 && pct == 1 && show_extended))
			? EXTENDED_PARTITION : LINUX_NATIVE;
	else if (get_ul (fields[2], &ul, LINUX_NATIVE, 16))
		return 0;
	
	if (ul > 255)
	{
		warn ("Illegal type\n");
		return 0;
	}
	
	p.p.sys_type = ul;
	is_extd = is_xgm (ul);
	
	/* find start */
	ff = first_free(pno, is_extd, ep, format, 0, z);
	ff1 = ff * unitsize(format);
	def = orig ? orig->start : (pno > 4 && pct > 1) ? 0 : ff1;
	if (fno < 1 || !*(fields[0]))
		p.start = def;
	else
	{
		if (get_ul (fields[0], &ul, def / unitsize (0), 0))
			return 0;
		p.start = ul * unitsize (0);
		p.start -= (p.start % unitsize (format));
	}
	
	/* find length */
	ml = max_length(pno, is_extd, ep, format, p.start / unitsize(format), z);
	ml1 = ml * unitsize(format);
	
	def = orig ? orig->size : (pno > 4 && pct > 1) ? 0 : ml1;
	if (fno < 2 || !*(fields[1]))
		p.size = def;
	else
	{
		if (get_ul (fields[1], &ul, def / unitsize (0), 0))
			return 0;
		p.size = ul * unitsize(0) + unitsize (format) - 1;
		p.size -= (p.size % unitsize (format));
	}
	
	if (p.size > ml1)
	{
		warn ("Warning: exceeds max allowable size (%lu)\n", ml1 / unitsize (0));
		if (!force)
			return 0;
	}
	
	if (p.size == 0 && pno >= 4 && (fno < 2 || !*(fields[1])))
	{
		warn ("Warning: empty partition\n");
		if (!force)
			return 0;
	}
	p.p.size = be2cpu32 (p.size);
	
	if (p.size == 0 && !orig) {
		if (fno < 1 || !*(fields[0]))
			p.start = 0;
		if (fno < 3 || !*(fields[2]))
			p.p.sys_type = EMPTY_PARTITION;
	}
	
	if (p.start < ff1 && p.size > 0)
	{
		warn ("Warning: bad partition start (earliest %lu)\n",
			(ff1 + unitsize(0) - 1) / unitsize(0));
		if (!force)
			return 0;
	}
	
	if (fno < 4 || !*(fields[3]))
		ul = (orig ? orig->p.bootable : 0);
	else if (!strcmp (fields[3], "-"))
		ul = 0;
	else if (!strcmp (fields[3], "*") || !strcmp (fields[3], "+"))
		ul = 0x80;
	else
	{
    		warn ("unrecognized bootable flag - choose - or *\n");
    		return 0;
	}
	p.p.bootable = ul;
	
	if (ep && ep->p.sys_type == EMPTY_PARTITION)
	{
		if (!build_surrounding_extended (&p, ep, z))
			return 0;
	}
	else
		if (!compute_start_sect (&p, ep))
			return 0;
	
	if (pno > 3 && p.size && show_extended && p.p.sys_type != EMPTY_PARTITION
		&& (is_xgm (p.p.id) != (pct == 1)))
	{
		warn ("Extended partition not where expected\n");
		if (!force)
			return 0;
	}
	
	z->partitions[pno] = p;
	if (pno >= z->partno)
		z->partno += 4;		/* reqd for out_partition() */
	
	if (interactive)
		out_partition (dev, 0, &(z->partitions[pno]), z);
	
	return 1;
}

/* ep either points to the extended partition to contain this one,
 * or to the empty partition that may become extended or is 0
 */
static int
read_partition (char *dev, int interactive, int pno, struct part_desc *ep,
		struct disk_desc *z)
{
	struct part_desc *p = &(z->partitions[pno]);
	int i;
	
	if (one_only)
	{
		*p = oldp.partitions[pno];
		if (one_only_pno != pno)
			goto ret;
	}
	else if (!show_extended && pno > 4 && pno%4)
		goto ret;
	
	while (!(i = read_line (pno, ep, dev, interactive, z)))
		if (!interactive)
			fatal("bad input\n");
	
	if (i < 0)
	{
		p->ep = ep;
		return 0;
	}

ret:
    p->ep = ep;
    if (pno >= z->partno)
      z->partno += 4;
	return 1;
}

void
read_partition_chain(char *dev, int interactive, struct part_desc *ep,
		     struct disk_desc *z) {
    int i, base;

    eob = 0;
    while (1) {
	base = z->partno;
	if (base+4 > SIZE(z->partitions)) {
	    printf("too many partitions\n");
	    break;
	}
	for (i=0; i<4; i++)
	  if (!read_partition(dev, interactive, base+i, ep, z))
	    return;
	for (i=0; i<4; i++) {
	    ep = &(z->partitions[base+i]);
	    if (is_xgm(ep->p.sys_type) && ep->size)
	      break;
	}
	if (i == 4) {
	    /* nothing found - maybe an empty partition is going
	       to be extended */
	    if (one_only || show_extended)
	      break;
	    ep = &(z->partitions[base+1]);
	    if (ep->size || ep->p.sys_type != EMPTY_PARTITION)
	      break;
	}
    }
}

static void
read_input (char *dev, int interactive, struct disk_desc *z)
{
	int i;
	struct part_desc *partitions = &(z->partitions[0]), *ep;
	
	for (i = 0; i < SIZE (z->partitions); i++)
		partitions[i] = zero_part_desc;
	z->partno = 0;
	
	if (interactive)
		warn ("\
Input in the following format; absent fields get a default value.\n\
<start> <size> <type [E,S,L,X,hex]> <bootable [-,*]> <c,h,s> <c,h,s>\n\
Usually you only need to specify <start> and <size> (and perhaps <type>).\n\
");
	eof = 0;
	
	for (i = 0; i < 4; i++)
		read_partition (dev, interactive, i, 0, z);
	
	for (i = 0; i < 4; i++)
	{
		ep = partitions + i;
		if (is_xgm (ep->p.id) && ep->size)
			read_partition_chain (dev, interactive, ep, z);
	}
	
	add_sector_and_offset (z);
}
# endif

/* END */
/****************************************************************************/

/****************************************************************************/
/* BEGIN main */

static void
version (void)
{
	printf (PROGNAME " version " VERSION " (fnaumann@freemint.de, " DATE ")\n");
}

static void
usage (void)
{
	version ();
	printf ("Usage:\n\
	" PROGNAME " [options] device ...\n\
device: something like /dev/hda or /dev/sda\n\
useful options:\n\
    -s [or --show-size]: list size of a partition\n\
    -c [or --id]:        print or change partition Id\n\
    -l [or --list]:      list partitions of each device\n\
    -d [or --dump]:      idem, but in a format suitable for later input\n\
    -i [or --increment]: number sectors etc. from 1 instead of from 0\n\
    -uS, -uM:            accept/report in units of sectors/MB\n\
    -T [or --list-types]:list the known partition types\n\
    -R [or --re-read]:   make kernel reread partition table\n\
    -N# :                change only the partition with number #\n\
    -n :                 do not actually write to disk\n\
    -O file :            save the sectors that will be overwritten to file\n\
    -I file :            restore these sectors again\n\
    -v [or --version]:   print version\n\
    -? [or --help]:      print this message\n\
dangerous options:\n\
    -x [or --show-extended]: also list extended partitions on output\n\
                           or expect descriptors for them on input\n\
    -q  [or --quiet]:      suppress warning messages\n\
    You can override the detected geometry using:\n\
    -S# [or --sectors #]:  set the number of sectors to use\n\
    You can disable all consistency checking with:\n\
    -f  [or --force]:      do what I say, even if it is stupid\n\
");
	exit(1);
}

static void
activate_usage (char *progn)
{
	printf ("Usage:\n\
  %s device	 	list active partitions on device\n\
  %s device n1 n2 ...	activate partitions n1 ..., inactivate the rest\n\
  %s device		activate partition n, inactivate the other ones\n\
", progn, progn, PROGNAME " -An");
	exit (1);
}

static char short_opts[] = "cdfilnqsu:vx?1A::I:N:O:RS:TV";

# define PRINT_ID	0400
# define CHANGE_ID	01000

static const struct option long_opts[] =
{
	{ "change-id",		no_argument, NULL, 'c' + CHANGE_ID },
	{ "print-id",		no_argument, NULL, 'c' + PRINT_ID },
	{ "id",			no_argument, NULL, 'c' },
	{ "dump",		no_argument, NULL, 'd' },
	{ "force",		no_argument, NULL, 'f' },
	{ "increment",		no_argument, NULL, 'i' },
	{ "list",		no_argument, NULL, 'l' },
	{ "quiet",		no_argument, NULL, 'q' },
	{ "show-size",		no_argument, NULL, 's' },
	{ "unit",       	required_argument, NULL, 'u' },
	{ "version",		no_argument, NULL, 'v' },
	{ "show-extended",	no_argument, NULL, 'x' },
	{ "help",		no_argument, NULL, '?' },
	{ "one-only",		no_argument, NULL, '1' },
	{ "sectors",		required_argument, NULL, 'S' },
	{ "activate",		optional_argument, NULL, 'A' },
	{ "re-read",		no_argument, NULL, 'R' },
	{ "list-types",		no_argument, NULL, 'T' },
	{ "no-reread",		no_argument, NULL, 160 },
	{ "leave-last",		no_argument, NULL, 161 },
	{ NULL, 0, NULL, 0 }
};

static void do_list (char *dev, int silent);
static void do_size (char *dev, int silent);
static void do_fdisk (char *dev);
static void do_reread (char *dev);
static void do_change_id (char *dev, char *part, char *id);
static void do_activate (char **av, int ac, char *arg);

static int total_size;

int
main (int argc, char **argv)
{
	char *progn;
	int c;
	char *dev;
	int opt_size = 0;
	int opt_reread = 0;
	int activate = 0;
	int do_id = 0;
	int fdisk = 0;
	char *activatearg = 0;
	
	
	if (argc < 1)
		fatal ("no command?\n");
	
	progn = strrchr(argv[0], '/');
	if (!progn)
		progn = argv[0];
	else
		progn++;
	
	if (!strcmp (progn, "activate"))
		activate = 1;		/* equivalent to `fdisk -A' */
	else
		fdisk = 1;
	
	while ((c = getopt_long (argc, argv, short_opts, long_opts, NULL)) != -1)
	{
		switch (c)
		{
			case 'f':
				force = 1; break;
			case 'i':
				increment = 1; break;
			case 'c':
			case 'c' + PRINT_ID:
			case 'c' + CHANGE_ID:
				do_id = c; break;
			case 'd':
				dump = 1; /* fall through */
			case 'l':
				opt_list = 1; break;
			case 'n':
				no_write = 1; break;
			case 'q':
				quiet = 1; break;
			case 's':
				opt_size = 1; break;
			case 'u':
				set_format (*optarg); break;
			case 'v':
				version ();
				exit (0);
			case 'x':
				show_extended = 1; break;
			case 'A':
				activatearg = optarg;
				activate = 1; break;
			case 'N':
				one_only = atoi (optarg); break;
			case 'I':
				restore_sector_file = optarg; break;
			case 'O':
				save_sector_file = optarg; break;
			case 'R':
				opt_reread = 1; break;
			case 'S':
				specified_sectors = atoi (optarg); break;
			case 'T':
				list_types ();
				exit (0);
			case 'V':
				verify = 1; break;
			case '?':
			default:
				usage (); break;
		}
	}
	
# if 0
	if (optind == argc && (opt_list || opt_size || verify))
	{
		struct devd *dp;
		char *lp;
		char device[10];
		
		total_size = 0;
		
		for (dp = defdevs; dp-defdevs < SIZE (defdevs); dp++)
		{
			lp = dp->letters;
			while (*lp)
			{
				sprintf (device, "/dev/%s%c", dp->pref, *lp++);
				if (opt_size)
					do_size (device, 1);
				if (opt_list || verify)
					do_list (device, 1);
			}
		}
		
		if (opt_size)
			printf ("total: %d blocks\n", total_size);
		
		exit (exit_status);
	}
# endif
	
	if (optind == argc)
	{
		if (activate)
			activate_usage (fdisk ? "fdisk -A" : progn);
		else
			usage ();
	}
	
	if (opt_list || opt_size || verify)
	{
		while (optind < argc)
		{
			if (opt_size)
				do_size (argv[optind], 0);
			if (opt_list || verify)
				do_list (argv[optind], 0);
			
			optind++;
		}
		
		exit (exit_status);
	}
	
	if (activate)
	{
		do_activate (argv+optind, argc-optind, activatearg);
		exit (exit_status);
	}
	
	if (do_id)
	{
        	if ((do_id & PRINT_ID) != 0 && optind != argc-2)
			fatal ("usage: fdisk --print-id device partition-number\n");
		else if ((do_id & CHANGE_ID) != 0 && optind != argc-3)
			fatal ("usage: fdisk --change-id device partition-number Id\n");
		else if (optind != argc-3 && optind != argc-2)
			fatal ("usage: fdisk --id device partition-number [Id]\n");
		
		do_change_id (argv[optind], argv[optind+1], (optind == argc-2) ? 0 : argv[optind+2]);
		exit (exit_status);
	}
	
	if (optind != argc-1)
		fatal ("can specify only one device (except with -l or -s)\n");
	dev = argv[optind];
	
	if (opt_reread)
		do_reread (dev);
	else if (restore_sector_file)
		restore_sectors (dev);
	else
		do_fdisk (dev);
	
	return 0;
}

/* END main */
/****************************************************************************/

/****************************************************************************/
/* BEGIN */

/*
 *  H. Listing the current situation
 */

static int
my_open (char *dev, int rw, int silent)
{
	int fd, mode;
	
	mode = (rw ? O_RDWR : O_RDONLY);
	fd = _my_open (dev, mode);
	if (fd < 0 && !silent)
	{
		perror (dev);
		fatal ("cannot open %s %s\n", dev, rw ? "read-write" : "for reading");
	}
	
	return fd;
}

static void
do_list (char *dev, int silent)
{
	int fd;
	struct disk_desc *z;
	
	fd = my_open (dev, 0, silent);
	if (fd < 0)
		return;
	
	z = &oldp;
	
	free_sectors ();
	get_sectors (dev, fd, dump ? 1 : opt_list ? 0 : 1);
	get_partitions (dev, fd, z);
	
	if (opt_list)
		out_partitions (dev, z);
	
	if (verify)
	{
		if (partitions_ok (z))
			warn ("%s: OK\n", dev);
		else
			exit_status = 1;
	}
}

/* for compatibility with earlier fdisk: provide option -s */
static void
do_size (char *dev, int silent)
{
	int fd, size;
	
	fd = my_open (dev, 0, silent);
	if (fd < 0)
		return;
	
	if (_my_ioctl (fd, BLKGETSIZE, &size))
	{
		if (!silent)
		{
			perror (dev);
			fatal ("BLKGETSIZE ioctl failed for %s\n", dev);
		}
		
		return;
	}
	
	size /= 2;			/* convert sectors to blocks */
	if (silent)
		printf ("%s: %d\n", dev, size);
	else
		printf ("%d\n", size);
	
	total_size += size;
}

/*
 * Activate: usually one wants to have a single primary partition
 * to be active. OS/2 fdisk makes non-bootable logical partitions
 * active - I don't know what that means to OS/2 Boot Manager.
 *
 * Call: activate /dev/hda 2 5 7	make these partitions active
 *					and the remaining ones inactive
 * Or:   fdisk -A /dev/hda 2 5 7
 *
 * If only a single partition must be active, one may also use the form
 *       fdisk -A2 /dev/hda
 *
 * With "activate /dev/hda" or "fdisk -A /dev/hda" the active partitions
 * are listed but not changed. To get zero active partitions, use
 * "activate /dev/hda none" or "fdisk -A /dev/hda none".
 * Use something like `echo ",,,*" | fdisk -N2 /dev/hda' to only make
 * /dev/hda2 active, without changing other partitions.
 *
 * A warning will be given if after the change not precisely one primary
 * partition is active.
 *
 * The present syntax was chosen to be (somewhat) compatible with the
 * activate from the LILO package.
 */
static void
set_active (struct disk_desc *z, char *pnam)
{
	int pno;
	
	pno = asc_to_index (pnam, z);
	z->partitions[pno].p.flg |= PART_BOOTABLE;
}

static void
do_activate (char **av, int ac, char *arg)
{
	char *dev = av[0];
	int fd;
	int rw, i, pno;
	struct disk_desc *z;
	
	z = &oldp;
	
	rw = (!no_write && (arg || ac > 1));
	fd = my_open (dev, rw, 0);
	
	free_sectors ();
	get_sectors (dev, fd, 1);
	get_partitions (dev, fd, z);
	
	if (!arg && ac == 1)
	{
		/* list active partitions */
		for (pno = 0; pno < z->partno; pno++)
		{
			if (z->partitions[pno].p.flg & PART_BOOTABLE)
				printf ("%s#%d\n", dev, pno);
		}
	}
	else
	{
		/* clear `active byte' everywhere */
		for (pno = 0; pno < z->partno; pno++)
			z->partitions[pno].p.flg &= ~PART_BOOTABLE;
		
		/* then set where desired */
		if (ac == 1)
			set_active (z, arg);
		else for (i = 1; i < ac; i++)
			set_active (z, av[i]);
		
		/* then write to disk */
		if (write_partitions (dev, fd, z))
			warn ("Done\n\n");
		else
			exit_status = 1;
	}
	
	i = 0;
	for (pno = 0; pno < z->partno && pno < 4; pno++)
		if (z->partitions[pno].p.flg & PART_BOOTABLE)
			i++;
	
	if (i != 1)
		warn ("You have %d active primary partitions.\n", i);
}

static void
do_change_id (char *dev, char *pnam, char *id)
{
	int fd, rw, pno;
	struct disk_desc *z;
	int i;
	
	z = &oldp;
	
	rw = !no_write;
	fd = my_open (dev, rw, 0);
	
	free_sectors ();
	get_sectors (dev, fd, 1);
	get_partitions (dev, fd, z);
	
	pno = asc_to_index (pnam, z);
	if (!id)
	{
		printf ("%c%c%c\n",
			z->partitions[pno].p.id[0],
			z->partitions[pno].p.id[1],
			z->partitions[pno].p.id[2]);
		return;
	}
	
	for (i = 0; i < 3; i++)
	{
		if (!id[i])
			fatal ("Bad Id '%s'\n", id);
	}
	
	z->partitions[pno].p.id[0] = id[0];
	z->partitions[pno].p.id[1] = id[1];
	z->partitions[pno].p.id[2] = id[2];
	
	if (write_partitions (dev, fd, z))
		warn ("Done\n\n");
	else
		exit_status = 1;
}

static void
do_reread (char *dev)
{
	int fd;
	
	fd = my_open (dev, 0, 0);
	if (reread_ioctl (fd))
		printf ("This disk is currently in use.\n");
}

/* END */
/****************************************************************************/

/****************************************************************************/
/* BEGIN */

/*
 *  I. Writing the new situation
 */
static void
do_fdisk (char *dev)
{
# if 0
	int fd;
	int c, answer;
	int interactive = isatty (0);
	struct disk_desc *z;
	
	fd = my_open (dev, !no_write, 0);
	
	if(!no_write && !no_reread)
	{
		warn ("Checking that no-one is using this disk right now ... ");
		if (reread_ioctl (fd))
		{
			printf (
"This disk is currently in use - repartitioning is probably a bad idea."
"Umount all file systems, and swapoff all swap partitions on this disk."
"Use the --no-reread flag to suppress this check.\n");
			
			if (!force)
			{
				printf ("Use the --force flag to overrule all checks.\n");
				exit (1);
			}
		}
		else
			warn ("OK\n");
	}
	
	z = &oldp;
	
	free_sectors ();
	get_sectors (dev, fd, 0);
	get_partitions (dev, fd, z);
	
	printf ("Old situation:\n");
	out_partitions (dev, z);
	
	if (one_only && (one_only_pno = tos_to_index (one_only, z)) < 0)
		fatal ("Partition %d does not exist, cannot change it\n", one_only);
	
	z = &newp;
	
	while (1)
	{
		
		read_input (dev, interactive, z);
		
		printf ("New situation:\n");
		out_partitions (dev, z);
		
		if (!partitions_ok (z) && !force)
		{
			if (!interactive)
				fatal ("I don't like these partitions - nothing changed.\n"
					"(If you really want this, use the --force option.)\n");
			else
				printf ("I don't like this - probably you should answer No\n");
		}
ask:
		if (interactive)
		{
			if (no_write)
				printf ("Are you satisfied with this? [ynq] ");
			else
				printf ("Do you want to write this to disk? [ynq] ");
			
			answer = c = getchar ();
			while (c != '\n' && c != EOF)
				c = getchar();
			
			if (c == EOF)
				printf ("\nfdisk: premature end of input\n");
			
			if (c == EOF || answer == 'q' || answer == 'Q')
				fatal ("Quitting - nothing changed\n");
			else if (answer == 'n' || answer == 'N')
				continue;
			else if (answer == 'y' || answer == 'Y')
				break;
			else
			{
				printf("Please answer one of y,n,q\n");
				goto ask;
			}
		}
		else
			break;
	}
	
	if (write_partitions (dev, fd, z))
		printf ("Successfully wrote the new partition table\n\n");
	else
		exit_status = 1;
	
	reread_disk_partition (dev, fd);
	
	warn( "If you created or changed a DOS partition, /dev/foo7, say, then use dd(1)\n"
		"to zero the first 512 bytes:  dd if=/dev/zero of=/dev/foo7 bs=512 count=1\n"
		"(See fdisk(8).)\n");
	
	sync ();
	exit (exit_status);
# endif
}

/* END main */
/****************************************************************************/
