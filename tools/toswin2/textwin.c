
#include <mintbind.h>
#include <signal.h>
#include <sys/ioctl.h>

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


#define CLEARED 2			/* redrawing a cleared area */

/*
 * lokale Variablen 
*/
static MFDB scr_mfdb;	/* left NULL so it refers to the screen by default */

/*
 * lokale Prototypen
*/
static void draw_buf(TEXTWIN *t, char *buf, short x, short y, short flag, short force);
static void update_chars(TEXTWIN *t, short firstcol, short lastcol, short firstline, short lastline, short force);
static void update_screen(TEXTWIN *t, short xc, short yc, short wc, short hc, short force);
static void draw_textwin(WINDOW *v, short x, short y, short w, short h);
static void close_textwin(WINDOW *v);
static void full_textwin(WINDOW *v);
static void move_textwin(WINDOW *v, short x, short y, short w, short h);
static void size_textwin(WINDOW *v, short x, short y, short w, short h);
static void newxoff(TEXTWIN *t, short x);
static void newyoff(TEXTWIN *t, short y);
static void scrollupdn(TEXTWIN *t, short off, short direction);
static void scrollleftright(TEXTWIN *t, short off, short direction);
static void arrow_textwin(WINDOW *v, short msg);
static void hslid_textwin(WINDOW *v, short hpos);
static void vslid_textwin(WINDOW *v, short vpos);
static void set_scroll_bars(TEXTWIN *t);
static void set_cwidths(TEXTWIN *t);
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
		*xp = t->win->work.g_x - t->offx + col * t->cmaxwidth;
	}
	else 
	{	
		if (col >= t->maxx) 
			*xp = t->win->work.g_x + t->win->work.g_w;
		else 
		{
			x = t->win->work.g_x - t->offx;
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
	x = x - t->win->work.g_x + t->offx;

	if (WIDE == 0) 
		col = x / t->cmaxwidth;
	else 
	{
		count = 0;
		for (col = 0; col < t->maxx - 1; col++) 
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


/*
 * draw a (part of a) line on screen, with certain attributes (e.g.
 * inverse video) indicated by "flag". (x, y) is the upper left corner
 * of the box which will contain the line.
 * If "force" is 1, we may assume that the screen is already cleared
 * (this is done in update_screen() for us).
 * SPECIAL CASE: if buf is an empty string, we clear from "x" to
 * the end of the window.
 */
static void draw_buf(TEXTWIN *t, char *buf, short x, short y, short flag, short force)
{
	char *s, *lastnonblank;
	int x2, fillcolor, textcolor;
	int texteffects;
	short *WIDE = t->cwidths;
	short temp[4];

	fillcolor = flag & CBGCOL;
	textcolor = (flag & CFGCOL) >> 4;
	texteffects = (flag & CEFFECTS) >> 8;
	if (flag & (CINVERSE|CSELECTED)) 
	{	
		x2 = fillcolor; 
		fillcolor = textcolor; 
		textcolor = x2;
	}
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
		set_textcolor(textcolor);
		set_texteffects(texteffects);
		v_gtext(vdi_handle, x, y + t->cbase, buf);
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
	short px, py, ax, i, cnt, flag, bufwidth;
	short *WIDE = t->cwidths;
	short lineforce = 0;
	short curflag;

#define flushbuf()	\
	{ 	buf[i] = 0;	\
			draw_buf(t, buf, px, py, flag, lineforce); \
			px += bufwidth; \
			i = bufwidth = 0; \
	}

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
				/* watch out for characters that can't be drawn in this font */
				if (c < t->minADE || c > t->maxADE)
					c = '?';
				curflag = t->cflag[firstline][cnt] & ~(CDIRTY|CTOUCHED);
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
			draw_buf(t, "", px, py, t->cflag[firstline][t->maxx-1], lineforce);
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
		for (col = 0; col < t->maxx; col++) 
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

	/* if `force' is set, clear the area to be redrawn -- it looks better */
	if (force == CLEARED) 
	{
		set_fillcolor(t->cfg->bg_color);
		set_fillstyle(1, 1);
		pxy[0] = xc;
		pxy[1] = yc;
		pxy[2] = xc + wc - 1;
		pxy[3] = yc + hc - 1;
		vr_recfl(vdi_handle, pxy);
	}

	/* convert from on-screen coordinates to window rows & columns */
	pixel2char(t, xc, yc, &firstcol, &firstline);

	if (firstline < 0) 
		firstline = 0;
	else if (firstline >= t->maxy) 
		firstline = t->maxy - 1;

	lastline = 1 + firstline + (hc + t->cheight - 1) / t->cheight;
	if (lastline > t->maxy) 
		lastline = t->maxy;

	/* kludge for proportional fonts */
	if (t->cwidths) 
	{
		firstcol = 0;
		lastcol = t->maxx;
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
			set_clipping(vdi_handle, t1.g_x, t1.g_y, t1.g_w, t1.g_h, TRUE);
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
}

/* resize a window */
static void move_textwin(WINDOW *v, short x, short y, short w, short h)
{
	GRECT full;
	TEXTWIN	*t = v->extra;
	
	wind_get_grect(v->handle, WF_FULLXYWH, &full);

	if (w > full.g_w) 
		w = full.g_w;
	if (h > full.g_h) 
		h = full.g_h;

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
}

static void size_textwin(WINDOW *v, short x, short y, short w, short h)
{
	TEXTWIN *t = v->extra;

	(*v->moved)(v, x, y, w, h);
	set_scroll_bars(t);
}

/*
 * handle an arrow event to a window
 */
static void newxoff(TEXTWIN *t, short x)
{
	t->offx = x;
	set_scroll_bars(t);
}

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
			set_clipping(vdi_handle, t1.g_x, t1.g_y, t1.g_w, t1.g_h, TRUE);
			if (off >= t1.g_h) 
				update_screen(t, t1.g_x, t1.g_y, t1.g_w, t1.g_h, TRUE);
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
					update_screen(t, t1.g_x, pxy[7], t1.g_w, off, TRUE);
				else
					update_screen(t, t1.g_x, t1.g_y, t1.g_w, off, TRUE);
			}
			}
		wind_get_grect(v->handle, WF_NEXTXYWH, &t1);
	}
	if (m_off)
		show_mouse();
	wind_update(FALSE);
}

#define LEFT 0
#define RIGHT 1

#define scrolllf(t, off) scrollleftright(t, off, LEFT)
#define scrollrt(t, off) scrollleftright(t, off, RIGHT)

static void scrollleftright(TEXTWIN *t, short off, short direction)
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
			set_clipping(vdi_handle, t1.g_x, t1.g_y, t1.g_w, t1.g_h, TRUE);
			if (off >= t1.g_w) 
				update_screen(t, t1.g_x, t1.g_y, t1.g_w, t1.g_h, TRUE);
			else 
			{
				if (direction == LEFT) 
				{
					pxy[0] = t1.g_x + off;	/* "from" address */
					pxy[1] = t1.g_y;
					pxy[2] = t1.g_x + t1.g_w - 1;
					pxy[3] = t1.g_y + t1.g_h - 1;
					pxy[4] = t1.g_x;	/* "to" address */
					pxy[5] = t1.g_y;
					pxy[6] = t1.g_x + t1.g_w - off - 1;
					pxy[7] = t1.g_y + t1.g_h - 1;
				} 
				else 
				{
					pxy[0] = t1.g_x;
					pxy[1] = t1.g_y;
					pxy[2] = t1.g_x + t1.g_w - off - 1;
					pxy[3] = t1.g_y + t1.g_h;
					pxy[4] = t1.g_x + off;
					pxy[5] = t1.g_y;
					pxy[6] = t1.g_x + t1.g_w - 1;
					pxy[7] = t1.g_y + t1.g_h - 1;
				}
				vro_cpyfm(vdi_handle, S_ONLY, pxy, &scr_mfdb, &scr_mfdb);
				if (direction == LEFT)
					update_screen(t, pxy[6], t1.g_y, off, t1.g_h, TRUE);
				else
					update_screen(t, t1.g_x, t1.g_y, off, t1.g_h, TRUE);
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
		case WA_LFPAGE:
			newxoff(t, t->offx - v->work.g_w);
			break;
		case WA_RTPAGE:
			newxoff(t, t->offx + v->work.g_w);
			break;
		case WA_LFLINE:
			oldoff = t->offx;
			newxoff(t, t->offx - t->cmaxwidth);
			scrollrt(t, oldoff - t->offx);
			return;
		case WA_RTLINE:
			oldoff = t->offx;
			newxoff(t, t->offx + t->cmaxwidth);
			scrolllf(t, t->offx - oldoff);
			return;
	}
	refresh_textwin(t, TRUE);
}

/*
 * handle horizontal and vertical slider events for a window
 */
static void hslid_textwin(WINDOW *v, short hpos)
{
	TEXTWIN	*t = (TEXTWIN *)v->extra;
	long width;
	short oldoff;

	width = t->cmaxwidth * t->maxx - v->work.g_w;
	oldoff = t->offx;
	newxoff(t, (int)((hpos * width) / 1000L));
	oldoff -= t->offx;
	if (oldoff < 0)
		scrolllf(t, -oldoff);
	else
		scrollrt(t, oldoff);
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
	short hsize, vsize;
	short hpos, vpos;
	long width, height;

	width = t->cmaxwidth * t->maxx;
	height = t->cheight * t->maxy;

	/* see if the new offset is too big for the window */
	if (t->offx + v->work.g_w > width)
		t->offx  = (int)(width - v->work.g_w);
	if (t->offx < 0) 
		t->offx = 0;

	if (t->offy + v->work.g_h > height)
		t->offy = (int)(height - v->work.g_h);
	if (t->offy < 0) 
		t->offy = 0;

	hsize = (int)(1000L * v->work.g_w / width);
	if (hsize > 1000) 
		hsize = 1000;
	else if (hsize < 1) 
		hsize = 1;

	vsize = (int)(1000L * v->work.g_h / height);
	if (vsize > 1000) 
		vsize = 1000;
	else if (vsize < 1) 
		vsize = 1;

	if (width > v->work.g_w)
		hpos = (int)(1000L * t->offx / (width - v->work.g_w));
	else
		hpos = 1;

	if (height > v->work.g_h)
		vpos = (int)(1000L * t->offy / (height - v->work.g_h));
	else
		vpos = 1;

	if (hpos < 1) 
		hpos = 1;
	else if (hpos > 1000) 
		hpos = 1000;

	if (vpos < 1) 
		vpos = 1;
	else if (vpos > 1000) 
		vpos = 1000;

	if (v->kind & HSLIDE) 
	{
		wind_set(v->handle, WF_HSLIDE, hpos, 0, 0, 0);
		wind_set(v->handle, WF_HSLSIZE, hsize, 0, 0, 0);
	}
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
				(void)Fputchar(t->fd, c, 0);
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
	short i, j;

	t = malloc(sizeof(TEXTWIN));
	if (!t) 
		return t;

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
		t->char_set = NORM_CHAR;
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
	t->data = malloc(sizeof(char *) * t->maxy);
	t->cflag = malloc(sizeof(short *) * t->maxy);
	t->dirty = malloc((size_t)t->maxy);

	if (!t->dirty || !t->cflag || !t->data) 
		return 0;

	for (i = 0; i < t->maxy; i++) 
	{
		t->dirty[i] = 0; /* the window starts off clear */
		t->data[i] = malloc((size_t)t->maxx+1);
		t->cflag[i] = malloc(sizeof(short) * (size_t)(t->maxx+1));
		if (!t->cflag[i] || !t->data[i]) 
			return 0;
		for (j = 0; j < t->maxx; j++) 
		{
			t->data[i][j] = ' ';
			t->cflag[i][j] = COLORS(cfg->fg_color, cfg->bg_color);
		}
	}

	t->scrolled = t->nbytes = t->draw_time = 0;

	/* initialize the WINDOW struct */
	v = create_window(title, cfg->kind, cfg->xpos, cfg->ypos, cfg->width, cfg->height, 
							t->maxx * t->cmaxwidth, cfg->row * t->cheight);
	if (!v) 
	{
		free(t);
		return 0;
	}

	v->extra = t;
	t->win = v;

	if (t->vt_mode == MODE_VT100)
		clearalltabs(t);

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
	v->hslid = hslid_textwin;
	v->vslid = vslid_textwin;

	v->keyinp = text_type;
	v->mouseinp = text_click;

	t->offx = 0;
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

	t->term_cattr = COLORS(cfg->fg_color, cfg->bg_color);
	if (cfg->wrap)
		t->term_flags = FWRAP;
	else
		t->term_flags = 0;
	t->fd = t->pgrp = 0;
	
	t->pty[0] = '\0';
	t->shell = NO_SHELL;
	t->cfg = cfg;

	t->prog = t->cmdlin = t->progdir = 0;
	
	t->block_x1 = 0;
	t->block_x2 = 0;
	t->block_y1 = 0;
	t->block_y2 = 0;
	
	return t;
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

/*
 * make a text window have a new number of rows and columns, and
 * a new amount of scrollback
 */
void resize_textwin(TEXTWIN *t, short cols, short rows, short scrollback)
{
	WINDOW *w = t->win;
	int i, j, mincols;
	int delta;
	unsigned char **newdata;
	short **newcflag;
	char *newdirty;
	short width, height, dummy;
	int reopen = 0;

	if (t->maxx == cols && t->miny == scrollback && t->maxy == rows + scrollback)
		return;		/* no change */
		
	newdata = malloc(sizeof(char *) * (rows+scrollback));
	newcflag = malloc(sizeof(short *) * (rows+scrollback));
	newdirty = malloc((size_t)(rows+scrollback));
	if (!newdata || !newcflag || !newdirty)
		return;

	mincols = (cols < t->maxx) ? cols : t->maxx;

	/* first, initialize the new data to blanks */
	for (i = 0; i < rows+scrollback; i++) 
	{
		newdirty[i] = 0;
		newdata[i] = malloc((size_t)cols+1);
		newcflag[i] = malloc(sizeof(short)*(cols+1));
		if (!newcflag[i] || !newdata[i])
			return;
		for(j = 0; j < cols; j++)
		{
			newdata[i][j] = ' '; 
			newcflag[i][j] = COLORS(t->cfg->fg_color, t->cfg->bg_color);
		}
	}

	/* now, copy as much scrollback as we can */
	if (rows+scrollback >= t->maxy) 
	{
		delta = rows+scrollback - t->maxy;
		for (i = 0; i < t->maxy;i++) 
		{
			for (j = 0; j < mincols; j++) 
			{
				newdata[i+delta][j] = t->data[i][j];
				newcflag[i+delta][j] = t->cflag[i][j];
			}
		}
	} 
	else 
	{
		delta = t->maxy - (rows+scrollback);
		for (i = 0; i < rows+scrollback; i++) 
		{
			for (j = 0; j < mincols; j++) 
			{
				newdata[i][j] = t->data[i+delta][j];
				newcflag[i][j] = t->cflag[i+delta][j];
			}
		}
	}

	/* finally, free the old data and flags */
	for(i=0; i < t->maxy; i++) 
	{
		free(t->data[i]); 
		free(t->cflag[i]);
	}
	free(t->dirty);
	free(t->cflag);
	free(t->data);

	t->dirty = newdirty;
	t->cflag = newcflag;
	t->data = newdata;
	t->cy = t->cy - SCROLLBACK(t) + scrollback;
	t->maxx = cols;
	t->maxy = rows+scrollback;
	t->miny = scrollback;

	/* reset scrolling region */
	t->scroll_top = t->miny;
	t->scroll_bottom = t->maxy -1 ;

	if (t->cx >= cols) 
		t->cx = 0;
	if (t->cy >= t->maxy) 
		t->cy = t->maxy-1;
	if (t->cy < scrollback) 
		t->cy = scrollback;
	if (t->offy < scrollback * t->cheight)
		t->offy = scrollback * t->cheight;

	if (w->handle >= 0) 
	{
		wind_close(w->handle);
		wind_delete(w->handle);
		reopen = 1;
		gl_winanz--;
	}
	w->handle = -1;

	t->win->max_w = width = t->maxx * t->cmaxwidth;
	t->win->max_h = height = rows * t->cheight;

	wind_calc(WC_BORDER, w->kind, w->full.g_x, w->full.g_y, width, height, &dummy, &dummy, &w->full.g_w, &w->full.g_h);
	if (w->full.g_w > gl_desk.g_w) 
		w->full.g_w = gl_desk.g_w;
	if (w->full.g_h > gl_desk.g_h) 
		w->full.g_h = gl_desk.g_h;

	if (w->full.g_x + w->full.g_w > gl_desk.g_x + gl_desk.g_w)
		w->full.g_x = gl_desk.g_x + (gl_desk.g_w - w->full.g_w)/2;
	if (w->full.g_y + w->full.g_h > gl_desk.g_y + gl_desk.g_h)
		w->full.g_y = gl_desk.g_y + (gl_desk.g_h - w->full.g_h)/2;

	wind_calc(WC_WORK, w->kind, w->full.g_x, w->full.g_y, w->full.g_w, w->full.g_h, &dummy, &dummy, &width, &height);

	if (w->work.g_w > width) 
		w->work.g_w = width;
	if (w->work.g_h > height) 
		w->work.g_h = height;

	if (reopen)
		open_window(w, FALSE);
}

void reconfig_textwin(TEXTWIN *t, WINCFG *cfg)
{
	struct winsize tw;
	int i, j;

	curs_off(t);
	change_window_gadgets(t->win, cfg->kind);
	if (cfg->title[0] != '\0')
		title_window(t->win, cfg->title);
	resize_textwin(t, cfg->col, cfg->row, cfg->scroll);
	textwin_setfont(t, cfg->font_id, cfg->font_pts);
	if (cfg->wrap)
		t->term_flags |= FWRAP;
	else
		t->term_flags &= ~FWRAP;

	t->term_cattr = COLORS(cfg->fg_color, cfg->bg_color);
	for (i = 0; i < t->maxy; i++) 
	{
		t->dirty[i] = ALLDIRTY;
		for (j = 0; j < t->maxx; j++) 
			t->cflag[i][j] = COLORS(cfg->fg_color, cfg->bg_color);
	}
	curs_on(t);

	refresh_textwin(t, TRUE);

	tw.ws_row = cfg->row;
	tw.ws_col = cfg->col;
	tw.ws_xpixel = tw.ws_ypixel = 0;
	(void)Fcntl(t->fd, &tw, TIOCSWINSZ);
	(void)Pkill(-t->pgrp, SIGWINCH);

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
	unsigned char *buf = (unsigned char *)b, c;
	int limit = NROWS(t) - 1;
	long cnt;

	if (len == -1)
		cnt = strlen(b);
	else
		cnt = len;
	while (cnt-- > 0) 
	{
		c = *buf++;
		if (c != '\0')
		{
			(*t->output)(t, c);
			t->nbytes++;
			if (t->nbytes >= THRESHOLD || t->scrolled >= limit)
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
