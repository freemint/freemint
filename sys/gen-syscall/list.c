/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * 
 * begin:	2000-01-01
 * last change:	2000-03-07
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * known bugs:
 * 
 * todo:
 * 
 * optimizations:
 * 
 */

# include "list.h"

# include <stdlib.h>
# include <string.h>


SYSTAB *DOS;
SYSTAB *BIOS;
SYSTAB *XBIOS;


int
resize_tab (SYSTAB *tab, int newsize)
{
	if (newsize > tab->size)
	{
		SYSCALL **newtable = malloc (newsize * sizeof (*newtable));
		
		if (newtable)
		{
			bzero (newtable, newsize * sizeof (*newtable));
			memcpy (newtable, tab->table, tab->size * sizeof (*(tab->table)));
			
			free (tab->table);
			tab->table = newtable;
			tab->size = newsize;
			
			return 1;
		}
	}
	else
	{
		tab->size = newsize;
		
		return 1;
	}
	
	return 0;
}

int
add_tab (SYSTAB *tab, int nr, const char *class, const char *name, LIST *p)
{
	SYSCALL *call = NULL;
	
	if (tab->size <= nr)
		if (!resize_tab (tab, nr + 100))
			return 1;
	
	if (name)
	{
		call = malloc (sizeof (*call));
		if (!call)
			return 1;
		
		bzero (call, sizeof (*call));
		
		if (class)
			strcpy (call->class, class);
		
		strcpy (call->name, name);
		call->args = p;
	}
	
	tab->table [nr] = call;
	
	if (tab->maxused < nr)
		tab->maxused = nr;
	
	return 0;
}

void
add_list (LIST **head, LIST *t)
{	
	while (*head)
		head = &((*head)->next);
	
	t->next = NULL;
	*head = t;
}

LIST *
make_list (int type, const char *s)
{
	LIST *l = malloc (sizeof (*l));
	
	if (l)
	{
		bzero (l, sizeof (*l));
		
		if (s) strcpy (l->types, s);
		l->type = type;
	}
	
	return l;
}
