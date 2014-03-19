/*
 * USBNet API. i.e. the EUSB cookie.
 */

struct usb_eth_prob_dev {
	void (*before_probe)(void *api);
	long (*probe)(void *dev, unsigned int ifnum, void *ss);
	long (*get_info)(void *dev, void *ss, void *dev_desc);
};

struct usb_netapi {
	long			majorVersion;
	long			minorVersion;
	long			numDevices;
	long (*usb_eth_register)(struct usb_eth_prob_dev *ethdev);
	void (*usb_eth_deregister)(long i);

	/* not used by ethernet drives, only USB ethernet device driver */
	struct usb_eth_prob_dev	*usbnet;
};

