/*
 * $Id$
 * 
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

# include "proc_help.h"

# include "arch/cpu.h"
# include "arch/mprot.h"
# include "arch/user_things.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/proc_extensions.h"

# ifdef JAR_PRIVATE
# include "cookie.h"
# endif

# include "filesys.h"
# include "k_fds.h"
# include "kmemory.h"
# include "memory.h"


/* p_mem */

void
init_page_table_ptr (struct memspace *m)
{
# ifdef NO_MMU
	m->page_table = NULL;
	m->pt_mem = NULL;
# else
	if (no_mem_prot)
	{
		m->page_table = NULL;
		m->pt_mem = NULL;
	}
	else
	{
# ifdef MMU040
		extern int page_ram_type;	/* mprot040.c */
		MEMREGION *pt;
		
		pt = NULL;
		if (page_ram_type & 2)
			pt = get_region (alt, page_table_size + 512, PROT_S);
		if (!pt && (page_ram_type & 1))
			pt = get_region (core, page_table_size + 512, PROT_S);
		
		/* For the 040, the page tables must be on 512 byte boundaries */
		m->page_table = pt ? ROUND512 (pt->loc) : NULL;
		m->pt_mem = pt;
# else
		void *pt;
		
		pt = kmalloc (page_table_size + 16);
		
		/* page tables must be on 16 byte boundaries, so we
		 * round off by 16 for that; however, we will want to
		 * kfree that memory at some point, so we squirrel
		 * away the original address for later use
		 */
		m->page_table = pt ? ROUND16 (pt) : NULL;
		m->pt_mem = pt;
# endif
		
		if (!pt) DEBUG(("init_page_table_ptr: no mem for page table"));
	}
# endif
}

static void
free_page_table_ptr (struct memspace *m)
{
# ifndef NO_MMU
	if (!no_mem_prot)
	{
# ifdef MMU040
		MEMREGION *pt;
		
		pt = m->pt_mem;
		m->pt_mem = NULL;
		
		pt->links--;
		if (!pt->links)
			free_region(pt);
# else
		void *pt;
		
		pt = m->pt_mem;
		m->pt_mem = NULL;
		
		kfree(pt);
# endif
	}
# endif
}

struct memspace *
copy_mem (struct proc *p)
{
	struct user_things *ut;
	struct memspace *m;
	void *ptr;
	int i;
	
	TRACE (("copy_mem: pid %i (%lx)", p->pid, p));
	assert (p && p->p_mem && p->p_mem->links > 0);
	
	m = kmalloc (sizeof (*m));
	if (!m)
	{
		DEBUG(("copy_mem: kmalloc failed -> NULL"));
		return NULL;
	}
	
	bcopy (p->p_mem, m, sizeof (*m));
	m->links = 1;
	/* don't copy over F_OS_SPECIAL flag */
	m->memflags &= ~F_OS_SPECIAL;
	
	init_page_table_ptr (m);
	
	ptr = kmalloc (m->num_reg * sizeof (void *) * 2);
	m->mem = ptr;
	m->addr = ptr + m->num_reg * sizeof (void *);
	
	/* Trampoline. Space is added for user cookie jar */

# ifdef JAR_PRIVATE
	/* When the jar is not global, it is more than doubtful,
	 * that anybody would need so many cookies.
	 */
# define PRIV_JAR_SLOTS	64
# define PRIV_JAR_SIZE	(PRIV_JAR_SLOTS * sizeof(struct cookie))

# else

# define PRIV_JAR_SLOTS	0
# define PRIV_JAR_SIZE	0

# endif

	m->tp_reg = get_region(alt, user_things.len + PRIV_JAR_SIZE, PROT_P);
	if (!m->tp_reg)
		m->tp_reg = get_region(core, user_things.len + PRIV_JAR_SIZE, PROT_P);
	
	if ((!no_mem_prot && !m->pt_mem) || !ptr || !m->tp_reg)
		goto nomem;
	
	/* copy memory */
	for (i = 0; i < m->num_reg; i++)
	{
		m->mem[i] = p->p_mem->mem[i];
		if (m->mem[i])
			m->mem[i]->links++;
		
		m->addr[i] = p->p_mem->addr[i];
	}
	
	/* initialize trampoline things */
	ut = m->tp_ptr = (struct user_things *) m->tp_reg->loc;

	/* temporary attach to curproc so it's accessible */
	attach_region(curproc, m->tp_reg);

	/* trampoline is inherited from the kernel */
	bcopy(&user_things, m->tp_ptr, user_things.len);

	/* Initialize user vectors */
	ut->terminateme_p += (long)ut;

	ut->sig_return_p += (long)ut;
	ut->pc_valid_return_p += (long)ut;

	ut->slb_init_and_exit_p += (long)ut;
	ut->slb_open_p += (long)ut;
	ut->slb_close_p += (long)ut;
	ut->slb_close_and_pterm_p += (long)ut;
	ut->slb_exec_p += (long)ut;

	ut->user_xhdi_p += (long)ut;

# ifdef JAR_PRIVATE
	/* Cookie Jar is appended at the end of the trampoline */
	ut->user_jar_p = (struct cookie *)((long)ut + user_things.len);

	/* Copy the cookies over */
	{
		struct user_things *ct;
		struct cookie *ctj, *utj;

		/* The child inherits the jar from the parent
		 */
		ct = p->p_mem->tp_ptr;

		utj = ut->user_jar_p;
		ctj = ct->user_jar_p;

		for (i = 0; ctj->tag && i < PRIV_JAR_SLOTS-1; i++)
		{
			utj->tag = ctj->tag;

			/* Setup private XHDI cookie for new process */
			if (ctj->tag == COOKIE_XHDI)
				utj->value = ut->user_xhdi_p;
			else
				utj->value = ctj->value;

			utj++;
			ctj++;
		}

		/* User jars have less slots, than the kernel one (1024) */
		utj->tag = 0;
		utj->value = PRIV_JAR_SLOTS;
	}

# endif

	cpush(ut, sizeof(*ut));
	detach_region(curproc, m->tp_reg);
	
	TRACE (("copy_mem: ok (%lx)", m));
	return m;
	
nomem:
	if (m->pt_mem) free_page_table_ptr (m);
	if (ptr) kfree (ptr);
	if (m->tp_reg) { m->tp_reg->links--; free_region(m->tp_reg); }
	kfree (m);
	
	return NULL;
}

void
free_mem (struct proc *p)
{
	struct memspace *p_mem;
	void *ptr;
	int i;
	
	assert (p && p->p_mem && p->p_mem->links > 0);
	p_mem = p->p_mem;
	// XXX p->p_mem = NULL; --- dependency in arch/mprot0?0.c
	
	if (--p_mem->links > 0)
		return;
	
	DEBUG (("freeing p_mem for %s", p->name));
	
	if (p_mem->tp_reg)
	{
		p_mem->tp_reg->links--;
		if (p_mem->tp_reg->links == 0)
			free_region(p_mem->tp_reg);
		else
			ALERT("p_mem->tp_reg->links != 0 ???");
	}

	/* free all memory
	 * if mflags & M_KEEP then attach it to process 0
	 */
	for (i = p_mem->num_reg - 1; i >= 0; i--)
	{
		MEMREGION *m = p_mem->mem[i];
		
		p_mem->mem[i] = NULL;
		p_mem->addr[i] = 0;
		
		if (m)
		{
			/* don't free specially allocated memory */
			if ((m->mflags & M_KEEP) && (m->links <= 1))
				if (p != rootproc)
					attach_region (rootproc, m);
			
			m->links--;
			if (m->links == 0)
				free_region (m);
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
	
	ptr = p_mem->mem;
	
	p_mem->mem = NULL;
	p_mem->addr = NULL;
	p_mem->num_reg = 0;
	
	kfree (ptr);
	
	/* invalidate memory space for proc before freeing it
	 * -> so the MMU code don't access it
	 */
	p->p_mem = NULL;
	
	free_page_table_ptr (p_mem);
	kfree (p_mem);
}

long
increase_mem(struct memspace *mem)
{
	unsigned long size;
	void *ptr;

	size = (mem->num_reg + NUM_REGIONS) * sizeof(void *);
	ptr = kmalloc(size * 2);

	if (ptr)
	{
		MEMREGION **newmem = ptr;
		long *newaddr = ptr + size;
		int i;

		/* copy over the old address mapping */
		for (i = 0; i < mem->num_reg; i++)
		{
			newmem[i] = mem->mem[i];
			newaddr[i] = mem->addr[i];

			if (newmem[i] == NULL)
				assert(newaddr[i] == 0);
		}

		/* initialize the rest of the tables */
		for(; i < mem->num_reg + NUM_REGIONS; i++)
		{
			newmem[i] = NULL;
			newaddr[i] = 0;
		}

		/* free the old tables (carefully for memory protection!) */
		ptr = mem->mem;

		mem->mem = newmem;
		mem->addr = newaddr;
		mem->num_reg += NUM_REGIONS;

		kfree(ptr);

		return 0;
	}
	return ENOMEM;
}

/* p_fd */

struct filedesc *
copy_fd (struct proc *p)
{
	struct filedesc *org_fd;
	struct filedesc *fd;
	long i;
	
	TRACE (("copy_fd: pid %i (%lx)", p->pid, p));
	assert (p && p->p_fd && p->p_fd->links > 0);
	org_fd = p->p_fd;
	
	fd = kmalloc (sizeof (*fd));
	if (!fd)
	{
		DEBUG(("copy_fd: kmalloc failed -> NULL"));
		return NULL;
	}
	
	bcopy (org_fd, fd, sizeof (*fd));
	fd->links = 1;
	
# if 0
	if (!(fd->lastfile < NDFILE))
	{
		i = fd->nfiles;
		while (i >= 2 * NDEXTENT && i > fd->lastfile * 2)
			i /= 2;
		
		fd->ofiles = kmalloc (i * OFILESIZE);
		fd->ofileflags = (char *) &fd->ofiles[i];
	}
	else
# endif
	{
		fd->ofiles = &fd->dfiles[0];
		fd->ofileflags = &fd->dfileflags[0];
		i = NDFILE;
	}
	
	fd->nfiles = i;
	
	//bcopy (org_fd->ofiles, fd->ofiles, i * sizeof (FILEPTR **));
	//bcopy (org_fd->ofileflags, fd->ofileflags, i * sizeof (char));
	
	for (i = MIN_HANDLE; i < fd->nfiles; i++)
	{
		FILEPTR *f = fd->ofiles[i];
		
		if (f)
		{
			if ((f == (FILEPTR *) 1L) || (f->flags & O_NOINHERIT))
				/* oops, we didn't really want to copy this
				 * handle
				 */
				fd->ofiles[i] = NULL;
			else
				f->links++;
		}
	}
	
	/* clear directory search info */
	bzero (fd->srchdta, NUM_SEARCH * sizeof (DTABUF *));
	bzero (fd->srchdir, sizeof (fd->srchdir));
	fd->searches = NULL;
	
	TRACE (("copy_fd: ok (%lx)", fd));
	return fd;
}

void
free_fd (struct proc *p)
{
	struct filedesc *p_fd;
	FILEPTR *f;
	int i;
	
	assert (p && p->p_fd && p->p_fd->links > 0);
	p_fd = p->p_fd;
	p->p_fd = NULL;
	
	if (--p_fd->links > 0)
		return;
	
	DEBUG (("freeing p_fd for %s", p->name));
	
	/* release the controlling terminal,
	 * if we're the last member of this pgroup
	 */
	f = p_fd->control;
	if (f && is_terminal (f))
	{
		struct tty *tty = (struct tty *) f->devinfo;
		int pgrp = p->pgrp;
		
		if (pgrp == tty->pgrp)
		{
			FILEPTR *pfp;
			PROC *p1;
			
			if (tty->use_cnt > 1)
			{
				for (p1 = proclist; p1; p1 = p1->gl_next)
				{
					if (p1->wait_q == ZOMBIE_Q || p1->wait_q == TSR_Q)
						continue;
					
					if (p1->pgrp == pgrp
						&& p1 != p
						&& ((pfp = p1->p_fd->control) != NULL)
						&& pfp->fc.index == f->fc.index
						&& pfp->fc.dev == f->fc.dev)
					{
						goto found;
					}
				}
			}
			else
			{
				for (p1 = proclist; p1; p1 = p1->gl_next)
				{
					if (p1->wait_q == ZOMBIE_Q || p1->wait_q == TSR_Q)
						continue;
					
					if (p1->pgrp == pgrp
						&& p1 != p
						&& p1->p_fd->control == f)
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
	
	/* close all files */
	for (i = MIN_HANDLE; i < p_fd->nfiles; i++)
	{
		f = p_fd->ofiles[i];
		
		if (f)
		{
			p_fd->ofiles[i] = NULL;
			do_close (p, f);
		}
	}
	
	/* close any unresolved Fsfirst/Fsnext directory searches */
	for (i = 0; i < NUM_SEARCH; i++)
	{
		if (p_fd->srchdta[i])
		{
			DIR *dirh = &p_fd->srchdir[i];
			xfs_closedir (dirh->fc.fs, dirh);
			release_cookie (&dirh->fc);
			dirh->fc.fs = 0;
		}
	}
	
	/* close pending opendir/readdir searches */
	{
		register DIR *dirh = p_fd->searches;
		
		while (dirh)
		{
			register DIR *nexth = dirh->next;
			
			if (dirh->fc.fs)
			{
				xfs_closedir (dirh->fc.fs, dirh);
				release_cookie (&dirh->fc);
			}
			
			kfree (dirh);
			dirh = nexth;
		}
	}
	
	if (p_fd->nfiles > NDFILE)
		kfree (p_fd->ofiles);
	
	kfree (p_fd);
}

/* p_cwd */

struct cwd *
copy_cwd (struct proc *p)
{
	struct cwd *org_cwd;
	struct cwd *cwd;
	int i;
	
	TRACE (("copy_cwd: pid %i (%lx)", p->pid, p));
	assert (p && p->p_cwd && p->p_cwd->links > 0);
	org_cwd = p->p_cwd;
	
	cwd = kmalloc (sizeof (*cwd));
	if (!cwd)
	{
		DEBUG(("copy_cwd: kmalloc failed -> NULL"));
		return NULL;
	}
	
	bzero (cwd, sizeof (*cwd));
	
	cwd->links = 1;
	cwd->cmask =  org_cwd->cmask;
	
	if (cwd->root_dir)
	{
		cwd->root_dir = kmalloc (strlen (cwd->root_dir) + 1);
		if (!cwd->root_dir)
		{
			kfree (cwd);
			return NULL;
		}
		
		strcpy (cwd->root_dir, org_cwd->root_dir);
		dup_cookie (&cwd->rootdir, &org_cwd->rootdir);
	}
	
	cwd->curdrv = org_cwd->curdrv;
	
	/* copy root and current directories */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		dup_cookie (&cwd->root[i], &org_cwd->root[i]);
		dup_cookie (&cwd->curdir[i], &org_cwd->curdir[i]);
	}
	
	TRACE (("copy_cwd: ok (%lx)", cwd));
	return cwd;
}

void
free_cwd (struct proc *p)
{
	struct cwd *p_cwd;
	int i;
	
	assert (p && p->p_cwd && p->p_cwd->links > 0);
	p_cwd = p->p_cwd;
	p->p_cwd = NULL;
	
	if (--p_cwd->links > 0)
		return;
	
	DEBUG (("freeing p_cwd for %s", p->name));
	
	/* release the directory cookies held by the process */
	for (i = 0; i < NUM_DRIVES; i++)
	{
		release_cookie (&p_cwd->curdir[i]);
		p_cwd->curdir[i].fs = NULL;
		release_cookie (&p_cwd->root[i]);
		p_cwd->root[i].fs = NULL;
	}
	
	if (p_cwd->root_dir)
	{
		DEBUG (("free root_dir = %s", p_cwd->root_dir));
		
		release_cookie (&p_cwd->rootdir);
		p_cwd->rootdir.fs = NULL;
		kfree (p_cwd->root_dir);
		p_cwd->root_dir = NULL;
	}
	
	kfree (p_cwd);
}

/* p_sigacts */

struct sigacts *
copy_sigacts (struct proc *p)
{
	struct sigacts *p_sigacts;
	
	TRACE (("copy_sigacts: pid %i (%lx)", p->pid, p));
	assert (p && p->p_sigacts && p->p_sigacts->links > 0);
	
	p_sigacts = kmalloc (sizeof (*p_sigacts));
	if (!p_sigacts)
	{
		DEBUG(("copy_sigacts: kmalloc failed -> NULL"));
		return NULL;
	}
	
	bcopy (p->p_sigacts, p_sigacts, sizeof (*p_sigacts));
	p_sigacts->links = 1;
	
	TRACE (("copy_sigacts: ok (%lx)", p_sigacts));
	return p_sigacts;
}

void
free_sigacts (struct proc *p)
{
	struct sigacts *p_sigacts;
	
	assert (p && p->p_sigacts && p->p_sigacts->links > 0);
	p_sigacts = p->p_sigacts;
	p->p_sigacts = NULL;
	
	if (--p_sigacts->links > 0)
		return;
	
	DEBUG (("freeing p_sigacts for %s", p->name));
	
	kfree (p_sigacts);
}

/* p_limits */

#if 0
struct plimit *
copy_limits (struct proc *p)
{
	struct plimit *n;
	
	TRACE (("copy_limit: %lx links %li", p_limit, p_limit->links));
	assert (p_limit->links > 0);
	
	if (p_limit->links == 1)
		return p_limit;
	
	n = kmalloc (sizeof (*n));
	if (n)
	{
		/* copy */
		memcpy (n->limits, p_limit->limits, sizeof (n->limits));
		
		/* reset flags */
		n->flags = 0;
		
		/* adjust link counters */
		p_limit->links--;
		n->links = 1;
	}
		DEBUG(("copy_limit: kmalloc failed -> NULL"));
	
	return n;
}
#endif

void
free_limits (struct proc *p)
{
#if 0
	assert (p_limit->links > 0);
	
	if (--p_limit->links == 0)
		kfree (p_limit);
#endif
}

/* p_ext */

struct p_ext *
share_ext(struct proc *p)
{
	struct p_ext *p_ext;

	if (!p->p_ext)
		return NULL;

	p_ext = kmalloc(sizeof(*p_ext));
	if (p_ext)
	{
		int i;

		p_ext->size = p->p_ext->size;
		p_ext->used = p->p_ext->used;

		for (i = 0; i < p_ext->size; i++)
		{
			if (i < p_ext->used)
			{
				struct proc_ext *ext;

				ext = p->p_ext->ext[i];
				assert(ext);

				ext->links++;
				p_ext->ext[i] = ext;

				/* share callback */
				if (ext->cb_vector && ext->cb_vector->share)
					(*ext->cb_vector->share)(ext->data);
			}
			else
				p_ext->ext[i] = NULL;
		}
	}

	return p_ext;
}

void
free_ext(struct proc *p)
{
	struct p_ext *p_ext;
	int i;

	if (!p->p_ext)
		return;

	p_ext = p->p_ext;
	p->p_ext = NULL;

	for (i = 0; i < p_ext->used; i++)
	{
		struct proc_ext *ext = p_ext->ext[i];

		if (--ext->links <= 0)
		{
			/* release callback */
			if (ext->cb_vector && ext->cb_vector->release)
				(*ext->cb_vector->release)(ext->data);

			kfree(ext->data);
			kfree(ext);
		}
	}

	kfree(p_ext);
}

void * _cdecl
proc_lookup_extension(struct proc *p, long ident)
{
	struct p_ext *p_ext;

	if (!p) p = curproc;

	p_ext = p->p_ext;
	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			if (p_ext->ext[i]->ident == ident)
				return p_ext->ext[i]->data;
		}
	}

	/* not found */
	return NULL;
}

void * _cdecl
proc_attach_extension(struct proc *p, long ident, unsigned long size, struct module_callback *cb)
{
	struct p_ext *p_ext;
	struct proc_ext *ext;

	if (!p) p = curproc;

	assert(size);

	p_ext = p->p_ext;
	if (!p_ext)
	{
		p_ext = kmalloc(sizeof(*p_ext));
		if (!p_ext)
		{
			DEBUG(("proc_attach_extension: out of memory"));
			return NULL;
		}

		/* clean memory */
		bzero(p_ext, sizeof(*p_ext));

		/* initialize */
		p_ext->size = sizeof(p_ext->ext) / sizeof(p_ext->ext[0]);
		DEBUG(("proc_attach_extension: free slots %i", p_ext->size));

		/* remember */
		p->p_ext = p_ext;
	}

	if (p_ext->used >= p_ext->size)
	{
		DEBUG(("proc_attach_extension: no free extension slots"));
		return NULL;
	}

	ext = kmalloc(sizeof(*ext));
	if (ext)
	{
		bzero(ext, sizeof(*ext));

		/* initialize data */
		ext->ident = ident;
		ext->links = 1;
		ext->cb_vector = cb;

		/* allocate data area */
		ext->data = kmalloc(size);
		if (ext->data)
		{
			/* clean data area */
			bzero(ext->data, size);

			/* remember */
			p_ext->ext[p_ext->used++] = ext;

			/* return success */
			return ext->data;
		}
		else
			kfree(ext);
	}

	if (!p_ext->used)
	{
		p->p_ext = NULL;
		kfree(p_ext);
	}

	DEBUG(("proc_attach_extension: out of memory"));
	return NULL;
}

void _cdecl
proc_detach_extension(struct proc *p, long ident)
{
	struct p_ext *p_ext;

	if (!p) p = curproc;

	p_ext = p->p_ext;
	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			struct proc_ext *ext = p_ext->ext[i];

			if (ext->ident == ident)
			{
				/* if no longer in use free up memory */
				if (--ext->links <= 0)
				{
					/* call release callback */
					if (ext->cb_vector && ext->cb_vector->release)
						(*ext->cb_vector->release)(ext->data);

					kfree(ext->data);
					kfree(ext);
				}

				p_ext->used--;
				break;
			}
		}

		for ( ; i < p_ext->used; i++)
			p_ext->ext[i] = p_ext->ext[i+1];

		if (!p_ext->used)
		{
			p->p_ext = NULL;
			kfree(p_ext);
		}
	}
}

void
proc_ext_on_exit(struct proc *p, int code)
{
	struct p_ext *p_ext = p->p_ext;

	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			struct proc_ext *ext = p_ext->ext[i];

			if (ext->cb_vector && ext->cb_vector->on_exit)
				(*ext->cb_vector->on_exit)(ext->data, p, code);
		}
	}
}

void
proc_ext_on_exec(struct proc *p)
{
	struct p_ext *p_ext = p->p_ext;

	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			struct proc_ext *ext = p_ext->ext[i];

			if (ext->cb_vector && ext->cb_vector->on_exec)
				(*ext->cb_vector->on_exec)(ext->data, p);
		}
	}
}

void
proc_ext_on_fork(struct proc *p, long flags, struct proc *child)
{
	struct p_ext *p_ext = p->p_ext;

	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			struct proc_ext *ext = p_ext->ext[i];

			if (ext->cb_vector && ext->cb_vector->on_fork)
				(*ext->cb_vector->on_fork)(ext->data, p, flags, child);
		}
	}
}

void
proc_ext_on_stop(struct proc *p, unsigned short nr)
{
	struct p_ext *p_ext = p->p_ext;

	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			struct proc_ext *ext = p_ext->ext[i];

			if (ext->cb_vector && ext->cb_vector->on_stop)
				(*ext->cb_vector->on_stop)(ext->data, p, nr);
		}
	}
}

void
proc_ext_on_signal(struct proc *p, unsigned short nr)
{
	struct p_ext *p_ext = p->p_ext;

	if (p_ext)
	{
		int i;

		for (i = 0; i < p_ext->used; i++)
		{
			struct proc_ext *ext = p_ext->ext[i];

			if (ext->cb_vector && ext->cb_vector->on_signal)
				(*ext->cb_vector->on_signal)(ext->data, p, nr);
		}
	}
}
