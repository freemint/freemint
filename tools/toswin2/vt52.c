/*
 * VT52 emulator for a GEM text window (with type TEXTWIN)
 */

#include <cflib.h>

#include "global.h"
#include "console.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"


/*
 * lokale prototypen
 */
static void escy_putch(TEXTWIN *v, int c);

/*
 * delete_line(v, r): delete line r of window v. The screen below this
 * line is scrolled up, and the bottom line is cleared.
 */
static void delete_line(TEXTWIN *v, int r)
{
	int 				y;
	int 				doscroll = (r == 0);
	unsigned char	*oldline;
	short 			*oldflag;

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
	short 			*oldflag;

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
static void capture(TEXTWIN *v, int c)
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
static void quote_putch(TEXTWIN *v, int c)
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

short color_translate[]={0,2,3,6,4,7,5,8,9,10,11,14,12,15,13,1};

static void fgcol_putch(TEXTWIN *v, int c)
{
	v->term_cattr = (v->term_cattr & ~CFGCOL) | (color_translate[c & 0x000f]<<4);
	v->output = vt52_putch;
}

static void bgcol_putch(TEXTWIN *v, int c)
{
	v->term_cattr = (v->term_cattr & ~CBGCOL) | color_translate[c & 0x000f];
	v->output = vt52_putch;
}

/* set special effects */
static void seffect_putch(TEXTWIN *v, int c)
{
	v->term_cattr |= ((c & 0x1f) << 8);
	v->output = vt52_putch;
}

/* clear special effects */
static void ceffect_putch(TEXTWIN *v, int c)
{
	v->term_cattr &= ~((c & 0x1f) << 8);
	v->output = vt52_putch;
}

/*
 * putesc(v, c): handle the control sequence ESC c
 */
static void putesc(TEXTWIN *v, int c)
{
	int 		cx, cy;

	curs_off(v);
	cx = v->cx; 
	cy = v->cy;

	switch (c) 
	{
		case 'A':		/* cursor up */
			gotoxy(v, cx, cy-1);
			break;
		case 'B':		/* cursor down */
			gotoxy(v, cx, cy+1);
			break;
		case 'C':		/* cursor right */
			gotoxy(v, cx+1, cy);
			break;
		case 'D':		/* cursor left */
			gotoxy(v, cx-1, cy);
			break;
		case 'E':		/* clear home */
			clear(v);
			/* fall through... */
		case 'H':		/* cursor home */
			gotoxy(v, 0, v->miny);
			break;
		case 'I':		/* cursor up, insert line */
			if (cy == v->miny)
				insert_line(v, v->miny);
			else
				gotoxy(v, cx, cy-1);
			break;
		case 'J':		/* clear below cursor */
			clrfrom(v, cx, cy, v->maxx-1, v->maxy-1);
			break;
		case 'K':		/* clear remainder of line */
			clrfrom(v, cx, cy, v->maxx-1, cy);
			break;
		case 'L':		/* insert a line */
			insert_line(v, cy);
			gotoxy(v, 0, cy);
			break;
		case 'M':		/* delete line */
			delete_line(v, cy);
			gotoxy(v, 0, cy);
			break;
		case 'Q':		/* MW extension: quote next character */
			v->output = quote_putch;
			curs_on(v);
			return;
		case 'R':		/* TW extension: set window size */
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_size;
			curs_on(v);
			return;
		case 'S':		/* MW extension: set title bar */
			v->captsiz = 0;
			v->output = capture;
			v->callback = set_title;
			curs_on(v);
			return;
		case 'Y':
			v->output = escy_putch;
			curs_on(v);
			return;
		case 'a':		/* MW extension: delete character */
			delete_char(v, cx, cy);
			break;
		case 'b':		/* set foreground color */
			v->output = fgcol_putch;
			curs_on(v);
			return;		/* `return' to avoid resetting v->output */
		case 'c':		/* set background color */
			v->output = bgcol_putch;
			curs_on(v);
			return;
		case 'd':		/* clear to cursor position */
			clrfrom(v, 0, v->miny, cx, cy);
			break;
		case 'e':		/* enable cursor */
			v->term_flags |= FCURS;
			break;
		case 'f':		/* cursor off */
			v->term_flags &= ~FCURS;
			break;
		case 'h':		/* MW extension: enter insert mode */
			v->term_flags |= FINSERT;
			break;
		case 'i':		/* MW extension: leave insert mode */
			v->term_flags &= ~FINSERT;
			break;
		case 'j':		/* save cursor position */
			v->savex = v->cx;
			v->savey = v->cy;
			break;
		case 'k':		/* restore saved position */
			gotoxy(v, v->savex, v->savey);
			break;
		case 'l':		/* clear line */
			clrline(v, cy);
			gotoxy(v, 0, cy);
			break;
		case 'o':		/* clear from start of line to cursor */
			clrfrom(v, 0, cy, cx, cy);
			break;
		case 'p':		/* reverse video on */
			v->term_cattr |= CINVERSE;
			break;
		case 'q':		/* reverse video off */
			v->term_cattr &= ~CINVERSE;
			break;
		case 't':		/* backward compatibility for TW 1.x: set cursor timer */
			return;
		case 'v':		/* wrap on */
			v->term_flags |= FWRAP;
			break;
		case 'w':
			v->term_flags &= ~FWRAP;
			break;
		case 'y':		/* TW extension: set special effects */
			v->output = seffect_putch;
			curs_on(v);
			return;
		case 'z':		/* TW extension: clear special effects */
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
static void escy1_putch(TEXTWIN *v, int c)
{
	curs_off(v);
	gotoxy(v, c - ' ', v->miny + v->escy1 - ' ');
	v->output = vt52_putch;
	curs_on(v);
}

/*
 * escy_putch(v, c): for when an ESC Y has been seen
 */
static void escy_putch(TEXTWIN *v, int c)
{
	curs_off(v);
	v->escy1 = c;
	v->output = escy1_putch;
	curs_on(v);
}

/*
 * vt52_putch(v, c): put character 'c' on screen 'v'. This is the default
 * for when no escape, etc. is active
 */
void vt52_putch(TEXTWIN *v, int c)
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
					gotoxy(v, cx, cy+1);
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
