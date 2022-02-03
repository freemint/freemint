/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
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

# include "dev-mouse.h"

# include "arch/intr.h"
# include "arch/init_intr.h"
# include "arch/syscall.h"
# include "mint/ioctl.h"

# include "biosfs.h"
# include "dev-null.h"
# include "proc.h"
# include "random.h"
# include "time.h"


static long	_cdecl mouse_open	(FILEPTR *f);
static long	_cdecl mouse_read	(FILEPTR *f, char *buf, long nbytes);
static long	_cdecl mouse_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl mouse_close	(FILEPTR *f, int pid);
static long	_cdecl mouse_select	(FILEPTR *f, long p, int mode);
static void	_cdecl mouse_unselect 	(FILEPTR *f, long p, int mode);

DEVDRV mouse_device =
{
	mouse_open,
	null_write, mouse_read, null_lseek,
	mouse_ioctl, null_datime,
	mouse_close,
	mouse_select, mouse_unselect,
	NULL, NULL
};


/*
 * mouse device driver
 */

# define MOUSESIZ 128 * 4
static uchar mousebuf [MOUSESIZ];
static int mousehead;
static int mousetail;

/* is someone calling select() on the mouse?
 */
static long mousersel;

/* shift key status; set by checkkeys() in bios.c
 */
char mshift = 0;

/* mouse pos. variables; used by big screen emulators
 */
static short *gcurx = 0;
static short *gcury = 0;


/* buf must be a *signed* character
 */

void _cdecl
mouse_handler (const char *buf)
{
	uchar *mbuf, buttons;
	int newmtail;
	short dx, dy;
	
	/* the Sun mouse driver has 0=down, 1=up, while the atari hardware
	 * gives us the reverse. also, we have the "middle" button and the
	 * "left" button reversed; so we use this table to convert (and
	 * also to add the 0x80 to indicate a mouse packet)
	 */
	static int _cnvrt [8] =
	{
		0x87, 0x86, 0x83, 0x82, 0x85, 0x84, 0x81, 0x80
	};
	
# ifdef DEV_RANDOM
	/* FIXME:  We only get here if somebody opened "/dev/mouse" for
	 * reading/writing.
	 */
	union { const char *c; ulong *ul; } ptr;
	ptr.c = buf;
	add_mouse_randomness( *ptr.ul);
# endif
	
	mbuf = &mousebuf [mousetail];
	newmtail = mousetail + 3;
	if (newmtail >= MOUSESIZ)
		newmtail = 0;
	
	if (newmtail == mousehead)
	{
		/* buffer full */
		return;
	}
	
	buttons = *buf++ & 0x7;		/* convert to SUN format */
	if (mshift & 0x3)		/* a shift key held down? */
	{
		/* if so, convert shift+button to a "middle" button */
		if (buttons == 0x1 || buttons == 0x2)
			buttons = 0x4;
		else if (buttons == 0x3)
			buttons = 0x7;
	}
	
	*mbuf++ = _cnvrt [buttons];	/* convert to Sun format */
	dx = *buf++;
	*mbuf++ = dx;			/* copy X delta */
	dy = *buf;
	*mbuf = -dy;			/* invert Y delta for Sun format */
	mousetail = newmtail;
	*gcurx += dx;			/* update line A variables */
	*gcury += dy;
	
	/* if someone has called select() waiting for mouse input, wake them
	 * up
	 */
	if (mousersel)
		wakeselect ((PROC *) mousersel);
}

long oldvec = 0;
long oldjvec = 0;

static long _cdecl
mouse_open (FILEPTR *f)
{
	static char parameters [] =
	{
		0, 		/* Y=0 in lower corner */
		0,		/* normal button handling */
		1, 1		/* X, Y scaling factors */
	};
	
	UNUSED (f);
	
	if (oldvec)
	{
		/* mouse in use */
		return EACCES;
	}
	
	/* initialize pointers to line A variables */
	if (!gcurx)
	{
		char *aline;
		
		aline = lineA0 ();
		if (aline == 0)		/* should never happen */
		{
			ALERT ("unable to read line A variables");
			return -1;
		}
		
		gcurx = (short *) (aline - 0x25a);
		gcury = (short *) (aline - 0x258);
		
		/* magic number -- what MGR uses */
		*gcurx = *gcury = 32;
	}
	
	oldvec = syskey->mousevec;
	
	/* jr: save old joystick vector */
	oldjvec = syskey->joyvec;
	
	ROM_Initmous (1, parameters, newmvec);
	
	/* jr: set up new joystick handler */
	syskey->joyvec = (long) newjvec;
	
	mousehead = mousetail = 0;
	
	return E_OK;
}

static long _cdecl
mouse_close (FILEPTR *f, int pid)
{
	static char parameters [] =
	{
		0, 		/* Y=0 in lower corner */
		0,		/* normal button handling */
		1, 1		/* X, Y scaling factors */
	};
	
	UNUSED (pid);
	
	if (!f)
		return EBADF;
	
	if (f->links <= 0)
	{
		if (!oldvec)
		{
			DEBUG (("Mouse not open!!"));
			return -1;
		}
		
		ROM_Initmous (1, parameters, oldvec);
		
		/* jr: restore old joystick handler */
		syskey->joyvec = oldjvec;
		
		oldvec = 0;
	}
	
	return E_OK;
}

static long _cdecl
mouse_read (FILEPTR *f, char *buf, long nbytes)
{
	long count = 0;
	int mhead = mousehead;
	uchar *foo = &mousebuf[mhead];
	
	if (mhead == mousetail)
	{
		if (f->flags & O_NDELAY)
			return 0;
		
		do {
			yield ();
		}
		while (mhead == mousetail);
	}
	
	while ((mhead != mousetail) && (nbytes > 0))
	{
		*buf++ = *foo++;
		mhead++;
		if (mhead >= MOUSESIZ)
		{
			mhead = 0;
			foo = mousebuf;
		}
		count++;
		--nbytes;
	}
	
	mousehead = mhead;
	
	if (count > 0)
	{
		struct bios_file *b = (struct bios_file *) f->fc.index;
		
		b->xattr.atime = timestamp;
		b->xattr.adate = datestamp;
	}
	
	return count;
}

static long _cdecl
mouse_ioctl (FILEPTR *f, int mode, void *buf)
{
	long r;
	
	UNUSED(f);
	
	switch (mode)
	{
		case FIONREAD:
		{
			r = mousetail - mousehead;
			if (r < 0) r += MOUSESIZ;
			*((long *) buf) = r;
			break;
		}
		case FIONWRITE:
		{
			*((long *) buf) = 0;
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
mouse_select (FILEPTR *f, long p, int mode)
{
	UNUSED (f);
	
	if (mode != O_RDONLY)
	{
		if (mode == O_WRONLY)
			return 1;	/* we can always take output :-) */
		else
			return 0;	/* but don't care for anything else */
	}
	
	if (mousetail - mousehead)
		return 1;		/* input waiting already */
	
	if (mousersel)
		return 2;		/* collision */
	
	mousersel = p;
	
	return 0;
}

static void _cdecl
mouse_unselect (FILEPTR *f, long p, int mode)
{
	UNUSED (f);
	
	if (mode == O_RDONLY && mousersel == p)
		mousersel = 0;
}
