/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
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

#ifndef _print_h
#define _print_h

#include <sys/types.h>

/* generic types */

void print_int(pid_t pid, int data);
void print_long(pid_t pid, long data);
void print_pointer(pid_t pid, const void *data);
void print_string(pid_t pid, const char *data);

/* special types */

void print_struct_dtabuf(pid_t pid, const void *data);
void print_struct_xattr(pid_t pid, const void *data);
void print_struct_stat(pid_t pid, const void *data);
void print_struct_sigaction(pid_t pid, const void *data);
void print_struct_timeval(pid_t pid, const void *data);
void print_struct_timezone(pid_t pid, const void *data);
void print_struct_pollfd(pid_t pid, const void *data);
void print_struct_iovec(pid_t pid, const void *data);
void print_struct_sockaddr(pid_t pid, const void *data);
void print_struct_msghdr(pid_t pid, const void *data);
void print_struct_shmid_ds(pid_t pid, const void *data);
void print_union___semun(pid_t pid, const void *data);
void print_struct_sembuf(pid_t pid, const void *data);
void print_struct_msqid_ds(pid_t pid, const void *data);

typedef void SHARED_LIB;
typedef void SLB_EXEC;

void print_SHARED_LIB(pid_t pid, const void *data);
void print_SLB_EXEC(pid_t pid, const void *data);

#endif /* _print_h */
