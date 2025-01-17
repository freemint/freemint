/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _memory_h
# define _memory_h

# include "mint/mint.h"
# include "mint/mem.h"
# include "mint/proc.h"


extern MMAP core, alt, swap;

extern short forcefastload;
extern unsigned long initialmem;


void	init_mem (void);
void	restr_screen (void);
int	add_region (MMAP map, ulong place, ulong size, ushort mflags);
void	init_core (void);
void	init_swap (void);

long	change_prot_status (struct proc *proc, long start, short newmode);
long	_cdecl attach_region (struct proc *proc, MEMREGION *reg);
void	_cdecl detach_region (struct proc *proc, MEMREGION *reg);
long	detach_region_by_addr (struct proc *p, unsigned long block);

MEMREGION *get_region (MMAP map, ulong size, short mode);
MEMREGION *_get_region (MMAP map, ulong size, short mode, short cmode, MEMREGION *descr, short kernel_flag);
void	free_region (MEMREGION *reg);
long	shrink_region (MEMREGION *reg, unsigned long newsize);

long	max_rsize (MMAP map, long needed);
long	tot_rsize (MMAP map, short flag);
long	totalphysmem (void);
long	freephysmem (void);
long 	alloc_region (MMAP map, ulong size, short mode);
MEMREGION *fork_region (MEMREGION *reg, long txtsize);
MEMREGION *create_env (const char *env, ulong flags);
MEMREGION *create_base (const char *cmd, MEMREGION *env,
			unsigned long flags, unsigned long prgsize, long *err);
MEMREGION *load_region (const char *name, MEMREGION *env, const char *cmdlin, XATTR *x,
			long *fp, long *err);
long	load_and_reloc (FILEPTR *f, FILEHEAD *fh, char *where, long start,
			long nbytes, BASEPAGE *base);
long	memused (const struct proc *p);
void	recalc_maxmem (struct proc *p, long size);
int	valid_address (long addr);
MEMREGION *_cdecl addr2mem (struct proc *p, long addr);
MEMREGION *addr2region (unsigned long addr);
MEMREGION *proc_addr2region (const struct proc *p, unsigned long addr);

long	realloc_region (MEMREGION *, long);
long	_cdecl sys_s_realloc (long);

# ifdef DEBUG_INFO
void DUMP_ALL_MEM (void);
void DUMPMEM (MMAP map);
# if WITH_KERNFS
long kern_get_memdebug (SIZEBUF **buffer, const struct proc *p);
# endif
# endif

# endif /* _memory_h */
