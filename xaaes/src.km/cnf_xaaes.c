/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "cnf_xaaes.h"

#include "xa_global.h"
#include "xa_shel.h"

#include "cnf_xaaes.h"
#include "init.h"

#include "mint/fcntl.h"
#include "cnf.h"


struct cnfdata
{
};


/* callback declarations: */

/*
 * setenv name val ..... set up environment
 * 
 * toppage=bold|faint
 * cancel=string,string
 * filters=
 * menu=pull,push,leave,nolocking
 * 
 * shell=
 * run=
 */

static PCB_TTx	pCB_setenv;		/* setenv name val	*/

static PCB_T	pCB_toppage;
static PCB_T	pCB_next_active;
static PCB_A    pCB_cancel;
static PCB_A    pCB_filters;
static PCB_T    pCB_menu;
static PCB_A    pCB_helpserver;

static PCB_ATK  pCB_shell;
static PCB_TAx  pCB_run;


/* The item table, note the 'NULL' entry at the end. */

#if !POINT_TO_TYPE
#define pCB_point_to_type NULL
#endif

static struct parser_item parser_tab[] =
{
	/* config variables */
	{ "LAUNCHER",       PI_R_T,     cfg.launch_path         , sizeof(cfg.launch_path) },
	{ "CLIPBOARD",      PI_R_T,     cfg.scrap_path          , sizeof(cfg.scrap_path)  },
	{ "ACCPATH",        PI_R_T,     cfg.acc_path            , sizeof(cfg.acc_path)    },
	{ "WIDGETS",        PI_R_T,     cfg.widg_name           , sizeof(cfg.widg_name)  },
	{ "RESOURCE",       PI_R_T,     cfg.rsc_name            , sizeof(cfg.rsc_name)   },
	{ "USEHOME",        PI_R_B,   & cfg.usehome             },
	{ "FONT_ID",        PI_R_S,   & cfg.font_id             },
	{ "STANDARD_POINT", PI_R_S,   & cfg.standard_font_point },
	{ "MEDIUM_POINT",   PI_R_S,   & cfg.medium_font_point   },
	{ "SMALL_POINT",    PI_R_S,   & cfg.small_font_point    },
	{ "POPSCROLL",      PI_R_S,   & cfg.popscroll           },
	{ "DC_TIME",        PI_R_S,   & cfg.double_click_time   },
	{ "VIDEO",	    PI_R_S,   & cfg.videomode		},
	
	/* config settings */
	{ "SETENV",         PI_C_TT,  pCB_setenv                },
	
	{ "TOPPAGE",        PI_V_T,   pCB_toppage               },
	{ "NEXT_ACTIVE",    PI_V_T,   pCB_next_active           },
	{ "FOCUS",          PI_V_T,   pCB_point_to_type         },
	{ "CANCEL",         PI_V_A,   pCB_cancel                },
	{ "FILTERS",        PI_V_A,   pCB_filters               },
	{ "MENU",           PI_V_T,   pCB_menu                  },
	{ "HELPSERVER",     PI_V_A,   pCB_helpserver            },
	
	/* startup things */
	{ "SHELL",          PI_V_ATK, pCB_shell                 },
	{ "RUN",            PI_C_TA,  pCB_run                   },
	
	/* debug */
	
	{ NULL }
};


/*============================================================================*/
/* Miscellaneous support routines
 */

#define CHAR2DRV(c) ((int)(strrchr(drv_list, toupper((int)c & 0xff)) - drv_list))

static char *
skip(char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\r')
		s++;

	return s;
}

static char *
get_delim_string(char **line)
{
	char *s, *ret;

	s = *line;

	while (*s
		&& *s != ' '
		&& *s != '\t'
		&& *s != '\r'
		&& *s != '\n'
		&& *s != ','
		&& *s != '|') s++;

	if (s == *line)
		return NULL;

	ret = *line;

	if (*s) *s++ = '\0';
	*line = s;

	return ret;
}

static char *
get_string(char **line)
{
	char *s, *ret;

	s = *line = skip(*line);

	for (;;)
	{
		int c = *s;

		if (!isalnum(c) && !(c == '_'))
			break;

		s++;
	}

	if (s == *line)
		return NULL;

	ret = *line;

	if (*s) *s++ = '\0';
	*line = s;

	return ret;
}

/*============================================================================*/
/* Now follows the callback definitions in alphabetical order
 */

/*----------------------------------------------------------------------------*/
static void
pCB_setenv(const char *var, const char *arg, struct parsinf *inf)
{
	char p[512];
	unsigned long len;

	len = strlen(var) + strlen(arg) + 2;

	if (len < sizeof(p))
	{
		strcpy(p, var);
		strcat(p, "=");
		strcat(p, arg);

		put_env(NOLOCKING, p);
		DIAGS(("pCB_setenv: %s=%s", var, arg));
	}
	else
		DIAGS(("pCB_setenv: len %lu > sizeof(p) %lu", len, sizeof(p)));
}

/*----------------------------------------------------------------------------*/
static void
pCB_toppage(const char *str)
{
	if (stricmp(str, "bold") == 0)
	{
		cfg.topname = BOLD;
		cfg.backname = 0;
	}
	else if (stricmp(str, "faint") == 0)
	{
		cfg.topname = 0;
		cfg.backname = FAINT;
	}
	else
	{
		// XXX print error, invalid string
	}

	DIAGS(("pCB_toppage: %s (topname %i, backname %i)",
		str, cfg.topname, cfg.backname));
}

static void
pCB_next_active(const char *str)
{
	if (stricmp(str, "client") == 0)
	{
		cfg.next_active = 1;
	}
	else if (stricmp(str, "window") == 0)
	{
		cfg.next_active = 0;
	}
	else
	{
		// XXX print error, invalid string
	}
	DIAGS(("pCB_next_active: %s (next_active %i)",
		str, cfg.next_active));
}
/*----------------------------------------------------------------------------*/
#if POINT_TO_TYPE
static void
pCB_point_to_type(const char *str)
{
	cfg.point_to_type = (stricmp(str, "point") == 0);
	DIAGS(("pCB_point_to_type = %s", cfg.point_to_type ? "true" : "false"));
}
#endif

/*----------------------------------------------------------------------------*/
static void
pCB_cancel(const char *line)
{
	int i = 0;

	while (i < (NUM_CB - 1)) /* last entry kept clear as a stopper */
	{
		char *s;

		s = get_string(&line);
		if (!s)
			break;

		if (strlen(s) < CB_L)
		{
			strcpy(cfg.cancel_buttons[i++], s);
			DIAGS(("pCB_cancel[%i]: %s", i-1, s));
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_filters(const char *line)
{
	const int max = sizeof(cfg.Filters)/sizeof(cfg.Filters[0]);
	int i = 0;

	DIAGS(("pCB_filters: max i = %i", max));

	while (i < max)
	{
		char *s;

		/* skip any space */
		line = skip(line);

		s = get_delim_string(&line);
		if (!s)
			break;

		if (strlen(s) < sizeof(cfg.Filters[0]))
		{
			strcpy(cfg.Filters[i++], s);
			DIAGS(("pCB_filters[%i]: %s", i-1, s));
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_menu(const char *line)
{
	for (;;)
	{
		char *s;

		s = get_string(&line);
		if (!s)
			break;

		if (stricmp(s, "pull") == 0)
			cfg.menu_behave = PULL;
		else if (stricmp(s, "push") == 0)
			cfg.menu_behave = PUSH;
		else if (stricmp(s, "leave") == 0)
			cfg.menu_behave = LEAVE;
		else if (stricmp(s, "nolocking") == 0)
			cfg.menu_locking = false;
		else
			; // XXX print error, invalid name

		DIAGS(("pCB_menu: %s (menu_locking %i)", s, cfg.menu_locking));
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_helpserver(const char *line)
{
	struct helpserver *hs;
	char *ext, *name, *path;
	size_t len;

	ext  = get_delim_string(&line);
	name = get_delim_string(&line);
	path = get_delim_string(&line);

	if (!ext || !name)
	{
		DIAGS(("pCB_helpserver: invalid helpserver config line"));
		// XXX print error message
		return;
	}

	DIAGS(("pCB_helpserver: ext '%s' name '%s' path '%s'",
		ext, name, path ? path : "<null>"));

	len = sizeof(*hs);
	len += strlen(ext) + 1;
	len += strlen(name) + 1;

	if (path)
		len += strlen(path) + 1;

	hs = kmalloc(len);
	if (hs)
	{
		bzero(hs, len);

		hs->ext = (char *)hs + sizeof(*hs);
		hs->name = hs->ext + strlen(ext) + 1;

		if (path)
			hs->path = hs->name + strlen(name) + 1;

		strcpy(hs->ext, ext);
		strcpy(hs->name, name);

		if (path)
			strcpy(hs->path, path);

		/* remember */
		{
			struct helpserver **list = &(cfg.helpservers);

			while (*list)
				list = &((*list)->next);

			*list = hs;
		}
	}
	else
	{
		DIAGS(("pCB_helpserver: out of memory"));
		// XXX print error message
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_shell(const char *path, const char *line, long data)
{
	if (cfg.cnf_shell)
	{
		kfree(cfg.cnf_shell);
		cfg.cnf_shell = NULL;
	}
	if (cfg.cnf_shell_arg)
	{
		kfree(cfg.cnf_shell_arg);
		cfg.cnf_shell_arg = NULL;
	}

	cfg.cnf_shell = kmalloc(strlen(path)+1);
	cfg.cnf_shell_arg = kmalloc(strlen(line)+1);

	if (cfg.cnf_shell && cfg.cnf_shell_arg)
	{
		strcpy(cfg.cnf_shell, path);
		strcpy(cfg.cnf_shell_arg, line);

		DIAGS(("pCB_shell: %s", path));
		DIAGS(("pCB_shell: args %s", line));
	}
	else
	{
		if (cfg.cnf_shell)
		{
			kfree(cfg.cnf_shell);
			cfg.cnf_shell = NULL;
		}
		if (cfg.cnf_shell_arg)
		{
			kfree(cfg.cnf_shell_arg);
			cfg.cnf_shell_arg = NULL;
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_run(const char *path, const char *line, struct parsinf *inf)
{
	int i;

	for (i = 0; i < sizeof(cfg.cnf_run)/sizeof(cfg.cnf_run[0]); i++)
	{
		if (!cfg.cnf_run[i])
		{
			cfg.cnf_run[i] = kmalloc(strlen(path)+1);
			cfg.cnf_run_arg[i] = kmalloc(strlen(line)+1);

			if (cfg.cnf_run[i] && cfg.cnf_run_arg[i])
			{
				strcpy(cfg.cnf_run[i], path);
				strcpy(cfg.cnf_run_arg[i], line);
				DIAGS(("pCB_run[%i]: %s", i, path));
				DIAGS(("pCB_run[%i]: args %s", i, line));
			}
			else
			{
				if (cfg.cnf_run[i])
				{
					kfree(cfg.cnf_run[i]);
					cfg.cnf_run[i] = NULL;
				}
				if (cfg.cnf_run_arg[i])
				{
					kfree(cfg.cnf_run_arg[i]);
					cfg.cnf_run_arg[i] = NULL;
				}
			}

			break;
		}
	}
}

/*============================================================================*/

/*
 * Read & parse the '.scl' files.
 */

#if GENERATE_DIAGS
#define CNF_NAME "xaaesdbg.cnf"
#else
#define CNF_NAME "xaaes.cnf"
#endif

void
load_config(void)
{
	char path[128];

	struct cnfdata mydata;
//	struct options *curopt = &default_options;	/* specify some options for some programs. */
//	bool have_brace = false, have_opt = false;
//	char *cnf = NULL;

	strcpy(path, C.Aes->home_path);
	strcat(path, CNF_NAME);

	DIAGS(("Loading config %s", path));
	parse_cnf(path, parser_tab, &mydata);

#if GENERATE_DIAGS
	{
		struct opt_list *op = S.app_options;

		DIAGS(("Options for:"));
		while (op)
		{
			DIAGS(("    '%s'", op->name));
			op = op->next;
		}
	}
#endif
}
