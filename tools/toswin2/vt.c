/*
 * Funktionen, die beide Emulatoren benutzen.
 */

#include "global.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"


/*
 * paint(v, c): put character 'c' at the current (x, y) coordinates
 * of window v. If insert mode is on, this involves moving all the
 * other characters one space to the left, first.
 */
void paint(TEXTWIN *v, int c)
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
		for (i = v->maxx-1; i > v->cx; --i) 
		{
			v->data[line][i] = v->data[line][i-1];
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

/*
 * gotoxy (v, x, y): move current cursor address of window v to (x, y)
 * verifies that (x,y) is within range
 */
void gotoxy(TEXTWIN *v, int x, int y)
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
}

/*
 * clrline(v, r): clear line r of window v
 */
void clrline(TEXTWIN *v, int r)
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
}

/*
 * clrchar(v, x, y): clear the (x,y) position on window v
 */
void clrchar(TEXTWIN *v, int x, int y)
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
 * clrfrom(v, x1, y1, x2, y2): clear window v from position (x1,y1) to
 * position (x2, y2) inclusive. It is assumed that y2 >= y1.
 */
void clrfrom(TEXTWIN *v, int x1, int y1, int x2, int y2)
{
	int i;

	for (i = x1; i <= x2; i++)
		clrchar(v, i, y1);
	if (y2 > y1) 
	{
		for (i = 0; i <= x2; i++)
			clrchar(v, i, y2);
		for (i = y1+1; i < y2; i++)
			clrline(v, i);
	}
}

/*
 * delete_char(v, x, y): delete the character at position (x, y) on
 * the screen; the rest of the line is scrolled left, and a blank is
 * inserted at the end of the line.
 */
void delete_char(TEXTWIN *v, int x, int y)
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

/*
 * routines for setting the cursor state in window v 
*/
void set_curs(TEXTWIN *v, int on)
{
	if (on && (v->term_flags & FCURS) && !(v->term_flags & FFLASH)) 
	{
		v->cflag[v->cy][v->cx] ^= CINVERSE;
		v->cflag[v->cy][v->cx] |= CTOUCHED;
		v->dirty[v->cy] |= SOMEDIRTY;
		v->term_flags |= FFLASH;
	} 
	else if ( (!on) && (v->term_flags & FFLASH)) 
	{
		v->cflag[v->cy][v->cx] ^= CINVERSE;
		v->cflag[v->cy][v->cx] |= CTOUCHED;
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

/* Reset colors to original colors (i. e. those that were
 * configured in the window configuration dialog).
 */
void original_colors(TEXTWIN *v)
{
	v->term_cattr = (v->term_cattr & 
		~(CFGCOL | CBGCOL)) |
		COLORS(v->cfg->fg_color, v->cfg->bg_color);
	v->output = vt52_putch;
}

