/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

/*
 * Copyright 1990,1991,1992 Eric R. Smith.
 * Copyright 1992,1993,1994 Atari Corporation.
 * All rights reserved.
 */
 
# include "memory.h"
# include "global.h"

# include "libkern/libkern.h"
# include "mint/basepage.h"
# include "mint/falcon.h"	/* FIXME: Include this in osbind.h and ostruct.h! */
# include "mint/signal.h"
# include "mint/slb.h"

# include "arch/cpu.h"		/* cpush */
# include "arch/mprot.h"
# include "arch/syscall.h"	/* lineA0 */

# include "bios.h"
# include "dosfile.h"
# include "dosmem.h"
# include "fasttext.h"	/* for line A stuff */
# include "filesys.h"
# include "init.h"	/* __printf(f) */
# include "kmemory.h"
# include "util.h"


/*
 * all functions in this module (!!)
 */

void		init_mem	(void);
void		init_core	(void);
void		init_swap	(void);
int		add_region	(MMAP map, ulong place, ulong size, unsigned mflags);

static long	core_malloc	(long, int);
static void	core_free	(long);
static void	terminateme	(int code);

# if 0
void		restr_screen	(void);
# endif

MEMREGION *	get_region	(MMAP map, ulong size, int mode);
MEMREGION *	_get_region	(MMAP map, ulong size, int mode, MEMREGION *descr, int kernel_flag);
void		free_region	(MEMREGION *reg);
long		shrink_region	(MEMREGION *reg, ulong newsize);

virtaddr	attach_region	(PROC *proc, MEMREGION *reg);
void		detach_region	(PROC *proc, MEMREGION *reg);

long		max_rsize	(MMAP map, long needed);
long		tot_rsize	(MMAP map, int flag);

virtaddr	alloc_region	(MMAP map, ulong size, int mode);

MEMREGION *	fork_region	(MEMREGION *reg, long txtsize);
MEMREGION *	create_env	(const char *env, ulong flags);
MEMREGION *	create_base	(const char *cmd, MEMREGION *env, ulong flags, ulong prgsize,
				 PROC *execproc, SHTEXT *s, FILEPTR *f, FILEHEAD *fh, XATTR *xp, long *err);
MEMREGION *	load_region	(const char *name, MEMREGION *env, const char *cmdlin, XATTR *x,
				 MEMREGION **text, long *fp, int isexec, long *err);
SHTEXT *	get_text_seg	(FILEPTR *f, FILEHEAD *fh, XATTR *xp, SHTEXT *s, int noalloc, long *err);
MEMREGION *	find_text_seg	(FILEPTR *f);
long		load_and_reloc	(FILEPTR *f, FILEHEAD *fh, char *where, long start,
				 long nbytes, BASEPAGE *base);
void		rts		(void);
PROC *		exec_region	(PROC *p, MEMREGION *mem, int thread);
long		memused		(PROC *p);
void		recalc_maxmem	(PROC *p);

int		valid_address	(long addr);
MEMREGION *	addr2mem	(PROC *p, virtaddr a);
MEMREGION *	addr2region	(long addr);
MEMREGION *	proc_addr2region(PROC *p, long addr);

# ifdef DEBUG_INFO
void		DUMP_ALL_MEM	(void);
void		DUMPMEM		(MMAP map);
# endif

# if 1
# ifdef DEBUG_INFO
# define SANITY_CHECKING
# endif
# endif

# ifdef SANITY_CHECKING
# define SANITY_CHECK(map)	sanity_check (map, __LINE__)
static void	sanity_check	(MMAP map, ulong line);
# else
# define SANITY_CHECK(map)
# endif

long		change_prot_status (PROC *proc, long start, int newmode);

/* from realloc.c
 */
long		realloc_region	(MEMREGION *, long);
long		_cdecl s_realloc(long);


short forcefastload = 0;	/* for MINT.CNF keyword */
ulong initialmem = 4096;	/* dito */

/*
 * memory.c:: routines for managing memory regions
 */

/* macro for testing whether a memory region is free */
# define ISFREE(m) ((m)->links == 0)

/*
 * list of shared text regions currently being executed
 */
SHTEXT *text_reg = 0;

/*
 * initialize memory routines
 */

/* variable for debugging purposes; number of times we've needed
 * to get new regions
 */
int num_reg_requests = 0;

/* these variables are set in init_core(), and used in
 * init_mem()
 */
static ulong scrnsize, scrnplace;
static SCREEN *vscreen;

void
init_mem (void)
{
	MEMREGION *r;
	long newbase;
	
	/* initialize first kernel memory */
	init_kmemory ();
	
	/* */
	init_core();
	
# ifdef VM_EXTENSION
	/* Draco: fix for a bug in TOS 4.04 */
	if (core_malloc (-1, 1) == 0)
	{
		*(long *) 0x000005a8L = 0L;
	}
# endif
	
	/* initialize swap */
	init_swap();
	
	/* initialize MMU constants */
	init_tables();
	
	/* mark all unused regions in the core & alt lists as "invalid", used
	 * as "super".
	 */
	for (r = *core; r; r = r->next)
	{
		if (r->links)
			mark_region (r, PROT_S);
		else
			mark_region (r, PROT_I);
	}
	for (r = *alt; r; r = r->next)
	{
		if (r->links)
			mark_region (r, PROT_S);
		else
			mark_region (r, PROT_I);
	}
	
	/* make sure the screen is set up properly */
	newbase = s_realloc (scrnsize);
	
	/* if we did get a new screen, point the new screen
	 * at the right place after copying the data
	 * if possible, save the screen to another buffer,
	 * since if the new screen and old screen overlap
	 * the blit will look very ugly.
	 * Note that if the screen isn't moveable, then we set
	 * scrnsize to a ridiculously large value, and so the
	 * s_realloc above failed.
	 */
	if (newbase)
	{
		/* find a free region for temp storage */
		for (r = *core; r; r = r->next)
		{
			if (ISFREE(r) && r->len >= scrnsize)
				break;
		}
		
		if (r)
		{
			quickmove((char *)r->loc, (char *)scrnplace, scrnsize);
			Setscreen((void *)r->loc, (void *)r->loc, -1);
			Vsync();
			quickmove((char *)newbase, (char *)r->loc, scrnsize);
		}
		else
		{
			quickmove((char *)newbase, (char *)scrnplace, scrnsize);
		}
		
		Setscreen((void *)newbase, (void *)newbase, -1);
		boot_print ("\r\n"); 
	}
}

/* The function below is not used anywhere anymore */

# if 0
void
restr_screen(void)
{
	long base = (long) Physbase ();
	MEMREGION *r;
	
	if (base != scrnplace)
	{
		for (r = *core; r; r = r->next)
		{
			if (ISFREE (r) && r->len >= scrnsize)
				break;
		}
		
		if (r)
		{
			quickmove ((char *) r->loc, (char *) base, scrnsize);
			Setscreen ((void *) r->loc, (void *) r->loc, -1);
			Vsync ();
			quickmove ((char *) scrnplace, (char *) r->loc, scrnsize);
		}
		else
		{
			quickmove ((char *) scrnplace, (char *) base, scrnsize);
		}
		
		Setscreen ((void *) scrnplace, (void *) scrnplace, -1);
		boot_print ("\r\n"); 
	}
}
# endif

/*
 * init_core(): initialize the core memory map (normal ST ram) and also
 * the alternate memory map (fast ram on the TT)
 */

static MEMREGION *_core_regions	= NULL;
static MEMREGION *_alt_regions	= NULL;

MMAP core = &_core_regions;
MMAP alt  = &_alt_regions;

/* note: add_region must adjust both the size and starting
 * address of the region being added so that memory is
 * always properly aligned
 * 
 * mflags - initial flags for region
 */

int
add_region (MMAP map, ulong place, ulong size, unsigned mflags)
{
  	MEMREGION *m;
	ulong trimsize;
	
	TRACELOW(("add_region(map=%lx,place=%lx,size=%lx,flags=%x)",
		map,place,size,mflags));
	
	m = kmr_get ();
	if (!m)
		return 0;	/* failure */
	
	bzero (m, sizeof (*m));
	
	if (place & MASKBITS)
	{
		/* increase place & shorten size by the amount we're trimming */
		trimsize = (MASKBITS + 1) - (place & MASKBITS);
		if (size <= trimsize)
			goto lose;
		
		size -= trimsize;
		place += trimsize;
	}
	
	/* now trim size DOWN to a multiple of pages */
	if (size & MASKBITS)
		size &= ~MASKBITS;
	
	/* only add if there's anything left */
	if (size)
	{
		m->len = size;
		m->loc = place;
		m->next = *map;
		m->mflags = mflags;
		*map = m;
	}
	else
	{
		/* succeed but don't do anything; dispose of region */
lose:
		kmr_free (m);
	}

	/* sucess */
	return 1;
}

static long
core_malloc (long amt, int mode)
{
	static int mxalloc = -1;	/* does GEMDOS know about Mxalloc? */
	long ret;
	
	if (mxalloc < 0)
	{
		ret = (long)Mxalloc(-1L, 0);
		if (ret == -32)
			mxalloc = 0;	/* unknown function */
		else
			if (ret >= 0)
				mxalloc = 1;
			else
			{
				ALERT("GEMDOS returned %ld from Mxalloc", ret);
				mxalloc = 0;
			}
	}

	if (mxalloc)
		return (long) Mxalloc(amt, mode);
	else
		if (mode == 1)
			return 0L;
		else
			return (long) Malloc(amt);
}

static void
core_free (long where)
{
	Mfree((void *)where);
}

# ifdef VM_EXTENSION
void	*pagesave;
void	*st_pagesave;
long	st_pagesize;
ulong	tablesize;
# endif

void
init_core (void)
{
	int scrndone = 0;
	ulong size;
	ulong place;
	ulong temp;
# ifdef OLDTOSFS
	void *tossave;
# endif
	
# ifdef VERBOSE_BOOT
	boot_print ("Initializing core memory:\r\n");
# endif
# ifdef OLDTOSFS	
	tossave = (void *)core_malloc((long)TOS_MEM, 0);
	if (!tossave)
	{
		FATAL("Not enough memory to run MiNT");
	}
# endif
# ifdef VM_EXTENSION
	if (vm_in_use)
	{
		/* allocate st-ram used for vm*/
		st_pagesize = core_malloc (-1L, 0) - st_left_after_vm;
		if (st_pagesize < 0)
		{
			st_pagesize = 0;
		}
		tablesize = 16+8*16*4+4*2*(64*256/2+8*1024 +
			(((ulong) core_malloc(-1L, 1)) + st_pagesize)/4096);
		pagesave = (void *) core_malloc (tablesize + st_pagesize, 0);
		if (!pagesave)
		{
			FATAL ("Not enough memory to run MiNT: could not run VM!");
			vm_in_use=0;
		}
		else
		{
			st_pagesave = ((char *) pagesave) + tablesize;
			if (st_pagesize == 0)
			{
				st_pagesave = 0;
			}
		}
	}
# endif
	
	/*
	 * find out where the screen is. We want to manage the screen
	 * memory along with all the other memory, so that Srealloc()
	 * can be used by the XBIOS to allocate screens from the
	 * end of memory -- this avoids fragmentation problems when
	 * changing resolutions.
	 *
	 * Note, however, that some graphics boards (e.g. Matrix)
	 * are unable to change the screen address. We fake out the
	 * rest of our code by pretending to have a really huge
	 * screen that can't be changed.
	 */
	scrnplace = (long) Physbase();
	
# if 1
	/* kludge: some broken graphics card drivers (notably, some versions of 
	 * NVDI's REDIRECT.PRG) return the base of the ST screen from Physbase(),
	 * not the base of the graphics card memory.  Try to get the real screen
	 * memory base from v_bas_ad (0x44e).
	 */
	if ((*(long *) 0x44eL) > scrnplace)
		scrnplace = (*(long *) 0x44eL);
# endif
	
	vscreen = (SCREEN *)((char *)lineA0() - 346);
	if (FalconVideo)
	{
		/* the Falcon can tell us the screen size */
		scrnsize = VgetSize(VsetMode(-1));
	}
	else
	{
		/* otherwise, use the line A variables */
		scrnsize = (vscreen->maxy+1)*(long)vscreen->linelen;
	}
	
	/* check for a graphics card with fixed screen location */
# define phys_top_st (*(ulong *) 0x42eL)
	
	if (scrnplace >= phys_top_st)
	{
		/* screen isn't in ST RAM */
		scrnsize = 0x7fffffffUL;
		scrndone = 1;
	}
	else
	{
		temp = (ulong)core_malloc(scrnsize+256L, 0);
		if (temp)
		{
			(void)Setscreen((void *)-1L, (void *)((temp+511)&(0xffffff00L)), -1);
			if ((long)Physbase() != ((temp+511)&(0xffffff00L)))
			{
				scrnsize = 0x7fffffffUL;
				scrndone = 1;
			}
			(void)Setscreen((void *)-1L, (void *)scrnplace, -1);
			core_free(temp);
		}
	}
	
	/* initialize ST RAM */
	size = (ulong) core_malloc(-1L, 0);
	
# ifdef VERBOSE_BOOT
	boot_printf ("%lu bytes of ST RAM, ", size);
# endif
	
       	while (size > 0)
	{
		place = (ulong) core_malloc (size, 0);
		if (!scrndone && (place + size == scrnplace))
		{
			size += scrnsize;
			scrndone = 1;
		}
		if (!add_region(core, place, size, M_CORE))
			FATAL("init_mem: unable to add a region");
		size = (ulong) core_malloc(-1L, 0);
	}
	
	if (!scrndone)
	{
		(void) add_region (core, scrnplace, scrnsize, M_CORE);
	}
	
	/* initialize alternate RAM */
# ifndef VM_EXTENSION
 	size = (ulong)core_malloc(-1L, 1);
	
# ifdef VERBOSE_BOOT
	if (size)
		boot_printf ("%lu bytes of Fast RAM found\r\n", size);
	else
		boot_print ("no Fast RAM found\r\n");
# endif /* VERBOSE_BOOT */
	
	while (size > 0)
	{
		place = (ulong)core_malloc(size, 1);
		if (!add_region(alt, place, size, M_ALT))
			FATAL("init_mem: unable to add a region");
		size = (ulong)core_malloc (-1L, 1);
	}
# else /* VM_EXTENSION */
	if (!vm_in_use)
	{
		size = (ulong) core_malloc (-1L, 1);

# ifdef VERBOSE_BOOT
		if (size)
			boot_printf ("%lu bytes of Fast RAM found\r\n", size);
		else
			boot_print ("no Fast RAM found\r\n");
# endif /* VERBOSE_BOOT */

	 	while (size > 0)
		{
			place = (ulong)core_malloc(size, 1);
			if (!add_region(alt, place, size, M_ALT))
				FATAL("init_mem: unable to add a region");
			size = (ulong)core_malloc(-1L, 1);
	 	}
	}
# endif /* VM_EXTENSION */
# ifdef OLDTOSFS	
	(void) Mfree (tossave); /* leave some memory for TOS to use */
# endif
}

/*
 * init_swap(): initialize the swap area;
 */

MEMREGION *_swap_regions = 0;
MMAP swap = &_swap_regions;

# ifdef VM_EXTENSION
extern void *Init_the_VM(ulong,ulong,ulong,ulong,ulong);     /* in vm5.spp */
# endif

void
init_swap (void)
{
# ifdef VM_EXTENSION
	ulong table_mem; /* ptr to the tablemem */
	ulong page_mem;  /* ptr to pagemem      */ 
	ulong page_size; /* length of pagemem   */
	struct
	{
		ulong start;	/* start adr of VM */
		ulong size;	/* size of VM      */
		ulong stop;	/* stop of VM      */
	} *VM_ptr;		/* VMmem struct    */
	
	/* allocate some TT-ram for me.. */
	/* take the largest block and add the rest... */
	
	if (vm_in_use)
	{
# if 1
		page_size = (ulong)core_malloc(-1L, 1);
		if (page_size > 0)
		{
			page_mem = (ulong)core_malloc(page_size, 1);
		}
		else
		{
			page_size=0;
			page_mem=0;
		}
# else
		page_size=0;
		page_mem=0;
# endif
		
		if (((page_size+st_pagesize)>0) &&((page_size+st_pagesize)<=0x02000000))
		{
			/* add the virtmem if we got the tablemem */
			if (table_mem=pagesave)
			{
				/* everything is ok run VM.. */
				
				if (!(VM_ptr=Init_the_VM(table_mem,page_mem,page_size,st_pagesave,st_pagesize)))
				{
					FATAL("init_swap: Could not init VM"); /* should never happen... */
				}
				else
				{
					if (VM_ptr->size<=0)
					{
						/* swap partition not found
						 * free page and table mem
						 */
						if (!add_region(core,table_mem,tablesize+st_pagesize,M_CORE))
							FATAL("init_swap: unable to add a region");    
						if (!add_region(alt, page_mem, page_size, M_ALT))
							FATAL("init_swap: unable to add a region");      
						vm_in_use=0;
					}
					else
					{
# ifdef VERBOSE_BOOT
						boot_printf ("%lu bytes of virtual memory.\r\n", VM_ptr->size);
# endif
						if (!add_region(alt, VM_ptr->start, VM_ptr->size, M_ALT))
							FATAL("init_swap: unable to add VM region");    
					}
				}
				/* originally the lowest VM is in mem
				 * the rest is on the partition
				 */
			}
			else
			{
				/* tt-ram too large
				 * free the alloced mem
				 * free page and table mem
				 */
				if (!add_region(core,table_mem,tablesize+st_pagesize,M_CORE))
					FATAL("init_swap: unable to add a region");    
				if (!add_region(alt, page_mem, page_size, M_ALT))
					FATAL("init_swap: unable to add a region");
				vm_in_use=0;
			}
		}
		else
		{
			vm_in_use=0;
		}
		
		/* add the rest */
		page_size = (ulong)core_malloc(-1L, 1);
		while (page_size > 0)
		{
   			page_mem = (ulong)core_malloc(page_size, 1);
			if (!add_region(alt, page_mem, page_size, M_ALT))
				FATAL("init_mem: unable to add a region");
			page_size = (ulong)core_malloc(-1L, 1);
		}
	}
# endif
	
# ifdef VERBOSE_BOOT
	boot_print ("\r\n");
# endif
}

/*
 * routines for allocating/deallocating memory regions
 */

# if 0
/* notused: see dosmem.c
 *
 * change_prot_status: change the status of a region to 'newmode'.  We're
 * given its starting address, not its region structure pointer, so we have
 * to find the region pointer; since this is illegal if proc doesn't own
 * the region, we know we'll find the region struct pointer in proc->mem.
 *
 * If the proc doesn't own it, you get EACCES.  There are no other errors.
 * God help you if newmode isn't legal!
 */

long
change_prot_status (PROC *proc, long start, int newmode)
{
	MEMREGION **mr;
	int i;
	
	/* return EACCES if you don't own the region in question */
	if (!proc->mem)
		return EACCES;
	
	for (mr = proc->mem, i = 0; i < proc->num_reg; i++, mr++)
	{
		if ((*mr)->loc == start)
			goto found;
	}
	
	return EACCES;
	
found:
	mark_region(*mr,newmode);
	return E_OK;
}
# endif

/*
 * virtaddr
 * attach_region(proc, reg): attach the region to the given process:
 * returns the address at which it was attached, or NULL if the process
 * cannot attach more regions. The region link count is incremented if
 * the attachment is successful.
 */

virtaddr
attach_region (PROC *p, MEMREGION *reg)
{
	MEMREGION **newmem;
	virtaddr *newaddr;
	int i;
	
	TRACELOW(("attach_region %lx (%s) len %lx to pid %d",
		reg->loc, ((reg->mflags & M_CORE) ? "core" : "alt"),
		reg->len, p->pid));
	
	if (!reg || !reg->loc)
	{
		ALERT ("attach_region: attaching a null region??");
		return 0;
	}
	
again:
	for (i = 0; i < p->num_reg; i++)
	{
		if (!p->mem[i])
		{
			assert (p->addr[i] == 0);
			
			p->mem[i] = reg;
			p->addr[i] = (virtaddr) reg->loc;
			
			reg->links++;
			mark_proc_region (p, reg, PROT_P);
			
			return p->addr[i];
		}
	}
	
	/* Hmmm, OK, we have to expand the process' memory table */
	TRACELOW(("Expanding process memory table"));
	i = p->num_reg + NUM_REGIONS;
	
	newmem = kmalloc (i * sizeof (MEMREGION *));
	newaddr = kmalloc (i * sizeof (virtaddr));
	
	if (newmem && newaddr)
	{
		/*
		 * We have to use temps while allocating and freeing mem
		 * and addr so the memory protection code won't walk this
		 * process' memory list in the middle.
		 */
		void *pmem, *paddr;
		
		/* copy over the old address mapping */
		for (i = 0; i < p->num_reg; i++)
		{
			newmem[i] = p->mem[i];
			newaddr[i] = p->addr[i];
			
			if (newmem[i] == 0)
				assert (newaddr[i] == 0);
		}
		
		/* initialize the rest of the tables */
		for(; i < p->num_reg + NUM_REGIONS; i++)
		{
			newmem[i] = 0;
			newaddr[i] = 0;
		}
		
		/* free the old tables (carefully! for memory protection) */
		pmem = p->mem;
		paddr = p->addr;
		
		p->mem = NULL;
		p->addr = NULL;
		
		kfree (pmem);
		kfree (paddr);
		
		p->mem = newmem;
		p->addr = newaddr;
		p->num_reg += NUM_REGIONS;
		
		/* this time we will succeed */
		goto again;
	}
	
	if (newmem) kfree (newmem);
	if (newaddr) kfree (newaddr);
	
	DEBUG(("attach_region: failed"));
	return 0;
}

/*
 * detach_region(proc, reg): remove region from the procedure's address
 * space. If no more processes reference the region, return it to the
 * system. Note that we search backwards, so that the most recent
 * attachment of memory gets detached!
 */

void
detach_region (PROC *p, MEMREGION *reg)
{
	int i;
	
	TRACELOW(("detach_region %lx len %lx from pid %d",
		reg->loc, reg->len, p->pid));
	
	if (!reg || !reg->loc)
	{
		ALERT ("detach_region: detaching a null region??");
		return;
	}
	
	for (i = p->num_reg - 1; i >= 0; i--)
	{
		if (p->mem[i] == reg)
		{
			p->mem[i] = 0;
			p->addr[i] = 0;
			
			reg->links--;
			if (reg->links == 0)
				free_region (reg);
			else
				/* cause curproc's table to be updated */
				mark_proc_region (p, reg, PROT_I);
			
			return;
		}
	}
	
	DEBUG(("detach_region: region not attached"));
}

/*
 * get_region(MMAP map, ulong size, int mode) -- allocate a new region of the
 * given size in the given memory map. if no region big enough is available,
 * return NULL, otherwise return a pointer to the region.
 * "mode" tells us about memory protection modes
 * 
 * the "links" field in the region is set to 1
 */

MEMREGION *
get_region (MMAP map, ulong size, int mode)
{
	MEMREGION *m = kmr_get ();
	MEMREGION *n;
	
	n = _get_region (map, size, mode, m, 0);
	if (!n && m)
		kmr_free (m);
	
	return n;
}

MEMREGION *
_get_region (MMAP map, ulong size, int mode, MEMREGION *m, int kernel_flag)
{
	MEMREGION *n, *s, *k = NULL;
	
	TRACE (("get_region (%s, %li (%lx), %x)",
		((map == core) ? "core" : ((map == alt) ? "alt" : "???")),
		size, size, mode));
	
	SANITY_CHECK (map);
	
	if (kernel_flag)
	{
		assert (m);
		
		/* for ST-RAM don't follow special kernel alloc strategy */
		if (map == core)
			kernel_flag = 0;
	}
	
	/* precautionary measures */
	if (!size)
	{
		DEBUG (("request for 0 bytes??"));
		size = 1;
	}
	
	size = ROUND (size);
	
	/* We come back and try again if we found and freed any unattached shared
	 * text regions.
	 */
retry:
	n = *map;
	s = NULL;
	
	while (n)
	{
		if (ISFREE (n))
		{
			if (n->len == size)
			{
				if (kernel_flag)
					k = n;
				else
				{
					if (m) kmr_free (m);
					goto win;
				}
			}
			else
			{
				if (n->len > size)
				{
					if (kernel_flag)
						k = n;
					else
					
					/* split a new region, 'm', which will
					 * contain the free bytes after n
					 */
					if (m)
					{
						bzero (m, sizeof (*m));
						
						m->mflags = n->mflags & M_MAP;
						m->next = n->next;
						n->next = m;
						
						m->loc = n->loc + size;
						m->len = n->len - size;
						n->len = size;
						
						assert (n->loc + n->len == m->loc);
						
						goto win;
					}
					else
					{
						DEBUG (("get_region: no regions left"));
						return NULL;
					}
				}
			}
		}
		else
		{
			/* If this is an unattached shared text region,
			 * leave it as a last resort
			 */
			if (n->links == 0xffff && (n->mflags & M_SHTEXT))
			{
				if (!s && n->len >= size)
				{
					s = n;
				}
			}
		}
		
		n = n->next;
	}
	
	if (kernel_flag && k)
	{
		if (k->len == size)
		{
			kmr_free (m);	
			
			n = k;
			goto win;
		}
		else
		{
			assert (k->len > size);
			
			bzero (m, sizeof (*m));
			
			m->mflags = k->mflags & M_MAP;
			m->next = k->next;
			k->next = m;
			
			k->len = k->len - size;
			m->len = size;
			m->loc = k->loc + k->len;
			
			n = m;
			goto win;
		}
	}
	
	/* Looks like we're out of free memory. Try freeing an unattached shared text
	 * region, and then try again to fill this request.
	 */
	if (s)
	{
		s->links = 0;
		free_region (s);
		goto retry;
	}
	
	TRACELOW (("get_region: no memory left in this map"));
	return NULL;
	
win:
	n->links++;
	
	mark_region (n, mode & PROT_PROTMODE);
	if (mode & M_KEEP)
		n->mflags |= M_KEEP;
	
	return n;
}

/*
 * free_region (MEMREGION *reg): free the indicated region. The map
 * in which the region is contained is given by reg->mflags.
 * the caller is responsible for making sure that the region
 * really should be freed, i.e. that reg->links == 0.
 *
 * special things to do:
 * if the region is a shared text region, we must close the
 * associated file descriptor
 * if the region has shadow regions, we delete the descriptor
 * and free one of the save regions
 */

void
free_region (MEMREGION *reg)
{
	MMAP map;
	MEMREGION *m, *shdw, *save;
	SHTEXT *s, **old;
	long txtsize;
	int prot_hold;
	
	if (!reg)
		return;
	
	assert (ISFREE (reg));
	
	/*
	 * Check for shadows being present. If the shadow ring is non-empty, free
	 * the save region of the first member from the shadow region (which is
	 * made active).
	 */
	assert (!(reg->mflags & M_FSAVED));
	
	shdw = reg->shadow;
	if (shdw)
	{
		TRACE(("Freeing region with shadows"));
		save = reg->save;
		if (!save)
		{
			TRACELOW(("Restoring region from save memory"));
			save = shdw->save;
			assert(save);
			
			/* If we get here, the kernel already has detached the region
			 * from the current process, so it can longer access this
			 * region. To copy the contents of the save region back into
			 * the "real" region, we must access memory protection to
			 * give us temporarly access to the region, again. This
			 * call should not fail, because the region still should be
			 * intact for the process owning the shadow region.
			 */
			prot_hold = prot_temp (reg->loc, reg->len, -1);
			assert (prot_hold < 0);
			
			txtsize = (long)save->save;
			if (!txtsize)
				quickmove((char *)reg->loc, (char *)save->loc,
					  reg->len);
			else
			{
				TRACELOW(("Restoring region with shared text section"));
				quickmove((char *)reg->loc,
					  (char *)save->loc, 256);
				quickmove((char *)reg->loc + (txtsize+256),
					  (char *)save->loc + 256,
					  reg->len - (txtsize+256));
			}
			shdw->save = 0;

			if (prot_hold != -1)
				prot_temp(reg->loc, reg->len, prot_hold);
		}
		reg->save = 0;
		
		while (shdw->shadow != reg)
			shdw = shdw->shadow;
		shdw->shadow = reg->shadow;
		if (shdw->shadow == shdw)
			shdw->shadow = 0;
		reg->shadow = 0;
		
		/* If the `next' pointer of the shadow region points to the freed
		 * region, we can simply unlink and dispose the region descriptor.
		 * Otherwise the we cut down the length of the region descriptor
		 * to 0 and leave if to `free_region' to get rid of the region
		 * descriptor.
		 */
		if (shdw->next == reg)
		{
			shdw->next = reg->next;
			kmr_free (reg);
		}
		else
		{
			reg->len = 0;
			free_region (reg);
		}
		
		/* Free the save region instead of the original region */
		save->mflags &= ~M_FSAVED;
		save->save = 0;
		save->links--;
		
		assert(ISFREE(save));
		reg = save;
	}
	
	if (reg->mflags & M_SHTEXT)
	{
		TRACE(("freeing shared text region"));
		old = &text_reg;
		for(;;)
		{
			s = *old;
			if (!s) break;
			if (s->text == reg)
			{
				if (s->f)
					do_close(s->f);
				*old = s->next;
				kfree(s);
				break;
			}
			old = &s->next;
		}
		if (!s)
		{
			 DEBUG (("No shared text entry for M_SHTEXT region??"));
		}
	}
	
	if (reg->mflags & M_CORE)
		map = core;
	else if (reg->mflags & M_ALT)
		map = alt;
	else
		FATAL ("free_region: region flags not valid (%x)", reg->mflags);
	
	reg->mflags &= M_MAP;
	
# if 0
	/* unhook any vectors pointing into this region */
	unlink_vectors (reg->loc, reg->loc + reg->len);
# endif
	
	/* BUG(?): should invalidate caches entries - a copyback cache could stuff
	 * things into freed memory.
	 *	cinv(reg->loc, reg->len);
	 */
	m = *map;
	assert (m);
	
	/* MEMPROT: invalidate */
	if (map == core || map == alt)
		mark_region (reg, PROT_I);
	
	if (m == reg)
		goto merge_after;
	
	/* merge previous region if it's free and contiguous with 'reg' */
	
	/* first, we find the region */
	while (m && m->next != reg)
		m = m->next;
	
	if (m == NULL)
		FATAL ("couldn't find region %lx: loc: %lx len: %ld",
			reg, reg->loc, reg->len);
	
	if (ISFREE (m) && ((m->loc + m->len) == reg->loc))
	{
		m->len += reg->len;
		assert(m->next == reg);
		m->next = reg->next;
		reg->next = NULL;
		kmr_free (reg);
		reg = m;
	}
	
merge_after:
	/* merge next region if it's free and contiguous with 'reg' */
	m = reg->next;
	if (m && ISFREE (m) && ((reg->loc + reg->len) == m->loc))
	{
		reg->len += m->len;
		reg->next = m->next;
		m->next = 0;
		kmr_free (m);
	}
	
	SANITY_CHECK (map);
}

/*
 * shrink_region (MEMREGION *reg, ulong newsize):
 *   shrink region 'reg', so that it is now 'newsize' bytes long.
 *   if 'newsize' is bigger than the region's current size, return ESBLOCK;
 *   otherwise return E_OK.
 */

long
shrink_region (MEMREGION *reg, ulong newsize)
{
	MEMREGION *n;
	ulong diff;


	newsize = ROUND(newsize);

	assert(reg->links > 0);

	if (!(reg->mflags & (M_CORE | M_ALT | M_KER)))
	{
		FATAL("shrink_region: bad region flags (%x)", reg->mflags);
	}

	/* shrinking to 0 is the same as freeing */
	if (newsize == 0)
	{
		detach_region(curproc, reg);
		return E_OK;
	}

	/* if new size is the same as old size, don't do anything */
	if (newsize == reg->len)
	{
		/* nothing to do */
		return E_OK;
	}

	if (newsize > reg->len)
	{
		/* growth failure */
		DEBUG(("shrink_region: request to make region bigger"));
		return ESBLOCK;
	}
	
	/* if there are any shadows on the region we cannot shrink the region, as
	 * this would effect the shadows, too.
	 */
	if (reg->shadow)
	{
		DEBUG(("shrink_region: region has shadows"));
		return EACCES;
	}
	
	/* OK, we're going to free (reg->len - newsize) bytes at the end of
	 * this block. If the block after us is already free, simply add the
	 * space to that block.
	 */
	n = reg->next;
	diff = reg->len - newsize;

	if (n && ISFREE(n) && reg->loc + reg->len == n->loc)
	{
		reg->len = newsize;
		n->loc -= diff;
		n->len += diff;
		/* MEMPROT: invalidate the second half
		 * (part of it is already invalid; that's OK)
		 */
		mark_region(n,PROT_I);

		return E_OK;
	}
	else
	{
		n = kmr_get ();
		if (!n)
		{
			DEBUG(("shrink_region: new_region failed"));
			return EINTERNAL;
		}
		reg->len = newsize;
		n->loc = reg->loc + newsize;
		n->len = diff;
		n->mflags = reg->mflags & M_MAP;
		n->next = reg->next;
		reg->next = n;
		
		/* MEMPROT: invalidate the new, free region */
		mark_region(n,PROT_I);
	}
	
	return E_OK;
}

/*
 * max_rsize(map, needed): return the length of the biggest free region
 * in the given memory map, or 0 if no regions remain.
 * needed is minimun amount needed, if != 0 try to keep unattached
 * shared text regions, else count them all as free.
 */

long
max_rsize (MMAP map, long needed)
{
	MEMREGION *m;
	long size = 0, lastsize = 0, end = 0;

	if (needed)
	{
		for (m = *map; m; m = m->next)
		{
			if (ISFREE(m) || (m->links == 0xfffe && !(m->mflags & M_SHTEXT) && !m->shadow))
			{
				if (end == m->loc)
				{
					lastsize += m->len;
				}
				else
				{
					lastsize = m->len;
				}
				end = m->loc + m->len;
				if (lastsize > size)
				{
					size = lastsize;
				}
			}
		}
		if (size >= needed)
			return size;

		lastsize = end = 0;
	}
	for (m = *map; m; m = m->next)
	{
		if (ISFREE(m) || (m->links == 0xfffe && !m->shadow) || (m->links == 0xffff && (m->mflags & M_SHTEXT)))
		{
			if (end == m->loc)
			{
				lastsize += m->len;
			}
			else
			{
				lastsize = m->len;
			}
			end = m->loc + m->len;
			if (lastsize > size)
			{
				if (needed && lastsize >= needed)
					return lastsize;
				size = lastsize;
			}
		}
	}
	
	return size;
}

/*
 * tot_rsize(map, flag): if flag == 1, return the total number of bytes in
 * the given memory map; if flag == 0, return only the number of free
 * bytes
 */

long
tot_rsize (MMAP map, int flag)
{
	MEMREGION *m;
	long size = 0;
	
	for (m = *map; m; m = m->next)
	{
		if (flag || ISFREE(m) || (m->links == 0xffff && (m->mflags & M_SHTEXT)))
		{
			size += m->len;
		}
	}
	
	return size;
}

long
freephysmem (void)
{
	long size;
	
	size = tot_rsize (core, 0);
	size += tot_rsize (alt, 0);
	
	return size;
}

/*
 * alloc_region(MMAP map, ulong size, int mode): allocate a new region and
 * attach it to the current process; returns the address at which the region
 * was attached, or NULL. The mode argument is the memory protection mode to
 * give to get_region, and in turn to mark_region.
 */

virtaddr
alloc_region (MMAP map, ulong size, int mode)
{
	MEMREGION *m;
	PROC *proc = curproc;
	virtaddr v;

	TRACELOW(("alloc_region(map,size: %lx,mode: %x)",size,mode));
	
	if (!size)
	{
	    DEBUG(("alloc_region of zero bytes?!"));
	    return 0;
	}

	m = get_region(map, size, mode);
	if (!m)
	{
		TRACELOW(("alloc_region: get_region failed"));
		return 0;
	}

	/* sanity check: even addresses only, please */
	assert((m->loc & MASKBITS) == 0);

	v = attach_region(proc, m);
	/* NOTE: get_region returns a region with link count 1; since attach_region
	 * increments the link count, we restore it after calling attach_region
	 */
	m->links--;
	if (!v)
	{
		m->links--;
		free_region(m);
		TRACE(("alloc_region: attach_region failed"));
		return 0;
	}
	
	return v;
}

/*
 * fork_region(MEMREGION *reg, long txtsize): creates a shadow region
 * descriptor for reg and allocates a new save region to be for the
 * contents of the region.
 *
 * If txtsize in non-zero, this is assumed to be the first region of the
 * program and txtsize is the size of text section of the program, which
 * does not need to be saved (we assume processes don't write on their
 * own code segment).
 *
 * Returns 0 if no region could be allocated.
 */

MEMREGION *
fork_region (MEMREGION *reg, long txtsize)
{
	long len;
	MEMREGION *shdw, *save;
	
	assert(reg != 0);
	len = reg->len;
	if (txtsize)
	{
		len -= txtsize;
		assert(len >= 256);
	}
	
	/* Save regions must be created with PROT_S so the kernel can always access
	 * these regions, even though they are not attached to any process.
	 */
	shdw = kmr_get ();
	if (!shdw)
		return 0;
	
	save = get_region(alt, len, PROT_S);
	if (!save)
	{
		save = get_region(core, len, PROT_S);
		if (!save)
		{
			kmr_free (shdw);
			return 0;
		}
	}
	
	/* The shadow region descriptor is an exact replica of the original region
	 * descriptor, except for the link count.
	 */
	*shdw = *reg;
	shdw->links = 1;
	reg->links--;
	if (!shdw->shadow)
		shdw->shadow = reg;
	reg->next = shdw;
	reg->shadow = shdw;
	
	/* Now copy the contents of the region into the saveplace region */
	if (!txtsize)
		quickmove((char *)save->loc, (char *)reg->loc, len);
	else
	{
		/* KLUDGE: To avoid adding an extra field to memory region
		 * descriptors, to hold the `txtsize' value of the region,
		 * we use the `save' field of the saveplace region's descriptor
		 * (which otherwise would always be 0, or are you going to save
		 * a save region? :-)
		 */
		save->save = (struct memregion *)txtsize;
		quickmove((char *)save->loc, (char *)reg->loc, 256);
		quickmove((char *)save->loc + 256,
			  (char *)reg->loc + (txtsize+256), len-256);
	}
	save->mflags |= M_FSAVED;
	shdw->save = save;
	
	return shdw;
}

/*
 * routines for creating a copy of an environment, and a new basepage.
 * note that the memory regions created should immediately be attached to
 * a process! Also note that create_env always operates in ST RAM, but
 * create_base might not.
 */

MEMREGION *
create_env (const char *env, ulong flags)
{
	long size;
	MEMREGION *m;
	virtaddr v;
	const char *old;
	char *new;
	short protmode;
	
	TRACELOW (("create_env: %lx, %lx", env, flags));
	
	if (!env)
	{
		/* duplicate parent's environment */
		env = curproc->base->p_env;
		TRACELOW (("create_env: using parents env: %lx", curproc->base->p_env));
	}
	
	size = 2;
	old = env;
	while (*env || *(env+1))
		env++,size++;
	
	protmode = (flags & F_PROTMODE) >> F_PROTSHIFT;
	
	v = alloc_region(core, size, protmode);
	/* if core fails, try alt */
	if (!v)
	    v = alloc_region(alt, size, protmode);
	
	if (!v)
	{
		DEBUG(("create_env: alloc_region failed"));
		return NULL;
	}
	m = addr2mem(curproc, v);

	/* copy the old environment into the new */
	new = (char *) m->loc;
	TRACE(("copying environment: from %lx to %lx", old, new));
	while (size > 0)
	{
		*new++ = *old++;
		size--;
	}
	TRACE(("finished copying environment"));
	
	return m;
}

static void
terminateme (int code)
{
	Pterm (code);
}

MEMREGION *
create_base (const char *cmd, MEMREGION *env, ulong flags, ulong prgsize, PROC *execproc, SHTEXT *s, FILEPTR *f, FILEHEAD *fh, XATTR *xp, long *err)
{
	long len = 0, minalt = 0, coresize, altsize;
	MMAP map;
	MEMREGION *m;
	BASEPAGE *b, *bparent = 0;
	PROC *parent = 0;
	short protmode;
	int i, ismax = 1;
	
	/* if we're about to do an exec tell max_rsize which of the exec'ing
	 * process regions will be freed, but don't free them yet so the process
	 *  can still get an ENOMEM...
	 */
	if (execproc)
	{
		for (i = 0; i < execproc->num_reg; i++)
		{
			m = execproc->mem[i];
			if (m && m->links == 1)
			{
				m->links = 0xfffe;

				/* if the region has shadows, free_region will
				 * free the save region of the first shadow.
				 */
				if (m->shadow)
				{
					if (m->save)
						m->save->links = 0xfffe;
					else
						m->shadow->save->links = 0xfffe;
				}
			}
		}
	}
	
	/* if flags & F_ALTLOAD == 1, then we might decide to load in alternate
	 * RAM if enough is available. "enough" is: if more alt ram than ST ram,
	 * load there; otherwise, if more than (minalt+1)*128K alt ram available
	 * for heap space, load in alt ram ("minalt" is the high byte of flags)
	 */
again2:
	if (flags & (F_ALTLOAD|F_SHTEXT))
	{
		minalt = (flags & F_MINALT) >> 28L;
		minalt = len = (minalt+1)*128*1024L + prgsize + 256;
		if ((flags & F_MINALT) == F_MINALT)
			len = 0;
		else
			ismax = 0;
	}
again1:
	if (flags & F_ALTLOAD)
	{
		coresize = max_rsize (core, len);
		altsize = max_rsize (alt, len);
		if (altsize >= coresize)
		{
			map = alt;
			len = altsize;
		}
		else
		{
			if (altsize >= minalt)
			{
				map = alt;
				len = altsize;
			}
			else
			{
				map = core;
				len = coresize;
			}
		}
	}
	else
	{
		map = core;
		len = max_rsize (map, len);
	}
	
	/* make sure that a little bit of memory is left over */
	if (len > 2 * KEEP_MEM)
		len -= KEEP_MEM;

	/* BUG:
	 * in case Pexec(7, F_SMALLTPA, ...) is used, the code below results
	 * in len == 1280 bytes (because prgsize is zero). In fact, this should
	 * be controlled by the calling program. What about a Pexec(7) expansion
	 * to pass the requested size as third parameter?
	 */
	if (flags & F_SMALLTPA && (len > (prgsize + SLB_INIT_STACK)))
		len = prgsize + SLB_INIT_STACK + 256L; 		/* -> basepage */
	
	if (initialmem && (len > (prgsize + 1024L * initialmem)))
		len = prgsize + 1024L * initialmem;
	
	if (curproc->maxmem && len > curproc->maxmem)
	{
		if (ismax >= 0)
			len = curproc->maxmem;
		else if (len > curproc->maxmem + fh->ftext)
			len = curproc->maxmem + fh->ftext;
	}
	
	/*
	 * TeSche: the third part of this comparison can cause problems when
	 * alternate memory is getting low, in this case `len' is already set
	 * to coresize but won't get decremented here. when actually allocating
	 * the memory later fh->ftext and KERNEL_MEM are added again resulting
	 * in a request bigger than coresize -> error... :(
	 */
# if 1
	if (s && !s->text)
	{
# else
	if (s && !s->text && (!(flags & F_ALTLOAD) || map == alt || altsize < fh->ftext))
	{
# endif
		if (len > fh->ftext + KERNEL_MEM)
			len -= fh->ftext + KERNEL_MEM;
		else
			len = 0;
# if 1
		if (prgsize && len < prgsize + 0x400)
		{
			if (!ismax)
			{
				len = minalt + fh->ftext;
				ismax = -1;
				goto again1;
			}
			if (NULL != (s->text = addr2mem(curproc, alloc_region(map, fh->ftext, PROT_P))))
			{
				goto again2;
			}
		}
# endif
	}

	if (prgsize && len < prgsize + 0x400)
	{
		/* can't possibly load this file in its eligible regions */
		DEBUG(("create_base: max_rsize smaller than prgsize"));

		if (execproc)
		{
			/* error, undo the above */
			for (i = 0; i < execproc->num_reg; i++)
			{
				m = execproc->mem[i];
				if (m)
				{
					if (m->links == 0xfffe)
					{
						m->links = 1;
						if (m->shadow)
						{
							if (m->save)
								m->save->links = 1;
							else
								m->shadow->save->links = 1;
						}
					}
				}
			}
		}
		
		if (s && !s->text)
			kfree (s);
		
		*err = ENOMEM;
		return NULL;
	}
	
	if (execproc)
	{
		/* free exec'ing process memory... if the exec returns after this make it
		 * _exit (SIGKILL << 8);
		 */
		*((short *) (execproc->stack + ISTKSIZE + sizeof (void (*)()))) = (SIGKILL << 8);
		execproc->ctxt[SYSCALL].term_vec = (long)rts;
		execproc->ctxt[SYSCALL].pc = (long)terminateme;
		execproc->ctxt[SYSCALL].sr |= 0x2000;
		execproc->ctxt[SYSCALL].ssp = (long)(execproc->stack + ISTKSIZE);
		
		/* save basepage p_parent */
		bparent = execproc->base->p_parent;
		
		/* blocking forks keep the same basepage for parent and child,
		 * so this p_parent actually was our grandparents...
		 */
		parent = pid2proc(execproc->ppid);
		if (parent
			&& parent->wait_q == WAIT_Q
			&& parent->wait_cond == (long) execproc)
		{
			bparent = parent->base;
		}
		for (i = 0; i < execproc->num_reg; i++)
		{
			m = execproc->mem[i];
			if (m && m->links == 0xfffe)
			{
				execproc->mem[i] = 0;
				execproc->addr[i] = 0;
				if (m->mflags & M_SHTEXT_T)
				{
					TRACE (("create_base: keeping sticky text segment (%lx, len %lx)",
						m->loc, m->len));
					m->links = 0xffff;
				}
				else
				{
					m->links = 0;
					if (m->shadow)
					{
						if (m->save)
							m->save->links = 1;
						else
							m->shadow->save->links = 1;
					}
					if (mem_prot_flags & MPF_STRICT)
						mark_proc_region(execproc, m, PROT_I);
					free_region(m);
				}
			}
		}
	}
	
	protmode = (flags & F_PROTMODE) >> F_PROTSHIFT;

	m = 0;
	if (s && !s->f)
	{
		if (!s->text)
		{
			m = addr2mem(curproc, alloc_region(map, len + fh->ftext + KERNEL_MEM, protmode));
			if (!m ||
			    (((len > minalt &&
				((flags & F_MINALT) < F_MINALT) &&
				max_rsize (map, -1) < fh->ftext) ||
			      0 == (s->text = addr2mem(curproc, alloc_region(map, fh->ftext, PROT_P))) ||
			      (m->next == s->text &&
				!(detach_region (curproc, s->text), s->text = 0))) &&
			     shrink_region(m, fh->ftext)))
			{
				if (m)
					detach_region(curproc, m);
				kfree (s);
				*err = ENOMEM;
				return NULL;
			}
			if (!s->text)
			{
				s->text = m;
				if (protmode != PROT_P)
					mark_region(m, PROT_P);
				m = 0;
			}
		}
		s = get_text_seg(f, fh, xp, s, 0, err);
		if (!s)
		{
			if (m) detach_region(curproc, m);
			
			DEBUG (("create_base: unable to load shared text segment"));
			return NULL;
		}
	}
	
	if (!m)
		m = addr2mem (curproc, alloc_region (map, len, protmode));
	
	if (!m)
	{
		*err = ENOMEM;
		
		DEBUG (("create_base: alloc_region failed"));
		return NULL;
	}
	
	b = (BASEPAGE *)(m->loc);
	
	bzero (b, sizeof (*b));
	b->p_lowtpa = (long) b;
	b->p_hitpa = m->loc + m->len;
	b->p_env = env ? ((char *) env->loc) : NULL;
	b->p_flags = flags;
	
	if (execproc)
	{
		execproc->base = b;
		b->p_parent = bparent;
	}
	
	if (cmd)
		strncpy (b->p_cmdlin, cmd, 127);
	
	return m;
}

/*
 * load_region(): loads the program with the given file name
 * into a new region, and returns a pointer to that region. On
 * an error, returns 0 and leaves the error number in err.
 * "env" points to an already set up environment region, as returned
 * by create_env. On success, "xp" points to the file attributes, which
 * Pexec has already determined, and "fp" points to the programs
 * prgflags. "text" is a pointer to a MEMREGION
 * pointer, which will be set to the region occupied by the shared
 * text segment of this program (if applicable).
 *
 * xp:     attributes for the file just loaded
 * text:   set to point to shared text region, if any
 * fp:     prgflags for this file
 * isexec: this is an exec*() (overlay)
 * err:    error code is stored here
 */

MEMREGION *
load_region (const char *filename, MEMREGION *env, const char *cmdlin, XATTR *xp, MEMREGION **text, long *fp, int isexec, long *err)
{
	FILEPTR *f;
	MEMREGION *reg, *shtext;
	BASEPAGE *b;
	long size, start;
	FILEHEAD fh;
	SHTEXT *s;
	
	/* bug: this should be O_DENYW mode, not O_DENYNONE
	 * we must use O_DENYNONE because of the desktop and because of the
	 * TOS file system brain-damage
	 */
# if 0
 	f = do_open(filename, O_DENYNONE | O_EXEC, 0, xp, err);
# else
	f = do_open(filename, O_DENYW | O_EXEC, 0, xp, err);
# endif
	
	if (!f) return NULL;
	
	size = xdd_read (f, (void *) &fh, (long) sizeof (fh));
	if (fh.fmagic != GEMDOS_MAGIC || size != (long) sizeof (fh))
	{
		DEBUG (("load_region: file not executable"));
		*err = ENOEXEC;
failed:
		if (*err == E_OK)
			*err = ENOEXEC ;
		
		do_close (f);
		return NULL;
	}
	
	if (((fh.flag & F_PROTMODE) >> F_PROTSHIFT) > PROT_MAX_MODE)
	{
		DEBUG (("load_region: invalid protection mode changed to private"));
		fh.flag = (fh.flag & ~F_PROTMODE) | F_PROT_P;
	}
	
	*fp = fh.flag;
	
	if (fh.flag & F_SHTEXT)
	{
		TRACE (("loading shared text segment"));
		s = get_text_seg (f, &fh, xp, 0L, isexec, err);
		if (!s)
		{
			DEBUG(("load_region: unable to get shared text segment"));
			goto failed;
		}
		size = fh.fdata + fh.fbss;
		shtext = s->text;
	}
	else
	{
		size = fh.ftext + fh.fdata + fh.fbss;
		shtext = 0;
		s = 0;
	}
	
	if (env) env->links++;
	
	if (s && !shtext)
	{
		reg = create_base (cmdlin, env, fh.flag, size, isexec ? curproc : 0L, s, f, &fh, xp, err);
		shtext = s->text;
	}
	else
	{
		if (shtext)
			shtext->links++;
		
		reg = create_base (cmdlin, env, fh.flag, size, isexec ? curproc : 0L, 0L, 0L, 0L, 0L, err);
		
		if (shtext)
		{
			shtext->links--;
# if 1
			/* if create_base failed maybe the (sticky) text segment itself is
			 * fragmenting memory... force it reloaded and have a second try
			 */
			if (!reg && shtext->links == 1 && isexec)
			{
				s->f = 0;
				f->links--;
				detach_region (curproc, shtext);
				s = get_text_seg(f, &fh, xp, 0L, isexec, err);
				if (!s)
				{
					DEBUG (("load_region: unable to get shared text segment"));
					goto failed;
				}
				reg = create_base (cmdlin, env, fh.flag, size, curproc, s, f, &fh, xp, err);
				shtext = s->text;
			}
# endif
		}
	}
	
	if (env) env->links--;
	
	if (reg && size+1024L > reg->len)
	{
		DEBUG (("load_region: insufficient memory to load"));
		detach_region (curproc, reg);
		reg = NULL;
		*err = ENOMEM;
	}
	
	if (reg == NULL)
	{
		if (shtext)
			detach_region (curproc, shtext);
		
		goto failed;
	}

	b = (BASEPAGE *) reg->loc;
	b->p_flags = fh.flag;
	if (shtext)
	{
		b->p_tbase = shtext->loc;
		b->p_tlen = 0;
		b->p_dbase = b->p_lowtpa + 256;
	}
	else
	{
		b->p_tbase = b->p_lowtpa + 256;
		b->p_tlen = fh.ftext;
		b->p_dbase = b->p_tbase + b->p_tlen;
	}
	b->p_dlen = fh.fdata;
	b->p_bbase = b->p_dbase + b->p_dlen;
	b->p_blen = fh.fbss;
	
	/* if shared text, then we start loading at the end of the
	 * text region, since that is already set up
	 */
	if (shtext)
	{
		/* skip over text info */
		size = fh.fdata;
		start = fh.ftext;
	}
	else
	{
		size = fh.ftext + fh.fdata;
		start = 0;
	}
	
	*err = load_and_reloc (f, &fh, (char *) b + 256, start, size, b);
	if (*err)
	{
		detach_region (curproc, reg);
		if (shtext) detach_region (curproc, shtext);
		goto failed;
	}

	/* Draco: if the user has set FASTLOAD=YES in the CNF file, the actual
	 * program flag will be ignored and we behave like it was set. This is
	 * to avoid clearing all the virtual memory (on the disk) by the system
	 * and to teach programers they should have all necessary stuff initialized...
	 * ;)
	 */
	if ((fh.flag & F_FASTLOAD) || forcefastload) /* fastload bit */
		size = b->p_blen;
	else
		size = b->p_hitpa - b->p_bbase;
	
	if (size > 0)
	{
		start = b->p_bbase;
		if (start & 1)
		{
			*(char *) start = 0;
			start++;
			--size;
		}
		bzero ((void *) start, size);
	}
	
	do_close (f);
	*text = shtext;
	
	DEBUG (("load_region: return region = %lx", reg));
	return reg;
}

/*
 * load_and_reloc(f, fh, where, start, nbytes): load and relocate from
 * the open GEMDOS executable file f "nbytes" bytes starting at offset
 * "start" (relative to the end of the file header, i.e. from the first
 * byte of the actual program image in the file). "where" is the address
 * in (physical) memory into which the loaded image must be placed; it is
 * assumed that "where" is big enough to hold "nbytes" bytes!
 */

long
load_and_reloc (FILEPTR *f, FILEHEAD *fh, char *where, long start, long nbytes, BASEPAGE *base)
{
	uchar c, *next;
	long r;
# define LRBUFSIZ 8196
	static uchar buffer[LRBUFSIZ];
	long fixup, size, bytes_read;
	long reloc;
	
	
	TRACE (("load_and_reloc: %ld to %ld at %lx", start, nbytes+start, where));
	
	r = xdd_lseek (f, start + sizeof (FILEHEAD), SEEK_SET);
	if (r < E_OK) return r;
	r = xdd_read (f, where, nbytes);
	if (r != nbytes)
	{
		DEBUG (("load_and_reloc: unexpected EOF"));
		return ENOEXEC;
	}
	
	/* now do the relocation
	 * skip over symbol table, etc.
	 */
	r = xdd_lseek (f, sizeof (FILEHEAD) + fh->ftext + fh->fdata + fh->fsym, SEEK_SET);
	if (r < E_OK)
		return ENOEXEC;
	
	if (fh->reloc != 0 || xdd_read (f, (char *) &fixup, 4L) != 4L || fixup == 0)
	{
		cpush ((void *) base->p_tbase, base->p_tlen);
		/* no relocation to be performed */
		return E_OK;
	}
	
	size = LRBUFSIZ;
	bytes_read = 0;
	next = buffer;
	
	do {
		if (fixup >= nbytes + start)
		{
			TRACE (("load_and_reloc: end of relocation at %ld", fixup));
			break;
		}
		else if (fixup >= start)
		{
			reloc = *((long *)(where + fixup - start));
			if (reloc < fh->ftext)
			{
				reloc += base->p_tbase;
			}
			else if (reloc < fh->ftext + fh->fdata && base->p_dbase)
			{
				reloc += base->p_dbase - fh->ftext;
			}
			else if (reloc < fh->ftext + fh->fdata + fh->fbss && base->p_bbase)
			{
				reloc += base->p_bbase - (fh->ftext + fh->fdata);
			}
			else
			{
				DEBUG (("load_region: bad relocation: %ld", reloc));
				if (base->p_dbase)
				    reloc += base->p_dbase - fh->ftext;	/* assume data reloc */
				else if (base->p_bbase)
				    reloc += base->p_bbase - (fh->ftext + fh->fdata);
				else
				    return ENOEXEC;
			}
			*((long *)(where + fixup - start)) = reloc;
		}
		do {
			if (!bytes_read)
			{
				bytes_read = xdd_read (f,(char *)buffer,size);
				next = buffer;
			}
			if (bytes_read < 0)
			{
				DEBUG (("load_region: EOF in relocation"));
				return ENOEXEC;
			}
			else if (bytes_read == 0)
			{
				c = 0;
			}
			else
			{
				c = *next++; bytes_read--;
			}
			if (c == 1) fixup += 254;
		}
		while (c == 1);
		
		fixup += ((unsigned) c) & 0xff;
	}
	while (c);
	
	cpush ((void *) base->p_tbase, base->p_tlen);
	return E_OK;
}

/*
 * function to check for existence of a shared text region
 * corresponding to file "f", and if none is found, to create one
 * the memory region being returned is attached to the current
 * process
 */

SHTEXT *
get_text_seg (FILEPTR *f, FILEHEAD *fh, XATTR *xp, SHTEXT *s, int noalloc, long *err)
{
	MEMREGION *m;
	BASEPAGE b;

	if (s)
	{
		m = s->text;
	}
	else
	{
		s = text_reg;
		
		while(s)
		{
			if (s->f && samefile (&f->fc, &s->f->fc) &&
			    xp->mtime == s->mtime &&
			    xp->mdate == s->mdate)
			{
				m = s->text;
				
				/* Kludge for unattached shared region */
				if (m->links == 0xffff)
					m->links = 0;
				
				if (attach_region(curproc, m))
				{
					TRACE (("re-using shared text region %lx", m));
					return s;
				}
				else
				{
					*err = ENOMEM;
					return NULL;
				}
			}
			
			s = s->next;
		}
		
		/* hmmm, not found; OK, we'll have to create a new text region */
		s = kmalloc (sizeof (*s));
		if (!s)
		{
			*err = ENOMEM;
			return NULL;
		}
		
		if (noalloc)
		{
			s->f = 0;
			s->text = 0;
			return s;
		}
		
		m = 0;
	}
	
	if (!m)
	{
		/* actually, I can't see why loading in TT RAM is ever undesireable,
		 * since shared text programs should be very clean (and since only
		 * the text segment is going in there). But better safe than sorry.
		 */
		if (fh->flag & F_ALTLOAD)
			m = addr2mem (curproc, alloc_region (alt, fh->ftext, PROT_P));
		
		if (!m)
			m = addr2mem (curproc, alloc_region (core, fh->ftext, PROT_P));
	}
	
	if (!m)
	{
		kfree (s);
		*err = ENOMEM;
		return NULL;
	}
	
	/* set up a fake "basepage" for load_and_reloc
	 * note: the 0 values should make load_and_reloc
	 * barf on any attempts at data relocation, since we have
	 * no data segment
	 */
	TRACE (("attempting to create shared text region"));
	
	b.p_tbase = m->loc;
	b.p_tlen = fh->ftext;
	b.p_dbase = 0;
	b.p_dlen = 0;
	b.p_bbase = b.p_blen = 0;
	
	*err = load_and_reloc (f, fh, (char *)m->loc, 0, fh->ftext, &b);
	if (*err)
	{
		detach_region (curproc, m);
		kfree (s);
		return NULL;
	}
	
	/* region has valid shared text data */
	m->mflags |= M_SHTEXT;
# if 0
	/* gryf: today no longer neccessary */
	if (xp->mode & 01000)
# endif
	/* make it sticky (this should depend on the files mode!?) */
	m->mflags |= M_SHTEXT_T;
	
	/*
	 * KLUDGE: to make sure we always have up to date shared text
	 * info, even across a network, we leave the file passed
	 * to us open with DENYWRITE mode, so that nobody will
	 * modify it.
	 */
	f->links++;	/* keep the file open longer */
	
	/* BUG: what if someone already has the file open for
	 * writing? Then we could get screwed...
	 */
	f->flags = (f->flags & ~O_SHMODE) | O_DENYW;
	s->f = f;
	s->text = m;
	s->next = text_reg;
	s->mtime = xp->mtime;
	s->mdate = xp->mdate;
	text_reg = s;
	
	TRACE (("shared text region %lx created", m));
	return s;
}

/*
 * function to just check for existence of a shared text region
 * corresponding to file "f"
 */

MEMREGION *
find_text_seg (FILEPTR *f)
{
	SHTEXT *s;
	
	for (s = text_reg; s; s = s->next)
	{
		if (s->f && samefile (&f->fc, &s->f->fc))
			return s->text;
	}
	
	return NULL;
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

void rts (void) {}		/* dummy termination routine */

PROC *
exec_region (PROC *p, MEMREGION *mem, int thread)
{
	BASEPAGE *b;
	FILEPTR *f;
	int i;
	MEMREGION *m;
	
	TRACE (("exec_region"));
	
	b = (BASEPAGE *) mem->loc;
	
	cpush ((void *) b->p_tbase, b->p_tlen);	/* flush cached versions of the text */
	
	/* set some (undocumented) variables in the basepage */
	b->p_defdrv = p->curdrv;
	for (i = 0; i < 6; i++)
		b->p_devx[i] = i;
	
	p->dta = (DTABUF *)(b->p_dta = &b->p_cmdlin[0]);
	p->base = b;
	
	/* close extra open files */
	for (i = MIN_OPEN; i < MAX_OPEN; i++)
	{
		if ((f = p->handle[i]) != 0 && (p->fdflags[i] & FD_CLOEXEC))
		{
			do_pclose (p, f);
			p->handle[i] = 0;
		}
	}
	
	/* initialize memory */
	recalc_maxmem (p);
	if (p->maxmem)
	{
		shrink_region (mem, p->maxmem);
		b->p_hitpa = b->p_lowtpa + mem->len;
	}
	
	p->memflags = b->p_flags;
	
	if (!thread)
	{
		for (i = 0; i < p->num_reg; i++)
		{
			m = p->mem[i];
			if (m)
			{
				m->links--;
# if 1
				if (m->links <= 0)
				{
					if (!m->links)
					{
						if (m->mflags & M_SHTEXT_T)
						{
							TRACE (("exec_region: keeping sticky text segment (%lx, len %lx)",
								m->loc, m->len));
							m->links = 0xffff;
						}
						else
							free_region (m);
					}
					else
						ALERT ("exec_region: region %lx bogus link count %d, not freed (len %lx)",
							m->loc, m->links, m->len);
				}
				else
				{
					/* Update curproc's mmu table, but not
					 * for basepage and environment */
					if ((m->loc != (long)b) &&
					    (m->loc != (long)b->p_env) &&
					    (mem_prot_flags & MPF_STRICT))
					{
						mark_proc_region(p, m, PROT_I);
					}
				}
# else
				if (m->links <= 0)
					free_region (m);
				else
				{
					if ((m->loc != (long)b) &&
					    (m->loc != (long)b->p_env) &&
					    (mem_prot_flags & MPF_STRICT))
					{
						mark_proc_region(p, m, PROT_I);
					}
				}
# endif
			}
		}
		if (p->num_reg > NUM_REGIONS)
		{
			/*
			 * If the proc struct has a larger mem array than
			 * the default, then free it and allocate a
			 * default-sized one.
			 */

			/*
			 * hoo ha! Memory protection problem here. Use
			 * temps and pre-clear p->mem so memprot doesn't try
			 * to walk these structures as we're freeing and
			 * reallocating them!  (Calling kmalloc can cause
			 * a table walk if the alloc results in calling
			 * get_region.)
			 */
			void *pmem, *paddr;

			pmem = p->mem;
			paddr = p->addr;
			p->mem = NULL; p->addr = NULL;
			kfree (pmem); kfree (paddr);
			
			pmem = kmalloc (NUM_REGIONS * sizeof (MEMREGION *));
			paddr = kmalloc (NUM_REGIONS * sizeof (virtaddr));
			assert (pmem && paddr);
			p->mem = pmem;
			p->addr = paddr;
			p->num_reg = NUM_REGIONS;
		}
		
		bzero (p->mem, (p->num_reg) * sizeof (MEMREGION *));
		bzero (p->addr, (p->num_reg) * sizeof (virtaddr));
	}
	
	/* initialize signals */
	p->sigmask = 0;
	for (i = 0; i < NSIG; i++)
	{
		if (p->sighandle[i] != SIG_IGN)
		{
			p->sighandle[i] = SIG_DFL;
			p->sigflags[i] = 0;
			p->sigextra[i] = 0;
		}
	}
	
	/* zero the user registers, and set the FPU in a "clear" state */
	for (i = 0; i < 15; i++)
		p->ctxt[CURRENT].regs[i] = 0;
	p->ctxt[CURRENT].sr = 0;
	p->ctxt[CURRENT].fstate[0] = 0;
	
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
	if (valid_address (b->p_hitpa - 0x28))
		p->ctxt[CURRENT].usp = b->p_hitpa - 0x28;
	else
		p->ctxt[CURRENT].usp = mem->loc + mem->len - 0x28;
	
	p->ctxt[CURRENT].ssp = (long)(p->stack + ISTKSIZE);
	p->ctxt[CURRENT].term_vec = (long)rts;
	
	/* set up stack for process */
	*((long *)(p->ctxt[CURRENT].usp + 4)) = (long) b;
	
	/* check for a valid text region. some compilers (e.g. Lattice 3) just
	 * throw everything into the text region, including data; fork() must
	 * be careful to save the whole region, then. We assume that if the
	 * compiler (or assembler, or whatever) goes to the trouble of making
	 * separate text, data, and bss regions, then the text region is code
	 * and isn't modified and fork doesn't have to save it.
	 */
	if (b->p_blen != 0 || b->p_dlen != 0)
		p->txtsize = b->p_tlen;
	else
		p->txtsize = 0;
	
	/* An ugly hack: dLibs tries to poke around in the parent's address
	 * space to find stuff. For now, we'll allow this by faking a pointer
	 * into the parent's address space in the place in the basepage where
	 * dLibs is expecting it. This ugly hack only works correctly if the
	 * Pexec'ing program (i.e. curproc) is in user mode.
	 */
	if (curproc != rootproc)
		curproc->base->p_usp = curproc->ctxt[SYSCALL].usp - 0x32;
	
	return p;
}

/*
 * misc. utility routines
 */

/*
 * long memused(p): return total memory allocated to process p
 */

long
memused (PROC *p)
{
	int i;
	long size;
	
	/* a ZOMBIE owns no memory and its mem array ptr is zero */
	if (p->mem == NULL) return 0;
	
	size = 0;
	for (i = 0; i < p->num_reg; i++)
	{
		if (p->mem[i])
		{
			if (p->mem[i]->mflags & M_SEEN)
				continue;	/* count links only once */
			
			p->mem[i]->mflags |= M_SEEN;
			size += p->mem[i]->len;
		}
	}
	
	for (i = 0; i < p->num_reg; i++)
	{
		if (p->mem[i])
			p->mem[i]->mflags &= ~M_SEEN;
	}
	
	return size;
}

/* 
 * recalculate the maximum memory limit on a process; this limit depends
 * on the max. allocated memory and max. total memory limits set by
 * p_setlimit (see dos.c), and (perhaps) on the size of the program
 * that the process is executing. whenever any of these things
 * change (through p_exec or p_setlimit) this routine must be called
 */

void
recalc_maxmem (PROC *p)
{
	long siz = 0;
	
	if (p->base)
		siz = p->base->p_tlen + p->base->p_dlen + p->base->p_blen;
	
	p->maxmem = 0;
	
	if (p->maxdata)
		p->maxmem = p->maxdata + siz;
	
	if (p->maxcore)
		if (p->maxmem == 0 || p->maxmem > p->maxcore)
			p->maxmem = p->maxcore;
	
	if (p->maxmem && p->maxmem < siz)
		p->maxmem = siz;
}

/*
 * valid_address: checks to see if the indicated address falls within
 * memory attached to the current process
 */

int
valid_address (long addr)
{
	int i;
	
	for (i = 0; i < curproc->num_reg; i++)
	{
		MEMREGION *m = curproc->mem[i];
		
		if (m && addr >= m->loc && addr <= m->loc + m->len)
				return 1;
	}
	
	return 0;
}

/*
 * given an address, find the corresponding memory region in this program's
 * memory map
 */

MEMREGION *
addr2mem (PROC *p, virtaddr a)
{
	register int i;
	
	for (i = 0; i < p->num_reg; i++)
		if (a == p->addr[i])
			return p->mem[i];
	
	return NULL;
}

/*
 * convert an address to a memory region; this works only in
 * the ST RAM and TT RAM maps, and will fail for memory that
 * MiNT doesn't own or which is virtualized
 */

static MEMREGION *
addr2region1 (MMAP map, long addr)
{
	MEMREGION *r;
	
	for (r = *map; r; r = r->next)
	{
		if (addr >= r->loc && addr < r->loc + r->len && !r->save)
		{
			if (r->mflags & M_FSAVED)
			{
				DEBUG (("addr2region: why look up a save region?"));
				return NULL;
			}
			
			return r;
		}
	}
	
	return NULL;
}

MEMREGION *
addr2region (long addr)
{
	MEMREGION *r;
	
	r = addr2region1 (core, addr);
	if (!r) r = addr2region1 (alt, addr);
	
	return r;
}

/*
 * convert an address to the memory region attached to a process
 */

MEMREGION *
proc_addr2region (PROC *p, long addr)
{
	int i;
	MEMREGION *m;
	
	for (i = 0; i < p->num_reg; i++)
	{
		m = p->mem[i];
		if (m && m->loc <= addr && addr < m->loc + m->len)
			return m;
	}
	
	return NULL;
}

/*
 * long
 * realloc_region(MEMREGION *reg, long newsize):
 * attempt to resize "reg" to the indicated size. If newsize is
 * less than the current region size, the call always
 * succeeds; otherwise, we look for free blocks next to the
 * region, and try to merge these.
 *
 * If newsize == -1L, simply returns the maximum size that
 * the block could be allocated to.
 *
 * Returns: the (physical) address of the new bottom of the
 * region, or 0L if the resize attempt fails.
 *
 * NOTES: if reg == 0, this call does a last-fit allocation
 * of memory of the requested size, and returns a MEMREGION *
 * (cast to a long) pointing at the last region that works
 *
 * This call works ONLY in the "core" memory region (aka ST RAM)
 * and only on non-shared text regions.
 */

long
realloc_region (MEMREGION *reg, long newsize)
{
	MMAP map = core;
	MEMREGION *m, *prevptr;
	long oldsize, trysize;
	
	if (!((reg == 0) || (reg->mflags & M_CORE)))
		return 0;
	
	if (newsize != -1L)
		newsize = ROUND (newsize);
	
	oldsize = reg->len;
	
	/* last fit allocation: this is pretty straightforward,
	 * we just look for the last block that would work
	 * and slice off the top part of it.
	 * problem: we don't know what the "last block that would fit"
	 * is for newsize == -1L, so we look for the biggest block
	 */
	if (reg == 0)
	{
		MEMREGION *lastfit = 0;
		MEMREGION *newm = kmr_get ();
		
		for (m = *map; m; m = m->next)
		{
			if (ISFREE(m))
			{
				if (newsize == -1L && lastfit 
					&& m->len >= lastfit->len)
				{
					lastfit = m;
				}
				else if (m->len >= newsize)
				{
					lastfit = m;
				}
			}
		}
		if (!lastfit)
			return 0;

		if (newsize == -1L)
			return lastfit->len;

		/* if the sizes match exactly, we save a bit of work */
		if (lastfit->len == newsize)
		{
			if (newm) kmr_free (newm);
			lastfit->links++;
			mark_region (lastfit, PROT_G);
			return (long) lastfit;
		}
		if (!newm) return 0;	/* can't get a new region */

		/* chop off the top "newsize" bytes from lastfit
		 * and add it to "newm"
		 */
		lastfit->len -= newsize;
		newm->loc = lastfit->loc + lastfit->len;
		newm->len = newsize;
		newm->mflags = lastfit->mflags & M_MAP;
		newm->links++;
		newm->next = lastfit->next;
		lastfit->next = newm;
		mark_region (newm, PROT_G);
		
		return (long) newm;
	}
	
	/* check for trivial resize */
	if (newsize == oldsize)
		return reg->loc;
	
	/* find the block just before ours
	 */
	if (*map == reg)
		prevptr = 0;
	else
	{
		prevptr = *map;
		while (prevptr->next != reg && prevptr)
		{
			prevptr = prevptr->next;
		}
	}
	
	/* If we're shrinking the block, there's not too much to
	 * do (we just free the first "oldsize-newsize" bytes by
	 * creating a new region, putting those bytes into it,
	 * and freeing it).
	 */
	if (newsize < oldsize && newsize != -1L)
	{

		if (prevptr && ISFREE (prevptr))
		{
			/* add this memory to the previous free region */
			
			prevptr->len += oldsize - newsize;
			reg->loc += oldsize - newsize;
			reg->len -= oldsize - newsize;
			
			mark_region (prevptr, PROT_I);
			mark_region (reg, PROT_G);
			
			return reg->loc;
		}
		
		/* make a new region for the freed memory */
		m = kmr_get ();
		if (!m)
		{
			/* oops, couldn't get a region -- we lose
			 * punt and pretend we succeeded; after all,
			 * we have enough memory!
			 */
			return reg->loc;
		}

		/* set up the fake region */
		m->links = 0;
		m->mflags = reg->mflags & M_MAP;
		m->loc = reg->loc;
		m->len = oldsize - newsize;
		
		/* update our region (it's smaller now) */
		reg->loc += m->len;
		reg->len -= m->len;
		
		/* link the region in just ahead of us */
		if (prevptr)
			prevptr->next = m;
		else
			*map = m;
		m->next = reg;
		
		mark_region (m, PROT_I);
		mark_region (reg, PROT_G);
		
		return reg->loc;
	}
	
	/* OK, here we have to grow the region: to do this, we first try adding
	 * bytes from the region after us (if any) and then the region before
	 * us
	 */
	trysize = oldsize;
	if (reg->next && ISFREE (reg->next)
		&& (reg->loc + reg->len == reg->next->loc))
	{
		trysize += reg->next->len;
	}
	if (prevptr && ISFREE (prevptr)
		&& (prevptr->loc + prevptr->len == reg->loc))
	{
		trysize += prevptr->len;
	}
	if (trysize < newsize)
	{
		FORCE ("realloc_region: need %ld bytes, only have %ld", newsize, trysize);
		return 0;	/* not enough room */
	}
	
	if (newsize == -1L)	/* size inquiry only?? */
		return trysize;
	
	/* BUG: we can be a bit too aggressive at sweeping up
	 * memory regions coming after our region; on the other
	 * hand, unless something goes seriously wrong there
	 * never should *be* any such regions
	 */
	if (reg->next && ISFREE (reg->next)
		&& (reg->loc + reg->len == reg->next->loc))
	{
		MEMREGION *foo = reg->next;
		
		reg->len += foo->len;
		reg->next = foo->next;
		kmr_free (foo);
		mark_region (reg, PROT_G);
		if (reg->len >= newsize)
			return reg->loc;
		oldsize = reg->len;
	}
	
	assert (prevptr && ISFREE (prevptr) && prevptr->loc + prevptr->len == reg->loc);

	if (newsize > oldsize)
	{
		reg->loc -= (newsize - oldsize);
		reg->len += (newsize - oldsize);
		prevptr->len -= (newsize - oldsize);
		if (prevptr->len == 0)
		{
			/* hmmm, we used up the whole region -- we must dispose of the
			 * region descriptor
			 */
			if (*map == prevptr)
				*map = prevptr->next;
			else
			{
				for (m = *map; m; m = m->next)
				{
					if (m->next == prevptr)
					{
						m->next = prevptr->next;
						break;
					}
				}
			}
			kmr_free (prevptr);
		}
		mark_region (reg, PROT_G);
	}
	
	/* finally! we return the new starting address of "our" region */
	return reg->loc;
}


/*
 * s_realloc emulation: this isn't quite perfect, since the memory
 * used up by the "first" screen will be wasted (we could recover
 * this if we knew the screen start and size, and manually built
 * a region for that screen and linked it into the "core" map
 * (probably at the end))
 * We must always ensure a 256 byte "pad" area is available after
 * the screen (so that it doesn't abut the end of memory).
 */
/* Draco: I am somehow unsure about the assumption, that adding
 * a fixed PAD is a good idea. On Falcons this results with pad
 * area of 512 bytes, while it should be 256 or 384 bytes depending
 * on whether the size & 0x000000ffUL equals zero or 128. Doing so
 * makes Videl Inside II happy in mono (duochrome) modes.
 *
 * NOTE: some buggy screen expanders for Falcon (like all of them...)
 *       do not have any sense of humour and treat ANY number they get
 *       from Srealloc() seriously, as a valid pointer. Even if it
 *       is a NULL.
 *
 *       One of the most frustrating things on Atari is, when we
 *       realize how many idiots think themselves can actually
 *       write system software...
 */

MEMREGION *screen_region = 0;

long _cdecl
s_realloc (long size)
{
	long r;
	
	TRACE (("s_realloc(%ld)", size));
	
	if (size != -1L)
	{
		size += 0x000000ffUL;
		size &= 0xffffff00UL;
	}
	
	if (!screen_region)
	{
		r = realloc_region (screen_region, size);
		if (size == -1L)	/* inquiry only */
		{
			TRACE (("%ld bytes max srealloc", r));
			return r;
		}
		screen_region = (MEMREGION *) r;
		if (!screen_region)
		{
			DEBUG (("s_realloc: no screen region!!"));
			return 0;
		}
		return screen_region->loc;
	}
	
	r = realloc_region (screen_region, size);
	
	return r;
}

/*
 * some debugging stuff
 */

# ifdef DEBUG_INFO

void
DUMP_ALL_MEM (void)
{
	DUMPMEM (core);
	DUMPMEM (alt);
	FORCE ("new memory region descriptor pages: %d", num_reg_requests);
}

void
DUMPMEM (MMAP map)
{
	MEMREGION *m;
	
	m = *map;
	FORCE ("%s memory dump: starting at region %lx",
		(map == core ? "core" : "alt"), m);
	
	while (m)
	{
		FORCE ("%8ld bytes at %lx: next %lx [%d links, mflags %x]",
			m->len, m->loc, m->next, m->links, m->mflags);
		
		if (m->shadow)
			FORCE ("\t\tshadow %lx, save %lx", m->shadow, m->save);
		
		m = m->next;
	}
}
# endif

# ifdef SANITY_CHECKING
static void
sanity_check (MMAP map, ulong line)
{
	MEMREGION *m = *map;
	while (m)
	{
		MEMREGION *next = m->next;
		if (next)
		{
			long end = m->loc + m->len;
			
			if (m->loc < next->loc && end > next->loc)
			{
				FATAL ("%s, %lu: MEMORY CHAIN CORRUPTED", __FILE__, line);
			}
			else if (m->loc == next->loc && m->len > 0
				&& (m->shadow != next || m->len != next->len))
			{
				FATAL ("%s, %lu: SHADOW RING CORRUPTED", __FILE__, line);
			}
			else if (end == next->loc && ISFREE (m) && ISFREE (next))
			{
				ALERT ("%s, %lu: Continguous memory regions not merged!", __FILE__, line);
			}
			else if (!no_mem_prot && (m->loc != ROUND(m->loc)))
			{
				ALERT ("%s, %lu: Memory region unaligned", __FILE__, line);
			}
			else if (!no_mem_prot && (m->len != ROUND(m->len)))
			{
				ALERT ("%s, %lu: Memory region length unaligned", __FILE__, line);
			}
			
			if (m->save && !(m->mflags & M_FSAVED) && !m->shadow)
			{
				FATAL ("%s, %lu: SAVE REGION ATTACHED TO A NON-SHADOW REGION!", __FILE__, line);
			}
		}
		m = next;
	}
}
# endif
