/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/*
 * read/write routines for TTY devices
 */

# include "tty.h"

# include "mint/signal.h"

# include "bios.h"
# include "biosfs.h"
# include "dosfile.h"
# include "kmemory.h"
# include "proc.h"
# include "signal.h"
# include "timeout.h"

# include <osbind.h>
# include <stddef.h>


/* setting a special character to this value disables it */
#define UNDEF 0

#define HAS_WRITEB(f)	(((f)->fc.fs != &bios_filesys || \
			 (((struct bios_file *)(f)->fc.index)->drvsize > \
				offsetof (DEVDRV, writeb))) && \
			 (f)->dev->writeb)

/* default terminal characteristics */

struct tty default_tty =
{
	0,			/* process group */
	0,			/* state */
	0,			/* use_cnt */
	0,			/* aux_cnt */
	{
	13, 13,			/* input speed == output speed == 9600 baud */
	CTRL('H'),		/* erase */
	CTRL('U'),		/* kill */
	T_ECHO|T_CRMOD|T_TOSTOP|T_XKEY|T_ECHOCTL, /* flags */
	},
	{
	CTRL('C'),		/* interrupt */
	CTRL('\\'),		/* quit */
	CTRL('Q'),		/* start */
	CTRL('S'),		/* stop */
	CTRL('D'),		/* EOF */
	'\r'			/* alternate end of line */
	},
	{
	CTRL('Z'),		/* suspend */
	CTRL('Y'),		/* suspend after read */
	CTRL('R'),		/* reprint */
	CTRL('O'),		/* flush output */
	CTRL('W'),		/* erase word */
	CTRL('V')		/* quote next char */
	},
	{
	0, 0, 0, 0		/* window size is unknown */
	},
	0,			/* no process is selecting us for reading */
	0,			/* or for writing */
	0,			/* use default XKEY map */
	0,			/* not currently hanging up */
	1, 0			/* RAW reads need 1 char, no timeout */
};

#define _put(f, c) (tty_putchar((f), (c), RAW))

static void
_erase (FILEPTR *f, int c, int mode)
{
	_put(f, '\010');
	_put(f, ' ');
	_put(f, '\010');
/* watch out for control characters -- they're printed as e.g. "^C" */
/* BUG: \t is messed up. We really need to keep track of the output
 * column
 */
	if ((mode & T_ECHOCTL) && c >= 0 && c < ' ' && c != '\t') {
		_put(f, '\010'); _put(f, ' '); _put(f, '\010');
	}
}

#define put(f, c)   { if (mode & T_ECHO) _put(f, c); }
#define erase(f, c) { if (mode & T_ECHO) _erase(f, c, mode); }

long
tty_read(FILEPTR *f, void *buf, long nbytes)
{
	long r;
	long bytes_read = 0;
	unsigned char ch, *ptr;
	int rdmode, mode;
	struct tty *tty;
	
	tty = (struct tty *) f->devinfo;
	assert (tty != 0);
	
	if (f->flags & O_HEAD)
	{
		/* pty server side - always raw mode */
		rdmode = RAW;
		mode = T_RAW;
	}
	else if (curproc->domain == DOM_MINT)
	{
		/* MiNT domain process */
		mode = tty->sg.sg_flags;
		
		if (mode & (T_RAW | T_CBREAK))
			rdmode = (mode & T_RAW) ? RAW : COOKED;
		else
			rdmode = COOKED | NOECHO;
		
		if (mode & T_XKEY)
			rdmode |= ESCSEQ;
	}
	else
	{
		rdmode = COOKED | NOECHO;
		mode = T_TOS | T_ECHO | T_ECHOCTL;
	}
	
	if (nbytes == 0)
		return bytes_read;
	
	/* if RAW or CBREAK do VTIME:  select for input, return on timeout
	 */
	if (tty->vtime
		&& (mode & (T_RAW | T_CBREAK))
		&& !(f->flags & (O_NDELAY | O_HEAD))
		&& (!(tty->state & TS_ESC) || !(rdmode & ESCSEQ)))
	{
		long r, bytes = 0;
		
		r = (*f->dev->select)(f, (long) curproc, O_RDONLY);
		if (r != 1)
		{
			TIMEOUT *t;
			
			curproc->wait_cond = (long) wakeselect;	/* flag */
			t = addtimeout (curproc, (long) tty->vtime, (to_func *) wakeselect);
			if (!t)
			{
				(*f->dev->unselect)(f, (long) curproc, O_RDONLY);
				return ENOMEM;
			}
			
			while (curproc->wait_cond == (long) wakeselect)
			{
				TRACE (("sleeping in tty_read (VTIME)"));
				if (sleep (SELECT_Q | 0x100, (long) wakeselect))
					break;
			}
			
			canceltimeout (t);
		}
		
		(void)(*f->dev->ioctl)(f, FIONREAD, &bytes);
		if (!r)
		{
			(*f->dev->unselect)(f, (long) curproc, O_RDONLY);
			wake (SELECT_Q, (long) &select_coll);
		}
		
		if (!bytes)
			return bytes_read;
	}
# if 1
	/* see if we can do fast RAW byte IO thru the device driver...
	 */
	if (rdmode == RAW
		&& (!(tty->state & TS_ESC) || !(rdmode & ESCSEQ) || (f->flags & O_HEAD))
		&& (f->fc.fs != &bios_filesys
			|| (nbytes > 1
				&& ((struct bios_file *)f->fc.index)->drvsize >
					offsetof (DEVDRV, readb)))
		&& f->dev->readb
		&& ((f->flags & O_HEAD)
			|| ((tty->state &= ~TS_COOKED), !tty->pgrp)
			|| tty->pgrp == curproc->pgrp
			|| f->fc.dev != curproc->control->fc.dev
			|| f->fc.index != curproc->control->fc.index)
		&& !(tty->state & TS_BLIND)
		&& (r = (*f->dev->readb)(f, buf, nbytes)) != ENODEV)
	{
		return r;
	}
# endif
	
	ptr = buf;
	
	while (bytes_read < nbytes)
	{
		r = tty_getchar (f, rdmode);
		if (r < E_OK)
		{
tty_error:
			DEBUG (("tty_read: tty_getchar returned %ld", r));
			return (bytes_read) ? bytes_read : r;
		}
		else if (r == MiNTEOF)
			return bytes_read;
		
		ch = r & 0xff;
		
		if ((rdmode & COOKED) && (mode & T_CRMOD) && (ch == '\r'))
			ch = '\n';
		
		/* 1 character reads in TOS mode are always raw */
		if (nbytes == 1 && (mode & T_TOS))
		{
			put (f, ch);
			*ptr = ch;
			return 1;
		}
		
		/* T_CBREAK mode doesn't do erase or kill processing */
		/* also note that setting a special character to UNDEF disables it */
		
		if (rdmode & COOKED && !(mode & T_CBREAK) && ch != UNDEF)
		{
			if ((char) ch == tty->sg.sg_erase)
			{
				/* backspace */
				if (bytes_read > 0)
				{
					--ptr;
					erase (f, *ptr);
					bytes_read--;
				}
				continue;
			}
			else if ((mode & T_TOS) && ch == CTRL ('X'))
			{
				while (bytes_read > 0)
				{
					--ptr;
					erase (f, *ptr);
					bytes_read--;
				}
				continue;
			}
			else if ((char) ch ==tty->ltc.t_rprntc || 
				 (char) ch == tty->sg.sg_kill)
			{
				if (mode & T_TOS)
					put (f, '#');
				put (f, '\r');
				put (f, '\n');
				ptr = buf;
				if ((char) ch == tty->sg.sg_kill)
				{
					bytes_read = 0;
				}
				else
				{
					for (r = 0; r < bytes_read; r++, ptr++)
						put (f, *ptr);
				}
				continue;
			}
			else if ((char) ch == tty->ltc.t_werasc)
			{
				while (bytes_read > 0 &&
				       (ptr[-1] == ' ' || ptr[-1] == '\t'))
				{
					ptr--;
					erase (f, *ptr);
					bytes_read--;
				}
				while (bytes_read > 0 &&
				       !(ptr[-1] == ' ' || ptr[-1] == '\t'))
				{
					ptr--;
					erase (f, *ptr);
					bytes_read--;
				}
				continue;
			}
			else if ((char) ch == tty->ltc.t_lnextc)
			{
				if (mode & T_ECHOCTL)
				{
					put(f, '^');
					put(f, '\b');
				}
				r = tty_getchar (f, RAW);
				if (rdmode & COOKED)
					tty->state |= TS_COOKED;
				else
					tty->state &= ~TS_COOKED;
				
				if (r < E_OK)
					goto tty_error;
				else if (r == MiNTEOF)
					return bytes_read;
				
				ch = r & 0xff;
				goto stuff_it;
			}
			else if ((char)ch == tty->tc.t_eofc && !(mode & T_TOS))
				return bytes_read;
		}
		
		/* both T_CBREAK and T_COOKED modes have to do signals, though
		 */
		if ((rdmode & COOKED) && ch != UNDEF)
		{
			if ((char) ch == tty->tc.t_intrc
				|| (char)ch == tty->tc.t_quitc
				|| (char)ch == tty->ltc.t_dsuspc
				|| (char)ch == tty->ltc.t_suspc)
			{
				/* the device driver raised the appropriate
				 * signal; if we get here, the signal was
				 * caught by the user (or ignored). flush
				 * buffers and continue
				 */
				if (!(tty->sg.sg_flags & T_NOFLSH))
				{
					DEBUG (("tty_read: flushing input"));
					bytes_read = 0;
					ptr = buf;
				}
				continue;
			}
			else if ((char) ch == tty->ltc.t_flushc)
			{
				continue;
			}
			else if (ch == '\n' || (char)ch == tty->tc.t_brkc)
			{
				put (f, '\r');
				if (!(mode & T_TOS))
				{
					*ptr = ch;
					put (f, '\n');
					bytes_read++;
				}
				if (rdmode & COOKED)
					tty->state |= TS_COOKED;
				else
					tty->state &= ~TS_COOKED;
				
				return bytes_read;
			}

		}
		
		/* do the following for both RAW and COOKED mode
		 */
stuff_it:
		*ptr++ = ch;
		if ((mode & T_ECHOCTL) && (ch < ' ') && (ch != '\t'))
		{
			/* ch is unsigned */
			put (f, '^');
			put (f, ch+'@');
		}
		else
			put (f, ch);
		
		bytes_read++;
		
		/* for RAW mode, if there are no more characters then break */
		if ((mode & (T_RAW|T_CBREAK)) && !(tty->state & TS_ESC))
		{
			r = 1;
			(void)(*f->dev->ioctl)(f, FIONREAD, &r);
			if (r <= 0)
				break;
		}
	}
	
	if (rdmode & COOKED)
		tty->state |= TS_COOKED;
	else
		tty->state &= ~TS_COOKED;
	
	return bytes_read;
}

/* job control checks
 * AKP: added T_TOSTOP; don't stop BG output if T_TOSTOP is clear
 * 
 * entropy: only do the job control if SIGTTOU is neither blocked nor ignored,
 * and only for the controlling tty (IEEE 1003.1-1990 7.1.1.4 79-87).
 * 
 * BUG: if the process group is orphaned and SIGTTOU *is not* blocked
 *      or ignored, we should return EIO instead of signalling.
 */
void
tty_checkttou (FILEPTR *f, struct tty *tty)
{
	if (tty->pgrp
		&& (tty->pgrp != curproc->pgrp)
		&& (tty->sg.sg_flags & T_TOSTOP)
		&& (curproc->sighandle[SIGTTOU] != SIG_IGN)
		&& ((curproc->sigmask & (1L << SIGTTOU)) == 0L)
		&& (f->fc.dev == curproc->control->fc.dev)
		&& (f->fc.index == curproc->control->fc.index))
	{
		TRACE (("job control: tty pgrp is %d proc pgrp is %d", tty->pgrp, curproc->pgrp));
		killgroup (curproc->pgrp, SIGTTOU, 1);
		check_sigs ();
	}
}

long
tty_write (FILEPTR *f, const void *buf, long nbytes)
{
	unsigned const char *ptr;
	long c;
	long bytes_written;
	int mode, rwmode;
	struct tty *tty;
	int use_putchar = 0;
	static long cr_char = '\r';
# define LBUFSIZ 128
	long lbuf[LBUFSIZ];
	
	tty = (struct tty *) f->devinfo;
	assert (tty != 0);
	
	ptr = (unsigned const char *) buf;
	if (f->flags & O_HEAD)
	{
		use_putchar = 1;
		mode = T_RAW;
	}
	else if (curproc->domain == DOM_TOS)
		/* for TOS programs, 1 byte writes are always in raw mode */
		mode = (nbytes == 1) ? T_RAW : T_TOS;
	else
		mode = tty->sg.sg_flags;
	
	rwmode = (mode & T_RAW) ? RAW : COOKED;
	
	bytes_written = 0;
	
	/*
	 * "mode" can now be reduced to just T_CRMODE or not
	 */
	if ((curproc->domain == DOM_MINT) && (mode & T_CRMOD) && !(mode & T_RAW))
		mode = T_CRMOD;
	else
		mode = 0;
	
	if (nbytes == 0)
		return bytes_written;
# if 1
	/* see if we can do fast RAW byte IO thru the device driver... */
	if (!use_putchar && HAS_WRITEB(f))
	{
		tty_checkttou (f, tty);
		if (rwmode & COOKED)
			tty->state |= TS_COOKED;
		else
			tty->state &= ~TS_COOKED;
		if (tty->state & TS_BLIND)
			return bytes_written;
		if (mode)
		{
			/* i.e. T_CRMODE */
			if ((*f->dev->writeb)(f, buf, 0L) != ENODEV)
			{
				/* write in big chunks if possible;
				 * lines if CRMODE (if we get here flow
				 * control is taken care of by the device)
				 */
				long bytes_to_write = 0;
				unsigned const char *s = ptr;

				while (nbytes-- > 0)
				{
					if (*ptr++ == '\n')
					{
						if (0 != (bytes_to_write = ptr - s - 1))
						{
							c = (*f->dev->writeb)(f, (const char *) s, bytes_to_write);
							bytes_written += c;
							if (c != bytes_to_write)
							{
								if (c < 0)
									bytes_written = c;
								
								return bytes_written;
							}
						}
						s = ptr-1;
						c = (*f->dev->writeb)(f, "\r", 1);
						if (c != 1)
						{
							if (c < 0)
								bytes_written = c;
							
							return bytes_written;
						}
					}
				}
				
				if (0 != (bytes_to_write = ptr - s))
				{
					c = (*f->dev->writeb)(f, (const char *)s, bytes_to_write);
					bytes_written += c;
					if (c < 0)
						bytes_written = c;
				}
				
				return bytes_written;
			}
		}
		else
		{
			c = (*f->dev->writeb)(f, buf, nbytes);
			if (c != ENODEV)
				return c;
		}
	}
# endif
	
	/*
	 * we always write at least 1 byte with tty_putchar, since that takes
	 * care of job control and terminal states. After that, we may be able
	 * to use (*f->dev->write) directly.
	 */
	
	c = *ptr++;
	
	if (c == '\n' && mode)
	{
		/* remember, "mode" now means CRMOD */
		tty_putchar (f, cr_char, rwmode);
	}
	
	tty_putchar (f, c, rwmode);
	nbytes--;
	bytes_written++;
	
	if (use_putchar)
	{
		while (nbytes-- > 0)
		{
			c = *ptr++;
			if (c == '\n' && mode)
				tty_putchar (f, cr_char, rwmode);
			tty_putchar (f, c, rwmode);
			bytes_written++;
		}
	}
	else
	{
		/* write in big chunks if possible; but never more than 1 line
		 * (so that ^S/^Q can happen reasonably quickly for the user)
		 */
		long bytes_to_write = 0;
		long *s = lbuf;
		
		while (nbytes-- > 0)
		{
			c = *ptr++;
			if (c == '\n')
			{
				if (bytes_to_write)
				{
					(*f->dev->write)(f, (char *) lbuf, bytes_to_write);
					bytes_to_write = 0;
					s = lbuf;
				}
				if (mode)	/* i.e. T_CRMODE */
					tty_putchar (f, cr_char, rwmode);
				tty_putchar (f, (long) c, rwmode);
				bytes_written++;
			}
			else
			{
				*s++ = c;
				bytes_written++;
				bytes_to_write += 4;
				if (bytes_to_write >= LBUFSIZ * 4)
				{
					(*f->dev->write)(f, (char *) lbuf, bytes_to_write);
					bytes_to_write = 0;
					s = lbuf;
				}
			}
		}
		
		if (bytes_to_write)
			(*f->dev->write)(f, (char *) lbuf, bytes_to_write);
	}
	
	return bytes_written;
}

/* some notable scan codes */
#define K_INSERT	0x52
#define K_HOME		0x47
#define K_UNDO		0x61
#define K_HELP		0x62
#define CURS_UP		0x48
#define CURS_DN		0x50
#define CURS_RT		0x4d
#define CURS_LF		0x4b
#define F_1		0x3b
#define F_10		0x44
#define F_11		0x54
#define F_20		0x5d
#define ALT_1		0x78
#define ALT_0		0x81

/* Default function key table:
 * entries:	0-9 are F1-F10
 *        	10-19 are F11-F20
 *		20-23 are cursor up, down, right, and left
 *		24-27 are help, undo, insert, and home
 *		28-31 are shift+cursor up, down, right, and left
 */

static char vt52xkey[256] =
{
	'\033', 'P', 0, 0, 0, 0, 0, 0,
	'\033', 'Q', 0, 0, 0, 0, 0, 0,
	'\033', 'R', 0, 0, 0, 0, 0, 0,
	'\033', 'S', 0, 0, 0, 0, 0, 0,
	'\033', 'T', 0, 0, 0, 0, 0, 0,
	'\033', 'U', 0, 0, 0, 0, 0, 0,
	'\033', 'V', 0, 0, 0, 0, 0, 0,
	'\033', 'W', 0, 0, 0, 0, 0, 0,
	'\033', 'X', 0, 0, 0, 0, 0, 0,
	'\033', 'Y', 0, 0, 0, 0, 0, 0,
	'\033', 'p', 0, 0, 0, 0, 0, 0,
	'\033', 'q', 0, 0, 0, 0, 0, 0,
	'\033', 'r', 0, 0, 0, 0, 0, 0,
	'\033', 's', 0, 0, 0, 0, 0, 0,
	'\033', 't', 0, 0, 0, 0, 0, 0,
	'\033', 'u', 0, 0, 0, 0, 0, 0,
	'\033', 'v', 0, 0, 0, 0, 0, 0,
	'\033', 'w', 0, 0, 0, 0, 0, 0,
	'\033', 'x', 0, 0, 0, 0, 0, 0,
	'\033', 'y', 0, 0, 0, 0, 0, 0,
	'\033', 'A', 0, 0, 0, 0, 0, 0,
	'\033', 'B', 0, 0, 0, 0, 0, 0,
	'\033', 'C', 0, 0, 0, 0, 0, 0,
	'\033', 'D', 0, 0, 0, 0, 0, 0,
	'\033', 'H', 0, 0, 0, 0, 0, 0,
	'\033', 'K', 0, 0, 0, 0, 0, 0,
	'\033', 'I', 0, 0, 0, 0, 0, 0,
	'\033', 'E', 0, 0, 0, 0, 0, 0,
	'\033', 'a', 0, 0, 0, 0, 0, 0,
	'\033', 'b', 0, 0, 0, 0, 0, 0,
	'\033', 'c', 0, 0, 0, 0, 0, 0,
	'\033', 'd', 0, 0, 0, 0, 0, 0,
};

/* convert a number describing the baud rate into a Unix
 * style baud rate number. Returns the Unix baud rate,
 * or 21 (EXTA) if the rate is unknown
 */

# define EXTA 21

static ulong ubaud [EXTA] =
{
	     0L,     50L,     75L,    110L,    134L,
	   150L,    200L,    300L,    600L,   1200L,
	  1800L,   2400L,   4800L,   9600L,  19200L,
	 38400L,  57600L, 115200L, 230400L, 460800L,
	921600L
};

static char
unxbaud (ulong baud)
{
	int i;
	
	for (i = 1; i < EXTA; i++)
	{
		if (ubaud [i] == baud)
			break;
	}
	
	return i;
}

# define tosbaud(c) (((c) < 0 || (c) >= EXTA) ? -1L : ubaud [(ushort) c])

long
tty_ioctl (FILEPTR *f, int mode, void *arg)
{
	struct sgttyb *sg;
	struct tchars *tc;
	struct ltchars *ltc;
	struct tty *tty;
	struct winsize *sz;
	struct xkey *xk;
	char *xktab;
	int i;
	long baud;
	short flags;
	long outq;
	
	if (!is_terminal (f))
	{
		DEBUG (("tty_ioctl(mode %x): file is not a tty", mode));
		return ENOSYS;
	}
	
	tty = (struct tty *) f->devinfo;
	assert (tty != 0);
	
	switch (mode)
	{
		case FIONREAD:
		{
			long r;
			
			r = (*f->dev->ioctl)(f, FIONREAD, (void *) arg);
			if (r || (f->flags & O_HEAD))
				return r;
			
			if (tty->state & TS_BLIND)
				*(long *) arg = 0;
			
			if ((tty->sg.sg_flags & T_XKEY) && (tty->state & TS_ESC) && !*(long *) arg)
				*(long *) arg = 1;
			
			return E_OK;
		}
		case FIONWRITE:
		{
			long r;
			
			r = (*f->dev->ioctl)(f, FIONWRITE, (void *) arg);
			if (r || (f->flags & O_HEAD))
				return r;
			
			if ((tty->state & (TS_BLIND | TS_HOLD)))
				*(long *) arg = 0;
			
			return E_OK;
		}
		case TIOCSBRK:
		{
			if (!(tty->state & TS_BLIND) || (f->flags & O_HEAD))
				return (*f->dev->ioctl)(f, TIOCSBRK, (void *) arg);
			
			return E_OK;
		}
		case TIOCFLUSH:
		{
			long r;
			
			r = (*f->dev->ioctl)(f, TIOCFLUSH, (void *) arg);
			if (r || (f->flags & O_HEAD))
				return r;
			
			if (!arg || (*(long *) arg & 1))
				tty->state &= ~TS_ESC;
			
			return E_OK;
		}
		case TIOCGETP:
		{
			ulong bits[2] = { -1, TF_FLAGS };
			sg = (struct sgttyb *) arg;
			
			/* get input and output baud rates from the terminal
			 * device
			 */
			
			baud = -1L;
			(*f->dev->ioctl)(f, TIOCIBAUD, &baud);
			tty->sg.sg_ispeed = unxbaud (baud);
			
			baud = -1L;
			(*f->dev->ioctl)(f, TIOCOBAUD, &baud);
			tty->sg.sg_ospeed = unxbaud (baud);
			
			/* get terminal flags */
			flags = 0;
			if ((*f->dev->ioctl)(f, TIOCSFLAGSB, &bits) == 0)
			{
				tty->sg.sg_flags &= ~TF_FLAGS;
				tty->sg.sg_flags |= (*bits & TF_FLAGS);
			}
			else if ((*f->dev->ioctl)(f, TIOCGFLAGS, &flags) == 0)
			{
				tty->sg.sg_flags &= ~TF_FLAGS;
				tty->sg.sg_flags |= (flags & TF_FLAGS);
			}
			
			*sg = tty->sg;
			return E_OK;
		}
		case TIOCSETP:
		{
			while (((*f->dev->ioctl)(f, TIOCOUTQ, &outq) == 0) && outq)
				nap (200);
			
			/* FALL THROUGH */
		}
		case TIOCSETN:
		{
			ulong bits[2];
			static ushort v[] = {1, 0};
			ushort oflags = tty->sg.sg_flags;
			
			sg = (struct sgttyb *) arg;
			tty->sg = *sg;
			
			/* change tty state */
			if (sg->sg_flags & T_RAW)
				tty->state &= ~TS_COOKED;
			else
				tty->state |= TS_COOKED;
			
			if (!(sg->sg_flags & T_XKEY))
				tty->state &= ~TS_ESC;
			
			/* set baud rates */
			baud = tosbaud (sg->sg_ispeed);
			(*f->dev->ioctl)(f, TIOCIBAUD, &baud);
			baud = tosbaud (sg->sg_ospeed);
			(*f->dev->ioctl)(f, TIOCOBAUD, &baud);
			
			/* reset VMIN/VTIME */
			if ((*f->dev->ioctl)(f, TIOCSVMIN, &v) < 0)
			{
				tty->vmin = 1;
				tty->vtime = 0;
			}
			
			/* set parity, etc. */
			flags = TF_8BIT;
			if (sg->sg_flags & (T_EVENP | T_ODDP))
				flags = TF_7BIT;
			
			flags |= (sg->sg_flags & TF_FLAGS);
			
			/* default allow breaks to SIGINT unless RAW and no echo... */
			if (!(sg->sg_flags & T_RAW) || (sg->sg_flags & T_ECHO))
				flags |= TF_BRKINT;
			
			/* leave local mode bit alone */
			bits[0] = (unsigned) flags;
			bits[1] = ~TF_CAR;
			if ((*f->dev->ioctl)(f, TIOCSFLAGSB, &bits) >= 0)
				return 0;
			
			/* if TIOCSFLAGSB failed clear TF_CAR, assume the
			 * device doesn't know about carrier anyway...
			 */
			if ((*f->dev->ioctl)(f, TIOCSFLAGS, &flags) >= 0)
				return E_OK;
			
			/* cannot set flags, don't save them */
			tty->sg.sg_flags = (tty->sg.sg_flags & ~TF_FLAGS) | (oflags & TF_FLAGS);
			
			return E_OK;
		}
		case TIOCGETC:
		{
			tc = (struct tchars *) arg;
			*tc = tty->tc;
			return E_OK;
		}
		case TIOCSETC:
		{
			tc = (struct tchars *) arg;
			tty->tc = *tc;
			return E_OK;
		}
		case TIOCGLTC:
		{
			ltc = (struct ltchars *) arg;
			*ltc = tty->ltc;
			return E_OK;
		}
		case TIOCSLTC:
		{
			ltc = (struct ltchars *) arg;
			tty->ltc = *ltc;
			return E_OK;
		}
		case TIOCGWINSZ:
		{
			sz = (struct winsize *) arg;
			*sz = tty->wsiz;
			return E_OK;
		}
		case TIOCSWINSZ:
		{
			sz = (struct winsize *) arg;
			
			if (sz->ws_row != tty->wsiz.ws_row
				|| sz->ws_col != tty->wsiz.ws_col
				|| sz->ws_xpixel != tty->wsiz.ws_xpixel
				|| sz->ws_ypixel != tty->wsiz.ws_ypixel)
				i = 1;
			else
				i = 0;
			
			tty->wsiz = *sz;
			
			if (i && tty->pgrp)
				killgroup (tty->pgrp, SIGWINCH, 1);
			
			return E_OK;
		}
		case TIOCGPGRP:
		{
			*((long *)arg) = tty->pgrp;
			return E_OK;
		}
		case TIOCSPGRP:
		{
			if (!tty->pgrp)
			{
				tty->pgrp = (*((long *) arg) & 0x00007fffL);
				
				if (!(f->flags & O_NDELAY) && (tty->state & TS_BLIND))
					(*f->dev->ioctl)(f, TIOCWONLINE, 0);
			}
			else
				tty->pgrp = (*((long *) arg) & 0x00007fffL);
			
			return E_OK;
		}
		case TIOCSTART:
		{
			/* tty in the middle of a hangup? */
			if (tty->hup_ospeed)
				return E_OK;
			
			/* if the device has writeb writers may sleep for
			 * TS_HOLD (instead of polling), tell the device and
			 * wake them up
			 */
			if (HAS_WRITEB (f))
			{
				(void)(*f->dev->ioctl)(f, TIOCSTART, &tty->state);
				wake (IO_Q, (long) &tty->state);
			}
			tty->state &= ~TS_HOLD;
			if (tty->wsel)
			{
				long r = 0;
				
				(void)(*f->dev->ioctl)(f, FIONWRITE, &r);
				if (r && !(tty->state & TS_BLIND))
					wakeselect ((PROC *)tty->wsel);
			}
			
			return E_OK;
		}
		case TIOCSTOP:
		{
			if (HAS_WRITEB (f))
				(void)(*f->dev->ioctl)(f, TIOCSTOP, &tty->state);
			
			tty->state |= TS_HOLD;
			
			return E_OK;
		}
		case TIOCGXKEY:
		{
			xk = (struct xkey *)arg;
			
			i = xk->xk_num;
			if (i < 0 || i > 31) return EBADARG;
			xktab = tty->xkey;
			if (!xktab) xktab = vt52xkey;
			xktab += i*8;
			for (i = 0; i < 8; i++)
				xk->xk_def[i] = *xktab++;
			
			return E_OK;
		}
		case TIOCSXKEY:
		{
			xk = (struct xkey *) arg;
			
			xktab = tty->xkey;
			if (!xktab)
			{
				xktab = kmalloc (256);
				if (!xktab)
					return ENOMEM;
				
				for (i = 0; i < 256; i++)
					xktab[i] = vt52xkey[i];
				
				tty->xkey = xktab;
			}
			
			i = xk->xk_num;
			if (i < 0 || i > 31)
				return EBADARG;
			
			xktab += i*8;
			for (i = 0; i < 7; i++)
				xktab[i] = xk->xk_def[i];
			xktab[7] = 0;
			
			return E_OK;
		}
		case TIOCSSTATEB:
		{
			/* change tty->state bits...  really only makes sense
			 * to touch TS_HPCL or maybe TS_COOKED.
			 * (TS_HOLD is already handled by TIOCSTART/STOP)
			 */
			long mask = ((long *) arg)[1] & ~(TS_HOLD|TS_BLIND);
			
			if (!(tty->sg.sg_flags & T_XKEY))
				mask &= ~TS_ESC;
			
			if (*(long *) arg != -1)
				tty->state = (tty->state & ~mask) | (*((long *)arg) & mask);
			
			*(long *) arg = tty->state;
			
			return E_OK;
		}
		case TIOCGSTATE:
		{
			*(long *) arg = tty->state;
			return E_OK;
		}
		case TIOCHPCL:
		{
			/* hang up on close, handled by kernel if the device
			 * understands TIOCOBAUD
			 */
			tty->state |= TS_HPCL;
			return E_OK;
		}
		/* set/reset local mode */
		case TIOCNCAR:
		case TIOCCAR:
		{
			ulong bits[2] = {0, TF_CAR};
			
			if (mode == TIOCCAR)
				*bits = TF_CAR;
			
			(*f->dev->ioctl)(f, TIOCSFLAGSB, &bits);
			
			/* if the ioctl failed the device does not know about
			 * carrier but don't return an error then since its
			 * the same as carrier always on (and anyway who puts
			 * a dialup line on a port that doesn't know how to
			 * SIGHUP or hang up safely... :)
			 */
			return E_OK;
		}
		
		/* emulate some new calls, they only get here when a device
		 * does not know them:
		 */
		case TIOCSFLAGSB:
		{
			long fnew;
			long mask = ((ulong *) arg)[1];
			
			if (*(long *) arg < 0)
			{
				(*f->dev->ioctl)(f, TIOCGFLAGS, &flags);
				*((ulong *) arg) = flags;
				
				return E_OK;
			}
			
			flags = 0;
			if (mask != -1)
				(*f->dev->ioctl)(f, TIOCGFLAGS, &flags);
			
			fnew = (flags & ~mask) | (*((ulong *)arg) & mask);
			if (mask == -1 || fnew != flags)
			{
				flags = fnew;
				(*f->dev->ioctl)(f, TIOCSFLAGS, &flags);
				(*f->dev->ioctl)(f, TIOCGFLAGS, &flags);
			}
			
			*(ulong *) arg = flags;
			return E_OK;
		}
		
		/*
		 * tty_read handles VTIME itself but doing VMIN > 1 without support
		 * from the device won't be very efficient
		 */
		case TIOCGVMIN:
		{
			((ushort *) arg)[0] = 1;		/* VMIN */
			((ushort *) arg)[1] = tty->vtime;	/* VTIME */
			return E_OK;
		}
		case TIOCSVMIN:
		{
			tty->vmin = 1;
			tty->vtime = ((ushort *) arg)[1];
			return E_OK;
		}
		
		case TIOCWONLINE:
		{
			/* devices that don't know about carrier are always
			 * online...
			 */
			return E_OK;
		}
		
		/*
		 * if the device didn't do TIOC[GS]FLAGS try transforming into
		 * TIOCSFLAGSB
		 */
		case TIOCGFLAGS:
		case TIOCSFLAGS:
		{
			ulong bits[2] = {-1, (ushort) -1};
			long r;
			
			if (mode == TIOCSFLAGS)
				bits[0] = *(ushort *) arg;
			
			r = (*f->dev->ioctl)(f, TIOCSFLAGSB, &bits);
			if (!r && mode == TIOCGFLAGS)
				*((ushort *) arg) = *bits;
			
			return r;
		}
		
		case TIOCNOTTY:
			/* Disassociate from controlling tty.  */
			if (curproc->control == NULL)
				return ENOTTY;
				
			if (curproc->control->fc.index != f->fc.index ||
			    curproc->control->fc.dev != f->fc.dev)
		    		return ENOTTY;
		    	
		    	/* Session leader.  Disassociate from controlling
		    	 * tty.
		    	 */
			if (curproc->pgrp == curproc->pid) {
				if (tty->pgrp > 0) {
					killgroup (tty->pgrp, SIGHUP, 0);
					killgroup (tty->pgrp, SIGCONT, 0);
				}
				
			}
			tty->pgrp = 0;
			do_close(curproc->control);
			curproc->control = NULL;
			return 0;
			
		/* Set controlling tty to file descriptor.  The process
		 * must be a session leader and not have a controlling
		 * tty already.  
		 */
		case TIOCSCTTY:
			if (curproc->pgrp == curproc->pid &&
			    curproc->pgrp == tty->pgrp)
				return 0;
			
			if (f->flags & O_HEAD)
				return ENOTTY;
				
			if (curproc->pgrp != curproc->pid || curproc->control)
				return EPERM;

			f->links++;
			tty->use_cnt++;
			
			/* If the tty is already the controlling tty of
			 * another session than the behavior depends on
			 * arg.  
			 */
			if (tty->pgrp > 0) {
				PROC* p;
				for (p = proclist; p; p = p->gl_next) {
					if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
						continue;
					if (p->control &&
					    p->pgrp == p->pid &&
					    p->control->fc.index == f->fc.index &&
					    p->control->fc.dev == f->fc.dev) {
						    if (curproc->euid != 0 ||
							(long) arg != 1)
						    {
							    do_close (f);
							    return EPERM;
						    }
						    do_close (p->control);
						    p->control = NULL;
					}
				}
			}
			
			curproc->control = f;
			tty->pgrp = curproc->pgrp;
			if (!(f->flags & O_NDELAY) && (tty->state & TS_BLIND))
				(*f->dev->ioctl)(f, TIOCWONLINE, 0);
				
			return 0;
			
		/*
		 * transform mdm0 ioctls, to allow old binaries run on new
		 * devices note this does nothing for the other way around
		 * i.e. transform the BSD ones (TIOCCAR/HPCL etc.) for mdm0...
		 */
		case TIOCGHUPCL:
		{
			*(short *) arg = tty->state & TS_HPCL ? 1 : 0;
			return E_OK;
		}
		case TIOCSHUPCL:
		{
			flags = *(short *) arg;
			*(short *) arg = tty->state & TS_HPCL ? 1 : 0;
			if (flags)
				tty->state |= TS_HPCL;
			else
				tty->state &= ~TS_HPCL;
			
			return E_OK;
		}
		case TIOCGSOFTCAR:
		{
			long bits[2];
			
			flags = 1;
			bits[0] = -1;
			bits[1] = TF_CAR;
			
			if ((*f->dev->ioctl)(f, TIOCSFLAGSB, &bits) >= 0)
				flags = bits[0] & TF_CAR ? 0 : 1;
			
			*(short *) arg = flags;
			return E_OK;
		}
		case TIOCSSOFTCAR:
		{
			long bits[2];
			
			flags = 1;
			bits[0] = *(short *) arg ? 0 : TF_CAR;
			bits[1] = TF_CAR;
			
			if ((*f->dev->ioctl)(f, TIOCSFLAGSB, &bits) >= 0)
				flags = bits[0] & TF_CAR ? 0 : 1;
			
			*(short *) arg = flags;
			return E_OK;
		}
		
		default:
		{
			DEBUG (("tty_ioctl: bad function call"));
			return ENOSYS;
		}
	}
	
	/* not reached */
	return ENOSYS;
}

/*
 * function for translating extended characters (e.g. cursor keys, or
 * ALT+key sequences) into either escape sequences or meta characters.
 * for escape sequences, we return the the first character of the
 * sequence (normally ESC) and set the tty's state so that subsequent
 * calls to tty_getchar will pick up the remaining characters.
 * Note that escape sequences are limited to 7 characters at most.
 */

static int
escseq (struct tty *tty, int scan)
{
	char *tab;
	int i;

	switch (scan)
	{
		case CURS_UP:		i = 20; break;
		case CURS_DN:		i = 21; break;
		case CURS_RT:		i = 22; break;
		case CURS_LF:		i = 23; break;
		case K_HELP:		i = 24; break;
		case K_UNDO:		i = 25; break;
		case K_INSERT:		i = 26; break;
		case K_HOME:		i = 27; break;
		case CURS_UP + 0x100:	i = 28; break;
		case CURS_DN + 0x100:	i = 29; break;
		case CURS_RT + 0x100:	i = 30; break;
		case CURS_LF + 0x100:	i = 31; break;
		default:
			if (scan >= F_1 && scan <= F_10)
				i = scan - F_1;
			else if (scan >= F_11 && scan <= F_20)
				i = 10 + scan - F_11;
			else
				i = -1;
	}
	
	if (i >= 0)
	{
		/* an extended escape sequence */
		
		tab = tty->xkey;
		if (!tab)
			tab = vt52xkey;
		
		i *= 8;
		
		scan = tab[i++];
		if (scan)
		{
			if (tab[i] == 0) i = 0;
			tty->state = (tty->state & ~TS_ESC) | i;
		}
		
		return scan;
	}
	
	if (scan >= ALT_1 && scan <= ALT_0)
	{
		scan -= (ALT_1-1);
		if (scan == 10) scan = 0;
		return (scan + '0') | 0x80;
	}
	
	tab = *(((char **) Keytbl((void *) -1UL, (void *) -1UL, (void *) -1UL)) + 2 );
	scan = tab[scan];
	if (scan >= 'A' && scan <= 'Z')
		return scan | 0x80;
	
	return 0;
}

long
tty_getchar (FILEPTR *f, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;
	char c, *tab;
	long r, ret;
	int scan;
	int master = f->flags & O_HEAD;
	
	assert (tty);
	
	/* pty masters never worry about job control and always read in
	 * raw mode
	 */
	if (master)
	{
		ret = (*f->dev->read)(f, (char *)&r, 4L);
		return (ret != 4L) ? MiNTEOF : r;
	}
	
	/* job control check */
	
	/*
	 * entropy: only do the job control if SIGTTIN is neither blocked nor
	 * ignored, and only for the controlling tty (IEEE 1003.1-1990 7.1.1.4 70-78).
	 * BUG:  if the process group is orphaned or SIGTTIN *is* blocked or ignored,
	 * we should return EIO instead of signalling.
	 */
	
	if ((tty->pgrp && tty->pgrp != curproc->pgrp)
		&& (f->fc.dev == curproc->control->fc.dev)
		&& (f->fc.index == curproc->control->fc.index))
	{
		TRACE (("job control: tty pgrp is %d proc pgrp is %d", tty->pgrp, curproc->pgrp));
		killgroup (curproc->pgrp, SIGTTIN, 1);
		check_sigs ();
	}
	
	if (mode & COOKED)
		tty->state |= TS_COOKED;
	else
		tty->state &= ~TS_COOKED;
	
	c = UNDEF+1;	/* set to UNDEF when we successfully read a character */
	
	/* we may be in the middle of an escape sequence */
	scan = (tty->state & TS_ESC);
	if (scan != 0)
	{
		tab = tty->xkey ? tty->xkey : vt52xkey;
		r = (uchar) tab[scan++];
		if (r)
		{
			c = UNDEF;
			if (tab[scan] == 0) scan = 0;
		}
		else
			scan = 0;
		
		tty->state = (tty->state & ~TS_ESC) | scan;
	}
	
	while (c != UNDEF)
	{
		if (tty->state & TS_BLIND)
		{
			TRACE (("tty_getchar: offline"));
			return MiNTEOF;
		}
		
		ret = (*f->dev->read)(f, (char *) &r, 4L);
		if (ret != 4L)
		{
			DEBUG (("EOF on tty device"));
			return MiNTEOF;
		}
		
		c = r & 0x00ff;
		scan = (int)((r & 0x00ff0000L) >> 16);
		if ((c == 0) && (mode & ESCSEQ) && scan)
		{
			c = UNDEF;
			/* translate cursor keys, etc. into escape sequences or
			 * META characters
			 */
			r = escseq (tty, scan);
		}
		else if ((mode & ESCSEQ) && ((scan == CURS_UP && c == '8')
			|| (scan == CURS_DN && c == '2')
			|| (scan == CURS_RT && c == '6')
			|| (scan == CURS_LF && c == '4')))
		{
			c = UNDEF;
			r = escseq (tty, scan+0x100);
		}
		else if (mode & COOKED)
		{
			if (c == UNDEF)
				;	/* do nothing */
			else if (c == tty->ltc.t_dsuspc)
			{
				killgroup (curproc->pgrp, SIGTSTP, 1);
				check_sigs ();
			}
			else if (c == tty->tc.t_intrc)
			{
				killgroup (curproc->pgrp, SIGINT, 1);
				check_sigs ();
			}
			else if (c == tty->tc.t_quitc)
			{
				killgroup (curproc->pgrp, SIGQUIT, 1);
				check_sigs ();
			}
			else if (c == tty->tc.t_stopc)
				tty_ioctl(f, TIOCSTOP, 0);
			else if (c == tty->tc.t_startc)
				tty_ioctl(f, TIOCSTART, 0);
			else
				c = UNDEF;
		}
		else
				c = UNDEF;
	}
	
	if (mode & ECHO)
		tty_putchar (f, r, mode);
	
	return r;
}

/*
 * tty_putchar: returns number of bytes successfully written
 */

long
tty_putchar (FILEPTR *f, long data, int mode)
{
	struct tty *tty;
	int master;		/* file is pty master side */
	char ch;
	
	tty = (struct tty *) f->devinfo;
	
	master = f->flags & O_HEAD;

	/* pty masters don't need to worry about job control */
	if (master)
	{
		ch = data & 0xff;
		
		if ((tty->state & TS_COOKED) && ch != UNDEF)
		{
			long r = 1;
			
			/* see if we're putting control characters into the
			 * buffer
			 */
			if (ch == tty->tc.t_intrc)
			{
				killgroup (tty->pgrp, SIGINT, 1);
				if (!(tty->sg.sg_flags & T_NOFLSH))
					tty_ioctl (f, TIOCFLUSH, &r);
				tty_ioctl (f, TIOCSTART, 0);
				return 4L;
			}
			else if (ch == tty->tc.t_quitc)
			{
				killgroup (tty->pgrp, SIGQUIT, 1);
				if (!(tty->sg.sg_flags & T_NOFLSH))
					tty_ioctl (f, TIOCFLUSH, &r);
				tty_ioctl (f, TIOCSTART, 0);
				return 4L;
			}
			else if (ch == tty->ltc.t_suspc)
			{
				killgroup (tty->pgrp, SIGTSTP, 1);
				if (!(tty->sg.sg_flags & T_NOFLSH))
					tty_ioctl (f, TIOCFLUSH, &r);
				tty_ioctl (f, TIOCSTART, 0);
				return 4L;
			}
			else if (ch == tty->tc.t_stopc)
			{
				tty_ioctl (f, TIOCSTOP, 0);
				return 4L;
			}
			else if (ch == tty->tc.t_startc)
			{
				tty_ioctl (f, TIOCSTART, 0);
				return 4L;
			}
			else if (ch == tty->ltc.t_flushc)
			{
				long r = 2;
				tty_ioctl (f, TIOCFLUSH, &r);
				return 4L;
			}
			else if (tty->state & TS_HOLD)
			{
				return 0;
			}
		}
		goto do_putchar;
	}
	
	tty_checkttou (f, tty);
	
	if (mode & COOKED)
	{
		tty->state |= TS_COOKED;
		while (tty->state & (TS_HOLD|TS_BLIND))
		{
			short bdev;
			
			if (tty->state & TS_BLIND)
			{
				TRACE (("tty_putchar: offline"));
				return 0;
			}
#if 1
	/* hack: BIOS devices != console never reset TS_HOLD themselves
	   unless another process happens to call tty_getchar on them while
	   we're here.  someone has a better fix? :-(  -nox
	*/
/* BIOS device definitions */
#define CONSDEV 2
			if (f->dev == &bios_tdevice &&
			    (bdev=f->fc.aux) != CONSDEV) {
				long c;

				if (!bconstat(bdev) ||
				    (c = bconin(bdev) & 0x7fffffffL) == UNDEF)
					;	/* do nothing */
				else if (c == tty->ltc.t_suspc) {
					tty_ioctl(f, TIOCSTART, 0);
					killgroup(tty->pgrp, SIGTSTP, 1);
				} else if (c == tty->tc.t_intrc) {
					tty_ioctl(f, TIOCSTART, 0);
					killgroup(tty->pgrp, SIGINT, 1);
				} else if (c == tty->tc.t_quitc) {
					tty_ioctl(f, TIOCSTART, 0);
					killgroup(tty->pgrp, SIGQUIT, 1);
				} else if (c == tty->tc.t_startc)
					tty_ioctl(f, TIOCSTART, 0);
				else if (c == tty->ltc.t_flushc) {
					long r = 2;
					tty_ioctl(f, TIOCFLUSH, &r);
				}
			}
			else
#endif
			if (HAS_WRITEB (f))
			{
				/* if the device has writeb assume it wakes
				 * us when TS_HOLD resets
				 */
				sleep (IO_Q, (long) &tty->state);
				continue;
			}
			
			/* sleep for 60 milliseconds */
			nap (60);
		}
	}
	else
		tty->state &= ~TS_COOKED;
	
do_putchar:
	return (*f->dev->write)(f, (char *)&data, 4L);
}

/*
 * special select() function that takes T_XKEY into account
 */
long
tty_select(FILEPTR *f, long proc, int mode)
{
	if (mode == O_RDONLY)
	{
		struct tty *tty = (struct tty *) f->devinfo;
		
		if ((tty->sg.sg_flags & T_XKEY) && (tty->state & TS_ESC))
			return 1;
	}
	
	return (*f->dev->select)(f, proc, mode);
}

