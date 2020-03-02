/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * - 20.9.96:
 *   Stack fuer Kontext auf 16384 Bytes vergroessert, damit es bei
 *   Super()-Aufrufen auf dem (Power) Mac keine Probleme mehr gibt.
 * - 11.02.97:
 *   Test ob Relokationsinformationen vorhanden sind, um Probleme mit
 *   Modula-Kompilaten zu vermeiden.
 * - 28.12.97 (COPS 1.08):
 *   Wenn beim cpx_init() innerhalb von open_cpx_context() eine 1 zurueckgeliefert wird,
 *   interpretiert das COPS als Anweisung kein Fenster zu oeffnen (fuer NPRNCONF)
 */

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <time.h>
#include <mint/sysvars.h>
#undef etv_critic
#include "include/country.h"

#include "cops_rsc.h"
#include "adaptrsc.h"
#include "callback.h"
#include "popup.h"
#include "cops.h"
#include "cpx_bind.h"
#include "key_map.h"
#include "list.h"
#include "objmacro.h"
#include "phstuff.h"
#include "vaproto.h"
#include "wlib.h"

/* XXX from mintlib */
extern short _app;

#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif

/* magic mt_aes -> gemlib wrapper */
#ifndef __EVNTDATA
#define __EVNTDATA
typedef struct {
	_WORD x;
	_WORD y;
	_WORD bstate;
	_WORD kstate;
} EVNTDATA;
#endif
#define graf_mkstate_evntdata(evntdata) \
	graf_mkstate(&(evntdata)->x,&(evntdata)->y,&(evntdata)->bstate,&(evntdata)->kstate)

/* default window style */
#define MAINWINDOWSTYLE	\
		NAME|CLOSER|FULLER|MOVER|SIZER|VSLIDE|HSLIDE|UPARROW|DNARROW|LFARROW|RTARROW|SMALLER

/*
 * some compilers define their own constants for these
 * (e.g PureC, Lattice-C),
 * but we need the MiNT definitions
 */
#undef SIGTERM
#undef SIGQUIT
#define SIGTERM		15		/* software termination signal */
#define SIGQUIT		3		/* quit signal */

#define DEBUG_TRANSLATIONS 0

/*----------------------------------------------------------------------------------------*/

/* Fileversion 7 */
struct old_alphaheader
{
	long	magic;
	short	version;
	GRECT	mw;
	short	whslide;
	short	wvslide;
	short	booticon;	/* Hauptfenster beim Start oeffnen und ikonifizieren */
	short	clickact;	/* inaktive CPXe werden durch Doppelklick umbenannt und geoeffnet */
	short	term;
	short	after;
	short	count;
	char	cpx_path[128];
};

typedef struct
{
	long	id;
	short	icon_x;		/* Position des Icons */
	short	icon_y;
	short	window_x;	/* Position des CPX-Fenster */
	short	window_y;
	short	autostart;
} ALPHACPX;

struct auto_start
{
	struct auto_start *next;
	CPX_DESC *cpx_desc;
};

/*----------------------------------------------------------------------------------------*/

_WORD app_id;
_WORD vdi_handle;
_WORD aes_handle;
unsigned long aes_flags;
_WORD aes_font;
_WORD aes_height;
_WORD pwchar;
_WORD phchar;
_WORD pwbox;
_WORD phbox;

_WORD menu_id;
_WORD quit;

GRECT desk_grect;
WINDOW *main_window = NULL;

static char *help_buf;
static char home[128];
static char inf_name[128];

struct alphaheader settings;

struct cpxlist *cpxlist = NULL;
CPX_DESC *cpx_desc_list = NULL;

static CPX_DESC *g_open_cpx = NULL;
static struct auto_start *auto_start_list = NULL;

/*----------------------------------------------------------------------------------------*/

static _KEYTAB *kt = NULL;

static short no_open_cpx;
static short do_reload;
static short do_close;
static short must_read_inf;
static time_t termtime;
static _WORD windowtitle_str = WINDOWTITLE_STR_EN;
static _WORD menutitle_str = MENUTITLE_STR_EN;
static _WORD cops_popup = COPS_POPUP_ABOUT_EN;
static _WORD cpx_popup = CPX_POPUP_OPEN_EN;
static _WORD cpxpath_str = CPXPATH_STR_EN;
static _WORD noaes_str = NOAES_STR_EN;
static _WORD nowindow_alert = NOWINDOW_ALERT_EN;
_WORD save_dflt_alert = SAVE_DFLT_ALERT_EN;
_WORD mem_err_alert = MEM_ERR_ALERT_EN;
_WORD file_err_alert = FILE_ERR_ALERT_EN;
_WORD fnf_err_alert = FNF_ERR_ALERT_EN;
_WORD reload_alert = RELOAD_ALERT_EN;
_WORD al_save_header = AL_SAVE_HEADER_EN;
_WORD al_no_sound_dma = AL_NO_SOUND_DMA_EN;
static _WORD cpxload_alert = CPXLOAD_ALERT_EN;
static _WORD quit_alert = QUIT_ALERT_EN;

/* internal functions */

static short cpx_in_rect(CPX_DESC *cpx_desc, GRECT *rect);
static void cpx_to_end(CPX_DESC *cpx_desc);
static void get_cpx_bbox(GRECT *bbox);
static void full_redraw_main_window(void);
static void redraw_main_window(WINDOW *w, GRECT *area);
static short activate_cpx(CPX_DESC *cpx_desc);
static void update_cpx_path(void);
static short do_dialog(OBJECT *tree);
static short handle_message(_WORD msg[8]);
static void do_args(char *args);
static void set_termtime(void);
static short MapKey(short keystate, short key);
static short handle_evnt_cpx(CPX_DESC *cpx_desc, EVNT *events);
static void _cdecl sig_handler(long sig);

/*----------------------------------------------------------------------------------------*/

static _WORD pop_handle(_WORD mx, _WORD my, char **items, _WORD num_items, _WORD default_item, unsigned short disabled)
{
	GRECT gr;
	char popup_strs[12][30];
	char *popup_items[12];
	_WORD i;

	for (i = 0; i < num_items; i++)
	{
		popup_items[i] = popup_strs[i];
		if (disabled & 1)
		{
			strcpy(popup_items[i], "- ");
			strcat(popup_items[i], items[i]);
			strcat(popup_items[i], " ");
		} else if (items[i][0] == '-')
		{
			strcpy(popup_items[i], "-");
		} else
		{
			strcpy(popup_items[i], "  ");
			strcat(popup_items[i], items[i]);
			strcat(popup_items[i], " ");
		}
		disabled >>= 1;
	}
	gr.g_x = mx;
	gr.g_y = my;
	gr.g_w = 0;
	gr.g_h = 0;
	return do_popup(&gr, popup_items, num_items, 0, default_item);
}

/*----------------------------------------------------------------------------------------*/

/*
 * Set translatable text fields in resource
 */
static void set_texts(_WORD country)
{
	const _WORD *p;
	OBJECT *tree;
	static _WORD const trans_table[3][36] = {
		{
			/* english */
			WINDOWTITLE_STR_EN,
			MENUTITLE_STR_EN,
			CPXPATH_STR_EN,
			NOAES_STR_EN,
			NOWINDOW_ALERT_EN,
			SAVE_DFLT_ALERT_EN,
			MEM_ERR_ALERT_EN,
			FILE_ERR_ALERT_EN,
			FNF_ERR_ALERT_EN,
			RELOAD_ALERT_EN,
			AL_SAVE_HEADER_EN,
			AL_NO_SOUND_DMA_EN,
			CPXLOAD_ALERT_EN,
			QUIT_ALERT_EN,
			COPS_POPUP_ABOUT_EN,
			CPX_POPUP_OPEN_EN,
			ABOUT_BY_EN,
			OK_EN,
			INFO_FILENAME_EN,
			INFO_VERSION_EN,
			INFO_ID_EN,
			INFO_RAM_EN,
			INFO_SETONLY_EN,
			INFO_BOOTINIT_EN,
			INFO_AUTOBOOT_EN,
			OK_EN,
			CANCEL_EN,
			SET_SETTINGS_EN,
			SET_SELPATH_EN,
			SET_ICONIFY_EN,
			SET_DCLICK_EN,
			SET_SORTNAME_EN,
			SET_TERM_EN,
			SET_TERMAFTER_EN,
			OK_EN,
			CANCEL_EN,
		},
		{
			/* german */
			WINDOWTITLE_STR_DE,
			MENUTITLE_STR_DE,
			CPXPATH_STR_DE,
			NOAES_STR_DE,
			NOWINDOW_ALERT_DE,
			SAVE_DFLT_ALERT_DE,
			MEM_ERR_ALERT_DE,
			FILE_ERR_ALERT_DE,
			FNF_ERR_ALERT_DE,
			RELOAD_ALERT_DE,
			AL_SAVE_HEADER_DE,
			AL_NO_SOUND_DMA_DE,
			CPXLOAD_ALERT_DE,
			QUIT_ALERT_DE,
			COPS_POPUP_ABOUT_DE,
			CPX_POPUP_OPEN_DE,
			ABOUT_BY_DE,
			OK_EN, /* no translation needed */
			INFO_FILENAME_DE,
			INFO_VERSION_DE,
			INFO_ID_DE,
			INFO_RAM_DE,
			INFO_SETONLY_DE,
			INFO_BOOTINIT_DE,
			INFO_AUTOBOOT_DE,
			OK_EN, /* no translation needed */
			CANCEL_DE,
			SET_SETTINGS_DE,
			SET_SELPATH_DE,
			SET_ICONIFY_DE,
			SET_DCLICK_DE,
			SET_SORTNAME_DE,
			SET_TERM_DE,
			SET_TERMAFTER_DE,
			OK_EN, /* no translation needed */
			CANCEL_DE,
		},
		{
			/* french */
			WINDOWTITLE_STR_FR,
			MENUTITLE_STR_FR,
			CPXPATH_STR_FR,
			NOAES_STR_FR,
			NOWINDOW_ALERT_FR,
			SAVE_DFLT_ALERT_FR,
			MEM_ERR_ALERT_FR,
			FILE_ERR_ALERT_FR,
			FNF_ERR_ALERT_FR,
			RELOAD_ALERT_FR,
			AL_SAVE_HEADER_FR,
			AL_NO_SOUND_DMA_FR,
			CPXLOAD_ALERT_FR,
			QUIT_ALERT_FR,
			COPS_POPUP_ABOUT_FR,
			CPX_POPUP_OPEN_FR,
			ABOUT_BY_FR,
			OK_EN, /* no translation needed */
			INFO_FILENAME_FR,
			INFO_VERSION_FR,
			INFO_ID_FR,
			INFO_RAM_FR,
			INFO_SETONLY_FR,
			INFO_BOOTINIT_FR,
			INFO_AUTOBOOT_FR,
			OK_EN, /* no translation needed */
			CANCEL_FR,
			SET_SETTINGS_FR,
			SET_SELPATH_FR,
			SET_ICONIFY_FR,
			SET_DCLICK_FR,
			SET_SORTNAME_FR,
			SET_TERM_FR,
			SET_TERMAFTER_FR,
			OK_EN, /* no translation needed */
			CANCEL_FR,
		},
	};

	switch (country)
	{
	default:
		/* Default case is USA/UK */
		p = trans_table[0];
		break;
	case COUNTRY_DE:
	case COUNTRY_SG:
		p = trans_table[1];
		break;
	case COUNTRY_FR:
	case COUNTRY_SF:
		p = trans_table[2];
		break;
	}

#define XText(obj) tree[obj].ob_spec.tedinfo->te_ptext = fstring_addr[*p++]
#define XTemplate(obj) tree[obj].ob_spec.tedinfo->te_ptmplt = fstring_addr[*p++]
#if DEBUG_TRANSLATIONS
#define XString(obj) \
	if ((tree[obj].ob_type & 0xff) == G_USERDEF) \
		tree[obj].ob_spec.userblk->ub_parm = (long)fstring_addr[*p++]; \
	else \
		tree[obj].ob_spec.free_string = fstring_addr[*p++]
#else
#define XString(obj) tree[obj].ob_spec.free_string = fstring_addr[*p++]
#endif

	windowtitle_str = *p++;
	menutitle_str = *p++;
	cpxpath_str = *p++;
	noaes_str = *p++;
	nowindow_alert = *p++;
	save_dflt_alert = *p++;
	mem_err_alert = *p++;
	file_err_alert = *p++;
	fnf_err_alert = *p++;
	reload_alert = *p++;
	al_save_header= *p++;
	al_no_sound_dma = *p++;
	cpxload_alert = *p++;
	quit_alert = *p++;
	cops_popup = *p++;
	cpx_popup = *p++;
	
	tree = tree_addr[ABOUT_DIALOG];
	XText(ABOUT_BY);
	XString(ABOUT_OK);

	tree = tree_addr[INFO_DIALOG];
	XString(INFO_FILENAME);
	XString(INFO_VERSION);
	XString(INFO_ID);
	XString(INFO_RAM);
	XString(INFO_SETONLY);
	XString(INFO_BOOTINIT);
	XString(INFO_AUTOBOOT);
	XString(INFO_OK);
	XString(INFO_CANCEL);
	
	tree = tree_addr[SET_DIALOG];
	XString(SET_SETTINGS);
	XString(SET_SELPATH);
	XString(SET_ICONIFY);
	XString(SET_DCLICK);
	XString(SET_SORTNAME);
	XString(SET_TERM);
	XTemplate(SET_TERMAFTER);
	XString(SET_OK);
	XString(SET_CANCEL);

#undef XText
#undef XTemplate
#undef XString

}

/*----------------------------------------------------------------------------------------*/

/*
 * select default language
 */
static long get_syshdr(void)
{
	OSHEADER *syshdr;
	
	syshdr = *((OSHEADER **)0x4f2);
	return (long)syshdr->os_beg;
}

static _WORD select_language(void)
{
	_WORD ag1, dummy;
	long akp;
	unsigned char nvram;
	OSHEADER *syshdr;
	_WORD os_conf;
	
	if ((aes_flags & GAI_INFO) && appl_getinfo(3, &ag1, &dummy, &dummy, &dummy) != 0)
		return ag1;
	if (get_cookie(0x5F414B50L, &akp))
		return ((unsigned short)akp) >> 8;
	if (NVMaccess(0, 6, 1, &nvram) == 0)
		return nvram;
	syshdr = (OSHEADER *)Supexec(get_syshdr);
	os_conf = (syshdr->os_conf >> 1) & 0x7f;
	return os_conf;
}

/*----------------------------------------------------------------------------------------*/

static long
read_file(char *name, void *dest, long offset, long len)
{
	long read = 0;
	short fh;

	fh = (short)Fopen(name, O_RDONLY);
	if (fh >= 0)
	{
		read = Fseek(offset, fh, SEEK_SET);

		if (read == offset)
		{
			read = Fread(fh, len, dest);
			if (read < 0)
				read = 0;
		}
		else
			/* failure */
			read = 0;

		Fclose(fh);
	}

	return read;
}

/*----------------------------------------------------------------------------------------*/
/* Service-Routine fuer CPX-Fensterdialog */
/* Funktionsergebnis:	0: Dialog schliessen 1: weitermachen */
/*	dialog:		Zeiger auf die Dialog-Struktur */
/*	events:		Zeiger auf EVNT-Struktur oder NULL */
/*	obj:		Nummer des Objekts oder Ereignisnummer */
/*	clicks:		Anzahl der Mausklicks */
/*	data:		Zeiger auf zusaetzliche Daten */
/*----------------------------------------------------------------------------------------*/
static _WORD _cdecl
handle_form_cpx(struct HNDL_OBJ_args args)
{
	DEBUG((DEBUG_FMT ": %s\n", DEBUG_ARGS, ((CPX_DESC *)args.data)->file_name));

	/* Ereignis oder Objektnummer? */
	if (args.obj < 0)
	{
		/* Closer betaetigt? */
		if (args.obj == HNDL_CLSD)
			/* beenden */
			return 0;
	}
	else
	{
		/* an object was selected */
		CPX_DESC *cpx_desc = (CPX_DESC *)args.data;

		/* double-click? */
		if (args.clicks == 2)
		{
			if (is_obj_TOUCHEXIT(cpx_desc->tree, args.obj))
				/* nur bei Touchexit-Objekten Doppelklick zurueckliefern */
				args.obj |= 0x8000;
		}

		/* return object number */
		cpx_desc->button = args.obj;
	}

	/* alles in Ordnung - weiter so */
	return 1;
}

/*----------------------------------------------------------------------------------------*/
/* CPX-Fenster mit leerer IBOX oeffnen */
/* Funktionsergebnis:	0: Fehler 1: weitermachen */
/*	cpx_desc:	CPX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static _WORD
cpx_open_window(CPX_DESC *cpx_desc)
{
	OBJECT *tree;
	_WORD kind = NAME | CLOSER | MOVER;

	DEBUG(("cpx_open_window\n"));

	cpx_desc->tree = NULL;

	tree = &cpx_desc->empty_tree[0];

	tree[0] = tree_addr[EMPTY_TREE][0];	/* leere IBOX fuer anfaenglichen Objektbaum */
	tree[1] = tree_addr[EMPTY_TREE][1];	/* leere IBOX fuer anfaenglichen Objektbaum */
	tree[0].ob_width = 256;			/* Grundausmasse eintragen */
	tree[0].ob_height = 176;

	if (cpx_desc->box_width != -1)
	{
		tree->ob_width = cpx_desc->box_width;
		tree->ob_height = cpx_desc->box_height;
	}

	/* Dialog-Struktur initialisieren */
	if (aes_flags & GAI_WDLG)
		cpx_desc->dialog = wdlg_create(handle_form_cpx, tree, cpx_desc, 0, NULL, 0);
	else
		cpx_desc->dialog = NULL;
	if (cpx_desc->dialog)
	{
		cpx_desc->whdl = wdlg_open(cpx_desc->dialog, cpx_desc->old.header.title_txt,
					   kind,
					   cpx_desc->window_x, cpx_desc->window_y, 0, NULL);

		/* Dialogfenster geoeffnet? */
		if (cpx_desc->whdl)
		{
			cpx_desc->size = *(GRECT *) &tree->ob_x;
			/* Anzahl der offenen CPXe erhoehen */
			no_open_cpx++;

			return 1;
		}
		else
		{
			/* Struktur freigeben */
			wdlg_delete(cpx_desc->dialog);
			cpx_desc->dialog = NULL;
		}
	} else
	{
		cpx_desc->whdl = wind_create_grect(kind, &desk_grect);
		if (cpx_desc->whdl > 0)
		{
			GRECT gr;

			gr.g_x = cpx_desc->window_x;
			gr.g_y = cpx_desc->window_y;
			gr.g_w = tree[ROOT].ob_width;
			gr.g_h = tree[ROOT].ob_height;
			if (gr.g_x < 0 || gr.g_y < 0)
			{
				gr.g_x = ((desk_grect.g_w - gr.g_w) >> 1) + desk_grect.g_x;
				gr.g_y = ((desk_grect.g_h - gr.g_h) >> 1) + desk_grect.g_y;
			}
			wind_calc_grect(WC_WORK, kind, &gr, &gr);
			tree[ROOT].ob_x = gr.g_x;
			tree[ROOT].ob_y = gr.g_y;
			gr.g_w = tree->ob_width;
			gr.g_h = tree->ob_height;
			wind_calc_grect(WC_BORDER, kind, &gr, &gr);
			if (gr.g_x < desk_grect.g_x)
				gr.g_x = desk_grect.g_x;
			if (gr.g_y < desk_grect.g_y)
				gr.g_y = desk_grect.g_y;
			wind_set_str(cpx_desc->whdl, WF_NAME, cpx_desc->old.header.title_txt);
			wind_open_grect(cpx_desc->whdl, &gr);
			wind_get_grect(cpx_desc->whdl, WF_WORKXYWH, &cpx_desc->size);
			tree[ROOT].ob_x = cpx_desc->size.g_x;
			tree[ROOT].ob_y = cpx_desc->size.g_y;

			/* Anzahl der offenen CPXe erhoehen */
			no_open_cpx++;

			return 1;
		}
	}
	return 0;
}

/*----------------------------------------------------------------------------------------*/
/* CPX-Fenster schliessen */
/* Funktionsergebnis:	0: Fehler 1: alles in Ordnung */
/*	cpx_desc:	CPX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static short
cpx_close_window(CPX_DESC *cpx_desc, short msg)
{
	DEBUG(("cpx_close_window\n"));

	/* Fenster offen? */
	if (cpx_desc->dialog)
	{
		cpx_desc->window_x = cpx_desc->size.g_x;
		cpx_desc->window_y = cpx_desc->size.g_y;
		wdlg_close(cpx_desc->dialog, &cpx_desc->window_x, &cpx_desc->window_y);
		wdlg_delete(cpx_desc->dialog);
		/* Fenster geschlossen */
		cpx_desc->dialog = NULL;
		cpx_desc->whdl = 0;
		/* Anzahl der offenen CPXe verringern */
		no_open_cpx--;

		return 1;
	} else if (cpx_desc->whdl > 0)
	{
		cpx_desc->window_x = cpx_desc->size.g_x;
		cpx_desc->window_y = cpx_desc->size.g_y;
		if (msg == WM_CLOSED)
		{
			wind_close(cpx_desc->whdl);
			wind_delete(cpx_desc->whdl);
		}
		cpx_desc->whdl = 0;

		no_open_cpx--;

		return 1;
	}

	return 0;
}

/*----------------------------------------------------------------------------------------*/
/* Handle des obersten Fenster zurueckliefern */
/* Funktionsresultat:	Handle des Fensters oder -1 (kein Fenster der eigenen Applikation)*/
/*----------------------------------------------------------------------------------------*/
static _WORD
top_whdl(void)
{
	_WORD whdl;

	if (wind_get_int(0, WF_TOP, &whdl) == 0) /* Fehler? */
		return -1;

	if (whdl < 0) /* liegt ein Fenster einer anderen Applikation vorne? */
		return -1;

	return whdl; /* Handle des obersten Fensters */
}

static void
remove_cpx(CPX_DESC *cpx_desc)
{
	list_remove((void **) &cpx_desc_list, cpx_desc, offsetof(CPX_DESC, next));
	list_remove((void **) &cpxlist, &cpx_desc->old, offsetof(struct cpxlist, next));

	/* resident? */
	if (cpx_desc->start_of_cpx)
		unload_cpx(cpx_desc->start_of_cpx);

	free(cpx_desc);
	DEBUG((DEBUG_FMT": free(%p)\n", DEBUG_ARGS, cpx_desc));
}

static void
init_cpx(const char *file_path, const char *file_name, short inactive)
{
	size_t cpx_desc_len = sizeof(CPX_DESC) + strlen(file_name) + 1;
	CPX_DESC *cpx_desc;

	cpx_desc = malloc(cpx_desc_len);
	if (cpx_desc)
	{
		void *addr;
		long size;

		DEBUG((DEBUG_FMT ": malloc(%lu) -> %p\n", DEBUG_ARGS, cpx_desc_len, cpx_desc));
		memset(cpx_desc, 0, cpx_desc_len);

		cpx_desc->button = -1;

		/* CPZ? */
		if (inactive)
			cpx_desc->flags |= CPXD_INACTIVE;

		cpx_desc->window_x = -1; /* Fensterposition zentriert */
		cpx_desc->window_y = -1;
		cpx_desc->icon_x = -1; /* Iconposition undefiniert */
		cpx_desc->icon_y = -1;
	
		cpx_desc->obfix_cnt = 0;
		cpx_desc->box_width = -1;
		cpx_desc->box_height = -1;

		cpx_desc->edit_obj = 0;
		cpx_desc->cursor_idx = -1;
		cpx_desc->cursor = FALSE;

		strcpy(cpx_desc->file_name, file_name);

		cpx_desc->xctrl_pb = xctrl_pb; /* Parameterblock fuer CPX kopieren */
		cpx_desc->xctrl_pb.handle = aes_handle;	/* VDI-Handle des AES */
		cpx_desc->xctrl_pb.SkipRshFix = 0;
		cpx_desc->xctrl_pb.booting = 0;

		cpx_desc->old.next = NULL;
		strncpy(cpx_desc->old.f_name, file_name, 13); /* aus Kompatibilitaetsgruenden */
		cpx_desc->old.segm = &cpx_desc->segm;						

		DEBUG(("load_cpx(%s)\n", cpx_desc->file_name));

		/* CPX laden */
		addr = load_cpx(cpx_desc, file_path, &size, 1);

		/* konnte das CPX geladen werden? */
		if (addr)
		{
			cpx_desc->start_of_cpx = addr;
			cpx_desc->end_of_cpx = (void *)((char *)addr + size);
			cpx_desc->old.segm_ok = 1; /* Segmentinformationen sind in Ordnung */

			if (cpx_desc->old.header.magic == 100)
			{
				/* CPX-Header ist in Ordnung */
				cpx_desc->old.head_ok = 1;

				list_append((void **) &cpx_desc_list, cpx_desc, offsetof(CPX_DESC, next));
				/* Kompatibilitaetsliste */
				list_append((void **) &cpxlist, &cpx_desc->old, offsetof(struct cpxlist, next));

				/* CPX aufrufen? */
				if ((cpx_desc->old.header.flags & CPX_BOOTINIT) ||
					(cpx_desc->old.header.flags & CPX_SETONLY))
				{
					/* aktives CPX? */
					if (inactive == 0)
					{
						cpx_desc->xctrl_pb.booting = 1;
						if (cpx_init(cpx_desc, &cpx_desc->xctrl_pb) == NULL)
						{
							remove_cpx(cpx_desc);
							return;
						}
						cpx_desc->xctrl_pb.booting = 0;
					}
				}
			}
			else
			{
				remove_cpx(cpx_desc);
				return;
			}

			if (!(cpx_desc->old.header.flags & CPX_RESIDENT) || inactive)
			{
				cpx_desc->start_of_cpx = NULL;
				cpx_desc->end_of_cpx = NULL;
				/* Speicher fuer CPX freigeben */
				unload_cpx(addr);
			}
		}
		else
		{
			free(cpx_desc);
			DEBUG((DEBUG_FMT ": free(%p)\n", DEBUG_ARGS, cpx_desc));
		}
	}
}

/*----------------------------------------------------------------------------------------*/
/* selektierten CPX-Eintrag suchen */
/* Funktionsresultat:	0: selektierten Eintrag gefunden 1: nicht gefunden */
/*	selected:	wird ignoriert */
/*	entry:		Zeiger auf CPX_DESC-Struktur */
/*----------------------------------------------------------------------------------------*/
static short
search_cpx_selected(long selected, void *entry)
{
	(void)selected;
	if (((CPX_DESC *) entry)->selected)
		return 0;
	else
		return 1;
}

/*----------------------------------------------------------------------------------------*/
/* CPX-ID vergleichen */
/* Funktionsresultat:	0: Eintrag gefunden <0: ID kleiner >0: ID groesser */
/*	id:		gesuchte CPX-ID */
/*	entry:		Zeiger auf CPX_DESC-Struktur */
/*----------------------------------------------------------------------------------------*/
static short
cmp_cpx_id(long id, void *entry)
{
	id -= ((CPX_DESC *) entry)->old.header.cpx_id;
	
	if (id < 0)
		return 1;
	if (id > 0)
		return -1;

	return 0;
}

static char *mybasename(char *name)
{
	char *file_name;
	char *file_name2;

	file_name = strrchr(name, '\\');
	file_name2 = strrchr(name, '/');
	if (file_name == NULL || file_name2 > file_name)
		file_name = file_name2;
	if (file_name == NULL)
		file_name = name;
	else
		file_name += 1;
	return file_name;
}

/*----------------------------------------------------------------------------------------*/
/* selektierten CPX-Eintrag suchen */
/* Funktionsresultat:	0: selektierten Eintrag gefunden 1: nicht gefunden */
/*	selected:	wird ignoriert */
/*	entry:		Zeiger auf CPX_DESC-Struktur */
/*----------------------------------------------------------------------------------------*/
static short
search_cpx_name(long name, void *entry)
{
	return stricmp(((CPX_DESC *)entry)->file_name, mybasename((char *)name));
}

#define	DOPEN_NORMAL	0

static short handle_entry(const char *name, short changes)
{
	CPX_DESC *cpx_desc;
	long	len;
	char tmp[128];

	/* "." oder ".."? */
	if ((name[0] == '.') && ((name[1] == 0) || ((name[1] == '.') && (name[2] == 0))))
		return changes;

	len = strlen(name);

	if (len <= 4)
		return changes;

	strcpy(tmp, name + len - 4); /* die letzten 4 Zeichen kopieren */
	strupr(tmp);

	cpx_desc = list_search(cpx_desc_list, (long) name, offsetof(CPX_DESC, next), search_cpx_name);

	/* CPX bereits vorhanden? */
	if (cpx_desc)
		/* Eintrag ist gueltig */
		cpx_desc->flags &= ~CPXD_INVALID;
	else
	{
		/* aktives CPX? */
		if (strcmp(".CPX", tmp) == 0)
		{
			strcpy(tmp, name);
			tmp[len - 1] = 'Z';

			cpx_desc = list_search(cpx_desc_list, (long)tmp,
					       offsetof(CPX_DESC, next),
					       search_cpx_name);

			if (cpx_desc)
				/* alten Eintrag entfernen */
				remove_cpx(cpx_desc);

			init_cpx(settings.cpx_path, name, 0);
			changes++;
		}
		else if (strcmp(".CPZ", tmp) == 0) /* inaktives CPX? */
		{
			strcpy(tmp, name);
			tmp[len - 1] = 'X';

			cpx_desc = list_search(cpx_desc_list, (long)tmp,
					       offsetof(CPX_DESC, next),
					       search_cpx_name);

			if (cpx_desc)
			{
				cpx_desc->file_name[len - 1] += 'Z' - 'X';
				/* CPX demnaechst inaktiv */
				cpx_desc->flags |= CPXD_INACTIVE;
				/* Eintrag ist gueltig */
				cpx_desc->flags &= ~CPXD_INVALID;
			}
			else
				init_cpx(settings.cpx_path, name, 1);

			changes++;
		}
	}

	return changes;
}

static short
update_cpx_list(void)
{
	CPX_DESC *cpx_desc;
	long dir_handle;
	short changes;

	changes = 0;

	/* doppelte oder alte CPXe entfernen */
	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		cpx_desc->flags |= CPXD_INVALID;
		cpx_desc = cpx_desc->next;
	}

	dir_handle = Dopendir(settings.cpx_path, DOPEN_NORMAL);
	if (dir_handle >= 0)
	{
		long err;

		do {
			struct xattr xattr;
			long err_xr;
			char buf[256+4];
			#define	name	(buf + 4)
			
			err = Dxreaddir(256 + 4, dir_handle, buf, &xattr, &err_xr);
			if (err_xr)
				err = err_xr;

			if (err == 0)
			{
				changes = handle_entry(name, changes);
			}

			#undef	name
		}
		while (err == 0);

		Dclosedir(dir_handle);
	} else if (dir_handle == -32)
	{
		_DTA *olddta;
		_DTA dta;
		char tmp_path[256];
		int err;

		strcat(strcpy(tmp_path, settings.cpx_path), "*.CP?");
		olddta = Fgetdta();
		Fsetdta(&dta);

		err = Fsfirst(tmp_path, FA_RDONLY|FA_HIDDEN|FA_SYSTEM);
		while (err == 0)
		{
			changes = handle_entry(dta.dta_name, changes);
			err = Fsnext();
		}
		Fsetdta(olddta);
	}

	cpx_desc = cpx_desc_list;

	/* nicht mehr vorhandene CPXe entfernen */
	while (cpx_desc)
	{
		CPX_DESC *next;

		next = cpx_desc->next;

		/* Eintrag ungueltig? */
		if (cpx_desc->flags & CPXD_INVALID)
		{
			remove_cpx(cpx_desc);
			changes++;
		}

		cpx_desc = next;
	}

	cpx_desc = cpx_desc_list;

	/* doppelte oder alte CPXe entfernen */
	while (cpx_desc)
	{
		CPX_DESC *test;
		CPX_DESC *next;
		
		next = cpx_desc->next;
		test = list_search(next, cpx_desc->old.header.cpx_id, offsetof(CPX_DESC, next), cmp_cpx_id);

		/* gleiches CPX mit anderem Namen? */
		if (test)
		{
			if (test->old.header.cpx_version > cpx_desc->old.header.cpx_version)
				remove_cpx(cpx_desc);
			else if (test->old.header.cpx_version < cpx_desc->old.header.cpx_version)
			{
				remove_cpx(test);
				next = cpx_desc; /* weiter mit cpx_desc vergleichen */
			}
			else /* gleiche Versionsnummer */
			{
				/* CPZ-Datei? */
				if (test->flags & CPXD_INACTIVE)
				{
					remove_cpx(test);
					next = cpx_desc; /* weiter mit cpx_desc vergleichen */
				}
				else
					remove_cpx(cpx_desc);
			}			
		}
		cpx_desc = next;
	}

	/* Anzahl der Veraenderungen */
	return changes;
}

static void
unload_all_cpx(void)
{
	/* Speicher fuer CPXe freigeben */
	while (cpx_desc_list)
		remove_cpx(cpx_desc_list);
}

/*----------------------------------------------------------------------------------------*/
/* Iconnamen von zwei Kontrollfeldern vergleichen */
/* Funktionsresultat:	< 0: Name b ist groesser	> 0: Name a ist groesser */
/*	a:		Zeiger auf Zeiger auf CPX_DESC-Struktur a */
/*	b:		Zeiger auf Zeiger auf CPX_DESC-Struktur b */
/*----------------------------------------------------------------------------------------*/
static int
cmp_cpx_names(const void *_a, const void *_b)
{
	const CPX_DESC * const *a = _a;
	const CPX_DESC * const *b = _b;

	return stricmp((*a)->old.header.title_txt, (*b)->old.header.title_txt);
}

/*----------------------------------------------------------------------------------------*/
/* Position von zwei Kontrollfeldern vergleichen */
/* Funktionsresultat:	< 0: Position von b ist groesser	> 0: Position von a ist groesser   */
/*	a:		Zeiger auf Zeiger auf CPX_DESC-Struktur a */
/*	b:		Zeiger auf Zeiger auf CPX_DESC-Struktur b */
/*----------------------------------------------------------------------------------------*/
static int
cmp_cpx_position(const void *_a, const void *_b)
{
	const CPX_DESC * const *a = _a;
	const CPX_DESC * const *b = _b;

	short ax;
	short ay;
	short bx;
	short by;

	ax = ((*a)->icon_x - 8) / 96; /* Rasterposition fuer Icon a */
	ay = ((*a)->icon_y - 8) / 40;
	bx = ((*b)->icon_x - 8) / 96; /* Rasterposition fuer Icon b */
	by = ((*b)->icon_y - 8) / 40;

	if (ay < by)
		return -1;

	if (ay > by)
		return 1;

	if (ax < bx)
		return -1;

	if (ax > bx)
		return 1;

	/* x-Koordinaten vergleichen, weil die Rasterposition gleich ist */
	if ((*a)->icon_x < (*b)->icon_x)
		return -1;

	return 1;
}

static void
sort_cpx_icons(short x, short y, short window_width)
{
	CPX_DESC *cpx_desc;

	x += 8; /* 8 Pixel Rand lassen */
	y += 8;

	cpx_desc = cpx_desc_list;

	/* neue CPXe ganz unten positionieren */
	while (cpx_desc)
	{
		/* CPX noch nicht positioniert? */
		if ((cpx_desc->icon_x < 0) || (cpx_desc->icon_y < 0))
		{
			cpx_desc->icon_x = x + 32;
			cpx_desc->icon_y = y;
			
			x += 96;
			if (x + 96 > window_width)
			{
				x = 8; /* 8 Pixel Rand lassen */
				y += 40;
			}
		}
		cpx_desc = cpx_desc->next;
	}
}

static void
tidy_up_icons(void)
{
	long entries;

	/* Kontrollfelder vorhanden? */
	entries = list_count((void *) cpx_desc_list, offsetof(CPX_DESC, next));
	if (entries > 0)
	{
		CPX_DESC **cpxd_array;
		CPX_DESC *cpx_desc;

		cpxd_array = malloc(entries * sizeof(CPX_DESC *));
		if (cpxd_array)
		{
			short i;

			/* Feld mit Zeigern auf CPX_DESC aufbauen */
			cpx_desc = cpx_desc_list;
			for (i = 0; i < entries; i++)
			{
				cpxd_array[i] = cpx_desc;
				cpx_desc = cpx_desc->next;
			}

			if (settings.sortmode)
				qsort(cpxd_array, entries, sizeof(CPX_DESC *), cmp_cpx_names);
			else
				qsort(cpxd_array, entries, sizeof(CPX_DESC *), cmp_cpx_position);

			/* Verkettung aendern */
			for (i = 1; i < entries; i++)
				cpxd_array[i - 1]->next = cpxd_array[i];
			
			cpxd_array[entries - 1]->next = NULL; /* letzter CPX_DESC hat keinen Nachfolger */
			cpx_desc_list = cpxd_array[0]; /* Zeiger auf den ersten CPX_DESC */
	
			free(cpxd_array);
		}

		cpx_desc = cpx_desc_list;
		while (cpx_desc)
		{
			cpx_desc->icon_x = -1; /* Icon neu positionieren */
			cpx_desc = cpx_desc->next;
		}
	}

	/* Hauptfenster offen? */
	if (main_window)
	{
		WINDOW *window;
		GRECT bbox;

		window = main_window;

		/* ikonifiziert? */
		if (window->wflags.iconified)
			sort_cpx_icons(0, 0, settings.mw.g_w);
		else
		{
			sort_cpx_icons(0, 0, window->workarea.g_w);
			get_cpx_bbox(&bbox);
	
			window->w = bbox.g_x + bbox.g_w;
			window->h = bbox.g_y + bbox.g_h;
			if ((window->w <= 0) || (window->h <= 0))
			{
				window->w = 1;
				window->h = 1;
			}
			set_slsize(window);
			set_slpos(window);
			full_redraw_main_window();
		}
	}
	else
		/* anhand gesicherter Breite Icons positionieren */
		sort_cpx_icons(0, 0, settings.mw.g_w);
}

/*----------------------------------------------------------------------------------------*/
/* CPX-Icons positionieren, Position aus COPS.inf einlesen */
/* Funktionsresultat:	- */
/*----------------------------------------------------------------------------------------*/
static void
read_inf(void)
{
	long offset, result;
	short version;
	GRECT out;
	int y,i,ww;
	ALPHACPX cpxpos;
	CPX_DESC *cpx_desc;

	wind_calc_grect(WC_WORK, MAINWINDOWSTYLE, &settings.mw, &out);
	ww = out.g_w;

	result = read_file(inf_name, &version,
			   offsetof(struct alphaheader, version),
			   sizeof(short));

	if (result == sizeof(short))
	{
		/* Neue Eintraege seit Fileversion 8 */
		if (version >= 8)
			offset = sizeof(struct alphaheader);
		else
			offset = sizeof(struct old_alphaheader);
	}
	else
		offset = sizeof(struct alphaheader);

	y = 0;

	for (i = 0; i < settings.count; i++)
	{
		if (read_file(inf_name, &cpxpos, offset, sizeof(ALPHACPX)) == sizeof(ALPHACPX))
		{
			offset += sizeof(ALPHACPX);

			cpx_desc = list_search(cpx_desc_list, cpxpos.id, offsetof(CPX_DESC, next), cmp_cpx_id);

			/* CPX gefunden? */
			if (cpx_desc)
			{
				cpx_desc->icon_x = cpxpos.icon_x;
				cpx_desc->icon_y = cpxpos.icon_y;

				/* Werte noch nicht initialisiert */
				if ((cpx_desc->window_x < 0) && (cpx_desc->window_y < 0))
				{
					cpx_desc->window_x = cpxpos.window_x; /* Fensterposition */
					cpx_desc->window_y = cpxpos.window_y;
				}

				/* Fensterposition ausserhalb des sichtbaren Bereichs? */
				if (cpx_desc->window_x < desk_grect.g_x)
					cpx_desc->window_x = -1;
				if (cpx_desc->window_y < desk_grect.g_y)
					cpx_desc->window_y = -1;

				/* Fensterposition ausserhalb des sichtbaren Bereichs? */
				if (cpx_desc->window_x >= (desk_grect.g_x + desk_grect.g_w))
					cpx_desc->window_x = -1;
				if (cpx_desc->window_y >= (desk_grect.g_y + desk_grect.g_h))
					cpx_desc->window_y = -1;

				if (cpxpos.icon_y > y)
					y = cpxpos.icon_y;

				/* automatisch starten? */
				if (cpxpos.autostart && cpx_desc->whdl <= 0)
				{
					struct auto_start *auto_start;

					cpx_desc->flags |= CPXD_AUTOSTART;

					auto_start = malloc(sizeof(*auto_start));
					if (auto_start)
					{
						auto_start->next = NULL;
						auto_start->cpx_desc = cpx_desc;
						list_append((void **) &auto_start_list, (void *)auto_start,
								offsetof(struct auto_start, next));
					}
				}
				/* CPX-Icon in den Vordergrund (ans Ende der Liste) */
				cpx_to_end(cpx_desc);
			}
		}
	}
	sort_cpx_icons(0, y, ww);
}

/*----------------------------------------------------------------------------------------*/
/* Einstellungen und Iconpositionen speichern */
/* Funktionsresultat:	- */
/*----------------------------------------------------------------------------------------*/
static void
save_inf(void)
{
	long ret;

	ret = Fcreate(inf_name, 0);
	if (ret < 0)
	{
		strcpy(inf_name, home);
		strcat(inf_name, "COPS.inf");		
		ret = Fcreate(inf_name, 0);

		if (ret < 0)
		{
			strcpy(inf_name, "COPS.inf");		
			ret = Fcreate(inf_name, 0);
		}
	}

	if (ret > 0)
	{
		short handle = (short)ret;

		settings.count = list_count(cpx_desc_list, offsetof(CPX_DESC, next));

		ret = Fwrite(handle, sizeof(settings), &settings);
		if (ret == sizeof(settings))
		{
			CPX_DESC *cpx_desc;

			cpx_desc = cpx_desc_list;
			while (cpx_desc)
			{
				ALPHACPX cpxpos;

				cpxpos.id = cpx_desc->old.header.cpx_id;
				cpxpos.icon_x = cpx_desc->icon_x;
				cpxpos.icon_y = cpx_desc->icon_y;
				cpxpos.window_x = cpx_desc->window_x; /* Fensterposition */
				cpxpos.window_y = cpx_desc->window_y;
				if (cpx_desc->flags & CPXD_AUTOSTART)
					cpxpos.autostart = 1;
				else
					cpxpos.autostart = 0;

				ret = Fwrite(handle, sizeof(cpxpos), &cpxpos);
				if (ret != sizeof(cpxpos))
					break;

				cpx_desc = cpx_desc->next;
			}
		}

		Fclose(handle);
	}
}

static short
get_help_id(void)
{
	return appl_find("ST-GUIDE");
}

static void
call_help(void)
{
	short help_id;

	help_id = get_help_id();
	if (help_id >= 0 && help_buf)
	{
		short msg[8];
		char **pp;

		msg[0] = VA_START;
		msg[1] = app_id;
		msg[2] = 0;
		pp = (char **)&msg[3];
		*pp = help_buf;
		msg[5] = 0;
		msg[6] = 0;
		msg[7] = 0;

		appl_write(help_id, 16, msg);
	}
}

static void
open_main_window(void)
{
	if (main_window == NULL)
	{
		WINDOW *window;
		short handle;
		short tidy_up;
		
		/* CPX-Ordner durchsuchen, CPX-Liste aktualisieren */
		tidy_up = update_cpx_list();

		/* OS liefert kein AP_TERM oder INF-Datei muss zum ersten Mal gelesen werden */
		if (!(aes_flags & GAI_APTERM) || must_read_inf)
		{
			read_inf(); /* CPXe nochmals anordnen */
			must_read_inf = 0;
		}
		
		if (tidy_up)
			/* Icons neu anordnen */
			tidy_up_icons();
			
		window = create_window(MAINWINDOWSTYLE, &settings.mw, &handle,
				       fstring_addr[windowtitle_str], fstring_addr[ICNFTITLE_STR]);

		/* Fenster angelegt? */
		if (window)
		{
			GRECT	null = { 0, 0, 0, 0 };
			GRECT	bbox;

			main_window = window;

			window->redraw = redraw_main_window; /* Adresse der Redrawroutine */

			window->x = settings.whslide; /* Sliderposition */
			window->y = settings.wvslide;

			get_cpx_bbox(&bbox);
			window->w = bbox.g_w + bbox.g_x; /* Breite der Arbeitsflaeche */
			window->h = bbox.g_h + bbox.g_y; /* Hoehe der Arbeitsflaeche */

			/* keine Icons? */
			if ((window->w <= 0) || (window->h <= 0))
			{
				window->w = 1;
				window->h = 1;
			}

			window->dx = 8; /* Breite einer Scrollspalte */
			window->dy = 8; /* Hoehe einer Scrollzeile */

			/* auf ungueltige Werte setzen, damit Slider positioniert werden */
			window->hslide = -32767;
			window->vslide = -32767;
			window->hslsize = -32767;
			window->vslsize = -32767;
	
			set_slsize(window); /* Slidergroesse berechnen */
			set_slpos(window); /* Sliderposition berechnen */

			wind_set_int(handle, WF_BEVENT, 1); /* Fenster auch im Hintergrund bedienbar */

			graf_growbox_grect(&null, &settings.mw);

			wind_open_grect(handle, &window->border);
		}
		else
			form_alert(1, fstring_addr[nowindow_alert]);
	}
	else
	{
		if (main_window->wflags.iconified)
			uniconify_window(main_window->handle, NULL);
		else
			wind_set_int(main_window->handle, WF_TOP, 0);
	}
}

static void
close_main_window(_WORD msg)
{
	if (main_window)
	{
		if (main_window->wflags.iconified)
			/* Fensterposition merken */
			settings.mw = main_window->saved_border;
		else
			/* Fensterposition merken */
			settings.mw = main_window->border;

		settings.whslide = (short) main_window->x;
		settings.wvslide = (short) main_window->y;

		delete_window(main_window->handle, msg != AC_CLOSE);
		main_window = NULL;
	}
}

static CPX_DESC *
find_cpx(short mx, short my)
{
	CPX_DESC *cpx_desc;

	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		GRECT	m;
		
		m.g_x = mx;
		m.g_y = my;
		m.g_w = 1;
		m.g_h = 1;
		
		if (cpx_in_rect(cpx_desc, &m))	
			return cpx_desc;
		
		cpx_desc = cpx_desc->next;
	}
	return NULL;
}

static short
cpx_in_rect(CPX_DESC *cpx_desc, GRECT *rect)
{
	WINDOW *w;
	GRECT box;
	_WORD extent[8];
	_WORD dummy;

	w = main_window;
	box.g_x = (_WORD)(cpx_desc->icon_x + w->workarea.g_x - w->x);
	box.g_y = (_WORD)(cpx_desc->icon_y + w->workarea.g_y - w->y);
	box.g_w = 32;
	box.g_h = 24;
	
	if (rc_intersect(rect, &box))
		return 1;

	vst_point(vdi_handle, 8, &dummy, &dummy, &dummy, &dummy);
	vqt_extent(vdi_handle, cpx_desc->old.header.title_txt, extent);
	
	box.g_w = extent[2] - extent[0];
	box.g_x = (_WORD)(cpx_desc->icon_x + w->workarea.g_x - w->x);
	box.g_y = (_WORD)(cpx_desc->icon_y + w->workarea.g_y - w->y);
	box.g_x += 16 - (box.g_w / 2);
	box.g_y += 24 + 2;
	box.g_h = extent[7] - extent[1];

	if (rc_intersect(rect, &box))
		return 1;
	
	return 0;
}

/*----------------------------------------------------------------------------------------*/
/* CPX_DESC-Struktur ans Ende der liste haengen, CPX-Icon kommt dadurch in den Vordergrund */
/* Funktionsergebnis:	- */
/*	cpx_desc:	CPX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static void
cpx_to_end(CPX_DESC *cpx_desc)
{
	list_remove((void **) &cpx_desc_list, cpx_desc, offsetof(CPX_DESC, next));
	list_remove((void **) &cpxlist, &cpx_desc->old, offsetof(struct cpxlist, next));

	cpx_desc->next = NULL;
	cpx_desc->old.next = NULL;

	list_append((void **) &cpx_desc_list, cpx_desc, offsetof(CPX_DESC, next));
	list_append((void **) &cpxlist, &cpx_desc->old, offsetof(struct cpxlist, next));
}

static void
get_cpximg_size(CPX_DESC *cpx_desc, GRECT *area)
{
	_WORD extent[8];
	_WORD dummy;
	_WORD tw;
	
	vst_point(vdi_handle, 8, &dummy, &dummy, &dummy, &dummy);
	vqt_extent(vdi_handle, cpx_desc->old.header.title_txt, extent);
	tw = extent[2] - extent[0];

	area->g_x = cpx_desc->icon_x;
	area->g_y = cpx_desc->icon_y;

	area->g_h = 27 + extent[7] - extent[1];
	area->g_w = 32;

	if (tw > 32)
	{
		area->g_x += 16 - (tw / 2) - 1;
		area->g_w = tw + 2;
	}
}

static void
redraw_cpximg(CPX_DESC *cpx_desc)
{
	GRECT area;

	get_cpximg_size(cpx_desc, &area);

	area.g_x += (_WORD)(main_window->workarea.g_x - main_window->x);
	area.g_y += (_WORD)(main_window->workarea.g_y - main_window->y);

	if (main_window)
		redraw_window(main_window->handle, &area);
}

static void
get_cpx_bbox(GRECT *bbox)
{
	CPX_DESC *cpx_desc;

	bbox->g_x = 32767;
	bbox->g_y = 32767;
	bbox->g_w = 0;
	bbox->g_h = 0;

	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		GRECT	area;

		get_cpximg_size(cpx_desc, &area);

		if (area.g_x < bbox->g_x)
			bbox->g_x = area.g_x;
		if (area.g_y < bbox->g_y)
			bbox->g_y = area.g_y;
		if ((area.g_x + area.g_w) > bbox->g_w)
			bbox->g_w = area.g_x + area.g_w;
		if ((area.g_y + area.g_h) > bbox->g_h)
			bbox->g_h = area.g_y + area.g_h;

		cpx_desc = cpx_desc->next;
	}

	bbox->g_w -= bbox->g_x - 1;
	bbox->g_h -= bbox->g_y - 1;
}

static void
full_redraw_main_window(void)
{
	if (main_window)
		redraw_window(main_window->handle, &desk_grect);
}

static void
redraw_main_window(WINDOW *w, GRECT *area)
{
	_WORD clip[4], colors[2], pxy[8];
	CPX_DESC *cpx_desc;
	MFDB src;
	MFDB des;
	_WORD dummy;


	if (w->wflags.iconified)
	{
		OBJECT *tree;
		
		tree = tree_addr[ICONIFIED_DIALOG];
		*(GRECT *) &tree->ob_x = w->workarea;
		tree[ICFDLG_ICON].ob_x = (tree->ob_width - tree[ICFDLG_ICON].ob_width) / 2;
		tree[ICFDLG_ICON].ob_y = (tree->ob_height - tree[ICFDLG_ICON].ob_height) / 2;
		objc_draw_grect(tree_addr[ICONIFIED_DIALOG], ROOT, MAX_DEPTH, area);
	}
	else
	{
		clip[0] = area->g_x;
		clip[1] = area->g_y;
		clip[2] = area->g_x + area->g_w - 1;
		clip[3] = area->g_y + area->g_h - 1;

		vs_clip(vdi_handle, 1, clip);

		vswr_mode(vdi_handle, MD_REPLACE);
		vsf_color(vdi_handle, G_WHITE);
		vsf_interior(vdi_handle, 1);
		vr_recfl(vdi_handle, clip); /* Fensterhintergrund weiss */

		des.fd_addr = NULL;

		vst_point(vdi_handle, 8, &dummy, &dummy, &dummy, &dummy);
		vst_alignment(vdi_handle, 1, 5, &dummy, &dummy);

		pxy[0] = 0;
		pxy[1] = 0;
		pxy[2] = 31;
		pxy[3] = 23;

		src.fd_w = 32;
		src.fd_h = 24;
		src.fd_wdwidth = 2;
		src.fd_stand = 1;
		src.fd_nplanes = 1;

		cpx_desc = cpx_desc_list;
		while (cpx_desc)
		{
			struct cpxlist *cpx;
			short mode;
			
			cpx = &cpx_desc->old;
			pxy[4] = (_WORD)(w->workarea.g_x + cpx_desc->icon_x - w->x);
			pxy[5] = (_WORD)(w->workarea.g_y + cpx_desc->icon_y - w->y);
			pxy[6] = pxy[4] + 31;
			pxy[7] = pxy[5] + 23;
			
			src.fd_addr = &cpx->header.icon;
	
			if (cpx_desc->selected)
			{
				_WORD extent[8];
				_WORD xy[4];
				
				vqt_extent(vdi_handle, cpx_desc->old.header.title_txt, extent);
				
				vswr_mode(vdi_handle, MD_REPLACE);
				vsf_color(vdi_handle, 1);
				vsf_interior(vdi_handle, 1);
				vr_recfl(vdi_handle, pxy + 4);
	
				xy[0] = pxy[4] + 16 - ((extent[2] - extent[0]) / 2) - 1;
				xy[1] = pxy[5] + 24 + 1;
				xy[2] = xy[0] + extent[2] - extent[0] + 1;
				xy[3] = xy[1] + 2 + extent[7] - extent[1] - 1;
	
				vr_recfl(vdi_handle, xy);
	
				colors[0] = G_WHITE;
				colors[1] = G_BLACK;
				mode = MD_TRANS;
			}
			else
			{
				if (cpx_desc->flags & CPXD_INACTIVE)
					colors[0] = G_LBLACK;
				else
					colors[0] = cpx_desc->old.header.i_info.i_color;
	
				colors[1] = G_WHITE;
				mode = MD_REPLACE;
			}
	
			vswr_mode(vdi_handle, mode);
			vrt_cpyfm(vdi_handle, mode, pxy, &src, &des, colors);
			vst_color(vdi_handle, colors[0]);
			v_gtext(vdi_handle, pxy[4] + 16, pxy[5] + 24 + 2, cpx->header.title_txt);
	
			if (cpx_desc->flags & CPXD_INACTIVE)
			{
				BITBLK *inactive;
	
				inactive = tree_addr[ICON_DIALOG][INACTIVE_IMG].ob_spec.bitblk;
				
				colors[0] = G_RED;
				colors[1] = G_WHITE;
				
				src.fd_addr = inactive->bi_pdata;
				vrt_cpyfm(vdi_handle, MD_TRANS, pxy, &src, &des, colors);
			}
			
			cpx_desc = cpx_desc->next;
		}
	}
}

static void
deselect_all_cpx_draw(void)
{
	CPX_DESC *cpx_desc;

	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		if (cpx_desc->selected)
		{
			cpx_desc->selected = 0;
			redraw_cpximg(cpx_desc);
		}
		cpx_desc = cpx_desc->next;
	}
}


static void
select_all_cpx(void)
{
	CPX_DESC *cpx_desc;

	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		cpx_desc->selected = 1;
		cpx_desc = cpx_desc->next;
	}

	full_redraw_main_window();
}

static void
draw_cpx_frames(int xof, int yof)
{
	CPX_DESC *cpx_desc;
	_WORD xy[10];

	graf_mouse(M_OFF, NULL);

	xy[0] = desk_grect.g_x;
	xy[1] = desk_grect.g_y;
	xy[2] = xy[0] + desk_grect.g_w - 1;
	xy[3] = xy[1] + desk_grect.g_h - 1;

	vs_clip(vdi_handle, 1, xy);

	vswr_mode(vdi_handle,MD_XOR);
	vsl_type(vdi_handle,USERLINE);
	vsl_udsty(vdi_handle,0x5555);

	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		if (cpx_desc->selected)
		{
			GRECT area;

			get_cpximg_size(cpx_desc, &area);

			area.g_x += (_WORD)(main_window->workarea.g_x - main_window->x + xof);
			area.g_y += (_WORD)(main_window->workarea.g_y - main_window->y + yof);

			xy[0] = area.g_x;
			xy[1] = area.g_y;
			xy[4] = area.g_x + area.g_w - 1;
			xy[5] = area.g_y + area.g_h - 1;

			xy[2] = xy[4];
			xy[3] = xy[1];
			xy[6] = xy[0];
			xy[7] = xy[5];
			xy[8] = xy[0];
			xy[9] = xy[1];

			v_pline(vdi_handle, 5, xy);
		}
		cpx_desc = cpx_desc->next;
	}

	graf_mouse(M_ON, NULL);
}

/*----------------------------------------------------------------------------------------*/
/* CPX oeffnen */
/* Funktionsergebnis:	- */
/*	cpx_desc:	CPX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static void
open_cpx(CPX_DESC *cpx_desc)
{
	/* CPX schon offen? */
	if (cpx_desc->whdl > 0)
	{
		wind_set_int(cpx_desc->whdl, WF_TOP, 0);
		return;
	}

	/* CPX inaktiv? */
	if (cpx_desc->flags & CPXD_INACTIVE)
	{
		/* Doppelklick aktiviert CPX? */
		if (settings.clickact)
		{
			/* CPX aktivieren */
			activate_cpx(cpx_desc);
			/* den Desktop ueber Veraenderungen informieren */
			update_cpx_path();
		}
		else
			return;
	}

	g_open_cpx = cpx_desc;
}
	
/*----------------------------------------------------------------------------------------*/
/* cpx_init() und cpx_call() aufrufen (Kontext und Stack gehoeren dem CPX) */
/* Funktionsergebnis:	- */
/*	cpx_desc:	CPX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
void
open_cpx_context(CPX_DESC *cpx_desc)
{
	long size;
	short err = 1;

	DEBUG(("open_cpx_context\n"));

	/* ist das CPX resident? */
	if (cpx_desc->start_of_cpx)
	{
		DEBUG(("open_cpx_context: resident cpx\n"));

		cpx_desc->info = cpx_init(cpx_desc, &cpx_desc->xctrl_pb);

		/* soll kein Fenster geoeffnet werden? */
		if ((unsigned long) cpx_desc->info == 1)
			err = 0;
		else if (cpx_desc->info) /* alles in Ordnung? */
		{
			cpx_desc->is_evnt_cpx = 0; /* wahrscheinlich Form-CPX */

			/* Fenster oeffnen */
			if (cpx_open_window(cpx_desc))
			{
				err = 0; /* soweit hat alles geklappt */
				cpx_desc->size.g_y += desk_grect.g_h; /* CPX-Rechteck ausserhalb des Schirms */

				/* Form-CPX? */
				if (cpx_call(cpx_desc, &cpx_desc->size) == 0)
					cpx_close_window(cpx_desc, WM_CLOSED);
				else
				{
					cpx_desc->is_evnt_cpx = 1;

					/* Fensterinnenflaeche */
					wind_get_grect(cpx_desc->whdl, WF_WORKXYWH, &cpx_desc->size);
					/* dem CPX mitteilen, wo sich der Fensterinhalt befindet */
					cpx_wmove(cpx_desc, &cpx_desc->size);
				}
			}
		}
	}
	else
	{
		/* CPX neu laden */
		DEBUG(("open_cpx_context: load cpx new\n"));

		/* Resource noch nicht angepasst */
		cpx_desc->xctrl_pb.SkipRshFix = 0;
		/* CPX laden, Header nicht einlesen */
		cpx_desc->start_of_cpx = load_cpx(cpx_desc, settings.cpx_path, &size, 0);

		/* Fehler beim Laden? */
		if (cpx_desc->start_of_cpx == NULL)
		{
			long id;

			id = cpx_desc->old.header.cpx_id;

			/* Liste aktualisieren */
			if (update_cpx_list())
			{
				tidy_up_icons(); /* Icons neu anordnen */

				cpx_desc = cpx_desc_list;
				while (cpx_desc)
				{
					/* CPX wiedergefunden? */
					if (cpx_desc->old.header.cpx_id == id)
					{
						/* CPX aktiv? */
						if ((cpx_desc->flags & CPXD_INACTIVE) == 0)
						{
							/* CPX laden, Header nicht einlesen */
							cpx_desc->start_of_cpx = load_cpx(cpx_desc, settings.cpx_path, &size, 0);
							break;
						}
					}
					cpx_desc = cpx_desc->next;
				}
			}
		}

		if (cpx_desc && cpx_desc->start_of_cpx)
		{
			cpx_desc->end_of_cpx = (void *)((char *)cpx_desc->start_of_cpx + size);
			cpx_desc->info = cpx_init(cpx_desc, &cpx_desc->xctrl_pb);

			/* soll kein Fenster geoeffnet werden? */
			if ((unsigned long) cpx_desc->info == 1)
				err = 0;
			else if (cpx_desc->info) /* alles in Ordnung? */
			{
				cpx_desc->is_evnt_cpx = 0; /* wahrscheinlich Form-CPX */

				/* Fenster oeffnen */
				if (cpx_open_window(cpx_desc))
				{
					err = 0; /* soweit hat alles geklappt */
					cpx_desc->size.g_y += desk_grect.g_h; /* CPX-Rechteck ausserhalb des Schirms */

					/* Form-CPX? */
					if (cpx_call(cpx_desc, &cpx_desc->size) == 0)
						cpx_close_window(cpx_desc, WM_CLOSED);
					else
					{
						cpx_desc->is_evnt_cpx = 1;

						/* Fensterinnenflaeche */
						wind_get_grect(cpx_desc->whdl, WF_WORKXYWH, &cpx_desc->size);
						/* dem CPX mitteilen, wo sich der Fensterinhalt befindet */
						cpx_wmove(cpx_desc, &cpx_desc->size);
					}
				}
			}

			/* Form-CPX? */
			if (cpx_desc->is_evnt_cpx == 0)
			{
				unload_cpx(cpx_desc->start_of_cpx);
				cpx_desc->start_of_cpx = NULL;
				cpx_desc->end_of_cpx = NULL;
			}
		}
	}

	/* Fehler beim Oeffnen? */
	if (err)
		form_alert(1, fstring_addr[cpxload_alert]);

	DEBUG(("open_cpx_context: done (err = %d)\n", err));
}

/*----------------------------------------------------------------------------------------*/
/* CPX aktivieren (CPZ -> CPX), ggf. laden und cpx_init() aufrufen */
/* Funktionsergebnis:	0: Fehler 1: CPX aktiviert */
/*	cpx_desc:	PX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static short
activate_cpx(CPX_DESC *cpx_desc)
{
	char old[128];
	char new[128];

	/* CPX deaktiviert? */
	if (cpx_desc->flags & CPXD_INACTIVE)
	{
		strcpy(old, settings.cpx_path);
		strcat(old, cpx_desc->file_name);

		strcpy(new, old);
		new[strlen(new) - 1] -= 'Z' - 'X';

		/* laesst sich das CPX umbenennen? */
		if (Frename(0, old, new) == 0)
		{
			cpx_desc->flags &= ~CPXD_INACTIVE;
			cpx_desc->file_name[strlen(cpx_desc->file_name) - 1] -= 'Z' - 'X';

			if ((cpx_desc->old.header.flags & CPX_BOOTINIT) ||
			    (cpx_desc->old.header.flags & CPX_RESIDENT))
			{
				void *addr;
				long size;

				addr = load_cpx(cpx_desc, settings.cpx_path, &size, 1);	/* CPX laden */

				/* konnte das CPX geladen werden? */
				if (addr)
				{
					cpx_desc->start_of_cpx = addr;
					cpx_desc->end_of_cpx = (void *)((char *)addr + size);

					/* CPX aufrufen? */
					if (cpx_desc->old.header.flags & CPX_BOOTINIT)
					{
						cpx_desc->xctrl_pb.booting = 1;
						if (cpx_init(cpx_desc, &cpx_desc->xctrl_pb) == NULL)
						{
							remove_cpx(cpx_desc);
							return 0;
						}
						cpx_desc->xctrl_pb.booting = 0;
					}

					if (!(cpx_desc->old.header.flags & CPX_RESIDENT))
					{
						unload_cpx(addr); /* Speicher fuer CPX freigeben */
						cpx_desc->start_of_cpx = NULL;
						cpx_desc->end_of_cpx = NULL;
					}
				}
			}
		}
		else
			return 0;
	}

	/* CPX aktiv */
	return 1;
}

/*----------------------------------------------------------------------------------------*/
/* CPX deaktivieren (CPX -> CPZ), ggf. Speicher freigeben */
/* Funktionsergebnis:	0: Fehler 1: CPX inaktiv */
/*	cpx_desc:	CPX-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static short
deactivate_cpx(CPX_DESC *cpx_desc)
{
	char old[128];
	char new[128];

	/* CPX aktiv? */
	if ((cpx_desc->flags & CPXD_INACTIVE) == 0)
	{
		strcpy(old, settings.cpx_path);
		strcat(old, cpx_desc->file_name);

		strcpy(new, old);
		new[strlen(new) - 1] += 'Z' - 'X';

		/* CPX resident im Speicher und Dialog nicht geoeffnet? */
		if (cpx_desc->start_of_cpx && cpx_desc->whdl <= 0)
		{
			unload_cpx(cpx_desc->start_of_cpx); /* Speicher freigeben */
			cpx_desc->start_of_cpx = NULL;
			cpx_desc->end_of_cpx = NULL;
		}

		/* laesst sich das CPX umbenennen? */
		if (Frename(0, old, new) == 0)
		{
			cpx_desc->flags |= CPXD_INACTIVE;
			cpx_desc->file_name[strlen(cpx_desc->file_name) - 1] += 'Z' - 'X';
		}
		else
			return 0;
	}

	/* CPX inaktiv */
	return 1;
}

/*----------------------------------------------------------------------------------------*/
/* Nachricht ueber geaenderten CPX-Namen verschicken */
/* Funktionsergebnis:	- */
/*----------------------------------------------------------------------------------------*/
static void
update_cpx_path(void)
{
	short msg[8];

	full_redraw_main_window();

	msg[0] = SH_WDRAW;
	msg[1] = app_id;
	msg[2] = 0;
	msg[3] = settings.cpx_path[0]; /* Laufwerksbuchstabe */
	if (msg[3] < 'a')
		msg[3] -= 'A';
	else
		msg[3] -= 'a';
	msg[4] = msg[5] = msg[6] = msg[7] = 0;

	shel_write(SHW_BROADCAST, 0, 0, (char *)msg, NULL); /* andere Prozesse benachrichtigen */
}

static char *myitoa(unsigned int value, char *buffer, unsigned int radix)
{
	char *p;
	char tmpbuf[12];
	short i = 0;
	static char const __atoi_numstr[] = "0123456789ABCDEF";

	do {
		tmpbuf[i++] = __atoi_numstr[value % radix];
	} while ((value /= radix) != 0);

	p = buffer;
	while (--i >= 0)	/* reverse it back  */
		*p++ = tmpbuf[i];
	*p = '\0';

	return buffer;
}

static void
cpx_info(CPX_DESC *cpx_desc)
{
	OBJECT *cpxinfo;
	char txt[34];
	struct cpxlist *cpx;
	char *str;
	
	cpx = &cpx_desc->old;
	cpxinfo = tree_addr[INFO_DIALOG];
	
	str = is_userdef_title(cpxinfo + CITITLE);

	if (str == NULL)
		str = cpxinfo[CITITLE].ob_spec.free_string;

	strcpy(str, cpx->header.title_txt);
	
	if (strlen(cpx_desc->file_name) > 20)
	{
		strncpy(cpxinfo[CIFILE].ob_spec.free_string, cpx_desc->file_name, 17);
		strcpy(cpxinfo[CIFILE].ob_spec.free_string + 17, "...");
	}
	else
		strcpy(cpxinfo[CIFILE].ob_spec.free_string, cpx_desc->file_name);

	myitoa(cpx->header.cpx_version, txt, 16);
	strrev(txt);
	while (strlen(txt) < 3)
		strcat(txt,"0");
	txt[5]=txt[4];
	txt[4]=txt[3];
	txt[3]=txt[2];
	txt[2]='.';
	strcpy(cpxinfo[CIVER].ob_spec.free_string, strrev(txt));
	strncpy(cpxinfo[CIID].ob_spec.free_string, (char *) &(cpx->header.cpx_id), 4);

	if (cpx->header.flags & CPX_RESIDENT)
		cpxinfo[INFO_RAM].ob_state |= OS_SELECTED;
	else
		cpxinfo[INFO_RAM].ob_state &= ~OS_SELECTED;

	if (cpx->header.flags & CPX_SETONLY)
		cpxinfo[INFO_SETONLY].ob_state |= OS_SELECTED;
	else
		cpxinfo[INFO_SETONLY].ob_state &= ~OS_SELECTED;

	if (cpx->header.flags & CPX_BOOTINIT)
		cpxinfo[INFO_BOOTINIT].ob_state |= OS_SELECTED;
	else
		cpxinfo[INFO_BOOTINIT].ob_state &= ~OS_SELECTED;

	if (cpx_desc->flags & CPXD_AUTOSTART)
		cpxinfo[INFO_AUTOBOOT].ob_state |= OS_SELECTED;
	else
		cpxinfo[INFO_AUTOBOOT].ob_state &= ~OS_SELECTED;

	if (do_dialog(cpxinfo) == INFO_OK)
	{
		if (is_obj_SELECTED(cpxinfo, INFO_AUTOBOOT))
			cpx_desc->flags |= CPXD_AUTOSTART;
		else
			cpx_desc->flags &= ~CPXD_AUTOSTART;
	
		if (is_obj_SELECTED(cpxinfo, INFO_RAM))
			cpx->header.flags |= CPX_RESIDENT;
		else
			cpx->header.flags &= ~CPX_RESIDENT;

		save_header(cpx);
	}
}

static short
do_dialog(OBJECT *tree)
{
	EVNTDATA mouse;
	GRECT start;
	GRECT center;
	_WORD dummy;
	_WORD obj;

	wind_update(BEG_UPDATE);
	wind_update(BEG_MCTRL);

	graf_mkstate_evntdata(&mouse);
	start.g_x = mouse.x - 8;
	start.g_y = mouse.y - 8;
	start.g_w = 16;
	start.g_h = 16;

	form_center_grect(tree, &center);
	graf_growbox_grect(&start, &center);

	if (aes_flags & GAI_FLYDIAL)
	{
		void *flyinf = NULL;
		form_xdial_grect(FMD_START, &center, &center, &flyinf);

		objc_draw_grect(tree, ROOT,MAX_DEPTH, &center);
		obj = form_xdo(tree, 0, &dummy, NULL, flyinf) & 0x7fff;
		form_xdial_grect(FMD_FINISH, &center, &center, &flyinf);
	} else
	{
		form_dial_grect(FMD_START, &center, &center);

		objc_draw_grect(tree, ROOT,MAX_DEPTH, &center);
		obj = form_do(tree, 0) & 0x7fff;
		form_dial_grect(FMD_FINISH, &center, &center);
	}

	graf_shrinkbox_grect(&start, &center);

	wind_update(END_MCTRL);
	wind_update(END_UPDATE);

	tree[obj].ob_state &= ~OS_SELECTED;

	return obj;
}


static void append_slash(char *path)
{
	size_t len = strlen(path);
	if (len > 0)
	{
		if (path[len - 1] != '\\' && path[len - 1] != '/')
			strcat(path, "\\");
	}
}


static void dosify(char *path)
{
	char *p;
	while ((p = strchr(path, '/')) != NULL)
		*p = '\\';
}

static void
einstellungen(void)
{
	OBJECT *alphaconf;
	char path[128];
	char *save_ptext;
	short button;

	alphaconf = tree_addr[SET_DIALOG];
	save_ptext = alphaconf[SET_PATH].ob_spec.tedinfo->te_ptext;

	strcpy(path, settings.cpx_path);

	alphaconf[SET_PATH].ob_spec.tedinfo->te_ptext = path;
	myitoa(settings.after, alphaconf[SET_TERMAFTER].ob_spec.tedinfo->te_ptext, 10);

	if (settings.booticon)
		obj_SELECTED(alphaconf, SET_ICONIFY);
	else
		obj_DESELECTED(alphaconf, SET_ICONIFY);

	if (settings.clickact)
		obj_SELECTED(alphaconf, SET_DCLICK);
	else
		obj_DESELECTED(alphaconf, SET_DCLICK);

	if (settings.sortmode)
		obj_SELECTED(alphaconf, SET_SORTNAME);
	else
		obj_DESELECTED(alphaconf, SET_SORTNAME);

	if (settings.term)
		obj_SELECTED(alphaconf, SET_TERM);
	else
		obj_DESELECTED(alphaconf, SET_TERM);

	do {
		if ((button = do_dialog(alphaconf)) == SET_SELPATH)
		{
			char tmp_path[128];
			char file[128];
			_WORD btn;
			_WORD ok;
			
			file[0] = 0;

			dosify(path);
			append_slash(path);
			strcpy(tmp_path, path);

			wind_update(BEG_UPDATE);

			if (aes_flags & GAI_MAGIC)
			{
				strcat(tmp_path, "*.CPX,*.CPZ");
				ok = fsel_exinput(tmp_path, file, &btn, fstring_addr[cpxpath_str]);
			}
			else
			{
				strcat(tmp_path, "*.CP?");
				if (_AESversion >= 0x140)
					ok = fsel_exinput(tmp_path, file, &btn, fstring_addr[cpxpath_str]);
				else
					ok = fsel_input(tmp_path, file, &btn);
			}

			wind_update(END_UPDATE);

			if (ok && (btn == 1))
			{
				char *c;

				c = mybasename(tmp_path);

				if (c != tmp_path)
					c[0] = '\0';

				strcpy(path, tmp_path);
			}
		}
	}
	while (button == SET_SELPATH);

	alphaconf[SET_PATH].ob_spec.tedinfo->te_ptext = save_ptext;

	if (button == SET_OK)
	{
		settings.booticon = is_obj_SELECTED(alphaconf, SET_ICONIFY);
		settings.clickact = is_obj_SELECTED(alphaconf, SET_DCLICK);
		settings.sortmode = is_obj_SELECTED(alphaconf, SET_SORTNAME);
		settings.term = is_obj_SELECTED(alphaconf, SET_TERM);

		settings.after = max(1, atoi(alphaconf[SET_TERMAFTER].ob_spec.tedinfo->te_ptext));
		set_termtime();

		/* unterschiedlicher Pfad? */
		if (stricmp(settings.cpx_path, path) != 0)
		{
			strcpy(settings.cpx_path, path);
			do_reload = 1; /* neuladen */
		}
	}
}

static void
about(void)
{
	do_dialog(tree_addr[ABOUT_DIALOG]);
}

static short
handle_keyboard(unsigned short kstate, unsigned short key)
{
	DEBUG(("handle_keyboard 0x%x 0x%x\n", kstate, key));

	/* Scancode auswerten? */
	if (kstate & KbSCAN)
	{
		switch (kstate & ~KbSCAN)
		{
		case KbNORMAL: /* keine Umschalttaste gedrueckt */
			switch (key)
			{
			case KbHELP:
#if DEBUG_TRANSLATIONS
				/* for testing translations */
				xctrl_pb.Country_Code = (xctrl_pb.Country_Code + 1) % 3;
				set_texts(xctrl_pb.Country_Code);
				wind_set_str(main_window->handle, WF_NAME, fstring_addr[windowtitle_str]));
#else
				call_help();
#endif
				break;
			case KbUP: /* darueberliegendes Icon anwaehlen */
				break;
			case KbDOWN: /* darunterliegendes Icon anwaehlen */
				break;
			case KbLEFT: /* linksliegendes Icon anwaehlen */
				break;
			case KbRIGHT: /* rechtsliegendes Icon anwaehlen */
				break;
			}
			break;

		case KbCONTROL: /* Control */
			if (main_window && (main_window->wflags.iconified == 0))
			{
				_WORD msg[8];

				msg[0] = WM_ARROWED;
				msg[1] = app_id;
				msg[2] = 0;
				msg[3] = main_window->handle;
				msg[4] = -1;

				switch (key)
				{
				case KbUP: /* nach oben scrollen */
					msg[4] = WA_UPLINE;
					break;
				case KbDOWN: /* nach unten scrollen */
					msg[4] = WA_DNLINE;
					break;
				case KbCtrlLEFT: /* nach links scrollen */
					msg[4] = WA_LFLINE;
					break;
				case KbCtrlRIGHT: /* nach rechts scrollen */
					msg[4] = WA_RTLINE;
					break;
				}

				if (msg[4] >= 0)
					handle_message(msg);
			}
			break;
		}
	}
	else
	{
		/* ASCII-Codes auswerten */
		switch (kstate)
		{
		case KbNORMAL:
			if ((key == 13) && (main_window->wflags.iconified == 0))
			{
				CPX_DESC *cpx_desc;

				cpx_desc = list_search(cpx_desc_list, 1, offsetof(CPX_DESC, next), search_cpx_selected);
				if (cpx_desc)
					open_cpx(cpx_desc);
			}
			break;

		case KbLSHIFT:
		case KbRSHIFT:
			if (main_window && (main_window->wflags.iconified == 0))
			{
				_WORD	msg[8];

				msg[0] = WM_ARROWED;
				msg[1] = app_id;
				msg[2] = 0;
				msg[3] = main_window->handle;
				msg[4] = -1;

				switch (key)
				{
					case 56: /* Seite nach oben scrollen */
					{
						msg[4] = WA_UPPAGE;
						break;
					}	
					case 50: /* Seite nach unten scrollen */
					{
						msg[4] = WA_DNPAGE;
						break;
					}
					case 52: /* Seite nach links scrollen */
					{
						msg[4] = WA_LFPAGE;
						break;
					}
					case 54: /* Seite nach rechts scrollen */
					{
						msg[4] = WA_RTPAGE;
						break;
					}
				}

				if (msg[4] >= 0)
					handle_message(msg);
			}
			break;

		case KbCONTROL: /* Ctrl-Shortcut */
			switch (key)
			{
			case 'i': /* Info... */
				if (main_window->wflags.iconified == 0)
				{
					CPX_DESC *cpx_desc;

					cpx_desc = list_search(cpx_desc_list, 1,
							offsetof(CPX_DESC, next), search_cpx_selected);
					if (cpx_desc)
						cpx_info(cpx_desc);
				}
				break;
			case 'a': /* alles markieren */
				if (main_window->wflags.iconified == 0)
					select_all_cpx();
				break;
			case 'e': /* Einstellungen */
				einstellungen();
				break;
			case 'o': /* Oeffnen */
				if (main_window->wflags.iconified == 0)
				{
					CPX_DESC *cpx_desc;

					cpx_desc = list_search(cpx_desc_list, 1,
							offsetof(CPX_DESC, next), search_cpx_selected);
					if (cpx_desc)
						open_cpx(cpx_desc);
				}
				break;
			case 'q':
				if (_app)
					/* COPS laeuft als Applikation */
					return 1; /* Beenden moeglich */
		
				if (_app == 0 && aes_global[1] != 1) /* Accessory and Multitasking AES? */
				{
					if (form_alert(1, fstring_addr[quit_alert]) == 1)
						return 1;
				}

				break;
			case 'u':
				if (main_window)
				{
					_WORD msg[8];
					
					msg[0] = WM_CLOSED;
					msg[1] = app_id;
					msg[2] = 0;
					msg[3] = main_window->handle;

					return handle_message(msg);
				}
				break;
			case 'w':
				/* cycle */
				break;
			}
			break;

		case KbCONTROL|KbALT:
			switch (key)
			{
			case ' ':
				if (main_window && (top_whdl() == main_window->handle))
				{
					if (main_window->wflags.iconified)
						uniconify_window(main_window->handle, NULL);
					else
						iconify_window(main_window->handle, NULL);
				}
				break;
			case '+':
				if (main_window && (main_window->wflags.iconified == 0))
				{
					_WORD msg[8];
					
					msg[0] = WM_FULLED;
					msg[1] = app_id;
					msg[2] = 0;
					msg[3] = main_window->handle;

					handle_message(msg);
				}
				break;
			}
			break;
		}
	}

	/* nicht verwendete Tasten an App mit der aktiven Menueleiste schicken */
	return 0;
}

static void
drag_icons(void)
{
	EVNTDATA m;

	/* kurz warten */
	evnt_timer(10);

	wind_update(BEG_UPDATE);
	wind_update(BEG_MCTRL);
	graf_mkstate_evntdata(&m);

	/* Maustaste immer noch gedrueckt? */
	if (m.bstate == 1)
	{
		EVNTDATA n;
		WINDOW *w;
		short x_offset;
		short y_offset;

		w = main_window;

		x_offset = 0;
		y_offset = 0;
		n = m;

		/* Mausform umschalten */
		graf_mouse(FLAT_HAND, NULL);

		/* Maustaste gedrueckt? */
		while (n.bstate == 1)
		{
			short x;
			short y;

			draw_cpx_frames(x_offset, y_offset); /* Iconrahmen zeichnen */
			
			x = n.x; /* Mauskoordinaten merken */
			y = n.y;

			do {
				graf_mkstate_evntdata(&n);
			}
			while ((n.x == x) && (n.y == y) && (n.bstate == 1));

			draw_cpx_frames(x_offset, y_offset); /* Iconrahmen entfernen */

			x_offset = n.x - m.x;
			y_offset = n.y - m.y;
		}

		if (((n.x != m.x) || (n.y != m.y)) && 
			(between(n.x, w->workarea.g_x, w->workarea.g_x + w->workarea.g_w - 1) &&
			  between(n.y, w->workarea.g_y, w->workarea.g_y + w->workarea.g_h - 1)))
		{
			CPX_DESC *cpx_desc;
			GRECT old_bbox;
			GRECT bbox;

			get_cpx_bbox(&old_bbox); /* umgebendes Iconrechteck */

			cpx_desc = cpx_desc_list;

			/* selektierte Icons verschieben und Bereich dahinter neuzeichnen */
			while (cpx_desc)
			{
				if (cpx_desc->selected)
				{
					GRECT area;
				
					get_cpximg_size(cpx_desc, &area);
					area.g_x += (_WORD)(w->workarea.g_x - w->x);
					area.g_y += (_WORD)(w->workarea.g_y - w->y);

					cpx_desc->icon_x += x_offset; /* Icon verschieben */
					cpx_desc->icon_y += y_offset;

					redraw_window(w->handle, &area); /* Bereich dahinter neuzeichnen */
				}
				cpx_desc = cpx_desc->next;
			}

			/* neues umgebendes Iconrechteck */
			get_cpx_bbox(&bbox);
	
			/* Icon nach links herausgeschoben? */
			if (bbox.g_x < 0)
				x_offset = bbox.g_x;
			else if ((old_bbox.g_x < w->x) && (bbox.g_x > old_bbox.g_x)) /* kann die Sliderposition korrigiert werden? */
			{
				x_offset = bbox.g_x - old_bbox.g_x;
				if (x_offset > w->x)
					x_offset = w->x;
			}
			else
				/* keine horizontale Verschiebung */
				x_offset = 0;

			/* Icon nach oben herausgeschoben? */
			if (bbox.g_y < 0)
				y_offset = bbox.g_y;
			else if ((old_bbox.g_y < w->y) && (bbox.g_y > old_bbox.g_y)) /* kann die Sliderposition korrigiert werden? */
			{
				y_offset = bbox.g_y - old_bbox.g_y;
				if (y_offset > w->y)
					y_offset = w->y;
			}
			else
				/* keine vertikale Verschiebung */
				y_offset = 0;

			/* Iconpositionen korrigieren */
			cpx_desc = cpx_desc_list;
			while (cpx_desc)
			{
				cpx_desc->icon_x -= x_offset;
				cpx_desc->icon_y -= y_offset;
	
				cpx_desc = cpx_desc->next;
			}
	
			w->x -= x_offset; /* Sliderposition korrigieren */
			w->y -= y_offset;
	
			get_cpx_bbox(&bbox); /* umgebendes Iconrechteck */
			w->w = bbox.g_x + bbox.g_w; /* Breite des Fensterinhalts */
			w->h = bbox.g_y + bbox.g_h; /* Hoehe des Fensterinhalts */
			set_slsize(w); /* Slidergroesse setzen */
			set_slpos(w); /* Sliderposition setzen */

			/* verschobene (selektierte) Icons neuzeichnen */
			cpx_desc = cpx_desc_list;
			while (cpx_desc)
			{
				if (cpx_desc->selected)
					redraw_cpximg(cpx_desc);
				cpx_desc = cpx_desc->next;
			}
	
		}
		graf_mouse(ARROW, NULL);
	}

	wind_update(END_MCTRL);
	wind_update(END_UPDATE);
}

static void
select_icons(void)
{
	EVNTDATA mevnt;

	/* kurz warten */
	evnt_timer(10);

	wind_update(BEG_MCTRL);
	graf_mkstate_evntdata(&mevnt);

	/* Maustaste noch immer gedrueckt? */
	if (mevnt.bstate == 1)
	{
		GRECT area;
		_WORD w;
		_WORD h;

		area.g_x = mevnt.x;
		area.g_y = mevnt.y;

		graf_mouse(POINT_HAND, NULL);

		/* Gummiband aufgezogen? */
		if (graf_rubbox(area.g_x, area.g_y, -32767, -32767, &w, &h))
		{
			/* Gummiband nach links aufgezogen? */
			if (w < 1)
			{
				area.g_x += w - 1;
				area.g_w = 2 - w;
			}
			else
				/* Gummiband nach rechts aufgezogen */
				area.g_w = w;

			/* Gummiband nach oben aufgezogen? */
			if (h < 1)
			{
				area.g_y += h - 1;
				area.g_h = 2 - h;
			}
			else
				/* Gummiband nach unten aufgezogen */
				area.g_h = h;

			if (rc_intersect(&main_window->workarea, &area))
			{
				CPX_DESC *cpx_desc;

				cpx_desc = cpx_desc_list;
				while (cpx_desc)
				{
					/* CPX-Icon innerhalb des Gummibands? */
					if (cpx_in_rect(cpx_desc, &area))
					{
						cpx_desc->selected = !cpx_desc->selected;
						redraw_cpximg(cpx_desc);
					}
					cpx_desc = cpx_desc->next;
				}
			}
		}
		graf_mouse(ARROW, NULL);
	}
	wind_update(END_MCTRL);
}

static void
handle_bt1(CPX_DESC *cpx_desc, int kstate, int clicks)
{
	switch (clicks)
	{
		case 1: /* Einfachklick */
		{
			if (cpx_desc)
			{
				if (cpx_desc->selected)
				{
					/* Shift gedrueckt? */
					if (kstate & K_SHIFT)
					{
						cpx_desc->selected = 0;
						redraw_cpximg(cpx_desc);
					}
					else
						drag_icons(); /* Icons verschieben */
				}
				else /* CPX selektieren */
				{
					/* Shift nicht gedrueckt? */
					if ((kstate & K_SHIFT) == 0)
						deselect_all_cpx_draw();

					cpx_desc->selected = 1;
					cpx_to_end(cpx_desc); /* CPX-Icon in den Vordergrund (ans Ende der Liste) */
					redraw_cpximg(cpx_desc);
				}
			}
			else /* Mausklick ging nicht auf ein CPX-Icon */
			{
				if (top_whdl() != main_window->handle)
					wind_set_int(main_window->handle, WF_TOP, 0);
				else
				{
					/* Shift nicht gedrueckt? */
					if ((kstate & K_SHIFT) == 0)
						deselect_all_cpx_draw();

					/* Gummiband aufziehen und Icons selektieren */
					select_icons();
				}
			}
			break;
		}	
		case 2: /* Doppelklick */
		{
			/* Klick auf CPX-Icon? */
			if (cpx_desc)
			{
				deselect_all_cpx_draw();
				cpx_desc->selected = 1;
				redraw_cpximg(cpx_desc);
				open_cpx(cpx_desc);
			}
			break;
		}
	}
}

static void
handle_button(int mx, int my, int bstate, int kstate, int clicks)
{
	if (main_window == NULL)
		return;

	/* Mausklick in ein anderes Fenster? */
	if (wind_find(mx, my) != main_window->handle)
		return;

	if (main_window->wflags.iconified && (bstate==1))
	{
		if (clicks == 2)
			uniconify_window(main_window->handle, NULL);
	}
	else
	{
		CPX_DESC *cpx;

		if (main_window->wflags.iconified)
			cpx = NULL;
		else
			cpx = find_cpx(mx, my);

		switch (bstate)
		{
			case 1: /* linke Maustaste */
			{
				handle_bt1(cpx, kstate, clicks);
				break;
			}
			case 2:
			{
				switch (clicks)
				{
					case 1:
					{
						if (cpx)
						{
							char **items;
							_WORD obj;
							unsigned short disabled = 0;
							
							deselect_all_cpx_draw();
							cpx->selected = 1;
							redraw_cpximg(cpx);

							items = &fstring_addr[cpx_popup];

							if (cpx->flags & CPXD_INACTIVE)
							{
								disabled |= 1 << (CPX_POPUP_OPEN_EN - CPX_POPUP_OPEN_EN);
								disabled |= 1 << (CPX_POPUP_DISABLE_EN - CPX_POPUP_OPEN_EN);
							}
							else
							{
								disabled |= 1 << (CPX_POPUP_ENABLE_EN - CPX_POPUP_OPEN_EN);
							}
							
							obj = pop_handle(mx, my, items, CPX_POPUP_INFO_EN - CPX_POPUP_OPEN_EN + 1, -1, disabled);
							evnt_timer(0);
							switch (obj)
							{
							case CPX_POPUP_OPEN_EN - CPX_POPUP_OPEN_EN:
								open_cpx(cpx);
								break;
							case CPX_POPUP_DISABLE_EN - CPX_POPUP_OPEN_EN:
								deactivate_cpx(cpx);
								/* den Desktop ueber Veraenderungen informieren */
								update_cpx_path();
								break;
							case CPX_POPUP_ENABLE_EN - CPX_POPUP_OPEN_EN:
								activate_cpx(cpx);
								/* den Desktop ueber Veraenderungen informieren */
								update_cpx_path();
								break;
							case CPX_POPUP_INFO_EN - CPX_POPUP_OPEN_EN:
								cpx_info(cpx);
								break;
							}
						}
						else
						{
							char **items;
							_WORD obj;
							unsigned short disabled = 0;
							
							deselect_all_cpx_draw();

							items = &fstring_addr[cops_popup];
							
							if (main_window->wflags.iconified)
							{
								disabled |= 1 << (COPS_POPUP_TIDY_UP_EN - COPS_POPUP_ABOUT_EN);
								disabled |= 1 << (COPS_POPUP_RELOAD_EN - COPS_POPUP_ABOUT_EN);
								disabled |= 1 << (COPS_POPUP_SELECT_ALL_EN - COPS_POPUP_ABOUT_EN);
							}

							if (get_help_id() >= 0)
								disabled |= 1 << (COPS_POPUP_HELP_EN - COPS_POPUP_ABOUT_EN);
							
							obj = pop_handle(mx, my, items, COPS_POPUP_HELP_EN - COPS_POPUP_ABOUT_EN + 1, -1, disabled);
							evnt_timer(0);
							switch (obj)
							{
							case COPS_POPUP_ABOUT_EN - COPS_POPUP_ABOUT_EN:
								about();
								break;
							case COPS_POPUP_SETTINGS_EN - COPS_POPUP_ABOUT_EN:
								einstellungen();
								break;
							case COPS_POPUP_RELOAD_EN - COPS_POPUP_ABOUT_EN:
								if (form_alert(1, fstring_addr[reload_alert]) == 1)
									do_reload = 1;
								break;
							case COPS_POPUP_TIDY_UP_EN - COPS_POPUP_ABOUT_EN:
								tidy_up_icons();
								break;
							case COPS_POPUP_SELECT_ALL_EN - COPS_POPUP_ABOUT_EN:
								select_all_cpx();
								break;
							case COPS_POPUP_HELP_EN - COPS_POPUP_ABOUT_EN:
								call_help();
								break;
							}
						}
						break;
					}	
					case 2:
						break;
				}
				break;
			}
		}
	}
}

static short
handle_message(_WORD msg[8])
{
	switch (msg[0])
	{
		case WM_REDRAW:
			redraw_window(msg[3], (GRECT *)&msg[4]);
			break;
		case WM_TOPPED:
		case WM_NEWTOP:
			wind_set_int(msg[3], WF_TOP, 0);
			break;
		case WM_CLOSED:
		{
			if (main_window && (main_window->handle == msg[3]))
				/* Hauptfenster schliessen */
				close_main_window(WM_CLOSED);

			/* Accessory? */
			if (_app == 0)
				/* OS liefert kein AP_TERM */
				if (!(aes_flags & GAI_APTERM))
					save_inf();

			break;
		}
		case WM_FULLED:
		{
			GRECT bbox;
			
			get_cpx_bbox(&bbox);
			full_window(msg[3], bbox.g_x + bbox.g_w + 8, bbox.g_y + bbox.g_h + 8);
			break;
		}
		case WM_ARROWED:
			arr_window(msg[3], msg[4]);
			break;
		case WM_HSLID:
			hlsid_window(msg[3], msg[4]);
			break;
		case WM_VSLID:
			vslid_window(msg[3], msg[4]);
			break;
		case WM_SIZED:
			size_window(msg[3], (GRECT *)&msg[4]);
			break;
		case WM_MOVED:
			move_window(msg[3], (GRECT *)&msg[4]);
			break;
		case WM_ALLICONIFY:
		case WM_ICONIFY:
			iconify_window(msg[3], (GRECT *) &msg[4]);
			break;
		case WM_UNICONIFY:
			uniconify_window(msg[3], (GRECT *) &msg[4]);
			break;
		case AP_TERM:
			return 1; /* beenden */
		case AP_DRAGDROP:
			break;
		case AC_OPEN:
		{
			/* Hauptfenster oeffnen */
			if (msg[4] == menu_id)
				open_main_window();
			break;
		}
		case AC_CLOSE:
		{
			/* Accessory schliessen */

			/* Hauptfenster schliessen */
			close_main_window(AC_CLOSE);
			if (no_open_cpx > 0)
			{
				/* OS liefert kein AP_TERM */
				if (!(aes_flags & GAI_APTERM))
					save_inf();
			}
			break;
		}
		case VA_START:
		{
			short av_app;
			short redraw;

			/* CPX-Verzeichnis durchsuchen */
			redraw = update_cpx_list();

			/* Dateiname uebergeben? */
			if ((*(char **) &msg[3]) && (**(char **) &msg[3]))
			{
				char *args;
				short len;
	
				len = (short) strlen(*(char **)&msg[3]);
				args = malloc(len + 2);
				if (args)
				{
					args[0] = 0; /* ein Nullbyte am Anfang */
					strcpy(args + 1, *(char **)&msg[3]);

					do_args(args + 1);

					free(args);
				}

				/* Message zuruecksenden */
				msg[0] = AV_STARTED;
				av_app = msg[1];
				msg[1] = app_id;
				appl_write(av_app, 16, msg);
			}
			else
				/* Hauptfenster oeffnen */
				open_main_window();

			if (redraw)
				/* Icons anordnen und neuzeichnen */
				tidy_up_icons();

			break;
		}
	}
	return 0;
}

/* das Byte vor args muss ein beschreibbares Nullbyte sein */
static void
do_args(char *args)
{
	char *path;
	short len;

	len = strlen(args);
	path = args;

	while (len >= 0)
	{
		char b;

		b = *args++;

		if (b == ' ')
		{
			/* mehrere Leerzeichen hintereinander? */
			if (*(args - 2) == 0)
				path++;
			else
				b = 0;
		}
		else if (b == '\'' && (path == args - 1)) /* Hochkomma? */
		{
			path++;
			
			while ((b = *args++) != 0)
			{
				len--;

				/* Hochkomma? */
				if (b == '\'')
				{
					/* folgt darauf noch ein Hochkomma? */
					if (*args == '\'')
					{
						memmove(path + 1, path, args - path - 1); /* Bereich verschieben */
						path++;
						args++;
						len--;
					}
					else
					{
						b = 0;
						break;
					}
				}
			}
		}

		/* String-Ende gefunden? */
		if ((b == 0) && (path < (args - 1)))
		{
			CPX_DESC *cpx_desc;

			/* String terminieren */
			*(args - 1) = 0;

			cpx_desc = list_search(cpx_desc_list, (long) path, offsetof(CPX_DESC, next), search_cpx_name);
			if (cpx_desc)
				open_cpx(cpx_desc);

			/* Anfang des naechsten Strings */
			path = args;
		}
		len--;
	}
}

static void
set_termtime(void)
{
	termtime = time(NULL) + settings.after * 60;
}

/*----------------------------------------------------------------------------------------*/
/* oberster CPX zurueckliefern */
/* Funktionsergebnis:	Zeiger auf CPX_DESC */
/*----------------------------------------------------------------------------------------*/
static CPX_DESC *
top_cpx(void)
{
	CPX_DESC *cpx_desc;
	short top_window;
	
	/* oberstes Fenster */
	top_window = top_whdl();
	cpx_desc = cpx_desc_list;
	
	while (cpx_desc)
	{
		/* Fenster geoeffnet? */
		if (cpx_desc->whdl > 0)
		{
			/* CPX-Fenster gefunden? */
			if (cpx_desc->whdl == top_window)
				return cpx_desc;
		}
		cpx_desc = cpx_desc->next;
	}
	return NULL;
}

/*----------------------------------------------------------------------------------------*/
/* CPXe schliessen und anschliessend neuladen */
/* Funktionsergebnis:	- */
/*----------------------------------------------------------------------------------------*/
static void
close_all_cpx(short msg)
{
	CPX_DESC *cpx_desc;

	wind_update(BEG_UPDATE);

	/* alle offenen CPX-Dialoge schliessen */
	cpx_desc = cpx_desc_list;
	while (cpx_desc)
	{
		/* CPX geoeffnet? */
		if (cpx_desc->whdl > 0)
		{
			/* Event-CPX? */
			if (cpx_desc->is_evnt_cpx)
			{
				cpx_close(cpx_desc, 1); /* CPX benachrichtigen */
				cpx_close_window(cpx_desc, msg); /* Fenster schliessen */
				if (!(cpx_desc->old.header.flags & CPX_RESIDENT))
				{
					unload_cpx(cpx_desc->start_of_cpx);
					cpx_desc->start_of_cpx = NULL;
					cpx_desc->end_of_cpx = NULL;
				}
			}
			else
			{
				/* Form-CPX schliessen */
				cpx_desc->button = -1; /* signalisiert Message */
				cpx_desc->msg[0] = AC_CLOSE;
				wind_update(END_UPDATE);

				/* Kontext umschalten, in anderem Kontext wieder cpx_main_loop() aufrufen */
				switch_context(cpx_desc);
			}
		}

		cpx_desc = cpx_desc->next;
	}

	wind_update(END_UPDATE);
}


/*
 * like form_button, used for form-CPX when no window dialogs are available
 */
static int fm_button(OBJECT *tree, _WORD obj, _WORD clicks, _WORD *pobj, const GRECT *work)
{
	_WORD state;
	_WORD flags;
	_WORD parent;
	_WORD tobj;
	BOOLEAN cont;
	MRETS m;

	cont = TRUE;
	flags = tree[obj].ob_flags;
	state = tree[obj].ob_state;

	if (flags & OF_TOUCHEXIT)				/* don't wait for button up */
		cont = FALSE;

	/* handle selectable case */
	if ((flags & OF_SELECTABLE) && !(state & OS_DISABLED))
	{
		/* if it's a radio button */
		if ((flags & OF_RBUTTON) && obj > 0)
		{
			parent = obj;
			do
			{
				tobj = parent;
				parent = tree[tobj].ob_next;
			} while (tree[parent].ob_tail != tobj);

			for (tobj = tree[parent].ob_head; tobj != parent; tobj = tree[tobj].ob_next)
			{
				state = tree[tobj].ob_state;
				if ((tree[tobj].ob_flags & OF_RBUTTON) && ((state & OS_SELECTED) || tobj == obj))
				{
					if (tobj == obj)
						state |= OS_SELECTED;
					else
						state &= ~OS_SELECTED;
					objc_change_grect(tree, tobj, MAX_DEPTH, work, state, TRUE);
				}
			}
		} else
		{
			/* turn on new object */
			graf_watchbox(tree, obj, state ^ OS_SELECTED, state);
		}

		/* if not touchexit then wait for button up */
		if (cont && (flags & (OF_SELECTABLE | OF_EDITABLE)))
			evnt_button(1, 1, 0, &m.x, &m.y, &m.buttons, &m.kstate);
	}

	/* see if this selection gets us out */
	if ((tree[obj].ob_state & OS_SELECTED) && (flags & OF_EXIT))
		cont = FALSE;
	/* handle click on another editable field */
	if (cont && !(flags & OF_EDITABLE))
		obj = 0;
	/* handle touchexit case; if double click, then set high bit */
	if ((flags & OF_TOUCHEXIT) && clicks == 2)
		obj |= 0x8000;

	*pobj = obj;
	return cont;
}


static _WORD handle_wdlg_evnt(CPX_DESC *cpx_desc, EVNT *events)
{
	_WORD edit_obj = 0;

	if (cpx_desc->dialog)
	{
		_WORD ret;
		_WORD edit_obj2;
		_WORD dummy;

		if (events->mwhich & MU_BUTTON)
		{
			edit_obj = wdlg_get_edit(cpx_desc->dialog, &cpx_desc->cursor_idx);
			if (edit_obj)
				objc_wedit(cpx_desc->tree, edit_obj, 0, &cpx_desc->cursor_idx, ED_END, cpx_desc->whdl);
		}
		ret = wdlg_evnt(cpx_desc->dialog, events);
		if (ret && edit_obj)
		{
			edit_obj2 = wdlg_get_edit(cpx_desc->dialog, &dummy);
			if (edit_obj2)
			{
				if (edit_obj2 == edit_obj)
				{
					objc_wedit(cpx_desc->tree, edit_obj, 0, &cpx_desc->cursor_idx, ED_INIT, cpx_desc->whdl);
					edit_obj2 = 0;
				}
				edit_obj = edit_obj2;
			}
			if (edit_obj)
				wdlg_set_edit(cpx_desc->dialog, edit_obj);
		}
		return ret;
	}

	if ((events->mwhich & MU_MESAG) && events->msg[3] == cpx_desc->whdl)
	{
		switch (events->msg[0])
		{
		case WM_CLOSED:
		case AC_CLOSE:
			return 0;
		case WM_MOVED:
		case WM_SIZED:
			wind_set_grect(cpx_desc->whdl, WF_CURRXYWH, (GRECT *)&events->msg[4]);
			wind_get_grect(cpx_desc->whdl, WF_WORKXYWH, &cpx_desc->size);
			if (cpx_desc->tree)
			{
				cpx_desc->tree[ROOT].ob_x = cpx_desc->size.g_x;
				cpx_desc->tree[ROOT].ob_y = cpx_desc->size.g_y;
			}
			break;
		}
	}
	if ((events->mwhich & MU_BUTTON) &&
		cpx_desc->whdl == wind_find(events->mx, events->my) &&
		cpx_desc->tree)
	{
		_WORD obj = objc_find(cpx_desc->tree, ROOT, MAX_DEPTH, events->mx, events->my);
		if (obj >= 0)
		{
			edit_obj = cpx_desc->edit_obj;

			if (edit_obj && cpx_desc->cursor)
			{
				objc_edit(cpx_desc->tree, edit_obj, 0, &cpx_desc->cursor_idx, ED_END);
				cpx_desc->cursor = FALSE;
			}
			if (!fm_button(cpx_desc->tree, obj, events->mclicks, &obj, &cpx_desc->size))
			{
				/* return object number */
				cpx_desc->button = obj;
			} else if ((cpx_desc->tree[obj].ob_flags & OF_EDITABLE))
			{
				cpx_desc->edit_obj = obj;
			}
		}
	}
	return 1;
}

/* do_redraw()
 *==========================================================================
 * redraw object tree redraw for form-CPX
 */
static void do_redraw(CPX_DESC *cpx_desc, GRECT *prect)
{
	GRECT rect;

	if (cpx_desc->tree == NULL || cpx_desc->whdl <= 0)
		return;
	if (cpx_desc->cursor)
		objc_edit(cpx_desc->tree, cpx_desc->edit_obj, 0, &cpx_desc->cursor_idx, ED_END);
	wind_get_grect(cpx_desc->whdl, WF_FIRSTXYWH, &rect);
	while (rect.g_w > 0 && rect.g_h > 0)
	{
		if (rc_intersect(prect, &rect))
		{
			objc_draw_grect(cpx_desc->tree, ROOT, MAX_DEPTH, &rect);
		}
		wind_get_grect(cpx_desc->whdl, WF_NEXTXYWH, &rect);
	}
	if (cpx_desc->cursor)
		objc_edit(cpx_desc->tree, cpx_desc->edit_obj, 0, &cpx_desc->cursor_idx, ED_INIT);
}

/*----------------------------------------------------------------------------------------*/
/* CPX-Eventhauptschleife */
/* Funktionsergebnis:	Zeiger auf CPX_DESC fuer cpx_form_do() */
/*----------------------------------------------------------------------------------------*/
void
cpx_main_loop(void)
{
	EVNT events;

	while (quit == 0)
	{
		CPX_DESC *cpx_desc;

		/* sollen CPXe automatisch gestartet werden? */
		if (auto_start_list)
		{
			struct auto_start *tmp = auto_start_list;

			/* aus der Autostartliste entfernen */
			auto_start_list = tmp->next;
			cpx_desc = tmp->cpx_desc;

			/* Speicher fuer Struktur freigeben */
			free(tmp);

			/* neuen Kontext erzeugen und CPX aufrufen */
			new_context(cpx_desc);
		}

		/* CPXe neuladen? */
		if (do_reload)
		{
			close_all_cpx(WM_CLOSED);
			unload_all_cpx(); /* CPXe aus dem Speicher entfernen */
			update_cpx_list(); /* CPXe neu einlesen */
			tidy_up_icons(); /* Icons anordnen */
			do_reload = 0; /* Flag loeschen */
		}

		/* AC_CLOSE, alle CPXe und das Hauptfenster schliessen? */
		if (do_close)
		{
			close_all_cpx(AC_CLOSE); /* alle CPXe schliessen */
			close_main_window(AC_CLOSE);/* Hauptfenster schliessen */

			if (no_open_cpx > 0)
			{
				/* OS liefert kein AP_TERM */
				if (!(aes_flags & GAI_APTERM))
					save_inf();
			}
			wind_update(END_UPDATE); /* Taskwechsel unter TOS wieder freigeben */
			do_close = 0; /* Flag loeschen */
		}

		/* Applikation und alle Fenster geschlossen? */
		if (_app && main_window == NULL && no_open_cpx == 0)
		{
			quit = 1;
			break;
		}

		/* oberstes CPX */
		cpx_desc = top_cpx();

		/* liegt ein CPX-Fenster oben (wg. Event-CPX mit Mausrechteck oder Timer)? */
		if (cpx_desc)
		{
			if (!cpx_desc->is_evnt_cpx &&
				cpx_desc->dialog == NULL &&
				cpx_desc->edit_obj &&
				!cpx_desc->cursor)
			{
				objc_edit(cpx_desc->tree, cpx_desc->edit_obj, 0, &cpx_desc->cursor_idx, ED_INIT);
				cpx_desc->cursor = TRUE;
			}
			EVNT_multi(MU_KEYBD | MU_BUTTON | MU_MESAG | MU_TIMER | cpx_desc->mask,
				   258, 3, 0,
				   &cpx_desc->m1, &cpx_desc->m2, cpx_desc->time, &events);
		}
		else
		{
			/* kein CPX oben */
			EVNT_multi(MU_KEYBD | MU_BUTTON | MU_MESAG | MU_TIMER,
				   258, 3, 0,
				   NULL, NULL, 30000L, &events);
		}

		if ((events.mwhich & MU_MESAG) && events.msg[0] == AC_CLOSE)
		{
			do_close = 1;
			wind_update(BEG_UPDATE); /* Taskwechsel unter TOS blockieren */
			continue;
		}

		cpx_desc = cpx_desc_list;
		
		/* CPX-Liste durchlaufen */
		while (cpx_desc)
		{
			/* CPX geoeffnet? */
			if (cpx_desc->whdl > 0)
			{
				OBJECT	*dummy;
				
				if (cpx_desc->dialog)
					wdlg_get_tree(cpx_desc->dialog, &dummy, &cpx_desc->size);

				/* Event-CPX? */
				if (cpx_desc->is_evnt_cpx)
				{
					/* CPX verlassen? */
					if (handle_evnt_cpx(cpx_desc, &events) == 0)
					{
						cpx_close(cpx_desc, 1); /* CPX benachrichtigen */
						cpx_close_window(cpx_desc, WM_CLOSED); /* Fenster schliessen */

						if (!(cpx_desc->old.header.flags & CPX_RESIDENT))
						{
							unload_cpx(cpx_desc->start_of_cpx);
							cpx_desc->start_of_cpx = NULL;
							cpx_desc->end_of_cpx = NULL;
						}
					}
				}
				else
				{
					/* Form-CPX */
					short mwhich;

					/* fuer Nachricht ans CPX merken */
					mwhich = events.mwhich;

					/* weitermachen? */
					if (handle_wdlg_evnt(cpx_desc, &events))
					{
						/* wurde ein Button gedrueckt? */
						if (cpx_desc->button != -1)
						{
							/* CPX-Kontext aufrufen */
							switch_context(cpx_desc);
						}
						
						/* Tastendruck? */
						if (mwhich & MU_KEYBD)
						{
							/* oberstes Fenster? */
							if (cpx_desc->whdl == top_whdl())
							{
								short key;
								
								key = MapKey(events.kstate, events.key);
								
								if (cpx_desc->dialog == NULL && cpx_desc->tree)
								{
									DIRS direction = NODIR;

									switch (events.key & 0xff00)
									{
									case KbRETURN << 8:
									case KbNumENTER << 8:
										direction = DEFAULTDIR;
										break;

									case KbTAB << 8:
										direction = events.kstate & K_SHIFT ? BACKWARD : FORWARD;
										break;

									case KbUP << 8:
										direction = BACKWARD;
										break;

									case KbDOWN << 8:
										direction = FORWARD;
										break;
									}
						
									if (direction != NODIR)
									{
										_WORD next_obj;

										key = 0;
										next_obj = find_obj(cpx_desc->tree, cpx_desc->edit_obj, direction);
										if (direction == DEFAULTDIR && next_obj != 0)
										{
											objc_change_grect(cpx_desc->tree, next_obj, MAX_DEPTH, &cpx_desc->size, cpx_desc->tree[next_obj].ob_state | OS_SELECTED, TRUE);
											cpx_desc->button = next_obj;
											switch_context(cpx_desc);
										}
										if (next_obj != cpx_desc->edit_obj)
										{
											if (cpx_desc->cursor)
											{
												objc_edit(cpx_desc->tree, cpx_desc->edit_obj, 0, &cpx_desc->cursor_idx, ED_END);
											}
											cpx_desc->edit_obj = next_obj;
											cpx_desc->cursor = FALSE;
										}
									}
									if (key && cpx_desc->edit_obj)
									{
										objc_edit(cpx_desc->tree, cpx_desc->edit_obj, events.key, &cpx_desc->cursor_idx, ED_CHAR);
									}
								}
								
								/* ALT-Kombination oder nicht druckbaren Scancode auswerten? */
								if (key & (KbSCAN | KbALT))
								{
									cpx_desc->msg[0] = CT_KEY;
									cpx_desc->msg[1] = app_id;
									cpx_desc->msg[2] = 0;
									cpx_desc->msg[3] = events.key;
									cpx_desc->button = -1;
									/* CPX-Kontext aufrufen */
									switch_context(cpx_desc);
								}
							}
						}

						/* Message? */
						if (mwhich & MU_MESAG)
						{
							short i;

							/* Message kopieren */
							for (i = 0; i < 8; i++)
								cpx_desc->msg[i] = events.msg[i];

							/* signalisiert Message */
							cpx_desc->button = -1;

							switch (events.msg[0])
							{
							case AC_CLOSE:
								/* CPX-Kontext aufrufen */
								switch_context(cpx_desc);
								break;
							case WM_REDRAW:
								/* fuer dieses Fenster? */
								if (events.msg[3] == cpx_desc->whdl)
								{
									if (cpx_desc->dialog == NULL)
									{
										do_redraw(cpx_desc, (GRECT *)&events.msg[4]);
									}
									/* CPX-Kontext aufrufen */
									switch_context(cpx_desc);
								}
								break;
							}
						}
					}
					else
					{
						/* Dialog wurde geschlossen */
						cpx_desc->button = -1; /* CPX schliessen */
						cpx_desc->msg[0] = WM_CLOSED;
						switch_context(cpx_desc); /* CPX-Kontext aufrufen */
					}
				}
			}

			cpx_desc = cpx_desc->next;
		}

		if (events.mwhich & MU_MESAG)
			/* Message-Behandlung fuer das Hauptfenster */
			quit |= handle_message(events.msg);

		if (events.mwhich & MU_KEYBD)
		{
			if (main_window && (top_whdl() == main_window->handle))	/* liegt das Hauptfenster oben? */
			{
				unsigned short key;
			
				key = MapKey(events.kstate, events.key);
				/* Tastaturbehandlung fuer das Hauptfenster */
				quit |= handle_keyboard(key & 0xff00, key & 0x00ff);
			}
		}

		if (events.mwhich & MU_BUTTON)
			/* Buttonbehandlung fuer das Hauptfenster */
			handle_button(events.mx, events.my, events.mbutton & 3, events.kstate, events.mclicks);

		/* automatisch terminieren? */
		if (settings.term && time(NULL) > termtime)
		{
			/* Applikation oder MagiC als OS? */
			if (_app || aes_global[1] != 1)
				quit = 1;
		}

		/* CPX oeffnen? */
		if (g_open_cpx)
		{
			cpx_desc = g_open_cpx;
			g_open_cpx = NULL;

			/* neuen Kontext erzeugen und CPX aufrufen */
			new_context(cpx_desc);
		}
	}

	DEBUG((DEBUG_FMT ": leave with NULL\n", DEBUG_ARGS));
	a_call_return();

	/* never reached */
	__builtin_unreachable();
}

static short
MapKey(short keystate, short key)
{
	short scancode, ret;

	if (!kt)
		kt = Keytbl((void *)-1L, (void *)-1L, (void *)-1L);

	scancode = (key >> 8) & 0xff;

	if ((keystate & K_ALT) && scancode >= KbAlt1 && scancode <= KbAltAPOSTR)
		/* als 1 bis ' ummappen */
		scancode -= KbAlt1 - KbN1;

	/* CAPS gedrueckt? */
	if (keystate & K_CAPSLOCK)
	{
		ret = ((char *)kt->caps)[scancode];
	}
	else
	{
		/* Shift? */
		if (keystate & K_SHIFT)
		{
			if (scancode >= KbF11 && scancode <= KbF20)
				ret = ((char *)kt->shift)[scancode - (KbF11 - KbF1)];
			else
				ret = ((char *)kt->shift)[scancode];
		}
		else
			ret = ((char *)kt->unshift)[scancode];
	}

	/* Scancode auswerten? */
	if (ret == 0)
		ret = scancode | KbSCAN;
	else if (scancode == KbNumMINUS ||
		     scancode == KbNumPLUS ||
		     (scancode >= KbNumBrOPEN && scancode <= KbNumENTER))
		ret |= KbNUM;

	return ret | (keystate << 8);
}

/*----------------------------------------------------------------------------------------*/
/* Event-CPX behandeln */
/* Funktionsergebnis:	0: CPX schliessen 1: alles in Ordnung */
/*	cpx_desc:	CPX-Beschreibung */
/*	events:		Event-Beschreibung */
/*----------------------------------------------------------------------------------------*/
static short
handle_evnt_cpx(CPX_DESC *cpx_desc, EVNT *events)
{
	OBJECT *tree;
	short mwhich;
	
	/* angefallene Ereignisse merken */
	mwhich = events->mwhich;
	/* ggf. Dummydialog verschieben */
	handle_wdlg_evnt(cpx_desc, events);
	/* Dialogposition fuer Redraw etc. */
	if (cpx_desc->dialog)
		wdlg_get_tree(cpx_desc->dialog, &tree, &cpx_desc->size);
	else
		tree = cpx_desc->tree;

	if (top_whdl() != cpx_desc->whdl)
		mwhich &= ~(MU_KEYBD | MU_M1 | MU_M2 | MU_TIMER);

	/* Mausclicks? */
	if (mwhich & MU_BUTTON)
	{
		if (wind_find(events->mx, events->my) != cpx_desc->whdl)
			mwhich &= ~MU_BUTTON;
	}

	/* Mitteilungen des SCRENMGR? */
	if (mwhich & MU_MESAG)
	{
		if ((events->msg[0] >= WM_REDRAW) && (events->msg[0] <= WM_ALLICONIFY))
		{
			/* anderes Fenster? */
			if (events->msg[3] != cpx_desc->whdl)
				mwhich &= ~MU_MESAG;
		}
	}

	/* CPX verlassen? */
	if (cpx_hook(cpx_desc, mwhich, events->msg, (MRETS *) &events->mx, &events->key, &events->mclicks))
		return 0;

	/* Tastendruck? */
	if (mwhich & MU_KEYBD)
	{
		/* CPX verlassen? */
		if (cpx_key(cpx_desc, events->kstate, events->key))
			return 0;
	}

	/* Mausclicks? */
	if (mwhich & MU_BUTTON)
	{
		/* CPX verlassen? */
		if (cpx_button(cpx_desc, (MRETS *) &events->mx, events->mclicks))
			return 0;
	}

	/* Rechteck 1 betreten oder verlassen? */
	if (mwhich & MU_M1)
	{
		/* CPX verlassen? */
		if (cpx_m1(cpx_desc, (MRETS *) &events->mx))
			return 0;
	}

	/* Rechteck 2 betreten oder verlassen? */
	if (mwhich & MU_M2)
	{
		/* CPX verlassen? */
		if (cpx_m2(cpx_desc, (MRETS *) &events->mx))
			return 0;
	}

	/* Timer-Ereignis? */
	if (mwhich & MU_TIMER)
	{
		/* CPX verlassen? */
		if (cpx_timer(cpx_desc))
			return 0;
	}

	/* Mitteilungen des SCRENMGR? */
	if (mwhich & MU_MESAG)
	{
		switch (events->msg[0])
		{
			/* Redraw ans CPX weiterleiten */
			case WM_REDRAW:
			{
				cpx_draw(cpx_desc, (GRECT *) &events->msg[4]);
				break;
			}
			/* Fenster verschoben */
			case WM_MOVED:
			{
				cpx_wmove(cpx_desc, &cpx_desc->size);
				break;
			}
			/* Fenster schliessen */
			case WM_CLOSED:
			{
				/* CPX benachrichtigen */
				cpx_close(cpx_desc, 1);
				return 0;
			}
			case AC_CLOSE:
			{
				/* CPX benachrichtigen */
				cpx_close(cpx_desc, 0);
				return 0;
			}
		}
	}
	return 1;
}

/*----------------------------------------------------------------------------------------*/
/* Objektbaum von Zeichen- in Pixelkoordinaten umrechnen */
/*----------------------------------------------------------------------------------------*/
static void
fix_tree(OBJECT *obj)
{
	obj--;
	
	do {
		obj++;
		rsrc_obfix(obj, 0);
	}
	while ((obj->ob_flags & OF_LASTOB) == 0);
}

/*
 * replacement for getenv(), because some startup routines
 * do not setup the environ[] array when run as accessory
 * (e.g. the one from mintlib)
 */
static char *acc_getenv(const char *str)
{
	BASEPAGE *bp;
	char *env;
	size_t len;
	
#if defined(__PUREC__)
	bp = _BasPag;
#else
	bp = _base;
#endif
	env = bp->p_env;
	if (env == 0)
		return NULL;
	len = strlen(str);
	while (*env)
	{
		if (strncmp(env, str, len) == 0 && env[len] == '=')
			return env + len + 1;
		env += strlen(env) + 1;
	}
	return NULL;
}


/*----------------------------------------------------------------------------------------*/
/* Voreinstellungen setzen, Header der COPS.inf einlesen */
/* Funktionsresultat:	- */
/*----------------------------------------------------------------------------------------*/
static void
std_settings(void)
{
	_WORD hor_3d;
	_WORD ver_3d;
	char *env;
	long header_size;

	(void) Pdomain(1); /* verstehe lange Dateinamen */
	Psignal(SIGTERM, sig_handler);
	Psignal(SIGQUIT, sig_handler);

	aes_flags = get_aes_info(&aes_font, &aes_height, &hor_3d, &ver_3d);
	if (aes_flags & GAI_APTERM)
		shel_write(SHW_INFRECGN, 1, 0, NULL, NULL); /* Programm versteht AP_TERM */

	wind_get_grect(0, WF_WORKXYWH, &desk_grect);

	cpx_desc_list = NULL;
	auto_start_list = NULL;	/* CPXe nicht automatisch starten */
	do_reload = 0;		/* CPXe nicht neuladen */
	do_close = 0;		/* CPXe nicht schliessen */
	no_open_cpx = 0;	/* keine CPXe offen */
	quit = 0;		/* Programm nicht beenden */

	env = acc_getenv("HOME");	/* Environment suchen */
	if (env)
	{
		strcpy(home, env);
		dosify(home);
	}
	else
	{
		/* aktuellen Pfad benutzen */

		home[0] = Dgetdrv() + 'A';
		home[1] = ':';
		Dgetpath(home + 2, 0);
	}
	/* Backslash anhaengen */
	append_slash(home);

	help_buf = (char *)Mxalloc(sizeof("*:\\COPS.hyp"), MX_PREFTTRAM | MX_GLOBAL);
	if (help_buf == (char *)-32)
		help_buf = (char *)Malloc(sizeof("*:\\COPS.hyp"));
	if (help_buf)
	{
		strcpy(help_buf, "*:\\COPS.hyp");
	}

	strcpy(inf_name, home);
	strcat(inf_name, "COPS.inf");

	header_size = read_file(inf_name, &settings, 0, sizeof(struct alphaheader));
	if (header_size != sizeof(struct alphaheader))
	{
		/* war auch keine alte INF-Datei ohne CPX-Eintraege... */
		if (header_size != sizeof(struct old_alphaheader))
		{
			strcpy(inf_name, home);
			strcat(inf_name, "defaults\\COPS.inf");
			header_size =  read_file(inf_name, &settings, 0, sizeof(struct alphaheader));
			
			if (header_size != sizeof(struct alphaheader))
			{
				if (header_size != sizeof(struct old_alphaheader))
					settings.magic = 0;
			}
		}
	}

	/* 'COPS' -> 0x434f5053L */
	if ((settings.magic == 0x434f5053L) && (settings.version >= 7))
	{
		/* Fensterposition ausserhalb des sichtbaren Bereichs? */
		if (settings.mw.g_x < desk_grect.g_x)
			settings.mw.g_x = desk_grect.g_x;
		if (settings.mw.g_y < desk_grect.g_y)
			settings.mw.g_y = desk_grect.g_y;
		if (settings.mw.g_x >= (desk_grect.g_x + desk_grect.g_w))
			settings.mw.g_x = (desk_grect.g_x + (desk_grect.g_w >> 2));
		if (settings.mw.g_y >= (desk_grect.g_y + desk_grect.g_h))
			settings.mw.g_y = (desk_grect.g_y + (desk_grect.g_h >> 2));

		/* Fenster zu breit, so dass Fuller und Size nicht erreichbar? */
		if (settings.mw.g_w > desk_grect.g_w)
			settings.mw.g_w = desk_grect.g_w;
		if (settings.mw.g_h > desk_grect.g_h)
			settings.mw.g_h = desk_grect.g_h;

		if (settings.after < 1)
			settings.after = 1;
		else if (settings.after > 99)
			settings.after = 99;

		if (settings.version < 8)
		{
			/* alte Version */
			settings.version = 8;	/* Neue Eintraege setzen, damit beim */
			settings.sortmode = 1;	/* Sichern neues Format geschrieben wird */
		}

		append_slash(settings.cpx_path);
		set_termtime();
		return;
	}

	settings.magic = 0x434f5053L; /* 'COPS' */
	settings.version = ALPHAFILEVERSION;
	settings.count = 0;
	settings.mw.g_x = desk_grect.g_w >> 2;
	settings.mw.g_y = desk_grect.g_h >> 2;
	settings.mw.g_w = desk_grect.g_w >> 1;
	settings.mw.g_h = desk_grect.g_h >> 1;
	settings.booticon = 0;
	settings.clickact = 0;
	settings.sortmode = 1;
	settings.term = 0;
	settings.after = 2;
	strcpy(settings.cpx_path, "C:\\CPX\\");

	set_termtime();
}

/*----------------------------------------------------------------------------------------*/
/* Resource und dazugehoerige Strukturen initialisieren */
/*----------------------------------------------------------------------------------------*/
static void
init_rsrc(void)
{
	OBJECT	*obj;
	short	i;

	for (i = 0; i < NUM_TREE; i++)
		fix_tree(tree_addr[i]);

	if ((aes_flags & GAI_CICN) /* && (aes_flags & GAI_MAGIC) */
		&& aes_global[10] >= 4)
	{
		/* Farbicons, unter MagiC */

		obj = tree_addr[CICON_DIALOG] + COPS_CICON;
		obj->ob_spec.iconblk->ib_char = 0xf000; 	/* Zeichen nicht anzeigen */
		obj->ob_spec.iconblk->ib_wtext = 0;
		obj->ob_spec.iconblk->ib_htext = 0;

		obj = tree_addr[ABOUT_DIALOG] + ABOUT_ICON;
		obj->ob_type = G_CICON;
		obj->ob_spec.ciconblk = tree_addr[CICON_DIALOG][COPS_CICON].ob_spec.ciconblk;

		obj = tree_addr[ICONIFIED_DIALOG] + ICFDLG_ICON;
		obj->ob_type = G_CICON;
		obj->ob_spec.ciconblk = tree_addr[CICON_DIALOG][COPS_CICON].ob_spec.ciconblk;
	}
	else
	{
		/* keine Farbicons */

		obj = tree_addr[ICON_DIALOG] + COPS_ICON;
		obj->ob_spec.iconblk->ib_char = 0xf000; /* Zeichen nicht anzeigen */
		obj->ob_spec.iconblk->ib_wtext = 0;
		obj->ob_spec.iconblk->ib_htext = 0;

		obj = tree_addr[ABOUT_DIALOG] + ABOUT_ICON;
		obj->ob_type = G_ICON;
		obj->ob_spec.iconblk = tree_addr[ICON_DIALOG][COPS_ICON].ob_spec.iconblk;

		obj = tree_addr[ICONIFIED_DIALOG] + ICFDLG_ICON;
		obj->ob_type = G_ICON;
		obj->ob_spec.iconblk = tree_addr[ICON_DIALOG][COPS_ICON].ob_spec.iconblk;
	}

}


static void _cdecl
sig_handler(long sig)
{
	if ((sig == SIGTERM) || (sig == SIGQUIT))
	{
		quit = 1;
	}
}

static int
init_vwork(void)
{
	_WORD work_out[57];
	_WORD work_in[11];
	int i;
	
	for (i = 0; i < 10; i++)
		work_in[i] = 1;
	
	work_in[10] = 2;
	vdi_handle = aes_handle;
	v_opnvwk(work_in, &vdi_handle, work_out);
	
	return vdi_handle;
}


int
main(int argc, char *argv[])
{
	/* Accessory? */
	if (_app == 0)
		argc = 0;

	app_id = appl_init();
 
	/* alles in Ordnung, AES vorhanden? */
	if (app_id >= 0)
	{
		/* Accessory? */
		if (_app == 0)
		{
			/* Taskwechsel unter TOS blockieren */
			wind_update(BEG_UPDATE);
		}
		else
			/* als Applikation gestartet */
			graf_mouse(ARROW, NULL);
		
		aes_handle = graf_handle(&pwchar, &phchar, &pwbox, &phbox);

		if (init_vwork())
		{
			/* Voreinstellungen vornehmen */
			std_settings();
			xctrl_pb.Country_Code = select_language();
			init_rsrc();
			set_texts(xctrl_pb.Country_Code);
			if (_app == 0)
			{
				/* als Accessory registrieren */
				menu_id = menu_register(app_id, fstring_addr[menutitle_str]);
			}
			/* Checkboxen und šberschriften anpassen */
			substitute_objects(rsc_rs_object, NUM_OBS, aes_flags, NULL, NULL);

			/* Fensterdialoge vorhanden? */
			if (init_wlib(app_id))
			{
				/* Applikation oder ikonifiziertes Accessory? */
				if (_app || settings.booticon)
				{
					/* kein Kontrollfeld sondern das Hauptfenster oeffnen? */
					if (argc <= 1)
					{
						/* OS liefert AP_TERM */
						if ((aes_flags & GAI_APTERM))
							/* INF-Datei muss einmal eingelesen werden */
							must_read_inf = 1;

						/* Hauptfenster oeffnen */
						open_main_window();

						/* und ikonifizieren? */
						if (settings.booticon && main_window)
							iconify_window(main_window->handle, NULL);
					}
				}

				/* Fenster noch nicht offen und CPXe noch nicht gescannt? */
				if (main_window == NULL)
				{
					/* CPXe suchen, ggf. cpx_init() aufrufen */
					update_cpx_list();
					/* COPS.inf laden, CPXe im Hauptfenster positionieren */
					read_inf();
				}

				/* Kontrollfelder direkt starten? */
				while (argc > 1)
				{
					char *path;
					argc--;

					path = argv[argc];

					if (path)
					{
						CPX_DESC *cpx_desc;

						cpx_desc = list_search(cpx_desc_list,
								       (long)path,
								       offsetof(CPX_DESC, next), search_cpx_name);
						if (cpx_desc)
						{
							struct auto_start *auto_start;
							
							auto_start = malloc(sizeof(*auto_start));
							if (auto_start)
							{
								auto_start->next = NULL;
								auto_start->cpx_desc = cpx_desc;

								list_append((void **) &auto_start_list,
									    (void *)auto_start,
									    offsetof(struct auto_start, next));
							}
						}
					}
				}

				/* Accessory? */
				if (_app == 0)
					/* Taskwechsel unter TOS wieder zulassen */
					wind_update(END_UPDATE);

				/* Kontext sichern und Hauptschleife aufrufen */
				a_call_main();
				DEBUG((DEBUG_FMT ": a_call_main returned\n", DEBUG_ARGS));

				/* Hauptfenster schliessen */
				close_main_window(WM_CLOSED);
				/* Einstellungen sichern */
				save_inf();
				/* Speicher fuer CPXe freigeben */
				unload_all_cpx();

				/* Fensterstrukturen freigeben */
				reset_wlib();
			}

			/* Speicher fuer ersetzte Objekttypen freigeben */
			substitute_free();
			v_clsvwk(vdi_handle);
		}

		/* Accessory? */
		if (_app == 0)
		{
			/* Taskwechsel unter TOS wieder zulassen */
			wind_update(END_UPDATE);
			if (aes_global[1] == 1)
			{
				for (;;)
					evnt_mesag((_WORD *)home);
			}
		}

		appl_exit();
	}
	else
		(void) Cconws(fstring_addr[noaes_str]);

	return 0;
}


#if 0
/*
 * Dummy routines for debugging, to check that wdialog is not required
 */
DIALOG *wdlg_create(HNDL_OBJ handle_exit, OBJECT *tree, void *user_data, _WORD code, void *data, _WORD flags)
{
	(void)handle_exit;
	(void)tree;
	(void)user_data;
	(void)code;
	(void)data;
	(void)flags;
	return 0;
}

_WORD wdlg_close(DIALOG *dialog, _WORD *x, _WORD *y)
{
	(void)dialog;
	(void)x;
	(void)y;
	return 0;
}

_WORD wdlg_delete(DIALOG *dialog)
{
	(void)dialog;
	return 0;
}


_WORD wdlg_open(DIALOG *dialog, const char *title, _WORD kind, _WORD x, _WORD y, _WORD code, void *data)
{
	(void)dialog;
	(void)title;
	(void)kind;
	(void)x;
	(void)y;
	(void)code;
	(void)data;
	return 0;
}

_WORD wdlg_get_tree(DIALOG *dialog, OBJECT **tree, GRECT *r)
{
	(void)dialog;
	*tree = 0;
	(void)r;
	return 0;
}

_WORD wdlg_set_tree(DIALOG *dialog, OBJECT *new_tree)
{
	(void)dialog;
	(void)new_tree;
	return 0;
}

_WORD wdlg_set_edit(DIALOG *dialog, _WORD obj)
{
	(void)dialog;
	(void)obj;
	return 0;
}

_WORD wdlg_get_edit(DIALOG *dialog, _WORD *cursor)
{
	(void)dialog;
	(void)cursor;
	return 0;
}

_WORD wdlg_evnt(DIALOG *dialog, EVNT *events)
{
	(void)dialog;
	(void)events;
	return 0;
}
#endif
