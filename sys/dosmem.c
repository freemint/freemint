/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 * 
 * 
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 */

/*
 * GEMDOS emulation routines:
 * 
 * these are for the GEMDOS system calls concerning
 * allocating/freeing memory
 */

# include "dosmem.h"

# include "libkern/libkern.h"

# include "mint/proc.h"
# include "arch/mprot.h"

# include "k_prot.h"
# include "memory.h"
# include "util.h" /* pid2proc */


/* new call for TT TOS, for the user to inform DOS of alternate memory
 * FIXME: we really shouldn't trust the user so completely
 * FIXME: doesn't work if memory protection is on
 */
long _cdecl
m_addalt (long start, long size)
{
	struct proc *p = curproc;
	
	assert (p->p_cred && p->p_cred->ucr);
	
	if (!suser (p->p_cred->ucr))
		return EPERM;
	
	if (!no_mem_prot)
		/* pretend to succeed */
		return E_OK;
	
	if (!add_region (alt, start, size, M_ALT))
		return EINTERNAL;
	
	return E_OK;
}

/*
 * internal routine for doing Malloc on a particular memory map
 */
static long
_do_malloc (MMAP map, long size, int mode)
{
	virtaddr v;
	MEMREGION *m;
	long maxsize, mleft;

	if (size == -1L)
	{
		maxsize = max_rsize(map, 0L);
		if (curproc->maxmem)
		{
			mleft = curproc->maxmem - memused(curproc);
			if (maxsize > mleft)
				maxsize = mleft;
			if (maxsize < 0)
				maxsize = 0;
		}
		
		/* make sure to round down */
		maxsize &= ~MASKBITS;
		maxsize -= (MASKBITS + 1);
		if (maxsize < 0)
			maxsize = 0;
		
		return maxsize;
	}

	/* special case: Malloc(0) should always return NULL */
	if (size == 0) return 0;

	/* memory limit? */
	if (curproc->maxmem)
	{
		if (size > curproc->maxmem - memused(curproc))
		{
			DEBUG(("malloc: memory request would exceed limit"));
			return 0;
		}
	}

	m = get_region(map, size, mode);
	if (!m)
	{
		return 0;
	}
	
	v = attach_region(curproc, m);
	if (!v)
	{
		m->links = 0;
		free_region(m);
		return 0;
	}
	
	/* NOTE: get_region returns a region with link count 1;
	 * since attach_region increments the link count, we have
	 * to remember to decrement the count to correct for this.
	 */
	m->links--;
	
	if ((mode & F_KEEP))
	{
		/* request for permanent memory */
		m->mflags |= M_KEEP;
	}
	
	/* some programs crash when newly allocated memory isn't all zero,
	 * like TOS 1.04 GEM when loading xcontrol.acc ...
	 */
	if ((curproc->memflags & F_ALLOCZERO) || (mode & F_ALLOCZERO))
		bzero ((void *) m->loc, m->len);
	
	return (long) v;
}

long _cdecl
m_xalloc (long size, int mode)
{
	long r = 0;
	int protmode;
# ifdef DEBUG_INFO
	int origmode = mode;
# endif

	TRACE(("Mxalloc(%ld,%x)",size,mode));

	/* AKP: Hack here: if the calling process' PC is in ROM, then this
	 * is a Malloc call made by VDI's v_opnvwk routine.  So we change
	 * mode to include "super accessible."  This is temporary, until
	 * VDI catches up with multitasking TOS.
	 */
	if (((mode & F_PROTMODE) == 0)
		&& (curproc->ctxt[SYSCALL].pc > 0x00e00000L)
		&& (curproc->ctxt[SYSCALL].pc < 0x00efffffL))
	{
# ifndef MULTITOS
		if (gem_start && curproc->ctxt[SYSCALL].pc >= gem_start)
		{
			mode |= F_PROT_G + 0x10;
			TRACE (("m_xalloc: AES special (call from ROM)"));
		}
		else
# endif
		{
			mode |= (F_PROT_S + 0x10) | F_KEEP;
			TRACE(("m_xalloc: VDI special (call from ROM)"));
		}
	}
	
	/* If the mode argument comes in a zero, then set it to the default
	 * value from prgflags.  Otherwise subtract one from it to bring it
	 * into line with the actual argument to alloc_region.
	 */
	protmode = (mode & F_PROTMODE) >> F_PROTSHIFT;

	if (protmode == 0)
	{
		protmode = (curproc->memflags & F_PROTMODE) >> F_PROTSHIFT;
	}
	else
		--protmode;

	if (protmode > PROT_MAX_MODE)
	{
		DEBUG(("Mxalloc: invalid protection mode changed to private"));
		protmode = PROT_P;
	}

# if 0
	/* I'm very suspicious of the 0x08 flag; I can't see how it could
	 * work as the comment below seems to indicate -- ERS
	 */

	/* if the mode argument has the 0x08 bit set then you're trying
	 * to change the protection mode of a block you already own.
	 * "size" is really its base address. (new as of 2/6/1992)
	 */
	if (mode & 0x08) change_prot_status(curproc,size,protmode);
# endif

	/*
	 * Copy the F_KEEP attribute into protmode.  We didn't do that
	 * before now because change_prot_status don't want to see no
	 * steenking nofree attributes.
	 */

	protmode |= (mode & F_KEEP);

	/* mask off all but the ST/alternative RAM bits before further use */
	mode &= 3;
	
	if (size == -1)
	{
		switch (mode)
		{
			/* modes 2 and 3 are the same for for size -1 */
			case 3:
			case 2:
			{
				long r1;
				
				r = _do_malloc (core, -1L, PROT_P);
				r1 = _do_malloc (alt, -1L, PROT_P);
				if (r1 > r) r = r1;
				break;
			}
			case 1:
			{
				r = _do_malloc (alt, -1L, PROT_P);
				break;
			}
			case 0:
			{
				r = _do_malloc (core, -1L, PROT_P);
				break;
			}
		}
	}
	else
	{
		switch (mode)
		{
			case 3:
			{
				r = _do_malloc (alt, size, protmode);
				if (r == 0) r = _do_malloc (core, size, protmode);
				break;
			}
			case 2:
			{
				r = _do_malloc (core, size, protmode);
				if (r == 0) r = _do_malloc (alt, size, protmode);
				break;
			}
			case 1:
			{
				r = _do_malloc (alt, size, protmode);
				break;
			}
			case 0:
			{
				r = _do_malloc (core, size, protmode);
				break;
			}
		}
	}	

	if (r == 0)
	{
		DEBUG(("m_xalloc(%lx,%x) returns NULL",size,origmode));
	}
	else
	{
		TRACE(("m_xalloc(%lx,%x) returns %lx",size,origmode,r));
	}
	
	return r;
}

long _cdecl
m_alloc (long size)
{
	long r;
	
	TRACE(("Malloc(%lx)", size));
	if (curproc->memflags & F_ALTALLOC)
		r = m_xalloc(size, 3);
	else
		r = m_xalloc(size, 0);
	
	TRACE(("Malloc: returning %lx", r));
	return r;
}

long _cdecl
m_free (virtaddr block)
{
	struct memspace *mem;
	long r;
	int i;
	
	TRACE(("Mfree(%lx)", block));
	
	if (!block)
	{
		DEBUG(("Mfree: null pointer"));
		return EFAULT;
	}
	
	r = detach_region_by_addr (curproc, block);
	if (r == 0)
		return r;
	
	mem = rootproc->p_mem;
	
	/* hmmm... if we didn't find the region, perhaps it's a global
	 * one (with the M_KEEP flag set) belonging to a process that
	 * terminated
	 * 
	 * XXX I don't understand that
	 *     in my eyes the process can't access this region as this region
	 *     isn't mapped to the process address space (page_table)
	 */
	for (i = mem->num_reg - 1; i >= 0; i--)
	{
		if (mem->addr[i] == block)
		{
			MEMREGION *m = mem->mem[i];
			
			assert (m != NULL);
			assert (m->loc == (long) block);
			
			if (!(m->mflags & M_KEEP))
				continue;
			
			TRACE(("Freeing M_KEPT memory"));
			
			mem->mem[i] = 0;
			mem->addr[i] = 0;
			
			m->links--;
			if (m->links == 0)
				free_region(m);
			
			return E_OK;
		}
	}

	DEBUG(("Mfree: bad address %lx", block));
	return EFAULT;
}

long _cdecl
m_shrink (int dummy, virtaddr block, long size)
{
	struct proc *p = curproc;
	struct memspace *mem = p->p_mem;
	int i;
	
	
	TRACE(("Mshrink: %lx to %ld", block, size));
	
	if (!block)
	{
		DEBUG(("Mshrink: null pointer"));
		goto error;
	}
	
	if (!mem || !mem->mem)
		goto error;
	
	for (i = 0; i < mem->num_reg; i++)
	{
		if (mem->addr[i] == block)
		{
			MEMREGION *m = mem->mem[i];
			
			assert (m != NULL);
			assert (m->loc == (long) block);
			
			return shrink_region (m, size);
		}
	}
	
	DEBUG(("Mshrink: bad address (%lx)", block));
error:
	return EFAULT;
}

long _cdecl
sys_m_validate (int pid, void *addr, long size, long *flags)
{
	struct proc *p = NULL;
	MEMREGION *m;
	
	TRACE (("Mvalidate(%i, %lx, %li, %lx)", pid, addr, size, flags));
	
	if (pid == 0)
		p = curproc;
	else if (pid > 0)
		p = pid2proc (pid);
	
	if (!p)
	{
		DEBUG (("Mvalidate: no such process (pid %i)", pid));
		return ESRCH;
	}
	
	if (p != curproc && !suser (curproc->p_cred->ucr) && !(curproc->memflags & F_OS_SPECIAL))
	{
		DEBUG (("Mvalidate: permission denied"));
		return EPERM;
	}
	
	m = proc_addr2region (p, (long) addr);
	if (m && ((long) addr + size) <= (m->loc + m->len))
	{
		if (flags)
		{
			long mflags;
			
			mflags = get_prot_mode (m);
			
			*flags = (mflags << F_PROTSHIFT) | (m->mflags & M_MAP);
		}
		
		return 0;
	}
	
	DEBUG (("Mvalidate: invalid vector"));
	return EINVAL;
}

long _cdecl
sys_m_access (void *addr, long size, int mode)
{
	struct proc *p = curproc;
	MEMREGION *m;
	
	TRACE (("Maccess(%i, %lx, %li)", mode, addr, size));
	
	if ((mode < 0) || (mode > 1))
	{
		DEBUG (("Maccess: invalid argument"));
		return EINVAL;
	}
	
	/* search in the process memory regions */
	m = proc_addr2region (p, (long) addr);
	if (m && ((long) addr + size) <= (m->loc + m->len))
		/* always accessible */
		return 0;
	
	/* search in all memory reagions */
	m = addr2region ((long) addr);
	if (m && ((long) addr + size) <= (m->loc + m->len))
	{
		long mflags;
		
		mflags = get_prot_mode (m);
		
		/* want read only access -> true for global and read-only */
		if ((mode == 1) && ((mflags == PROT_G) || (mflags == PROT_PR)))
			return 0;
		
		/* want read/write access -> true for global */
		if (mflags == PROT_G)
			return 0;
	}
	
	DEBUG (("Maccess: invalid vector"));
	return EINVAL;
}
