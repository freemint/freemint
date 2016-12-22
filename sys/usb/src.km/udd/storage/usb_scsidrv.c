/*
 * USB Storage SCSIDRV Implementation (C) 2014-2015.
 * By Alan Hourihane <alanh@fairlite.co.uk>
 */

#ifdef TOSONLY
#include <mint/osbind.h>
#endif
#include "../../global.h"
#include "scsi.h"
#include "part.h"
#include "../../usb.h"
#include "../../usb_api.h"
#include "usb_storage.h"

extern block_dev_desc_t *usb_stor_get_dev (long);

#define USBNAME "USB Mass Storage"

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
		Super ((void *) stack);
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

static COOKIE *
get_cookie (long id)
{
	COOKIE *p;
	p = findcookie ();
	while (p)
	{
		if (p->ident == id)
			return p;
		p = ncookie (p);
	}
	return ((COOKIE *) 0);
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
static COOKIE *old_cookie;
#endif /* TOSONLY */

static REQDATA reqdata;

typedef struct SCSIDRV_Data
{
	ushort features; /* this has to be at the top ! */
	short changed;
} SCSIDRV_Data;

static SCSIDRV_Data private[8];
static SCSIDRV scsidrv;
static SCSIDRV oldscsi;
static unsigned short USBbus = 3; /* default */

void SCSIDRV_MediaChange(int dev);

/*
 * USB functions
 */

void
SCSIDRV_MediaChange(int dev)
{
	private[dev].changed = TRUE;
}

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
	long i;

	debug ("IN\r\n");

	for (i = 0; i < 8; i++) {
		if (&private[i] == (SCSIDRV_Data *) parms->handle) {
			priv = (SCSIDRV_Data *) parms->handle;
			break;
		}
	}

	if (priv)
	{
		block_dev_desc_t *dev_desc = usb_stor_get_dev (i);
		if (dev_desc->target != 0xff)
		{
			struct usb_device *dev = (struct usb_device *) dev_desc->priv;
			struct us_data *ss = (struct us_data *) dev->privptr;
			long retries = 0;
			ccb srb;
			long r;

			if (parms->cmdlen > 16) {
				return -1;
			}

			/* No LUN supported - yet */
			if (parms->cmd[1] & 0xE0) {
				return -1;
			}

			memset (&srb, 0, sizeof (srb));
			for (i = 0; i < parms->cmdlen; i++)
			{
				srb.cmd[i] = parms->cmd[i];
			}

			srb.cmdlen = parms->cmdlen;
			srb.datalen = parms->transferlen;
			srb.pdata = parms->buf;

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
				pccb.cmd[1] = 0;
				pccb.datalen = 0;
				pccb.cmdlen = 12;
				if(ss->transport(&pccb, ss) != 0) {
					return -1;
				}
			}

			if (srb.cmd[0] == SCSI_TST_U_RDY)
				retries = 10;

			/* HDDRUTIL 10.x issues this */
			if (srb.cmd[0] == SCSI_REPORT_LUN) {
				return -1;
 			}

			/* HDDRUTIL does this and locks up my USB CDROM */
			if (srb.cmd[0] == SCSI_GET_CONFIG) {
				return -1;
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
				srb.cmd[1] = 0;
				srb.cmd[2] = (block >> 24) & 0xff;
				srb.cmd[3] = (block >> 16) & 0xff;
				srb.cmd[4] = (block >> 8) & 0xff;
				srb.cmd[5] = block & 0xff;
				srb.cmd[6] = 0;
				srb.cmd[7] = 0;
				srb.cmdlen = 10;
			}

			if (srb.cmd[0] == SCSI_MODE_SEN6 ||
				srb.cmd[0] == SCSI_MODE_SEN10) {
				/* fail for now - but may need special handling */
				return -1;
			}

			/* XXXX: Needs verification !!!!!
			 */
			if (srb.cmd[0] == SCSI_TST_U_RDY && priv->changed) {
				/* Report Media Change sense key */
				parms->sense[2] = 0x06;
				parms->sense[12] = 0x28;
				priv->changed = FALSE;
				return 2;
			}

retry:
			r = ss->transport (&srb, ss);
			if (r !=0 && retries--) {
				goto retry;
			}

			/* Fix up INQUIRY NUL bytes to spaces. */
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

			/* 
			 * If we failed, get the sense data.
			 */
			if (r != 0)
			{
#if 0 /* ss->transport does this for us */
				char *ptr = (char *) srb.pdata;
				memset (&srb.cmd[0], 0, 12);
				srb.cmd[0] = SCSI_REQ_SENSE;
				srb.cmd[4] = 18;
				srb.datalen = 18;
				srb.pdata = (unsigned char *) &parms->sense[0];
				srb.cmdlen = 12;
				ss->transport (&srb, ss);
				srb.pdata = (unsigned char *) ptr;
#endif
				return -1;
			}
			return 0;
		}
		return -1;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.In (parms);
		}
	}

	return -1;
}

static long
SCSIDRV_Out (SCSICMD *parms)
{
	SCSIDRV_Data *priv = NULL;
	long i;

	debug ("OUT\r\n");

	for (i = 0; i < 8; i++) {
		if (&private[i] == (SCSIDRV_Data *) parms->handle) {
			priv = (SCSIDRV_Data *) parms->handle;
			break;
		}
	}


	if (priv)
	{
		block_dev_desc_t *dev_desc = usb_stor_get_dev (i);
		if (dev_desc->target != 0xff)
		{
			struct usb_device *dev = (struct usb_device *) dev_desc->priv;
			struct us_data *ss = (struct us_data *) dev->privptr;
			ccb srb;
			long r;

			if (parms->cmdlen > 16) {
				return -1;
			}

			/* No LUN supported - yet */
			if (parms->cmd[1] & 0xE0) {
				return -1;
			}

			memset (&srb, 0, sizeof (srb));
			for (i = 0; i < parms->cmdlen; i++)
			{
				srb.cmd[i] = parms->cmd[i];
			}

			srb.cmdlen = parms->cmdlen;
			srb.datalen = parms->transferlen;
			srb.pdata = parms->buf;

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
				srb.cmd[1] = 0;
				srb.cmd[2] = (block >> 24) & 0xff;
				srb.cmd[3] = (block >> 16) & 0xff;
				srb.cmd[4] = (block >> 8) & 0xff;
				srb.cmd[5] = block & 0xff;
				srb.cmd[6] = 0;
				srb.cmd[7] = 0;
				srb.cmdlen = 10;
			}

			r = ss->transport (&srb, ss);

			if (r != 0)
			{
#if 0 /* ss->transport does this for us */
				char *ptr = (char *) srb.pdata;
				memset (&srb.cmd[0], 0, 12);
				srb.cmd[0] = SCSI_REQ_SENSE;
				srb.cmd[4] = 18;
				srb.datalen = 18;
				srb.pdata = (unsigned char *) &parms->sense[0];
				srb.cmdlen = 12;
				ss->transport (&srb, ss);
				srb.pdata = (unsigned char *) ptr;
#endif
				return -1;
			}
			return 0;
		}
		return -1;
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.Out (parms);
		}
	}
	return -1;
}

static long
SCSIDRV_InquireSCSI (short what, BUSINFO * info)
{
	long ret;

	debug ("INQSCSI\r\n");

	if (what == cInqFirst) {
		info->busids = 0;
	}

	/* 
	 * We let Uwe go first because it looks nicer in HDDRUTIL to show
	 * 0, 1, 2, and then 3 :-)
	 */
	if (oldscsi.version)
	{
		ret = oldscsi.InquireSCSI (what, info);
		if (ret == 0)
			return 0;
	}

	/*
	 * We shouldn't fail here as we scanned the busses when we installed
	 * so our USBbus number should be valid.
	 */
	if (!(info->busids & (1<<USBbus)))
	{
		strncpy (info->busname, USBNAME, sizeof(info->busname));
		info->busids |= 1<<USBbus;
		info->busno = USBbus;
		info->features = cArbit | cAllCmds | cTargCtrl | cTarget | cCanDisconnect;
		info->maxlen = 64L * 1024L;
		return 0;
	}

	return -1;
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
		block_dev_desc_t *dev_desc;
		memset (dev->priv, 0, 32);
		if (inqbusnext >= 8)
		{
			return -1;
		}

again:
		dev_desc = usb_stor_get_dev (inqbusnext);
		if (dev_desc->target == 0xff)
		{
			inqbusnext++;
			if (inqbusnext >= 8)
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
		block_dev_desc_t *dev_desc;

		memset (Name, 0, 20);
		strcat (Name, USBNAME);
		*Features = 0;
		if (DevNo->hi != 0L)
		{
			return ENODEV;
		}
		if (DevNo->lo >= 8L)
		{
			return ENODEV;
		}
		dev_desc = usb_stor_get_dev (DevNo->lo);
		if (dev_desc->target != 0xff)
		{
			*Features = cArbit | cAllCmds | cTargCtrl | cTarget | cCanDisconnect;
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
	debug ("OPEN\r\n");
	if (bus == USBbus)
	{
		block_dev_desc_t *dev_desc;

		if (Id->hi != 0)
			return -1;

		if (Id->lo >= 8)
			return -1;

		dev_desc = usb_stor_get_dev (Id->lo);
		if (dev_desc->target != 0xff)
		{
			struct usb_device *dev = (struct usb_device *) dev_desc->priv;
			struct us_data *ss = (struct us_data *) dev->privptr;

			/* We only allow SCSI compliant USB devices */
			if (ss->subclass != US_SC_SCSI) {
				return -1;
			}
		} else {
			return -1;
		}
		*MaxLen = 64 * 1024L;
		return (long) &private[Id->lo];
	}
	else
	{
		if (oldscsi.version)
		{
			return oldscsi.Open (bus, Id, MaxLen);
		}
	}
	return -1;
}

static long
SCSIDRV_Close (short *handle)
{
	long i;

	debug ("CLOSE\r\n");

	for (i = 0; i < 8; i++) {
		if (&private[i] == (SCSIDRV_Data *) handle) {
			return 0;
		}
	}

	if (oldscsi.version) {
		return oldscsi.Close (handle);
	}

	return -1;
}

static long
SCSIDRV_Error (short *handle, short rwflag, short ErrNo)
{
	long i;

	debug ("ERROR\r\n");

	for (i = 0; i < 8; i++) {
		if (&private[i] == (SCSIDRV_Data *) handle) {
			return 0;
		}
	}

	if (oldscsi.version)
	{
		return oldscsi.Error (handle, rwflag, ErrNo);
	}

	return -1;
}

void install_scsidrv (void);
void
install_scsidrv (void)
{
	short i;

	scsidrv.version = SCSIRevision;
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

	for (i = 0; i < 8; i++)
	{
		private[i].features = cArbit | cAllCmds | cTargCtrl | cTarget | cCanDisconnect;
		private[i].changed = FALSE;
	}

#ifdef TOSONLY
	old_cookie = (COOKIE *) get_cookie (0x53435349L);
	if (old_cookie) {
		SCSIDRV *tmp = (SCSIDRV *)old_cookie->v.l;
		BUSINFO info[32];
		short j;
		long ret;

		/*
		 * Find a busno. We start at 3, and work up to a max of 32.
		 */
		i = 0;
		ret = tmp->InquireSCSI(cInqFirst, &info[i++]);

		while (ret == 0 && i < 32)
		{
			ret = tmp->InquireSCSI(cInqNext, &info[i++]);
		}

again:
		for (j = 0; j < i; j++) {
			if (info[j].busno == USBbus) {
				USBbus++;
				goto again;
			}
		}

		/* don't install, we couldn't find a bus */
		if (USBbus >= 32) 
			return;

		/* Take a copy of the old pointers, and replace with ours.
		 * This way we don't delete and replace the existing cookie.
		 */
		memcpy(&oldscsi, (char*)old_cookie->v.l, sizeof(oldscsi));
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
		SCSIDRV_COOKIE.ident = 0x53435349L;
		SCSIDRV_COOKIE.v.l = (long) &scsidrv;
		add_cookie (&SCSIDRV_COOKIE);
	}
#else
	BUSINFO info[32];
	short j;
	long ret;
	SCSIDRV *tmpscsi;

	/*
	 * Find a busno. We start at 3, and work up to a max of 32.
	 */
	i = 0;
	ret =scsidrv_InquireSCSI(cInqFirst, &info[i++]);

	while (ret == 0 && i < 32)
	{
		ret = scsidrv_InquireSCSI(cInqNext, &info[i++]);
	}

again:
	for (j = 0; j < i; j++) {
		if (info[j].busno == USBbus) {
			USBbus++;
			goto again;
		}
	}

	/* don't install, we couldn't find a bus */
	if (USBbus >= 32) 
		return;

	tmpscsi = (SCSIDRV *)scsidrv_InstallNewDriver(&scsidrv);
	if (tmpscsi)
		memcpy(&oldscsi, tmpscsi, sizeof(oldscsi));
#endif /* TOSONLY */
}
