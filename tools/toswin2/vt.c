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
			vt100_putch (tw, '\n');
			tw->cx = 1;
			tw->do_wrap = 0;
		} else {
			tw->cx = tw->maxx - 1;
		}
	} else if (tw->cx >= tw->maxx - 1)
		tw->do_wrap = 1;
}

/*
 * gotoxy (v, x, y): move current cursor address of window v to (x, y)
 * verifies that (x,y) is within range
 */
void gotoxy(TEXTWIN *v, short x, short y)
{
	if (x < 0) 
		x = 0;
	else if (x >= v->maxx) 
		x = v->maxx - 1;
	if (y < v->miny) 
		y = v->miny;
	else if (y >= v->maxy) 
		y = v->maxy - 1;

	v->cx = x;
	v->cy = y;
	v->do_wrap = 0;
}

/* Delete line ROW of window TW. The screen up to and
 * including line END is scrolled up, and line END is cleared.
 */
void 
delete_line (TEXTWIN* tw, int row, int end)
{
	int doscroll = (row == 0);
	unsigned char* saved_data;
	unsigned long* saved_cflag;

	if (end > tw->maxy - 1)
		end = tw->maxy - 1;
	if (row == tw->miny)
		row = 0;

	/* FIXME: A ring buffer would be better.  */	
	saved_data = tw->data[row];
	saved_cflag = tw->cflag[row];
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
	tw->data[end] = saved_data;
	tw->cflag[end] = saved_cflag;
	
	/* clear the last line */
	clrline (tw, end);
	if (doscroll)
		++tw->scrolled;
	tw->do_wrap = 0;
}

/* Scroll the entire window from line ROW to line END down,
 * then clear line ROW.
 */
void 
insert_line (TEXTWIN* tw, int row, int end)
{
	unsigned char* saved_data;
	unsigned long* saved_cflag;

	if (end > tw->maxy - 1)
		end = tw->maxy - 1;

	/* FIXME: A ring buffer would be better.  */	
	saved_data = tw->data[end];
	saved_cflag = tw->cflag[end];
	memulset (tw->dirty + row, ALLDIRTY, end - row);
	memmove (tw->data + row - 1, tw->data + row, 
		 (end - row) * sizeof tw->data[0]);
	memmove (tw->cflag + row - 1, tw->cflag + row,
		 (end - row) * sizeof tw->cflag[0]);
	tw->data[row] = saved_data;
	tw->cflag[row] = saved_cflag;

	clrline (tw, row);
	tw->do_wrap = 0;
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
	
	/* Last line.  */
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

