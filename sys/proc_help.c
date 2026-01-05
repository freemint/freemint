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

# include "proc_help.h"

# include "mint/asm.h"

# include "arch/cpu.h"
# include "arch/mprot.h"
# include "arch/user_things.h"

# include "libkern/libkern.h"
# include "mint/file.h"
# include "mint/proc_extensions.h"

# include "filesys.h"
# include "k_fds.h"
# include "kmemory.h"
# include "memory.h"

# include "proc.h"


/* p_mem */

void
init_page_table_ptr (struct memspace *m)
{
# ifndef WITH_MMU_SUPPORT
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
		MEMREGION *pt = NULL;
# if defined (M68040) || defined (M68060)
		
// 		FORCE("init_page_table_ptr: p_mem->mem = %lx", curproc->p_mem->mem);
		if (page_ram_type & 2)
			pt = get_region (alt, page_table_size + 512, PROT_S);
		if (!pt && (page_ram_type & 1))
			pt = get_region (core, page_table_size + 512, PROT_S);
		
		/* For the 040, the page tables must be on 512 byte boundaries */
		m->page_table = pt ? ROUND512 (pt->loc) : NULL;
		m->pt_mem = pt;
# else /* M68040 || M68060 */

		if (tt_mbytes)
			pt = get_region (alt, page_table_size + 16, PROT_S);
		if (!pt && !tt_mbytes)
			pt = get_region (core, page_table_size + 16, PROT_S);

		/* page tables must be on 16 byte boundaries, so we
		 * round off by 16 for that; however, we will want to
		 * free that memory at some point, so we squirrel
		 * away the original address for later use
		 */
		m->page_table = pt ? ROUND16 (pt->loc) : NULL;
		m->pt_mem = pt;
# endif /* M68040 || M68060 */
		
		if (!pt) DEBUG(("init_page_table_ptr: no mem for page table"));
	}
# endif
}

static void
free_page_table_ptr (struct memspace *m)
{
# ifdef WITH_MMU_SUPPORT
	if (!no_mem_prot)
	{
# if defined(M68030) || defined(M68040) || defined(M68060)
		MEMREGION *pt;
		
		pt = m->pt_mem;
		m->pt_mem = NULL;
		
		pt->links--;
		if (!pt->links)
			free_region(pt);
# endif
	}
# endif
}

struct memspace *
copy_mem (struct proc *p)
{
	struct user_things *ut;
	struct memspace *m;
	union { char *c; void *v; } ptr;
	int i;
	
	TRACE (("copy_mem: pid %i (%p)", p->pid, p));
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
	
	ptr.v = kmalloc (m->num_reg * sizeof (void *) * 2);
	m->mem = ptr.v;
	m->addr = (void *)(ptr.c + m->num_reg * sizeof (char *));
	
	m->tp_reg = get_region(alt, user_things.len, PROT_P);
	if (!m->tp_reg)
		m->tp_reg = get_region(core, user_things.len, PROT_P);
	
	TRACE(("copy_mem: ptr=%p, m->pt_mem = %p, m->tp_reg = %p, mp=%s", ptr.v, m->pt_mem, m->tp_reg,
#ifdef WITH_MMU_SUPPORT
		no_mem_prot
#else
		true
#endif
		? "off":"on"));

#ifndef WITH_MMU_SUPPORT
	if (!ptr.c || !m->tp_reg)
#else
	if ((!no_mem_prot && !m->pt_mem) || !ptr.c || !m->tp_reg)
#endif
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
	attach_region(get_curproc(), m->tp_reg);

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

#ifndef M68000
	cpushi(ut, sizeof(*ut));
#endif
	detach_region(get_curproc(), m->tp_reg);
	
	TRACE (("copy_mem: ok (%p)", m));
	return m;
	
nomem:
	TRACE (("copy_mem: out of mem!"));
	if (m->pt_mem) free_page_table_ptr (m);
	if (ptr.c) kfree (ptr.c);
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
	union { char *c; void *v; } ptr;

	size = (mem->num_reg + NUM_REGIONS) * sizeof(void *);
	ptr.v = kmalloc(size * 2);

	if (ptr.v)
	{
		MEMREGION **newmem = ptr.v;
		unsigned long *newaddr = (void *)(ptr.c + size);
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
		for (; i < mem->num_reg + NUM_REGIONS; i++)
		{
			newmem[i] = NULL;
			newaddr[i] = 0;
		}

		/* free the old tables (carefully for memory protection!) */
		ptr.v = mem->mem;

		mem->mem = newmem;
		mem->addr = newaddr;
		mem->num_reg += NUM_REGIONS;

		kfree(ptr.c);

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
	
	TRACE (("copy_fd: pid %i (%p)", p->pid, p));
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
		fd->ofileflags = (unsigned char *)&fd->dfileflags[0];
		i = NDFILE;
	}
	
	fd->nfiles = i;
	
	//mint_bcopy (org_fd->ofiles, fd->ofiles, i * sizeof (FILEPTR **));
	//mint_bcopy (org_fd->ofileflags, fd->ofileflags, i * sizeof (char));
	
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
	mint_bzero (fd->srchdta, NUM_SEARCH * sizeof (DTABUF *));
	mint_bzero (fd->srchdir, sizeof (fd->srchdir));
	fd->searches = NULL;
	
	TRACE (("copy_fd: ok (%p)", fd));
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
			DEBUG(("free_fd: assigned tty->pgrp = %i", tty->pgrp));
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
	
	TRACE (("copy_cwd: pid %i (%p)", p->pid, p));
	assert (p && p->p_cwd && p->p_cwd->links > 0);
	org_cwd = p->p_cwd;
	
	cwd = kmalloc (sizeof (*cwd));
	if (!cwd)
	{
		DEBUG(("copy_cwd: kmalloc failed -> NULL"));
		return NULL;
	}
	
	mint_bzero (cwd, sizeof (*cwd));
	
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
	
	TRACE (("copy_cwd: ok (%p)", cwd));
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
	
	TRACE (("copy_sigacts: pid %i (%p)", p->pid, p));
	assert (p && p->p_sigacts && p->p_sigacts->links > 0);
	
	p_sigacts = kmalloc (sizeof (*p_sigacts));
	if (!p_sigacts)
	{
		DEBUG(("copy_sigacts: kmalloc failed -> NULL"));
		return NULL;
	}
	
	bcopy (p->p_sigacts, p_sigacts, sizeof (*p_sigacts));
	p_sigacts->links = 1;
	
	TRACE (("copy_sigacts: ok (%p)", p_sigacts));
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

static struct proc_ext *
new_proc_ext(long ident, unsigned long flags, unsigned long size, struct module_callback *cb)
{
	struct proc_ext *e;
	
	e = kmalloc(sizeof(*e));
	
	if (e)
	{
		mint_bzero(e, sizeof(*e));
		
		if ((e->data = kmalloc(size)))
		{
			/* Initialize data */
			e->ident = ident;
			e->flags = flags;
			e->size = size;
			e->links = 1;
			e->cb_vector = cb;
			mint_bzero(e->data, size);
		}
		else
		{
			kfree(e);
			e = NULL;
		}
	}
	return e;
}

/* p_ext */

struct p_ext *
share_ext(struct proc *p, struct proc *p2)
{
	struct p_ext *p_ext = NULL, *p_ext2 = NULL;

	p2->p_ext = NULL;

	if (p->p_ext)
	{
		p_ext = kmalloc(sizeof(*p_ext));
		p_ext2 = kmalloc(sizeof(*p_ext2));

		if (p_ext && p_ext2)
		{
			int i;

			/* clean memory */
			mint_bzero(p_ext, sizeof(*p_ext));

			p_ext->size = p->p_ext->size;
			p_ext->used = 0;

			/* Ozk:
			 * When sharing extensions, we prepare/share/copy ALL extensions
			 * THEN we call the 'share' callback vector.
			 */
			/* Ozk:
			 * First we share all extensions
			 */
			for (i = 0; i < p->p_ext->used; i++)
			{
				struct proc_ext *ext, *ext2 = NULL;

				ext = p->p_ext->ext[i];
				assert(ext);
				if (!(ext->flags & PEXT_NOSHARE))
				{
					if ((ext->flags & PEXT_COPYONSHARE))
					{
						ext2 = new_proc_ext(ext->ident, ext->flags, ext->size, ext->cb_vector);
						assert(ext2);
						memcpy(ext2->data, ext->data, ext->size);
						p_ext->ext[p_ext->used++] = ext2;
						if ((ext2->flags & PEXT_SHAREONCE))
							ext2->flags |= PEXT_NOSHARE;
					}
					else
					{
						ext->links++;
						p_ext->ext[p_ext->used++] = ext;
					}						
				}
			}
			if (p_ext->used)
			{
				/* Ozk:
				 * Beyond here we cannot access the real p_ext structure, since
				 * */
				p2->p_ext = p_ext;

				memcpy(p_ext2, p_ext, sizeof(*p_ext));

				/* Ozk:
				 * Then we call 'share' vector for the extensions that really
				 * was shared.
				 */
				for (i = 0; i < p_ext2->used; i++)
				{
					struct proc_ext *ext;
				
					ext = p_ext2->ext[i];
					if (ext->cb_vector && ext->cb_vector->share)
						(*ext->cb_vector->share)(ext->data, p, p2);
				}
			}
			else
			{
				kfree(p_ext);
				p_ext = NULL;
			}
		}
		if (p_ext2)
			kfree(p_ext2);
	}
	return p_ext;
}

void
free_ext(struct proc *p)
{
	if (p->p_ext)
	{
		struct p_ext *p_ext = p->p_ext;
		int i;

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

		p->p_ext = NULL;
		kfree(p_ext);
	}
}

void * _cdecl
proc_lookup_extension(struct proc *p, long ident)
{
	if (!p) p = get_curproc();

	if (p->p_ext)
	{
		struct p_ext *p_ext = p->p_ext;
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

/* attach_extension()
 *
 * Attach a control block 'size' bytes large, whose identifier is 'ident',
 * to a process 'p'
 *
 * Currently defined flags;
 *  PEXT_NOSHARE
 *      The attachment will not be shared (and the 'share' callback is NOT taken)
 *      upon process forks or when a child process is created.
 *  PEXT_COPYONSHARE
 *      A new control block is allocated, and the content of the parents control-
 *      block is copied to the new.
 *  PEXT_SHAREONCE
 *      This will only work in conjunction with PEXT_COPYONSHARE. When set, will
 *      cause the PEXT_NOSHARE flag to be set on the copy, preventing it from
 *      being shared beyond the fist child.
 */
void * _cdecl
proc_attach_extension(struct proc *p, long ident, unsigned long flags, unsigned long size, struct module_callback *cb)
{
	struct p_ext *p_ext;
	struct proc_ext *ext;

	if (!p) p = get_curproc();

	assert(size);
	
	p_ext = p->p_ext;

	/* Ozk:
	 * Check if an extension with 'ident' id already exists, and return NULL
	 * to indicate failure if so.
	 */
	if (p_ext && p_ext->used) {
		int i;
		for (i = 0; i < p_ext->used; i++) {
			ext = p_ext->ext[i];
			if (ext->ident == ident) {
				ALERT("proc_attach_extension: Extension with ident %lx already attached to %s!", ident, p->name);
				return NULL;
			}
		}
	}

	if (!p_ext)
	{
		p_ext = kmalloc(sizeof(*p_ext));
		if (!p_ext)
		{
			DEBUG(("proc_attach_extension: out of memory"));
			return NULL;
		}

		/* clean memory */
		mint_bzero(p_ext, sizeof(*p_ext));

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
	
	ext = new_proc_ext(ident, flags, size, cb);
	
	if (ext)
	{
		p_ext->ext[p_ext->used++] = ext;
		return ext->data;
	}

	if (!p_ext->used)
	{
		p->p_ext = NULL;
		kfree(p_ext);
	}

	DEBUG(("proc_attach_extension: out of memory"));
	return NULL;
}

static void
detach_ext(struct proc *p, long ident)
{
	if (p->p_ext)
	{
		struct p_ext *p_ext = p->p_ext;
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

		if (i < p_ext->used)
			_mint_bcopy(&p_ext->ext[i+1], p_ext->ext[i], (p_ext->used - i - 1) * sizeof(struct proc_ext));

		if (!p_ext->used)
		{
			p->p_ext = NULL;
			kfree(p_ext);
		}
	}
}

void _cdecl
proc_detach_extension(struct proc *p, long ident)
{
	if (!p) p = get_curproc();

	/* Ozk: special action when p == -1L; Remove 'ident' attachment
	 *      from ALL processes currently running. This means that the
	 *      kernel module for which these attachments were installed
	 *      is about to exit, and does a detach_extension(-1L, ident)
	 *      to clean up. This is very important, so that the vectors
	 *      dont point into nowhere land after a module (like XaAES)
	 *      exits.
	 */
	if ((long)p == -1L)
	{
		for (p = proclist; p; p = p->gl_next)
			detach_ext(p, ident);
	}
	else
		detach_ext(p, ident);
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
			{
				(*ext->cb_vector->on_exit)(ext->data, p, code);
			}
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
