/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _m68k_intr_h
# define _m68k_intr_h

# include "mint/mint.h"


/* interrupt vectors linked by new_xbra_install() */
extern long old_5ms;
extern long old_bus;
extern long old_addr;
extern long old_ill;
extern long old_divzero;
extern long old_priv;
extern long old_trace;
extern long old_linef;
extern long old_chk;
extern long old_trapv;
extern long old_mmuconf;
extern long old_format;
extern long old_cpv;
extern long old_uninit;
extern long old_spurious;
extern long old_fpcp_0;
extern long old_fpcp_1;
extern long old_fpcp_2;
extern long old_fpcp_3;
extern long old_fpcp_4;
extern long old_fpcp_5;
extern long old_fpcp_6;
extern long old_pmmuill;
extern long old_pmmuacc;

/* old reset vector */
extern long old_resvec;

/* old ikbd vector */
extern long old_ikbd;

/* old ikbd vector (other way) */
extern long oldkeys;

/* BIOS disk vectors */
extern long old_mediach, old_getbpb, old_rwabs;

/* Critical error handler */
extern long old_criticerr;
extern long old_exec_os;

long _cdecl	reset		(void);
void _cdecl	reboot		(void) NORETURN;
void _cdecl	newmvec		(void);
void _cdecl	newjvec		(void);
long _cdecl	newkeys		(void);
void _cdecl	kbdclick	(short scancode);
long _cdecl	new_rwabs	(void);
long _cdecl	new_mediach	(void);
long _cdecl	new_getbpb	(void);

void _cdecl	send_packet	(long func, char *buf, char *bufend);

# endif /* _m68k_intr_h */
