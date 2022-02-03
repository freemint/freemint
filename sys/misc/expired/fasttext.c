/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 */

# include "fasttext.h"

# include "arch/syscall.h"
# include "libkern/libkern.h"

# include "bios.h"
# include "dev-null.h"
# include "info.h"
# include "kmemory.h"
# include "time.h"


# ifdef FASTTEXT

#define CONDEV	(2)

static SCREEN *current;
static short scr_usecnt;

static void paint	(SCREEN *, int, char *);
static void paint8c	(SCREEN *, int, char *);
static void paint816m	(SCREEN *, int, char *);

INLINE void curs_off	(SCREEN *);
INLINE void curs_on	(SCREEN *);
INLINE void flash	(SCREEN *);
static void normal_putch(SCREEN *, int);
static void escy_putch	(SCREEN *, int);
static void quote_putch	(SCREEN *, int);

static char *chartab[256];

#define MAX_PLANES 8
static int fgmask[MAX_PLANES], bgmask[MAX_PLANES];

static long scrnsize;

short hardscroll;
static char *hardbase, *oldbase;

typedef void (*Vfunc) (SCREEN *, int);

#define base *((char **)0x44eL)
#define escy1 *((short *)0x4acL)

static Vfunc state;

static short hardline;
static void (*vpaint) (SCREEN *, int, char *);
static char *rowoff;

void fasttext_init (void);
void hardware_scroll (SCREEN *);
INLINE char *PLACE (SCREEN *, int, int);
INLINE void gotoxy (SCREEN *, int, int);
INLINE void clrline (SCREEN *, int);
INLINE void clear (SCREEN *);
INLINE void clrchar (SCREEN *, int, int);
INLINE void clrfrom (SCREEN *, int, int, int, int);
INLINE void delete_line (SCREEN *, int);
INLINE void insert_line (SCREEN *, int);
static void setbgcol (SCREEN *, int);
static void setfgcol (SCREEN *, int);
static void setcurs (SCREEN *, int);
static void putesc (SCREEN *, int);
static void escy1_putch (SCREEN *, int);
INLINE void put_ch (SCREEN *, int);

/* routines for flashing the cursor for screen v
 */

/* flash(v): invert the character currently under the cursor
 */
INLINE void
flash(SCREEN *v)
{
	char *place;
	long i, j, vplanes;

	vplanes = v->planes + v->planes;
	place = v->cursaddr;

	for (j = v->cheight; j > 0; --j) {
		for (i = 0; i < vplanes; i+=2)
			place[i] = ~place[i];

		place += v->planesiz;
	}
	v->curstimer = v->period;
}

/* make sure the cursor is off
 */
INLINE void
curs_off(SCREEN *v)
{
	if (v->flags & CURS_ON) {
		if (v->flags & CURS_FSTATE) {
			flash(v);
			v->flags &= ~CURS_FSTATE;
		}
	}
}

/* OK, show the cursor again (if appropriate)
 */
INLINE void
curs_on(SCREEN *v)
{
	if (v->hidecnt) return;

	if (v->flags & CURS_ON) {
#if 0
	/* if the cursor is flashing, we cheat a little and leave it off
	 * to be turned on again (if necessary) by the VBL routine
	 */
		if (v->flags & CURS_FLASH) {
			v->curstimer = 2;
			return;
		}
#endif
		if (!(v->flags & CURS_FSTATE)) {
			v->flags |= CURS_FSTATE;
			flash(v);
		}
	}
}

void
fasttext_init(void)
{
	SCREEN *v;
	int i, j;
	char *data, *foo;
	static char chardata[256*16];
	register int linelen;

	foo = lineA0();
	v = (SCREEN *)(foo - 346);
	
	/* Ehem... The screen might be bigger than 32767 bytes.
	   Let's do some casting... 
	   Erling
	*/
	linelen = v->linelen;
	scrnsize = (v->maxy+1)*(long)linelen;
	rowoff = kmalloc((long)((v->maxy+1) * sizeof(long)));
	if (rowoff == 0) {
		FATAL(ERR_ftxt_insuff_mem_for_table);
	} else {
		long off, *lptr = (long *)rowoff;
		for (i=0, off=0; i<=v->maxy; i++) {
			*lptr++ = off;
			off += linelen;
		}
	}
	if (hardscroll == -1) {
	/* request for auto-setting */
		hardscroll = v->maxy+1;
	}
	if (hardscroll > 0) {
		if (!hardbase)
		    hardbase = (char *)(((long)kcore(SCNSIZE(v)+256L)+255L)
					   & 0xffffff00L);

		if (hardbase == 0) {
			ALERT(MSG_ftxt_insuff_mem_for_scroll);
		} else {
			v->curstimer = 0x7f;
			quickmove(hardbase, base, scrnsize);
			v->cursaddr = v->cursaddr + (hardbase - base);
			oldbase = base;
			base = hardbase;
			Setscreen(hardbase, hardbase, -1);
			v->curstimer = v->period;
		}
	}
	hardline = 0;
	if (v->cheight == 8 && v->planes == 2) {
		foo = &chardata[0];
		vpaint = paint8c;
		for (i = 0; i < 256; i++) {
			chartab[i] = foo;
			data = v->fontdata + i;
			for (j = 0; j < 8; j++) {
				*foo++ = *data;
				data += v->form_width;
			}
		}
	} else if ((v->cheight == 16 || v->cheight == 8) && v->planes == 1) {
		foo = &chardata[0];
		vpaint = paint816m;
		for (i = 0; i < 256; i++) {
			chartab[i] = foo;
			data = v->fontdata + i;
			for (j = 0; j < v->cheight; j++) {
				*foo++ = *data;
				data += v->form_width;
			}
		}
	}
	else
		vpaint = paint;

	if (v->hidecnt == 0) {
	/*
	 * make sure the cursor is set up correctly and turned on
	 */
		(void)Cursconf(0,0);	/* turn cursor off */

		v->flags &= ~CURS_FSTATE;

	/* now turn the cursor on the way we like it */
		v->curstimer = v->period;
		v->hidecnt = 0;
		v->flags |= CURS_ON;
		curs_on(v);
	} else {
		(void)Cursconf(0,0);
		v->flags &= ~CURS_ON;
		v->hidecnt = 1;
	}

	current = v;
	/* setup bgmask and fgmask */
	setbgcol(v, v->bgcol);
	setfgcol(v, v->fgcol);
	state = normal_putch;
}

/*
 * PLACE(v, x, y): the address corresponding to the upper left hand corner of
 * the character at position (x,y) on screen v
 */
INLINE
char *PLACE(SCREEN *v, int x, int y)
{
	char *place;
	int i, j;

	place = base + x;
	if (y == v->maxy)
		place += scrnsize - v->linelen;
	else if (y) {
		y+=y;	/* Make Y into index for longword array. */
		y+=y;	/* Two word-size adds are faster than a 2-bit shift. */
		place += *(long *)(rowoff + y);
	}
	if ((j = v->planes-1)) {
		i = (x & 0xfffe);
		do place += i;
		while (--j);
	}
	return place;
}

/*
 * paint(v, c, place): put character 'c' at position 'place' on screen
 * v. It is assumed that x, y are proper coordinates!
 * Specialized versions (paint8c and paint816m) of this routine follow;
 * they assume 8 line high characters, medium res. and 8 or 16 line/mono,
 * respectively.
 */

static void
paint(SCREEN *v, int c, char *place)
{
	char *data, d, doinverse;
	long j, planecount;
	int vplanes;
	long vform_width, vplanesiz;

	vplanes = v->planes;

	data = v->fontdata + c;
	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	vform_width = v->form_width;
	vplanesiz = v->planesiz;

	for (j = v->cheight; j > 0; --j) {
		d = *data ^ doinverse;
		for (planecount = 0; planecount < vplanes; planecount++)
		  place[planecount << 1]
		    = ((d & (char) fgmask[planecount])
		       | (~d & (char) bgmask[planecount]));
		place += vplanesiz;
		data += vform_width;
	}
}

static void
paint8c(SCREEN *v, int c, char *place)
{
	char *data;
	char d, doinverse;
	char bg0, bg1, fg0, fg1;
	long vplanesiz;

	data = chartab[c];

	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	vplanesiz = v->planesiz;
	bg0 = bgmask[0];
	bg1 = bgmask[1];
	fg0 = fgmask[0];
	fg1 = fgmask[1];

	if (!doinverse && !bg0 && !bg1 && fg0 && fg1) {
		/* line 1 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 2 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 3 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 4 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 5 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 6 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 7 */
		d = *data++;
		*place = d;
		place[2] = d;
		place += vplanesiz;

		/* line 8 */
		d = *data;
		*place = d;
		place[2] = d;
	} else {
		/* line 1 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 2 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 3 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 4 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 5 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 6 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 7 */
		d = *data++ ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
		place += vplanesiz;

		/* line 8 */
		d = *data ^ doinverse;
		*place = ((d & fg0) | (~d & bg0));
		place[2] = ((d & fg1) | (~d & bg1));
	}
}

static void
paint816m(SCREEN *v, int c, char *place)
{
	char *data;
	char d, doinverse;
	long vplanesiz;

	data = chartab[c];
	doinverse = (v->flags & FINVERSE) ? 0xff : 0;
	doinverse ^= bgmask[0];
	vplanesiz = v->planesiz;

	if (bgmask[0] == fgmask[0])
	  {
	    /* fgcol and bgcol are the same -- easy */
	    d = (char) bgmask[0];
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    if (v->cheight == 8)
		return;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	    place += vplanesiz;
	    *place = d;
	  }
	else if (!doinverse) {
		/* line 1 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 2 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 3 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 4 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 5 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 6 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 7 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 8 */
		d = *data++;
		*place = d;

		if (v->cheight == 8)
			return;

		place += vplanesiz;

		/* line 9 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 10 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 11 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 12 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 13 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 14 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 15 */
		d = *data++;
		*place = d;
		place += vplanesiz;

		/* line 16 */
		d = *data;
		*place = d;
	} else {
		/* line 1 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 2 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 3 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 4 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 5 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 6 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 7 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 8 */
		d = ~*data++;
		*place = d;

		if (v->cheight == 8)
			return;

		place += vplanesiz;

		/* line 9 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 10 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 11 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 12 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 13 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 14 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 15 */
		d = ~*data++;
		*place = d;
		place += vplanesiz;

		/* line 16 */
		d = ~*data;
		*place = d;
	}
}

/*
 * gotoxy (v, x, y): move current cursor address of screen v to (x, y)
 * makes sure that (x, y) will be legal
 */

INLINE void
gotoxy(SCREEN *v, int x, int y)
{
	if (x > v->maxx) x = v->maxx;
	else if (x < 0) x = 0;
	if (y > v->maxy) y = v->maxy;
	else if (y < 0) y = 0;

	v->cx = x;
	v->cy = y;
	v->cursaddr = PLACE(v, x, y);
}

/*
 * clrline(v, r): clear line r of screen v
 */

INLINE void
clrline(SCREEN *v, int r)
{
	int *dst, *m;
	long nwords;
	int i, vplanes;

	/* Hey, again the screen might be bigger than 32767 bytes.
	   Do another cast... */
	r += r;
	r += r;
	dst = (int *)(base + *(long *)(rowoff+r));
	if (v->bgcol == 0)
	  bzero(dst, v->linelen);
	else
	  {
	    /* do it the hard way */
	    vplanes = v->planes;
	    for (nwords = v->linelen >> 1; nwords > 0; nwords -= vplanes)
	      {
		m = bgmask;
		for (i = 0; i < vplanes; i++)
		  *dst++ = *m++;
	      }
	  }
}
	
/*
 * clear(v): clear the whole screen v
 */

INLINE void
clear(SCREEN *v)
{
	int i, vplanes;
	int *dst, *m;
	long nwords;

	if (v->bgcol == 0)
	  bzero(base, scrnsize);
	else
	  {
	    /* do it the hard way */
	    dst = (int *) base;
	    vplanes = v->planes;
	    for (nwords = scrnsize >> 1; nwords > 0; nwords -= vplanes)
	      {
		m = bgmask;
		for (i = 0; i < vplanes; i++)
		  *dst++ = *m++;
	      }
	  }
}

/*
 * clrchar(v, x, y): clear the (x,y) position on screen v
 */

INLINE void
clrchar(SCREEN *v, int x, int y)
{
	int i, j, vplanes;
	char *place;
	int *m;

	vplanes = v->planes + v->planes;

	place = PLACE(v, x, y);

	for (j = v->cheight; j > 0; --j) {
		m = bgmask;
		for (i = 0; i < vplanes; i += 2)
			place[i] = (char) *m++;
		place += v->planesiz;
	}
}

/*
 * clrfrom(v, x1, y1, x2, y2): clear screen v from position (x1,y1) to
 * position (x2, y2) inclusive. It is assumed that y2 >= y1.
 */

INLINE void
clrfrom(SCREEN *v, int x1, int y1, int x2, int y2)
{
	int i;

	for (i = x1; i <= v->maxx; i++)
		clrchar(v, i, y1);
	if (y2 > y1) {
		for (i = 0; i <= x2; i++)
			clrchar(v, i, y2);
		for (i = y1+1; i < y2; i++)
			clrline(v, i);
	}
}

/*
 * scroll a screen in hardware; if we still have hardware scrolling lines left,
 * just move the physical screen base, otherwise copy the screen back to the
 * hardware base and start over
 */
void
hardware_scroll(SCREEN *v)
{

	++hardline;
	if (hardline < hardscroll) { /* just move the screen */
		base += v->linelen;
	} else {
		hardline = 0;
		quickmove(hardbase, base + v->linelen, scrnsize - v->linelen);
		base = hardbase;
	}
	v->cursaddr = PLACE(v, v->cx, v->cy);
	Setscreen(base, base, -1);
}

/*
 * delete_line(v, r): delete line r of screen v. The screen below this
 * line is scrolled up, and the bottom line is cleared.
 */

#define scroll(v) delete_line(v, 0)

INLINE void
delete_line(SCREEN *v, int r)
{
	long *src, *dst, nbytes;

	if (r == 0) {
		if (hardbase) {
			hardware_scroll(v);
			clrline(v, v->maxy);
			return;
		}
		nbytes = scrnsize - v->linelen;
	} else {
		register int i = v->maxy - r;
		i += i;
		i += i;
		nbytes = *(long *)(rowoff+i);
	}

	/* Sheeze, how many times do we really have to cast... 
	   Erling.	
	*/

	r += r;
	r += r;
	dst = (long *)(base + *(long *)(rowoff + r));
	src = (long *)( ((long)dst) + v->linelen);

	quickmove(dst, src, nbytes);

/* clear the last line */
	clrline(v, v->maxy);
}

/*
 * insert_line(v, r): scroll all of the screen starting at line r down,
 * and then clear line r.
 */

INLINE void
insert_line(SCREEN *v, int r)
{
	long *src, *dst;
	int i, j, linelen;

	i = v->maxy - 1;
	i += i;
	i += i;
	j = r+r;
	j += j;
	linelen = v->linelen;
	src = (long *)(base + *(long *)(rowoff + i));
	dst = (long *)((long)src + linelen);
	for (; i >= j ; i -= 4) {
	/* move line i to line i+1 */
		quickmove(dst, src, linelen);
		dst = src;
		src = (long *)((long) src - linelen);
	}

/* clear line r */
	clrline(v, r);
}

/*
 * special states for handling ESC b x and ESC c x. Note that for now,
 * color is ignored.
 */

static void
setbgcol(SCREEN *v, int c)
{
	int i;

	v->bgcol = c & ((1 << v->planes)-1);
	for (i = 0; i < v->planes; i++)
	    bgmask[i] = (v->bgcol & (1 << i)) ? -1 : 0;
	state = normal_putch;
}

static void
setfgcol(SCREEN *v, int c)
{
	int i;

	v->fgcol = c & ((1 << v->planes)-1);
	for (i = 0; i < v->planes; i++)
	    fgmask[i] = (v->fgcol & (1 << i)) ? -1 : 0;
	state = normal_putch;
}

static void
setcurs(SCREEN *v, int c)
{
	c -= ' ';
	if (!c) {
		v->flags &= ~CURS_FLASH;
	} else {
		v->flags |= CURS_FLASH;
		v->period = (unsigned char) c;
	}
	state = normal_putch;
}

static void
quote_putch(SCREEN *v, int c)
{
	(*vpaint)(v, c, v->cursaddr);
	state = normal_putch;
}

/*
 * putesc(v, c): handle the control sequence ESC c
 */

static void
putesc(SCREEN *v, int c)
{
	int i;
	int cx, cy;

	cx = v->cx; cy = v->cy;

	switch (c) {
	case 'A':		/* cursor up */
		if (cy) {
moveup:			v->cy = --cy;
			v->cursaddr -= v->linelen;
		}
		break;
	case 'B':		/* cursor down */
		if (cy < v->maxy) {
			v->cy = ++cy;
			v->cursaddr += v->linelen;
		}
		break;
	case 'C':		/* cursor right */
		if (cx < v->maxx) {
			if ((i = v->planes-1) && (cx & 1))
				v->cursaddr += i + i;
			v->cx = ++cx;
			v->cursaddr++;
		}
		break;
	case 'D':		/* cursor left */
		if (cx) {
			v->cx = --cx;
			v->cursaddr--;
			if ((i = v->planes-1) && (cx & 1))
				v->cursaddr -= i + i;
		}
		break;
	case 'E':		/* clear home */
		clear(v);
		/* fall through... */
	case 'H':		/* cursor home */
		v->cx = 0; v->cy = 0;
		v->cursaddr = base;
		break;
	case 'I':		/* cursor up, insert line */
		if (cy == 0) {
			insert_line(v, 0);
		}
		else
			goto moveup;
		break;
	case 'J':		/* clear below cursor */
		clrfrom(v, cx, cy, v->maxx, v->maxy);
		break;
	case 'K':		/* clear remainder of line */
		clrfrom(v, cx, cy, v->maxx, cy);
		break;
	case 'L':		/* insert a line */
		v->cx = 0;
		i = cy + cy;
		i += i;
		v->cursaddr = base + *(long *)(rowoff + i);
		insert_line(v, cy);
		break;
	case 'M':		/* delete line */
		v->cx = 0;
		i = cy + cy;
		i += i;
		v->cursaddr = base + *(long *)(rowoff + i);
		delete_line(v, cy);
		break;
	case 'Q':		/* EXTENSION: quote-next-char */
		state = quote_putch;
		return;
	case 'Y':
		state = escy_putch;
		return;		/* YES, this should be 'return' */

	case 'b':
		state = setfgcol;
		return;
	case 'c':
		state = setbgcol;
		return;
	case 'd':		/* clear to cursor position */
		clrfrom(v, 0, 0, cx, cy);
		break;
	case 'e':		/* enable cursor */
		v->flags |= CURS_ON;
		v->hidecnt = 1;	/* so --v->hidecnt shows the cursor */
		break;
	case 'f':		/* cursor off */
		v->hidecnt++;
		v->flags &= ~CURS_ON;
		break;
	case 'j':		/* save cursor position */
		v->savex = v->cx;
		v->savey = v->cy;
		break;
	case 'k':		/* restore saved position */
		gotoxy(v, v->savex, v->savey);
		break;
	case 'l':		/* clear line */
		v->cx = 0;
		i = cy + cy;
		i += i;
		v->cursaddr = base + *(long *)(rowoff + i);
		clrline(v, cy);
		break;
	case 'o':		/* clear from start of line to cursor */
		clrfrom(v, 0, cy, cx, cy);
		break;
	case 'p':		/* reverse video on */
		v->flags |= FINVERSE;
		break;
	case 'q':		/* reverse video off */
		v->flags &= ~FINVERSE;
		break;
	case 't':		/* EXTENSION: set cursor flash rate */
		state = setcurs;
		return;
	case 'v':		/* wrap on */
		v->flags |= FWRAP;
		break;
	case 'w':
		v->flags &= ~FWRAP;
		break;
	}
	state = normal_putch;
}

/*
 * escy1_putch(v, c): for when an ESC Y + char has been seen
 */
static void
escy1_putch(SCREEN *v, int c)
{
#if 0
	gotoxy(v, c - ' ', escy1 - ' ');
#else
	/* some (un*x) termcaps seem to always set the hi bit on
	   cm args (cm=\EY%+ %+ :) -> drop that unless the screen
	   is bigger.	-nox
	*/
	gotoxy(v, (c - ' ') & (v->maxx|0x7f), (escy1 - ' ') & (v->maxy|0x7f));
#endif
	state = normal_putch;
}

/*
 * escy_putch(v, c): for when an ESC Y has been seen
 */
static void
escy_putch(SCREEN *v, int c)
{
	UNUSED(v);
	escy1 = c;
	state = escy1_putch;
}

/*
 * normal_putch(v, c): put character 'c' on screen 'v'. This is the default
 * for when no escape, etc. is active
 */

static void
normal_putch(SCREEN *v, int c)
{
	register int i;

/* control characters */
	if (c < ' ') {
		switch (c) {
		case '\r':
col0:			v->cx = 0;
			i = v->cy + v->cy;
			i += i;
			v->cursaddr = base + *(long *)(rowoff + i);
			return;
		case '\n':
			if (v->cy == v->maxy) {
				scroll(v);
			} else {
				v->cy++;
				v->cursaddr += v->linelen;
			}
			return;
		case '\b':
			if (v->cx) {
				v->cx--;
				v->cursaddr--;
				if ((i = v->planes-1) && (v->cx & 1))
					v->cursaddr -= i+i;
			}
			return;
		case '\007':		/* BELL */
			(void)bconout(CONDEV, 7);
			return;
		case '\033':		/* ESC */
			state = putesc;
			return;
		case '\t':
			if (v->cx < v->maxx) {
			/* this can't be register for an ANSI compiler */
				union {
					long l;
					short i[2];
				} j;
				j.l = 0;
				j.i[1] = 8 - (v->cx & 7);
				v->cx += j.i[1];
				if (v->cx - v->maxx > 0) {
					v->cx = v->maxx;
					/* Can't rely on the calc below,
					 * so do it the safe way... */
					v->cursaddr = PLACE(v, v->cx, v->cy);
				} else {
					v->cursaddr += j.l;
					if ((i = v->planes-1)) {
						if (j.l & 1)
							j.i[1]++;
						do v->cursaddr += j.l;
						while (--i);
					}
				}
			}
			return;
		default:
			return;
		}
	}

	(*vpaint)(v, c, v->cursaddr);
	v->cx++;
	if (v->cx > v->maxx) {
		if (v->flags & FWRAP) {
			normal_putch(v, '\n');
			goto col0;
		} else {
			v->cx = v->maxx;
		}
	} else {
		v->cursaddr++;
		if ((i = v->planes-1) && !(v->cx & 1))	/* new word */
			v->cursaddr += i + i;
	}
}

INLINE void
put_ch(SCREEN *v, int c)
{
	(*state)(v, c & 0x00ff);
}


static long	_cdecl screen_open	(FILEPTR *f);
static long	_cdecl screen_read	(FILEPTR *f, char *buf, long nbytes);
static long	_cdecl screen_write	(FILEPTR *f, const char *buf, long nbytes);
static long	_cdecl screen_lseek	(FILEPTR *f, long where, int whence);
static long	_cdecl screen_ioctl	(FILEPTR *f, int mode, void *buf);
static long	_cdecl screen_close	(FILEPTR *f, int pid);
static long	_cdecl screen_select	(FILEPTR *f, long p, int mode);
static void	_cdecl screen_unselect	(FILEPTR *f, long p, int mode);

DEVDRV screen_device =
{
	screen_open, screen_write, screen_read, screen_lseek, screen_ioctl,
	null_datime, screen_close, screen_select, screen_unselect
};

static long _cdecl 
screen_open(FILEPTR *f)
{

	if (!current) {
		fasttext_init();
	}
	++scr_usecnt;

	f->flags |= O_TTY;
	return 0;
}

static long _cdecl 
screen_close(FILEPTR *f, int pid)
{
	SCREEN *v = current;
	UNUSED(pid);

	if (v && f->links <= 0 && !--scr_usecnt) {
		if (hardbase) {
			v->curstimer = 0x7f;
			v->cursaddr = v->cursaddr + (oldbase-base);
			quickmove(oldbase, base, scrnsize);
			base = oldbase;
			Setscreen(oldbase, oldbase, -1);
			v->curstimer = v->period;
		}
		current = 0;
	}
	return 0;
}

static long _cdecl 
screen_write(FILEPTR *f, const char *buf, long bytes)
{
	SCREEN *v = current;
	struct bios_file *b = (struct bios_file *)f->fc.index;
	long *r;
	long ret = 0;
	int c;

	UNUSED(f);

	(void)checkkeys();
	v->hidecnt++;
	v->flags |= CURS_UPD;		/* for TOS 1.0 */
	curs_off(v);
	r = (long *)buf;
	while (bytes > 0) {
		c = (int) *r++;
		put_ch(v, c);
		bytes -= 4; ret+= 4;
	}
	if (v->hidecnt > 0)
		--v->hidecnt;
	else
		v->hidecnt = 0;
	curs_on(v);
	v->flags &= ~CURS_UPD;
	if (ret > 0) {
		b->xattr.atime = b->xattr.mtime = timestamp;
		b->xattr.adate = b->xattr.mdate = datestamp;
	}
	return ret;
}

static long _cdecl 
screen_read(FILEPTR *f, char *buf, long bytes)
{
	struct bios_file *b = (struct bios_file *)f->fc.index;
	long *r, ret = 0;

	r = (long *)buf;

	while (bytes > 0) {
		if ( (f->flags & O_NDELAY) && !bconstat(CONDEV) )
			break;
		*r++ = bconin(CONDEV) & 0x7fffffffL;
		bytes -= 4; ret += 4;
	}
	if (ret > 0) {
		b->xattr.atime = timestamp;
		b->xattr.adate = datestamp;
	}
	return ret;
}

static long _cdecl 
screen_lseek(FILEPTR *f, long where, int whence)
{
/* terminals always are at position 0 */
	UNUSED(f); UNUSED(where);
	UNUSED(whence);
	return 0;
}

static long _cdecl 
screen_ioctl(FILEPTR *f, int mode, void *buf)
{
	long *r = (long *)buf;
	struct winsize *w;

	UNUSED(f);

	switch(mode) {
	case FIONREAD:
		if (bconstat(CONDEV))
			*r = 1;
		else
			*r = 0;
		break;
	case FIONWRITE:
			*r = 1;
		break;
	case FIOEXCEPT:
			*r = 0;
		break;
	case TIOCFLUSH:
/* BUG: this should flush the input/output buffers */
		break;
	case TIOCGWINSZ:
		w = (struct winsize *)buf;
		w->ws_row = current->maxy+1;
		w->ws_col = current->maxx+1;
		break;
		case TCURSOFF:
		curs_off(current);
		current->hidecnt++;
		current->flags &= ~CURS_ON;
			break;
		case TCURSON:
		current->flags |= CURS_ON;
		current->hidecnt = 0;
		curs_on(current);
			break;
		case TCURSBLINK:
		curs_off(current);
		current->flags |= CURS_FLASH;
		curs_on(current);
			break;
		case TCURSSTEADY:
		curs_off(current);
		current->flags &= ~CURS_FLASH;
		curs_on(current);
			break;
		case TCURSSRATE:
		current->period = *((short *)buf);
			break;
		case TCURSGRATE:
		*((short *)buf) = current->period;
		break;
	case TCURSSDELAY:	/* undocumented! */
		current->v_delay = *((short *)buf);
		break;
	case TCURSGDELAY:	/* undocumented! */
		*((short *)buf) = current->v_delay;
		break;
	default:
		return ENOSYS;
	}
	return E_OK;
}

static long _cdecl 
screen_select(FILEPTR *f, long p, int mode)
{
	struct tty *tty = (struct tty *)f->devinfo;
	int dev = CONDEV;

	if (mode == O_RDONLY) {
		if (bconstat(dev)) {
			return 1;
		}
		if (tty) {
		/* avoid collisions with other processes */
			if (tty->rsel)
				return 2;	/* collision */
			tty->rsel = p;
		}
		return 0;
	} else if (mode == O_WRONLY) {
		return 1;
	}
	/* default -- we don't know this mode, return 0 */
	return 0;
}

static void _cdecl 
screen_unselect(FILEPTR *f, long p, int mode)
{
	struct tty *tty = (struct tty *)f->devinfo;

	if (tty) {
		if (mode == O_RDONLY && tty->rsel == p)
			tty->rsel = 0;
		else if (mode == O_WRONLY && tty->wsel == p)
			tty->wsel = 0;
	}
}

# endif /* FASTTEXT */
