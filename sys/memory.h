/*
 * $Id$
 * 
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
extern ulong initialmem;


void init_mem (void);
void restr_screen (void);
int add_region (MMAP map, ulong place, ulong size, ushort mflags);
void init_core (void);
void init_swap (void);

long change_prot_status (PROC *proc, long start, short newmode);
long attach_region (PROC *proc, MEMREGION *reg);
void detach_region (PROC *proc, MEMREGION *reg);
int detach_region_by_addr (PROC *p, long block);

MEMREGION *get_region (MMAP map, ulong size, short mode);
MEMREGION *_get_region (MMAP map, ulong size, short mode, short cmode, MEMREGION *descr, short kernel_flag);
void free_region (MEMREGION *reg);
long shrink_region (MEMREGION *reg, ulong newsize);

long max_rsize (MMAP map, long needed);
long tot_rsize (MMAP map, short flag);
long freephysmem (void);
long alloc_region (MMAP map, ulong size, short mode);
MEMREGION *fork_region (MEMREGION *reg, long txtsize);
MEMREGION *create_env (const char *env, ulong flags);
MEMREGION *create_base (const char *cmd, MEMREGION *env, ulong flags, ulong prgsize,
			PROC *execproc, FILEPTR *f, FILEHEAD *fh, XATTR *xp, long *err);
MEMREGION *load_region (const char *name, MEMREGION *env, const char *cmdlin, XATTR *x,
			long *fp, short isexec, long *err);
long load_and_reloc (FILEPTR *f, FILEHEAD *fh, char *where, long start,
			long nbytes, BASEPAGE *base);
long memused (PROC *p);
void recalc_maxmem (PROC *p);
int valid_address (long addr);
MEMREGION *addr2mem (PROC *p, long addr);
MEMREGION *addr2region (long addr);
MEMREGION *proc_addr2region (PROC *p, long addr);

long realloc_region (MEMREGION *, long);
long _cdecl s_realloc (long);

# ifdef DEBUG_INFO
void DUMP_ALL_MEM (void);
void DUMPMEM (MMAP map);

# if WITH_KERNFS
long  kern_get_memdebug (SIZEBUF **buffer);
# endif

# endif


# endif /* _memory_h */
