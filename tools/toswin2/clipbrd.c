/*
 * clipbrd.c
 *
 * Funktionen fÅr Textselektion und Klemmbrett
 *
 */

#include <osbind.h>
#include <ctype.h>
#include <string.h>

#include <cflib.h>

#include "global.h"
#include "clipbrd.h"
#include "config.h"
#include "textwin.h"
#include "toswin2.h"
#include "window.h"


static char	clip_path[80] = "";


static int CharClass(char ch)
{
	/* Filename identifier class */
	if (isalnum(ch) || ch=='/' || ch=='.' || ch=='_' || ch=='\\' || ch=='~' ||
	    ch==':' || ch=='-')
		return 1;
	else if (isspace(ch))
		return 2;
	else
		return 1000+ch; /* One class per character */
}

static void selectfrom(TEXTWIN *t, short x1, short y1, short x2, short y2)
{
	int	i, j;
	int	first, last;

	/* first, normalize coordinates */
	if ( (y1 > y2) || (y1 == y2 && x1 > x2) ) 
	{
		i = x1; j = y1;
		x1 = x2; y1 = y2;
		x2 = i; y2 = j;
	}

	t->block_x1 = x1;
	t->block_x2 = x2;
	t->block_y1 = y1;
	t->block_y2 = y2;

	for (j = 0; j <= y1; j++) 
	{
		last = (j == y1) ? x1 : t->maxx;
		for (i = 0; i < last; i++) 
		{
			if (t->cflag[j][i] & CSELECTED) 
			{
				t->cflag[j][i] &= ~CSELECTED;
				t->cflag[j][i] |= CTOUCHED;
				t->dirty[j] |= SOMEDIRTY;
			}
		}
	}
	for (j = y1; j <= y2; j++) 
	{
		first = (j == y1) ? x1 : 0;
		last = (j == y2) ? x2+1 : t->maxx;
		for (i = first; i < last; i++) 
		{
			if (!(t->cflag[j][i] & CSELECTED)) 
			{
				t->cflag[j][i] |= (CTOUCHED|CSELECTED);
				t->dirty[j] |= SOMEDIRTY;
			}
		}
	}
	for (j = y2; j < t->maxy; j++) 
	{
		first = (j == y2) ? x2+1 : 0;
		for (i = first; i < t->maxx; i++) 
		{
			if (t->cflag[j][i] & CSELECTED) 
			{
				t->cflag[j][i] &= ~CSELECTED;
				t->cflag[j][i] |= CTOUCHED;
				t->dirty[j] |= SOMEDIRTY;
			}
		}
	}
	refresh_textwin(t, FALSE);
}

int select_word(TEXTWIN *t, short curcol, short currow)
{
	int	curclass = CharClass(t->data[currow][curcol]);
	int	lcol = curcol;
	int	rcol = curcol;

	refresh_textwin(t, FALSE);

	while (lcol>0 && CharClass(t->data[currow][lcol])==curclass)
		lcol--;
	if (CharClass(t->data[currow][lcol])!=curclass)
		lcol++;

	while (rcol<t->maxx && CharClass(t->data[currow][rcol])==curclass)
		rcol++;
	if (CharClass(t->data[currow][rcol])!=curclass)
		rcol--;

	selectfrom(t, lcol, currow, rcol, currow);
	return 1;
}

int select_text(TEXTWIN *t, short curcol, short currow, short kshift)
{
	WINDOW *v = t->win;
	short x, y, firstx, firsty;
	short event, dummy, msx, msy, mbutton;
	short anchorcol, anchorrow;
	short *WIDE = t->cwidths;
	short cboxw;
	GRECT w;
	
	refresh_textwin(t, FALSE);

	/* shift+select adds to existing text; regular select replaces */
	anchorrow = currow; 
	anchorcol = curcol;

	if (kshift & 3) 
	{
		for (y = 0; y < t->maxy; y++)
			for (x = 0; x < t->maxx; x++) 
			{
				if (t->cflag[y][x] & CSELECTED) 
				{
					anchorrow = y;
					anchorcol = x;
					if (y < currow || (y == currow && x < curcol))
						goto foundselect;
				}
			}
	} 
	else
		unselect(t);

foundselect:

	char2pixel(t, curcol, currow, &x, &y);
	firstx = msx = x; 
	firsty = msy = y;

	wind_update(BEG_MCTRL);
	graf_mouse(POINT_HAND, 0L);
	selectfrom(t, anchorcol, anchorrow, curcol, currow);
	cboxw = t->cmaxwidth;
	w = v->work;
	for(;;) 
	{
		if (WIDE)
			cboxw = WIDE[t->data[currow][curcol]];

		event = evnt_multi(MU_M1 | MU_M2 | MU_BUTTON, 
									0x101, 0x0003, 0x0001,
									1, x, y, cboxw, t->cheight,
									1, w.g_x, w.g_y, w.g_w, w.g_h,
									0L, 0L,
									&msx, &msy, &mbutton, &dummy,
									&dummy, &dummy);
		if (event & MU_M1) 
		{
			if (msx < v->work.g_x)
				msx = v->work.g_x;
			else if (msx >= v->work.g_x + v->work.g_w)
				msx = v->work.g_x + v->work.g_w - 1;
			if (msy < v->work.g_y)
				msy = v->work.g_y;
			else if (msy >= v->work.g_y + v->work.g_h)
				msy = v->work.g_y + v->work.g_h - 1;
			pixel2char(t, msx, msy, &curcol, &currow);
			char2pixel(t, curcol, currow, &x, &y);
			selectfrom(t, anchorcol, anchorrow, curcol, currow);
		}
		if (event & MU_M2)
		{
			if (msx == w.g_x)
				v->arrowed(v, WA_LFLINE);
			if (msx == (w.g_x + w.g_w - 1))
				v->arrowed(v, WA_RTLINE);
			if (msy == w.g_y)
				v->arrowed(v, WA_UPLINE);
			if (msy == (w.g_y + w.g_h - 1))
				v->arrowed(v, WA_DNLINE);
		}
		if (event & MU_BUTTON)
			break;
	}
	graf_mouse(ARROW, 0L);
	wind_update(END_MCTRL);

	if ( ((msx - firstx) < 3) && ((msx - firstx) > -3) &&
	     ((msy - firsty) < 3) && ((msy - firsty > -3)) ) 
	{
		unselect(t);
		return 0;
	}
	return 1;
}

/* unselect all text in the indicated window */
void unselect(TEXTWIN *t)
{
	int i, j;

	for (i = 0; i < t->maxy; i++)
		for (j = 0; j < t->maxx; j++) 
		{
			if (t->cflag[i][j] & CSELECTED) 
			{
				t->cflag[i][j] &= ~CSELECTED;
				t->cflag[i][j] |= CTOUCHED;
				t->dirty[i] |= SOMEDIRTY;
			}
		}
	refresh_textwin(t, FALSE);
}


/*
 * Dateioperationen auf SCRAP.TXT
*/
static void get_clipdir(void)
{
	char	*p = NULL;
	int	i;
	
	if (clip_path[0] == '\0')
	{
		i = scrp_read(clip_path);
		if (i == 0 || !clip_path[0])
		{	
			shel_envrn(&p, "CLIPBRD=");
			if (p != NULL)
				strcpy(clip_path, p);
			else
			{
				shel_envrn(&p, "SCRAPDIR=");
				if (p != NULL)
					strcpy(clip_path, p);
				else
				{
					alert(1, 0, NOSCRAP);
					return;
				}
			}
		}
		make_normalpath(clip_path);
/*
printf("\nclip_path: %s\n", clip_path);
*/
		if (!path_exists(clip_path))
			(void)Dcreate(clip_path);
	}
}

static void write_scrap(char *data, int len)
{
	char	filename[256];
	int	fd;

	get_clipdir();
	strcpy(filename, clip_path);
	strcat(filename, "scrap.txt");
	
	fd = (int)Fcreate(filename, 0);
	if (fd < 0) 
	{
		alert(1, 0, SCRAPERR);
		return;
	}
	Fwrite(fd, (long)len, data);
	Fclose(fd);
	
	/* Broadcast an alle anderen Applikationen */
	send_scchanged();
}

char *read_scrap(void)
{
	char	filename[256];
	int 	fd;
	long 	len, r;
	char 	*data;

	get_clipdir();
	strcpy(filename, clip_path);
	strcat(filename, "scrap.txt");

	fd = (int)Fopen(filename, 0);
	if (fd < 0)
		return NULL;

	len = Fseek(0L, fd, 2);
	(void)Fseek(0L, fd, 0);
	if (len >= 0) 
	{
		data = malloc((size_t)len+1);
		if (data) 
		{
			r = Fread(fd, len, data);
			if (r <= 0) 
			{
				free(data);
				data = NULL;
			} 
			else
				data[r] = 0;
		}
	} 
	else
		data = NULL;
	Fclose(fd);
	return data;
}


/* copy selected text to buffer or clipboard (dest == NULL) */
void copy(WINDOW *w, char *dest, int length)
{
	TEXTWIN	*t;
	int		i, j, numchars, numlines;
	int		linedone;
	char		*s, c;
	char 		*cliptext = NULL;

	t = w->extra;
	numchars = numlines = 0;
	for (i = 0; i < t->maxy; i++) 
	{
		linedone = 0;
		for (j = 0; j < t->maxx; j++) 
		{
			if (t->cflag[i][j] & CSELECTED) 
			{
				numchars++;
				if (!linedone) 
				{
					numlines++;
					linedone = 1;
				}
			}
		}
	}
	if (!numchars)
	{
		if (dest)
			*dest = '\0';
		return;
	}

	cliptext = malloc(numchars+numlines+numlines+2+1);	/* +2 fÅr CRLF am Ende */
	if (!cliptext) 
	{
		alert(1, 0, SCRAPMEM);
		return;
	}

	s = cliptext;
	for (i = 0; i < t->maxy; i++) 
	{
		linedone = 0;
		for (j = 0; j < t->maxx; j++) 
		{
			if (t->cflag[i][j] & CSELECTED) 
			{
				c = t->data[i][j];
				if (!c) 
					c = ' ';
				*s++ = c;
				if ((c == ' ') && t->data[i][j-1] == ' ')
					linedone = 1;
				else
					linedone = 0;
			}
		}
		if (linedone) 
		{
			while (s > cliptext && s[-1] == ' ')
					--s;
			*s++ = '\r'; *s++ = '\n';
		}
	}
	*s++ = 0;

	if (dest)
	{
		strncpy(dest, cliptext, length);
		dest[length] = '\0';
	}
  	else
  		write_scrap(cliptext, (int)strlen(cliptext));

	free(cliptext);
}

/* paste text into a window */
void paste(WINDOW *w)
{
	char 		*cliptext = NULL, *s;
	char		c;
	TEXTWIN 	*t = w->extra;
	WINCFG 	*cfg = t->cfg;
	int 		tab;

	cliptext = read_scrap();
	if (!cliptext)
	{ 
		alert(1, 0, SCRAPEMPTY);
		return;
	}

	/* keine énderungen beim EinfÅgen vornehmen */
	tab = cfg->char_tab;
	cfg->char_tab = TAB_ATARI;
	for (s = cliptext; *s; s++) 
	{
		c = *(unsigned char *)s;
		(*w->keyinp)(w, c, 0);
	}
	cfg->char_tab = tab;
	free(cliptext);
}
