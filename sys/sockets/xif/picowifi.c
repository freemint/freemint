/*
 * Modified for the FreeMiNT USB subsystem by Alan Hourihane 2014.
 *
 * Modified for PicoWifi adapter by Christian Zietz 2022.
 *
 * Copyright (c) 2011 The Chromium OS Authors.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

//#define DEV_DEBUG 1

#define u32 __u32
#define u16 __u16
#define u8 __u8
#include "cookie.h"
#include "init.h"
#include "global.h"
#include "buf.h"
#include "usbnet.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"
#include "netinfo.h"
#include "mint/time.h"
#include "inet4/mii.h"
#include "mint/sockio.h"
#include "mint/endian.h"
#include "mint/mdelay.h"
#include <mint/osbind.h>

#include "../../usb/src.km/usb.h"
#include "../../usb/src.km/usb_api.h"
#include "../../usb/src.km/udd/eth/usb_ether.h"

struct usb_module_api   *api = NULL;
static struct proc *picowifi_p;
static long usbEthID = -1;

typedef struct
{
        unsigned long  name;
        unsigned long  val;
} COOKIE;

#define COOKIEBASE (*(COOKIE **)0x5a0)

#define eth_device netif

#define CONFIG_SYS_HZ CLOCKS_PER_SEC

void picowifi_poll_thread(void *);
void picowifi_poll(PROC *proc, long dummy);

/*
 * Debug section
 */

/* local vars */

/* driver private */

#define MTU 1600
#define MAGIC 0xAA55AA55ul

typedef struct {
	u32 magic;
	u32 len;
	u8 payload[MTU];
} pkt_s;

typedef struct {
	u32 magic;
	u32 len;
} pkt_hdr_s;

/*
 * picowifi callbacks
 */

#ifndef offsetof
#define offsetof __builtin_offsetof
#endif
#define PICOWIFI_BASE_NAME "pwi"
#define USB_BULK_SEND_TIMEOUT 5000
#define USB_BULK_RECV_TIMEOUT 5000
#define USB_CTRL_SET_TIMEOUT 5000
#define USB_CTRL_GET_TIMEOUT 5000

#define VENDOR_REQUEST_WIFI 2

#define WIFI_CONNECT_TIMEOUT 30000 /* ms */

enum {
	WIFI_SET_SSID = 0,
	WIFI_SET_PASSWD,
	WIFI_CONNECT,
	WIFI_STATUS,
	FIRMWARE_UPDATE = 0x100,
};

static long picowifi_load_wificred(char** wifi_ssid, char** wifi_pass)
{
	static char buffer[256]; /* will return pointer within this buffer, therefore static */
	long fhandle, res;
	int idx;

	*wifi_ssid = NULL;
	*wifi_pass = NULL;

	/* If drive C: exists, try to load from C:, else from A: */
	if (Drvmap() & (1<<2)) {
		fhandle = Fopen("C:\\WIFICRED.CFG", 0);
	} else {
		fhandle = Fopen("A:\\WIFICRED.CFG", 0);
	}

	if (fhandle < 0) {
		/* Try /etc/wificred.cfg for MiNT */
		fhandle = Fopen("U:\\etc\\wificred.cfg", 0);
	}

	if (fhandle < 0) {
		return -1;
	}

	res = Fread(fhandle, sizeof(buffer), buffer);
	Fclose(fhandle);
	if (res <= 0) {
		return -1;
	}

	/* zero terminate string */
	buffer[res] = '\0';
	*wifi_ssid = buffer;

	idx = 0;
	/* find password */
	while (buffer[idx]) {
		if (buffer[idx] == '\r') {
			/* strip CR */
			buffer[idx]= '\0';
		}
		if (buffer[idx] == '\n') {
			buffer[idx] = '\0';
			idx++;
			*wifi_pass = &buffer[idx];
			break;
		}
		idx++;
	}

	/* strip CR/NL */
	while (buffer[idx]) {
		if (buffer[idx] == '\r') {
			buffer[idx]= '\0';
		}
		if (buffer[idx] == '\n') {
			buffer[idx] = '\0';
		}
		idx++;
	}

	return 0;
}

static long picowifi_init(struct eth_device *eth)
{

	struct ueth_data	*dev = (struct ueth_data *)eth->data;
	long len;
	char link_detected;
	int timeout = 0;
	char *wifi_ssid, *wifi_pass;
#define TIMEOUT_RESOLUTION 50	/* ms */

	if (picowifi_load_wificred(&wifi_ssid, &wifi_pass)) {
		ALERT(("unable to load wifi credentials.\r\n"));
		return -1;
	}
	DEBUG(("Wifi credentials: '%s', '%s'\r\n", wifi_ssid, wifi_pass?wifi_pass:"(no password)"));

	/* Set SSID */
	len = usb_control_msg(
	dev->pusb_dev,
	usb_sndctrlpipe(dev->pusb_dev, 0),
	VENDOR_REQUEST_WIFI,
	USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
	0,
	WIFI_SET_SSID,
	wifi_ssid,
	strlen(wifi_ssid),
	USB_CTRL_SET_TIMEOUT);

	if (wifi_pass && (strlen(wifi_pass) > 0)) {
		/* Set password */
		len = usb_control_msg(
		dev->pusb_dev,
		usb_sndctrlpipe(dev->pusb_dev, 0),
		VENDOR_REQUEST_WIFI,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0,
		WIFI_SET_PASSWD,
		wifi_pass,
		strlen(wifi_pass),
		USB_CTRL_SET_TIMEOUT);

		/* Connect */
		len = usb_control_msg(
		dev->pusb_dev,
		usb_sndctrlpipe(dev->pusb_dev, 0),
		VENDOR_REQUEST_WIFI,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0x406, /* WPA2/WPA */
		WIFI_CONNECT,
		NULL,
		0,
		USB_CTRL_SET_TIMEOUT);
	} else {
		/* Connect */
		len = usb_control_msg(
		dev->pusb_dev,
		usb_sndctrlpipe(dev->pusb_dev, 0),
		VENDOR_REQUEST_WIFI,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0, /* open */
		WIFI_CONNECT,
		NULL,
		0,
		USB_CTRL_SET_TIMEOUT);
	}

	/* Wait for link */
	do {
		link_detected = 0;
		
		len = usb_control_msg(
		dev->pusb_dev,
		usb_rcvctrlpipe(dev->pusb_dev, 0),
		VENDOR_REQUEST_WIFI,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0,
		WIFI_STATUS,
		&link_detected,
		sizeof(link_detected),
		USB_CTRL_GET_TIMEOUT);
		
		if (!link_detected) {
			if (timeout == 0) {
				ALERT(("Waiting for Wifi connection... "));
			}
			mdelay(TIMEOUT_RESOLUTION);
			timeout += TIMEOUT_RESOLUTION;
		}
	} while (!link_detected && timeout < WIFI_CONNECT_TIMEOUT);
	if (link_detected) {
		if (timeout != 0) {
			ALERT(("done.\r\n"));
			//mdelay(500);
		}
	} else {
		ALERT(("unable to connect.\r\n"));
		return -1;
	}

	(void)len;
	addroottimeout(250, picowifi_poll, 0);

	return 0;
}

static long picowifi_send(struct eth_device *eth, void *packet, long length)
{
	struct ueth_data *dev = (struct ueth_data *)eth->data;
	static pkt_s outpkt;
	long size;
	long actual_len;
	long err;

	DEBUG(("** %s(), len %ld\r\n", __func__, length));
	if (dev->pusb_dev == 0) {
		return 0;
	}

	outpkt.magic = cpu2le32(MAGIC);
	outpkt.len   = cpu2le32(length);

	memcpy(outpkt.payload, packet, length);

	size = length + offsetof(pkt_s, payload);

	err = usb_bulk_msg(dev->pusb_dev,
				usb_sndbulkpipe(dev->pusb_dev, (long)dev->ep_out),
				(void *)&outpkt,
				size,
				&actual_len,
				USB_BULK_SEND_TIMEOUT, 0);

	if (err != 0)
		return -1;
	else 
		return 0;
}

#define FIFO_SIZE (2*4096) // twice the device FIFO
static struct recv_fifo_s {
	u8  buffer[FIFO_SIZE];
	int level;
	int readidx;
	int writeidx;
} *recv_fifo;

static void fifo_reset(void)
{
	recv_fifo->readidx = recv_fifo->writeidx = recv_fifo->level = 0;
}

static void fifo_enqueue(u8* data, int len)
{

	if (recv_fifo->writeidx + len <= FIFO_SIZE) {
		memcpy(&recv_fifo->buffer[recv_fifo->writeidx], data, len);
		recv_fifo->level += len;
		recv_fifo->writeidx += len;
	} else { // data goes past the break
		int part1 = FIFO_SIZE - recv_fifo->writeidx;
		int part2 = len - part1;
		memcpy(&recv_fifo->buffer[recv_fifo->writeidx], data, part1);
		memcpy(&recv_fifo->buffer[0], &data[part1], part2);
		recv_fifo->level += len;
		recv_fifo->writeidx += len - FIFO_SIZE;
	}

}

static void fifo_dequeue(u8* data, int len, int peek)
{
	if (recv_fifo->readidx + len <= FIFO_SIZE) {
		memcpy(data, &recv_fifo->buffer[recv_fifo->readidx], len);
		if (!peek) {
			recv_fifo->level -= len;
			recv_fifo->readidx += len;
		}
	} else { // data goes past the break
		int part1 = FIFO_SIZE - recv_fifo->readidx;
		int part2 = len - part1;
		memcpy(data, &recv_fifo->buffer[recv_fifo->readidx], part1);
		memcpy(&data[part1], &recv_fifo->buffer[0], part2);
		if (!peek) {
			recv_fifo->level -= len;
			recv_fifo->readidx += len - FIFO_SIZE;
		}
	}
}

static long picowifi_recv(struct eth_device *eth)
{
	struct ueth_data *dev = (struct ueth_data *)eth->data;
	static u8 recv_buffer[FIFO_SIZE/2];
	static int resync_count = 0;
	pkt_hdr_s next_hdr;
	long actual_len = 0;
	long size = 0;
	long err;
	BUF *buf;

	DEBUG(("** %s()\r\n", __func__));

	if (dev->pusb_dev == 0) {
		size = -1;
		goto out;
	}
	
	if (!(eth->flags & IFF_UP)) {
		size = -1;
		goto out;
	}

	if (FIFO_SIZE - recv_fifo->level >= FIFO_SIZE/2) { // try to get a new packet from USB
		DEBUG(("** %s(): usb xfer\r\n", __func__));
		err = usb_bulk_msg(dev->pusb_dev,
					usb_rcvbulkpipe(dev->pusb_dev, (long)dev->ep_in),
					(void *)recv_buffer,
					FIFO_SIZE/2,
					&actual_len,
					USB_BULK_RECV_TIMEOUT,
					USB_BULK_FLAG_EARLY_TIMEOUT);
		DEBUG(("** %s(): usb xfer %ld, %ld\r\n", __func__, err, actual_len));
		if ((err == 0) && (actual_len > 0)) { // got new data
			DEBUG(("actual_len = %ld\n", actual_len));
			fifo_enqueue(recv_buffer, actual_len);
		}
	}

	while (recv_fifo->level > offsetof(pkt_s, payload)) {
		// check if we have a full packet in the FIFO
		
		fifo_dequeue((u8*)&next_hdr, sizeof(next_hdr), 1);

		if (next_hdr.magic != le2cpu32(MAGIC)) {
			DEBUG(("invalid magic(%d) = %lx\n", resync_count, next_hdr.magic));
			if (resync_count == 3) {
				fifo_reset();
				resync_count = 0;
			} else {
				fifo_dequeue((u8*)&next_hdr, 4, 0); // pop 4 bytes and try to resync
				resync_count++;
			}
			goto out;
		}
		resync_count = 0;
		size = le2cpu32(next_hdr.len);
		
		if (recv_fifo->level >= offsetof(pkt_s, payload) + size) {
			DEBUG(("pkt_recv: len = %ld\n", size));
		
			buf = buf_alloc (size, 0, BUF_ATOMIC);
			if (!buf)
				{
				DEBUG (("picowifi_recv: out of mem (buf_alloc failed)"));
				eth->in_errors++;
				size = -1;
				goto out;
			}
			buf->dend = buf->dstart + size;
			fifo_dequeue((u8*)&next_hdr, sizeof(next_hdr), 0);
			fifo_dequeue((u8*)buf->dstart, size, 0);

			if (eth->bpf)
				bpf_input (eth, buf);

			/* and enqueue packet */
			if (!if_input (eth, buf, 0, eth_remove_hdr (buf)))
				eth->in_packets++;
			else
				eth->in_errors++;

		} else {
			/* incomplete packet in the FIFO, keep it for later. */
			break;
		}

	}

out:
#ifdef M68000
	addroottimeout(50, picowifi_poll, 0);
#else
	addroottimeout(10, picowifi_poll, 0);
#endif

	DEBUG(("** %s(): return %ld\r\n", __func__, size));
	return size>0? 0 : -1;
}

static long _cdecl
picowifi_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;

        nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
        if (!nbuf)
        {
                DEBUG (("picowifi: eth_build_hdr failed, out of memory!"));

                nif->out_errors++;
                return ENOMEM;
        }

        /* pass to upper layer */
        if (nif->bpf)
		bpf_input (nif, nbuf);

	if (picowifi_send(nif, nbuf->dstart, nbuf->dend - nbuf->dstart) < 0) {
		buf_deref (nbuf, BUF_NORMAL);
                nif->out_errors++;
		return ENOMEM;
	}

	buf_deref (nbuf, BUF_NORMAL);
	nif->out_packets++;

	return E_OK;
}

static long picowifi_halt(struct eth_device *eth)
{
	DEBUG(("** %s()\n", __func__));

	return 0;
}

static long _cdecl
picowifi_ioctl (struct netif *nif, short cmd, long arg)
{
	switch (cmd)
	{
		case SIOCSIFNETMASK:
		case SIOCSIFFLAGS:
		case SIOCSIFADDR:
			return 0;
		
		case SIOCSIFMTU:
			/*
			 * Limit MTU to 1500 bytes. MintNet has alraedy set nif->mtu
			 * to the new value, we only limit it here.
			 */
			if (nif->mtu > ETH_MAX_DLEN)
				nif->mtu = ETH_MAX_DLEN;
			return 0;
	}
	
	return ENOSYS;
}

/*
 * Picowifi probing functions
 */
static void picowifi_eth_before_probe(void *a)
{
	api = a;
}

#define USB_VID 0x20A0u
#define USB_PID 0x42ECu

/* Probe to see if a new device is actually an picowifi device */
static long
picowifi_eth_probe(void *vdev, unsigned int ifnum, void *vss)
{
	struct usb_device *dev = vdev;
	struct ueth_data *ss = vss;
	
	struct usb_interface *iface;
	struct usb_interface_descriptor *iface_desc;
	int ep_in_found = 0, ep_out_found = 0;
	int i;

	/* let's examine the device now */
	iface = &dev->config.if_desc[ifnum];
	iface_desc = &dev->config.if_desc[ifnum].desc;

	if (dev->descriptor.idVendor != USB_VID ||
	    dev->descriptor.idProduct != USB_PID)
		/* Found a supported dongle */
		return 0;

	memset(ss, 0, sizeof(struct ueth_data));

	/* At this point, we know we've got a live one */
	DEBUG(("\r\n\r\nUSB Ethernet device detected: %04x:%04x\r\n",
	      dev->descriptor.idVendor, dev->descriptor.idProduct));

	/* Initialize the ueth_data structure with some useful info */
	ss->ifnum = ifnum;
	ss->pusb_dev = dev;
	ss->subclass = iface_desc->bInterfaceSubClass;
	ss->protocol = iface_desc->bInterfaceProtocol;
	ss->dev_priv = NULL;
	/* alloc fifo */
	recv_fifo = kmalloc(sizeof(struct recv_fifo_s));
	if (!recv_fifo)
		return 0;
	memset(recv_fifo, 0, sizeof(struct recv_fifo_s));

	/*
	 * We are expecting a minimum of 2 endpoints - in, out (bulk)
	 * We will ignore any others.
	 */
	for (i = 0; i < iface_desc->bNumEndpoints; i++) {
		/* is it an BULK endpoint? */
		if ((iface->ep_desc[i].bmAttributes &
		     USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
			u8 ep_addr = iface->ep_desc[i].bEndpointAddress;
			if (ep_addr & USB_DIR_IN) {
				if (!ep_in_found) {
					ss->ep_in = ep_addr &
						USB_ENDPOINT_NUMBER_MASK;
					ep_in_found = 1;
				}
			} else {
				if (!ep_out_found) {
					ss->ep_out = ep_addr &
						USB_ENDPOINT_NUMBER_MASK;
					ep_out_found = 1;
				}
			}
		}
	}

	/* Do some basic sanity checks, and bail if we find a problem */
	if (usb_set_interface(dev, iface_desc->bInterfaceNumber, 0) ||
	    !ss->ep_in || !ss->ep_out) {
		DEBUG(("Problems with device\r\n"));
		return 0;
	}
	dev->privptr = (void *)ss;
	return 1;
}

static int hexchar(unsigned char c)
{
	int v = 0;

	if ((c >= '0') && (c <= '9')) {
		v = c - '0';
	} else if ((c >= 'A') && (c <= 'F')) {
		v = c - 'A' + 0xa;
	} else if ((c >= 'a') && (c <= 'f')) {
		v = c - 'a' + 0xa;
	}

	return v;
}

static int picowifi_read_mac(struct eth_device *eth)
{
	struct ueth_data *dev = (struct ueth_data *)eth->data;
	char mac_address[ETH_ALEN];

	int k;
	// parse MAC encoded in device serial number
	for (k=0; k < ETH_ALEN; k++) {
		mac_address[k] = hexchar(dev->pusb_dev->serial[2*k]) << 4 | hexchar(dev->pusb_dev->serial[2*k+1]);
	}
	DEBUG(("MAC: %02x%02x%02x%02x%02x%02x\n", mac_address[0],mac_address[1],mac_address[2],mac_address[3],mac_address[4],mac_address[5]));
	memcpy(eth->hwlocal.adr.bytes, mac_address, ETH_ALEN);
	return 0;
}

static long 
picowifi_eth_get_info(void *vdev, void *vss, void *veth)
{
	struct usb_device *dev = vdev;
	struct ueth_data *ss = vss;
	struct eth_device *eth = veth;
	//struct picowifi_private *priv = (struct picowifi_private *)ss->dev_priv;
	long r;

	(void) dev;

	if (!eth) {
		DEBUG(("%s: missing parameter.\n", __func__));
		return 0;
	}

	strcpy(eth->name, PICOWIFI_BASE_NAME);
	eth->unit = 0;
	eth->flags = IFF_BROADCAST;
	eth->metric = 0;
	eth->mtu = 1500;
	eth->timer = 0;
	eth->hwtype = HWTYPE_ETH;
	eth->hwlocal.len = ETH_ALEN;
	eth->hwbrcst.len = ETH_ALEN;
	eth->snd.maxqlen = IF_MAXQ;	
	eth->rcv.maxqlen = IF_MAXQ;	
	eth->timeout = NULL;
	eth->maxpackets = 4;
	eth->open = picowifi_init;
	eth->ioctl = picowifi_ioctl;
	eth->output = picowifi_output;
	eth->close = picowifi_halt;
	eth->data = ss;

        memcpy (eth->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

	/* Get the MAC address */
	if (picowifi_read_mac(eth))
		return 0;

	//picowifi_init(eth); /* we do init here to initialize early */
        r = kthread_create(picowifi_poll_thread, eth, &picowifi_p, "pwipoll");
        if (r)
        {
		return 0;
	}

	if_register(eth); /* should happen in USB stack, but oh-well */

	return 1;
}

void
picowifi_poll(PROC *proc, long dummy)
{
	wake(WAIT_Q, (long)&picowifi_poll_thread);
}

void
picowifi_poll_thread(void *e)
{
	struct eth_device *eth = (struct eth_device *)e;
	struct ueth_data *dev = (struct ueth_data *)eth->data;

	sleep(WAIT_Q, (long)&picowifi_poll_thread);

	while (dev->pusb_dev) 
	{
		picowifi_recv(eth);
		sleep(WAIT_Q, (long)&picowifi_poll_thread);
	}

	kfree(recv_fifo);

	if_deregister(eth); /* should remove now */

	eth->flags = IFF_BROADCAST; /* IFF_DOWN */

	kthread_exit(0);
}

long driver_init(void);
long
driver_init (void)
{
	COOKIE *cookie = COOKIEBASE;
	struct usb_netapi *usbNetAPI = NULL;
	struct usb_eth_prob_dev usb_eth;

        if (cookie)
        {
                while (cookie->name)
                {
                        if (cookie->name == COOKIE_EUSB) {
				usbNetAPI = (struct usb_netapi *)cookie->val;
	                }
			cookie++;
	        }
	}

	if (!usbNetAPI) {
		return -1;
	}

	usb_eth.before_probe = picowifi_eth_before_probe;
	usb_eth.probe = picowifi_eth_probe;
	usb_eth.get_info = picowifi_eth_get_info;

	usbEthID = usbNetAPI->usb_eth_register(&usb_eth);
	if (usbEthID < 0) {
		return -1;
	}

	return 0;
}
