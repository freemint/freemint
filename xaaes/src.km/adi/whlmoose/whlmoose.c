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
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
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

# include "mint/mint.h"
# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/dcntl.h"
# include "mint/file.h"
# include "mint/ioctl.h"
# include "mint/proc.h"
# include "mint/stat.h"

# include "../adidefs.h"


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
# define VER_MINOR	10
# define VER_STATUS	

# define MSG_VERSION	str (VER_MAJOR) "." str (VER_MINOR) str (VER_STATUS) 
# define MSG_BUILDDATE	__DATE__

# define MSG_BOOT	\
	"\033p Moose " MSG_VERSION " \033q\r\n"

# define MSG_GREET	\
	" May 17 2003 by Odd Skancke <ozk@atari.org>\r\n" \
	" " MSG_BUILDDATE " by Odd Skancke for AssemSoft.\r\n\r\n"

# define MSG_MINT	\
	"\033pMiNT too old!\033q\r\n"

# define MSG_FAILURE	\
	"\7\r\nSorry, Moose NOT installed (Dcntl(...) failed)!\r\n\r\n"

/****************************************************************************/
/* BEGIN kernel interface */

struct kentry	*kentry;
struct adiinfo	*ainf;

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
void cmotv (void);
void cwhlv (void);
void timer_handler(void);

/*
 * internal functions
 */

static void do_button_packet(void);

/*
 * AES device interface
 */

static long _cdecl	moose_open		(struct adif *a);
static long _cdecl	moose_ioctl		(struct adif *a, short cmd, long arg);
static long _cdecl	moose_close		(struct adif *a);
static long _cdecl	moose_timeout		(struct adif *a);

static char lname[] = "Moose AES Device interface for XaAES\0";

static struct adif moose_aif = 
{
	0,		/* *next */
	ADIC_MOUSE,	/* class */
	lname,		/* lname */
	"moose",	/* name */
	0,		/* unit */
	0,		/* flags */
	moose_open,	/* open */
	moose_close,	/* close */
	0,		/* resrvd1 */
	moose_ioctl,	/* ioctl */
	moose_timeout,	/* timeout */
	{ NULL },
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

long _cdecl		init		(struct kentry *k, struct adiinfo *ai, char **reason);

/* END definition part */
/****************************************************************************/

/****************************************************************************/
/* BEGIN global data definition & access implementation */

#define MAX_DC_TIME	200
#define MAX_TIMEGAP	20
#define SYSTIMER	0x4baL
#define SYSVBI		0x456L
#define SYSNVBI		0x454L

static long *VBI_entry;

static long lpkt;
static long time_between;
static short eval_timegap;
static short pkt_timegap;

static short moose_inuse;

static short click_x;
static short click_y;
static short click_state;
static short click_cstate;
static short click_count;
static union {
	char chars[16];
	unsigned long ulongs[4];
} clicks;
//static short l_clicks;
//static short r_clicks;

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

#define PAK_BUFFERS	32
#define PAK_BUFFER_SIZE	(PAK_BUFFERS * (sizeof(struct mouse_pak)))

static short pak_buffer[PAK_BUFFER_SIZE+1];

/* END global data & access implementation */
/****************************************************************************/

void
cbutv(void)
{
	long t = *(long *)(SYSTIMER);
	long t1;

	if (eval_timegap)
	{
		t1 = t - lpkt;
		lpkt = t;
		/*
		 * Ozk: Button-release can never follow a button-press too fast.
		 *      But a Button-press CAN follow a button release too fast!
		 * So we check the time between a button-release and button-press
		 * event, and if too short we ingore. This solves problems with
		 * "extra-clicks" when you release button after a click-hold session
		 * caused by unstable mouse-switches (I think .. what else may cause this?)
		 */
		if (sample_butt && (t1 <= (long)pkt_timegap))
			return;
	}
	else
	{
		t1 = -1;			/* for debugging purposes */
		eval_timegap = 2;
		lpkt = t;
	}

	pak_tail->len		= sizeof(struct mouse_pak);
	pak_tail->ty		= BUT_PAK;
	pak_tail->t.but.state	= (unsigned char)sample_butt;
	pak_tail->t.but.time	= (short)t;
	pak_tail->x		= sample_x;
	pak_tail->y		= sample_y;
	pak_tail->dbg		= t1; //0;

	pak_tail++;

	if (pak_tail > pak_end)
		pak_tail = (struct mouse_pak *)&pak_buffer;

	inbuf += sizeof(struct mouse_pak);
}

void
cwhlv(void)
{
	struct moose_data md;

	md.l		= sizeof(md);
	md.ty		= MOOSE_WHEEL_PREFIX;
	md.x = md.sx	= sample_x;
	md.y = md.sy	= sample_y;
	md.state	= (unsigned char)sample_wheel; //pak_head->t.whl.wheel;
	md.cstate	= md.state;
	md.clicks	= sample_wclicks; //pak_head->t.whl.clicks;
	md.kstate	= 0;
	md.dbg1		= 0;
	md.dbg2		= 0;
	(*ainf->wheel)(&moose_aif, &md);
}

void
cmotv(void)
{
	(*ainf->move)(&moose_aif, sample_x, sample_y);
}

static void
chk_but_timeout(short tm)
{
	if (timeout)
	{
		short t = tm;
		t		-= last_time;
		last_time	= tm;
		timeout		-= t;
		if (timeout <= 0)
			do_button_packet();
	}
}

void
timer_handler(void)
{
	if (moose_inuse)
	{
		if (eval_timegap)
			eval_timegap--;

		if (!inbuf)
		{
			chk_but_timeout(*(short*)(SYSTIMER+2));
		}
		else
		{
			if (inbuf)
			{
				if (pak_head->ty == BUT_PAK)
				{	
					register short tm = pak_head->t.but.time;
					register unsigned short s = (unsigned char)pak_head->t.but.state;
					time_between = pak_head->dbg;

					if (s != last_state) /* We skip identical button state events */
					{
						register short i;
						register char *c = clicks.chars;

						if (timeout)
						{
							register unsigned short ls = last_state;
						/* Add clicks to a not-yet-timed-out button packet */
							last_state = s;
							click_state |= s;
							click_cstate = s;
							for (i = 0; i < 16; i++)
							{
								if ((s & 1) && !(ls & 1))
								{
									c[i]++;
									click_count++;
								}
								s  >>= 1;
								ls >>= 1;
							}
							if (click_count > 1)
								timeout += 10;	/* extend timeout, so a triple-click becomes easier */

							chk_but_timeout(tm);
						}
						else
						{
						/* NEW PACKET */
							click_state	= s;
							click_cstate	= s;
							last_state	= s;
							click_x		= pak_head->x;
							click_y		= pak_head->y;

							if (s)
							/* Ozk 180603: Starting a new event... */
							{
								last_time	= tm;
								timeout		= dc_time;

								for (i = 0; i < 16; i++)
								{
									if (s & 1)
									{
										c[i]++;
										click_count++;
									}
								
									s >>= 1;
								}
							}
							else
							/* Ozk 180603: A new event, but with a initial button-released state
							*  makes us send it immediately. This is because the previous event
							*  most likely was sent while button(s) were still pressed (click-hold).
							*/
							{
								do_button_packet();
							}
						}
					}
					else
						chk_but_timeout(tm);
				}
				/* more pak types here ... */
				pak_head++;
				if (pak_head > pak_end)
					pak_head = (struct mouse_pak *)&pak_buffer;

				inbuf -= sizeof(struct mouse_pak);
			}
		}
	}
}

static void
do_button_packet(void)
{
	struct moose_data md;
	register unsigned long *s, *d;

	md.l		= sizeof(struct moose_data);
	md.ty		= MOOSE_BUTTON_PREFIX;
	md.x		= click_x;
	md.y		= click_y;
	md.sx		= sample_x;
	md.sy		= sample_y;
	md.state	= click_state;
	md.cstate	= click_cstate;
	md.clicks	= click_count;
	md.kstate	= 0;		/* Not set here -- will change*/
	s = md.iclicks.ulongs;
	d = clicks.ulongs;
	*s++ = *d, *d++ = 0;
	*s++ = *d, *d++ = 0;
	*s++ = *d, *d++ = 0;
	*s   = *d, *d   = 0;
	md.dbg1	= (short)(time_between >> 16);
	md.dbg2 = (short)time_between;
	(*ainf->button)(&moose_aif, &md);
	click_count	= 0;
	timeout		= 0;
}

static long _cdecl
moose_open (struct adif *a)
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

	moose_inuse	= 0;
	dc_time		= 50;
	pkt_timegap	= 3;
	click_count	= 0;
	for (i = 0; i < 16; i++) clicks.chars[i] = 0;

	timeout		= 0;

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

	return E_OK;
}

static long _cdecl
moose_ioctl (struct adif *a, short cmd, long arg)
{
	switch (cmd)
	{
		case FS_INFO:
		{
			*(long *)arg = (((long)VER_MAJOR << 16) | VER_MINOR);
			break;
		}
		case MOOSE_READVECS:
		{
			struct moose_vecsbuf *vecs = (struct moose_vecsbuf *)arg;
			
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
			if (arg > MAX_DC_TIME)
				dc_time = MAX_DC_TIME;
			else
				dc_time = arg;
			break;
		}
		case MOOSE_PKT_TIMEGAP :
		{
			if (arg > MAX_TIMEGAP)
				pkt_timegap = MAX_TIMEGAP;
			else
				pkt_timegap = arg;

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
moose_close (struct adif *a)
{
	*VBI_entry = NULL;
	moose_inuse = 0;
	dc_time = 0;

	return E_OK;
}

static long _cdecl
moose_timeout(struct adif *a)
{
	return 0;
}

long _cdecl
init (struct kentry *k, struct adiinfo *ainfo, char **reason)
{
	long ret;

	kentry	= k;
	ainf	= ainfo;

	if (check_kentry_version())
		return -1;

	//c_conws (MSG_BOOT);
	//c_conws (MSG_GREET);
	DEBUG (("%s: enter init", __FILE__));

	ret = (*ainf->adi_register)(&moose_aif);
	if (ret)
	{
		DEBUG (("%s: init failed!", __FILE__));
		return 1;
	}
	DEBUG (("%s: init ok", __FILE__));
	return 0;
}
#if 0
static long _cdecl
moose_unregister(struct adif *a)
{
	long ret;
	(void)moose_close(a);
	ret = (*ainf->adi_unregister)(&moose_aif);
	return ret;
}
#endif
