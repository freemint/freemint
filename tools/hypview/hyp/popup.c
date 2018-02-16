/*
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __GNUC__
	#include <osbind.h>
#else
	#include <tos.h>
#endif
#include <gemx.h>
#include "../diallib.h"
#include "../defs.h"
#include "../hyp.h"

extern WINDOW_DATA *Win;

static short
PopupWindow(WINDOW_DATA *ptr, short obj, void *data)
{
	WINDOW_DATA *win;
	POPUP_INFO *popup = ptr->data;
	Win = win = ptr;

	if (obj == WIND_INIT)
	{
		/*	Als Rasterweite werden die Font-Groessen benutzt	*/
		win->x_raster = font_cw;
		win->y_raster = font_ch;

		win->max_width = popup->cols;
		win->max_height = popup->lines;
	}
	else if(obj == WIND_EXIT)
	{
		popup->doc->popup = NULL;
		Mfree(popup);
	}
	else if(obj == WIND_OPEN)
	{
	}
	else if(obj == WIND_OPENSIZE)	/*	Fenstergroesse beim oeffnen	*/
	{
	GRECT *osize=(GRECT *)data,screen;
		wind_get_grect(0,WF_WORKXYWH,&screen);
		
		if(popup->x+osize->g_w>screen.g_x+screen.g_w)
			popup->x=screen.g_x+screen.g_w-osize->g_w;
		osize->g_x=max(screen.g_x,popup->x);
		if(popup->y+osize->g_h>screen.g_y+screen.g_h)
			popup->y=screen.g_y+screen.g_h-osize->g_h;
		osize->g_y=max(screen.g_y,popup->y);
	}
	else if(obj==WIND_CLOSE)
	{
	}
	else if(obj==WIND_REDRAW)
	{
	short pxy[4];
	GRECT *box=(GRECT *)data;
	DOCUMENT *doc=popup->doc;
	HYP_DOCUMENT *hyp=doc->data;
	LOADED_ENTRY *old_entry=hyp->entry;
	long old_lines=doc->lines,old_cols=doc->columns;

		pxy[0]=box->g_x;
		pxy[1]=box->g_y;
		pxy[2]=box->g_x+box->g_w-1;
		pxy[3]=box->g_y+box->g_h-1;
		vs_clip(vdi_handle,1,pxy);					/*	Clipping ON	*/

		vsf_color(vdi_handle,G_WHITE);
		vsf_interior(vdi_handle,FIS_SOLID);
		vswr_mode(vdi_handle,MD_REPLACE);
		vr_recfl(vdi_handle,pxy);					/*	weisses Rechteck	*/

		hyp->entry=popup->entry;
		doc->lines=popup->lines;
		doc->columns=popup->cols;

		HypDisplayPage(doc);

		hyp->entry=old_entry;
		doc->lines=old_lines;
		doc->columns=old_cols;

		vs_clip(vdi_handle,0,pxy);					/*	Clipping OFF	*/
	}
	else if(obj==WIND_KEYPRESS)
	{
		EVNT *event=(EVNT *)data;
		SendClose(win);
		event->mwhich &= ~MU_KEYBD;
	}
	else if(obj==WIND_CLICK)
	{
		EVNT *event=(EVNT *)data;

		if(event->mbutton & 1)
		{
			GRECT work;
			do
			{
				graf_mkstate(&event->mx, &event->my, &event->mbutton, &event->kstate);
			}while(event->mbutton & 1);

			wind_get_grect(ptr->whandle,WF_WORKXYWH,&work);
/*			GetLink(file,event->mx-work.g_x,event->my-work.g_y-ptr->y_offset);
*/
			SendClose(win);
		}
	}
	return(1);
}

void OpenPopup(DOCUMENT *doc, long num, short x, short y)
{
	WINDOW_DATA *win = Win;
	HYP_DOCUMENT *hyp=doc->data;
	POPUP_INFO *popup;

	graf_mouse(BUSYBEE,NULL);

	popup=(POPUP_INFO *)Malloc(sizeof(POPUP_INFO));
	
	if(popup)
	{
		GRECT work;
		LOADED_ENTRY *old_entry = hyp->entry;
		long old_lines = doc->lines, old_cols = doc->columns;
		short old_buttons = doc->buttons;
		char *old_wtitle = doc->window_title;

		doc->gotoNodeProc(doc, NULL, num);

		wind_get_grect(win->whandle, WF_WORKXYWH, &work);

		popup->doc = doc;
		popup->lines = doc->lines;
		popup->cols = doc->columns;
		popup->x = x + work.g_x - (short)win->x_pos * win->x_raster;
		popup->y = y + work.g_y + win->y_offset;
		popup->entry = hyp->entry;

		hyp->entry = old_entry;
		doc->lines = old_lines;
		doc->columns = old_cols;
		doc->buttons = old_buttons;
		doc->window_title = old_wtitle;

		doc->popup = OpenWindow(PopupWindow, 0, NULL, -1, -1, popup);
	}
	else
		Debug("ERROR: Not enough memory to display popup");

	graf_mouse(ARROW,NULL);
}

