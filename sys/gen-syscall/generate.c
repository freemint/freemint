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

# include "generate.h"

# include <stdlib.h>
# include <string.h>


static int
list_length (LIST *l)
{
	int length = 0;
	
	while (l)
	{
		length++;
		l = l->next;
	}
	
	return length;
}

static void
generate_args (FILE *out, LIST *l, const char *pre, int flag, const char *post)
{
	while (l)
	{
		fprintf (out, "%s%s ", pre, l->types);
		
		if (l->flags & FLAG_POINTER)
			fprintf (out, "*");
		
		fprintf (out, "%s", l->name);
		
		if (l->flags & FLAG_ARRAY)
			fprintf (out, " [%i]", l->ar_size);
		
		if (l->next || flag)
			fprintf (out, "%s", post);
		
		l = l->next;
	}
}

void
generate_struct (FILE *out, SYSTAB *tab)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		SYSCALL *call = tab->table [i];
		
		if (call)
		{
			if (call->args)
			{
				fprintf (out, "struct sys_%s_%s_args\n", call->class, call->name);
				fprintf (out, "{\n");
				
				generate_args (out, call->args, "\t", 1, ";\n");
				
				fprintf (out, "};\n");
				fprintf (out, "\n");
			}
		}
	}
}

void
generate_proto (FILE *out, SYSTAB *tab)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		SYSCALL *call = tab->table [i];
		
		if (call && strcmp (call->name, "RESERVED"))
		{
			fprintf (out, "long _cdecl sys_%s_%s\t", call->class, call->name);
			
			if ((strlen (call->class) + strlen (call->name)) < (7))
				fprintf (out, "\t");
			
			fprintf (out, "(struct proc *");
			
			if (call->args)
				fprintf (out, ", struct sys_%s_%s_args", call->class, call->name);
			
			fprintf (out, ");\n");
		}
	}
}

void
generate_tab (FILE *out, SYSTAB *tab, const char *prefix)
{
	int i, j;
	
	fprintf (out, "ushort %s_max = 0x%x;\n", prefix, tab->max);
	fprintf (out, "\n");
	fprintf (out, "struct systab %s_tab [ 0x%x ] =\n", prefix, tab->max);
	fprintf (out, "{\n");
	
	for (i = 0, j = 1; i < tab->size; i++, j++)
	{
		SYSCALL *call = tab->table [i];
		
		fprintf (out, "/* 0x%03x */\t{ ", i);
		
		if (call)
		{
			int nr_args = list_length (call->args);
			
			if (strcmp (call->name, "RESERVED"))
			{
				fprintf (out, "sys_%s_%s, ", call->class, call->name);
				fprintf (out, "%i, ", nr_args);
				
				if (call->args)
					fprintf (out, "sizeof (struct sys_%s_%s_args)", call->class, call->name);
				else
					fprintf (out, "0");
			}
			else
			{
				fprintf (out, "sys_enosys, 0, 0");
			}
		}
		else
		{
			fprintf (out, "NULL, 0, 0");
		}
		
		fprintf (out, " },\n");
		
		if (j == 0x10)
		{
			j = 0;
			fprintf (out, "\n");
		}
	}
	
	fprintf (out, "/* 0x%03x */\t/* MAX */\n", tab->max);
	fprintf (out, "};\n");
}

void
generate_wrapper (FILE *out, SYSTAB *tab, const char *prefix)
{
	int i, j;
	
	for (i = 0; i < tab->size; i++)
	{
		SYSCALL *call = tab->table [i];
		
		if (call && strcmp (call->name, "RESERVED"))
		{
			fprintf (out, "static long\n");
			fprintf (out, "old_%s_%s (", call->class, call->name);
			
			if (call->args)
				fprintf (out, "struct sys_%s_%s_args args", call->class, call->name);
			else
				fprintf (out, "void");
			
			fprintf (out, ")\n");
			
			/* body */
			fprintf (out, "{\n");
			
			fprintf (out, "\treturn sys_%s_%s (curproc", call->class, call->name);
			if (call->args)
				fprintf (out, ", args");
			fprintf (out, ");\n");
			
			fprintf (out, "}\n");
			fprintf (out, "\n");
		}
	}
	
	fprintf (out, "\n");
	
	fprintf (out, "Func %s_tab_old [ 0x%x ] =\n", prefix, tab->max);
	fprintf (out, "{\n");
	
	for (i = 0, j = 1; i < tab->size; i++, j++)
	{
		SYSCALL *call = tab->table [i];
		
		fprintf (out, "\t/* 0x%03x */\t", i);
		
		if (call)
		{
			if (strcmp (call->name, "RESERVED"))
				fprintf (out, "old_%s_%s", call->class, call->name);
			else
				fprintf (out, "old_enosys");
		}
		else
			fprintf (out, "NULL");
		
		fprintf (out, ",\n");
		
		if (j == 0x10)
		{
			j = 0;
			fprintf (out, "\n");
		}
	}
	
	fprintf (out, "\t/* 0x%03x */\t/* MAX */\n", tab->max);
	fprintf (out, "};\n");
}
