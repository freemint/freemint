
#include <string.h>
#include <support.h>

#include "config.h"
#include "ansicol.h"
#include "toswin2.h"
#include "av.h"
#include "event.h"
#include "console.h"
#include "version.h"

#define COLOR_BITS 4

/*
 * exportierte Variablen
 */
bool	gl_shortcut = TRUE;
bool	gl_allogin = TRUE;

bool	gl_con_auto = FALSE;		/* beim Start anlegen */
bool	gl_con_output = TRUE;		/* bei Ausgaben ffnen */
bool	gl_con_log = FALSE;		/* Datei-Loggin */
char	gl_con_logname[128] = "U:\\ram\\con.log";

WINCFG	*gl_wincfg = NULL;

/*
 * lokale Variablen
*/
static WDIALOG	*cfg_wd, *con_wd;
static WINCFG	*p_cfg = NULL;
static TEXTWIN	*cfg_win;		/* Fenster, fr die der Dialog offen ist */
static short	new_id, new_pts;	/* neuer Font von FONTSEL */
static int	new_term, new_fg, new_bg, new_tab;
static char	new_log[128];
static OBJECT	*popups;


/******************************************************************************/
/* CFG-Liste (FIFO)																				*/
/******************************************************************************/
static WINCFG *crt_newcfg(const char *prog)
{
	WINCFG	*new, *p;

	new = (WINCFG*)malloc(sizeof(WINCFG));
	if (gl_wincfg == NULL)
	{
		gl_wincfg = new;
		new->kind = 0x4FEF;
		new->font_id = 1;
		new->font_pts = 10;
		new->col = 80;
		new->row = 24;
		new->scroll = 0;
		new->xpos = -1;
		new->ypos = -1;
		new->vt_mode = MODE_VT100;
		new->autoclose = TRUE;
		new->blockcursor = FALSE;
		new->iconified = FALSE;
		new->fg_color = 7;
		new->bg_color = 0;
		new->vdi_colors = new->fg_effects = new->bg_effects = 0;
		new->char_tab = TAB_ATARI;
	} else
	{
		p = gl_wincfg;
		*new = *p;
		while (p->next != NULL)
			p = p->next;
		p->next = new;
	}
	new->next = NULL;

	if (strcmp(prog, console_progname) == 0)
		new->vt_mode = MODE_VT52;
	strcpy(new->progname, prog);

	/* Default-Werte */
	strcpy(new->arg, "");
	strcpy(new->title, "");
	return new;
}

WINCFG *get_wincfg(const char *prog)
{
	WINCFG	*p;

	p = gl_wincfg;
	while (p != NULL)
	{
		if (strcmp(prog, p->progname) == 0)
			return p;
		p = p->next;
	}
	/* nichts gefunden -> Default */
	return gl_wincfg;
}

static void validate_cfg(WINCFG *cfg)
{
	if (cfg->kind < 0)	cfg->kind = 0x4FEF;
	if (cfg->font_id < 1)	cfg->font_id = 1;
	if (cfg->font_pts < 1)	cfg->font_pts = 10;
	if (cfg->col < 1)	cfg->col = 80;
	if (cfg->row < 1)	cfg->row = 24;
	if (cfg->scroll < 0)	cfg->scroll = 0;
	if (cfg->vt_mode < MODE_VT52 || cfg->vt_mode > MODE_VT100)
		cfg->vt_mode = MODE_VT52;

	if (cfg->vdi_colors)
	{
		cfg->fg_color &= 0xf;
		cfg->bg_color &= 0xf;
	}
	else
	{
		cfg->fg_color = cfg->fg_color & 0x7;
		cfg->bg_color = cfg->bg_color & 0x7;
	}
	if (cfg->char_tab < TAB_ATARI || cfg->char_tab > TAB_ISO)
		cfg->char_tab = TAB_ATARI;
}

/******************************************************************************/
/* Dialoge																							*/
/******************************************************************************/

static void open_conwd(WDIALOG *wd)
{
	char	s[20];

	set_state(wd->tree, CAUTO, OS_SELECTED, gl_con_auto);
	set_state(wd->tree, COUTPUT, OS_SELECTED, gl_con_output);

	set_state(wd->tree, CLOGACTIVE, OS_SELECTED, gl_con_log);
	make_shortpath(gl_con_logname, s, 19);
	set_string(wd->tree, CLOGNAME, s);

	strcpy(new_log, gl_con_logname);
}

static int exit_conwd(WDIALOG *wd, short exit_obj)
{
	bool	close = FALSE;
	char	path[128], name[128];

	switch (exit_obj)
	{
		case CLOGSEL :
			split_filename(new_log, path, name);
			if (select_file(path, name, "", rsc_string(STRLOGSEL), FSCB_NULL))
			{
				strcpy(new_log, path);
				strcat(new_log, name);
				make_shortpath(new_log, name, 19);
				set_string(con_wd->tree, CLOGNAME, name);
				redraw_wdobj(wd, CLOGNAME);
			}
			set_state(wd->tree, exit_obj, OS_SELECTED, FALSE);
			redraw_wdobj(wd, exit_obj);
			break;

		case CHELP :
			menu_help(TOPTION, MCCONFIG);
			set_state(wd->tree, exit_obj, OS_SELECTED, FALSE);
			redraw_wdobj(wd, exit_obj);
			break;

		case CABBRUCH :
			close = TRUE;
			break;

		case COK :
			gl_con_auto = get_state(wd->tree, CAUTO, OS_SELECTED);
			gl_con_output = out_console(get_state(wd->tree, COUTPUT, OS_SELECTED));

			strcpy(gl_con_logname, new_log);
			gl_con_log = log_console(get_state(wd->tree, CLOGACTIVE, OS_SELECTED));
			set_state(wd->tree, CLOGACTIVE, OS_SELECTED, gl_con_log);

			close = TRUE;
			break;
	}
	if (close)
		wdial_close(wd);

	return close;
}

void conconfig_open(void)
{
	open_wdial(con_wd, -1, -1);
}


/* ---- */

static void get_fontname(int id, char *name)
{
	int	d, i;
	char	str[33];

	strcpy(name, "??");
	for (d = 1; d <= font_anz; d++)
	{
		i = vqt_name(vdi_handle, d, str);
		if (i == id)
		{
			strcpy(name, str);
			for (i = strlen(name); i < 24; i++)
				strcat(name, " ");
			name[24] = '\0';
			return ;
		}
	}
}


static void open_cfgwd(WDIALOG *wd)
{
	char		str[32], title[256];
	WINCFG	*cfg;

	if (wd->mode == 0)		/* nur beim ersten Mal */
	{
		if (gl_topwin && !(gl_topwin->flags & WISDIAL)
		  && !(gl_topwin->flags & WICONIFIED)
		  && !(gl_topwin->flags & WSHADED))
		{
			/* oberstes Fenster */
			cfg_win = gl_topwin->extra;
			cfg = get_wincfg(cfg_win->prog);
			if (cfg == gl_wincfg)
			{
				cfg = crt_newcfg(cfg_win->prog);
				cfg_win->cfg = cfg;
			}
			sprintf(title, rsc_string(STRCFGPROG), cfg->progname);
			title[40] = '\0';
			set_string(wd->tree, WPROG, title);
			if (cfg->title[0] != '\0')
			{
				strcpy(title, cfg->title);
			} else if (strcmp(cfg->progname, console_progname) == 0)
			{
				strcpy(title, rsc_string(STRCONSTITLE));
			} else
			{
				title[0] = '\0';
			}
		}
		else
		{
			/* default window */
			cfg_win = NULL;
			cfg = gl_wincfg;
			sprintf(title, rsc_string(STRCFGPROG), rsc_string(STRCFGSTD));
			set_string(wd->tree, WPROG, title);
			title[0] = '\0';
		}

		_ltoa(cfg->col, str, 10);
		set_string(wd->tree, WCOL, str);

		_ltoa(cfg->row, str, 10);
		set_string(wd->tree, WROW, str);

		_ltoa(cfg->scroll, str, 10);
		set_string(wd->tree, WBUFFER, str);

		get_fontname(cfg->font_id, str);
		set_string(wd->tree, WFNAME, str);
		_ltoa(cfg->font_pts, str, 10);
		set_string(wd->tree, WFSIZE, str);

		set_state(wd->tree, WGCLOSER, OS_SELECTED, cfg->kind & CLOSER);
		set_state(wd->tree, WGTITLE, OS_SELECTED, cfg->kind & NAME);
		set_state(wd->tree, WGSMALLER, OS_SELECTED, cfg->kind & SMALLER);
		set_state(wd->tree, WGFULLER, OS_SELECTED, cfg->kind & FULLER);
		set_state(wd->tree, WGSLVERT, OS_SELECTED, cfg->kind & VSLIDE);
		set_state(wd->tree, WGSIZER, OS_SELECTED, cfg->kind & SIZER);
		set_string(wd->tree, WTITLE, title);

		switch (cfg->vt_mode)
		{
			case MODE_VT52 :
				get_string(popups, TP52, str);
				new_term = 0;
				break;

			case MODE_VT100 :
				get_string(popups, TP100, str);
				new_term = 1;
				break;

			default:
				strcpy(str, "???");
				new_term = 0;
		}
		set_string(wd->tree, WTERM, str);

		get_string(popups, TPATARI + cfg->char_tab, str);
		set_string(wd->tree, WTAB, str);

		if (cfg->vdi_colors) {
			set_popobjcolor (wd->tree, WFGCOL, cfg->fg_color);
			set_popobjcolor (wd->tree, WBGCOL, cfg->bg_color);
		} else {
			set_popobjcolor (wd->tree, WFGCOL,
					 cfg->fg_effects & CE_BOLD ?
					 ansibright2vdi[cfg->fg_color] :
					 ansi2vdi[cfg->fg_color]);
			set_popobjcolor (wd->tree, WBGCOL,
					 cfg->bg_effects & CE_BOLD ?
					 ansibright2vdi[cfg->bg_color] :
					 ansi2vdi[cfg->bg_color]);
		}
		/* FIXME: Set future checkboxes for pseudo effects.  */

		set_state(wd->tree, WCLOSE, OS_SELECTED, cfg->autoclose);
		set_state(wd->tree, WBLOCKCURSOR, OS_SELECTED, cfg->blockcursor);

		new_id = cfg->font_id;
		new_pts = cfg->font_pts;
		new_fg = cfg->vdi_colors ? cfg->fg_color : (cfg->fg_effects & CE_BOLD ? ansibright2vdi[cfg->fg_color & 0x7] : ansi2vdi[cfg->fg_color & 0x7]);
		new_bg = cfg->vdi_colors ? cfg->bg_color : (cfg->bg_effects & CE_BOLD ? ansibright2vdi[cfg->bg_color & 0x7] : ansi2vdi[cfg->bg_color & 0x7]);
		new_tab = cfg->char_tab;
	}
/*
	wdial_open(wd);
*/
}

static int exit_cfgwd(WDIALOG *wd, short exit_obj)
{
	char		str[32];
	bool		close = FALSE, ok;
	WINCFG	*cfg;
	int		y;

	if (cfg_win == NULL)
		cfg = gl_wincfg;
	else
		cfg = cfg_win->cfg;

	switch (exit_obj)
	{
		case WFSEL:
			ok = do_fontsel((FS_M_ALL|FS_F_MONO), rsc_string(STRFONTSEL), &new_id, &new_pts);
			if (ok)
			{
				_ltoa(new_pts, str, 10);
				set_string(wd->tree, WFSIZE, str);
				get_fontname(new_id, str);
				set_string(wd->tree, WFNAME, str);
			}
			else if (new_id == -1)
				alert(1, 0, NOXFSL);
			set_state(wd->tree, exit_obj, OS_SELECTED, FALSE);
			redraw_wdobj(wd, WFBOX);
			break;

		case WTERMSTR :
		case WTERM :
			if (exit_obj == WTERM)
				y = handle_popup(wd->tree, WTERM, popups, TERMPOP, POP_OPEN);
			else
				y = handle_popup(wd->tree, WTERM, popups, TERMPOP, POP_CYCLE);
			if (y > 0)
				new_term = y - TP52;
			break;

		case WTABSTR :
		case WTAB :
			if (exit_obj == WTAB)
				y = handle_popup(wd->tree, WTAB, popups, TABPOP, POP_OPEN);
			else
				y = handle_popup(wd->tree, WTAB, popups, TABPOP, POP_CYCLE);
			if (y > 0)
				new_tab = y - TPATARI;
			break;


		case WFGSTR :
		case WFGCOL :
			if (exit_obj == WFGCOL)
				y = handle_colorpop(wd->tree, WFGCOL, POP_OPEN, COLOR_BITS, 0);
			else
				y = handle_colorpop(wd->tree, WFGCOL, POP_CYCLE, COLOR_BITS, 0);
			if (y > -1)
				new_fg = y & 0xf;
			set_state(wd->tree, exit_obj, OS_SELECTED, FALSE);
			redraw_wdobj(wd, exit_obj);
			break;

		case WBGSTR :
		case WBGCOL :
			if (exit_obj == WBGCOL)
				y = handle_colorpop(wd->tree, WBGCOL, POP_OPEN, COLOR_BITS, 0);
			else
				y = handle_colorpop(wd->tree, WBGCOL, POP_CYCLE, COLOR_BITS, 0);
			if (y > -1)
				new_bg = y & 0xf;
			set_state(wd->tree, exit_obj, OS_SELECTED, FALSE);
			redraw_wdobj(wd, exit_obj);
			break;

		case WHELP :
			menu_help(TOPTION, MWCONFIG);
			set_state(wd->tree, WHELP, OS_SELECTED, FALSE);
			redraw_wdobj(wd, WHELP);
			break;

		case WABBRUCH :
			close = TRUE;
			break;

		case WOK :
			get_string(wd->tree, WCOL, str);
			cfg->col = atoi(str);
			if (cfg->col < MINCOLS)
				cfg->col = MINCOLS;
			else if (cfg->col > MAXCOLS)
				cfg->col = MAXCOLS;
			get_string(wd->tree, WROW, str);
			cfg->row = atoi(str);
			if (cfg->row < MINROWS)
				cfg->row = MINROWS;
			else if (cfg->row > MAXROWS)
				cfg->row = MAXROWS;
			get_string(wd->tree, WBUFFER, str);
			cfg->scroll = atoi(str);
			if (cfg->scroll < 0)
				cfg->scroll = 0;
			else if (cfg->scroll > 999)
				cfg->scroll = 999;

			if (new_id != -1)
				cfg->font_id = new_id;
			if (new_pts != -1)
				cfg->font_pts = new_pts;

			cfg->kind = 0;
			if (get_state(wd->tree, WGCLOSER, OS_SELECTED))
				cfg->kind |= CLOSER;
			if (get_state(wd->tree, WGTITLE, OS_SELECTED))
				cfg->kind |= (NAME|MOVER);
			if (get_state(wd->tree, WGSMALLER, OS_SELECTED))
				cfg->kind |= SMALLER;
			if (get_state(wd->tree, WGFULLER, OS_SELECTED))
				cfg->kind |= FULLER;
			if (get_state(wd->tree, WGSLVERT, OS_SELECTED))
				cfg->kind |= (VSLIDE|UPARROW|DNARROW);
			if (get_state(wd->tree, WGSIZER, OS_SELECTED))
				cfg->kind |= SIZER;
			get_string(wd->tree, WTITLE, cfg->title);

			if (new_term == 0)
				cfg->vt_mode = MODE_VT52;
			else
				cfg->vt_mode = MODE_VT100;
			cfg->char_tab = new_tab;

			cfg->fg_color = cfg->vdi_colors ? new_fg : vdi2ansi[new_fg & 0x7];
			cfg->bg_color = cfg->vdi_colors ? new_bg : vdi2ansi[new_bg & 0x7];

			if (new_fg == VDI_WHITE_BRIGHT || new_fg == VDI_BLACK_BRIGHT
			    || (new_fg > 1 && new_fg < 8))
				cfg->fg_effects |= CE_BOLD;
			else
				cfg->fg_effects &= ~CE_BOLD;

			if (new_bg == VDI_WHITE_BRIGHT || new_bg == VDI_BLACK_BRIGHT
			    || (new_bg > 1 && new_bg < 8))
				cfg->bg_effects |= CE_BOLD;
			else
				cfg->bg_effects &= ~CE_BOLD;

			cfg->autoclose = get_state(wd->tree, WCLOSE, OS_SELECTED);
			cfg->blockcursor = get_state(wd->tree, WBLOCKCURSOR, OS_SELECTED);

			if (cfg_win)
				reconfig_textwin(cfg_win, cfg);

			close = TRUE;
			break;
	}

	if (close)
		wdial_close(wd);

	return close;
}

void winconfig_open(void)
{
	open_wdial(cfg_wd, -1, -1);
}

void update_font(WINDOW *w, int id, int pts)
{
	TEXTWIN *text;
	char		str[32];

	if (cfg_wd->mode == WD_OPEN)
	{
		new_id = id;
		new_pts = pts;
		get_fontname(id, str);
		set_string(cfg_wd->tree, WFNAME, str);
		_ltoa(pts, str, 10);
		set_string(cfg_wd->tree, WFSIZE, str);
		redraw_wdobj(cfg_wd, WFBOX);
	}
	else if (w != NULL)
	{
		text = w->extra;
		if (id != 0)
			text->cfg->font_id = id;
		if (pts != 0)
			text->cfg->font_pts = pts;
		textwin_setfont(text, id, pts);
	}
}

/******************************************************************************/
/* Dateioperationen																				*/
/******************************************************************************/
static char cfg_path[80] = "";
static FILE *fd;

static bool path_from_env(char *env, char *path)
{
	char *p;
	bool ret = FALSE;

	p = getenv(env);
	if (p != NULL)
	{
		strcpy(path, p);
		ret = make_normalpath(path);
	}
	return ret;
}

static bool get_cfg_path(void)
{
	bool found = FALSE;
	char env[256], p_for_save[256] = "";

	if (!gl_debug)
	if (path_from_env("HOME", env)) 			/* 1. $HOME */
	{
		bool	h = FALSE;

		strcpy(cfg_path, env);
		strcat(cfg_path, CFGNAME);
		if (p_for_save[0] == '\0')
		{
			h = TRUE;
			strcpy(p_for_save, cfg_path);
		}
		found = file_exists(cfg_path);
		if (!found)										/* 2. $HOME/defaults */
		{
			strcpy(cfg_path, env);
			strcat(cfg_path, "defaults\\");
			if (path_exists(cfg_path))
			{
				strcat(cfg_path, CFGNAME);
				if (p_for_save[0] == '\0' || h)
					strcpy(p_for_save, cfg_path);
				found = file_exists(cfg_path);
			}
		}
	}

	if (!found && gl_appdir[0] != '\0')			/* 3. Startverzeichnis */
	{
		strcpy(cfg_path, gl_appdir);
		strcat(cfg_path, CFGNAME);
		if (p_for_save[0] == '\0')
			strcpy(p_for_save, cfg_path);
		found = file_exists(cfg_path);
	}

	if (!found && file_exists(CFGNAME))			/* 4. aktuelles Verzeichnis */
	{
		get_path(cfg_path, 0);
		strcat(cfg_path, CFGNAME);
		if (p_for_save[0] == '\0')
			strcpy(p_for_save, cfg_path);
		found = TRUE;
	}

	if (!found)
		strcpy(cfg_path, p_for_save);

	return found;
}


/******************************************************************************/
/* Laden																								*/
/******************************************************************************/
static void get_bool(char *str, bool *val)
{
	if ((strcmp(str, "TRUE") == 0) || (strcmp(str, "true") == 0))
		*val = TRUE;
	if ((strcmp(str, "FALSE") == 0) || (strcmp(str, "false") == 0))
		*val = FALSE;
}

static void get_str(char *str, char *val)
{
	char	*p;

	if ((str[0] == '=') && (str[1] == '=')) /* nur "" -> leer */
		val[0] = '\0';
	else
	{
		if (str[0] == '"')
		{
			p = strrchr(str, '"');
			if (p != NULL)
			{
				strncpy(val, str+1, p-str-1);
				val[p-str-1] = '\0';
			}
		}
	}
}


static void parse_line(char *zeile)
{
	char	var[30],	value[256], *p, tmp[80];
	long	l;

	p = strchr(zeile, '=');
	if (p != NULL)
	{
		strncpy(var, zeile, p-zeile);
		var[p-zeile] = '\0';
		strcpy(value, p+1);

		if (strcmp(var, "AllLoginshell") == 0)
			get_bool(value, &gl_allogin);

		if (strcmp(var, "ConsAutoOpen") == 0)
			get_bool(value, &gl_con_auto);
		if (strcmp(var, "ConsLogging") == 0)
			get_bool(value, &gl_con_log);
		if (strcmp(var, "ConsLogName") == 0)
			get_str(value, gl_con_logname);
		if (strcmp(var, "ConsOutputOpen") == 0)
			get_bool(value, &gl_con_output);


		if (strcmp(var, "MenuShortcut") == 0)
			get_bool(value, &gl_shortcut);
		if (strcmp(var, "WinCycleAV") == 0)
			get_bool(value, &gl_avcycle);

		else if (strcmp(var, "WinCfgBegin") == 0)
		{
			get_str(value, tmp);
			if (strcmp(tmp, "default") == 0)
				p_cfg = gl_wincfg;
			else
				p_cfg = crt_newcfg(tmp);
		}
		else if (strcmp(var, "WinCfgEnd") == 0)
		{
			validate_cfg(p_cfg);
			p_cfg = NULL;
		}
		else if (p_cfg != NULL)
		{
			if (strcmp(var, "WinArg") == 0)
				get_str(value, p_cfg->arg);
			else if (strcmp(var, "WinAsIcon") == 0)
				get_bool(value, &p_cfg->iconified);
			else if (strcmp(var, "WinAutoClose") == 0)
				get_bool(value, &p_cfg->autoclose);
			else if (strcmp(var, "WinBlockCursor") == 0)
				get_bool(value, &p_cfg->blockcursor);
			else if (strcmp(var, "WinCharTab") == 0)
				p_cfg->char_tab = atoi(value);
			else if (strcmp(var, "WinCol") == 0)
				p_cfg->col = atoi(value);
			else if (strcmp(var, "WinColorBg") == 0)
				p_cfg->bg_color = atoi(value);
			else if (strcmp(var, "WinColorFg") == 0)
				p_cfg->fg_color = atoi(value);
			else if (strcmp(var, "WinFontID") == 0)
				p_cfg->font_id = atoi(value);
			else if (strcmp(var, "WinFontPts") == 0)
				p_cfg->font_pts = atoi(value);
			else if (strcmp(var, "WinKind") == 0)
				p_cfg->kind = atoi(value);
			else if (strcmp(var, "WinPosX") == 0)
			{
				l = atoi(value);
				if (l >= 0)
					p_cfg->xpos = (int)(l * (gl_desk.g_x + gl_desk.g_w - 1L) / 10000L);
				else
					p_cfg->xpos = -1;
			}
			else if (strcmp(var, "WinPosY") == 0)
			{
				l = atoi(value);
				if (l >= 0)
					p_cfg->ypos = (int)(l * (gl_desk.g_y + gl_desk.g_h - 1L) / 10000L);
				else
					p_cfg->ypos = -1;
			}
			else if (strcmp(var, "WinRow") == 0)
				p_cfg->row = atoi(value);
			else if (strcmp(var, "WinScroll") == 0)
				p_cfg->scroll = atoi(value);
			else if (strcmp(var, "WinTitle") == 0)
				get_str(value, p_cfg->title);
			else if (strcmp(var, "WinVtMode") == 0)
				p_cfg->vt_mode = atoi(value);
			else if (strcmp(var, "WinVDIColors") == 0)
				p_cfg->vdi_colors = atoi(value);
			else if (strcmp(var, "WinFGEffects") == 0)
				p_cfg->fg_effects = (CE_ANSI_EFFECTS) & atoi(value);
			else if (strcmp(var, "WinBGEffects") == 0)
				p_cfg->bg_effects = (CE_ANSI_EFFECTS) & atoi(value);
			}
		}
}

bool config_load(void)
{
	int	ret = FALSE;
	char	buffer[256];

	if (get_cfg_path() && (fd = fopen(cfg_path, "r")) != NULL)
	{
		/* 1. Zeile auf ID checken */
		fgets(buffer, sizeof(buffer), fd);
		if (strncmp(buffer, "ID=TosWin2", 10) == 0)
		{
			while (fgets(buffer, sizeof(buffer), fd) != NULL)
			{
				if (buffer[strlen(buffer) - 1] == '\n')
					buffer[strlen(buffer) - 1] = '\0';
				parse_line(buffer);
			}

			ret = TRUE;
		}
		else
			alert(1, 0, IDERR);
		fclose(fd);
		fd = NULL;
	}

	if (gl_con_log)
	{
		bool on = gl_con_log;
		gl_con_log = FALSE;
		gl_con_log = log_console(on);
	}

	if (gl_con_output)
	{
		bool on = gl_con_output;
		gl_con_output = FALSE;
		gl_con_output = out_console(on);
	}

	return ret;
}


/******************************************************************************/
/* Speichern																						*/
/******************************************************************************/
static void write_str(char *var, char *value)
{
	fprintf(fd, "%s=\"%s\"\n", var, value);
}

static void write_int(char *var, int value)
{
	fprintf(fd, "%s=%d\n", var, value);
}

static void write_ulong(char *var, ulong value)
{
	fprintf(fd, "%s=%lu\n", var, value);
}

static void write_bool(char *var, bool value)
{
	char	str[6];

	if (value)
		strcpy(str, "TRUE");
	else
		strcpy(str, "FALSE");
	fprintf(fd, "%s=%s\n", var, str);
}

static void check_iconify(void)
{
	WINDOW	*w;
	TEXTWIN *t;

	for (w = gl_winlist; w; w = w->next)
	{
		t = w->extra;
		if (t->cfg != gl_wincfg)
			t->cfg->iconified = (w->flags & WICONIFIED);
	}
}

void config_save(void)
{
	WINCFG	*p;

	graf_mouse(BEE, NULL);
	fd = fopen(cfg_path, "w");
	if (fd != NULL)
	{
		/* ID zur Identifikation */
		fprintf(fd, "ID=TosWin2 %s\n", TWVERSION);

		write_bool("AllLoginshell", gl_allogin);
		write_bool("ConsAutoOpen", gl_con_auto);
		write_bool("ConsLogging", gl_con_log);
		write_str("ConsLogName", gl_con_logname);
		write_bool("ConsOutputOpen", gl_con_output);
		write_bool("MenuShortcut", gl_shortcut);
		write_bool("WinCycleAV", gl_avcycle);

		check_iconify();
		p = gl_wincfg;
		while (p != NULL)
		{
			write_str("WinCfgBegin", p->progname);
			write_str("WinArg", p->arg);
			write_bool("WinAsIcon", p->iconified);
			write_bool("WinAutoClose", p->autoclose);
			write_bool("WinBlockCursor", p->blockcursor);
			write_int("WinCharTab", p->char_tab);
			write_int("WinCol", p->col);
			write_int("WinColorBg", p->bg_color);
			write_int("WinColorFg", p->fg_color);
			write_int("WinFontID", p->font_id);
			write_int("WinFontPts", p->font_pts);
			write_int("WinKind", p->kind);
			if (p->xpos >= 0)
				write_int("WinPosX", (int)(p->xpos * 10000L / (gl_desk.g_x + gl_desk.g_w - 1)));
			else
				write_int("WinPosX", -1);
			if (p->ypos >= 0)
				write_int("WinPosY", (int)(p->ypos * 10000L / (gl_desk.g_y + gl_desk.g_h - 1)));
			else
				write_int("WinPosY", -1);
			write_int("WinRow", p->row);
			write_int("WinScroll", p->scroll);
			write_str("WinTitle", p->title);
			write_int("WinVtMode", p->vt_mode);
			write_int("WinVDIColors", p->vdi_colors);
			write_ulong("WinFGEffects", p->fg_effects);
			write_ulong("WinBGEffects", p->bg_effects);
			write_str("WinCfgEnd", p->progname);
			p = p->next;
		}

		fclose(fd);
		fd = NULL;
	}
	else
		alert(1, 0, SAVEERR);
	graf_mouse(ARROW, NULL);
}


/******************************************************************************/
void config_init(void)
{
	OBJECT *tmp;

	rsrc_gaddr(R_TREE, CONCONFIG, &tmp);
	fix_dial(tmp);
	con_wd = create_wdial(tmp, winicon, 0, open_conwd, exit_conwd);

	rsrc_gaddr(R_TREE, WINCONFIG, &tmp);
	fix_dial(tmp);
	cfg_wd = create_wdial(tmp, winicon, WCOL, open_cfgwd, exit_cfgwd);
	/* Farbpopup */
	init_colorpop(COLOR_BITS);
	fix_colorpopobj(tmp, WFGCOL, 0);
	fix_colorpopobj(tmp, WBGCOL, 0);

	rsrc_gaddr(R_TREE, POPUPS, &popups);
	fix_dial(popups);

	/* Default-Cfg in die Liste, wird beim Laden ggf. berschrieben! */
	crt_newcfg("default");
}

void config_term(void)
{
	delete_wdial(con_wd);
	delete_wdial(cfg_wd);
	exit_colorpop();
}
