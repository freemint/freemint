/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999 Ralph Lowinski <ralph@aquaplan.de>
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef MINT_CNF_PARSER

#include "cnf_xaaes.h"

#include <mint/mintbind.h>

#include "xa_types.h"
#include "xa_global.h"
#include "xa_shel.h"

#include "cnf.h"
#include "config.h"
#include "display.h"
#include "global.h"
#include "xalloc.h"


struct options default_options;
struct options local_options;

struct lconfig lcfg =
{
	WIDGNAME,
	RSCNAME,

	0,
	0,

	0,
	DOUBLE_CLICK_TIME,
	0,

	false,
	false,
	false
};

struct cnfdata
{
	LOCK lock;
};

/*** First the command callback declarations: */

/*
 * setenv name val ..... set up environment
 */

static PCB_TTx	pCB_setenv;		/* setenv name val	*/


/*** Now the variable callback deklarations: */

/*
 * priority=n
 * toppage=bold|faint
 * cancel=string,string
 * filters=
 * menu=pull,push,leave,nolocking
 */

static PCB_L    pCB_priority;
static PCB_T	pCB_toppage;
static PCB_T	pCB_point_to_type;
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
 * SUPERPROGDEF=[yn]
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
	{ "PRIORITY",       PI_V_L,   pCB_priority              },
	{ "SETENV",         PI_C_TT,  pCB_setenv                },
	
	/* global config */
	{ "LAUNCHER",       PI_R_T,     cfg.launch_path         , sizeof(cfg.launch_path) },
	{ "CLIPBOARD",      PI_R_T,     cfg.scrap_path          , sizeof(cfg.scrap_path)  },
	{ "ACCPATH",        PI_R_T,     cfg.acc_path            , sizeof(cfg.acc_path)    },
	{ "TOPPAGE",        PI_V_T,   pCB_toppage               },
	{ "NOFSEL",         PI_R_B,   & cfg.no_xa_fsel          },
	{ "SUPERPROGDEF",   PI_R_B,   & cfg.superprogdef        },
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
void
pCB_priority(long prio)
{
	if (prio >= -20 && prio <= 20)
		Psetpriority(0, C.AESpid, prio);
}

/*----------------------------------------------------------------------------*/
static void
pCB_setenv(const char *var, const char *arg, struct parsinf *inf)
{
	struct cnfdata *mydata = inf->data;
	char p[512];
	
	strcpy(p, var);
	strcat(p, "=");
	strcat(p, arg);
	
	put_env(mydata->lock, 1, 0, p);
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
}

/*----------------------------------------------------------------------------*/
#if POINT_TO_TYPE
static void
pCB_point_to_type(const char *str)
{
	cfg.point_to_type = (stricmp(str, "point") == 0);
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
			strcpy(cfg.cancel_buttons[i++], s);
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
			strcpy(cfg.Filters[i++], s);
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
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_shell(const char *path, struct parsinf *inf)
{
	struct cnfdata *mydata = inf->data;
	Path parms;
	
	parms[0] = sdisplay(parms+1, "%s", path);
	
	C.DSKpid = launch(mydata->lock, 0, 0, 0, path, parms, C.Aes);
	if (C.DSKpid > 0)
		strcpy(C.desk, path);
}

/*----------------------------------------------------------------------------*/
static void
pCB_run(const char *path, struct parsinf *inf)
{
	struct cnfdata *mydata = inf->data;
	Path parms;
	
	parms[0] = sdisplay(parms+1, "%s", path);
	
	launch(mydata->lock, 0, 0, 0, path, parms, C.Aes);
}

/*============================================================================*/
/* find cnf file
 */

/*
 * find a xaaes file. When the cd command to XaAES load directory is
 * missing in mint.cnf (which is likely for inexperienced mint users. ;-) 
 * XaAES wont fail completely.
 */
static char **aes_path;
static char *dirs[] =
{
	"",					/* plain name only */
	Aes_home_path,				/* Dont forget to fill. */
	"c:\\gemsys\\",
	"c:\\gemsys\\xaaes\\",
	"c:\\aes\\",
	"c:\\aes\\xaaes\\",
	"c:\\mint\\",
	"c:\\mint\\xaaes\\",
	"c:\\multitos\\",
	"c:\\multitos\\xaaes\\",
	"c:\\",
	"c:\\xaaes\\",
	NULL
};

/* last resort if shell_find fails. */
char *
xa_find(char *fn)
{
	static char p[200];
	long h;
	char *f;

	DIAGS(("xa_find '%s'\n", fn ? fn : "~"));

	/* combined shell_find & xa_find permanently. */
	f = shell_find(NOLOCKING, C.Aes, fn);
	if (f)
		return f;
	else
	{
		aes_path = dirs;
		while (*aes_path)
		{
			char *pad = *aes_path;
			sdisplay(p,"%s%s", pad, fn);
#if GENERATE_DIAGS
			if (lcfg.booting)
				display("try open '%s'\n", p);
			else
				DIAGS(("%s\n", p));
#endif
			h = Fopen(p,0);
			if (h > 0)
			{
				Fclose(h);
				return p;
			}
			aes_path++;
		}
	}

	DIAGS((" - NULL\n"));
	return NULL;
}

/*
 * Read & parse the '.scl' files.
 */
#if GENERATE_DIAGS
#define debugp(d,s) curopt->point[d] = s
#endif

/* since xa_scl, SCL needs lock */
void
SCL(LOCK lock, int co, char *name, char *full, char *txt)
{
	struct cnfdata mydata = { lock };
	struct options *curopt = &default_options;	/* specify some options for some programs. */
	bool have_brace = false, have_opt = false;
	char *cnf = NULL;

	if (lcfg.booting)
	{
		fdisplay(loghandle, false, "\n");
		fdisplay(loghandle, false, "**** Executing SCL phase %d from '%s' ****\n", co, name ? name : "string");
		fdisplay(loghandle, false, "\n");
	}

	if (name)
	{
		if (!full)
			full = xa_find(name);

		if (full)
			parse_cnf(full, parser_tab, &mydata);
		else
			display("SCL file '%s' absent\n", name);
	}
	else if (txt)
		display("XA_M_EXEC not supported yet!\n");
	else
		display("no config filename?\n");

#if GENERATE_DIAGS
	{
		OPT_LIST *op = S.app_options;

		err = 1;

		display("Options for:\n");
		while (op)
		{
			display("    '%s'\n", op->name);
			op = op->next;
		}

		list_sym();
	}
#endif

	if (err && co != 1 IFDIAG(&& !D.debug_file))
	{
		display("hit any key\n");
		Bconin(2);
	}
}

#endif /* MINT_CNF_PARSER */
