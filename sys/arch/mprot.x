/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * 
 * Copyright 1991,1992,1993,1994 Atari Corporation.
 * All rights reserved.
 * 
 * Copyright 2004 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

#include "mint/module.h"
#include "arch/context.h"
#if 0
#define SPECIAL_DEBUG
#endif

#ifdef SPECIAL_DEBUG
static void
lookup_addr(long addr, const char *descr)
{
	MEMREGION *m;
	
	m = addr2region(addr);
	if (m)
	{
		char alertbuf[128];
		char *aptr;
		long len;
		struct proc *p;
		
		aptr = alertbuf;
		len = sizeof(alertbuf);
		
		ksprintf(aptr, len, "%s=%lx ->", descr, addr);
		aptr = alertbuf + strlen(alertbuf);
		len = sizeof(alertbuf) - strlen(alertbuf);
		
		for (p = proclist; p; p = p->gl_next)
		{
			if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
				continue;
			
			if (p->p_mem && p->p_mem->mem)
			{
				MEMREGION **mr;
				int i;
				
				mr = p->p_mem->mem;
				for (i = 0; i < p->p_mem->num_reg; i++, mr++)
				{
					if (*mr == m)
					{
						/* match :-) */
						ksprintf(aptr, len, " pid %i", p->pid);
						aptr = alertbuf + strlen(alertbuf);
						len = sizeof(alertbuf) - strlen(alertbuf);
					}
				}
			}
		}
		
		ALERT(alertbuf);
	}
}
#endif

#if 1
extern struct kernel_module root_module;

static struct proc *
get_address_owner(unsigned long addr, int *idx, MEMREGION **ret_m, ulong *ra, ulong *rl)
{
	struct proc *p;
	struct kernel_module *km;
	ulong a,l,e;

	km = &root_module;
	do {
		if (km->b) {
			
			a = km->b->p_tbase;
 			l = km->b->p_tlen;
 			e = a + l;
 			if (addr >= a && addr < e) {
 				if (idx) *idx = -10;
 				if (ra)  *ra  = a;
 				if (rl)  *rl = l;
 				return (void *)km;
			}
			a = km->b->p_dbase;
 			l = km->b->p_dlen;
 			e = a + l;
 			if (addr >= a && addr < e) {
 				if (idx) *idx = -11;
 				if (ra)  *ra  = a;
 				if (rl)  *rl = l;
 				return (void *)km;
			}
			a = km->b->p_bbase;
 			l = km->b->p_blen;
 			e = a + l;
 			if (addr >= a && addr < e) {
 				if (idx) *idx = -12;
 				if (ra)  *ra  = a;
 				if (rl)  *rl = l;
 				return (void *)km;
			}
		}
		if (km->children) {
			km = km->children;
		} else {
			if (!km->next)
				km = km->parent;
			if (km)
				km = km->next;
		}
	} while (km);

	for (p = proclist; p; p = p->gl_next) {
		ulong ll[6];
		ulong sav[8];
		BASEPAGE *b = p->p_mem->base;

		if (!b || p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
			continue;
		
		proc_access(&(p->ctxt[CURRENT]), sav);
		ll[0] = b->p_tbase;
		ll[1] = b->p_tlen;
		ll[2] = b->p_dbase;
		ll[3] = b->p_dlen;
		ll[4] = b->p_bbase;
		ll[5] = b->p_blen;
		proc_access(NULL, sav);
		
		if (addr >= ll[0] && addr < (ll[0] + ll[1])) {
			if (idx) *idx = -1;
			if (ra)  *ra = ll[0];
			if (rl)  *rl = ll[1];
			return p;
		}
		
		if (addr >= ll[2] && addr < (ll[2] + ll[3])) {
			if (idx) *idx = -2;
			if (ra)  *ra = ll[2];
			if (rl)  *rl = ll[3];
			return p;
		}
		
		if (addr >= ll[4] && addr < (ll[4] + ll[5])) {
			if (idx) *idx = -3;
			if (ra)  *ra = ll[4];
			if (rl)  *rl = ll[5];
			return p;
		}
	}
#if 1
	{
	MEMREGION *m;
	m = addr2region(addr);
	if (m) {
		struct memspace *mem;
		
		for (p = proclist; p; p = p->gl_next) {
			if (p->wait_q == ZOMBIE_Q || p->wait_q == TSR_Q)
				continue;
			
			if ((mem = p->p_mem) && mem->mem) {
				MEMREGION **mr;
				int i;
				
				mr = mem->mem;
				for (i = 0; i < mem->num_reg; i++, mr++) {
					if (*mr == m) {
						if (idx)
							*idx = i;
						if (ret_m)
							*ret_m = m;
						return p;
					}
				}
			}
		}
	}
	}
#endif
	return NULL;
}
#endif

static const char *berr_msg[] = { 
/*  "........." */
    "private  ",
    "global   ",    /* turned into "hardware" for violation reports */
    "super    ",
    "readable ",
    "free     ",
    "hardware "	    /* used when the memory is not controlled by us */
};

static const char *rw_msg[] = {
    "??",
    "w ",
    "r ",
    "rw"
};
static const char *mrmap[] = {
 "", "CORE","ALT","SWAP","KERN"
};

static const char *protmodes[] = {
 "Private",
 "Global",
 "Super",
 "Readable",
 "Invalid",
};

void
report_buserr(void)
{
	struct proc *aa_proc, *pc_proc;
	struct kernel_module *km;
	MEMREGION *aa_mr, *pc_mr;
	int i, l;
	int aaidx, pcidx;
	const char *type, *rw;
	short mode;
	ulong aa, pc, aabase, aalen, pcbase, pclen;
	char *a, *alertbuf;
	
	if (no_mem_prot)
		return;

	aa = curproc->exception_addr;
	pc = curproc->exception_pc;
#if 0
	if (!stricmp(curproc->name, "worldclk"))
		dump_proc_table(curproc);
#endif
	aa_proc = get_address_owner(aa, &aaidx, &aa_mr, &aabase, &aalen);
	pc_proc = get_address_owner(pc, &pcidx, &pc_mr, &pcbase, &pclen);

	if ((mint_top_tt && aa < mint_top_tt) || (aa < mint_top_st))
	{
		mode = global_mode_table[(curproc->exception_addr >> 13)];
		if (mode == PROT_G)
		{
			/* page is global: obviously a hardware bus error */
			mode = 5;
		}
	}
	else
	{
		/* (addr is > mint_top_tt) set mode = 5 so we don't look for owners */
		mode = 5;
	}
	
	type = berr_msg[mode];
	rw = rw_msg[curproc->exception_access];
	
	/* construct an AES alert box for this error:
	 * | PROCESS  "buserrxx"  KILLED: |
	 * | MEMORY VIOLATION.  (PID 000) |
	 * |                              |
	 * | Type: ......... PC: pc...... |
	 * | Addr: ........  BP: ........ |
	 */
#if 0
	ksprintf(alertbuf, sizeof(alertbuf),
		 "[1][ PROCESS  \"%s\"  KILLED: |"
		 " MEMORY VIOLATION.  (PID %03d) |"
		 " Type: %s PC: %08lx |"
		 " Addr: %08lx  BP: %08lx |"
		 "                  OS: %08lx][ OK ]",
		 curproc->name,
		 curproc->pid,
		 type, pc,
		 aa, curproc->p_mem->base,
		 pc - ((ulong)curproc->p_mem->base + 256));
#else
/*

[1][ PROCESS  ABCDEFGG  KILLED:        |
MEMORY VIOLATION. (PID 123)            |
Type: hardware  PC: 0x12345678         |
Addr: 0x12345678 BP: 0x12345678        |
PC is 0x12345678 -> Abcdefgh TEXT      |
AA is 0x12345678 -> Abcdefgh DATA      |

*/
	if (!(alertbuf = kmalloc(512)))
		return;

	l = 512;
	a = alertbuf;
//	l = sizeof(alertbuf);
	i = ksprintf(a, l,
		 "[1][ PROCESS  \"%s\"  KILLED: |"
		 " MEMORY VIOLATION.  (PID %03d) |"
		 "   Type: %s PC: %08lx |"
		 " (%s)AA: %08lx  BP: %08lx |",
		 curproc->name,
		 curproc->pid,
		 type, pc, rw,
		 aa, curproc->p_mem->base);
	a += i;
	l -= i;
	if (pc_proc) {

		if (pcidx == -1) {
			i = ksprintf(a, l,
				" PC is %08lx -> %s TEXT |",
				pc - pcbase, pc_proc->name);
		} else if (pcidx == -2) {
			i = ksprintf(a, l,
				" PC is %08lx -> %s DATA |",
				pc - pcbase, pc_proc->name);
		} else if (pcidx == -3) {
			i = ksprintf(a, l,
				" PC is %08lx -> %s BSS  |",
				pc - pcbase, pc_proc->name);
		} else if (pcidx == -10) {
			km = (void *)pc_proc;
			i = ksprintf(a, l,
				" PC is %08lx into kernelmodule |"
				"   %s TEXT segment |",
				pc - pcbase, km->fname);
		} else if (pcidx == -11) {
			km = (void *)pc_proc;
			i = ksprintf(a, l,
				" PC is %08lx into kernelmodule |"
				"   %s DATA segment |",
				pc - pcbase, km->fname);
		} else if (pcidx == -12) {
			km = (void *)pc_proc;
			i = ksprintf(a, l,
				" PC is %08lx into kernelmodule |"
				"   %s BSS segment |",
				pc - pcbase, km->fname);
		} else {
			i = ksprintf(a, l,
				" PC is memory owned by PID %03d %s |"
				"   loc %08lx - %08lx |"
				"   len %ld links %ld|"
				"   map %s protection %s|"
				" Save %s Shadow %s BPage %s|"
				" mflags %04x |",
				pc_proc->pid, pc_proc->name, pc_mr->loc, pc_mr->loc + pc_mr->len, pc_mr->len, pc_mr->links,
				mrmap[pc_mr->mflags & M_MAP], protmodes[get_prot_mode(pc_mr)],
				pc_mr->save ? "Yes":"No",
				pc_mr->shadow ? "Yes":"No",
				(pc_mr->mflags & M_BPAGE) ? "Yes":"No",
				pc_mr->mflags);
		}
	} else {
		i = ksprintf(a, l,
			" PC is free memory |");
	}
	a += i;
	l -= i;
	if (aa_proc) {
		if (aaidx == -1) {
			i += ksprintf(a, l,
				" AA is %08lx -> %s TEXT][OK]",
				aa - aabase, aa_proc->name);
		} else if (aaidx == -2) {
			i += ksprintf(a, l,
				" AA is %08lx -> %s DATA][OK]",
				aa - aabase, aa_proc->name);
		} else if (aaidx == -3) {
			i += ksprintf(a, l,
				" AA is %08lx -> %s BSS][OK]",
				aa - aabase, aa_proc->name);
		} else if (aaidx == -10) {
			km = (void *)aa_proc;
			i = ksprintf(a, l,
				" AA is %08lx into kernelmodule |"
				"    %s TEXT segment |",
				aa - aabase, km->fname);
		} else if (aaidx == -11) {
			km = (void *)aa_proc;
			i = ksprintf(a, l,
				" AA is %08lx into kernelmodule |"
				"   %s DATA segment |",
				aa - aabase, km->fname);
		} else if (aaidx == -12) {
			km = (void *)aa_proc;
			i = ksprintf(a, l,
				" AA is %08lx into kernelmodule |"
				"   %s BSS segment |",
				aa - aabase, km->fname);
		} else {
			i = ksprintf(a, l,
				" AA is memory owned by PID %03d (%s) |"
				"  loc  %08lx - %08lx |"
				"  len  %ld links %ld |"
				"  map  %s Protection %s |"
				"  Save %s Shadow %s BPage %s |"
				"  mflags %04x][OK]",
				aa_proc->pid, aa_proc->name, aa_mr->loc, aa_mr->loc + aa_mr->len, aa_mr->len, aa_mr->links,
				mrmap[aa_mr->mflags & M_MAP], protmodes[get_prot_mode(aa_mr)],
				aa_mr->save ? "Yes":"No",
				aa_mr->shadow ? "Yes":"No",
				(aa_mr->mflags & M_BPAGE) ? "Yes":"No",
				aa_mr->mflags);
		}
	} else {
		i += ksprintf(a, l,
			" AA is free memory][OK]");
	}

#endif
	
	DEBUG((alertbuf));
	
	if (!_ALERT (alertbuf))
	{
		/* this will call _alert again, but it will just fail again */
		ALERT ("MEMORY VIOLATION: type=%s RW=%s AA=%lx PC=%lx BP=%lx",
			type, rw, aa, pc, curproc->p_mem->base);
		
#ifdef SPECIAL_DEBUG
		lookup_addr(aa, "AA");
		lookup_addr(pc, "PC");
#endif
	}
	
	if (curproc->pid == 0 || curproc->p_mem->memflags & F_OS_SPECIAL)
	{
		/* the system is so thoroughly hosed that anything we try will
		 * likely cause another bus error; so let's just hang up
		 */
		
		FATAL ("Operating system killed");
	}
	kfree(alertbuf);
}
