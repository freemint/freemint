/*
 * This file gives the address of system variables of the TOS operating system.
 * The source for the information is TOS.hyp
 * It contains definitions only so it can be in assembly files.
 * 
 * Author: Vincent Barrilliot
 * Licence: Public domain
 * TabSize=4
 */

#ifndef _m68k_tos_vars_h_
#define _m68k_tos_vars_h_

/* The names are capitalized on purpose to make it clear that these are constant helpers
 * rather than something that can directly be assigned or dereferenced. */

 /* These are vectors */

#define ETV_CRITIC	0x404
#define ETV_TERM	0x408
#define RESVALID	0x426
#define RESVECTOR	0x42a
#define HDV_BPB		0x472
#define HDV_RW		0x476
#define HDV_MEDIACH	0x47e

/* These are variables */

#define FLOCK		0x43e
#define _TIMR_MS	0x442
#define _DRVBITS	0x4c2
#define CONTERM		0x484
#define _LONGFRAME	0x59e
#define KCL_HOOK	0x5b0

#endif
