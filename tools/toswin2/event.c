/*
 * event.c
 *
 * Eventverarbeitung fr TosWin
 *
 */

#include <string.h>
#include <gem.h>

#include "version.h"
#include "av.h"
#include "textwin.h"
#include "event.h"
#include "config.h"
#include "proc.h"
#include "drag.h"
#include "share.h"
#include "console.h"
#include "toswin2.h"

#ifdef DEBUG
extern int do_debug;
# include <syslog.h>
#endif

bool gl_done = FALSE;

/*
 * lokale Variablen
*/
static OBJECT	*menu;
static WDIALOG	*about;

int draw_ticks = MAX_DRAW_TICKS;
int curs_ticks = 0;

/******************************************************************************/
#include <stdio.h>

static void about_open(WDIALOG *dial)
{
	extern char __Ident_gnulib[];
	extern char __Ident_gem[];
	extern char __Ident_cflib[];
	char pl[32];

	set_string (dial->tree, AVERSION, TWVERSION);
	set_string (dial->tree, ADATUM, __DATE__);
#if defined(__GNUC__)
	set_string (dial->tree, ACOMP, "GNU C");
#elif defined(__PUREC__)
	set_string (dial->tree, ACOMP, "PureC");
#endif

	get_patchlev (__Ident_gnulib, pl);
	set_string (dial->tree, AMINT, pl);

	get_patchlev (__Ident_gem, pl);
	set_string (dial->tree, AGEM, pl);

	get_patchlev (__Ident_cflib, pl);
	set_string (dial->tree, ACF, pl);
	
	wdial_open (dial);
}

static int about_close(WDIALOG *dial, short obj)
{
	wdial_close(dial);

	return TRUE;
}

/******************************************************************************/
static void update_menu(void)
{
	/* Fenster */
	menu_ienable(menu, MCYCLE, ((gl_winanz >= 2) || gl_avcycle));
	menu_ienable(menu, MCLOSE, ((gl_topwin != NULL) 
										&& !(gl_topwin->flags & WICONIFIED)
										&& !(gl_topwin->flags & WSHADED)));

	/* Optionen */
	menu_icheck(menu, MALLOGIN, gl_allogin);
	menu_icheck(menu, MAVCYCLE, gl_avcycle);
	menu_icheck(menu, MSHORTCUT, gl_shortcut);
}

/******************************************************************************/

void menu_help(int title, int item)
{
	char *p, str[50], s[50];
	
	get_string(menu, item, s);
	/* die fhrenden '  ' berspringen und das letzte auch nicht */
	strncpy(str, s + 2, strlen(s) - 3);
	
	p = str;
	while (TRUE)
	{
		/* Meneintrag endet entweder bei min. zwei Blanks oder Punkten */
		if ((*p == ' ' && *(p+1) == ' ') ||
		    (*p == '.' && *(p+1) == '.'))
			break;
		p++;
	}
	*p = '\0';
	call_stguide(str, TRUE);
}

static void handle_menu(int title, int item, bool help)
{
	bool	old, new;
	
	if (!help)
	{
		switch (item)
		{
/* Desk */
			case MABOUT :
				open_wdial(about, -1, -1);
				break;
				
/* Datei */
			case MPROG :
				start_prog();
				break;
	
			case MSHELL :
				new_shell();
				break;
				
			case MENDE :
				gl_done = TRUE;
				break;

/* Fenster */			
			case MCONSOLE :
				open_console();
				break;
	
			case MCYCLE :
				if ((gl_avcycle) && (send_avkey(4, 0x1117)))	/* ^W */
					break;
				cycle_window();
				break;
	
			case MCLOSE :
				(*gl_topwin->closed)(gl_topwin);
				break;

/* Optionen */	
			case MAVCYCLE :
				old = gl_avcycle;
				new = !gl_avcycle;
				if (old && !new)		/* war an, nun aus */
				{
					update_avwin(new);
					gl_avcycle = new;
				}
				if (new && !old)		/* nun an, war aus */
				{
					gl_avcycle = new;
					update_avwin(new);
				}
				break;

			case MSHORTCUT :
				gl_shortcut = !gl_shortcut;
				break;

			case MALLOGIN :
				gl_allogin = !gl_allogin;
				break;

			case MCCONFIG :
				conconfig_open();
				break;
	
			case MWCONFIG :
				winconfig_open();
				break;
	
			case MSAVE :
				config_save();
				break;
	
			default:
				break;
		}
	}
	else
		menu_help(title, item);
	menu_tnormal(menu, title, 1);
}

/******************************************************************************/

static void handle_msg(short *msgbuff)
{
	TEXTWIN	*t;
	short msx, msy, d, kstate;

	graf_mkstate(&msx, &msy, &d, &kstate);
	if (!message_wdial(msgbuff))
	{
		if (!window_msg(msgbuff))
		{
			switch (msgbuff[0]) 
			{
				case TWSTART:
					handle_share(msgbuff[1]);
					break;
				case TWTOP :
					t = get_textwin(0, msgbuff[1]);
					if (t)
						(*t->win->topped)(t->win);
					break;
				case TWSHELL :
					new_shell();
					break;

				case MN_SELECTED:
					handle_menu(msgbuff[3], msgbuff[4], (kstate == K_CTRL));
					break;

				case AP_DRAGDROP:
					handle_dragdrop(msgbuff);
					break;
				case AP_TERM:
					gl_done = TRUE;
					break;

				case VA_START :
				case VA_PROTOSTATUS :
				case VA_DRAGACCWIND :
					handle_av(msgbuff);
					break;

				case AV_SENDKEY :
					if ((msgbuff[3] == 4) && (msgbuff[4] == 0x1117))	/* ^W */
						cycle_window();
					break;
			

				case 0x7A18 :	/* FONT_CHANGE */
					{
						WINDOW	*winp;
						
						winp = find_window(msx, msy);
						update_font(winp, msgbuff[4], msgbuff[5]);
					}
					break;

				case 72 :		/* von Freedom */
					break;
					
				default:
					if (gl_debug)
					{
						char str[12];
						short i, id;
			
						i = appl_search(0, str, &d, &id);
						while (i != 0)
						{
							if (id == msgbuff[1])
								break;
							i = appl_search( 1, str, &d, &id);
						}
						debug("Unbekannte Msg %d (0x%X) von %d %s\n", msgbuff[0], msgbuff[0], id, str);
					}
					break;
			}
		}
		update_menu();
	}
}

/******************************************************************************/
/*
 * After this many bytes have been written to a window, it's time to
 * update it. Note that we must also keep a timer and update all windows
 * when the timer expires, or else small writes will never be shown!
 */
#define THRESHOLD 400

short needs_redraw(TEXTWIN *t)
{
	int limit = NROWS(t) - 1;

	return ((draw_ticks > MAX_DRAW_TICKS &&
		t->nbytes >= THRESHOLD)
		|| (0 && t->scrolled >= limit));
}

/******************************************************************************/
void event_loop(void)
{
	short evset, event, msx, msy, mbutton, kstate, mbreturn, kreturn;
	short msgbuff[8], item, title;

	evset = (MU_MESAG | MU_BUTTON | MU_KEYBD);
	do
	{
		if (gl_con_log || gl_winanz > 0)
			evset |= MU_TIMER;
		else
			evset &= ~MU_TIMER;
			
		event = evnt_multi(evset,
				0x0101, 3, 0,
				0, 0, 0, 0, 0,
				0, 0, 0, 0, 0,
				msgbuff,
				50,
				&msx, &msy, &mbutton, &kstate,
				&kreturn, &mbreturn);

		if (event & MU_MESAG) 
			handle_msg(msgbuff);
		
		if (event & MU_KEYBD) 
		{
			if (!key_wdial(kreturn, kstate))
			{
				if (gl_shortcut && is_menu_key(kreturn, kstate, &title, &item))
					handle_menu(title, item, FALSE);
				else
#ifdef DEBUG
				{
					if (do_debug)
						syslog (LOG_ERR, "Got key 0x%04x | 0x%04x", 
							(unsigned) kreturn, (unsigned) kstate);
#endif
					window_key(kreturn, kstate);
#ifdef DEBUG
				}
#endif
			}
		}

		if (event & MU_BUTTON) 
			if (!click_wdial(mbreturn, msx, msy, kstate, mbutton))
				window_click(mbreturn, msx, msy, kstate, mbutton);

		if (event & MU_TIMER) {
			fd_input();
			if (curs_ticks % 6 == 0)
				window_timer ();
		}
		
		++draw_ticks;
		++curs_ticks;
		if (draw_ticks > MAX_DRAW_TICKS)
			draw_ticks = 0;
	}
	while (!gl_done);
	delete_menu();
	delete_wdial(about);
}

/******************************************************************************/

void event_init(void)
{
	OBJECT	*tmp;
	
	rsrc_gaddr(R_TREE, MENUTREE, &menu);
	create_menu(menu);

	rsrc_gaddr(R_TREE, ABOUT, &tmp);
	fix_dial(tmp);
	about = create_wdial(tmp, winicon, 0, about_open, about_close);
	
	if (!file_exists("u:\\etc\\passwd"))
	{
		menu_ienable(menu, MSHELL, FALSE);
		menu_ienable(menu, MALLOGIN, FALSE);
	}
	
	if (!file_exists(XCONNAME))
	{
		menu_ienable(menu, MCONSOLE, FALSE);
		menu_ienable(menu, MCCONFIG, FALSE);
		gl_con_auto = FALSE;
	}

	/* for debugging under MagiC :-) */
	if (gl_debug && gl_magx)
		menu_ienable(menu, MCCONFIG, TRUE);
	
	/* fr Alerts und Fontauswahl im Fenster */
	set_mdial_wincb(handle_msg);
	
	update_menu();
}
