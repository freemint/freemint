/*
 * VT52 emulator for a GEM text window (with type TEXTWIN)
 */

#include <osbind.h>

#include "vt.h"
#include "ansicol.h"
#include "console.h"

#ifdef DEBUG
extern int do_debug;
# include <syslog.h>
#endif

/*
 * lokale prototypen
 */
static void escy_putch (TEXTWIN *v, unsigned int c);

/* capture(v, c): put character c into the capture buffer
 * if c is '\r', then we're finished and we call the callback
 * function
 */
static void capture(TEXTWIN *v, unsigned int c)
{
	int 	i = v->captsiz;

	if (c == '\r')
		c = 0;

	if (i < CAPTURESIZE || c == 0)
	{
		v->captbuf[i++] = c;
		v->captsiz = i;
	}
	if (c == 0)
	{
		v->output = vt52_putch;
		(*v->callback)(v);
	}
}

/* Legacy functions for color support.	*/
static
void fgcol_putch (TEXTWIN *v, unsigned int c)
{
	v->curr_cattr = (v->curr_cattr & ~CFGCOL) |
			 ((c & 0xff) << 4);
	v->output = vt52_putch;
}

static
void bgcol_putch (TEXTWIN *v, unsigned int c)
{
	v->curr_cattr = (v->curr_cattr & ~CBGCOL) |
			 (c & 0xff);
	v->output = vt52_putch;
}

/* ANSI color functions.  */
static
void ansi_fgcol_putch (TEXTWIN *v, unsigned int c)
{
	set_ansi_fg_color (v, c);
	v->output = vt52_putch;
}

static void
ansi_bgcol_putch (TEXTWIN *v, unsigned int c)
{
	set_ansi_bg_color (v, c);
	v->output = vt52_putch;
}

/* set special effects */
static void seffect_putch(TEXTWIN *v, unsigned int c)
{
	v->curr_cattr |= ((c & 0x1f) << 8);
	if (!v->vdi_colors)
		v->curr_cattr &= ~CE_ANSI_EFFECTS;
	v->output = vt52_putch;
}

/* clear special effects */
static void ceffect_putch(TEXTWIN *v, unsigned int c)
{
	if (c == '_' && !v->vdi_colors)
		v->curr_cattr &= ~CE_ANSI_EFFECTS;

	v->curr_cattr &= ~((c & 0x1f) << 8);
	v->output = vt52_putch;
}

/*
 * putesc(v, c): handle the control sequence ESC c
 */
static void putesc(TEXTWIN *v, unsigned int c)
{
	int 		cx, cy;

#ifdef DEBUG
	if (do_debug) {
		char* printable;
		char ctrl[3] = { '^', 'X', '\0' };
		char letter[2] = { 'X', '\0' };

		if (c < ' ') {
			ctrl[1] = c + 32;
			printable = ctrl;
		} else {
			letter[0] = c;
			printable = letter;
		}
		syslog (LOG_ERR, "got escape character %d (%s, 0x%08x)",
			c, printable, (unsigned) c);
	}
#endif

	cx = v->cx;
	cy = v->cy;

	switch (c)
	{
		case '3':		/* GFL: Set ANSI fg color.  */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set ANSI fg");
#endif
			v->output = ansi_fgcol_putch;
			return;
		case '4':		/* GFL: Set ANSI fg color.  */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set ANSI bg");
#endif
			v->output = ansi_bgcol_putch;
			return;
		case 'A':		/* cursor up */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor up");
#endif
			cuu1 (v);
			break;
		case 'B':		/* cursor down */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor down");
#endif
			cud1 (v);
			break;
		case 'C':		/* cursor right */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor right");
#endif
			cuf1 (v);
			break;
		case 'D':		/* cursor left */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor left");
#endif
			cub1 (v);
			break;
		case 'F':		/* smacs, start alternate character set.  */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is smacs");
#endif
			v->curr_tflags = 
				(v->curr_tflags & ~TCHARSET_MASK) | 1;
			break;

		case 'G':		/* rmacs, end alternate character set.  */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is rmacs");
#endif
			v->curr_tflags = v->curr_tflags & ~TCHARSET_MASK;
			break;

		case 'E':		/* clear home */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear");
#endif
			clear(v);
			/* fall through... */
		case 'H':		/* cursor home */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is home");
#endif
			gotoxy (v, 0, 0);
			break;
		case 'I':		/* cursor up, insert line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor up, insert line");
#endif
			reverse_cr (v);
			break;
		case 'J':		/* clear below cursor */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear below cursor");
#endif
			clrfrom(v, cx, cy, v->maxx-1, v->maxy-1);
			break;
		case 'K':		/* clear remainder of line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear rest of line");
#endif
			clrfrom(v, cx, cy, v->maxx-1, cy);
			break;
		case 'L':		/* insert a line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is insert line");
#endif
			insert_line (v);
			break;
		case 'M':		/* delete line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is delete line");
#endif
			delete_line (v);
			break;
		case 'R':		/* TW extension: set window size */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set window size");
#endif
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_size;
			return;
		case 'S':		/* MW extension: set title bar */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set title bar");
#endif
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_title;
			return;
		case 'Y':
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is goto xy");
#endif
			v->output = escy_putch;
			return;
		case 'a':		/* MW extension: delete character */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is delete character");
#endif
			delete_char(v, cx, cy);
			break;
		case 'b':		/* set foreground color */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set foreground color");
#endif
			v->output = fgcol_putch;
			return;		/* `return' to avoid resetting v->output */
		case 'c':		/* set background color */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set background color");
#endif
			v->output = bgcol_putch;
			return;
		case 'd':		/* clear to cursor position */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear to cursor position");
#endif
			clrfrom(v, 0, v->miny, cx, cy);
			break;
		case 'e':		/* enable cursor */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor on");
#endif
			v->curr_tflags |= TCURS_ON;
			break;
		case 'f':		/* cursor off */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor off");
#endif
			v->curr_tflags &= ~TCURS_ON;
			break;
		case 'h':		/* MW extension: enter insert mode */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is enter insert mode");
#endif
			v->curr_tflags |= TINSERT;
			break;
		case 'i':		/* MW extension: leave insert mode */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is leave insert mode");
#endif
			v->curr_tflags &= ~TINSERT;
			break;
		case 'j':		/* save cursor position */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is save cursor position");
#endif
			v->saved_x = v->cx;
			v->saved_y = v->cy;
			break;
		case 'k':		/* restore saved position */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is restore cursor position");
#endif
			/* FIXME: origin mode!!! */
			gotoxy (v, v->saved_x, v->saved_y);
			break;
		case 'l':		/* clear line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear line");
#endif
			clrline (v, cy);
			gotoxy (v, 0, cy - RELOFFSET (v));
			break;
		case 'o':		/* clear from start of line to cursor */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear from start of line to cursor");
#endif
			clrfrom(v, 0, cy, cx, cy);
			break;
		case 'p':		/* reverse video on */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is reverse video on");
#endif
			v->curr_cattr |= CINVERSE;
			break;
		case 'q':		/* reverse video off */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is reverse video off");
#endif
			v->curr_cattr &= ~CINVERSE;
			break;
		case 't':		/* backward compatibility for TW 1.x: set cursor timer */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "set cursor timer");
#endif
			return;
		case 'u':		/* original colors */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is original colors");
#endif
			original_colors (v);
			break;
		case 'v':		/* wrap on */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is linewrap on");
#endif
			v->curr_tflags |= TWRAPAROUND;
			break;
		case 'w':
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is linewrap off");
#endif
			v->curr_tflags &= ~TWRAPAROUND;
			break;
		case 'y':		/* TW extension: set special effects */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set text effects");
#endif
			v->output = seffect_putch;
			return;
		case 'z':		/* TW extension: clear special effects */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "clear text effects");
#endif
			v->output = ceffect_putch;
			return;
	}
	v->output = vt52_putch;
}

/*
 * escy1_putch(v, c): for when an ESC Y + char has been seen
 */
static void escy1_putch(TEXTWIN *v, unsigned int c)
{
	if (v->escy1 - ' ' < 0) {
		gotoxy (v, c - ' ', v->cy);
	} else {
		gotoxy (v, c - ' ', v->miny + v->escy1 - ' ');
	}
	v->output = vt52_putch;
}

/*
 * escy_putch(v, c): for when an ESC Y has been seen
 */
static void escy_putch(TEXTWIN *v, unsigned int c)
{
	v->escy1 = c;
	v->output = escy1_putch;
}

/*
 * vt52_putch(v, c): put character 'c' on screen 'v'. This is the default
 * for when no escape, etc. is active
 */
void vt52_putch(TEXTWIN *v, unsigned int c)
{
	int cx, cy;

	cx = v->cx;
	cy = v->cy;

	c &= 0x00ff;

	/* control characters */
	if (c < ' ')
	{
		switch (c)
		{
			case '\r':
				gotoxy (v, 0, cy - RELOFFSET (v));
				break;

			case '\n':
				new_line (v);
				break;

			case '\b':
				cub1 (v);
				break;

			case '\007':		/* BELL */
				if (con_fd)
				{
					/* xconout2: Bconout wrde zu deadlock fhren! */
					char ch = 7;
					Fwrite(con_fd, 1, &ch);
				}
				else
					Bconout(2, 7);
				break;

			case '\033':		/* ESC */
				v->output = putesc;
				break;

			case '\t':
				gotoxy (v, (v->cx +8) & ~7,
					v->cy - RELOFFSET (v));
				break;

			default:
				break;
		}
	}
	else
	{
		vt_quote_putch (v, c);
	}
}
