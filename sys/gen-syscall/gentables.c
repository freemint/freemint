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


static void
generate_struct(FILE *out, struct systab *tab)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];
		
		if (call && call->args)
		{
			fprintf(out, "struct sys_%c_%s_args\n", call->class, call->name);
			fprintf(out, "{\n");
			
			generate_args(out, call->args, "\t", 1, ";\n");
			
			fprintf(out, "};\n");
			fprintf(out, "\n");
		}
	}
}

static void
generate_proto(FILE *out, struct systab *tab)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];
		
		if (call && IS_REGULAR_SYSCALL(call))
		{
			fprintf(out, "long _cdecl sys_%c_%s\t", call->class, call->name);
			
			if ((1 + strlen(call->name)) < (7))
				fprintf(out, "\t");
			
			fprintf(out, "(struct proc *");
			
			if (call->args)
				fprintf(out, ", struct sys_%c_%s_args", call->class, call->name);
			
			fprintf(out, ");\n");
		}
	}
}

static void
generate_tab(FILE *out, struct systab *tab, const char *prefix)
{
	int i, j;
	
	fprintf(out, "ushort %s_max = 0x%x;\n", prefix, tab->max);
	fprintf(out, "\n");
	fprintf(out, "struct systab %s_tab[ 0x%x ] =\n", prefix, tab->max);
	fprintf(out, "{\n");
	
	for (i = 0, j = 1; i < tab->size; i++, j++)
	{
		struct syscall *call = tab->table[i];
		
		fprintf(out, "/* 0x%03x */\t{ ", i);
		
		if (call)
		{
			int nr_args = arg_length(call->args);
			
			if (IS_REGULAR_SYSCALL(call))
			{
				fprintf(out, "sys_%c_%s, ", call->class, call->name);
				fprintf(out, "%i, ", nr_args);
				
				if (call->args)
					fprintf(out, "sizeof(struct sys_%c_%s_args)", call->class, call->name);
				else
					fprintf(out, "0");
			}
			else
			{
				fprintf(out, "sys_enosys, 0, 0");
			}
		}
		else
		{
			fprintf(out, "NULL, 0, 0");
		}
		
		fprintf(out, " },\n");
		
		if (j == 0x10)
		{
			j = 0;
			fprintf(out, "\n");
		}
	}
	
	fprintf(out, "/* 0x%03x */\t/* MAX */\n", tab->max);
	fprintf(out, "};\n");
}

static void
generate_wrapper(FILE *out, struct systab *tab, const char *prefix)
{
	int i, j;
	
	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];
		
		if (call && IS_REGULAR_SYSCALL(call))
		{
			fprintf(out, "static long\n");
			fprintf(out, "old_%c_%s(", call->class, call->name);
			
			if (call->args)
				fprintf(out, "struct sys_%c_%s_args args", call->class, call->name);
			else
				fprintf(out, "void");
			
			fprintf(out, ")\n");
			
			/* body */
			fprintf(out, "{\n");
			
			fprintf(out, "\treturn sys_%c_%s(curproc", call->class, call->name);
			if (call->args)
				fprintf(out, ", args");
			fprintf(out, ");\n");
			
			fprintf(out, "}\n");
			fprintf(out, "\n");
		}
	}
	
	fprintf(out, "\n");
	
	fprintf(out, "Func %s_tab_old [ 0x%x ] =\n", prefix, tab->max);
	fprintf(out, "{\n");
	
	for (i = 0, j = 1; i < tab->size; i++, j++)
	{
		struct syscall *call = tab->table[i];
		
		fprintf(out, "\t/* 0x%03x */\t", i);
		
		if (call)
		{
			if (IS_REGULAR_SYSCALL(call))
				fprintf(out, "old_%c_%s", call->class, call->name);
			else
				fprintf(out, "old_enosys");
		}
		else
			fprintf(out, "NULL");
		
		fprintf(out, ",\n");
		
		if (j == 0x10)
		{
			j = 0;
			fprintf(out, "\n");
		}
	}
	
	fprintf(out, "\t/* 0x%03x */\t/* MAX */\n", tab->max);
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
		fprintf(f, "# ifndef _syscalls_h\n# define _syscalls_h\n\n");
		fprintf(f, "# include \"mint/mint.h\"\n");
		fprintf(f, "# include \"mint/mem.h\"\n");
		fprintf(f, "\n");
		fprintf(f, "\n");
		fprintf(f, "struct systab\n");
		fprintf(f, "{\n");
		fprintf(f, "\tlong _cdecl (*call)();\n");
		fprintf(f, "\tushort nr_of_args;\n");
		fprintf(f, "\tushort size_of_args;\n");
		fprintf(f, "};\n");
		fprintf(f, "\n");
		fprintf(f, "\n");
		fprintf(f, "extern struct systab dos_tab[];\n");
		fprintf(f, "extern struct systab bios_tab[];\n");
		fprintf(f, "extern struct systab xbios_tab[];\n");
		fprintf(f, "\n");
		fprintf(f, "extern Func dos_tab_old[];\n");
		fprintf(f, "extern Func bios_tab_old[];\n");
		fprintf(f, "extern Func xbios_tab_old[];\n");
		fprintf(f, "\n");
		fprintf(f, "extern ushort dos_max;\n");
		fprintf(f, "extern ushort bios_max;\n");
		fprintf(f, "extern ushort xbios_max;\n");
		fprintf(f, "\n");
		
		fprintf(f, "\n/*\n * DOS syscalls argument structs\n */\n");
		generate_struct(f, gemdos_table());
		
		fprintf(f, "\n/*\n * BIOS syscalls argument structs\n */\n");
		generate_struct(f, bios_table());
		
		fprintf(f, "\n/*\n * XBIOS syscalls argument structs\n */\n");
		generate_struct(f, xbios_table());
		
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
		fprintf(f, "static long\n");
		fprintf(f, "sys_enosys(struct proc *p, void *v)\n");
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
		
		
		f = fopen("syscalls_old.c", "w");
		if (!f)
		{
			perror("syscalls_old.c");
			exit(1);
		}
		
		print_head(f, myname);
		fprintf(f, "# include \"syscalls.h\"\n");
		fprintf(f, "\n");
		fprintf(f, "# include \"mint/proc.h\"\n");
		fprintf(f, "\n");
		fprintf(f, "\n");
		fprintf(f, "static long\n");
		fprintf(f, "old_enosys(void)\n");
		fprintf(f, "{\n");
		fprintf(f, "\treturn ENOSYS;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
		
		fprintf(f, "\n/*\n * DOS old syscall wrapper & table\n */\n");
		generate_wrapper(f, gemdos_table(), "dos");
		
		fprintf(f, "\n/*\n * BIOS old syscall wrapper & table\n */\n");
		generate_wrapper(f, bios_table(), "bios");
		
		//not used
		//fprintf(f, "\n/*\n * XBIOS old syscall wrapper & table\n */\n");
		//generate_wrapper(f, xbios_table(), "xbios");
		
		fclose(f);
	}
	
	return error;
}
