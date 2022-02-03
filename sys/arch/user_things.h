/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 2003 Konrad M. Kokoszkiewicz <draco@atari.org>
 * All rights reserved.
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
 * Started: 26.II.2003.
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _m68k_user_things_h
# define _m68k_user_things_h

# include "mint/mint.h"

/* The `user things' alias trampoline is a part of the kernel procedures and structures
 * which are accessed by applications in user mode and reside in their (applications')
 * memory space. The struct below defines a set of vectors necessary for the kernel to
 * manage all this stuff properly. Everything this is not directly visible for the user
 * (i.e. there is not any pointer in the user space that would point to the struct).
 */

struct user_things
{
	const long	len;		/* number of bytes to copy (this struct and the code) */
	BASEPAGE	*bp;		/* user basepage ptr (warning: UNDEFINED before Slbopen()! */

	/* pointers to functions; they are longs for conveniency
	 * i.e. smaller number of casts
	 */
	unsigned long terminateme_p;		/* a pointer to Pterm() */
	unsigned long sig_return_p;		/* a pointer to Psigreturn() (internal use ONLY) */
	unsigned long pc_valid_return_p;	/* this is signalling a return from a sighandler (internal use ONLY) */

	unsigned long slb_init_and_exit_p;	/* startup code for an SLB library (its `TEXT segment') */
	unsigned long slb_open_p;		/* a pointer to a call to SLB open() function */
	unsigned long slb_close_p;		/* a pointer to a call to SLB close() function */
	unsigned long slb_close_and_pterm_p;	/* a pointer to a call to slb_close_and_pterm() */
	unsigned long slb_exec_p;		/* exec an SLB function */

	unsigned long user_xhdi_p;		/* call the XHDI interface */
# ifdef JAR_PRIVATE
	struct cookie *user_jar_p;	/* user's copy of the Cookie Jar */
# endif

};

extern struct user_things user_things;
extern struct user_things kernel_things;

# endif /* _m68k_user_things_h */
