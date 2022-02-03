/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Started: 2000-12-05
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _sysv_msg_h
# define _sysv_msg_h

# include "mint/mint.h"
# include "mint/msg.h"


long _cdecl sys_p_msgctl (long msqid, long cmd, struct msqid_ds *buf);
long _cdecl sys_p_msgget (long key, long msgflg);
long _cdecl sys_p_msgsnd (long msqid, const void *msgp, long msgsz, long msgflg);
long _cdecl sys_p_msgrcv (long msqid, void *msgp, long msgsz, long msgtyp, long msgflg);


# endif	/* _sysv_msg_h  */
