/*
 * lpdev.c: installs the "/dev/lp" device. It is a
 * buffered Centronics device. This program is free software; see the
 * file "COPYING" for details.
 *
 * This file must be compiled with 16-bit integers.
 *
 * Author:  Thierry Bousch (bousch@suntopo.matups.fr)
 * Version: 0.7 (June 1994)
 *
 * Revision history:
 *  0.1: First attempt, using SLEEP instead of NAP: it didn't work.
 *  0.2: Added version number, napping in lp_write(), and the TIOCFLUSH
 *        ioctl function. Cleaned up things a bit.
 *  0.3: Introduced spl7() and spl() to fix competition problems. Added
 *        a few tests before installation, to check that MiNT is running
 *        and the device is not already installed.
 *  0.4: Added file locking. This is completely untested, so be
 *        careful. More cleanup and sanity checks during installation.
 *        Modified the sleep conditions in lp_write and lp_select.
 *  0.5: Deleted the now unnecessary stuff about low and high water marks.
 *        More comments added.
 *  0.6: Added support for the O_NDELAY and O_LOCK flags, inlined spl7/spl.
 *        Moved the definitions to lpdev.h.
 *  0.7: ++Ulrich Kuehn
 *        added support for mint's xdd feature, call print_head() after
 *        napping in case an interrupt gets lost. Added the possibility
 *        to configure port sharing at compile time.
 */

# include "lp.h"

# include "arch/timer.h"
# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/signal.h"
# include "mint/stat.h"

# include <mint/osbind.h>
# include "mint/arch/mfp.h"


# define  LP_VERSION	"0.9"

/*
 * Global variables
 */

struct kerinfo *kernel;

static char *buffer_start, *buffer_end, *buffer_tail;
static volatile char *buffer_head;
static volatile long buffer_contents;
static long selector = 0L;
static struct flock our_lock = { F_WRLCK, 0, 0L, 0L, -1 };

/*
 * internal prototypes
 */

static void	reset_buffer(void);
static long	init_centr(void);
       void	wake_up(void);
       void	print_head(void);
static void	print_tail(const char *buf, long nbytes);



/*
 * bios emulation - top half
  */
  static long _cdecl      lp_instat      (int dev);
  static long _cdecl      lp_in          (int dev);
  static long _cdecl      lp_outstat     (int dev);
  static long _cdecl      lp_out         (int dev, int c);
  static long _cdecl      lp_rsconf      (int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr);  


/*
 * Forward declarations of the device driver functions
 */

static long	_cdecl lp_open		(FILEPTR *f);
static long	_cdecl lp_write		(FILEPTR *f, const char *buf, long bytes);
static long	_cdecl lp_read		(FILEPTR *f, char *buf, long bytes);
static long	_cdecl lp_lseek		(FILEPTR *f, long where, int whence);
static long	_cdecl lp_ioctl		(FILEPTR *f, int mode, void *buf);
static long	_cdecl lp_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long	_cdecl lp_close		(FILEPTR *f, int pid);
static long	_cdecl lp_select	(FILEPTR *f, long proc, int mode);
static void	_cdecl lp_unselect	(FILEPTR *f, long proc, int mode);


/*
 * device driver maps
 */
  static BDEVMAP bios_devtab =
  {
        lp_instat, lp_in,
        lp_outstat, lp_out,
        lp_rsconf
  };

static DEVDRV lp_device =
{
	lp_open, lp_write, lp_read, lp_lseek, lp_ioctl,
	lp_datime, lp_close, lp_select, lp_unselect
};

static struct dev_descr devinfo =
{
	&lp_device,
	0,              /* dinfo -> fc.aux */
	0,              /* flags */
	NULL,           /* struct tty * */
	0,              /* drvsize */
	S_IFCHR |
	S_IRUSR |
	S_IWUSR |
	S_IRGRP |
	S_IWGRP |
	S_IROTH |
	S_IWOTH,        /* fmode */
	&bios_devtab,   /* bdevmap */
	0,              /* bdev */
	0               /* reserved */
};


/* Initializes the circular buffer
 */
static void
reset_buffer(void)
{
	register ushort sr;
	
	sr = spl7();
	buffer_head = buffer_tail = buffer_start;
	buffer_end = buffer_start + BUFSIZE;
	buffer_contents = 0L;
	spl(sr);
}

/* uk: moved this stuf here, so that it can be called on every
 *     open() call. This is used if CENTR_SHARING is defined
 */
static long
init_centr(void)
{
	Mfpint(0, new_centr_vector);
	Jenabint(0);
	return 0;
}

/* uk: if this is linked and started as an externel device driver,
 *     the memory allocation stuff is already done and we are in
 *     super mode already.
 */
DEVDRV *_cdecl
init_xdd(struct kerinfo *k)
{
	kernel = k;
	
	c_conws ("Spooled Centronics device driver, by T.Bousch (version "
		LP_VERSION ").\r\n"
		"This program is FREE SOFTWARE, and comes with NO WARRANTY.\r\n"
		"See the file \"COPYING\" for more information.\r\n\r\n"
	);
	
	buffer_start = kmalloc(BUFSIZE);
	if (!buffer_start)
	{
		c_conws("lpdev: not enough memory\r\n\r\n");
		return NULL;
	}
	
	reset_buffer();
	
	if (d_cntl(DEV_INSTALL, DEVNAME, (long) &devinfo) < 0)
	{
		c_conws("lpdev: unable to install device\r\n\r\n");
		return NULL;
	}
	
	/* Finally! */
	init_centr();
	
	/* we are already installed */
	return (DEVDRV *) 1;
}

/*
 * Will wake any process select'ing the printer;
 * this routine is called by the interrupt handler, but also when the
 * buffer is flushed.
 */
void
wake_up(void)
{
	if (selector)
		wakeselect(selector);
}

/*
 * Sends as many bytes as possible (usually one) to the printer until
 * he gets busy. This routine is called by lp_write and by the
 * interrupt handler, so it _must_ be multi-thread. It will not work if
 * you remove the spl7()/spl() pair.
 *
 * On a more general note, it is safest to disable all interrupts before
 * modifying the volatile variables (buffer_contents and buffer_head).
 */

# if 0
# define PRINTER_BUSY	(*(char *) 0xFFFFFA01 & 1)
# else
# define PRINTER_BUSY	(_mfpregs->gpip & 1)
# endif

void
print_head(void)
{
	register ushort sr;
	
	sr = spl7();
	while (!PRINTER_BUSY && buffer_contents)
	{
		print_byte(*buffer_head);
		
		--buffer_contents;
		if (++buffer_head >= buffer_end)
			buffer_head -= BUFSIZE;
	}
	spl(sr);
}

/*
 * Copies a linear buffer into the circular one. We assume that's there
 * enough room for this operation, ie
 * nbytes + buffer_contents <= BUFSIZE
 *
 * Note: the while() loop will be executed at most twice.
 * Note2: the instruction "buffer_contents += N" looks atomic, but it
 *   isn't (the Gcc outputs several assembly instructions). Therefore it
 *   must be wrapped in spl7()/spl().
 */
static void
print_tail (const char *buf, long nbytes)
{
	while (nbytes)
	{
		register long n;
		
		n = buffer_end - buffer_tail;
		if (n > nbytes)
			n = nbytes;
		bcopy(buf, buffer_tail, n);
		buf += n;
		nbytes -= n;
		
		{
			register ushort sr;
			
			sr = spl7();
			buffer_contents += n;
			spl(sr);
		}
		
		buffer_tail += n;
		if (buffer_tail >= buffer_end)
			buffer_tail -= BUFSIZE;
	}
	
	/* To initiate printing */
	print_head();
}


/****************************************************************************/
/* BEGIN bios emulation - top half */

static long _cdecl
lp_instat (int dev)
{
	return 0;
}

static long _cdecl                                                        
lp_in (int dev)
{
	TRACE(("lp: foolish attempt to read"));
	return 0;
}

static long _cdecl                                                        
lp_outstat (int dev)
{
        return ( BUFSIZE - buffer_contents > 1 ? -1 : 0);
}

static long _cdecl                                                        
lp_out (int dev, int c)
{
	char x = c;
	
	print_tail(&x, 1);
	return -1;
}

static long _cdecl                                                        
lp_rsconf (int dev, int speed, int flowctl, int ucr, int rsr, int tsr, int scr)
{
	return 0;
}

/*
 * Here are the actual device driver functions
 */
 
# define  LP_LOCKED	(our_lock.l_pid >= 0)

static long _cdecl
lp_open(FILEPTR *f)
{
	TRACE(("lp: open device"));
	
	init_centr();
	
	return 0;
}

static long _cdecl
lp_close(FILEPTR *f, int pid)
{
	TRACE(("lp: close device"));
	
	if ((f->flags & O_LOCK) && our_lock.l_pid == pid)
	{
		TRACE(("lp: releasing lock on close"));
		f->flags &= ~O_LOCK;
		our_lock.l_pid = -1;
		wake(IO_Q, (long) &our_lock);
	}
	
	return 0;
}

static long _cdecl
lp_read(FILEPTR *f, char *buf, long bytes)
{
	TRACE(("lp: foolish attempt to read"));
	return 0;
}

static long _cdecl
lp_datime(FILEPTR *f, ushort *timeptr, int rwflag)
{
	if (rwflag)
	{
		DEBUG(("lp: can't modify date/time"));
		return EACCES;
	}
	
	TRACE(("lp: read time and date"));
	
	*timeptr++ = timestamp;
	*timeptr   = datestamp;
	
	return 0;
}

static long _cdecl
lp_lseek(FILEPTR *f, long where, int whence)
{
	TRACE(("lp: foolish attempt to seek"));
	
	if (whence < 0 || whence > 2)
		return ENOSYS;
	
	return where ? EBADARG : 0L;
}

static long _cdecl
lp_ioctl(FILEPTR *f, int mode, void *buf)
{
	switch (mode)
	{
		case FIONREAD:
		{
			TRACE(("lp: ioctl(FIONREAD)"));
			*(long *)buf = 0L;
			
			return E_OK;
		}
		case FIONWRITE:
		{
			TRACE(("lp: ioctl(FIONWRITE)"));
			*(long *)buf = BUFSIZE - buffer_contents;
			
			return E_OK;
		}
		case TIOCFLUSH:
		{
			TRACE(("lp: clear buffer"));
			reset_buffer();

			/* Wake up any select'ing process */
			wake_up();
			
			return E_OK;
		}
		case F_GETLK:
		{
			struct flock *g = (struct flock *) buf;
			
			if (LP_LOCKED)
			{
				TRACE(("lp: get_lock succeeded"));
				*g = our_lock;
			}
			else
			{
				TRACE(("lp: get_lock failed"));
				g->l_type = F_UNLCK;
			}
			
			return E_OK;
		}
		case F_SETLK:
		case F_SETLKW:
		{
			struct flock *g = (struct flock *) buf;
			
			switch (g->l_type)
			{
				case F_UNLCK:
				{
					if (!(f->flags & O_LOCK) || g->l_pid != our_lock.l_pid)
					{
						DEBUG(("lp: no such lock"));
						return ENSLOCK;
					}
					else
					{
						TRACE(("lp: remove lock"));
						f->flags &= ~O_LOCK;
						our_lock.l_pid = -1;
						wake(IO_Q, (long) &our_lock);
					}
					break;
				}
				case F_RDLCK:
				{
					TRACE(("lp: read locks are ignored"));
					break;
				}
				case F_WRLCK:
				{
					while (LP_LOCKED)
					{
						DEBUG(("lp: conflicting locks"));
						if (mode == F_SETLK)
						{
							*g = our_lock;
							return ELOCKED;
						}
						sleep(IO_Q, (long) &our_lock);
					}
					TRACE(("lp: set lock"));
					f->flags |= O_LOCK;
					our_lock.l_pid = g->l_pid;
					break;
				}
				default:
				{
					DEBUG(("lp: invalid lock type"));
					return ENOSYS;
				}
			}
			
			return E_OK;
		}
	}
	
	DEBUG(("lp: invalid ioctl mode"));
	return ENOSYS;
}

static long _cdecl
lp_write(FILEPTR *f, const char *buf, long bytes)
{
	long _bytes = bytes;
	int  ndel = (f->flags & O_NDELAY);	/* don't wait */
	
	while (bytes)
	{
		long n = BUFSIZE - buffer_contents;
		
		/* If the data won't fit into the buffer,
		 * and if the buffer itself is almost full, we won't
		 * be able to copy much. So we better sleep a bit (unless
		 * the O_NDELAY flag is set).
		 */
		if (n < bytes && n < BUFSIZE / 4 && !ndel)
		{
			TRACE(("lp: napping in lp_write"));
			/* let's wait 200 milliseconds */
			nap(200);
			
			/* uk: to prevent lost interrupts stopping the driver
			 *     start printing again. If the device is still busy, it
			 *     does not harm.
			 */
			print_head();
			continue; /* and try again */
		}
		
		if (bytes < n)
			n = bytes;
		
		/* Now n contains the number of bytes we want to copy
		 * into the circular buffer
		 */
		print_tail(buf, n);
		buf += n;
		bytes -= n;
		
		/*
		 * If the O_NDELAY flag is set, we don't make a second
		 * attempt to write the remaining "bytes" bytes.
		 */
		if (ndel)
			break;
	}
	
	TRACE(("lp: wrote %ld bytes, skipped %ld", _bytes-bytes, bytes));
	return _bytes - bytes;
}

/* Bug: only one process can select the printer
 */
static long _cdecl
lp_select(FILEPTR *f, long proc, int mode)
{
	if (buffer_contents == BUFSIZE && !selector)
	{
		TRACE(("lp: select returned 0"));
		selector = proc;
		return 0;
	}
	
	TRACE(("lp: select returned 1"));
	return 1;
}

static void _cdecl
lp_unselect(FILEPTR *f, long proc, int mode)
{
	TRACE(("lp: unselect"));
	selector = 0L;
}
