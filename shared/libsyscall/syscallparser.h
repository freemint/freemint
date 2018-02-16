/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000-2005 Frank Naumann <fnaumann@freemint.de>
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

#ifndef _syscallparser_h
#define _syscallparser_h

#include <stdio.h>
#include "syscalldefs.h"

struct systab *gemdos_table(void);
struct systab *bios_table(void);
struct systab *xbios_table(void);

void print_head(FILE *out, const char *myname);
void generate_args(FILE *out, struct arg *l, const char *pre, int flag, const char *post);

int arg_length(struct arg *l);
int arg_size_bytes(struct arg *l);

int is_regular_syscall(struct syscall *call);
int is_passthrough_syscall(struct syscall *call);
int is_syscall(struct syscall *call);

int parse_syscall_description(FILE *infile);

#endif /* _syscallparser_h */
