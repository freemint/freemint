#include <mint/osbind.h>
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
#define debug(a) (void)Cconws(a)
#else
#define debug(a)
#endif

#define cdecl
#include "scsidefs.h"
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
                return (-2);    /* problem */
        }
        i++;
        p = ncookie (p);
    }
    return (-1);                /* no cookie-jar */
}

long ssp;

typedef struct SCSIDRV_Data
{
    short changed;
} SCSIDRV_Data;

static SCSIDRV_Data private[8];
static COOKIE SCSIDRV_COOKIE;
static tScsiCall scsidrv;
static tScsiCall oldscsi;
static COOKIE *old_cookie;
static tReqData reqdata;
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
SCSIDRV_Install (WORD bus, tpTargetHandler handler)
{
    return 0;
}

static long
SCSIDRV_Deinstall (WORD bus, tpTargetHandler handler)
{
    return 0;
}

static long
SCSIDRV_GetCmd (WORD bus, char *cmd)
{
    return 0;
}

static long
SCSIDRV_SendData (WORD bus, char *buf, ULONG len)
{
    return 0;
}

static long
SCSIDRV_GetData (WORD bus, void *buf, ULONG len)
{
    return 0;
}

static long
SCSIDRV_SendStatus (WORD bus, UWORD status)
{
    return 0;
}

static long
SCSIDRV_SendMsg (WORD bus, UWORD msg)
{
    return 0;
}

static long
SCSIDRV_GetMsg (WORD bus, UWORD * msg)
{
    return 0;
}

static long
SCSIDRV_In (tpSCSICmd Parms)
{
    SCSIDRV_Data *priv = NULL;
    long i;

    debug ("IN\r\n");

    for (i = 0; i < 8; i++) {
        if (&private[i] == (SCSIDRV_Data *) Parms->Handle) {
            priv = (SCSIDRV_Data *) Parms->Handle;
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

            if (Parms->CmdLen > 16) {
                return -1;
            }

            memset (&srb, 0, sizeof (srb));
            for (i = 0; i < Parms->CmdLen; i++)
            {
                srb.cmd[i] = Parms->Cmd[i];
            }

            srb.cmdlen = Parms->CmdLen;
            srb.datalen = Parms->TransferLen;
            srb.pdata = Parms->Buffer;

#if 0
            c_conws ("SCSIPACKET\r\n");
            hex_long (srb.cmd[0]);
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
        
            if (srb.cmd[0] == SCSI_RD_CAPAC)
                retries = 5;

            if (srb.cmd[0] == SCSI_TST_U_RDY)
                retries = 10;

            /* HDDRUTIL does this and locks up my USB CDROM */
            if (srb.cmd[0] == SCSI_GET_CONFIG) {
                return -1;
            }

            /* promote read6 to read10 */
            if (srb.cmd[0] == SCSI_READ6)
            {
                long block;
                srb.cmd[0] = SCSI_READ10;
                block = ((long) srb.cmd[1] & 0x1f) << 16 |
                    (long) srb.cmd[2] << 8 | (long) srb.cmd[3];
                srb.cmd[6] = 0;
                srb.cmd[7] = 0;
                srb.cmd[8] = srb.cmd[4];
                srb.cmd[1] = 0;
                srb.cmd[2] = (block >> 24) & 0xff;
                srb.cmd[3] = (block >> 16) & 0xff;
                srb.cmd[4] = (block >> 8) & 0xff;
                srb.cmd[5] = block & 0xff;
                srb.cmd[9] = 0;
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
                Parms->SenseBuffer[2] = 0x06;
                Parms->SenseBuffer[12] = 0x28;
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
                srb.pdata = (unsigned char *) &Parms->SenseBuffer[0];
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
        if (old_cookie)
        {
            return oldscsi.In (Parms);
        }
    }

    return -1;
}

static long
SCSIDRV_Out (tpSCSICmd Parms)
{
    SCSIDRV_Data *priv = NULL;
    long i;

    debug ("OUT\r\n");

    for (i = 0; i < 8; i++) {
        if (&private[i] == (SCSIDRV_Data *) Parms->Handle) {
            priv = (SCSIDRV_Data *) Parms->Handle;
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

            if (Parms->CmdLen > 16) {
                return -1;
            }

            memset (&srb, 0, sizeof (srb));
            for (i = 0; i < Parms->CmdLen; i++)
            {
                srb.cmd[i] = Parms->Cmd[i];
            }

            srb.cmdlen = Parms->CmdLen;
            srb.datalen = Parms->TransferLen;
            srb.pdata = Parms->Buffer;

            /* promote write6 to write10 */
            if (srb.cmd[0] == SCSI_WRITE6)
            {
                long block;
                srb.cmd[0] = SCSI_WRITE10;
                block = ((long) srb.cmd[1] & 0x1f) << 16 |
                    (long) srb.cmd[2] << 8 | (long) srb.cmd[3];
                srb.cmd[6] = 0;
                srb.cmd[7] = 0;
                srb.cmd[8] = srb.cmd[4];
                srb.cmd[1] = 0;
                srb.cmd[2] = (block >> 24) & 0xff;
                srb.cmd[3] = (block >> 16) & 0xff;
                srb.cmd[4] = (block >> 8) & 0xff;
                srb.cmd[5] = block & 0xff;
                srb.cmd[9] = 0;
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
                srb.pdata = (unsigned char *) &Parms->SenseBuffer[0];
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
        if (old_cookie)
        {
            return oldscsi.Out (Parms);
        }
    }
    return -1;
}

static long
SCSIDRV_InquireSCSI (WORD what, tBusInfo * Info)
{
    long ret;

    debug ("INQSCSI\r\n");

    if (what == cInqFirst) {
        Info->Private.BusIds = 0;
    }

    /* 
     * We let Uwe go first because it looks nicer in HDDRUTIL to show
     * 0, 1, 2, and then 3 :-)
     */
    if (old_cookie)
    {
        ret = oldscsi.InquireSCSI (what, Info);
        if (ret == 0)
            return 0;
    }

    /*
     * We shouldn't fail here as we scanned the busses when we installed
     * so our USBbus number should be valid.
     */
    if (!(Info->Private.BusIds & (1<<USBbus)))
    {
        strncpy (Info->BusName, USBNAME, sizeof(Info->BusName));
        Info->Private.BusIds |= 1<<USBbus;
        Info->BusNo = USBbus;
        Info->Features = cArbit | cAllCmds | cTargCtrl | cTarget | cCanDisconnect;
        Info->MaxLen = 64L * 1024L;
        return 0;
    }

    return -1;
}


static long
SCSIDRV_InquireBus (WORD what, WORD BusNo, tDevInfo * Dev)
{
    static long inqbusnext;
    long ret;

    debug ("INQBUS\r\n");

    if (what == cInqFirst)
    {
        inqbusnext = 0;
    }

    if (old_cookie)
    {
        ret = oldscsi.InquireBus (what, BusNo, Dev);
        if (ret == 0)
            return 0;
    }

    if (BusNo == USBbus)
    {
        block_dev_desc_t *dev_desc;
        memset (Dev->Private, 0, 32);
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

        Dev->SCSIId.hi = 0;
        Dev->SCSIId.lo = inqbusnext;
        inqbusnext++;
        return 0;
    }

    return -1;
}

static long
SCSIDRV_CheckDev (WORD BusNo,
                      const DLONG * DevNo, char *Name, UWORD * Features)
{
    debug ("CHECKDEV\r\n");

    if (BusNo == USBbus)
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
        if (old_cookie)
        {
            return oldscsi.CheckDev (BusNo, DevNo, Name, Features);
        }
    }
    return ENODEV;
}

static long
SCSIDRV_RescanBus (WORD BusNo)
{
    debug ("RESCAN\r\n");
    if (BusNo == USBbus)
    {
        return 0;
    }
    else
    {
        if (old_cookie)
        {
            return oldscsi.RescanBus (BusNo);
        }
    }
    return -1;
}

static long
SCSIDRV_Open (WORD bus, const DLONG * Id, ULONG * MaxLen)
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
        if (old_cookie)
        {
            return oldscsi.Open (bus, Id, MaxLen);
        }
    }
    return -1;
}

static long
SCSIDRV_Close (tHandle handle)
{
    long i;

    debug ("CLOSE\r\n");

    for (i = 0; i < 8; i++) {
        if (&private[i] == (SCSIDRV_Data *) handle) {
            return 0;
        }
    }

    if (old_cookie) {
        return oldscsi.Close (handle);
    }

    return -1;
}

static long
SCSIDRV_Error (tHandle handle, WORD rwflag, WORD ErrNo)
{
    long i;

    debug ("ERROR\r\n");

    for (i = 0; i < 8; i++) {
        if (&private[i] == (SCSIDRV_Data *) handle) {
            return 0;
        }
    }

    if (old_cookie)
    {
        return oldscsi.Error (handle, rwflag, ErrNo);
    }

    return -1;
}

void install_scsidrv (void);
void
install_scsidrv (void)
{
    WORD i;

    scsidrv.Version = SCSIRevision;
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
        private[i].changed = FALSE;
    }

    old_cookie = (COOKIE *) get_cookie (0x53435349L);
    if (old_cookie) {
        tBusInfo Info[32];
        WORD j;
        LONG ret;
        tScsiCall *tmp = (tScsiCall *)old_cookie->v.l;

        /*
         * Find a BusNo. We start at 3, and work up to a max of 32.
         */
        i = 0;
        ret = tmp->InquireSCSI(cInqFirst, &Info[i++]);

        while (ret == 0 && i < 32)
        {
            ret = tmp->InquireSCSI(cInqNext, &Info[i++]);
        }

again:
        for (j = 0; j < i; j++) {
            if (Info[j].BusNo == USBbus) {
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
}
