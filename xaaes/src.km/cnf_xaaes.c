/*
 * $Id$
 * 
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for MiNT
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

#include "xa_types.h"
#include "xa_global.h"
#include "xa_shel.h"

#include "cnf_xaaes.h"
#include "init.h"
#include "xalloc.h"

#include "mint/fcntl.h"
#include "cnf.h"


struct lconfig lcfg =
{
	WIDGNAME,
	RSCNAME,

	0,

	0,
	DOUBLE_CLICK_TIME
};

struct cnfdata
{
};

/*** First the command callback declarations: */

/*
 * setenv name val ..... set up environment
 */

static PCB_TTx	pCB_setenv;		/* setenv name val	*/


/*** Now the variable callback deklarations: */

/*
 * toppage=bold|faint
 * cancel=string,string
 * filters=
 * menu=pull,push,leave,nolocking
 */

static PCB_T	pCB_toppage;
static PCB_A    pCB_cancel;
static PCB_A    pCB_filters;
static PCB_T    pCB_menu;
static PCB_Tx   pCB_shell;
static PCB_Tx   pCB_run;


/*** The variable reference declarations: */

/*
 * LAUNCHER=path
 * CLIPBOARD=path
 * ACCPATH=path
 * NOFSEL=[yn]
 * FOCUS=POINT
 * USEHOME=[yn]
 * FONT_ID=n
 * STANDARD_POINT=n
 * MEDIUM_POINT=n
 * SMALL_POINT=n
 * POPSCROLL=n
 * WIDGETS=file
 * RESOURCE=file
 * DC_TIME=n
 */

/* The item table, note the 'NULL' entry at the end. */

#if !POINT_TO_TYPE
#define pCB_point_to_type NULL
#endif

static struct parser_item parser_tab[] =
{
	/* generic */
	{ "SETENV",         PI_C_TT,  pCB_setenv                },
	
	/* global config */
	{ "LAUNCHER",       PI_R_T,     cfg.launch_path         , sizeof(cfg.launch_path) },
	{ "CLIPBOARD",      PI_R_T,     cfg.scrap_path          , sizeof(cfg.scrap_path)  },
	{ "ACCPATH",        PI_R_T,     cfg.acc_path            , sizeof(cfg.acc_path)    },
	{ "TOPPAGE",        PI_V_T,   pCB_toppage               },
	{ "NOFSEL",         PI_R_B,   & cfg.no_xa_fsel          },
	{ "FOCUS",          PI_V_T,   pCB_point_to_type         },
	{ "USEHOME",        PI_R_B,   & cfg.usehome             },
	{ "CANCEL",         PI_V_A,   pCB_cancel                },
	{ "FILTERS",        PI_V_A,   pCB_filters               },
	{ "MENU",           PI_V_T,   pCB_menu                  },
	{ "FONT_ID",        PI_R_S,   & cfg.font_id             },
	{ "STANDARD_POINT", PI_R_S,   & cfg.standard_font_point },
	{ "MEDIUM_POINT",   PI_R_S,   & cfg.medium_font_point   },
	{ "SMALL_POINT",    PI_R_S,   & cfg.small_font_point    },
	{ "POPSCROLL",      PI_R_S,   & cfg.popscroll           },
	
	/* bootup config */
	{ "WIDGETS",        PI_R_T,     lcfg.widg_name          , sizeof(lcfg.widg_name)  },
	{ "RESOURCE",       PI_R_T,     lcfg.rsc_name           , sizeof(lcfg.rsc_name)   },
	{ "DC_TIME",        PI_R_S,   & lcfg.double_click_time  },
	
	{ "SHELL",          PI_V_T,   pCB_shell                 },
	{ "DESK",           PI_V_T,   pCB_shell                 },
	{ "EXEC",           PI_V_T,   pCB_run                   },
	{ "RUN",            PI_V_T,   pCB_run                   },
	
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

	if (len > sizeof(p))
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
	DIAGS(("pCB_toppage: %s (topname %i, backname %i)",
		str, cfg.topname, cfg.backname));
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
	int i = 0;
	
	/* XXX array size hardocoded */
	while (i < 23)
	{
		char *s;
		
		/* skip any space */
		line = skip(line);
		
		s = get_delim_string(&line);
		if (!s)
			break;
		
		if (strlen(s) < 16)
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

		DIAGS(("pCB_menu: %s (menu_locking %i)", s, cfg.menu_locking));
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_shell(const char *path, struct parsinf *inf)
{
	/* remember */
	strcpy(cfg.cnf_shell, path);
	DIAGS(("pCB_shell -> %s", path));
}

/*----------------------------------------------------------------------------*/
static void
pCB_run(const char *path, struct parsinf *inf)
{
	int i;

	for (i = 0; i < sizeof(cfg.cnf_run)/sizeof(cfg.cnf_run[0]); i++)
	{
		if (!cfg.cnf_run[i])
		{
			cfg.cnf_run[i] = xmalloc(strlen(path)+1, 23);
			if (cfg.cnf_run[i])
			{
				strcpy(cfg.cnf_run[i], path);
				DIAGS(("pCB_run[%i]: %s", i, path));
			}
		}
	}
}

/*============================================================================*/
/* find cnf file
 */

/*
 * Read & parse the '.scl' files.
 */
#if GENERATE_DIAGS
#define debugp(d,s) curopt->point[d] = s
#endif

void
SCL(const char *name)
{
	struct cnfdata mydata;
//	struct options *curopt = &default_options;	/* specify some options for some programs. */
//	bool have_brace = false, have_opt = false;
//	char *cnf = NULL;

	if (name)
	{
		DIAGS(("Loading config %s", name));
		parse_cnf(name, parser_tab, &mydata);
	}
	else
		ALERT(("no config filename?\n"));

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
