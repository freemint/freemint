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

#ifdef __GNUC__
	#include <osbind.h>
	#include <fcntl.h>
#else
	#include <tos.h>
#endif
#include <gem.h>
#include "include/scancode.h"
#include "diallib.h"
#include "defs.h"

#define AUTOLOC_SIZE		26

extern WINDOW_DATA *Win;

extern long procAutolocator(DOCUMENT *doc,long line);
extern short GetScrapPath(char *scrap_path, short clear);

/*	Initialisiert und aktiviert den Autolocator
	Rueckgabewert ist die Position des naechsten Zeichens.
*/
static char *
AutolocatorInit(DOCUMENT *doc)
{
	char *ptr;
	/*	Noch kein Speicher fuer Autolocator alloziert?	*/
	if(!doc->autolocator)
	{
		ptr = (char *)Malloc(AUTOLOC_SIZE);
		if(ptr == NULL) {
			form_alert(1, tree_addr[DIAL_LIBRARY][DI_MEMORY_ERROR].ob_spec.free_string);
			return NULL;
		}
		doc->autolocator = ptr;
		ptr[AUTOLOC_SIZE-1] = 0;
		*ptr = 0;
	}
	else
	{
		/*	Zum Stringende...	*/
		ptr = doc->autolocator;
		while(*ptr)
			ptr++;
	}

	/*	Search-Box noch nicht aktiviert?	*/
	if(!(doc->buttons & BITVAL(TO_SEARCHBOX)))
		doc->buttons |= BITVAL(TO_SEARCHBOX);
	
	return ptr;
}

/*	Aktualisiert den Autolocator und startet evtl. eine Suche	*/
static void
AutolocatorUpdate(DOCUMENT *doc, long start_line)
{
	WINDOW_DATA *win = Win;
	GRECT tbar;
	long line = start_line;

	if ((doc->buttons & BITVAL(TO_SEARCHBOX)) == 0)
		return;

	/*	Toolbar mit neuem Text zeichnen	*/
	wind_get_grect(win->whandle, WF_WORKXYWH, &tbar);
	tbar.g_h = win->y_offset;

	win->toolbar[TO_STRNOTFOUND].ob_flags |= OF_HIDETREE;

	if ( tbar.g_w > 472 )				/* Box vergroessern damit kein haesslicher Streifen bleibt */
		win->toolbar[TO_SEARCHBOX].ob_width = tbar.g_w; 
	else
		win->toolbar[TO_SEARCHBOX].ob_width = 472;		/* Standardwert aus Resource setzen */

	wind_update(BEG_UPDATE);
	objc_draw_grect(win->toolbar, TO_BACKGRND, MAX_DEPTH, &tbar);
	wind_update(END_UPDATE);

	/*	Wenn der Auto-Locator nicht leer ist... */
	if(*doc->autolocator) {
		graf_mouse(BUSYBEE, NULL);
		line = doc->autolocProc(doc, start_line);
		graf_mouse(ARROW, NULL);
	}

/*	Debug("Autolocator: <%s> = %d",doc->autolocator,line);
*/
	if (line >= 0)
	{
		if (line != win->y_pos)
		{
			win->y_pos = line;
			SendRedraw(win);
			SetWindowSlider(win);
		}
	}
	else
	{
		win->toolbar[TO_STRNOTFOUND].ob_flags &= ~OF_HIDETREE;
		Cconout(7);
		wind_update(BEG_UPDATE);
		objc_draw_grect(win->toolbar, TO_SEARCHBOX,MAX_DEPTH, &tbar);
		wind_update(END_UPDATE);
	}
}

/*	Fuegt dem Autolocator ein neues Zeichen ein und aktiviert die Suche */
short AutolocatorKey(DOCUMENT *doc, short kbstate, short ascii)
{
	WINDOW_DATA *win = Win;
	char *ptr;
	long line = win->y_pos;

	if(!ascii)
		return FALSE;

	ptr = AutolocatorInit(doc);
	doc->autolocator_dir = 1;
	
	if (ascii == 8)				/* Backspace */
	{
		if (ptr > doc->autolocator)
			ptr--;
		*ptr=0;
	}
	else if (ascii == 13)		/* Return */
	{
		if (kbstate & KbSHIFT) {
			doc->autolocator_dir = 0;
			line--;
		}
		else
			line++;
	}
	else if (ascii == 27)		/* Escape */
	{
		if(ptr>doc->autolocator)
		{
			ptr=doc->autolocator;
			*ptr=0;
		}
		else
		{
			RemoveSearchBox(doc);
		}
	}
	else if(ascii==' ')
	{
		/*	Leerzeichen am Anfang werden ignoriert	*/
		if(ptr != doc->autolocator)
			*ptr++=' ';

		*ptr=0;
	}
	else if(ascii>' ')
	{
		if(ptr-doc->autolocator < AUTOLOC_SIZE)
		{
			*ptr++=ascii;
			*ptr=0;
		}
		else
		{
			--ptr;
			*ptr=ascii;
		}
	}

	ToolbarUpdate(doc, win->toolbar, FALSE);
	AutolocatorUpdate(doc, line);
	
	return TRUE;
}

/* Fuegt den Inhalt des Clipboards im Autolocator ein. */
void AutoLocatorPaste(DOCUMENT *doc)
{
	WINDOW_DATA *win = Win;
	long ret;
	char scrap_file[DL_PATHMAX];
	
	if(GetScrapPath(scrap_file, FALSE))
	{
		Debug("No clipboard defined");
		return;
	}

	ret = Fopen(scrap_file, O_RDONLY);
	if(ret >= 0)
	{
		long error;
		char c, *ptr;
	
		ptr = AutolocatorInit(doc);

		error = Fread ( (short)ret, 1L, &c );
		while ( error == 1 )
		{
			if ( c == ' ')
			{
				/*	Leerzeichen am Anfang werden ignoriert	*/
				if(ptr != doc->autolocator)
					*ptr++ = c;
				*ptr = 0;
			}
			else if ( c > ' ')
			{
				if(ptr-doc->autolocator < AUTOLOC_SIZE)
				{
					*ptr++ = c;
					*ptr = 0;
				}
				else
				{
					--ptr;
					*ptr = c;
					break;
				}
			}
			error = Fread ( (short)ret, 1L, &c );
		}
		Fclose((short)ret);

		ToolbarUpdate(doc, win->toolbar, FALSE);
		AutolocatorUpdate(doc, win->y_pos);
	}
}
