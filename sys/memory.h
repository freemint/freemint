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
extern ulong initialmem;


void init_mem (void);
void restr_screen (void);
int add_region (MMAP map, ulong place, ulong size, unsigned mflags);
void init_core (void);
void init_swap (void);

long change_prot_status (PROC *proc, long start, int newmode);
virtaddr attach_region (PROC *proc, MEMREGION *reg);
void detach_region (PROC *proc, MEMREGION *reg);

MEMREGION *get_region (MMAP map, ulong size, int mode);
MEMREGION *_get_region (MMAP map, ulong size, int mode, MEMREGION *descr, int kernel_flag);
void free_region (MEMREGION *reg);
long shrink_region (MEMREGION *reg, ulong newsize);

long max_rsize (MMAP map, long needed);
long tot_rsize (MMAP map, int flag);
long freephysmem (void);
virtaddr alloc_region (MMAP map, ulong size, int mode);
MEMREGION *fork_region (MEMREGION *reg, long txtsize);
MEMREGION *create_env (const char *env, ulong flags);
MEMREGION *create_base (const char *cmd, MEMREGION *env, ulong flags, ulong prgsize,
			PROC *execproc, SHTEXT *s, FILEPTR *f, FILEHEAD *fh, XATTR *xp, long *err);
MEMREGION *load_region (const char *name, MEMREGION *env, const char *cmdlin, XATTR *x,
			MEMREGION **text, long *fp, int isexec, long *err);
SHTEXT *get_text_seg (FILEPTR *f, FILEHEAD *fh, XATTR *xp, SHTEXT *s, int noalloc, long *err);
MEMREGION *find_text_seg (FILEPTR *f);
long load_and_reloc (FILEPTR *f, FILEHEAD *fh, char *where, long start,
			long nbytes, BASEPAGE *base);
void rts (void);
PROC *exec_region (PROC *p, MEMREGION *mem, int thread);
long memused (PROC *p);
void recalc_maxmem (PROC *p);
int valid_address (long addr);
MEMREGION *addr2mem (PROC *p, virtaddr a);
MEMREGION *addr2region (long addr);
MEMREGION *proc_addr2region (PROC *p, long addr);

long realloc_region (MEMREGION *, long);
long _cdecl s_realloc (long);

# ifdef DEBUG_INFO
void DUMP_ALL_MEM (void);
void DUMPMEM (MMAP map);
# endif


# endif /* _memory_h */
