
#undef DEBUG_VT
#undef DUMP

/*
 * VT100 emulator for a GEM text window (with type TEXTWIN)
 * Also understands (most) ANSI escape sequences
 */

#include <support.h>

#include <cflib.h>

#include "global.h"
#include "console.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"


/*
 * lokale Prototypen
 */
static void escy_putch(TEXTWIN *v, int c);
static void soft_reset(TEXTWIN *v);
static void vt100_putesc (TEXTWIN *v, int c);

char	vt_table[] = 
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
	
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 

	' ', '!', '"', '#', '$', '%', '&', '\'', 
	'(', ')', '*', 0x03, 0x04, 0x01, 0x02, '/', 
	0xDB, '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', ':', ';', '<', '=', '>', '?', 
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_', 
	
	0xFA, 0xB1, ' ', 0x0C, 0x0D, ' ', 0xF8, 0xF1, 
	0xB0, ' ', 0xD9, 0xBF, 0xDA, 0xC0, 0xC5, 0xFF, 
	' ', 0xC4, ' ', '_', 0xC3, 0xB4, 0xC1, 0xC2, 
	0xB3, 0xF3, 0xF2, 0xE3, 0xF0, 0x9C, 0xF9, 
															0x7F, 
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
	0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 
	0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 
	0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 
	0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 
	0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

#ifdef DUMP
static int	dump_pos = 1;

static void dump(char c)
{
	if (dump_pos % 80 == 0)
	{
		printf("\n");
		dump_pos = 1;
	}
	printf("%c", c);
	dump_pos++;
}
#endif

/*
 * gototab(v, x, y):move cursor to next tab position
 */
static void gototab(TEXTWIN *v, int x, int y)
{
	TABLIST *tab;

	if (v->tabs) 
	{
		tab = v->tabs;
		while (tab != NULL && tab->tabpos <= x) 
			tab = tab->nexttab;
		if (tab != NULL && tab->tabpos > x)
			x = tab->tabpos;
		gotoxy (v, x, y);
	}
	curs_on(v);
	return;
}

/*
 * insert_char(v, x, y): insert a blank character at position (x, y)
 * on the screen; the rest of the line is scrolled right, and the last
 * character is lost.
 */
static void insert_char(TEXTWIN *v, int x, int y)
{
	int i;

	for (i = v->maxx; i > x; i--) 
	{
		v->data[y][i] = v->data[y][i-1];
		v->cflag[y][i] = v->cflag[y][i-1] | CDIRTY;
	}
	v->data[y][x] = ' ';
	v->cflag[y][x] = CDIRTY | (v->term_cattr & (CBGCOL|CFGCOL));
	v->dirty[y] |= SOMEDIRTY;
}

/*
 * delete_line(v, r, b): delete line r of window v. The screen up to and
 * including line b is scrolled up, and line b is cleared.
 */
static void delete_line(TEXTWIN *v, int r, int b)
{
	int 				y;
	int 				doscroll = (r == 0);
	unsigned char	*oldline;
	short				*oldflag;

	if (b > v->maxy - 1)
		b = v->maxy - 1;
	if (r == v->miny)
		r = 0;

	oldline = v->data[r];
	oldflag = v->cflag[r];
	for (y = r; y < b; y++) 
	{
		v->data[y] = v->data[y+1];
		v->cflag[y] = v->cflag[y+1];
		v->dirty[y] = doscroll ? v->dirty[y+1] : ALLDIRTY;
	}

	v->data[y] = oldline;
	v->cflag[y] = oldflag;

	/* clear the last line */
	clrline(v, b);
	if (doscroll)
		v->scrolled++;
}

/*
 * insert_line(v, r, b): scroll all of the window from line r to line b down,
 * and then clear line r.
 */
static void insert_line(TEXTWIN *v, int r, int b)
{
	int 				i, limit;
	unsigned char	*oldline;
	short				*oldflag;

	if (b > v->maxy - 1)
		b = v->maxy - 1;
	limit = b;
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

/*
 * capture(v, c): put character c into the capture buffer
 * if c is '\r', then we're finished and we call the callback
 * function
 */
void capture(TEXTWIN *v, int c)
{
	int i = v->captsiz;

	if (c == '\r') 
		c = 0;

	if (i < CAPTURESIZE || c == 0) 
	{
		v->captbuf[i++] = c;
		v->captsiz = i;
	}
	if (c == 0) 
	{
		v->output = vt100_putch;
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
	if (v->cx == v->maxx) 
	{
		if (v->term_flags & FWRAP) 
		{
			v->cx = 0;
			vt100_putch(v, '\n');
		}
	} 
	else
		v->cx++;
	curs_on(v);
	v->output = vt100_putch;
}

/* set special effects */
static void seffect_putch(TEXTWIN *v, int c)
{
	v->term_cattr |= ((c & 0x1f) << 8);
	v->output = vt100_putch;
}

/* clear special effects */
static void ceffect_putch(TEXTWIN *v, int c)
{
	v->term_cattr &= ~((c & 0x1f) << 8);
	v->output = vt100_putch;
}

/* Black, Red, Green, Yellow, Magenta, White, Blue, Cyan */
short vt100_color_translate[]={1,2,3,6,7,0,4,5,0,0,0,0,0,0,0,0};

static void fgcol_putch(TEXTWIN *v, int c)
{
	v->term_cattr = (v->term_cattr & ~CFGCOL) | (vt100_color_translate[c & 0x000f] << 4);
	v->output = vt100_putch;
}

static void bgcol_putch(TEXTWIN *v, int c)
{
	v->term_cattr = (v->term_cattr & ~CBGCOL) | vt100_color_translate[c & 0x000f];
	v->output = vt100_putch;
}

/*
 * pushescbuf (escbuf, c): add character c to buffer for extended escapes:
 * if we have an escape sequence > 127 characters, character 127 will be
 * overwritten with each character in turn
 */
static void pushescbuf (char *escbuf, int c)
{
	int i = 0;

	while ( i < (ESCBUFSIZE -2) && escbuf[i] != '\0' ) 
		i++;
	escbuf[i++] = c;
	escbuf[i] = '\0';
}

/*
 * popescbuf (escbuf): return int (n) from [?n, #n, }n or (n esc sequence:
  * removes first n; from buf (to handle multiple n;n;...;n)
 */
static int popescbuf (TEXTWIN *v, char *escbuf)
{
	int 	n = 0;
	char 	s[ESCBUFSIZE] = "\0";
	char 	*sp = s;
	char 	*ep1, *ep2;

	ep2 = ep1 = escbuf;
	while (*ep1 != '\0' && *ep1 != ';') 
		*sp++ = *ep1++;
	*sp = '\0';
	while (*ep1++ != '\0')
		*ep2++ = *ep1;
	*ep2 = '\0';
	if (s[0] != '\0')
	{
		n = atoi(s);
		return n;
	} 
	else
		return -1;
}

/*
 * keylockedl(v, c): discard all keys but l
 */
static void keylockedl(TEXTWIN *v, int c)
{
	switch (c) 
	{
		case 'l':		/* Unlock keyboard */
			v->output = vt100_putch;
			curs_on(v);
			break;
	}
}

/*
 * keylocked2(v, c): discard all keys but 2l
 */
static void keylocked2(TEXTWIN *v, int c)
{
	switch (c) 
	{
		case '2':
			v->output = keylockedl;
			break;
	}
}

/*
 * keylocked(v, c): discard all keys but ESC2l
 */
static void keylocked(TEXTWIN *v, int c)
{
	switch (c) 
	{
		case '\033':		/* ESC */
			v->output = keylocked2;
			break;
	}
}

/*
 * invert_scr(v): invert the screen
 */
static void invert_scr(TEXTWIN *v)
{
	short colour;
	int 	i, j;

	colour = v->term_cattr & CFGCOL;
	v->term_cattr = (v->term_cattr & ~CFGCOL) | ((v->term_cattr & CBGCOL) << 4);
	v->term_cattr = (v->term_cattr & ~CBGCOL) | (colour >> 4);
	for (i = v->miny; i < v->maxy; i++) 
	{
		for (j 	= v->maxx-1; j >= 0; --j) 
			v->cflag[i][j] = v->term_cattr & (CBGCOL|CFGCOL);
		v->dirty[i] = ALLDIRTY;
	}
}

/*
 * vt100_esc_mode (v, c): handle the control sequence ESC [?...n
 */
static void vt100_esc_mode (TEXTWIN *v, int c)
{
	int	cx, cy, count, ei;

	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_mode: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case 'h':		/* mode set */
			count = 0;
			do 
			{
				ei = popescbuf (v, v->escbuf);
				if (count == 0 && ei == -1)
					ei = 0;
				switch(ei) 
				{
					case 1:		/* Cursor keys in application/keypad transmit mode */
						v->curs_mode = CURSOR_TRANSMIT;
						break;
					case 3:		/* 132 column mode */
						resize_textwin(v, 132, NROWS(v), SCROLLBACK(v));
						break;
					case 4:		/* smooth scroll */
						/* Not supported sice version 2.0 */
						break;
					case 5:		/* inverse video on */
						invert_scr(v);
						break;
					case 6:		/* origin mode */
						/* Not yet implemented */
						break;
					case 7:		/* wrap on */
						v->term_flags |= FWRAP;
						break;
					case 8:		/* autorepeat on */
						/* Not yet implemented */
						break;
					case 9:		/* interface on */
						/* Not yet implemented */
						break;
					case 14:	/* immediate operation of ENTER */
						/* Not yet implemented */
						break;
					case 16:	/* edit selection immediate */
						/* Not yet implemented */
						break;
					case 25:	/* cursor on */
						v->term_flags |= FCURS;
						break;
					case 50:	/* cursor on */
						v->term_flags |= FCURS;
						break;
					case 75:	/* screen display on */
						/* Not yet implemented */
						break;
				}
				count++;
			} 
			while (ei >= 0);
			break;
		case 'l':		/* mode reset */
			count = 0;
			do 
			{
				ei = popescbuf (v, v->escbuf);
				if (count == 0 && ei == -1)
					ei = 0;
				switch(ei) 
				{
					case 1:		/* Cursor keys in cursor mode */
						v->curs_mode = CURSOR_NORMAL;
						break;
					case 2:		/* VT52 mode */
						v->vt_mode = MODE_VT52;
						v->scroll_top = v->miny;
						v->scroll_bottom = v->maxy -1 ;
						break;
					case 3:		/* 80 column mode */
						resize_textwin(v, 80, NROWS(v), SCROLLBACK(v));
						break;
					case 4:		/* jump scroll */
						/* Not supported since version 2.0 */
						break;
					case 5:		/* inverse video off */
						invert_scr(v);
						break;
					case 6:		/* relative mode */
						/* Not yet implemented */
						break;
					case 7:		/* wrap off */
						v->term_flags &= ~FWRAP;
						break;
					case 8:		/* autorepeat off */
						/* Not yet implemented */
						break;
					case 9:		/* interface off */
						/* Not yet implemented */
						break;
					case 14:	/* deferred operation of ENTER */
						/* Not yet implemented */
						break;
					case 16:	/* edit selection deferred */
						/* Not yet implemented */
						break;
					case 25:	/* cursor off */
						v->term_flags &= ~FCURS;
						break;
					case 50:	/* cursor off */
						v->term_flags &= ~FCURS;
						break;
					case 75:	/* screen display off */
						/* Not yet implemented */
						break;
				}
				count++;
			} 
			while (ei >= 0);
			break;
		case 'r':		/* mode restore */
			/* Not yet implemented */
			break;
		case 's':		/* mode save */
			/* Not yet implemented */
			break;
		case 'y':	/* invoke test(s) */
			if (popescbuf (v, v->escbuf) == 2) 
			{
				/* Not yet implemented */
			}
			break;
		case '0': 
		case '1': 
		case '2': 
		case '3':
		case '4': 
		case '5': 
		case '6': 
		case '7':
		case '8': 
		case '9': 
		case ';':	/* parameters for extended escape */
			pushescbuf (v->escbuf, c);
			return;
		default:
#ifdef DEBUG_VT
			debug("vt100_esc_mode: unknown %c\n", c);
#endif
			break;
	}
	curs_on(v);
	v->output = vt100_putch;
}

/*
 * cleartab(v, x): clear tab at position x
 */

static void cleartab(TEXTWIN *v, int x)
{
	TABLIST *tab, *oldtab;

	oldtab = tab = v->tabs;
	while (tab != NULL && tab->tabpos < x) 
	{
		oldtab = tab;
		tab = tab->nexttab;
	}
	if (tab != NULL && tab->tabpos == x) 
	{
		if (tab == v->tabs)
			v->tabs = tab->nexttab;
		else
			oldtab->nexttab = tab->nexttab;
		free (tab);
	}
}

/*
 * settab(v, x): set tab at position x
 */
static void settab (TEXTWIN *v, int x)
{
	TABLIST *tab, *oldtab, *newtab;

	oldtab = tab = v->tabs;
	while (tab != NULL && tab->tabpos < x) 
	{
		oldtab = tab;
		tab = tab->nexttab;
	}
	if (tab == NULL || (tab->tabpos != x)) 
	{
		if ((newtab = (TABLIST *) malloc(sizeof(TABLIST))) != NULL) 
		{
			newtab->tabpos = x;
			newtab->nexttab = tab;
			if (tab == v->tabs)
				v->tabs = newtab;
			else
				oldtab->nexttab = newtab;
		}
	}
}

/*
 * clearalltabs(v): clear all tabs and reset them at 8 character intervals.
 * Also referenced from textwin.c:create_textwin.
 */
void clearalltabs (TEXTWIN *v)
{
	TABLIST 	*tab, *oldtab;
	int 		x;

	oldtab = tab = v->tabs;
	while (tab != NULL) 
	{
		oldtab = tab;
		tab = tab->nexttab;
		free (oldtab);
	}
	v->tabs = NULL;

	x = 0;
	while (x < v-> maxx) 
	{
		settab(v, x);
		x += 8;
	}
}

/*
 * reportxy(v): Report cursor position as ESCy;xR
 */
static void reportxy(TEXTWIN *v, int x, int y)
{
	char r[ESCBUFSIZE] = "\0";

	/* Convert from internal to screen coordinates */
	x++;
	y++;
	sendstr(v,"\033");
	_ltoa(y, r, 10);
	sendstr(v,r);
	sendstr(v, ";");
	_ltoa(x, r, 10);
	sendstr(v,r);
	sendstr(v, "R");
}

/*
 * vt100_esc_attr (v, c): handle the control sequence ESC [...n
 */
static void vt100_esc_attr (TEXTWIN *v, int c)
{
	int 	cx, cy, count, ei;

	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_attr: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case '?':		/* start of screen/keyboard mode command */
			v->output = vt100_esc_mode;
			return;
		case '}':		/* set protected fields */
			/* Not yet implemented */
			break;
		case '@':		/* insert N characters */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			while (count) 
			{
				insert_char(v, cx, cy);
				count--;
			}
			break;
		case 'A':		/* cursor up N */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			gotoxy(v, cx, cy-count);
			break;
		case 'B':		/* cursor down N */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			gotoxy(v, cx, cy+count);
			break;
		case 'C':		/* cursor right N */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			gotoxy(v, cx+count, cy);
			break;
		case 'D':		/* cursor left N */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			gotoxy(v, cx-count, cy);
			break;
		case 'H':		/* cursor to y,x */
			cy = popescbuf (v, v->escbuf);
			if (cy < 1)
				cy = 1;
			cx = popescbuf (v, v->escbuf);
			if (cx < 1)
				cx = 1;
			gotoxy(v, cx-1, v->miny+cy-1);
			break;
		case 'J':		/* clear screen parts */
			count = popescbuf (v, v->escbuf);
			if (count < 0)
				count = 0;
			switch (count) 
			{
				case 0:		/* clear to end */
					clrfrom(v, cx, cy, v->maxx-1, v->maxy-1);
					break;
				case 1:		/* clear from beginning */
					clrfrom(v, 0, 0, cx, cy);
					break;
				case 2:		/* clear all but do no move */
					clrfrom(v, 0, 0, v->maxx-1, v->maxy-1);
					break;
			}
		case 'K':		/* clear line parts */
			count = popescbuf (v, v->escbuf);
			if (count < 0)
				count = 0;
			switch (count) 
			{
				case 0:		/* clear to end */
					clrfrom(v, cx, cy, v->maxx-1, cy);
					break;
				case 1:		/* clear from beginning */
					clrfrom(v, 0, cy, cx, cy);
					break;
				case 2:		/* clear all but do no move */
					clrfrom(v, 0, cy, v->maxx-1, cy);
					break;
			}
			break;
		case 'L':		/* insert N lines */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			while (count) 
			{
				insert_line(v, cy, v->scroll_bottom);
				count--;
			}
			gotoxy(v, 0, cy);
			break;
		case 'M':		/* delete N lines */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			while (count) 
			{
				delete_line(v, cy, v->scroll_bottom - 1);
				count --;
			}
			gotoxy(v, 0, cy);
			break;
		case 'P':		/* delete N characters at cursor position */
			count = popescbuf (v, v->escbuf);
			if (count < 1)
				count = 1;
			while (count) 
			{
				delete_char(v, cx, cy);
				count--;
			}
			break;
		case 'R':		/* Cursor position response */
			break;
		case 'c':		/* device attributes (0c is same)*/
			ei = popescbuf (v, v->escbuf);
			if (ei < 1) 
			{
				/* 2 = base vt100 with advanced video option (AVO) */
				sendstr(v,"\033[?1;2c");
			}
			break;
		case 'f':		/* cursor to y,x  same as H */
			vt100_esc_attr (v, 'H');
			break;
		case 'g':		/* clear tabs */
			count = popescbuf (v, v->escbuf);
			if (count < 0)
				count = 0;
			switch (count) 
			{
				case 0:		/* clear tab at cursor position */
					cleartab(v, cx);
					break;
				case 3:		/* clear all tabs */
					clearalltabs(v);
					break;
			}
			break;
		case 'h':		/* various keyboard & screen attributes */
			count = 0;
			do 
			{
				ei = popescbuf (v, v->escbuf);
				if (count == 0 && ei == -1)
					ei = 0;
				switch(ei) 
				{
					case 2:		/* keyboard locked */
						v->output = keylocked;
						break;
					case 4:		/* insert mode */
						v->term_flags |= FINSERT;
						break;
					case 20:	/* <return> sends crlf */
						/* Not yet implemented */
						break;
				}
				count++;
			} 
			while (ei >= 0);
			break;
		case 'l':		/* various keyboard & screen attributes */
			count = 0;
			do 
			{
				ei = popescbuf (v, v->escbuf);
				if (count == 0 && ei == -1)
					ei = 0;
				switch(ei) 
				{
					case 2:		/* keyboard unlocked */
						/* Unlock handled by keylockd() */
						break;
					case 4:		/* replace/overwrite mode */
						v->term_flags &= ~FINSERT;
						break;
					case 20:	/* <return> sends lf */
						/* Not yet implemented */
						break;
				}
				count++;
			} 
			while (ei >= 0);
			break;
		case 'm':		/* various text attributes */
			count = 0;
			do 
			{
				ei = popescbuf (v, v->escbuf);
				if (count == 0 && ei == -1)
					ei = 0;
				switch(ei) 
				{
					case 0:		/* attributes off */
						v->term_cattr &= ~CE_BOLD;
						v->term_cattr &= ~CE_LIGHT;
						v->term_cattr &= ~CE_ITALIC;
						v->term_cattr &= ~CE_UNDERLINE;
						v->term_cattr &= ~CINVERSE;
						break;
					case 1:		/* bold on */
						v->term_cattr |= CE_BOLD;
						break;
					case 2:		/* TW addition - light on */
						v->term_cattr |= CE_LIGHT;
						break;
					case 4:		/* underline on */
						v->term_cattr |= CE_UNDERLINE;
						break;
					case 5:		/* blink on - uses italics */
						v->term_cattr |= CE_ITALIC;
						break;
					case 7:		/* reverse video (standout) on */
						v->term_cattr |= CINVERSE;
						break;
					case 21:	/* bold off */
						v->term_cattr &= ~CE_BOLD;
						break;
					case 22:		/* TW addition - light off */
						v->term_cattr &= ~CE_LIGHT;
						break;
					case 24:	/* underline off */
						v->term_cattr &= ~CE_UNDERLINE;
						break;
					case 25:	/* blink off - uses italics */
						v->term_cattr &= ~CE_ITALIC;
						break;
					case 27:	/* reverse video (standout) off */
						v->term_cattr &= ~CINVERSE;
						break;
					case 30:	/* white foreground */
						fgcol_putch(v, '0');
						break;
					case 31:	/* black foreground */
						fgcol_putch(v, '1');
						break;
					case 32:	/* red foreground */
						fgcol_putch(v, '2');
						break;
					case 33:	/* green foreground */
						fgcol_putch(v, '3');
						break;
					case 34:	/* yellow foreground */
						fgcol_putch(v, '6');
						break;
					case 35:	/* blue foreground */
						fgcol_putch(v, '4');
						break;
					case 36:	/* magenta foreground */
						fgcol_putch(v, '7');
						break;
					case 37:	/* cyan foreground */
						fgcol_putch(v, '5');
						break;
					case 40:	/* white background */
						bgcol_putch(v, '0');
						break;
					case 41:	/* black background */
						bgcol_putch(v, '1');
						break;
					case 42:	/* red background */
						bgcol_putch(v, '2');
						break;
					case 43:	/* green background */
						bgcol_putch(v, '3');
						break;
					case 44:	/* yellow background */
						bgcol_putch(v, '6');
						break;
					case 45:	/* blue background */
						bgcol_putch(v, '4');
						break;
					case 46:	/* magenta background */
						bgcol_putch(v, '7');
						break;
					case 47:	/* cyan background */
						bgcol_putch(v, '5');
						break;
				}
				count++;
			} 
			while (ei >= 0);
			break;
		case 'n':
			count = popescbuf (v, v->escbuf);
			if (count < 0)
				count = 0;
			switch (count) 
			{
				case 5:		/* status report */
						/* response is ESC[0n for terminal OK */
					sendstr(v,"\033[0n");
					break;
				case 6:		/* cursor position report */
					reportxy(v, cx, cy - v->miny);
					break;
			}
			break;
		case 'p':
			if (v->escbuf[0] == '!') 
				soft_reset(v);
			break;
		case 'r':		/* set size of scrolling region */
			ei = popescbuf (v, v->escbuf);
			if (ei < 1) 	/* If not supplied ... */
				ei = 1;
			v->scroll_top = v->miny+ei-1;
			ei = popescbuf (v, v->escbuf);
			if (ei < 1) 	/* If not supplied ... */
				ei = NROWS (v);	/* ... set to size of screen */
			v->scroll_bottom = v->miny+ei-1;
			break;
		case 'q':	/* keyboard light controls */
			/* Not yet implemented */
			break;
		case 'x':	/* return terminal parameters */
			/* Not yet implemented */
			break;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9': case ';':	/* parameters for extended escape */
			pushescbuf (v->escbuf, c);
			return;
		default:
#ifdef DEBUG_VT
			printf("vt100_esc_attr: unknown %c\n", c);
#endif
			break;
	}
	curs_on(v);
	v->output = vt100_putch;
}

/*
 * vt100_esc_char_size (v, c): handle the control sequence ESC #n
 */
static void vt100_esc_char_size (TEXTWIN *v, int c)
{
	int cx, cy;

	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_char_size: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case '1':	/* double height, single width top half chars */
			/* Not yet implemented */
			break;
		case '2':	/* double height, single width bottom half chars */
			/* Not yet implemented */
			break;
		case '3':	/* double height, double width top half chars */
			/* Not yet implemented */
			break;
		case '4':	/* double height, double width bottom half chars */
			/* Not yet implemented */
			break;
		case '5':	/* single height, single width */
			/* Not yet implemented */
			break;
		case '6':	/* single height, double width */
			/* Not yet implemented */
			break;
		case '8':	/* fill screen with E's */
			/* Not yet implemented */
			break;
		default:
#ifdef DEBUG_VT
			printf("vt100_esc_char_size: unknown %c\n", c);
#endif
			break;
	}
	curs_on(v);
	v->output = vt100_putch;
}

/*
 * vt100_esc_tests (v, c): handle the control sequence ESC }n
 */
static void vt100_esc_tests (TEXTWIN *v, int c)
{
	int cx, cy;

	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_tests: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case '1':	/* fill screen with following character */
			/* Not yet implemented */
			break;
		case '2':	/* video attribute test */
			/* Not yet implemented */
			break;
		case '3':	/* character set test */
			/* Not yet implemented */
			break;
		default:
#ifdef DEBUG
			debug("vt100_esc_tests: unknown %c\n", c);
#endif
			break;
	}
	curs_on(v);
	v->output = vt100_putch;
}

/*
 * vt100_esc_std_char (v, c): handle the control sequence ESC (n
 * used to change G0 (standard) character set - should use appropriate font
 */

static void vt100_esc_std_char (TEXTWIN *v, int c)
{
	int cx, cy;

	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_std_char: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case 'A':		/* British */
			/* Not yet implemented */
			break;
		case 'B':		/* North American ASCII */
			v->char_set = NORM_CHAR;
			break;
		case 'C':		/* Finnish */
			/* Not yet implemented */
			break;
		case 'E':		/* Danish/Norwegian */
			/* Not yet implemented */
			break;
		case 'H':		/* Swedish */
			/* Not yet implemented */
			break;
		case 'K':		/* German */
			/* Not yet implemented */
			break;
		case 'Q':		/* French Canadian */
			/* Not yet implemented */
			break;
		case 'R':		/* Flemish (French/Belgian) */
			/* Not yet implemented */
			break;
		case 'Y':		/* Italian */
			/* Not yet implemented */
			break;
		case 'Z':		/* Spanish */
			/* Not yet implemented */
			break;
		case '0':		/* Line Drawing */
			v->char_set = LINE_CHAR;
			break;
		case '1':		/* Alternative Character */
			/* Not yet implemented */
			break;
		case '2':		/* Alternative Line Drawing */
			/* Not yet implemented */
			break;
		case '4':		/* Dutch */
			/* Not yet implemented */
			break;
		case '5':		/* Finnish */
			/* Not yet implemented */
			break;
		case '6':		/* Danish/Norwegian */
			/* Not yet implemented */
			break;
		case '7':		/* Swedish */
			/* Not yet implemented */
			break;
		case '=':		/* Swiss (French or German) */
			/* Not yet implemented */
			break;
		default:
#ifdef DEBUG_VT
			debug("vt100_esc_std_char: unknown %c\n", c);
#endif
			break;
	}
	curs_on(v);
	v->output = vt100_putch;
}

/*
 * vt100_esc_alt_char (v, c): handle the control sequence ESC )n
 * used to change G1 (alternate) character set - should use appropriate font
 */

static void vt100_esc_alt_char (TEXTWIN *v, int c)
{
	int cx, cy;

	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_alt_char: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case 'A':		/* British */
			/* Not yet implemented */
			break;
		case 'B':		/* North American ASCII */
			v->char_set = NORM_CHAR;
			break;
		case 'C':		/* Finnish */
			/* Not yet implemented */
			break;
		case 'E':		/* Danish/Norwegian */
			/* Not yet implemented */
			break;
		case 'H':		/* Swedish */
			/* Not yet implemented */
			break;
		case 'K':		/* German */
			/* Not yet implemented */
			break;
		case 'Q':		/* French Canadian */
			/* Not yet implemented */
			break;
		case 'R':		/* Flemish (French/Belgian) */
			/* Not yet implemented */
			break;
		case 'Y':		/* Italian */
			/* Not yet implemented */
			break;
		case 'Z':		/* Spanish */
			/* Not yet implemented */
			break;
		case '0':		/* Line Drawing */
			v->char_set = LINE_CHAR;
			break;
		case '1':		/* Alternative Character */
			/* Not yet implemented */
			break;
		case '2':		/* Alternative Line Drawing */
			/* Not yet implemented */
			break;
		case '4':		/* Dutch */
			/* Not yet implemented */
			break;
		case '5':		/* Finnish */
			/* Not yet implemented */
			break;
		case '6':		/* Danish/Norwegian */
			/* Not yet implemented */
			break;
		case '7':		/* Swedish */
			/* Not yet implemented */
			break;
		case '=':		/* Swiss (French or German) */
			/* Not yet implemented */
			break;
		default:
#ifdef DEBUG_VT
			debug("vt100_esc_alt_char: unknown %c\n", c);
#endif
			break;
	}
	curs_on(v);
	v->output = vt100_putch;
}

/*
 * soft_reset(v): do a soft reset
 */
static void soft_reset(TEXTWIN *v)
{
	v->escbuf[0] = '\0';				/* ESC <	- terminal into vt100 mode */
	vt100_putesc (v, '<');			/* ESC [20l	- <return> sends lf */
	v->escbuf[0] = '2';
	v->escbuf[1] = '0';
	v->escbuf[2] = '\0';
	vt100_esc_attr (v, 'l');		/* ESC [?1l	- cursor key to cursor mode */
											/* ESC [?6l	- absolute/origin mode */
											/* ESC [?9l	- interface off */
	v->escbuf[0] = '1';
	v->escbuf[1] = ';';
	v->escbuf[2] = '6';
	v->escbuf[3] = ';';
	v->escbuf[4] = '9';
	v->escbuf[5] = '\0';
	vt100_esc_mode (v, 'l');		/* ESC [r	- scrolling region to whole screen */
	v->escbuf[0] = '\0';
	vt100_esc_attr (v, 'r');		/* ESC q	- inverse video off*/
	vt100_putesc (v, 'q');			/* ESC (B	- standard character set is US ASCII */
	vt100_esc_std_char (v, 'B');	/* ^O 		- switch to standard character set */
	vt100_putch (v, '\017');		/* ESC )0	- alternate character set is special/linedraw */
	vt100_esc_alt_char (v, '0');	/* ESC >	- keypad in application mode */
	vt100_putesc (v, '>');
}

/*
 * full_reset(v): do a full reset
 */
void full_reset(TEXTWIN *v)
{
	soft_reset(v);							/* clear all tabs */
	clearalltabs (v);						/* white foreground, black foreground */
	fgcol_putch(v, '1');
	bgcol_putch(v, '0');					/* cursor on */
	v->term_flags |= FCURS;				/* all attributes off */
	v->term_cattr &= ~CE_BOLD;
	v->term_cattr &= ~CE_LIGHT;
	v->term_cattr &= ~CE_ITALIC;
	v->term_cattr &= ~CE_UNDERLINE;
	clear(v);
	gotoxy(v, 0, v->miny);
}

/*
 * vt100_putesc (v, c): handle the control sequence ESC c
 * additions JC - handle extended escapes,
 * 		e.g. ESC[24;1H is cursor to x1, y24
 */
static void vt100_putesc (TEXTWIN *v, int c)
{
	int 	cx, cy;

	curs_off(v);
	cx = v->cx; 
	cy = v->cy;

#ifdef DEBUG_VT
	debug("vt100_putesc: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) 
	{
		case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
			gotoxy(v, cx-1, cy);
			break;
		case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
			gotoxy(v, cx, cy+1);
			break;
		case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
			gotoxy(v, cx, cy-2);
			break;
		case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
			gotoxy(v, cx+1, cy);
			break;
		case '\033':		/* ESC - restart sequence */
			v->output = vt100_putesc;
			v->escbuf[0] = '\0';
			curs_on(v);
			return;
		case '<':		/* switch to vt100 mode */
			switch(v->vt_mode) 
			{
				case MODE_VT52:		/* switch to vt100 mode */
					v->vt_mode = MODE_VT100;
					break;
				case MODE_VT100:	/* Nothing to do */
					break;
			}
			break;
		case '=':		/* keypad in "keypad transmit" mode */
			/* Not yet implemented */
			break;
		case '>':		/* keypad out of "keypad transmit" mode */
			break;
		case '7':		/* save cursor position */
			v->savex = v->cx;
			v->savey = v->cy;
			break;
		case '8':		/* restore cursor position */
			gotoxy(v, v->savex, v->savey);
			break;
		case 'A':		/* cursor up */
			gotoxy(v, cx, cy-1);
			break;
		case 'B':		/* cursor down */
			gotoxy(v, cx, cy+1);
			break;
		case 'C':		/* cursor right */
			gotoxy(v, cx+1, cy);
			break;
		case 'D':
			switch (v->vt_mode) 
			{
				case MODE_VT52:		/* cursor left */
					gotoxy(v, cx-1, cy);
					break;
				case MODE_VT100:	/* cr */
					gotoxy(v, cx, cy+1);
					break;
			}
			break;
		case 'E':
			switch (v->vt_mode) 
			{
				case MODE_VT52:		/* clear home */
					clear(v);
					gotoxy(v, 0, v->miny);
					break;
				case MODE_VT100:	/* crlf */
					gotoxy(v, 0, cy+1);
					break;
			}
			break;
		case 'H':
			switch (v->vt_mode) 
			{
				case MODE_VT52:		/* cursor home */
					gotoxy(v, 0, v->miny);
					break;
				case MODE_VT100:	/* set tab at cursor position */
					settab(v, cx);
					break;
			}
			break;
		case 'I':		/* cursor up, insert line */
			if (cy == v->miny)
				insert_line(v, v->miny, v->maxy);
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
			insert_line(v, cy, v->maxy);
			gotoxy(v, 0, cy);
			break;
		case 'M':
			switch (v->vt_mode) 
			{
				case MODE_VT52:		/* delete line */
					delete_line(v, cy, v->maxy - 1);
					gotoxy(v, 0, cy);
					break;
				case MODE_VT100:	/* reverse cr */
					if (cy == v->scroll_top) 
					{
						curs_off(v);
						insert_line(v, v->scroll_top, v->scroll_bottom);
					}
					else
						gotoxy(v, 0, cy-1);
					break;
			}
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
		case 'Y':		/* Cursor motion follows */
			v->output = escy_putch;
			curs_on(v);
			return;
		case 'Z':		/* return terminal id */
			v->escbuf[0] = '\0';
			vt100_esc_attr (v, 'c');
			break;
		case 'a':		/* MW extension: delete character */
			delete_char(v, cx, cy);
			break;
		case 'b':		/* set foreground color */
			v->output = fgcol_putch;
			curs_on(v);
			return;		/* `return' to avoid resetting v->output */
		case 'c':
			switch (v->vt_mode) 
			{
				case MODE_VT52:		/* set background color */
					v->output = bgcol_putch;
					curs_on(v);
					return;
				case MODE_VT100:	/* reset */
					full_reset(v);
					break;
			}
			break;
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
		case 'w':		/* wrap off */
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
		case '[':	/* extended escape [ - various attributes */
			v->output = vt100_esc_attr;
			return;
		case '#':	/* extended escape # - ANSI character sizes */
			v->output = vt100_esc_char_size;
			return;
		case '}':	/* extended escape } - tests */
			v->output = vt100_esc_tests;
			return;
		case '('	/* extended escape ( - set G0 character set */:
			v->output = vt100_esc_std_char;
			return;
		case ')':	/* extended escape ( - set G1 character set */
			v->output = vt100_esc_alt_char;
			return;
		case '*':	/* extended escape ( - set G2 character set */
			/* Not yet implemented */
			return;
		case '+':	/* extended escape ( - set G3 character set */
			/* Not yet implemented */
			return;
		case '!':	/* program a progammable key (local) */
			/* Not yet implemented */
			break;
		case '@':	/* program a progammable key (on-line) */
			/* Not yet implemented */
			break;
		case '%':	/* transmit progammable key contents */
			/* Not yet implemented */
			break;
		default:
#ifdef DEBUG_VT
			debug("vt100_putesc: unknown %c\n", c);
#endif
			break;
	}
	v->output = vt100_putch;
	curs_on(v);
}

/*
 * escy1_putch(v, c): for when an ESC Y + char has been seen
 */
static void escy1_putch(TEXTWIN *v, int c)
{
#ifdef DEBUG_VT
	debug("escy1_putch: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	curs_off(v);
	gotoxy(v, c - ' ', v->miny + v->escy1 - ' ');
	v->output = vt100_putch;
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
 * vt100_putch(v, c): put character 'c' on screen 'v'. This is the default
 * for when no escape, etc. is active
 */
void vt100_putch(TEXTWIN *v, int c)
{
	int cx, cy;

#ifdef DUMP
dump(c);
#endif

	cx = v->cx; 
	cy = v->cy;
	curs_off(v);

	c &= 0x00ff;

	/* control characters */
	if (c < ' ') 
	{
		switch (c) 
		{
			case '\r':			/* carriage return */
				gotoxy(v, 0, cy);
				break;
				
			case '\n':			/* new line */
				if (cy == v->scroll_bottom)
				{
					curs_off(v);
					delete_line(v, v->scroll_top, v->scroll_bottom);
				}
				else 
				{
					if (v->cx >= v->maxx)
						gotoxy(v, 0, cy+1);
					else
						gotoxy(v, cx, cy+1);
				}
				break;
				
			case '\b':			/* backspace */
				gotoxy(v, cx-1, cy);
				break;
				
			case '\007':		/* bell */
				if (con_fd)						
				{
					/* xconout2: Bconout wrde zu deadlock fhren! */
					char ch = 7;
					Fwrite(con_fd, 1, &ch);
				}
				else
					Bconout(2, 7);
				break;
				
			case '\016':		/* ^N - switch to alternate char set */
				v->char_set = LINE_CHAR;
				break;
				
			case '\017':		/* ^O - switch to standard char set */
				v->char_set = NORM_CHAR;
				break;
				
			case '\033':		/* ESC */
				v->output = vt100_putesc;
				v->escbuf[0] = '\0';
				break;
				
			case '\t':		/* tab */
				gototab(v, v->cx, v->cy);
				break;
				
			default:
#ifdef DEBUG_VT
				debug("vt100_putch: unknown %c\n", c);
#endif
				break;
		}
	}
	else
	{
		if (v->char_set == LINE_CHAR)
			c = vt_table[c];
	
		paint(v, c);
		v->cx++;
		if (v->cx == v->maxx) 
		{
			if (v->term_flags & FWRAP) 
			{
				v->cx = 0;
				vt100_putch(v, '\n');
			}
			else
				v->cx = v->maxx - 1;
		}
	}
	curs_on(v);
}
