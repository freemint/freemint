/*
 * USB mass storage driver
 *
 * Copyright (C) 2019 by David Galvez
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See /COPYING.GPL for details.
 */

#ifndef TOSONLY
#include "mint/mint.h"
#include "libkern/libkern.h"
#endif
#include "../../global.h"

#include "part.h"
#include "scsi.h"
#include "usb_storage.h"

/* Global variables */
short num_multilun_dev = 0;
#ifndef TOSONLY
static int polling_on = 0;
#endif

/* External declarations */
extern block_dev_desc_t usb_dev_desc[USB_MAX_STOR_DEV];
extern struct us_data usb_stor[USB_MAX_STOR_DEV];

extern long usb_test_unit_ready(ccb *srb, struct us_data *ss);
extern void usb_stor_eject(long device);
extern long usb_stor_get_info(struct usb_device *, struct us_data *, block_dev_desc_t *);
extern void part_init(long dev_num, block_dev_desc_t *stor_dev);

/* Functions prototypes */
void init_polling(void);
#ifndef TOSONLY
static void stor_poll_thread(void *dummy);


static void storage_int(void)
{
	int i, r;
	ccb pccb;

	for (i = 0; i < USB_MAX_STOR_DEV; i++) {
		if (usb_dev_desc[i].target == 0xff) {
			continue;
		}
		pccb.lun = usb_dev_desc[i].lun;
		r = usb_test_unit_ready(&pccb, &usb_stor[usb_dev_desc[i].usb_phydrv]);
		if ((r) && (usb_dev_desc[i].ready)) { /* Card unplugged */
			if (!usb_dev_desc[i].sw_ejected)
				usb_stor_eject(i);
			usb_dev_desc[i].ready = 0;
			usb_dev_desc[i].sw_ejected = 0;
		}
		else if ((!r) && (!usb_dev_desc[i].ready)) { /* Card plugged */
			usb_stor_get_info(usb_dev_desc[i].priv, &usb_stor[usb_dev_desc[i].usb_phydrv], &usb_dev_desc[i]);
			part_init(i, &usb_dev_desc[i]);

			ALERT(("USB Mass Storage Device (%d) LUN (%d) inserted %s",
				usb_dev_desc[i].usb_phydrv, usb_dev_desc[i].lun, usb_dev_desc[i].product));
		}
	}
}

static void
stor_poll(PROC *proc, long dummy)
{
	wake(WAIT_Q, (long)&stor_poll_thread);
}

static void
stor_poll_thread(void *dummy)
{
	/* This thread is only running while there are
	 * devices with more than 1 LUN connected
	 */
	while(num_multilun_dev)
	{
		storage_int();
		addtimeout(2000L, stor_poll);
		sleep(WAIT_Q, (long)&stor_poll_thread);
	}

	polling_on = 0;
	kthread_exit(0);
}
#endif

void init_polling(void)
{
#ifndef TOSONLY
	int r = 0;

	num_multilun_dev++;

	if (!polling_on)
		r = kthread_create(get_curproc (), stor_poll_thread, NULL, NULL, "usbstor");

	if (r) {
		DEBUG(("Failed to create storage polling thread"));
		return;
	}
	else
		polling_on = 1;
#endif
}
