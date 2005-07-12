/* Emulator for DEC VT100 series terminal for MiNT.
 * Copyright (C) 2001, Guido Flohr <guido@freemint.de>,
 * all rights reserved.
 */

#undef DEBUG_VT
#undef DUMP

#include <mintbind.h>
#include <support.h>
#include <string.h>

#include "textwin.h"
#include "vt.h"
#include "window.h"
#include "ansicol.h"
#include "console.h"

/* Local prototypes.  */
static void escy_putch (TEXTWIN* tw, unsigned int c);
static void vt100_putesc (TEXTWIN* tw, unsigned int c);
static void clearalltabs (TEXTWIN* tw);
static void fill_window (TEXTWIN* tw, unsigned int c);

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

#if 0
#define DEBUG_VT
#ifdef debug
# undef debug
#endif
#define debug printf
#endif

/* Move cursor in textwin TW from position (X|Y)
* to next tab position.  */
static void gototab(TEXTWIN* tw, int x, int y)
{
	TABLIST *tab;

	if (tw->tabs)
	{
		for (tab = tw->tabs; tab != NULL && tab->tabpos <= x;
		     tab = tab->nexttab);
		if (tab != NULL && tab->tabpos > x)
			x = tab->tabpos;
		gotoxy (tw, x, y - RELOFFSET (tw));
	}
	return;
}

/* Insert a space at position (X|Y).  Scroll rest of the line
 * to the right.  Discard characters pushed out of the screen.	*/
static void insert_char(TEXTWIN* tw, int x, int y)
{
	int i;
	unsigned char *twdata = tw->cdata[y];
	unsigned long *twcflag = tw->cflags[y];

	if (x < 1 || (NCOLS (tw) - x < 1))
		return;

	memmove (twdata + x, twdata + x - 1, NCOLS (tw) - x);
	for (i = NCOLS (tw) - 1; i > x; --i)
	{
		twcflag[i] = twcflag[i - 1] | CDIRTY;
	}
	twdata[x] = ' ';
	twcflag[x] = CDIRTY | (tw->curr_cattr & (CBGCOL | CFGCOL));
	tw->dirty[y] |= SOMEDIRTY;
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

	if (i < CAPTURESIZE || c == 0)
	{
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
	tw->curr_cattr |= ((c & 0x1f) << 8);
	tw->output = vt100_putch;
}

/* clear special effects */
static void
ceffect_putch(TEXTWIN* tw, unsigned int c)
{
	tw->curr_cattr &= ~((c & 0x1f) << 8);
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
	unsigned long new_attribute = tw->curr_cattr &
		(CBGCOL | CFGCOL);
	int cols = NCOLS (tw);

	for (row = tw->miny; row < tw->maxy; ++row) {
		memset (tw->cdata[row], c, cols);
		memulset (tw->cflags[row], new_attribute, cols);
	}
	memset (tw->dirty + tw->miny, ALLDIRTY, NROWS (tw));
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

/* Handle control sequence ESC [?...n.	*/
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
		cub1 (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		cud1 (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		cuu (tw, 2);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		cuf1 (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		return;
	case 'J':		/* Erase in Display (DECSED).  */
	case 'K':		/* Erase in Line (DECSEL).  */
		/* Not yet implemented.  */
		count = 0;
		do
		{
			ei = popescbuf (tw, tw->escbuf);
			++count;
		} while (ei >= 0);
		break;
	case 'c':		/* Extended private modes.	*/
		count = 0;
		do {
			ei = popescbuf (tw, tw->escbuf);
			if (count == 0 && ei == -1)
				ei = 0;
			switch (ei) {
			case 0:
				tw->curr_tflags &= ~TCURS_VVIS;
				break;
			case 8:
				tw->curr_tflags |= (TCURS_ON | TCURS_VVIS);
				break;
			}
			++count;
		} while (ei >= 0);
		break;
	case 'h':		/* mode set */
		count = 0;
		do {
			ei = popescbuf(tw, tw->escbuf);
			if (count == 0 && ei == -1)
				ei = 0;
			switch (ei) {
			case 1:	/* Application Cursor Keys (DECCKM).  */
				tw->curs_mode = CURSOR_TRANSMIT;
				break;
			case 2: /* Designate USASCII for character 
				   sets G0-G3 (DECANM), and set VT100
				   mode.  */
				memcpy (tw->gsets, "BBBB", 
					sizeof tw->gsets);
				break;
			case 3:	/* 132 column mode (DECCOLM).  */
				/* The original VT100
				   also did a clear/home on that.  */
				if (tw->curr_tflags & TDECCOLM) {
					resize_textwin (tw, 132, 
							NROWS (tw),
							SCROLLBACK (tw));
					clear (tw);
					gotoxy (tw, 0, 0);
				}
				break;
			case 4:	/* Smooth (slow) scroll (DECSCLM).  */
				/* Not supported since version 2.0.  */
				break;
			case 5:	/* Reverse Video (DECSCNM).  */
				inverse_video (tw, 1);
				break;
			case 6:	/* Origin Mode (DECOM).  */
				tw->curr_tflags |= TORIGIN;
				gotoxy (tw, 0, 0);
				break;
			case 7:	/* Wraparound Mode (DECAWM).  */
				tw->curr_tflags |= TWRAPAROUND;
				break;
			case 8:	/* Auto-repeat Keys (DECARM).  */
				/* Not yet implemented */
				break;
			case 9:	/* Send Mouse X & Y on button press.  */
				/* Not yet implemented */
				break;
			case 18: /* Print Form Feed (DECPFF).  */
				/* Not yet implemented */
				break;				
			case 19: /* Set print extent to full screen 
				    (DECPEX).  */
				/* Not yet implemented */
				break;				
			case 25: /* Show Cursor (DECTCEM).  */
				tw->curr_tflags |= TCURS_ON;
				break;
			case 35: /* Enable font-shifting functions
				    (rxvt).  */
				/* Not yet implemented */
				break;
			case 38: /* Enter Tektronix Mode (DECTEK).  */
				/* Beurk! :-(  */
				break;
			case 40: /* Allow 80 -> 132 Mode (xterm?).  */
				tw->curr_tflags |= TDECCOLM;
				break;
			case 41: /* more(1) fix (xterm?).  */
				/* Dunno.  */
				break;
			case 42: /* Enable Nation Replacement Character
				    sets (DECNRCM).  */
				/* Dunno.  */
				break;
			case 44: /* Turn On Margin Bell.  */
				/* Would be funny ...  */
				break;
			case 45: /* Reverse-wraparound Mode (xterm?).  */
				/* Dunno.  */
				break;
			case 47: /* Use Alternate Screen Buffer.  */
				/* Not yet implemented.  */
				break;
			case 66: /* Application Keypad (DECNKM).  */
			case 67: /* Backarrow key sends delete 
				    (DECBKM).  */
				/* Not yet implemented.  */
			case 1001: /* Use Hilite Mouse Tracking.  */
			case 1002: /* Use Cell Motion Mouse Tracking.  */
			case 1003: /* Use All Motion Mouse Tracking.  */
			case 1010: /* Scroll to bottom on tty output
				      (rxvt).  */
			case 1011: /* Scroll to bottom on key press
				      (rxvt).  */
			case 1035: /* Enable special modifiers for Alt
				      and NumLock keys.  */
			case 1036: /* Send ESC when Meta modifies a
				      key.  */
			case 1037: /* Send DEL from the editing-keypad
				      Delete key.  */
			case 1047: /* Use Alternate Screen Buffer.  */
			case 1048: /* Save cursor as in DECSC.  */
			case 1049: /* Save cursor as in DECSC and
				      use Alternate Screen Buffer, 
				      clearing it first.  */
			case 1051: /* Set Sun function-key mode.  */
			case 1052: /* Set HP function-key mode.  */
			case 1060: /* Set legacy keyboard emulation 
				      (X11R6).  */
			case 1061: /* Set Sun/PC keyboard emulation of 
				      VT220 keyboard.  */
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
				tw->scroll_bottom = tw->maxy;
				break;
			case 3:	/* DECCOLM - 80 column mode.  The original VT100
				   also did a clear/home on that.  */
				if (tw->curr_tflags & TDECCOLM) {
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
				inverse_video (tw, 0);
				break;
			case 6:	/* relative mode */
				tw->curr_tflags &= ~TORIGIN;
				gotoxy (tw, 0, 0);
				break;
			case 7:	/* wrap off */
				tw->curr_tflags &= ~TWRAPAROUND;
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
				tw->curr_tflags &= ~TCURS_ON;
				break;
			case 40:	/* Reset column mode.  */
				tw->curr_tflags &= ~TDECCOLM;
				break;
			/* FIXME: Insert set modes as implemented.  */
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
	tw->output = vt100_putch;
}

/*
* cleartab(tw, x): clear tab at position x
 */

static void cleartab(TEXTWIN* tw, int x)
{
	TABLIST *tab, *oldtab;

	oldtab = tab = tw->tabs;
	while (tab != NULL && tab->tabpos < x)
	{
		oldtab = tab;
		tab = tab->nexttab;
	}
	if (tab != NULL && tab->tabpos == x)
	{
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

/* Answer back.  Send the answer back string.  */
static
void answerback (TEXTWIN* tw)
{
	sendstr (tw, tw100_env);
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
	unsigned long param1, param2, param3;

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
		cub1 (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		cud1 (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		cuu (tw, 2);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		cuf1 (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		return;
	case '!':		/* Maybe Soft terminal reset (DECSTR)
				   if followed by p.  */
		pushescbuf (tw->escbuf, c);
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
		pushescbuf (tw->escbuf, c);
		return;
	case '?':		/* DEC Private Mode Set (DECSET).  */
		tw->output = vt100_esc_mode;
		return;
	case '@':		/* Insert Ps (Blank) Character(s)
				   (default = 1) (ICH).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count) {
			/* FIXME: Make insert_char take a fourth parameter
			   with the number of characters to insert.	*/
			insert_char (tw, cx, cy);
			--count;
		}
		break;
	case 'A':		/* Cursor Up Ps Times (default = 1)
				   (CUU).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 1)
			count = 1;
		cuu (tw, count);
		break;
	case 'B':		/* Cursor Down Ps Times (default = 1)
				   (CUD).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 1)
			count = 1;
		cud (tw, count);
		break;
	case 'C':		/* Cursor Forward Ps Times (default = 1)
				   (CUF).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 1)
			count = 1;
		cuf (tw, count);
		break;
	case 'D':		/* Cursor Backward Ps Times (default = 1)
				   (CUD).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 1)
			count = 1;
		cub (tw, count);
		break;
	case 'H':		/* Cursor Position [row;column]
				   (default = [1,1]) (CUP).  */
		cy = popescbuf (tw, tw->escbuf);
		if (cy < 1)
			cy = 1;
		cx = popescbuf(tw, tw->escbuf);
		if (cx < 1)
			cx = 1;
		gotoxy (tw, cx - 1, tw->miny + cy - 1);
		break;
	case 'J':		/* Erase in Display (ED).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 0)
			count = 0;

		switch (count) {
		case 0:	/* Erase Below (default).  */
			clrfrom (tw, cx, cy, tw->maxx - 1, tw->maxy - 1);
			break;
		case 1:	/* Erase Above.  */
			clrfrom (tw, 0, tw->miny, cx, cy);
			break;
		case 2:	/* Erase All.  */
			clrfrom (tw, 0, tw->miny,
				 tw->maxx - 1, tw->maxy - 1);
			break;
		case 3: /* Erase Saved Lines (xterm).  */
			clrfrom (tw, 0, 0, tw->miny, tw->maxx - 1);
			break;
		}
		break;
	case 'K':		/* Erase in Line (EL).  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 0)
			count = 0;
		switch (count) {
		case 0:	/* Erase to Right (default).  */
			clrfrom (tw, cx, cy, tw->maxx - 1, cy);
			break;
		case 1:	/* Erase to Left.  */
			clrfrom (tw, 0, cy, cx, cy);
			break;
		case 2:	/* Erase All.  */
			clrfrom (tw, 0, cy, tw->maxx - 1, cy);
			break;
		}
		break;
	case 'L':		/* Insert Ps line(s) (default = 1)
				   (IL).  */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count > 0) {
			insert_line (tw);
			--count;
		}
		tw->do_wrap = 0;
		break;
	case 'M':		/* Delete Ps line(s) (default = 1)
				   (DL).  */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count > 0) {
			delete_line (tw);
			--count;
		}
		break;
	case 'P':		/* Delete Ps Character(s) (default = 1)
				   (DCH).  */
		count = popescbuf(tw, tw->escbuf);
		if (count < 1)
			count = 1;
		while (count) {
			delete_char(tw, cx, cy);
			count--;
		}
		break;
	case 'R':		/* ??? Cursor position response */
		break;
	case 'S':		/* Scroll up Ps (line(s) (default = 1)
				   (SU).  */
	case 'T':		/* Scroll down Ps (line(s) (default = 1)
				   (SD).  */
		/* Not yet implemented.  */
		/* If CSI Ps ; Ps ; Ps ; Ps ; Ps T then mouse 
		   tracking is requested.  */
		break;
	case 'X':		/* Erase Ps character(s) (default = 1)
				   (ECH).  */
		count = popescbuf (tw, tw->escbuf);
		ei = tw->cx;
		do {
			paint (tw, ' ');
			--count;
			++tw->cx;
		} while (tw->cx < tw->maxx && count > 1);
		tw->cx = ei;
		break;
	case 'Z':		/* Cursor Backward Tabulation Ps tab 
				   stops (default = 1) (CBT).  */
		/* Not yet implemented.  */
		break;
	case '`':		/* Character Position Absolute [column]
				   (default = [row,1]) (HPA).  */
		count = popescbuf (tw, tw->escbuf);
		gotoxy (tw, count, tw->cy - RELOFFSET (tw));
		break;
	case 'b':		/* Repeat the preceding graphic character
				   Ps times (REP).  */
		/* Not yet implemented.  */
		count = popescbuf (tw, tw->escbuf);
		do {
			ei = popescbuf (tw, tw->escbuf);
			++count;
		} while (ei >= 0);
		break;
	case 'c':		/* Send Device Attributes 
				   (Primary DA).  */
		ei = popescbuf (tw, tw->escbuf);
		if (ei < 1) {
			/* 2 = base vt100 with advanced video 
			   option (AVO) */
			sendstr (tw, "\033[?1;2c");
			/* FIXME: Should be configurable.  */
		}
		/* FIXME: CSI > Ps c requests Secondary DA.  Insert
		   into caller ... */
		break;
	case 'd':		/* Line Position Absolute [row]
				   (default = [1,column]) (VPA).  */
		/* FIXME: Obey origin mode or not?  */
		count = popescbuf (tw, tw->escbuf);
		if (count < 1)
			count = 1;
		gotoxy (tw, count, tw->cy - RELOFFSET (tw));
		break;
	case 'f':		/* Horizontal and Vertical Position
				   [row;column] (default = [1,1]) 
				   (HVP).  */
		vt100_esc_attr (tw, 'H');
		break;
	case 'g':		/* Tab Clear (TBC).  */
		count = popescbuf(tw, tw->escbuf);
		if (count < 0)
			count = 0;
		switch (count) {
		case 0:	/* Clear Current Column (default).  */
			cleartab(tw, cx);
			break;
		case 3:	/* Clear All.  */
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
				tw->curr_tflags |= TINSERT;
				break;
			case 20:	/* <return> sends crlf */
				/* Not yet implemented */
				break;
			case 34:
				tw->curr_tflags &= ~TCURS_VVIS;
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
				tw->curr_tflags &= ~TINSERT;
				break;
			case 20:	/* <return> sends lf */
				/* Not yet implemented */
				break;
			case 34:
				tw->curr_tflags |= (TCURS_ON | TCURS_VVIS);
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
			case 0:	/* All attributes off.  
				   FIXME: Is this too much?  */
				tw->curr_cattr &=
					~(CE_BOLD | CE_LIGHT | 
					  CE_ITALIC | CE_UNDERLINE | 
					  CINVERSE | CPROTECTED |
					  CINVISIBLE);
				original_colors (tw);
				break;
			case 1:		/* bold on (x) */
				set_ansi_fg_color (tw, 'M' + 48);
				break;
			case 2:		/* light on (x) */
				set_ansi_fg_color (tw, 'N' + 48);
				break;
			case 4:	/* underline on */
				tw->curr_cattr |= CE_UNDERLINE;
				break;
			case 5:	/* blink on - uses italics */
				tw->curr_cattr |= CE_ITALIC;
				break;
			case 7:	/* reverse video (standout) on */
				tw->curr_cattr |= CINVERSE;
				break;
			case 8: /* Concealed on.  */
				tw->curr_cattr =
					((tw->curr_cattr & CBGCOL) << 4) |
					 (tw->curr_cattr &
					  ~(CE_ANSI_EFFECTS |
						 CFGCOL));
				break;
			case 21:	/* bold off */
				tw->curr_cattr &= ~CE_BOLD;
				break;
			case 22:	/* TW addition - light off */
				tw->curr_cattr &= ~CE_LIGHT;
				break;
			case 24:	/* underline off */
				tw->curr_cattr &= ~CE_UNDERLINE;
				break;
			case 25:	/* blink off - uses italics */
				tw->curr_cattr &= ~CE_ITALIC;
				break;
			case 27:	/* reverse video (standout) off */
				tw->curr_cattr &= ~CINVERSE;
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
		ei = popescbuf (tw, tw->escbuf);
		if (tw->escbuf[0] == '!')		/* Soft terminal reset (DECSTR).  */
			vt_reset (tw, FALSE, FALSE);
		else if (tw->escbuf[0] == '\000')	/* Full terminal reset (RSI).  */
			vt_reset (tw, TRUE, TRUE);
		break;
	case 'r':		/* set size of scrolling region */
		param1 = popescbuf (tw, tw->escbuf);
		param2 = popescbuf (tw, tw->escbuf);

		if (param1 < 1)
			param1 = 1;
		if (param2 == -1 || param2 > NROWS (tw) || param2 == 0)
			param2 = NROWS (tw);
		if (param2 > param1) {
			tw->scroll_top = tw->miny + param1 - 1;
			tw->scroll_bottom = tw->miny + param2;
			gotoxy (tw, 0, 0);
		}
		break;
	case 't':		/* window modification */
		param1 = popescbuf (tw, tw->escbuf);
		param2 = popescbuf (tw, tw->escbuf);
		param3 = popescbuf (tw, tw->escbuf);
debug ("window modification: %lu %lu %lu\n", param1, param2, param3);

		switch (param1) {
			case 1:  /* De-Iconify.  */
				(*(tw->win)).uniconify (tw->win,
						         tw->win->prev.g_x,
						         tw->win->prev.g_y,
						         tw->win->prev.g_w,
						         tw->win->prev.g_h);
				break;
			case 2:  /* Iconify.  */
				(*(tw->win)).iconify (tw->win, 
						       -1, -1, -1, -1);
				break;
			case 3:  /* Position.  */
				break;
			case 5:  /* Raise window.  */
				break; /* Not yet implemented.  */
			case 6:  /* Lower window.  */
				break; /* Not yet implemented.  */
			case 4:  /* Size in pixels.  */
				/*
				 * If we have too small but non-zero values
				 * for width or height, round them up to
				 * one character.
				 */
				if (param2 > 0 && param2 < tw->cheight)
					param2 = tw->cheight;
				if (param3 > 0 && param3 < tw->cmaxwidth)
					param3 = tw->cmaxwidth;
				param2 /= tw->cheight;
				param3 /= tw->cmaxwidth;
				/* FALLTHRU */
			case 8:  /* Size in characters.  */
				if (param2 == 0 || param2 > 768) {
					param2 = tw->win->full_work.g_h;
					param2 /= tw->cheight;
				}
				if (param3 == 0 || param3 > 1024) {
					param3 = tw->win->full_work.g_w;
					param3 /= tw->cmaxwidth;
				}
debug ("resize to %d x %d (%d)\n", param3, param2, SCROLLBACK (tw));
				resize_textwin (tw, param3, param2, 
						SCROLLBACK (tw));
				refresh_textwin (tw, 1);
				break;
		}
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
		debug ("vt100_esc_attr: unknown %c\n", c);
#endif
		break;
	}
	tw->output = vt100_putch;
}

/* vt100_esc_ansi (tw, c): handle control sequence ESC ' ' (SP).  */
static void
vt100_esc_ansi (TEXTWIN* tw, unsigned int c)
{
#ifdef DEBUG_VT
	debug ("vt100_esc_ansi: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump (c);
#endif
	switch (c) {
	case 'F':		/* 7-bit controls (S7C1T).  */
		/* Not yet implemented.  */
		break;
	case 'G':		/* 8-bit controls (S8CIT).  */
		/* Not yet implemented.  */
		break;
	case 'L':		/* Set ANSI conformance level 1 
				   (vt100, 7-bit controls).  */
		/* Not yet implemented.  */
		break;
	case 'M':		/* Set ANSI conformance level 2 
				   (vt200).  */
		/* Not yet implemented.  */
		break;
	case 'N':		/* Set ANSI conformance level 3 
				   (vt300).  */
		/* Not yet implemented.  */
		break;
	default:
#ifdef DEBUG_VT
		debug ("vt100_esc_ansi: unknown %c\n", c);
#endif
		break;
	}
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
		cub1 (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		cud1 (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		cuu (tw, 2);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		cuf1 (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		return;
	case '1':		/* double height, single width top half chars */
		/* ??? Not yet implemented */
		break;
	case '2':		/* double height, single width bottom half chars */
		/* ??? Not yet implemented */
		break;
	case '3':		/* DEC double-height line, top half
				   (DECDHL).  */
		/* Not yet implemented */
		break;
	case '4':		/* DEC double-height line, bottom half
				   (DECDHL).  */
		/* Not yet implemented */
		break;
	case '5':		/* DEC single-width line (DECSWL).  */
		/* Not yet implemented */
		break;
	case '6':		/* DEC single-width line (DECSWL).  */
		/* Not yet implemented */
		break;
	case '8':		/* DEC Screen Alignment Test (DECALN).  */
		fill_window (tw, 'E');
		break;
	default:
#ifdef DEBUG_VT
		printf("vt100_esc_char_size: unknown %c\n", c);
#endif
		break;
	}
	tw->output = vt100_putch;
}

/* vt100_esc_codeset (tw, c): handle control sequence ESC '%' (SP).  */
static void
vt100_esc_codeset (TEXTWIN* tw, unsigned int c)
{
#ifdef DEBUG_VT
	debug ("vt100_esc_codeset: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump (c);
#endif
	switch (c) {
	case '@':		/* Select default character set,
				   ISO-8859-1 (ISO 2022).  */
		tw->cfg->char_tab = TAB_ISO;
		break;
	case 'G':		/* Select UTF-8 character set
				   (ISO 2022).  */
		tw->cfg->char_tab = TAB_ISO;  /* Better than Atari.  */
		break;
	default:
#ifdef DEBUG_VT
		debug ("vt100_esc_codeset: unknown %c\n", c);
#endif
		break;
	}
	tw->output = vt100_putch;
}

/* vt100_esc_charset (tw, c): handle control sequences ESC '(',
   ESC ')', ESC '*', or ESC '+'.  */  
static void
vt100_esc_charset (TEXTWIN* tw, unsigned int c)
{
	int g;
	
#ifdef DEBUG_VT
	debug ("vt100_esc_codeset: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump (c);
#endif
	switch (c) {
	case '\010':		/* ANSI spec - ^H in middle of ESC (yuck!) */
		cub1 (tw);
		return;
	case '\012':		/* ANSI spec - ^J in middle of ESC (yuck!) */
		cud1 (tw);
		return;
	case '\013':		/* ANSI spec - ^K in middle of ESC (yuck!) */
		cuu (tw, 2);
		return;
	case '\014':		/* ANSI spec - ^L in middle of ESC (yuck!) */
		cuf1 (tw);
		return;
	case '\033':		/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		return;
	case '0':		/* DEC Special Character and Line
				   Drawing Set.  */
	case 'A':		/* United Kingdom (UK).  */
	case 'B':		/* United States (USASCII).  */
	case '4':		/* Dutch.  */
	case 'C':		/* Finnish.  */
	case '5':		/* Finnish.  */
	case 'R':		/* French.  */
	case 'Q':		/* French Canadian.  */
	case 'K':		/* German.  */
	case 'Y':		/* Italian.  */
	case 'E':		/* Norwegian/Danish.  */
	case '6':		/* Norwegian/Danish.  */
	case 'Z':		/* Spanish.  */
	case 'H':		/* Swedish.  */
	case '7':		/* Spanish.  */
	case '=':		/* Swiss.  */
		break;
	default:
#ifdef DEBUG_VT
		debug ("vt100_esc_codeset: unknown %c\n", c);
#endif
		tw->output = vt100_putch;
		return;
	}

	/* Get first designator.  */
	g = tw->escbuf[0];
	tw->curr_tflags &= ~TCHARSET_MASK;
	switch (g) {
	case '(':
		g = 0;
		break;
	case ')':
		g = 1;
		break;
	case '*':
		g = 2;
		break;
	case '+':
		g = 3;
		break;
	default:
#ifdef DEBUG_VT
		debug ("vt100_esc_codeset: unknown Gn %c\n", c);
#endif
		tw->output = vt100_putch;
		return;
	}
	
	tw->gsets[g] = c;
	tw->output = vt100_putch;
}

/* Handle the control sequence ESC c
 * additions JC - handle extended escapes,
 * 		e.g. ESC[24;1H is cursor to x1, y24
 *
 * FIXME: Everything not supported by xterm and friends
 * should vanish from here.
 */
static void
vt100_putesc (TEXTWIN* tw, unsigned int c)
{
	int cx, cy;

	cx = tw->cx;
	cy = tw->cy;

#ifdef DEBUG_VT
	debug("vt100_putesc: %c (%d)\n", c, c);
#endif
#ifdef DUMP
	dump(c);
#endif
	switch (c) {
	case '\010':	/* ANSI spec - ^H in middle of ESC (yuck!) */
		cub1 (tw);
		return;
	case '\012':	/* ANSI spec - ^J in middle of ESC (yuck!) */
		cud1 (tw);
		return;
	case '\013':	/* ANSI spec - ^K in middle of ESC (yuck!) */
		cuu (tw, 2);
		return;
	case '\014':	/* ANSI spec - ^L in middle of ESC (yuck!) */
		cuf1 (tw);
		return;
	case '\033':	/* ESC - restart sequence */
		tw->output = vt100_putesc;
		tw->escbuf[0] = '\0';
		return;
	case ' ':	/* Set ANSI levels.  */
		tw->output = vt100_esc_ansi;
		return;
	case '<':	/* switch to vt100 mode ??? */
		switch (tw->vt_mode) {
		case MODE_VT52:	/* switch to vt100 mode */
			tw->vt_mode = MODE_VT100;
			break;
		case MODE_VT100:	/* Nothing to do */
			break;
		}
		break;
	case '#':		/* DEC character sizes.  */
		tw->output = vt100_esc_char_size;
		return;
	case '%':		/* ISO codeset selection.  */
		tw->output = vt100_esc_codeset;
		return;
	case '(': 		/* Designate G0 Character 
				   Set (ISO 2022).  */
	case ')': 		/* Designate G0 Character 
				   Set (ISO 2022).  */
	case '*': 		/* Designate G0 Character 
				   Set (ISO 2022).  */
	case '+': 		/* Designate G0 Character 
				   Set (ISO 2022).  */
		tw->escbuf[0] = '\000';
		pushescbuf (tw->escbuf, c);
		tw->output = vt100_esc_charset;
		return;
	case '7':		/* Save Cursor (DECSC).  */
		save_cursor (tw);
		break;
	case '8':		/* Restore Cursor (DECRC).  */
		restore_cursor (tw);
		break;
	case '=':		/* Application Keypad (DECPAM).  */
		/* Not yet implemented.  */
		break;
	case '>':		/* Normal Keypad (DECPNM).  */
		/* Not yet implemented.  */
		break;
	case 'A':		/* cursor up */
		cuu1 (tw);
		break;
	case 'B':		/* cursor down */
		cud1 (tw);
		break;
	case 'C':		/* cursor right */
		cuf1 (tw);
		break;
	case 'D':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* cursor left */
			cub1 (tw);
			break;
		case MODE_VT100:	/* Index (IND: 0x84).  */
			cud1 (tw);
			break;
		}
		break;
	case 'E':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* clear home */
			clear (tw);
			gotoxy (tw, 0, 0);
			break;
		case MODE_VT100:	/* Next Line (NEL: 0x85).  */
			gotoxy (tw, 0, cy + 1 + RELOFFSET (tw));
			break;
		}
		break;
	case 'F':			/* Cursor to lower left corner
					   of the screen (for buggy
					   HP applications).  FIXME:
					   Should be configurable.  */
		gotoxy (tw, 0, tw->maxy - 1);
		break;
	case 'H':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* cursor home */
			gotoxy (tw, 0, 0);
			break;
		case MODE_VT100:	/* Tab Set (HTS: 0x88).  */
			settab(tw, cx);
			break;
		}
		break;
	case 'I':		/* cursor up, insert line */
		reverse_cr (tw);
		break;
	case 'J':		/* clear below cursor */
		clrfrom(tw, cx, cy, tw->maxx - 1, tw->maxy - 1);
		break;
	case 'K':		/* clear remainder of line */
		clrfrom(tw, cx, cy, tw->maxx - 1, cy);
		break;
	case 'L':		/* insert a line */
		insert_line (tw);
		break;
	case 'M':
		switch (tw->vt_mode) {
		case MODE_VT52:	/* delete line */
			delete_line (tw);
			break;
		case MODE_VT100:	/* Reverse Index (RI: 0x8d).  */
			reverse_cr (tw);
			break;
		}
		break;
	case 'N':			/* Single Shift Select of G2
					   Character Set (SS2: 0x8e):
					   affects next character
					   only.  */
		/* Not yet implemented.  */
		break;
	case 'O':			/* Single Shift Select of G3
					   Character Set (SS3: 0x8f):
					   affects next character
					   only.  */
		/* Not yet implemented.  */
		break;
	case 'P':			/* Device Control String
					   (DCS: 0x90).  */
		/* Not yet implemented.  */
		/* FIXME: Eat following characters until ST.  */
		break;		
	case 'R':		/* TW extension: set window size */
		tw->captsiz = 0;
		tw->output = capture;
		tw->callback = set_size;
		return;
	case 'S':		/* MW extension: set title bar */
		tw->captsiz = 0;
		tw->output = capture;
		tw->callback = set_title;
		return;
	case 'V':			/* Start of Guarded Area
					   (SPA: 0x96).  */
		/* Not yet implemented.  */
		break;
	case 'W':			/* END of Guarded Area
					   (EPA: 0x97).  */
		/* Not yet implemented.  */
		break;					  
	case 'X':			/* Start of String
					   (SOS: 0x98).  */
		/* Not yet implemented.  */
		break;
	case 'Y':		/* cursor motion follows */
		tw->output = escy_putch;
		return;
	case 'Z':		/* Return Terminal ID (DECID: 0x91a).
				   Obsolete form of CSI c (DA).  */
		tw->escbuf[0] = '\0';
		vt100_esc_attr(tw, 'c');
		break;
	case 'a':		/* MW extension: delete character */
		delete_char(tw, cx, cy);
		break;
	case 'c':		/* Full Reset (RIS).  */
		vt_reset (tw, TRUE, TRUE);
		break;
	case 'd':		/* clear to cursor position */
		clrfrom(tw, 0, tw->miny, cx, cy);
		break;
	case 'e':		/* enable cursor */
		tw->curr_tflags |= TCURS_ON;
		break;
	case 'f':		/* cursor off */
		tw->curr_tflags &= ~TCURS_ON;
		break;
	case 'j':		/* save cursor position */
		tw->saved_x = tw->cx;
		tw->saved_y = tw->cy;
		break;
	case 'k':		/* restore saved position */
		/* FIXME: Obey origin mode!!! */
		gotoxy (tw, tw->saved_x, tw->saved_y);
		break;
	case 'l':		/* HP memory lock.  */
		if (tw->scroll_bottom > tw->cy)
			tw->scroll_top = tw->cy;
		break;
	case 'm':		/* HP memory unlock.  */
		tw->scroll_top = tw->miny;
		break;
	case 'n':		/* Invoke the G2 Character Set (LS2).  */
		tw->curr_tflags = (tw->curr_tflags & ~TCHARSET_MASK) | 2;
		break;
	case 'o':		/* Invoke the G3 Character Set (LS3).  */
		tw->curr_tflags = (tw->curr_tflags & ~TCHARSET_MASK) | 3;
		break;
	case 'p':		/* reverse video on */
		tw->curr_cattr |= CINVERSE;
		break;
	case 'q':		/* reverse video off */
		tw->curr_cattr &= ~CINVERSE;
		break;
	case 't':		/* backward compatibility for TW 1.x: set cursor timer */
		return;
	case 'v':		/* wrap on */
		tw->curr_tflags |= TWRAPAROUND;
		break;
	case 'w':		/* wrap off */
		tw->curr_tflags &= TWRAPAROUND;
		break;
	case 'y':		/* TW extension: set special effects */
		tw->output = seffect_putch;
		return;
	case 'z':		/* TW extension: clear special effects */
		tw->output = ceffect_putch;
		return;
	case '[':		/* Control Sequence Introducer
				   (CSI: 0x9b).  */
		tw->output = vt100_esc_attr;
		return;
	case '\\':		/* String Terminator (ST: 0x9c).  */
		/* Not yet implemented.  */
		break;
	case ']':		/* Operating System Command 
				   (OSC: 0x9d).  */
		/* Not yet implemented.  */
		break;
	case '^':		/* Privacy Message (PM: 0x9e).  */
		/* Not yet implemented.  */
		break;
	case '_':		/* Application Program Command 
				   (APC: 0x9f).  */
		/* Not yet implemented.  */
		/* FIXME: Eat following characters until ST.  */
		break;
	case '|':		/* Invoke the G3 Character Set as GR
				   (LS3R).  Has no visible effect
				   in xterm, we follow happily.  */
	case '}':		/* Invoke the G2 Character Set as GR
				   (LS2R).  Has no visible effect
				   in xterm, we follow happily.  */
	case '~':		/* Invoke the G1 Character Set as GR
				   (LS1R).  Has no visible effect
				   in xterm, we follow happily.  */
		break;
	case '@':		/* program a progammable key (on-line) */
		/* Not yet implemented */
		break;
	default:
#ifdef DEBUG_VT
		debug("vt100_putesc: unknown %c\n", c);
#endif
		break;
	}
	tw->output = vt100_putch;
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
	/* FIXME: Origin mode!!! */
	gotoxy (tw, c - ' ', tw->miny + tw->escy1 - ' ');
	tw->output = vt100_putch;
}

/* Save character after ESC Y.	*/
static void
escy_putch (TEXTWIN* tw, unsigned int c)
{
	tw->escy1 = c;
	tw->output = escy1_putch;
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

	c &= 0x00ff;

	/* control characters */
	if (c < ' ') {
		switch (c) {
		case '\005':	/* ENQ - Return Terminal Status 
				   (Ctrl-E).  Default response is
				   the terminal name, e. g., 
				   "tw100".  Should later be
				   possible to be overridden by
				   the answerback string.  */
			answerback (tw);
			break;
		case '\007':	/* BEL - Bell (Ctrl-G).  */
			if (con_fd) {
				/* xconout2: Bconout would result in
				 * deadlock.
				 */
				char ch = 7;
				Fwrite (con_fd, 1, &ch);
			} else
				Bconout (2, 7);
			break;

		case '\010':	/* Backspace - BS (Ctrl-H).  */
			cub1 (tw);
			break;

		case '\011':	/* Horizontal Tab (HT) (Ctrl-I).  */
			gototab (tw, tw->cx, tw->cy);
			break;

		case '\012':	/* Line Feed or New Line (NL) (Ctrl-J).  */
		case '\013':	/* Vertical Tab (VT) (Ctrl-K)
				   same as LF.  */
		case '\014':	/* Form Feed or New Page (NP) (Ctrl-L)
				   same as LF.  */
			new_line (tw);
			break;
			
		/* FIXME: Handle newline mode!  */
		case '\015':	/* Carriage Return - CR (Ctrl-M).  */
			gotoxy (tw, 0, cy - RELOFFSET (tw));
			break;
			
		case '\016':	/* Shift Out (SO) (Ctrl-N) -> Switch 
				   to Alternate Character Set.  
				   Invokes the G1 character set.  */
			tw->curr_tflags = 
				(tw->curr_tflags & ~TCHARSET_MASK) | 1;
			break;

		case '\017':	/* Shift In (SI) (Ctrl-O) -> Switch 
				   to Standard Character Set.  
				   Invokes the G0 character set.  */
			tw->curr_tflags = 
				(tw->curr_tflags & ~TCHARSET_MASK);
			break;

		case '\033':	/* ESC */
			tw->output = vt100_putesc;
			tw->escbuf[0] = '\0';
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
}
