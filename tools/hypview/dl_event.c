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
#include "defs.h"

#if USE_AV_PROTOCOL != NO
	#include "include/av.h"
#endif
#if USE_BUBBLEGEM == YES
	#include "bubble/bubble.h"
#endif
#if USE_DOCUMENTHISTORY == YES
	#include "include/dhst.h"
#endif

extern short _app;

short doneFlag=FALSE, quitApp=FALSE;

void DoKeybd(EVNT *event);
void DoMessage(EVNT *event);
void DoEventDispatch(EVNT *event);
void DoEvent(void);

void DoKeybd(EVNT *event)
{
short ascii=event->key,scan;
short kstate=event->kstate;
	ConvertKeypress(&ascii,&kstate);

	scan=(ascii>>8) & 0xff;
	ascii=ascii & 0xff;
	
	if(kstate == (K_CTRL|K_ALT))
	{
#if USE_ITEM == YES
		if(ascii==' ')
		{
			FlipIconify();
			event->mwhich&=~MU_KEYBD;
		}
#endif
	}
	else if(kstate == (K_CTRL|K_ALT|K_RSHIFT|K_LSHIFT))
	{
#if USE_ITEM == YES
		if(ascii==' ')
		{
		short msg[8]={WM_ALLICONIFY,0,0,0,-1,-1,-1,-1}, d;
			msg[1]=ap_id;
			wind_get(0,WF_TOP, &msg[3],&d, &d, &d);
			appl_write(ap_id,16,msg);
			event->mwhich&=~MU_KEYBD;
		}
#endif
	}
	else if (kstate == K_CTRL)
	{
		switch(ascii)
		{
			case 'Q':
#if USE_MENU==YES
				ChooseMenu(ME_FILE, ME_QUIT);
#else
				if (_app)
					doneFlag=TRUE;
				else
					RemoveItems();
#endif
				event->mwhich&=~MU_KEYBD;
				break;
#if USE_ITEM == YES
			case 'W':
				CycleItems();
				event->mwhich&=~MU_KEYBD;
				break;
			case 'U':
			{
			short msg[8]={WM_CLOSED,0,0,0,0,0,0,0}, d;
				msg[1]=ap_id;
				wind_get(0,WF_TOP, &msg[3],&d, &d, &d);
				appl_write(ap_id,16,&msg[0]);
				event->mwhich&=~MU_KEYBD;
				break;
			}
#endif
		}
	}
	else if(kstate==0)
	{
#if USE_STGUIDE==YES
		if(scan == KbHELP)			/*	HELP	*/
		{
			STGuideHelp();
			event->mwhich&=~MU_KEYBD;
		}
#endif
	}

#if USE_GLOBAL_KEYBOARD == YES
	if(modal_items<0)
	{
		if(DoUserKeybd(kstate, scan, ascii))
			event->mwhich&=~MU_KEYBD;
	}
#endif

#if DEBUG==ON
	Debug("Key: %d Scan: %d  Ascii: %d -- %c %c %c %c %s%s%s%s%s",
		event->key,scan,ascii,ascii,key_table->unshift[scan],
		key_table->shift[scan],key_table->caps[scan],
		((kstate & K_CTRL) ? ("+CTRL"):("")),
		((kstate & K_ALT) ? ("+ALT"):("")),
		((kstate & K_LSHIFT) ? ("+LSHIFT"):("")),
		((kstate & K_RSHIFT) ? ("+RSHIFT"):("")),
		((kstate & 0x10/*KbNUM*/) ? ("+NUM"):("")));
#endif
}

void DoMessage(EVNT *event)
{
	if(event->msg[2]>0)								/*	erweiterte Message ??	*/
	{
	short *xmsg;
		xmsg=(short *)Mxalloc(event->msg[2],3);	/* Platz vorbereiten	*/
		if(xmsg!=NULL)
		{
			appl_read(ap_id,event->msg[2],xmsg);	/*	"Message" lesen	*/
			Mfree(xmsg);								/*	Speicher freigeben */
		}
		else
			form_alert(1,tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
	}

	switch((unsigned short)event->msg[0])
	{
#if USE_MENU==YES
		case MN_SELECTED:
			ChooseMenu(event->msg[3], event->msg[4]);
			break;
#endif
		case AP_TERM:
			quitApp=TRUE;
#if USE_MENU==YES
			ChooseMenu(ME_FILE, ME_QUIT);
#else
			doneFlag=TRUE;
#endif
			break;
#if USE_DRAGDROP==YES
		case AP_DRAGDROP:
			DragDrop(event->msg);
			break;
#endif
#if USE_AV_PROTOCOL != NO
	#if USE_AV_PROTOCOL >= 2
		case VA_PROTOSTATUS:			/*	Server bestaetigt Anmeldung	*/
			DoVA_PROTOSTATUS(event->msg);
			break;
	#endif
	#if USE_AV_PROTOCOL == 3
		case VA_SETSTATUS:
		case VA_FILEFONT:
		case VA_CONFONT:
		case VA_OBJECT:
		case VA_CONSOLEOPEN:
		case VA_WINDOPEN:
		case VA_PROGSTART:
		case VA_DRAGACCWIND:
		case VA_COPY_COMPLETE:
		case VA_THAT_IZIT:
		case VA_DRAG_COMPLETE:
		case VA_FONTCHANGED:
		case VA_XOPEN:
		case VA_VIEWED:
		case VA_FILECHANGED:
		case VA_FILECOPIED:
		case VA_FILEDELETED:
		case VA_PATH_UPDATE:
		case AV_STARTED:
			DoVA_Message(event->msg);
			break;			
	#endif
		case VA_START:					/*	Kommandozeile bergeben	*/
			DoVA_START(event->msg);
			break;
		case AV_SENDCLICK:			/*	Mausklick gemeldet (BubbleGEM)	*/
			event->mwhich=MU_BUTTON;
			event->mx=event->msg[3];
			event->my=event->msg[4];
			event->mbutton=event->msg[5];
			event->kstate=event->msg[6];
			event->mclicks=event->msg[7];
			DoEventDispatch(event);
			break;
		case AV_SENDKEY:				/*	Tastendruck gemeldet (BubbleGEM)	*/
			event->mwhich=MU_KEYBD;
			event->kstate=event->msg[3];
			event->key=event->msg[4];
			DoEventDispatch(event);
			break;
#endif
#if USE_BUBBLEGEM==YES
		case BUBBLEGEM_REQUEST:
			Bubble(event->msg[4],event->msg[5]);
			break;
		case BUBBLEGEM_ACK:
			break;
#endif
#if USE_DOCUMENTHISTORY == YES
		case DHST_ACK:
			DhstFree(event->msg);
			break;
#endif
#if DEBUG==ON
		default:
			Debug("Message :%d %x erhalten",event->msg[0],event->msg[0]);
			break;
#endif
	}

}


void DoEventDispatch(EVNT *event)
{
#if USE_USER_EVENTS == YES
	DoUserEvents(event);
#endif

	if(event->mwhich & MU_BUTTON)
		DoButton(event);

	if(event->mwhich & MU_KEYBD)
		DoKeybd(event);

#if USE_ITEM == YES
	ItemEvent(event);
#endif

	if(event->mwhich & MU_MESAG)
		DoMessage(event);
}

void DoEvent(void)
{
EVNT event;
	EVNT_multi(EVENTS, MBCLICKS, MBMASK, MBSTATE, MBLOCK1, MBLOCK2,
			WAIT,&event);
	DoEventDispatch(&event);
}
