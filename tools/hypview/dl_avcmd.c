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
#include "include/av.h"
#include "diallib.h"

extern short ap_id,server_id;

/*	Nachfrage nach dem Konfigurationsstring, der mit AV_STATUS gesetzt wurde	*/
void DoAV_GETSTATUS(void)
{
	short msg[8] = {AV_GETSTATUS,0,0,0,0,0,0,0};
	msg[1] = ap_id;
	appl_write(server_id,16,msg);
}

/*	Uebermittelt einen Konfigurationsstring, den der Server speichert	*/
void DoAV_STATUS(char *string)
{
	short msg[8]={AV_STATUS,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=string;
	appl_write(server_id,16,msg);
}

/*	Uebermittelt dem Server einen Tastendruck, der nicht verarbeitet wurde	*/
void DoAV_SENDKEY(short kbd_state, short code)
{
	short msg[8]={AV_SENDKEY,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=kbd_state;
	msg[4]=code;
	appl_write(server_id,16,msg);
}

/*	Fragt beim Server nach, mit welchem Font die Dateien dargestellt werden	*/
void DoAV_ASKFILEFONT(void)
{
	short msg[8]={AV_ASKFILEFONT,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	appl_write(server_id,16,msg);
}

/*	Fragt beim Server nach dem Konsolen-Font	*/
void DoAV_ASKCONFONT(void)
{
	short msg[8]={AV_ASKCONFONT,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	appl_write(server_id,16,msg);
}

/*	Fragt beim Server nach dem selektierten Objekt	*/
void DoAV_ASKOBJECT(void)
{
	short msg[8]={AV_ASKOBJECT,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	appl_write(server_id,16,msg);
}

/*	Der Server soll ein Consolenfenster oeffnen	*/
void DoAV_OPENCONSOLE(void)
{
	short msg[8]={AV_OPENCONSOLE,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	appl_write(server_id,16,msg);
}

/*	Der Server soll ein Verzeichnisfenster oeffnen	*/
void DoAV_OPENWIND(char *path, char *wildcard)
{
	short msg[8]={AV_OPENWIND,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=path;
	*(char **)&msg[5]=wildcard;
	appl_write(server_id,16,msg);
}

/*	Der Server soll ein Programmstarten oder eine Datei oeffnen	*/
void DoAV_STARTPROG(char *path, char *commandline, short id)
{
	short msg[8]={AV_STARTPROG,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=path;
	*(char **)&msg[5]=commandline;
	msg[7]=id;
	appl_write(server_id,16,msg);
}

/*	Es wird dem Server ein neues Fenster gemeldet (fuer Cycling und Drag&Drop)	*/
void DoAV_ACCWINDOPEN(short handle)
{
	short msg[8]={AV_ACCWINDOPEN,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=handle;
	appl_write(server_id,16,msg);
}

/*	Es wird dem Server mitgeteilt, dass ein Fenster geschlossen wurde	*/
void DoAV_ACCWINDCLOSED(short handle)
{
	short msg[8]={AV_ACCWINDCLOSED,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=handle;
	appl_write(server_id,16,msg);
}

/*	Der Server soll die Dateien kopieren, die ihm zuvor gemeldet wurden	*/
void DoAV_COPY_DRAGGED(short kbd_state, char *path)
{
	short msg[8]={AV_COPY_DRAGGED,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=kbd_state;
	*(char **)&msg[4]=path;
	appl_write(server_id,16,msg);
}

/*	Der Server soll das Verzeichnisfenster <path> neu einlesen	*/
void DoAV_PATH_UPDATE(char *path)
{
	short msg[8]={AV_PATH_UPDATE,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=path;
	appl_write(server_id,16,msg);
}

/*	Fragt beim Server, was sich an der Position x,y befindet	*/
void DoAV_WHAT_IZIT(short x,short y)
{
	short msg[8]={AV_WHAT_IZIT,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=x;
	msg[4]=y;
	appl_write(server_id,16,msg);
}

/*	Teilt dem Server mit, dass Objekte auf sein Fenster gezogen wurden	*/
void DoAV_DRAG_ON_WINDOW(short x,short y, short kbd_state, char *path)
{
	short msg[8]={AV_DRAG_ON_WINDOW,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=x;
	msg[4]=y;
	msg[5]=kbd_state;
	*(char **)&msg[6]=path;
	appl_write(server_id,16,msg);
}

/*	Antwort auf VA_START	*/
void DoAV_STARTED(char *ptr)
{
	short msg[8]={AV_STARTED,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=ptr;
	appl_write(server_id,16,msg);
}

/*	Der Server soll ein bestimmtes Fenster oeffnen	*/
void DoAV_XWIND(char *path, char *wild_card, short bits)
{
	short msg[8]={AV_XWIND,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=path;
	*(char **)&msg[5]=wild_card;
	msg[7]=bits;
	appl_write(server_id,16,msg);
}

/*	Der Server soll den passenden Viewer fuer eine Datei starten	*/
void DoAV_VIEW(char *path)
{
	short msg[8]={AV_VIEW,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=path;
	appl_write(server_id,16,msg);
}

/*	Der Server soll die Datei-/Ordnerinformationen anzeigen	*/
void DoAV_FILEINFO(char *path)
{
	short msg[8]={AV_FILEINFO,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=path;
	appl_write(server_id,16,msg);
}

/*	Der Server soll die Dateien/Ordner ans Ziel kopieren	*/
void DoAV_COPYFILE(char *file_list, char *dest_path,short bits)
{
	short msg[8]={AV_COPYFILE,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=file_list;
	*(char **)&msg[5]=dest_path;
	msg[7]=bits;
	appl_write(server_id,16,msg);
}

/*	Der Server soll die Dateien/Ordner loeschen	*/
void DoAV_DELFILE(char *file_list)
{
	short msg[8]={AV_DELFILE,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	*(char **)&msg[3]=file_list;
	appl_write(server_id,16,msg);
}

/*	Teilt dem Server mit, wo das naechste Fenster geffnet werden soll	*/
void DoAV_SETWINDPOS(short x,short y,short w,short h)
{
	short msg[8]={AV_SETWINDPOS,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=x;
	msg[4]=y;
	msg[5]=w;
	msg[6]=h;
	appl_write(server_id,16,msg);
}

/*	Dem Server wird ein Mouseklick gemeldet	*/
void DoAV_SENDCLICK(EVNTDATA *m, short ev_return)
{
	short msg[8]={AV_SENDCLICK,0,0,0,0,0,0,0};
	msg[1]=ap_id;
	msg[3]=m->x;
	msg[4]=m->y;
	msg[5]=m->bstate;
	msg[6]=m->kstate;
	msg[7]=ev_return;
	
	appl_write(server_id,16,msg);
}
