/*
 * VT52 emulator for a GEM text window (with type TEXTWIN)
 */

#include <osbind.h>

#include "global.h"
#include "console.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"
#include "ansicol.h"

#ifdef DEBUG
extern int do_debug;
# include <syslog.h>
#endif

/*
 * lokale prototypen
 */
static void escy_putch (TEXTWIN *v, unsigned int c);

/*
 * delete_line(v, r): delete line r of window v. The screen below this
 * line is scrolled up, and the bottom line is cleared.
 */
static void delete_line(TEXTWIN *v, int r)
{
	int 				y;
	int 				doscroll = (r == 0);
	unsigned char	*oldline;
	long 			*oldflag;

	oldline = v->data[r];
	oldflag = v->cflag[r];
	for (y = r; y < v->maxy-1; y++) 
	{
		v->data[y] = v->data[y+1];
		v->cflag[y] = v->cflag[y+1];
		v->dirty[y] = doscroll ? v->dirty[y+1] : ALLDIRTY;
	}

	v->data[y] = oldline;
	v->cflag[y] = oldflag;

	/* clear the last line */
	clrline(v, v->maxy - 1);
	if (doscroll)
		v->scrolled++;
}

/*
 * insert_line(v, r): scroll all of the window from line r down,
 * and then clear line r.
 */
static void insert_line(TEXTWIN *v, int r)
{
	int 				i, limit;
	unsigned char	*oldline;
	long 			*oldflag;

	limit = v->maxy - 1;
	oldline = v->data[limit];
	oldflag = v->cflag[limit];

	for (i = limit-1; i >= r ; --i) 
	{
		/* move line i to line i+1 */
		v->data[i+1] = v->data[i];
		v->cflag[i+1] = v->cflag[i];
		v->dirty[i+1] = ALLDIRTY;
	}

	v->cflag[r] = oldflag;
	v->data[r] = oldline;
	/* clear line r */
	clrline(v, r);
}

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

/*
 * paint a character, even if it's a graphic character
 */
static void quote_putch(TEXTWIN *v, unsigned int c)
{
	if (c == 0) 
		c = ' ';
	curs_off(v);
	paint(v, c);
	v->cx++;
	if (v->cx == v->maxx) 
	{
		if (v->term_flags & FWRAP) 
		{
			v->cx = 0;
			vt52_putch(v, '\n');
		} 
		else
			v->cx = v->maxx - 1;
	}

	curs_on(v);
	v->output = vt52_putch;
}

/* Legacy functions for color support.  */
static 
void fgcol_putch (TEXTWIN *v, unsigned int c)
{
	v->term_cattr = (v->term_cattr & ~CFGCOL) | 
			 ((c & 0xff) << 4);
	v->output = vt52_putch;
}

static 
void bgcol_putch (TEXTWIN *v, unsigned int c)
{
	v->term_cattr = (v->term_cattr & ~CBGCOL) | 
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
	v->term_cattr |= ((c & 0x1f) << 8);
	if (!v->vdi_colors)
		v->term_cattr &= ~CE_ANSI_EFFECTS;
	v->output = vt52_putch;
}

/* clear special effects */
static void ceffect_putch(TEXTWIN *v, unsigned int c)
{
	if (c == '_' && !v->vdi_colors)
		v->term_cattr &= ~CE_ANSI_EFFECTS;
	
	v->term_cattr &= ~((c & 0x1f) << 8);
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

	curs_off(v);
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
			gotoxy(v, cx, cy-1);
			break;
		case 'B':		/* cursor down */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor down");
#endif
			gotoxy(v, cx, cy+1);
			break;
		case 'C':		/* cursor right */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor right");
#endif
			gotoxy(v, cx+1, cy);
			break;
		case 'D':		/* cursor left */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor left");
#endif
			gotoxy(v, cx-1, cy);
			break;
		case 'F':		/* smacs, start alternate character set.  */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is smacs");
#endif
			v->term_cattr |= CACS;
			break;
			
		case 'G':		/* rmacs, end alternate character set.  */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is rmacs");
#endif
			v->term_cattr &= ~CACS;
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
			gotoxy(v, 0, v->miny);
			break;
		case 'I':		/* cursor up, insert line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor up, insert line");
#endif
			if (cy == v->miny)
				insert_line(v, v->miny);
			else
				gotoxy(v, cx, cy-1);
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
			insert_line(v, cy);
			gotoxy(v, 0, cy);
			break;
		case 'M':		/* delete line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is delete line");
#endif
			delete_line(v, cy);
			gotoxy(v, 0, cy);
			break;
		case 'Q':		/* MW extension: quote next character */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is quote next char");
#endif
			v->output = quote_putch;
			curs_on(v);
			return;
		case 'R':		/* TW extension: set window size */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set window size");
#endif
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_size;
			curs_on(v);
			return;
		case 'S':		/* MW extension: set title bar */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set title bar");
#endif
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_title;
			curs_on(v);
			return;
		case 'Y':
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is goto xy");
#endif
			v->output = escy_putch;
			curs_on(v);
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
			curs_on(v);
			return;		/* `return' to avoid resetting v->output */
		case 'c':		/* set background color */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set background color");
#endif
			v->output = bgcol_putch;
			curs_on(v);
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
			v->term_flags |= FCURS;
			break;
		case 'f':		/* cursor off */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is cursor off");
#endif
			v->term_flags &= ~FCURS;
			break;
		case 'h':		/* MW extension: enter insert mode */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is enter insert mode");
#endif
			v->term_flags |= FINSERT;
			break;
		case 'i':		/* MW extension: leave insert mode */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is leave insert mode");
#endif
			v->term_flags &= ~FINSERT;
			break;
		case 'j':		/* save cursor position */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is save cursor position");
#endif
			v->savex = v->cx;
			v->savey = v->cy;
			break;
		case 'k':		/* restore saved position */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is restore cursor position");
#endif
			gotoxy(v, v->savex, v->savey);
			break;
		case 'l':		/* clear line */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is clear line");
#endif
			clrline(v, cy);
			gotoxy(v, 0, cy);
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
			v->term_cattr |= CINVERSE;
			break;
		case 'q':		/* reverse video off */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is reverse video off");
#endif
			v->term_cattr &= ~CINVERSE;
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
			v->term_flags |= FWRAP;
			break;
		case 'w':
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is linewrap off");
#endif
			v->term_flags &= ~FWRAP;
			break;
		case 'y':		/* TW extension: set special effects */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "is set text effects");
#endif
			v->output = seffect_putch;
			curs_on(v);
			return;
		case 'z':		/* TW extension: clear special effects */
#ifdef DEBUG
			if (do_debug) syslog (LOG_ERR, "clear text effects");
#endif
			v->output = ceffect_putch;
			curs_on(v);
			return;
	}
	v->output = vt52_putch;
	curs_on(v);
}

/*
 * escy1_putch(v, c): for when an ESC Y + char has been seen
 */
static void escy1_putch(TEXTWIN *v, unsigned int c)
{
	curs_off(v);
	if (v->escy1 - ' ' < 0) {
		gotoxy (v, c - ' ', v->cy);
	} else {
		gotoxy (v, c - ' ', v->miny + v->escy1 - ' ');
	}
	v->output = vt52_putch;
	curs_on(v);
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
	curs_off(v);

	c &= 0x00ff;

	/* control characters */
	if (c < ' ') 
	{
		switch (c) 
		{
			case '\r':
				gotoxy(v, 0, cy);
				break;

			case '\n':
				if (cy == v->maxy - 1)
				{
					curs_off(v);
					delete_line(v, 0);
				}
				else 
				{
					gotoxy(v, cx, cy+1);
				}
				break;

			case '\b':
				gotoxy(v, cx-1, cy);
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
				gotoxy(v, (v->cx +8) & ~7, v->cy);
				break;

			default:
				break;
		}
	}
	else
	{
		paint(v, c);
		v->cx++;
		if (v->cx == v->maxx) 
		{
			if (v->term_flags & FWRAP) 
			{
				v->cx = 0;
				vt52_putch(v, '\n');
			} 
			else
				v->cx = v->maxx - 1;
		}
	}
	curs_on(v);
}
