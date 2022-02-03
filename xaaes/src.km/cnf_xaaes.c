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
#include "taskman.h"

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
//static PCB_T	pCB_close_lastwind;
static PCB_A	pCB_app_options;
static PCB_A    pCB_cancel;
static PCB_A    pCB_filters;
static PCB_A	pCB_ctlalta_survivors;
static PCB_A	pCB_kill_without_question;
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
	{ "LAUNCHER",              PI_R_T,     cfg.launch_path         , { dat: sizeof(cfg.launch_path) } },
	{ "CLIPBOARD",             PI_R_T,     cfg.scrap_path          , { dat: sizeof(cfg.scrap_path)  } },
	{ "ACCPATH",               PI_R_T,     cfg.acc_path            , { dat: sizeof(cfg.acc_path)    } },
	{ "WIDGETS",               PI_R_T,     cfg.widg_name           , { dat: sizeof(cfg.widg_name)   } },
	{ "RESOURCE",              PI_R_T,     cfg.rsc_name            , { dat: sizeof(cfg.rsc_name)    } },
	{ "USEHOME",               PI_R_B,   & cfg.usehome		},
	{ "NAES_COOKIE",           PI_R_B,   & cfg.naes_cookie		},
	{ "MENUPOP_PIDS",          PI_R_B,   & cfg.menupop_pids		},
	{ "LRMB_SWAP",	           PI_R_B,   & cfg.lrmb_swap		},
	{ "WIDGET_AUTO_HIGHLIGHT", PI_R_B,   & cfg.widg_auto_highlight	},
	{ "REMAP_CICONS",          PI_R_B,   & cfg.remap_cicons		},
	{ "SET_RSC_PALETTE",       PI_R_B,   & cfg.set_rscpalette	},
	{ "ALERT_WINDOWS",	   PI_R_US,  & cfg.alert_winds		},
	{ "FONT_ID",               PI_R_S,   & cfg.font_id		},
	{ "STANDARD_POINT",        PI_R_S,   & cfg.standard_font_point	},
	{ "MEDIUM_POINT",          PI_R_S,   & cfg.medium_font_point	},
	{ "SMALL_POINT",           PI_R_S,   & cfg.small_font_point	},
	{ "POPSCROLL",             PI_R_S,   & cfg.popscroll		},
	{ "DC_TIME",               PI_R_S,   & cfg.double_click_time	},
	{ "MP_TIMEGAP",            PI_R_S,   & cfg.mouse_packet_timegap },
	{ "VIDEO",	           PI_R_S,   & cfg.videomode		},
	{ "REDRAW_TIMEOUT",        PI_R_S,   & cfg.redraw_timeout, Range(0, 32000)	},
	{ "POPUP_TIMEOUT",	   PI_R_S,   & cfg.popup_timeout, Range(0, 32000)	},
	{ "POPOUT_TIMEOUT",        PI_R_S,   & cfg.popout_timeout, Range(0, 32000)	},

	/* config settings */
	{ "SETENV",                PI_C_TT,  pCB_setenv		},
	
	{ "TOPPAGE",               PI_V_T,   pCB_toppage		},
	{ "NEXT_ACTIVE",           PI_V_T,   pCB_next_active		},
//	{ "CLOSE_LASTWIND",        PI_V_T,   pCB_close_lastwind		},
	{ "FOCUS",                 PI_V_T,   pCB_point_to_type		},
	{ "APP_OPTIONS",           PI_V_A,   pCB_app_options		},
	{ "CANCEL",                PI_V_A,   pCB_cancel			},
	{ "FILTERS",               PI_V_A,   pCB_filters		},
	{ "CTLALTA_SURVIVORS",	   PI_V_A,   pCB_ctlalta_survivors	},
	{ "KILL_WO_QUESTION",	   PI_V_A,   pCB_kill_without_question	},
	{ "MENU",                  PI_V_T,   pCB_menu			},
	{ "HELPSERVER",            PI_V_A,   pCB_helpserver		},
	
	/* Mouse wheel settings */
	{ "VERTICAL_WHEEL_ID",       PI_R_S,   & cfg.ver_wheel_id         },
	{ "HORIZONTAL_WHEEL_ID",     PI_R_S,   & cfg.hor_wheel_id         },
	{ "VERTICAL_WHEEL_AMOUNT",   PI_R_S,   & cfg.ver_wheel_amount, Range(1, 20) },
	{ "HORIZONTAL_WHEEL_AMOUNT", PI_R_S,   & cfg.hor_wheel_amount, Range(1, 20) },
	
	/* Iconify options */
	{ "ICNFY_ORIENT",	PI_R_S, &cfg.icnfy_orient		},
	{ "ICNFY_LEFT",		PI_R_S, &cfg.icnfy_l_x			},
	{ "ICNFY_RIGHT",	PI_R_S, &cfg.icnfy_r_x			},
	{ "ICNFY_TOP",		PI_R_S, &cfg.icnfy_t_y			},
	{ "ICNFY_BOTTOM",	PI_R_S, &cfg.icnfy_b_y			},
	{ "ICNFY_WIDTH",	PI_R_S, &cfg.icnfy_w			},
	{ "ICNFY_HEIGHT",	PI_R_S, &cfg.icnfy_h			},
	{ "ICNFY_REORDER_TO",	PI_R_S, &cfg.icnfy_reorder_to, Range(0, 32000) },

	{ "GRADIENTS",		PI_R_S, &cfg.gradients			},

	/* startup things */
	{ "SHELL",                 PI_V_ATK, pCB_shell			},
	{ "RUN",                   PI_C_TA,  pCB_run			},
	
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
get_commadelim_string(char **line)
{
	char *s, *ret;

	s = *line = skip(*line);

	while (*s
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

/*
 * Ozk: Very rough implementation -- clean and make this safer later..
 */
static short
get_argval(char *wfarg, short *result)
{
	bool negative = false;
	char *end;

	wfarg = skip(wfarg);

	if (*wfarg == '-')
	{
		negative = true;
		wfarg++;
	}

	end = wfarg;
					
	while (isdigit(*end))
		end++;

	if (end == wfarg)
	{
		DIAGS(("get_argval: no argument!"));
		return -1;
	}
	else
	{
		short r;
		char sc = *end;
		*end = 0;
		r = (short)atol(wfarg);
		*end = sc;
		if (result)
			*result = negative ? -r : r;
	}
	return 0;
}

static short
get_argument(char *wfarg, short *result)
{
	DIAGS(("get_argument: string = '%s'", wfarg));

	wfarg = skip(wfarg);

	if (*wfarg == '=')
		wfarg++;
	else
	{
		DIAGS(("get_argument: equation expected"));
		return -1;
	}
	return get_argval(wfarg, result);
}

static short
get_boolarg(char *s, bool *result)
{
	short ret = 0;
	char *end;

	DIAGS(("get_boolarg: string = '%s'", s));

	s = skip(s);

	if (*s == '=')
		s++;
	else
	{
		DIAGS(("get_boolarg: equation expected"));
		return -1;
	}

	s = skip(s);

	end = s;
	if (isdigit(*end))
	{
		while (isdigit(*end))
			end++;
					
		if (end == s)
		{
			DIAGS(("get_boolarg: no argument!"));
			ret = -1;
		}
		else
		{
			char sc = *end;
			*end = 0;
			ret = (short)atol(s);
			*end = sc;
			if (ret < 0 || ret > 1)
			{
				DIAGS(("get_boolarg: Invalid argument value %d", ret));
				ret = -1;
			}
		}
	}
	else if (!stricmp(s, "true") ||
		 !stricmp(s, "on")   ||
		 !stricmp(s, "yes"))
	{
		ret = 1;
	}
	else if (!stricmp(s, "false") ||
		 !stricmp(s, "off")   ||
		 !stricmp(s, "no"))
	{
		ret = 0;
	}

	if (ret >= 0)
	{
		if (result)
			*result = ret ? true : false;
		ret = 0;
	}
	
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
pCB_toppage(char *str)
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
pCB_next_active(char *str)
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
#if 0
static void
pCB_close_lastwind(char *str)
{
	if (stricmp(str, "client") == 0)
	{
		cfg.last_wind = 1;
	}
	else if (stricmp(str, "window") == 0)
	{
		cfg.last_wind = 0;
	}
	else
	{
		// XXX print error, invalid string
	}
	DIAGS(("pCB_close_lastwind: %s (last_wind %i)",
		str, cfg.last_wind));
}
#endif

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
pCB_cancel(char *line)
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
static void
pCB_app_options(char *line)
{
	struct opt_list *op = S.app_options;
	struct options *opts = NULL;
	char *s;

	if ((s = get_string(&line)))
	{
		DIAGS(("pCB_app_options for %s", s));

		if (!stricmp(s, "default"))
			opts = &default_options;
		else if (!stricmp(s, "aessys"))
			opts = &C.Aes->options;
		else
		{
			op = S.app_options;

			while (op)
			{
				if (!stricmp(s, op->name))
				{
					DIAGS(("pCB_app_options: already defined"));
					opts = &op->options;
					break;
				}
				op = op->next;
			}
		}

		if (!opts)
		{
			op = kmalloc(sizeof(*op));
			if (!op)
			{
				DIAGS(("pCB_app_options: Could not allocate memory for app_options - out of memory?"));
				return;
			}
			else
			{
				DIAGS(("pCB_app_options: new entry %s", s));
				bzero(op, sizeof(*op));
				op->next = S.app_options;
				S.app_options = op;
				strcpy(op->name, s);
				opts = &op->options;
				*opts = default_options;
				
			}
		}

		DIAGS(("opts = %lx (aesys=%lx, default=%lx)",
			opts, (long)&C.Aes->options, (long)&default_options));

		while ((s = get_commadelim_string(&line)))
		{
			DIAGS(("pCB_app_options: arg = '%s'", s));

			if (!strnicmp(s, "windowner", 9))
				get_boolarg(s + 9, &opts->windowner);
			else if (!strnicmp(s, "nohide", 6))
				get_boolarg(s + 6, &opts->nohide);
			else if (!strnicmp(s, "xa_nohide", 9))
				get_boolarg(s + 9, &opts->xa_nohide);
			else if (!strnicmp(s, "xa_nomove", 9))
				get_boolarg(s + 9, &opts->xa_nomove);
		//	else if (!strnicmp(s, "xa_objced", 9))
		//		get_boolarg(s + 9, &opts->xa_objced);
			else if (!strnicmp(s, "xa_none", 7))
				get_boolarg(s + 7, &opts->xa_none);
			else if (!strnicmp(s, "noleft", 6))
				get_boolarg(s + 6, &opts->noleft);
			else if (!strnicmp(s, "thinwork", 8))
				get_boolarg(s + 8, &opts->thinwork);
			else if (!strnicmp(s, "nolive", 6))
				get_boolarg(s + 6, &opts->nolive);
			else if (!strnicmp(s, "wheel_reverse", 13))
				get_boolarg(s + 13, &opts->wheel_reverse);
			else if (!strnicmp(s, "naesff", 6))
				get_boolarg(s + 7, &opts->naes_ff);
			else if (!strnicmp(s, "naes12", 6))
				get_boolarg(s + 6, &opts->naes12);
			else if (!strnicmp(s, "naes", 4))
				get_boolarg(s + 4, &opts->naes);
			else if (!strnicmp(s, "winframe_size", 13))
				get_argument(s + 13, &opts->thinframe);
			else if (!strnicmp(s, "inhibit_hide", 12))
				get_boolarg(s + 12, &opts->inhibit_hide);
			else if (!strnicmp(s, "clwtna", 6))
				get_argument(s + 6, &opts->clwtna); //get_boolarg(s + 6, &opts->clwtna);

#if GENERATE_DIAGS
			else
				DIAGS(("pCB_app_options: unknown keyword %s", s));
#endif
		}
	}
}

/*----------------------------------------------------------------------------*/
static void
pCB_filters(char *line)
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

static char *
isolate_strarg(char **str)
{
	char *s, *e, *ret = NULL;

	DIAGS(("isolate_strarg: start=%lx", *str));

	s = *str;

	s = skip(s);
	
	DIAGS((" -- remaining='%s'", s));
	
	if (*s == 0x22 || *s == 0x27)
	{
		s++;
		e = s;
		while (1)
		{
			if (!*e || (*e && (*e == '\r' || *e == '\n' || *e == ',')))
			{
				DIAGS(("isolate_string: missing end quote!"));
				break;
			}
			
			if (*e != 0x5c && (*e == 0x22 || *e == 0x27))
			{
				ret = s;
				if (*e)
					*e++ = 0;
				break;
			}
			e++;
		}
#if GENERATE_DIAGS
		if (ret)
		{
			DIAGS((" -- got quoted string = '%s', start of next = %lx", ret, e));
		}
		else
			DIAGS((" -- some error"));
#endif
	}
	else
	{
		e = s;

		while (*e
			&& *e != '\r'
			&& *e != '\n'
			&& *e != ',') e++;
	
		if (s != e)
		{
			ret = s;
			if (*e)
				*e++ = 0;
		}
		
#if GENERATE_DIAGS
		if (ret)
		{
			DIAGS((" -- got unquoted string = '%s', start of next = %lx", ret, e));
		}
		else
			DIAGS((" -- some unquote error"));
#endif		
	}
	
	if (ret)
	{
		*str = e;
	}
	
	return ret;
}


static void
pCB_ctlalta_survivors(char *line)
{
	char *s;

	while ((s = isolate_strarg(&line)))
	{
		DIAGS(("pCB_ctlalta_surv: Got string '%s'", s));
		addto_namelist(&cfg.ctlalta, s);
	}
}
static void
pCB_kill_without_question(char *line)
{
	char *s;

	while ((s = isolate_strarg(&line)))
	{
		DIAGS(("pCB_kill_without_question: Got string '%s'", s));
		addto_namelist(&cfg.kwq, s);
	}
}	
/*----------------------------------------------------------------------------*/
static void
pCB_menu(char *line)
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
pCB_helpserver(char *line)
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
				DIAGS(("pCB_run[%i]:(%lx) '%s'", i, cfg.cnf_run[i], cfg.cnf_run[i]));
				DIAGS(("pCB_run[%i]:(%lx) args '%s'", i, cfg.cnf_run_arg[i], cfg.cnf_run_arg[i]));
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

#if GENERATE_DIAGS
static void
diags_opts(struct options *o)
{
	DIAGS(("        windowner  = %s", o->windowner ? "true" : "false"));
	DIAGS(("        nohide     = %s", o->nohide    ? "true" : "false"));
	DIAGS(("        naes       = %s", o->naes      ? "true" : "false"));
	DIAGS(("        naes12     = %s", o->naes12    ? "true" : "false"));

	DIAGS(("        xa_nohide  = %s", o->xa_nohide ? "true" : "false"));
	DIAGS(("        xa_nomove  = %s", o->xa_nomove ? "true" : "false"));
	DIAGS(("        noleft     = %s", o->noleft    ? "true" : "false"));
	DIAGS(("        thinwork   = %s", o->thinwork  ? "true" : "false"));
	DIAGS(("        nolive     = %s", o->nolive    ? "true" : "false"));
	DIAGS(("        winframe   = %d", o->thinframe));
}

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
    parse_cnf(path, parser_tab, &mydata, 0);

#if GENERATE_DIAGS
	{
		struct opt_list *op = S.app_options;

		DIAGS(("Options for:"));
		DIAGS(("    AESSYS"));
		diags_opts(&C.Aes->options);
		DIAGS(("    DEFAULT"));
		diags_opts(&default_options);
		
		while (op)
		{
			DIAGS(("    '%s'", op->name));
			diags_opts(&op->options);
			op = op->next;
		}
	}
#endif
}

