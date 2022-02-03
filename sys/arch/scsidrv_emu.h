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
 * Started: 2000-03-31
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 */

# ifndef _m68k_scsidrv_emu_h
# define _m68k_scsidrv_emu_h

# include "mint/mint.h"
# include "scsidrv.h"


long _cdecl emu_scsidrv_In		(SCSICMD *par);
long _cdecl emu_scsidrv_Out		(SCSICMD *par);
long _cdecl emu_scsidrv_InquireSCSI	(short what, BUSINFO *info);
long _cdecl emu_scsidrv_InquireBUS	(short what, short busno, DEVINFO *dev);
long _cdecl emu_scsidrv_CheckDev	(short busno, const DLONG *SCSIId, char *name, ushort *features);
long _cdecl emu_scsidrv_RescanBus	(short busno);
long _cdecl emu_scsidrv_Open		(short busno, const DLONG *SCSIId, ulong *maxlen);
long _cdecl emu_scsidrv_Close		(short *handle);
long _cdecl emu_scsidrv_Error		(short *handle, short rwflag, short ErrNo);
long _cdecl emu_scsidrv_Install		(ushort bus, TARGET *handler);
long _cdecl emu_scsidrv_Deinstall	(ushort bus, TARGET *handler);
long _cdecl emu_scsidrv_GetCmd		(ushort bus, char *cmd);
long _cdecl emu_scsidrv_SendData	(ushort bus, char *buf, ulong len);
long _cdecl emu_scsidrv_GetData		(ushort bus, void *buf, ulong len);
long _cdecl emu_scsidrv_SendStatus	(ushort bus, ushort status);
long _cdecl emu_scsidrv_SendMsg		(ushort bus, ushort msg);
long _cdecl emu_scsidrv_GetMsg		(ushort bus, ushort *msg);


# endif /* _m68k_scsidrv_emu_h */
