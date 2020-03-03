/*
 * COPS (c) 1995 - 2003 Sven & Wilfried Behne
 *                 2004 F.Naumann & O.Skancke
 *
 * A XCONTROL compatible CPX manager.
 *
 * This file is part of COPS.
 *
 * COPS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * COPS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*----------------------------------------------------------------------------------------*/
/* Routinen zum Verwalten einer einfach verketteten Liste. */
/* LIST_ENTRY * ist immer ein Zeiger auf die nachfolgende Struktur, z.B. &FONT->next */
/*----------------------------------------------------------------------------------------*/

#include <stddef.h>
#include "list.h"

#define	NEXT(ptr, offset)	(*(void **) ((char *)ptr + offset))

/*----------------------------------------------------------------------------------------*/
/* Eintrag aus der verketteten Liste entfernen */
/* Funktionsresultat:	0: Fehler 1: Eintrag entfernt */
/*	root:		Zeiger auf den Zeiger auf den ersten Eintrag */
/* entry:		Zeiger auf den Eintrag */
/*----------------------------------------------------------------------------------------*/
short
list_remove(void **root, void *entry, long offset)
{
	void *search = (void *)((char *)root - offset); /* virtueller Anfangseintrag */

	while (NEXT(search, offset)) /* Nachfolger vorhanden? */
	{
		void *prev;

		prev = search;
		search = NEXT(search, offset);
		
		if (search == entry) /* gesuchter Eintrag? */
		{
			NEXT(prev, offset) = NEXT(search, offset); /* Eintrag ausketten */
			return 1;
		}
		
	}
	return 0;
}

/*----------------------------------------------------------------------------------------*/
/* Eintrag als erstes Element in die verkettete Liste einfuegen */
/* Funktionsresultat:	1: alles in Ordnung */
/*	root:		Zeiger auf den Zeiger auf den ersten Eintrag */
/*	entry:		Zeiger auf den Eintrag */
/*----------------------------------------------------------------------------------------*/
void
list_insert(void **root, void *entry, long offset)
{
	NEXT(entry, offset) = *root;
	*root = entry;
}

/*----------------------------------------------------------------------------------------*/
/* Eintrag and die verkettete Liste anhaengen */
/* Funktionsresultat:	1: alles in Ordnung */
/*	root:		Zeiger auf den Zeiger auf den ersten Eintrag */
/*	entry:		Zeiger auf den Eintrag */
/*----------------------------------------------------------------------------------------*/
void
list_append(void **root, void *entry, long offset)
{
	void *append = (void *)((char *) root - offset); /* virtueller Anfangseintrag */

	while (NEXT(append, offset)) /* Nachfolger vorhanden? */
		append = NEXT(append, offset); /* Zeiger auf den Nachfolger */

	NEXT(append, offset) = entry; /* Eintrag anhaengen */
}

/*----------------------------------------------------------------------------------------*/
/* Eintrag in einer Liste suchen */
/* Funktionsresultat:	Zeiger auf den Eintrag oder 0L, wenn nicht vorhanden */
/*	root:		Zeiger auf den Zeiger auf den ersten Eintrag */
/*	what:		zu suchender Wert */
/*	cmp_entries:	Vergleichsfunktion */
/*----------------------------------------------------------------------------------------*/
void *
list_search(void *root, long what, long offset, short (*cmp_entries)(long what, void *entry))
{
	void *search = root;

	while (search)
	{
		if (cmp_entries(what, search) == 0) /* gesuchter Eintrag? */
			return search;
			
		search = NEXT(search, offset);
	}

	return NULL; /* Eintrag wurde nicht gefunden */
}

/*----------------------------------------------------------------------------------------*/
/* Eintrage zaehlen */
/* Funktionsresultat:	Anzahl der Eintraege */
/*	root:		Zeiger auf den ersten Eintrag */
/*----------------------------------------------------------------------------------------*/
long
list_count(void *root, long offset)
{
	long count = 0;

	while (root)
	{
		count++;
		root = NEXT(root, offset);
	}

	return count;
}
