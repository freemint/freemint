/*
 * Modified for the FreeMiNT USB subsystem by Alan Hourihane 2014.
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
#include <mint/osbind.h>

#include "../../usb/src.km/config.h"
#include "../../usb/src.km/usb.h"
#include "../../usb/src.km/udd/udd_defs.h"
#include "../../usb/src.km/usb_api.h"
#include "../../usb/src.km/udd/eth/usb_ether.h"

struct usb_module_api   *api = NULL;
static struct proc *asix_p;
static long usbEthID = -1;

typedef struct
{
        unsigned long  name;
        unsigned long  val;
} COOKIE;

#define COOKIEBASE (*(COOKIE **)0x5a0)

#define eth_device netif

#define CONFIG_SYS_HZ CLOCKS_PER_SEC

void asix_poll_thread(void *);
void asix_poll(PROC *proc, long dummy);

/*
 * Debug section
 */

#if 0
# define DEV_DEBUG	1
#endif

#ifdef DEV_DEBUG
# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	KERNEL_DEBUG x
# define TRACE(x)	KERNEL_TRACE x
# define ASSERT(x)	assert x
#else
# define FORCE(x)	
# define ALERT(x)	KERNEL_ALERT x
# define DEBUG(x)	
# define TRACE(x)	
# define ASSERT(x)	assert x
#endif

/* ASIX AX8817X based USB 2.0 Ethernet Devices */

#define AX_CMD_SET_SW_MII		0x06
#define AX_CMD_READ_MII_REG		0x07
#define AX_CMD_WRITE_MII_REG		0x08
#define AX_CMD_SET_HW_MII		0x0a
#define AX_CMD_READ_EEPROM		0x0b
#define AX_CMD_READ_RX_CTL		0x0f
#define AX_CMD_WRITE_RX_CTL		0x10
#define AX_CMD_WRITE_IPG0		0x12
#define AX_CMD_READ_NODE_ID		0x13
#define AX_CMD_WRITE_NODE_ID		0x14
#define AX88172_CMD_READ_NODE_ID	0x17
#define AX_CMD_READ_PHY_ID		0x19
#define AX_CMD_WRITE_MEDIUM_MODE	0x1b
#define AX_CMD_WRITE_GPIOS		0x1f
#define AX_CMD_SW_RESET			0x20
#define AX_CMD_SW_PHY_SELECT		0x22

#define AX_SWRESET_CLEAR		0x00
#define AX_SWRESET_PRTE			0x04
#define AX_SWRESET_PRL			0x08
#define AX_SWRESET_IPRL			0x20
#define AX_SWRESET_IPPD			0x40

#define AX88772_IPG0_DEFAULT		0x15
#define AX88772_IPG1_DEFAULT		0x0c
#define AX88772_IPG2_DEFAULT		0x12

/* AX88772 & AX88178 Medium Mode Register */
#define AX_MEDIUM_PF		0x0080
#define AX_MEDIUM_JFE		0x0040
#define AX_MEDIUM_TFC		0x0020
#define AX_MEDIUM_RFC		0x0010
#define AX_MEDIUM_ENCK		0x0008
#define AX_MEDIUM_AC		0x0004
#define AX_MEDIUM_FD		0x0002
#define AX_MEDIUM_GM		0x0001
#define AX_MEDIUM_SM		0x1000
#define AX_MEDIUM_SBP		0x0800
#define AX_MEDIUM_PS		0x0200
#define AX_MEDIUM_RE		0x0100

#define AX88178_MEDIUM_DEFAULT	\
	(AX_MEDIUM_PS | AX_MEDIUM_FD | AX_MEDIUM_AC | \
	 AX_MEDIUM_RFC | AX_MEDIUM_TFC | AX_MEDIUM_JFE | \
	 AX_MEDIUM_RE)

#define AX88772_MEDIUM_DEFAULT	\
	(AX_MEDIUM_FD | AX_MEDIUM_RFC | \
	 AX_MEDIUM_TFC | AX_MEDIUM_PS | \
	 AX_MEDIUM_AC | AX_MEDIUM_RE)
	
#define AX88172_MEDIUM_DEFAULT \
	(AX_MEDIUM_FD | AX_MEDIUM_RFC | \
	 AX_MEDIUM_AC)

/* AX88772 & AX88178 RX_CTL values */
#define AX_RX_CTL_SO			0x0080
#define AX_RX_CTL_AB			0x0008

#define AX_DEFAULT_RX_CTL	\
	(AX_RX_CTL_SO | AX_RX_CTL_AB)

/* GPIO 2 toggles */
#define AX_GPIO_GPO2EN		0x10	/* GPIO2 Output enable */
#define AX_GPIO_GPO_2		0x20	/* GPIO2 Output value */
#define AX_GPIO_RSE		0x80	/* Reload serial EEPROM */

/* local defines */
#define ASIX_BASE_NAME "asx"
#define USB_CTRL_SET_TIMEOUT 5000
#define USB_CTRL_GET_TIMEOUT 5000
#define USB_BULK_SEND_TIMEOUT 5000
#define USB_BULK_RECV_TIMEOUT 5000

#define AX_RX_URB_SIZE 2048
#define PHY_CONNECT_TIMEOUT 5000

/* asix_flags defines */
#define FLAG_NONE			0
#define FLAG_TYPE_AX88172	(1U << 0)
#define FLAG_TYPE_AX88772	(1U << 1)
#define FLAG_TYPE_AX88772B	(1U << 2)
#define FLAG_EEPROM_MAC		(1U << 3) /* initial mac address in eeprom */

void asix_eth_before_probe(void *api);
long asix_eth_get_info(void *vdev, void *vss, void *veth);
long asix_eth_probe(void *vdev, unsigned int ifnum, void *vss);
/* local vars */

/* driver private */
struct asix_private {
	long flags;
};

/*
 * Asix infrastructure commands
 */
static int asix_write_cmd(struct ueth_data *dev, u8 cmd, u16 value, u16 index,
			     u16 size, void *data)
{
	long len;

	DEBUG(("asix_write_cmd() cmd=0x%02x value=0x%04x index=0x%04x "
		"size=%d\n", cmd, value, index, size));

	len = usb_control_msg(
		dev->pusb_dev,
		usb_sndctrlpipe(dev->pusb_dev, 0),
		cmd,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		value,
		index,
		data,
		size,
		USB_CTRL_SET_TIMEOUT);

	return len == size ? 0 : -1;
}

static int asix_read_cmd(struct ueth_data *dev, u8 cmd, u16 value, u16 index,
			    u16 size, void *data)
{
	long len;

	DEBUG(("asix_read_cmd() cmd=0x%02x value=0x%04x index=0x%04x size=%d\n",
		cmd, value, index, size));

	len = usb_control_msg(
		dev->pusb_dev,
		usb_rcvctrlpipe(dev->pusb_dev, 0),
		cmd,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		value,
		index,
		data,
		size,
		USB_CTRL_GET_TIMEOUT);
	return len == size ? 0 : -1;
}

static inline int asix_set_sw_mii(struct ueth_data *dev)
{
	int ret;

	ret = asix_write_cmd(dev, AX_CMD_SET_SW_MII, 0x0000, 0, 0, NULL);
	if (ret < 0)
		DEBUG(("Failed to enable software MII access\n"));
	return ret;
}

static inline int asix_set_hw_mii(struct ueth_data *dev)
{
	int ret;

	ret = asix_write_cmd(dev, AX_CMD_SET_HW_MII, 0x0000, 0, 0, NULL);
	if (ret < 0)
		DEBUG(("Failed to enable hardware MII access\n"));
	return ret;
}

static int asix_mdio_read(struct ueth_data *dev, int phy_id, int loc)
{
	__u16 res[1];

	asix_set_sw_mii(dev);
	asix_read_cmd(dev, AX_CMD_READ_MII_REG, phy_id, (__u16)loc, 2, res);
	asix_set_hw_mii(dev);

	DEBUG(("asix_mdio_read() phy_id=0x%02x, loc=0x%02x, returns=0x%04x\n",
			phy_id, loc, le2cpu16(*res)));

	return le2cpu16(*res);
}

static void
asix_mdio_write(struct ueth_data *dev, int phy_id, int loc, int val)
{
	__u16 res[1];
	*res = cpu2le16(val);

	DEBUG(("asix_mdio_write() phy_id=0x%02x, loc=0x%02x, val=0x%04x\n",
			phy_id, loc, val));
	asix_set_sw_mii(dev);
	asix_write_cmd(dev, AX_CMD_WRITE_MII_REG, phy_id, (__u16)loc, 2, res);
	asix_set_hw_mii(dev);
}

/*
 * Asix "high level" commands
 */
static int asix_sw_reset(struct ueth_data *dev, u8 flags)
{
	int ret;

	ret = asix_write_cmd(dev, AX_CMD_SW_RESET, flags, 0, 0, NULL);
	if (ret < 0)
		DEBUG(("Failed to send software reset: %02x\n", ret));
	else
		mdelay(150);

	return ret;
}

static inline int asix_get_phy_addr(struct ueth_data *dev)
{
	u8 buf[2];

	int ret = asix_read_cmd(dev, AX_CMD_READ_PHY_ID, 0, 0, 2, buf);

	DEBUG(("asix_get_phy_addr()\n"));

	if (ret < 0) {
		DEBUG(("Error reading PHYID register: %02x\n", ret));
		goto out;
	}
	DEBUG(("asix_get_phy_addr() returning 0x%02x%02x\n", buf[0], buf[1]));
	ret = buf[1];

out:
	return ret;
}

static int asix_write_medium_mode(struct ueth_data *dev, u16 mode)
{
	int ret;

	DEBUG(("asix_write_medium_mode() - mode = 0x%04x\n", mode));
	ret = asix_write_cmd(dev, AX_CMD_WRITE_MEDIUM_MODE, mode,
			0, 0, NULL);
	if (ret < 0) {
		DEBUG(("Failed to write Medium Mode mode to 0x%04x: %02x\n",
			mode, ret));
	}
	return ret;
}

static u16 asix_read_rx_ctl(struct ueth_data *dev)
{
	__u16 v[1];

	int ret = asix_read_cmd(dev, AX_CMD_READ_RX_CTL, 0, 0, 2, v);

	if (ret < 0)
		DEBUG(("Error reading RX_CTL register: %02x\n", ret));
	else
		ret = le2cpu16(*v);
	return ret;
}

static int asix_write_rx_ctl(struct ueth_data *dev, u16 mode)
{
	int ret;

	DEBUG(("asix_write_rx_ctl() - mode = 0x%04x\n", mode));
	ret = asix_write_cmd(dev, AX_CMD_WRITE_RX_CTL, mode, 0, 0, NULL);
	if (ret < 0) {
		DEBUG(("Failed to write RX_CTL mode to 0x%04x: %02x\n",
				mode, ret));
	}
	return ret;
}

static int asix_write_gpio(struct ueth_data *dev, u16 value, int sleepy)
{
	int ret;

	DEBUG(("asix_write_gpio() - value = 0x%04x\n", value));
	ret = asix_write_cmd(dev, AX_CMD_WRITE_GPIOS, value, 0, 0, NULL);
	if (ret < 0) {
		DEBUG(("Failed to write GPIO value 0x%04x: %02x\n",
			value, ret));
	}
	if (sleepy)
		mdelay(sleepy);

	return ret;
}

/*
 * mii commands
 */

/*
 * mii_nway_restart - restart NWay (autonegotiation) for this interface
 *
 * Returns 0 on success, negative on error.
 */
static int mii_nway_restart(struct ueth_data *dev)
{
	int bmcr;
	int r = -1;

	/* if autoneg is off, it's an error */
	bmcr = asix_mdio_read(dev, dev->phy_id, MII_BMCR);

	if (bmcr & BMCR_ANENABLE) {
		bmcr |= BMCR_ANRESTART;
		asix_mdio_write(dev, dev->phy_id, MII_BMCR, bmcr);
		r = 0;
	}

	return r;
}

static int asix_read_mac(struct eth_device *eth)
{
	struct ueth_data *dev = (struct ueth_data *)eth->data;
	struct asix_private *priv = (struct asix_private *)dev->dev_priv;
	int i;
	unsigned char buf[ETH_ALEN];

	if (priv->flags & FLAG_EEPROM_MAC) {
		for (i = 0; i < (ETH_ALEN >> 1); i++) {
			if (asix_read_cmd(dev, AX_CMD_READ_EEPROM,
					  0x04 + i, 0, 2, buf) < 0) {
				DEBUG(("Failed to read SROM address 04h.\n"));
				return -1;
			}
			memcpy((eth->hwlocal.adr.bytes + i * 2), buf, 2);
		}
	} else if (priv->flags & FLAG_TYPE_AX88172) {
		if (asix_read_cmd(dev, AX88172_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, buf)
		     < 0) {
			DEBUG(("Failed to read MAC address.\n"));
			return -1;
		}
		memcpy(eth->hwlocal.adr.bytes, buf, ETH_ALEN);
	} else {
		if (asix_read_cmd(dev, AX_CMD_READ_NODE_ID, 0, 0, ETH_ALEN, buf)
		     < 0) {
			DEBUG(("Failed to read MAC address.\n"));
			return -1;
		}
		memcpy(eth->hwlocal.adr.bytes, buf, ETH_ALEN);
	}

	return 0;
}

static int asix_basic_reset_172(struct ueth_data *dev)
{
	if (asix_write_gpio(dev,
			AX_GPIO_RSE | AX_GPIO_GPO_2 | AX_GPIO_GPO2EN, 5) < 0)
		return -1;
				     
	if (asix_write_rx_ctl(dev, 0x0000) < 0) {
        	DEBUG(("asix_read_rx_ctl failed\n"));
		return -1;
	}

	dev->phy_id = asix_get_phy_addr(dev);
	if (dev->phy_id < 0)
		DEBUG(("Failed to read phy id\n"));

	asix_mdio_write(dev, dev->phy_id, MII_BMCR, BMCR_RESET);
	asix_mdio_write(dev, dev->phy_id, MII_ADVERTISE,
			ADVERTISE_ALL | ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP);
	mii_nway_restart(dev);

	if (asix_write_medium_mode(dev, AX88172_MEDIUM_DEFAULT) < 0)
		return -1;

	return 0;
}

static int asix_basic_reset_772(struct ueth_data *dev)
{
	int embd_phy;
	u16 rx_ctl;

	if (asix_write_gpio(dev,
			AX_GPIO_RSE | AX_GPIO_GPO_2 | AX_GPIO_GPO2EN, 5) < 0)
		return -1;

	/* 0x10 is the phy id of the embedded 10/100 ethernet phy */
	embd_phy = ((asix_get_phy_addr(dev) & 0x1f) == 0x10 ? 1 : 0);
	if (asix_write_cmd(dev, AX_CMD_SW_PHY_SELECT,
				embd_phy, 0, 0, NULL) < 0) {
        	DEBUG(("Select PHY #1 failed"));
		return -1;
	}

	if (asix_sw_reset(dev, AX_SWRESET_IPPD | AX_SWRESET_PRL) < 0) {
        	DEBUG(("asix_sw_reset failed"));
		return -1;
	}

	if (asix_sw_reset(dev, AX_SWRESET_CLEAR) < 0) {
        	DEBUG(("asix_sw_reset clear failed"));
		return -1;
	}

	if (embd_phy) {
		if (asix_sw_reset(dev, AX_SWRESET_IPRL) < 0) {
			return -1;
		}
	} else {
		if (asix_sw_reset(dev, AX_SWRESET_PRTE) < 0) {
			return -1;
		}
	}

	rx_ctl = asix_read_rx_ctl(dev);
	DEBUG(("RX_CTL is 0x%04x after software reset\n", rx_ctl));
	if (asix_write_rx_ctl(dev, 0x0000) < 0) {
		return -1;
	}

	rx_ctl = asix_read_rx_ctl(dev);
	DEBUG(("RX_CTL is 0x%04x setting to 0x0000\n", rx_ctl));

	dev->phy_id = asix_get_phy_addr(dev);
	if (dev->phy_id < 0)
		DEBUG(("Failed to read phy id\n"));

	asix_mdio_write(dev, dev->phy_id, MII_BMCR, BMCR_RESET);
	asix_mdio_write(dev, dev->phy_id, MII_ADVERTISE,
			ADVERTISE_ALL | ADVERTISE_CSMA);
	mii_nway_restart(dev);

	if (asix_write_medium_mode(dev, AX88772_MEDIUM_DEFAULT) < 0)
		return -1;

	if (asix_write_cmd(dev, AX_CMD_WRITE_IPG0,
				AX88772_IPG0_DEFAULT | AX88772_IPG1_DEFAULT,
				AX88772_IPG2_DEFAULT, 0, NULL) < 0) {
		DEBUG(("Write IPG,IPG1,IPG2 failed\n"));
		return -1;
	}

	return 0;
}

/*
 * Asix callbacks
 */
static long asix_init(struct eth_device *eth)
{
	struct ueth_data	*dev = (struct ueth_data *)eth->data;
	int timeout = 0;
#define TIMEOUT_RESOLUTION 50	/* ms */
	int link_detected;
	unsigned char rx_ctl;

	DEBUG(("** %s()\n", __func__));

	if (asix_write_rx_ctl(dev, AX_DEFAULT_RX_CTL) < 0)
		goto out_err;

	rx_ctl = asix_read_rx_ctl(dev);

	do {
		link_detected = asix_mdio_read(dev, dev->phy_id, MII_BMSR) &
			BMSR_LSTATUS;
		if (!link_detected) {
			if (timeout == 0) {
				ALERT(("Waiting for Ethernet connection... "));
			}
			mdelay(TIMEOUT_RESOLUTION);
			timeout += TIMEOUT_RESOLUTION;
		}
	} while (!link_detected && timeout < PHY_CONNECT_TIMEOUT);
	if (link_detected) {
		if (timeout != 0) {
			ALERT(("done.\n"));
		}
	} else {
		ALERT(("unable to connect.\n"));
		goto out_err;
	}

	return 0;
out_err:
	return -1;
}

static long asix_send(struct eth_device *eth, void *packet, long length)
{
	struct ueth_data *dev = (struct ueth_data *)eth->data;
	long err = 0;
	u32 packet_len;
	long actual_len = 0;
	long size;
	unsigned char msg[length + sizeof(packet_len) + 3 /* pad */];

	DEBUG(("** %s(), len %d\n", __func__, length));
	if (dev->pusb_dev == 0) {
		return E_OK;
	}

	packet_len = ((length ^ 0x0000ffff) << 16) + length;
	packet_len = cpu2le32(packet_len);

	memcpy(msg, &packet_len, sizeof(packet_len));
	memcpy(msg + sizeof(packet_len), (void *)packet, length);
	if (length & 1)
		length++;

	size = length + sizeof(packet_len);

	err = usb_bulk_msg(dev->pusb_dev,
				usb_sndbulkpipe(dev->pusb_dev, (long)dev->ep_out),
				(void *)msg,
				size,
				&actual_len,
				USB_BULK_SEND_TIMEOUT, 0);
	if (err != 0)
		return ENOMEM;
	else 
		return E_OK;
}

static int asix_recv(struct eth_device *eth)
{
	unsigned char recv_buf[AX_RX_URB_SIZE];
	struct ueth_data *dev = (struct ueth_data *)eth->data;
	unsigned char *buf_ptr;
	long err;
	long actual_len;
	u32 packet_len;
	BUF *buf;
//	int i;

//	DEBUG(("** %s()\n", __func__));

	/* while (1) */ {
		if (dev->pusb_dev == 0) {
			err = -1;
			goto out;
		}

		err = usb_bulk_msg(dev->pusb_dev,
					usb_rcvbulkpipe(dev->pusb_dev, (long)dev->ep_in),
					(void *)recv_buf,
					AX_RX_URB_SIZE,
					&actual_len,
					USB_BULK_RECV_TIMEOUT, 1);
		if (err < 0) {
			DEBUG(("Rx: failed to receive\n"));
			goto out;
		}
		if (actual_len > AX_RX_URB_SIZE) {
			DEBUG(("Rx: received too many bytes %d\n", actual_len));
			err = -1;
			goto out;
		}

		buf_ptr = recv_buf;
		while (actual_len > 0) {
			/*
			 * 1st 4 bytes contain the length of the actual data as two
			 * complementary 16-bit words. Extract the length of the data.
			 */
			if (actual_len < sizeof(packet_len)) {
				DEBUG(("Rx: incomplete packet length\n"));
				err = -1;
				goto out;
			}

			memcpy(&packet_len, buf_ptr, sizeof(packet_len));
			packet_len = le2cpu32(packet_len);

			if (((~packet_len >> 16) & 0x7ff) != (packet_len & 0x7ff)) {
				DEBUG(("Rx: malformed packet length: %#x (%#x:%#x)\n",
					packet_len, (~packet_len >> 16) & 0x7ff,
					packet_len & 0x7ff));
				err = -1;
				goto out;
			}

			packet_len = packet_len & 0x7ff;
			if (packet_len > actual_len - sizeof(packet_len)) {
				err = -1;
				goto out;
			}

			buf = buf_alloc (packet_len, 0, BUF_ATOMIC);
       			if (!buf)
		        {
				DEBUG (("asix_recv: out of mem (buf_alloc failed)"));
				eth->in_errors++;
				err = -1;
				goto out;
			}
			memcpy(buf->dstart, buf_ptr + sizeof(packet_len), packet_len);
			buf->dend = buf->dstart + packet_len;

			if (eth->bpf)
				bpf_input (eth, buf);

			/* and enqueue packet */
			if (!if_input (eth, buf, 0, eth_remove_hdr (buf)))
				eth->in_packets++;
			else
				eth->in_errors++;

			/* Adjust for next iteration. Packets are padded to 16-bits */
			if (packet_len & 1)
				packet_len++;

			actual_len -= (sizeof(packet_len) + packet_len);
			buf_ptr += (sizeof(packet_len) + packet_len);
		}
	}

out:
#ifdef M68000
	addroottimeout(50, asix_poll, 0);
#else
	addroottimeout(10, asix_poll, 0);
#endif

	return err;
}

static long _cdecl
asix_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF *nbuf;

        nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
        if (!nbuf)
        {
                DEBUG (("asix: eth_build_hdr failed, out of memory!"));

                nif->out_errors++;
                return ENOMEM;
        }

        /* pass to upper layer */
        if (nif->bpf)
		bpf_input (nif, nbuf);

	if (asix_send(nif, nbuf->dstart, nbuf->dend - nbuf->dstart) < 0) {
		buf_deref (nbuf, BUF_NORMAL);
                nif->out_errors++;
		return ENOMEM;
	}

	buf_deref (nbuf, BUF_NORMAL);
	nif->out_packets++;

	return E_OK;
}

static long asix_halt(struct eth_device *eth)
{
	DEBUG(("** %s()\n", __func__));

	return 0;
}

static long _cdecl
asix_ioctl (struct netif *nif, short cmd, long arg)
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
 * Asix probing functions
 */
void asix_eth_before_probe(void *a)
{
	api = a;
}

struct asix_dongle {
	unsigned short vendor;
	unsigned short product;
	int flags;
};

static const struct asix_dongle const asix_dongles[] = {
	{ 0x05ac, 0x1402, FLAG_TYPE_AX88772 },	/* Apple USB Ethernet Adapter */
	{ 0x0b95, 0x772a, FLAG_TYPE_AX88772 },  /* Cables-to-Go USB Ethernet Adapter */
	{ 0x0b95, 0x7720, FLAG_TYPE_AX88772 },	/* Trendnet TU2-ET100 V3.0R */
	{ 0x0b95, 0x1720, FLAG_TYPE_AX88172 },	/* SMC 2209 - others ?? */
	{ 0x0db0, 0xa877, FLAG_TYPE_AX88772 },	/* MSI - ASIX 88772a */
	{ 0x13b1, 0x0018, FLAG_TYPE_AX88172 },	/* Linksys 200M v2.1 */
	{ 0x1557, 0x7720, FLAG_TYPE_AX88772 },	/* 0Q0 cable ethernet */
	{ 0x2001, 0x1a00, FLAG_TYPE_AX88172 },	/* D-Link DUB-E100 */
	{ 0x07d1, 0x3c05, FLAG_TYPE_AX88772 },	/* D-Link DUB-E100 H/W Ver B1 */
	{ 0x2001, 0x3c05, FLAG_TYPE_AX88772 },  /* D-Link DUB-E100 H/W Ver B1 Alternate */
	{ 0x2001, 0x1a02, FLAG_TYPE_AX88772 },	/* D-Link DUB-E100 H/W Ver C1 */
	/* ASIX 88772B */
	{ 0x0b95, 0x772b, FLAG_TYPE_AX88772B | FLAG_EEPROM_MAC },
	{ 0x0000, 0x0000, FLAG_NONE }	/* END - Do not remove */
};

/* Probe to see if a new device is actually an asix device */
long
asix_eth_probe(void *vdev, unsigned int ifnum, void *vss)
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

	for (i = 0; asix_dongles[i].vendor != 0; i++) {
		if (dev->descriptor.idVendor == asix_dongles[i].vendor &&
		    dev->descriptor.idProduct == asix_dongles[i].product)
			/* Found a supported dongle */
			break;
	}

	if (asix_dongles[i].vendor == 0)
		return 0;

	memset(ss, 0, sizeof(struct ueth_data));

	/* At this point, we know we've got a live one */
	DEBUG(("\n\nUSB Ethernet device detected: %#04x:%#04x\n",
	      dev->descriptor.idVendor, dev->descriptor.idProduct));

	/* Initialize the ueth_data structure with some useful info */
	ss->ifnum = ifnum;
	ss->pusb_dev = dev;
	ss->subclass = iface_desc->bInterfaceSubClass;
	ss->protocol = iface_desc->bInterfaceProtocol;

	/* alloc driver private */
	ss->dev_priv = kmalloc(sizeof(struct asix_private));
	if (!ss->dev_priv)
		return 0;
	memset(ss->dev_priv, 0, sizeof(struct asix_private));

	((struct asix_private *)ss->dev_priv)->flags = asix_dongles[i].flags;

	/*
	 * We are expecting a minimum of 3 endpoints - in, out (bulk), and
	 * int. We will ignore any others.
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

		/* is it an interrupt endpoint? */
		if ((iface->ep_desc[i].bmAttributes &
		    USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
			ss->ep_int = iface->ep_desc[i].bEndpointAddress &
				USB_ENDPOINT_NUMBER_MASK;
			ss->irqinterval = iface->ep_desc[i].bInterval;
		}
	}

	/* Do some basic sanity checks, and bail if we find a problem */
	if (usb_set_interface(dev, iface_desc->bInterfaceNumber, 0) ||
	    !ss->ep_in || !ss->ep_out || !ss->ep_int) {
		DEBUG(("Problems with device\n"));
		kfree(ss->dev_priv);
		return 0;
	}
	dev->privptr = (void *)ss;
	return 1;
}

long 
asix_eth_get_info(void *vdev, void *vss, void *veth)
{
	struct usb_device *dev = vdev;
	struct ueth_data *ss = vss;
	struct eth_device *eth = veth;
	struct asix_private *priv = (struct asix_private *)ss->dev_priv;
	long r;

	(void) dev;

	if (!eth) {
		DEBUG(("%s: missing parameter.\n", __func__));
		return 0;
	}

	strcpy(eth->name, ASIX_BASE_NAME);
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
	eth->maxpackets = 1;
	eth->open = asix_init;
	eth->ioctl = asix_ioctl;
	eth->output = asix_output;
	eth->close = asix_halt;
	eth->data = ss;

        memcpy (eth->hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

	if (priv->flags & FLAG_TYPE_AX88172) {
		if (asix_basic_reset_172(ss))
			return 0;
	} else {
		if (asix_basic_reset_772(ss))
			return 0;
	}

	/* Get the MAC address */
	if (asix_read_mac(eth))
		return 0;

	//asix_init(eth); /* we do init here to initialize early */
        r = kthread_create(asix_poll_thread, eth, &asix_p, "asixpoll");
        if (r)
        {
		return 0;
	}

	if_register(eth); /* should happen in USB stack, but oh-well */

	return 1;
}

void
asix_poll(PROC *proc, long dummy)
{
	wake(WAIT_Q, (long)&asix_poll_thread);
}

void
asix_poll_thread(void *e)
{
	struct eth_device *eth = (struct eth_device *)e;
	struct ueth_data *dev = (struct ueth_data *)eth->data;

	while (dev->pusb_dev) 
	{
		asix_recv(eth);
		sleep(WAIT_Q, (long)&asix_poll_thread);
	}

	kfree(dev->dev_priv);

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

	usb_eth.before_probe = asix_eth_before_probe;
	usb_eth.probe = asix_eth_probe;
	usb_eth.get_info = asix_eth_get_info;

	usbEthID = usbNetAPI->usb_eth_register(&usb_eth);
	if (usbEthID < 0) {
		return -1;
	}

	return 0;
}
