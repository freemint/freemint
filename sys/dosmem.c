/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */

/*
 * GEMDOS emulation routines:
 * 
 * these are for the GEMDOS system calls concerning
 * allocating/freeing memory
 * 
 * including Pexec() (since this allocates memory)
 * and Pterm() (since this, implicitly, frees it).
 */

# include "dosmem.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/asm.h"
# include "mint/basepage.h"
# include "mint/signal.h"

# include "arch/context.h"
# include "arch/kernel.h"
# include "arch/mprot.h"

# include "bios.h"
# include "cmdline.h"
# include "dosdir.h"
# include "dosfile.h"
# include "dossig.h"
# include "filesys.h"
# include "kmemory.h"
# include "memory.h"
# include "proc.h"
# include "procfs.h"
# include "rendez.h"
# include "signal.h"
# include "slb.h"
# include "time.h"
# include "timeout.h"
# include "util.h"
# include "xbios.h"


/* new call for TT TOS, for the user to inform DOS of alternate memory
 * FIXME: we really shouldn't trust the user so completely
 * FIXME: doesn't work if memory protection is on
 */
long _cdecl
m_addalt (long start, long size)
{
	if (curproc->euid)
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
_do_malloc(MMAP map, long size, int mode)
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
m_alloc(long size)
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
m_free(virtaddr block)
{
	MEMREGION *m;
	int i;

	TRACE(("Mfree(%lx)", block));
	
	if (!block)
	{
		DEBUG(("Mfree: null pointer"));
		return EFAULT;
	}

	/* search backwards so that most recently allocated incarnations of
	 * shared memory blocks are freed first
	 * (this doesn't matter very often)
	 */

	for (i = curproc->num_reg - 1; i >= 0; i--)
	{
		if (curproc->addr[i] == block)
		{
			m = curproc->mem[i];
			
			assert(m != NULL);
			assert(m->loc == (long)block);
			
			curproc->mem[i] = 0;
			curproc->addr[i] = 0;
			
			m->links--;
			if (m->links == 0)
			{
				free_region(m);
			}
			
			return E_OK;
		}
	}

	/* hmmm... if we didn't find the region, perhaps it's a global
	 * one (with the M_KEEP flag set) belonging to a process that
	 * terminated
	 */
	for (i = rootproc->num_reg - 1; i >= 0; i--)
	{
		if (rootproc->addr[i] == block)
		{
			m = rootproc->mem[i];
			
			assert(m != NULL);
			assert(m->loc == (long)block);
			
			if (!(m->mflags & M_KEEP))
				continue;
			
			TRACE(("Freeing M_KEPT memory"));
			
			rootproc->mem[i] = 0;
			rootproc->addr[i] = 0;
			
			m->links--;
			if (m->links == 0)
			{
				free_region(m);
			}
			
			return E_OK;
		}
	}

	DEBUG(("Mfree: bad address %lx", block));
	return EFAULT;
}

long _cdecl
m_shrink(int dummy, virtaddr block, long size)
{
	MEMREGION *m;
	int i;

	UNUSED(dummy);
	
	TRACE(("Mshrink: %lx to %ld", block, size));
	
	if (!block)
	{
		DEBUG(("Mshrink: null pointer"));
		return EFAULT;
	}

	for (i = 0; i < curproc->num_reg; i++)
	{
		if (curproc->addr[i] == block)
		{
			m = curproc->mem[i];
			
			assert(m != NULL);
			assert(m->loc == (long)block);
			
			return shrink_region(m, size);
		}
	}
	
	DEBUG(("Mshrink: bad address (%lx)", block));
	return EFAULT;
}

long _cdecl
m_validate (int pid, void *addr, long size, long *flags)
{
	struct proc *p = NULL;
	MEMREGION *m;
	
	TRACE (("Mvalidate(%i, %lx, %li)", pid, addr, size));
	
	if (pid == 0)
		p = curproc;
	else if (pid > 0)
		p = pid2proc (pid);
	
	if (!p)
	{
		DEBUG (("Mvalidate: no such process (pid %i)", pid));
		return ESRCH;
	}
	
	if (p != curproc && curproc->euid && !(curproc->memflags & F_OS_SPECIAL))
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
m_access (int mode, void *addr, long size)
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

long _cdecl
p_exec (int mode, const void *ptr1, const void *ptr2, const void *ptr3)
{
	MEMREGION *base;
	MEMREGION *env = NULL;	/* assignment suppresses spurious warning */
	MEMREGION *text = NULL;	/* for shared text regions */
	PROC *p;
	long r, flags = 0;
	long status;
	int i;
	char mkbase = 0, mkload = 0, mkgo = 0, mkwait = 0, mkfree = 0;
	char overlay = 0;
	char thread = 0;
	char ptrace;
	char mkname = 0;
	char localname[PNAMSIZ+1];
	XATTR xattr;
	int newpid;

# ifdef DEBUG_INFO
	/* tfmt and tail_offs are used for debugging only */
	const char *tfmt = "Pexec(%d,%s,\"%s\",%lx)";
	int tail_offs = 1;
# endif

	/* the high bit of mode controls process tracing */
	switch(mode & 0x7fff)
	{
		case 0:
			mkwait = 1;		/* fall through */
		case 100:
			mkload = mkgo = mkfree = 1;
			mkname = 1;
			break;
		case 200:			/* overlay current process */
			mkload = mkgo = 1;
			overlay = mkname = 1;
			break;
		case 3:
			mkload = 1;
			break;
		case 6:
			mkfree = 1;
			/* fall through */
		case 4:
			mkwait = mkgo = 1;
			thread = (mode == 4);
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,%lx,BP:%lx,%lx)";
			tail_offs = 0;
# endif
			break;
		case 106:
			mkfree = 1;		/* fall through */
		case 104:
			thread = (mode == 104);
			mkgo = 1;
			mkname = (ptr1 != 0);
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,%s,BP:%lx,%lx)";
			tail_offs = 0;
# endif
			break;
		case 206:
# if 0
	/* mkfree has no effect when overlay is set, since
	 * in this case the "parent" and "child" are the same
	 * process; since the "child" will run in memory that the
	 * "parent" allocated, we don't want that memory freed!
	 */
			mkfree = 1;
# endif
			/* fall through */
		case 204:
			mkgo = overlay = 1;
			mkname = (ptr1 != 0);
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,%s,BP:%lx,%lx)";
			tail_offs = 0;
# endif
			break;
		case 7:
			flags = (long)ptr1;	/* set program flags */
			if (((flags & F_PROTMODE) >> F_PROTSHIFT) > PROT_MAX_MODE)
			{
				DEBUG (("Pexec: invalid protection mode changed to private"));
				flags = (flags & ~F_PROTMODE) | F_PROT_P;
			}
# ifndef MULTITOS
			if ((flags & F_PROTMODE) == 0 && curproc->base == gem_base)
			{
				flags |= F_PROT_G;
				TRACE (("p_exec: AES special"));
			}
# endif
			/* and fall through */
		case 5:
			mkbase = 1;
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,%lx,%s,%lx)";
			tail_offs = 0;
# endif
			break;
		default:
		{
			DEBUG(("Pexec(%d,%lx,%lx,%lx): bad mode", mode, ptr1, ptr2, ptr3));
			return ENOSYS;
		}
	}
	
	TRACE((tfmt, mode, ptr1, (char *) ptr2 + tail_offs, ptr3));
	
	/* Pexec with mode 0x8000 indicates tracing should be active */
	ptrace = (!mkwait && (mode & 0x8000));
	
	/* in most cases, we'll want a process struct to exist,
	 * so make sure one is around. Note that we must be
	 * careful to free it later!
	 */
	
	TRACE(("Checking for memory for new PROC structure"));
	p = NULL;
	if (!overlay)
	{
		p = new_proc();
		if (!p)
		{
			DEBUG(("Pexec: couldn't get a PROC struct"));
			return ENOMEM;
		}
	}

	/* make a local copy of the name, in case we are overlaying
	 * the current process
	 */
	if (mkname)
	{
		const char *n = ptr1;
		const char *lastslash = NULL;
		char *newname = localname;
		
		while (*n)
		{
			if (*n == '\\' || *n == '/')
				lastslash = n;
			
			n++;
		}
		
		if (!lastslash)
			lastslash = ptr1;
		else
			lastslash++;
		
		i = 0;
		while (i++ < PNAMSIZ)
		{
			if (*lastslash == '.' || *lastslash == 0)
			{
				*newname = 0; break;
			}
			else
				*newname++ = *lastslash++;
		}
		
		*newname = 0;
	}

	TRACE(("creating environment"));
	if (mkload || mkbase)
	{
		env = create_env((char *)ptr3, flags);
		if (!env)
		{
			DEBUG(("Pexec: unable to create environment"));
			if (p) dispose_proc(p);
			return ENOMEM;
		}
	}
	
	TRACE(("creating base page"));
	if (mkbase)
	{
		long r;
		
		base = create_base((char *)ptr2, env, flags, 0L, 0L, 0L, 0L, 0L, 0L, &r);
		if (!base)
		{
			DEBUG(("Pexec: unable to create basepage"));
			detach_region(curproc, env);
			if (p) dispose_proc(p);
			return r;
		}
		TRACELOW(("Pexec: basepage region(%lx) is %ld bytes at %lx", base, base->len, base->loc));
	}
	else if (mkload)
	{
		char cbuf[128];
		const char *tail = ptr2;
		long r;
		
		if (overlay)
		{
			static char fbuf[PATH_MAX];
			cbuf[127] = 0;
			ptr1 = strncpy (fbuf, ptr1, PATH_MAX-2);
			tail = strncpy (cbuf, ptr2, 127);
		}
# if 0
		base = load_region (ptr1, env, (char *)tail, &xattr, &text, &flags, 0, &r);
# else
		base = load_region (ptr1, env, (char *)tail, &xattr, &text, &flags, overlay, &r);
# endif
		if (!base)
		{
			DEBUG(("Pexec: load_region failed"));
			detach_region(curproc, env);
			if (p) dispose_proc(p);
			return r;
		}
		TRACE(("Pexec: basepage region(%lx) is %ld bytes at %lx", base, base->len, base->loc));
	}
	else
	{
		/* mode == 4, 6, 104, 106, 204, or 206 -- just go */
		
		base = addr2mem(curproc, (virtaddr)ptr2);
		if (base)
			env = addr2mem(curproc, *(void **)(base->loc + 0x2c));
		else
			env = NULL;
		if (!env)
		{
			DEBUG(("Pexec: memory not owned by parent"));
			if (p) dispose_proc(p);
			return EFAULT;
		}
# if 0
		/* make sure that the PC we are about to use is in a region
		 * which is attached to the process; this is most commonly a
		 * problem for shared text segment programs.
		 * 
		 * BUG: we should verify that the PC is in a region to which the
		 * child process should legitimately have access.
		 */
		text = addr2region(((BASEPAGE *)base->loc)->p_tbase);
		if (text == base)
		{
			/* text segment is part of base region */
			text = NULL;
		}
# endif
	}
	
	if (mkload || mkbase)
	{
		/* Now that the file's loaded, flags is set to the prgflags
		 * for the file.  In the case of mkbase it's been right
		 * all along. Here's where we change the protection on
		 * the environment to match those flags.
		 */
		mark_region(env,(short)((flags & F_PROTMODE) >> F_PROTSHIFT));
	}
	
	if (p)
	{
		/* free the PROC struct so fork_proc will succeed
		 * 
		 * FIXME: it would be much better to pass the PROC
		 * as a parameter to fork_proc!!
		 */
		dispose_proc(p);
		p = NULL;
	}

	if (mkgo)
	{
		BASEPAGE *b;
		long r = 0;
		
		/* tell the child who the parent was */
		b = (BASEPAGE *)base->loc;
		
		if (overlay)
		{
			b->p_parent = curproc->base->p_parent;
			p = curproc;
			
			/* make sure that exec_region doesn't free the base and env */
			base->links++;
			env->links++;
			if (text) text->links++;
		}
		else
		{
			b->p_parent = curproc->base;
			p = fork_proc(&r);
		}
		
		if (!p)
		{
			if (mkbase)
			{
				detach_region(curproc, base);
				detach_region(curproc, env);
				if (text) detach_region(curproc, text);
			}
			
			return r;
		}
		
		/* jr: add Pexec information to PROC struct */
		strncpy(p->cmdlin, b->p_cmdlin, 128);
		if (mkload || (thread && ptr1))
		{
			char tmp[PATH_MAX];
			const char *source = ptr1;
			fcookie exe;
			fcookie dir;
			
			tmp[1] = ':';
			if (source[1] == ':')
			{
				tmp[0] = toupper(source[0]);
				source += 2;
			}
			else
			{
				tmp[0] = curproc->curdrv + ((curproc->curdrv < 26) ? 'A' : '1'-26);
			}
			
			/* FIXME: This is completely wrong!!! */
			if (DIRSEP (source[0]))	/* absolute path? */
			{
				strncpy (&tmp[2], &source[0], PATH_MAX-2);
				strcpy (p->fname, tmp);
			}
			else
			{
				if (!d_getcwd (&tmp[2], tmp[0] - ((tmp[0] <= 'Z'+6) ? 'A' : '1'-26) + 1, PATH_MAX-2))
					ksprintf (p->fname, sizeof (p->fname), "%s\\%s", tmp, source);
				else
					ksprintf (p->fname, sizeof (p->fname), "%s", source);
			}
			
			status = make_real_cmdline (p);
			if (status)
			{
				if (mkbase)
				{
					detach_region (curproc, base);
					detach_region (curproc, env);
					if (text) detach_region (curproc, text);
				}
				
				return status;
			}
			
			/* Get a cookie for the name of the executable */
			if (path2cookie (p->fname, tmp, &dir) == 0)
			{
			 	if (relpath2cookie (&dir, tmp, follow_links, &exe, 0) == 0)
			 	{
			 		/* the release make nothing
			 		 * if the exe cookie is clean
			 		 * (initialized with 0)
			 		 */
					release_cookie (&p->exe);
					p->exe = exe;
				}
				
				release_cookie (&dir);
			}
		}
		
		if (ptrace)
			p->ptracer = pid2proc (p->ppid);
		
# ifdef MULTITOS
		/* Stephan Haslbeck: GEM kludge no. x+1
		 * If a program is started by AESSYS, reset its euid/egid,
		 * so AES can run with root rights, but user programs don't.
		 * This should be done by the AES, however.
		 */
		if(!strcmp (curproc->name, "AESSYS"))
		{
			p->euid = p->suid = p->ruid;
			p->egid = p->sgid = p->rgid;
		}
# endif

		/* Even though the file system won't allow unauthorized access
		 * to setuid/setgid programs, it's better to err on the side
		 * of caution and forbid them to be traced (since the parent
		 * can arrange to share the child's address space, not all
		 * accesses need to go through the file system.)
		 */
		if (mkload && mkgo && !p->ptracer)	/* setuid/setgid is OK */
		{
			if (xattr.mode & S_ISUID)
				p->euid = p->suid = xattr.uid;
			else
				p->suid = p->euid;
			
			if (xattr.mode & S_ISGID)
				p->egid = p->sgid = xattr.gid;
			else
				p->sgid = p->egid;
		}
		
		/* exec_region frees the memory attached to p;
		 * that's always what we want, since fork_proc duplicates
		 * the memory, and since if we didn't call fork_proc
		 * then we're overlaying.
		 * 
		 * NOTE: after this call, we may not be able to access the
		 * original address space that the Pexec was taking place in
		 * (if this is an overlaid Pexec, we just freed that memory).
		 */
		(void)exec_region(p, base, thread);
		attach_region(p, env);
		attach_region(p, base);
		if (text) attach_region(p, text);

		if (mkname)
		{
			/* interesting coincidence:
			 * if a process needs a name, it usually needs to have
			 * its domain reset to DOM_TOS. Doing it this way
			 * (instead of doing it in exec_region) means that
			 * Pexec(4,...) can be used to create new threads of
			 * execution which retain the same domain.
			 */
			if (!thread)
				p->domain = DOM_TOS;

			/* put in the new process name we saved above */
			strcpy(p->name, localname);
		}

		/* turn on tracing for the new process */
		if (p->ptracer)
		{
			p->ctxt[CURRENT].ptrace = 1;
			// XXX - hack alert - handled by context_*
			// post_sig (p, SIGTRAP);
		}
		
		/* set the time/date stamp of u:\proc */
		procfs_stmp = xtime;
		
		if (overlay)
		{
			/* correct for temporary increase in links (see above) */
			base->links--;
			env->links--;
			if (text) text->links--;
			
			/* let our parent run, if it Vfork'd() */
			if ((p = pid2proc(curproc->ppid)) != NULL)
			{
				short sr = spl7();
				if (p->wait_q == WAIT_Q
					&&  p->wait_cond == (long)curproc)
				{
					rm_q(WAIT_Q, p);
					add_q(READY_Q, p);
				}
				spl(sr);
			}

			/* OK, let's run our new code
			 * we guarantee ourselves at least 3 timeslices to do
			 * an Mshrink
			 */
			assert(curproc->magic == CTXT_MAGIC);
			fresh_slices(3);
			leave_kernel();
			change_context(&(curproc->ctxt[CURRENT]));
		}
		else
		{
			/* we want this process to run ASAP
			 * so we temporarily give it high priority
			 * and put it first on the run queue
			 */
			run_next(p, 3);
		}
	}

	if (mkfree)
	{
		detach_region(curproc, base);
		detach_region(curproc, env);
		if (text) detach_region(curproc, text);
	}

	if (mkwait)
	{
		long oldsigint, oldsigquit;

		oldsigint = curproc->sighandle[SIGINT];
		oldsigquit = curproc->sighandle[SIGQUIT];
		curproc->sighandle[SIGINT] =
			 curproc->sighandle[SIGQUIT] = SIG_IGN;

		newpid = p->pid;
		for(;;)
		{
			r = p_waitpid(curproc->pid ? newpid : -1, 0, (long *)0);
			if (r < 0)
			{
				ALERT("p_exec: wait error");
				return EINTERNAL;
			}
			if (newpid == ((r&0xffff0000L) >> 16))
			{
				TRACE(("leaving Pexec; child return code %ld", r));
				r = r & 0x0000ffffL;
				break;
			}
			if (curproc->pid)
				DEBUG(("Pexec: wrong child found"));
		}
		curproc->sighandle[SIGINT] = oldsigint;
		curproc->sighandle[SIGQUIT] = oldsigquit;
		return r;
	}
	else if (mkgo)
	{
		/* warning: after the yield() the "p" structure may not
		 * exist any more (if the child exits right away)
		 */
		newpid = p->pid;
		yield();	/* let the new process run */
		return newpid;
	}
	else
	{
		/* guarantee ourselves at least 3 timeslices to do an Mshrink */
		fresh_slices(3);
		TRACE(("leaving Pexec with basepage address %lx", base->loc));
		return base->loc;
	}
}

/*
 * terminate a process, with return code "code".
 * If que == ZOMBIE_Q, free all resources attached to the child.
 * If que == TSR_Q, free everything but memory.
 * 
 * NOTE: terminate() should be called only when the process is to be
 * "terminated with extreme prejuidice". Most times, p_term or p_termres
 * are the functions to use, since they allow the user to do some cleaning
 * up, etc.
 */

long
terminate (PROC *curproc, int code, int que)
{
	PROC *p;
	FILEPTR *fp;
	int  i, wakemint = 0;
	DIR *dirh, *nexth;
	ushort sr;
	
	if (bconbsiz)
		(void) bflush();
	
	assert(que == ZOMBIE_Q || que == TSR_Q);
	
	/* Exit all non-closed shared libraries */
	while (slb_close_on_exit(1))
		;
	
	/* Remove structure if curproc is an SLB */
	remove_slb();
	
	if (curproc->pid == 0)
		FATAL("attempt to terminate MiNT");
	
	/* cancel all user-specified interrupt signals */
	cancelsigintrs();
	
	/* cancel all pending timeouts for this process */
	cancelalltimeouts();
	
	/* cancel alarm clock */
	curproc->alarmtim = 0;
	
	/* release any drives locked by Dlock */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		if (dlockproc[i] == curproc)
		{
			dlockproc[i] = 0;
			changedrv(i);
		}
	}

# if 0
	/* release the controlling terminal,
	 * if we're a process group leader
	 */
	fp = curproc->handle[-1];
	if (fp && is_terminal(fp) && curproc->pgrp == curproc->pid)
	{
		struct tty *tty = (struct tty *)fp->devinfo;
		if (curproc->pgrp == tty->pgrp)
			tty->pgrp = 0;
	}
# else
	/* release the controlling terminal,
	 * if we're the last member of this pgroup
	 */
	fp = curproc->control;
	if (fp && is_terminal(fp))
	{
		struct tty *tty = (struct tty *)fp->devinfo;
		int pgrp = curproc->pgrp;

		if (pgrp == tty->pgrp)
		{
			PROC *p;
			FILEPTR *pfp;

			if (tty->use_cnt > 1)
			{
				for (p = proclist; p; p = p->gl_next)
				{
					if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
						continue;
					
					if (p->pgrp == pgrp
						&& p != curproc
						&& (0 != (pfp = p->control))
						&& pfp->fc.index == fp->fc.index
						&& pfp->fc.dev == fp->fc.dev)
					{
						goto found;
					}
				}
			}
			else
			{
				for (p = proclist; p; p = p->gl_next)
				{
					if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
						continue;
					
					if (p->pgrp == pgrp
						&& p != curproc
						&& p->control == fp)
					{
						goto found;
					}
				}
			}
			tty->pgrp = 0;
		}
found:
		;
	}
# endif

	/* close all files */
	for (i = MIN_HANDLE; i < MAX_OPEN; i++)
	{
		if ((fp = curproc->handle[i]) != 0)
			do_close(fp);
		curproc->handle[i] = 0;
	}
	
	/* close any unresolved Fsfirst/Fsnext directory searches */
	for (i = 0; i < NUM_SEARCH; i++)
	{
		if (curproc->srchdta[i])
		{
			DIR *dirh = &curproc->srchdir[i];
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie(&dirh->fc);
			dirh->fc.fs = 0;
		}
	}

	/* close pending opendir/readdir searches */
	for (dirh = curproc->searches; dirh; )
	{
		if (dirh->fc.fs)
		{
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie(&dirh->fc);
		}
		nexth = dirh->next;
		kfree (dirh);
		dirh = nexth;
	}
	
	/* release the directory cookies held by the process */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		release_cookie(&curproc->curdir[i]);
		curproc->curdir[i].fs = 0;
		release_cookie(&curproc->root[i]);
		curproc->root[i].fs = 0;
	}
	
	if (curproc->root_dir)
	{
		DEBUG (("free root_dir = %s", curproc->root_dir));
		
		release_cookie (&curproc->root_fc);
		curproc->root_fc.fs = 0;
		kfree (curproc->root_dir);
		curproc->root_dir = NULL;
	}
	
	/* Free saved commandline */
	if (curproc->real_cmdline)
	{
		kfree (curproc->real_cmdline);
		curproc->real_cmdline = NULL;
	}
	
	/* free exec cookie */
	release_cookie (&curproc->exe);
	
	/* release all semaphores owned by this process */
	free_semaphores (curproc->pid);
	
	/* free all memory
	 * if mflags & M_KEEP then attach it to process 0
	 */
	if (que == ZOMBIE_Q)
	{
		MEMREGION **hold_mem;
		virtaddr *hold_addr;
		
		for (i = curproc->num_reg - 1; i >= 0; i--)
		{
			MEMREGION *m = curproc->mem[i];
			
			curproc->mem[i] = 0;
			curproc->addr[i] = 0;
			
			if (m)
			{
				/* don't free specially allocated memory */
				if ((m->mflags & M_KEEP) && (m->links <= 1))
					if (curproc != rootproc)
						attach_region(rootproc, m);
				
				/* Leave shared text regions in memory,
				 * get_region will reclaim them if memory
				 * runs low. Assume that we will never
				 * have 65535 processes using a particular
				 * memory region.
				 */
				m->links--;
				if (m->links == 0)
				{
					if (m->mflags & M_SHTEXT_T)
						m->links = 0xffff;
					else
						free_region(m);
				}
			}
		}

		/*
		 * mark the mem & addr arrays as void so the memory
		 * protection code won't try to walk them. Do this before
		 * freeing them so we don't try to walk them when marking
		 * those pages themselves as free!
		 *
		 * Note: when a process terminates, the MMU root pointer
		 * still points to that process' page table, until the next
		 * process is dispatched.  This is OK, since the process'
		 * page table is in system memory, and it isn't going to be
		 * freed.  It is going to wind up on the free process list,
		 * though, after dispose_proc. This might be Not A Good
		 * Thing.
		 */

		hold_addr = curproc->addr;
		hold_mem = curproc->mem;

		curproc->mem = NULL;
		curproc->addr = NULL;
		curproc->num_reg = 0;

		kfree(hold_addr);
		kfree(hold_mem);
	}
/*	else
		 make TSR process non-swappable */
	
	/* make sure that any open files that refer to this process are
	 * closed
	 */
	// XXX changedrv (PROC_RDEV_BASE | curproc->pid);
	
	/* find our parent (if parent not found, then use process 0 as parent
	 * since that process is constantly in a wait loop)
	 */
	p = pid2proc (curproc->ppid);
	if (!p)
	{
		TRACE (("terminate: parent not found"));
		p = pid2proc(0);
	}
	
	/* NOTE: normally just post_sig is sufficient for sending a signal;
	 * but in this particular case, we have to worry about processes
	 * that are blocking all signals because they Vfork'd and are waiting
	 * for us to finish (which is indicated by a wait_cond matching our
	 * PROC structure), and also processes that are ignoring SIGCHLD
	 * but are waiting for us.
	 */
	sr = spl7 ();
	if (p->wait_q == WAIT_Q
		&& (p->wait_cond == (long) curproc || p->wait_cond == (long)p_waitpid))
	{
		rm_q (WAIT_Q, p);
		add_q (READY_Q, p);
		
		TRACE (("terminate: waking up parent"));
	}
	spl (sr);
	
	if (curproc->ptracer && curproc->ptracer != p)
	{
		/* BUG: should we ensure curproc->ptracer is awake ? */
		
		/* tell tracing process */
		post_sig (curproc->ptracer, SIGCHLD);
	}
	
	/* inform of process termination */
	post_sig (p, SIGCHLD);
	
	/* find our children, and orphan them
	 * also, check for processes we were tracing, and
	 * cancel the trace
	 */
	i = curproc->pid;
	for (p = proclist; p; p = p->gl_next)
	{
		if (p->ppid == i)
		{
			p->ppid = 0;	/* have the system adopt it */
			if (p->wait_q == ZOMBIE_Q) 
				wakemint = 1;	/* we need to wake proc. 0 */
		}
		if (p->ptracer == curproc)
		{
			p->ptracer = 0;
			
			/* `FEATURE': we terminate traced processes when the
			 * tracer terminates. It might plausibly be argued
			 * that it would be better to let them continue,
			 * to let some (new) tracer take them over. On the
			 * other hand, if the tracer terminated normally,
			 * it should have used Fcntl(PTRACESFLAGS) to reset
			 * the trace nicely, so something must be wrong for
			 * us to have reached here.
			 */
			post_sig(p, SIGTERM);	/* arrange for termination */
		}
	}
	
	if (wakemint)
	{
		short sr = spl7();
		p = rootproc;		/* pid 0 */
		if (p->wait_q == WAIT_Q)
		{
			rm_q(WAIT_Q, p);
			add_q(READY_Q, p);
		}
		spl(sr);
	}

	/* this makes sure that our children are inherited by the system;
	 * plus, it may help avoid problems if somehow a signal gets
	 * through to us
	 */
	for (i = 0; i < NSIG; i++)
		curproc->sighandle[i] = SIG_IGN;
	
	/* finally, reset the time/date stamp for /proc and /sys.  */
	procfs_stmp = xtime;
	
	sleep (que, (long)(unsigned)code);
	
	/* we shouldn't ever get here */
	FATAL ("terminate: sleep woke up when it shouldn't have");
	return 0;
}

/*
 * TOS process termination entry points:
 * p_term terminates the process, freeing its memory
 * p_termres lets the process hang around resident in memory, after
 * shrinking its transient program area to "save" bytes
 *
 * ATTENTION: From within MiNT, use kernel_pterm(), to avoid problems with
 * shared libraries!
 */

long _cdecl
p_term (int code)
{
	/* Exit all non-closed shared libraries */
	for (;;)
	{
		int cont;
		
		cont = slb_close_on_exit (0);
		if (cont == 1)
			return code;
		
		if (cont == 0)
			break;
	}
	
	return (kernel_pterm (curproc, code));
}

long
kernel_pterm (PROC *p, int code)
{
	CONTEXT *syscall;
	
	TRACE(("Pterm(%d)", code));
	
	/* call the process termination vector */
	syscall = &p->ctxt[SYSCALL];
	
	if (syscall->term_vec != (long) rts)
	{
		TRACE(("term_vec: user has something to do"));
		
		/* we handle the termination vector just like Supexec(), by
		 * sending signal 0 to the process. See supexec() in xbios.c
		 * for details. Note that we _always_ want to unwind the
		 * signal stack, and setting bit 1 of curproc->sigmask tells
		 * handle_sig to do that -- see signal.c.
		 */
		p->sigmask |= 1L;
		(void) supexec ((Func) syscall->term_vec, 0L, 0L, 0L, 0L, (long) code);
		/*
		 * if we arrive here, continue with the termination...
		 */
	}
	
	return terminate (p, code, ZOMBIE_Q);
}

long _cdecl
p_term0 (void)
{
	return p_term (0);
}

long _cdecl
p_termres (long save, int code)
{
	MEMREGION *m;
	int i;

	TRACE(("Ptermres(%ld, %d)", save, code));
	
	m = curproc->mem[1];	/* should be the basepage (0 is env.) */
	if (m)
	{
		(void)shrink_region(m, save);
	}
	
	/* make all of the TSR's private memory globally accessible;
	 * this means that more TSR's will "do the right thing"
	 * without having to have prgflags set.
	 */
	for (i = 0; i < curproc->num_reg; i++)
	{
		m = curproc->mem[i];
		if (m && m->links == 1)
		{
			/* only the TSR is owner */
			if (get_prot_mode(m) == PROT_P)
			{
				mark_region(m, PROT_G);
			}
		}
	}
	
	return terminate (curproc, code, TSR_Q);
}

/*
 * routine for waiting for children to die. Return has the pid of the
 * found child in the high word, and the child's exit code in
 * the low word.  
 * If the process was killed by a signal the signal that caused the 
 * termination is put into the last seven bits. If no children exist, 
 * return "File Not Found".
 *
 * If (nohang & 1) is nonzero, then return a 0 immediately if we have
 * no dead children but some living ones that we still have to wait
 * for. If (nohang & 2) is nonzero, then we return any stopped
 * children; otherwise, only children that have exited or are stopped
 * due to a trace trap are returned.
 * If "rusage" is non-zero and a child is found, put the child's
 * resource usage into it (currently only the user and system time are
 * sent back).
 * The pid argument specifies a set of child processes for which status
 * is requested:
 * 	If pid is equal to -1, status is requested for any child process.
 *
 *	If pid is greater than zero, it specifies the process ID of a
 *	single child process for which status is requested.
 *
 *	If pid is equal to zero, status is requested for any child
 *	process whose process group ID is equal to that of the calling
 *	process.
 *
 *	If pid is less than -1, status is requested for any child process
 *	whose process group ID is equal to the absolute value of pid.
 *
 * Note this call is a real standard crosser... POSIX.1 doesn't have the
 * rusage stuff, BSD doesn't have the pid stuff; both are useful, so why
 * not have it all!
 *
 * NOTE:  There is one important difference to the standard behaviour
 *        of the waitpid function:  If WNOHANG was specified and no
 *        children were found, waitpid should actually return immediately
 *        with no error.  Instead, MiNT will return ENOENT in that
 *        case.  This is probably a bug but we won't change it now
 *        because existing binaries may expect that behavior.  A library
 *        binding should take that into account:  If WNOHANG was specified
 *        and the return value is ENOENT,  don't set errno and return
 *        zero to the process.  If you recompile your `make' program
 *        with that kludge you will never see the
 *
 *        	make: *** wait: file not found.  Stop.
 *
 *        again.
 */

long _cdecl
p_waitpid(int pid, int nohang, long *rusage)
{
	long r;
	PROC *p, *q;
	int ourpid, ourpgrp;
	int found;
	
	
	TRACE(("Pwaitpid(%d, %d, %lx)", pid, nohang, rusage));
	
	
	ourpid = curproc->pid;
	ourpgrp = curproc->pgrp;
	
	/* if there are terminated children, clean up and return their info;
	 * if there are children, but still running, wait for them;
	 * if there are no children, return an error
	 */
	do {
		/* look for any children */
		found = 0;
		for (p = proclist; p; p = p->gl_next)
		{
			if ((p->ppid == ourpid || p->ptracer == curproc)
				&& (pid == -1
					|| (pid > 0 && pid == p->pid)
# if 0
					|| (pid == 0 && p->pgrp == ourpid)
# else
					|| (pid == 0 && p->pgrp == ourpgrp)
# endif
					|| (pid < -1 && p->pgrp == -pid)))
			{
				found++;
				if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
					break;
				
				/* p->wait_cond == 0 if a stopped process
				 * has already been waited for
				 */
				if ((p->wait_q == STOP_Q) && p->wait_cond)
				{
					if ((nohang & 2)
# if 0
						|| ((p->wait_cond & 0x1f00) == (SIGTRAP << 8)))
# else
						|| p->ptracer)
# endif
					{
						break;
					}
				}
			}
		}
		
		if (!p)
		{
			if (found)
			{
				if (nohang & 1)
				{
					TRACE(("Pwaitpid(%d, %d) -> 0 [nohang & 1]", pid, nohang));
					return 0;
				}
				
				if (curproc->pid)
					TRACE(("Pwaitpid: going to sleep"));
				
				sleep (WAIT_Q, (long) p_waitpid);
			}
			else
			{
				/* Don't report that for WNOHANG.  */
				if (!(nohang & 1))
					DEBUG(("Pwaitpid: no children found"));
				
				return ENOENT;
			}
		}
	}
	while (!p);

	/* OK, we've found our child
	 * calculate the return code from the child's exit code and pid and
	 * the possibly fatal signal that caused it to die.
	 */
	r = (((unsigned long)p->pid) << 16) | (p->wait_cond & 0x0000ffff);
	/* This assumes that NSIG is 32, i. e. the highest possible
	   signal is 31.  We have to use 0x9f instead of 0x1f to be
	   prepared for a possible core dump flag.  */
	if (p->signaled)
		r = (r & 0xffff00ff) | ((p->last_sig << 8) & 0x00009f00);
	else if (p->wait_q != STOP_Q && (p->wait_cond & 0x1f00) != SIGTRAP << 8)
		/* Strip down the return code of the child down to 8 bits.
		   Some misbehaving programs exceed this limit which could
		   cause a misinterpretation of the wait status signaled
		   (with possible memory faults if the signal would be
		   > NSIG).  */
		r &= 0xffff00ff;


	/* check resource usage */
	if (rusage)
	{
		*rusage++ = p->usrtime + p->chldutime;
		*rusage = p->systime + p->chldstime;
	}

	/* avoid adding adopted trace processes usage to the foster parent */
	if (curproc->pid == p->ppid)
	{
		/* add child's resource usage to parent's */
		if (p->wait_q == TSR_Q || p->wait_q == ZOMBIE_Q)
		{
			curproc->chldstime += p->systime + p->chldstime;
			curproc->chldutime += p->usrtime + p->chldutime;
		}
	}

	/* if it was stopped, mark it as having been found and again return */
	if (p->wait_q == STOP_Q)
	{
		p->wait_cond = 0;
		
		TRACE(("Pwaitpid(%d, %d) -> %lx [p->wait_q == STOP_Q]", pid, nohang, r));
		return r;
	}

	/* We have to worry about processes which attach themselves to running
	 * processes which they want to trace. We fix things up so that the
	 * second time the signal gets delivered we will go all the way to the
	 * end of this function.
	 */
 	if (p->ptracer && p->ptracer->pid != p->ppid)
 	{
		if (curproc == p->ptracer)
		{
			/* deliver the signal to the tracing process first */
			TRACE(("Pwaitpid(ptracer): returning status %lx to tracing process", r));
			
			p->ptracer = NULL;
			if (p->ppid != -1)
				return r;
		}
		else
		{
			/* Hmmm, the real parent got here first */
			TRACE(("Pwaitpid(ptracer): returning status %lx to parent process", r));
			
			p->ppid = -1;
			return r;
		}
	}
	
	/* if it was a TSR, mark it as having been found and return */
	if (p->wait_q == TSR_Q)
	{
		p->ppid = -1;
		
		TRACE(("Pwaitpid(%d, %d) -> %lx [p->wait_q == TSR_Q]", pid, nohang, r));
		return r;
	}

	/* it better have been on the ZOMBIE queue from here on in... */
	assert(p->wait_q == ZOMBIE_Q);
	assert(p != curproc);

	/* take the child off both the global and ZOMBIE lists */
	{
		short sr = spl7();
		rm_q(ZOMBIE_Q, p);
		spl(sr);
	}

	if (proclist == p)
	{
		proclist = p->gl_next;
		p->gl_next = 0;
	}
	else
	{
		q = proclist;
		while(q && q->gl_next != p)
		{
			q = q->gl_next;
		}
		assert(q);
		q->gl_next = p->gl_next;
		p->gl_next = 0;
	}
	
	/* free the PROC structure */
	dispose_proc(p);
	
	TRACE(("Pwaitpid(%d, %d) -> %lx [end]", pid, nohang, r));
	return r;
}

/*
 * p_wait3: BSD process termination primitive, here to maintain
 * compatibility with existing binaries.
 */
long _cdecl
p_wait3(int nohang, long *rusage)
{
	return p_waitpid(-1, nohang, rusage);
}

/*
 * p_wait: block until a child has exited, and don't worry about
 * resource stats. this is provided as a convenience, and to maintain
 * compatibility with existing binaries (yes, I'm lazy...). we could
 * make do with Pwaitpid().
 */
long _cdecl
p_wait(void)
{
	/* BEWARE:
	 * POSIX says that wait() should be implemented as
	 * Pwaitpid(-1, 0, (long *)0). Pwait is really not
	 * useful for much at all, but we'll keep it around
	 * for a while (with it's old, crufty semantics)
	 * for backwards compatibility. People implementing
	 * POSIX style libraries should use Pwaitpid even
	 * to implement wait().
	 */
	return p_wait3(2, (long *)0);
}

/*
 * p_vfork(): create a duplicate of  the current process. The parent's
 * address space is not saved. The parent is suspended until either the
 * child process (the duplicate) exits or does a Pexec which overlays its
 * memory space.
 */

long _cdecl
p_vfork(void)
{
	PROC *p;
	int newpid;
	long sigmask;
	long r;

	p = fork_proc(&r);
	if (!p)
	{
		DEBUG(("p_vfork: couldn't get new PROC struct"));
		return r;
	}
	
	/* set u:\proc time+date */
	procfs_stmp = xtime;
	
	p->ctxt[CURRENT] = p->ctxt[SYSCALL];
	p->ctxt[CURRENT].regs[0] = 0;		/* child returns a 0 from call */
	p->ctxt[CURRENT].sr &= ~(0x2000);	/* child must be in user mode */
# if 0	/* set up in fork_proc() */
	p->ctxt[CURRENT].ssp = (long)(p->stack + ISTKSIZE);
# endif

	/* watch out for job control signals, since our parent can never wake
	 * up to respond to them. solution: block them; exec_region (in mem.c)
	 * clears the signal mask, so an exec() will unblock them.
	 */
	p->sigmask |= (1L << SIGTSTP) | (1L << SIGTTIN) | (1L << SIGTTOU);

	TRACE(("p_vfork: parent going to sleep, wait_cond == %lx", (long)p));

	/* WARNING: This sleep() must absolutely not wake up until the child
	 * has released the memory space correctly. That's why we mask off
	 * all signals.
	 */
	sigmask = curproc->sigmask;
	curproc->sigmask = ~(((unsigned long)1 << SIGKILL) | 1);

	{
		short sr = spl7();
		add_q(READY_Q, p);		/* put it on the ready queue */
		sleep(WAIT_Q, (long)p);		/* while we wait for it */
		spl(sr);
	}
	
	TRACE(("p_vfork: parent waking up"));

	curproc->sigmask = sigmask;
	/* note that the PROC structure pointed to by p may be freed during
	 * the check_sigs call!
	 */
	newpid = p->pid;
	check_sigs();	/* did we get any signals while sleeping? */
	return newpid;
}

/*
 * p_fork(save): create a duplicate of  the current process. The parent's
 * address space is duplicated using a shadow region descriptor and a save
 * region, so both the parent and the child can be run. On a context switch
 * the parent's and child's address spaces will be exchanged (see proc.c).
 *
 * "txtsize" is the size of the process' TEXT area, if it has a valid one;
 * this is part of the second memory region attached (the basepage one)
 * and need not be saved (we assume processes don't write on their own
 * code segment).
 */

long _cdecl
p_fork (void)
{
	PROC *p;
	MEMREGION *m, *n;
	BASEPAGE *b;
	long txtsize;
	int i, j, newpid;
	long r;
	
	p = fork_proc (&r);
	if (!p)
	{
		DEBUG (("p_fork: couldn't get new PROC struct"));
		return r;
	}
	
	/* set u:\proc time+date */
	procfs_stmp = xtime;
	
	/*
	 * save the parent's address space
	 */
	txtsize = p->txtsize;
	
	TRACE (("p_fork: duplicating parent memory"));
	for (i = 0; i < p->num_reg; i++)
	{
		m = p->mem[i];
		if (m && !(m->mflags & (M_SHTEXT|M_SHARED)))
		{
			if (m->mflags & M_SEEN)
			{
				/* Hmmm, this region has already been duplicated,
				 * reuse the duplicated region's descriptor and
				 * correct the link counts. Note that `m' still
				 * points to the parent's memory descriptor, so
				 * we must search the parent's memory table here.
				 */
				for (j = 0; curproc->mem[j] != m; j++)
					;
				n = p->mem[j];
				n->links++;
				m->links--;
			}
			else
			{
				/* Okay we have to create a new shadow and save
				 * region for this one
				 */
				n = fork_region(m, i == 1 ? txtsize : 0);
				m->mflags |= M_SEEN; /* save links only once */
			}

			if (!n)
			{
				DEBUG(("p_fork: can't save parent's memory"));
				p->ppid = 0;		/* abandon the child */
				post_sig(p, SIGKILL);	/* then kill it */
				p->ctxt[CURRENT].pc = (long)check_sigs;
				p->ctxt[CURRENT].sr |= 0x2000; /* use supervisor stack */
# if 0	/* stack set up in fork_proc() */
				p->ctxt[CURRENT].ssp = (long)(p->stack + ISTKSIZE);
# endif
				p->pri = MAX_NICE+1;
				run_next(p, 1);
				yield();
				return ENOMEM;
			}
			p->mem[i] = n;
		}
	}

	/* The save regions are initially attached to the child's regions. To
	 * prevent swapping of the region and its save region (which should
	 * still be identical) on the next context switch, we attach them here
	 * to the current process.
	 */
	for (i = 0; i < curproc->num_reg; i++)
	{
		m = curproc->mem[i];
		if (m)
		{
			m->mflags &= ~M_SEEN;
			n = p->mem[i];
			if (n->save)
			{
				m->save = n->save;
				n->save = 0;
			}
		}
	}

	/* Correct the parent basepage pointer on the child, it stills
	 * points to its grandparent's basepage.
	 */
	b = (BASEPAGE *)p->mem[1]->loc;
	b->p_parent = (BASEPAGE *)curproc->mem[1]->loc;

	p->ctxt[CURRENT] = p->ctxt[SYSCALL];
	p->ctxt[CURRENT].regs[0] = 0;		/* child returns a 0 from call */
	p->ctxt[CURRENT].sr &= ~(0x2000);	/* child must be in user mode */
# if 0	/* set up in fork_proc() */
	p->ctxt[CURRENT].ssp = (long)(p->stack + ISTKSIZE);
# endif

	/* Now run the child process. We want it to run ASAP, so we
	 * temporarily give it high priority and put it first on the
	 * run queue. Warning: After the yield() the "p" structure may
	 * not exist any more.
	 */
	run_next(p, 3);
	newpid = p->pid;
	yield();
	
	return newpid;
}

