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
#include "include/scancode.h"
#include "diallib.h"
#include "defs.h"
#include "hyp.h"

extern WINDOW_DATA *Win;

short ProportionalFont(short *width)
{
#define PF_START	32
#define PF_END		155
	char str[2];
	short ext[8];
	short i=PF_START;
	short isProp=FALSE;
	short last_width=0;
	long w=0;
	
	str[1]=0;
	do
	{
		*str=i++;
		vqt_extent(vdi_handle,str,ext);

		if(last_width && last_width!=(ext[2]-ext[1]))
			isProp=TRUE;

		last_width=ext[2]-ext[1];
		w+=last_width;

	}while(i<PF_END);
	*width=(short)(w/(PF_END-PF_START)+1);
	return(isProp);
}

void SwitchFont(DOCUMENT *doc)
{
	DOCUMENT *ptr;
	WINDOW_DATA *win;
	
	if(sel_font_id==font_id)
	{
		sel_font_id=xfont_id;
		sel_font_pt=xfont_pt;
	}
	else
	{
		sel_font_id=font_id;
		sel_font_pt=font_pt;
	}
	vst_font(vdi_handle,sel_font_id);
	vst_point(vdi_handle,sel_font_pt,&font_w,&font_h,&font_cw,&font_ch);

/*	Debug("font_w: %d font_h: %d font_cw: %d font_ch: %d",font_w,font_h,font_cw,font_ch);
*/	
	if(ProportionalFont(&font_w))
	{
		font_cw=font_w;
/*		Debug("ProportsFont true => w %d",font_cw);
*/	}

	/*	Alle geoeffneten Fenster und Dokumente anpassen	*/
	win = (WINDOW_DATA *)all_list;
	
	while (win)
	{
		if(win->type==WIN_WINDOW)
		{
			ptr=win->data;
			/*	Seite/Datei neuladen	*/
			graf_mouse(BUSYBEE,NULL);
			
			if(doc->type==F_HYP)
			{
			long node=doc->getNodeProc(doc);
				/*	Nodes aus dem Cache entfernen	*/
				RemoveNodes(doc->data);
				
				/*	Seite neu laden	*/
				doc->gotoNodeProc(doc,NULL,node);
			}
			else
				doc->gotoNodeProc(doc,NULL,doc->getNodeProc(doc));

			ptr->start_line=win->y_pos;
			graf_mouse(ARROW,NULL);
		
			/*	Fuller-Status "vergessen"	*/
			win->status&=~WIS_FULL;

			/*	Fenster neu initialisieren	*/
			ReInitWindow(win,ptr);
		}
		win = win->next;
	}

	/*	Das "Event-Fenster" wieder eintragen	*/
	Win = find_window_by_data(doc);
}


