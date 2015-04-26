/*
 * $Id$
 * 
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
 * Started: 2000-03-24
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

/* Header only for SCSIDRV system call emulation.
 * IMPORTANT: Drivers must use /mint/scsidrv.h
 */

# ifndef _scsidrv_h
# define _scsidrv_h

# include "mint/mint.h"
# include "mint/scsidrv.h"

/* exported data
 */

extern ushort scsidrv_installed;


/* exported functions
 */

long	scsidrv_init		(void);
long	_cdecl sys_scsidrv	(ushort op,
				 long a1, long a2, long a3, long a4,
				 long a5, long a6, long a7);

long	scsidrv_InstallNewDriver	(SCSIDRV *newdrv);

/* SCSIDRV interface functions */
long	scsidrv_In		(SCSICMD *par);
long	scsidrv_Out		(SCSICMD *par);
long	scsidrv_InquireSCSI	(short what, BUSINFO *info);
long	scsidrv_InquireBus	(short what, short BusNo, DEVINFO *dev);
long	scsidrv_CheckDev	(short BusNo, const DLONG *SCSIId, char *Name, ushort *Features);
long	scsidrv_RescanBus	(short BusNo);
long	scsidrv_Open		(short BusNo, const DLONG *SCSIId, ulong *MaxLen);
long	scsidrv_Close		(short *handle);
long	scsidrv_Error		(short *handle, short rwflag, short ErrNo);
long	scsidrv_Install		(ushort bus, TARGET *handler);
long	scsidrv_Deinstall	(ushort bus, TARGET *handler);
long	scsidrv_GetCmd		(ushort bus, char *cmd);
long	scsidrv_SendData	(ushort bus, char *buf, ulong len);
long	scsidrv_GetData		(ushort bus, void *buf, ulong len);
long	scsidrv_SendStatus	(ushort bus, ushort status);
long	scsidrv_SendMsg		(ushort bus, ushort msg);
long	scsidrv_GetMsg		(ushort bus, ushort *msg);


# endif /* _scsidrv_h */
