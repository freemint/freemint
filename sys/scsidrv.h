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


# ifndef _scsidrv_h
# define _scsidrv_h

# include "mint/mint.h"


/* forward definitions
 */

typedef struct scsicmd	SCSICMD;
typedef struct businfo	BUSINFO;
typedef struct scsidrv	SCSIDRV;
typedef struct target	TARGET;
typedef struct dlong	DLONG;
typedef struct devinfo	DEVINFO;


/* exported data
 */

extern ushort scsidrv_installed;


/* exported functions
 */

long	scsidrv_init		(void);

long	_cdecl sys_scsidrv	(ushort op,
				 long a1, long a2, long a3, long a4,
				 long a5, long a6, long a7);

long	scsidrv_In		(SCSICMD *par);
long	scsidrv_Out		(SCSICMD *par);

/* error codes ffor In and Out */

# define NOSCSIERROR		  0L /* no error */
# define SELECTERROR		 -1L /* selection error */
# define STATUSERROR		 -2L /* default error */
# define PHASEERROR		 -3L /* invalid phase */
# define BSYERROR		 -4L /* BSY lost */
# define BUSERROR		 -5L /* bus failure by DMA transfer */
# define TRANSERROR		 -6L /* error during DMA transfer */
# define FREEERROR		 -7L /* bus isn't free */
# define TIMEOUTERROR		 -8L /* timeout */
# define DATATOOLONG		 -9L /* data to long for ACSI softtransfer */
# define LINKERROR		-10L /* error during linked-command (ACSI) sending */
# define TIMEOUTARBIT		-11L /* arbitration timeout */
# define PENDINGERROR		-12L /* pending error on this handle */
# define PARITYERROR		-13L /* parity error during transfer */

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


/* the struct definitions
 */

struct scsicmd
{
	short	*handle;		/* Handle f�r Bus und Ger�t */
	char	*cmd;			/* Zeiger auf CmdBlock */
	ushort	cmdlen;			/* L�nge des Cmd-Block (f�r ACSI n�tig) */
	void	*buf;			/* Datenpuffer */
	ulong	transferlen;		/* �bertragungsl�nge */
	char	*sense;			/* Puffer f�r ReqSense (18 Bytes) */
	ulong	timeout;		/* Timeout in 1/200 sec */
	ushort	flags;			/* Bitvektor f�r Ablaufw�nsche */
# define Disconnect 0x10		/* versuche disconnect */
};

struct businfo
{
	/* abgearbeitete Busnummern
	 * jeder Treiber mu� bei InuireSCSI das
	 * mit seiner Busnummer korrespondierende
	 * Bit setzen.
	 */
	ulong	BusIds;
	char	res [28];	/* f�r Erweiterungen */
	
	char	BusName [20];	/* 'SCSI', 'ACSI', 'PAK-SCSI' */
	ushort	BusNo;		/* Nummer, unter der der Bus anzusprechen ist */
	
	/* bis zu 16 Features, die der Bus kann, zB Arbit,
	 * Full-SCSI (alle SCSI-Cmds im Gegensatz zu ACSI)
	 * Target oder Initiator gesteuert
	 * Ein SCSI-Handle ist auch ein Zeiger auf eine Kopie dieser Information!
	 */
	ushort	features;
# define cArbit		0x01	/* auf dem Bus wird arbitriert */
# define cAllCmds	0x02	/* hier k�nnen ale SCSI-Cmds abgesetzt werden */
# define cTargCtrl	0x04	/* Das Target steuert den Ablauf (so soll's sein!) */
# define cTarget	0x08	/* auf diesem Bus kann man sich als Target installieren */
# define cCanDisconnect	0x10	/* Disconnect ist m�glich */
# define cScatterGather	0x20	/* scatter gather bei virtuellem RAM m�glich */
	
	/* maximale Transferl�nge auf diesem Bus (in Bytes)
	 * entspricht zB bei ACSI der Gr��e des FRB
	 */
	ulong	MaxLen;
};

struct dlong
{
	ulong hi;
	ulong lo;
};

struct devinfo
{
	char priv [32];
	DLONG SCSIId;
};

struct target
{
	TARGET *next;
	ushort	_cdecl (*TSel)		(ushort bus, ushort CSB, ushort CSD);
	ushort	_cdecl (*TCmd)		(ushort bus, char *cmd);
	ushort	_cdecl (*TCmdLen)	(ushort bus, ushort cmd);
	void	_cdecl (*TReset)	(ushort bus);
	void	_cdecl (*TEOP)		(ushort bus);
	void	_cdecl (*TPErr)		(ushort bus);
	void	_cdecl (*TPMism)	(ushort bus);
	void	_cdecl (*TBLoss)	(ushort bus);
	void	_cdecl (*TUnknownInt)	(ushort bus);
};

typedef char REQDATA [18];

struct scsidrv
{
	ushort	version;		/* Revision in BCD: $0100 = 1.00 */
	
	/* Routinen als Initiator */
	long _cdecl (*In)		(SCSICMD *par);
	long _cdecl (*Out)		(SCSICMD *par);
	
	long _cdecl (*InquireSCSI)	(short what, BUSINFO *info);
# define cInqFirst	0
# define cInqNext	1
	long _cdecl (*InquireBus)	(short what, short busno, DEVINFO *dev);
	long _cdecl (*CheckDev)		(short busno, const DLONG *SCSIId, char *name, ushort *features);
	long _cdecl (*RescanBus)	(short busno);
	long _cdecl (*Open)		(short busno, const DLONG *SCSIId, ulong *maxlen);
	long _cdecl (*Close)		(short *handle);
	long _cdecl (*Error)		(short *handle, short rwflag, short ErrNo);
# define cErrRead	0
# define cErrWrite	1
# define cErrMediach	0
# define cErrReset	1
	
	/* Routinen als Target (optional) */
	long _cdecl (*Install)		(ushort bus, TARGET *handler);
	long _cdecl (*Deinstall)	(ushort bus, TARGET *handler);
	long _cdecl (*GetCmd)		(ushort bus, char *cmd);
	long _cdecl (*SendData)		(ushort bus, char *buf, ulong len);
	long _cdecl (*GetData)		(ushort bus, void *buf, ulong len);
	long _cdecl (*SendStatus)	(ushort bus, ushort status);
	long _cdecl (*SendMsg)		(ushort bus, ushort msg);
	long _cdecl (*GetMsg)		(ushort bus, ushort *msg);
	
	/* globale Variablen (f�r Targetroutinen) */
	REQDATA *ReqData;
};


# endif /* _scsidrv_h */
