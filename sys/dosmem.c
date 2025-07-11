/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 *
 *
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 *
 */

/**
 * GEMDOS emulation routines:
 *
 * these are for the GEMDOS system calls concerning
 * allocating/freeing memory
 */

# include "dosmem.h"

# include "libkern/libkern.h"

# include "mint/proc.h"
# include "arch/mprot.h"
# include "arch/detect.h"

# include "cookie.h"	/* get_cookie */
# include "k_prot.h"
# include "keyboard.h"	/* emutos */
# include "memory.h"
# include "util.h"		/* pid2proc */

# include "proc.h"

/* new call for TT TOS, for the user to inform DOS of alternate memory
 * FIXME: doesn't work if memory protection is on
 */
long _cdecl
sys_m_addalt (long start, long size)
{
	struct proc *p = get_curproc();
	long x;

	assert (p->p_cred && p->p_cred->ucr);

	if (!suser (p->p_cred->ucr))
		return EPERM;

# ifdef WITH_MMU_SUPPORT
	if (!no_mem_prot)
		/* pretend to succeed */
		return E_OK;
# endif

	for (x = 0; x < size; x += 0x2000)
	{
		if (test_long_rd(start + x) == 0)
			return EFAULT;
	}

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
	long v;
	MEMREGION *m;
	long maxsize, mleft;

	if (size == -1L)
	{
		maxsize = max_rsize(map, 0L);
		if (get_curproc()->maxmem)
		{
			mleft = get_curproc()->maxmem - memused(get_curproc());
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
	if (get_curproc()->maxmem)
	{
		if (size > get_curproc()->maxmem - memused(get_curproc()))
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

	v = attach_region(get_curproc(), m);
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
	if ((get_curproc()->p_mem->memflags & F_ALLOCZERO) || (mode & F_ALLOCZERO))
		mint_bzero ((void *) m->loc, m->len);

	return (long) v;
}

/*
F_XALLOCMODE:0-2	Treatment of the TT-RAM	0 =	Allocate ST-RAM only
F_ALTONLY:     1 =	Allocate Alternative-RAM only
F_STPREF:      2 =	Allocate either, ST-RAM preferred
F_ALTPREF:     3 =	Allocate either, Alternative-RAM preferred

3	Reserved
F_PROTMODE: 4-7	Protection mode	0 =	Default (from your PRGFLAGS)
F_PROT_P: 1 =	Private
F_PROT_G: 2 =	Global
F_PROT_S: 3 =	Supervisor-mode-only access
F_PROT_PR:4 =	World-readable access


 Other values are undefined and reserved.
F_KEEP: 14	No-Free modus

 When set, this bit means "if the owner of this block terminates, don't
free this block. Instead, let MiNT inherit it, so it'll never be freed."
This is a special mode meant for the OS only, and may not remain
available to user processes.
*/

long _cdecl
sys_m_xalloc (long size, short mode)
{
	long r = 0;
	int protmode;
# ifdef DEBUG_INFO
	int origmode = mode;
# endif

	TRACE(("Mxalloc(%ld,%x)",size,mode));

#ifndef M68000
	/* AKP: Hack here: if the calling process' PC is in ROM, then this
	 * is a Malloc call made by VDI's v_opnvwk routine.  So we change
	 * mode to include "super accessible."  This is temporary, until
	 * VDI catches up with multitasking TOS.
	 *
	 * Unfortunately, the check for PC doesn't work in all cases anymore; it has
	 * an effect on Milan/CTPCI VDI which are still bugged but if v_opnvwk() is
	 * called from an application, ctxt contains its PC, not ROM's so further
	 * checking is necessary.
	 */
	if ((mode & F_PROTMODE) == 0
		&& !emutos
		&& ((get_curproc()->ctxt[SYSCALL].pc > 0x00e00000L
			 && get_curproc()->ctxt[SYSCALL].pc < 0x00efffffL)
			|| (get_curproc()->in_vdi
				&& get_cookie(NULL, COOKIE_NVDI, NULL) == EERROR
				&& get_cookie(NULL, COOKIE_fVDI, NULL) == EERROR
				&& get_cookie(NULL, COOKIE_NOVA, NULL) == EERROR)))
	{
		mode |= (F_PROT_PR) | F_KEEP;
		DEBUG(("m_xalloc: VDI special (call from TOS 3/4)"));
	}
#endif

	/* If the mode argument comes in a zero, then set it to the default
	 * value from prgflags.  Otherwise subtract one from it to bring it
	 * into line with the actual argument to alloc_region.
	 */
	protmode = (mode & F_PROTMODE) >> F_PROTSHIFT;

	if (protmode == 0)
	{
		protmode = (get_curproc()->p_mem->memflags & F_PROTMODE) >> F_PROTSHIFT;
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
	if (mode & 0x08) change_prot_status(get_curproc(),size,protmode);
# endif

	/*
	 * Copy the F_KEEP attribute into protmode.  We didn't do that
	 * before now because change_prot_status don't want to see no
	 * steenking nofree attributes.
	 */

	protmode |= (mode & F_KEEP);

	/* mask off all but the ST/alternative RAM bits before further use */
	mode &= F_XALLOCMODE;

	if (size == -1)
	{
		switch (mode)
		{
			/* modes 2 and 3 are the same for for size -1 */
			case F_ALTPREF:
			case F_STPREF:
			{
				long r1;

				r = _do_malloc (core, -1L, PROT_P);
				r1 = _do_malloc (alt, -1L, PROT_P);
				if (r1 > r) r = r1;
				break;
			}
			case F_ALTONLY:
			{
				r = _do_malloc (alt, -1L, PROT_P);
				break;
			}
			case F_STONLY:
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
			case F_ALTPREF:
			{
				r = _do_malloc (alt, size, protmode);
				if (r == 0) r = _do_malloc (core, size, protmode);
				break;
			}
			case F_STPREF:
			{
				r = _do_malloc (core, size, protmode);
				if (r == 0) r = _do_malloc (alt, size, protmode);
				break;
			}
			case F_ALTONLY:
			{
				r = _do_malloc (alt, size, protmode);
				break;
			}
			case F_STONLY:
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
sys_m_alloc (long size)
{
	long r;

	TRACE(("Malloc(%lx)", size));
	if (get_curproc()->p_mem->memflags & F_ALTALLOC)
		r = sys_m_xalloc(size, F_ALTPREF);
	else
		r = sys_m_xalloc(size, F_STONLY);

	TRACE(("Malloc: returning %lx", r));
	return r;
}

long _cdecl
sys_m_free (void *_block)
{
	long block = (long)_block;
	struct proc *p = get_curproc();
	struct memspace *mem = p->p_mem;
	int i;

	TRACE(("Mfree(%lx)", block));

	if (!block)
	{
		DEBUG(("Mfree: null pointer from pid %i", p->pid));
		return EFAULT;
	}

	if (mem && mem->mem)
	{
		long r;

		/* Releasing own basepage cannot be legal */
		if (block == (long)mem->base)
		{
			DEBUG(("Mfree: cannot free bp!"));
			return EINVAL;
		}

		r = detach_region_by_addr(p, block);
		if (r == 0)
			return r;
	}
	else
		DEBUG(("Mfree: no memory context for pid %i", p->pid));

	mem = rootproc->p_mem;

	/* hmmm... if we didn't find the region, perhaps it's a global
	 * one (with the M_KEEP flag set) belonging to a process that
	 * terminated
	 *
	 * XXX I don't understand that
	 *     in my eyes the process can't access this region as this region
	 *     isn't mapped to the process address space (page_table)
	 *
	 *     Right, this code seems to allow anyone to release memory
	 *     reserved for TSR programs, which had called Ptermres().
	 *     Provided the caller knows the address. Am I wrong?
	 *
	 */
	for (i = mem->num_reg - 1; i >= 0; i--)
	{
		if (mem->addr[i] == block)
		{
			MEMREGION *m = mem->mem[i];

			assert(m != NULL);
			assert(m->loc == (long) block);

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
sys_m_shrink (short dummy, void *_block, long size)
{
	unsigned long block = (unsigned long)_block;
	struct proc *p = get_curproc();
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

	/* The basepage cannot be shrunk to size smaller than 256 bytes.
	 * BTW., shrinking to 0 is equal to releasing, see shrink_region();
	 * and releasing the basepage is illegal, see m_free() above.
	 */
	/* XXX perhaps this shouldn't be '256L', but rather 'PAGESIZE'?
	 */
	if (block == (unsigned long)mem->base && size < 256L)
		size = 256L;

	for (i = 0; i < mem->num_reg; i++)
	{
		if (mem->addr[i] == block)
		{
			long r;
			MEMREGION *m = mem->mem[i];

			assert (m != NULL);
			assert (m->loc == block);

			DEBUG(("Mshrink - shrink region"));
			r = shrink_region(m, size);
			DEBUG(("Mshrink - returning %lx", r));
			return r; //shrink_region (m, size);
		}
	}

	DEBUG(("Mshrink: bad address (%lx)", block));
error:
	return EFAULT;
}

long _cdecl
sys_m_validate (short pid, void *_addr, long size, long *flags)
{
	unsigned long addr = (unsigned long)_addr;
	struct proc *p = NULL;
	MEMREGION *m;

	TRACE (("Mvalidate(%i, %lx, %li, %p)", pid, addr, size, flags));

	if (pid == 0)
		p = get_curproc();
	else if (pid > 0)
		p = pid2proc (pid);

	if (!p)
	{
		DEBUG (("Mvalidate: no such process (pid %i)", pid));
		return ESRCH;
	}

	if (p != get_curproc() && !suser (get_curproc()->p_cred->ucr) && !(get_curproc()->p_mem->memflags & F_OS_SPECIAL))
	{
		DEBUG (("Mvalidate: permission denied"));
		return EPERM;
	}

	m = proc_addr2region (p, addr);
	if (m && (addr + size) <= (m->loc + m->len))
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
sys_m_access(void *_addr, long size, short mode)
{
	unsigned long addr = (unsigned long)_addr;
	struct proc *p = get_curproc();
	MEMREGION *m;

	TRACE (("Maccess(%i, %lx, %li)", mode, addr, size));

	if ((mode < 0) || (mode > 1))
	{
		DEBUG (("Maccess: invalid argument"));
		return EINVAL;
	}

	/* search in the process memory regions */
	m = proc_addr2region (p, addr);
	if (m && (addr + size) <= (m->loc + m->len))
		/* always accessible */
		return 0;

	/* search in all memory reagions */
	m = addr2region (addr);
	if (m && (addr + size) <= (m->loc + m->len))
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
