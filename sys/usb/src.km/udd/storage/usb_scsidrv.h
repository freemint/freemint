/*
 * Copyright (c) 2026 The FreeMiNT development team
 *
 * USB Storage SCSIDRV Integration Header
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _USB_SCSIDRV_H
#define _USB_SCSIDRV_H

#include "mint/scsidrv.h"

/* The USB bus is always assigned bus number 4 in the SCSIDRV cookie */
#define SCSIDRV_USB_BUS  4

extern SCSIDRV scsidrv;

/* Direct entry points — bypass the SCSIDRV cookie/chain for internal use */
long SCSIDRV_In(SCSICMD *par);
long SCSIDRV_Out(SCSICMD *par);
long SCSIDRV_Open(short busno, const DLONG *SCSIId, ulong *maxlen);
long SCSIDRV_Close(short *handle);
void scsidrv_set_mediach(long global_lun_id);

void install_scsidrv(void);

#endif /* _USB_SCSIDRV_H */
