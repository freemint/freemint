/*
 * USB Storage SCSIDRV Implementation (C) 2014-2015.
 * By Alan Hourihane <alanh@fairlite.co.uk>
 * Improved by Claude Labelle
 */

#include "../../global.h"
#include "scsi.h"
#include "part.h"
#include "../../usb.h"
#include "../../usb_api.h"
#include "usb_storage.h"

extern struct mass_storage_dev mass_storage_dev[USB_MAX_STOR_DEV];
extern block_dev_desc_t usb_dev_desc[MAX_TOTAL_LUN_NUM];

extern long usb_stor_get_info(struct usb_device *, struct us_data *, block_dev_desc_t *);
extern void part_init(long dev_num, block_dev_desc_t *stor_dev);

extern void usb_stor_eject (long);
extern long usb_request_sense (ccb *srb, struct us_data *ss);

#define USBNAME "USB Mass Storage"
#define MAX_HANDLES 32

//#define DEBUGSCSIDRV
#ifdef DEBUGSCSIDRV
#ifdef TOSONLY
#define debug(a) (void)Cconws(a)
#else
#define debug(a) DEBUG((a))
#endif
#else
#define debug(a)
#endif


#include "mint/scsidrv.h"

#ifdef TOSONLY
typedef struct
{
	long ident;
	union
	{
		long l;
		short i[2];
		char c[4];
	} v;
} COOKIE;

static COOKIE *
findcookie (void)
{
	COOKIE *p;
	long stack;
	stack = 0;
	if (Super (1L) >= 0)
		stack = Super (0L);
	p = *(COOKIE **) 0x5a0;
	if (stack)
		SuperToUser ((void *) stack);
	if (!p)
		return ((COOKIE *) 0);
	return (p);
}

static COOKIE *
ncookie (COOKIE * p)
{
	if (!p->ident)
		return (0);
	return (++p);
}

static int
add_cookie (COOKIE * cook)
{
	COOKIE *p;
	int i = 0;
	p = findcookie ();
	while (p)
	{
		if (p->ident == cook->ident)
		{
			*p = *cook;
			return (0);
		}
		if (!p->ident)
		{
			if (i + 1 < p->v.l)
			{
				*(p + 1) = *p;
				*p = *cook;
				return (0);
			}
			else
			{
				c_conws("Failed to add SCSIDRV cookie, no slots left. Install bigger cookie jar.\r\n");
				return (-2);	/* problem */
			}
		}
		i++;
		p = ncookie (p);
	}

	c_conws("Failed to add SCSIDRV cookie, no slots left. Install bigger cookie jar.\r\n");
	return (-1);				/* no cookie-jar */
}

long ssp;
static COOKIE SCSIDRV_COOKIE;

/* The SCSI driver routines use D0–D2 and A0–A1. D2 is the delicate one, since it
 * isn’t a GCC scratch register, so we should save it before calling InquireSCSI().
 * Note that this is only necessary for the TOS driver. On MiNT, the call goes
 * through the kernel, and D2 should be saved and restored there.
 */

static long
old_InquireSCSI(void *func, short what, BUSINFO *info)
{
	register long ret __asm__("d0");

	__asm__ volatile
	(
		"move.l	%3,-(%%sp)\n\t"
		"move.w	%2,-(%%sp)\n\t"
		"move.l	%1,%%a0\n\t"
		"jsr	(%%a0)\n\t"
		"addql	#6,%%sp\n\t"
		: "=r"(ret)				/* outputs */
		: "g"(func), "g"(what), "g"(info)	/* inputs  */
		: __CLOBBER_RETURN("d0")
		  "d1", "d2", "a0", "a1", "cc",		/* clobbered regs */
		  "memory"
	);
	return ret;
}
#endif /* TOSONLY */

static REQDATA reqdata;

typedef struct SCSIDRV_Data
{
	ushort features; /* this has to be at the top ! */
	short changed;
	short handleOpened;
	short devID;
} SCSIDRV_Data;

static SCSIDRV_Data* private = NULL;
static SCSIDRV scsidrv;
static SCSIDRV oldscsi;
static unsigned short USBbus = 4; /* default */

/*
 * SCSIDRV handlers
 */

static long
SCSIDRV_Install (ushort bus, TARGET *handler)
{
	return 0;
}

static long
SCSIDRV_Deinstall (ushort bus, TARGET *handler)
{
	return 0;
}

static long
SCSIDRV_GetCmd (ushort bus, char *cmd)
{
	return 0;
}

static long
SCSIDRV_SendData (ushort bus, char *buf, ulong len)
{
	return 0;
}

static long
SCSIDRV_GetData (ushort bus, void *buf, ulong len)
{
	return 0;
}

static long
SCSIDRV_SendStatus (ushort bus, ushort status)
{
	return 0;
}

static long
SCSIDRV_SendMsg (ushort bus, ushort msg)
{
	return 0;
}

static long
SCSIDRV_GetMsg (ushort bus, ushort * msg)
{
	return 0;
}

static long
SCSIDRV_In (SCSICMD *parms)
{
	SCSIDRV_Data *priv = NULL;
	long i, dev;

	debug ("IN\r\n");

	for (i = 0; i < MAX_HANDLES; i++) {
		if (&private[i] == (SCSIDRV_Data *) parms->handle) {
			priv = (SCSIDRV_Data *) parms->handle;
			break;
		}
	}

	if (priv)
	{
		if (! priv->handleOpened) {
			return EBADF;
		}

		dev = priv->devID;

		if (mass_storage_dev[dev].target != 0xff)
		{
			struct us_data *ss = &mass_storage_dev[dev].usb_stor;
			long retries = 0;
			ccb srb;
			long r;

			if (parms->cmdlen > 16) {
				return STATUSERROR;
			}

			/* Note for SCSI_REQ_SENSE command: Last sense data will be in the */
			/* data buffer. Sense data for the command itself, if it failed, */
			/* will be in the sense buffer, */

			/* Filter commands for non existent LUNs */
			if (((parms->cmd[1] & 0xE0) >> 5 ) > mass_storage_dev[dev].total_lun) {
				return  SELECTERROR;
			}

			memset (&srb, 0, sizeof (srb));
			for (i = 0; i < parms->cmdlen; i++)
			{
				srb.cmd[i] = parms->cmd[i];
			}

			srb.cmdlen = parms->cmdlen;
			srb.datalen = parms->transferlen;
			srb.pdata = parms->buf;
			srb.lun = (parms->cmd[1] & 0xE0) >> 5;
			srb.direction = USB_CMD_DIRECTION_IN;
			srb.timeout = parms->timeout * 5;

#if 0
			c_conws ("SCSIPACKET\r\n");
			hex_long (srb.cmd[0]);
			c_conout(' ');
			hex_long (srb.cmd[1]);
			c_conout(' ');
			hex_long (srb.cmd[2]);
			c_conout(' ');
			hex_long (srb.cmd[3]);
			c_conout(' ');
			hex_long (srb.cmd[4]);
			c_conout(' ');
			hex_long (srb.cmd[5]);
			c_conws ("\r\n");
			hex_long (srb.cmdlen);
			c_conws ("\r\n");
			hex_long (srb.datalen);
			c_conws ("\r\n");
#endif

			if (srb.cmd[0] == SCSI_INQUIRY)
				retries = 3;

			if (srb.cmd[0] == SCSI_RD_CAPAC ||
			    srb.cmd[0] == SCSI_RD_CAPAC16) {
				ccb pccb;

				retries = 5;

				memset(&pccb.cmd[0], 0, 12);
				pccb.cmd[0] = SCSI_TST_U_RDY;
				pccb.datalen = 0;
				pccb.cmdlen = 12;
				pccb.lun = srb.lun;
				r = ss->transport(&pccb, ss);
				if (r == USB_STOR_TRANSPORT_FAILED
					|| r == USB_STOR_TRANSPORT_DATA_FAILED
					|| r == USB_STOR_TRANSPORT_ERROR)
					return STATUSERROR;
				else if(r == USB_STOR_TRANSPORT_SENSE) {
					usb_request_sense(&srb, ss);
					memcpy(parms->sense, srb.sense_buf, 18);
					return S_CHECK_COND;
				}
			}
			
			/* an EJECT command? */
			if (srb.cmd[0] == SCSI_START_STP &&
				srb.cmd[4] & SCSI_START_STP_LOEJ &&
				!(srb.cmd[4] & SCSI_START_STP_START) &&
				!(srb.cmd[4] & SCSI_START_STP_PWCO))
			{
				usb_stor_eject(mass_storage_dev[dev].usb_dev_desc[srb.lun]->usb_logdrv);
			}

			if (srb.cmd[0] == SCSI_TST_U_RDY) {
				retries = 10;
			}

			if (srb.cmd[0] == SCSI_REPORT_LUN) {
				return SELECTERROR;
			}

			/* promote read6 to read10 */
			if (srb.cmd[0] == SCSI_READ6)
			{
				long block;
				srb.cmd[0] = SCSI_READ10;
				block = (long) (srb.cmd[1] & 0x1f) << 16 |
					(long) srb.cmd[2] << 8 | (long) srb.cmd[3];
                		/* do 4 & 5 here as we overwrite them later */
				srb.cmd[8] = srb.cmd[4];
				srb.cmd[9] = srb.cmd[5];
				srb.cmd[2] = (block >> 24) & 0xff;
				srb.cmd[3] = (block >> 16) & 0xff;
				srb.cmd[4] = (block >> 8) & 0xff;
				srb.cmd[5] = block & 0xff;
				srb.cmd[6] = 0;
				srb.cmd[7] = 0;
				srb.cmdlen = 10;
			}

			/* XXXX: Needs verification !!!!!
			 */
			/* This failed sdrvtest on a USB key. */
			/* priv->changed is set in part_init() in usb_storage.c */
			/* by SCSIDRV_MediaChange but is not need to detect media changes */
#if 0
			if (srb.cmd[0] == SCSI_TST_U_RDY && priv->changed) {
				/* Report Media Change sense key */
				/* 2 = sense key (bits 0 to 3) */
				/* 12 = ASC. 28h = media was inserted. */
				/* 13 = ASCQ. Set to 00h */
				parms->sense[2] = SENSE_UNIT_ATTENTION;
				parms->sense[12] = 0x28;
				parms->sense[13] = 0x00;
				priv->changed = FALSE;
				return S_CHECK_COND;
			}
#endif


retry:
			r = ss->transport (&srb, ss);
			
			if (r == USB_STOR_TRANSPORT_SENSE) {
				usb_request_sense(&srb, ss);
				memcpy(parms->sense, srb.sense_buf, 18);
				return S_CHECK_COND;
			}
			
			if (r != USB_STOR_TRANSPORT_GOOD && retries--) {
				goto retry;
			}
			
			switch(r)
			{
				case USB_STOR_TRANSPORT_GOOD :
					return NOSCSIERROR;
				case USB_STOR_TRANSPORT_DATA_FAILED :
					return TRANSERROR;
				case USB_STOR_TRANSPORT_PHASE_ERROR :
					return PHASEERROR;
				case USB_STOR_TRANSPORT_TIMEOUT :
					return TIMEOUTERROR;
				default:
					return STATUSERROR;
			}
			
#if 0		/* Fix up INQUIRY NUL bytes to spaces. */
			/* Doesn't seem to be needed. */
			/* If so, move higher, this code isn't executed. */
			if (srb.cmd[0] == SCSI_INQUIRY)
			{
				for (i = 8; i < ((srb.datalen < 44) ? srb.datalen : 44); i++)
				{
					if (srb.pdata[i] == 0)
					{
						srb.pdata[i] = ' ';
					}
				}
			}
#endif
		}
		return SELECTERROR;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.In (parms);
		}
	}

	return SELECTERROR;
}

static long
SCSIDRV_Out (SCSICMD *parms)
{
	SCSIDRV_Data *priv = NULL;
	long i, dev;

	debug ("OUT\r\n");

	for (i = 0; i < MAX_HANDLES; i++) {
		if (&private[i] == (SCSIDRV_Data *) parms->handle) {
			priv = (SCSIDRV_Data *) parms->handle;
			break;
		}
	}

	if (priv)
	{
		if (! priv->handleOpened) {
			return EBADF;
		}

		dev = priv->devID;

		if (mass_storage_dev[dev].target != 0xff)
		{
			struct us_data *ss = &mass_storage_dev[dev].usb_stor;
			long retries = 0;
			ccb srb;
			long r;

			if (parms->cmdlen > 16) {
				return STATUSERROR;
			}

			/* Filter commands for non existent LUNs */
			if (((parms->cmd[1] & 0xE0) >> 5 ) > mass_storage_dev[dev].total_lun) {
				return  SELECTERROR;
			}

			memset (&srb, 0, sizeof (srb));
			for (i = 0; i < parms->cmdlen; i++)
			{
				srb.cmd[i] = parms->cmd[i];
			}

			srb.cmdlen = parms->cmdlen;
			srb.datalen = parms->transferlen;
			srb.pdata = parms->buf;
			srb.lun = (parms->cmd[1] & 0xE0) >> 5;
			srb.direction = USB_CMD_DIRECTION_OUT;
			srb.timeout = parms->timeout * 10;

			/* promote write6 to write10 */
			if (srb.cmd[0] == SCSI_WRITE6)
			{
				long block;
				srb.cmd[0] = SCSI_WRITE10;
				block = (long) (srb.cmd[1] & 0x1f) << 16 |
					(long) srb.cmd[2] << 8 | (long) srb.cmd[3];

                		/* do 4 & 5 here as we overwrite them later */
				srb.cmd[8] = srb.cmd[4];
				srb.cmd[9] = srb.cmd[5];
				srb.cmd[2] = (block >> 24) & 0xff;
				srb.cmd[3] = (block >> 16) & 0xff;
				srb.cmd[4] = (block >> 8) & 0xff;
				srb.cmd[5] = block & 0xff;
				srb.cmd[6] = 0;
				srb.cmd[7] = 0;
				srb.cmdlen = 10;
			}

retry:
			r = ss->transport (&srb, ss);
			
			if (r == USB_STOR_TRANSPORT_SENSE) {
				usb_request_sense(&srb, ss);
				memcpy(parms->sense, srb.sense_buf, 18);
				return S_CHECK_COND;
			}

			if (r != USB_STOR_TRANSPORT_GOOD && retries--) {
				goto retry;
			}
			
			switch(r)
			{
				case USB_STOR_TRANSPORT_GOOD :
					return NOSCSIERROR;
				case USB_STOR_TRANSPORT_DATA_FAILED :
					return TRANSERROR;
				case USB_STOR_TRANSPORT_PHASE_ERROR :
					return PHASEERROR;
				case USB_STOR_TRANSPORT_TIMEOUT :
					return TIMEOUTERROR;
				default:
					return STATUSERROR;
			}
		}
		else
		{
			return SELECTERROR;
		}
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.Out (parms);
		}
	}
	return SELECTERROR;
}

static long
SCSIDRV_InquireSCSI (short what, BUSINFO * info)
{
	long ret;
	int i;

	debug ("INQSCSI\r\n");

	if (what == cInqFirst) {
		info->busids = 0;
		/* zero private bytes */
		for (i = 0;i < 28;i++)
			info->res[i] = 0;
	}

	/* We call the previous driver to get its bus ids */
	if (oldscsi.version)
	{
		ret = oldscsi.InquireSCSI (what, info);
		if (ret == 0)
			return 0;
	}

	/*
	 * We assign our BUS id no. 4
	 * We shouldn't fail here as we scanned the busses when we installed
	 * so our USBbus number should be valid.
	 */
	if (!(info->busids & (1<<USBbus)))
	{
		strncpy (info->busname, USBNAME, sizeof(info->busname));
		info->busids |= 1<<USBbus;
		info->busno = USBbus;
		info->features = cAllCmds;
		info->maxlen = 64L * 1024L;
		return 0;
	}

	return ENODEV;
}


static long
SCSIDRV_InquireBus (short what, short busno, DEVINFO * dev)
{
	static long inqbusnext;
	long ret;

	debug ("INQBUS\r\n");

	if (what == cInqFirst)
	{
		inqbusnext = 0;
	}

	if (oldscsi.version)
	{
		ret = oldscsi.InquireBus (what, busno, dev);
		if (ret == 0)
			return 0;
	}

	if (busno == USBbus)
	{
		memset (dev->priv, 0, 32);
		if (inqbusnext >= USB_MAX_STOR_DEV)
		{
			return -1;
		}

again:
		if (mass_storage_dev[inqbusnext].target == 0xff)
		{
			inqbusnext++;
			if (inqbusnext >= USB_MAX_STOR_DEV)
			{
				return -1;
			}
			goto again;
		}

		dev->SCSIId.hi = 0;
		dev->SCSIId.lo = inqbusnext;
		inqbusnext++;
		return 0;
	}

	return -1;
}

static long
SCSIDRV_CheckDev (short busno,
					  const DLONG * DevNo, char *Name, ushort * Features)
{
	debug ("CHECKDEV\r\n");

	if (busno == USBbus)
	{
		memset (Name, 0, 20);
		strcat (Name, USBNAME);
		*Features = 0;
		if (DevNo->hi != 0L)
		{
			return ENODEV;
		}
		if (DevNo->lo >= USB_MAX_STOR_DEV)
		{
			return ENODEV;
		}
		if (mass_storage_dev[DevNo->lo].target != 0xff)
		{
			*Features = cAllCmds;
			return 0;
		} 
		return ENODEV;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.CheckDev (busno, DevNo, Name, Features);
		}
	}
	return ENODEV;
}

static long
SCSIDRV_RescanBus (short busno)
{
	debug ("RESCAN\r\n");
	if (busno == USBbus)
	{
		return 0;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.RescanBus (busno);
		}
	}
	return -1;
}

static long
SCSIDRV_Open (short bus, const DLONG * Id, ulong * MaxLen)
{
	SCSIDRV_Data *priv;
	long i;

	debug ("OPEN\r\n");

	if (bus == USBbus)
	{
		if (Id->hi != 0)
			return ENODEV;

		if (Id->lo >= USB_MAX_STOR_DEV)
			return ENODEV;

		/* Find a free handle*/
		for (i = 0; i < MAX_HANDLES; i++)
			if (! private[i].handleOpened)
				break;

		if (i >= MAX_HANDLES)
			return EMFILE;

		priv = &private[i];
		priv->changed = FALSE;
		priv->handleOpened = TRUE;
		priv->devID = Id->lo;

		if (mass_storage_dev[Id->lo].target != 0xff)
		{
			struct us_data *ss = &mass_storage_dev[Id->lo].usb_stor;

			/* We only handle certain protocols. */
			if (ss->subclass != US_SC_UFI &&
				ss->subclass != US_SC_SCSI &&
				ss->subclass != US_SC_8020 &&
				ss->subclass != US_SC_8070)
			{
				return ENODEV;
			}
		} else {
			return ENODEV;
		}
		*MaxLen = 64 * 1024L;
		return (long) priv;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.Open (bus, Id, MaxLen);
		}
	}
	return ENODEV;
}

static long
SCSIDRV_Close (short *handle)
{
	SCSIDRV_Data *priv = NULL;
	long i;

	debug ("CLOSE\r\n");

	for (i = 0; i < MAX_HANDLES; i++)
		if (&private[i] == (SCSIDRV_Data *) handle)
			break;

	if (i < MAX_HANDLES)
		priv = &private[i];

	if (priv)
	{
		if (priv->handleOpened) {
			priv->handleOpened = FALSE;
			priv->changed = FALSE;

			return 0;
		} else {
			return EMFILE;
		}
	} else {
		if (oldscsi.version) {
			return oldscsi.Close (handle);
		}
	}

	return EMFILE;
}

static long
SCSIDRV_Error (short *handle, short rwflag, short ErrNo)
{
	SCSIDRV_Data *priv = NULL;
	long i;
	long dev;

	debug ("ERROR\r\n");

	for (i = 0; i < MAX_HANDLES; i++) {
		if (&private[i] == (SCSIDRV_Data *) handle) {
			priv = (SCSIDRV_Data *) handle;
			break;
		}
	}

	if (priv)
	{
		dev = priv->devID;

		if (rwflag == cErrRead)
		{
			ushort status = priv->changed;
			priv->changed = FALSE;
			return status;
		}
		else if (rwflag == cErrWrite)
		{
			if (ErrNo == cErrMediach) {
				/* Report Media Change to storage driver */
				usb_stor_eject(dev);
				usb_dev_desc[dev].sw_ejected = 0;
				if (usb_stor_get_info(usb_dev_desc[dev].priv, &mass_storage_dev[usb_dev_desc[dev].usb_phydrv].usb_stor, &usb_dev_desc[dev]) > 0)
						part_init(dev, &usb_dev_desc[dev]);
				/* Report Media Change to all opened handles on this device */
				for (i = 0; i < MAX_HANDLES; i++) {
					priv = (SCSIDRV_Data *) &private[i];
					if (priv->devID == dev && priv->handleOpened) {
						priv->changed = TRUE;
					}
				}
			}
		}
		return 0;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.Error (handle, rwflag, ErrNo);
		}
	}

	return SELECTERROR;
}

void install_scsidrv (void);
void
install_scsidrv (void)
{
	short i;

	scsidrv.version = 0x0101;
	scsidrv.In = SCSIDRV_In;
	scsidrv.Out = SCSIDRV_Out;
	scsidrv.InquireSCSI = SCSIDRV_InquireSCSI;
	scsidrv.InquireBus = SCSIDRV_InquireBus;
	scsidrv.CheckDev = SCSIDRV_CheckDev;
	scsidrv.RescanBus = SCSIDRV_RescanBus;
	scsidrv.Open = SCSIDRV_Open;
	scsidrv.Close = SCSIDRV_Close;
	scsidrv.Error = SCSIDRV_Error;

	/* optional - SCSIDRV2 */
	scsidrv.Install = SCSIDRV_Install;
	scsidrv.Deinstall = SCSIDRV_Deinstall;
	scsidrv.GetCmd = SCSIDRV_GetCmd;
	scsidrv.SendData = SCSIDRV_SendData;
	scsidrv.GetData = SCSIDRV_GetData;
	scsidrv.SendStatus = SCSIDRV_SendStatus;
	scsidrv.SendMsg = SCSIDRV_SendMsg;
	scsidrv.GetMsg = SCSIDRV_GetMsg;
	scsidrv.ReqData = &reqdata;
	
	/* Allocate globally accessible memory for SCSIDRV handles.
	 * As per SCSIDRV spec the caller is allowed to read the memory pointed to by the handle.
	 */
#ifndef TOSONLY
	private = (SCSIDRV_Data*)m_xalloc(MAX_HANDLES * sizeof(SCSIDRV_Data), 0x20|0);
#else
	private = (SCSIDRV_Data*)Malloc(MAX_HANDLES * sizeof(SCSIDRV_Data));
#endif
	if (private == NULL)
		return;

	for (i = 0; i < MAX_HANDLES; i++)
	{
		private[i].features = cAllCmds;
		private[i].changed = FALSE;
		private[i].handleOpened = FALSE;
	}

#ifdef TOSONLY
	SCSIDRV *tmp = NULL;
	if (getcookie (COOKIE_SCSI, (long *)&tmp))
	{
		static BUSINFO info[32];
		long ret;
		oldscsi.version = 0;

		/* Scan for busses reported by previous driver */

		ret = old_InquireSCSI(tmp->InquireSCSI, cInqFirst, info);

		while (ret == 0)
		{
			ret = old_InquireSCSI(tmp->InquireSCSI, cInqNext, info);
		}

		/*
		 * Find a free busno for USB bus. We use a fixed bus number of 4.
		 * If it's occupied, we don't install.
		 */

		if (info->busids & 1<<USBbus)
		{
			c_conws("Bus ID 4 already exists. SCSIDRV not installed.\r\n");
			return;
		}
		/* Take a copy of the old pointers, and replace with ours.
		 * This way we don't delete and replace the existing cookie.
		 */
		memcpy(&oldscsi, (char*)tmp, sizeof(oldscsi));
		tmp->In = SCSIDRV_In;
		tmp->Out = SCSIDRV_Out;
		tmp->InquireSCSI = SCSIDRV_InquireSCSI;
		tmp->InquireBus = SCSIDRV_InquireBus;
		tmp->CheckDev = SCSIDRV_CheckDev;
		tmp->RescanBus = SCSIDRV_RescanBus;
		tmp->Open = SCSIDRV_Open;
		tmp->Close = SCSIDRV_Close;
		tmp->Error = SCSIDRV_Error;
	} else {
		SCSIDRV_COOKIE.ident = COOKIE_SCSI;
		SCSIDRV_COOKIE.v.l = (long) &scsidrv;
		add_cookie (&SCSIDRV_COOKIE);
	}
#else
	static BUSINFO info[32];
	long ret;
	SCSIDRV *tmpscsi;

	/* Scan for busses reported by previous driver */

	ret = scsidrv_InquireSCSI(cInqFirst, info);

	while (ret == 0)
	{
		ret = scsidrv_InquireSCSI(cInqNext, info);
	}

	/*
	 * Find a free busno for USB bus. We use a fixed bus number of 4.
	 * If it's occupied, we don't install.
	 */

	if (info->busids & 1<<USBbus)
	{
		c_conws("Bus ID 4 already exists. SCSIDRV not installed.\r\n");
		return;
	}

	tmpscsi = (SCSIDRV *)scsidrv_InstallNewDriver(&scsidrv);
	if (tmpscsi)
		memcpy(&oldscsi, tmpscsi, sizeof(oldscsi));

#endif /* TOSONLY */
}
