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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include "sysenttab.h"

// XXX -> header should be in mintlib
#include "../../sys/mint/arch/register.h"

#ifdef __MINT__
long _stksize = 64 * 1024;
#endif


static void enter_systrace(void);

int
main(int argc, char **argv)
{
	const char *command;
	pid_t child;
	
	argv++;
	if (*argv == NULL)
		exit(1);
	
	command = *argv;
	
	child = fork();
	if (child == 0)
	{
		/* in child */
		
		if (ptrace(PTRACE_TRACEME, 0, 0, 0))
		{
			perror("can't trace myself");
			exit(1);
		}
		
		execv(command, argv);
		perror("exec failed");
		exit(1);
	}
	
	/* parent */
	
	enter_systrace();
	
	return 0;
}

static void
analyze_syscall_gemdos(pid_t pid, long sp)
{
	size_t opcode;
	int r;
	
	r = ptrace(PTRACE_PEEKDATA, pid, (caddr_t)sp, 0);
	
	opcode = r >> 16;
	printf("current stack: 0x%x -> opcode 0x%03x\n", r, (int)opcode);
	
	if (opcode < tab_gemdos_size)
	{
		long argbuf[64];
		int i;
		
		printf("system call: \"%s\"\n", tab_gemdos[opcode].name);
		
		for (i = 0; i < tab_gemdos[opcode].argsize; i += 4)
		{
			r = ptrace(PTRACE_PEEKDATA, pid, (caddr_t)(sp+2+i), 0);
			
			assert((i/4) < (sizeof(argbuf)/sizeof(argbuf[0])));
			argbuf[i/4] = r;
		}
		
		tab_gemdos[opcode].print(pid, argbuf);
	}
}

static void
analyze_syscall(pid_t pid)
{
	struct reg regs;
	int r;
	
	r = ptrace(PTRACE_GETREGS, pid, (caddr_t)&regs, 0);
	if (r == -1)
	{
		printf("ptrace() -> %i (errno %i \"%s\")\n", r, errno, strerror(errno));
		return;
	}
	
	printf("SP: 0x%lx\n", regs.regs[15]); /* A7 */
	printf("PC: 0x%lx\n", regs.pc);
	
	r = ptrace(PTRACE_PEEKTEXT, pid, (caddr_t)(regs.pc - 4), 0);
	if (r == -1)
	{
		printf("ptrace() -> %i (errno %i \"%s\")\n", r, errno, strerror(errno));
		return;
	}
	
	r &= 0xffff;
	printf("executed instruction: 0x%x\n", r);
	
	switch (r)
	{
		case 0x4e41:
		{
			analyze_syscall_gemdos(pid, regs.regs[15]);
			break;
		}
	}
}

static void
enter_systrace(void)
{
	int syscall_waiting = 0;
	
	for (;;)
	{
		int status;
		pid_t pid;
		
		pid = waitpid(-1, &status, 0);
		printf("waitpid() -> %i\n", pid);
		
		if (WIFSTOPPED(status))
		{
			int r;
			
			printf("process stopped with signal \"%s\"\n",
				strsignal(WSTOPSIG(status)));
			
			if (syscall_waiting)
			{
				analyze_syscall(pid);
			}
			else
				syscall_waiting = 1;
			
			r = ptrace(PTRACE_SYSCALL, pid, (caddr_t)1, 0);
			if (r == -1)
			{
				printf("ptrace() -> %i (errno %i \"%s\")\n",
					r, errno, strerror(errno));
			}
		}
		else
		{
			if (WIFEXITED(status))
			{
				printf("process exited normally with code %i\n",
					WEXITSTATUS(status));
			}
			else if (WIFSIGNALED(status))
			{
				printf("process terminated due to signal \"%s\"\n",
					strsignal(WTERMSIG(status)));
			}
			else
				printf("unknown exit status???\n");
			
			break;
		}
	}
}
