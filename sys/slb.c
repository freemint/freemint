/*
 * $Id$
 * 
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
# include "arch/slb_util.h"

# include "dos.h"
# include "dosmem.h"
# include "dossig.h"
# include "k_exec.h"
# include "k_exit.h"
# include "kmemory.h"
# include "memory.h"
# include "rendez.h"
# include "signal.h"
# include "util.h"

# include <osbind.h>


const char slbpath[]="SLBPATH=";	/* used by slb_util.spp */

/* The linked list of used SLBs */
SHARED_LIB *slb_list = NULL;

/*
 * slb_init_and_exit
 *
 * Helper function that is used as the text segment for shared libraries. It
 * calls the library's init() function and stores the return value in the
 * basepage (which in turn is read by s_lbopen()). After that, it goes to
 * sleep, and calls the library's exit() function when it is woken up again.
 *
 * BUGS: Can be killed by a regular Pkill(), so that the library dies while
 * still being in use.
 *
 * Input:
 * b: Pointer to the basepage
 */
static void _cdecl
slb_init_and_exit (BASEPAGE *b)
{
	long *exec_longs;
	volatile SLB_HEAD *head;
	
	/* Act like a daemon */
	P_domain (1);
	Fclose (0);
	Fclose (1);
	Fclose (2);
	Fclose (3);
	Fclose (4);
	P_sigsetmask (-1L);
	P_setpgrp (0, 0);
	
	/*
	 * Note: Unlike MagiC, this implementation calls a library's init() and
	 * exit() function in user mode, as anything else would be a security
	 * risk. Maybe this should depend on secure_mode?
	 */
	
	/* Test for the new programm-format */
	exec_longs = (long *) (b->p_lowtpa + 256L);
	if (exec_longs[0] == 0x283a001aL && exec_longs[1] == 0x4efb48faL) 
	{
		head = (SLB_HEAD *)(b->p_lowtpa + 256L + 228L);
	}
	else
	{	
		head = (SLB_HEAD *)(b->p_lowtpa + 256L);
	}
	
	if (head->slh_magic != 0x70004afcL)
		*(long *) b->p_cmdlin = -1L;
	else
		*(long *) b->p_cmdlin = head->slh_slb_init ();
	
	P_kill (0, SIGSTOP);
	head->slh_slb_exit ();
	Pterm0 ();
}

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
	if (sl->slb_users [pid/8] & (1 << (pid % 8)))
		return 1;
	else
		return 0;
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
	if (sl->slb_opened [pid/8] & (1 << (pid % 8)))
		return 1;
	else
		return 0;
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
load_and_init_slb(char *name, char *path, long min_ver, SHARED_LIB **sl)
{
	int		newpid,
			prot_cookie;
	long		r,
			hitpa,
			oldsigint,
			oldsigquit,
			oldcmdlin;
	char		*fullpath;
	BASEPAGE	*b;
	MEMREGION	*mr;
	long		*exec_longs;

	/* Construct the full path name of the SLB */

# if 1
	/* Treat SLBPATH value as a single pathname pointing to the default
	 * folder supposed to contain libraries (draco)
	 */

	if (!path)
		path = getslbpath(curproc->base);
# endif		
	if (!path)
		path = "./";

	fullpath = kmalloc(strlen(path) + strlen(name) + 2);
	if (!fullpath)
	{
		DEBUG(("Slbopen: Couldn't kmalloc() full pathname"));
		return(ENOMEM);
	}
	strcpy(fullpath, path);
	strcat(fullpath, "/");
	strcat(fullpath, name);
	
	/* Create the new shared library structure */
	mr = get_region(alt, sizeof(SHARED_LIB) + strlen(name), PROT_PR);
	if (mr == 0L)
	{
		mr = get_region(core, sizeof(SHARED_LIB) + strlen(name),
			PROT_PR);
	}
	if (mr == 0L)
	{
		DEBUG(("Slbopen: Couldn't get region for new SHARED_LIB structure"));
		kfree(fullpath);
		return(ENOMEM);
	}
	mark_proc_region(curproc, mr, PROT_G);
	*sl = (SHARED_LIB *)mr->loc;
	bzero(*sl, sizeof(SHARED_LIB));
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
		m_free((virtaddr)b->p_env);
		m_free((virtaddr)b);
		if (--mr->links == 0)
			free_region(mr);
		return(r);
	}

	/* Test for the new program format */
	exec_longs = (long *) b->p_tbase;
	if (exec_longs[0] == 0x283a001a && exec_longs[1] == 0x4efb48fa)
	{
		(*sl)->slb_head = (SLB_HEAD *)(b->p_tbase + 228);
	}
	else
	{
		(*sl)->slb_head = (SLB_HEAD *)b->p_tbase;
	}
	
	/* Check the magic value */
	if ((*sl)->slb_head->slh_magic != 0x70004afcL)
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
	hitpa = (long)b + 256 + b->p_tlen + b->p_dlen + b->p_blen +
		SLB_INIT_STACK;
	if (hitpa < b->p_hitpa)
	{
		b->p_hitpa = hitpa;
		r = m_shrink(0, (virtaddr)b, b->p_hitpa - (long)b);
		if (r)
		{
			DEBUG(("Slbopen: Couldn't shrink basepage"));
			/* XXX: why not `goto slb_error'? */
			if (--mr->links == 0)
				free_region(mr);
			return(r);
		}
	}
	else if (hitpa > b->p_hitpa)
	{
		DEBUG(("Slbopen: Warning: SLB uses minimum TPA - "
			"no additional user stack possible"));
	}

	/* Set the start of the fake code to call init as p_tbase */
	b->p_tbase = (long)slb_init_and_exit;

	/* Run the shared library, i.e. call its init() routine */
	oldcmdlin = *(long *)b->p_cmdlin;
	r = sys_pexec(106, name, b, 0L);
	if (r < 0L)
	{
		DEBUG(("Slbopen: Pexec() returned: %ld", r));
		if (--mr->links == 0)
			free_region(mr);
		return(r);
	}

	/* Wait for the init routine to finish */
	
	assert (curproc->p_sigacts);
	
	oldsigint = SIGACTION(curproc, SIGINT).sa_handler;
	oldsigquit = SIGACTION(curproc, SIGQUIT).sa_handler;
	
	SIGACTION(curproc, SIGINT).sa_handler =
	SIGACTION(curproc, SIGQUIT).sa_handler = SIG_IGN;
	
	newpid = (int) r;
	r = sys_pwaitpid (newpid, 2, NULL);
	
	SIGACTION(curproc, SIGINT).sa_handler = oldsigint;
	SIGACTION(curproc, SIGQUIT).sa_handler = oldsigquit;
	
	if (r < 0)
	{
		p_kill(newpid, SIGKILL);
		if (--mr->links == 0)
			free_region(mr);
		return(EINTERNAL);
	}
	if ((r & 0x0000ff00L) != (SIGSTOP << 8))
	{
		DEBUG(("Slbopen: child died"));
		p_kill(newpid, SIGKILL);
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
	r = *(long *)b->p_cmdlin;
	*(long *)b->p_cmdlin = oldcmdlin;
	if (prot_cookie != -1)
		prot_temp((ulong)b, sizeof(BASEPAGE), prot_cookie);
	if (r < 0L)
	{
		DEBUG(("Slbopen: slb_init() returned %ld", r));
		p_kill(newpid, SIGKILL);
		if (--mr->links == 0)
			free_region(mr);
		return(r);
	}

	/*
	 * Fill the shared library structure and insert it into the global list
	 */
	strcpy((*sl)->slb_name, name);
	(*sl)->slb_proc = pid2proc(newpid);
	assert((*sl)->slb_proc);
	(*sl)->slb_next = slb_list;
	slb_list = *sl;
	mark_proc_region(curproc, mr, PROT_PR);
	return(0);
}

/*
 * s_lbopen
 *
 * Implementation of Slbopen().
 *
 * BUGS: Must always be called from user mode. Using a NULL-pointer for the
 * path does not cause the library to be searched in all paths given in the
 * environment variable SLBPATH (because IMO, evaluating an environment
 * variable /inside the kernel/ is not a good idea).
 *
 * Input:
 * name: Filename of the shared library to open/load (without path)
 * path: Path to look for name; if NULL, the current directory is assumed
 * min_ver: Minimum version number of the library
 * sl: Pointer to store the library's descriptor in
 * fn: Pointer to store the pointer to the library's execution function in
 *
 * Returns:
 * <0: GEMDOS error code, opening failed
 * Otherwise: The version number of the opened SLB
 */
long _cdecl
s_lbopen(char *name, char *path, long min_ver, SHARED_LIB **sl, SLB_EXEC *fn)
{
	SHARED_LIB	*slb;
	long		r,
			*usp;
	ulong		i;
	MEMREGION	**mr;

	/* First, ensure the call came from user mode */
	if (curproc->ctxt[SYSCALL].sr & 0x2000)
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
		if (slb->slb_proc == curproc)
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
		if (is_user(slb, curproc->pid) && has_opened(slb, curproc->pid))
		{
			DEBUG(("Slbopen: Library was already opened by this "
				"process"));
			return(EACCES);
		}
		assert(!has_opened(slb, curproc->pid));
		if (!is_user(slb, curproc->pid))
			*sl = slb;
	}
	else
	{
		/* Library is not available, try to load it */
		r = load_and_init_slb(name, path, min_ver, sl);
		if (r < 0L)
		{
			ALERT ("Could not open shared library %s", name);
			return(r); /* DEBUG info already written out */
		}
		slb = *sl;
	}

	/* Allow curproc temporary global access to the library structure */
	mark_proc_region (curproc, slb->slb_region, PROT_G);

	/* Mark all memregions of the shared library as "global" for curproc */
	assert (slb->slb_proc->p_mem);
	mr = slb->slb_proc->p_mem->mem;
	if (mr)
	{
		for (i = 0; i < slb->slb_proc->p_mem->num_reg; i++, mr++)
		{
			if (*mr != 0L)
				mark_proc_region (curproc, *mr, PROT_G);
		}
	}

	/*
	 * If curproc is marked as user of this SLB, but as having opened it,
	 * this is the internal call of s_lbopen (from slb_open in
	 * slb_util.spp), which means the open() routine has just been
	 * successfully executed.
	 */
	if (is_user(slb, curproc->pid))
	{
		DEBUG(("Slbopen: open() successful"));
		mark_opened(slb, curproc->pid, 1);
		slb->slb_used++;
		mark_proc_region(curproc, slb->slb_region, PROT_PR);
		return(slb->slb_version);
	}

	/*
	 * Otherwise, mark curproc as user and change the context so that upon
	 * returning from this GEMDOS call, slb_open() in slb_util.spp gets
	 * called
	 */
	mark_users(slb, curproc->pid, 1);
# if 0
	if (slb->slb_head->slh_flags & 1L)
		*fn = slb_fast;
	else
# endif
		*fn = slb_exec;
	
	usp = (long *)curproc->ctxt[SYSCALL].usp;
	*(--usp) = curproc->ctxt[SYSCALL].pc;
	*(--usp) = (long)slb;
	*(--usp) = (long)curproc->base;
	*(--usp) = (long)slb->slb_head;
	curproc->ctxt[SYSCALL].pc = (long)slb_open;
	curproc->ctxt[SYSCALL].usp = (long)usp;
	mark_proc_region(curproc, slb->slb_region, PROT_PR);
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
s_lbclose(SHARED_LIB *sl)
{
	SHARED_LIB	*slb;
	long		*usp;
	ulong		i;
	MEMREGION	**mr;

	/* First, ensure the call came from user mode */
	if (curproc->ctxt[SYSCALL].sr & 0x2000)
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
	if (!is_user(slb, curproc->pid))
	{
		DEBUG(("Slbclose: Process is not user of this SLB"));
		return(EACCES);
	}

	/*
	 * If curproc has successfully called open(), unmark it and change the
	 * context so that upon returning from Slbclose(), slb_close() in
	 * slb_util.spp will be called
	 */
	mark_proc_region(curproc, slb->slb_region, PROT_G);
	if (has_opened(slb, curproc->pid))
	{
		slb->slb_used--;
		mark_opened(slb, curproc->pid, 0);
		usp = (long *)curproc->ctxt[SYSCALL].usp;
		*(--usp) = curproc->ctxt[SYSCALL].pc;
		*(--usp) = (long)slb;
		*(--usp) = (long)curproc->base;
		curproc->ctxt[SYSCALL].pc = (long)slb_close;
		curproc->ctxt[SYSCALL].usp = (long)usp;
		mark_proc_region(curproc, slb->slb_region, PROT_PR);
		DEBUG(("Slbclose: Calling close()"));
		return(0);
	}

	/*
	 * If we get here, curproc has either never called open() successfully,
	 * or just called close(). It's now time to remove curproc from the
	 * list of users, deny any further access to the library's memory, and
	 * - if no more users remain - remove the library from memory.
	 */
	mark_users(slb, curproc->pid, 0);
	assert (slb->slb_proc->p_mem);
	mr = slb->slb_proc->p_mem->mem;
	if (mr)
	{
		for (i = 0; i < slb->slb_proc->p_mem->num_reg; i++, mr++)
		{
			if (*mr != 0L)
				mark_proc_region(curproc, *mr, PROT_I);
		}
	}
	if (slb->slb_used == 0)
	{
		short	pid = slb->slb_proc->pid;

		/*
		 * Clear the name of the SLB, to prevent further usage as there
		 * might occur another call to Slbopen() before the library is
		 * finally removed from memory
		 */
		slb->slb_name[0] = 0;
		mark_proc_region(curproc, slb->slb_region, PROT_PR);
		p_kill(pid, SIGCONT);
	}
	else
		mark_proc_region(curproc, slb->slb_region, PROT_PR);

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
slb_close_on_exit (int terminate)
{
	SHARED_LIB	*slb;
	MEMREGION	**mr;
	ulong		i;
	long		*usp;
	
	/* Is curproc user of a shared library? */
	for (slb = slb_list; slb; slb = slb->slb_next)
	{
		if (is_user (slb, curproc->pid))
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
	mark_proc_region (curproc, slb->slb_region, PROT_G);
	if (terminate || (curproc->ctxt[SYSCALL].sr & 0x2000) ||
		!has_opened(slb, curproc->pid))
	{
		mark_users(slb, curproc->pid, 0);
		if (has_opened(slb, curproc->pid))
			slb->slb_used--;
		mark_opened(slb, curproc->pid, 0);
		assert (slb->slb_proc->p_mem);
		mr = slb->slb_proc->p_mem->mem;
		if (mr)
		{
			for (i = 0; i < slb->slb_proc->p_mem->num_reg; i++, mr++)
			{
				if (*mr != 0L)
					mark_proc_region(curproc, *mr, PROT_I);
			}
		}
		if (slb->slb_used == 0)
		{
			short	pid = slb->slb_proc->pid;

			slb->slb_name[0] = 0;
			mark_proc_region(curproc, slb->slb_region, PROT_PR);
			p_kill(pid, SIGCONT);
		}
		else
			mark_proc_region(curproc, slb->slb_region, PROT_PR);
		return(-1);
	}

	/*
	 * Otherwise, change curproc's context to call slb_close_and_pterm() in
	 * slb_util.spp upon "returning" from Pterm().
	 */
	assert(has_opened(slb, curproc->pid));
	slb->slb_used--;
	mark_opened(slb, curproc->pid, 0);
	usp = (long *)curproc->ctxt[SYSCALL].usp;
	*(--usp) = curproc->ctxt[SYSCALL].pc;
	*(--usp) = (long)slb;
	*(--usp) = (long)curproc->base;
	curproc->ctxt[SYSCALL].pc = (long)slb_close_and_pterm;
	curproc->ctxt[SYSCALL].usp = (long)usp;
	mark_proc_region(curproc, slb->slb_region, PROT_PR);
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
		if (slb->slb_proc == curproc)
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
		ALERT("Freeing shared library %s, which is still in use!",
			slb->slb_name);
	}
	if (last)
	{
		mark_proc_region(curproc, last->slb_region, PROT_G);
		last->slb_next = slb->slb_next;
		mark_proc_region(curproc, last->slb_region, PROT_PR);
	}
	else
		slb_list = slb->slb_next;
	mr = slb->slb_region;
	if (--mr->links == 0)
		free_region(mr);
}
