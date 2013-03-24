/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 200? ???? ?????? <????@freemint.de>
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
 * Author: ???? ?????? <????@freemint.de>
 * Started: 200?-0?-??
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * 
 * 200?-??-??:	(v?.??)
 * 
 * - inital revision
 * 
 * 
 * bugs/todo:
 * 
 * - known bugs
 * 
 */

# include "mint/mint.h"

# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/signal.h"
# include "mint/ssystem.h"
# include "mint/stat.h"
# include "cookie.h"

# include "libkern/libkern.h"

// if you need Setexc() or so
// # include <osbind.h>

# include "dummy.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	00
# define VER_STATUS	


/*
 * debugging stuff
 */

# if 1
# define DEV_DEBUG	1
# endif

# if 0
# define INT_DEBUG	1
# endif


/*
 * default settings
 */



/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Dummy device driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 " MSG_BUILDDATE " by ???? ???????.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_MILAN	\
	"\033pThis driver requires a Milan!\033q\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, driver NOT installed - initialization failed!\r\n\r\n"


/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

DEVDRV * _cdecl init(struct kerinfo *k);


// XXX
// 
// your definitions (structures, defines, ...)
// internal prototypes
// etc.


/*
 * device driver routines - top half
 */
static long _cdecl	dummy_open	(FILEPTR *f);
static long _cdecl	dummy_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	dummy_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	dummy_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	dummy_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	dummy_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	dummy_close	(FILEPTR *f, int pid);
static long _cdecl	dummy_select	(FILEPTR *f, long proc, int mode);
static void _cdecl	dummy_unselect	(FILEPTR *f, long proc, int mode);


/*
 * device driver map
 */

static DEVDRV raw_devtab =
{
	dummy_open,
	dummy_write, dummy_read, dummy_lseek, dummy_ioctl, dummy_datime,
	dummy_close,
	dummy_select, dummy_unselect,
	NULL, NULL
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

# ifdef INT_DEBUG
#  define DEBUG_I(x)	KERNEL_DEBUG x
#  define TRACE_I(x)	KERNEL_TRACE x
#  define ALERT_I(x)	KERNEL_ALERT x
# else
#  define DEBUG_I(x)
#  define TRACE_I(x)
#  define ALERT_I(x)	KERNEL_ALERT x
# endif

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN buffer manipulation - mixed */


/* END buffer manipulation - mixed */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

/*
 * global data structures
 */


/* END global data & access implementation */
/****************************************************************************/

/****************************************************************************/
/* BEGIN locking functions - bottom/top half */



/* END locking functions - bottom/top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

DEVDRV * _cdecl
init (struct kerinfo *k)
{
//	long mch;
	
	struct dev_descr raw_dev_descriptor =
	{
		&raw_devtab,
		0,		/* dinfo -> fc.aux */
		0,		/* flags */
		NULL,		/* struct tty * */
		0,		/* drvsize */
		S_IFCHR |
		S_IRUSR |
		S_IWUSR |
		S_IRGRP |
		S_IWGRP |
		S_IROTH |
		S_IWOTH,	/* fmode */
		0,		/* bdevmap */
		0,		/* bdev */
		0		/* reserved */
	};
	
	
	kernel = k;
	
	c_conws (MSG_BOOT);
	c_conws (MSG_GREET);
	
	DEBUG (("%s: enter init", __FILE__));
	
	if ((MINT_MAJOR == 0)
		|| ((MINT_MAJOR == 1) && ((MINT_MINOR < 15) || (MINT_KVERSION < 2))))
	{
		c_conws (MSG_MINT);
		goto failure;
	}
	
# if 0
	// require a Milan or any other special hardware?
	if ((s_system (S_GETCOOKIE, COOKIE__MCH, (long) &mch) != 0)
		|| (mch != MILAN_C))
	{
		c_conws (MSG_MILAN);
		goto failure;
	}
# endif
	
	// this is FILEPTR *f->fc.aux
	raw_dev_descriptor.dinfo = 0;
	
	// install it
	if (d_cntl (DEV_INSTALL2, "u:\\dev\\dummy", (long) &raw_dev_descriptor) >= 0)
		DEBUG (("%s: %s installed with BIOS remap", __FILE__, "dummy"));
	
	
	return (DEVDRV *) 1;
	
failure:
	c_conws (MSG_FAILURE);
	return NULL;
}

/* END initialization - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN interrupt handling - bottom half */

# if 0
// 
// call back for time consuming jobs
// or if you need to do things that must be synchron to the kernel
// 
// the check_ioevent(PROC *p, arg) is called after all interrupt processing
// is done
// 
// check_ioevent() can be interrupted too
// 
INLINE void
notify_top_half (IOVAR *iovar)
{
	if (!iovar->iointr)
	{
		TIMEOUT *t;
		
		t = addroottimeout (0L, check_ioevent, 0x1);
		if (t)
		{
			t->arg = (long) iovar;
			iovar->iointr = 1;
		}
	}
}
# endif

/* END interrupt handling - bottom half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN interrupt handling - top half */

# if 0
// 
// this routine process interrupt events after the real interrupt
// can be interrupted too, run synchron to the kernel
// 
// for example wake processes if I/O become possible
// 
static void
check_ioevent (PROC *p, long arg)
{
	IOVAR *iovar = (IOVAR *) arg;
	
	iovar->iointr = 0;
	
	if (iovar->tty.rsel)
	{
		if (!iorec_empty (&iovar->input))
		{
			DEBUG (("dummy.xdd: wakeselect -> read (%s)", p->fname));
			wakeselect (iovar->tty.rsel);
		}
	}
	
	if (iovar->tty.wsel)
	{
		if (iorec_used (&iovar->output) < iovar->output.low_water)
		{
			DEBUG (("dummy.xdd: wakeselect -> write (%s)", p->fname));
			wakeselect (iovar->tty.wsel);
		}
	}
}
# endif

/* END interrupt handling - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver routines - top half */

static long _cdecl
dummy_open (FILEPTR *f)
{
//	ushort dev = f->fc.aux;
	
	DEBUG (("dummy_open [%i]: enter (%lx)", f->fc.aux, f->flags));
	
# if 0
	if (dev >= IOVAR_REAL_MAX)
		return EACCES;
	
	if (!iovar->open)
	{
		/* first open */
		
		// assign ressources
		// do whatever is neccessary to bring the device up and running
	}
	else
	{
		if (denyshare (iovar->open, f))
		{
			DEBUG (("dummy_open: file sharing denied"));
			return EACCES;
		}
	}
	
	f->pos = 0;
	f->next = iovar->open;
	iovar->open = f;
	
	DEBUG (("dummy_open: return E_OK (added %lx)", f));
	return E_OK;
# else
	return EPERM;
# endif
}

static long _cdecl
dummy_close (FILEPTR *f, int pid)
{
	DEBUG (("dummy_close [%i]: enter", f->fc.aux));
	
# if 0
	if ((f->flags & O_LOCK)
	    && ((iovar->lockpid == pid) || (f->links <= 0)))
	{
		DEBUG (("dummy_close: remove lock by %i", pid));
		
		f->flags &= ~O_LOCK;
		iovar->lockpid = -1;
		
		/* wake anyone waiting for this lock */
		wake (IO_Q, (long) iovar);
	}
	
	if (f->links <= 0)
	{
		register FILEPTR **temp;
		register long flag = 1;
		
		DEBUG (("dummy_close: freeing FILEPTR %lx", f));
		
		/* remove the FILEPTR from the linked list */
		temp = &iovar->open;
		while (*temp)
		{
			if (*temp == f)
			{
				*temp = f->next;
				f->next = NULL;
				flag = 0;
				
				break;
			}
			
			temp = &(*temp)->next;
		}
		
		if (flag)
			ALERT (("dummy_close: remove open FILEPTR fail!", f->fc.aux));
		
		//
		// cleanup
		// free ressources
	}
# endif
	
	return E_OK;
}


/* raw write/read routines
 */
static long _cdecl
dummy_write (FILEPTR *f, const char *buf, long bytes)
{
	long done = 0;
	
	DEBUG (("dummy_write [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
	
	DEBUG (("dummy_write: leave (%ld)", done));
	return done;
}

static long _cdecl
dummy_read (FILEPTR *f, char *buf, long bytes)
{
	long done = 0;
	
	DEBUG (("dummy_read [%i]: enter (%lx, %ld)", f->fc.aux, buf, bytes));
	
	
	DEBUG (("dummy_read: leave (%ld)", done));
	return done;
}

static long _cdecl
dummy_lseek (FILEPTR *f, long where, int whence)
{
	DEBUG (("dummy_lseek [%i]: enter (%ld, %d)", f->fc.aux, where, whence));
	
	return 0;
}

static long _cdecl
dummy_ioctl (FILEPTR *f, int mode, void *buf)
{
	long r = E_OK;
	
	DEBUG (("dummy_ioctl [%i]: (%x, (%c %i), %lx)", f->fc.aux, mode, (char) (mode >> 8), (mode & 0xff), buf));
	
	switch (mode)
	{
		case FIONREAD:
		{
// must be implemented
			r = ENOSYS;
			break;
		}
		case FIONWRITE:
		{
// must be implemented
			r = ENOSYS;
			break;
		}
		case FIOEXCEPT:		/* anywhere documented? */
		{
// must be implemented
			r = ENOSYS;
			break;
		}
		
# if 0
// 
// file locking
// just correct the pointer to your data structures and add
// these members
// 
		case F_SETLK:
		case F_SETLKW:
		{
			struct flock *lck = (struct flock *) buf;
			int cpid = p_getpid ();
			
			while (iovar->lockpid >= 0 && iovar->lockpid != cpid)
			{
				if (mode == F_SETLKW && lck->l_type != F_UNLCK)
				{
					DEBUG (("dummy_ioctl: sleep in SETLKW"));
					sleep (IO_Q, (long) iovar);
				}
				else
					return ELOCKED;
			}
			
			if (lck->l_type == F_UNLCK)
			{
				if (!(f->flags & O_LOCK))
				{
					DEBUG (("dummy_ioctl: wrong file descriptor for UNLCK"));
					return ENSLOCK;
				}
				
				if (iovar->lockpid != cpid)
					return ENSLOCK;
				
				iovar->lockpid = -1;
				f->flags &= ~O_LOCK;
				
				/* wake anyone waiting for this lock */
				wake (IO_Q, (long) iovar);
			}
			else
			{
				iovar->lockpid = cpid;
				f->flags |= O_LOCK;
			}
			
			break;
		}
		case F_GETLK:
		{
			struct flock *lck = (struct flock *) buf;
			
			if (iovar->lockpid >= 0)
			{
				lck->l_type = F_WRLCK;
				lck->l_start = lck->l_len = 0;
				lck->l_pid = iovar->lockpid;
			}
			else
				lck->l_type = F_UNLCK;
			
			break;
		}
# endif
		
		default:
		{
			/* Fcntl will automatically call tty_ioctl to handle
			 * terminal calls that we didn't deal with
			 */
			r = ENOSYS;
			break;
		}
	}
	
	DEBUG (("dummy_ioctl: return %li", r));
	return r;
}

static long _cdecl
dummy_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	DEBUG (("dummy_datime [%i]: enter (%i)", f->fc.aux, rwflag));
	
	if (rwflag)
		return EACCES;
	
	*timeptr++ = timestamp;
	*timeptr = datestamp;
	
	return E_OK;
}

static long _cdecl
dummy_select (FILEPTR *f, long proc, int mode)
{
// 
// look at the scc/uart xdd for examples
//
	DEBUG (("dummy_select [%i]: enter (%li, %i)", f->fc.aux, proc, mode));
	
	
	/* default -- we don't know this mode, return 0 */
	return 0;
}

static void _cdecl
dummy_unselect (FILEPTR *f, long proc, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;
	
	DEBUG (("dummy_unselect [%i]: enter (%li, %i, %lx)", f->fc.aux, proc, mode, tty));
	
	if (tty)
	{
		if (mode == O_RDONLY && tty->rsel == proc)
			tty->rsel = 0;
		else if (mode == O_WRONLY && tty->wsel == proc)
			tty->wsel = 0;
	}
}

/* END device driver routines - top half */
/****************************************************************************/
