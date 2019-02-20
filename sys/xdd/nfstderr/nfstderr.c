/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2005 STanda Opichal <opichals@seznam.cz>
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
 */

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/stat.h"
# include "mint/arch/nf_ops.h"


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
# define VER_MINOR	1
# define VER_STATUS	

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p NF_STDERR native feature output device version " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"½ Jan  25 2005 by STanda Opichal <opichals@seznam.cz>\r\n" \
	"½                 build date " MSG_BUILDDATE ".\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_NFNOTPRESENT	\
	"\7\r\nSorry, native features not supported!\r\n\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, nfstderr NOT installed (Dcntl(...) failed)!\r\n\r\n"


/*
 * default settings
 */

# define INSTALL	"u:\\dev\\nfstderr"

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

/* NF_STDERR feature ID value fetched by nf_ops->get_id() */
long nf_stderr_id = 0;
/* Cache for the nf_ops->call() function pointer */
long (*nf_call)(long id, ...);


DEVDRV * _cdecl		init		(struct kerinfo *k);


/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

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
	
	if ( ! kernel->nf_ops ) {
		c_conws (MSG_NFNOTPRESENT);
		return NULL;
	}
	       
	nf_stderr_id = kernel->nf_ops->get_id( "NF_STDERR" );
	nf_call = kernel->nf_ops->call;
	
	r = d_cntl (DEV_INSTALL, INSTALL, (long) &dev_descriptor);
	if (r < 0)
	{
		c_conws (MSG_FAILURE);
		return NULL;
	}
	
	DEBUG (("%s: init ok", __FILE__));
	return (DEVDRV *) 1;
}


/*
 *	the device driver routines
 */

static long _cdecl
xopen (FILEPTR *f)
{
	
	return E_OK;
}

static long _cdecl
xwrite (FILEPTR *f, const char *buf, long bytes)
{
	char outb[2] = { '\0', '\0' };
	long nwrite = 0;

	while (bytes > 0)
	{
		/* call host os to print the data */

		/* byte by byte because of NF_STDERR operates with
		 * char* as with a string and not a byte array
		 * ('\0' would terminate the output)
		 **/
		outb[0] = buf[nwrite];
		nf_call( nf_stderr_id, outb);

		nwrite++;
		bytes--;
	}
	
	return nwrite;
}

static long _cdecl
xread (FILEPTR *f, char *buf, long bytes)
{
	return 0;
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
			*((long *) buf) = 0;
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
		ALERT (("nfstderr: close(%d), device remaining open", pid));
		return EINTERNAL;
	}
	
	if (f->links == 0)
	{
		/* unhook whatever hooked */
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
			ret = 0;
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
			/* no exceptional conditions */
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
}
