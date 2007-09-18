/*
 * Daynaport SCSI/Link driver for FreeMiNT.
 * GNU C conversion by Miro Kropacek, <miro.kropacek@gmail.com>
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Roger Burrows.
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
 */

/*
 *	scsidrv.c
 *
 *	FOR LATTICE C.  MUST BE COMPILED WITH SHORT INTS!
 *
 *	version 1.0		rfb (march 1999)
 *		this exists to provide a convenient way for Lattice C code (with
 *		default long ints) to call SCSIDRV routines, some of which expect
 *		short integers on the stack
 *
 *	version 1.1		rfb (july 2001)
 *		. add new routines SCSI_In_Super()/SCSI_Out_Super()
 *		. allow arg to SCSI_Init() to be NULL, in which case the busnames
 *		  are not returned to the caller
 *
 *	version 1.2		rfb (august 2001)
 *		. turn off stack checking in PRJ file
 *		. allow SCSI_Inquiry() to be called from user or supervisor state
 *
 *	version 1.3		rfb (february 2002)
 *		. allow SCSI_In()/SCSI_Out() to be called from user or supervisor
 *		  mode; removed SCSI_In()_Super()/SCSI_Out_Super() (not needed)
 *
 *	version 1.4		rfb (may 2002)
 *		. a new approach to the supervisor mode requirement: use Supexec()
 *		  which works whether called from user or supervisor mode
 *
 *	version 1.5		rfb (may 2002)
 *		. add SCSI_Options() function: initially it is used to specify how
 *		  the supervisor mode requirement is handled
 *
 */
#include <osbind.h>

#include "libkern/libkern.h"
#include "cookie.h"
#include "mint/ssystem.h"

#include "portab.h"
#include "scsidriver.h"

#include "mint/time.h"


#define	ERROR	-1L
#define	OK		0L

char *version = "SCSIDRV.LIB v1.5 by Roger Burrows, Anodyne Software";

tpScsiCall scsi = NULL;
tpSCSICmd Inparms, Outparms;
LONG Options = 0L;

/*
 *	function prototypes
 */
static LONG call_SCSI_In(void);
static LONG call_SCSI_Out(void);

/*
 *	initialise interface
 */
ULONG SCSI_Init(char *busnames[BUSNAMELEN])
{
tBusInfo info;
ULONG busses = 0L;
int i, what;

	if (s_system (S_GETCOOKIE, COOKIE_SCSI, (long)&scsi) == 1)	/* if no cookie, no SCSIDRV */
		return -1L;
		
	if (busnames)
		for (i = 0; i < NUMBUSES; i++)
			busnames[i][0] = '\0';

	for (what = cInqFirst; ; what = cInqNext) {
		if (SCSI_InquireSCSI(what,&info) < 0)
			break;
		busses |= (1<<info.BusNo);
		if (busnames)
			strcpy(busnames[info.BusNo],info.BusName);
	}
	
	return busses;
}

LONG SCSI_Options(LONG options)
{
long save = Options;
int mode;

	if (options < 0L)
		return save;

	if (options&~VALIDBITS_MASK)
		return ERROR;

	mode = (int)(options&MODESWITCH_MASK);
	switch(mode) {
	case USE_SUPER:
	case USE_SUPEXEC:
	case DONT_SWITCH:
		Options &= ~MODESWITCH_MASK;
		Options |= mode;
		break;
	default:
		return ERROR;
	}

	return save;
}

/*
 *	interface routines
 */
LONG SCSI_In(tpSCSICmd parms)
{
LONG rc, is_super;
LONG oldstack = 1L;

	if (!scsi)
		return ERROR;

	switch((int)(Options&MODESWITCH_MASK)) {
	case USE_SUPER:
		is_super = (long) Super(1L);	/* remember current mode */
		if (!is_super)
			oldstack = Super(0L);
		rc = scsi->In(parms);
		if (!is_super)
			Super(oldstack);
		break;
	case USE_SUPEXEC:
		Inparms = parms;			/* make a copy for called routine */
		rc = Supexec(call_SCSI_In);
		break;
	case DONT_SWITCH:
		rc = scsi->In(parms);
		break;
	default:
		rc = ERROR;
	}

	return rc;
}

static LONG call_SCSI_In()
{
	return scsi->In(Inparms);
}

LONG SCSI_Out(tpSCSICmd parms)
{
LONG rc, is_super;
LONG oldstack = 1L;

	if (!scsi)
		return ERROR;

	switch((int)(Options&MODESWITCH_MASK)) {
	case USE_SUPER:
		is_super = (long) Super(1L);	/* remember current mode */
		if (!is_super)
			oldstack = Super(0L);
		rc = scsi->Out(parms);
		if (!is_super)
			Super(oldstack);
		break;
	case USE_SUPEXEC:
		Outparms = parms;			/* make a copy for called routine */
		rc = Supexec(call_SCSI_Out);
		break;
	case DONT_SWITCH:
		rc = scsi->Out(parms);
		break;
	default:
		rc = ERROR;
	}

	return rc;
}

static LONG call_SCSI_Out()
{
	return scsi->Out(Outparms);
}

LONG SCSI_InquireSCSI(LONG what,tBusInfo *Info)
{
	if (!scsi)
		return ERROR;

	return scsi->InquireSCSI((WORD)what,Info);
}

LONG SCSI_InquireBus(LONG what,LONG BusNo,tDevInfo *Dev)
{
	if (!scsi)
		return ERROR;

	return scsi->InquireBus((WORD)what,(WORD)BusNo,Dev);
}

LONG SCSI_CheckDev(LONG BusNo,const DLONG *SCSIId,char *name,UWORD *features)
{
	if (!scsi)
		return ERROR;

	return scsi->CheckDev((WORD)BusNo,SCSIId,name,features);
}

LONG SCSI_RescanBus(LONG BusNo)
{
	if (!scsi)
		return ERROR;

	return scsi->RescanBus((WORD)BusNo);
}

LONG SCSI_Open(LONG BusNo,const DLONG *SCSIId,ULONG *MaxLen)
{
	if (!scsi)
		return ERROR;

	return scsi->Open((WORD)BusNo,SCSIId,MaxLen);
}

LONG SCSI_Close(tHandle handle)
{
	if (!scsi)
		return ERROR;

	return scsi->Close(handle);
}

LONG SCSI_Error(tHandle handle,LONG rwflag,LONG ErrNo)
{
	if (!scsi)
		return ERROR;

	return scsi->Error(handle,(WORD)rwflag,(WORD)ErrNo);
}

/*
 *	derived routines
 */
LONG SCSI_Inquiry(LONG BusNo,const DLONG *scsiid,LONG lun,char *inqdata)
{
int i;
LONG rc;
ULONG maxlen;
tSCSICmd cmd;
BYTE cdb[6] = { 0x12, 0, 0, 0, 32, 0 };
WORD reqbuff[18/2];	/* even alignment for safety */

	cdb[1] = lun<<5;
	if ((rc=SCSI_Open(BusNo,scsiid,&maxlen)) < 0L)
		return ERROR;

	cmd.Handle = (tHandle) rc;
	cmd.Cmd = cdb;
	cmd.CmdLen = 6;
	cmd.Buffer = inqdata;
	cmd.TransferLen = 32;
	cmd.SenseBuffer = (char *) reqbuff;
	cmd.Timeout = 1 * CLOCKS_PER_SEC;
	cmd.Flags = 0;
	for (i = 0; i < 5; i++)
		if ((rc=SCSI_In(&cmd)) != 2)
			break;

	if (SCSI_Close(cmd.Handle) < 0L)
		return ERROR;

	if (rc != 0L)
		return ERROR;

	return OK;
}
