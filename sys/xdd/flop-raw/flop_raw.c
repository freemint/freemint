/******-----------------------------------------------------******
 * rawdevices - a multi-devices driver for raw floppy disks      *
 * for PURE_C 1.1                                                *
 *                                                               *
 * This is Dave Gymer's raw MiNT floppy device hacked up         *
 * by Stephane Boisson (boisso_s@epita.fr)                       *
 * and is hereby placed in the public domain.                    *
 *                                                               *
 ******-----------------------------------------------------******/

# define __KERNEL_XDD__

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/proc.h"

# include <mint/osbind.h>


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	20
# define VER_STATUS	

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p raw floppy devices version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"½ 1992, 1993 by Dave Gymer, Stephane Boisson.\r\n" \
	"½ " MSG_BUILDDATE " by Frank Naumann.\r\n"

# define MSG_MINT	\
	"\033pMiNT to old!\033q\r\n"

# define MSG_FAILURE	\
	"\7Sorry, flop_raw.xdd NOT installed (Dcntl(...) failed)!\r\n\r\n"


# if 0
# define DEBUG(x)	KERNEL_DEBUG x
# define ALERT(x)	KERNEL_ALERT x
# define TRACE(x)	KERNEL_TRACE x
# else
# define DEBUG(x)
# define ALERT(x)	KERNEL_ALERT x
# define TRACE(x)
# endif


DEVDRV * _cdecl init (struct kerinfo *k);

static long _cdecl flop_open	(FILEPTR *f);
static long _cdecl flop_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl flop_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl flop_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl flop_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl flop_datime	(FILEPTR *f, ushort *timeptr, int wrflag);
static long _cdecl flop_close	(FILEPTR *f, int pid);
static long _cdecl flop_select	(FILEPTR *f, long proc, int mode);
static void _cdecl flop_unselect(FILEPTR *f, long proc, int mode);

static DEVDRV devtab =
{
	flop_open,
	flop_write, flop_read, flop_lseek, flop_ioctl, flop_datime,
	flop_close,
	flop_select, flop_unselect
};

typedef struct rawdev RAWDEV;
struct rawdev
{
	char	*name;		/* Device name */
	int	drive;		/* BIOS drive number, 0 == A:, 1 == B: */
	int	tracks;		/* tracks per disk */
	int	sides;		/* sides per track */
	int	sectors;	/* sectors per track */
	int	secsize;	/* bytes per sector */
	long	blocksize;	/* calculated... */
};


/* characteristics of the floppies
 */
static RAWDEV drivers [] =
{
	{ "fd0", 0, 80, 2,  9, 512 },	/* 720 kbyte Double density */
	{ "fh0", 0, 80, 2, 18, 512 }	/* 1.44 meg high density */
};


/* kernel information
 */
struct kerinfo *kernel;

/* Install time
 */
static ushort install_date;
static ushort install_time;

/* structure to hold a block buffer
 */
typedef struct floppy_block FLOBLOCK;
struct floppy_block
{
	ushort	dev_id;
	int	track;
	int	side;
	long	byte;
	enum {
		INVALID, VALID, DIRTY, ATEOF
	} state;
	char	*buffer;
};


/* Initial setup, return the device driver to the OS
 */
DEVDRV * _cdecl
init (struct kerinfo *k)
{
	struct dev_descr dev_descriptor =
	{
		&devtab,
		0,
		0,
		NULL,
		0,
		S_IFCHR |
		S_IRUSR |
		S_IWUSR |
		S_IRGRP |
		S_IWGRP |
		S_IROTH |
		S_IWOTH,
		NULL,
		0,
		0
	};
	
	char name [32] = "u:\\dev\\";
	long i, len;
	
	kernel = k;
	
	DEBUG (("%s: enter init", __FILE__));
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	c_conws ("\r\n");
	
	len = strlen (name);
	
	for (i = 0; i < (sizeof (drivers) / sizeof (RAWDEV)); i++)
	{
		strcpy (name + len, drivers[i].name);
		dev_descriptor.dinfo = i;
		drivers[i].blocksize = ((long) drivers[i].secsize) * drivers[i].sectors; 
		
		DEBUG (("[RAWDEV] init: device [%s]", name));
		
		if (d_cntl (DEV_INSTALL, name, (long) &dev_descriptor) <= 0)
			ALERT (("[RAWDEV] floppy_init: Unable to install %s", name));
	}
	
	install_time = timestamp;
	install_date = datestamp;
	
	return (DEVDRV *) 1L;
}

/* utility functions
 * note that blocks are whole tracks
 */

/* read a block
 */
static int
read_block (FLOBLOCK *floblock)
{
	int r;
	
	TRACE (("[RAWDEV] read_block: drive #%d, track #%d, side %d", 
		drivers[floblock->dev_id].drive, floblock->track, floblock->side));
	
	r = Floprd (floblock->buffer,
			0L,
			drivers[floblock->dev_id].drive,
			1,
			floblock->track, floblock->side,
			drivers[floblock->dev_id].sectors);
		
	if (r)
	{
		DEBUG (("[RAWDEV] read_block: Floprd error #%d", r));
		floblock->state = INVALID;
	}
	else
		floblock->state = VALID;
	
	return r;
}

/* flush a block
 */
static int
flush_block (FLOBLOCK *floblock)
{
	int r;
	
	if (floblock->state != DIRTY)
		return 0;
	
	TRACE (("[RAWDEV] flush_block: drive #%d, track #%d, side %d", 
		drivers[floblock->dev_id].drive, floblock->track, floblock->side));
	
	r = Flopwr (floblock->buffer,
			0L,
			drivers[floblock->dev_id].drive,
			1,
			floblock->track, floblock->side,
			drivers[floblock->dev_id].sectors);
	
	if (r)
		DEBUG (("[RAWDEV] flush_block: Flopwr error #%d", r));
	else
		floblock->state = VALID;
	
	return r;
}

/* convert long seek position into floppy_block
 */
static void
seek2int (long pos, FLOBLOCK *floblock)
{
	TRACE (("[RAWDEV] seek2int: convert long seek position #%ld", pos));
	
	if (pos < 0)
		pos = 0;
	
	floblock->byte = pos % drivers[floblock->dev_id].blocksize;
	pos /= drivers[floblock->dev_id].blocksize;
	floblock->side = pos % drivers[floblock->dev_id].sides;
	pos /= drivers[floblock->dev_id].sides;
	floblock->track = pos;
	
	if (floblock->track >= drivers[floblock->dev_id].tracks)
	{
		floblock->track = drivers[floblock->dev_id].tracks;
		floblock->side = 0;
		floblock->byte = 0;
		floblock->state = ATEOF;
	}
}

/* convert floppy_block into long seek position
 */
static long
int2seek (FLOBLOCK *floblock)
{
	long r;
	
	TRACE (("[RAWDEV] int2seek: convert floppy_block"));
	
	r  = floblock->track;
	r *= drivers[floblock->dev_id].sides;
	r += floblock->side;
	r *= drivers[floblock->dev_id].blocksize;
	r += floblock->byte;
	
	return r;
}

/* move to next block - read it, after flushing the old one
 */
static int
next_block (FLOBLOCK *floblock)
{
	int r = 0;
	
	TRACE (("[RAWDEV] next_block: move to next block"));
	
	if (floblock->state != ATEOF)
	{
		r = flush_block (floblock);
		if (++floblock->side == drivers[floblock->dev_id].sides)
		{
			floblock->side = 0;
			if (++floblock->track == drivers[floblock->dev_id].tracks)
			{
				DEBUG (("[RAWDEV] next_block: end of file"));
				floblock->state = ATEOF;
				floblock->side = 0;
			}
		}
		
		if (floblock->state != ATEOF)
		{
			if (r)
				floblock->state = INVALID;
			else
				r = read_block (floblock);
		}
		
		floblock->byte = 0;
	}
	
	return r;
}

/* here are the actual device driver functions
 */
static long _cdecl
flop_open (FILEPTR *f)
{
	FLOBLOCK *floblock;
	
	TRACE (("[RAWDEV] floppy_open: open device %s", drivers[f->fc.aux].name));
	
	floblock = kmalloc (sizeof (*floblock));
	if (!floblock)
	{
		ALERT (("[RAWDEV] floppy_open: out of memory for FLOBLOCK"));
		
		return ENOMEM;
	}
	
	floblock->buffer = kmalloc (drivers[f->fc.aux].blocksize);
	if (!floblock->buffer)
	{
		ALERT (("[RAWDEV] floppy_open: out of memory for blocksize"));
		
		kfree (floblock);
		return ENOMEM;
	}
	
	f->devinfo = (long) floblock;
	
	floblock->state = INVALID;
	floblock->track = 0;
	floblock->side = 0;
	floblock->byte = 0;
	floblock->dev_id = f->fc.aux;
	
	if (d_lock (1, drivers[floblock->dev_id].drive))
	{
		kfree (floblock->buffer);
		kfree (floblock);
		
		return EACCES;
	}
	
	return E_OK;
}

static long _cdecl
flop_write (FILEPTR *f, const char *buf, long bytes)
{
	FLOBLOCK *floblock = (FLOBLOCK *) f->devinfo;
	int r = 0;
	long bytes_written = 0;
	
	TRACE (("[RAWDEV] floppy_write: write %ld bytes to %s", bytes, drivers[f->fc.aux].name));
	
	if (floblock->state == INVALID)
	{
		/* not started yet */
		r = read_block (floblock);
	}
	
	/* keep going until we've written enough, or there's an error or EOF
	 */
	while (!r && floblock->state != ATEOF && bytes)
	{
		if (floblock->byte < drivers[floblock->dev_id].blocksize)
		{
			/* data in buffer */
			
			char *ptr = floblock->buffer + floblock->byte;
			long num = drivers[floblock->dev_id].blocksize - floblock->byte;
			
			if (num > bytes) num = bytes;
			bytes_written += num;
			bytes -= num;
			floblock->byte += num;
# if 1
			while (num--)
				*ptr++ = *buf++;
# else
			memcpy (ptr, buf, num);
# endif
			floblock->state = DIRTY;
		}
		else
		{
			/* must get next block */
			r = next_block (floblock);
		}
	}
	
	return bytes_written;
}

static long _cdecl
flop_read (FILEPTR *f, char *buf, long bytes)
{
	FLOBLOCK *floblock = (FLOBLOCK *) f->devinfo;
	int r = 0;
	long bytes_read = 0;
	
	TRACE (("[RAWDEV] floppy_read: read %ld bytes from %s", bytes, drivers[f->fc.aux].name));
	
	if (floblock->state == INVALID)
	{
		/* not started yet */
		r = read_block (floblock);
	}
	
	/* keep going until we've read enough, or there's an error or EOF
	 */
	while (!r && floblock->state != ATEOF && bytes)
	{
		if (floblock->byte < drivers[floblock->dev_id].blocksize)
		{
			/* data in buffer */
			
			char *ptr = floblock->buffer + floblock->byte;
			long num = drivers[floblock->dev_id].blocksize - floblock->byte;

			if (num > bytes) num = bytes;
			bytes_read += num;
			bytes -= num;
			floblock->byte += num;
# if 1
			while (num--)
				*buf++ = *ptr++;
# else
			memcpy (buf, ptr, num);
# endif
		}
		else
		{
			/* must get next block */
			r = next_block (floblock);
		}
	}
	
	return bytes_read;
}

static long _cdecl
flop_lseek (FILEPTR *f, long where, int whence)
{
	FLOBLOCK *floblock = (FLOBLOCK *) f->devinfo;
	long newpos = where;
	
	TRACE (("[RAWDEV] floppy_lseek: seek mode %d of %ld on %s", whence, where, drivers[f->fc.aux].name));
	
	switch (whence)
	{
		case SEEK_SET:
		{
			/* nothing to do */
			break;
		}
		case SEEK_CUR:
		{
			newpos += int2seek (floblock);
			break;
		}
		case SEEK_END:
		{
			newpos  = drivers[floblock->dev_id].sides;
			newpos *= drivers[floblock->dev_id].tracks;
			newpos *= drivers[floblock->dev_id].blocksize;
			newpos -= newpos;
			break;
		}
		default:
		{
			DEBUG (("[RAWDEV] floppy_lseek: illegal whence (%d) in seek for %s", whence, drivers[floblock->dev_id].name));
			return EBADARG;
		}
	}
	
	if (int2seek (floblock) % drivers[floblock->dev_id].blocksize != newpos % drivers[floblock->dev_id].blocksize)
	{
		if (flush_block (floblock))
			DEBUG (("[RAWDEV] floppy_lseek: flush_block failed on %s", drivers[floblock->dev_id].name));
		
		floblock->state = INVALID;
	}
	
	seek2int (newpos, floblock);
	
	return newpos;
}

static long _cdecl
flop_ioctl (FILEPTR *f, int mode, void *buf)
{
	TRACE (("[RAWDEV] floppy_ioctl: mode %d on %s", mode, drivers[f->fc.aux].name));
	
	switch (mode)
	{
		case FIONREAD:
		case FIONWRITE:
		{
			/* we never block - use BLOCKSIZE as a sensible
			 * number to read as a chunk
			 */
			*((long *) buf) = drivers[f->fc.aux].secsize * drivers[f->fc.aux].sectors;
			
			TRACE (("[RAWDEV] floppy_ioctl: FIONREAD/FIONWRITE on %s -> %ld", drivers[f->fc.aux].name, *((long *) buf)));
			
			return E_OK;
		}
	}
	
	return ENOSYS;
}

static long _cdecl
flop_datime (FILEPTR *f, ushort *timeptr, int wrflag)
{
	TRACE (("[RAWDEV] floppy_datime: on %s", drivers[f->fc.aux].name));
	
	if (wrflag)
		return ENOSYS;
	
	*timeptr++ = install_time;
	*timeptr = install_date;
	
	return E_OK;
}

static long _cdecl
flop_close (FILEPTR *f, int pid)
{
	FLOBLOCK *floblock = (FLOBLOCK *) f->devinfo;
	int r = E_OK;
	
	TRACE (("[RAWDEV] floppy_close: %s by pid #%d", drivers[f->fc.aux].name, pid));
	
	if (f->links <= 0)
	{
		/* flush the buffer */
		r = flush_block (floblock);
		
		(void) d_lock (0, drivers[floblock->dev_id].drive);
		
		kfree (floblock->buffer);
		kfree (floblock);
	}
	
	return r;
}

static long _cdecl
flop_select (FILEPTR *f, long proc, int mode)
{
	TRACE (("[RAWDEV] floppy_select: %s always ready!", drivers[f->fc.aux].name));
	
	/* we're always ready for I/O */
	return 1;
}

static void _cdecl
flop_unselect (FILEPTR *f, long proc, int mode)
{
	TRACE (("[RAWDEV] floppy_unselect: %s (nothing to do)", drivers[f->fc.aux].name));
	
	/* nothing for us to do here */
}
