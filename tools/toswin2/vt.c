/*
 * Funktionen, die beide Emulatoren benutzen.
 *
 * This file needs to be rewritten.  Many functions can be
 * optimized a lot.
 */

#include "vt.h"
#include "isotost.h"
#include "ansicol.h"

/*
 * paint(v, c): put character 'c' at the current (x, y) coordinates
 * of window v. If insert mode is on, this involves moving all the
 * other characters one space to the right, first.
 */
void paint(TEXTWIN *v, unsigned int c)
{
	int i;
	int line = v->cy;

	ulong use_attribute = ~CLINEDRAW & v->curr_cattr;
	char charset = v->gsets[v->curr_tflags & TCHARSET_MASK];
	
	if ((charset == '0') &&
	    (c== '`' ||
	     c == 'a' ||
	     (c >= 'f' && c <= '~') ||
	     (c >= '+' && c <= '.') ||
	     c == '.' ||
	     c == '0'))
	     	use_attribute |= CLINEDRAW;

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

	if (v->curr_tflags & TINSERT)
	{
		memmove (v->cdata[line] + v->cx + 1, v->cdata[line] + v->cx, (NCOLS(v) - v->cx - 1) * sizeof(v->cdata[0][0]));
		for (i = NCOLS(v) - 1; i > v->cx; --i)
		{
			v->cflags[line][i] = v->cflags[line][i-1] | CDIRTY;
		}
	}
	v->cdata[line][v->cx] = c;
	v->cflags[line][v->cx] = CDIRTY | use_attribute;
	v->dirty[line] |= SOMEDIRTY;
}

/* Unconditionally display character C even if it is a
 * graphic character.  */
void vt_quote_putch (TEXTWIN* tw, unsigned int c)
{
	if (tw->do_wrap && (tw->curr_tflags & TWRAPAROUND) && tw->cx >= tw->maxx - 1)
	{
		new_line (tw);
		tw->cx = 0;
		tw->do_wrap = 0;
	}
	paint (tw, c);
	++tw->cx;

	if (tw->cx >= tw->maxx)
	{
		tw->cx = tw->maxx - 1;
		/* Character was drawn in last column.  */
		if (tw->curr_tflags & TWRAPAROUND)
			tw->do_wrap = 1;
	}
}

/* Cursor down one line.  */
void
cud1 (TEXTWIN* tw)
{
	gotoxy (tw, tw->cx, tw->cy + 1 - RELOFFSET (tw));
}

/* Cursor down N lines, no scrolling */
void
cud (TEXTWIN* tw, short n)
{
	gotoxy (tw, tw->cx, tw->cy + n - RELOFFSET (tw));
}

/* Cursor up one line.	*/
void
cuu1 (TEXTWIN* tw)
{
	gotoxy (tw, tw->cx, tw->cy - 1 - RELOFFSET (tw));
}

/* Cursor up N lines.  */
void
cuu (TEXTWIN* tw, short n)
{
	gotoxy (tw, tw->cx, tw->cy - n - RELOFFSET (tw));
}

/* Cursor left one column.  */
void
cub1 (TEXTWIN* tw)
{
	gotoxy (tw, tw->cx - 1, tw->cy - RELOFFSET (tw));
}

/* Cursor left N columns.  */
void
cub (TEXTWIN* tw, short n)
{
	gotoxy (tw, tw->cx - n, tw->cy - RELOFFSET (tw));
}

/* Cursor right one column.  */
void
cuf1 (TEXTWIN* tw)
{
	gotoxy (tw, tw->cx + 1, tw->cy - RELOFFSET (tw));
}

/* Cursor right N columns.  */
void
cuf (TEXTWIN* tw, short n)
{
	gotoxy (tw, tw->cx + n, tw->cy - RELOFFSET (tw));
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

	y += RELOFFSET (tw);
	if (tw->curr_tflags & TORIGIN) {
		/*
		 * line & column numbers are relativ to the margin,
		 * and cursor cannot be places outside margins
		 */
		if (y < tw->scroll_top)
			y = tw->scroll_top;
		else if (y >= tw->scroll_bottom)
			y = tw->scroll_bottom - 1;
	} else
	{
		/*
		 * line & column numbers are relativ to the upper left
		 * character position on the screen
		 */
		if (y < tw->miny)
			y = tw->miny;
		else if (y >= tw->maxy)
			y = tw->maxy - 1;
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

/*
 * Moves cursor down amount lines, scrolls if necessary.
 * Won't leave scrolling region. No carriage return.
 */
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

void carriage_return(TEXTWIN *tw)
{
	gotoxy (tw, 0, tw->cy - RELOFFSET (tw));
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

	if (tw->last_cy >= cy && tw->last_cy < tw->scroll_bottom)
		--tw->last_cy;
		
	/* FIXME: A ring buffer would be better.  */
	saved_data = tw->cdata[cy];
	saved_cflag = tw->cflags[cy];
	memmove (tw->cdata + cy, tw->cdata + cy + 1,
		 (sizeof saved_data) * lines);
	memmove (tw->cflags + cy, tw->cflags + cy + 1,
		 (sizeof saved_cflag) * lines);
	if (cy == 0) {
		/* This must be tw->cy, not cy!  */
		memmove (tw->dirty + cy, tw->dirty + cy + 1, lines);
	} else {
		memset (tw->dirty + cy, ALLDIRTY, lines);
	}
	tw->cdata[tw->scroll_bottom - 1] = saved_data;
	tw->cflags[tw->scroll_bottom - 1] = saved_cflag;

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

	if (tw->last_cy > cy && tw->last_cy < tw->scroll_bottom)
		++tw->last_cy;
		
	/* FIXME: A ring buffer would be better.  */
	saved_data = tw->cdata[cy + lines];
	saved_cflag = tw->cflags[cy + lines];
	memset (tw->dirty + cy + 1, ALLDIRTY, lines);
	memmove (tw->cdata + cy + 1, tw->cdata + cy,
		 (sizeof saved_data) * lines);
	memmove (tw->cflags + cy + 1, tw->cflags + cy,
		 (sizeof saved_cflag) * lines);
	tw->cdata[cy] = saved_data;
	tw->cflags[cy] = saved_cflag;

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
		v->cdata[r][i] = ' ';
		v->cflags[r][i] = v->curr_cattr &
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
	v->cdata[y][x] = ' ';
	v->cflags[y][x] = CDIRTY | (v->curr_cattr & (CBGCOL|CFGCOL));
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
		v->cdata[y][i] = v->cdata[y][i+1];
		v->cflags[y][i] = v->cflags[y][i+1] | CDIRTY;
	}
	v->cdata[y][v->maxx-1] = ' ';
	v->cflags[y][v->maxx-1] = CDIRTY | (v->curr_cattr & (CBGCOL|CFGCOL));
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

/* Turn inverse video on/off.  */
void
inverse_video (TEXTWIN* tw, int flag)
{
	if (flag && (tw->curr_tflags & TDECSCNM))
		return;
	if (!flag && !(tw->curr_tflags & TDECSCNM))
		return;

	tw->cfg->bg_color ^= tw->cfg->fg_color;
	tw->cfg->fg_color ^= tw->cfg->bg_color;
	tw->cfg->bg_color ^= tw->cfg->fg_color;
	tw->cfg->bg_effects ^= tw->cfg->fg_effects;
	tw->cfg->fg_effects ^= tw->cfg->bg_effects;
	tw->cfg->bg_effects ^= tw->cfg->fg_effects;

	if (tw->curr_tflags & TDECSCNM)
		tw->curr_tflags &= ~TDECSCNM;
	else
		tw->curr_tflags |= TDECSCNM;
	
	memset (tw->dirty + tw->miny, ALLDIRTY, NROWS (tw));
	refresh_textwin (tw, 0);
}

/* Reset colors to original colors (i. e. those that were
 * configured in the window configuration dialog).
 */
void 
original_colors(TEXTWIN *v)
{
	if (v->cfg->vdi_colors) {
		v->curr_cattr = (v->curr_cattr &
			~(CFGCOL | CBGCOL)) |
			COLORS (v->cfg->fg_color, v->cfg->bg_color);
	} else {
		v->curr_cattr = (v->curr_cattr &
			~(CFGCOL | CBGCOL | CE_ANSI_EFFECTS)) |
			COLORS (ANSI_DEFAULT, ANSI_DEFAULT);
	}
}

/* Save cursor position and attributes.
   FIXME: Two data sets (standard and alternate display)
   have to be stored.  */
void
save_cursor (TEXTWIN* tw)
{
	tw->saved_x = tw->cx;
	tw->saved_y = tw->cy;
	tw->saved_tflags = tw->curr_tflags;
	tw->saved_cattr = tw->curr_cattr;
	memcpy (tw->saved_gsets, tw->gsets, sizeof tw->saved_gsets);
}

/* Restore cursor position and attributes.
   FIXME: Two data sets (standard and alternate display)
   have to be stored.  */
void
restore_cursor (TEXTWIN* tw)
{
	/* Xterm restores the character sets if the cursor has
	   actually never been saved before.  */
	if (tw->saved_x != -1 && tw->saved_y != -1) {
		memcpy (tw->gsets, tw->saved_gsets, sizeof tw->gsets);
	} else {
		tw->gsets[0] = 'B';
		tw->gsets[1] = '0';
		tw->gsets[2] = 'B';
		tw->gsets[3] = 'B';
	}
	
	tw->curr_cattr &= ~ATTRIBUTES;
	tw->curr_cattr |= (tw->saved_cattr & ATTRIBUTES);
	tw->curr_tflags &= ~DECSC_FLAGS;
	tw->curr_tflags |= (tw->saved_tflags & DECSC_FLAGS);
	/* If never saved the coordinates will be negative.
	   but that will place the cursor in the upper left
	   corner anyway, which is exactly what we want.  */	
	gotoxy (tw, tw->saved_x, tw->saved_y - RELOFFSET (tw));
}


/* Implement hard or soft reset.  Modelled after VTReset in
   xterm.  */
void
vt_reset (struct textwin* tw, bool full, bool saved)
{	
	tw->scroll_top = tw->miny;
	tw->scroll_bottom = tw->maxy;
	
	tw->curr_cattr &= ~CPROTECTED;
	tw->curr_tflags &= ~TORIGIN;
	original_colors (tw);
	
	memcpy (tw->gsets, "B0BB", sizeof tw->gsets);
	tw->curr_tflags = (tw->curr_tflags & ~TCHARSET_MASK);
	
	if (full) {
		tw->curr_tflags |= TWRAPAROUND;
		tw->curr_tflags &= ~TINSERT;
		tw->curr_cattr &= ~(ATTRIBUTES ^ CBGCOL ^ CFGCOL);

		reset_tabs (tw);
		tw->curs_mode = CURSOR_NORMAL;
		tw->vt_mode = tw->cfg->vt_mode;
		clear (tw);
		
		/* Leave alternate screen!  */
		
		/* Reset to 80 columns.  */
		if (tw->curr_tflags & TDECCOLM)
			resize_textwin (tw, 80, NROWS (tw), SCROLLBACK (tw));
		/* Reset to normal video.  */
		inverse_video (tw, 0);

		gotoxy (tw, 0, 0);
		save_cursor (tw);			
	} else {
		tw->curr_tflags |= TWRAPAROUND;  /* FIXME: Read from config.  */
		tw->curr_tflags &= ~TINSERT;
		tw->curr_cattr &= ~(ATTRIBUTES ^ CBGCOL ^ CFGCOL);
		tw->saved_x = tw->saved_y = -1;
	}
	
	if (saved) {
		short i;
		for (i = 0; i < tw->miny; ++i) {
			memset (tw->cdata[i], ' ', 
				(sizeof tw->cdata[i][0]) * tw->maxx);
			memulset (tw->cflags[i], tw->curr_cattr, tw->maxx);
		}
	}
}
