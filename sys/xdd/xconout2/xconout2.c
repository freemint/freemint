/*
 * Filename:     xconout2.c
 * Version:      0.3
 * Author:       Torsten Scherer, Frank Naumann
 * Started:      1999-01-06
 * Last Updated: 1999-01-06
 * Target O/S:   TOS/MiNT
 * Description:  a small device driver that allows us to catch any output
 *               being done through the xconout2 handler
 * 
 * Note:         Please send suggestions, patches or bug reports to me
 *               or the MiNT mailing list (mint@).
 * 
 * Copying:      Copyright 1998 Frank Naumann (fnaumann@cs.uni-magdeburg.de)
 *               Copyright 1995 Torsten Scherer (itschere@techfak.uni-bielefeld.de)
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
 * changes since last version:
 * 
 * 1999-01-06:	(v0.3)
 * 
 * - inital revision
 * 
 * 
 * known bugs:
 * 
 * - nothing
 * 
 * todo:
 * 
 * 
 */

# define __KERNEL_XDD__

# include <mint/mint.h>
# include <mint/asm.h>
# include <mint/dcntl.h>
# include <mint/file.h>
# include <mint/proc.h>
# include <libkern/libkern.h>


/*
 * debugging stuff
 */

# if 0
# define DEV_DEBUG	1
# endif


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	3
# define VER_STATUS	

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p xconout2 output catcher version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"½ Jan  1 1995 by TeSche <itschere@techfak.uni-bielefeld.de>\r\n" \
	"½ " MSG_BUILDDATE " by Frank Naumann.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT to old!\033q\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, xconout2 NOT installed (Dcntl(...) failed)!\r\n\r\n"


/*
 * default settings
 */

# define INSTALL	"u:\\dev\\xconout2"

# if 0
# define BUFSIZE	2048
# else
# define BUFSIZE	16384
# endif

/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * device driver
 */

static long _cdecl	xopen		(FILEPTR *f);
static long _cdecl	xwrite		(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	xread		(FILEPTR *f, char *buf, long bytes);
static long _cdecl	xlseek		(FILEPTR *f, long where, int whence);
static long _cdecl	xioctl		(FILEPTR *f, int mode, void *buf);
static long _cdecl	xdatime		(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	xclose		(FILEPTR *f, int pid);
static long _cdecl	xselect		(FILEPTR *f, long proc, int mode);
static void _cdecl	xunselect	(FILEPTR *f, long proc, int mode);

static DEVDRV devtab =
{
	xopen,
	xwrite, xread, xlseek, xioctl, xdatime,
	xclose,
	xselect, xunselect,
	xwrite, xread
};


/*
 * debugging stuff
 */

# ifdef DEV_DEBUG
#  define DEBUG(x)	KERNEL_DEBUG x
#  define TRACE(x)	KERNEL_TRACE x
#  define ALERT(x)	KERNEL_ALERT x
# else
#  define DEBUG(x)
#  define TRACE(x)
#  define ALERT(x)	KERNEL_ALERT x
# endif


DEVDRV * _cdecl		init		(struct kerinfo *k);
static void		myxconout2	(long ch);


/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

static long		(*oldxconout2)()	= NULL;
static long		rsel			= 0;

static short		inbuf;
static short		wptr;
static short		rptr;

static char		xconbuf [BUFSIZE];

/* END global data & access implementation */
/****************************************************************************/


DEVDRV * _cdecl
init (struct kerinfo *k)
{
	struct dev_descr dev_descriptor =
	{
		&devtab,
		0,
		0,
		NULL,
		sizeof (DEVDRV),
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
	long r;
	
	kernel = k;
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	
	DEBUG (("%s: enter init", __FILE__));
	
	r = d_cntl (DEV_INSTALL, INSTALL, (long) &dev_descriptor);
	if (r < 0)
	{
		c_conws (MSG_FAILURE);
		return NULL;
	}
	
	oldxconout2 = NULL;
	rsel = 0;
	
	DEBUG (("%s: init ok", __FILE__));
	return (DEVDRV *) 1;
}


/*
 *	the xconout functions
 */

/* geee, that's ugly, but I can't do it any better :( */

INLINE void
printc (register long c)
{
	register long r;
	
	__asm__ volatile
	(
		"moveml d0-d7/a0-a6,sp@-;"
		"movl %1,sp@-;"
		"movl _oldxconout2, a0;"
		"jbsr a0@;"
		"addqw #4,sp;"
		"moveml sp@+,d0-d7/a0-a6;"
		: "=r" (r)			/* outputs */
		: "g" (c)			/* inputs  */
	);
}

void
myxconout2 (long ch)
{
	if (rsel)
	{
		wakeselect (rsel);
	}
	
	/* do not worry about buffer overflows here */
	if (inbuf == BUFSIZE)
	{
		return;
	}
	
	if (ch == 7)
	{
		printc (ch);
	}
	else
	{
		xconbuf[wptr] = ch;
		if (++wptr == BUFSIZE) wptr = 0;
		inbuf++;
	}
}

/*
 *	the device driver routines
 */

static long _cdecl
xopen (FILEPTR *f)
{
	long l;
	
	if (oldxconout2)
	{
		DEBUG (("%s: device already in use", __FILE__));
		return EACCES;
	}
	
	/* get xconout2 (console) vector */
	l = *((long *) 0x586);
	
# if 0
	if ((l < 0x00E00000) || (l > 0x00FFFFFF))
	{
		/* probably already somewhere in ram */
		return EINTERNAL;
	}
# endif
	
	inbuf	= 0;
	wptr	= 0;
	rptr	= 0;
	
	/* set new vector */
	oldxconout2 = (void *) l;
	*((long *) 0x586) = (long) myxconout2;
	
	return E_OK;
}

static long _cdecl
xwrite (FILEPTR *f, const char *buf, long bytes)
{
	long nwrite = 0;
	
	while (bytes-- > 0)
	{
		printc ((long) *buf++);
		nwrite++;
	}
	
	return nwrite;
}

static long _cdecl
xread (FILEPTR *f, char *buf, long bytes)
{
	register long ret = 0;
	register ushort sr;
	
	/*
	 * we must lock out the xcounout handler
	 * while mucking with inbuf and rptr.
	 *
	 * ++kay
	 */
	sr = splhigh ();
	while (bytes && inbuf)
	{
		*buf++ = xconbuf[rptr++];
		if (rptr == BUFSIZE)
			rptr = 0;
		
		inbuf--;
		spl (sr);
		
		bytes--;
		ret++;
		
		sr = splhigh ();
	}
	spl (sr);
	
	return ret;
}

static long _cdecl
xlseek (FILEPTR *f, long where, int whence)
{
	/* (terminal) devices are always at position 0 */
	return E_OK;
}

static long _cdecl
xioctl (FILEPTR *f, int mode, void *buf)
{
	switch (mode)
	{
		case FIONREAD:
		{
			*((long *) buf) = (long) inbuf;
			break;
		}
		case FIONWRITE:
		{
			*((long *) buf) = 1;
			break;
		}
		case FIOEXCEPT:
		{
			*((long *) buf) = 0;
			break;
		}
		default:
		{
			return ENOSYS;
		}
	}
	
	return E_OK;
}

static long _cdecl
xdatime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	if (rwflag)
		return EACCES;
	
	*timeptr++ = timestamp;
	*timeptr = datestamp;
	
	return E_OK;
}

static long _cdecl
xclose (FILEPTR *f, int pid)
{
	if (f->links < 0)
	{
		ALERT (("xconio: close(%d), device remaining open", pid));
		return EINTERNAL;
	}
	
	if (f->links == 0)
	{
		/* reset xconout2 (console) vector */
		if (!oldxconout2)
		{
			return EINTERNAL;
		}
		
		*((long *) 0x586) = (long) oldxconout2;
		oldxconout2 = 0;
	}
	
	return E_OK;
}

static long _cdecl
xselect (FILEPTR *f, long proc, int mode)
{
	long ret;

	switch (mode)
	{
		case O_RDONLY:
		{
			if (inbuf)
			{
				ret = 1;
			}
			else if (!rsel)
			{
				ret = 0;
				rsel = proc;
			}
			else
			{
				ret = 2;
			}
			break;
		}
		case O_WRONLY:
		{
			/* we're always ready to print */
			ret = 1;
			break;
		}
		case O_RDWR:
		{
			/* no exceptional consitions */
			ret = 0;
			break;
		}
		default:
		{
			/* we don't know this mode */
			ret = 0;
		}
	}
	
	return ret;
}

static void _cdecl
xunselect (FILEPTR *f, long proc, int mode)
{
	if (mode == O_RDONLY && proc == rsel)
	{
		rsel = 0;
	}
}
