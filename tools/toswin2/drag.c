
#include <mintbind.h>
#include <strings.h>
#include <support.h>
#include <sys/stat.h>
#include <unistd.h>

#include "global.h"
#include "clipbrd.h"
#include "textwin.h"
#include "toswin2.h"
#include "window.h"
#include "drag.h"


/*-- D&D Empfang ------------------------------------------------------------- */

/*
 * Gedraggten Name aufbereiten.
 * Fr den Fall, daž im Fenster eine Loginshell l„uft wird der Name etwas 
 * ge„ndert:
 *	  - /dev wird ggf. entfernt (/dev/c/ -> /c/)
 *   - bei einem Pfad wird ein 'cd' dorthin ausgel”st
*/
static void check_name(char *name, TEXTWIN *t, bool cd, char *new)
{
	char			*p, str[256];
	struct stat	st;
	
	if (Dpathconf(name, 6) != 0)
		str_tolower(name);
	dos2unx(name, str);

	if ((t->shell != NO_SHELL))			/* l„uft Shell in Fenster? */
	{
		if (strncmp(str, "/dev/", 5) == 0)
			p = str + 4;						/* berspringen: /dev/c/ -> /c/ */
		else
			p = str;
			
		stat(p, &st);
		if ((st.st_mode & S_IFDIR) && cd)	/* 'cd' nur bei genau einem Arg */
		{
			strcpy(new, "cd ");
			strcat(new, p);
			strcat(new, "\r");
		}
		else
		{
			strcpy(new, p);
			strcat(new, " ");
		}
	}
}

/*
 * Anzahl der Args ermitteln.
*/
static int parse_args(char *str)
{
	int		cnt = 1;
	char 		*c = str;
	bool		in_quote = FALSE;

	while (*c)
	{
		switch (*c)
		{
			case ' ' :
				if (!in_quote)
				{
					*c = '\0';
					cnt++;
				}
				break;
			case '\'' :
				strcpy(c, c + 1);
				if (!in_quote)
					in_quote = TRUE;
				else
				{
					if (*c != '\'')
					{
						in_quote = FALSE;
						*c = 0;
						if (c[1])
							cnt++;
					}
				}
				break;
			default:
				break;
		} /* switch */
		c += 1;
	} /* while */
	return cnt;
}

static void parse(char *cmdline, WINDOW *win)
{
	int	comps;
	char	*c, n[256];
	bool	cd;
	
	comps = parse_args(cmdline);
	cd = (comps == 1);
	c = cmdline;
	while (comps)
	{
		check_name(c, win->extra, cd, n);
		sendstr(win->extra, n);
		c += strlen(c) + 1;
		comps--;
	}
}


void handle_dragdrop(short *msg)
{
	WINDOW	*w;
	int fd, pnam;
	char txtname[DD_NAMEMAX], ext[5];
	char *dd_data;
	long size;

	w = get_window(msg[3]);
	if (w && !(w->flags & WICONIFIED) && ! (w->flags & WSHADED) && !(w->flags & WISDIAL))
	{
		pnam = msg[7];
		fd = dd_open(pnam, "ARGS.TXT");	/* Argumente und Texte */
		if (fd < 0) 
			return;
		for(;;) 
		{
			if (!dd_rtry(fd, txtname, ext, &size)) 
			{
				dd_close(fd);
				return;
			}
			if ((strncmp(ext, "ARGS", 4) == 0) || (strncmp(ext, ".TXT", 4) == 0))
			{
				dd_data = malloc(size + 1);
				if (!dd_data) 
				{
					dd_reply(fd, DD_LEN);
					continue;
				}
				dd_reply(fd, DD_OK);
				/* size = Fread(fd, size, dd_data); */
				/* size = _text_read(fd, dd_data, size); */
				size = read(fd, dd_data, size);
				dd_close(fd);
				dd_data[size] = 0;

				if (strncmp(ext, ".TXT", 4) == 0)
					sendstr(w->extra, dd_data);
				else
					parse(dd_data, w);

				(*w->topped)(w);
				free(dd_data);
				return;
			}
			dd_reply(fd, DD_EXT);
		}
	}
}

void handle_avdd(short win_id, char *data)
{
	WINDOW *w;
	
	w = get_window(win_id);
	if (w && !(w->flags & WICONIFIED) && ! (w->flags & WSHADED) && !(w->flags & WISDIAL))
	{
		char		*cmdline;
	
		cmdline = strdup(data);
		parse(cmdline, w);
		free(cmdline);
	}
}



/*-- D&D Senden -------------------------------------------------------------- */

void drag_selection(TEXTWIN *t)
{
	short x, y, w, h, d;
	short win_id, i, app_id, kstate;
	WINDOW	*own;
		
	if (t->block_y1 == t->block_y2)
	{
		x = t->win->work.g_x + t->block_x1 * t->cmaxwidth;
		w = t->win->work.g_x + (t->block_x2 + 1) * t->cmaxwidth - x;
	}
	else
	{
		x = t->win->work.g_x;
		w = t->win->work.g_w;
	}
	y = t->win->work.g_y + (t->block_y1 * t->cheight - t->offy);
	h = (t->block_y2 - t->block_y1 + 1) * t->cheight;

	if (y < t->win->work.g_y)
	{
		h -= (t->win->work.g_y - y); 
		y = t->win->work.g_y;
	}
	if (y + h > (t->win->work.g_y + t->win->work.g_h - 1))
		h = (t->win->work.g_y + t->win->work.g_h - 1) - y;
	
	graf_mouse(FLAT_HAND, NULL);
	graf_dragbox(w, h, x, y, gl_desk.g_x, gl_desk.g_y, gl_desk.g_w, gl_desk.g_h, &x, &y);
	graf_mouse(ARROW, NULL);
	graf_mkstate(&x, &y, &d, &kstate);

	win_id = wind_find(x, y);
	own = get_window(win_id);
	
	if (own != NULL)					/* Ziel ist TW2-Fenster */
	{
		own->topped(own);
		unselect((TEXTWIN *)own->extra);
		paste(own);
	}
	else
	{
		if ((appl_xgetinfo(11, &i, &d, &d, &d)) && (i & 16)) /* gibts WF_OWNER? */
			wind_get(win_id, WF_OWNER, &app_id, &d, &d, &d);
		else
		{
			alert(1, 0, NODDSEND);
			return;
		}

		if (win_id >= 0)			/* Ziel ist Fenster anderer App */
		{
			int	pipe;
			char	ext[33];
			char	*text;
			
			pipe = dd_create(app_id, win_id, x, y, kstate, ext);
			if (pipe > 0)
			{
				text = read_scrap();
				d = dd_stry(pipe, ".TXT", "Text", strlen(text));
				if (d == DD_OK)
					d = (int)Fwrite(pipe, strlen(text), text);
				dd_close(pipe);
			}
		}
	}
}
