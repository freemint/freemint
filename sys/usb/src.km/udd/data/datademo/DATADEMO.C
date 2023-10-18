/*
 * USB Data Interface Class Demo Program
 *
 * To send data to a USB device that supports the Data Interface Class
 *
 * By Claude Labelle
 *
 * Based on USBTOOL By Alan Hourihane <alanh@fairlite.co.uk>
 *
 * Description: Among all the devices connected to the root hub and or other hub,
 * this program finds the first device that support the Data Interface Class.
 *
 * It then sends data to it. See the function send_data() for the parameters
 * that you specify, including your own data.
 *
 */

#include <stdio.h>
#include <aes.h>
#include <tos.h>
#include <portab.h>
#include <datademo.h>
#include <cookie.h>
#include <usb_api.h>

struct usb_module_api *api = 0;
struct usb_device *data_dev;
static struct dev_info info;

main(int argc, char *argv[])
{
	if (appl_init() < 0)
		return -1;
	find_data_interface();
	if (info.dev)
		printf("Data Class interface found.\n");
		/* send_data(); */
	else
		printf("No Data Class interface found.\n");
	appl_exit();
	return 0;
}

void find_data_interface(void)
{
	int i;
	short dev_count = 0;
	struct usb_device *dev;
	struct usb_interface *iface;
	struct usb_interface f;
	unsigned char ifaces;
	struct usb_endpoint_descriptor *ep_desc;

	if (!api)
	{
		/* Get USB cookie */
		getcookie(0x5F555342L,(long *)&api); /* '_USB' */
	}

	if (api)
	{
		dev_count = api->max_devices;
		for (i = 0; i < dev_count; i++)
		{
			dev = usb_get_dev_index (i);
			if (dev)
				{
				#ifdef DEBUGTOS
				printf("Got a device %s\n", dev->prod);
				#endif
				ifaces = dev->config.no_of_if;
				while (ifaces--)
				{
					iface = &dev->config.if_desc[ifaces];
					if (iface->desc.bInterfaceClass == USB_CLASS_DATA)
					{
					#ifdef DEBUGTOS
						printf("  It has a Data Class Interface.\n");
					#endif
						info.dev = dev;
						/* Get endpoints */
						for (i = 0; i < iface->desc.bNumEndpoints; i++) {
							ep_desc = &iface->ep_desc[i];
					#ifdef DEBUGTOS
							printf("  Found Endpoint");
					#endif
							if ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
					#ifdef DEBUGTOS
								printf(" of type bulk ");
					#endif
								if (ep_desc->bEndpointAddress & USB_DIR_IN) {
					#ifdef DEBUGTOS
									printf("in.\n");
					#endif
									info.ep_in = ep_desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
								}
								else {
					#ifdef DEBUGTOS
									printf("out.\n");
					#endif
									info.ep_out = ep_desc->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
								}
							}else {
					#ifdef DEBUGTOS
								printf(" of other type.\n");
					#endif
							}
						}
						break;
					}
				}
			}
		}
		if (dev_count == 0)
		{
			printf("No USB interface found.\n");
		}
	}
	else
	{
		printf("USB driver not loaded.\n");
	}
}

void send_data(void)
{
	char buf[16] = "Your data here.";
	long buflen = 16; /* your data length here */
	long actlen; /* actual length transferred */
	long timeout = 60000L; /* 60 seconds, adjust as necessary */
	unsigned long pipe = usb_sndbulkpipe (info.dev, (long) info.ep_out);
	long result;

	buf[15] = 0x0C; /* form feed. */

	usb_disable_asynch(0); /* asynch transfer allowed */
	result = usb_bulk_msg(info.dev, pipe, buf, buflen, &actlen, timeout, 0);
	if (result < 0)
	{
		printf("Send data error, device status: %ld", info.dev->status);
	}
	else
	{
		printf("SUCCESS!");
	}
	usb_disable_asynch(1); /* asynch transfer disallowed */
}

void receive_data(void)
{
	/* TO DO */
}


