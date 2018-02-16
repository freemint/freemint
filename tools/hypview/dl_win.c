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
#include "diallib.h"

#if USE_WINDOW == YES


WINDOW_DATA *OpenWindow(HNDL_WIN proc, short kind, char *title, 
					WP_UNIT max_w, WP_UNIT max_h,void *user_data);
void CloseWindow(WINDOW_DATA *ptr);
void CloseAllWindows(void);
void RemoveWindow(WINDOW_DATA *ptr);
short ScrollWindow(WINDOW_DATA *ptr, short *rel_x, short *rel_y);
void WindowEvents(WINDOW_DATA *ptr, EVNT *event);
void ResizeWindow(WINDOW_DATA *ptr, WP_UNIT max_w, WP_UNIT max_h);
void SetWindowSlider(WINDOW_DATA *ptr);
void IconifyWindow(WINDOW_DATA *ptr,GRECT *r);
void UniconifyWindow(WINDOW_DATA *ptr);
WINDOW_DATA *find_openwindow_by_whandle(short handle);
WINDOW_DATA *find_window_by_whandle(short handle);
WINDOW_DATA *find_window_by_proc(HNDL_WIN proc);
WINDOW_DATA *find_window_by_data(void *data);


#if USE_TOOLBAR==YES
	#define DL_WIN_XADD	ptr->x_offset
	#define DL_WIN_YADD	ptr->y_offset
#else
	#define DL_WIN_XADD	0
	#define DL_WIN_YADD	0
#endif



WINDOW_DATA *OpenWindow(HNDL_WIN proc, short kind, char *title,
			WP_UNIT max_w, WP_UNIT max_h,void *user_data)
{
	WINDOW_DATA *ptr;
	ptr = find_window_by_proc(proc);
	if((ptr) && (user_data == ptr->data))
	{
		if (ptr->status & WIS_OPEN)
		{
			short msg[8]={WM_TOPPED,0,0,0,0,0,0,0};
			msg[3] = ptr->whandle;
			msg[1] = ap_id;
			appl_write(ap_id,16,msg);
		}
		if (ptr->status & WIS_ICONIFY)
		{
			short msg[8]={WM_UNICONIFY,0,0,0,0,0,0,0};
			msg[3]=ptr->whandle;
			wind_get_grect(msg[3],WF_UNICONIFY,(GRECT *)&msg[4]);
			msg[1]=ap_id;
			appl_write(ap_id,16,msg);
		}
		if ((ptr->status & WIS_ALLICONIFY) && !(ptr->status & WIS_OPEN))
		{
			GRECT small;
			short i,copy=FALSE;
			wind_get_grect(iconified_list[0]->whandle,WF_CURRXYWH,&small);

			graf_growbox_grect(&small,&ptr->last);
			wind_open_grect(ptr->whandle,&ptr->last);
			ptr->status&=~WIS_ALLICONIFY;
			ptr->status|=WIS_OPEN;
			for(i=0;i<iconified_count;i++)
			{
				if(copy)
					iconified_list[i-1]=iconified_list[i];
				if((WINDOW_DATA *)iconified_list[i]==ptr)
					copy=TRUE;
			}
			iconified_count--;
		}
		if(!(ptr->status & WIS_OPEN) &&
			!(ptr->status & (WIS_ICONIFY|WIS_ALLICONIFY)))
		{
		GRECT small={0,0,0,0};
			graf_growbox_grect(&small,&ptr->last);
			if(wind_open_grect(ptr->whandle,&ptr->last))
				ptr->status|=WIS_OPEN;
		}
	}
	else
	{
		GRECT screen;

		ptr = (WINDOW_DATA *)Mxalloc(sizeof(WINDOW_DATA),3);
		if (ptr == NULL)
		{
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
			return(NULL);
		}

#if MAGIC_ONLY==NO
		if(!has_iconify)
			kind &= 0x0fff;					/*	SMALLER und andere Buttons wegfiltern	*/
#endif
		ptr->type = WIN_WINDOW;
		ptr->status = 0;
		ptr->proc = proc;
		ptr->title = title;
		ptr->kind = kind;
		ptr->max_width = max_w;
		ptr->max_height = max_h;
		ptr->x_pos = 0;
		ptr->y_pos = 0;
		ptr->x_speed = 1;
		ptr->y_speed = 1;
#if USE_LOGICALRASTER==YES
		ptr->x_raster = 1;
		ptr->y_raster = 1;
#endif
#if USE_TOOLBAR==YES
		ptr->toolbar = NULL;
		ptr->x_offset = 0;
		ptr->y_offset = 0;
#endif
		ptr->data = user_data;

		wind_get_grect(0,WF_WORKXYWH,&screen);
		ptr->whandle = wind_create_grect(ptr->kind,&screen);
		if(ptr->whandle >= 0)
		{
			short ok = TRUE;
#if OPEN_VDI_WORKSTATION==YES
			short i,workin[11];

			for (i = 0; i < 10; workin[i++] = 1);
			workin[10] = 2;
			ptr->vdi_handle = aes_handle;

			v_opnvwk(workin, &ptr->vdi_handle, ptr->workout);
			if(ptr->vdi_handle)
				vq_extnd(ptr->vdi_handle, 1, ptr->ext_workout);
			else
				ok = FALSE;
#endif
			if(ok && proc(ptr, WIND_INIT, NULL))
			{
				GRECT small = {0,0,0,0}, win;
				GRECT open_size;

				wind_calc_grect(WC_WORK, ptr->kind, &screen, &win);

#if USE_LOGICALRASTER==YES
				win.g_w = (short)min(DL_WIN_XADD + ptr->max_width * ptr->x_raster, win.g_w - (win.g_w - DL_WIN_XADD) % ptr->x_raster);
				win.g_h = (short)min(DL_WIN_YADD + ptr->max_height * ptr->y_raster, win.g_h - (win.g_h-DL_WIN_YADD) % ptr->y_raster);
#else
				win.g_w = min(DL_WIN_XADD + ptr->max_width, win.g_w);
				win.g_h = min(DL_WIN_YADD + ptr->max_height, win.g_h);
#endif

				wind_calc_grect(WC_BORDER,ptr->kind,&win,&ptr->full);

				open_size = ptr->full;


#if USE_LOGICALRASTER==YES
				if(proc(ptr, WIND_OPENSIZE, &open_size))
				{
					GRECT real_worksize;
					wind_calc_grect(WC_WORK, ptr->kind, &open_size,&win);
					real_worksize.g_x = win.g_x;
					real_worksize.g_y = win.g_y;
					real_worksize.g_w = (short)min(32000L, ptr->max_width * ptr->x_raster + DL_WIN_XADD);
					real_worksize.g_h = (short)min(32000L, ptr->max_height * ptr->y_raster + DL_WIN_YADD);
					rc_intersect_my(&real_worksize, &win);
					wind_calc_grect(WC_BORDER, ptr->kind, &win, &open_size);
				}
#else
				proc(ptr, WIND_OPENSIZE, &open_size);
#endif

				graf_growbox_grect(&small, &open_size);
				if(wind_open_grect(ptr->whandle, &open_size))
				{
					ptr->status = WIS_OPEN;
					add_item((CHAIN_DATA *)ptr);
					if ((ptr->kind & NAME) && ptr->title)
						wind_set_str(ptr->whandle, WF_NAME, ptr->title);
					proc(ptr, WIND_OPEN, NULL);
					SetWindowSlider(ptr);
				}
				else
					ok = FALSE;
			}

			if(!ok)
			{
				proc(ptr,WIND_EXIT,NULL);
#if OPEN_VDI_WORKSTATION==YES
				if(ptr->vdi_handle)
					v_clsvwk(ptr->vdi_handle);
#endif
				wind_delete(ptr->whandle);
				Mfree(ptr);
				ptr = NULL;
			}
		}
		else
		{
			Mfree(ptr);
			ptr = NULL;
		}
	}
	return(ptr);
}

void CloseWindow(WINDOW_DATA *ptr)
{
GRECT small={0,0,0,0};
	if(!ptr)
		return;
	if(ptr->status & WIS_ICONIFY)
	{
	EVNT event;
		event.mwhich=MU_MESAG;
		event.msg[0]=WM_UNICONIFY;
		event.msg[1]=ap_id;
		event.msg[2]=0;
		event.msg[3]=ptr->whandle;
		wind_get_grect(ptr->whandle,WF_UNICONIFY,(GRECT *)&event.msg[4]);
		DoEventDispatch(&event);
	}
	if(ptr->status & WIS_ALLICONIFY)
	{
	GRECT sm;
	short i,copy=FALSE;
		wind_get_grect(iconified_list[0]->whandle,WF_CURRXYWH,&sm);

		graf_growbox_grect(&sm,&ptr->last);
		wind_open_grect(ptr->whandle,&ptr->last);
		ptr->status&=~WIS_ALLICONIFY;
		ptr->status|=WIS_OPEN;
		for(i=0;i<iconified_count;i++)
		{
			if(copy)
				iconified_list[i-1]=iconified_list[i];
			if(iconified_list[i]==(CHAIN_DATA *)ptr)
				copy=TRUE;
		}
		iconified_count--;
	}
	if((ptr->status & WIS_OPEN)&&(ptr->proc(ptr,WIND_CLOSE,NULL)))
	{
		ptr->status&=~WIS_OPEN;
		wind_get_grect(ptr->whandle,WF_CURRXYWH,&ptr->last);
		wind_close(ptr->whandle);
		graf_shrinkbox_grect(&small,&ptr->last);
		if(modal_items>=0)
			modal_items--;
	}
}

void CloseAllWindows(void)
{
WINDOW_DATA *ptr=(WINDOW_DATA *)all_list;
	while(ptr)
	{
		if(ptr->type==WIN_WINDOW)
			CloseWindow(ptr);
		ptr=ptr->next;
	};
}

void RemoveWindow(WINDOW_DATA *ptr)
{
GRECT big,small={0,0,0,0};
	if(ptr)
	{
		if(ptr->status & WIS_OPEN)
		{
			wind_get_grect(ptr->whandle,WF_CURRXYWH,&big);
			ptr->proc(ptr,WIND_CLOSE,NULL);
			wind_close(ptr->whandle);
			graf_shrinkbox_grect(&small,&big);
		}
		ptr->proc(ptr,WIND_EXIT,NULL);
#if OPEN_VDI_WORKSTATION==YES
		v_clsvwk(ptr->vdi_handle);
#endif
		wind_delete(ptr->whandle);
		remove_item((CHAIN_DATA *)ptr);
		Mfree(ptr);
		if(modal_items>=0)
			modal_items--;
	}
}

short
ScrollWindow(WINDOW_DATA *ptr, short *r_x, short *r_y)
{
	WP_UNIT oy = ptr->y_pos, ox = ptr->x_pos;
	MFDB screen = {0,0,0,0};
	short pxy[8] = {-99};
	short rel_x, rel_y;
	short abs_rel_x, abs_rel_y, move_screen = TRUE;
	GRECT work, box, redraw;
#if USE_LOGICALRASTER==YES
	short lines_width, lines_height;
#endif

	if (r_x)
		rel_x = *r_x;
	else
		rel_x = 0;
	
	if(r_y)
		rel_y = *r_y;
	else
		rel_y = 0;

	wind_get_grect(ptr->whandle, WF_WORKXYWH, &work);

	/*	Zu bearbeitender Bereich auf den Bildschirm beschraenken	*/
	wind_get_grect(0, WF_WORKXYWH, &box);
	rc_intersect_my(&box, &work);

#if USE_TOOLBAR==YES
	work.g_x += ptr->x_offset;
	work.g_y += ptr->y_offset;
	work.g_w -= ptr->x_offset;
	work.g_h -= ptr->y_offset;
#endif

/*	Debug("Work: %d %d %d %d",work);
	Debug("(%d %d) to (%d %d)",work.g_x,work.g_y,work.g_x+work.g_w,work.g_y+work.g_h);
	Debug("x_pos: %ld  y_pos: %ld  rel_x: %d  rel_y: %d",ox, oy, rel_x, rel_y);
*/	

#if USE_LOGICALRASTER == YES
	lines_width = work.g_w / ptr->x_raster;
	lines_height = work.g_h / ptr->y_raster;
	
	if (rel_x < 0)
		ptr->x_pos = max(ptr->x_pos + rel_x, 0);
	else
	{
		ptr->x_pos = min(ptr->x_pos + rel_x, ptr->max_width - lines_width);
		ptr->x_pos = max(0, ptr->x_pos);
	}

	if(rel_y < 0)
		ptr->y_pos = max(ptr->y_pos + rel_y, 0);
	else
	{
		ptr->y_pos = min(ptr->y_pos + rel_y, ptr->max_height - lines_height);
		ptr->y_pos = max(0, ptr->y_pos);
	}
#else
	if(rel_x < 0)
		ptr->x_pos=max(ptr->x_pos+rel_x,0);
	else
	{
		ptr->x_pos = min(ptr->x_pos + rel_x, ptr->max_width - work.g_w);
		ptr->x_pos = max(0, ptr->x_pos);
	}

	if(rel_y < 0)
		ptr->y_pos = max(ptr->y_pos + rel_y, 0);
	else
	{
		ptr->y_pos = min(ptr->y_pos + rel_y, ptr->max_height - work.g_h);
		ptr->y_pos = max(0, ptr->y_pos);
	}
#endif

	if((ox == ptr->x_pos) && (oy == ptr->y_pos))
	{
		SetWindowSlider(ptr);
		return(FALSE);
	}

	rel_x = (short)(ptr->x_pos-ox);
	rel_y = (short)(ptr->y_pos-oy);
	if(r_x)
		*r_x = rel_x;
	if(r_y)
		*r_y = rel_y;

#if USE_LOGICALRASTER == YES
	if((abs(ptr->x_pos - ox) >= lines_width) ||
	   (abs(ptr->y_pos - oy) >= lines_height))
	{
		move_screen = FALSE;
		abs_rel_x = abs(rel_x);
		abs_rel_y = abs(rel_y);
	}
	else
	{
		rel_x = rel_x * ptr->x_raster;
		rel_y = rel_y * ptr->y_raster;	
		abs_rel_x = abs(rel_x);
		abs_rel_y = abs(rel_y);
	}
#else
	abs_rel_x = abs(rel_x);
	abs_rel_y = abs(rel_y);
	if(abs_rel_x >= work.g_w || abs_rel_y >= work.g_h)
		move_screen = FALSE;
#endif

	graf_mouse(M_OFF,NULL);
	wind_update(BEG_UPDATE);
	wind_get_grect(ptr->whandle, WF_FIRSTXYWH, &box);

	while(box.g_w && box.g_h)
	{
		if(rc_intersect_my(&work, &box))
		{
			if(!move_screen || abs_rel_x >= box.g_w || abs_rel_y >= box.g_h)
			{
				ptr->proc(ptr,WIND_REDRAW, (void *)&box);
			}
			else
			{
				pxy[0] = box.g_x;
				pxy[2] = box.g_x + box.g_w - 1;
				pxy[4] = box.g_x;
				pxy[6] = box.g_x + box.g_w - 1;

				if(rel_x > 0)
				{
					pxy[0] += rel_x;
					pxy[6] -= rel_x;
				}
				else if(rel_x < 0)
				{
					pxy[2] += rel_x;
					pxy[4] -= rel_x;
				}

				pxy[1] = box.g_y;
				pxy[3] = box.g_y + box.g_h - 1;
				pxy[5] = box.g_y;
				pxy[7] = box.g_y + box.g_h - 1;
				if(rel_y > 0)
				{
					pxy[1] += rel_y;
					pxy[7] -= rel_y;
				}
				else if(rel_y < 0)
				{
					pxy[3] += rel_y;
					pxy[5] -= rel_y;
				}

#if OPEN_VDI_WORKSTATION==YES
				vro_cpyfm(ptr->vdi_handle, 3, pxy, &screen, &screen);
#else
	#if USE_GLOBAL_VDI==YES
				vro_cpyfm(vdi_handle, 3, pxy, &screen, &screen);
	#else
		#error "Es muss mindestens eine VDI Workstation geffnet werden! (USE_GLOBAL_VDI=YES oder OPEN_VDI_WORKSTATION=YES)"
	#endif
#endif

				redraw = box;

				if(rel_x > 0)
				{
					redraw.g_x += box.g_w-rel_x;
					redraw.g_w = rel_x;
					box.g_w -= rel_x;
				}
				else if(rel_x < 0)
				{
					redraw.g_w =- rel_x;
					box.g_x -= rel_x;
					box.g_w += rel_x;
				}

				if(rel_x)
					ptr->proc(ptr, WIND_REDRAW, (void *)&redraw);

				redraw = box;

				if(rel_y > 0)
				{
					redraw.g_y += box.g_h-rel_y;
					redraw.g_h = rel_y;
				}
				else if(rel_y < 0)
				{
					redraw.g_h =- rel_y;
				}

				if(rel_y)
					ptr->proc(ptr,WIND_REDRAW,(void *)&redraw);
			}
		}

		wind_get_grect(ptr->whandle, WF_NEXTXYWH,&box);
	};
	SetWindowSlider(ptr);
	wind_update(END_UPDATE);
	graf_mouse(M_ON,NULL);

	return(TRUE);
}


void WindowEvents(WINDOW_DATA *ptr, EVNT *event)
{
/* Debug ( "WindowEvents: msg[0]=%d", event->msg[0]); */

	if(ptr->status & WIS_ICONIFY)
	{
		if(event->mwhich & MU_KEYBD)
			event->mwhich&=~MU_KEYBD;
		if(event->mwhich & MU_BUTTON)
			event->mwhich&=~MU_BUTTON;
		if(event->mwhich & MU_MESAG)
		{
			switch(event->msg[0])
			{
				case WM_REDRAW:
				case WM_UNICONIFY:
				case WM_MOVED:
					break;
				case WM_TOPPED:
					wind_set(event->msg[3],WF_TOP,0,0,0,0);
				default:
					event->mwhich&=~MU_MESAG;
					break;
			}
		}
	}

	if(event->mwhich & MU_BUTTON)
	{
#if USE_TOOLBAR==YES
	GRECT toolbar;
		wind_get_grect(ptr->whandle, WF_WORKXYWH,&toolbar);
		
		if(ptr->toolbar)
		{
			ptr->toolbar->ob_x=toolbar.g_x;
			ptr->toolbar->ob_y=toolbar.g_y;
	
			if(ptr->y_offset)
			{
				ptr->toolbar->ob_width=toolbar.g_w;
				toolbar.g_h=DL_WIN_YADD;
			}
			else if(ptr->x_offset)
			{
				ptr->toolbar->ob_height=toolbar.g_h;
				toolbar.g_h=DL_WIN_XADD;
			}

			ptr->proc(ptr,WIND_TBUPDATE,&toolbar);

			/*	Wurde in die Toolbar geklickt?	*/		
			if((event->mx>=toolbar.g_x) && (event->my>=toolbar.g_y) && 
				(event->mx<toolbar.g_x+toolbar.g_w) && (event->my<toolbar.g_y+toolbar.g_h))
			{
			short num;
				/*	Welches Objekt?	*/
				num=objc_find(ptr->toolbar,0,MAX_DEPTH,event->mx,event->my);
	
				/*	Gueltiges (>=0), selektierbares, aktives Objekt mit
					Exit / Touchexit Flag??	*/
			 	if((num>=0)&&(ptr->toolbar[num].ob_flags & OF_SELECTABLE)&&
			 			!(ptr->toolbar[num].ob_state & OS_DISABLED)&&
			 			(ptr->toolbar[num].ob_flags & (OF_EXIT|OF_TOUCHEXIT)))
			 	{
					if((ptr->toolbar[num].ob_flags & OF_TOUCHEXIT) ||
						((ptr->toolbar[num].ob_flags & OF_EXIT) &&
							graf_watchbox(ptr->toolbar,num,OS_SELECTED,0)))
					{
						evnt_timer(10);
						ptr->proc(ptr,WIND_TBCLICK,&num);
					}
				}
			}
			else
				ptr->proc(ptr,WIND_CLICK,event);
		}
		else
			ptr->proc(ptr,WIND_CLICK,event);
#else
		ptr->proc(ptr,WIND_CLICK,event);
#endif
	}
	if(event->mwhich & MU_KEYBD)
	{
		ConvertKeypress(&event->key,&event->kstate);
		ptr->proc(ptr,WIND_KEYPRESS,event);
	}
	if(event->mwhich & MU_MESAG)
	{
		if(event->msg[3]!=ptr->whandle)	/*	Message fuer ein anderes Fenster?	*/
			return;

		event->mwhich&=~MU_MESAG;
		switch(event->msg[0])
		{
			case WM_REDRAW:
			{
			GRECT box;
				wind_update(BEG_UPDATE);
				if(ptr->status & WIS_ICONIFY)
				{
					wind_get_grect(event->msg[3], WF_WORKXYWH,&box);
					tree_addr[DIAL_LIBRARY][0].ob_x=box.g_x;
					tree_addr[DIAL_LIBRARY][0].ob_y=box.g_y;
#if MAGIC_ONLY==YES
					objc_wdraw(tree_addr[DIAL_LIBRARY],0,1,(GRECT *)&event->msg[4],ptr->whandle);
#else
					wind_get_grect(event->msg[3], WF_FIRSTXYWH,&box);
					graf_mouse(M_OFF,NULL);
					while(box.g_w && box.g_h)
					{
						if(rc_intersect_my((GRECT *)&event->msg[4], &box))
							objc_draw_grect(tree_addr[DIAL_LIBRARY],0,1,&box);
						wind_get_grect(event->msg[3], WF_NEXTXYWH,&box);
					}
					graf_mouse(M_ON,NULL);
#endif
				}
				else
				{
#if USE_TOOLBAR==YES
				GRECT toolbar;
					wind_get_grect(event->msg[3], WF_WORKXYWH,&toolbar);
					if(ptr->toolbar)
					{

						ptr->toolbar->ob_x=toolbar.g_x;
						ptr->toolbar->ob_y=toolbar.g_y;
						if(ptr->y_offset)		/*	Horizontale Toolbar?	*/
						{
							ptr->toolbar->ob_width=toolbar.g_w;		/*	Objektbreite anpassen	*/
							toolbar.g_h=ptr->y_offset;
						}
						if(ptr->x_offset)		/*	Vertikale Toolbar?	*/
						{
							ptr->toolbar->ob_height=toolbar.g_h;	/*	Objekthoehe anpassen	*/
							toolbar.g_w=ptr->x_offset;
						}

						ptr->proc(ptr,WIND_TBUPDATE,&toolbar);
					}
					else
					{
						toolbar.g_w=0;
						toolbar.g_h=0;
					}
#endif
					wind_get_grect(event->msg[3], WF_FIRSTXYWH,&box);

					graf_mouse(M_OFF,NULL);
					while(box.g_w && box.g_h)
					{
#if USE_TOOLBAR==YES
					GRECT temp_tbar;
						temp_tbar=toolbar;
						if(rc_intersect_my(&box,&temp_tbar))
						{
							if(ptr->x_offset)		/*	Vertikale Toolbar?	*/
							{
								box.g_x+=temp_tbar.g_w;
								box.g_w-=temp_tbar.g_w;
							}
							if(ptr->y_offset)		/*	Horizontale Toolbar?	*/
							{
								box.g_y+=temp_tbar.g_h;
								box.g_h-=temp_tbar.g_h;
							}
						}
						if(rc_intersect_my((GRECT *)&event->msg[4], &temp_tbar))
							objc_draw_grect(ptr->toolbar,0,MAX_DEPTH,&temp_tbar);
#endif
						if(rc_intersect_my((GRECT *)&event->msg[4], &box))
							ptr->proc(ptr,WIND_REDRAW,(void *)&box);
						wind_get_grect(event->msg[3], WF_NEXTXYWH,&box);
					}
					graf_mouse(M_ON,NULL);
				}
				wind_update(END_UPDATE);
				break;
			}
			case WM_TOPPED:
				if(ptr->proc(ptr,WIND_TOPPED,event))
					wind_set(event->msg[3],WF_TOP,0,0,0,0);
				break;
			case WM_CLOSED:
#if WIND_CLOSE_IS_REMOVE==YES
				RemoveWindow(ptr);
#else
				CloseWindow(ptr);
#endif
				break;
			case WM_FULLED:
			{
			GRECT big;
				if(ptr->status & WIS_FULL)
				{
					wind_get_grect(event->msg[3],WF_CURRXYWH,&big);

					ptr->proc(ptr,WIND_FULLED,&ptr->last);
					graf_shrinkbox_grect(&ptr->last,&big);
					wind_set_grect(event->msg[3],WF_CURRXYWH,&ptr->last);

					ptr->status&=~WIS_FULL;
					SetWindowSlider(ptr);
					break;
				}
				else
				{
				GRECT win;
				short rel_x,rel_y;
					wind_get_grect(event->msg[3],WF_CURRXYWH,&ptr->last);
					
					ptr->proc(ptr,WIND_FULLED,&ptr->full);
					graf_growbox_grect(&ptr->last,&ptr->full);
					wind_set_grect(event->msg[3],WF_CURRXYWH,&ptr->full);

					ptr->status|=WIS_FULL;

					wind_get_grect(event->msg[3],WF_WORKXYWH,&win);
#if USE_LOGICALRASTER==YES
					rel_x = (short)((ptr->max_width - ptr->x_pos) - (win.g_w - DL_WIN_XADD) / ptr->x_raster);
					rel_y = (short)((ptr->max_height - ptr->y_pos) - (win.g_h - DL_WIN_YADD) / ptr->y_raster);
#else
					rel_x=ptr->max_width-ptr->x_pos-(win.g_w-DL_WIN_XADD);
					rel_y=ptr->max_height-ptr->y_pos-(win.g_h-DL_WIN_YADD);
#endif
					if((rel_x<0)||(rel_y<0))
					{
						rel_x=min(rel_x,0);
						rel_y=min(rel_y,0);
						ScrollWindow(ptr,&rel_x,&rel_y);
					}
					else
						SetWindowSlider(ptr);
					break;
				}
			}
			case WM_ARROWED:
			{
			short rel_x=0,rel_y=0;
			GRECT win;

				wind_get_grect(event->msg[3],WF_WORKXYWH,&win);

#if USE_TOOLBAR==YES
				/*	Toolbar beim Scrollen beruecksichtigen	*/
				win.g_w-=DL_WIN_XADD;
				win.g_h-=DL_WIN_YADD;
#endif

				switch(event->msg[4])
				{
#if USE_LOGICALRASTER==YES
				case WA_UPPAGE:
					rel_y=-win.g_h/ptr->y_raster;
					break;
				case WA_DNPAGE:
					rel_y=win.g_h/ptr->y_raster;
					break;
				case WA_LFPAGE:
					rel_x=-win.g_w/ptr->x_raster;
					break;
				case WA_RTPAGE:
					rel_x=win.g_w/ptr->x_raster;
					break;
#else
				case WA_UPPAGE:
					rel_y=-win.g_h;
					break;
				case WA_DNPAGE:
					rel_y=win.g_h;
					break;
				case WA_LFPAGE:
					rel_x=-win.g_w;
					break;
				case WA_RTPAGE:
					rel_x=win.g_w;
					break;
#endif
				case WA_UPLINE:
					rel_y=-ptr->y_speed;
					break;
				case WA_DNLINE:
					rel_y=ptr->y_speed;
					break;
				case WA_LFLINE:
					rel_x=-ptr->x_speed;
					break;
				case WA_RTLINE:
					rel_x=ptr->x_speed;
					break;
				}
				ScrollWindow(ptr,&rel_x,&rel_y);
				break;
			}
			case WM_HSLID:
			{
			short rel_x;
			GRECT win;
				wind_get_grect(event->msg[3],WF_WORKXYWH,&win);
#if USE_LOGICALRASTER==YES
				rel_x=(short)((event->msg[4]*(ptr->max_width-(win.g_w-DL_WIN_XADD)/ptr->x_raster))/1000-ptr->x_pos);
#else
				rel_x=(short)(((long)event->msg[4]*(ptr->max_width-win.g_w+DL_WIN_XADD))/1000L-ptr->x_pos);
#endif
				ScrollWindow(ptr,&rel_x,NULL);
				break;
			}
			case WM_VSLID:
			{
			short rel_y;
			GRECT win;
				wind_get_grect(event->msg[3],WF_WORKXYWH,&win);
#if USE_LOGICALRASTER==YES
				rel_y=(short)((event->msg[4]*(ptr->max_height-(win.g_h-DL_WIN_YADD)/ptr->y_raster))/1000-ptr->y_pos);
#else
				rel_y=(short)(((long)event->msg[4]*(ptr->max_height-win.g_h+DL_WIN_YADD))/1000L-ptr->y_pos);
#endif
				ScrollWindow(ptr,NULL,&rel_y);
				break;
			}
			case WM_SIZED:
			{
			GRECT win;
			short rel_x,rel_y;

				ptr->proc(ptr,WIND_SIZED,&event->msg[4]);
				wind_set_grect(event->msg[3],WF_CURRXYWH,(GRECT *)&event->msg[4]);
				ptr->status&=~WIS_FULL;

				wind_get_grect(event->msg[3],WF_WORKXYWH,&win);
#if USE_LOGICALRASTER==YES
				rel_x=(short)((ptr->max_width-ptr->x_pos)-(win.g_w-DL_WIN_XADD)/ptr->x_raster);
				rel_y=(short)((ptr->max_height-ptr->y_pos)-(win.g_h-DL_WIN_YADD)/ptr->y_raster);
#else
				rel_x=ptr->max_width-ptr->x_pos-(win.g_w-DL_WIN_XADD);
				rel_y=ptr->max_height-ptr->y_pos-(win.g_h-DL_WIN_YADD);
#endif
				if((rel_x<0)||(rel_y<0))
				{
					rel_x=min(rel_x,0);
					rel_y=min(rel_y,0);
					ScrollWindow(ptr,&rel_x,&rel_y);
				}
				else
					SetWindowSlider(ptr);
				break;
			}
			case WM_MOVED:
				ptr->proc(ptr,WIND_MOVED,&event->msg[4]);
				wind_set_grect(event->msg[3],WF_CURRXYWH,(GRECT *)&event->msg[4]);
				ptr->status&=~WIS_FULL;
				SetWindowSlider(ptr);
				break;
			case WM_NEWTOP:
				ptr->proc(ptr,WIND_NEWTOP,NULL);
				break;
			case WM_UNTOPPED:
				ptr->proc(ptr,WIND_UNTOPPED,NULL);
				break;
			case WM_ONTOP:
				ptr->proc(ptr,WIND_ONTOP,NULL);
				break;
			case WM_BOTTOMED:
				if(ptr->proc(ptr,WIND_BOTTOM,event))
					wind_set(event->msg[3],WF_BOTTOM,0,0,0,0);
				break;
			case WM_ICONIFY:
				IconifyWindow(ptr,(GRECT *)&event->msg[4]);
				ptr->proc(ptr,WIND_ICONIFY,(void *)&event->msg[4]);
				break;
			case WM_UNICONIFY:
				UniconifyWindow(ptr);
				ptr->proc(ptr,WIND_UNICONIFY,(void *)&event->msg[4]);
				break;
			case WM_ALLICONIFY:
				AllIconify(event->msg[3],(GRECT *)&event->msg[4]);
				ptr->proc(ptr,WIND_ALLICONIFY,(void *)&event->msg[4]);
				break;
			default:
				event->mwhich|=MU_MESAG;
				break;
		}
	}
}

void SetWindowSlider(WINDOW_DATA *ptr)
{
	GRECT win;
	long temp;
	if(ptr->status & WIS_ICONIFY)
		return;

	wind_get_grect(ptr->whandle,WF_WORKXYWH,&win);

#if USE_LOGICALRASTER==YES
	temp = ptr->max_width - (win.g_w - DL_WIN_XADD) / ptr->x_raster;
#else
	temp = ptr->max_width - (win.g_w-DL_WIN_XADD);
#endif
	if(temp > 0)
	{
		wind_set(ptr->whandle,WF_HSLIDE, (short)(ptr->x_pos * 1000L / temp), 0,0,0);
#if USE_LOGICALRASTER==YES
		wind_set(ptr->whandle,WF_HSLSIZE,(short)((win.g_w - DL_WIN_XADD) / ptr->x_raster * 1000L / ptr->max_width),0,0,0);
#else
		wind_set(ptr->whandle,WF_HSLSIZE,(short)((win.g_w - DL_WIN_XADD) * 1000L / ptr->max_width), 0,0,0);
#endif
	}
	else
	{
		wind_set(ptr->whandle,WF_HSLIDE,1000,0,0,0);
		wind_set(ptr->whandle,WF_HSLSIZE,1000,0,0,0);
	}
#if USE_LOGICALRASTER==YES
	temp = ptr->max_height - (win.g_h - DL_WIN_YADD) / ptr->y_raster;
#else
	temp = ptr->max_height - (win.g_h - DL_WIN_YADD);
#endif
	if(temp > 0)
	{
		wind_set(ptr->whandle, WF_VSLIDE,(short)(ptr->y_pos * 1000L / temp),0,0,0);
#if USE_LOGICALRASTER==YES
		wind_set(ptr->whandle,WF_VSLSIZE,(short)((win.g_h - DL_WIN_YADD) * 1000L / ptr->y_raster / ptr->max_height),0,0,0);
#else
		wind_set(ptr->whandle,WF_VSLSIZE,(short)((win.g_h - DL_WIN_YADD) * 1000L /ptr->max_height),0,0,0);
#endif
	}
	else
	{
		wind_set(ptr->whandle,WF_VSLIDE,1000,0,0,0);
		wind_set(ptr->whandle,WF_VSLSIZE,1000,0,0,0);
	}
}

void ResizeWindow(WINDOW_DATA *ptr, WP_UNIT max_w, WP_UNIT max_h)
{
	GRECT wind;
	wind_get_grect(0,WF_WORKXYWH,&wind);
	wind_calc_grect(WC_WORK,ptr->kind,&wind,&wind);
#if USE_LOGICALRASTER==YES
	wind.g_w=(short)min(DL_WIN_XADD+max_w*ptr->x_raster,wind.g_w-(wind.g_w-DL_WIN_XADD)%ptr->x_raster);
	wind.g_h=(short)min(DL_WIN_YADD+max_h*ptr->y_raster,wind.g_h-(wind.g_h-DL_WIN_YADD)%ptr->y_raster);
#else
	wind.g_w=min(DL_WIN_XADD+max_w,wind.g_w);
	wind.g_h=min(DL_WIN_YADD+max_h,wind.g_h);
#endif
	ptr->max_width=max_w;
	ptr->max_height=max_h;

	wind_calc_grect(WC_BORDER,ptr->kind,&wind,&ptr->full);
}

void IconifyWindow(WINDOW_DATA *ptr,GRECT *r)
{
GRECT	current_size,new_pos;
	if(ptr->status & WIS_ICONIFY)
		return;

	wind_get_grect(ptr->whandle,WF_CURRXYWH,&current_size);
	wind_close(ptr->whandle);
	wind_set_grect(ptr->whandle,WF_ICONIFY,r);
	wind_set_grect(ptr->whandle,WF_UNICONIFYXYWH,&current_size);
#if SET_ICONIFY_NAME==YES
	wind_set_string(ptr->whandle,WF_NAME,iconified_name);
#endif
	wind_get_grect(ptr->whandle,WF_CURRXYWH,&new_pos);
	if(new_pos.g_x!=-1)
		graf_shrinkbox_grect(&new_pos,&current_size);

	wind_get_grect(ptr->whandle,WF_WORKXYWH,&new_pos);
	tree_addr[DIAL_LIBRARY][DI_ICON].ob_x=(new_pos.g_w-tree_addr[DIAL_LIBRARY][DI_ICON].ob_width)>>1;
	tree_addr[DIAL_LIBRARY][DI_ICON].ob_y=(new_pos.g_h-tree_addr[DIAL_LIBRARY][DI_ICON].ob_height)>>1;

	wind_open_grect(ptr->whandle,r);
	ptr->status|=WIS_ICONIFY;
	CycleItems();
}

void UniconifyWindow(WINDOW_DATA *ptr)
{
GRECT small,current_size;
	if(!(ptr->status & WIS_ICONIFY))
		return;

	wind_get_grect(ptr->whandle,WF_CURRXYWH,&small);

	if((CHAIN_DATA *)ptr==iconified_list[0])
	{
	CHAIN_DATA *ptr2;
		while(--iconified_count>0)
		{
			ptr2=iconified_list[iconified_count];
			graf_growbox_grect(&small,&ptr2->last);
			wind_open_grect(ptr2->whandle,&ptr2->last);
			ptr2->status&=~WIS_ALLICONIFY;
			ptr2->status|=WIS_OPEN;
		};
		iconified_list[0]=NULL;
		iconified_count=0;
		ptr->status&=~WIS_ALLICONIFY;
	}

	wind_get_grect(ptr->whandle,WF_UNICONIFY,&current_size);
	graf_growbox_grect(&small,&current_size);
	wind_set(ptr->whandle,WF_TOP,0,0,0,0);
#if SET_ICONIFY_NAME==YES
	wind_set_string(ptr->whandle,WF_NAME,ptr->title);
#endif
	wind_set_grect(ptr->whandle,WF_UNICONIFY,&current_size);
	ptr->status&=~WIS_ICONIFY;
}

#if USE_TOOLBAR==YES
void DrawToolbar(WINDOW_DATA *win)
{
	if(!win->toolbar)
		return;
	if(win->status & WIS_ICONIFY)
		return;
	if(win->status & WIS_OPEN)
	{
	GRECT toolbar,box;
		wind_get_grect(win->whandle, WF_WORKXYWH,&toolbar);

		win->toolbar->ob_x=toolbar.g_x;
		win->toolbar->ob_y=toolbar.g_y;
		if(win->y_offset)		/*	Horizontale Toolbar?	*/
		{
			win->toolbar->ob_width=toolbar.g_w;		/*	Objektbreite anpassen	*/
			toolbar.g_h=win->y_offset;
		}
		if(win->x_offset)		/*	Vertikale Toolbar?	*/
		{
			win->toolbar->ob_height=toolbar.g_h;	/*	Objekthoehe anpassen	*/
			toolbar.g_w=win->x_offset;
		}

		win->proc(win,WIND_TBUPDATE,&toolbar);

		wind_get_grect(win->whandle, WF_FIRSTXYWH,&box);
		graf_mouse(M_OFF,NULL);
		while(box.g_w && box.g_h)
		{
		GRECT temp_tbar;
			temp_tbar=toolbar;
			if(rc_intersect_my(&box, &temp_tbar))
				objc_draw_grect(win->toolbar,0,MAX_DEPTH,&temp_tbar);
			wind_get_grect(win->whandle, WF_NEXTXYWH,&box);
		}
		graf_mouse(M_ON,NULL);
	}
	wind_update(END_UPDATE);
}
#endif

WINDOW_DATA *find_openwindow_by_whandle(short handle)
{
WINDOW_DATA *ptr=(WINDOW_DATA *)all_list;
	while(ptr)
	{
		if((ptr->type==WIN_WINDOW)&&(ptr->whandle==handle)&&
				(ptr->status & WIS_OPEN))
			return(ptr);
		ptr=ptr->next;
	}
	return(NULL);
}

WINDOW_DATA *find_window_by_whandle(short handle)
{
WINDOW_DATA *ptr=(WINDOW_DATA *)all_list;
	while(ptr)
	{
		if((ptr->type==WIN_WINDOW)&&(ptr->whandle==handle))
			return(ptr);
		ptr=ptr->next;
	}
	return(NULL);
}

WINDOW_DATA *find_window_by_proc(HNDL_WIN proc)
{
WINDOW_DATA *ptr=(WINDOW_DATA *)all_list;
	while(ptr)
	{
		if((ptr->type==WIN_WINDOW)&&(ptr->proc==proc))
			return(ptr);
		ptr=ptr->next;
	};
	return(NULL);
}

WINDOW_DATA *find_window_by_data(void *data)
{
WINDOW_DATA *ptr=(WINDOW_DATA *)all_list;
	while(ptr)
	{
		if((ptr->type==WIN_WINDOW)&&(ptr->data==data))
			return(ptr);
		ptr=ptr->next;
	};
	return(NULL);
}

short count_window ( void )
{
short i;
WINDOW_DATA *ptr=(WINDOW_DATA *)all_list;

	i = 0;
	while(ptr)
	{
		if((ptr->type==WIN_WINDOW))
			i++;
		ptr=ptr->next;
	};
	return (i);
}
#endif
