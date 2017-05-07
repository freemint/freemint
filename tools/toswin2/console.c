
#include <mintbind.h>

#include "console.h"
#include "textwin.h"
#include "proc.h"
#include "toswin2.h"

long con_fd = 0;
long con_log_fd = 0;

TEXTWIN	*con_win = NULL;
static bool	is_dirty = FALSE;


static void uniconify_con(WINDOW *v, short x, short y, short w, short h)
{
	if (v->wco)
	{
		wind_xset_grect(v->handle, WF_UNICONIFY, &v->prev, &v->work);
	}
	else
	{
		wind_calc(WC_BORDER, v->kind, v->prev.g_x, v->prev.g_y, v->prev.g_w, v->prev.g_h, &x, &y, &w, &h);
  		wind_set(v->handle, WF_UNICONIFY, x, y, w, h);
		wind_get_grect(v->handle, WF_WORKXYWH, &v->work);
	}
	v->mouseinp = v->oldmouse;
	v->flags &= ~WICONIFIED;
	(*v->topped)(v);
	is_dirty = FALSE;
}

static bool open_con_fd(void)
{
	if (con_fd == 0)
	{
		con_fd = Fopen(XCONNAME, 2);
		if (con_fd > 0)
		{
			Fsymlink(XCONNAME, TWCONNAME);
			add_fd(con_fd);
		}
		else
		{
			debug("open_con_fd() failed\n");
			return FALSE;
		}
	}

	return TRUE;
}

static void close_con_fd(void)
{
	if (con_fd != 0 && con_win == NULL)
	{
		Fdelete(TWCONNAME);
		Fclose(con_fd);
		con_fd = 0;
	}
}

void open_console(void)
{
	WINCFG	*cfg;
	char		str[30];

	if (con_win == NULL)
	{
		cfg = get_wincfg("Console");
		if (cfg->title[0] == '\0')
			strcpy(str, " Console ");
		else
			strcpy(str, cfg->title);

		con_win = create_textwin(str, cfg);
		if (con_win)
		{
			if (!open_con_fd())
			{
				destroy_textwin(con_win);
				alert(1, 0, XCONERR);
				return;
			}

			con_win->win->uniconify = uniconify_con;
			con_win->prog = strdup(str);
			con_win->fd = con_fd;

			/* Cursor mu an, sonst kommt die Ausgabe durcheinander!! */
			(*con_win->output)(con_win, '\033');
			(*con_win->output)(con_win, 'e');

			open_window(con_win->win, cfg->iconified);
		}
		else
		{
			alert(1, 0, XCONERR);
			return;
		}
	}
	else
	{
		if (con_win->win->flags & WICONIFIED)
		{
			is_dirty = FALSE;
			(*con_win->win->uniconify)(con_win->win, -1, -1, -1, -1);
		}
		else if (con_win->win->flags & WSHADED)
			(*con_win->win->shaded)(con_win->win, -1);
		else
			(*con_win->win->topped)(con_win->win);
	}
}

void handle_console(char *txt, long len)
{
	/*
	 * Wenn das erste Zeichen ein Ping ist, wird auf weitere Pings
	 * geprft. Werden nur Pings gefunden, wird die Funktion verlassen,
	 * damit das Fenster nicht unntig aufgeht.
	*/
	if (txt[0] == 7)
	{
		int	i, j = 0;
		
		for (i = 0; i<len; i++)
		{
			if (txt[i] == 7)
				j++;
			else
			{
				j = 0;
				break;
			}
		}
		if (j > 0)
			return;
	}

	if (con_win != NULL)
	{
		if (con_win->win->flags == WICONIFIED && !gl_con_output)
		{
			is_dirty = TRUE;
			draw_winicon(con_win->win);
		}
		else
			is_dirty = FALSE;

		if (gl_con_output)
		{
			if (con_win->win->flags & WICONIFIED)
				(*con_win->win->uniconify)(con_win->win, -1, -1, -1, -1);
			else if (con_win->win->flags & WSHADED)
				(*con_win->win->shaded)(con_win->win, -1);
		}
	}

	if (con_log_fd > 0)
		Fwrite(con_log_fd, len, txt);
}


bool log_console(bool on)
{
	if (gl_con_log && !on && con_log_fd > 0)
	{
		Fclose(con_log_fd);
		con_log_fd = 0;

		close_con_fd();
	}
	if (!gl_con_log && on && con_log_fd == 0)
	{
		int	log;

		log = (int)Fopen(gl_con_logname, 0x200|34);
		if (log > 0)
		{
			Fseek(0, log, 2);
			con_log_fd = log;

			if (!open_con_fd())
			{
				Fclose(con_log_fd);
				con_log_fd = 0;
				on = FALSE;
			}
		}
		else
		{
			on = FALSE;
			debug("Fopen(%s) failed: %d\n", gl_con_logname, log);
		}
	}
	return on;
}

bool out_console(bool on)
{
	if (gl_con_output && !on && con_fd > 0)
	{
		close_con_fd();
	}
	if (!gl_con_output && on && con_fd == 0)
	{
		open_con_fd();
	}
}

bool is_console(WINDOW *win)
{
	return ((con_win != NULL) && (win == (WINDOW *)con_win->win));
}

OBJECT *get_con_icon(void)
{
	if (is_dirty)
		return conicon;
	else
		return winicon;
}
