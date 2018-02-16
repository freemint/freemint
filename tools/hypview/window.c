/*
 * HypView - (c) 2001 - 2006 Philipp Donze
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
#include "include/scancode.h"
#include "diallib.h"
#include "defs.h"
#include "hyp.h"

extern WINDOW_DATA *Win;

void SendClose(WINDOW_DATA *win)
{
	short msg[8] = {WM_CLOSED,0,0,0,0,0,0,0};
	msg[1] = ap_id;
	msg[3] = win->whandle;
	appl_write(ap_id,16,msg);
}

void SendRedraw(WINDOW_DATA *win)
{
short msg[8]={WM_REDRAW,0,0,0,0,0,0,0};
	msg[1] = ap_id;
	msg[3] = win->whandle;
	wind_get_grect(win->whandle, WF_WORKXYWH, (GRECT *)&msg[4]);
	appl_write(ap_id, 16, msg);
}

void ReInitWindow(WINDOW_DATA *win, DOCUMENT *doc)
{
	short old_rx = win->x_raster;
	short old_ry = win->y_raster;
	short visible_lines;
	GRECT curr;
	
	win->data = doc;
	win->title = doc->window_title;
	win->x_raster = font_cw;
	win->y_raster = font_ch;
	wind_set_str(win->whandle,WF_NAME,win->title);
	doc->selection.valid=FALSE;

	/*	Fenstergroesse: mindestens 5 Kolonnen und eine Zeile	*/
	ResizeWindow(win, max(doc->columns,5), max(doc->lines,1));

	/*	Breite und Hoehe des Fensters den neuen Ausmassen anpassen	*/
	if (adjust_winsize)
	{
		GRECT screen;

		wind_get_grect(0,WF_WORKXYWH, &screen);
		wind_get_grect(win->whandle,WF_CURRXYWH, &curr);
		
		curr.g_w = win->full.g_w;
		curr.g_h = win->full.g_h;

		wind_calc_grect(WC_WORK, win->kind, &curr, &curr);
		if(curr.g_w < tree_addr[TOOLBAR][TO_SEARCHBOX].ob_width)
			curr.g_w = tree_addr[TOOLBAR][TO_SEARCHBOX].ob_width;

		if(curr.g_h < win->y_offset)
			curr.g_y = win->y_offset + font_ch;
		wind_calc_grect(WC_BORDER, win->kind, &curr, &curr);

		if(curr.g_x + curr.g_w > screen.g_x + screen.g_w)
		{
			short val = curr.g_x - screen.g_x + curr.g_w - screen.g_w;
			curr.g_w -= (short)(val / font_cw + 1) * font_cw;
		}

		if(curr.g_y+curr.g_h>screen.g_y+screen.g_h)
		{
			short val = curr.g_y - screen.g_y + curr.g_h - screen.g_h;
			curr.g_h -= (short)(val / font_ch + 1) * font_ch;
		}

		wind_set_grect(win->whandle, WF_CURRXYWH,&curr);
	}
	else
	{
		wind_get_grect(win->whandle, WF_WORKXYWH, &curr);

		if(old_rx < win->x_raster)
			curr.g_w += win->x_raster - 1;
		curr.g_w -= curr.g_w % win->x_raster;
		if(old_ry < win->y_raster)
			curr.g_h += win->y_raster - 1;
		curr.g_h -= (curr.g_h - win->y_offset) % win->y_raster;

		wind_calc_grect(WC_BORDER, win->kind, &curr, &curr);
		wind_set_grect(win->whandle, WF_CURRXYWH, &curr);
	}
	
	wind_get_grect(win->whandle,WF_WORKXYWH, &curr);
	visible_lines = (curr.g_h - win->y_offset) / win->y_raster;
	
	win->y_pos = min(doc->lines - visible_lines, doc->start_line);
	win->y_pos = max(0, win->y_pos);
	win->x_pos = 0;


	if(!intelligent_fuller)
		wind_get_grect(0, WF_WORKXYWH, &win->full);

	SetWindowSlider(win);
	ToolbarUpdate(doc,win->toolbar, FALSE);
	SendRedraw(win);
}


short
HelpWindow(WINDOW_DATA *p, short obj, void *data)
{
	DOCUMENT *doc = (DOCUMENT *)p->data;
	WINDOW_DATA *win;

	Win = win = p;

	if(obj == WIND_INIT)
	{
		/*	Als Rasterweite werden die Font-Groessen benutzt	*/
		win->x_raster = font_cw;
		win->y_raster = font_ch;

		/*	Toolbar installieren	*/
		win->toolbar = tree_addr[TOOLBAR];
		win->x_offset = 0;
		win->y_offset = tree_addr[TOOLBAR][0].ob_height + 2;

		win->title = doc->window_title;

		/*	Fenstergrsse: mindestens 5 Kolonnen und eine Zeile	*/
		win->max_width = max(doc->columns, 5);
		win->max_height = max(doc->lines, 1);

		win->x_pos = 0;
		win->y_pos = doc->start_line;

		DhstAddFile(doc->path);
	}
	else if(obj == WIND_EXIT)
	{
		if(doc->popup)		/*	Falls noch ein Popup offen ist...	*/
			RemoveWindow(doc->popup);	/*	... schliessen	*/
		CloseFile(doc);
		RemoveAllHistoryEntries(win);
		win = NULL;
	}
	else if(obj == WIND_OPEN)
	{
		graf_mouse(ARROW,NULL);

		if(!server_cfg)					/*	AV noch nicht initialisiert?	*/
			DoAV_PROTOKOLL(AV_P_QUOTING|AV_P_VA_START|AV_P_AV_STARTED);

		DoAV_ACCWINDOPEN(win->whandle);

		if(!intelligent_fuller)
			wind_get_grect(0, WF_WORKXYWH, &win->full);
	}
	else if(obj == WIND_OPENSIZE)	/*	Fenstergrsse beim ffnen	*/
	{
		GRECT *open_size = (GRECT *)data;
		GRECT screen;
		wind_get_grect(0, WF_WORKXYWH, &screen);

		if(!adjust_winsize)
			*open_size = screen;

		/*	Default X-Koordinate angegeben?	*/
		if(win_x && (win_x <= screen.g_x + screen.g_w - 50))
		{
			if(win_x + open_size->g_w>screen.g_x + screen.g_w)
			{
				short val = win_x - open_size->g_x + open_size->g_w - screen.g_w;
				open_size->g_w -= (short)(val / font_cw + 1) * font_cw;
			}
			open_size->g_x = win_x;
		}

		/*	Default Y-Koordinate angegeben?	*/
		if(win_y && (win_y<=screen.g_y+screen.g_h-70))
		{
			if(win_y + open_size->g_h>screen.g_y + screen.g_h)
			{
			short val=win_y-open_size->g_y+open_size->g_h-screen.g_h;
				open_size->g_h-=(short)(val/font_ch+1)*font_ch;
			}
			open_size->g_y=win_y;
		}

		/*	Keine automatische Groessenanpassung?
			Und Fenster-Breite oder Fensterhoehe definiert?	*/
		if(!adjust_winsize && (win_w || win_h))
		{
			wind_calc_grect(WC_WORK, win->kind, open_size, open_size);
			
			if(win_w && (win_w*font_cw<open_size->g_w))
				open_size->g_w=win_w*font_cw;
			else
				open_size->g_w-=open_size->g_w%font_cw;

			if(win_h && (win_h*font_ch<open_size->g_h))
				open_size->g_h = win->y_offset+win_h*font_ch;
			else
				open_size->g_h -= (open_size->g_h - win->y_offset)%font_ch;

			wind_calc_grect(WC_BORDER, win->kind,open_size,open_size);

			return(0);
		}
		else
		{
			wind_calc_grect(WC_WORK, win->kind, open_size, open_size);
			if(open_size->g_w<tree_addr[TOOLBAR][TO_SEARCHBOX].ob_width)
				open_size->g_w=tree_addr[TOOLBAR][TO_SEARCHBOX].ob_width;

			if(open_size->g_h<win->y_offset+font_ch)
				open_size->g_y=win->y_offset+font_ch;
			wind_calc_grect(WC_BORDER, win->kind,open_size,open_size);
			return(0);
		}
	}
	else if(obj==WIND_CLOSE)
	{
		DoAV_ACCWINDCLOSED(win->whandle);
	}
	else if(obj==WIND_REDRAW)
	{
		short pxy[4];
		GRECT *box = (GRECT *)data;

		pxy[0] = box->g_x;
		pxy[1] = box->g_y;
		pxy[2] = box->g_x+box->g_w - 1;
		pxy[3] = box->g_y+box->g_h - 1;

		vsf_color(vdi_handle, background_col);
		vsf_interior(vdi_handle, FIS_SOLID);
		vswr_mode(vdi_handle, MD_REPLACE);
		vr_recfl(vdi_handle, pxy);					/*	Hintergrund Rechteck	*/

		vs_clip(vdi_handle, 1, pxy);					/*	Clipping ON	*/

		doc->displayProc(doc);
		DrawSelection(doc);

		vs_clip(vdi_handle, 0, pxy);					/*	Clipping OFF	*/
	}
	else if(obj == WIND_SIZED)						/*	Fenstergroesse gendert	*/
	{
		GRECT *out = (GRECT *)data,in;
		wind_calc_grect(WC_WORK,win->kind,out,&in);	/*	Arbeitsbereich berechnen	*/

		in.g_w -= in.g_w % win->x_raster;	/*	Breite auf Rastergrsse "stutzen"	*/
		in.g_h -= (in.g_h - win->y_offset) % win->y_raster;	/*	Hhe auf Rastergroesse "stutzen"	*/

		wind_calc_grect(WC_BORDER, win->kind, &in,out);	/*	Fensterrand berechnen	*/
	}
	else if(obj == WIND_FULLED)						/*	Fuller angeklickt?	*/
	{
		GRECT *out = (GRECT *)data,in;
		GRECT screen;
		wind_get_grect(0,WF_WORKXYWH,&screen);
	
		wind_calc_grect(WC_WORK,win->kind,out,&in);	/*	Arbeitsbereich berechnen	*/

		/*	Falls Vergroessern: Verhindern, das Toolbar unsichtbar wird	*/
		if(((win->status & WIS_FULL) == 0) &&
				in.g_w < tree_addr[TOOLBAR][TO_SEARCHBOX].ob_width)
		{
			in.g_w = tree_addr[TOOLBAR][TO_SEARCHBOX].ob_width;
		}

		in.g_w-=in.g_w%win->x_raster;	/*	Breite auf Rastergroesse "stutzen"	*/
		in.g_h-=(in.g_h-win->y_offset)%win->y_raster;	/*	Hhe auf Rastergroesse "stutzen"	*/

		wind_calc_grect(WC_BORDER,win->kind,&in,out);	/*	Fensterrand berechnen	*/

		if(intelligent_fuller && ((win->status & WIS_FULL) == 0))
		{
			out->g_x=win->last.g_x;
			out->g_y=win->last.g_y;
			if(out->g_x+out->g_w>screen.g_x+screen.g_w)
				out->g_x-=out->g_x+out->g_w-(screen.g_x+screen.g_w);
			else if(out->g_x<screen.g_x)
				out->g_x=screen.g_x;
			if(out->g_y+out->g_h>screen.g_y+screen.g_h)
				out->g_y-=out->g_y+out->g_h-(screen.g_y+screen.g_h);
			else if(out->g_y<screen.g_y)
				out->g_y=screen.g_y;
		}
	}
	else if (obj == WIND_KEYPRESS)
	{
		EVNT *event=(EVNT *)data;
		short scan=(event->key>>8) & 0xff;
		short ascii=event->key & 0xff;

		KeyboardOnOff();			/*	Tastatur-Repeat aus	*/
		wind_get_grect(win->whandle,WF_WORKXYWH, &win->work);

		event->mwhich&=~MU_KEYBD;

		if( (event->kstate & KbSHIFT) && (event->kstate & KbCTRL))	/*	Tastenkomb. mit SHIFT + CTRL	*/
		{
			if(scan==KbUP || scan==KbLEFT)
				ToolbarClick(doc,TO_PREVIOUS);
			else if(scan==KbDOWN || scan==KbRIGHT)
				ToolbarClick(doc,TO_NEXT);
			else if(ascii=='V')
				BlockPaste(!clipbrd_new_window);
			else
				event->mwhich|=MU_KEYBD;
		}
		else if (event->kstate & KbSHIFT)		/*	Tastenkomb. mit SHIFT			*/
		{
			/*	Toolbarhhe abziehen	*/
			win->work.g_h -= win->y_offset;

			if(scan==KbLEFT)
			{
				short val =- win->work.g_w / win->x_raster;
				ScrollWindow(win,&val,NULL);
			}
			else if(scan==KbRIGHT)
			{
				short val = win->work.g_w / win->x_raster;
				ScrollWindow(win,&val,NULL);
			}
			else if(scan==KbUP)
			{
				short val =- win->work.g_h/win->y_raster;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbDOWN)
			{
			short val=win->work.g_h/win->y_raster;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbHOME)
			{
				win->x_pos=0;
				win->y_pos=win->max_height-win->work.g_h/win->y_raster;

				SetWindowSlider(win);
				SendRedraw(win);
			}
			else if ( scan >= KbF11 && scan <= KbF20 )
				MarkerSave ( doc, scan-KbF11 );
			else 
				event->mwhich|=MU_KEYBD;
		}
		else if(event->kstate & KbALT)		/*	Tastenkomb. mit Alt		*/
		{
			if(ascii=='D')
			{
				if(*default_file)
					OpenFileSW(default_file,NULL, 0);
			}
			else if(ascii=='E')
			{
				RemoveAllHistoryEntries(win);
				ToolbarUpdate(doc,win->toolbar,TRUE);
			}
			else if(ascii=='K')
				OpenFileSW(catalog_file,NULL,0);
			else if(ascii=='T')
				GoThisButton(doc,TO_HOME);
			else if(ascii=='X')
				GotoIndex(doc);
		}
		else if(event->kstate & KbCTRL)		/*	Tastenkomb. mit CTRL	*/
		{
			if(ascii=='A')
				SelectAll(doc);
			else if(ascii=='C')
				BlockCopy(doc);
			else if(ascii=='I')
				ProgrammInfos( doc );
			else if(ascii=='F')
				Hypfind( doc );
			else if(ascii=='O')
				ToolbarClick(doc,TO_LOAD);
			else if(ascii=='V') {
				if(doc->buttons & BITVAL(TO_SEARCHBOX))
					AutoLocatorPaste(doc);
				else
					BlockPaste(clipbrd_new_window);
			}
			else if(ascii=='Z')
				SwitchFont(doc);
			else if(scan==KbUP)
			{
				short val =- win->work.g_h / win->y_raster;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbDOWN)
			{
			short val=win->work.g_h/win->y_raster;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbLEFT)
				ToolbarClick(doc,TO_PREVIOUS);
			else if(scan==KbRIGHT)
				ToolbarClick(doc,TO_NEXT);
			else if ( scan >= KbF1 && scan <= KbF10 )
				MarkerShow ( doc, scan-KbF1, 1 );
			else
				event->mwhich|=MU_KEYBD;
		}
		else if(event->kstate & KbNUM)		/*	Tasten auf dem Ziffernblock	*/
		{
			if(ascii=='-')
				ToolbarClick(doc,TO_PREVIOUS);
			else if(ascii=='+')
				ToolbarClick(doc,TO_NEXT);
			else
				event->mwhich|=MU_KEYBD;
		}
		else if(event->kstate==0)
		{
			if(scan==KbLEFT)
			{
			short val=-win->x_speed;
				ScrollWindow(win,&val,NULL);
			}
			else if(scan==KbRIGHT)
			{
			short val=win->x_speed;
				ScrollWindow(win,&val,NULL);
			}
			else if(scan==KbUP)
			{
			short val=-win->y_speed;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbDOWN)
			{
			short val=win->y_speed;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbPAGEUP)
			{
			short val=-(win->work.g_h - win->y_offset) /win->y_raster;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbPAGEDOWN)
			{
			short val=(win->work.g_h - win->y_offset)/win->y_raster;
				ScrollWindow(win,NULL,&val);
			}
			else if(scan==KbHOME)
			{
				if(win->y_pos)
				{
					win->x_pos=0;
					win->y_pos=0;
					SetWindowSlider(win);
					SendRedraw(win);
				}
			}
			else if(scan==KbEND)
			{
				win->x_pos=0;
				win->y_pos=win->max_height-(win->work.g_h - win->y_offset)/win->y_raster;

				SetWindowSlider(win);
				SendRedraw(win);
			}
			else if(scan==KbHELP)
				ToolbarClick(doc,TO_HELP);
			else if(scan==KbUNDO)
				ToolbarClick(doc,TO_BACK);
			else if ( scan >= KbF1 && scan <= KbF10 )
				MarkerShow ( doc, scan-KbF1, 0 );
			else if((ascii==27) && !(doc->buttons & BITVAL(TO_SEARCHBOX)))
				ToolbarClick(doc,TO_BACK);
			else
				event->mwhich|=MU_KEYBD;
		}
		else
			event->mwhich|=MU_KEYBD;


		if(event->mwhich & MU_KEYBD)
		{
			if(AutolocatorKey(doc, event->kstate, ascii))
				event->mwhich&=~MU_KEYBD;
		}

		KeyboardOnOff();			/*	Tastatur-Repeat wieder an	*/
	}
	else if(obj==WIND_DRAGDROPFORMAT)
	{
	short i;
	long *format=data;
		for(i=0;i<MAX_DDFORMAT;i++)
			format[i]=0L;

		format[0] = 0x41524753L; /* 'ARGS' */
	}
	else if(obj==WIND_DRAGDROP)
	{
		char *ptr = data,*chapter;
		chapter = ParseData(ptr);
		OpenFileSW(ptr,chapter,0);
	}
	else if(obj==WIND_CLICK)
	{
		EVNT *event=(EVNT *)data;

		if(doc->popup)				/*	Popup aktiv?	*/
			SendClose(doc->popup);

		RemoveSearchBox(doc);

		if(event->mbutton & 1)		/*	Links-Klick	*/
		{
			graf_mkstate(&event->mx, &event->my, &event->mbutton, &event->kstate);

			if(check_time)
				CheckFiledate(doc);

			if((event->mbutton & 1) ||			/*	Maustaste noch gedrckt?	*/
					(event->kstate & KbSHIFT))	/*	oder Shift gedrckt?	*/
				MouseSelection(doc,(EVNTDATA *)&event->mx);
			else
			{
				RemoveSelection(doc);
				doc->clickProc(doc,(EVNTDATA *)&event->mx);
			}
		}
		else if(event->mbutton & 2)	/*	Rechts-Klick	*/
		{
			if ( has_wlffp!=0 )
			{
				short num;
				num = form_popup(tree_addr[CONTEXT],event->mx,event->my);
				BlockOperation(doc, num);
			}
		}
	}
	else if(obj == WIND_TBUPDATE)
	{
		DOCUMENT **id = (DOCUMENT **)&(tree_addr[TOOLBAR][TO_ID].ob_spec.free_string);
		if(*id == doc)
			return(1);

		ToolbarUpdate(doc, win->toolbar, FALSE);
		*id=doc;
	}
	else if(obj==WIND_TBCLICK)
	{
		ToolbarClick(doc,*(short *)data);
	}
	else if(obj==WIND_ICONIFY)
		wind_set_str(win->whandle,WF_NAME,doc->filename);
	else if(obj==WIND_UNICONIFY)
		wind_set_str(win->whandle,WF_NAME,win->title);
	else if(obj==WIND_TOPPED)
	{
		if(doc->popup)				/*	Popup aktiv?	*/
		{
		short top;
			wind_get(0,WF_TOP,&top,NULL,NULL,NULL);

			if(top!=doc->popup->whandle)
			{
				wind_set(win->whandle,WF_TOP,0,0,0,0);
				wind_set(doc->popup->whandle,WF_TOP,0,0,0,0);
				return(0);
			}
			else
				SendClose(doc->popup);
		}
		if(check_time)
			CheckFiledate(doc);
	}
	return(1);
}
