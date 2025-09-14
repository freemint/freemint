/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 */

# ifndef _m68k_cpu_h
# define _m68k_cpu_h

# include "mint/mint.h"

/* Exception vectors */
#define VEC_BUS_ERROR			0x08
#define VEC_ADDRESS_ERROR		0x0c
#define VEC_ILLEGAL_INSTRUCTION 0x10
#define VEC_DIVISION_BY_ZERO	0x14
#define VEC_CHK					0x18
#define VEC_TRAPV				0x1c
#define VEC_PRIVILEGE_VIOLATION	0x20
#define VEC_TRACE				0x24
#define VEC_LINE_A				0x28
#define VEC_LINE_F				0x2C
#define VEC_COPRO_PROTOCOL_VIOLATION 0x34 // 68030
#define VEC_FORMAT_ERROR		0x38 // 68010 only
#define VEC_UNINITIALIZED		0x3c
#define VEC_SPURIOUS_INTERRUPT	0x60

/* Math coprocessor*/
#define VEC_FFCP0				0xC0 // Branch or Set on Unordered Condition
#define VEC_FFCP1				0xC4 // Inexact Result
#define VEC_FFCP2				0xC8 // Divide by Zero
#define VEC_FFCP3				0xCC // Underflow
#define VEC_FFCP4				0xD0 // Operand Error
#define VEC_FFCP5				0xD4 // Overflow
#define VEC_FFCP6				0xD8 // Signaling NAN
#define VEC_FFCP7				0xDC // Unassigned, Reserved

/* MMU */
#define VEC_MMU_CONFIG_ERROR	0xe0
#define VEC_MMU1				0xe4 // MC68851, not used by MC68030+
#define VEC_MMU2				0xe8 // MC68851, not used by MC68030+

/* Traps */
#define TRAP0	0x80
#define TRAP1	0x84
#define TRAP2	0x88
#define TRAP3	0x8c
#define TRAP4	0x90
#define TRAP5	0x94
#define TRAP6	0x98
#define TRAP7	0x9c
#define TRAP8	0xa0
#define TRAP9	0xa4
#define TRAP10	0xa8
#define TRAP11	0xac
#define TRAP12	0xb0
#define TRAP13	0xb4
#define TRAP14	0xb8
#define TRAP15	0xbc

void _cdecl	init_cache	(void);

long _cdecl	ccw_getdmask	(void);
long _cdecl	ccw_get		(void);
long _cdecl	ccw_set		(ulong ctrl, ulong mask);

void _cdecl	cpush		(const void *base, long size);
void _cdecl	cpushi		(const void *base, long size);

void _cdecl	setstack	(long);

# ifdef DEBUG_INFO
long _cdecl	get_ssp		(void);
# endif
long _cdecl	get_usp		(void);

void _cdecl	get_superscalar	(void);
short _cdecl	is_superscalar	(void);

# endif /* _m68k_cpu_h */
