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
	#include <osbind.h>
#else
	#include <tos.h>
#endif
#include "diallib.h"
#include "defs.h"

extern WINDOW_DATA *Win;

HISTORY *history = NULL;			/*	Pointer auf die History-Daten	*/

void AddHistoryEntry(WINDOW_DATA *the_win)
{
	WINDOW_DATA *win = Win;
	HISTORY *new_entry;
	DOCUMENT *doc = the_win->data;
	long memsize = sizeof(HISTORY) + strlen(the_win->title) + 1;

	new_entry = (HISTORY *)Malloc(memsize);
	
	if(!new_entry)
	{
		Debug("ERROR: out of memory => can't add new history entry");
		return;
	}
	new_entry->win = the_win;
	new_entry->doc = doc;
	new_entry->node = doc->getNodeProc(doc);
	new_entry->line = win->y_pos;
	new_entry->next=history;
	new_entry->title=' ';
	strcpy(&new_entry->title + 1, the_win->title);

	history = new_entry;				/*	History Pointer anpassen	*/
}

short RemoveHistoryEntry(DOCUMENT **doc,long *node,long *line)
{
	WINDOW_DATA *win = Win;
	HISTORY *entry=history;
	
	if(entry == NULL)
		return(FALSE);

	if(entry->win == win)
		history = entry->next;
	else
	{
		HISTORY *prev = entry;
		entry = entry->next;
		while(entry)
		{
			if(entry->win == win)
			{
				prev->next = entry->next;
				break;
			}
			prev = entry;
			entry = entry->next;
		}
	}

	if(!entry)
		return(FALSE);

	*node = entry->node;
	*line = entry->line;
	*doc = entry->doc;

	Mfree(entry);
	return(TRUE);
}

/*	Entfernt alle Eintrge, die mit <win> verbunden sind	*/
void RemoveAllHistoryEntries(WINDOW_DATA *win)
{
	HISTORY *entry=history;
	HISTORY *previous;

	while(entry)
	{
		if(entry->win != win)
			break;
		history = history->next;
		Mfree(entry);
		entry = history;
	}

	/*	Keine weiteren Eintraege? ... Ende	*/
	if(!entry)
		return;

	previous = entry;
	entry = previous->next;
	while(entry)
	{
		if(entry->win == win)
		{
			previous->next = entry->next;
			Mfree(entry);
		}
		else
			previous = entry;

		entry = previous->next;
	}
}

short CountWindowHistoryEntries(WINDOW_DATA *win)
{
	HISTORY *entry = history;
	short num = 0;
	
	while(entry)
	{
		if(entry->win == win)
			num++;
		entry = entry->next;
	}
	return(num);
}

short CountDocumentHistoryEntries(DOCUMENT *doc)
{
	HISTORY *entry = history;
	short num = 0;

	while(entry)
	{
		if(entry->doc == doc)
			num++;
		entry=entry->next;
	}
	return(num);
}

void DeletLastHistory(void *entry)
{
	HISTORY *previous;

	previous = (HISTORY *)entry;
	entry = previous->next;
	while(entry)
	{
		previous->next = ((HISTORY *)entry)->next;
		Mfree(entry);
		entry = previous->next;
	}
}

void *GetLastHistory(void)
{
	long memsize;
	HISTORY *last = NULL;
	HISTORY *entry = history;
	HISTORY *new_entry;
	
	while (entry)
	{
		memsize = sizeof(HISTORY) + strlen(&entry->title) + 1;
		new_entry = (HISTORY *)Malloc(memsize);
		if(!new_entry)
		{
			Debug("ERROR: out of memory => can't add new (last)history entry");
			return last;
		}
		new_entry->doc = entry->doc;
		new_entry->node = entry->node;
		new_entry->line = entry->line;
		new_entry->next = last;
		strcpy(&new_entry->title, &entry->title);
		last = new_entry;			/*	History Pointer anpassen	*/

		entry = entry->next;
	}
	return last;
}

void SetLastHistory(WINDOW_DATA *the_win,void *last)
{
	long memsize;
	HISTORY *entry=last;
	HISTORY *new_entry;

	while(entry)
	{
		memsize=sizeof(HISTORY)+strlen(&entry->title)+1;
		new_entry=(HISTORY *)Malloc(memsize);
		if(!new_entry)
		{
			Debug("ERROR: out of memory => can't add new history entry");
			return;
		}
		new_entry->win=the_win;
		new_entry->doc=entry->doc;
		new_entry->node=entry->node;
		new_entry->line=entry->line;
		new_entry->next=history;
		strcpy(&new_entry->title,&entry->title);
		history=new_entry;				/*	History Pointer anpassen	*/

		entry=entry->next;
	}
}
