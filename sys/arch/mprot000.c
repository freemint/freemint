/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Konrad M. Kokoszkiewicz <draco@atari.org>
 * Started: 1999-03-24
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 * This is a dummy memprot module, compiled for 68000-only kernels.
 * It doesn't contain any real code, and the purpose of it is to
 * save space (always short on a 68000 ST).
 *
 */

# include "mprot.h"

# include "memory.h"

#ifndef WITH_MMU_SUPPORT

void
init_tables(void)
{
	return;
}

int
get_prot_mode(MEMREGION *r)
{
	return PROT_G;
}

void
mark_region(MEMREGION *region, short mode, short cmode)
{
	return;
}

void
mark_proc_region(struct memspace *p_mem, MEMREGION *region, short mode, short pid)
{
	return;
}

int
prot_temp(ulong loc, ulong len, short mode)
{
	return -1;
}

void
init_page_table (PROC *proc, struct memspace *p_mem)
{
	return;
}

void
mem_prot_special(PROC *proc)
{
	return;
}

/*----------------------------------------------------------------------------
 * DEBUGGING SECTION
 *--------------------------------------------------------------------------*/

void
QUICKDUMP(void)
{
	return;
}

void
report_buserr(void)
{
	return;
}

void
BIG_MEM_DUMP(int bigone, PROC *proc)
{
	return;
}

int
mem_access_for(PROC *p, ulong start, long nbytes)
{
	return -1;
}

#endif
