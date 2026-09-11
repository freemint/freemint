/*
 * Interrupt entry for the SVEthlana MAC, see svethlana_i6.S.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef _svethlana_i6_h
#define _svethlana_i6_h

/* handler that was installed on the vector before ours */
extern void (*svethlana_old_vector)(void);

/* the vector entry itself */
void svethlana_interrupt (void);

/* the C handler it calls, returns IRQ_NONE or IRQ_HANDLED */
long _cdecl ethoc_interrupt (void);

#endif /* _svethlana_i6_h */
