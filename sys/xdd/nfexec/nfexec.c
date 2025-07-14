/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2017 Miro Kropacek <miro.kropacek@gmail.com>
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
 * Author: Miro Kropacek <miro.kropacek@gmail.com>
 * Started: 2017-05-06
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 * bugs/todo:
 *
 */

# include "mint/mint.h"
# include "mint/arch/nf_ops.h"

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

# include "nfexec.h"


/*
 * version
 */

# define VER_MAJOR	0
# define VER_MINOR	1
# define VER_STATUS


/*
 * debugging stuff
 */

# if 1
# define DEV_DEBUG	1
# endif


/*
 * messages
 */

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS)
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p NF host exec driver version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"\275 " MSG_BUILDDATE " by Miro Kropacek.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_NFNOTPRESENT	\
	"\7\r\nSorry, native features not supported!\r\n\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, driver NOT installed - initialization failed!\r\n\r\n"


/****************************************************************************/
/* BEGIN kernel interface */

struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/* NF_STDERR feature ID value fetched by nf_ops->get_id() */
static long nf_id;
/* Cache for the nf_ops->call() function pointer */
static long (*nf_call)(long id, ...);

/*
 * device driver routines - top half
 */
static long _cdecl	nfexec_open	(FILEPTR *f);
static long _cdecl	nfexec_write	(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	nfexec_read	(FILEPTR *f, char *buf, long bytes);
static long _cdecl	nfexec_lseek	(FILEPTR *f, long where, int whence);
static long _cdecl	nfexec_ioctl	(FILEPTR *f, int mode, void *buf);
static long _cdecl	nfexec_datime	(FILEPTR *f, ushort *timeptr, int rwflag);
static long _cdecl	nfexec_close	(FILEPTR *f, int pid);
static long _cdecl	nfexec_select	(FILEPTR *f, long proc, int mode);
static void _cdecl	nfexec_unselect	(FILEPTR *f, long proc, int mode);


/*
 * device driver map
 */

static DEVDRV devtab =
{
	nfexec_open,
	nfexec_write, nfexec_read, nfexec_lseek, nfexec_ioctl, nfexec_datime,
	nfexec_close,
	nfexec_select, nfexec_unselect,
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

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN initialization - top half */

DEVDRV * _cdecl
init_xdd (struct kerinfo *k)
{
	struct dev_descr raw_dev_descriptor =
	{
		&devtab,
		0,		/* dinfo -> fc.aux */
		0,		/* flags */
		NULL,		/* struct tty * */
		0,		/* drvsize */
		S_IFCHR |
		S_IWUSR |
		S_IWGRP |
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
		|| ((MINT_MAJOR == 1) && ((MINT_MINOR < 15) || (MINT_KVERSION < KERINFO_VERSION))))
	{
		c_conws (MSG_MINT);
		goto failure;
	}

	if (!kernel->nf_ops)
	{
		c_conws (MSG_NFNOTPRESENT);
		goto failure;
	}
	nf_id = kernel->nf_ops->get_id("HOSTEXEC");
	nf_call = kernel->nf_ops->call;
	
	if (nf_id == 0 || nf_call == NULL) {
		c_conws (MSG_NFNOTPRESENT);
		goto failure;
	}

	/* this is FILEPTR *f->fc.aux */
	raw_dev_descriptor.dinfo = 0;

	/* install it */
	if (d_cntl (DEV_INSTALL2, "u:\\dev\\hostexec", (long) &raw_dev_descriptor) >= 0)
		DEBUG (("%s: %s installed with BIOS remap", __FILE__, "hostexec"));


	return (DEVDRV *) 1;

failure:
	c_conws (MSG_FAILURE);
	return NULL;
}

/* END initialization - top half */
/****************************************************************************/

/****************************************************************************/
/* BEGIN device driver routines - top half */

static long _cdecl
nfexec_open (FILEPTR *f)
{
	DEBUG (("nfexec_open [%i]: enter (%x)", f->fc.aux, f->flags));

	return E_OK;
}

static long _cdecl
nfexec_close (FILEPTR *f, int pid)
{
	DEBUG (("nfexec_close [%i]: enter", f->fc.aux));

	if (f->links <= 0)
	{
		/* ALERT (("nfexec_close: f->links <= 0!")); */
	}

	return E_OK;
}


/* raw write/read routines
 */
static long _cdecl
nfexec_write (FILEPTR *f, const char *buf, long bytes)
{
	long done = 0;

	DEBUG (("nfexec_write [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));

	done = nf_call(nf_id | 1, buf, bytes);

	DEBUG (("nfexec_write: leave (%ld)", done));
	return done;
}

static long _cdecl
nfexec_read (FILEPTR *f, char *buf, long bytes)
{
	long done = 0;

	DEBUG (("nfexec_read [%i]: enter (%lx, %ld)", f->fc.aux, (unsigned long)buf, bytes));


	DEBUG (("nfexec_read: leave (%ld)", done));
	return done;
}

static long _cdecl
nfexec_lseek (FILEPTR *f, long where, int whence)
{
	DEBUG (("nfexec_lseek [%i]: enter (%ld, %d)", f->fc.aux, where, whence));

	return 0;
}

static long _cdecl
nfexec_ioctl (FILEPTR *f, int mode, void *buf)
{
	long r = E_OK;

	DEBUG (("nfexec_ioctl [%i]: (%x, (%c %i), %lx)", f->fc.aux, mode, (char) (mode >> 8), (mode & 0xff), (unsigned long)buf));

	switch (mode)
	{
		case FIONREAD:
		{
			*((long *) buf) = 0;
			break;
		}
		case FIONWRITE:
		{
			*((long *) buf) = 1;
			break;
		}
		case FIOEXCEPT:		/* anywhere documented? */
		{
			*((long *) buf) = 0;
			break;
		}
		default:
		{
			/* Fcntl will automatically call tty_ioctl to handle
			 * terminal calls that we didn't deal with
			 */
			r = ENOSYS;
			break;
		}
	}

	DEBUG (("nfexec_ioctl: return %li", r));
	return r;
}

static long _cdecl
nfexec_datime (FILEPTR *f, ushort *timeptr, int rwflag)
{
	DEBUG (("nfexec_datime [%i]: enter (%i)", f->fc.aux, rwflag));

	if (rwflag)
		return EACCES;

	*timeptr++ = timestamp;
	*timeptr = datestamp;

	return E_OK;
}

static long _cdecl
nfexec_select (FILEPTR *f, long proc, int mode)
{
	long r;

	DEBUG (("nfexec_select [%i]: enter (%li, %i)", f->fc.aux, proc, mode));

	switch (mode)
	{
		case O_RDONLY:
		{
			r = 0;
			break;
		}
		case O_WRONLY:
		{
			/* we're always ready to print */
			r = 1;
			break;
		}
		case O_RDWR:
		{
			/* no exceptional conditions */
			r = 0;
			break;
		}
		default:
		{
			/* we don't know this mode */
			r = 0;
		}
	}

	return r;
}

static void _cdecl
nfexec_unselect (FILEPTR *f, long proc, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;

	DEBUG (("nfexec_unselect [%i]: enter (0x%lx, %i, 0x%lx)", f->fc.aux, proc, mode, (unsigned long)tty));

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
