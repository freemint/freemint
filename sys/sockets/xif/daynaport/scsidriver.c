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
 *	version 2.0		rfb (november 2007)
 *		. rename SCSI_Options() function to SCSI_Modeswitch() for clarity;
 *		  change SCSI_In() and SCSI_Out() to only do a direct call or a
 *		  Supexec() (no Super()).
 *
 *	version 2.1		rfb (september 2015)
 *		the following changes accommodate Uwe Seimet's SCSIDRV driver for
 *		Hatari/Aranym under Linux
 *		. all routines now assume that the underlying driver must be called
 *		  in supervisor mode, and therefore check the value of 'Modeswitch'
 *		. fix bug in SCSI_Init(): if a bus had an id > 15, it was not included
 *		  in the returned bitmap
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

char *version = "SCSIDRV.LIB v2.1 by Roger Burrows, Anodyne Software";

static tpScsiCall scsi = NULL;
static LONG Modeswitch = TRUE;


/*
 *	function prototypes
 */
static LONG call_SCSI_In(void);
static LONG call_SCSI_Out(void);
static LONG call_SCSI_InquireSCSI(void);
static LONG call_SCSI_InquireBus(void);
static LONG call_SCSI_CheckDev(void);
static LONG call_SCSI_RescanBus(void);
static LONG call_SCSI_Open(void);
static LONG call_SCSI_Close(void);
static LONG call_SCSI_Error(void);

/*
 *	initialise interface
 */
ULONG SCSI_Init(char *busnames[BUSNAMELEN])
{
tBusInfo info;
ULONG busses = 0L;
int i, what;

	if (s_system (S_GETCOOKIE, COOKIE_SCSI, (long)&scsi) == 1)	/* if no cookie, no SCSIDRV */
		return 0L;
		
	if (busnames)
		for (i = 0; i < NUMBUSES; i++)
			busnames[i][0] = '\0';

	for (what = cInqFirst; ; what = cInqNext) {
		if (SCSI_InquireSCSI(what,&info) < 0)
			break;
		busses |= (1L<<info.BusNo);
		if (busnames)
			strcpy(busnames[info.BusNo],info.BusName);
	}

	return busses;
}

/*
 *	call this routine to set the Modeswitch flag to TRUE or FALSE
 *
 *	if TRUE (default), the library will switch to supervisor mode
 *	before calling SCSIDRV
 */
LONG SCSI_Modeswitch(LONG required)
{
	Modeswitch = required?TRUE:FALSE;

	return 0L;
}

/*
 *	SCSI_Inquiry() - a derived routine
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
	cmd.SenseBuffer = (BYTE *) reqbuff;
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

/*
 *	low-level routines
 */

/*
 ********** SCSI_In() **********
 */
static tpSCSICmd Inparms;

static LONG call_SCSI_In()
{
	return scsi->In(Inparms);
}

LONG SCSI_In(tpSCSICmd parms)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		Inparms = parms;			/* make a copy for called routine */
		return Supexec(call_SCSI_In);
	}

	return scsi->In(parms);
}

/*
 ********** SCSI_Out() **********
 */
static tpSCSICmd Outparms;

static LONG call_SCSI_Out()
{
	return scsi->Out(Outparms);
}

LONG SCSI_Out(tpSCSICmd parms)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		Outparms = parms;			/* make a copy for called routine */
		return Supexec(call_SCSI_Out);
	}

	return scsi->Out(parms);
}

/*
 ********** SCSI_InquireSCSI() **********
 */
static WORD ISwhat;
static tBusInfo *ISinfo;

static LONG call_SCSI_InquireSCSI()
{
	return scsi->InquireSCSI(ISwhat,ISinfo);
}

LONG SCSI_InquireSCSI(LONG what,tBusInfo *Info)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		ISwhat = (WORD)what;		/* make copies for called routine */
		ISinfo = Info;
		return Supexec(call_SCSI_InquireSCSI);
	}

	return scsi->InquireSCSI((WORD)what,Info);
}

/*
 ********** SCSI_InquireBus() **********
 */
static WORD IBwhat;
static WORD IBbusno;
static tDevInfo *IBdev;

static LONG call_SCSI_InquireBus()
{
	return scsi->InquireBus(IBwhat,IBbusno,IBdev);
}

LONG SCSI_InquireBus(LONG what,LONG BusNo,tDevInfo *Dev)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		IBwhat = (WORD)what;		/* make copies for called routine */
		IBbusno = (WORD)BusNo;
		IBdev = Dev;
		return Supexec(call_SCSI_InquireBus);
	}

	return scsi->InquireBus((WORD)what,(WORD)BusNo,Dev);
}

/*
 ********** SCSI_CheckDev() **********
 */
static WORD CDbusno;
static const DLONG *CDscsiid;
static char *CDname;
static UWORD *CDfeatures;

static LONG call_SCSI_CheckDev()
{
	return scsi->CheckDev(CDbusno,CDscsiid,CDname,CDfeatures);
}

LONG SCSI_CheckDev(LONG BusNo,const DLONG *SCSIId,char *name,UWORD *features)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		CDbusno = (WORD)BusNo;		/* make copies for called routine */
		CDscsiid = SCSIId;
		CDname = name;
		CDfeatures = features;
		return Supexec(call_SCSI_CheckDev);
	}

	return scsi->CheckDev((WORD)BusNo,SCSIId,name,features);
}

/*
 ********** SCSI_RescanBus() **********
 */
static WORD RBbusno;

static LONG call_SCSI_RescanBus()
{
	return scsi->RescanBus(RBbusno);
}

LONG SCSI_RescanBus(LONG BusNo)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		RBbusno = (WORD)BusNo;		/* make copies for called routine */
		return Supexec(call_SCSI_RescanBus);
	}

	return scsi->RescanBus((WORD)BusNo);
}

/*
 ********** SCSI_Open() **********
 */
static WORD SObusno;
static const DLONG *SOscsiid;
static ULONG *SOmaxlen;

static LONG call_SCSI_Open()
{
	return scsi->Open(SObusno,SOscsiid,SOmaxlen);
}

LONG SCSI_Open(LONG BusNo,const DLONG *SCSIId,ULONG *MaxLen)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		SObusno = (WORD)BusNo;		/* make copies for called routine */
		SOscsiid = SCSIId;
		SOmaxlen = MaxLen;
		return Supexec(call_SCSI_Open);
	}

	return scsi->Open((WORD)BusNo,SCSIId,MaxLen);
}

/*
 ********** SCSI_Close() **********
 */
static tHandle SChandle;

static LONG call_SCSI_Close()
{
	return scsi->Close(SChandle);
}

LONG SCSI_Close(tHandle handle)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		SChandle = handle;			/* make copies for called routine */
		return Supexec(call_SCSI_Close);
	}

	return scsi->Close(handle);
}

/*
 ********** SCSI_Error() **********
 */
static tHandle SEhandle;
static WORD SErwflag;
static WORD SEerrno;

static LONG call_SCSI_Error()
{
	return scsi->Error(SEhandle,SErwflag,SEerrno);
}

LONG SCSI_Error(tHandle handle,LONG rwflag,LONG ErrNo)
{
	if (!scsi)
		return ERROR;

	if (Modeswitch) {
		SEhandle = handle;			/* make copies for called routine */
		SErwflag = (WORD)rwflag;
		SEerrno = (WORD)ErrNo;
		return Supexec(call_SCSI_Error);
	}

	return scsi->Error(handle,(WORD)rwflag,(WORD)ErrNo);
}
