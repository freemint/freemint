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
 * begin:	2000-03-24
 * last change: 2000-03-31
 * 
 * Author: Frank Naumann <fnaumann@freemint.de>
 * 
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 * 
 * 
 * changes since last version:
 * 
 * 2000-03-31:
 * 
 * - initial revision
 * 
 * known bugs:
 * 
 * todo:
 * 
 */

# include "scsidrv.h"
# include "global.h"

# include "arch/scsidrv_emu.h"
# include "mint/proc.h"

# include "cookie.h"	/* cookie handling */
# include "init.h"	/* boot_printf */


static REQDATA emu_scsidrv_ReqData;

static SCSIDRV emu_scsidrv =
{
	0,
	
	emu_scsidrv_In,
	emu_scsidrv_Out,
	emu_scsidrv_InquireSCSI,
	emu_scsidrv_InquireBUS,
	emu_scsidrv_CheckDev,
	emu_scsidrv_RescanBus,
	emu_scsidrv_Open,
	emu_scsidrv_Close,
	emu_scsidrv_Error,
	
	emu_scsidrv_Install,
	emu_scsidrv_Deinstall,
	emu_scsidrv_GetCmd,
	emu_scsidrv_SendData,
	emu_scsidrv_GetData,
	emu_scsidrv_SendStatus,
	emu_scsidrv_SendMsg,
	emu_scsidrv_GetMsg,
	
	&emu_scsidrv_ReqData
};


# ifdef DEBUG_INFO
# define SCSIDRV_DEBUG(x)	FORCE x
# else
# define SCSIDRV_DEBUG(x)	
# endif


ushort scsidrv_installed = 0;

static SCSIDRV *scsidrv = NULL;


long
scsidrv_init (void)
{
	long r;
	
	r = get_toscookie (COOKIE_SCSI, (long *) &scsidrv);
	if (!r && scsidrv)
	{
		scsidrv_installed = scsidrv->version;
		
		emu_scsidrv.version = scsidrv_installed;
		if (emu_scsidrv.version > 0x0101)
			emu_scsidrv.version = 0x0101;
		
		set_cookie (COOKIE_SCSI, (long) &emu_scsidrv);
	}
	else
	{
		scsidrv_installed = 0;
		scsidrv = NULL;
	}
	
	return r;
}

long _cdecl
sys_scsidrv (ushort op,
		long a1, long a2, long a3, long a4,
		long a5, long a6, long a7)
{
	typedef long (*wrap1)(long);
	typedef long (*wrap2)(long, long);
	typedef long (*wrap3)(long, long, long);
	typedef long (*wrap4)(long, long, long, long);
	
	/* only superuser can use this interface */
	if (curproc->euid)
		return EPERM;
	
	if (!scsidrv)
		return ENOSYS;
	
	switch (op)
	{
		/* SCSIDRV exist */
		case 0:
		{
			return E_OK;
		}
		/* In */
		case 1:
		{
			wrap1 f = (wrap1) scsidrv_In;
			return (*f)(a1);
		}
		/* Out */
		case 2:
		{
			wrap1 f = (wrap1) scsidrv_Out;
			return (*f)(a1);
		}
		/* InquireSCSI */
		case 3:
		{
			wrap2 f = (wrap2) scsidrv_InquireSCSI;
			return (*f)(a1, a2);
		}
		/* InquireBus */
		case 4:
		{
			wrap2 f = (wrap2) scsidrv_InquireBus;
			return (*f)(a1, a2);
		}
		/* CheckDev */
		case 5:
		{
			wrap4 f = (wrap4) scsidrv_CheckDev;
			return (*f)(a1, a2, a3, a4);
		}
		/* RescanBus */
		case 6:
		{
			wrap1 f = (wrap1) scsidrv_RescanBus;
			return (*f)(a1);
		}
		/* Open */
		case 7:
		{
			wrap3 f = (wrap3) scsidrv_Open;
			return (*f)(a1, a2, a3);
		}
		/* Close */
		case 8:
		{
			wrap1 f = (wrap1) scsidrv_Close;
			return (*f)(a1);
		}
		/* Error */
		case 9:
		{
			wrap2 f = (wrap2) scsidrv_Error;
			return (*f)(a1, a2);
		}
		
		/* target interface
		 * optional and can't be supported by MiNT
		 */
		case 10:	/* Install */
		case 11:	/* Deinstall */
		case 12:	/* GetCmd */
		case 13:	/* SendData */
		case 14:	/* GetData */
		case 15:	/* SendStat */
		case 16:	/* SendMsg */
		case 17:	/* GetMsg */
			return ENOSYS;
	}
	
	return EBADARG;
}


long
scsidrv_In (SCSICMD *par)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_In (%lx)", par));
	ret = (*scsidrv->In)(par);
	SCSIDRV_DEBUG (("scsidrv_In (...) -> %li", ret));
	return ret;
}

long
scsidrv_Out (SCSICMD *par)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_Out (%lx)", par));
	ret = (*scsidrv->Out)(par);
	SCSIDRV_DEBUG (("scsidrv_Out (...) -> %li", ret));
	return ret;
}

long
scsidrv_InquireSCSI (short what, BUSINFO *info)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_InquireSCSI (%i, %lx)", what, info));
	ret = (*scsidrv->InquireSCSI)(what, info);
	SCSIDRV_DEBUG (("scsidrv_InquireSCSI (...) -> %li", ret));
	return ret;
}

long
scsidrv_InquireBus (short what, short busno, DEVINFO *dev)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_InquireBUS (%i, %i, %lx)", what, busno, dev));
	ret = (*scsidrv->InquireBus)(what, busno, dev);
	SCSIDRV_DEBUG (("scsidrv_InquireBUS (...) -> %li", ret));
	return ret;
}

long
scsidrv_CheckDev (short busno, const llong *SCSIId, char *name, ushort *features)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_CheckDev (%i, %lx, %lx, %lx)", busno, SCSIId, name, features));
	ret = (*scsidrv->CheckDev)(busno, SCSIId, name, features);
	SCSIDRV_DEBUG (("scsidrv_CheckDev (...) -> %li", ret));
	return ret;
}

long
scsidrv_RescanBus (short busno)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_RescanBus (%i)", busno));
	ret = (*scsidrv->RescanBus)(busno);
	SCSIDRV_DEBUG (("scsidrv_RescanBus (...) -> %li", ret));
	return ret;
}

long
scsidrv_Open (short busno, const llong *SCSIId, ulong *maxlen)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_Open (%i, %lx, %lx)", busno, SCSIId, maxlen));
	ret = (*scsidrv->Open)(busno, SCSIId, maxlen);
	SCSIDRV_DEBUG (("scsidrv_Open (...) -> %li", ret));
	return ret;
}

long
scsidrv_Close (short *handle)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_Close (%lx)", handle));
	ret = (*scsidrv->Close)(handle);
	SCSIDRV_DEBUG (("scsidrv_Close (...) -> %li", ret));
	return ret;
}

long
scsidrv_Error (short *handle, short rwflag, short ErrNo)
{
	register long ret;
	SCSIDRV_DEBUG (("scsidrv_Error (%lx, %i, %i)", handle, rwflag, ErrNo));
	ret = (*scsidrv->Error)(handle, rwflag, ErrNo);
	SCSIDRV_DEBUG (("scsidrv_Error (...) -> %li", ret));
	return ret;
}


long
scsidrv_Install (ushort bus, TARGET *handler)
{
	return (*scsidrv->Install)(bus, handler);
}

long
scsidrv_Deinstall (ushort bus, TARGET *handler)
{
	return (*scsidrv->Deinstall)(bus, handler);
}

long
scsidrv_GetCmd (ushort bus, char *cmd)
{
	return (*scsidrv->GetCmd)(bus, cmd);
}

long
scsidrv_SendData (ushort bus, char *buf, ulong len)
{
	return (*scsidrv->SendData)(bus, buf, len);
}

long
scsidrv_GetData (ushort bus, void *buf, ulong len)
{
	return (*scsidrv->GetData)(bus, buf, len);
}

long
scsidrv_SendStatus (ushort bus, ushort status)
{
	return (*scsidrv->SendStatus)(bus, status);
}

long
scsidrv_SendMsg (ushort bus, ushort msg)
{
	return (*scsidrv->SendMsg)(bus, msg);
}

long
scsidrv_GetMsg (ushort bus, ushort *msg)
{
	return (*scsidrv->GetMsg)(bus, msg);
}
