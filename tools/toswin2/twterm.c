/* Emulator for DEC VT100 series terminal for MiNT.
* Copyright (C) 2001, Guido Flohr <guido@freemint.de>,
* all rights reserved.
 */

#undef DEBUG_VT
#undef DUMP

#include <mintbind.h>
#include <support.h>
#include <string.h>

#include "global.h"
#include "console.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"
#include "ansicol.h"

/* Local prototypes.  */
static void escy_putch (TEXTWIN* tw, unsigned int c);
static void soft_reset (TEXTWIN* tw);
static void vt100_putesc (TEXTWIN* tw, unsigned int c);
static void clearalltabs (TEXTWIN* tw);
static void fill_window (TEXTWIN* tw, unsigned int c);
static void decscnm (TEXTWIN* tw, int flag);

#ifdef DUMP
static int dump_pos = 1;

static void dump(char c)
{
	if (dump_pos % 80 == 0) {
		puts("");
		dump_pos = 1;
	}
	printf("%c", c);
	++dump_pos;
}
#endif

/* Move cursor in textwin TW from position (X|Y)
* to next tab position.  */
static void gototab(TEXTWIN* tw, int x, int y)
{
	TABLIST *tab;

	if (tw->tabs) {
		for (tab = tw->tabs; tab != NULL && tab->tabpos <= x;
		     tab = tab->nexttab);
		if (tab != NULL && tab->tabpos > x)
			x = tab->tabpos;
		gotoxy(tw, x, y);
	}
	curs_on (tw);
	return;
}

/* Insert a space at position (X|Y).  Scroll rest of the line
 * to the right.  Discard characters pushed out of the screen.  */
static void insert_char(TEXTWIN* tw, int x, int y)
{
	int i;
	unsigned char *twdata = tw->data[y];
	unsigned long *twcflag = tw->cflag[y];

	if (x < 1 || (NCOLS (tw) - x < 1))
		return;
	
	memmove (twdata + x, twdata + x - 1, NCOLS (tw) - x);
	for (i = NCOLS (tw) - 1; i > x; --i) {
		twcflag[i] = twcflag[i - 1] | CDIRTY;
	}
	twdata[x] = ' ';
	twcflag[x] = CDIRTY | (tw->term_cattr & (CBGCOL | CFGCOL));
	tw->dirty[y] |= SOMEDIRTY;
	tw->do_wrap = 0;
}

/* Delete line ROW of window TW. The screen up to and
* including line END is scrolled up, and line END is cleared.
 */
static void delete_line(TEXTWIN* tw, int row, int end)
{
	int doscroll = (row == 0);
	unsigned char *oldline;
	long *oldflag;

	if (end > tw->maxy - 1)
		end = tw->maxy - 1;
	if (row == tw->miny)
		row = 0;

	/* FIXME: A ring buffer would be better.  */	
	oldline = tw->data[row];
	oldflag = tw->cflag[row];
	memmove (tw->data + row, tw->data + row + 1,
		 (end - row) * sizeof (tw->data[row]));
	memmove (tw->cflag + row, tw->cflag + row + 1,
		 (end - row) * sizeof (tw->cflag[row]));
	if (doscroll) {
		memmove (tw->dirty + row, tw->dirty + row + 1,
			 (end - row) * sizeof (tw->dirty[row]));
	} else {
		memset (tw->dirty, ALLDIRTY, end - row);
	}
	tw->data[end] = oldline;
	tw->cflag[end] = oldflag;

	/* clear the last line */
	clrline(tw, end);
	if (doscroll)
		++tw->scrolled;
	tw->do_wrap = 0;
}

/* Scroll the entire window from line ROW to line END down,
 * then clear line ROW.
 */
static void insert_line(TEXTWIN* tw, int row, int end)
{
	unsigned char *oldline;
	long *oldflag;

	if (end > tw->maxy - 1)
		end = tw->maxy - 1;

	/* FIXME: A ring buffer would be better.  */	
	oldline = tw->data[end];
	oldflag = tw->cflag[end];
	memulset (tw->dirty + row, ALLDIRTY, end - row);
	memmove (tw->data + row - 1, tw->data + row, 
		 (end - row) * sizeof tw->data[0]);
	memmove (tw->cflag + row - 1, tw->cflag + row,
		 (end - row) * sizeof tw->cflag[0]);
	tw->cflag[row] = oldflag;
	tw->data[row] = oldline;

	clrline(tw, row);
	tw->do_wrap = 0;
}

/* Store character C in the internal capture buffer. If 
 * C is a carriage return, the buffer is closed and passed
 * to the callback function.   
 */
static void 
capture (TEXTWIN* tw, unsigned int c)
{
	int i = tw->captsiz;

	if (c == '\r')
		c = 0;

	if (i < CAPTURESIZE || c == 0) {
		tw->captbuf[i++] = c;
		tw->captsiz = i;
		if (c == 0) {
			tw->output = vt100_putch;
			(*tw->callback) (tw);
		}
	}
}

/* Set special effects.  FIXME: A separate function is waste
 * of time for this.  */
static void 
seffect_putch (TEXTWIN* tw, unsigned int c)
{
	tw->term_cattr |= ((c & 0x1f) << 8);
	tw->output = vt100_putch;
}

/* clear special effects */
static void 
ceffect_putch(TEXTWIN* tw, unsigned int c)
{
	tw->term_cattr &= ~((c & 0x1f) << 8);
	tw->output = vt100_putch;
}

/* Add character C to buffer for extended escapes:
 * if we have an escape sequence > 127 characters, character 127 will be
 * overwritten with each character in turn
 */
static void 
pushescbuf (unsigned char* escbuf, unsigned int c)
{
	int i = 0;

	while (i < (ESCBUFSIZE - 2) && escbuf[i] != '\0')
		i++;
	/* Two semi-colons in a row are the same as '0;'.  */
	if (c == ';' && i >= 1 && escbuf[i - 1] == ';') {
		pushescbuf (escbuf, '0');
		pushescbuf (escbuf, c);
		return;
	}
	escbuf[i++] = c;
	escbuf[i] = '\0';
}

/* Fill window TW with character C.  */
static void
fill_window (TEXTWIN* tw, unsigned int c)
{
	int row;
	unsigned long new_attribute = tw->term_cattr & 
		(CBGCOL | CFGCOL);
	int cols = NCOLS (tw);
	
	for (row = tw->miny; row < tw->maxy; ++row) {
		memset (tw->data[row], c, cols);
		memulset (tw->cflag[row], new_attribute, cols);
	}
	memset (tw->dirty + tw->miny, ALLDIRTY, NROWS (tw));
	curs_on (tw);
}

/* Return int (n) from [?n, #n, }n or (n esc sequence:
 * Removes first n; from buf (to handle multiple n;n;...;n)
 */
static int 
popescbuf (TEXTWIN* tw, unsigned char* escbuf)
{
	int n = 0;
	char s[ESCBUFSIZE] = "\0";
	char *sp = s;
	char *ep1, *ep2;

	ep2 = ep1 = escbuf;
	while (*ep1 != '\0' && *ep1 != ';')
		*sp++ = *ep1++;
	*sp = '\0';
	while (*ep1++ != '\0')
		*ep2++ = *ep1;
	*ep2 = '\0';
	if (s[0] != '\0') {
		n = atoi(s);
		return n;
	} else
		return -1;
}

/* Discard all keys but l.  */
static void 
keylockedl (TEXTWIN* tw, unsigned int c)
{
	switch (c) {
	case 'l':		/* Unlock keyboard */
		tw->output = vt100_putch;
		curs_on (tw);
		break;
	}
}

/* Discard all characters but '2l'.  */
static void 
keylocked2 (TEXTWIN* tw, unsigned int c)
{
	switch (c) {
	case '2':
		tw->output = keylockedl;
		break;
	}
}

/* Discard all characters but ESC2l.  */
static void 
keylocked (TEXTWIN* tw, unsigned int c)
{
	switch (c) {
	case '\033':		/* ESC */
		tw->output = keylocked2;
		break;
	}
}

/* Turn inverse video on/off.  */
static void 
decscnm (TEXTWIN* tw, int flag)
{
	if (flag && tw->decscnm)
		return;
	if (!flag && !tw->decscnm)
		return;

	tw->cfg->bg_color ^= tw->cfg->fg_color;
	tw->cfg->fg_color ^= tw->cfg->bg_color;
	tw->cfg->bg_color ^= tw->cfg->fg_color;
	tw->cfg->bg_effects ^= tw->cfg->fg_effects;
	tw->cfg->fg_effects ^= tw->cfg->bg_effects;
	tw->cfg->bg_effects ^= tw->cfg->fg_effects;
	
	tw->decscnm = !tw->decscnm;

	memset (tw->dirty + tw->miny, ALLDIRTY, NROWS (tw));
	refresh_textwin (tw, 0);
}

/* Handle control sequence ESC [?...n.  */
static void vt100_esc_mode(TEXTWIN* tw, unsigned int c)
{
	int cx, cy, count, ei;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_mode: %c (%u)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy(tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy(tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
	case 'h':		/* mode set */
		count = 0;
		do {
			ei = popescbuf(tw, tw->escbuf);
			if (count == 0 && ei == -1)
				ei = 0;
			switch (ei) {
			case 1:	/* Cursor keys in application/keypad transmit mode */
				tw->curs_mode = CURSOR_TRANSMIT;
				break;
			case 3:	/* DECCOLM - 132 column mode.  The original VT100
				   also did a clear/home on that.  */
				if (tw->deccolm) {
					resize_textwin (tw, 132, NROWS (tw),
							SCROLLBACK (tw));
					clear (tw);
					gotoxy (tw, 0, 0);
				}
				break;
			case 4:	/* smooth scroll */
				/* Not supported since version 2.0 */
				break;
			case 5:	/* DECSCNM: inverse video on */
				decscnm (tw, 1);
				break;
			case 6:	/* origin mode */
				/* Not yet implemented */
				break;
			case 7:	/* wrap on */
				tw->term_flags &= ~FNOAM;
				break;
			case 8:	/* autorepeat on */
				/* Not yet implemented */
				break;
			case 9:	/* interface on */
				/* Not yet implemented */
				break;
			case 14:	/* immediate operation of ENTER */
				/* Not yet implemented */
				break;
			case 16:	/* edit selection immediate */
				/* Not yet implemented */
				break;
			case 25:	/* cursor on */
				tw->term_flags |= FCURS;
				break;
			case 40:	/* Set column mode.  */
				tw->deccolm = 1;
				break;
			case 50:	/* cursor on */
				tw->term_flags |= FCURS;
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
		do {
			ei = popescbuf(tw, tw->escbuf);
			if (count == 0 && ei == -1)
				ei = 0;
			switch (ei) {
			case 1:	/* Cursor keys in cursor mode */
				tw->curs_mode = CURSOR_NORMAL;
				break;
			case 2:	/* VT52 mode */
				tw->vt_mode = MODE_VT52;
				tw->scroll_top = tw->miny;
				tw->scroll_bottom = tw->maxy - 1;
				break;
			case 3:	/* DECCOLM - 80 column mode.  The original VT100
				   also did a clear/home on that.  */
				if (tw->deccolm) {
					resize_textwin (tw, 80, NROWS (tw),
							SCROLLBACK (tw));
					clear (tw);
					gotoxy (tw, 0, 0);
				}
				break;
			case 4:	/* jump scroll */
				/* Not supported since version 2.0 */
				break;
			case 5:	/* DECSCNM: inverse video off.  */
				decscnm (tw, 0);
				break;
			case 6:	/* relative mode */
				/* Not yet implemented */
				break;
			case 7:	/* wrap off */
				tw->term_flags |= FNOAM;
				break;
			case 8:	/* autorepeat off */
				/* Not yet implemented */
				break;
			case 9:	/* interlace off */
				/* Not yet implemented */
				break;
			case 14:	/* deferred operation of ENTER */
				/* Not yet implemented */
				break;
			case 16:	/* edit selection deferred */
				/* Not yet implemented */
				break;
			case 25:	/* cursor off */
				tw->term_flags &= ~FCURS;
				break;
			case 40:	/* Reset column mode.  */
				tw->deccolm = 0;
				break;
			case 50:	/* cursor off */
				tw->term_flags &= ~FCURS;
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
	case 'y':		/* invoke test(s) */
		if (popescbuf(tw, tw->escbuf) == 2) {
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
	case ';':		/* parameters for extended escape */
		pushescbuf(tw->escbuf, c);
		return;
	default:
#ifdef DEBUG_VT
		debug("vt100_esc_mode: unknown %c\n", c);
#endif
		break;
	}
	curs_on (tw);
	tw->output = vt100_putch;
}

/*
* cleartab(tw, x): clear tab at position x
 */

static void cleartab(TEXTWIN* tw, int x)
{
	TABLIST *tab, *oldtab;

	oldtab = tab = tw->tabs;
	while (tab != NULL && tab->tabpos < x) {
		oldtab = tab;
		tab = tab->nexttab;
	}
	if (tab != NULL && tab->tabpos == x) {
		if (tab == tw->tabs)
			tw->tabs = tab->nexttab;
		else
			oldtab->nexttab = tab->nexttab;
		free(tab);
	}
}

/*
* settab(tw, x): set tab at position x
 */
static void settab(TEXTWIN* tw, int x)
{
	TABLIST *tab, *oldtab, *newtab;

	oldtab = tab = tw->tabs;
	while (tab != NULL && tab->tabpos < x) {
		oldtab = tab;
		tab = tab->nexttab;
	}
	if (tab == NULL || (tab->tabpos != x)) {
		if ((newtab = (TABLIST *) malloc(sizeof(TABLIST))) != NULL) {
			newtab->tabpos = x;
			newtab->nexttab = tab;
			if (tab == tw->tabs)
				tw->tabs = newtab;
			else
				oldtab->nexttab = newtab;
		}
	}
}

/*
 * Clear all tabs and reset them at 8 character intervals.
 * Also referenced from textwin.c:create_textwin.
 */
void 
reset_tabs (TEXTWIN* tw)
{
	int x = 0;

	clearalltabs (tw);

	for (x = 0; x < tw->maxx; x+= 8) {
		settab(tw, x);
	}
}

static void
clearalltabs (TEXTWIN* tw)
{
	TABLIST* tab;
	TABLIST* oldtab;
	
	oldtab = tab = tw->tabs;
	while (tab != NULL) {
		oldtab = tab;
		tab = tab->nexttab;
		free (oldtab);
	}
	tw->tabs = NULL;
}

/*
 * reportxy (tw): Report cursor position as ESCy;xR
 */
static void reportxy(TEXTWIN* tw, int x, int y)
{
	char r[ESCBUFSIZE] = "\0";

	/* Convert from internal to screen coordinates */
	x++;
	y++;
	sendstr(tw, "\033");
	_ltoa(y, r, 10);
	sendstr(tw, r);
	sendstr(tw, ";");
	_ltoa(x, r, 10);
	sendstr(tw, r);
	sendstr(tw, "R");
}

/* Handle the control sequence ESC [...n.  */
static void vt100_esc_attr(TEXTWIN* tw, unsigned int c)
{
	int cx, cy, count, ei;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_attr: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy (tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy (tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy (tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy (tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
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
	case ';':		/* parameters for extended escape */
		pushescbuf(tw->escbuf, c);
		return;
	case '?':		/* start of screen/keyboard mode command */
		tw->output = vt100_esc_mode;
		return;
	case '@':		/* insert N characters */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count) {
			/* FIXME: Make insert_char take a fourth parameter
			   with the number of characters to insert.  */
			insert_char(tw, cx, cy);
			--count;
		}
		break;
	case 'A':		/* cursor up N */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		gotoxy(tw, cx, cy - count);
		break;
	case 'B':		/* cursor down N */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		gotoxy(tw, cx, cy + count);
		break;
	case 'C':		/* cursor right N */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		gotoxy(tw, cx + count, cy);
		break;
	case 'D':		/* cursor left N */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		gotoxy(tw, cx - count, cy);
		break;
	case 'H':		/* cursor to y,x */
		cy = popescbuf(tw, tw->escbuf);
		if (cy < 1)
			cy = 1;
		cx = popescbuf(tw, tw->escbuf);
		if (cx < 1)
			cx = 1;
		gotoxy(tw, cx - 1, tw->miny + cy - 1);
		break;
	case 'J':		/* clear screen parts */
		count = popescbuf(tw, tw->escbuf);
		if (count < 0)
			count = 0;

		switch (count) {
		case 0:	/* clear to end */
			clrfrom (tw, cx, cy, tw->maxx - 1, tw->maxy - 1);
			break;
		case 1:	/* clear from beginning */
			clrfrom (tw, 0, tw->miny, cx, cy);
			break;
		case 2:	/* clear all but do no move */
			clrfrom (tw, 0, tw->miny, 
				 tw->maxx - 1, tw->maxy - 1);
			break;
		}
		break;
	case 'K':		/* clear line parts */
		count = popescbuf(tw, tw->escbuf);
		if (count < 0)
			count = 0;
		switch (count) {
		case 0:	/* clear to end */
			clrfrom(tw, cx, cy, tw->maxx - 1, cy);
			break;
		case 1:	/* clear from beginning */
			clrfrom(tw, 0, cy, cx, cy);
			break;
		case 2:	/* clear all but do no move */
			clrfrom(tw, 0, cy, tw->maxx - 1, cy);
			break;
		}
		break;
	case 'L':		/* insert N lines */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count) {
			insert_line(tw, cy, tw->scroll_bottom);
			count--;
		}
		gotoxy(tw, 0, cy);
		break;
	case 'M':		/* delete N lines */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count) {
			delete_line(tw, cy, tw->scroll_bottom - 1);
			count--;
		}
		gotoxy(tw, 0, cy);
		break;
	case 'P':		/* delete N characters at cursor position */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count) {
			delete_char(tw, cx, cy);
			count--;
		}
		break;
	case 'R':		/* Cursor position response */
		break;
	case 'c':		/* device attributes (0c is same) */
		ei = popescbuf(tw, tw->escbuf);
		if (ei < 1) {
			/* 2 = base vt100 with advanced video option (AVO) */
			sendstr(tw, "\033[?1;2c");
		}
		break;
	case 'f':		/* cursor to y,x  same as H */
		vt100_esc_attr(tw, 'H');
		break;
	case 'g':		/* clear tabs */
		count = popescbuf(tw, tw->escbuf);
		if (count < 0)
			count = 0;
		switch (count) {
		case 0:	/* clear tab at cursor position */
			cleartab(tw, cx);
			break;
		case 3:	/* clear all tabs */
			clearalltabs(tw);
			break;
		}
		break;
	case 'h':		/* various keyboard & screen attributes */
		count = 0;
		do {
			ei = popescbuf(tw, tw->escbuf);
			if (count == 0 && ei == -1)
				ei = 0;
			switch (ei) {
			case 2:	/* keyboard locked */
				tw->output = keylocked;
				break;
			case 4:	/* insert mode */
				tw->term_flags |= FINSERT;
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
		do {
			ei = popescbuf(tw, tw->escbuf);
			if (count == 0 && ei == -1)
				ei = 0;
			switch (ei) {
			case 2:	/* keyboard unlocked */
				/* Unlock handled by keylockd() */
				break;
			case 4:	/* replace/overwrite mode */
				tw->term_flags &= ~FINSERT;
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
		do {
			ei = popescbuf (tw, tw->escbuf);
			if (ei == -1 && count == 0)
				ei = 0;
			switch (ei) {
			case 0:	/* All attributes off */
				tw->term_cattr &= 
					~(CE_BOLD | CE_LIGHT | CE_ITALIC | 
					  CE_UNDERLINE | CINVERSE);
				set_ansi_fg_color (tw, ANSI_DEFAULT + 48);
				set_ansi_bg_color (tw, ANSI_DEFAULT + 48);
				break;
			case 1:		/* bold on (x) */
				set_ansi_fg_color (tw, 'M' + 48);
				break;
			case 2:		/* light on (x) */
				set_ansi_fg_color (tw, 'N' + 48);
				break;
			case 4:	/* underline on */
				tw->term_cattr |= CE_UNDERLINE;
				break;
			case 5:	/* blink on - uses italics */
				tw->term_cattr |= CE_ITALIC;
				break;
			case 7:	/* reverse video (standout) on */
				tw->term_cattr |= CINVERSE;
				break;
			case 8: /* Concealed on.  */
				tw->term_cattr = 
					((tw->term_cattr & CBGCOL) << 4) |
					 (tw->term_cattr & 
					  ~(CE_ANSI_EFFECTS | 
					    CFGCOL));
				break;
			case 21:	/* bold off */
				tw->term_cattr &= ~CE_BOLD;
				break;
			case 22:	/* TW addition - light off */
				tw->term_cattr &= ~CE_LIGHT;
				break;
			case 24:	/* underline off */
				tw->term_cattr &= ~CE_UNDERLINE;
				break;
			case 25:	/* blink off - uses italics */
				tw->term_cattr &= ~CE_ITALIC;
				break;
			case 27:	/* reverse video (standout) off */
				tw->term_cattr &= ~CINVERSE;
				break;
			case 30:	/* white foreground (x) */
				set_ansi_fg_color (tw,  ANSI_BLACK + 48);
				break;
			case 31:	/* black foreground (x) */
				set_ansi_fg_color (tw,  ANSI_RED + 48);
				break;
			case 32:	/* red foreground (x) */
				set_ansi_fg_color (tw,  ANSI_GREEN + 48);
				break;
			case 33:	/* green foreground (x) */
				set_ansi_fg_color (tw,  ANSI_YELLOW + 48);
				break;
			case 34:	/* yellow foreground (x) */
				set_ansi_fg_color (tw,  ANSI_BLUE + 48);
				break;
			case 35:	/* blue foreground (x) */
				set_ansi_fg_color (tw,  ANSI_MAGENTA + 48);
				break;
			case 36:	/* magenta foreground (x) */
				set_ansi_fg_color (tw,  ANSI_CYAN + 48);
				break;
			case 37:	/* cyan foreground (x) */
				set_ansi_fg_color (tw,  ANSI_WHITE + 48);
				break;
			case 39:	/* cyan foreground (x) */
				set_ansi_fg_color (tw,  ANSI_DEFAULT + 48);
				break;
			case 40:	/* white background (x) */
				set_ansi_bg_color (tw,  ANSI_BLACK + 48);
				break;
			case 41:	/* black background (x) */
				set_ansi_bg_color (tw,  ANSI_RED + 48);
				break;
			case 42:	/* red background (x) */
				set_ansi_bg_color (tw,  ANSI_GREEN + 48);
				break;
			case 43:	/* green background (x) */
				set_ansi_bg_color (tw,  ANSI_YELLOW + 48);
				break;
			case 44:	/* yellow background (x) */
				set_ansi_bg_color (tw,  ANSI_BLUE + 48);
				break;
			case 45:	/* blue background (x) */
				set_ansi_bg_color (tw,  ANSI_MAGENTA + 48);
				break;
			case 46:	/* magenta background (x) */
				set_ansi_bg_color (tw,  ANSI_CYAN + 48);
				break;
			case 47:	/* cyan background (x) */
				set_ansi_bg_color (tw,  ANSI_WHITE + 48);
				break;
			case 49:	/* cyan background (x) */
				set_ansi_bg_color (tw,  ANSI_DEFAULT + 48);
				break;
			}
			count++;
		}
		while (ei >= 0);
		break;
	case 'n':
		count = popescbuf(tw, tw->escbuf);
		if (count < 0)
			count = 0;
		switch (count) {
		case 5:	/* status report */
			/* response is ESC[0n for terminal OK */
			sendstr(tw, "\033[0n");
			break;
		case 6:	/* cursor position report */
			reportxy(tw, cx, cy - tw->miny);
			break;
		}
		break;
	case 'p':
		if (tw->escbuf[0] == '!')
			soft_reset (tw);
		break;
	case 'r':		/* set size of scrolling region */
		ei = popescbuf(tw, tw->escbuf);
		if (ei < 1)	/* If not supplied ... */
			ei = 1;
		tw->scroll_top = tw->miny + ei - 1;
		ei = popescbuf(tw, tw->escbuf);
		if (ei < 1)	/* If not supplied ... */
			ei = NROWS (tw); /* ... set to size of screen */
		tw->scroll_bottom = tw->miny + ei - 1;
		break;
	case 'q':		/* keyboard light controls */
		/* Not yet implemented */
		break;
	case 'x':		/* return terminal parameters */
		/* Not yet implemented */
		break;
	case '}':		/* set protected fields */
		/* Not yet implemented */
		break;
	default:
#ifdef DEBUG_VT
		printf("vt100_esc_attr: unknown %c\n", c);
#endif
		break;
	}
	curs_on (tw);
	tw->output = vt100_putch;
}

/*
* vt100_esc_char_size (tw, c): handle the control sequence ESC #n
 */
static void 
vt100_esc_char_size (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_char_size: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy(tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy(tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
	case '1':		/* double height, single width top half chars */
		/* Not yet implemented */
		break;
	case '2':		/* double height, single width bottom half chars */
		/* Not yet implemented */
		break;
	case '3':		/* double height, double width top half chars */
		/* Not yet implemented */
		break;
	case '4':		/* double height, double width bottom half chars */
		/* Not yet implemented */
		break;
	case '5':		/* single height, single width */
		/* Not yet implemented */
		break;
	case '6':		/* single height, double width */
		/* Not yet implemented */
		break;
	/* DECALN */
	case '8':		/* Fill screen with uppercase E.  */
		fill_window (tw, 'E');
		break;
	default:
#ifdef DEBUG_VT
		printf("vt100_esc_char_size: unknown %c\n", c);
#endif
		break;
	}
	curs_on (tw);
	tw->output = vt100_putch;
}

/*
* vt100_esc_tests (tw, c): handle the control sequence ESC }n
 */
static void 
vt100_esc_tests (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_tests: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy(tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy(tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
	case '1':		/* fill screen with following character */
		/* Not yet implemented */
		break;
	case '2':		/* video attribute test */
		/* Not yet implemented */
		break;
	case '3':		/* character set test */
		/* Not yet implemented */
		break;
	default:
#ifdef DEBUG
		debug("vt100_esc_tests: unknown %c\n", c);
#endif
		break;
	}
	curs_on (tw);
	tw->output = vt100_putch;
}

/* Handle the control sequence ESC (n
 * Used to change G0 (standard) character set - should use appropriate font
 */
static void 
vt100_esc_std_char (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_std_char: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy(tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy(tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
	case 'A':		/* British */
	case 'B':		/* North American ASCII */
	case 'C':		/* Finnish */
	case 'E':		/* Danish/Norwegian */
	case 'H':		/* Swedish */
	case 'K':		/* German */
	case 'Q':		/* French Canadian */
	case 'R':		/* Flemish (French/Belgian) */
	case 'Y':		/* Italian */
	case 'Z':		/* Spanish */
		tw->term_cattr &= ~CACS;
		break;
	case '0':		/* Line Drawing */
		tw->term_cattr |= CACS;
		break;
	case '1':		/* Alternative Character */
	case '2':		/* Alternative Line Drawing */
	case '4':		/* Dutch */
	case '5':		/* Finnish */
	case '6':		/* Danish/Norwegian */
	case '7':		/* Swedish */
	case '=':		/* Swiss (French or German) */
		tw->term_cattr &= ~CACS;
		break;
	default:
#ifdef DEBUG_VT
		debug("vt100_esc_std_char: unknown %c\n", c);
#endif
		break;
	}
	curs_on (tw);
	tw->output = vt100_putch;
}

/* Handle the control sequence ESC )n
 * Used to change G1 (alternate) character set - should use appropriate font
 */
static void 
vt100_esc_alt_char (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_esc_alt_char: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy(tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy(tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
	case 'A':		/* British */
	case 'B':		/* North American ASCII */
	case 'C':		/* Finnish */
	case 'E':		/* Danish/Norwegian */
	case 'H':		/* Swedish */
	case 'K':		/* German */
	case 'Q':		/* French Canadian */
	case 'R':		/* Flemish (French/Belgian) */
	case 'Y':		/* Italian */
	case 'Z':		/* Spanish */
	case '0':		/* Line Drawing */
	case '1':		/* Alternative Character */
	case '2':		/* Alternative Line Drawing */
	case '4':		/* Dutch */
	case '5':		/* Finnish */
	case '6':		/* Danish/Norwegian */
	case '7':		/* Swedish */
	case '=':		/* Swiss (French or German) */
		tw->term_cattr &= ~CACS;
		break;
	default:
#ifdef DEBUG_VT
		debug("vt100_esc_alt_char: unknown %c\n", c);
#endif
		break;
	}
	curs_on (tw);
	tw->output = vt100_putch;
}

/*
* soft_reset (tw): do a soft reset
 */
static void soft_reset(TEXTWIN* tw)
{
	tw->escbuf[0] = '\0';	/* ESC <        - terminal into vt100 mode */
	vt100_putesc(tw, '<');	/* ESC [20l     - <return> sends lf */
	tw->escbuf[0] = '2';
	tw->escbuf[1] = '0';
	tw->escbuf[2] = '\0';
	vt100_esc_attr(tw, 'l');	/* ESC [?1l     - cursor key to cursor mode */
	/* ESC [?6l     - absolute/origin mode */
	/* ESC [?9l     - interface off */
	tw->escbuf[0] = '1';
	tw->escbuf[1] = ';';
	tw->escbuf[2] = '6';
	tw->escbuf[3] = ';';
	tw->escbuf[4] = '9';
	tw->escbuf[5] = '\0';
	vt100_esc_mode(tw, 'l');	/* ESC [r       - scrolling region to whole screen */
	tw->escbuf[0] = '\0';
	vt100_esc_attr(tw, 'r');	/* ESC q        - inverse video off */
	vt100_putesc(tw, 'q');	/* ESC (B       - standard character set is US ASCII */
	vt100_esc_std_char(tw, 'B');	/* ^O           - switch to standard character set */
	vt100_putch(tw, '\017');	/* ESC )0       - alternate character set is special/linedraw */
	vt100_esc_alt_char(tw, '0');	/* ESC >        - keypad in application mode */
	vt100_putesc(tw, '>');
}

static void 
full_reset (TEXTWIN* tw)
{
	soft_reset (tw);	

	clearalltabs (tw);	

	set_ansi_fg_color (tw, ANSI_DEFAULT + 48);
	set_ansi_bg_color (tw, ANSI_DEFAULT + 48);
	tw->term_flags |= FCURS;	

	tw->term_cattr &= ~CE_BOLD;
	tw->term_cattr &= ~CE_LIGHT;
	tw->term_cattr &= ~CE_ITALIC;
	tw->term_cattr &= ~CE_UNDERLINE;
	clear (tw);
	gotoxy (tw, 0, tw->miny);
}

/* Handle the control sequence ESC c
 * additions JC - handle extended escapes,
 * 		e.g. ESC[24;1H is cursor to x1, y24
 *
 * FIXME: Everything not supported by the original vt100 
 * should vanish from here.
 */
static void 
vt100_putesc (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

	curs_off (tw);
	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_putesc: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		gotoxy(tw, cx - 1, cy);
		curs_on (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy + 1);
		curs_on (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		gotoxy(tw, cx, cy - 2);
		curs_on (tw);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		gotoxy(tw, cx + 1, cy);
		curs_on (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		curs_on (tw);
		return;
	case '<':		/* switch to vt100 mode */
		switch (tw->vt_mode) {
		case MODE_VT52:	/* switch to vt100 mode */
			tw->vt_mode = MODE_VT100;
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
		tw->savex = tw->cx;
		tw->savey = tw->cy;
		tw->save_cattr = tw->term_cattr & (CATTRIBUTES | CACS);
		break;
	case '8':		/* restore cursor position */
		gotoxy(tw, tw->savex, tw->savey);
		tw->term_cattr = (tw->term_cattr & ~(CATTRIBUTES | CACS)) |
			tw->save_cattr;
		break;
	case 'A':		/* cursor up */
		gotoxy(tw, cx, cy - 1);
		break;
	case 'B':		/* cursor down */
		gotoxy(tw, cx, cy + 1);
		break;
	case 'C':		/* cursor right */
		gotoxy(tw, cx + 1, cy);
		break;
	case 'D':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* cursor left */
			gotoxy(tw, cx - 1, cy);
			break;
		case MODE_VT100:	/* cr */
			gotoxy(tw, cx, cy + 1);
			break;
		}
		break;
	case 'E':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* clear home */
			clear (tw);
			gotoxy (tw, 0, tw->miny);
			break;
		case MODE_VT100:	/* crlf */
			gotoxy (tw, 0, cy + 1);
			break;
		}
		break;
	case 'H':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* cursor home */
			gotoxy(tw, 0, tw->miny);
			break;
		case MODE_VT100:	/* set tab at cursor position */
			settab(tw, cx);
			break;
		}
		break;
	case 'I':		/* cursor up, insert line */
		if (cy == tw->miny)
			insert_line(tw, tw->miny, tw->maxy);
		else
			gotoxy(tw, cx, cy - 1);
		break;
	case 'J':		/* clear below cursor */
		clrfrom(tw, cx, cy, tw->maxx - 1, tw->maxy - 1);
		break;
	case 'K':		/* clear remainder of line */
		clrfrom(tw, cx, cy, tw->maxx - 1, cy);
		break;
	case 'L':		/* insert a line */
		insert_line(tw, cy, tw->maxy);
		gotoxy(tw, 0, cy);
		break;
	case 'M':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* delete line */
			delete_line(tw, cy, tw->maxy - 1);
			gotoxy(tw, 0, cy);
			break;
		case MODE_VT100:	/* reverse cr */
			if (cy == tw->scroll_top) {
				curs_off (tw);
				insert_line(tw, tw->scroll_top,
					    tw->scroll_bottom);
			} else
				gotoxy(tw, cx, cy - 1);
			break;
		}
		break;
	case 'R':		/* TW extension: set window size */
		tw->captsiz = 0;
		tw->output = capture;
		tw->callback = set_size;
		curs_on (tw);
		return;
	case 'S':		/* MW extension: set title bar */
		tw->captsiz = 0;
		tw->output = capture;
		tw->callback = set_title;
		curs_on (tw);
		return;
	case 'Y':		/* Cursor motion follows */
		tw->output = escy_putch;
		curs_on (tw);
		return;
	case 'Z':		/* return terminal id */
		tw->escbuf[0] = '\0';
		vt100_esc_attr(tw, 'c');
		break;
	case 'a':		/* MW extension: delete character */
		delete_char(tw, cx, cy);
		break;
	case 'b':		/* set foreground color */
#if 0
		tw->output = fgcol_putch;
		curs_on (tw);
		return;		/* `return' to avoid resetting tw->output */
#else
		break;
#endif
	case 'c':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* set background color */
#if 0
			tw->output = bgcol_putch;
			curs_on (tw);
			return;
#else
			break;
#endif
		case MODE_VT100:	/* reset */
			full_reset (tw);
			break;
		}
		break;
	case 'd':		/* clear to cursor position */
		clrfrom(tw, 0, tw->miny, cx, cy);
		break;
	case 'e':		/* enable cursor */
		tw->term_flags |= FCURS;
		break;
	case 'f':		/* cursor off */
		tw->term_flags &= ~FCURS;
		break;
	case 'h':		/* MW extension: enter insert mode */
		tw->term_flags |= FINSERT;
		break;
	case 'i':		/* MW extension: leave insert mode */
		tw->term_flags &= ~FINSERT;
		break;
	case 'j':		/* save cursor position */
		tw->savex = tw->cx;
		tw->savey = tw->cy;
		break;
	case 'k':		/* restore saved position */
		gotoxy(tw, tw->savex, tw->savey);
		break;
	case 'l':		/* clear line */
		clrline(tw, cy);
		gotoxy(tw, 0, cy);
		break;
	case 'o':		/* clear from start of line to cursor */
		clrfrom(tw, 0, cy, cx, cy);
		break;
	case 'p':		/* reverse video on */
		tw->term_cattr |= CINVERSE;
		break;
	case 'q':		/* reverse video off */
		tw->term_cattr &= ~CINVERSE;
		break;
	case 't':		/* backward compatibility for TW 1.x: set cursor timer */
		return;
	case 'v':		/* wrap on */
		tw->term_flags &= ~FNOAM;
		break;
	case 'w':		/* wrap off */
		tw->term_flags |= FNOAM;
		break;
	case 'y':		/* TW extension: set special effects */
		tw->output = seffect_putch;
		curs_on (tw);
		return;
	case 'z':		/* TW extension: clear special effects */
		tw->output = ceffect_putch;
		curs_on (tw);
		return;
	case '[':		/* extended escape [ - various attributes */
		tw->output = vt100_esc_attr;
		return;
	case '#':		/* extended escape # - ANSI character sizes */
		tw->output = vt100_esc_char_size;
		return;
	case '}':		/* extended escape } - tests */
		tw->output = vt100_esc_tests;
		return;
	case '(' /* extended escape ( - set G0 character set */ :
		tw->output = vt100_esc_std_char;
		return;
	case ')':		/* extended escape ( - set G1 character set */
		tw->output = vt100_esc_alt_char;
		return;
	case '*':		/* extended escape ( - set G2 character set */
		/* Not yet implemented */
		return;
	case '+':		/* extended escape ( - set G3 character set */
		/* Not yet implemented */
		return;
	case '!':		/* program a progammable key (local) */
		/* Not yet implemented */
		break;
	case '@':		/* program a progammable key (on-line) */
		/* Not yet implemented */
		break;
	case '%':		/* transmit progammable key contents */
		/* Not yet implemented */
		break;
	default:
#ifdef DEBUG_VT
		debug("vt100_putesc: unknown %c\n", c);
#endif
		break;
	}
	tw->output = vt100_putch;
	curs_on (tw);
}

/* Save character after ESC Y + char.  */
static void 
escy1_putch (TEXTWIN* tw, unsigned int c)
{
#ifdef DEBUG_VT
	debug ("escy1_putch: %c (%u)\n", c, c);
#endif
#ifdef DUMP
	dump (c);
#endif
	curs_off (tw);
	gotoxy(tw, c - ' ', tw->miny + tw->escy1 - ' ');
	tw->output = vt100_putch;
	curs_on (tw);
}

/* Save character after ESC Y.  */
static void 
escy_putch (TEXTWIN* tw, unsigned int c)
{
	curs_off (tw);
	tw->escy1 = c;
	tw->output = escy1_putch;
	curs_on (tw);
}

/* Put character C on screen V. This is the default
 * for when no escape, etc. is active.
 */
void 
vt100_putch (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

#ifdef DUMP
	dump(c);
#endif

	cx = tw->cx;
	cy = tw->cy;
	curs_off (tw);

	c &= 0x00ff;

	/* control characters */
	if (c < ' ') {
		switch (c) {
		/* FIXME: Handle newline mode!  */
		case '\r':	/* carriage return */
			gotoxy (tw, 0, cy);
			break;

		case '\n':	/* new line */
			if (cy == tw->scroll_bottom) {
				curs_off (tw);
				delete_line (tw, tw->scroll_top,
					     tw->scroll_bottom);
			} else {
				if (tw->cx >= tw->maxx)
					gotoxy (tw, 0, cy + 1);
				else
					gotoxy (tw, cx, cy + 1);
			}
			break;

		case '\b':	/* backspace */
			gotoxy (tw, cx - 1, cy);
			break;

		case '\007':	/* bell */
			if (con_fd) {
				/* xconout2: Bconout would result in 
				 * deadlock. 
				 */
				char ch = 7;
				Fwrite (con_fd, 1, &ch);
			} else
				Bconout (2, 7);
			break;

		case '\016':	/* ^N - switch to alternate char set */
			tw->term_cattr |= CACS;
			break;

		case '\017':	/* ^O - switch to standard char set */
			tw->term_cattr &= ~CACS;
			break;

		case '\033':	/* ESC */
			tw->output = vt100_putesc;
			tw->escbuf[0] = '\0';
			break;

		case '\t':	/* tab */
			gototab(tw, tw->cx, tw->cy);
			break;

		default:
#ifdef DEBUG_VT
			debug("vt100_putch: unknown %c\n", c);
#endif
			break;
		}
	} else {
		vt_quote_putch (tw, c);
	}
	curs_on (tw);
}
