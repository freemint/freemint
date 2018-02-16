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

short GetScrapPath(char *scrap_path, short clear);

void BlockOperation(DOCUMENT *doc, short num)
{
	WINDOW_DATA *win = Win;
	switch(num)
	{
		case CO_SAVE:
			SelectFileSave(doc);
			break;
		case CO_BACK:
			GoBack(doc);
			break;
		case CO_COPY:
			BlockCopy(doc);
			break;
		case CO_PASTE:
			BlockPaste(clipbrd_new_window);
			break;
		case CO_SELECT_ALL:
			SelectAll(doc);
			break;
	  case CO_SEARCH:
	    Hypfind( doc );
	    break;
		case CO_DELETE_STACK:
			RemoveAllHistoryEntries(win);
			ToolbarUpdate(doc,win->toolbar,TRUE);
			break;
		case CO_PRINT:
			break;
	}
}

void BlockSelectAll(DOCUMENT *doc, BLOCK *b)
{
	b->start.line = 0;
	b->start.y = 0;
	b->start.offset = 0;
	b->start.x = 0;
	b->end.line = doc->lines;
	b->end.y = doc->height;
	b->end.offset = 0;
	b->end.x = 0;
	b->valid = TRUE;
}

void BlockCopy(DOCUMENT *doc)
{
	char scrap_file[DL_PATHMAX];
	BLOCK b = doc->selection;
	
	if(!b.valid)
		BlockSelectAll(doc,&b);

	/*	Kopier-Aktion ins Clipboard	*/
	graf_mouse(BUSYBEE,NULL);
	if(GetScrapPath(scrap_file, TRUE))
	{
		Debug("No clipboard defined");
	}
	else
	{
		short msg[8] = {SC_CHANGED,0,0,2,0x2e54/*'.T'*/,0x5854/*'XT'*/,0,0};
		BlockAsciiSave(doc, scrap_file);
		msg[1] = ap_id;
		shel_write(SHW_BROADCAST,0,0,(char *)msg,NULL);
	}
	graf_mouse(ARROW,NULL);
}

void BlockPaste(short new_window)
{
	/* WINDOW_DATA *win = Win; */
	char scrap_file[DL_PATHMAX];
	
	/*	"Einfuege"-Aktion ladet SCRAP.TXT aus dem Clipboard	*/
	if(GetScrapPath(scrap_file, FALSE))
	{
		Debug("No clipboard defined");
	}
	else
	{
		long ret;
		ret = Fopen(scrap_file, O_RDONLY);
		if(ret >= 0)
		{
			Fclose((short)ret);
			if (new_window)
				Win = NULL;
			OpenFileSW(scrap_file, NULL, 0);
		}
	}
}

void BlockAsciiSave(DOCUMENT *doc, char *path)
{
	long ret;
	if(!doc->blockProc)
	{
		Cconout(7);			/*	Bing!!!!	*/
		return;
	}

	ret = Fopen(path,O_WRONLY|O_TRUNC|O_CREAT);
	if(ret < 0)
		FileErrorNr(path, ret);
	else
	{
		short handle = (short)ret;
		BLOCK b = doc->selection;
		
		if(!b.valid)		/*	Kein gltiger Block angewhlt?	*/
			BlockSelectAll(doc, &b);
		
		doc->blockProc(doc, BLK_ASCIISAVE, &b, &handle);
		Fclose(handle);
	}
}

short GetScrapPath(char *scrap_path, short clear)
{
	long ret;
	if(!scrp_read(scrap_path))
		return(TRUE);

	if ( clear )			/* Klemmbrett loeschen?				*/
	{
		if(!scrp_clear())		/*	scrp_clear() nicht vorhanden?	*/
		{
			long dirhandle;
	
			/*	Verzeichnis oeffnen und alle "SCRAP.*" Dateien loeschen	*/
			dirhandle = Dopendir(scrap_path, 0);
			if((dirhandle >> 24) != 0xff)
			{
				char filename[DL_NAMEMAX], *ptr;
				XATTR xattr;
				long xret;
				ptr = &scrap_path[strlen(scrap_path)];
				do
				{
					ret = Dxreaddir(DL_NAMEMAX, dirhandle, filename, &xattr, &xret);
					if((ret >= 0) && (xret >= 0) && 
						((xattr.mode&S_IFMT) == S_IFREG) && 
						!strnicmp(filename+4,"SCRAP.",6))
					{
						strcpy(ptr, filename + 4);
						Fdelete(scrap_path);
					}
				}while(ret == 0);
				Dclosedir(dirhandle);
			}
		}
	}
	
	strcat(scrap_path,"SCRAP.TXT");
	return(FALSE);
}
