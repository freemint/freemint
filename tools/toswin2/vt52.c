/*
 * VT52 emulator for a GEM text window (with type TEXTWIN)
 */

#include <osbind.h>

#include "vt.h"
#include "ansicol.h"
#include "console.h"

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
	v->curr_cattr = (v->curr_cattr & ~CFGCOL) | ((c & 0x0f) << 4);
	v->output = vt52_putch;
}

static
void bgcol_putch (TEXTWIN *v, unsigned int c)
{
	v->curr_cattr = (v->curr_cattr & ~CBGCOL) | (c & 0x0f);
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
		SYSLOG((LOG_ERR, "got escape character %d (%s, 0x%08x)", c, printable, c));
	}
#endif

	cx = v->cx;
	cy = v->cy;

	switch (c)
	{
		case '3':		/* GFL: Set ANSI fg color.  */
			SYSLOG((LOG_ERR, "is set ANSI fg"));
			v->output = ansi_fgcol_putch;
			return;
		case '4':		/* GFL: Set ANSI fg color.  */
			SYSLOG((LOG_ERR, "is set ANSI bg"));
			v->output = ansi_bgcol_putch;
			return;
		case 'A':		/* cursor up */
			SYSLOG((LOG_ERR, "is cursor up"));
			cuu1 (v);
			break;
		case 'B':		/* cursor down */
			SYSLOG((LOG_ERR, "is cursor down"));
			cud1 (v);
			break;
		case 'C':		/* cursor right */
			SYSLOG((LOG_ERR, "is cursor right"));
			cuf1 (v);
			break;
		case 'D':		/* cursor left */
			SYSLOG((LOG_ERR, "is cursor left"));
			cub1 (v);
			break;
		case 'E':		/* clear home */
			SYSLOG((LOG_ERR, "is clear"));
			clear(v);
			gotoxy (v, 0, 0);
			break;
		case 'F':		/* smacs, start alternate character set.  */
			SYSLOG((LOG_ERR, "is smacs"));
			v->curr_tflags = (v->curr_tflags & ~TCHARSET_MASK) | 1;
			break;
		case 'G':		/* rmacs, end alternate character set.  */
			SYSLOG((LOG_ERR, "is rmacs"));
			v->curr_tflags = v->curr_tflags & ~TCHARSET_MASK;
			break;
		case 'H':		/* cursor home */
			SYSLOG((LOG_ERR, "is home"));
			gotoxy (v, 0, 0);
			break;
		case 'I':		/* cursor up, insert line */
			SYSLOG((LOG_ERR, "is cursor up, insert line"));
			reverse_cr (v);
			break;
		case 'J':		/* clear below cursor */
			SYSLOG((LOG_ERR, "is clear below cursor"));
			clrfrom(v, cx, cy, v->maxx-1, v->maxy-1);
			break;
		case 'K':		/* clear remainder of line */
			SYSLOG((LOG_ERR, "is clear rest of line"));
			clrfrom(v, cx, cy, v->maxx-1, cy);
			break;
		case 'L':		/* insert a line */
			SYSLOG((LOG_ERR, "is insert line"));
			insert_line (v);
			break;
		case 'M':		/* delete line */
			SYSLOG((LOG_ERR, "is delete line"));
			delete_line (v);
			break;
		case 'R':		/* TW extension: set window size */
			SYSLOG((LOG_ERR, "is set window size"));
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_size;
			return;
		case 'S':		/* MW extension: set title bar */
			SYSLOG((LOG_ERR, "is set title bar"));
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_title;
			return;
		case 'Y':
			SYSLOG((LOG_ERR, "is goto xy"));
			v->output = escy_putch;
			return;
		case 'Z':
			sendstr(v, "\033/Z");
			break;
		case 'a':		/* MW extension: delete character */
			SYSLOG((LOG_ERR, "is delete character"));
			delete_char(v, cx, cy);
			break;
		case 'b':		/* set foreground color */
			SYSLOG((LOG_ERR, "is set foreground color"));
			v->output = fgcol_putch;
			return;		/* `return' to avoid resetting v->output */
		case 'c':		/* set background color */
			SYSLOG((LOG_ERR, "is set background color"));
			v->output = bgcol_putch;
			return;
		case 'd':		/* clear to cursor position */
			SYSLOG((LOG_ERR, "is clear to cursor position"));
			clrfrom(v, 0, v->miny, cx, cy);
			break;
		case 'e':		/* enable cursor */
			SYSLOG((LOG_ERR, "is cursor on"));
			v->curr_tflags |= TCURS_ON;
			break;
		case 'f':		/* cursor off */
			SYSLOG((LOG_ERR, "is cursor off"));
			v->curr_tflags &= ~TCURS_ON;
			break;
		case 'h':		/* MW extension: enter insert mode */
			SYSLOG((LOG_ERR, "is enter insert mode"));
			v->curr_tflags |= TINSERT;
			break;
		case 'i':		/* MW extension: leave insert mode */
			SYSLOG((LOG_ERR, "is leave insert mode"));
			v->curr_tflags &= ~TINSERT;
			break;
		case 'j':		/* save cursor position */
			SYSLOG((LOG_ERR, "is save cursor position"));
			v->saved_x = v->cx;
			v->saved_y = v->cy;
			break;
		case 'k':		/* restore saved position */
			SYSLOG((LOG_ERR, "is restore cursor position"));
			/* FIXME: origin mode!!! */
			gotoxy (v, v->saved_x, v->saved_y);
			break;
		case 'l':		/* clear line */
			SYSLOG((LOG_ERR, "is clear line"));
			clrline (v, cy);
			gotoxy (v, 0, cy - RELOFFSET (v));
			break;
		case 'o':		/* clear from start of line to cursor */
			SYSLOG((LOG_ERR, "is clear from start of line to cursor"));
			clrfrom(v, 0, cy, cx, cy);
			break;
		case 'p':		/* reverse video on */
			SYSLOG((LOG_ERR, "is reverse video on"));
			v->curr_cattr |= CINVERSE;
			break;
		case 'q':		/* reverse video off */
			SYSLOG((LOG_ERR, "is reverse video off"));
			v->curr_cattr &= ~CINVERSE;
			break;
		case 't':		/* backward compatibility for TW 1.x: set cursor timer */
			SYSLOG((LOG_ERR, "set cursor timer"));
			return;
		case 'u':		/* original colors */
			SYSLOG((LOG_ERR, "is original colors"));
			original_colors (v);
			break;
		case 'v':		/* wrap on */
			SYSLOG((LOG_ERR, "is linewrap on"));
			v->curr_tflags |= TWRAPAROUND;
			break;
		case 'w':
			SYSLOG((LOG_ERR, "is linewrap off"));
			v->curr_tflags &= ~TWRAPAROUND;
			break;
		case 'y':		/* TW extension: set special effects */
			SYSLOG((LOG_ERR, "is set text effects"));
			v->output = seffect_putch;
			return;
		case 'z':		/* TW extension: clear special effects */
			SYSLOG((LOG_ERR, "clear text effects"));
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
	c &= 0x00ff;

	/* control characters */
	if (c < ' ')
	{
		switch (c)
		{
			case '\r':
				gotoxy (v, 0, v->cy - RELOFFSET (v));
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
