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
 * SCSIDRV_MON debug support contributed by Uwe Seimet
 * see <http://www.seimet.de/atari_english.html> for more details
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
# include "libkern/libkern.h"

# include "cookie.h"	/* cookie handling */
# include "init.h"	/* boot_printf */
# include "k_prot.h"

# include "proc.h"


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
# define SCSIDRV_DEBUG(x)	DEBUG (x)
# if 0
# define SCSIDRV_MON
# endif
# else
# define SCSIDRV_DEBUG(x)
# endif


ushort scsidrv_installed = 0;

static SCSIDRV *scsidrv = NULL;


long
scsidrv_init (void)
{
	long r;
	unsigned long t = 0;

# ifdef SCSIDRV_MON
	static void init_scsidrv_mon(void);
	init_scsidrv_mon();
# endif
	r = get_toscookie(COOKIE_SCSI, &t);
	scsidrv = (SCSIDRV *)t;
	if (!r && scsidrv)
	{
		scsidrv_installed = scsidrv->version;

		emu_scsidrv.version = scsidrv_installed;
		if (emu_scsidrv.version > 0x0101)
			emu_scsidrv.version = 0x0101;

		set_toscookie (COOKIE_SCSI, (long) &emu_scsidrv);
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
	if (!suser (get_curproc()->p_cred->ucr))
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
scsidrv_InstallNewDriver (SCSIDRV *newdrv)
{
	long olddrv = NULL;

	if (!scsidrv)
	{
		scsidrv = newdrv;
		scsidrv_installed = scsidrv->version;

		emu_scsidrv.version = scsidrv_installed;
		if (emu_scsidrv.version > 0x0101)
			emu_scsidrv.version = 0x0101;

		set_cookie (NULL, COOKIE_SCSI, (long) &emu_scsidrv);
	}
	else
	{
		if (newdrv->version < 0x0101)
			emu_scsidrv.version = newdrv->version;

		olddrv = (long) scsidrv;
		scsidrv = newdrv;
	}
	return olddrv;
}

long
scsidrv_In (SCSICMD *par)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_In (%p)", par));
	ret = (*scsidrv->In)(par);
	SCSIDRV_DEBUG (("scsidrv_In (...) -> %li", ret));
	return ret;
}

long
scsidrv_Out (SCSICMD *par)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_Out (%p)", par));
	ret = (*scsidrv->Out)(par);
	SCSIDRV_DEBUG (("scsidrv_Out (...) -> %li", ret));
	return ret;
}

long
scsidrv_InquireSCSI (short what, BUSINFO *info)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_InquireSCSI (%i, %p)", what, info));
	ret = (*scsidrv->InquireSCSI)(what, info);
	SCSIDRV_DEBUG (("scsidrv_InquireSCSI (...) -> %li", ret));
	return ret;
}

long
scsidrv_InquireBus (short what, short busno, DEVINFO *dev)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_InquireBUS (%i, %i, %p)", what, busno, dev));
	ret = (*scsidrv->InquireBus)(what, busno, dev);
	SCSIDRV_DEBUG (("scsidrv_InquireBUS (...) -> %li", ret));
	return ret;
}

long
scsidrv_CheckDev (short busno, const DLONG *SCSIId, char *name, ushort *features)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_CheckDev (%i, %p, %p, %p)", busno, SCSIId, name, features));
	ret = (*scsidrv->CheckDev)(busno, SCSIId, name, features);
	SCSIDRV_DEBUG (("scsidrv_CheckDev (...) -> %li", ret));
	return ret;
}

long
scsidrv_RescanBus (short busno)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_RescanBus (%i)", busno));
	ret = (*scsidrv->RescanBus)(busno);
	SCSIDRV_DEBUG (("scsidrv_RescanBus (...) -> %li", ret));
	return ret;
}

long
scsidrv_Open (short busno, const DLONG *SCSIId, ulong *maxlen)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_Open (%i, %p, %p)", busno, SCSIId, maxlen));
	ret = (*scsidrv->Open)(busno, SCSIId, maxlen);
	SCSIDRV_DEBUG (("scsidrv_Open (...) -> %li", ret));
	return ret;
}

long
scsidrv_Close (short *handle)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_Close (%p)", handle));
	ret = (*scsidrv->Close)(handle);
	SCSIDRV_DEBUG (("scsidrv_Close (...) -> %li", ret));
	return ret;
}

long
scsidrv_Error (short *handle, short rwflag, short ErrNo)
{
	register long ret;

	if (!scsidrv)
		return ENOSYS;

	SCSIDRV_DEBUG (("scsidrv_Error (%p, %i, %i)", handle, rwflag, ErrNo));
	ret = (*scsidrv->Error)(handle, rwflag, ErrNo);
	SCSIDRV_DEBUG (("scsidrv_Error (...) -> %li", ret));
	return ret;
}


long
scsidrv_Install (ushort bus, TARGET *handler)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->Install)(bus, handler);
}

long
scsidrv_Deinstall (ushort bus, TARGET *handler)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->Deinstall)(bus, handler);
}

long
scsidrv_GetCmd (ushort bus, char *cmd)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->GetCmd)(bus, cmd);
}

long
scsidrv_SendData (ushort bus, char *buf, ulong len)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->SendData)(bus, buf, len);
}

long
scsidrv_GetData (ushort bus, void *buf, ulong len)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->GetData)(bus, buf, len);
}

long
scsidrv_SendStatus (ushort bus, ushort status)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->SendStatus)(bus, status);
}

long
scsidrv_SendMsg (ushort bus, ushort msg)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->SendMsg)(bus, msg);
}

long
scsidrv_GetMsg (ushort bus, ushort *msg)
{
	if (!scsidrv)
		return ENOSYS;

	return (*scsidrv->GetMsg)(bus, msg);
}


# ifdef SCSIDRV_MON

/* prototypes */

static long   _cdecl scsidrv_mon_In(SCSICMD *);
static long   _cdecl scsidrv_mon_Out(SCSICMD *);
static long   _cdecl scsidrv_mon_InquireSCSI(short, BUSINFO *);
static long   _cdecl scsidrv_mon_InquireBus(short, short, DEVINFO *);
static long   _cdecl scsidrv_mon_CheckDev(short, const DLONG *, char *, ushort *);
static long   _cdecl scsidrv_mon_RescanBus(short);
static long   _cdecl scsidrv_mon_Open(short, const DLONG *, ulong *);
static long   _cdecl scsidrv_mon_Close(short *);
static long   _cdecl scsidrv_mon_Error(short *, short, short);
static long   _cdecl scsidrv_mon_Install(ushort, TARGET *);
# if 0
static long   _cdecl scsidrv_mon_Deinstall(ushort, TARGET *);
static long   _cdecl scsidrv_mon_GetCmd(ushort, char *);
static long   _cdecl scsidrv_mon_SendData(ushort, char *, ulong);
static long   _cdecl scsidrv_mon_GetData(ushort, void *, ulong);
static long   _cdecl scsidrv_mon_SendStatus(ushort, ushort);
static long   _cdecl scsidrv_mon_SendMsg(ushort, ushort);
static long   _cdecl scsidrv_mon_GetMsg(ushort, ushort *);
# endif
static ushort _cdecl scsidrv_mon_TSel(ushort, ushort, ushort);
static ushort _cdecl scsidrv_mon_TCmd(ushort, char *);
static ushort _cdecl scsidrv_mon_TCmdLen(ushort, ushort);
static void   _cdecl scsidrv_mon_TReset(ushort);
static void   _cdecl scsidrv_mon_TEOP(ushort);
static void   _cdecl scsidrv_mon_TPErr(ushort);
static void   _cdecl scsidrv_mon_TPMism(ushort);
static void   _cdecl scsidrv_mon_TBLoss(ushort);
static void   _cdecl scsidrv_mon_TUnknownInt(ushort);

static void scsidrv_mon_prres(SCSICMD *parms, long res);
static void scsidrv_mon_prerr(long res);
static void scsidrv_mon_prparms(SCSICMD *parms);
static long scsidrv_mon_result(long res);


static SCSIDRV *scsiCall = NULL;

static SCSIDRV oldScsiCall;
static SCSIDRV myScsiCall =
{
	0x0000,

	scsidrv_mon_In,
	scsidrv_mon_Out,
	scsidrv_mon_InquireSCSI,
	scsidrv_mon_InquireBus,
	scsidrv_mon_CheckDev,
	scsidrv_mon_RescanBus,
	scsidrv_mon_Open,
	scsidrv_mon_Close,
	scsidrv_mon_Error,
	scsidrv_mon_Install,
# if 0
	scsidrv_mon_Deinstall,
	scsidrv_mon_GetCmd,
	scsidrv_mon_SendData,
	scsidrv_mon_GetData,
	scsidrv_mon_SendStatus,
	scsidrv_mon_SendMsg,
	scsidrv_mon_GetMsg
# endif
};

static TARGET targetHandler =
{
	NULL,
	scsidrv_mon_TSel,
	scsidrv_mon_TCmd,
	scsidrv_mon_TCmdLen,
	scsidrv_mon_TReset,
	scsidrv_mon_TEOP,
	scsidrv_mon_TPErr,
	scsidrv_mon_TPMism,
	scsidrv_mon_TBLoss,
	scsidrv_mon_TUnknownInt
};

struct HandlerInfo
{
	TARGET *handler;
	ushort len;
};

static struct HandlerInfo handlerInfo[32];

static int
installHandler(void)
{
	BUSINFO busInfo;
	int installed = 0;
	long r;

	r = scsiCall->InquireSCSI(cInqFirst, &busInfo);
	while (!r)
	{
		if (busInfo.features & cTarget)
			if (!scsiCall->Install(busInfo.BusNo, &targetHandler))
				installed = 1;

		r = scsiCall->InquireSCSI(cInqNext, &busInfo);
	}

	return installed;
}

static void
init_scsidrv_mon(void)
{
	long r;

	r = get_toscookie (COOKIE_SCSI, (long *) &scsiCall);
	if (r == 0 && scsiCall)
	{
		installHandler();

		memcpy(&oldScsiCall, scsiCall, 38);
		myScsiCall.version = scsiCall->version;

		memcpy(scsiCall, &myScsiCall, 38);

		mint_bzero(handlerInfo, sizeof(handlerInfo));
	}
}

static long _cdecl
scsidrv_mon_In(SCSICMD *parms)
{
	long res;

	SCSIDRV_DEBUG(("In    tpSCSICmd $%p", parms));
	scsidrv_mon_prparms(parms);

	res = oldScsiCall.In(parms);
	if (res <= 0 || res == 2)
		scsidrv_mon_prres(parms, res);
	else
		SCSIDRV_DEBUG(("-> %ld", res));

	return res;
}

static long _cdecl
scsidrv_mon_Out(SCSICMD *parms)
{
	long res;

	SCSIDRV_DEBUG(("Out    tpSCSICmd $%p", parms));
	scsidrv_mon_prparms(parms);

	res = oldScsiCall.Out(parms);
	if (res <= 0 || res == 2)
		scsidrv_mon_prres(parms, res);
	else
		SCSIDRV_DEBUG(("-> %ld", res));

	return res;
}

static long _cdecl
scsidrv_mon_InquireSCSI(short what, BUSINFO *info)
{
	long res;

	SCSIDRV_DEBUG(("InquireSCSI    what %s", what ? "cInqNext" : "cInqFirst"));

	res = oldScsiCall.InquireSCSI(what, info);
	if (res)
		SCSIDRV_DEBUG(("-> %ld", res));
	else
	{
		char buf[256];
		char s[32];

		ksprintf(buf, sizeof(buf), "-> BusName \"%s\"  BusNo %d  Features", info->BusName, info->BusNo);

		if (!(info->features & 0x3f))
		{
			ksprintf(s, sizeof(s), " %d", info->features);
			strcat(buf, s);
		}
		if (info->features & 0x01) strcat(buf, " cArbit");
		if (info->features & 0x02) strcat(buf, " cAllCmds");
		if (info->features & 0x04) strcat(buf, " cTargCtrl");
		if (info->features & 0x08) strcat(buf, " cTarget");
		if (info->features & 0x10) strcat(buf, " cCanDisconnect");
		if (info->features & 0x20) strcat(buf, " cScatterGather");

		ksprintf(s, sizeof(s), "  MaxLen %ld", info->MaxLen);
		strcat(buf, s);

		SCSIDRV_DEBUG(("%s", buf));
	}

	return res;
}

static long _cdecl
scsidrv_mon_InquireBus(short what, short busno, DEVINFO *info)
{
	long res;

	SCSIDRV_DEBUG(("InquireBus    what %s  BusNo %d",
			what ? "cInqNext" : "cInqFirst", busno));

	res = oldScsiCall.InquireBus(what, busno, info);
	if (res)
		SCSIDRV_DEBUG(("-> %ld", res));
	else
		SCSIDRV_DEBUG(("-> SCSIId { %ld,%ld }",
				info->SCSIId.hi, info->SCSIId.lo));

	return res;
}

static long _cdecl
scsidrv_mon_CheckDev(short busno, const DLONG *id, char *name, ushort *features)
{
	long res;

	SCSIDRV_DEBUG(("CheckDev    BusNo %d  SCSIId { %ld,%ld }", busno, id->hi, id->lo));

	res = oldScsiCall.CheckDev(busno, id, name, features);
	if (res < 0)
		scsidrv_mon_prerr(res);
	else
	{
		char buf[256];

		ksprintf(buf, sizeof(buf), "-> %ld  Name \"%s\"  Features", res, name);

		if (!(*features & 0x3f))
		{
			char s[32];

			ksprintf(s, sizeof(s), " %d", *features);
			strcat(buf, s);
		}
		if (*features & 0x01) strcat(buf, " cArbit");
		if (*features & 0x02) strcat(buf, " cAllCmds");
		if (*features & 0x04) strcat(buf, " cTargCtrl");
		if (*features & 0x08) strcat(buf, " cTarget");
		if (*features & 0x10) strcat(buf, " cCanDisconnect");
		if (*features & 0x20) strcat(buf, " cScatterGather");

		SCSIDRV_DEBUG(("%s", buf));
	}

	return res;
}

static long _cdecl
scsidrv_mon_RescanBus(short busno)
{
	SCSIDRV_DEBUG(("RescanBus    BusNo %d", busno));

	return scsidrv_mon_result(oldScsiCall.RescanBus(busno));
}


static long _cdecl
scsidrv_mon_Open(short busno, const DLONG *id, ulong *maxlen)
{
	long res;

	SCSIDRV_DEBUG(("Open    BusNo %d  SCSIId { %ld,%ld }", busno, id->hi, id->lo));

	res = oldScsiCall.Open(busno, id, maxlen);
	if (res <= 0)
		scsidrv_mon_prerr(res);
	else
	{
		char buf[256];
		char s[32];
		short *handle = (short *) res;

		ksprintf(buf, sizeof(buf), "-> Handle $%p  Features", handle);

		if (!(*handle & 0x3f))
		{
			ksprintf(s, sizeof(s), " %d", *handle);
			strcat(buf, s);
		}
		if (*handle & 0x01) strcat(buf, " cArbit");
		if (*handle & 0x02) strcat(buf, " cAllCmds");
		if (*handle & 0x04) strcat(buf, " cTargCtrl");
		if (*handle & 0x08) strcat(buf, " cTarget");
		if (*handle & 0x10) strcat(buf, " cCanDisconnect");
		if (*handle & 0x20) strcat(buf, " cScatterGather");

		ksprintf(s, sizeof(s), "  MaxLen %ld", *maxlen);
		strcat(buf, s);

		SCSIDRV_DEBUG(("%s", buf));
	}

	return res;
}

static long _cdecl
scsidrv_mon_Close(short *handle)
{
	SCSIDRV_DEBUG(("Close    Handle $%p", handle));

	return scsidrv_mon_result(oldScsiCall.Close(handle));
}

static long _cdecl
scsidrv_mon_Error(short *handle, short rwflag, short errno)
{
	char buf[256];
	long res;

	if (rwflag)
	{
		ksprintf(buf, sizeof(buf), "Error    Handle $%p  rwflag cErrWrite  ErrNo", handle);

		if (!(errno & 0x03))
		{
			char s[32];

			ksprintf(s, sizeof(s), " %d", errno);
			strcat(buf, s);
		}
		if (errno & 0x01) strcat(buf, " cErrMediach");
		if (errno & 0x02) strcat(buf, " cErrReset");

		SCSIDRV_DEBUG(("%s", buf));
	}
	else
		SCSIDRV_DEBUG(("Error    Handle $%p  rwflag cErrRead", handle));

	res = oldScsiCall.Error(handle, rwflag, errno);

	ksprintf(buf, sizeof(buf), "-> %ld ", res);
	if (res & 0x01) strcat(buf, " cErrMediach");
	if (res & 0x02) strcat(buf, " cErrReset");
	SCSIDRV_DEBUG(("%s", buf));

	return res;
}

static long _cdecl
scsidrv_mon_Install(ushort bus, TARGET *handler)
{
	SCSIDRV_DEBUG(("Install    Bus %d  Handler %p", bus, handler));

	return oldScsiCall.Install(bus, handler);
}

# if 0
static long _cdecl
scsidrv_mon_Deinstall(ushort bus, TARGET *handler)
{
	SCSIDRV_DEBUG(("Deinstall    Bus %d  Handler %p", bus, handler));

	return oldScsiCall.Deinstall(bus, handler);
}

static long _cdecl
scsidrv_mon_GetCmd(ushort bus, char *cmd)
{
	return oldScsiCall.GetCmd(bus, cmd);
}

static long _cdecl
scsidrv_mon_SendData(ushort bus, char *buffer, ulong Len)
{
	return oldScsiCall.SendData(bus, buffer, Len);
}

static long _cdecl
scsidrv_mon_GetData(ushort bus, void *Buffer, ulong Len)
{
	return oldScsiCall.GetData(bus, Buffer, Len);
}

static long _cdecl
scsidrv_mon_SendStatus(ushort bus, ushort Status)
{
	SCSIDRV_DEBUG(("SendStatus    Bus %d  Status %d", bus, Status));

	return oldScsiCall.SendStatus(bus, Status);
}

static long _cdecl
scsidrv_mon_SendMsg(ushort bus, ushort Msg)
{
	SCSIDRV_DEBUG(("SendMsg    Bus %d  Msg %d", bus, Msg));

	return oldScsiCall.SendMsg(bus, Msg);
}

static long _cdecl
scsidrv_mon_GetMsg(ushort bus, ushort *Msg)
{
	return oldScsiCall.GetMsg(bus, Msg);
}
# endif

static ushort _cdecl
scsidrv_mon_TSel(ushort bus, ushort CSB, ushort CSD)
{
	/* Selektion immer annehmen */
	return !(CSB & 0x04);
}

static ushort _cdecl
scsidrv_mon_TCmd(ushort bus, char *cmd)
{
	TARGET *handler;
	int len;

	len = handlerInfo[bus].len;
	if (!len)
	{
		int c = ((uchar *) cmd)[0];
		switch (c >> 5)
		{
			case 0:  len = 6;	break;
			case 4:  len = 16;	break;
			case 5:  len = 12;	break;
			case 1:
			case 2:  len = 10;	break;
			default: len = 12;	break;
		}
	}

	SCSIDRV_DEBUG(("TCmd    BusNo %d", bus));

	if (len)
	{
		char buf[256];
		long buflen = sizeof(buf);
		char *s = buf;
		int i; long j;

		strcpy(s, "  Cmd $");
		j = strlen(s);
		s += j; buflen -= j;

		for (i = 0; i < len; i++)
		{
			ksprintf(s, buflen, "%s%02x", (i ? ":" : ""), cmd[i] & 0xff);

			j = strlen(s);
			s += j; buflen -= j;
		}

		SCSIDRV_DEBUG(("%s", buf));
	}

	handler = handlerInfo[bus].handler;

	handlerInfo[bus].handler = NULL;
	handlerInfo[bus].len = 0;

	if (handler) return handler->TCmd(bus, cmd);
	else return 0;
}

static ushort _cdecl
scsidrv_mon_TCmdLen(ushort bus, ushort cmd)
{
	TARGET *handler;

	handlerInfo[bus].len = 0;
	handlerInfo[bus].handler = NULL;

	handler = targetHandler.next;
	while (handler)
	{
		if (handler->TCmdLen)
			handlerInfo[bus].len = handler->TCmdLen(bus, cmd);

		if (handlerInfo[bus].len)
		{
			handlerInfo[bus].handler = handler;
			break;
		}
	}

	if (handlerInfo[bus].len)
	{
		SCSIDRV_DEBUG(("TCmdLen    BusNo %d  Cmd %02x -> Len %d",
				bus, cmd, handlerInfo[bus].len));
	}

	return handlerInfo[bus].len;
}

static void _cdecl
scsidrv_mon_TReset(ushort bus)
{
	SCSIDRV_DEBUG(("TReset    BusNo %d", bus));
}

static void _cdecl
scsidrv_mon_TEOP(ushort bus)
{
	SCSIDRV_DEBUG(("TEOP    BusNo %d", bus));
}

static void _cdecl
scsidrv_mon_TPErr(ushort bus)
{
	SCSIDRV_DEBUG(("TPErr    BusNo %d", bus));
}

static void _cdecl
scsidrv_mon_TPMism(ushort bus)
{
	SCSIDRV_DEBUG(("TPMism    BusNo %d", bus));
}

static void _cdecl
scsidrv_mon_TBLoss(ushort bus)
{
	SCSIDRV_DEBUG(("TBLoss    BusNo %d", bus));
}

static void _cdecl
scsidrv_mon_TUnknownInt(ushort bus)
{
	SCSIDRV_DEBUG(("TUnknownInt    BusNo %d", bus));
}

static void
scsidrv_mon_prres(SCSICMD *parms, long res)
{
	switch (res)
	{
		case SELECTERROR:
			SCSIDRV_DEBUG(("-> SELECTERROR"));
			break;
		case STATUSERROR:
			SCSIDRV_DEBUG(("-> STATUSERROR"));
			break;
		case PHASEERROR:
			SCSIDRV_DEBUG(("-> PHASEERROR"));
			break;
		case BSYERROR:
			SCSIDRV_DEBUG(("-> BSYERROR"));
			break;
		case BUSERROR:
			SCSIDRV_DEBUG(("-> BUSERROR"));
			break;
		case TRANSERROR:
			SCSIDRV_DEBUG(("-> TRANSERROR"));
			break;
		case FREEERROR:
			SCSIDRV_DEBUG(("-> FREEERROR"));
			break;
		case TIMEOUTERROR:
			SCSIDRV_DEBUG(("-> TIMEOUTERROR"));
			break;
		case DATATOOLONG:
			SCSIDRV_DEBUG(("-> DATATOOLONG"));
			break;
		case LINKERROR:
			SCSIDRV_DEBUG(("-> LINKERROR"));
			break;
		case TIMEOUTARBIT:
			SCSIDRV_DEBUG(("-> TIMEOUTARBIT"));
			break;
		case PENDINGERROR:
			SCSIDRV_DEBUG(("-> PENDINGERROR"));
			break;
		case PARITYERROR:
			SCSIDRV_DEBUG(("-> PARITYERROR"));
			break;
		case 0x02:
			if (parms->sense)
			{
				SCSIDRV_DEBUG(("-> CHECK CONDITION  SenseKey %d  ASC %d  ASCQ %d",
						parms->sense[2] & 0x0f, parms->sense[12] & 0xff,
						parms->sense[13] & 0xff));
			}
			else
				SCSIDRV_DEBUG(("-> CHECK CONDITION"));
			break;
		case 0x04:
			SCSIDRV_DEBUG(("-> CONDITION MET"));
			break;
		case 0x08:
			SCSIDRV_DEBUG(("-> BUSY"));
			break;
		case 0x10:
			SCSIDRV_DEBUG(("-> INTERMEDIATE"));
			break;
		case 0x14:
			SCSIDRV_DEBUG(("-> INTERMEDIATE-CONDITION MET"));
			break;
		case 0x18:
			SCSIDRV_DEBUG(("-> RESERVATION CONFLICT"));
			break;
		case 0x22:
			SCSIDRV_DEBUG(("-> COMMAND TERMINATED"));
			break;
		case 0x28:
			SCSIDRV_DEBUG(("-> TASK SET FULL"));
			break;
		case 0x30:
			SCSIDRV_DEBUG(("-> ACA ACTIVE"));
			break;
		default:
			SCSIDRV_DEBUG(("-> %ld", res));
			break;
	}
}

static void
scsidrv_mon_prerr(long res)
{
	switch (res)
	{
		case -2:
			SCSIDRV_DEBUG(("-> EDRVNR"));
			break;
		case -15:
			SCSIDRV_DEBUG(("-> EUNDEV"));
			break;
		case -35:
			SCSIDRV_DEBUG(("-> ENHNDL"));
			break;
		case -36:
			SCSIDRV_DEBUG(("-> EACCDN"));
			break;
		default:
			SCSIDRV_DEBUG(("-> %ld", res));
			break;
	}
}

static void
scsidrv_mon_prparms(SCSICMD *parms)
{
	char buf[256];
	long buflen = sizeof(buf);
	char *s = buf;
	int i; long j;

	SCSIDRV_DEBUG(("  Handle $%p", parms->handle));

	ksprintf(s, buflen, "  CmdLen %d  Cmd $", parms->cmdlen);
	j = strlen(s);
	s += j; buflen -= j;

	for (i = 0; i < parms->cmdlen; i++)
	{
		ksprintf(s, buflen, "%s%02x", (i ? ":" : ""), parms->cmd[i] & 0xff);

		j = strlen(s);
		s += j; buflen -= j;

		if (i >= 15)
			break;
	}
	SCSIDRV_DEBUG(("%s", buf));

	SCSIDRV_DEBUG(("  Buffer $%p  TransferLen %ld", parms->buf, parms->transferlen));
	SCSIDRV_DEBUG(("  SenseBuffer $%p  Timeout %ld", parms->sense, parms->timeout));

	if (parms->flags & 0x10)
		SCSIDRV_DEBUG(("  Flags Disconnect"));
	else
		SCSIDRV_DEBUG(("  Flags %d", parms->flags));
}

static long
scsidrv_mon_result(long res)
{
	SCSIDRV_DEBUG(("-> %ld", res));

	return res;
}

# endif
