/*
 * XaAES - XaAES Ain't the AES (c) 1999 - 2003 H.Robbers
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

#include <mint/mintbind.h>

#include "xa_types.h"
#include "xa_global.h"
#include "xa_shel.h"

#include "global.h"
#include "xalloc.h"
#include "config.h"
#include "taskman.h"
#include "xa_fsel.h"
#include "matchpat.h"

#include "ipff.h"


static APP_LIST *local_list;

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

enum
{
	ODEBUG,
	ONA12,
	ONA,
	ONOH,
	ONOL,
	OFRAME,
	OTHINW,
	OLIVE,
	OXNOH,
	OXNOM,
	OXNON,
	OWOWN,
	OHALF,
	OWHEEL,
	OWHLPAGE
};

#if !SEPARATE_SCL
static void
add_slash(char *p)
{
	char *pl = p + strlen(p) - 1;

	if (*pl != '/' && *pl != '\\')
		strcat(p, "\\");
}

static bool
is_wild(int c)
{
	switch (c)
	{
		case '*': break;
		case ']': break;
		case '?': break;
		case '!': break;
		case '[': break;
		default:
			return false;
	}
	return true;
}

static bool
have_wild(char *s)
{
	char *t = s + strlen(s);
	while (--t > s)
	{
		if (*t == '\\' || *t == ':' || *t == '/')
			break;
		if (is_wild(*t))
			return true;
	}
	return false;
}
#endif

static void
set_option(struct options *cur, int which)
{
	APP_LIST *loc = local_list;

	while (loc)
	{
		OPT_LIST *glo = S.app_options;

		while (glo)
		{
			if (stricmp(loc->name, glo->name) == 0)
				break;
			glo = glo->next;
		}

		if (glo)
		{
			struct options *o = &glo->options;

			switch (which)
			{
#if GENERATE_DIAGS
				case ODEBUG:
				{
					int i;
					
					for (i = 0; i < D_max; i++)
					{
						o->point[i] |= cur->point[i];
						 D.point[i] |= cur->point[i];
					}
				}
				break;
#endif
#if NAES12
				case ONA12:
					o->naes12 = cur->naes12;
				break;
#endif
#if NAES3D
				case ONA:
					o->naes = cur->naes;
				break;
#endif
				case ONOH:
					o->nohide = cur->nohide;
				break;
				case ONOL:
					o->noleft = cur->noleft;
				break;
				case OFRAME:
					o->thinframe = cur->thinframe;
				break;
				case OTHINW:
					o->thinwork = cur->thinwork;
				break;
				case OLIVE:
					o->live = cur->live;
				break;
				case OXNOM:
					o->xa_nomove = cur->xa_nomove;	/* fall thru is OK */
				case OXNOH:
					o->xa_nohide = cur->xa_nohide;
				break;
				case OXNON:
					o->xa_none = cur->xa_none;
				case OWOWN:
					o->windowner = cur->windowner;
				break;
				case OHALF:
					o->half_screen = cur->half_screen;
				break;
				case OWHEEL:
					o->wheel_reverse = cur->wheel_reverse;
				break;
				case OWHLPAGE:
					o->wheel_page = cur->wheel_page;
				break;
			}
		}
		loc = loc->next;
	}
}

/*
 * Read & parse the '.scl' files.
 */
#if GENERATE_DIAGS
#define debugp(d,s) curopt->point[d] = s
static char *
truthvalue(bool b)
{
	return b == 0 ? "true" : "false";
}
#endif

/* since xa_scl, SCL needs lock */
void
SCL(LOCK lock, int co, char *name, char *full, char *txt)
{
	bool iftruth = 0, ifexpr = 1;
	int inif = 0;
	Path parms;
	struct options *curopt = &default_options;	/* specify some options for some programs. */
	bool have_brace = false, have_opt = false;
	char *cnf = NULL;
	int fh, t;
	char *translation_table, *lb;
	long tl;
	int lnr = 0;

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
			cnf = Fload(full, &fh, &tl);

		if (cnf == 0)
		{
			display("SCL file '%s' absent\n", name);
			return;
		}
	}
	else
		cnf = txt;

	if (lcfg.booting)
		fdisplay(loghandle, false, "calling ipff_init\n");

	/* initialize */
	ipff_init(IPFF_L-1, sizeof(Path)-1, 0, NULL, cnf, &translation_table);

	/* translate everything to lower case */
	for (t = 'A'; t <= 'Z'; t++)
		translation_table[t] = t - 'A' + 'a';

	if (lcfg.booting)
		fdisplay(loghandle, false, "using translation table\n");

	/*
	 * It is not obligatory to do line based parsing;
	 * ipff_line zeroizes \r & \n,
	 * \r & \n are otherwise treated as white space,
	 * alternative:
	 *      replace lb by cnf,
	 *      dont use ipff_line and loop around anyway you like.
	 *      pe: while(is_keyword(rstr)) or something like that.
	 *      Comment must be enclosed then I suppose. :-)
	 */
	while ((lb = ipff_line(&tl)) != 0)
	{
		int c;
		char rstr[IPFF_L];

		lnr++;

		if (tl == 0)
			continue;

		c = sk();
		/* sk()  skip white space, return nonwhite character
		 * skc() skip 1 character, then sk()
		 * sk1() skip 1 character only return next character
		 */

		/* '#' is a comment line */
		if (c && c != '#')
		{
			ipff_trail(lb);	/* remove trailing spaces */

			if (lcfg.booting)
				fdisplay(loghandle, false, "%s%s\n",
					 iftruth == 0 ? "" : "# ", ipff_getp());

			ide(rstr);		/* get a identifier */
			sk();			/* skip white space */

			if (inif == 0)
			{
				if (strcmp(rstr, "if") == 0)
				{
					sk();
					inif = 1;
					ifexpr = truth();
					iftruth = 1;
					continue;
				}
			}
			else
			{
				if (inif == 1) /* in if but not yet then */
				{
					if (strcmp(rstr, "then") == 0)
					{
						iftruth = ifexpr;
						inif = 2;
						continue;
					}
				}
				else /* in then or else */
				{
					if (   inif == 2 /* in then */
					    && strcmp(rstr, "else") == 0)
					{
						iftruth ^= 1;
						inif = 3;
						continue;
					}
	
					if (strcmp(rstr, "fi") == 0)
					{
						iftruth = 0;
						inif = 0;
						continue;
					}
				}

				if (iftruth == 1)
					continue;
			}

			if (have_opt && !have_brace && c == '{')
			{
				skc();
				have_brace = true;
				continue; /* while lb = ipff_line */
			}
			if (have_brace && c == '}')
			{
				APP_LIST *loc = local_list;
				skc();
				curopt = &default_options;
				have_opt = false;
				have_brace = false;
				
				while (loc)
				{
					APP_LIST *nx = loc->next;
					free(loc);
					loc = nx;
				}
				local_list = NULL;
	
				continue; /* while lb = ipff_line */
			}
		
			if (*rstr == 0)
			{
				err = 1;
				if (lcfg.booting)
					fdisplay(loghandle, true,
						 "line %d: a line must start with a alphanumeric keyword\n", lnr);
			}
			/* Begin of options. */
			else if (!have_opt && strcmp(rstr, "options") == 0)
			{
				have_opt = true;
				fstr(parms, 0, 0);

				/* reorganization of options database.
				 * Make it more work as the syntax suggests.
				 */

				if (strcmp(parms, "default") == 0)
					curopt = &default_options;
				else
					curopt = &local_options,

				local_list = NULL;

				do {
					OPT_LIST *new;
					APP_LIST *loc;

					new = S.app_options;
					while (new)
					{
						if (stricmp(new->name, parms) == 0)
							break;
						new = new->next;
					}

					if (!new)
					{
						new = xcalloc(1, sizeof(*new), 2002);
						if (new == 0)
							break;
	
						new->next = S.app_options;
						S.app_options = new;
						strcpy(new->name, parms);
						new->options = default_options;
					}

					if (new == 0)
						break;

					loc = xcalloc(1, sizeof(*loc), 2003);
					if (loc == 0)
						break;

					loc->next = local_list;
					local_list = loc;
					strcpy(loc->name, parms);

					if (!infix())
						break;
					skc();
					fstr(parms, 0, 0);
				}
				while(1);
			}

#if NAES12
			else if (strcmp(rstr, "naes12") == 0)
			{
				curopt->naes12 = true,
				set_option(curopt, ONA12);
			}
#endif
#if NAES3D
			else if (strcmp(rstr, "naes3d") == 0)
			{
				curopt->naes = true,
				set_option(curopt, ONA);
			}
#endif
			else if (strcmp(rstr, "open") == 0)
			{
				if (co == 1)
				{
					ide(rstr);
					if (strncmp(rstr, "task", 4) == 0)
						open_taskmanager(lock, false);
				}
				else if (co == 2)
				{
					ide(rstr);
					if (strncmp(rstr, "task",4) == 0)
						cfg.opentaskman = true;
				}
			}
			else if (strcmp(rstr, "windows") == 0)
			{
				do {
					ide(rstr);
					if (strcmp(rstr, "nohide") == 0)
					{
						curopt->nohide = true;
						set_option(curopt, ONOH);
					}
					else if (strcmp(rstr, "noleft") == 0)
					{
						curopt->noleft = true;
						set_option(curopt, ONOL);
					}
					else if (strcmp(rstr, "thinframe") == 0)
					{
						curopt->thinframe = -1;
						set_option(curopt, OFRAME);
					}
					else if (strcmp(rstr, "thickframe") == 0)
					{
						curopt->thinframe = 1;
						set_option(curopt, OFRAME);
					}
					else if (strcmp(rstr, "frame_size") == 0)
					{
						sk();
						curopt->thinframe = idec();
						set_option(curopt, OFRAME);
					}
					else if (strcmp(rstr, "thinwork") == 0)
					{
						curopt->thinwork = true;
						set_option(curopt, OTHINW);
					}
					else if (strcmp(rstr, "live") == 0)
					{
						curopt->live = true;
						set_option(curopt, OLIVE);
					}

					if (!infix())
						break;
					
					skc();
				}
				while (1);
			}
			else if (strcmp(rstr, "wheel") == 0)
			{
				do {
					ide(rstr);
					if (strcmp(rstr, "content") == 0)
					{
						curopt->wheel_reverse = 1;
						set_option(curopt, OWHEEL);
					}
					else if (strcmp(rstr, "pagefactor") == 0)
					{
						if (sk() == '=')
							skc();
						curopt->wheel_page = idec();
						set_option(curopt, OWHLPAGE);
					}
					if (!infix())
						break;
					skc();
				}
				while (1);
			}
			else if (strcmp(rstr, "xa_windows") == 0)
			{
				do {
					ide(rstr);
					if (strcmp(rstr, "none") == 0)
					{
						curopt->xa_none = true;
						set_option(curopt, OXNON);
					}
					else if (strcmp(rstr, "nohide") == 0)
					{
						curopt->xa_nohide = true;
						set_option(curopt, OXNOH);
					}
					else if (strcmp(rstr, "nomove") == 0)
					{
						curopt->xa_nomove = true;
						curopt->xa_nohide = true;
						set_option(curopt, OXNOM);
					}
					else if (strcmp(rstr, "noleft") == 0)
					{
						curopt->noleft = true;
						set_option(curopt, ONOL);
					}
					if (!infix())
						break;
					skc();
				}
				while (1);
			}
			else if (   strcmp(rstr, "windowner") == 0
				 || strcmp(rstr, "winowner" ) == 0)
			{
				ide(rstr);
				if (   strcmp(rstr, "true" ) == 0
				    || strcmp(rstr, "short") == 0)
				{
					curopt->windowner = 1;
					set_option(curopt, OWOWN);
				}
				else if (strcmp(rstr, "long") == 0)
				{
					curopt->windowner = 2;
					set_option(curopt, OWOWN);
				}
				/* default is zero anyway. */
			}
			else if (strcmp(rstr, "menu") == 0)
			{
				do {
					ide(rstr);
					if (strcmp(rstr, "pull") == 0)
						cfg.menu_behave = PULL;
					else if (strcmp(rstr, "push") == 0)
						cfg.menu_behave = PUSH;
					else if (strcmp(rstr, "leave") == 0)
						cfg.menu_behave = LEAVE;
					else if (strcmp(rstr, "nolocking") == 0)
						cfg.menu_locking = false;
					if (!infix())
						break;
					skc();
				}
				while(1);
			}
			else if (strcmp(rstr, "popscroll") == 0)
				cfg.popscroll = idec();
			else if (strcmp(rstr, "priority") == 0)
			{
				int prio = idec();
				if (prio >= -20 && prio <= 20)
					Psetpriority(0, C.AESpid, prio);
			}
			else if (strcmp(rstr, "usehome") == 0)
				cfg.usehome = true;
			else if (strcmp(rstr, "font_id") == 0)
				cfg.font_id = idec();
			else if (strcmp(rstr, "standard_point") == 0)
				cfg.standard_font_point = idec();
			else if (strcmp(rstr, "medium_point") == 0)
				cfg.medium_font_point = idec();
			else if (strcmp(rstr, "small_point") == 0)
				cfg.small_font_point = idec();

			/* End of options. */
			else if (strcmp(rstr, "widgets") == 0)
				fstr(lcfg.widg_name, 0, 0);
			else if (strcmp(rstr, "resource") == 0)
				fstr(lcfg.rsc_name, 0, 0);
			else if (strcmp(rstr, "run") == 0)
			{
				if (co)
				{
					Path path;
		
					fstr(path, 0, 0);
#if !SEPARATE_SCL
					if (!have_wild(path))
#endif
					{
						sk();
						parms[0] = sdisplay(parms+1,"%s",ipff_getp());
						launch(lock, 0, 0, 0, path, parms, C.Aes);
					}
#if !SEPARATE_SCL
					else
					{
						Path nm, pattern;
						char *pat, *pbt, fslash[4];
						long i,rep;				
						
						pattern[0] = '*';
						pattern[1] = '\0';
						pat = strrchr(path, '\\');
						pbt = strrchr(path, '/');
						if (!pat) pat = pbt;
						if (pat)
						{
							strcpy(pattern, pat + 1);
							*(pat + 1) = 0;
							if (strcmp(pattern, "*.*") == 0)
								*(pattern + 1) = 0;
						}
					
						add_slash(path);
					
						i = Dopendir(path, 0);
						if (i > 0)
						{
							struct xattr xat;
	
							while (Dxreaddir(NAME_MAX,i,nm,(long)&xat, &rep) == 0)
							{
								char fulln[NAME_MAX+2];
								char *nam = nm+4;
								bool dir = ((xat.mode & 0xf000) == S_IFDIR);
								bool sln = ((xat.mode & 0xf000) == S_IFLNK);
								bool match = false;
					
								strcpy(fulln,path);
								strcat(fulln,nam);
								if (sln) 
								{
									Fxattr(0, fulln, &xat);
									dir = ((xat.mode & 0xf000) == S_IFDIR);
								}
								if (!dir)
								{
									match = match_pattern(nam, pattern);
									if (match)
									{
										parms[0] = 0;
										if (lcfg.booting)
											fdisplay(loghandle, false, "  --  '%s'\n", fulln);
										launch(lock, 0, 0, 0, fulln, parms, C.Aes);
									}
								}
							}
							Dclosedir(i);
						}
					}
#endif
				}
			}
			else if (   strcmp(rstr, "shell") == 0
				 || strcmp(rstr, "desk" ) == 0)
			{
				if (co)
				{
					Path p;
		
					fstr(p, 0, 0);
					sk();
					parms[0] = sdisplay(parms+1, "%s", ipff_getp());
					C.DSKpid = launch(lock, 0, 0, 0, p, parms, C.Aes);
					if (C.DSKpid > 0)
						strcpy(C.desk, p);
				}
			}
			else if (strcmp(rstr, "launcher") == 0)
				fstr(cfg.launch_path, 0, 0);
			else if (strcmp(rstr, "clipboard") == 0)
				fstr(cfg.scrap_path, 0, 0);
			else if (strcmp(rstr, "accpath") == 0)
				fstr(cfg.acc_path, 0, 0);
			else if (strcmp(rstr, "dc_time") == 0)
				lcfg.double_click_time = idec();
#if GENERATE_DIAGS
			else if (strcmp(rstr, "debug_lines") == 0)
				D.debug_lines = dec();
			else if (strcmp(rstr, "debug") == 0)
			{
				if (isdigit(sk()))
				{
					D.debug_level = idec();
				}
				else
				{
					char *p = ipff_getp();

					ide(rstr);

					if (strcmp(rstr,"off") == 0)
					{
						D.debug_level = 0;
					}
					else if (strcmp(rstr,"on") == 0)
					{
						/* This means only the DIAGS macro's (the old ones) */
						D.debug_level = 1;
					}
					else if (strcmp(rstr,"all") == 0)
					{
						int i;

						for (i = 0; i < D_max; i++)
							curopt->point[i] = 1;
						if (D.debug_level < 2)
							D.debug_level = 2;
					}
					else if (!D.debug_file)
					{
						ipff_putp(p);
						fstr(D.debug_path, 0, 0);
						if (strnicmp(D.debug_path, "screen", 6) != 0)
						{
							if (lcfg.booting)
								fdisplay(loghandle, true, "Debug to %s\n", D.debug_path);
					
						#if 1		/* HR: Want to see session date / time */
							D.debug_file = (int)Fcreate(D.debug_path, 0);
						#else
							D.debug_file = (int)Fopen(D.debug_path, O_CREAT|O_WRONLY);
						#endif
							Fforce(1, D.debug_file);
						}
					}
				}

				set_option(curopt, ODEBUG);
				if (curopt == &default_options)
					memcpy(D.point, curopt->point, sizeof(D.point));
			}
			/* If specified together with debug all, the meaning is reversed. */
			/* HR 081102: Increased intuitivity of debugpoint all except|but x,x,x,x,x,x */
			else if (strcmp(rstr, "debugpoint") == 0)
			{
				int state = 1;

				if (D.debug_level < 2)
					D.debug_level = 2;
				ide (rstr);
				if (strcmp (rstr,"all"    ) == 0)
				{
					int i;
					state = 1;
					for (i = 0; i < D_max; i++)
						curopt->point[i] = 1;
					sk();
					ide(rstr);
					if (   strcmp (rstr, "except" ) == 0
					    || strcmp (rstr, "but"    ) == 0)
					{
						state ^= 1;
						sk();
						ide(rstr);
					}
				}

				do {
					if (strlen(rstr) == 1 && isalpha(*rstr))
						debugp(D_a + (tolower(*rstr) - 'a'),state);
					else if (strncmp(rstr, "appl", 4) == 0 ) debugp(D_appl, state);
					else if (strcmp (rstr, "evnt"   ) == 0 ) debugp(D_evnt, state);
					else if (strncmp(rstr, "mult", 4) == 0 ) debugp(D_multi, state);
					else if (strncmp(rstr, "form", 4) == 0 ) debugp(D_form, state);
					else if (strncmp(rstr, "fsel", 4) == 0 ) debugp(D_fsel, state);
					else if (strncmp(rstr, "graf", 4) == 0 ) debugp(D_graf, state);
					else if (strncmp(rstr, "menu", 4) == 0 ) debugp(D_menu, state);
					else if (strcmp (rstr, "objc"   ) == 0 ) debugp(D_objc, state);
					else if (strcmp (rstr, "rsrc"   ) == 0 ) debugp(D_rsrc, state);
					else if (strcmp (rstr, "scrp"   ) == 0 ) debugp(D_scrp, state);
					else if (strncmp(rstr, "shel", 4) == 0 ) debugp(D_shel, state);
					else if (strncmp(rstr, "wind", 4) == 0 ) debugp(D_wind, state);
					else if (strncmp(rstr, "widg", 4) == 0 ) debugp(D_widg, state);
					else if (strncmp(rstr, "mous", 4) == 0 ) debugp(D_mouse,state);
					else if (strncmp(rstr, "butt", 4) == 0 ) debugp(D_button, state);
					else if (strncmp(rstr, "keyb", 4) == 0 ) debugp(D_keybd, state);
					else if (strncmp(rstr, "sema", 4) == 0 ) debugp(D_sema, state);
					else if (strncmp(rstr, "rect", 4) == 0 ) debugp(D_rect, state);
					else if (strncmp(rstr, "pipe", 4) == 0 ) debugp(D_pipe, state);
					else if (strncmp(rstr, "trap", 4) == 0 ) debugp(D_trap, state);
					else if (strncmp(rstr, "kern", 4) == 0 ) debugp(D_kern, state);
#if WDIAL
					else if (strcmp (rstr, "wdlg"   ) == 0 ) debugp(D_wdlg, state);
#endif
#if LBOX
					else if (strncmp(rstr, "lbox", 4) == 0 ) debugp(D_lbox, state);
#endif
					else if (strcmp (rstr, "this"   ) == 0 ) debugp(D_this, state);

					if (!infix())
						break;

					skc();
					ide(rstr);
				}
				while (1);

				set_option(curopt, ODEBUG);
				if (curopt == &default_options)
					memcpy(D.point, curopt->point, sizeof(D.point));
			}
#else
			else if (   strcmp(rstr, "debug") == 0
				 || strcmp(rstr, "debugpoint") == 0
				 || strcmp(rstr, "debug_lines") == 0)
			{
				/* Do nothing with this line */
			}
#endif
#if USE_CALL_DIRECT
			else if (strcmp(rstr, "direct") == 0)
			{
				ide(rstr);
				if (strcmp(rstr, "on") == 0)
					cfg.direct_call = true;
				else
					cfg.direct_call = false;
			}
#endif
			else if (strncmp(rstr, "half_scr",8) == 0)
			{
				curopt->half_screen = dec(); 
				set_option(curopt, OHALF);
			}
			else if (strcmp(rstr, "cancel") == 0)
			{
				t = 0;
				do {
					ide(rstr);
					if (strlen(rstr) >= NUM_CB)
						rstr[CB_L - 1] = 0;
					strcpy(cfg.cancel_buttons[t++],rstr);
					if (infix() && t < NUM_CB - 1) skc();	/* last entry kept clear as a stopper */
					else break;
				}
				while (1);
			}
			else if (strcmp(rstr, "toppage") == 0)
			{
				ide(rstr);
				if (strcmp(rstr, "bold") == 0)
				{
					cfg.topname = BOLD;
					cfg.backname = 0;
				}
				else if (strcmp(rstr, "faint") == 0)
				{
					cfg.topname = 0;
					cfg.backname = FAINT;
				}
			}
			else if (strcmp(rstr, "superprogdef") == 0)
			{
				cfg.superprogdef = true;
			}
			else if (   strcmp(rstr, "string"  ) == 0
				 || strcmp(rstr, "int"     ) == 0
				 || strcmp(rstr, "integral") == 0
				 || strcmp(rstr, "integer" ) == 0
				 || strcmp(rstr, "bool"    ) == 0
				 || strcmp(rstr, "boolean" ) == 0) /* integer, integral, boolean */
			{
				Path ident, p;
				ide(ident);
				if (*ident)
				{
					int atok = 0; /* 0 'is' constant, 1 ':=' variable */
					if (co == 1)
					{
						atok = assign();
						sk();
						if (atok < 0)
						{
							err = display("needs ':=' or 'is'\n");
							atok = 0;
						}
					}
					else
					{
						if (sk() == '=')
							skc();
					}
					if (*rstr == 'i') /* int */
						new_sym(ident, atok, Number, NULL, dec());
					else if (*rstr == 'b')
						new_sym(ident, atok, Bit, NULL, truth());
					else
					{
						fstr(p, 0, 0);
						new_sym(ident, atok, String, p, 0);
					}
				}
			}
			else if (strcmp (rstr, "setenv") == 0 || strcmp (rstr, "export") == 0)
			{
				Path ident;
				char p[512], *isp, *q;
				SYMBOL *s;

				q = ipff_getp();

				/* just replace the first occurrence
				 * of = in the source by space.
				 */
				isp = strstr(q, "=");
				if (isp)
					*isp = ' ';

				s = fstr(ident, 0, Lval);

				if (*ident)
				{
					int c = sk();

					strcpy(p,ident);
					strupr(p);
					strcat(p,"=");

					if (s && (c == 0 || c != '=' || c == ';'))
					{
						/* pattern 'export var' */
						if (s->mode == String)
							strcat(p, s->s);
						else
							sdisplay(p + strlen(p), "%ld", s->val);
					}
					else
					{
						if (c == '=')
							skc();
						fstr(p + strlen(p), 0, 0);
					}
					put_env(lock, 1, 0, p);
				}
			}
#if POINT_TO_TYPE
			else if (strcmp(rstr, "focus") == 0)
			{
				ide(rstr);
				cfg.point_to_type = (strcmp(rstr, "point") == 0);
			}
#endif
#if FILESELECTOR
			else if (strcmp(rstr, "filters") == 0)
			{
				int i = 0;
				while (i < 23)
				{
					fstr(parms, 0, 0);	/* get string */
					parms[15] = 0;
					strcpy(cfg.Filters[i++],parms);
					if (!infix())
						break;
					skc();
				}
			}
#endif
			else
			{
				err = 1;
				if (!co)
					display("line %d: unimplemented keyword '%s'\n", lnr, rstr);
			}
		}
	}

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

	if (txt == 0)
		free(cnf);
}
