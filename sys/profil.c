/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 * 
 * 
 * Low-level statistical profiling support function.  Mostly POSIX.1 version.
 * Copyright (C) 1996, 1997, 1998 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 * 
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU C Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 * 
 */

# include "profil.h"


# ifdef PROFILING

ushort profil_on = 0;

static ushort *samples;
static ulong nsamples;
static ulong pc_offset;
static ulong pc_scale;

void
profil_counter (void *pc)
{
	ulong i;
	
	i = ((ulong)pc - pc_offset) / 2UL;
	i = i / 65536 * pc_scale + i % 65536 * pc_scale / 65536;
	
	if (i < nsamples)
		++samples[i];
}

/* Enable statistical profiling, writing samples of the PC into at most
 * SIZE bytes of SAMPLE_BUFFER; every processor clock tick while profiling
 * is enabled, the system examines the user PC and increments
 * SAMPLE_BUFFER[((PC - OFFSET) / 2) * SCALE / 65536].  If SCALE is zero,
 * disable profiling.  Returns zero on success, -1 on error.
 */
long
profil (ushort *sample_buffer, ulong size, ulong offset, ulong scale)
{
	if (sample_buffer == NULL)
	{
		/* Disable profiling */
		if (samples == NULL)
			/* Wasn't turned on */
			return 0;
		
		samples = NULL;
		profil_on = 0;
		
		return 0;
	}
	
	samples = sample_buffer;
	nsamples = size / sizeof *samples;
	pc_offset = offset;
	pc_scale = scale;
	
	profil_on = 1;
	
	return 0;
}

long
profile_frequency (void)
{
	/* 5ms timer */
	return (1000000 / (5 * 1000));
}

# endif /* PROFILING */
