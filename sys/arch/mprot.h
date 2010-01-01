/*
 * $Id$
 * 
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _m68k_mprot_h
# define _m68k_mprot_h

# include "mint/mint.h"
# include "mint/mem.h"
# include "mint/proc.h"


extern int no_mem_prot;

extern long page_table_size;
extern ulong mem_prot_flags;
extern ulong mint_top_st;
extern ulong mint_top_tt;	

void init_tables (void);
int get_prot_mode (MEMREGION *);
void mark_region (MEMREGION *region, short mode, short cmode);
void mark_proc_region (struct memspace *p_mem, MEMREGION *region, short mode, short pid);
int prot_temp (ulong loc, ulong len, short mode);
void init_page_table (PROC *proc, struct memspace *p_mem); /* XXX */
void mem_prot_special (PROC *proc);
void QUICKDUMP (void);
void report_buserr (void);
void BIG_MEM_DUMP (int bigone, PROC *proc);
int mem_access_for (PROC *p, ulong where, long len);

void dump_proc_table(struct proc *p);

# endif /* _m68k_mprot_h */
