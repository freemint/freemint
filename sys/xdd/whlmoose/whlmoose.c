/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2003 Odd Skancke <ozk@atari.org>
 * Copyright 1998, 2000 Frank Naumann <fnaumann@freemint.de>
 * Copyright 1995 Torsten Scherer <itschere@techfak.uni-bielefeld.de>
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
 * Author: Odd Skancke <ozk@atari.org>
 * Started: 2003-05-17
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * 2003-06-17:	(v0.3)
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

# include "mint/mint.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/stat.h"

# include "whlmoose.h"

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
# define VER_MINOR	4
# define VER_STATUS	

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Moose " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	"½ May 17 2003 by Odd Skancke <ozk@atari.org>\r\n" \
	"½ " MSG_BUILDDATE " by Odd Skancke for AssemSoft.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, Moose NOT installed (Dcntl(...) failed)!\r\n\r\n"


/*
 * default settings
 */

# define INSTALL	"u:\\dev\\moose"
# define INSTALL1	"u:\\dev\\mooses"

/****************************************************************************/
/* BEGIN kernel interface */

static struct kerinfo *kernel;

/* END kernel interface */
/****************************************************************************/

/****************************************************************************/
/* BEGIN definition part */

/*
 * assembler functions
 */

void butv (void);
void motv (void);
void whlv (void);
void timv (void);
void th_wrapper(void);


/*
 * exported functions
 */

void cbutv (void);
void cwhlv (void);
void timer_handler(void);


/*
 * internal functions
 */

static void wake_listeners(void);
static void gen_write(struct moose_data *md);
static void do_button_packet(void);


/*
 * device driver
 */

static long _cdecl	open		(FILEPTR *f);
static long _cdecl	write		(FILEPTR *f, const char *buf, long bytes);
static long _cdecl	read		(FILEPTR *f, char *buf, long bytes);
static long _cdecl	lseek		(FILEPTR *f, long where, int whence);
static long _cdecl	ioctl		(FILEPTR *f, int mode, void *buf);
static long _cdecl	datime		(FILEPTR *f, unsigned short *timeptr, int rwflag);
static long _cdecl	close		(FILEPTR *f, int pid);
static long _cdecl	select		(FILEPTR *f, long proc, int mode);
static void _cdecl	unselect	(FILEPTR *f, long proc, int mode);

static DEVDRV devtab =
{
	open,
	write, read, lseek, ioctl, datime,
	close,
	select, unselect,
	write, read
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

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

#define MAX_DC_TIME	200
#define SYSTIMER	0x4baL
#define SYSVBI		0x456L
#define SYSNVBI		0x454L

static long *VBI_entry;

static short moose_inuse;

static short click_x;
static short click_y;
static short click_state;
static short click_cstate;
static short click_count;

static short timeout;
static short dc_time;
static short last_state;
static short last_time;
static short halve;

static struct mouse_pak *pak_head;
static struct mouse_pak *pak_tail;
static struct mouse_pak *pak_end;
static short inbuf;
static long rsel;

static short rptr;
static short wptr;
static short mused;

static short old_x;
static short old_y;
static short mmoved;

volatile short sample_butt;
volatile short sample_x;
volatile short sample_y;
volatile short sample_wheel;
volatile short sample_wclicks;


struct mouse_pak
{
	unsigned short len;

#define BUT_PAK 	0x42
#define WHL_PAK		0x57
	char ty;

	union
	{
		struct
		{
			unsigned char state;
			short time;
		} but;
		
		struct
		{
			unsigned char wheel;
			short clicks;
		} whl;
		
		struct
		{
			char state;
		} vbut;
	} t;
	short x;
	short y;
	long dbg;
};

#define MB_BUFFERS	32
#define MB_BUFFER_SIZE	(MB_BUFFERS * (sizeof(struct moose_data)))
#define PAK_BUFFERS	32
#define PAK_BUFFER_SIZE	(PAK_BUFFERS * (sizeof(struct mouse_pak)))

static short moose_buffer[MB_BUFFER_SIZE];
static short pak_buffer[PAK_BUFFER_SIZE];

/* END global data & access implementation */
/****************************************************************************/

void
cbutv(void)
{
	pak_tail->len		= sizeof(struct mouse_pak);
	pak_tail->ty		= BUT_PAK;
	pak_tail->t.but.state	= (unsigned char)sample_butt;
	pak_tail->t.but.time	= *(short *)(SYSTIMER+2);
	pak_tail->x		= sample_x;
	pak_tail->y		= sample_y;
	pak_tail->dbg		= 0;

	pak_tail++;

	if (pak_tail > pak_end)
		pak_tail = (struct mouse_pak *)&pak_buffer;

	inbuf	+= sizeof(struct mouse_pak);
	return;
}

void
cwhlv(void)
{
	pak_tail->len		= sizeof(struct mouse_pak);
	pak_tail->ty		= WHL_PAK;
	pak_tail->t.whl.wheel	= (unsigned char)sample_wheel;
	pak_tail->t.whl.clicks	= sample_wclicks;
	pak_tail->x		= sample_x;
	pak_tail->y		= sample_y;
	pak_tail->dbg		= 0;

	pak_tail++;

	if (pak_tail > pak_end)
		pak_tail = (struct mouse_pak *)&pak_buffer;

	inbuf	+= sizeof(struct mouse_pak);
	return;
}

void
timer_handler(void)
{

	halve--;

	if (halve)
		return;

	halve = 2;

	if (!mmoved && (sample_x != old_x || sample_y != old_y))
	{
		mmoved = 1;
		wake_listeners();
	}

	if (!inbuf)
	{
		if (!timeout)
			return;
		else
		{
			short t = *(short *)(SYSTIMER+2);

			t		-= last_time;
			last_time	= *(short *)(SYSTIMER+2);
			timeout		-= t;

			if (timeout > 0)
				return;
			else
				do_button_packet();

			return;
		}
	}
	else
	{
		while (inbuf)
		{
			if (pak_head->ty == BUT_PAK)
			{	
				short tm = pak_head->t.but.time;
				short s = (unsigned char)pak_head->t.but.state;

				if (timeout)
				{
					short t = tm;

					t		-= last_time;
					last_time	= tm;
					timeout		-= t;

					if (timeout <= 0)
						do_button_packet();
				}
			
				if (s != last_state) /* We skip identical button state events */
				{
					last_state = s;

					if (timeout)
					{
					/* Add clicks to a not-yet-timed-out button packet */
						click_state |= s;
						click_cstate = s;
						if (s & 1)
							click_count++;
						else if (s & 2)
							click_count++;
						if (click_count > 1)
							timeout += 10;	/* extend timeout, so a triple-click becomes easier */
					}
					else
					{
					/* NEW PACKET */
						click_state	= s;
						click_cstate	= s;
						click_x		= pak_head->x;
						click_y		= pak_head->y;

						if (s & 3)
						/* Ozk 180603: Starting a new event... */
						{
							last_time	= tm;
							timeout		= dc_time;
							click_count	= 1;
						}
						else
						/* Ozk 180603: A new event, but with a initial button-released state
						*  makes us send it immediately. This is because the previous event
						*  most likely was sent while button(s) were still pressed (click-hold).
						*/
						{
							click_count	= 0;
							do_button_packet();
						}
					}
				}
			}
#ifdef	HAVE_WHEEL 
			else if (pak_head->ty == WHL_PAK)
			{
				struct moose_data md;

				md.l		= sizeof(md);
				md.ty		= MOOSE_BUTTON_PREFIX;
				md.x		= pak_head->x;
				md.y		= pak_head->y;
				md.state	= pak_head->t.whl.wheel;
				md.cstate	= md.state;
				md.clicks	= pak_head->t.whl.clicks;
				md.dbg1		= 0;
				md.dbg2		= 0;

				gen_write(&md);
			}
#endif
			/* more pak types here ... */
			pak_head++;
			if (pak_head > pak_end)
				pak_head = (struct mouse_pak *)&pak_buffer;
			inbuf -= sizeof(struct mouse_pak);
		}
	}
	return;
}

static void
do_button_packet(void)
{
	struct moose_data md;

	md.l		= sizeof(md);
	md.ty		= MOOSE_BUTTON_PREFIX;
	md.x		= click_x;
	md.y		= click_y;
	md.state	= click_state;
	md.cstate	= click_cstate;
	md.clicks	= click_count;
	md.dbg1		= 0;
	md.dbg2		= 0;

	gen_write(&md);

	timeout = 0;

	return;
}

static void
gen_write(struct moose_data *md)
{

	short *mb = (short *)&moose_buffer;
	short *buf = (short *) md;
	short len;
	int i;
	
	if (!moose_inuse)
		return;

	if (wptr == rptr)
	{
		wptr	= 0;
		rptr	= 0;
		mused	= 0;
	}

	(char *)mb += wptr;
	len = md->l >> 1;

	if ((wptr + md->l) > MB_BUFFER_SIZE)
	{
		for (i = 0; i < len; i++)
		{
			if (wptr > MB_BUFFER_SIZE)
			{
				wptr 	= 0;
				mb	= (short *)&moose_buffer;
			}
			*mb++	= *buf++;
			wptr	+= 2;
		}
	}
	else
	{
		for (i = 0;i < len; i++)
			*mb++ = *buf++;

		wptr += md->l;
	}

	mused += md->l;

	if (wptr > MB_BUFFER_SIZE)
		wptr = 0;

	wake_listeners();

	return;
}

static void
wake_listeners(void)
{
	if (rsel)
	{
		wakeselect(rsel);
		rsel = 0;
	}
	return;
}


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

	
	dev_descriptor.dinfo = 0;
	r = d_cntl (DEV_INSTALL, INSTALL, (long) &dev_descriptor);
	if (r < 0)
	{
		c_conws (MSG_FAILURE);
		return NULL;
	}

	dev_descriptor.dinfo = 1;
	r = d_cntl (DEV_INSTALL, INSTALL1, (long) &dev_descriptor);
	if (r < 0)
	{
		c_conws (MSG_FAILURE);
		return NULL;
	}

	
	DEBUG (("%s: init ok", __FILE__));
	return (DEVDRV *) 1;
}


static long _cdecl
open (FILEPTR *f)
{

	if (f->fc.aux == 1)
		return E_OK;
	else
	{
		short nvbi;
		int i;

		nvbi		= *(short *)SYSNVBI;

		pak_tail	= (struct mouse_pak *)&pak_buffer;
		pak_head	= pak_tail;
		pak_end		= pak_tail + PAK_BUFFERS;
		halve		= 0;
		inbuf		= 0;
		rsel		= 0;
		rptr		= 0;
		wptr		= 0;
		mused		= 0;
		halve		= 4;

		VBI_entry = (long *) *(long *)SYSVBI;

		for (i = 0; i < nvbi; i++)
		{
			if (*VBI_entry == 0)
			{
				*VBI_entry = (long)th_wrapper;
				break;
			}
			VBI_entry++;
		}

		moose_inuse	= -1;
		dc_time		= 0;
	}

	return E_OK;
}

static long _cdecl
write (FILEPTR *f, const char *buf, long bytes)
{
	if (f->fc.aux == 1)
		return EACCES;

	return 0;
}

static long _cdecl
read (FILEPTR *f, char *buf, long bytes)
{
	register long ret = 0;

//	register unsigned short sr;
//	sr = splhigh ();
//	spl (sr);
	
	if (f->fc.aux == 1)
	{
		struct mooses_data *md = (struct mooses_data *) buf;

		md->state	= sample_butt;
		md->x		= sample_x;
		md->y		= sample_y;
		return sizeof(struct mooses_data);
	}
	else
	{
		if (!bytes)
			return ret;

	/* If button packets pending, prioritize those */
		if (mused)
		{
			short *mb = (short *)&moose_buffer;
			short *b = (short *) buf;
			long len = bytes;

			(char *)mb += rptr;

			if ((rptr + bytes) > MB_BUFFER_SIZE)
			{
				while(len)
				{
					if (rptr > MB_BUFFER_SIZE)
					{
						mb = (short *)&moose_buffer;
						rptr = 0;
					}
					*b++	= *mb++;
					rptr	+= 2;
					len	-= 2;
				}
			}
			else
			{
				while(len)
				{
					*b++	= *mb++;
					len	-= 2;
				}
				rptr +=	bytes;
			}

			mused	-= bytes;
			return bytes;
		}
	/* No button packets pending, check if mouse have moooooved */
		else if (mmoved && (bytes == sizeof(struct moose_data)))
		{
			struct moose_data *md = (struct moose_data *) buf;

			md->l		= sizeof(struct moose_data);
			md->ty		= MOOSE_MOVEMENT_PREFIX;
			md->x		= sample_x;
			md->y		= sample_y;
			md->state	= click_state;
			md->cstate	= click_cstate;
			md->clicks	= 0;
			md->dbg1	= 0;
			md->dbg2	= 0;

			old_x	= sample_x;
			old_y	= sample_y;
			mmoved	= 0;
			return sizeof(struct moose_data);
		}
	}
	return ret;
}

static long _cdecl
lseek (FILEPTR *f, long where, int whence)
{
	/* (terminal) devices are always at position 0 */
	return E_OK;
}

static long _cdecl
ioctl (FILEPTR *f, int mode, void *buf)
{
	switch (mode)
	{
		case FS_INFO:
		{
			struct fs_info *info = buf;
			
			info->version = (((long)VER_MAJOR << 16) | VER_MINOR);
			strcpy(info->name, "moose.xdd");
			strcpy(info->type_asc, "moose device for XaAES");
			
			break;
		}
		case FIONREAD:
		{
			if (f->fc.aux == 1)
				*((long *) buf) = sizeof(struct mooses_data);
			else
			{
				*((long *) buf) = (long) mused;
				if (mmoved)
					*((long *) buf) += sizeof(struct moose_data);
			}
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
		case MOOSE_READVECS:
		{
			struct moose_vecsbuf *vecs = buf;
			
			vecs->motv = (vdi_vec *) motv;
			vecs->butv = (vdi_vec *) butv;
			vecs->timv = (vdi_vec *) timv;
#ifdef HAVE_WHEEL
			vecs->whlv = (vdi_vec *) whlv;
#else
			vecs->whlv = (vdi_vec *) 0;
#endif
			break;
		}
		case MOOSE_DCLICK :
		{
			unsigned short dclick_time = *((unsigned short *) buf);
			
			if (dclick_time > MAX_DC_TIME)
				dc_time = MAX_DC_TIME;
			else
				dc_time = dclick_time;
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
datime (FILEPTR *f, unsigned short *timeptr, int rwflag)
{
	if (rwflag)
		return EACCES;
	
	*timeptr++ = timestamp;
	*timeptr = datestamp;
	
	return E_OK;
}

static long _cdecl
close (FILEPTR *f, int pid)
{
	if (f->links < 0)
		return EINTERNAL;

	if (f->links == 0 && f->fc.aux == 0)
	{
		*VBI_entry = 0;
		moose_inuse = 0;
		dc_time = 0;
	}
	
	return E_OK;
}

static long _cdecl
select (FILEPTR *f, long proc, int mode)
{
	long ret;

	switch (mode)
	{
		case O_RDONLY:
		{

			if (f->fc.aux == 0)
			{
				if (mused || mmoved)
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
			}
			else if (f->fc.aux == 1)
				ret = 1;			/* Always ready to read mooses */
			else
				ret = 0;
			
			break;
		}
		case O_WRONLY:
		{
			ret = 1;
			break;
		}
		case O_RDWR:
		{
			ret = 0;
			break;
		}
		default:
		{
			ret = 0;
		}
	}
	
	return ret;
}

static void _cdecl
unselect (FILEPTR *f, long proc, int mode)
{
	if (mode == O_RDONLY && proc == rsel)
	{
		rsel = 0;
	}
}
