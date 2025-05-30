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


void
generate_struct(FILE *out, struct syscall *call, const char *pre, const char *name)
{
	if (call->args)
	{
		/* '\t' + '\0' */
		char buf[strlen(pre)+1+1];
		
		fprintf(out, "%sstruct %s_%s_args\n", pre, name, call->name);
		fprintf(out, "%s{\n", pre);
		
		strcpy(buf, pre);
		strcat(buf, "\t");
		generate_args(out, call->args, buf, 1, ";\n");
		
		fprintf(out, "%s};\n", pre);
		fprintf(out, "%s\n", pre);
	}
}

void
generate_printer_impl(FILE *out, struct syscall *call, const char *pre)
{
	struct arg *arg = call->args;
	int flag = 0;
	
	while (arg)
	{
		if (flag)
		{
			fprintf(out, "%sprintf(\", \");\n", pre);
			flag = 0;
		}
		
		if (arg->flags & (FLAG_POINTER|FLAG_ARRAY))
		{
			if (arg->type == TYPE_CHAR)
			{
				fprintf(out, "%sprint_string(pid, args->%s);\n", pre, arg->name);
			}
			else if (arg->type == TYPE_IDENT)
			{
				char *tmp = strdup(arg->types);
				assert(tmp);
				
				/* convert blank to underscore */
				{ char *s = tmp; while (*s) { if (isspace((unsigned char)*s)) *s = '_'; s++; } }
				
				fprintf(out, "%sprint_%s(pid, args->%s);\n", pre, tmp, arg->name);
				free(tmp);
			}
			else
				fprintf(out, "%sprint_pointer(pid, args->%s);\n", pre, arg->name);
			
			flag = 1;
		}
		else switch (arg->type)
		{
			default:
			case TYPE_IDENT:
			case TYPE_VOID:
				break;
			case TYPE_INT:
			case TYPE_CHAR: 
			case TYPE_SHORT:
			case TYPE_UNSIGNED:
			case TYPE_UCHAR:
			case TYPE_USHORT:
				fprintf(out, "%sprint_int(pid, args->%s);\n", pre, arg->name);
				flag = 1;
				break;
			case TYPE_LONG:
			case TYPE_ULONG:
				fprintf(out, "%sprint_long(pid, args->%s);\n", pre, arg->name);
				flag = 1;
				break;
		}
		
		arg = arg->next;
	}
}

static void
my_strupr(char *s)
{
	int c;
	
	while ((c = *s))
		*s++ = toupper(c);
}

static void
generate_strace_printer(FILE *out, struct systab *tab, const char *name)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];
		
		if (call && is_syscall(call))
		{
			char *tmp;
			
			tmp = strdup(name);
			assert(tmp);
			my_strupr(tmp);
			
			fprintf(out, "#ifndef %s_%s\n", tmp, call->name);
			fprintf(out, "static void\n");
			fprintf(out, "print_%s_%s(pid_t pid, long *argbuf)\n", name, call->name);
			fprintf(out, "{\n");
			
			if (call->args)
			{
				generate_struct(out, call, "\t", name);
				fprintf(out, "\tstruct %s_%s_args *args = ", name, call->name);
				fprintf(out, "(struct %s_%s_args *)argbuf;\n", name, call->name);
				fprintf(out, "\n");
				fprintf(out, "\tprintf(\"%s(\");\n", call->name);
				generate_printer_impl(out, call, "\t");
				fprintf(out, "\tprintf(\")\\n\");\n");
			}
			
			fprintf(out, "}\n");
			fprintf(out, "#endif /* %s_%s */\n", tmp, call->name);
			fprintf(out, "\n");
			
			free(tmp);
		}
	}
}

static void
generate_strace_tab(FILE *out, struct systab *tab, const char *name)
{
	int i;
	
	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];
		
		fprintf(out, "/* 0x%03x */ { ", i);
		
		if (call && is_syscall(call))
		{
			fprintf(out, "%2i, ", arg_size_bytes(call->args));
			fprintf(out, "\"%s\", ", call->name);
			fprintf(out, "print_%s_%s", name, call->name);
		}
		else
			fprintf(out, " 0, \"unknown opcode 0x%03x\", NULL", i);
		
		fprintf(out, " },\n");
	}
}

static void
generate_strace(FILE *out, struct systab *tab, const char *name)
{
	generate_strace_printer(out, tab, name);
	fprintf(out, "struct sysent tab_%s[] =\n", name);
	fprintf(out, "{\n");
	generate_strace_tab(out, tab, name);
	fprintf(out, "};\n");
	fprintf(out, "unsigned long tab_%s_size = sizeof(tab_%s)/sizeof(tab_%s[0]);\n", name, name, name);
	fprintf(out, "\n");
	fprintf(out, "\n");
}

static void
generate_strace_header(FILE *out, struct systab *tab, const char *name)
{
	fprintf(out, "extern struct sysent tab_%s[];\n", name);
	fprintf(out, "extern unsigned long tab_%s_size;\n", name);
	fprintf(out, "\n");
}

static int
simplify_type(int type)
{
	switch (type)
	{
		case TYPE_INT:
		case TYPE_SHORT:
		case TYPE_UNSIGNED:
		case TYPE_USHORT:
		case TYPE_LONG:
		case TYPE_ULONG:
			type = TYPE_INT;
			break;
	}

	return type;
}

static struct arg *args[512]; 
static int args_size = sizeof(args) / sizeof(args[0]);

static int
lookup_arg(struct arg *_arg)
{
	int i;

	for (i = 0; i < args_size; i++)
	{
		struct arg *check = args[i];

		if (check)
		{
			struct arg *arg = _arg;
			int equal = 1;

			while (check && arg)
			{
				int type1 = check->type;
				int type2 = arg->type;

				int flags1 = check->flags;
				int flags2 = arg->flags;

				/* simplify types */
				if (!(flags1 & (FLAG_POINTER|FLAG_ARRAY)))
					type1 = simplify_type(type1);

				if (!(flags2 & (FLAG_POINTER|FLAG_ARRAY)))
					type2 = simplify_type(type2);

				/* ignore const qualifier */
				flags1 &= ~(FLAG_CONST);
				flags2 &= ~(FLAG_CONST);

				if (type1 != type2 || flags1 != flags2)
				{
					equal = 0;
					break;
				}

				if (type1 == TYPE_IDENT
				    && strcmp(check->types, arg->types) != 0)
				{
					equal = 0;
					break;
				}

				check = check->next;
				arg = arg->next;
			}

			if (check || arg)
				equal = 0;

			if (equal)
				return 1;
		}
	}

	return 0;
}

static void
insert_arg(struct arg *src)
{
	int i;

	for (i = 0; i < args_size; i++)
	{
		if (!args[i])
		{
			args[i] = src;
			break;
		}
	}
}

static int
determine_types(struct systab *tab)
{
	int types = 1; /* void syscalls */
	int i;

	for (i = 0; i < args_size; i++)
		args[i] = NULL;

	for (i = 0; i < tab->size; i++)
	{
		struct syscall *call = tab->table[i];

		if (call && is_syscall(call))
		{
			if (call->args)
			{
				if (!lookup_arg(call->args))
					insert_arg(call->args);
			}
		}
	}

	for (i = 0; i < args_size; i++)
	{
		if (args[i])
			types++;
	}

	return types;
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
		FILE *out;
		
		out = fopen("sysenttab.h", "w");
		if (out)
		{
			print_head(out, myname);
			
			fprintf(out, "#ifndef _sysenttab_h\n");
			fprintf(out, "#define _sysenttab_h\n");
			fprintf(out, "\n");
			fprintf(out, "struct sysent\n");
			fprintf(out, "{\n");
			fprintf(out, "\tint argsize; /* in bytes */\n");
			fprintf(out, "\tconst char *name; /* syscall name */\n");
			fprintf(out, "\tvoid (*print)(pid_t pid, long *argbuf); /* printing function */\n");
			fprintf(out, "};\n");
			fprintf(out, "\n");
			generate_strace_header(out, gemdos_table(), "gemdos");
			generate_strace_header(out, bios_table(), "bios");
			generate_strace_header(out, xbios_table(), "xbios");
			fprintf(out, "\n");
			fprintf(out, "#endif /* _sysenttab_h */\n");
			
			fclose(out);
		}
		else
		{
			perror("sysenttab.h");
			exit(1);
		}
		
		out = fopen("sysenttab.c", "w");
		if (out)
		{
			print_head(out, myname);
			
			fprintf(out, "#include <stdlib.h>\n");
			fprintf(out, "#include <stdio.h>\n");
			fprintf(out, "#include <mint/ostruct.h>\n");
			fprintf(out, "#include \"sysenttab.h\"\n");
			fprintf(out, "#include \"print.h\"\n");
			fprintf(out, "\n");
			generate_strace(out, gemdos_table(), "gemdos");
			generate_strace(out, bios_table(), "bios");
			generate_strace(out, xbios_table(), "xbios");
			
			fclose(out);
		}
		else
		{
			perror("sysenttab.c");
			exit(1);
		}
		
		printf("GEMDOS syscall types: %i\n", determine_types(gemdos_table()));
		printf("BIOS syscall types: %i\n", determine_types(bios_table()));
		printf("XBIOS syscall types: %i\n", determine_types(xbios_table()));
	}
	
	return error;
}
