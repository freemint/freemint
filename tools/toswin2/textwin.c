
#include <mintbind.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <limits.h>

#include "global.h"
#include "av.h"
#include "clipbrd.h"
#include "config.h"
#include "console.h"
#include "drag.h"
#include "proc.h"
#include "textwin.h"
#include "vt.h"
#include "window.h"
#include "ansicol.h"

#ifdef DEBUG
extern int do_debug;
# include <syslog.h>
#endif

#define CLEARED 2			/* redrawing a cleared area */

/*
 * lokale Variablen 
*/
static MFDB scr_mfdb;	/* left NULL so it refers to the screen by default */

/*
 * lokale Prototypen
*/
static void draw_buf(TEXTWIN *t, char *buf, short x, short y, ulong flag, short force);
static void update_chars(TEXTWIN *t, short firstcol, short lastcol, short firstline, short lastline, short force);
static void update_screen(TEXTWIN *t, short xc, short yc, short wc, short hc, short force);
static void draw_textwin(WINDOW *v, short x, short y, short w, short h);
static void close_textwin(WINDOW *v);
static void full_textwin(WINDOW *v);
static void move_textwin(WINDOW *v, short x, short y, short w, short h);
static void size_textwin(WINDOW *v, short x, short y, short w, short h);
static void newyoff(TEXTWIN *t, short y);
static void scrollupdn(TEXTWIN *t, short off, short direction);
static void arrow_textwin(WINDOW *v, short msg);
static void vslid_textwin(WINDOW *v, short vpos);
static void set_scroll_bars(TEXTWIN *t);
static void set_cwidths(TEXTWIN *t);

static void notify_winch (TEXTWIN* tw);
static void reread_size (TEXTWIN* t);
static void change_scrollback (TEXTWIN* t, short scrollback);
static void change_height (TEXTWIN* t, short rows);
static void change_width (TEXTWIN* t, short cols);

/*
static void output_textwin(TEXTWIN *t, short c);
*/


/* functions for converting x, y pixels to/from character coordinates */
/* NOTES: these functions give the upper left corner; to actually draw
 * a character, they must be adjusted down by t->cbase
 * Also: char2pixel accepts out of range character/column combinations,
 * but pixel2char never will generate such combinations.
 */
void char2pixel(TEXTWIN *t, short col, short row, short *xp, short *yp)
{
	short *WIDE = t->cwidths;
	short x;

	*yp = t->win->work.g_y - t->offy + row * t->cheight;
	if (!WIDE) 
	{
		*xp = t->win->work.g_x + col * t->cmaxwidth;
	}
	else 
	{	
		if (col >= NCOLS (t)) 
			*xp = t->win->work.g_x + t->win->work.g_w;
		else 
		{
			x = t->win->work.g_x;
			while(--col >= 0) 
				x += WIDE[t->data[row][col]];
			*xp = x;
		}
	}
}

void pixel2char(TEXTWIN *t, short x, short y, short *colp, short *rowp)
{
	short col, row, count, nextcount;
	short *WIDE = t->cwidths;

	row = (y - t->win->work.g_y + t->offy) / t->cheight;
	x -= t->win->work.g_x;

	if (WIDE == 0) 
		col = x / t->cmaxwidth;
	else 
	{
		count = 0;
		for (col = 0; col <= NCOLS (t); col++) 
		{
			nextcount = count + WIDE[t->data[row][col]];
			if (count <= x && x < nextcount) 
				break;
			count = nextcount;
		}
	}
	*rowp = row;
	*colp = col;
}

/* Draw a string in the alternate character set.  */
static void
draw_acs_text(TEXTWIN* t, short textcolor, short x, short y, char* buf)
{
	char* crs = buf;
	short max_x = x + t->win->work.g_w; /* FIXME: -1 ? */
	short cwidth = t->cmaxwidth;
	short cheight = t->cheight;
	short pxy[8];
	unsigned char letter[2] = { '\0', '\0' };
	
	while (*crs && x < max_x) {
		switch (*crs) {
			case '}': /* ACS_STERLING */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'\243' : '\234';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case '.': /* ACS_DARROW */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'v' : '\002';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case ',': /* ACS_LARROW */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'<' : '\004';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case '+': /* ACS_RARROW */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'>' : '\003';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case '-': /* ACS_UARROW */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'^' : '\001';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case 'h': /* ACS_BOARD */
				pxy[0] = x;
				pxy[1] = y;
				pxy[2] = x + cwidth - 1;
				pxy[3] = y + cheight - 1;
				set_fillcolor (textcolor);
				set_fillstyle (2, 22);
				vsf_perimeter (vdi_handle, 0);
				v_bar (vdi_handle, pxy);
				break;
				
			case '~': /* ACS_BULLET */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'o' : '\372';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case 'a': /* ACS_CKBOARD */
				pxy[0] = x;
				pxy[1] = y;
				pxy[2] = x + cwidth - 1;
				pxy[3] = y + t->cheight - 1;
				set_fillcolor (textcolor);
				set_fillstyle (2, 2);
				vsf_perimeter (vdi_handle, 0);
				v_bar (vdi_handle, pxy);
				break;
			
			case 'f': /* ACS_DEGREE */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'\260' : '\370';
				set_textcolor (textcolor);
				set_texteffects (0);
				v_gtext(vdi_handle, x, y + t->cbase, letter);
				break;
					
			case '`': /* ACS_DIAMOND */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y + (cheight >> 2);
				pxy[2] = pxy[0] + (cwidth >> 2);
				pxy[3] = y + (cheight >> 1);
				pxy[4] = pxy[0];
				pxy[5] = pxy[1] + (cheight >> 2);
				pxy[6] = pxy[0] - (cwidth >> 2);
				pxy[7] = pxy[3];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_fillarea (vdi_handle, 4, pxy);
				break;
			
			case 'z': /* ACS_GEQUAL */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'>' : '\362';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				if (t->cfg->char_tab != TAB_ATARI) {
					letter[0] = '_';
					v_gtext (vdi_handle, x, y + t->cbase, letter);
				}
				break;
			
			case '{': /* ACS_PI */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				if (t->cfg->char_tab != TAB_ATARI) {
					letter[0] = '\343';
					set_textcolor (textcolor);
					v_gtext (vdi_handle, x, y + t->cbase, letter);
				} else {
					/* FIXME: Paint greek letter pi.  */
					letter[0] = '*';
					set_textcolor (textcolor);
					v_gtext (vdi_handle, x, y + t->cbase, letter);
				}
				break;
			
			case 'q': /* ACS_HLINE */
				pxy[0] = x;
				pxy[1] = y + (cheight >> 1);
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				break;
				
			case 'i': /* ACS_LANTERN */
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				pxy[0] = x;
				pxy[1] = y + (cheight >> 1) - 1;
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				v_bar (vdi_handle, pxy);
				
				pxy[1] = y + (cheight >> 1) + 1;
				pxy[3] = pxy[1];
				v_bar (vdi_handle, pxy);
				
				pxy[0] = x + (cwidth >> 1) - 1;
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + (cheight >> 1) - 1;
				v_bar (vdi_handle, pxy);
				
				pxy[0] = x + (cwidth >> 1) + 1;
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + (cheight >> 1) - 1;
				v_bar (vdi_handle, pxy);
				
				pxy[1] = y + cheight - 1;
				v_bar (vdi_handle, pxy);
				
				pxy[0] = x + (cwidth >> 1) - 1;
				pxy[2] = pxy[0];
				v_bar (vdi_handle, pxy);
				
				break;
			
			case 'n': /* ACS_PLUS */
				pxy[0] = x;
				pxy[1] = y + (cheight >> 1);
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				pxy[4] = x + (cwidth >> 1);
				pxy[5] = y;
				pxy[6] = pxy[4];
				pxy[7] = y + cheight - 1;
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				v_bar (vdi_handle, pxy + 4);
				break;
				
			case 'y': /* ACS_LEQUAL */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'y' : '\363';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				if (t->cfg->char_tab != TAB_ATARI) {
					letter[0] = '_';
					v_gtext (vdi_handle, x, y + t->cbase, letter);
				}
				break;
			
			case 'm': /* ACS_LLCORNER */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + (cheight >> 1);
				pxy[4] = x + cwidth - 1;
				pxy[5] = pxy[3];
				vsl_type (vdi_handle, 1);
				vsl_color (vdi_handle, textcolor);
				v_pline (vdi_handle, 3, pxy);
				break;
				
			case 'j': /* ACS_LRCORNER */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + (cheight >> 1);
				pxy[4] = x;
				pxy[5] = pxy[3];
				vsl_type (vdi_handle, 1);
				vsl_color (vdi_handle, textcolor);
				v_pline (vdi_handle, 3, pxy);
				break;
				
			case '|': /* ACS_NEQUAL */
				letter[0] = '=';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				letter[0] = '/';
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case 'g': /* ACS_PLMINUS */
				/* We assume that if the Atari font is 
			         * selected,
				 * we have a font that is already in ISO-Latin 1.
				 */
				letter[0] = t->cfg->char_tab == TAB_ATARI ?
					'\261' : '\361';
				set_textcolor (textcolor);
				v_gtext (vdi_handle, x, y + t->cbase, letter);
				break;
			
			case 'o': /* ACS_S1 */
				pxy[0] = x;
				pxy[1] = y;
				pxy[2] = x + cwidth - 1;
				pxy[3] = y;
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				break;
				
			case 'p': /* ACS_S3 */
				pxy[0] = x;
				pxy[1] = y + cheight / 3;
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				break;
				
			case 'r': /* ACS_S7 */
				pxy[0] = x;
				pxy[1] = y + cheight - cheight / 3;
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				break;
				
			case 's': /* ACS_S9 */
				pxy[0] = x;
				pxy[1] = y + cheight - 1;
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				break;
				
			case '0': /* ACS_BLOCK */
				pxy[0] = x;
				pxy[1] = y;
				pxy[2] = x + cwidth - 1;
				pxy[3] = y + cheight - 1;
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				vsf_perimeter (vdi_handle, 0);
				v_bar (vdi_handle, pxy);
				break;
				
			case 'w': /* ACS_TTEE */
				pxy[0] = x;
				pxy[1] = y + (cheight >> 1);
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				pxy[4] = x + (cwidth >> 1);
				pxy[5] = pxy[1];
				pxy[6] = pxy[4];
				pxy[7] = y + cheight - 1;
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				v_bar (vdi_handle, pxy + 4);
				break;
			
			case 'u': /* ACS_RTEE */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + cheight - 1;
				pxy[4] = x;
				pxy[5] = y + (cheight >> 1);
				pxy[6] = pxy[0];
				pxy[7] = pxy[5];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				v_bar (vdi_handle, pxy + 4);
				break;
			
			case 't': /* ACS_LTEE */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + cheight - 1;
				pxy[4] = pxy[0];
				pxy[5] = y + (cheight >> 1);
				pxy[6] = x + cwidth - 1;
				pxy[7] = pxy[5];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				v_bar (vdi_handle, pxy + 4);
				break;
			
			case 'v': /* ACS_BTEE */
				pxy[0] = x;
				pxy[1] = y + (cheight >> 1);
				pxy[2] = x + cwidth - 1;
				pxy[3] = pxy[1];
				pxy[4] = x + (cwidth >> 1);
				pxy[5] = y;
				pxy[6] = pxy[4];
				pxy[7] = pxy[1];
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				v_bar (vdi_handle, pxy + 4);
				break;
			
			case 'l': /* ACS_ULCORNER */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y + cheight - 1;
				pxy[2] = pxy[0];
				pxy[3] = y + (cheight >> 1);
				pxy[4] = x + cwidth - 1;
				pxy[5] = pxy[3];
				vsl_type (vdi_handle, 1);
				vsl_color (vdi_handle, textcolor);
				v_pline (vdi_handle, 3, pxy);
				break;
							
			case 'k': /* ACS_URCORNER */
				pxy[0] = x;
				pxy[1] = y + (cheight >> 1);
				pxy[2] = x + (cwidth >> 1);
				pxy[3] = pxy[1];
				pxy[4] = pxy[2];
				pxy[5] = y + cheight - 1;
				vsl_type (vdi_handle, 1);
				vsl_color (vdi_handle, textcolor);
				v_pline (vdi_handle, 3, pxy);
				break;
							
			case 'x': /* ACS_VLINE */
				pxy[0] = x + (cwidth >> 1);
				pxy[1] = y;
				pxy[2] = pxy[0];
				pxy[3] = y + cheight - 1;
				set_fillcolor (textcolor);
				set_fillstyle (1, 1);
				v_bar (vdi_handle, pxy);
				break;
				
			default:
				break;
		}
		
		++crs;
		x += cwidth;
	}
}

/*
 * draw a (part of a) line on screen, with certain attributes (e.g.
 * inverse video) indicated by "flag". (x, y) is the upper left corner
 * of the box which will contain the line.
 * If "force" is 1, we may assume that the screen is already cleared
 * (this is done in update_screen() for us).
 * SPECIAL CASE: if buf is an empty string, we clear from "x" to
 * the end of the window.
 */
static void draw_buf(TEXTWIN *t, char *buf, short x, short y, ulong flag, short force)
{
	char *s, *lastnonblank;
	int x2, fillcolor, textcolor;
	int texteffects;
	short *WIDE = t->cwidths;
	short temp[4];
	int acs = flag & CACS;

	use_ansi_colors (t, flag, &textcolor, &fillcolor, &texteffects);
	x2 = x;
	s = buf;
	if (*s) 
	{
		lastnonblank = s-1;
		while (*s) 
		{
			if (*s != ' ') lastnonblank = s;
			if (WIDE)
				x2 += WIDE[(short)*s];
			else
				x2 += t->cmaxwidth;
			s++;
		}
		lastnonblank++;
		if (!(flag & CE_UNDERLINE))
			*lastnonblank = 0;
	} 
	else 
		x2 = t->win->work.g_x + t->win->work.g_w;

	set_wrmode(2);		/* transparent text */
	if (fillcolor >= 0 || (force != CLEARED)) 
	{
		temp[0] = x;
		temp[1] = y;
		temp[2] = x2 - 1;
		temp[3] = y + t->cheight - 1;
		set_fillcolor(fillcolor);
		set_fillstyle(1, 1);
		v_bar(vdi_handle, temp);
	}

	/* skip leading blanks -- we don't need to draw them again! */
	if (!(flag & CE_UNDERLINE)) 
	{
		while (*buf == ' ') 
		{
			buf++;
			x += WIDE ? WIDE[' '] : t->cmaxwidth;
		}
	}

	if (*buf) 
	{
		if (acs)
		{
			draw_acs_text(t, textcolor, x, y, buf);
		}
		else
		{
			set_textcolor(textcolor);
			set_texteffects(texteffects);
			v_gtext(vdi_handle, x, y + t->cbase, buf);
		}
	}
}

/*
 * update the characters on screen between "firstline,firstcol" and 
 * "lastline-1,lastcol-1" (inclusive)
 * if force == 1, the redraw must occur, otherwise it occurs only for
 * "dirty" characters. Note that we assume here that clipping
 * rectanges and wind_update() have already been set for us.
 */
static void update_chars(TEXTWIN *t, short firstcol, short lastcol, short firstline, 
									short lastline, short force)
{
#define CBUFSIZ 127
	unsigned char buf[CBUFSIZ+1], c;
	short px, py, ax, i, cnt, bufwidth;
	ulong flag;
	short *WIDE = t->cwidths;
	short lineforce = 0;
	ulong curflag;

#define flushbuf()	\
	{ 		\
		buf[i] = 0;	\
		draw_buf(t, buf, px, py, flag, lineforce); \
		px += bufwidth; \
		i = bufwidth = 0; \
	}

	if (t->windirty)
		reread_size (t);
	t->windirty = 0;
		
	/* make sure the font is set correctly */
	set_font(t->cfont, t->cpoints);

	/* find the place to start writing */
	char2pixel(t, firstcol, firstline, &ax, &py);

	/* now write the characters we need to */
	while (firstline < lastline) 
	{
		/* if no characters on the line need re-writing, skip the loop */
		if (!force && t->dirty[firstline] == 0) 
		{
			py += t->cheight;
			firstline++;
			continue;
		}
		px = ax;
		/*
		 * now, go along collecting characters to write into the buffer
		 * we add a character to the buffer if and only if (1) the
		 * character's attributes (inverse video, etc.) match the
		 * attributes of the character already in the buffer, and
		 * (2) the character needs redrawing. Otherwise, if there are
		 * characters in the buffer, we flush the buffer.
		 */
		i = bufwidth = 0;
		cnt = firstcol;
		flag = 0;
		lineforce = force;
		if (!lineforce && (t->dirty[firstline] & ALLDIRTY))
			lineforce = 1;
		while (cnt < lastcol) 
		{
		 	c = t->data[firstline][cnt];
		 	if (lineforce || (t->cflag[firstline][cnt] & (CDIRTY|CTOUCHED))) 
		 	{
				/* yes, this character needs drawing */
				/* if the font is proportional and the character has really changed,
				 * then all remaining characters will have to be redrawn, too
				 */
				if (WIDE && (lineforce == 0) && (t->cflag[firstline][cnt] & CDIRTY))
					lineforce = 1;

				curflag = t->cflag[firstline][cnt] & ~(CDIRTY|CTOUCHED);

				/* watch out for characters that can't be drawn in this font */
				if (!(curflag & CACS) && (c < t->minADE || c > t->maxADE))
					c = '?';

				if (flag == curflag) 
				{
					 buf[i++] = c;
					 bufwidth += (WIDE ? WIDE[c] : t->cmaxwidth);
					 if (i == CBUFSIZ)
					 { 
						flushbuf();
					}
				} 
				else 
				{
				 	if (i)
				 	{
				 		flushbuf();
					 }
					 flag = curflag;
					 buf[i++] = c;
					bufwidth += (WIDE ? WIDE[c] : t->cmaxwidth);
				}
		  	} 
		  	else 
		  	{
				if (i) 
				{
					flushbuf();
				}
				px += (WIDE ? WIDE[c] : t->cmaxwidth);
		  	}
		   cnt++;
		} /* while */
		if (i)
		{
			flushbuf();
		}
		if (WIDE)
		{
			/* the line's 'tail' */
			draw_buf(t, "", px, py, t->cflag[firstline][NCOLS (t) - 1], lineforce);
		}
		py += t->cheight;
		firstline++;
	}
}

/*
 * mark_clean: mark a window as having been completely updated
 */
void mark_clean(TEXTWIN *t)
{
	int line, col;

	for (line = 0; line < t->maxy; line++) 
	{
		if (t->dirty[line] == 0)
			continue;
		for (col = 0; col < NCOLS (t); col++) 
			t->cflag[line][col] &= ~(CDIRTY|CTOUCHED);
		t->dirty[line] = 0;
	}
}

/*
 * redraw all parts of window v which are contained within the
 * given rectangle. Assumes that the clipping rectange has already
 * been set correctly.
 * NOTE: more than one rectangle may cover the same area, so we
 * can't mark the window clean during the update; we have to do
 * it in a separate routine (mark_clean)
 */
static void update_screen(TEXTWIN *t, short xc, short yc, short wc, short hc, short force)
{
	short firstline, lastline, firstscroll;
	short firstcol, lastcol;
	short pxy[8];
	long scrollht = 0;

	if (t->win->flags & WSHADED)
		return;

	if (t->win->flags & WICONIFIED)
	{
		draw_winicon(t->win);
		return;
	}

	if (t->windirty)
		reread_size (t);
	t->windirty = 0;
	
	/* if t->scrolled is set, then the output routines faked the "dirty"
	 * flags on the scrolled lines under the assumption that we would
	 * do a blit scroll; so we do it here.
	 */
	if ((force == 0) && t->scrolled && (scrollht = t->scrolled * t->cheight) < hc) 
	{
		pxy[0] = xc;
		pxy[1] = yc + (int)scrollht;
		pxy[2] = xc + wc - 1;
		pxy[3] = yc + hc - 1;
		pxy[4] = xc;
		pxy[5] = yc;
		pxy[6] = pxy[2];
		pxy[7] = pxy[3] - (int)scrollht;
		vro_cpyfm(vdi_handle, S_ONLY, pxy, &scr_mfdb, &scr_mfdb);
	}

#if 0
	/* if `force' is set, clear the area to be redrawn -- it looks better */
	if (force == CLEARED) 
	{
		ulong flag = 0;
		WINCFG* cfg = t->cfg;
		int bg_color, fg_color, style;
		
		if (cfg->vdi_colors) {
			flag |= cfg->bg_color;
		} else {
			flag |= (9 | cfg->bg_effects);
		}
		use_ansi_colors (t, flag, &fg_color, &bg_color, &style);		
		set_fillcolor (bg_color);
		set_fillstyle (1, 1);
		pxy[0] = xc;
		pxy[1] = yc;
		pxy[2] = xc + wc - 1;
		pxy[3] = yc + hc - 1;
		vr_recfl(vdi_handle, pxy);
	}
#endif

	/* convert from on-screen coordinates to window rows & columns */
	pixel2char(t, xc, yc, &firstcol, &firstline);

	if (firstline < 0) 
		firstline = 0;
	else if (firstline >= t->maxy) 
		firstline = t->maxy - 1;

	lastline = 1 + firstline + (hc + t->cheight - 1) / t->cheight;

	/* The following if-clause preventd from drawing beyond
	   the lower limit of the window.  */
	if (lastline * t->cheight > t->offy + hc - t->cheight)
		lastline = (t->offy + hc - t->cheight) / t->cheight;
	/* And this one from forgetting the last line if it is partly
	   covered by another object.  */
	if (xc != t->win->work.g_x || yc == t->win->work.g_y ||
	    wc == t->win->work.g_w || hc == t->win->work.g_h)
		++lastline;
	/* And this one from crashes.  */
	if (lastline > t->maxy) 
		lastline = t->maxy;

	/* kludge for proportional fonts */
	if (t->cwidths) 
	{
		firstcol = 0;
		lastcol = NCOLS (t);
	}
	else
		pixel2char(t, xc+wc+t->cmaxwidth-1, yc, &lastcol, &firstline);

	/* if t->scrolled is set, the last few lines *must* be updated */
	if (t->scrolled && force == 0) 
	{
		firstscroll = firstline + (hc - (int)scrollht)/t->cheight;
		if (firstscroll <= firstline) 
		 	force = TRUE;
		else
		{
			update_chars(t, firstcol, lastcol, firstscroll, lastline, TRUE);
		   lastline = firstscroll;
		}
	}
	update_chars(t, firstcol, lastcol, firstline, lastline, force);
}

/*
 * redraw all parts of a window that need redrawing; this is called
 * after, for example, writing some text into the window
 */
void refresh_textwin(TEXTWIN *t, short force)
{
	WINDOW	*v = t->win;
	GRECT t1, t2;
	bool off = FALSE;
	
	/* exits if window isn't visible or was iconified/shaded */
	if (v->handle < 0 || (v->flags & WICONIFIED) || (v->flags & WSHADED))
		return;

	t2 = v->work;
	rc_intersect(&gl_desk, &t2);

	wind_update(TRUE);
	wind_get_grect(v->handle, WF_FIRSTXYWH, &t1);
	while (t1.g_w && t1.g_h) 
	{
		if (rc_intersect(&t2, &t1)) 
		{
			if (!off)
				off = hide_mouse_if_needed(&t1);
			if (memcmp (&t1, &t2, sizeof t1))
				set_clipping (vdi_handle, t1.g_x, t1.g_y, 
					      t1.g_w, t1.g_h, TRUE);
			else
				set_clipping (vdi_handle, 0, 0, 0, 0, FALSE);
			update_screen(t, t1.g_x, t1.g_y, t1.g_w, t1.g_h, force);
		}
		wind_get_grect(v->handle, WF_NEXTXYWH, &t1);
	}
	t->scrolled = t->nbytes = t->draw_time = 0;
	if (off)
		show_mouse();
	mark_clean(t);
	wind_update(FALSE);
}

/*
 * Methods for reacting to user events
 */
/* draw part of a window */
static void draw_textwin(WINDOW *v, short x, short y, short w, short h)
{
	TEXTWIN *t = v->extra;

	t->scrolled = 0;
	update_screen(v->extra, x, y, w, h, CLEARED);
	t->nbytes = t->draw_time = 0;
}


/* close a window (called when the closed box is clicked on) */
static void close_textwin(WINDOW *v)
{
	destroy_textwin(v->extra);
}

/* resize a window to its "full" size */
static void full_textwin(WINDOW *v)
{
	GRECT new;
	TEXTWIN	*t = v->extra;
	
	if (v->flags & WFULLED) 
		wind_get_grect(v->handle, WF_PREVXYWH, &new);
	else
		wind_get_grect(v->handle, WF_FULLXYWH, &new);
	wind_calc_grect(WC_WORK, v->kind, &new, &v->work);

	/* Snap */
	v->work.g_w -= (v->work.g_w % t->cmaxwidth);
	v->work.g_h -= (v->work.g_h % t->cheight);
	wind_calc_grect(WC_BORDER, v->kind, &v->work, &new);
	wind_set_grect(v->handle, WF_CURRXYWH, &new);

	v->flags ^= WFULLED;
	set_scroll_bars(t);
	
	t->windirty = 1;
}

/* resize a window */
static void move_textwin(WINDOW *v, short x, short y, short w, short h)
{
	GRECT full;
	TEXTWIN	*t = v->extra;
	
	wind_get_grect(v->handle, WF_FULLXYWH, &full);

#if 0
	if (w > full.g_w) 
		w = full.g_w;
	if (h > full.g_h) 
		h = full.g_h;
#endif

	wind_calc(WC_WORK, v->kind, x, y, w, h, &v->work.g_x, &v->work.g_y, &v->work.g_w, &v->work.g_h);

	if (!(v->flags & WICONIFIED))
	{
		/* Snap */
		v->work.g_w -= (v->work.g_w % t->cmaxwidth);
		v->work.g_h -= (v->work.g_h % t->cheight);
	}
	wind_calc(WC_BORDER, v->kind, v->work.g_x, v->work.g_y, v->work.g_w, v->work.g_h, &x, &y, &w, &h);
	wind_set(v->handle, WF_CURRXYWH, x, y, w, h);

	if (w != full.g_w || h != full.g_h)
		v->flags &= ~WFULLED;

	if (!(v->flags & WICONIFIED))
	{
		/* das sind BORDER-Gr”žen! */
		t->cfg->xpos = x;
		t->cfg->ypos = y;
		t->cfg->width = w;
		t->cfg->height = h;
	}
	t->windirty = 1;
}

static void size_textwin(WINDOW *v, short x, short y, short w, short h)
{
	TEXTWIN *t = v->extra;

	t->windirty = 1;
	v->moved (v, x, y, w, h);
	set_scroll_bars(t);
}

/*
 * handle an arrow event to a window
 */
static void newyoff(TEXTWIN *t, short y)
{
	t->offy = y;
	set_scroll_bars(t);
}

#define UP 0
#define DOWN 1

#define scrollup(t, off) scrollupdn(t, off, UP)
#define scrolldn(t, off) scrollupdn(t, off, DOWN)

static void scrollupdn(TEXTWIN *t, short off, short direction)
{
	WINDOW	*v = t->win;
	GRECT t1, t2;
	short pxy[8];
	bool m_off = FALSE;
	
	if (off <= 0)
		return;

	t2 = v->work;
	rc_intersect(&gl_desk, &t2);
	wind_update(TRUE);
	wind_get_grect(v->handle, WF_FIRSTXYWH, &t1);
	while (t1.g_w && t1.g_h) 
	{
		if (rc_intersect(&t2, &t1)) 
		{
			if (!m_off)
				m_off = hide_mouse_if_needed(&t1);
			if (memcmp (&t1, &t2, sizeof t1))
				set_clipping(vdi_handle, t1.g_x, t1.g_y, 
							 t1.g_w, t1.g_h, TRUE);
			else
				set_clipping (vdi_handle, 0, 0, 0, 0, FALSE);
				
			if (off >= t1.g_h) 
				update_screen(t, t1.g_x, t1.g_y, 
						 t1.g_w, t1.g_h, TRUE);
			else 
			{
				if (direction  == UP) 
				{
					pxy[0] = t1.g_x;	/* "from" address */
					pxy[1] = t1.g_y + off;
					pxy[2] = t1.g_x + t1.g_w - 1;
					pxy[3] = t1.g_y + t1.g_h - 1;
					pxy[4] = t1.g_x;	/* "to" address */
					pxy[5] = t1.g_y;
					pxy[6] = t1.g_x + t1.g_w - 1;
					pxy[7] = t1.g_y + t1.g_h - off - 1;
				} 
				else 
				{
					pxy[0] = t1.g_x;
					pxy[1] = t1.g_y;
					pxy[2] = t1.g_x + t1.g_w - 1;
					pxy[3] = t1.g_y + t1.g_h - off - 1;
					pxy[4] = t1.g_x;
					pxy[5] = t1.g_y + off;
					pxy[6] = t1.g_x + t1.g_w - 1;
					pxy[7] = t1.g_y + t1.g_h - 1;
				}
				vro_cpyfm(vdi_handle, S_ONLY, pxy, &scr_mfdb, &scr_mfdb);
				if (direction == UP)
					update_screen(t, t1.g_x, pxy[7], 
						      t1.g_w, off, TRUE);
				else
					update_screen(t, t1.g_x, t1.g_y, 
						      t1.g_w, off, TRUE);
			}
			}
		wind_get_grect(v->handle, WF_NEXTXYWH, &t1);
	}
	if (m_off)
		show_mouse();
	wind_update(FALSE);
}

static void arrow_textwin(WINDOW *v, short msg)
{
	TEXTWIN	*t = (TEXTWIN *)v->extra;
	short oldoff;

/*
	refresh_textwin(t, FALSE);
*/
	switch(msg) 
	{
		case WA_UPPAGE:
			newyoff(t, t->offy - v->work.g_h);
			break;
		case WA_DNPAGE:
			newyoff(t, t->offy + v->work.g_h);
			break;
		case WA_UPLINE:
			oldoff = t->offy;
			newyoff(t, t->offy - t->cheight);
			scrolldn(t, oldoff - t->offy);
			return;
		case WA_DNLINE:
			oldoff = t->offy;
			newyoff(t, t->offy + t->cheight);
			scrollup(t, t->offy - oldoff);
			return;
		default:
			return;
	}
	refresh_textwin(t, TRUE);
}

static void vslid_textwin(WINDOW *v, short vpos)
{
	TEXTWIN	*t = (TEXTWIN *)v->extra;
	long height;
	short oldoff;

	height = t->cheight * t->maxy - v->work.g_h;
	oldoff = t->offy;
	newyoff(t, (int)((vpos * height) / 1000L));
	oldoff -= t->offy;
	if (oldoff < 0)
		scrollup(t, -oldoff);
	else
		scrolldn(t, oldoff);
}

/*
 * correctly set up the horizontal and vertical scroll bars for TEXTWIN t
 */
static void set_scroll_bars(TEXTWIN *t)
{
	WINDOW	*v = t->win;
	short vsize;
	short vpos;
	long width, height;

	width = t->cmaxwidth * NCOLS (t);
	height = t->cheight * t->maxy;

	/* see if the new offset is too big for the window */
	if (t->offy + v->work.g_h > height)
		t->offy = (int)(height - v->work.g_h);
	if (t->offy < 0) 
		t->offy = 0;

	vsize = (int)(1000L * v->work.g_h / height);
	if (vsize > 1000) 
		vsize = 1000;
	else if (vsize < 1) 
		vsize = 1;

	if (height > v->work.g_h)
		vpos = (int)(1000L * t->offy / (height - v->work.g_h));
	else
		vpos = 1;

	if (vpos < 1) 
		vpos = 1;
	else if (vpos > 1000) 
		vpos = 1000;

	if (v->kind & VSLIDE) 
	{
		wind_set(v->handle, WF_VSLIDE, vpos, 0, 0, 0);
		wind_set(v->handle, WF_VSLSIZE, vsize, 0, 0, 0);
	}
}


/*
 * text_type: type the user's input into a window. Note that this routine is
 * called when doing a 'paste' operation, so we must watch out for possible
 * deadlock conditions
 */
#define READBUFSIZ 256
static char buf[READBUFSIZ];

static bool text_type(WINDOW *w, short code, short shift)
{
	TEXTWIN	*t = w->extra;
	WINCFG	*cfg = t->cfg;
	long offset, height;
	long c = (code & 0x00ff) | (((long)code & 0x0000ff00L) << 8L) | ((long)shift << 24L);
	long r;
	
	/* Context-sensitive help */
	if (code == 0x6200)		/* HELP */
	{
		char	selected[80];

		/*
		 * if there is a selected text region send it to ST-Guide and return,
		 * else send the HELP key to the program running in the window
		*/
		copy(w, selected, 80);
		if (strlen(selected) > 0)
		{
			call_stguide(selected, FALSE);
			return FALSE;
		}
	}

	/* no input on the console */
	if (con_fd > 0 && t->fd == con_fd)
		return FALSE;

	if (cfg->char_tab != TAB_ATARI)
	{
		int	asc;

		asc = (int) c & 0x00FF;
		switch (cfg->char_tab)
		{
			case TAB_ISO :
				asc = (int) st_to_iso[asc];
				break;
			default:
				debug("text_type: unknown char_tab: %d\n", cfg->char_tab);
				break;
		}
		c = c & 0xFFFFFF00L;
		c = c | (long) asc;
	}
	
	if (t->miny) 
	{
		offset = t->miny * t->cheight;
		if (offset > t->offy) 
		{
			/* we were looking at scroll back */
			/* now move so the cursor is visible */
			height = t->cheight * t->maxy - w->work.g_h;
			if (height <= 0)
				offset = 0;
			else 
			{
				offset = 1000L * t->cy * t->cheight/height;
				if (offset > 1000L) 
					offset = 1000L;
			}
			(*w->vslid)(w, (int)offset);
		}
	}
	if (t->fd) 
	{
		r = Foutstat(t->fd);
		if (r <= 0) 
		{
			r = Fread(t->fd, (long)READBUFSIZ, buf);
			write_text(t, buf, r);
			(void)Fselect(500, 0L, 0L, 0L);
			r = Foutstat(t->fd);
		}
		if (r > 0) 
		{
			/* vt52 -> vt100 cursor/function key remapping */
			if (t->vt_mode == MODE_VT100 && code >= 0x3b00 && code <= 0x5000) 
			{
				(void)Fputchar(t->fd, (long)(0x0001001bL | ((long)shift << 24L)), 0);	  /* 'ESC' */
			  	switch (code) 
			  	{
					case 0x3b00: 	/* F1 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x00180050L | ((long)shift << 24L)), 0);	 /* 'P' */
						break;
					case 0x3c00: 	/* F2 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x00100051L | ((long)shift << 24L)), 0);	 /* 'Q' */
						break;
					case 0x3d00: 	/* F3 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x00130052L | ((long)shift << 24L)), 0);	 /* 'R' */
						break;
				  	case 0x3e00: 	/* F4 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x001f0053L | ((long)shift << 24L)), 0);	 /* 'S' */
						break;
				  	case 0x3f00: 	/* F5 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x00140054L | ((long)shift << 24L)), 0);	 /* 'T' */
						break;
				  	case 0x4000: 	/* F6 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x00160055L | ((long)shift << 24L)), 0);	 /* 'U' */
						break;
				  	case 0x4100: 	/* F7 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x002f0056L | ((long)shift << 24L)), 0);	 /* 'V' */
						break;
				  	case 0x4200: 	/* F8 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x00110057L | ((long)shift << 24L)), 0);	 /* 'W' */
						break;
				 	case 0x4300: 	/* F9 */
						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
						(void)Fputchar(t->fd, (long)(0x002d0058L | ((long)shift << 24L)), 0);	 /* 'X' */
						break;
				  	case 0x4400: 	/* F10 */
 						(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0);	 /* 'O' */
 						(void)Fputchar(t->fd, (long)(0x00150059L | ((long)shift << 24L)), 0);	 /* 'Y' */
 						break;
				  	case 0x4800: 	/* up arrow */
						switch (t->curs_mode) 
						{
							case CURSOR_NORMAL:
								(void)Fputchar(t->fd, (long)(0x001a005bL | ((long)shift << 24L)), 0); 	/* '[' */
								break;
							case CURSOR_TRANSMIT:
								(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0); 	/* 'O' */
								break;
						}
						(void)Fputchar(t->fd, (long)(0x001e0041L | ((long)shift << 24L)), 0);	 /* 'A' */
						break;
				  	case 0x4b00: 	/* left arrow */
						switch (t->curs_mode) 
						{
							case CURSOR_NORMAL:
								(void)Fputchar(t->fd, (long)(0x001a005bL | ((long)shift << 24L)), 0); 	/* '[' */
								break;
							case CURSOR_TRANSMIT:
								(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0); 	/* 'O' */
								break;
						}
						(void)Fputchar(t->fd, (long)(0x00200044L | ((long)shift << 24L)), 0);	 /* 'D' */
						break;
				  	case 0x4d00: 	/* right arrow */
						switch (t->curs_mode) 
						{
							case CURSOR_NORMAL:
								(void)Fputchar(t->fd, (long)(0x001a005bL | ((long)shift << 24L)), 0); 	/* '[' */
								break;
							case CURSOR_TRANSMIT:
								(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0); 	/* 'O' */
								break;
						}
						(void)Fputchar(t->fd, (long)(0x002e0043L | ((long)shift << 24L)), 0);	 /* 'C' */
						break;
				  	case 0x5000: 	/* down arrow */
						switch (t->curs_mode) 
						{
							case CURSOR_NORMAL:
								(void)Fputchar(t->fd, (long)(0x001a005bL | ((long)shift << 24L)), 0); 	/* '[' */
								break;
							case CURSOR_TRANSMIT:
								(void)Fputchar(t->fd, (long)(0x0018004fL | ((long)shift << 24L)), 0); 	/* 'O' */
								break;
						}
						(void)Fputchar(t->fd, (long)(0x00300042L | ((long)shift << 24L)), 0);	 /* 'B' */
						break;
				  	default:
						(void)Fputchar(t->fd, c, 0);
				}
			}
			else
#ifdef DEBUG
			{
				if (do_debug)
					syslog (LOG_ERR, "Writing code 0x%08x",
						(unsigned) c);
#endif
				(void)Fputchar(t->fd, c, 0);
#ifdef DEBUG
			}
#endif
			return TRUE;
		}
	}
	else
	{
		if ((char)code == '\r')			/* Return/Enter */
			(*w->closed)(w);
	}
	return FALSE;
}

/*
 * Auswertung der Mausklicks innerhalb eines Fensters.
*/
static bool text_click(WINDOW *w, short clicks, short x, short y, short kshift, short button)
{
	TEXTWIN	*t;
	short x1, y1, d;

	t = w->extra;

	if (button == 1)									/* left click */
	{
		/* convert to character coordinates */
		pixel2char(t, x, y, &x1, &y1);
		
		if (clicks == 2)								/* double click -> select word */
		{
			if (select_word(t, x1, y1))
				copy(w, NULL, 0);
		}
		else
		{
			graf_mkstate(&d, &d, &button, &kshift);
			if (button == 1)							/* hold down -> mark region */
			{
				if (t->cflag[y1][x1] & CSELECTED)
					drag_selection(t);
				else
				{
					if (select_text(t, x1, y1, kshift))
						copy(w, NULL, 0);
				}
			}
			else
			{
				if (t->cflag[y1][x1] & CSELECTED)	/* clicked on selected text */
				{
					char	sel[80];
					
					copy(w, sel, 80);
					send_to("STRNGSRV", sel);		/* send it to StringServer */
				}
				else
					unselect(t);						/* single click -> deselect all */
			}
		}			
	}
	else if (button == 2)							/* right click */
	{
		if (clicks == 1)								/* single click -> insert */
			paste(w);
	}
	return TRUE;
}

/*
 * Create a new text window with title t, w columns, and h rows,
 * and place it at x, y on the screen; the new window should have the
 * set of gadgets specified by "kind", and should provide "s"
 * lines of scrollback.
 */
TEXTWIN *create_textwin(char *title, WINCFG *cfg)
{
	WINDOW *v;
	TEXTWIN *t;
	short firstchar, lastchar, distances[5], maxwidth, effects[3];
	short i;
	ulong flag;
	
	t = malloc (sizeof *t);
	if (!t) 
		return NULL;
		
	memset (t, 0, sizeof *t);

	t->maxx = cfg->col;
	t->maxy = cfg->row + cfg->scroll;
	t->miny = cfg->scroll;

	t->vt_mode = cfg->vt_mode;
	if (cfg->vt_mode == MODE_VT100)
	{
		t->curs_mode = CURSOR_NORMAL;
		t->scroll_top = t->miny;
		t->scroll_bottom = t->maxy-1;
		t->tabs = NULL;
	}

	/* we get font data from the VDI */
	set_font(cfg->font_id, cfg->font_pts);
	vqt_fontinfo(vdi_handle, &firstchar, &lastchar, distances, &maxwidth, effects);
	t->cfont = cfg->font_id;
	t->cpoints = cfg->font_pts;
	t->cmaxwidth = maxwidth;
	t->cheight = distances[0]+distances[4]+1;
	t->cbase = distances[4];
	t->minADE = firstchar;
	t->maxADE = lastchar;
	t->cwidths = 0;
	set_cwidths(t);

	/* initialize the window data */
	t->alloc_width = NCOLS (t);
	t->alloc_height = t->maxy;
		
	t->data = malloc(sizeof(char *) * t->alloc_height);
	t->cflag = malloc(sizeof(void *) * t->alloc_height);
	t->dirty = malloc((size_t)t->alloc_height);

	if (!t->dirty || !t->cflag || !t->data)
		goto bail_out;

	t->vdi_colors = cfg->vdi_colors;
	t->fg_effects = cfg->fg_effects;
	t->bg_effects = cfg->bg_effects;

	t->cfg = cfg;
	original_colors (t);
	flag = t->term_cattr;		

	memset (t->dirty, 0, (sizeof t->dirty[0]) * t->maxy);
	
	for (i = 0; i < t->alloc_height; ++i) 
	{
		if (cfg->vdi_colors)
			flag = COLORS (cfg->fg_color, cfg->bg_color);
		
		t->data[i] = malloc ((size_t) t->alloc_width);
		t->cflag[i] = malloc(sizeof (long) * 
		              ((size_t) (t->alloc_width)));
		if (!t->cflag[i] || !t->data[i]) 
			goto bail_out;
		
		memset (t->data[i], ' ', t->alloc_width);
		
		if (i == 0)
			memulset (t->cflag[i], flag, t->alloc_width);
		else
			memcpy (t->cflag[i], t->cflag[0],
				t->alloc_width * sizeof t->cflag[0][0]);
	}

	t->scrolled = t->nbytes = t->draw_time = 0;

	/* initialize the WINDOW struct */
	v = create_window(title, cfg->kind, cfg->xpos, cfg->ypos, cfg->width, cfg->height, 
			  SHRT_MAX, SHRT_MAX);
	if (!v) 
	{
		/* FIXME: Cleanup for dirty, data, and cflag, too.  */
		free(t);
		return 0;
	}

	v->extra = t;
	t->win = v;

	if (t->vt_mode == MODE_VT100)
		reset_tabs (t);

	/* overwrite the methods for v */
	if (t->vt_mode == MODE_VT100)
		t->output = vt100_putch;
	else
		t->output = vt52_putch;

	v->draw = draw_textwin;
	v->closed = close_textwin;
	v->fulled = full_textwin;
	v->moved = move_textwin;
	v->sized = size_textwin;
	v->arrowed = arrow_textwin;
	v->vslid = vslid_textwin;

	v->keyinp = text_type;
	v->mouseinp = text_click;

	t->cx = 0;
	if (cfg->height != -1)
	{
		t->offy = t->maxy * t->cheight - v->work.g_h;
		t->cy = t->maxy - (v->work.g_h / t->cheight);
	}
	else
	{	
		t->offy = cfg->scroll * t->cheight;
		t->cy = t->miny;
	}

	t->fd = t->pgrp = 0;
	
	t->pty[0] = '\0';
	t->shell = NO_SHELL;

	t->prog = t->cmdlin = t->progdir = 0;
	
	t->block_x1 = 0;
	t->block_x2 = 0;
	t->block_y1 = 0;
	t->block_y2 = 0;
	
	t->savex = t->savey = 0;
	t->save_cattr = t->term_cattr;

	return t;

bail_out:
#if 0
	if (t->data && t->dirty && t->cflag) {
		for (i = 0; i < t->maxy; ++i) {
			if (t->data[i])
				free (t->data[i]);
			if (t->cflag[i]) 
				free (t->cflag[i]);
		}
	}
#endif
	if (t->data)
		free (t->data);
	if (t->dirty)
		free (t->dirty);
	if (t->cflag)
		free (t->cflag);
	return NULL;	
}

/*
 * destroy a text window
 */
void destroy_textwin(TEXTWIN *t)
{
	int i;

	destroy_window(t->win);
	for (i = 0; i < t->maxy; i++) 
	{
		free(t->data[i]);
		free(t->cflag[i]);
	}
	if (t->prog) 
		free(t->prog);
	if (t->cmdlin) 
		free(t->cmdlin);
	if (t->progdir) 
		free(t->progdir);

	free(t->cflag);
	free(t->data);
	free(t->dirty);
	if (t->cwidths)
		free(t->cwidths);
	
	/* Prozež korrekt abmelden */
	term_proc(t);

	free(t);
}


void textwin_term(void)
{
	WINDOW	*v;
	TEXTWIN	*t;
	
	v = gl_winlist;
	while (v) 
	{
		t = v->extra;
		term_proc(t);
		v = v->next;
	}
}

/*
 * reset a window's font: this involves resizing the window, too
 */
void textwin_setfont(TEXTWIN *t, short font, short points)
{
	WINDOW *w;
	short firstchar, lastchar, distances[5], maxwidth, effects[3];
	short width, height;
	short dummy;
	int reopen = 0;

	w = t->win;
	if (t->cfont == font && t->cpoints == points)
		return;		/* no real change happens */

	if (w->handle >= 0) 
	{
		wind_close(w->handle);
		wind_delete(w->handle);
		reopen = 1;
		gl_winanz--;
	}
	w->handle = -1;

	t->cfont = font;
	t->cpoints = points;
	set_font(font, points);
	vqt_fontinfo(vdi_handle, &firstchar, &lastchar, distances, &maxwidth, effects);
	t->cmaxwidth = maxwidth;
	t->cheight = distances[0]+distances[4]+1;
	t->cbase = distances[4];
	t->minADE = firstchar;
	t->maxADE = lastchar;
	set_cwidths(t);

	t->win->max_w = width = NCOLS(t) * t->cmaxwidth;
	t->win->max_h = height = NROWS(t) * t->cheight;

	wind_calc(WC_BORDER, w->kind, w->full.g_x, w->full.g_y, width, height, &dummy, &dummy, &w->full.g_w, &w->full.g_h);
	if (w->full.g_w > gl_desk.g_w) 
		w->full.g_w = gl_desk.g_w;
	if (w->full.g_h > gl_desk.g_h) 
		w->full.g_h = gl_desk.g_h;

	if (w->full.g_x + w->full.g_w > gl_desk.g_x + gl_desk.g_w)
		w->full.g_x = gl_desk.g_x + (gl_desk.g_w - w->full.g_w)/2;
	if (w->full.g_y + w->full.g_h > gl_desk.g_y + gl_desk.g_h)
		w->full.g_y = gl_desk.g_y + (gl_desk.g_h - w->full.g_h)/2;

	wind_calc(WC_WORK, w->kind, w->full.g_x,w->full.g_y, w->full.g_w, w->full.g_h, &dummy, &dummy, &w->work.g_w, &w->work.g_h);
	if (reopen)
		open_window(w, FALSE);
}

static void
emergency_close (TEXTWIN* tw) 
{
	close_textwin (tw->win);
	form_alert (1, "[3][ Virtual memory exhausted ][ OK ]");
}

/* Change scrollback buffer of new window to new
   amount of lines.  */
static void 
change_scrollback (TEXTWIN* tw, short scrollback)
{
	short diff = scrollback - tw->miny;

	if (diff == 0)
		return;
	else if (diff < 0) {
		unsigned char* tmp_data = alloca (-diff * sizeof tmp_data);
		unsigned long* tmp_cflag = alloca ((-diff) * sizeof tmp_cflag);
						   		
		/* Scrollback is becoming smaller.  We move the lines 
		   at the top to the bottom.  */
		
		memmove (tw->dirty, tw->dirty - diff, 
			 (sizeof *tw->dirty) * (tw->maxy + diff));
		
		memcpy (tmp_data, tw->data + tw->maxy + diff,
			(sizeof tmp_data) * (-diff));
		memcpy (tmp_cflag, tw->cflag + tw->maxy + diff,
			(sizeof tmp_cflag) * (-diff));

		memmove (tw->data, tw->data - diff, 
			 (sizeof tmp_data) * (tw->maxy + diff));
		memmove (tw->cflag, tw->cflag - diff, 
			 (sizeof tmp_cflag) * (tw->maxy + diff));

		memcpy (tw->data + tw->maxy + diff, tmp_data,
			(sizeof tmp_data) * (-diff));
		memcpy (tw->cflag + tw->maxy + diff, tmp_cflag,
			(sizeof tmp_cflag) * (-diff));
	} else {
		int i;
		/* Scrollback is becoming bigger.  */
		if (tw->maxy + diff > tw->alloc_height) {
			unsigned short saved_height = tw->alloc_height;
			
			size_t data_chunk;
			size_t cflag_chunk;		

			tw->alloc_height = tw->maxy + diff;
			data_chunk = (sizeof tw->data[0][0]) * 
			              tw->alloc_width;
			cflag_chunk = (sizeof tw->cflag[0][0]) * 
				      tw->alloc_width;
	
			tw->dirty = realloc (tw->dirty, (sizeof *tw->dirty) * 
					     tw->alloc_height);
			if (tw->dirty == NULL)
				goto bail_out;
			tw->data = realloc (tw->data, (sizeof *tw->data) *
					    tw->alloc_height);
			if (tw->data == NULL)
				goto bail_out;
			tw->cflag = realloc (tw->cflag, (sizeof *tw->cflag) *
					     tw->alloc_height);
			if (tw->cflag == NULL)
				goto bail_out;
			
			for (i = saved_height; i < tw->alloc_height; ++i) {
				tw->data[i] = malloc (data_chunk);
				tw->cflag[i] = malloc (cflag_chunk);
			
				if (tw->data[i] == NULL || tw->cflag[i] == NULL)
					goto bail_out;
			}
		}
		
		memmove (tw->dirty, tw->dirty + diff, 
			 (sizeof *tw->dirty) * tw->maxy);

		memset (tw->dirty, 0, 
		        (sizeof tw->dirty[0]) * tw->maxy);
		
		memmove (tw->data + diff, tw->data, 
			 (sizeof *tw->data) * tw->maxy);
		memmove (tw->cflag + diff, tw->cflag, 
			 (sizeof *tw->cflag) * tw->maxy);

		for (i = 0; i < diff; ++i) {
			memset (tw->data[i], ' ', (sizeof tw->data[0][0]) * NCOLS (tw));
			
			if (i == 0)
				memulset (tw->cflag[i], tw->term_cattr, 
					  NCOLS (tw));
			else
				memcpy (tw->cflag[i], tw->cflag[0], 
					(sizeof tw->cflag[0][0]) * 
					 NCOLS (tw));
		}

	}

	tw->miny += diff;
	tw->maxy += diff;
	tw->cy += diff;
	
	/* Reset scrolling region.  */
	tw->scroll_top += diff;
	tw->scroll_bottom += diff;

	tw->offy += diff * tw->cheight;
	
	set_scroll_bars (tw);
	
	return;
	
bail_out:
	emergency_close (tw);
	
}

/* Change number of visible lines in window.  */
static void 
change_height (TEXTWIN* tw, short rows)
{
	short diff = rows - NROWS (tw);
	
	if (diff == 0)
		return;
	else if (diff > 0) {
		int i;

		/* We need to enlarge the buffer.  */
		if (tw->maxy + diff > tw->alloc_height) {
			unsigned short saved_height = tw->alloc_height;
			size_t data_chunk;
			size_t cflag_chunk;		

			tw->alloc_height = tw->maxy + diff;
			data_chunk = (sizeof tw->data[0][0]) * 
			             tw->alloc_width;
			cflag_chunk = (sizeof tw->cflag[0][0]) * 
				      tw->alloc_width;
			
			tw->dirty = realloc (tw->dirty, (sizeof *tw->dirty) * 
					     tw->alloc_height);
			if (tw->dirty == NULL)
				goto bail_out;
			tw->data = realloc (tw->data, (sizeof *tw->data) *
					    tw->alloc_height);
			if (tw->data == NULL)
				goto bail_out;
			tw->cflag = realloc (tw->cflag, (sizeof *tw->cflag) *
					     tw->alloc_height);
			if (tw->cflag == NULL)
				goto bail_out;
			
			for (i = saved_height; i < tw->alloc_height; ++i) {
				tw->data[i] = malloc (data_chunk);
				tw->cflag[i] = malloc (cflag_chunk);
			
				if (tw->data[i] == NULL || tw->cflag[i] == NULL)
					goto bail_out;
			}
		}
		
		for (i = tw->maxy; i < tw->maxy + diff; ++i) {
			memset (tw->data[i], ' ', (sizeof tw->data[0][0]) * NCOLS (tw));
			
			if (i == tw->maxy)
				memulset (tw->cflag[i], tw->term_cattr, 
					  NCOLS (tw));
			else
				memcpy (tw->cflag[i], tw->cflag[tw->maxy], 
					(sizeof tw->cflag[tw->maxy][0]) * 
					 NCOLS (tw));
		}

	        memset (tw->dirty + tw->maxy, ALLDIRTY, diff);
	}

	tw->maxy += diff;
	if (tw->cy > tw->maxy)
		tw->cy = tw->maxy;
	
	/* This may screw up curses programs but it will not
	   break anything.  */
	tw->scroll_bottom += diff;
	if (tw->scroll_top >= tw->scroll_bottom)
		tw->scroll_top = tw->miny;
		
	/* Dunno why this is necessary.  */
	tw->offy += diff * tw->cheight;
	
	set_scroll_bars (tw);
		
	return;
	
bail_out:
	emergency_close (tw);
	
}

static void
change_width (TEXTWIN* tw, short cols)
{
	short diff = cols - NCOLS (tw);
	
	if (diff == 0)
		return;
	else if (diff > 0) {
		int i;
		unsigned long flag = tw->term_cattr | CDIRTY | CTOUCHED;
		int old_cols = NCOLS (tw);
		int new_cols = old_cols + diff;
		
		/* We need to enlarge the buffer.  */
		if (new_cols + 1 > tw->alloc_width) {
			size_t data_chunk;
			size_t cflag_chunk;		

			tw->alloc_width = tw->maxx + diff;
			data_chunk = (sizeof tw->data[0][0]) * tw->alloc_width;
			cflag_chunk = (sizeof tw->cflag[0][0]) * tw->alloc_width;
			
			for (i = 0; i < tw->alloc_height; ++i) {
				tw->data[i] = realloc (tw->data[i], data_chunk);
				tw->cflag[i] = realloc (tw->cflag[i], cflag_chunk);
			
				if (tw->data[i] == NULL || tw->cflag[i] == NULL)
					goto bail_out;
			}
		}
		
		/* Initialize the freshly exposed columns.  */
		for (i = 0; i < tw->maxy; ++i) {
			memset (tw->data[i] + old_cols, ' ', diff);
			
			if (i == 0)
				memulset (tw->cflag[i] + old_cols, 
					  flag, diff);
			else
				memcpy (tw->cflag[i] + old_cols, 
					tw->cflag[0] + old_cols, 
					(sizeof tw->cflag[0][0]) * 
					 diff);
		}
	        memset (tw->dirty, SOMEDIRTY, tw->maxy);
	}

	tw->maxx += diff;
	if (tw->cx >= NCOLS (tw))
		tw->cx = NCOLS (tw) - 1;
	
	return;
	
bail_out:
	emergency_close (tw);
	
}

/* 
 * make a text window have a new number of rows and columns, and
 * a new amount of scrollback
 */
void 
resize_textwin (TEXTWIN* tw, short cols, short rows, short scrollback)
{
	int changed = 0;
	
	if (NCOLS (tw) == cols && SCROLLBACK (tw) == scrollback && 
	    tw->maxy == rows + scrollback)
		return;		/* no change */

	if (NCOLS (tw) != cols && (changed = 1))
		change_width (tw, cols);
		
	if (NROWS (tw) != rows && (changed = 1))
		change_height (tw, rows);
		
	if (tw->miny != scrollback)
		change_scrollback (tw, scrollback);
	
	if (changed) {
		GRECT border;
		WINDOW* win = tw->win;
			
		tw->cfg->col = NCOLS (tw);
		tw->cfg->row = NROWS (tw);
		tw->cfg->scroll = tw->miny;
	
		win->work.g_w = NCOLS (tw) * tw->cmaxwidth;
		win->work.g_h = NROWS (tw) * tw->cheight;

		wind_calc (WC_BORDER, win->kind, 
			   win->work.g_x, win->work.g_y,
			   win->work.g_w, win->work.g_h, 
		   	   &border.g_x, &border.g_y, 
		   	   &border.g_w, &border.g_h);
		wind_set (win->handle, WF_CURRXYWH, 
			  border.g_x, border.g_y,
		  	  border.g_w, border.g_h);

		notify_winch (tw);
	}

	return;
}

static void
reread_size (TEXTWIN* tw) 
{
	int rows = tw->win->work.g_h / tw->cheight;
	int cols = tw->win->work.g_w / tw->cmaxwidth;
	int changed = 0;
	
	if (cols != NCOLS (tw) && (changed = 1))
		change_width (tw, cols);
	if (rows != NROWS (tw) && (changed = 1))
		change_height (tw, rows);
	
	tw->cfg->col = cols;
	tw->cfg->row = rows;
	
	if (changed)
		notify_winch (tw);
}

void
notify_winch (TEXTWIN* tw)
{
	struct winsize ws;
	
	ws.ws_row = NROWS (tw);
	ws.ws_col = NCOLS (tw);
	ws.ws_xpixel = ws.ws_ypixel = 0;
	(void) Fcntl (tw->fd, &ws, TIOCSWINSZ);
	(void) Pkill (-(tw->pgrp), SIGWINCH);	
}

void 
reconfig_textwin(TEXTWIN *t, WINCFG *cfg)
{
	int i;

	curs_off(t);
	change_window_gadgets(t->win, cfg->kind);
	if (cfg->title[0] != '\0')
		title_window(t->win, cfg->title);
	resize_textwin(t, cfg->col, cfg->row, cfg->scroll);
	textwin_setfont(t, cfg->font_id, cfg->font_pts);

	original_colors (t);

	for (i = 0; i < t->maxy; i++) 
	{
		t->dirty[i] = ALLDIRTY;
		memulset (t->cflag[i], COLORS (cfg->fg_color, cfg->bg_color),
			  NCOLS (t));
	}
	curs_on(t);

	refresh_textwin(t, TRUE);

	/* cfg->vt_mode wird bewužt ignoriert -> wirkt erst bei neuem Fenster */
}

/* set the "cwidths" array for the given window correctly;
 * this function may be called ONLY when the font & height are already
 * set correctly, and only after t->cmaxwidth is set
 */
static void set_cwidths(TEXTWIN *t)
{
	short i, status, dummy, wide;
	short widths[256];
	short monospaced = 1;
	short dfltwide;

	if (t->cwidths) 
	{
		free(t->cwidths);
		t->cwidths = 0;
	}
	vqt_width(vdi_handle, '?', &dfltwide, &dummy, &dummy);

	for (i = 0; i < 255; i++) 
	{
		status = vqt_width(vdi_handle, i, &wide, &dummy, &dummy);
		if (status == -1) 
			wide = dfltwide;
		if (wide != t->cmaxwidth)
			monospaced = 0;
		widths[i] = wide;
	}
	if (!monospaced) 
	{
		t->cwidths = malloc(256 * sizeof(short));
		if (!t->cwidths) 
			return;
		for (i = 0; i < 255; i++)
			t->cwidths[i] = widths[i];
	}
}


/*
 * output a string to text window t, just as though the user typed it.
 */
void sendstr(TEXTWIN *t, char *s)
{
	long c;

	if (t->fd > 0 && t->fd != con_fd)
	{
		while (*s) 
		{
			c = *((unsigned char *)s);
			s++;
			(void)Fputchar(t->fd, c, 0);
		}
	}
}

/*
 * After this many bytes have been written to a window, it's time to
 * update it. Note that we must also keep a timer and update all windows
 * when the timer expires, or else small writes will never be shown!
 */
#define THRESHOLD 400

void write_text(TEXTWIN *t, char *b, long len)
{
	unsigned char *src = (unsigned char *) b, c;
	int limit = NROWS(t) - 1;
	long cnt;
	extern int draw_ticks;
	if (len == -1)
		cnt = strlen(b);
	else
		cnt = len;

	while (cnt-- > 0) 
	{
		c = *src++;
		if (c != '\0')
		{
			(*t->output)(t, c);
			t->nbytes++;
			/* The number of scrolled lines is ignored 
			   as an experimental feature.  There is no
			   point in artificially pulling a break 
			   when output is too fast too read.  
			   This may look strange but it enhance the
			   usability and subjective performance.  */
			if ((draw_ticks > MAX_DRAW_TICKS && 
			     t->nbytes >= THRESHOLD) 
			    || (0 && t->scrolled >= limit))
				refresh_textwin(t, FALSE);
		}
	}
}

/*
 * Sucht ein Fenster zu handle bzw. talkID.
*/
TEXTWIN *get_textwin(short handle, short talkID)
{
	WINDOW *w;
	TEXTWIN	*t;
	
	if (handle < 0 || talkID < 0) 
		return NULL;
	
	for (w = gl_winlist; w; w = w->next)
	{
		if (handle > 0 && w->handle == handle)
			return w->extra;
		t = w->extra;
		if (talkID > 0 && t->talkID == talkID)
			return t;
	}
	return NULL;
}
