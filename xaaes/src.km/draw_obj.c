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

#include "xa_types.h"
#include "xa_global.h"

#include "draw_obj.h"
#include "desktop.h"
#include "gradients.h"
#include "obtree.h"
#include "k_init.h"
#include "render_obj.h"
#include "trnfm.h"
#include "rectlist.h"
#include "c_window.h"
#include "widgets.h"
#include "menuwidg.h"
#include "scrlobjc.h"
#include "xa_user_things.h"
#include "mint/arch/asm.h"

/* call progdef-function via SIGUSR2
 * (not good because of possible VDI-calls inside signal-handler)
 */
#define PROGDEF_BY_SIGNAL	0

#include "mint/signal.h"

#define done(x) (*wt->state_mask &= ~(x))

static void d_g_progdef(struct widget_tree *wt, struct xa_vdi_settings *v);
static void d_g_slist(struct widget_tree *wt, struct xa_vdi_settings *v);


static struct widget_tree nil_wt;


#if GENERATE_DIAGS
static char *pstates[] =
{
	"SEL",
	"CROSS",
	"\10",
	"DIS",
	"OUTL",
	"SHA",
	"WBAK",
	"D3D",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"15"
};
#if 0
static char *pflags[] =
{
	"S",
	"DEF",
	"EXIT",
	"ED",
	"RBUT",
	"LAST",
	"TOUCH",
	"HID",
	">>",
	"INDCT",
	"BACKGR",
	"SUBM",
	"12",
	"13",
	"14",
	"15"
};
#endif
#if 0
static char *ob_types[] =
{
	"box",
	"text",
	"boxtext",
	"image",
	"progdef",
	"ibox",
	"button",
	"boxchar",
	"string",
	"ftext",
	"fboxtext",
	"icon",
	"title",
	"cicon",
	"Magic swbutton",	/* 34 */
	"Magic popup",	/* 35 */
	"Magic wintitle",	/* 36 */
	"Magic edit",		/* 37 */
	"Magic shortcut",	/* 38 */
	"xaaes slist",	/* 39 */
	"40"
};
#endif
#if 0
static char *
object_txt(OBJECT *tree, short t)			/* HR: I want to know the culprit in a glance */
{
    	static char nother[160];
	int ty = tree[t].ob_type;

	*nother = 0;

	switch (ty & 0xff)
	{
		case G_FTEXT:
		case G_TEXT:
		case G_BOXTEXT:
		case G_FBOXTEXT:
		{
			TEDINFO *ted = object_get_tedinfo(tree + t, NULL);
			sprintf(nother, sizeof(nother), " (%lx)'%s'", (long)ted->te_ptext, ted->te_ptext);
			break;
		}
		case G_BUTTON:
		case G_TITLE:
		case G_STRING:
		case G_SHORTCUT:
			sprintf(nother, sizeof(nother), " '%s'", object_get_spec(tree + t)->free_string);
			break;
	}
	return nother;
}

static char *
object_type(OBJECT *tree, short t)
{
	static char other[80];

	unsigned int ty = tree[t].ob_type;
	unsigned int tx;

	if (ty >= G_BOX && ty < G_BOX+sizeof(ob_types)/sizeof(*ob_types))
		return ob_types[ty-G_BOX];

	tx = ty & 0xff;
	if (tx >= G_BOX && tx < G_BOX+sizeof(ob_types)/sizeof(*ob_types))
		sprintf(other, sizeof(other), "ext: 0x%x + %s", ty >> 8, ob_types[tx-G_BOX]);
	else
		sprintf(other, sizeof(other), "unknown: 0x%x,%d", ty, ty);

	return other;
}
#endif
#else
#if 0
static char *ob_types[] =
{
	"box",
	"text",
	"boxtext",
	"image",
	"progdef",
	"ibox",
	"button",
	"boxchar",
	"string",
	"ftext",
	"fboxtext",
	"icon",
	"title",
	"cicon",
	"Magic swbutton",	/* 34 */
	"Magic popup",	/* 35 */
	"Magic wintitle",	/* 36 */
	"Magic edit",		/* 37 */
	"Magic shortcut",	/* 38 */
	"xaaes slist",	/* 39 */
	"40"
};
#endif
#endif


#if NAES3D
#define PW (default_options.naes ? 1 : 0)
#else
#define PW 0
#endif

long
init_client_objcrend(struct xa_client *client)
{
	long ret = E_OK;

	DIAGS(("init_client_objcrend: moduleapi=%lx, %s", client->objcr_module, client->name));
	if (!client->objcr_api)
	{
		ret = (*client->objcr_module->open)(&client->objcr_api);
		DIAGS((" -- open: ret %lx, objcr_api = %lx", ret, client->objcr_api));
		if (!ret && client->objcr_api)
		{
			client->objcr_api->drawers[G_PROGDEF] = d_g_progdef;
			client->objcr_api->drawers[G_SLIST] = d_g_slist;
		}
	}
	if (!ret && !client->objcr_theme)
	{
		ret = (*client->objcr_module->new_theme)(&client->objcr_theme);
		DIAGS((" -- theme: ret = %lx, theme = %lx", ret, client->objcr_theme));
	}
	return ret;
}

void
exit_client_objcrend(struct xa_client *client)
{
	DIAGS(("exit_client_objcrend: %s", client->name));
	if (client->objcr_theme)
	{
		(*client->objcr_module->free_theme)(client->objcr_theme);
		client->objcr_theme = NULL;
	}
	if (client->objcr_api)
	{
		(*client->objcr_module->close)(client->objcr_api);
		client->objcr_api = NULL;
	}
}

void
adjust_size(short d, RECT *r)
{
	r->x -= d;	/* positive value d means enlarge! :-)   as everywhere. */
	r->y -= d;
	r->w += d+d;
	r->h += d+d;
}
/* HR: 1 (good) set of routines for screen saving */

void
shadow_area(struct xa_vdi_settings *v, short d, short state, RECT *rp, short colour, short x_thick, short y_thick)
{
	RECT r;
	short offset, inc;

	/* Are we shadowing this object? (Borderless objects aren't shadowed!) */
	if ((x_thick || y_thick) && (state & OS_SHADOWED))
	{
		short i;

		if (x_thick)
		{
			r = *rp;
			offset	= x_thick > 0 ? x_thick : 0;
			inc	= -x_thick;

			r.y += offset;
			r.h -= offset;
			r.w += inc;

			for (i = x_thick < 0 ? -x_thick : x_thick; i > 0; i--)
			{
				r.w++;
				(*v->api->right_line)(v, d, &r, colour);
			}
		}
		if (y_thick)
		{
			r = *rp;
			offset	= y_thick > 0 ? y_thick : 0;
			inc	= -y_thick;

			r.x += offset;
			r.w -= offset;
			r.h += inc;

			for (i = y_thick < 0 ? -y_thick : y_thick; i > 0; i--) //(i = 0; i < abs(y_thick); i++)
			{
				r.h++;
				(*v->api->bottom_line)(v, d, &r, colour);
			}
		}
	}
}

#if 0
void
enable_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v)
{
	struct objc_edit_info *ei = wt->ei;

	if (!ei)
		return; //ei = &wt->e;

	if (edit_set(ei))
	{
		set_objcursor(wt, v, ei);
		ei->c_state |= OB_CURS_ENABLED;
	}
}

void
disable_objcursor(struct widget_tree *wt, struct xa_vdi_settings *v, struct xa_rect_list *rl)
{
	struct objc_edit_info *ei = wt->ei;
	if (!ei)
		return; //ei = &wt->e;
	undraw_objcursor(wt, v, rl);
// 	ei->c_state &= ~OB_CURS_ENABLED;
}
#endif


#if !PROGDEF_BY_SIGNAL

static inline long
do_callout ( void *f, PARMBLK *p)
{
	register long ret __asm__("d0");
	__asm__ volatile (
		PUSH_SP("d3-d7/a3-a6", 36)
		"move.l %2,-(sp)\n\t"
		"move.l %1,a0\n\t"
		"jsr	(a0)\n\t"
		"lea	4(sp),sp\n\t"
		POP_SP("d3-d7/a3-a6", 36)
			: "=r"(ret) 				/* outputs */
			: "g"(f),"g"(p)
			: __CLOBBER_RETURN("d0")
			"d1", "d2", "a0", "a1", "a2", 	/* clobbered regs */
			"memory"
	);
	return ret;
}

#define CHECK_PROGDEF_ADDR 0

typedef short __CDECL (*p_handler)(PARMBLK *pb);
#endif

/* HR: implement wt->x,y in all ObjectDisplay functions */
/* HR 290101: OBSPEC union & BFOBSPEC structure now fully implemented
 *            in xa_aes.h.
 *            Accordingly replaced ALL nasty casting & assemblyish approaches
 *            by simple straightforward C programming.
 */

/*
 * Draw a box (respecting 3d flags)
 */

#define userblk(ut) (*(USERBLK **)(ut->userblk_pp))
#define ret(ut)     (     (long *)(ut->ret_p     ))
#define parmblk(ut) (  (PARMBLK *)(ut->parmblk_p ))
static void
d_g_progdef(struct widget_tree *wt, struct xa_vdi_settings *v)
{
#if PROGDEF_BY_SIGNAL
	struct sigaction oact, act;
#endif
	struct xa_client *client = lookup_extension(NULL, XAAES_MAGIC);
	OBJECT *ob = wt->current.ob;
	PARMBLK *p;
	//short r[4];
	RECT save_clip;
#if GENERATE_DIAGS
	struct proc *curproc = get_curproc();

	DIAGS(("XaAES d_g_progdef: curproc - pid %i, name %s", curproc->pid, curproc->name));
	DIAGS(("XaAES d_g_progdef: client  - pid %i, name %s", client->p->pid, client->name));
#endif


//	KERNEL_DEBUG("ut = 0x%lx", ut);
//	KERNEL_DEBUG("ut->progdef_p = 0x%lx", ut->progdef_p);
//	KERNEL_DEBUG("ut->userblk_p = 0x%lx", ut->userblk_p);
//	KERNEL_DEBUG("ut->ret_p     = 0x%lx", ut->ret_p    );
//	KERNEL_DEBUG("ut->parmblk_p = 0x%lx", ut->parmblk_p);


	if ((client->status & CS_EXITING))
		return;

#if WITH_BKG_IMG || WITH_GRADIENTS
	if( !memcmp( &wt->r, &root_window->wa, sizeof(RECT)) && wt == get_desktop() )
	{

#if WITH_BKG_IMG
		if( !do_bkg_img(client, 0, 0) )
			return;
#endif
#if WITH_GRADIENTS
		if( is_gradient_installed( BOX_GRADIENT2 ) && wt->objcr_api->drawers[G_BOX] )
		{
			wt->objcr_api->drawers[G_BOX]( wt, v );
			return;
		}
#endif
	}
#endif
	p = parmblk(client->ut);
	p->pb_tree = wt->current.tree;
	p->pb_obj = wt->current.item;

	p->pb_prevstate = p->pb_currstate = ob->ob_state;

	*(RECT *)&(p->pb_x) = wt->r;

	*(RECT *)&(p->pb_xc) = save_clip = v->clip; //*clip;
	userblk(client->ut) = object_get_spec(ob)->userblk;
	p->pb_parm = userblk(client->ut)->ub_parm;

	(*v->api->wr_mode)(v, MD_TRANS);

#if GENERATE_DIAGS
	{
		char statestr[128];

		show_bits(*wt->state_mask & ob->ob_state, "state ", pstates, statestr);
		DIAG((D_o, wt->owner, "progdef before %s %04x", statestr, *wt->state_mask & ob->ob_state));
	}
#endif

#if !PROGDEF_BY_SIGNAL
	{
#if CHECK_PROGDEF_ADDR
		BASEPAGE *base = client->p->p_mem->base;
#endif
		p_handler pfunc;
		long pret;

		pfunc = (p_handler)userblk(client->ut)->ub_code;
#if CHECK_PROGDEF_ADDR
		/* check if callback-address is inside client-text */
		if( !(client->p->modeflags & M_SINGLE_TASK)
			&& ((long)pfunc < base->p_tbase || (long)pfunc >= base->p_tbase+base->p_tlen) )
		{
			BLOG((0,"PROGDEF: %s: user-func(%lx) outside TEXT:%lx-%lx", client->name, pfunc, base->p_tbase, base->p_tbase + base->p_tbase+base->p_tlen));
			return;
		}
#endif

		pret = do_callout(pfunc,p);

		*wt->state_mask = pret;
#if 0
		else
		{
			BLOG(("d_g_progdef:%s(%s):invalid state_mask-pointer:%lx, \
user-func:%lx TEXT:%lx-%lx (killed)", client->name, get_curproc()->name, wt->state_mask, pfunc, base->p_tbase, base->p_tbase + base->p_tlen));
			client->status |= (CS_EXITING | CS_SIGKILLED);
			raise(SIGKILL);
			yield();
			return;
		}
#endif
	}
#else
	act.sa_handler = client->ut->progdef_p;
	act.sa_mask = 0xffffffff;
	act.sa_flags = SA_RESETHAND;

	/* set new signal handler; remember old */
	p_sigaction(SIGUSR2, &act, &oact);

	DIAGS(("raise(SIGUSR2)"));
	raise(SIGUSR2);
	DIAGS(("handled SIGUSR2 progdef callout"));

	/* restore old handler */
	p_sigaction(SIGUSR2, &oact, NULL);

	/* The PROGDEF function returns the ob_state bits that
	 * remain to be handled by the AES:
	 */
	*wt->state_mask = *ret(client->ut);
#endif

	/* BUG: OS_SELECTED bit only handled in non-color mode!!!
	 * (Not too serious I believe... <mk>)
	 * HR: Yes I would call that correct, cause in color mode
	 *     selected appearance is object specific.
	 *     I even think that it shouldnt be done at ALL.
	 *
	 * The whole state_mask returning mechanism of progdefs should
         * be taken into account.
         * So we actually dont need to do anything special here.
         * But for progdef'd menu separators it is nicer to check here
         * and use bg_col.
	 */

#if GENERATE_DIAGS
	{
		char statestr[128];

		show_bits(*wt->state_mask, "state ", pstates, statestr);
		DIAG((D_o, wt->owner, "progdef remaining %s %04x", statestr, *wt->state_mask));
	}
#endif
	/*
	 * Ozk: Since it is possible that taskswitches happens during the callout of
	 *	progdef's, we need to restore the clip-rect used by this 'thread'
	 */

// 	*(RECT *)&r = v->clip; //*clip;
// 	r[2] += (r[0] - 1);
// 	r[3] += (r[1] - 1);
// 	vs_clip(v->handle, 1, (short *)&r);
	(*v->api->set_clip)(v, &save_clip);

	if (*wt->state_mask & OS_DISABLED)
	{
		(*v->api->write_disable)(v, &wt->r, objc_rend.dial_colours.bg_col);
		done(OS_DISABLED);
	}
	(*v->api->wr_mode)(v, MD_REPLACE);
}
#undef userblk
#undef ret
#undef parmblk

static void
d_g_slist(struct widget_tree *wt, struct xa_vdi_settings *v)
{
	RECT r = wt->r;
	SCROLL_INFO *list;
	struct xa_window *w;
	OBJECT *ob = wt->current.ob;

	list = (SCROLL_INFO*)object_get_spec(ob)->index;
	if (list)
	{
		w = list->wi;

		w->r.x = w->rc.x = r.x;
		w->r.y = w->rc.y = r.y;

		/* for after moving */
		calc_work_area(w);

		(*v->api->t_color)(v, G_BLACK);

		get_widget(w, XAW_TITLE)->stuff = list->title;
		r = v->clip;
		draw_window(list->lock, w, &r);
		draw_slist(0, list, NULL, &r);
	}
	done(OS_SELECTED);
}


// static ObjectDisplay *objc_jump_table[G_UNKNOWN];

/*
 * Initialise the object display jump table
 */
void
init_objects(void)
{
	clear_edit(&nil_wt.e);
	clear_focus(&nil_wt);
#if 0
	short f;

	clear_edit(&nil_tree.e);
	clear_focus(&nil_tree);

	for (f = 0; f <= G_UNKNOWN; f++)
		/* Anything with a NULL pointer won't get called */
		objc_jump_table[f] = NULL;

	objc_jump_table[G_BOX     ] = d_g_box;
	objc_jump_table[G_TEXT    ] = d_g_text;
	objc_jump_table[G_BOXTEXT ] = d_g_boxtext;
	objc_jump_table[G_IMAGE   ] = d_g_image;
	objc_jump_table[G_PROGDEF ] = d_g_progdef;
	objc_jump_table[G_IBOX    ] = d_g_ibox;
	objc_jump_table[G_BUTTON  ] = d_g_button;
	objc_jump_table[G_BOXCHAR ] = d_g_boxchar;
	objc_jump_table[G_STRING  ] = d_g_string;
	objc_jump_table[G_FTEXT   ] = d_g_ftext;
	objc_jump_table[G_FBOXTEXT] = d_g_fboxtext;
	objc_jump_table[G_ICON    ] = d_g_icon;
	objc_jump_table[G_TITLE   ] = d_g_title;
	objc_jump_table[G_CICON   ] = d_g_cicon;

// 	objc_jump_table[G_SWBUTTON] = d_g_swbutton;
	objc_jump_table[G_POPUP   ] = d_g_button;
// 	objc_jump_table[G_WINTITLE] = d_g_wintitle;
// 	objc_jump_table[G_EDIT    ] = d_g_edit;

	objc_jump_table[G_SHORTCUT] = d_g_string;
	objc_jump_table[G_SLIST   ] = d_g_slist;
	objc_jump_table[G_EXTBOX  ] = d_g_box;
#endif
}

/* object types */
#define G_BOX			20
#define G_TEXT			21
#define G_BOXTEXT		22
#define G_IMAGE			23
#define G_USERDEF		24
#define G_PROGDEF		G_USERDEF
#define G_IBOX			25
#define G_BUTTON		26
#define G_BOXCHAR		27
#define G_STRING		28
#define G_FTEXT			29
#define G_FBOXTEXT		30
#define G_ICON			31
#define G_TITLE			32
#define G_CICON			33

/* extended object types, MagiC only */
#define G_SWBUTTON		34
#define G_POPUP			35
#define G_WINTITLE		36
#define G_EDIT			37
#define G_SHORTCUT		38

/*
 * display or remove object-cursor
 * sr: coords of object
 * md & 1 1: show, 0: remove
 * md & 2 1: toolbar, 0: other
 */
static void do_object_cursor( struct xa_vdi_settings *v, RECT *sr, short md)
{
#if 0
		union { BFOBSPEC c; unsigned long l; } conv;
		struct extbox_parms *p = (struct extbox_parms *)(*api->object_get_spec)(ob)->index;
		short ty = ob->ob_type;

// 		c = *(BFOBSPEC *)&p->obspec;
		conv.l = p->obspec;
		c = conv.c;
#endif
	/* this should in fact be the parent-color */
	short color = 0;
	//short color = md ? G_RED : (screen.planes > 1 ? G_LWHITE : G_WHITE);
	switch( md )
	{
	case 0:
		color = screen.planes > 1 ? G_LWHITE : G_WHITE;
	break;
	case 1:
		color = G_RED;
	break;
	case 2:
		return;
		//color = G_BLACK;
	break;
	case 3:
		return;
		//color = G_LWHITE;
	break;
	}
	(*v->api->wr_mode)(v, MD_TRANS);	//MD_REPLACE);
	(*v->api->l_color)(v, color);
	(*v->api->l_width)(v, 1);
	if( md & 1 )
	{
		(*v->api->l_type)(v, USERLINE);
		(*v->api->l_udsty)(v, 0xaaaa);
	}
	else
	{
		(*v->api->l_type)(v, SOLID);
	}

	(*v->api->gbox)(v, 0, sr);
	(*v->api->l_type)(v, SOLID);

}

/*
 * Display a primitive object
 */
void
display_object(enum locks lock, XA_TREE *wt, struct xa_vdi_settings *v, struct xa_aes_object item, short parent_x, short parent_y, short flags)
{
	RECT r, o, sr;
	OBJECT *ob = aesobj_ob(&item);
	DrawObject *drawer = NULL;

	/* HR: state_mask is for G_PROGDEF originally.
	 * But it means that other objects must unflag what they
	 * can do themselves in the same manner.
	 * The best thing (no confusion) is to generalize the concept.
	 * Which I did. :-)
	 */
	unsigned short state_mask = (OS_SELECTED|OS_CROSSED|OS_CHECKED|OS_DISABLED|OS_OUTLINED);
	unsigned short t = ob->ob_type & 0xff;


	(*wt->objcr_api->obj_offsets)(wt, ob, &o);

	r.x = parent_x + ob->ob_x;
	r.y = parent_y + ob->ob_y;
	r.w = ob->ob_width;
	r.h = ob->ob_height;
	sr = r;

	o.x = r.x + o.x;
	o.y = r.y + o.y;
	o.w = r.w - o.w;
	o.h = r.h - o.h;

	if (   o.x		> (v->clip.x + v->clip.w - 1)
	    || o.x + o.w - 1	< v->clip.x
	    || o.y		> (v->clip.y + v->clip.h - 1)
	    || o.y + o.h - 1	< v->clip.y)
	{
		return;
	}

	/* Get display routine for this type of object from jump table */
	if (t <= G_UNKNOWN)
		drawer = wt->objcr_api->drawers[t];

//	if (t >= G_SWBUTTON && t <= G_SHORTCUT)
//		display("display %s(%d) for %s", ob_types[t], t, wt->owner->name);

	if (!drawer)
	{
		DIAG((D_objc,wt->owner,"no display_routine! ob_type: %d(0x%x)", t, ob->ob_type));
		/* dont attempt doing what could be indeterminate!!! */
		return;
	}

	if( cfg.menu_bar != 2 && !cfg.menu_ontop && cfg.menu_bar && wt != get_menu() && v->clip.y < get_menu_height()-2 )
	{
		if( cfg.menu_layout == 0 )
		{
			short d = get_menu_height()-2 - v->clip.y;
			if( v->clip.h <= d )
				return;
			v->clip.y = get_menu_height()-2;
			v->clip.h -= d;
			(*v->api->set_clip)(v, &v->clip);
		}
		else
		{
			C.rdm = 1;
		}
	}


	/* Fill in the object display parameter structure */
	wt->current = item;
	wt->r_parent.x = parent_x;
	wt->r_parent.y = parent_y;
// 	wt->parent_x = parent_x;
// 	wt->parent_y = parent_y;
	/* absolute RECT, ready for use everywhere. */
	wt->r = r;
	wt->state_mask = &state_mask;

	/* Better do this before AND after (fail safe) */
	(*v->api->wr_mode)(v, MD_TRANS);

#if 0
#if GENERATE_DIAGS
	if (wt->tree != xobj_rsc) //get_widgets())
	{
		char flagstr[128];
		char statestr[128];

		show_bits(ob->ob_flags, "flg=", pflags, flagstr);
		show_bits(ob->ob_state, "st=", pstates, statestr);

		DIAG((D_o, wt->owner, "ob=%d, %d/%d,%d/%d [%d: 0x%lx]; %s%s (%x)%s (%x)%s",
			aesobj_item(&item),
			 r.x, r.y, r.w, r.h,
			 t, display_routine,
			 object_type(aesobj_tree(&item), aesobj_item(&item)),
			 object_txt(aesobj_tree(&item), aesobj_item(&item)),
			 ob->ob_flags,flagstr,
			 ob->ob_state,statestr));
	}
#endif
#endif

	if( !(flags & UNDRAW_FOCUS) && screen.planes > 1 )	/* else on TT/ST-high checked is not visible if selected */
		(*v->api->wr_mode)(v, MD_TRANS);
	else
	{
		if (flags & UNDRAW_FOCUS)
			do_object_cursor(v, &sr, (wt->wind->dial & created_for_TOOLBAR) ? 2 : 0);
		else
			(*v->api->wr_mode)(v, MD_REPLACE);
	}

	/* Call the appropriate display routine */
	if( !(flags & DRAW_FOCUS) )
		(*drawer)(wt, v);

	if (t != G_PROGDEF)
	{
		/* Handle CHECKED object state: */
		if ((ob->ob_state & state_mask) & OS_CHECKED)
		{
			(*v->api->t_color)(v, G_BLACK);
			/* ASCII 8 = checkmark */
			v_gtext(v->handle, r.x + 2, r.y, "\10");
		}

		/* Handle DISABLED state: */
		/* already handled by renderer */
		//if ((ob->ob_state & state_mask) & OS_DISABLED)
			//(*v->api->write_disable)(v, &r, G_WHITE);

		/* Handle CROSSED object state: */
		if ((ob->ob_state & state_mask) & OS_CROSSED)
		{
			short p[4];
			(*v->api->l_color)(v, G_BLACK);
			p[0] = r.x;
			p[1] = r.y;
			p[2] = r.x + r.w - 1;
			p[3] = r.y + r.h - 1;
			v_pline(v->handle, 2, p);
			p[0] = r.x + r.w - 1;
			p[2] = r.x;
			v_pline(v->handle, 2, p);
		}

		/* Handle OUTLINED object state: */
		if ((ob->ob_state & state_mask) & OS_OUTLINED)
		{
			/* special handling of root object. */
			if (!wt->zen || aesobj_item(&item) != 0)
			{
				if (!MONO && obj_is_3d(ob))
				{
					(*v->api->tl_hook)(v, 1, &r, objc_rend.dial_colours.lit_col);
					(*v->api->br_hook)(v, 1, &r, objc_rend.dial_colours.shadow_col);
					(*v->api->tl_hook)(v, 2, &r, objc_rend.dial_colours.lit_col);
					(*v->api->br_hook)(v, 2, &r, objc_rend.dial_colours.shadow_col);
					(*v->api->gbox)(v, 3, &r);
				}
				else
				{
					(*v->api->l_color)(v, G_WHITE);
					(*v->api->gbox)(v, 1, &r);
					(*v->api->gbox)(v, 2, &r);
					(*v->api->l_color)(v, G_BLACK);
					(*v->api->gbox)(v, 3, &r);
				}
			}
		}

		if ( (ob->ob_state & state_mask) & OS_SELECTED)
		{
			(*wt->objcr_api->write_selection)(v, 0, &r);
		}
	}

	if ( focus_ob(wt) == ob && !(flags & UNDRAW_FOCUS) )	//(flags & DRAW_FOCUS) )
	{
		if (wt->e.c_state || wt->wind )		/* wt->wind is for wdialogs */
		{
			enum window_type dial = created_for_CLIENT;
			if( wt->wind )
				dial = wt->wind->dial;
			do_object_cursor(v, &sr, (dial & created_for_TOOLBAR) ? 3 : 1);
		}
	}

	(*v->api->wr_mode)(v, MD_TRANS);
}

/*
 * Walk an object tree, calling display for each object
 * HR: is_menu is true if a menu.
 */

/* draw_object_tree */
short
draw_object_tree(
enum locks lock,
XA_TREE *wt,
OBJECT *tree,
struct xa_vdi_settings *v,
struct xa_aes_object item,
short depth,
short *xy,
short flags)
{
	XA_TREE this;
	short /*current = 0, stop = -1, */rel_depth = 1, head;
	short x, y, dw = 0, dh = 0;
	struct objc_edit_info *ei = 0;
	bool start_drawing = false;
	bool curson = false;
	bool resized = false;
	bool docurs = ((flags & DRW_CURSOR));
	struct oblink_spec *oblink = NULL;
	struct xa_aes_object current, stop, *c;
	struct scroll_info *list = 0;
	struct xa_window *lwind = 0;


	if (wt == NULL)
	{
		wt = &this;
		clear_edit(&wt->e);
		this = nil_wt; //nil_tree;
		wt->ei = NULL;
		wt->owner = C.Aes;
		wt->objcr_api = C.Aes->objcr_api;
		wt->objcr_theme = C.Aes->objcr_theme;
	}
	else
		ei = wt->ei;

	if (tree == NULL)
		tree = wt->tree;
	else
		wt->tree = tree;

	if (!wt->owner)
		wt->owner = C.Aes;

	if (!wt->objcr_api || !wt->objcr_theme)
		return true;

	c = &current;
	*c = aesobj(wt->tree, 0);
	stop = inv_aesobj();

	/* set the font-size for the app */
	if( wt->owner->options.standard_font_point )
		x = wt->owner->options.standard_font_point;
	else
		x = cfg.standard_font_point;
	screen.standard_font_point = x;

	if( wt->wind && (wt->wind->window_status & XAWS_RESIZED ) ){
		set_toolbar_coords(wt->wind, NULL);

		/* used to move/resize objects inside the window */
		dw = wt->wind->r.w - wt->wind->min.x;
		dh = wt->wind->r.h - wt->wind->min.y;
		resized = true;
		wt->wind->window_status &= ~XAWS_RESIZED;
		wt->dx = wt->dy = 0;
	}
	/* dx,dy are provided by sliders if present. */
	x = -wt->dx;
	y = -wt->dy;

	DIAG((D_objc, wt->owner, "dx = %d, dy = %d", x, y));
	DIAG((D_objc, wt->owner, "draw_object_tree for %s to %d/%d,%d/%d; %lx + %d depth:%d",
		t_owner(wt), x + tree->ob_x, y + tree->ob_y,
		tree->ob_width, tree->ob_height, tree, item.item, depth));
	DIAG((D_objc, wt->owner, "  -   (%d)%s%s",
		wt->is_menu, obtree_is_menu(tree) ? "menu" : "object", wt->zen ? " with zen" : " no zen"));

// 	DIAG((D_objc, wt->owner, "  -   clip: %d.%d/%d.%d    %d/%d,%d/%d",
// 		cl[0], cl[1], cl[2], cl[3], cl[0], cl[1], cl[2] - cl[0] + 1, cl[3] - cl[1] + 1));

	depth++;

	if (ei)
	{
		if (ei->c_state & OB_CURS_ENABLED)
		{
			(*wt->objcr_api->undraw_cursor)(wt, v, NULL, 1);
			curson = true;
		}
// 		curson = ((ei->c_state & (OB_CURS_ENABLED | OB_CURS_DRAWN)) == (OB_CURS_ENABLED | OB_CURS_DRAWN)) ? true : false;
// 		if (curson) undraw_objcursor(wt, v, NULL);
	}
// 	else
// 		curson = false;

	do {

		if( resized == true ){
			switch( c->ob->ob_type ){
			case G_SLIST:
			{

				c->ob->ob_width += dw;
				c->ob->ob_height += dh;

				list =
					object_get_slist(((XA_TREE *)get_widget(wt->wind, XAW_TOOLBAR)->stuff)->tree + c->item);

				if( list ){
					lwind = list->wi;

					/* resize the list-window */
					if( lwind && lwind->send_message )
					{
						short amq = AMQ_REDRAW;
						RECT dr = v->clip;	/* send_message may change clipping */

						lwind->send_message(lock, lwind, NULL, amq, QMF_CHKDUP,
							WM_SIZED, 0,0, lwind->handle,
							lwind->r.x, lwind->r.y, lwind->r.w + dw, lwind->r.h + dh);
						(*v->api->set_clip)(v, &dr);	/* restore clip */
					}
				}
			}
			break;
			case G_CICON:
			case G_BUTTON:
			//case G_FBOXTEXT:
			case G_BOXTEXT:
			case G_STRING:

				/* relocate: move objects on resizing
				 * origin (upper left) of the window is set in set_and_update
				 */
				if( dh || dw )
				{
					if( (c->ob->ob_y + c->ob->ob_height) > (wt->wind->min.h) / 2 )
					{
						/* keep distance from lower border constant */
						c->ob->ob_y += dh;
					}

					{
						if( c->ob->ob_y < wt->wind->min.h / 2 && c->ob->ob_height > wt->wind->min.h / 2 )
							/* resize height for high objects */
							c->ob->ob_height += dh;
						if( c->ob->ob_x < wt->wind->min.w / 2 && c->ob->ob_width > wt->wind->min.w / 2 )
							/* resize width for wide objects */
							c->ob->ob_width += dw;
						else
						{
							short sw = wt->wind->sw;
							if( sw == 0 )
								sw = 2;
							if( c->ob->ob_x + dw > wt->wind->r.w / sw && c->ob->ob_x > wt->wind->min.w / sw )
							{
								/* keep distance from right border constant */
								c->ob->ob_x += dw;
								if( c->ob->ob_x + c->ob->ob_width > wt->wind->r.w )
									c->ob->ob_x = wt->wind->r.w - c->ob->ob_width - 1;
							}
						}
					}
				}
			break;
			}
		}

	//	DIAG((D_objc, NULL, " ~~~ obj=%d(%d/%d), flags=%x, state=%x, head=%d, tail=%d, next=%d, depth=%d, draw=%s",
	//		current, x, y, tree[current].ob_flags, tree[current].ob_state,
	//		tree[current].ob_head, tree[current].ob_tail, tree[current].ob_next,
	//		rel_depth, start_drawing ? "yes":"no"));
uplink:
		if (same_aesobj(&item, c))
		{
			stop = item;
			start_drawing = true;
			rel_depth = 0;
		}

		if (!aesobj_hidden(c))
		{
			if (set_aesobj_uplink(&tree, c, &stop, &oblink))
				goto uplink;

			if (start_drawing)
			{
				/* Display this object */
				display_object(lock, wt, v, *c, x, y, flags);
				if (!ei && docurs && same_aesobj(c, &wt->e.o))
				{
					if ((aesobj_type(c) & 0xff) != G_USERDEF)
					{
						(*wt->objcr_api->eor_cursor)(wt, v, NULL);
					}
					docurs = false;
				}
			}
		}

		head = aesobj_head(c);

		/* Any non-hidden children? */
		if (    head >= 0
		    && !aesobj_hidden(c)
		    &&  (!start_drawing || (start_drawing && rel_depth < depth)))
		{
			x += aesobj_getx(c);
			y += aesobj_gety(c);

			rel_depth++;

			*c = aesobj(aesobj_tree(c), head);
		}
		else
		{
			struct xa_aes_object next;
downlink:
			/* Try for a sibling */
			next = aesobj(aesobj_tree(c), aesobj_next(c));

			/* Trace back up tree if no more siblings */
			while (valid_aesobj(&next) && !same_aesobj(&next, &stop))
			{
				struct xa_aes_object tail = aesobj(aesobj_tree(&next), aesobj_tail(&next));
				if (same_aesobj(c, &tail))
				{
					*c = next;
					x -= aesobj_getx(c);
					y -= aesobj_gety(c);
					next = aesobj(aesobj_tree(c), aesobj_next(c));
					rel_depth--;
				}
				else
					break;
			}
			if (valid_aesobj(&next) && same_aesobj(&next, &stop) && set_aesobj_downlink(&tree, c, &stop, &oblink))
			{
				x -= aesobj_getx(c);
				y -= aesobj_gety(c);
				goto downlink;
			}
			*c = next;
		}
	}
	while (valid_aesobj(c) && !same_aesobj(c, &stop)  && rel_depth > 0);

	if (curson)
		(*wt->objcr_api->draw_cursor)(wt, v, NULL, 1);

	(*v->api->wr_mode)(v, MD_TRANS);
	(*v->api->f_interior)(v, FIS_SOLID);

	DIAGS(("draw_object_tree exit OK!"));

	clean_aesobj_links(&oblink);

	return true;
}
