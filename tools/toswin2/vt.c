/*
 * Funktionen, die beide Emulatoren benutzen.
 *
 * This file needs to be rewritten.  Many functions can be
 * optimized a lot.
 */

#include "global.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"

/*
 * paint(v, c): put character 'c' at the current (x, y) coordinates
 * of window v. If insert mode is on, this involves moving all the
 * other characters one space to the right, first.
 */
void paint(TEXTWIN *v, unsigned int c)
{
	int i;
	int line = v->cy;

	ulong use_attribute = ~CACS & v->term_cattr;

	if ((v->term_cattr & CACS) &&
	    (c== '`' ||
	     c == 'a' ||
	     (c >= 'f' && c <= '~') ||
	     (c >= '+' && c <= '.') ||
	     c == '.' ||
	     c == '0'))
	     	use_attribute |= CACS;

	switch (v->cfg->char_tab)
	{
		case TAB_ATARI :
			/* do nothing */
			break;

		case TAB_ISO :
			c = (int) iso_to_st[c];
			break;

		default:
			debug("paint: unknown char_tab: %d\n", v->cfg->char_tab);
			break;
	}

	if (v->term_flags & FINSERT)
	{
		memmove (v->data[line] + v->cx - 1, v->data[line] + v->cx,
			 NCOLS (v) - v->cx);
		for (i = v->maxx-1; i > v->cx; --i)
		{
			v->cflag[line][i] = v->cflag[line][i-1] | CDIRTY;
		}
	}
	if (v->data[line][v->cx] != c)
	{
		v->data[line][v->cx] = c;
		v->cflag[line][v->cx] = CDIRTY | use_attribute;
	}
	else
		v->cflag[line][v->cx] = (CTOUCHED | use_attribute);
	v->dirty[line] |= SOMEDIRTY;
}

/* Unconditionally display character C even if it is a
 * graphic character.  */
void
vt_quote_putch (TEXTWIN* tw, unsigned int c)
{
	paint (tw, c);
	++tw->cx;

	if (tw->cx >= tw->maxx) {
		/* Character was drawn in last column.  */
		if (tw->do_wrap && !(tw->term_flags & FNOAM)) {
			new_line (tw);
			tw->cx = 0;
			tw->do_wrap = 0;
		} else {
			tw->cx = tw->maxx - 1;
		}
	} else if (tw->cx >= tw->maxx - 1)
		tw->do_wrap = 1;
}

/* Cursor down one line.  */
void
cud1 (TEXTWIN* tw)
{
	if (tw->origin)
		gotoxy (tw, tw->cx, tw->cy + 1 - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx, tw->cy + 1);
}

/* Cursor down N lines.  */
void
cud (TEXTWIN* tw, short n)
{
	if (tw->origin)
		gotoxy (tw, tw->cx, tw->cy + n - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx, tw->cy + n);
}

/* Cursor up one line.	*/
void
cuu1 (TEXTWIN* tw)
{
	if (tw->origin)
		gotoxy (tw, tw->cx, tw->cy - 1 - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx, tw->cy - 1);
}

/* Cursor up N lines.  */
void
cuu (TEXTWIN* tw, short n)
{
	if (tw->origin)
		gotoxy (tw, tw->cx, tw->cy - n - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx, tw->cy - n);
}

/* Cursor left one column.  */
void
cub1 (TEXTWIN* tw)
{
	if (tw->origin)
		gotoxy (tw, tw->cx - 1, tw->cy - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx - 1, tw->cy);
}

/* Cursor left N columns.  */
void
cub (TEXTWIN* tw, short n)
{
	if (tw->origin)
		gotoxy (tw, tw->cx - n, tw->cy - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx - n, tw->cy);
}

/* Cursor right one column.  */
void
cuf1 (TEXTWIN* tw)
{
	if (tw->origin)
		gotoxy (tw, tw->cx + 1, tw->cy - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx + 1, tw->cy);
}

/* Cursor right N columns.  */
void
cuf (TEXTWIN* tw, short n)
{
	if (tw->origin)
		gotoxy (tw, tw->cx + n, tw->cy - RELOFFSET (tw));
	else
		gotoxy (tw, tw->cx + n, tw->cy);
}

/* Move current cursor address of window TW to (X|Y),
 * verifies that (X|Y) is within range
 */
void
gotoxy (TEXTWIN* tw, short x, short y)
{
	if (x < 0)
		x = 0;
	else if (x >= tw->maxx)
		x = tw->maxx - 1;
	if (!tw->origin) {
		if (y < tw->miny)
			y = tw->miny;
		else if (y >= tw->maxy)
			y = tw->maxy - 1;
	} else {
		y += RELOFFSET (tw);
		if (y < tw->scroll_top)
			y = tw->scroll_top;
		else if (y >= tw->scroll_bottom - 1)
			y = tw->scroll_bottom - 1;
	}

	tw->cx = x;
	tw->cy = y;
	tw->do_wrap = 0;
}

void
reverse_cr (TEXTWIN* tw)
{
	if (tw->cy > tw->scroll_top)
		cuu1 (tw);
	else if (tw->cy == tw->scroll_top)
		insert_line (tw);
}

void
new_line (TEXTWIN* tw)
{
	if (tw->cy < tw->scroll_bottom - 1)
		cud1 (tw);
	else if (tw->cy == tw->scroll_bottom - 1) {
		unsigned short cy = tw->cy;

		tw->cy = tw->scroll_top;
		delete_line (tw);
		tw->cy = cy;
	}
}

/* Delete line at cursor position.  */
void
delete_line (TEXTWIN* tw)
{
	unsigned char* saved_data;
	unsigned long* saved_cflag;
	unsigned short cy = tw->cy;
	unsigned long lines;

	if (cy < tw->scroll_top || cy >= tw->scroll_bottom)
		return;
	tw->do_wrap = 0;

	if (cy == tw->miny)
		cy = 0;
	lines = tw->scroll_bottom - cy - 1;

	/* FIXME: A ring buffer would be better.  */
	saved_data = tw->data[cy];
	saved_cflag = tw->cflag[cy];
	memmove (tw->data + cy, tw->data + cy + 1,
		 (sizeof saved_data) * lines);
	memmove (tw->cflag + cy, tw->cflag + cy + 1,
		 (sizeof saved_cflag) * lines);
	if (cy == 0) {
		/* This must be tw->cy, not cy!  */
		memmove (tw->dirty + cy, tw->dirty + cy + 1, lines);
	} else {
		memset (tw->dirty + cy, ALLDIRTY, lines);
	}
	tw->data[tw->scroll_bottom - 1] = saved_data;
	tw->cflag[tw->scroll_bottom - 1] = saved_cflag;

	/* clear the last line */
	clrline (tw, tw->scroll_bottom - 1);
	if (cy == 0)
		++tw->scrolled;
}

/* Insert line at current cursor position.  */
void
insert_line (TEXTWIN* tw)
{
	unsigned char* saved_data;
	unsigned long* saved_cflag;
	unsigned short cy = tw->cy;
	unsigned long lines;

	if (cy < tw->scroll_top || cy >= tw->scroll_bottom)
		return;
	tw->do_wrap = 0;
	lines = tw->scroll_bottom - cy - 1;

	/* FIXME: A ring buffer would be better.  */
	saved_data = tw->data[cy + lines];
	saved_cflag = tw->cflag[cy + lines];
	memulset (tw->dirty + cy + 1, ALLDIRTY, lines);
	memmove (tw->data + cy + 1, tw->data + cy,
		 (sizeof saved_data) * lines);
	memmove (tw->cflag + cy + 1, tw->cflag + cy,
		 (sizeof saved_cflag) * lines);
	tw->data[cy] = saved_data;
	tw->cflag[cy] = saved_cflag;

	clrline (tw, cy);
	tw->do_wrap = 0;
	refresh_textwin (tw, 0);
}

/*
 * clrline(v, r): clear line r of window v
 */
void clrline(TEXTWIN *v, short r)
{
	int i;

	for (i = v->maxx-1; i >= 0; --i)
	{
		v->data[r][i] = ' ';
		v->cflag[r][i] = v->term_cattr &
			(CBGCOL | CFGCOL);
	}
	v->dirty[r] = ALLDIRTY;
}

/*
 * clear(v): clear the whole window v
 */
void clear(TEXTWIN *v)
{
	int y;

	for (y = v->miny; y < v->maxy; y++)
		clrline(v, y);
	v->do_wrap = 0;
}

/*
 * clrchar(v, x, y): clear the (x,y) position on window v
 */
void clrchar(TEXTWIN *v, short x, short y)
{
	if (v->data[y][x] != ' ')
	{
		v->data[y][x] = ' ';
		v->cflag[y][x] = CDIRTY | (v->term_cattr & (CBGCOL|CFGCOL));
	}
	else
		v->cflag[y][x] = CTOUCHED | (v->term_cattr & (CBGCOL|CFGCOL));
	v->dirty[y] |= SOMEDIRTY;
}

/*
 * Clear window TW from position (X1, Y1) to position (X2, Y2)
 * inclusive. It is assumed that Y2 >= Y1.
 */
void
clrfrom (TEXTWIN* tw, short x1, short y1, short x2, short y2)
{
	int x, y;

	if (y2 < y1)
		return;
	/* Only one line to clear?  */
	if (y1 == y2) {
		if (x1 == 0 && x2 == tw->maxx - 1)
			clrline (tw, y1);
		else
			for (x = x1; x <= x2; ++x)
				clrchar (tw, x, y1);
		return;
	}

	/* Clear first line.  */
	if (x1 == 0 && x2 == tw->maxx - 1)
		clrline (tw, y1);
	else
		for (x = x1; x < tw->maxx; ++x) {
			clrchar (tw, x, y1);
		}
	y = y1 + 1;

	/* Middle part is always entire lines.  */
	while (y < y2) {
		clrline (tw, y++);
	}

	/* Last line.	*/
	if (x1 == 0 && x2 == tw->maxx - 1)
		clrline (tw, y2);
	else
		for (x = 0; x <= x2; ++x) {
			clrchar (tw, x, y2);
		}
}

/*
 * delete_char(v, x, y): delete the character at position (x, y) on
 * the screen; the rest of the line is scrolled left, and a blank is
 * inserted at the end of the line.
 */
void delete_char(TEXTWIN *v, short x, short y)
{
	int i;

	for (i = x; i < v->maxx-1; i++)
	{
		v->data[y][i] = v->data[y][i+1];
		v->cflag[y][i] = v->cflag[y][i+1] | CDIRTY;
	}
	v->data[y][v->maxx-1] = ' ';
	v->cflag[y][v->maxx-1] = CDIRTY | (v->term_cattr & (CBGCOL|CFGCOL));
	v->dirty[y] |= SOMEDIRTY;
	v->do_wrap = 0;
}

/*
 * assorted callback functions
 */
void set_title(TEXTWIN *v)
{
	WINDOW *w = v->win;

	title_window(w, v->captbuf);
}

void set_size(TEXTWIN *v)
{
	int 	rows, cols;
	char	*s;

	s = v->captbuf;
	while (*s && *s != ',')
		s++;
	if (*s)
		*s++ = 0;
	cols = atoi(v->captbuf);
	rows = atoi(s);
	if (rows == 0)
		rows = v->maxy;
	else if (rows < MINROWS)
		rows = MINROWS;
	else if (rows > MAXROWS)
		rows = MAXROWS;
	if (cols == 0)
		cols = v->maxx;
	else if (rows < MINCOLS)
		rows = MINCOLS;
	else if (cols > MAXCOLS)
		cols = MAXCOLS;
	resize_textwin(v, cols, rows, v->miny);
}

#if 0
/*
 * routines for setting the cursor state in window v
*/
void set_curs(TEXTWIN *v, int on)
{
	short cx = v->cx;

	if (cx >= v->maxx)
		cx = v->maxx - 1;

	if (on && (v->term_flags & FCURS))
	{
		v->cflag[v->cy][cx] ^= CINVERSE;
		v->cflag[v->cy][cx] |= CTOUCHED;
		v->dirty[v->cy] |= SOMEDIRTY;
		v->term_flags |= FFLASH;
	}
	else if (!on)
	{
		v->cflag[v->cy][cx] ^= CINVERSE;
		v->cflag[v->cy][cx] |= CTOUCHED;
		v->dirty[v->cy] |= SOMEDIRTY;
		v->term_flags &= ~FFLASH;
	}
}

void curs_on(TEXTWIN *v)
{
	 set_curs(v, 1);
}

void curs_off(TEXTWIN *v)
{
	 set_curs(v, 0);
}
#endif

/* Reset colors to original colors (i. e. those that were
 * configured in the window configuration dialog).
 */
void original_colors(TEXTWIN *v)
{
	if (v->cfg->vdi_colors) {
		v->term_cattr = (v->term_cattr &
			~(CFGCOL | CBGCOL)) |
			COLORS (v->cfg->fg_color, v->cfg->bg_color);
	} else {
		v->term_cattr = (v->term_cattr &
			~(CFGCOL | CBGCOL | CE_ANSI_EFFECTS)) |
			COLORS (9, 9);
	}
}

