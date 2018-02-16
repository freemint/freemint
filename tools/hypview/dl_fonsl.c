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
 
#include <gem.h>
#include "diallib.h"

#if USE_FONTSELECTOR == YES

FONTSEL_DATA *CreateFontselector(HNDL_FONT proc,short font_flag,char *sample_text,char *opt_button);
short OpenFontselector(FONTSEL_DATA *ptr,short button_flag,long id,long pt,long ratio);
void CloseFontselector(FONTSEL_DATA *ptr);
void RemoveFontselector(FONTSEL_DATA *ptr);
void FontselectorEvents(FONTSEL_DATA *ptr,EVNT *event);


FONTSEL_DATA *CreateFontselector(HNDL_FONT proc,short font_flag,char *sample_text,char *opt_button)
{
FONTSEL_DATA *ptr=NULL;
	if(has_wlffp & 4)
	{
	short i,tmp_workout[57];
		ptr=(FONTSEL_DATA *)Mxalloc(sizeof(FONTSEL_DATA),3);
		if(ptr==NULL)
		{
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
			return(NULL);
		}
		memset(ptr,0,sizeof(FONTSEL_DATA));
		
		for(i=0;i<10;tmp_workout[i++]=1);
		tmp_workout[10]=2;
		ptr->vdi_handle=aes_handle;
		v_opnvwk(tmp_workout,&ptr->vdi_handle,tmp_workout);
		if(!ptr->vdi_handle)
		{
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_VDI_WKS_ERROR].ob_spec.free_string);
			Mfree(ptr);
			return(NULL);
		}

		ptr->dialog=fnts_create(ptr->vdi_handle,0,font_flag,FNTS_3D,sample_text,opt_button);
		if(ptr->dialog)
		{
			ptr->type=WIN_FONTSEL;
			ptr->proc=proc;
			ptr->font_flag=font_flag;
			ptr->opt_button=opt_button;
			ptr->last.g_x=-1;
			ptr->last.g_y=-1;
			ptr->status=0;

			add_item((CHAIN_DATA *)ptr);
		}
		else
		{
			v_clsvwk(ptr->vdi_handle);
			Mfree(ptr);
			ptr=NULL;
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
		}
	}
	else
		form_alert(1,tree_addr[DIAL_LIBRARY][DI_WDIALOG_ERROR].ob_spec.free_string);
	return(ptr);
}

short OpenFontselector(FONTSEL_DATA *ptr,short button_flag,long id,long pt,long ratio)
{
	if((ptr==NULL)||(!ptr->dialog))
		return(0);

	ptr->whandle=fnts_open(ptr->dialog, button_flag,ptr->last.g_x,ptr->last.g_y,id,pt,ratio);

	if(ptr->whandle>0)
	{
		ptr->type=WIN_FONTSEL;
		ptr->id=id;
		ptr->pt=pt;
		ptr->ratio=ratio;
		ptr->status|=WIS_OPEN;
	}
	return(ptr->whandle);
}

void CloseFontselector(FONTSEL_DATA *ptr)
{
	if(!ptr->dialog)
		return;

	fnts_close(ptr->dialog,&ptr->last.g_x,&ptr->last.g_y);
	ptr->status&=~WIS_OPEN;
	ptr->whandle=0;
	if(modal_items>=0)
		modal_items--;
}

void RemoveFontselector(FONTSEL_DATA *ptr)
{
short x,y;
	fnts_close(ptr->dialog,&x,&y);
	fnts_delete(ptr->dialog,ptr->vdi_handle);
	if(modal_items>=0)
		modal_items--;
	remove_item((CHAIN_DATA *)ptr);
	v_clsvwk(ptr->vdi_handle);
	Mfree(ptr);
}

void FontselectorEvents(FONTSEL_DATA *ptr,EVNT *event)
{
	if(!fnts_evnt(ptr->dialog,event,&ptr->button,&ptr->check_boxes,
			&ptr->id, &ptr->pt, &ptr->ratio))
	{
		if(!ptr->proc(ptr))
			CloseFontselector(ptr);
	}
}

#endif
