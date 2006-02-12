/*
 * $Id$
 * 
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

#include <string.h>
#ifdef __GNUC__
	#include <mintbind.h>
	#include <fcntl.h>
	#include "stat.h"
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "diallib.h"
#include "defs.h"

extern WINDOW_DATA *Win;

short LoadFile(DOCUMENT *doc, short handle)
{
	short type;

	type = HypLoad(doc, handle);
	if(type == F_UNKNOWN)
		type = AsciiLoad(doc, handle);

	return(type);
}

static short
find_file(char *path, char *real_path)
{
	WINDOW_DATA *win = Win;
	long ret;
	char *list=path_list;
	char *dst;

	/*	Falls schon eine Datei/ein Fenster geffnet wurde	*/
	if (win)
	{
		DOCUMENT *doc = win->data;
		char *filename;

		/*	Die neue Datei in dessen Pfad suchen	*/
		strcpy(real_path,doc->path);
		
		filename=strrchr(real_path,'\\');
		if(!filename)
			filename=real_path;
		else
			filename++;
		
		strcpy(filename,path);
		ret=Fopen(real_path,O_RDONLY);
/*		Debug("1: Trying at %s => %ld",real_path,ret);
*/		if (ret >= 0)
		{
			Fclose((short)ret);
			return(TRUE);
		}


		/*	Versuche die Datei als Link aufzuschlsseln	*/
		ret=Freadlink(DL_PATHMAX,real_path,doc->path);
		if(ret==0)
		{
/*			Debug("=> %ld: '%s' is at '%s'",ret,doc->path,real_path);
*/
			filename=strrchr(real_path,'\\');
			if(!filename)
				filename=real_path;
			else
				filename++;
			
			strcpy(filename,path);

			ret=Fopen(real_path,O_RDONLY);
/*			Debug("2: Trying at %s => %ld",real_path,ret);
*/			if(ret>=0)
			{
				Fclose((short)ret);
				return(TRUE);
			}
		}
	}


	/*	Pfad, so wie er gegeben ist ausprobieren	*/
	ret=Fopen(path,O_RDONLY);
/*	Debug("3: Trying at %s => %ld",path,ret);
*/
	if(ret>=0)
	{
		if(path[1]==':')		/*	Handelt es sich um einen absoluten Pfad?	*/
			strcpy(real_path,path);
		else
		{
			/*	Absoluten Pfad ermitteln	*/
			real_path[0]=Dgetdrv()+'A';
			real_path[1]=':';
			Dgetpath(&real_path[2],0);
			dst=&real_path[strlen(real_path)];
			*dst++=dir_separator;
			strcat(real_path,path);
		}

		Fclose((short)ret);
		return(TRUE);
	}

/* [GS] 0.34 Start */
	if ( path[1] == ':' )     	/*	Handelt es sich um einen absoluten Pfad?	*/
	{							/*  Dann Dateinamen extrahieren					*/
		dst = strrchr( path, '\\' );
		if ( dst )
			strcpy ( path, dst + 1);
	}
/* Ende */

	/*	Pfad mit Pfad aus der Pfadliste kombiniert probieren	*/
	while(*list)
	{
		dst=real_path;
		while(*list && *list!=';')
			*dst++=*list++;

		*dst--=0;
		list++;
		if(*dst!=dir_separator)
		{
			dst++;
			*dst++=dir_separator;
			*dst=0;
		}
		strcat(real_path,path);


		ret=Fopen(real_path,O_RDONLY);
/*		Debug("4. Trying at %s => %ld",real_path,ret);
*/	
		if(ret>=0)
		{
			Fclose((short)ret);
			return(TRUE);
		}
	}
	return(FALSE);
}

static DOCUMENT *
open_file(char *path)
{
	DOCUMENT *doc = NULL;
	long ret;
	short handle;
	char *ptr;

	ret = Fopen(path,O_RDONLY);
	
	if (ret < 0)
	{
		FileErrorNr(path, ret);
		return (NULL);
	}
	handle = (short)ret;

	/*	Nicht gefunden? --> neues Dokument erstellen	*/	
	doc = (DOCUMENT *)Malloc(sizeof(DOCUMENT) + strlen(path) + 1);
	
	if(!doc)
	{
		form_alert(1, string_addr[DI_MEMORY_ERROR]);	/*	Fehler melden	*/
		Fclose(handle);
		return (NULL);
	}
	memset(doc, 0, sizeof(DOCUMENT));

	doc->path = (char *)((long)doc + sizeof(DOCUMENT));
	strcpy(doc->path,path);
	ptr = strrchr(doc->path, dir_separator);
	if(!ptr++)
		ptr = doc->path;
	doc->filename = ptr;
	doc->lines = 0;
	doc->columns = 0;
	doc->window_title = doc->path;
	doc->buttons = 0;
	doc->data = NULL;
	doc->next = NULL;
	doc->popup = NULL;
	doc->type = -1;
	doc->autolocator = NULL;
	doc->selection.valid = FALSE;
	
	/*	Hier folgt die typ-spezifische Lade-Routine	*/
	if(LoadFile(doc, handle) < 0)
	{
		FileError(doc->filename,"format not recognized");
		Mfree(doc);
		doc = NULL;
	}
	else
	{
		XATTR attr;
		/*	nderungsdatum und -zeit sichern	*/
		if (Fxattr(0/*FXATTR_RESOLVE*/, doc->path, &attr) < 0L)
		{
			doc->mtime = 0;
			doc->mdate = 0;
		}
		else
		{
			doc->mtime = attr.mtime;
			doc->mdate = attr.mdate;
		}
	}

	Fclose(handle);

	return(doc);
}


/*	Entfernt die Datei <doc> aus dem Speicher und aus der Liste.
	<doc> muss (!!) ein Zeiger auf ein Dokument in der Liste sein.	*/
void CloseFile(DOCUMENT *doc)
{
	/*	Hier folgt die typ-spezifische Aufrum-Arbeit.*/
	if(doc->data)
		doc->closeProc(doc);
	if(doc->autolocator)
		Mfree(doc->autolocator);

	Mfree(doc);				/*	Speicher der DOCUMENT-Struktur freigeben	*/
}


/*	ffnet eine Datei in einem neuen Fenster	*/
/* [GS] 0.35.2c Start: */
short OpenFileNW(char *path, char *chapter, long node)
/* Ende; alt:
short OpenFileNW(char *path, char *chapter)
*/
{
	DOCUMENT *doc;
	char real_path[DL_PATHMAX];
	/*	Leere Zeichenkette (=kein Pfad)	*/
	if(!*path)
		return FALSE;

/*
	Debug("OpenFileNW(<%s>,Chapter <%s>)",path,(chapter?chapter:"N/A"));
*/

	graf_mouse(BUSYBEE,NULL);

	/*	Falls der Suchpfad mit "*:\" beginnt (=> HYP-Datei): entfernen	*/
	if (strncmp(path, "*:\\", 3) == 0)
		path += 3;

	if (!find_file(path, real_path))
	{
		graf_mouse(ARROW, NULL);
		FileError(path, "not found");
		return FALSE;
	}

	/*	Datei laden/initialisieren	*/
	doc = open_file(real_path);
	if (doc)
	{
/* [GS] 0.35.2c Start: */
		doc->gotoNodeProc(doc, chapter, node);
/* Ende; alt:
		doc->gotoNodeProc(doc,chapter,0);
*/

		/*	neues Fenster anlegen?	*/
		OpenWindow(HelpWindow,NAME|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|
					VSLIDE|LFARROW|RTARROW|HSLIDE|SMALLER,doc->path,
					-1,-1,doc);
	}
	else
	{
		graf_mouse(ARROW,NULL);
		return FALSE;
	}

	return TRUE;
}


/*	ffnet eine Datei im gleichen Fenster falls <new_win> < 2	*/
short OpenFileSW(char *path, char *chapter,short new_win)
{
	WINDOW_DATA *win = Win;
	DOCUMENT *doc = NULL;
	char real_path[DL_PATHMAX];

	/*	Leere Zeichenkette (=kein Pfad)	*/
	if(!*path)
		return FALSE;


/*	Debug("OpenFileSW(<%s>,Chapter <%s>,new win %d)",path,(chapter?chapter:"N/A"),new_win);
*/

	graf_mouse(BUSYBEE,NULL);

	/*	Falls der Suchpfad mit "*:\" beginnt (=> HYP-Datei): entfernen	*/
	if(strncmp(path,"*:\\",3)==0)
		path+=3;

	if(!find_file(path,real_path))
	{
		graf_mouse(ARROW,NULL);
		FileError(path,"not found");
		return FALSE;
	}

	/*	Nur wenn ntig die Datei neu Laden	*/
	if (new_win == 1)
	{
		DOCUMENT *doc2;

		/*	Gibt es ein Fenster, dass die gewnschte Datei darstellt?	*/
		Win = (WINDOW_DATA *)all_list;
		while (Win)
		{
			if(Win->type == WIN_WINDOW)
			{
				doc2 = Win->data;
				if(strcmp(doc2->path, real_path) == 0)
				{
					AddHistoryEntry(Win);
					doc = doc2;
					break;
				}
			}
			Win = Win->next;
		}
		win = Win;
	}
	else if (win)
	{
		DOCUMENT *prev_doc = win->data;

		AddHistoryEntry(win);

		/*	Datei als aktuelles Dokument im Fenster geladen?	*/
		if(strcmp(prev_doc->path,real_path) == 0)
		{
			doc = prev_doc;
			win->data = prev_doc->next;
			prev_doc->next = NULL;
		}
		else
		{
			prev_doc->closeProc(prev_doc);

			doc = prev_doc->next;
			while (doc)
			{
				if(strcmp(doc->path, real_path) == 0)
				{
					prev_doc->next = doc->next;
					doc->next = NULL;
					break;
				}
				prev_doc = doc;
				doc = doc->next;
			}
		}
	}

	if (!doc)				/*	Wenn ntig Datei laden/initialisieren	*/
		doc = open_file(real_path);

	if (doc)
	{
		if (!doc->data)
		{
			long ret;
			/*	Datei erneut laden	*/
			ret = Fopen(doc->path, O_RDONLY);
			if(ret >= 0)
				LoadFile(doc, (short)ret);
			else
				FileErrorNr(doc->filename, ret);
			Fclose((short)ret);
		}
	
		doc->gotoNodeProc(doc, chapter, 0);

		/*	Noch kein Fenster offen?	*/
		if(!win)
			OpenWindow(HelpWindow,NAME|CLOSER|FULLER|MOVER|SIZER|UPARROW|DNARROW|
					VSLIDE|LFARROW|RTARROW|HSLIDE|SMALLER,doc->path,
					-1,-1,doc);
		else
		{
			if(win->status & WIS_ICONIFY)
				UniconifyWindow(win);
			doc->next = win->data;
			Win = win;
			ReInitWindow(win,doc);
			wind_set(win->whandle,WF_TOP,0,0,0,0);
			graf_mouse(ARROW,NULL);
		}
	}
	else
	{
		graf_mouse(ARROW,NULL);
		return FALSE;
	}

	return TRUE;
}



void CheckFiledate(DOCUMENT *doc)
{
	XATTR attr;
	long ret;
	ret = Fxattr(0/*FXATTR_RESOLVE*/,doc->path,&attr);
	if(ret == 0)
	{
		/*	Hat sich das Datum oder die Zeit gendert?	*/
		if(attr.mtime != doc->mtime || attr.mdate != doc->mdate)
		{
			long node = 0;

			graf_mouse(BUSYBEE,NULL);

			node = doc->getNodeProc(doc);
			doc->closeProc(doc);

			/*	Datei erneut laden	*/
			ret=Fopen(doc->path,O_RDONLY);
			if(ret>=0)
				LoadFile(doc,(short)ret);
			else
				FileErrorNr(doc->filename, ret);
			Fclose((short)ret);

			/*	zur vorherigen Seite springen	*/
			doc->gotoNodeProc(doc,NULL,node);

			ReInitWindow(Win, doc);
			graf_mouse(ARROW,NULL);
		}
	}
}
