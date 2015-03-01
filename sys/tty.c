/*
 * $Id$
 *
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 *
 * read/write routines for TTY devices
 *
 */

# include "tty.h"

# include "libkern/libkern.h"

# include "mint/filedesc.h"
# include "mint/ioctl.h"
# include "mint/signal.h"

# include "arch/syscall.h"

# include "bios.h"
# include "biosfs.h"
# include "dosfile.h"	/* select_coll */
# include "k_fds.h"
# include "k_prot.h"
# include "keyboard.h"
# include "kmemory.h"
# include "proc.h"
# include "signal.h"
# include "timeout.h"

# include <stddef.h>

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
		T_ECHO|T_CRMOD|T_XKEY|T_ECHOCTL, /* flags */
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
	NULL,			/* use default XKEY map */
	0,			/* not currently hanging up */
	1, 0,			/* RAW reads need 1 char, no timeout */
	0         /* index in escape-sequence */
};

#define _put(f, c) (tty_putchar((f), (c), RAW))

/*
 * Two functions that put/get the character to/from the TTY.
 * But it also checks the return code and aborts with either
 * the number of character written/read or the failed return code.
 */
#define check_tty_putchar(f, c, mode) \
{ \
	if ((r = tty_putchar(f, c, mode)) < E_OK) \
	{ \
		return bytes_written ? bytes_written : r; \
	} \
}

#define check_tty_getchar(f, mode) \
{ \
	if ((r = tty_getchar(f, mode)) < E_OK) \
	{ \
		return bytes_read ? bytes_read : r; \
	} \
}

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
	else if (get_curproc()->domain == DOM_MINT)
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

	DEBUG(("tty_read: pgrp=%d state=%x %ld bytes", tty->pgrp, tty->esc_state, nbytes ));

	if (nbytes == 0)
		return bytes_read;

	/* if RAW or CBREAK do VTIME:  select for input, return on timeout
	 */
	if (tty->vtime
		&& (mode & (T_RAW | T_CBREAK))
		&& !(f->flags & (O_NDELAY | O_HEAD))
		&& (!tty->esc_state || !(rdmode & ESCSEQ)))
	{
		long ret, bytes = 0;

		ret = (*f->dev->select)(f, (long) get_curproc(), O_RDONLY);
		if (ret != 1)
		{
			TIMEOUT *t;

			get_curproc()->wait_cond = (long) wakeselect;	/* flag */
			t = addtimeout (get_curproc(), (long) tty->vtime, (to_func *) wakeselect);
			if (!t)
			{
				(*f->dev->unselect)(f, (long) get_curproc(), O_RDONLY);
				return ENOMEM;
			}

			while (get_curproc()->wait_cond == (long) wakeselect)
			{
				TRACE (("sleeping in tty_read (VTIME)"));
				if (sleep (SELECT_Q | 0x100, (long) wakeselect))
					break;
			}

			canceltimeout (t);
		}

		(void)(*f->dev->ioctl)(f, FIONREAD, &bytes);
		if (!ret)
		{
			(*f->dev->unselect)(f, (long) get_curproc(), O_RDONLY);
			wake (SELECT_Q, (long) &select_coll);
		}

		if (!bytes)
		{
			DEBUG(("tty_read: return %ld *buf=%x", bytes_read, *(char*)buf));
			return bytes_read;
		}
	}
# if 1
	/* see if we can do fast RAW byte IO thru the device driver...
	 */
	if (rdmode == RAW
		&& (!tty->esc_state || !(rdmode & ESCSEQ) || (f->flags & O_HEAD))
		&& (f->fc.fs != &bios_filesys
			|| (nbytes > 1
				&& ((struct bios_file *)f->fc.index)->drvsize >
					offsetof (DEVDRV, readb)))
		&& f->dev->readb
		&& ((f->flags & O_HEAD)
			|| ((tty->state &= ~TS_COOKED), !tty->pgrp)
			|| tty->pgrp == get_curproc()->pgrp
			|| f->fc.dev != get_curproc()->p_fd->control->fc.dev
			|| f->fc.index != get_curproc()->p_fd->control->fc.index)
		&& !(tty->state & TS_BLIND)
		&& (r = (*f->dev->readb)(f, buf, nbytes)) != ENODEV)
	{
		DEBUG(("tty_read(2): return %ld *buf=%x", r, *(char*)buf));
		return r;
	}
# endif

	ptr = buf;

	while (bytes_read < nbytes)
	{
		check_tty_getchar (f, rdmode);
		if (r == MiNTEOF)
		{
			DEBUG(("tty_read(EOF,NDELAY): return %ld *buf=%x nbytes=%ld", bytes_read, *(char*)buf, nbytes));
			return bytes_read;
		}

		ch = r & 0xff;

		if ((rdmode & COOKED) && (mode & T_CRMOD) && (ch == '\r'))
			ch = '\n';

		/* 1 character reads in TOS mode are always raw */
		if (nbytes == 1 && (mode & T_TOS))
		{
			put (f, ch);
			*ptr = ch;
			DEBUG(("tty_read(3): return 1 *buf=%x", *(char*)buf));
			return 1;
		}

		/* T_CBREAK mode doesn't do erase or kill processing */
		/* also note that setting a special character to UNDEF disables it */

		if ((rdmode & COOKED) && !(mode & T_CBREAK) && (ch != UNDEF))
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
			else if ((char)ch == tty->ltc.t_rprntc ||
				 (char)ch == tty->sg.sg_kill)
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
				check_tty_getchar (f, RAW);
				if (r == MiNTEOF)
					return bytes_read;

				if (rdmode & COOKED)
					tty->state |= TS_COOKED;
				else
					tty->state &= ~TS_COOKED;

				ch = r & 0xff;
				goto stuff_it;
			}
			else if ((char)ch == tty->tc.t_eofc && !(mode & T_TOS))
			{
				DEBUG(("tty_read(4): return %ld *buf=%x", bytes_read, *(char*)buf));
				return bytes_read;
			}
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

				DEBUG(("tty_read(5): return %ld *buf=%x ch=%x", bytes_read, *(char*)buf, ch));
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
		if ((mode & (T_RAW|T_CBREAK)) && !tty->esc_state)
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

	DEBUG(("tty_read(6): return %ld *buf=%x", bytes_read, *(char*)buf));
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
		&& (tty->pgrp != get_curproc()->pgrp)
		&& (tty->sg.sg_flags & T_TOSTOP)
		&& (SIGACTION(get_curproc(), SIGTTOU).sa_handler != SIG_IGN)
		&& ((get_curproc()->p_sigmask & (1L << SIGTTOU)) == 0L)
		&& (f->fc.dev == get_curproc()->p_fd->control->fc.dev)
		&& (f->fc.index == get_curproc()->p_fd->control->fc.index))
	{
		TRACE (("job control: tty pgrp is %d proc pgrp is %d", tty->pgrp, get_curproc()->pgrp));
		killgroup (get_curproc()->pgrp, SIGTTOU, 1);
		check_sigs ();
	}
}

long
tty_write (FILEPTR *f, const void *buf, long nbytes)
{
	unsigned const char *ptr;
	long c;
	long r;
	long bytes_written = 0;
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
	else if (get_curproc()->domain == DOM_TOS)
		/* for TOS programs, 1 byte writes are always in raw mode */
		mode = (nbytes == 1) ? T_RAW : T_TOS;
	else
		mode = tty->sg.sg_flags;

	rwmode = (mode & T_RAW) ? RAW : COOKED;

	bytes_written = 0;

	/*
	 * "mode" can now be reduced to just T_CRMODE|T_ECHOCTL or not
	 * if T_ECHO is unset T_ECHOCTL has no effect except:
	 * T_ECHOCTL now is equivalent to T_CRMODE if T_ECHO is not set (avoid stairs).
	 *
	 */
	mode &= (T_CRMOD|((mode & T_ECHO)?0:T_ECHOCTL));

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
			long bytes_to_write;
			unsigned const char *s;
			c = (*f->dev->writeb)(f, buf, 0L);
			if (c == ENODEV)
			{
				goto nofastmode;
			}
			else if (c < E_OK)
			{
				return bytes_written ? bytes_written : c;
			}

			/* i.e. T_CRMODE */
			/* write in big chunks if possible;
			 * lines if CRMODE (if we get here flow
			 * control is taken care of by the device)
			 */

			s = ptr;
			bytes_to_write = 0;
			while (nbytes-- > 0)
			{
				if (*ptr++ == '\n')
				{
					if (0 != (bytes_to_write = ptr - s - 1))
					{
						c = (*f->dev->writeb)(f, (const char *) s, bytes_to_write);
						if (c < E_OK) {
							return bytes_written ? bytes_written : c;
						}
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
					if (c < E_OK) {
						return bytes_written ? bytes_written : c;
					}

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
				if (c < E_OK) {
					return bytes_written ? bytes_written : c;
				}
				bytes_written += c;
				if (c < 0)
					bytes_written = c;
			}

			return bytes_written;
		}
		else
		{
			c = (*f->dev->writeb)(f, buf, nbytes);
			if (c == ENODEV)
			{
				goto nofastmode;
			}
			return c;
		}
	}

nofastmode:
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
		check_tty_putchar (f, cr_char, rwmode);
	}
	check_tty_putchar (f, c, rwmode);
	nbytes--;
	bytes_written++;

	if (use_putchar)
	{
		while (nbytes-- > 0)
		{
			c = *ptr++;
			if (c == '\n' && mode)
			{
				check_tty_putchar (f, cr_char, rwmode);
			}
			check_tty_putchar (f, c, rwmode);
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
			//if(c<' ')DBG_FORCE(("mode=%d,c=%lx", mode,c));
			if (c == '\n')
			{
				if (bytes_to_write)
				{
					(*f->dev->write)(f, (char *) lbuf, bytes_to_write);
					bytes_to_write = 0;
					s = lbuf;
				}
				if (mode)	/* i.e. T_CRMODE */
				{
					check_tty_putchar (f, cr_char, rwmode);
				}
				check_tty_putchar (f, c, rwmode);
				bytes_written++;
			}
			else
			{
				*s++ = c;
				bytes_written++;
				bytes_to_write += 4;
				if (bytes_to_write >= LBUFSIZ * 4)
				{
					c = (*f->dev->write)(f, (char *) lbuf, bytes_to_write);
					if (c < E_OK) {
						return bytes_written ? bytes_written : c;
					}
					bytes_to_write = 0;
					s = lbuf;
				}
			}
		}

		if (bytes_to_write) {
			c = (*f->dev->write)(f, (char *) lbuf, bytes_to_write);
			if (c < E_OK) {
				return bytes_written ? bytes_written : c;
			}
		}
	}

	return bytes_written;
}

/* some notable scan codes */
/* modifiers */
#define KM_SHIFT	3
#define KM_CTRL	4
#define KM_ALT	8

#define K_1	0x02
#define K_0	0x0b
#define K_BACKSPACE	0x0e
#define K_TAB	0x0f
#define K_PLUS	0x1b
#define K_HASH	0x29
#define K_TILDE	0x2b
#define K_COMMA	0x33
#define K_MINUS	0x35
#define K_INSERT	0x52
#define K_DELETE	0x53
#define K_HOME		0x47
#define CTRL_HOME		0x77
#define K_UNDO		0x61
#define K_HELP		0x62
#define CURS_UP		0x48
#define CURS_DN		0x50
#define CURS_RT		0x4d
#define CURS_LF		0x4b
#define CTRL_CURS_RT		0x74
#define CTRL_CURS_LF		0x73
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

#if 0
#define XKEY_LEN 8
static const char vt52xkey[/*256*/] =
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
	'\033', 'e', 0, 0, 0, 0, 0, 0,
};
#else
#define XKEY_LEN 4
#if 0
WARNING:
TIOCSXKEY uses
struct xkey {
	  short xk_num;    /* function key number */
    char  xk_def[8]; /* associated string */
};
#endif
static const char vt52xkey[256] =
{
	/* F-keys */
   /*   0   */  '\033', 'P', 0, 0,
   /*   1   */  '\033', 'Q', 0, 0,
   /*   2   */  '\033', 'R', 0, 0,
   /*   3   */  '\033', 'S', 0, 0,
   /*   4   */  '\033', 'T', 0, 0,
   /*   5   */  '\033', 'U', 0, 0,
   /*   6   */  '\033', 'V', 0, 0,
   /*   7   */  '\033', 'W', 0, 0,
   /*   8   */  '\033', 'X', 0, 0,
   /*   9   */  '\033', 'Y', 0, 0,
	/* Shift-F-keys */
   /*  10   */  '\033', 'p', 0, 0,
   /*  11   */  '\033', 'q', 0, 0,
   /*  12   */  '\033', 'r', 0, 0,
   /*  13   */  '\033', 's', 0, 0,
   /*  14   */  '\033', 't', 0, 0,
   /*  15   */  '\033', 'u', 0, 0,
   /*  16   */  '\033', 'v', 0, 0,
   /*  17   */  '\033', 'w', 0, 0,
   /*  18   */  '\033', 'x', 0, 0,
   /*  19   */  '\033', 'y', 0, 0,
	/* Cursor Up, Down, Right, Left */
   /*  20   */  '\033', 'A', 0, 0,
   /*  21   */  '\033', 'B', 0, 0,
   /*  22   */  '\033', 'C', 0, 0,
   /*  23   */  '\033', 'D', 0, 0,
	/* */
   /*  24   */  '\033', 'H', 0, 0,
   /*  25   */  '\033', 'K', 0, 0,
   /*  26   */  '\033', 'I', 0, 0,
   /*  27   */  '\033', 'E', 0, 0,
   /*  28   */  '\033', 'a', 0, 0,
   /*  29   */  '\033', 'b', 0, 0,
   /*  30   */  '\033', 'c', 0, 0,
   /*  31   */  '\033', 'd', 0, 0,
	/* Ctrl-Cursor-Up/Down/Left/Right*/
   /*  32   */  '\033', 'e', 0, 0,
   /*  33   */  '\033', 'f', 0, 0,
   /*  34   */  '\033', 'g', 0, 0,
   /*  35   */  '\033', 'h', 0, 0,
	/* Ctrl-Ins/Del */
   /*  36   */  '\033', 'i', 0, 0,
   /*  37   */  '\033', 'j', 0, 0,
	/* Ctrl-Help/Undo */
   /*  38   */  '\033', 'k', 0, 0,
   /*  39   */  '\033', 'u', 0, 0,
#define CTRL_VT 40

#define CSC1 ';'
#define CSC2 ':'
#define CSC3 ','

#define CSI_CHAR '['
#define CSI_CHAR1 '('
#define CSI_CHAR2 '{'
#define CSI_CHAR3 '}'

/* to be appended seqs */
# define VT_CTRL_ESC CTRL_VT * XKEY_LEN + 1
   /*       */  0, CSC1, 0, 0,
# define VT_SHIFT_ESC (CTRL_VT+1) * XKEY_LEN + 1
   /*       */  0, CSC2, 0, 0,
# define VT_CTRL_SHIFT_ESC (CTRL_VT+2) * XKEY_LEN + 1
   /*       */  0, CSC3, 0, 0,
};

#define STATE_CTRL 1
#define STATE_SHIFT 2
#define STATE_CTRL_SHIFT 3
#define STATE_ESC_CTRL 4
#define STATE_CSI 8
#define STATE_CSI1 0x10
#define STATE_CSI2 0x20
#define STATE_CSI3 0x40
#define STATE_DIRECT 0x80

# undef T_XTABS_notyet	/* unimplemented */
#define T_XKEY_NP	0x0008      /* sg_falgs: not "numlock" */
#define XKEY_NP	0x0008      /* mode: not "numlock" */

static int is_np(int c)
{
	return ((c >= 0x63 && c <= 0x72) || c == 0x4a || c == 0x4e);
}
#endif
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
	struct tty *tty;

	if (!is_terminal (f))
	{
		DEBUG (("tty_ioctl(mode %x): file is not a tty", mode));
		return ENOSYS;
	}

	tty = (struct tty *)f->devinfo;
	assert (tty);

	switch (mode)
	{
		case FIONREAD:
		{
			long r;

			r = (*f->dev->ioctl)(f, FIONREAD, (void *)arg);
			if (r || (f->flags & O_HEAD))
				return r;

			if (tty->state & TS_BLIND)
				*(long *)arg = 0;

			if ((tty->sg.sg_flags & T_XKEY) && tty->esc_state && !*(long *)arg)
				*(long *)arg = 1;

			return E_OK;
		}
		case FIONWRITE:
		{
			long r;

			r = (*f->dev->ioctl)(f, FIONWRITE, (void *)arg);
			if (r || (f->flags & O_HEAD))
				return r;

			if ((tty->state & (TS_BLIND | TS_HOLD)))
				*(long *)arg = 0;

			return E_OK;
		}
		case TIOCSBRK:
		{
			if (!(tty->state & TS_BLIND) || (f->flags & O_HEAD))
				return (*f->dev->ioctl)(f, TIOCSBRK, (void *)arg);

			return E_OK;
		}
		case TIOCFLUSH:
		{
			long r;

			r = (*f->dev->ioctl)(f, TIOCFLUSH, (void *)arg);
			if (r || (f->flags & O_HEAD))
				return r;

			if (!arg || (*(long *)arg & 1))
				tty->esc_state = 0;

			return E_OK;
		}
		case TIOCGETP:
		{
			struct sgttyb *sg = (struct sgttyb *)arg;
			ulong bits[2] = { -1, TF_FLAGS };
			long baud;
			short flags;

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
			long outq;

			while (((*f->dev->ioctl)(f, TIOCOUTQ, &outq) == 0) && outq)
				nap (200);

			/* FALL THROUGH */
		}
		case TIOCSETN:
		{
			static ushort v[] = {1, 0};
			ulong bits[2];
			ushort oflags = tty->sg.sg_flags;
			long baud;
			short flags;

			struct sgttyb *sg = (struct sgttyb *) arg;
			tty->sg = *sg;

			/* change tty state */
			if (sg->sg_flags & T_RAW)
				tty->state &= ~TS_COOKED;
			else
				tty->state |= TS_COOKED;

			if (!(sg->sg_flags & T_XKEY))
				tty->esc_state = 0;

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
			struct tchars *tc = (struct tchars *)arg;
			*tc = tty->tc;
			return E_OK;
		}
		case TIOCSETC:
		{
			struct tchars *tc = (struct tchars *)arg;
			tty->tc = *tc;
			return E_OK;
		}
		case TIOCGLTC:
		{
			struct ltchars *ltc = (struct ltchars *)arg;
			*ltc = tty->ltc;
			return E_OK;
		}
		case TIOCSLTC:
		{
			struct ltchars *ltc = (struct ltchars *)arg;
			tty->ltc = *ltc;
			return E_OK;
		}
		case TIOCGWINSZ:
		{
			struct winsize *sz = (struct winsize *)arg;
			*sz = tty->wsiz;
			return E_OK;
		}
		case TIOCSWINSZ:
		{
			struct winsize *sz = (struct winsize *)arg;
			int i;

			if (sz->ws_row != tty->wsiz.ws_row
				|| sz->ws_col != tty->wsiz.ws_col
				|| sz->ws_xpixel != tty->wsiz.ws_xpixel
				|| sz->ws_ypixel != tty->wsiz.ws_ypixel)
				i = 1;
			else
				i = 0;

			tty->wsiz = *sz;

			if (i && tty->pgrp)
				killgroup(tty->pgrp, SIGWINCH, 1);

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

			DEBUG (("TIOCSPGRP: assigned tty->pgrp = %i", tty->pgrp));
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
			struct xkey *xk = (struct xkey *)arg;
			const char *xktab;
			int i;

			i = xk->xk_num;
			if (i < 0 || i > 31) return EBADARG;
			xktab = tty->xkey;
			if (!xktab) xktab = vt52xkey;
			xktab += i*XKEY_LEN;
			for (i = 0; i < XKEY_LEN; i++)
				xk->xk_def[i] = *xktab++;

			return E_OK;
		}
		case TIOCSXKEY:
		{
			struct xkey *xk = (struct xkey *)arg;
			char *xktab;
			int i;

			xktab = tty->xkey;
			if (!xktab)
			{
				xktab = kmalloc (sizeof(vt52xkey));	//256);
				if (!xktab)
					return ENOMEM;

				for (i = 0; i < sizeof(vt52xkey); i++)
					xktab[i] = vt52xkey[i];

				tty->xkey = xktab;
			}

			i = xk->xk_num;
			if (i < 0 || i > sizeof(vt52xkey)/XKEY_LEN /*31*/)
				return EBADARG;

			xktab += i*XKEY_LEN;
			for (i = 0; i < XKEY_LEN && xk->xk_def[i]; i++)
				xktab[i] = xk->xk_def[i];
			if (i < XKEY_LEN)
				xktab[i] = 0;
			//xktab[XKEY_LEN-1] = 0;

			return E_OK;
		}
		case TIOCSSTATEB:
		{
			/* change tty->state bits...  really only makes sense
			 * to touch TS_HPCL or maybe TS_COOKED.
			 * (TS_HOLD is already handled by TIOCSTART/STOP)
			 */
			long mask = ((long *)arg)[1] & ~(TS_HOLD|TS_BLIND);

			//if (!(tty->sg.sg_flags & T_XKEY))
				//mask &= ~TS_ESC;

			if (*(long *)arg != -1)
				tty->state = (tty->state & ~mask) | (*((long *)arg) & mask);

			*(long *)arg = tty->state;

			return E_OK;
		}
		case TIOCGSTATE:
		{
			*(long *)arg = tty->state;
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
			ulong bits[2] = { 0, TF_CAR };

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
			long mask = ((ulong *)arg)[1];
			long fnew;
			short flags;

			if (*(long *)arg < 0)
			{
				(*f->dev->ioctl)(f, TIOCGFLAGS, &flags);
				*((ulong *)arg) = flags;

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
			((ushort *)arg)[0] = 1;			/* VMIN */
			((ushort *)arg)[1] = tty->vtime;	/* VTIME */
			return E_OK;
		}
		case TIOCSVMIN:
		{
			tty->vmin = 1;
			tty->vtime = ((ushort *)arg)[1];
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
			ulong bits[2] = { -1, (ushort) -1 };
			long r;

			if (mode == TIOCSFLAGS)
				bits[0] = *(ushort *)arg;

			r = (*f->dev->ioctl)(f, TIOCSFLAGSB, &bits);
			if (!r && mode == TIOCGFLAGS)
				*((ushort *)arg) = *bits;

			return r;
		}

		case TIOCNOTTY:
		{
			/* Disassociate from controlling tty.  */
			if (get_curproc()->p_fd->control == NULL)
				return ENOTTY;

			if (get_curproc()->p_fd->control->fc.index != f->fc.index ||
			    get_curproc()->p_fd->control->fc.dev != f->fc.dev)
		    		return ENOTTY;

		    	/* Session leader.  Disassociate from controlling
		    	 * tty.
		    	 */
			if (get_curproc()->pgrp == get_curproc()->pid)
			{
				if (tty->pgrp > 0)
				{
					killgroup (tty->pgrp, SIGHUP, 0);
					killgroup (tty->pgrp, SIGCONT, 0);
				}

			}

			tty->pgrp = 0;
			DEBUG (("TIOCNOTTY: assigned tty->pgrp = %i", tty->pgrp));

			do_close (get_curproc(), get_curproc()->p_fd->control);
			get_curproc()->p_fd->control = NULL;

			return 0;
		}

		/* Set controlling tty to file descriptor.  The process
		 * must be a session leader and not have a controlling
		 * tty already.
		 */
		case TIOCSCTTY:
		{
			if (get_curproc()->pgrp == get_curproc()->pid &&
			    get_curproc()->pgrp == tty->pgrp)
				return 0;

			if (f->flags & O_HEAD)
				return ENOTTY;

			if (get_curproc()->pgrp != get_curproc()->pid || get_curproc()->p_fd->control)
				return EPERM;

			f->links++;
			tty->use_cnt++;

			/* If the tty is already the controlling tty of
			 * another session than the behavior depends on
			 * arg.
			 */
			if (tty->pgrp > 0)
			{
				struct proc *p;

				for (p = proclist; p; p = p->gl_next)
				{
					if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
						continue;

					if (p->p_fd->control &&
					    p->pgrp == p->pid &&
					    p->p_fd->control->fc.index == f->fc.index &&
					    p->p_fd->control->fc.dev == f->fc.dev)
					{
						    if (!suser (get_curproc()->p_cred->ucr) ||
							(long)arg != 1)
						    {
									DEBUG (("TIOCSCTTY: do_close %lx", f));
							    do_close (get_curproc(), f);
							    return EPERM;
						    }
								DEBUG (("TIOCSCTTY: do_close(control) %lx", p->p_fd->control));
						    do_close (get_curproc(), p->p_fd->control);
						    p->p_fd->control = NULL;
					}
				}
			}

			get_curproc()->p_fd->control = f;
			tty->pgrp = get_curproc()->pgrp;
			DEBUG (("TIOCSCTTY: assigned tty->pgrp = %i", tty->pgrp));

			if (!(f->flags & O_NDELAY) && (tty->state & TS_BLIND))
				(*f->dev->ioctl)(f, TIOCWONLINE, 0);

			return 0;
		}

		/*
		 * transform mdm0 ioctls, to allow old binaries run on new
		 * devices note this does nothing for the other way around
		 * i.e. transform the BSD ones (TIOCCAR/HPCL etc.) for mdm0...
		 */
		case TIOCGHUPCL:
		{
			*(short *)arg = tty->state & TS_HPCL ? 1 : 0;
			return E_OK;
		}
		case TIOCSHUPCL:
		{
			short flags;

			flags = *(short *)arg;
			*(short *)arg = tty->state & TS_HPCL ? 1 : 0;
			if (flags)
				tty->state |= TS_HPCL;
			else
				tty->state &= ~TS_HPCL;

			return E_OK;
		}
		case TIOCGSOFTCAR:
		{
			long bits[2];
			short flags;

			flags = 1;
			bits[0] = -1;
			bits[1] = TF_CAR;

			if ((*f->dev->ioctl)(f, TIOCSFLAGSB, &bits) >= 0)
				flags = bits[0] & TF_CAR ? 0 : 1;

			*(short *)arg = flags;
			return E_OK;
		}
		case TIOCSSOFTCAR:
		{
			long bits[2];
			short flags;

			flags = 1;
			bits[0] = *(short *)arg ? 0 : TF_CAR;
			bits[1] = TF_CAR;

			if ((*f->dev->ioctl)(f, TIOCSFLAGSB, &bits) >= 0)
				flags = bits[0] & TF_CAR ? 0 : 1;

			*(short *)arg = flags;
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
	const char *tab;
	int i;

	switch (scan)
	{
		case CURS_UP:		i = 20; break;
		case CURS_DN:		i = 21; break;
		case CTRL_CURS_RT:
		case CURS_RT:		i = 22; break;
		case CTRL_CURS_LF:
		case CURS_LF:		i = 23; break;
		case K_HELP:		i = 24; break;
		case K_UNDO:		i = 25; break;
		case K_INSERT:
		case K_INSERT + 0x100:		i = 26; break;
		case K_HOME:
		case CTRL_HOME:
		case K_HOME + 0x100:		i = 27; break;
		case CURS_UP + 0x100:	i = 28; break;
		case CURS_DN + 0x100:	i = 29; break;
		case CURS_RT + 0x100:	i = 30; break;
		case CURS_LF + 0x100:	i = 31; break;
		case CURS_UP + 0x200:	i = 32; break;
		case CURS_DN + 0x200:	i = 33; break;
		case K_DELETE + 0x100:	i = 37; break;
		case K_HELP + 0x200:		i = 38; break;
		case K_UNDO + 0x200:		i = 39; break;

		default:
		{
			if (scan >= F_1 && scan <= F_10)
					i = scan - F_1;
			else if (scan >= F_11 && scan <= F_20)	/* Shift-Fkey */
			{
				i = 10 + scan - F_11;
				tty->state &= ~VT_SHIFT_ESC;	/* already shift */
			}
			else
				i = -1;
		}
	}

	if (i >= 0 )
	{
		/* an extended escape sequence */

		tab = tty->xkey;
		if (!tab)
			tab = vt52xkey;

		/* !! */
		i *= XKEY_LEN;

		if( i < sizeof(vt52xkey) )
			scan = tab[i++];
		else
			scan = 0;
		if (scan)
		{
			if (tab[i] == 0) i = 0;
			tty->esc_state = i;
		}
		return scan;
	}

	if (scan >= ALT_1 && scan <= ALT_0)
	{
		scan -= (ALT_1-1);
		if (scan == 10) scan = 0;
		return (scan + '0') | 0x80;
	}

# ifdef NO_AKP_KEYBOARD
	tab = *(((const char **) ROM_Keytbl((void *) -1UL, (void *) -1UL, (void *) -1UL)) + 2 );
# else
	tab = (char *)get_keytab()->caps;
# endif
	scan = tab[scan];

	if (scan >= 'A' && scan <= 'Z')
		return scan | 0x80;

	return 0;
}

long
tty_getchar (FILEPTR *f, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;
	long r, ret;
	int scan;
	uchar c;

	assert (tty);

	/* pty masters never worry about job control and always read in
	 * raw mode
	 */
#if 0
	if (f->dev == &bios_tdevice && f->fc.aux == 2)
	{
		display("tty_getchar: tty->pgrp = %d, gcurproc->pgrp = %d HEAD = %s",
			tty->pgrp, get_curproc()->pgrp, (f->flags & O_HEAD) ? "yes":"no");
	}
#endif
	if (f->flags & O_HEAD)
	{
		ret = (*f->dev->read)(f, (char *)&r, 4L);
		if (ret == ENODEV)
		{
			return MiNTEOF;
		}
		else if (ret < E_OK)
		{
			return ret;
		}
		return (ret != 4L) ? MiNTEOF : r;
	}

	if (tty->sg.sg_flags & T_XKEY_NP)
		mode |= XKEY_NP;	/* ->todo! */
	/* job control check */

	/*
	 * entropy: only do the job control if SIGTTIN is neither blocked nor
	 * ignored, and only for the controlling tty (IEEE 1003.1-1990 7.1.1.4 70-78).
	 * BUG:  if the process group is orphaned or SIGTTIN *is* blocked or ignored,
	 * we should return EIO instead of signalling.
	 */

	if ((tty->pgrp && tty->pgrp != get_curproc()->pgrp)
		&& (f->fc.dev == get_curproc()->p_fd->control->fc.dev)
		&& (f->fc.index == get_curproc()->p_fd->control->fc.index))
	{
		TRACE (("job control: tty pgrp is %d proc pgrp is %d", tty->pgrp, get_curproc()->pgrp));
		killgroup (get_curproc()->pgrp, SIGTTIN, 1);
		check_sigs ();
	}

	if (mode & COOKED)
		tty->state |= TS_COOKED;
	else
		tty->state &= ~TS_COOKED;

	c = UNDEF + 1;	/* set to UNDEF when we successfully read a character */

	/* we may be in the middle of an escape sequence */
	scan = tty->esc_state;
	if( (tty->state & TS_ESC) & STATE_ESC_CTRL )
	{
		/* shift+ctrl+alpha -> ESC ctrl+alpha */
		c = UNDEF;
		r = scan;
		tty->esc_state = 0;
		tty->state &= ~TS_ESC;
	}
	else if( tty->state & STATE_CSI )
	{
		c = UNDEF;
		r = CSI_CHAR;
		tty->state &= ~STATE_CSI;
	}
	else if( tty->state & STATE_CSI1 )
	{
		c = UNDEF;
		r = CSI_CHAR1;
		tty->state &= ~STATE_CSI1;
	}
	else if( tty->state & STATE_CSI2 )
	{
		c = UNDEF;
		r = CSI_CHAR2;
		tty->state &= ~STATE_CSI2;
	}
	else if( tty->state & STATE_CSI3 )
	{
		c = UNDEF;
		r = CSI_CHAR3;
		tty->state &= ~STATE_CSI3;
	}

	else if( tty->state & STATE_DIRECT )
	{
		c = UNDEF;
		r = tty->esc_state;
		tty->esc_state = 0;
		tty->state &= ~STATE_DIRECT;

	}
	else if (scan % XKEY_LEN)
	{
		const char *tab = tty->xkey ? tty->xkey : vt52xkey;
		r = (uchar) tab[scan++];
		if (r)
		{
			c = UNDEF;
			if (tab[scan] == 0)
			{
				switch (tty->state & TS_ESC)
				{
				case STATE_CTRL:
					scan = VT_CTRL_ESC;
				break;
				case STATE_SHIFT:
					scan = VT_SHIFT_ESC;
				break;
				case STATE_CTRL_SHIFT:
					scan = VT_CTRL_SHIFT_ESC;
				break;
				default:
					scan = 0;
				}
				tty->state &= ~TS_ESC;
			}
		}
		else
			scan = 0;

		tty->esc_state = scan;
	}

	while (c != UNDEF)
	{
		ushort ctrl=0;
		int is_numpad;

		if (tty->state & TS_BLIND)
		{
			TRACE (("tty_getchar: offline"));
			return MiNTEOF;
		}
		ret = (*f->dev->read)(f, (char *) &r, 4L);
		if (ret == ENODEV)
		{
			return MiNTEOF;
		}
		else if (ret < E_OK)
		{
			return ret;
		}
		if (ret != 4L)
		{
			DEBUG (("tty_getchar: EOF on tty device (%li) flags:%x,NDELAY:%x", ret, f->flags, f->flags & O_NDELAY));
			return MiNTEOF;
		}

		c = r & 0x00ff;
		scan = (int)((r & 0x00ff0000L) >> 16);
		if( c == 0x80 && scan == 0xff )
			return 0;	/* EOF: ignore? */

		/* extended keycodes
		 * every key+modifier gives a uniqe esc-sequence
		 *
		 * if bit 3 (0x8) in sg_flags is set numpad generates esc-sequeneces
		 *
		 * bugs:
		 * whole code is a mess
		 */

		is_numpad = (mode & XKEY_NP) && is_np(scan);	/* numpad */
		ctrl = (int)((r & 0x0f000000L) >> 24);

		if( ctrl && scan == K_BACKSPACE )	/* exchange ctrl-shift (personel preference) */
		{
			if( ctrl == KM_CTRL )
				ctrl = KM_SHIFT;
			else if( (~ctrl & ~KM_SHIFT) == ~KM_SHIFT )
				ctrl = KM_CTRL;
		}

		tty->state &= ~TS_ESC;
		if (ctrl & KM_CTRL)	//CTRL	-> append CSC2 for some escapes
			tty->state |= STATE_CTRL;
		if (ctrl & KM_SHIFT)	//SHIFT	-> append CSC1 for some escapes
			tty->state |= STATE_SHIFT;
		if (c == 0 && (mode & ESCSEQ) && scan)
		{
			/* translate cursor keys, etc. into escape sequences or
			 * META characters
			 */
			c = UNDEF;
			r = escseq (tty, scan);
			if( is_numpad && (ctrl & KM_ALT) )
			{
				tty->state |= STATE_CSI3;
				if( ctrl & KM_SHIFT )
					scan += 0x11;
			}
			if( (!(r & 0x80) || (scan >= ALT_1 && scan <= ALT_0) ) && (ctrl & KM_ALT) )	/* ALT ,.-#+ï~  (?) */
			{
				tty->state |= STATE_DIRECT;
				tty->state |= STATE_CSI2;	/* -> "esc{c" */
				if( (scan >= F_1 && scan <= F_10) || (scan >= F_11 && scan <= F_20) )	/* alt(+shift)+fkey */
				{
					if( scan < F_11 )
						scan = 'a' + scan - F_1;
					else
						scan = 'A' + scan - F_11;
				}
				else if( scan >= ALT_1 && scan <= ALT_0 )	/* alt+digit */
				{
					if( ctrl & KM_SHIFT )
						scan = '!' + scan - ALT_1;
					else
						scan = '1' + scan - ALT_1;
				}
				else	/* not fkey not digit: ,.-#+ */
				{
					if( scan >= K_COMMA )
						scan += ',' - K_COMMA;
					else
						tty->state |= STATE_CSI1;
					if( (ctrl & KM_SHIFT) )
					{

						if( scan >= ',' )
							scan += 4 - ' ';	/* map to something different .. */
						else
						{
							if( scan == K_BACKSPACE )
								tty->state |= STATE_CSI;
							scan += '1' - '+';	/* map to something different .. */
						}

					}
				}
				if( scan <= 0x1b )	/* no esc in esc-seq */
				{
					if( ctrl & KM_SHIFT )
						scan += 'X' - 0x1b;
					else
						scan += 'x' - 0x1b;
				}
				tty->esc_state = scan;
				r = 0x1b;
			}
			else if( r & 0x80 )	/* alt+letter: todo: esq-seq if alt pressed (->int. chars) sg_flags ...? */
			{
				if( (tty->state & STATE_CTRL_SHIFT) == STATE_SHIFT )
					r -= 'a' - 'A';
			}
			else if( /*r == 0x1b &&*/ (tty->state & STATE_CTRL_SHIFT) )
			{
				/* ctrl-cursor etc. */
				if( r != 0x1b )
				{
					r = 0x1b;
					tty->esc_state = scan;
				}
				tty->state |= STATE_CSI;
			}
		}
		else if ((mode & ESCSEQ) && ((scan == CURS_UP && (c == '8' || c == 0x18))
			|| (scan == CURS_DN && (c == '2' || c == 0x12))
			|| (scan == CURS_RT && c == '6')
			|| (scan == CURS_LF && c == '4')
			|| scan == K_HOME
			|| (scan == K_INSERT)
			|| (scan == K_DELETE && (c == 0x1f || ((ctrl & KM_SHIFT) && c == 0x7f)))
			))
		{
			r = escseq (tty, scan+0x100);
			if( scan == CURS_UP || scan == CURS_DN || scan == CURS_RT || scan == CURS_LF )
				tty->state &= ~STATE_SHIFT;	/* already shift */

			if( tty->state & STATE_CTRL_SHIFT )
				tty->state |= STATE_CSI;

			c = UNDEF;
		}
		/* ctrl+digits, tab, numpad and ,.-#+ try to map to something useful */
		else if ( (mode & ESCSEQ) &&
			( ((ctrl & KM_CTRL) &&
				((scan >= K_1 && scan <= K_BACKSPACE)
			|| (scan >= K_COMMA && scan <= K_MINUS)
			|| scan == K_TAB
			|| (scan == K_HASH && (ctrl & (KM_CTRL|KM_SHIFT)) == KM_CTRL)
			|| (scan == K_TILDE || scan == K_PLUS /*|| scan == 0x0d*/)	))
			|| is_numpad
			) )
		{
			if( is_numpad )
			{
				if( scan >= 0x63 )
					c = scan - 0x63 + '(';
				else
				{
					c = scan - 0x4e + '`';
					if( ctrl & KM_SHIFT )
					{
						c += ' ';
						tty->state |= STATE_CSI;
					}
				}
				if( ctrl & KM_CTRL )
					tty->state |= STATE_CSI3;
			}
			else
			{
				if( scan <= K_BACKSPACE )	/* digits */
				{
					c = scan - 2 + '1';
					if( scan > K_0 )
						tty->state |= STATE_CSI3;
				}
				else if( scan >= K_COMMA )
				{
					c = scan - K_COMMA + 'k';
				}
				else
				{
					c = scan;
				}

				/* map 0-key to '0' */
				if( c == 0x3a )
					c = '0';
				else if( c < ' ' )
					c += ' ';
			}
			if( ctrl & KM_SHIFT )	/* shift */
			{
				if( is_numpad )
				{
					tty->state |= STATE_CSI1;
					c += ' ';
				}
				else if( c >= 'k' || c <= 0x3a )
					c -= 0x10;
				else
					c -= ('c' - ';');
			}
			if( isalpha(c) || scan == K_HASH || is_numpad )	/* "esc[letter" -> "esc[{letter" */
			{
				tty->state |= STATE_CSI2;
				if( is_numpad && (ctrl & KM_CTRL) )
				{
					if( ctrl & KM_SHIFT )
					{
						c += 'n' -0x51;
						c = tolower(c);
					}
					else
					{
						c -= 0x11;
						if( c < ' ' && scan >= 0x65 )
							c += ('a' - ' ');
					}
				}
				else if( scan == K_HASH )
					c += ' ';
			}
			else
				tty->state |= STATE_CSI;

			/* avoid non-printable characters */
			if( c > 'z' )
			{
				c -= 'z';
				tty->state |= STATE_CSI1;
			}
			if( c < ' ' )
			{
				c += 'A';
				tty->state &= ~STATE_CSI1;
				tty->state |= STATE_CSI3;
			}
			else if( !(tty->state & STATE_CSI1) )
			{
				tty->state |= STATE_CSI;
			}
			tty->state |= STATE_DIRECT;
			tty->esc_state = c;

			r = 0x1b;
			c = UNDEF;
		}

		else if (mode & COOKED)
		{
			if (c == UNDEF || c == 0xff)
				;	/* do nothing */
			else if (c == tty->ltc.t_dsuspc)
			{
				DEBUG (("tty_getchar: killgroup (%i, SIGTSTP, 1)", get_curproc()->pgrp));
				killgroup (get_curproc()->pgrp, SIGTSTP, 1);
				check_sigs ();
			}
			else if (c == tty->tc.t_intrc)
			{
				DEBUG (("tty_getchar: killgroup (%i, SIGINT, 1)", get_curproc()->pgrp));
				killgroup (get_curproc()->pgrp, SIGINT, 1);
				check_sigs ();
			}
			else if (c == tty->tc.t_quitc)
			{
				DEBUG (("tty_getchar: killgroup (%i, SIGQUIT, 1)", get_curproc()->pgrp));
				killgroup (get_curproc()->pgrp, SIGQUIT, 1);
				check_sigs ();
			}
			else if (c == tty->tc.t_stopc)
				tty_ioctl(f, TIOCSTOP, 0);
			else if (c == tty->tc.t_startc)
				tty_ioctl(f, TIOCSTART, 0);
			else
			{
				c = UNDEF;
			}
		}
		else
			c = UNDEF;

		if( c == UNDEF && ((r & 0xff) && (r & 0xff) < 0x1b) )
		{
			tty->state &= ~(STATE_CTRL|STATE_CSI);
			if( tty->state & STATE_SHIFT )		/* shift+ctrl+alpha -> ESC^alpha */
			{
				tty->state = STATE_ESC_CTRL;
				tty->esc_state = r & 0xff;
				r = 0x1b;
			}
		}
	}

	if (mode & ECHO) {
		if ((ret = tty_putchar(f, r, mode)) < E_OK)
		{
			return ret;
		}
	}
	return r;
}

/*
 * tty_putchar: returns number of bytes successfully written
 */

long
tty_putchar (FILEPTR *f, long data, int mode)
{
	struct tty *tty = (struct tty *) f->devinfo;
	char ch;

	assert (tty);
#if 0
	if (f->dev == &bios_tdevice && f->fc.aux == 2)
	{
		display("tty_putchar: tty->pgrp = %d, curproc->pgrp = %d HEAD = %s",
			tty->pgrp, get_curproc()->pgrp, (f->flags & O_HEAD) ? "yes":"no");
	}
#endif
#if 0
	if( data == 0x0a )	/* nl -> crnl */
	{
		tty_putchar( f, 0x0d, mode );
	}
#endif
	/* pty masters never worry about job control
	 */
	if (f->flags & O_HEAD)
	{
		ch = data & 0xff;

		if ((tty->state & TS_COOKED) && ch != UNDEF && ch != -1 )
		{
			long r = 1;

			/* see if we're putting control characters into the
			 * buffer
			 */
			if (ch == tty->tc.t_intrc)
			{
				DEBUG (("tty_putchar: killgroup (%i, SIGINT, 1)", tty->pgrp));
				killgroup (tty->pgrp, SIGINT, 1);
				if (!(tty->sg.sg_flags & T_NOFLSH))
					tty_ioctl (f, TIOCFLUSH, &r);
				tty_ioctl (f, TIOCSTART, 0);
				return 4L;
			}
			else if (ch == tty->tc.t_quitc)
			{
				DEBUG (("tty_putchar: killgroup (%i, SIGQUIT, 1)", tty->pgrp));
				killgroup (tty->pgrp, SIGQUIT, 1);
				if (!(tty->sg.sg_flags & T_NOFLSH))
					tty_ioctl (f, TIOCFLUSH, &r);
				tty_ioctl (f, TIOCSTART, 0);
				return 4L;
			}
			else if (ch == tty->ltc.t_suspc)
			{
				DEBUG (("tty_putchar: killgroup (%i, SIGTSTP, 1)", tty->pgrp));
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
				long ret = 2;
				tty_ioctl (f, TIOCFLUSH, &ret);
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
					DEBUG (("tty_putchar1: killgroup (%i, SIGTSTP, 1)", tty->pgrp));
					killgroup(tty->pgrp, SIGTSTP, 1);
				} else if (c == tty->tc.t_intrc) {
					tty_ioctl(f, TIOCSTART, 0);
					DEBUG (("tty_putchar1: killgroup (%i, SIGINT, 1)", tty->pgrp));
					killgroup(tty->pgrp, SIGINT, 1);
				} else if (c == tty->tc.t_quitc) {
					tty_ioctl(f, TIOCSTART, 0);
					DEBUG (("tty_putchar1: killgroup (%i, SIGQUIT, 1)", tty->pgrp));
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

		if ((tty->sg.sg_flags & T_XKEY) && tty->esc_state)
			return 1;
	}

	return (*f->dev->select)(f, proc, mode);
}

