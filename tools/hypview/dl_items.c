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

#include <gemx.h>
#include "diallib.h"

#if USE_ITEM == YES

OBJECT *iconified_tree;
short	iconified_icon=DI_ICON;
char	*iconified_name;

CHAIN_DATA	*iconified_list[MAX_ICONIFY_PLACE]={NULL};
short	iconified_count=0;

CHAIN_DATA	*all_list=NULL;

short modal_items=-1;
short modal_stack[MAX_MODALRECURSION];

void add_item(CHAIN_DATA *item);
void remove_item(CHAIN_DATA *item);
void FlipIconify(void);
void AllIconify(short handle, GRECT *r);
void CycleItems(void);
void RemoveItems(void);
void SetMenu(short enable);
void ModalItem(void);
void ItemEvent(EVNT *event);
CHAIN_DATA *find_ptr_by_whandle(short handle);
CHAIN_DATA *find_ptr_by_status(short mask, short status);


void add_item(CHAIN_DATA *item)
{
CHAIN_DATA *ptr=all_list;
	if(ptr)
	{
		while(ptr->next)
			ptr=ptr->next;
		ptr->next=item;
		item->previous=ptr;
	}
	else
	{
		all_list=item;
		item->previous=NULL;
	}
	item->next=NULL;
}

void remove_item(CHAIN_DATA *item)
{
CHAIN_DATA *ptr=all_list;

	while(ptr)
	{
		if(ptr==item)
		{
			if(ptr->next)
				ptr->next->previous=ptr->previous;
			if(ptr->previous)
				ptr->previous->next=ptr->next;
			else
				all_list=ptr->next;
			ptr=NULL;
		}
		else
			ptr=ptr->next;
	};
}

void FlipIconify(void)
{
CHAIN_DATA *ptr;
short msg[8]={WM_ICONIFY,0,0,0,-1,-1,-1,-1},d;
	if(!has_iconify)
		return;

	wind_get(0,WF_TOP, &msg[3],&d, &d, &d);
	ptr=find_ptr_by_whandle(msg[3]);
	if(!ptr)
		return;

	msg[1]=ap_id;
	if(ptr->status & WIS_ICONIFY)
	{
		msg[0]=WM_UNICONIFY;
		wind_get_grect(msg[3],WF_UNICONIFY,(GRECT *)&msg[4]);
	}

	appl_write(ap_id,16,msg);
}

void AllIconify(short handle, GRECT *r)
{
	CHAIN_DATA *ptr;
	GRECT big,small;

	ptr=find_ptr_by_whandle(handle);
	if(!ptr)
		return;

	if(iconified_list[0])
	{
		handle=iconified_list[0]->whandle;
		wind_get_grect(handle,WF_CURRXYWH,&small);
	}
	else
	{
		if(!(ptr->status & WIS_ICONIFY))
		{
			wind_get_grect(handle,WF_CURRXYWH,&big);

			if(ptr->type==WIN_DIALOG)
				wdlg_set_iconify(((DIALOG_DATA *)ptr)->dial,r,iconified_name,iconified_tree,iconified_icon);
			else if(ptr->type==WIN_WINDOW)
			{
				wind_close(ptr->whandle);
				wind_set_grect(ptr->whandle,WF_ICONIFY,r);
				wind_set_str(handle, WF_NAME, iconified_name);
				wind_set_grect(ptr->whandle,WF_UNICONIFYXYWH,&big);
				wind_open_grect(ptr->whandle,r);
			}

			ptr->status|=WIS_ALLICONIFY|WIS_ICONIFY;
		}
		else
			ptr->status|=WIS_ALLICONIFY;

		wind_get_grect(ptr->whandle,WF_CURRXYWH,r);
		graf_shrinkbox_grect(r,&big);
		iconified_list[iconified_count++]=(CHAIN_DATA *)ptr;

		small=*r;
	}

	wind_set(handle,WF_TOP,0,0,0,0);
	{
		short hndl = 0, pid, opn, above, below;
		while (wind_get(hndl, WF_OWNER, &pid, &opn, &above, &below))
		{
			ptr = find_ptr_by_whandle(hndl);
			if (ptr && !(ptr->status & WIS_ALLICONIFY) &&
			    (ptr->status & WIS_OPEN) &&
			    (iconified_count < MAX_ICONIFY_PLACE)
			    )
			{
				if ((ptr->status & WIS_OPEN))
					ptr->status |= WIS_MFCLOSE;
			}
			hndl = above;
		}
		while ((ptr = find_ptr_by_status((WIS_OPEN|WIS_MFCLOSE), (WIS_OPEN|WIS_MFCLOSE))))
		{
			ptr->status &= ~(WIS_OPEN|WIS_MFCLOSE);
			ptr->status |= WIS_ALLICONIFY;
				
			wind_get_grect(ptr->whandle, WF_CURRXYWH, &ptr->last);
			wind_close(ptr->whandle);
		}
	}
#if 0
	if((wind_get_ptr(0, WF_M_WINDLIST, (void **)&window_list)) && (window_list))
	{
#if DEBUG == ON
		Debug("WF_M_WINDLIST supported: %p", window_list);
#endif
		for(j = 0; window_list[j]; j++)
		{
			ptr = find_ptr_by_whandle(window_list[j]);
			if((ptr) && !(ptr->status & WIS_ALLICONIFY) &&
				(ptr->status & WIS_OPEN) && (iconified_count < MAX_ICONIFY_PLACE))
			{
				ptr->status &= ~WIS_OPEN;
				ptr->status |= WIS_ALLICONIFY;
				wind_get_grect(ptr->whandle,WF_CURRXYWH,&ptr->last);
				wind_close(ptr->whandle);
				graf_shrinkbox_grect(&small,&ptr->last);
				iconified_list[iconified_count++]=ptr;
				j=0;
			}
		}
	}
#if MAGIC_ONLY == NO
	else
	{
		ptr = all_list;
		while (ptr)
		{
			if(!(ptr->status & WIS_ALLICONIFY)&&
				(ptr->status & WIS_OPEN)&&(iconified_count<MAX_ICONIFY_PLACE))
			{
				ptr->status&=~WIS_OPEN;
				ptr->status|=WIS_ALLICONIFY;
				wind_get_grect(ptr->whandle,WF_CURRXYWH,&ptr->last);
				wind_close(ptr->whandle);
				graf_shrinkbox_grect(&small,&ptr->last);
				iconified_list[iconified_count++]=ptr;
			}
			ptr=ptr->next;
		}
	}
#endif
#endif
}

void CycleItems(void)
{
#if 0
	short handle = 0, our = -1, opn, owner;

	if (modal_items >= 0)
		return;

	while (wind_get(handle, WF_OWNER, &owner, &opn, &handle, NULL))
	{
		if (opn && owner == ap_id)
			our = handle;
	}
	if (our != -1)
	{
		short msg[8];
		msg[0] = WM_TOPPED;
		msg[1] = ap_id;
		msg[2] = 0;
		msg[3] = our;
		msg[4] = 0;
		msg[5] = 0;
		msg[6] = 0;
		msg[7] = 0;
		
		appl_write(ap_id, 16, msg);
	}
#endif
#if 1
	short j = 0, owner, *stack_list, hi, lo;
	short msg[]={WM_TOPPED,0,0,0,0,0,0,0};
	
	if(modal_items >= 0)
		return;
	
	msg[1] = ap_id;

	if ( wind_get ( 0, WF_M_WINDLIST, &hi, &lo, NULL, NULL ) )
	{
		stack_list = (short *)((hi << 16) | lo);
	  if ( stack_list )
		{
			while(stack_list[j])
				j++;
	
			for(; j > 0; j--)
			{
				wind_get(stack_list[j],WF_OWNER,&owner,NULL,NULL,NULL);
				if(owner == ap_id)
				{
					msg[3] = stack_list[j];
					appl_write(ap_id, 16, msg);
					break;
				}
			}
		}
	}
#if MAGIC_ONLY == NO
	else
	{
		short handle;
		CHAIN_DATA *start_ptr,*ptr;
		wind_get(0,WF_TOP,&handle,NULL,NULL,NULL);
		start_ptr=find_ptr_by_whandle(handle);
		if(!start_ptr)
			return;
		ptr=start_ptr;
		do
		{
			ptr=ptr->next;
			if(!ptr)
				ptr=all_list;

			if(ptr->status & WIS_OPEN)
			{
				msg[3]=ptr->whandle;
				appl_write(ap_id,16,msg);
			}
		}while((ptr!=start_ptr) && !(ptr->status & WIS_OPEN));
	}
#endif
#endif
}

void RemoveItems(void)
{
	while(all_list)
	{
		if(all_list->type==WIN_WINDOW)
#if USE_WINDOW == YES
			RemoveWindow((WINDOW_DATA *)all_list);
#else
			;
#endif
		else if(all_list->type==WIN_DIALOG)
#if USE_DIALOG == YES
			RemoveDialog((DIALOG_DATA *)all_list);
#else
			;
#endif
		else if(all_list->type==WIN_FILESEL)
#if USE_FILESELECTOR == YES
			RemoveFileselector((FILESEL_DATA *)all_list);
#else
			;
#endif
		else if(all_list->type==WIN_FONTSEL)
#if USE_FONTSELECTOR == YES
			RemoveFontselector((FONTSEL_DATA *)all_list);
#else
			;
#endif
	}
}

#if USE_MENU == YES
short menu_enabled=TRUE;

void SetMenu(short enable)
{
short title,width,last;
	if(menu_tree)
	{
		title=menu_tree[3].ob_next;
		if(enable == ((menu_tree[title].ob_state & DISABLED)!=0))
		{
			width=menu_tree[3].ob_width;
			do
			{
				width+=menu_tree[title].ob_width;
				menu_tnormal(menu_tree,title,TRUE);
				menu_ienable(menu_tree,title | 0x8000,enable);
				last=title;
				title=menu_tree[title].ob_next;
			}while(title != 2);
			menu_ienable(menu_tree,last+3,enable);
			if(enable)
				menu_tree[2].ob_width=width;
			else
				menu_tree[2].ob_width=menu_tree[3].ob_width;

			menu_enabled=enable;
		}
	}
}
#endif

void ModalItem(void)
{
	wind_get(0, WF_TOP, &modal_stack[++modal_items], NULL,NULL,NULL);
#if USE_MENU == YES
	SetMenu(FALSE);
#endif
}

void ItemEvent(EVNT *event)
{
	short whandle = 0, top_window, find_window;
	CHAIN_DATA *ptr;
	
	wind_get(0, WF_TOP, &top_window, NULL,NULL,NULL);
	find_window = wind_find(event->mx,event->my);

	if(modal_items >= 0)
	{
		whandle = modal_stack[modal_items];
		if((event->mwhich & MU_MESAG)&&
			((event->msg[0] == WM_REDRAW) || (event->msg[0] == WM_MOVED)))
			whandle = event->msg[3];
		else if((event->mwhich & MU_BUTTON) && (top_window != whandle))
			wind_set(whandle, WF_TOP,0,0,0,0);
	}
	else
	{
		if(event->mwhich & MU_MESAG)
		{
			if((event->msg[0]>=WM_REDRAW)&&(event->msg[0]<=WM_ALLICONIFY))
				whandle=event->msg[3];
		}
		else if(event->mwhich & MU_BUTTON)
			whandle=find_window;
		else
			whandle=top_window;
	}

	ptr=find_ptr_by_whandle(whandle);
	if(ptr && (ptr->status & WIS_OPEN))
	{
		if(ptr->type==WIN_WINDOW)
#if USE_WINDOW == YES
			WindowEvents((WINDOW_DATA *)ptr,event);
#else
			;
#endif
		else if(ptr->type==WIN_DIALOG)
#if USE_DIALOG == YES
			DialogEvents((DIALOG_DATA *)ptr,event);
#else
			;
#endif
		else if(ptr->type==WIN_FILESEL)
#if USE_FILESELECTOR == YES
			FileselectorEvents((FILESEL_DATA *)ptr,event);
#else
			;
#endif
		else if(ptr->type==WIN_FONTSEL)
#if USE_FONTSELECTOR == YES
			FontselectorEvents((FONTSEL_DATA *)ptr,event);
#else
			;
#endif
	}

#if USE_MENU == YES
	if((modal_items<0) && !menu_enabled)
		SetMenu(TRUE);
#endif
}

CHAIN_DATA *
find_ptr_by_whandle(short handle)
{
	CHAIN_DATA *ptr = all_list;
	
	while (ptr)
	{
		if (ptr->whandle==handle)
			return(ptr);
		ptr=ptr->next;
	}
	return(NULL);
}

CHAIN_DATA *
find_ptr_by_status(short mask, short status)
{
	CHAIN_DATA *ptr = all_list;

	while (ptr)
	{
		if ((ptr->status & mask) == status)
			break;
		ptr = ptr->next;
	}
	return ptr;
}

#endif
