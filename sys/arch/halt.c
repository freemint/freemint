/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution.
 * 
 * 
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2003-12-13
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# include <stdarg.h>
# include "halt.h"
# include "global.h"

# include "mint/asm.h"
# include "mint/proc.h"

# include "cookie.h"
# include "info.h"	/* MSG_* */
# include "init.h"	/* restore_intr */
# include "intr.h"	/* reboot */
# include "proc.h"
# include "filesys.h"

# include <osbind.h>


void
hw_poweroff(void)
{
# ifdef ARANYM
	nf_shutdown();
# else
	/* CT60 poweroff */
	long int dummy;

	if (get_cookie(NULL, COOKIE_CT60, &dummy) == E_OK)
	{
		/* any write to that address causes poweroff */
		*(volatile char *) 0xFA800000L = 1;
	
		/* does not return */ 
	}
# endif
}

EXITING
hw_coldboot(void)
{
	/* Invalidate the magic values TOS uses to
	 * detect a warm boot
	 */
	*(long *) 0x00000420L = 0L;
	*(long *) 0x0000043aL = 0L;
	
	/* does not return */
	reboot();
}

EXITING
hw_warmboot(void)
{
	/* does not return */
	reboot();
}

/* uk: a normal halt() function without an error message. This function
 *     may only be called if all processes are shut down and the file
 *     systems are synced.
 */
EXITING
hw_halt(void)
{
	long r;
	long key;
	int scan;
	
	DEBUG(("halt() called, system halting...\r\n"));
	debug_ws(MSG_system_halted);
	
	sys_q[READY_Q] = 0;	/* prevent context switches */
	restr_intr();		/* restore interrupts to normal */
	
	for (;;)
	{
		/* get a key; if ctl-alt then do it, else halt */
		if (mcpu == 60)
			cpu_lpstop();
		else
			cpu_stop();

		key = Bconin(out_device);
		
		if ((key & 0x0c000000L) == 0x0c000000L)
		{
			scan = (int)((key >> 16) & 0xff);
			do_func_key(scan);
		}
		else
			break;
	}
	
	for (;;)
	{
		debug_ws(MSG_system_halted);
		
		if (mcpu == 60)
			cpu_lpstop();
		else
			cpu_stop();

		r = Bconin(2);
		if ((r & 0x0ff) == 'x')
		{
		}
	}
}

EXITING 
HALT (void)
{
	long r;
	long key;
	int scan;
	
	DEBUG(("Fatal MiNT error: adjust debug level and hit a key...\r\n"));
	debug_ws(MSG_fatal_reboot);
	
	sys_q[READY_Q] = 0;	/* prevent context switches */
	restr_intr ();		/* restore interrupts to normal */
	
	for (;;)
	{
		/* get a key; if ctl-alt then do it, else halt */
		if (mcpu == 60)
			cpu_lpstop();
		else
			cpu_stop();

		key = Bconin(2);
		
		if ((key & 0x0c000000L) == 0x0c000000L)
		{
			scan = (int)((key >> 16) & 0xff);
			do_func_key(scan);
		}
		else
			break;
	}
	
	for (;;)
	{
		debug_ws(MSG_fatal_reboot);

		if (mcpu == 60)
			cpu_lpstop();
		else
			cpu_stop();

		r = Bconin(2);
		
		if (((r & 0x0ff) == 'x') || ((r & 0xff) == 's'))
		{
			close_filesys();
			
			/* if the user pressed 's', try to sync before halting the system */
			if ((r & 0xff) == 's')
			{
				debug_ws(MSG_debug_syncing);
				sys_s_ync();
				debug_ws(MSG_debug_syncdone);
			}
		}
	}
}
