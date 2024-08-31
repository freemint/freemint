#ifdef __PUREC__
#include <tos.h>
#include <vdi.h>
#include <aes.h>
#else
#include <gem.h>
#include <gemx.h>
#include <mint/mintbind.h>
#include <mint/slb.h>
#endif

#include <string.h>
#include <stdio.h>

#undef DESK
#include "tbarmenu.h"

static _WORD winh;
static _WORD aes_handle;
static _WORD vdi_handle;
static int ep;
static OBJECT *tbar;
static OBJECT *desk_popup;
static OBJECT *file_popup;
static OBJECT *text_popup;
static GRECT desk;


static int init_rsc(void)
{
	if (rsrc_load("tbarmenu.rsc") == 0)
	{
		form_alert(1, "[3][rsc not found][ Abort ]");
		return 0;
	}
	
	rsrc_gaddr(R_TREE, TBAR, &tbar);
	tbar[ROOT].ob_x = 0;
	tbar[ROOT].ob_y = 0;
	rsrc_gaddr(R_TREE, DESKPOPUP, &desk_popup);
	rsrc_gaddr(R_TREE, FILEPOPUP, &file_popup);
	rsrc_gaddr(R_TREE, TEXTPOPUP, &text_popup);

	return 1;
}


static void event_key(_WORD key)
{
	if ((key & 0xff) == 27)
		ep = 1;
}


static void event_mouse(_WORD mx, _WORD my)
{
	(void)mx;
	(void)my;
}


static void wind_redraw_toolbar(_WORD handle, _WORD ob, OBJECT *bar)
{
	_WORD top;
	GRECT dr;
	GRECT gr;

	wind_update(BEG_UPDATE);                    /* lock out other activity while we redraw */
	wind_get_int(0, WF_TOP, &top); /* get top window */
	objc_offset(bar, ob, &dr.g_x, &dr.g_y);
	dr.g_w = bar[ob].ob_width;
	dr.g_w = bar[ob].ob_height;
	
	if (handle == top)
	{
		wind_get_grect(handle, WF_FTOOLBAR, &gr);
		while (gr.g_w > 0 && gr.g_h > 0)
		{
			if (rc_intersect(&dr, &gr))
			{
				objc_draw_grect(bar, ob, MAX_DEPTH, &gr);
			}
			wind_get_grect(handle, WF_NTOOLBAR, &gr);
		}
	} else
	{
		wind_get_grect(handle, WF_FTOOLBAR, &gr);
		if (dr.g_x >= gr.g_x &&
			dr.g_y >= gr.g_y &&
			dr.g_x + dr.g_w <= gr.g_x + gr.g_w &&
			dr.g_y + dr.g_h <= gr.g_y + gr.g_h)
		{
			objc_draw_grect(bar, ob, MAX_DEPTH, &dr); /* redraw object only, not the entire rectangle */
		} else
		{
			form_dial_grect(FMD_START, &dr, &dr);
			form_dial_grect(FMD_FINISH, &dr, &dr);
		}
	}
	wind_update(END_UPDATE);                    /* reenable other GEM activity */
}


static void wind_redraw(_WORD handle, const GRECT *area)
{
	GRECT gr;
	_WORD pxy[4];
	
	wind_update(BEG_UPDATE);
	v_hide_c(vdi_handle);
	wind_get_grect(handle, WF_FIRSTXYWH, &gr);
	while (gr.g_w > 0 && gr.g_h > 0)
	{
		if (rc_intersect(area, &gr))
		{
			pxy[0] = gr.g_x;
			pxy[1] = gr.g_y;
			pxy[2] = gr.g_x + gr.g_w - 1;
			pxy[3] = gr.g_y + gr.g_h - 1;
			vs_clip(vdi_handle, 1, pxy);
			vsf_color(vdi_handle, G_WHITE);
			vsf_interior(vdi_handle, 1);
			vswr_mode(vdi_handle, MD_REPLACE);
			v_bar(vdi_handle, pxy);
		}
		wind_get_grect(handle, WF_NEXTXYWH, &gr);
	}
	v_show_c(vdi_handle, 1);
	wind_update(END_UPDATE);
}


static void event_message(_WORD *message)
{
	GRECT curr;
	_WORD ob;
	_WORD x, y;
	_WORD item;
	
	switch (message[0])
	{
	case WM_REDRAW:
		wind_redraw(message[3], (GRECT *)&message[4]);
		break;
	case WM_TOPPED:
		wind_set_int(message[3], WF_TOP, 0);
		break;
	case WM_CLOSED:
	case AP_TERM:
		ep = 1;
		break;
	case WM_FULLED:
		wind_get_grect(winh, WF_CURRXYWH, &curr);
		if (rc_equal(&curr, &desk))
		{
			wind_get_grect(winh, WF_PREVXYWH, &curr);
			wind_set_grect(winh, WF_CURRXYWH, &curr);
		} else
		{
			wind_get_grect(winh, WF_FULLXYWH, &curr);
			wind_set_grect(winh, WF_CURRXYWH, &curr);
		}
		break;
	case WM_SIZED:
	case WM_MOVED:
		wind_set_grect(winh, WF_CURRXYWH, (GRECT *)&message[4]);
		break;
	case WM_TOOLBAR:
		wind_get_grect(winh, WF_WORKXYWH, &curr);
		ob = message[4];
		x = curr.g_x + tbar[ob].ob_x;
		y = curr.g_y + tbar[ob].ob_y;
		tbar[ob].ob_state |= OS_SELECTED;
		wind_redraw_toolbar(winh, ob, tbar);
		switch (ob)
		{
		case DESK:
			desk_popup[ROOT].ob_x = x;
			desk_popup[ROOT].ob_y = y;
			item = form_popup(desk_popup, 0, 0);
			break;
		case FILE:
			file_popup[ROOT].ob_x = x;
			file_popup[ROOT].ob_y = y;
			item = form_popup(file_popup, 0, 0);
			break;
		case TEXT:
			text_popup[ROOT].ob_x = x;
			text_popup[ROOT].ob_y = y;
			item = form_popup(text_popup, 0, 0);
			break;
		default:
			item = -1;
			break;
		}
		printf("title=%d  item=%d\n", ob, item);
		tbar[ob].ob_state &= ~OS_SELECTED;
		wind_redraw_toolbar(winh, ob, tbar);
		break;
	}
}


int main(void)
{
	_WORD events;
	_WORD message[8];
	_WORD mx, my, mk, sk, key, nc;
	_WORD dummy;
	_WORD workin[11];
	_WORD workout[57];
	int i;
	
	appl_init();
	aes_handle = graf_handle(&dummy, &dummy, &dummy, &dummy);
	if (!init_rsc())
		return 1;
	vdi_handle = aes_handle;
	for (i = 0; i < 10; i++)
		workin[i] = 1;
	workin[10] = 2;
	v_opnvwk(workin, &vdi_handle, workout);
	
	wind_get_grect(0, WF_WORKXYWH, &desk);
	winh = wind_create_grect(NAME | CLOSER | FULLER | MOVER | INFO | SIZER, &desk);
	wind_set_ptr_int(winh, WF_TOOLBAR, tbar, 0);
	wind_set_str(winh, WF_INFO, "");
	wind_open(winh, desk.g_x, desk.g_y, desk.g_w / 2, desk.g_h / 2);
	do
	{
		events = evnt_multi(MU_KEYBD | MU_BUTTON | MU_MESAG,
			2, 1, 1,
			0, 0, 0, 0, 0,
			0, 0, 0, 0, 0,
			message,
			0,
			&mx, &my, &mk, &sk, &key, &nc);
		if (events & MU_MESAG)
			event_message(message);
		if (events & MU_BUTTON)
			event_mouse(mx, my);
		if (events & MU_KEYBD)
			event_key(key);
	} while (!ep);
	
	wind_close(winh);
	wind_delete(winh);
	v_clsvwk(vdi_handle);
	rsrc_free();
	
	appl_exit();
	return 0;
}
