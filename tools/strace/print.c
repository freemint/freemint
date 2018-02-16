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

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/ptrace.h>

/* XXX -> mintlib */
typedef long long llong;

#include "../../sys/mint/stat.h"
#include "../../sys/mint/un.h"
#include <netinet/in.h>

#include "print.h"


/* generic types */

void
print_int(pid_t pid, int data)
{
	printf("%i", data);
}

void
print_long(pid_t pid, long data)
{
	printf("%li", data);
}

void
print_pointer(pid_t pid, const void *data)
{
	if (data)
		printf("%p", data);
	else
		printf("NULL");
}

static char *
copy_character(char *dst, char *end, int c)
{
	if (dst && (dst < end) && c)
	{
		if (c == '\n')
		{
			*dst++ = '\\';
			*dst++ = 'n';
		}
		else if (isprint(c))
		{
			*dst++ = c;
		}
		else
			dst = NULL;
	}
	else
		dst = NULL;
	
	return dst;
}

void
print_string(pid_t pid, const char *data)
{
	char *buf;
	size_t buflen;
	
	buflen = 1024;
	buf = malloc(buflen);
	if (buf)
	{
		char *dst, *end;
		size_t i;
		
		memset(buf, 0, buflen);
		
		dst = buf;
		end = buf + buflen;
		i = 0;
		
		for (;;)
		{
			long r = ptrace(PTRACE_PEEKDATA, pid, (caddr_t)(data+i), 0);
			i += 4;
			
			dst = copy_character(dst, end, (r >> 24) & 0xff);
			dst = copy_character(dst, end, (r >> 16) & 0xff);
			dst = copy_character(dst, end, (r >>  8) & 0xff);
			dst = copy_character(dst, end, (r      ) & 0xff);
			
			if (!dst)
				break;
		}
		
		printf("\"%s\"", buf);
		free(buf);
	}
}


/* special types */

static void
copy_bytes(pid_t pid, const char *start, unsigned char *buf, size_t len)
{
	size_t i = 0, ibuf = 0;
	
	for (;;)
	{
		long r = ptrace(PTRACE_PEEKDATA, pid, (caddr_t)(start+i), 0);
		i += 4;
		
		buf[ibuf++] = (r >> 24) & 0xff; if (ibuf == len) break;
		buf[ibuf++] = (r >> 16) & 0xff; if (ibuf == len) break;
		buf[ibuf++] = (r >>  8) & 0xff; if (ibuf == len) break;
		buf[ibuf++] = (r      ) & 0xff; if (ibuf == len) break;
	}
}

void
print_struct_dtabuf(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_xattr(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_stat(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_sigaction(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_timeval(pid_t pid, const void *data)
{
	struct timeval tv;
	
	copy_bytes(pid, data, (char *)(&tv), sizeof(tv));
	
	printf("struct timeval { %li, %li }", tv.tv_sec, tv.tv_usec);
}

void
print_struct_timezone(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_pollfd(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_iovec(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

extern char *inet_ntoa (struct in_addr);

void
print_struct_sockaddr(pid_t pid, const void *data)
{
	struct sockaddr sa;
	
	if (!data)
	{
		printf("NULL");
		return;
	}
	
	copy_bytes(pid, data, (unsigned char *)(&sa), sizeof(sa));
	
	switch (sa.sa_family)
	{
		case AF_INET:
		{
			struct sockaddr_in sin;
			
			copy_bytes(pid, data, (unsigned char *)(&sin), sizeof(sin));
			
			printf("struct sockaddr_in { port %i, addr %s }",
				sin.sin_port, inet_ntoa(sin.sin_addr));
			break;
		}
		case AF_UNIX:
		{
			struct sockaddr_un sun;
			
			copy_bytes(pid, data, (unsigned char *)(&sun), sizeof(sun));
			
			printf("struct sockaddr_un { \"%s\" }", sun.sun_path);
			break;
		}
		default:
			printf("struct sockaddr { sa_family %i }", sa.sa_family);
			break;
	}
}


void
print_struct_msghdr(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_shmid_ds(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_union___semun(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_sembuf(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_struct_msqid_ds(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_SHARED_LIB(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}

void
print_SLB_EXEC(pid_t pid, const void *data)
{
	print_pointer(pid, data);
}
