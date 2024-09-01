/*
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

void
report_buserr(void)
{
	const char *type, *rw;
	short mode;
	ulong aa, pc;
	char alertbuf[256];	/* enough for an alert */
	PROC *p;

	if (no_mem_prot)
		return;
	
	p = curproc;
	aa = p->exception_addr;
	pc = p->exception_pc;
	
	if ((mint_top_tt && aa < mint_top_tt) || (aa < mint_top_st))
	{
		mode = global_mode_table[(p->exception_addr >> 13)];
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
	rw = rw_msg[p->exception_access];
	
	/* construct an AES alert box for this error:
	 * | PROCESS  "buserrxx"  KILLED: |
	 * | MEMORY VIOLATION.  (PID 000) |
	 * |                              |
	 * | Type: ......... PC: pc...... |
	 * | Addr: ........  BP: ........ |
	 */
	
	ksprintf(alertbuf, sizeof(alertbuf),
		 "[1][ PROCESS  \"%s\"  KILLED: |"
		 " MEMORY VIOLATION.  (PID %03d) |"
		 " Type: %s PC: %08lx |"
		 " Addr: %08lx  BP: %08lx |"
		 "                 OS: %08lx][ OK ]",
		 p->name,
		 p->pid,
		 type, pc,
		 aa, (ulong)p->p_mem->base,
		 pc - ((ulong)p->p_mem->base + 256));
	
	DEBUG((alertbuf));
	
	if (debug_level >= ALERT_LEVEL || p->debug_level >= ALERT_LEVEL)
	{
		if (!_ALERT (alertbuf))
		{
			/* this will call _alert again, but it will just fail again */
			ALERT ("MEMORY VIOLATION: type=%s RW=%s AA=%lx PC=%lx BP=%lx",
				type, rw, aa, pc, (ulong)p->p_mem->base);

#ifdef SPECIAL_DEBUG
			lookup_addr(aa, "AA");
			lookup_addr(pc, "PC");
#endif
		}
	}

	if (p->pid == 0 || (p->p_mem->memflags & F_OS_SPECIAL))
	{
		/* the system is so thoroughly hosed that anything we try will
		 * likely cause another bus error; so let's just hang up
		 */
		
		FATAL ("Operating system killed");
	}
}
