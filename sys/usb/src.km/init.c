/*
 * Modified for the FreeMiNT USB subsystem by David Galvez. 2010 - 2011
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "global.h"
#include "util.h"
#include "init.h"
#include "usb.h"
#include "hub.h"
#include "ucdload.h"
#include "uddload.h"
#include "usb_api.h"

long loader_pid = 0;
long loader_pgrp = 0;

struct usb_module_api usb_api;

#define MSG_BUILDDATE	__DATE__

#ifdef TOSONLY
#define MSG_VERSION		"TOS DRIVERS"
#define MSG_BOOT		\
		"\033p USB core API driver for TOS " MSG_VERSION " \033q\r\n" \
		"Brought to TOS by Alan Hourihane.\r\n"
#else
#define MSG_VERSION	"FreeMiNT DRIVERS"
#define MSG_BOOT	\
		"\033p USB core API driver for FreeMiNT " MSG_VERSION " \033q\r\n"
#endif

#define MSG_GREET		\
	"David Galvez 2010-2014.\r\n" \
	"Alan Hourihane 2013-2014.\r\n" \
	"Compiled " MSG_BUILDDATE ".\r\n\r\n"

static void
bootmessage(void)
{
	c_conws(MSG_BOOT);
	c_conws(MSG_GREET);
}

struct kentry *kentry;

/*
 * Module initialisation
 * - setup internal data
 * - start main kernel thread
 */

#ifndef TOSONLY
Path start_path;
static const struct kernel_module *self = NULL;
#else
static void
set_cookie (void)
{
	struct cookie *cjar = *CJAR;
	long n = 0;

	while (cjar->tag)
	{
		n++;
		if (cjar->tag == _USB)
		{
			cjar->value = (long)&usb_api;
			return;
		}
		cjar++;
	}

	n++;
	if (n < cjar->value)
	{
		n = cjar->value;
		cjar->tag = _USB;
		cjar->value = (long)&usb_api;

		cjar++;
		cjar->tag = 0L;
		cjar->value = n;
	}
}

extern unsigned long _PgmSize;
#endif

long			udd_register		(struct uddif *u);
long			udd_unregister		(struct uddif *u);
extern long	ucd_register		(struct ucdif *u, struct usb_device **dev);
extern long	ucd_unregister		(struct ucdif *u);

static void
setup_usb_module_api(void)
{
	usb_api.api_version = USB_API_VERSION;
	usb_api.max_devices = USB_MAX_DEVICE;
	usb_api.max_hubs = USB_MAX_HUB;

	usb_api.udd_register = &udd_register;
	usb_api.udd_unregister = &udd_unregister;
	usb_api.ucd_register = &ucd_register;
	usb_api.ucd_unregister = &ucd_unregister;
	usb_api.usb_rh_wakeup = &usb_rh_wakeup;

	usb_api.usb_set_protocol = &usb_set_protocol;
	usb_api.usb_set_idle = &usb_set_idle;
	usb_api.usb_get_dev_index = &usb_get_dev_index;
	usb_api.usb_get_hub_index = &usb_get_hub_index;
	usb_api.usb_control_msg = &usb_control_msg;
	usb_api.usb_bulk_msg = &usb_bulk_msg;
	usb_api.usb_submit_int_msg = &usb_submit_int_msg;
	usb_api.usb_disable_asynch = &usb_disable_asynch;
	usb_api.usb_maxpacket = &usb_maxpacket;
	usb_api.usb_get_configuration_no = &usb_get_configuration_no;
	usb_api.usb_get_report = &usb_get_report;
	usb_api.usb_get_class_descriptor = &usb_get_class_descriptor;
	usb_api.usb_clear_halt = &usb_clear_halt;
	usb_api.usb_string = &usb_string;
	usb_api.usb_set_interface = &usb_set_interface;
	usb_api.usb_parse_config = &usb_parse_config;
	usb_api.usb_set_maxpacket = &usb_set_maxpacket;
	usb_api.usb_get_descriptor = &usb_get_descriptor;
	usb_api.usb_set_address = &usb_set_address;
	usb_api.usb_set_configuration = &usb_set_configuration;
	usb_api.usb_get_string = &usb_get_string;
	usb_api.usb_alloc_new_device = &usb_alloc_new_device;
	usb_api.usb_new_device = &usb_new_device;
	
	/* For now we leave most of this hub specific functions out of the api.
	 */
	usb_api.usb_hub_events = &usb_hub_events;

//	usb_api.usb_get_hub_descriptor = &usb_get_hub_descriptor;
//	usb_api.usb_clear_port_feature = &usb_clear_port_feature;
//	usb_api.usb_get_hub_status = &usb_get_hub_status;
//	usb_api.usb_set_port_feature = &usb_set_port_feature;
//	usb_api.usb_get_port_status = &usb_get_port_status;
//	usb_api.usb_hub_allocate = &usb_hub_allocate;
//	usb_api.usb_hub_port_connect_change = &usb_hub_port_connect_change;
//	usb_api.usb_hub_configure = &usb_hub_configure;	
}

#ifdef TOSONLY
int init(int argc, char **argv, char **env);

int
init(int argc, char **argv, char **env)
#else
long init(struct kentry *k, const struct kernel_module *km);

long
init(struct kentry *k, const struct kernel_module *km)
#endif
{
	long err = 0L;

#ifndef TOSONLY
	/* setup kernel entry */
	kentry = k;
	self = km;

	bootmessage();

	get_drive_and_path(start_path, sizeof(start_path));

	if (check_kentry_version())
	{
		err = ENOSYS;
		goto error;
	}

	/* remember loader */

	loader_pid = p_getpid();
	loader_pgrp = p_getpgrp();
#else
	bootmessage();

	{
		struct usb_module_api *checkapi;

		checkapi = get_usb_cookie();
		if (checkapi != NULL) {
			c_conws("USB core already installed.\r\n");
			return 0;
		}
	}
#endif

	setup_usb_module_api();

	usb_main();

#ifdef TOSONLY
	{
		/* Set the _USB API cookie */
		Supexec(set_cookie);

		c_conws("USB core installed.\r\n");

		/* terminate and stay resident */
		Ptermres(_PgmSize, 0);
	}


#else
error:
#endif
	return err;
}

