/*
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

#include "xa_global.h"
#include "xa_shel.h"
#include "about.h"
#include "xa_fsel.h"
#include "taskman.h"
#include "util.h"

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

static PCB_T	pCB_next_active;
static PCB_A	pCB_app_options;
static PCB_A    pCB_cancel;
static PCB_A    pCB_filters;
static PCB_A	pCB_ctlalta_survivors;
static PCB_A	pCB_kill_without_question;
static PCB_T    pCB_menu;
static PCB_A    pCB_keyboards;
static PCB_Tx    pCB_include;
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
	{ "LAUNCHPATH",            PI_R_T,     cfg.launch_path       , { dat: sizeof(cfg.launch_path) } },
	{ "CLIPBOARD",             PI_R_T,     cfg.scrap_path          , { dat: sizeof(cfg.scrap_path)  } },
	{ "SNAPSHOT",  	           PI_R_T,     cfg.snapper  		       , { dat: sizeof(cfg.snapper)  } },
	{ "ACCPATH",               PI_R_T,     cfg.acc_path            , { dat: sizeof(cfg.acc_path)    } },
	{ "LOGFILE",               PI_R_T,     C.bootlog_path          , { dat: sizeof(C.bootlog_path)    } },
	{ "LOGLVL",                PI_R_S,   & C.loglvl   },
	{ "SAVE_WINDOWS",          PI_R_B,   & cfg.save_windows},
	{ "LANG",                  PI_R_T,     cfg.lang		             , { dat: sizeof(cfg.lang)   } },
	{ "FOCUS",                 PI_R_T,     cfg.focus	             , { dat: sizeof(cfg.focus)   } },
	{ "PALETTE",               PI_R_T,     cfg.palette             , { dat: sizeof(cfg.palette)   } },
	{ "WIDGETS",               PI_R_T,     cfg.widg_name           , { dat: sizeof(cfg.widg_name)   } },
	{ "RESOURCE",              PI_R_T,     cfg.rsc_name            , { dat: sizeof(cfg.rsc_name)    } },
#if EIFFEL_SUPPORT
	{ "EIFFEL_SUPPORT",        PI_R_B,   & cfg.eiffel_support},
#endif
	{ "USEHOME",               PI_R_B,   & cfg.usehome		},
	{ "NAES_COOKIE",           PI_R_B,   & cfg.naes_cookie		},
	{ "MENUPOP_PIDS",          PI_R_B,   & cfg.menupop_pids		},
#if WITH_BBL_HELP
	{ "XA_BUBBLE",	           PI_R_S,   & cfg.xa_bubble},
	{ "DESCRIBE_WIDGETS",      PI_R_S,   & cfg.describe_widgets},
#endif
	{ "LRMB_SWAP",	           PI_R_B,   & cfg.lrmb_swap		},
	{ "WIDGET_AUTO_HIGHLIGHT", PI_R_B,   & cfg.widg_auto_highlight	},
	{ "LEAVE_TOP_BORDER",      PI_R_B,   & cfg.leave_top_border},
	{ "REMAP_CICONS",          PI_R_B,   & cfg.remap_cicons		},
	{ "SET_RSC_PALETTE",       PI_R_B,   & cfg.set_rscpalette	},
	{ "ALERT_WINDOWS",         PI_R_US,  & cfg.alert_winds		},
	{ "BACK_COL",              PI_R_S,   & cfg.back_col	},
	{ "MENUBAR",               PI_R_S,   & cfg.menu_bar	},
	{ "MENU_LAYOUT",           PI_R_S,   & cfg.menu_layout	},
	{ "MENU_ONTOP",            PI_R_B,   & cfg.menu_ontop	},
	{ "FONT_ID",               PI_R_S,   & cfg.font_id		},
	{ "STANDARD_POINT",        PI_R_S,   & cfg.standard_font_point	},
	{ "MEDIUM_POINT",          PI_R_S,   & cfg.medium_font_point	},
	{ "SMALL_POINT",           PI_R_S,   & cfg.small_font_point	},
	{ "INFOLINE_POINT",        PI_R_S,   & cfg.info_font_point	},
	{ "XAW_POINT",             PI_R_S,   & cfg.xaw_point		},
	{ "POPSCROLL",             PI_R_S,   & cfg.popscroll		},
	{ "DC_TIME",               PI_R_S,   & cfg.double_click_time	},
	{ "MP_TIMEGAP",            PI_R_S,   & cfg.mouse_packet_timegap },
	{ "VIDEO",                 PI_R_US,  & cfg.videomode		},
	{ "MODECODE",              PI_R_US,  & cfg.videomode		},
	{ "SCREENDEV",             PI_R_US,  & cfg.device		},
	{ "ALLOW_SETEXC",          PI_R_S,   & cfg.allow_setexc, Range(0, 2)	},
	{ "REDRAW_TIMEOUT",        PI_R_S,   & cfg.redraw_timeout, Range(0, 32000)	},
	{ "POPUP_TIMEOUT",	       PI_R_S,   & cfg.popup_timeout, Range(0, 32000)	},
	{ "POPOUT_TIMEOUT",        PI_R_S,   & cfg.popout_timeout, Range(0, 32000)	},

	/* config settings */
	{ "SETENV",                PI_C_TT,  pCB_setenv		},

	{ "NEXT_ACTIVE",           PI_V_T,   pCB_next_active		},
	{ "INCLUDE",               PI_C_T,		pCB_include	},
	{ "APP_OPTIONS",           PI_V_A,   pCB_app_options		},
	{ "CANCEL",                PI_V_A,   pCB_cancel			},
	{ "KEYBOARDS",             PI_V_A,		pCB_keyboards	},
	{ "FILTERS",               PI_V_A,   pCB_filters		},
	{ "CTLALTA_SURVIVORS",	   PI_V_A,   pCB_ctlalta_survivors	},
	{ "KILL_WO_QUESTION",	     PI_V_A,   pCB_kill_without_question	},
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

#if WITH_GRADIENTS
	{ "GRADIENTS",		PI_R_T, cfg.gradients,       { dat: sizeof(cfg.gradients) } },
#endif
	{ "TEXTURES", 		PI_R_T, cfg.textures,       { dat: sizeof(cfg.textures) } },
	{ "TEXTURES_CACHE",	PI_R_B, &cfg.textures_cache		},
	/* startup things */
	{ "SHELL",                 PI_V_ATK, pCB_shell			},
	{ "RUN",                   PI_C_TA,  pCB_run			},

	/* debug */

	{ NULL }
};

/*============================================================================*/
/* Miscellaneous support routines
 */

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
		/*&& *s != '|'*/) s++;

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

#ifndef ST_ONLY
static char*
get_str_arg(char *wfarg)
{
	char *ret, *p, c = 0;
	DIAGS(("get_argument: string = '%s'", wfarg));

	wfarg = skip(wfarg);

	if (*wfarg == '=')
		wfarg++;
	else
	{
		DIAGS(("get_str_arg: equation expected"));
		return 0;
	}
	wfarg = skip(wfarg);
	for( p = wfarg; *p > ' '; p++ )
	;
	if( *p )
	{
		c = *p;
		*p = 0;
	}
	ret = xa_strdup( wfarg );
	*p = c;
	return ret;
}
#endif
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
#if 0
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
#endif
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

/*
static void
pCB_ctrl_alt_run(char *line)
{
	BLOG((0,"ctrl_alt_run: '%s'", line));
}
*/
/*----------------------------------------------------------------------------*/
static void
pCB_include(char *path, struct parsinf *inf)
{
	BLOG((0,"include: '%s'", path));
	parse_include(path, inf, parser_tab);	// should return error-value
}

/*----------------------------------------------------------------------------*/
static void
pCB_keyboards(char *line)
{
	int i;

	for( cfg.keyboards.c = *line; *line && *line != ','; line++ );
	if( !*line++ )
		return;
	if( cfg.keyboards.c == ',' )
		cfg.keyboards.c = 0xa;	// Enter
	for( i = 0; i < MAX_KEYBOARDS; i++ )
	{
		char *s;

		s = get_string(&line);
		if (!s)
			break;
		BLOG((0,"keyboard#%d=%s",i,s));

		strncpy(cfg.keyboards.keyboard[i], s, sizeof(cfg.keyboards.keyboard[i])-1);

	}
	cfg.keyboards.cur = -1;	//undef.
}
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
	short a;
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
			(unsigned long)opts, (unsigned long)&C.Aes->options, (unsigned long)&default_options));

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
			else if (!strnicmp(s, "rsc_lang", 8))
				get_argument(s + 8, &opts->rsc_lang);
			else if (!strnicmp(s, "ignore_rsc_size", 15))
				get_argument(s + 15, &opts->ignore_rsc_size);
#ifndef ST_ONLY
			else if (!strnicmp(s, "icn_pal_name", 12))
				opts->icn_pal_name = get_str_arg( s + 12 );
#endif
			else if (!strnicmp(s, "winframe_size", 13))
				get_argument(s + 13, &opts->thinframe);
			else if (!strnicmp(s, "submenu_indicator", 17))
				get_argument(s + 17, &opts->submenu_indicator);
			else if (!strnicmp(s, "inhibit_hide", 12))
				get_boolarg(s + 12, &opts->inhibit_hide);
			else if (!strnicmp(s, "clwtna", 6))
			{
				get_argument(s + 6, &a); //get_boolarg(s + 6, &opts->clwtna);
				opts->clwtna = a;
			}
			else if (!strnicmp(s, "alt_shortcuts", 13))
			{
				get_argument(s + 13, &a);
				opts->alt_shortcuts = a;
			}
			else if (!strnicmp(s, "standard_font_point", 19))
			{
				get_argument(s + 19, &a);
				opts->standard_font_point = a;
			} else
			{
				DIAGS(("pCB_app_options: unknown keyword %s", s));
			}
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
			strcpy(cfg.Filters[i++], s);	/* todo: malloc this */
			DIAGS(("pCB_filters[%i]: %s", i-1, s));
		}
	}
}

static char *
isolate_strarg(char **str)
{
	char *s, *e, *ret = NULL;

	DIAGS(("isolate_strarg: start=%lx", (unsigned long)*str));

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
		if (ret)
		{
			DIAGS((" -- got quoted string = '%s', start of next = %lx", ret, (unsigned long)e));
		} else
		{
			DIAGS((" -- some error"));
		}
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

		if (ret)
		{
			DIAGS((" -- got unquoted string = '%s', start of next = %lx", ret, (unsigned long)e));
		} else
		{
			DIAGS((" -- some unquote error"));
		}
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

		if (stricmp(s, "push") == 0)
			cfg.menu_behave = PUSH;
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
				DIAGS(("pCB_run[%i]:(%lx) '%s'", i, (unsigned long)cfg.cnf_run[i], cfg.cnf_run[i]));
				DIAGS(("pCB_run[%i]:(%lx) args '%s'", i, (unsigned long)cfg.cnf_run_arg[i], cfg.cnf_run_arg[i]));
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
static void diags_opts(struct options *o)
{
	DIAGS(("        windowner  = %s", o->windowner ? "true" : "false"));
	DIAGS(("        nohide     = %s", o->nohide    ? "true" : "false"));
	DIAGS(("        naesff     = %s", o->naes_ff   ? "true" : "false"));

	DIAGS(("        xa_nohide  = %s", o->xa_nohide ? "true" : "false"));
	DIAGS(("        xa_nomove  = %s", o->xa_nomove ? "true" : "false"));
	DIAGS(("        noleft     = %s", o->noleft    ? "true" : "false"));
	DIAGS(("        thinwork   = %s", o->thinwork  ? "true" : "false"));
	DIAGS(("        nolive     = %s", o->nolive    ? "true" : "false"));
	DIAGS(("        winframe   = %d", o->thinframe));
}
#else
#define diags_opts(o)
#endif

void
load_config(void *path )
{
	char cpath[FILENAME_MAX];

	struct cnfdata mydata;
	if( !path || !*(char*)path )
	{
		strncpy(cpath, C.Aes->home_path, sizeof(cpath)-sizeof(CNF_NAME)-1);
		strcat(cpath, CNF_NAME);
	}
	else
		strncpy(cpath, path, sizeof(cpath) );

	DIAGS(("Loading config %s", cpath));
	BLOG((0,"Loading config %s", cpath));
	parse_cnf(cpath, parser_tab, &mydata, INF_QUIET);


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


static short screen_r[4] = {0};

#define GROUP_BEG	PI_C__

enum{
	SCR_R,
	FS_WIND_R,
	SYS_WIND_R,
	TM_WIND_R,
	ABOUT_WIND_R,
	VIEW_WIND_R,
	NUL_R
};

static short *inr[] = {
	screen_r,
	&fs_data.fs_x,
	(short*)&systemalerts_r,
	(short*)&taskman_r,
	(short*)&about_r,
	(short*)&view_r
};

/* read rectangle-values into rect indicated by val
   try to adjust to screen and ignore invalid values
*/
static void pVArect(const char *p, const char *line, long val)
{
	const char *cp;
	int i;
	short *r, ri[4];


	if( val < 0 || val > sizeof(inr) / sizeof(short *) )	//>= NUL_R )
	{
		BLOG((1, "pVArect:invalid subcode: %ld", val));
		return;
	}
	r = inr[val];
	for( i = 0, cp = p; i < 4; i++ )
	{
		ri[i] = (short)atol( cp );
		if( i > 1 && ri[i] <= 0 )
			break;

		cp = strchr( cp, ',' );
		if( cp )
			cp++;
		else
			break;
	}
	if( i < 3 )
		return;
	if( val != SCR_R && screen_r[2] && memcmp( &screen.r, screen_r, sizeof( screen_r ) ) )
	{
		for( i = 0; i < 4; i++ )
			if( i & 1 )
				ri[i] = (short)( (long)ri[i] * (long)screen.r.g_h / (long)screen_r[3]);
			else
				ri[i] = (short)( (long)ri[i] * (long)screen.r.g_w / (long)screen_r[2]);
	}
	if( ri[2] > 20 && ri[3] > 20 && ri[0] < screen.r.g_w && ri[1] < screen.r.g_h && ri[2] <= screen.r.g_w && ri[3] <= screen.r.g_h )
		memcpy( r, ri, sizeof(ri) );
}

static struct parser_item inf_tab[] =
{
	{ "### Screen ###",          GROUP_BEG	,   0},
	{ "SCR_XYWH",                PI_V_ATK, pVArect, {dat: SCR_R}    },
	{ "### File-selector ###",   GROUP_BEG	,   0},
	{ "FS_SORTDIR",	             PI_R_S,   & fs_data.SortDir },
	{ "FS_SORT",   	             PI_R_S,   & fs_data.fs_sort},
	{ "FS_TREEVIEW",             PI_R_S,   & fs_data.treeview},
	{ "FS_RTBUILD",	             PI_R_S,   & fs_data.rtbuild},
	{ "FS_POINT",                PI_R_S,   & fs_data.fs_point },
	{ "FS_XYWH",                 PI_V_ATK, pVArect, {dat: FS_WIND_R}    },
	{ "### System-Window ###",   GROUP_BEG	,   0},
	{ "SYS_XYWH",                 PI_V_ATK, pVArect, {dat: SYS_WIND_R}    },
	{ "### Taskmanger ###",      GROUP_BEG	,   0},
	{ "TM_XYWH",                 PI_V_ATK, pVArect, {dat: TM_WIND_R}    },
	{ "### About ###",           GROUP_BEG	,   0},
	{ "ABOUT_XYWH",              PI_V_ATK, pVArect, {dat: ABOUT_WIND_R}    },
	{ "### View ###",            GROUP_BEG	,   0},
	{ "VIEW_XYWH",               PI_V_ATK, pVArect, {dat: VIEW_WIND_R}    },
	{ NULL }
};

#define SYS_RECT( htd_w, sr ) (htd&&htd->htd_w) ? (short*)&htd->htd_w->r :(short*)&sr

static char *inf_fname = "xaaes.inf";
void write_inf(void)
{
	int i, l;
	char buf[256];
	XA_FILE *of;
	struct helpthread_data *htd;

	sprintf( buf, sizeof(buf), "%s\%s", C.start_path, inf_fname );
	of = xa_fopen( buf, O_WRONLY|O_CREAT|O_TRUNC);

	if( !of )
		return;

	fs_save(0);	/* update fs_data */

	l = sprintf( buf, sizeof(buf), "### %s: written by %s ###", inf_fname, aes_version_verbose);
	if( xa_writeline( buf, l, of ) <= 0 )
		goto Ret;

	htd = lookup_xa_data_byname(&C.Hlp->xa_data, HTDNAME);

	for( i = 0; inf_tab[i].key; i++ )
	{
		if( !(inf_tab[i].type & 0x000f) )	/* not a command */
		{
			switch( inf_tab[i].type )
			{
			case GROUP_BEG:
				l = sprintf( buf, sizeof(buf), "%s", inf_tab[i].key );
			break;
			default:
			continue;
			}
		}
		else
		{
			l = sprintf( buf, sizeof(buf), "\t%s=", inf_tab[i].key );
			switch( inf_tab[i].type )
			{
			case PI_R_S:
				l += sprintf( buf+l, sizeof(buf)-l, "%d", *(short*)inf_tab[i].cb);
			break;
			case PI_V_ATK:
			{
				short *r;
				switch( inf_tab[i].dat.dat )
				{
				case SCR_R:
					r = (short*)&screen.r;
				break;
				case FS_WIND_R:
					r = &fs_data.fs_x;
				break;
				case SYS_WIND_R:
					r = SYS_RECT( w_sysalrt, systemalerts_r );
				break;
				case TM_WIND_R:
					r = SYS_RECT( w_taskman, taskman_r);
				break;
				case ABOUT_WIND_R:
					r = SYS_RECT( w_about, about_r);
				break;
				case VIEW_WIND_R:
					//r = (short*)&view_r;
					r = SYS_RECT( w_view, view_r);
				break;
				default:
					BLOG((1, "write_inf:unknown subcode: %ld", inf_tab[i].dat.dat ));
					continue;
				}
				l += sprintf( buf+l, sizeof(buf)-l, "%d,%d,%d,%d", r[0], r[1], r[2], r[3] );
			}
			break;
			default:
			continue;
			}
		}
		if( xa_writeline( buf, l, of ) <= 0 )
			break;
	}
Ret:
	xa_fclose(of);
}

void read_inf(void)
{
	struct cnfdata mydata;
	char buf[256];
	sprintf( buf, sizeof(buf), "%s\%s", C.start_path, inf_fname );
	BLOG((0,"%s:read_inf:%s", get_curproc()->name, buf));
	parse_cnf(buf, inf_tab, &mydata, INF_QUIET);
}
