/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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

# include "k_exec.h"

# include "libkern/libkern.h"

# include "mint/asm.h"
# include "mint/basepage.h"
# include "mint/credentials.h"
# include "mint/filedesc.h"
# include "mint/signal.h"
# include "mint/slb.h"
# include "mint/stat.h"

# include "arch/cpu.h"		/* cpush */
# include "arch/context.h"
# include "arch/kernel.h"
# include "arch/mprot.h"
# include "arch/user_things.h"

# include "cmdline.h"
# include "dosdir.h"
# include "filesys.h"
# include "k_exit.h"
# include "k_fds.h"
# include "k_fork.h"
# include "k_prot.h"
# include "k_resource.h"	/* sys_prenice */
# include "kmemory.h"
# include "memory.h"
# include "proc.h"
# include "proc_help.h"
# include "procfs.h"
# include "signal.h"
# include "time.h"
# include "timeout.h"
# include "util.h"

void rts (void);
static struct proc *exec_region(struct proc *p, MEMREGION *mem, int thread);

/*
 * make a local copy of the name, in case we are overlaying
 * the current process
 *
 * localname must be PNAMSIZ+1
 */
static void
make_name(char *localname, const char *ptr1)
{
	const char *n = ptr1;
	const char *lastslash = NULL;
	char *newname = localname;
	int i;

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
		if (*lastslash == '.' || *lastslash == '\0')
		{
			*newname = '\0';
			break;
		}
		else
			*newname++ = *lastslash++;
	}

	*newname = '\0';
}

static long
make_fname(struct proc *p, const char *source)
{
	char tmp[PATH_MAX];
	fcookie exe;
	fcookie dir;
	long r;

	tmp[1] = ':';
	if (source[1] == ':')
	{
		tmp[0] = toupper((int)source[0] & 0xff);
		source += 2;
	}
	else
	{
		assert(get_curproc()->p_cwd);
		tmp[0] = DriveToLetter(get_curproc()->p_cwd->curdrv);
	}

	/* FIXME: This is completely wrong!!! */
	if (DIRSEP(source[0]))	/* absolute path? */
	{
		strncpy(&tmp[2], &source[0], PATH_MAX-2);
		strcpy(p->fname, tmp);
	}
	else
	{
		if (!sys_d_getcwd(&tmp[2], DriveFromLetter(tmp[0]) + 1, PATH_MAX-2))
			ksprintf(p->fname, sizeof(p->fname), "%s\\%s", tmp, source);
		else
			ksprintf(p->fname, sizeof(p->fname), "%s", source);
	}

	r = make_real_cmdline(p);
	if (r)
		return r;

	/* Get a cookie for the name of the executable */
	if (path2cookie(p, p->fname, tmp, &dir) == 0)
	{
	 	if (relpath2cookie(p, &dir, tmp, follow_links, &exe, 0) == 0)
	 	{
	 		/* the release does nothing
	 		 * if the exe cookie is clean
	 		 * (initialized with 0)
	 		 */
			release_cookie(&p->exe);
			p->exe = exe;
		}

		release_cookie(&dir);
	}

	return 0;
}

long _cdecl
sys_pexec(short mode, const void *p1, const void *p2, const void *p3)
{
	union { const void *v; const char *cc; char *c; long l; } ptr_1, ptr_2, ptr_3;
	MEMREGION *base;
	MEMREGION *env = NULL;	/* assignment suppresses spurious warning */
	struct proc *p = NULL;
	long flags = 0;
	char mkbase = 0, mkload = 0, mkgo = 0, mkwait = 0, mkfree = 0;
	char overlay = 0;
	char thread = 0;
	char ptrace;
	char mkname = 0;
	char localname[PNAMSIZ+1];
	XATTR xattr;
	int new_pid;
	int aes_hack = 0;

# ifdef DEBUG_INFO
	/* tfmt and tail_offs are used for debugging only */
	const char *tfmt = "Pexec(%d,%s,\"%s\",$%08lx)";
	int tail_offs = 1;
# endif
	TRACE(("Pexec mode:%d",mode));

	ptr_1.v = p1;
	ptr_2.v = p2;
	ptr_3.v = p3;
#if 0
	{
		struct proc *pr = get_curproc();
		if (!strnicmp(pr->name, "guitar", 6))
			display("pexec(%d) for %s", mode, pr->name);
	}
#endif
	DEBUG(("Pexec: mode %d, $%08lx,$%08lx,$%08lx", mode, ptr_1.l, ptr_2.l, ptr_3.l));

	/* the high bit of mode controls process tracing */
	switch (mode & 0x7fff)
	{
		case 0:
			mkwait = 1;
			/* fall through */
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
			tfmt = "Pexec(%d,$%08lx,BP:$%08lx,$%08lx)";
			tail_offs = 0;
# endif
			break;
		case 106:
			mkfree = 1;		/* fall through */
		case 104:
			thread = (mode == 104);
			mkgo = 1;
			mkname = (ptr_1.v != NULL);
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,%s,BP:$%08lx,$%08lx)";
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
			mkname = (ptr_1.v != NULL);
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,%s,BP:$%08lx,$%08lx)";
			tail_offs = 0;
# endif
			break;
		case 7:
			flags = ptr_1.l;	/* set program flags */
			if (((flags & F_PROTMODE) >> F_PROTSHIFT) > PROT_MAX_MODE)
			{
				DEBUG(("Pexec: invalid protection mode changed to private"));
				flags = (flags & ~F_PROTMODE) | F_PROT_P;
			}
			/* and fall through */
		case 5:
			mkbase = 1;
# ifdef DEBUG_INFO
			tfmt = "Pexec(%d,$%08lx,%s,$%08lx)";
			tail_offs = 0;
# endif
			break;
		default:
		{
			DEBUG(("Pexec(%d,$%08lx,$%08lx,$%08lx): bad mode\n", mode, ptr_1.l, ptr_2.l, ptr_3.l));
			return ENOSYS;
		}
	}

	TRACE((tfmt, mode, ptr_1.l, ptr_2.c + tail_offs, ptr_3.c));

	/* Pexec with mode 0x8000 indicates tracing should be active */
	ptrace = (!mkwait && (mode & 0x8000));

	/* Was there passed a filename? */
	if (mkname)
		make_name(localname, ptr_1.cc);

	if (mkload || mkbase)
	{
		TRACE(("creating environment"));
		env = create_env(ptr_3.cc, flags);
		if (!env)
		{
			DEBUG(("Pexec: unable to create environment"));
			return ENOMEM;
		}
	}

	if (mkbase)
	{
		long ret;

		TRACE(("creating base page"));
		base = create_base(ptr_2.cc, env, flags, 0L, &ret);
		if (!base)
		{
			DEBUG(("Pexec: unable to create basepage"));
			detach_region(get_curproc(), env);
			return ret;
		}

		TRACELOW (("Pexec: basepage region(%p) is %ld bytes at $%08lx", base, base->len, base->loc));
	}
	else if (mkload)
	{
		char cbuf[128];
		//const char *tail = ptr_2.cc;
		long ret;
		union { const char *cc; char *c; } tail; tail.cc = ptr_2.cc;

		if (overlay)
		{
			static char fbuf[PATH_MAX];
			cbuf[127] = 0;
			ptr_1.c = strncpy(fbuf, ptr_1.c, PATH_MAX-2);
			tail.cc = strncpy(cbuf, ptr_2.cc, 127);
		}

		base = load_region(ptr_1.cc, env, tail.cc, &xattr, &flags, &ret);
		if (!base)
		{
			DEBUG(("Pexec: load_region failed"));
			detach_region(get_curproc(), env);
			return ret;
		}

		TRACE(("Pexec: basepage region(%p) is %ld bytes at $%08lx", base, base->len, base->loc));
	}
	else
	{
		/* mode == 4, 6, 104, 106, 204, or 206 -- just go */

		base = addr2mem(get_curproc(), ptr_2.l);
		if (base)
			env = addr2mem(get_curproc(), (long)((BASEPAGE *)(base->loc))->p_env);
		else
			env = NULL;

		if (!env)
		{
			DEBUG(("Pexec: memory not owned by parent"));
			return EFAULT;
		}
	}

	if (mkload || mkbase)
	{
		/* Now that the file's loaded, flags is set to the prgflags
		 * for the file.  In the case of mkbase it's been right
		 * all along. Here's where we change the protection on
		 * the environment to match those flags.
		 */
		mark_region(env, (short)((flags & F_PROTMODE) >> F_PROTSHIFT), 0);
	}

	if (mkgo)
	{
		BASEPAGE *b;
		long r = 0;

		/* tell the child who the parent was */
		b = (BASEPAGE *)base->loc;

		if (overlay)
		{
			b->p_parent = get_curproc()->p_mem->base->p_parent;
			p = get_curproc();

			/* make sure that exec_region doesn't free the base and env */
			base->links++;
			env->links++;
		}
		else
		{
			b->p_parent = get_curproc()->p_mem->base;
			p = fork_proc(thread ? (FORK_SHAREVM | FORK_SHAREEXT) : FORK_SHAREEXT, &r);
		}

		if (!p)
		{
			if (mkload || mkbase)
			{
				detach_region(get_curproc(), base);
				detach_region(get_curproc(), env);
			}

			return r;
		}

		/* jr: add Pexec information to PROC struct */
		strncpy(p->cmdlin, b->p_cmdlin, 128);

		if (mkload || (thread && ptr_1.cc))
		{
			r = make_fname(p, ptr_1.cc);
			if (r)
			{
				if (mkload || mkbase)
				{
					detach_region(get_curproc(), base);
					detach_region(get_curproc(), env);
				}

				return r;
			}
		}

		if (ptrace)
			p->ptracer = pid2proc(p->ppid);

		/* Stephan Haslbeck: GEM kludge no. x+1
		 * If a program is started by AESSYS, reset its euid/egid,
		 * so AES can run with root rights, but user programs don't.
		 * This should be done by the AES, however.
		 */
		if (!strcmp(get_curproc()->name, "AESSYS"))
		{
			struct pcred *cred = p->p_cred;

			assert(cred && cred->ucr);

			aes_hack = 1;

			cred->ucr = copy_cred(cred->ucr);
			cred->ucr->euid = cred->suid = cred->ruid;
			cred->ucr->egid = cred->sgid = cred->rgid;
		}

		/* Even though the file system won't allow unauthorized access
		 * to setuid/setgid programs, it's better to err on the side
		 * of caution and forbid them to be traced (since the parent
		 * can arrange to share the child's address space, not all
		 * accesses need to go through the file system.)
		 */
		if (mkload && mkgo && !p->ptracer)	/* setuid/setgid is OK */
		{
			struct pcred *cred = p->p_cred;

			assert(cred && cred->ucr);

			if (!aes_hack && (xattr.mode & S_ISUID))
			{
				cred->ucr = copy_cred(cred->ucr);
				cred->ucr->euid = cred->suid = xattr.uid;
			}
			else
				cred->suid = cred->ucr->euid;

			/* Whether or not the AES hack should still be used
			 * for the group ids is debatable.  --gfl
			 */
			if (!aes_hack && (xattr.mode & S_ISGID))
			{
				cred->ucr = copy_cred(cred->ucr);
				cred->ucr->egid = cred->sgid = xattr.gid;
			}
			else
				cred->sgid = cred->ucr->egid;
		}

		/* notify proc extensions */
		proc_ext_on_exec(get_curproc());

		/* exec_region frees the memory attached to p;
		 * that's always what we want, since fork_proc duplicates
		 * the memory, and since if we didn't call fork_proc
		 * then we're overlaying.
		 *
		 * NOTE: after this call, we may not be able to access the
		 * original address space that the Pexec was taking place in
		 * (if this is an overlaid Pexec, we just freed that memory).
		 */
		exec_region(p, base, thread);
		attach_region(p, env);
		attach_region(p, base);

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

			/* let our parent run, if it Vfork'd() */
			if ((p = pid2proc(get_curproc()->ppid)) != NULL)
			{
				unsigned short sr = splhigh();
				if (p->wait_q == WAIT_Q &&  p->wait_cond == (long)get_curproc())
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
			assert(get_curproc()->magic == CTXT_MAGIC);
			fresh_slices(3);
			leave_kernel();
			change_context(&(get_curproc()->ctxt[CURRENT]));
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
		detach_region(get_curproc(), base);
		detach_region(get_curproc(), env);
	}

	if (mkwait)
	{
		long oldsigint, oldsigquit;
		unsigned short retval;
		long ret;

		assert(get_curproc()->p_sigacts);

		oldsigint = SIGACTION(get_curproc(), SIGINT).sa_handler;
		oldsigquit = SIGACTION(get_curproc(), SIGQUIT).sa_handler;

		SIGACTION(get_curproc(), SIGINT).sa_handler = SIG_IGN;
		SIGACTION(get_curproc(), SIGQUIT).sa_handler = SIG_IGN;

		new_pid = p->pid;
		for(;;)
		{
			ret = pwaitpid(get_curproc()->pid ? new_pid : -1, 0, NULL, (short *)&retval);
			if (ret < 0)
			{
				ALERT("p_exec: wait error");
				ret = EINTERNAL;
				break;
			}

			if (new_pid == ((ret & 0xffff0000L) >> 16))
			{
				TRACE(("leaving Pexec; child return code %ld", ret));

				/* sys_pwaitpid() strips the low word down to 8 bit,
				 * fix that.
				 * Ozk: thats not enough, gotta pass back all 16 bits.
				 */
				ret = retval;
				ret &= 0x0000ffffL;
				break;
			}

			if (get_curproc()->pid)
				DEBUG(("Pexec: wrong child found"));
		}

		assert(get_curproc()->p_sigacts);

		SIGACTION(get_curproc(), SIGINT).sa_handler = oldsigint;
		SIGACTION(get_curproc(), SIGQUIT).sa_handler = oldsigquit;

		return ret;
	}
	else if (mkgo)
	{
		/* warning: after the yield() the "p" structure may not
		 * exist any more (if the child exits right away)
		 */
		new_pid = p->pid;
		yield();	/* let the new process run */
		return new_pid;
	}
	else
	{
		/* guarantee ourselves at least 3 timeslices to do an Mshrink */
		fresh_slices(3);
		TRACE(("leaving Pexec with basepage address $%08lx", base->loc));
		return base->loc;
	}
}

/*
 * exec_region(p, mem, thread): create a child process out of a mem region
 * "p" is the process structure set up by the parent; it may be "curproc",
 * if we're overlaying. "mem" is the loaded memory region returned by
 * "load region". Any open files (other than the standard handles) owned
 * by "p" are closed, and if thread !=0 all memory is released; the caller
 * must explicitly attach the environment and base region. The caller must
 * also put "p" on the appropriate queue (most likely READY_Q).
 */

void rts(void) {}		/* dummy termination routine */

static struct proc *
exec_region(struct proc *p, MEMREGION *mem, int thread)
{
	struct filedesc *fd = p->p_fd;
	BASEPAGE *b;
	int i;
	MEMREGION *m;

	TRACE(("exec_region: enter (PROC %p, mem = %p)", p, mem));
	assert(p && mem && fd);
	assert(p->p_cwd);
	assert(p->p_mem);

	b = (BASEPAGE *)mem->loc;

	/* flush cached versions of the text */
	cpushi((void *) b->p_tbase, b->p_tlen);

	/* set some (undocumented) variables in the basepage */
	b->p_defdrv = p->p_cwd->curdrv;
	for (i = 0; i < 6; i++)
		b->p_devx[i] = i;

	fd->dta = (DTABUF *)(b->p_dta = &b->p_cmdlin[0]);

	/* close extra open files */
	for (i = MIN_OPEN; i < fd->nfiles; i++)
	{
		FILEPTR *f = fd->ofiles[i];

		if (f && (fd->ofileflags[i] & FD_CLOEXEC))
		{
			FD_REMOVE(p, i);
			do_close(p, f);
		}
	}

	/* The "text segment" of the SLB library (i.e. its startup code)
	 * is inside its trampoline area, so we adapt the p_tbase appropriately.
	 *
	 * curproc->p_flag & 4 is only true when this call comes from the
	 * sys_pexec(106) made by Slbopen().
	 */
	if (get_curproc()->p_flag & P_FLAG_SLO)
	{
		long *exec_longs = (long *)b->p_tbase;

		/* Test for new program format */
		if (
		     /* Original binutils */
		     (exec_longs[0] == 0x283a001aL && exec_longs[1] == 0x4efb48faL)
		     /* binutils >= 2.18-mint-20080209 */
		     || (exec_longs[0] == 0x203a001aL && exec_longs[1] == 0x4efb08faL)
		   )
		{
			exec_longs += (228 / sizeof(long));
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
			exec_longs = (long *) ((char *) b->p_tbase + e_entry);
		}

		if (exec_longs[0] == SLB_HEADER_MAGIC)
		{
			/* attach temporarily to current process to avoid bus error,
			 * and pickup the address from the trampoline jumptable.
			 */
			attach_region(get_curproc(), p->p_mem->tp_reg);
			b->p_tbase = p->p_mem->tp_ptr->slb_init_and_exit_p;
			detach_region(get_curproc(), p->p_mem->tp_reg);
		}
	}

	/* initialize memory */
	recalc_maxmem(p, b->p_tlen + b->p_dlen + b->p_blen);
	if (p->maxmem)
	{
		shrink_region(mem, p->maxmem);
		b->p_hitpa = b->p_lowtpa + mem->len;
	}

	if (!thread)
	{
		assert(p->p_mem && p->p_mem->mem);

		p->p_mem->memflags = b->p_flags;
		p->p_mem->base = b;

		/* check for a valid text region. some compilers (e.g. Lattice 3) just
		 * throw everything into the text region, including data; fork() must
		 * be careful to save the whole region, then. We assume that if the
		 * compiler (or assembler, or whatever) goes to the trouble of making
		 * separate text, data, and bss regions, then the text region is code
		 * and isn't modified and fork doesn't have to save it.
		 */
		if (b->p_blen != 0 || b->p_dlen != 0)
			p->p_mem->txtsize = b->p_tlen;
		else
			p->p_mem->txtsize = 0;

		for (i = 0; i < p->p_mem->num_reg; i++)
		{
			m = p->p_mem->mem[i];
			if (m)
			{
				m->links--;
				if (m->links <= 0)
				{
					if (!m->links)
						free_region(m);
					else
						FATAL("exec_region: region $%08lx bogus link count %ld, not freed (len $%08lx)",
							m->loc, m->links, m->len);
				}
				else
				{
# ifdef WITH_MMU_SUPPORT
					/* Update curproc's mmu table, but not
					 * for basepage and environment */
					if ((m->loc != (unsigned long) b) &&
					    (m->loc != (unsigned long) b->p_env) &&
					    (mem_prot_flags & MPF_STRICT))
					{
						mark_proc_region(p->p_mem, m, PROT_I, p->pid);
					}
# endif
				}
			}
		}

		/*
		 * If the proc struct has a larger mem array than
		 * the default, then free it and allocate a
		 * default-sized one.
		 */
		if (p->p_mem->num_reg > NUM_REGIONS)
		{
			/*
			 * hoo ha! Memory protection problem here. Use
			 * temps and pre-clear p->mem so memprot doesn't try
			 * to walk these structures as we're freeing and
			 * reallocating them!  (Calling kmalloc can cause
			 * a table walk if the alloc results in calling
			 * get_region.)
			 */
			char *ptr = (char *)p->p_mem->mem;
			TRACE(("exec_region: shring large mem array"));
			p->p_mem->mem = NULL;
			p->p_mem->addr = NULL;
			p->p_mem->num_reg = 0;

			kfree(ptr);

			ptr = kmalloc(NUM_REGIONS * sizeof(void *) * 2);
			assert(ptr);

			p->p_mem->mem = (MEMREGION **)ptr;
			p->p_mem->addr = (unsigned long *)(ptr + (NUM_REGIONS * sizeof(void *)));
			p->p_mem->num_reg = NUM_REGIONS;
		}

		/* and clear out */
		mint_bzero(p->p_mem->mem, p->p_mem->num_reg * sizeof(void *) * 2);
	}

	assert(p->p_sigacts);

	/* initialize signals */
	p->p_sigmask = 0;
	for (i = 0; i < NSIG; i++)
	{
		struct sigaction *sigact = &SIGACTION(p, i);

		if (sigact->sa_handler != SIG_IGN)
		{
			sigact->sa_handler = SIG_DFL;
			sigact->sa_mask = 0;
			sigact->sa_flags = 0;
		}
	}

	/* zero the user registers, and set the FPU in a "clear" state */
	for (i = 0; i < 15; i++)
		p->ctxt[CURRENT].regs[i] = 0;

	p->ctxt[CURRENT].sr = 0;
	p->ctxt[CURRENT].fstate.bytes[0] = 0;

	/* set PC, stack registers, etc. appropriately */
	p->ctxt[CURRENT].pc = b->p_tbase;

	/* The "-0x28" is to make sure that syscall.s won't run past the end
	 * of memory when the user makes a system call and doesn't push very
	 * many parameters -- syscall always tries to copy the maximum
	 * possible number of parms.
	 *
	 * NOTE: there's a sanity check here in case programs Mshrink a
	 * basepage without fixing the p_hitpa field in the basepage; this is
	 * to ensure compatibility with older versions of MiNT, which ignore
	 * p_hitpa.
	 */
	if (valid_address(b->p_hitpa - 0x28))
		p->ctxt[CURRENT].usp = b->p_hitpa - 0x28;
	else
		p->ctxt[CURRENT].usp = mem->loc + mem->len - 0x28;

	p->ctxt[CURRENT].ssp = (long)(p->stack + ISTKSIZE);
	p->ctxt[CURRENT].term_vec = (long) rts;

	/* set up stack for process */
	*((long *)(p->ctxt[CURRENT].usp + 4)) = (long) b;

	/* An ugly hack: dLibs tries to poke around in the parent's address
	 * space to find stuff. For now, we'll allow this by faking a pointer
	 * into the parent's address space in the place in the basepage where
	 * dLibs is expecting it. This ugly hack only works correctly if the
	 * Pexec'ing program (i.e. curproc) is in user mode.
	 */
	if (get_curproc() != rootproc)
		get_curproc()->p_mem->base->p_usp = get_curproc()->ctxt[SYSCALL].usp - 0x32;

	TRACE(("exec_region: ok (%p)", p));
	return p;
}

long _cdecl
create_process(const void *filename, const void *cmdline, const void *newenv,
	       struct proc **pret, long stack, struct create_process_opts *opts)
{
	char localname[PNAMSIZ+1];
	MEMREGION *env = NULL;
	MEMREGION *base = NULL;
	BASEPAGE *b;
	struct proc *p;
	long r;

	if (pret)
		*pret = NULL;

	make_name(localname, filename);

	env = create_env(newenv, 0);
	if (!env)
	{
		DEBUG(("create_process: unable to create environment"));
		r = ENOMEM;
		goto leave;
	}

	base = load_region(filename, env, cmdline, NULL, NULL, &r);
	if (!base)
	{
		DEBUG(("create_process: load_region failed"));
		goto leave;
	}

	TRACE(("create_process: basepage region(%p) is %ld bytes at $%08lx", base, base->len, base->loc));

	b = (BASEPAGE *) base->loc;

	DEBUG(("create_process: p_flags=$%08lx", b->p_flags));

# ifdef WITH_SINGLE_TASK_SUPPORT
	if ((b->p_flags & F_SINGLE_TASK) && (rootproc->modeflags & M_SINGLE_TASK))
	{
		DEBUG(("create_process(single-task):already in single-task-mode."));
		r = EPERM;
		goto leave;
	}
# endif

	if (stack)
	{
		stack += 256 + b->p_tlen + b->p_dlen + b->p_blen;
		r = shrink_region(base, stack);
		DEBUG(("create_process: shrink_region(%li) -> %li", stack, r));
	}

	/* tell the child who the parent was */
	b->p_parent = get_curproc()->p_mem->base;

	r = 0;
	p = fork_proc(0, &r);
	if (!p)
		goto leave;

# ifdef WITH_SINGLE_TASK_SUPPORT
	if (b->p_flags & F_SINGLE_TASK)
	{
		DEBUG(("create_process: setting M_SINGLE_TASK for %s", (const char *)filename));
		p->modeflags |= M_SINGLE_TASK;
	}
	if (b->p_flags & F_DONT_STOP)
	{
		DEBUG(("create_process: setting M_DONT_STOP for %s", (const char *)filename));
		p->modeflags |= M_DONT_STOP;
	}
	b->p_flags &= ~(F_SINGLE_TASK | F_DONT_STOP);
# endif

	/* jr: add Pexec information to PROC struct */
	strncpy(p->cmdlin, b->p_cmdlin, 128);

	make_fname(p, filename);

	/* optionally set some things */
	if (opts && opts->mode)
	{
		if (opts->mode & CREATE_PROCESS_OPTS_MAXCORE)
		{
			/* just set maxcore, exec_region calls recalc_maxmem for us */
			if (opts->maxcore >= 0)
				p->maxcore = opts->maxcore;
		}

		if (opts->mode & CREATE_PROCESS_OPTS_NICELEVEL)
			sys_prenice(p->pid, opts->nicelevel);

		if (opts->mode & CREATE_PROCESS_OPTS_DEFDIR)
			sys_d_setpath0(p, opts->defdir);

		if (opts->mode & CREATE_PROCESS_OPTS_UID)
			proc_setuid(p, opts->uid);

		if (opts->mode & CREATE_PROCESS_OPTS_GID)
			proc_setgid(p, opts->gid);
	}

	/* notify proc extensions */
	proc_ext_on_exec(get_curproc());

	/* exec_region frees the memory attached to p;
	 * that's always what we want, since fork_proc duplicates
	 * the memory, and since if we didn't call fork_proc
	 * then we're overlaying.
	 *
	 * NOTE: after this call, we may not be able to access the
	 * original address space that the Pexec was taking place in
	 * (if this is an overlaid Pexec, we just freed that memory).
	 */
	exec_region(p, base, 0);
	attach_region(p, env);
	attach_region(p, base);

	/* interesting coincidence:
	 * if a process needs a name, it usually needs to have
	 * its domain reset to DOM_TOS. Doing it this way
	 * (instead of doing it in exec_region) means that
	 * Pexec(4,...) can be used to create new threads of
	 * execution which retain the same domain.
	 */
	p->domain = DOM_TOS;

	/* put in the new process name we saved above */
	strcpy(p->name, localname);

	/* set the time/date stamp of u:\proc */
	procfs_stmp = xtime;

	if (pret)
		*pret = p;

	run_next(p, 3);

leave:
	if (base) detach_region(get_curproc(), base);
	if (env) detach_region(get_curproc(), env);

	return r;
}
