/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
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
 * Author: Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * Started: 1999-08-17
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 *
 * Purpose:
 * Routines for MagiC-style "shared libraries". EXPERIMENTAL! USE WITH CAUTION!
 *
 * History:
 * 00/05/02: - Removed proc_self and pbaseaddr, as slb_util.spp no longer needs
 *             them (Gryf)
 * 99/08/18: - open() and close() of a shared library now get the address of
 *             the basepage of the calling process, instead of its PID (Gryf)
 *           - Added string and long constant needed by changes in slb_util.spp
 *             (Gryf)
 *           - Correctly initialized slb_list to 0L (Gryf)
 * 99/07/04-
 * 99/08/17: - Creation, with pauses (Gryf)
 *
 */

# include "slb.h"

# include "libkern/libkern.h"
# include "mint/basepage.h"
# include "mint/signal.h"

# include "arch/mprot.h"
# include "arch/user_things.h"

# include "dos.h"
# include "dosmem.h"
# include "dossig.h"
# include "global.h"		/* sysdir */
# include "info.h"
# include "k_exec.h"
# include "k_exit.h"
# include "kmemory.h"
# include "memory.h"
# include "proc.h"		/* proc_clock */
# include "signal.h"
# include "util.h"

/* The linked list of used SLBs */
SHARED_LIB *slb_list = NULL;

/*
 * mark_users
 *
 * Helper function that (un)marks a process as user of a shared library.
 *
 * Input:
 * sl: The SLB's descriptor
 * pid: The PID of the process to be (un)marked
 * setflag: Mark (1) or unmark (0) the process as user
 */
INLINE void
mark_users (SHARED_LIB *sl, int pid, int setflag)
{
	if (setflag)
		sl->slb_users [pid/8] |= 1 << (pid % 8);
	else
		sl->slb_users [pid/8] &= ~(1 << (pid % 8));
}

/*
 * is_user
 *
 * Helper function that checks whether a process is user of a shared library.
 *
 * Input:
 * sl: The SLB's descriptor
 * pid: The PID of the process in question
 *
 * Returns:
 * 0: Process is not user of the SLB
 * 1: Process is user of the SLB
 */
INLINE int
is_user (SHARED_LIB *sl, int pid)
{
	return (sl->slb_users [pid/8] & (1 << (pid % 8)));
}

/*
 * mark_opened
 *
 * Helper function that (un)marks a library as having successfully open()ed a
 * shared library.
 *
 * Input:
 * sl: The SLB's descriptor
 * pid: The PID of the process to be (un)marked
 * setflag: Mark (1) or unmark (0) the process as having opened the SLB
 */
INLINE void
mark_opened (SHARED_LIB *sl, int pid, int setflag)
{
	if (setflag)
		sl->slb_opened [pid/8] |= 1 << (pid % 8);
	else
		sl->slb_opened [pid/8] &= ~(1 << (pid % 8));
}

/*
 * has_opened
 *
 * Helper function that checks whether a process has successfully open()ed a
 * shared library.
 *
 * Input:
 * sl: The SLB's descriptor
 * pid: The PID of the process in question
 *
 * Returns:
 * 0: Process has not open()ed the SLB
 * 1: Process has open()ed the SLB
 */
INLINE int
has_opened (SHARED_LIB *sl, int pid)
{
	return (sl->slb_opened [pid/8] & (1 << (pid % 8)));
}

/*
 * load_and_init_slb
 *
 * Helper function for s_lbopen(); loads and initializes a shared library.
 *
 * Input:
 * (See s_lbopen())
 *
 * Returns:
 * 0: SLB has been successfully loaded and initialized
 * Otherwise: GEMDOS error
 */
static long
load_and_init_slb(char *name, char *path, long min_ver, SHARED_LIB **sl, int *continue_search)
{
	union { char *c; long *l;} ptr;
	int new_pid, prot_cookie;
	long r, hitpa, oldsigint, oldsigquit, oldcmdlin, *exec_longs;
	char *fullpath;
	BASEPAGE *b;
	MEMREGION *mr;
	int pathlen;

	/* Construct the full path name of the SLB */
	pathlen = (int)strlen(path);
	fullpath = kmalloc(pathlen + strlen(name) + 2);
	if (!fullpath)
	{
		DEBUG(("Slbopen: Couldn't kmalloc() full pathname"));
		return(ENOMEM);
	}
	strcpy(fullpath, path);
	if (pathlen && path[pathlen - 1] != '/' && path[pathlen - 1] != '\\')
		strcat(fullpath, "/");
	strcat(fullpath, name);

	/* Create the new shared library structure */
	mr = get_region(alt, sizeof(SHARED_LIB) + strlen(name), PROT_PR);
	if (mr == 0L)
	{
		mr = get_region(core, sizeof(SHARED_LIB) + strlen(name), PROT_PR);
	}
	if (mr == 0L)
	{
		DEBUG(("Slbopen: Couldn't get region for new SHARED_LIB structure"));
		kfree(fullpath);
		return(ENOMEM);
	}
	mark_proc_region(get_curproc()->p_mem, mr, PROT_G, get_curproc()->pid);
	*sl = (SHARED_LIB *)mr->loc;
	mint_bzero(*sl, sizeof(SHARED_LIB));
	(*sl)->slb_region = mr;

	/* Load, but don't run the SLB */
	r = sys_pexec (3, fullpath, fullpath, 0L);
	kfree(fullpath);
	if (r <= 0L)
	{
		DEBUG(("Slbopen: Couldn't create basepage"));
		if (--mr->links == 0)
			free_region(mr);
		return(r);
	}

	/*
	 * If the base of the text segment isn't 256 bytes behind the basepage,
	 * then the SLB has the shared text bit set - we don't want that!
	 */
	b = (BASEPAGE *)r;
	if (b->p_tbase != (b->p_lowtpa + 256L))
	{
		DEBUG(("Slbopen: SLB with shared text bit?!"));
		r = ENOEXEC;
slb_error:
		sys_m_free(b->p_env);
		sys_m_free(b);
		if (--mr->links == 0)
			free_region(mr);
		return(r);
	}

	/* Test for the new program format */
	exec_longs = (long *) b->p_tbase;
	if (
	     /* Original binutils */
	     (exec_longs[0] == 0x283a001aL && exec_longs[1] == 0x4efb48faL)
	     /* binutils >= 2.18-mint-20080209 */
	     || (exec_longs[0] == 0x203a001aL && exec_longs[1] == 0x4efb08faL)
	   )
	{
		(*sl)->slb_head = (SLB_HEAD *)(b->p_tbase + 228);
	} else if ((exec_longs[0] & 0xffffff00L) == 0x203a0000L &&              /* binutils >= 2.41-mintelf */
		exec_longs[1] == 0x4efb08faUL &&
		/*
		 * 40 = (minimum) offset of elf header from start of file
		 * 24 = offset of e_entry in common header
		 * 30 = branch offset (sizeof(GEMDOS header) + 2)
		 */
		(exec_longs[0] & 0xff) >= (40 + 24 - 30))
	{
		long elf_offset;
		long e_entry;

		elf_offset = (exec_longs[0] & 0xff);
		e_entry = *((long *)((char *)b->p_tbase + elf_offset + 2));
		(*sl)->slb_head = (SLB_HEAD *) ((char *) b->p_tbase + e_entry);
	}
	else
	{
		(*sl)->slb_head = (SLB_HEAD *)b->p_tbase;
	}

	/* Check the magic value */
	if ((*sl)->slb_head->slh_magic != SLB_HEADER_MAGIC)
	{
		DEBUG(("Slbopen: SLB is missing the magic value"));
		r = ENOEXEC;
		goto slb_error;
	}

	/* Check the name */
	if (stricmp((*sl)->slb_head->slh_name, name))
	{
		DEBUG(("Slbopen: name mismatch (SLB: %s, file: %s)",
			(*sl)->slb_head->slh_name, name));
		r = ENOENT;
		goto slb_error;
	}

	/* Check the version number */
	(*sl)->slb_version = (*sl)->slb_head->slh_version;
	if ((*sl)->slb_version < min_ver)
	{
		DEBUG(("Slbopen: SLB is version %ld, requested was %ld",
			(*sl)->slb_version, min_ver));
		r = EBADARG;
		goto slb_error;
	}

	/* Shrink the TPA to the minimum, including stack for init */
	hitpa = (long)b + 256 + b->p_tlen + b->p_dlen + b->p_blen + SLB_INIT_STACK;
	if (hitpa < b->p_hitpa)
	{
		b->p_hitpa = hitpa;
		r = sys_m_shrink(0, b, b->p_hitpa - (long)b);
		if (r)
		{
			DEBUG(("Slbopen: Couldn't shrink basepage"));
			goto slb_error;
		}
	}
	else if (hitpa > b->p_hitpa)
	{
		DEBUG(("Slbopen: Warning: SLB uses minimum TPA - "
			"no additional user stack possible"));
	}

	/*
	 * we have found it; do not attempt to search further
	 * if it is just the shared library failing
	 */
	*continue_search = 0;

	/* Run the shared library, i.e. call its init() routine.
	 *
	 * exec_region() called by sys_pexec() recognizes the
	 * SLB binary and makes proper modifications of the
	 * b->p_tbase so that slb_init_and_exit() is called
	 * instead of the "real" text segment.
	 *
	 * The curproc->p_flag manipulation is necessary to
	 * tell the exec_region() that this call comes from
	 * the inside of the Slbopen(). This prevents the SLB
	 * to be executed by a program with ordinary Pexec().
	 */

	ptr.c = b->p_cmdlin;
	
	oldcmdlin = *ptr.l;

	get_curproc()->p_flag |= P_FLAG_SLO;
	r = sys_pexec(106, name, b, 0L);
	get_curproc()->p_flag &= ~P_FLAG_SLO;

	if (r < 0L)
	{
		DEBUG(("Slbopen: Pexec() returned: %ld", r));
		goto slb_error;
	}

	/* Wait for the init routine to finish */

	assert (get_curproc()->p_sigacts);

	oldsigint = SIGACTION(get_curproc(), SIGINT).sa_handler;
	oldsigquit = SIGACTION(get_curproc(), SIGQUIT).sa_handler;

	SIGACTION(get_curproc(), SIGINT).sa_handler =
	SIGACTION(get_curproc(), SIGQUIT).sa_handler = SIG_IGN;

	new_pid = (int) r;
	r = sys_pwaitpid (new_pid, 2, NULL);

	SIGACTION(get_curproc(), SIGINT).sa_handler = oldsigint;
	SIGACTION(get_curproc(), SIGQUIT).sa_handler = oldsigquit;

	if (r < 0)
	{
		sys_p_kill(new_pid, SIGKILL);
		/* Not `goto slb_error' because Pexec(106)
		 * releases the basepage and env
		 */
		if (--mr->links == 0)
			free_region(mr);
		return(EINTERNAL);
	}
	if ((r & 0x0000ff00L) != (SIGSTOP << 8))
	{
		DEBUG(("Slbopen: child died"));
		sys_p_kill(new_pid, SIGKILL);
		if (--mr->links == 0)
			free_region(mr);
		return(EXCPT);
	}

	/*
	 * To access the return value of slb_init(), which was stored in the
	 * basepage of the shared lib, curproc needs to have global access, as
	 * the shared library is already a process of its own, with its TPA
	 * most probably being private.
	 */
	prot_cookie = prot_temp((ulong)b, sizeof(BASEPAGE), -1);
	assert(prot_cookie < 0);
	r = *ptr.l;
	*ptr.l = oldcmdlin;
	if (prot_cookie != -1)
		prot_temp((ulong)b, sizeof(BASEPAGE), prot_cookie);
	if (r < 0L)
	{
		DEBUG(("Slbopen: slb_init() returned %ld", r));
		sys_p_kill(new_pid, SIGKILL);
		if (--mr->links == 0)
			free_region(mr);
		return(r);
	}

	/*
	 * Fill the shared library structure and insert it into the global list
	 */
	strcpy((*sl)->slb_name, name);
	(*sl)->slb_proc = pid2proc(new_pid);
	assert((*sl)->slb_proc);
	(*sl)->slb_proc->p_flag |= P_FLAG_SLB;	/* mark as SLB */
	(*sl)->slb_proc->ppid = 0;	/* have the system adopt it */
	(*sl)->slb_next = slb_list;
	slb_list = *sl;
	mark_proc_region(get_curproc()->p_mem, mr, PROT_PR, get_curproc()->pid);
	return(0);
}

/*
 * s_lbopen
 *
 * Implementation of Slbopen().
 *
 * BUGS: Must always be called from user mode.
 *
 * Input:
 * name: Filename of the shared library to open/load (without path)
 * path: Path to look for name; if NULL, the lib is searched through SLBPATH.
 * min_ver: Minimum version number of the library
 * sl: Pointer to store the library's descriptor in
 * fn: Pointer to store the pointer to the library's execution function in
 *
 * Returns:
 * <0: GEMDOS error code, opening failed
 * Otherwise: The version number of the opened SLB
 */
long _cdecl
sys_s_lbopen(char *name, char *path, long min_ver, SHARED_LIB **sl, SLB_EXEC *fn)
{
	SHARED_LIB *slb;
	struct user_things *ut;
	long r, *usp;
	ulong i;
	MEMREGION **mr;
	int continue_search = 1;

	/* First, ensure the call came from user mode */
	if (get_curproc()->ctxt[SYSCALL].sr & 0x2000)
	{
		DEBUG(("Slbopen: Called from supervisor mode"));
		raise(SIGSYS);
		return(EACCES);
	}

	/* No empty names allowed */
	if (!*name)
	{
		DEBUG(("Slbopen: Empty name"));
		return(EACCES);
	}

	/* Shared libraries may not open shared libraries ... */
	for (slb = slb_list; slb; slb = slb->slb_next)
	{
		if (slb->slb_proc == get_curproc())
		{
			DEBUG(("Slbopen: Called from an SLB"));
			return(EACCES);
		}
	}

	/* Check whether this library is already in memory */
	for (slb = slb_list; slb; slb = slb->slb_next)
	{
		if (!stricmp(slb->slb_name, name))
			break;
	}
	if (slb)
	{
		/* If yes, check the version number */
		if (slb->slb_version < min_ver)
		{
			DEBUG(("Slbopen: Already loaded library is version "
				"%ld, requested was %ld",
				slb->slb_version, min_ver));
			return(EBADARG);
		}
		/* Ensure curproc hasn't already opened this library */
		if (is_user(slb, get_curproc()->pid) && has_opened(slb, get_curproc()->pid))
		{
			DEBUG(("Slbopen: Library was already opened by this "
				"process"));
			return(EACCES);
		}
		assert(!has_opened(slb, get_curproc()->pid));
		if (!is_user(slb, get_curproc()->pid))
			*sl = slb;
	}
	else
	{
		/* Library is not available, try to load it */

		r = -1;
		if (path)
		{
			r = load_and_init_slb(name, path, min_ver, sl, &continue_search);
			/* FORCE("Slbopen: search userpath %s/%s: $%08lx", path, name, r); */
		}

		if (r < 0 && continue_search)
		{
			char *npath, *np;

			/* $SLBPATH contains a list of pathnames where the library
			 * should be searched for (MagiC-like), if the user hasn't
			 * supplied an explicit pathname.
			 *
			 * The variable must be in MiNT format, i.e.
			 *
			 * ./;u:/c/multitos;u:/c/multitos/slb etc.
			 *
			 * or
			 *
			 * ./;/c/multitos;/usr/lib/slb etc.
			 *
			 * MS-Backslash as a path separator will be accepted.
			 * Directory paths may be also separated with commas.
			 *
			 */
			path = _mint_getenv(get_curproc()->p_mem->base, "SLBPATH");

			if (!path)
				path = "./";

			npath = kmalloc(strlen(path) + 3);

			if (!npath)
			{
				DEBUG(("Slbopen: Couldn't kmalloc() path"));
				return ENOMEM;
			}

			do
			{
				/* Searching may take some time. If the system has to
				 * run smoothly, we must from time to time give the CPU
				 * up.
				 */
				if (!proc_clock)
					sys_s_yield();

				np = npath;

				if (path[0] == '/' || path[0] == '\\')
				{
					*np++ = 'u';
					*np++ = ':';
				}

				while (*path && *path != ';' && *path != ',')
					*np++ = *path++;

				if (*path)
					path++;		/* skip the separator */
				*np = 0;

				r = load_and_init_slb(name, npath, min_ver, sl, &continue_search);
				/* FORCE("Slbopen: search $SLBPATH %s/%s: $%08lx", npath, name, r); */
			} while (*path && r < 0 && continue_search);

			kfree(npath);

			if (r < 0 && continue_search)
			{
				r = load_and_init_slb(name, sysdir, min_ver, sl, &continue_search);
				/* FORCE("Slbopen: search $SYSDIR %s/%s: $%08lx", sysdir, name, r); */
			}
		}

		if (r < 0)
		{
			/* ALERT (MSG_slb_couldnt_open, name); */
			/* FORCE("Slbopen: not found %s: $%08lx", name, r); */
			return(r); /* DEBUG info already written out */
		}
		slb = *sl;
	}

	/* Allow curproc temporary global access to the library structure */
	mark_proc_region (get_curproc()->p_mem, slb->slb_region, PROT_G, get_curproc()->pid);

	/* Mark all memregions of the shared library as "global" for curproc */
	assert (slb->slb_proc->p_mem);
	mr = slb->slb_proc->p_mem->mem;
	if (mr)
	{
		for (i = 0; i < slb->slb_proc->p_mem->num_reg; i++, mr++)
		{
			if (*mr != 0L)
				mark_proc_region (get_curproc()->p_mem, *mr, PROT_G, get_curproc()->pid);
		}
	}

	/*
	 * If curproc is marked as user of this SLB, but as having opened it,
	 * this is the internal call of s_lbopen (from slb_open in
	 * user_things.S), which means the open() routine has just been
	 * successfully executed.
	 */
	if (is_user(slb, get_curproc()->pid))
	{
		DEBUG(("Slbopen: open() successful"));
		mark_opened(slb, get_curproc()->pid, 1);
		slb->slb_used++;
		mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);
		return(slb->slb_version);
	}

	/*
	 * Otherwise, mark curproc as user and change the context so that upon
	 * returning from this GEMDOS call, slb_open() in user_things.S gets
	 * called
	 */
	mark_users(slb, get_curproc()->pid, 1);

	ut = get_curproc()->p_mem->tp_ptr;
	ut->bp = get_curproc()->p_mem->base;

	*fn = (SLB_EXEC)ut->slb_exec_p;

	usp = (long *)get_curproc()->ctxt[SYSCALL].usp;
	*(--usp) = get_curproc()->ctxt[SYSCALL].pc;
	*(--usp) = (long)slb;
	*(--usp) = (long)get_curproc()->p_mem->base;
	*(--usp) = (long)slb->slb_head;
	get_curproc()->ctxt[SYSCALL].pc = ut->slb_open_p;
	get_curproc()->ctxt[SYSCALL].usp = (long)usp;
	mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);
	DEBUG(("Slbopen: Calling open()"));
	return(slb->slb_version);
}

/*
 * s_lbclose
 *
 * Implementation of Slbclose().
 *
 * BUGS: Must be called from user mode.
 *
 * Input:
 * sl: The descriptor of the library to close
 *
 * Returns:
 * 0: Library has been closed
 * Otherwise: GEMDOS error code
 */
long _cdecl
sys_s_lbclose(SHARED_LIB *sl)
{
	SHARED_LIB *slb;
	long *usp;
	ulong i;
	MEMREGION **mr;

	/* First, ensure the call came from user mode */
	if (get_curproc()->ctxt[SYSCALL].sr & 0x2000)
	{
		DEBUG(("Slbclose: Called from supervisor mode"));
		raise(SIGSYS);
		return(EACCES);
	}

	/* Now try to find the structure in the global list */
	for (slb = slb_list; slb; slb = slb->slb_next)
	{
		if (slb == sl)
			break;
	}
	if (slb == 0L)
	{
		DEBUG(("Slbclose: Library structure not found"));
		return(EBADF);
	}

	/* Check whether curproc is user of this SLB */
	if (!is_user(slb, get_curproc()->pid))
	{
		DEBUG(("Slbclose: Process is not user of this SLB"));
		return(EACCES);
	}

	/*
	 * If curproc has successfully called open(), unmark it and change the
	 * context so that upon returning from Slbclose(), slb_close() in
	 * user_things.S will be called
	 */
	mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_G, get_curproc()->pid);
	if (has_opened(slb, get_curproc()->pid))
	{
		struct user_things *ut = get_curproc()->p_mem->tp_ptr;

		slb->slb_used--;
		mark_opened(slb, get_curproc()->pid, 0);
		usp = (long *)get_curproc()->ctxt[SYSCALL].usp;
		*(--usp) = get_curproc()->ctxt[SYSCALL].pc;
		*(--usp) = (long)slb;
		*(--usp) = (long)get_curproc()->p_mem->base;
		get_curproc()->ctxt[SYSCALL].pc = ut->slb_close_p;
		get_curproc()->ctxt[SYSCALL].usp = (long)usp;
		mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);
		DEBUG(("Slbclose: Calling close()"));
		return(0);
	}

	/*
	 * If we get here, curproc has either never called open() successfully,
	 * or we are just returning from slb_close(). It's now time to remove curproc from the
	 * list of users, deny any further access to the library's memory, and
	 * - if no more users remain - remove the library from memory.
	 */
	mark_users(slb, get_curproc()->pid, 0);
	assert (slb->slb_proc->p_mem);
	mr = slb->slb_proc->p_mem->mem;
	if (mr)
	{
		for (i = 0; i < slb->slb_proc->p_mem->num_reg; i++, mr++)
		{
			if (*mr != 0L)
				mark_proc_region(get_curproc()->p_mem, *mr, PROT_I, get_curproc()->pid);
		}
	}
	if (slb->slb_used == 0)
	{
		PROC *slb_proc = slb->slb_proc;

		/*
		 * Clear the name of the SLB, to prevent further usage as there
		 * might occur another call to Slbopen() before the library is
		 * finally removed from memory
		 */
		slb->slb_name[0] = 0;
		slb_proc->p_flag &= ~P_FLAG_SLB;
		mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);
		post_sig(slb_proc, SIGCONT);
		slb_proc->sigpending &= ~STOPSIGS;
		slb_proc->p_flag |= P_FLAG_SLB;
	}
	else
		mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);

	return(0);
}

/*
 * slb_close_on_exit
 *
 * Helper function that gets called whenenver a process terminates. It then
 * closes any shared library the process might haven't closed.
 *
 * BUGS: Won't call the library's close() function when the process calls
 * Pterm() in supervisor mode or when it dies without using Pterm() (e.g. when
 * killed by a signal).
 *
 * Input:
 * terminate: Is this a call from Pterm() (0), or from terminate() (1)
 *
 * Returns:
 * 0: Process had no (more) shared libraries open
 * 1: Process had a shared library open, return from Pterm() to close() the
 *    library
 * -1: Process had a shared library open and has been removed from the list of
 *     users
 */
int
slb_close_on_exit (int term, int code)
{
	struct user_things *ut;
	SHARED_LIB *slb;
	MEMREGION **mr;
	ulong i;
	long *usp;

	/* Is curproc user of a shared library? */
	for (slb = slb_list; slb; slb = slb->slb_next)
	{
		if (is_user (slb, get_curproc()->pid))
			break;
	}
	if (slb == 0L)
		return 0;

	/*
	 * On termination (i.e. non-planned exit), just remove curproc from the
	 * list of users and exit the library, if necessary. Do the same if the
	 * call came from supervisor mode or curproc has already close()d the
	 * library (or never open()ed it).
	 */
	mark_proc_region (get_curproc()->p_mem, slb->slb_region, PROT_G, get_curproc()->pid);
	if (term || (get_curproc()->ctxt[SYSCALL].sr & 0x2000) ||
		!has_opened(slb, get_curproc()->pid))
	{
		mark_users(slb, get_curproc()->pid, 0);
		if (has_opened(slb, get_curproc()->pid))
			slb->slb_used--;
		mark_opened(slb, get_curproc()->pid, 0);
		assert (slb->slb_proc->p_mem);

		mr = slb->slb_proc->p_mem->mem;
		if (mr)
		{
			for (i = 0; i < slb->slb_proc->p_mem->num_reg; i++, mr++)
			{
				if (*mr != 0L)
					mark_proc_region(get_curproc()->p_mem, *mr, PROT_I, get_curproc()->pid);
			}
		}
		if (slb->slb_used == 0)
		{
			PROC *slb_proc = slb->slb_proc;

			slb->slb_name[0] = 0;
			mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);

			/* Unblock the signal deliveries */
			slb_proc->p_flag &= ~P_FLAG_SLB;

			post_sig(slb_proc, SIGCONT);
			slb_proc->sigpending &= ~STOPSIGS;
			slb_proc->p_flag |= P_FLAG_SLB;
		}
		else
			mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);
		return(-1);
	}

	/*
	 * Otherwise, change curproc's context to call slb_close_and_pterm() in
	 * user_things.S upon "returning" from Pterm().
	 */
	ut = get_curproc()->p_mem->tp_ptr;

	assert(has_opened(slb, get_curproc()->pid));
	slb->slb_used--;
	mark_opened(slb, get_curproc()->pid, 0);
	usp = (long *)get_curproc()->ctxt[SYSCALL].usp;
	*(--usp) = code;
	*(--usp) = (long)slb;
	get_curproc()->ctxt[SYSCALL].pc = ut->slb_close_and_pterm_p;
	get_curproc()->ctxt[SYSCALL].usp = (long)usp;
	mark_proc_region(get_curproc()->p_mem, slb->slb_region, PROT_PR, get_curproc()->pid);
	return(1);
}

/*
 * remove_slb
 *
 * Helper function for terminate(); checks whether curproc is a shared library
 * and removes the structure from the linked list, if yes.
 */
void
remove_slb(void)
{
	SHARED_LIB	*slb;
	SHARED_LIB	*last = NULL;
	MEMREGION	*mr;

	/* Is curproc a shared library? */
	for (slb = slb_list; slb; slb = slb->slb_next)
	{
		if (slb->slb_proc == get_curproc())
			break;
		last = slb;
	}
	if (slb == NULL)
		return;

	/*
	 * If yes, the structure has to be removed. Before doing so, we check
	 * the usage counter and yell if for some reason the library was still
	 * in use.
	 */
	if (slb->slb_used)
	{
		ALERT(MSG_slb_freeing_used, slb->slb_name);
	}
	if (last)
	{
		mark_proc_region(get_curproc()->p_mem, last->slb_region, PROT_G, get_curproc()->pid);
		last->slb_next = slb->slb_next;
		mark_proc_region(get_curproc()->p_mem, last->slb_region, PROT_PR, get_curproc()->pid);
	}
	else
		slb_list = slb->slb_next;
	mr = slb->slb_region;
	if (--mr->links == 0)
		free_region(mr);
}
