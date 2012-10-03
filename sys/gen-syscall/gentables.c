/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
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
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libsyscall/syscalldefs.h"
#include "libsyscall/syscallparser.h"


#ifdef __MINT__
long _stksize = 64 * 1024;
#endif


static char *
lowercase(const char *src)
{
	char *d, *dst;
	
	d = dst = strdup(src);
	assert(dst);
	
	while (*d)
	{
		if (isupper((unsigned char)*d))
			*d = tolower((unsigned char)*d);
		d++;
	}
	
	return dst;
}

static void
generate_proto(FILE *out, struct systab *tab)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];
		
		if (call && is_regular_syscall(call))
		{
			char *name = lowercase(call->name);
			
			fprintf(out, "/*0x%03x*/ ", i);
			fprintf(out, "long _cdecl sys_%s\t", name);
			
			if (strlen(name) < 6)
				fprintf(out, "\t");
			
			fprintf(out, "(");
			
			if (call->args)
				generate_args(out, call->args, "", 0, ", ");
			else
				fprintf(out, "void");
			
			fprintf(out, ");\n");
			
			free(name);
		}
	}
}

static void
generate_tab(FILE *out, struct systab *tab, const char *prefix)
{
	int i, j;
	
	fprintf(out, "ushort %s_tab_max = 0x%x;\n", prefix, tab->max);
	fprintf(out, "\n");
	fprintf(out, "Func %s_tab[ 0x%x ] =\n", prefix, tab->max);
	fprintf(out, "{\n");
	
	for (i = 0, j = 1; i < tab->size; i++, j++)
	{
		struct syscall *call = tab->table[i];
		
		fprintf(out, "/*0x%03x*/\t", i);
		
		if (call)
		{
			if (is_regular_syscall(call))
			{
				char *name = lowercase(call->name);
				
				fprintf(out, "(Func) sys_%s", name);
				
				free(name);
			}
			else if (is_passthrough_syscall(call))
				fprintf(out, "NULL");
			else
				fprintf(out, "       sys_enosys");
		}
		else
			/* unspecified -> ENOSYS */
			fprintf(out, "sys_enosys");
		
		fprintf(out, ",\n");
		
		if (j == 0x10)
		{
			j = 0;
			fprintf(out, "\n");
		}
	}
	
	fprintf(out, "/* 0x%03x */\t/* MAX */\n", tab->max);
	fprintf(out, "};\n");
	fprintf(out, "\n");
	
	fprintf(out, "ushort %s_tab_argsize[ 0x%x ] =\n", prefix, tab->max);
	fprintf(out, "{\n");
	
	for (i = 0, j = 1; i < tab->size; i++, j++)
	{
		struct syscall *call = tab->table[i];
		
		fprintf(out, "/*0x%03x*/\t", i);
		
		if (call && is_regular_syscall(call))
			fprintf(out, "%i", arg_size_bytes(call->args) / 2);
		else
			fprintf(out, "0");
		
		fprintf(out, ",\n");
		
		if (j == 0x10)
		{
			j = 0;
			fprintf(out, "\n");
		}
	}
	
	fprintf(out, "/* 0x%03x */\t/* MAX */\n", tab->max);
	fprintf(out, "};\n");
}

int
main(int argc, char **argv)
{
	const char *myname = *argv++;
	int error;
	
	if (*argv == NULL)
	{
#if YYDEBUG != 0
		yydebug = 1;
#endif
		error = parse_syscall_description(NULL);
	}
	else
	{
		FILE *f;
		
		f = fopen(*argv, "rt");
		if (f)
		{
			error = parse_syscall_description(f);
			fclose(f);
		}
		else
		{
			perror(*argv);
			exit(1);
		}
	}
	
	if (!error)
	{
		FILE *f;
		
		f = fopen("syscalls.h", "w");
		if (!f)
		{
			perror("syscalls.h");
			exit(1);
		}
		
		print_head(f, myname);
		fprintf(f, "# ifndef _syscalls_h\n");
		fprintf(f, "# define _syscalls_h\n\n");
		fprintf(f, "# include \"mint/mint.h\"\n");
		fprintf(f, "# include \"mint/mem.h\"\n");
		fprintf(f, "# include \"mint/msg.h\"\n");
		fprintf(f, "# include \"mint/sem.h\"\n");
		fprintf(f, "# include \"mint/shm.h\"\n");
		fprintf(f, "# include \"mint/signal.h\"\n");
		fprintf(f, "# include \"mint/socket.h\"\n");
		fprintf(f, "# include \"mint/time.h\"\n");
		fprintf(f, "\n");
		fprintf(f, "\n");
		fprintf(f, "extern Func dos_tab[];\n");
		fprintf(f, "extern Func bios_tab[];\n");
		fprintf(f, "extern Func xbios_tab[];\n");
		fprintf(f, "\n");
		fprintf(f, "extern ushort dos_tab_argsize[];\n");
		fprintf(f, "extern ushort bios_tab_argsize[];\n");
		fprintf(f, "extern ushort xbios_tab_argsize[];\n");
		fprintf(f, "\n");
		fprintf(f, "extern ushort dos_tab_max;\n");
		fprintf(f, "extern ushort bios_tab_max;\n");
		fprintf(f, "extern ushort xbios_tab_max;\n");
		fprintf(f, "\n");
		
		fprintf(f, "\n/*\n * DOS syscalls prototypes\n */\n");
		generate_proto(f, gemdos_table());
		
		fprintf(f, "\n/*\n * BIOS syscalls prototypes\n */\n");
		generate_proto(f, bios_table());
		
		fprintf(f, "\n/*\n * XBIOS syscalls prototypes\n */\n");
		generate_proto(f, xbios_table());
		
		fprintf(f, "\n# endif /* _syscalls_h */\n");
		fclose(f);
		
		
		f = fopen("syscalls.c", "w");
		if (!f)
		{
			perror("syscalls.c");
			exit(1);
		}
		
		print_head(f, myname);
		fprintf(f, "# include \"syscalls.h\"\n\n");
		
		fprintf(f, "\n");
		fprintf(f, "static long _cdecl\n");
		fprintf(f, "sys_enosys(void)\n");
		fprintf(f, "{\n");
		fprintf(f, "\treturn ENOSYS;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
		
		fprintf(f, "\n/*\n * DOS syscall table\n */\n");
		generate_tab(f, gemdos_table(), "dos");
		
		fprintf(f, "\n/*\n * BIOS syscall table\n */\n");
		generate_tab(f, bios_table(), "bios");
		
		fprintf(f, "\n/*\n * XBIOS syscall table\n */\n");
		generate_tab(f, xbios_table(), "xbios");
		
		fclose(f);
	}
	
	return error;
}
