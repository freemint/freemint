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

#include <stdio.h>
#include <string.h>
#ifdef __GNUC__
	#include <osbind.h>
#else
	#include <tos.h>
#endif
#include <gemx.h>
#include "include/scancode.h"
#include "include/av.h"
#include "diallib.h"
#include "defs.h"
#include "hyp.h"
#include "bubble/bgh.h"
 
#define AC_HELP		1025	/*	PureC Hilfe-Protokoll */

extern WINDOW_DATA *Win;

long last_node;
char last_path[DL_PATHMAX];
HISTORY *last_history = NULL;

/*******************************************************/
/****** Events                                    ******/
/*******************************************************/
#if USE_GLOBAL_KEYBOARD == YES

short DoUserKeybd(short kstate, short scan, short ascii)
{
	if(kstate==KbCTRL)
	{
		switch(ascii)
		{
			case 'O':
				return(TRUE);
		}
	}
	return(FALSE);
}
#endif

void DoButton(EVNT *event)
{
#if USE_BUBBLEGEM==YES
	if(event->mbutton==2)
		Bubble(event->mx,event->my);
#endif
}

#if USE_USER_EVENTS == YES

void DoUserEvents(EVNT *event)
{
	if(event->mwhich & MU_MESAG)
	{
		if(event->msg[0]==AC_OPEN)
		{
			short ret=0;
			
			if ( count_window () == 0 && last_path[0] != '\0' )
			{
				ret=OpenFileNW(last_path, NULL, last_node);
				if(ret)
				{
/* ----
					SetLastHistory (Win,last_history);
 					DeletLastHistory(last_history);
 					last_history=NULL;
--- */
				}
			}

			if (ret==0)
			{
				if(*default_file)
					OpenFileNW(default_file,NULL, 0);
				else
					SelectFileLoad();
			}

			event->mwhich&=~MU_MESAG;
		}
		else if(event->msg[0]==AC_CLOSE)
		{
			event->mwhich&=~MU_MESAG;
			RemoveItems();
		}
		else if(event->msg[0]==AC_HELP)
		{
			char *data = *(char **)&event->msg[3];
			if(data!=NULL)
			{
#if DEBUG==ON
				Debug("AC_HELP from %d:",event->msg[1]);
				Debug("  <%s>",data);
#endif
				search_allref(data, FALSE );
			}
			event->mwhich&=~MU_MESAG;
		}
		else if(event->msg[0]==WM_CLOSED)
		{		/* Dies ist ein Hack um das letzte Fenster zufinden */
				/* MU_MESAG darf am Ende nicht maskiert werden sonst*/
				/* wird das Fenster nicht geschlossen!				*/
			CHAIN_DATA *ptr;
			
			ptr=find_ptr_by_whandle(event->msg[3]);
			if(ptr && (ptr->status & WIS_OPEN) && ptr->type==WIN_WINDOW )
			{
				if ( count_window () == 1 )
				{
					WINDOW_DATA *data;
					 data = find_window_by_whandle(event->msg[3]);
					 if ( data!=NULL )
					 {
						DOCUMENT *doc;
						
						doc = (DOCUMENT *) data->data;
						last_node=doc->getNodeProc(doc);
						strcpy ( last_path, doc->path );
						last_history = (HISTORY *)GetLastHistory();
					 }
				}

			}
		}
		else if(event->msg[0]==CH_EXIT)
		{
			HypfindFinsih ( event->msg[3], event->msg[4] );
			event->mwhich&=~MU_MESAG;
		}

	}
	else if(event->mwhich & MU_KEYBD)
	{
	short ascii=event->key;
	short kstate=event->kstate;

		ConvertKeypress(&ascii,&kstate);

		ascii=ascii & 0xff;

		if(kstate & KbCTRL)
		{
			if(ascii == 'W')
			{
				short global_cycle = (kstate & KbSHIFT ? !av_window_cycle : av_window_cycle);
				
				if(server_cfg && global_cycle)	/*	AV initialisiert?	*/
				{
					DoAV_SENDKEY(event->kstate, event->key);
					event->mwhich&=~MU_KEYBD;
				}
				else
				{
					CycleItems();
					event->mwhich&=~MU_KEYBD;
				}
			}
		}
	}
}
#endif



/*******************************************************/
/****** Men-Auswahl                              ******/
/*******************************************************/
#if USE_MENU == YES
void SelectMenu(short title, short entry)
{
	switch(title)
	{
		case ME_PROGRAM:
			switch(entry)
			{
				case ME_ABOUT:
					OpenDialog(HandleAbout,tree_addr[ABOUT],string_addr[WDLG_ABOUT],-1,-1);
			 		break;
			}
	 		break;
		case ME_FILE:
			switch(entry)
			{
				case ME_QUIT:
					doneFlag=TRUE;
			 		break;
			}
			break;
	}
}
#endif



/*******************************************************/
/****** Dialogobjekte mit Langen Editfeldern      ******/
/*******************************************************/
#if USE_LONGEDITFIELDS == YES
LONG_EDIT long_edit[]=
{
	{MAIN,MA_EDIT,20},
	{ABOUT,AB_EDIT,40}
};

short long_edit_count=(short)(sizeof(long_edit)/sizeof(LONG_EDIT));
#endif



/*******************************************************/
/****** Drag&Drop Protokoll                       ******/
/*******************************************************/
#if USE_DRAGDROP == YES
void DD_Object(DIALOG *dial,GRECT *rect,OBJECT *tree,short obj, char *data, unsigned long format)
/*
	Hier koennen je nach Zielobjekt <obj> die D&D-Daten <data> anders
	ausgewertet werden. <data> hat eines der gewuenschten Formate
	(<format>).
	Um <data> in seine Einzelbestaende zu zerlegen muss ParseData ver-
	wendet werden.
*/
{
char *next,*ptr=data;
	do
	{
		next=ParseData(ptr);
#if DEBUG==ON
		Debug(ptr);
#endif
		ptr=next;
	}while(*next);
}

void DD_DialogGetFormat(OBJECT *tree,short obj, unsigned long format[])
/*
	Hier kann an Hand des Objektbaumes und der Objektnummer
	Das gewuenschte Daten-Format angegeben werden.
*/
{
	short i;
	
	for(i = 0; i < MAX_DDFORMAT; i++)
		format[i] = 0L;

	format[0] = 0x41524753L; /* 'ARGS' */
}
#endif



/*******************************************************/
/****** ST-Guide Kapitelauswahl                   ******/
/*******************************************************/
#if USE_STGUIDE == YES

char *GetTopic(void)
{
	CHAIN_DATA *chain_ptr;
	short handle;
	short typ = BGH_USER, gruppe = 0, index = -1;
	extern void *Help;
	
	wind_get_int(0,WF_TOP,&handle);

	chain_ptr=find_ptr_by_whandle(handle);
	if(!chain_ptr)			/*	Kein offenes Fenster?	*/
		index=0;				/*	user 000	*/
	else if(chain_ptr->type==WIN_DIALOG)
	{
		typ=BGH_DIAL;		/*	dial <nr. entsprechend dem Dialog>	*/
		for(gruppe=0;((DIALOG_DATA *)chain_ptr)->obj!=tree_addr[gruppe];gruppe++)
			;
/*		Debug("Gruppe : %d",gruppe);
*/	}
	else
	{
		if(chain_ptr->type==WIN_WINDOW)
			index=1;			/*	user 001	*/
		else
			index=0;			/*	user 000	*/
	}

	return(BGH_gethelpstring(Help,typ,gruppe,index));
}
#endif



/*******************************************************/
/****** AV Protokoll                              ******/
/*******************************************************/
#if USE_AV_PROTOCOL != NO

void DoVA_START(short msg[8])
/*
	Der Server aktiviert das Programm und bergibt eine Kommandozeile.
	Evtl. muss mittels ParseData() alles ausgewertet werden.
*/
{
	short	fromApp;
	if(modal_items<0)				/*	klappt nur, wenn kein modaler Dialog offen ist	*/
	{
		char	*data;
		data=*(char **)&msg[3];
		if(data!=NULL)
		{
		char arg[DL_PATHMAX];
		char *chapter;
		short ret, count;

			count=count_window ();
#if DEBUG==ON
			Debug("AV_START from %d:",msg[1]);
			Debug("  <%s>",data);
#endif
			strcpy(arg,data);
			chapter=ParseData(arg);

			Win = get_first_window();

			if (va_start_newwin == 2 || !Win)
				ret = OpenFileNW(arg,chapter,0);
			else
				ret = OpenFileSW(arg, chapter, va_start_newwin);

			if ( count == 0 && ret==TRUE)
			{
/* ----
				SetLastHistory (Win,last_history);
				DeletLastHistory(last_history);
				last_history=NULL;
---- */
			}

		}
		else
		{
			if(*default_file)
				OpenFileNW(default_file,NULL,0);
			else
				SelectFileLoad();
		}
	}

	fromApp = msg[1];
	msg[0] = AV_STARTED;
	msg[1] = ap_id;
	appl_write(fromApp,16,&msg[0]);
}

#if USE_AV_PROTOCOL == 3
void DoVA_Message(short msg[8])
{
	switch(msg[0])
	{
		case AV_STARTED:
		case VA_PROGSTART:
		{
			extern char *av_parameter;
			if(av_parameter)
			{
				Mfree(av_parameter);
				av_parameter = NULL;
			}
			else
			{
				Debug("Received msg %x. But no AV action started.",msg[0]);
			}
			break;
		}
		case VA_DRAGACCWIND:
		{
			if(modal_items<0)				/*	klappt nur, wenn kein modaler Dialog offen ist	*/
			{
				char	*data;
				data = *(char **)&msg[6];
				if(data != NULL)
				{
					char arg[DL_PATHMAX];
#if DEBUG==ON
					Debug("VA_DRAGACCWIND from %d:",msg[1]);
					Debug("  <%s>",data);
#endif
					strcpy(arg, data);
					ParseData(arg);
		
					Win = find_window_by_whandle(msg[3]);
		
					if(va_start_newwin == 2)
						OpenFileNW(arg,NULL,0);
					else
						OpenFileSW(arg,NULL,va_start_newwin);
				}
			}	
			break;
		}
		default:
			Debug("User Routine fr's AV Protokoll: %x",msg[0]);
	}
}
#endif
#endif



