/*****************************************************************************
*******************************************************************************/




/*
 * Svethlana ethernet driver for FreeMiNT.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007-2016 Torbjorn and Henrik Gilda
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *	Svethlana packet driver version 0.9
 *  SuperVidel FW v10 is required by this driver
 *
 *	Usage:
 *		ifconfig en0 addr u.v.w.x
 *		route add u.v.w.x en0
 *
 *		20160426	/Henrik and Torbj�rn Gild�
 *
 */

#include "global.h"

#include "buf.h"
#include "cookie.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"

#include "netinfo.h"
#include "mint/delay.h"
#include "mint/mdelay.h"
#include "mint/sockio.h"
#include "mint/endian.h"

#include "sv_regs.h"
#include "svethlana_i6.h"

#include <mint/osbind.h>
#define ct60_vmalloc(mode,value) (unsigned long)trap_14_wwl((short)(0xc60e),(short)(mode),(unsigned long)(value))

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')

/*
 * From main.c
 */
extern long driver_init (void);

// to make compiler happy
void _cdecl svethlana_int (void);


/*
 * Our interface structure
 */
static struct netif if_svethlana;



/*
 * Prototypes for our service functions
 */
static long	svethlana_open		(struct netif *);
static long	svethlana_close		(struct netif *);
static long	svethlana_output	(struct netif *, BUF *, const char *, short, short);
static long	svethlana_ioctl		(struct netif *, short, long);
static long	svethlana_config	(struct netif *, struct ifopt *);

//does actual sending of packets for svethlana_output() and svethlana_service()
static long send_packet			(struct netif *nif, BUF *nbuf, long buf_alloc_type, uint32 slot, uint32 last_packet);

// Service function to check for incoming and outgoing packets
static void svethlana_service (struct netif * nif, uint32 int_src);

// For our interrupt
static void svethlana_install_int (void);

void Init_BD(void);
int32 Check_Rx_Buffers(void);


/* Convert single hex char to short */
short ch2i(char);

// Convert long to hex ascii
void hex2ascii(ulong l, uchar* c);


// Variable for locking out interrupt when accessing the hardware
//static volatile int in_use = 0;

// If this variable equals 1, then the interrupt will do nothing until init is done (=0)
static volatile int initializing = 1;

//static volatile int autoneg = 0;

// Keep track of when sending so we don't get fooled by TXEMPTY
//static volatile int sending = 0;

//handle for all logging
//static ushort loghandle;

static char message[100];

//global MAC address in longword format for RX use
static unsigned long mac_addr[2];

//next TX slot to be used
static volatile uint32	slot_index = 0UL;

//We keep track of which slot we checked last time this function was called.
//Since the MAC controller probably does round robin on the RX slots, we
//shouldn't require Mintnet to reorganise our packets, if we always started
//with checking slot 0.
static uint32 cur_rx_slot = 0;

//static volatile uint32	has_printed_functions = 0UL;

//static volatile uint32	in_queue = 0UL;

//static uchar upperleft[] = {27,'H',0};
//static uchar col40[] = {27,'Y',32,72,0};

//This is the pointer to the whole memory block for all the packet buffers needed.
//It's allocated in Init_BD() and deallocated at shutdown. The allocation is and
//must be done in SuperVidel video RAM (DDR RAM).
static char* packets_base = 0;

/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
svethlana_open (struct netif *nif)
{
//	short		tmp;

//	in_use = 1;

//	c_conws("Svethlana up!\r\n");

	//enable RX, RX error, TX, TX error and Busy int sources
	ETH_INT_MASK = ETH_INT_MASK_RXF | ETH_INT_MASK_RXE | ETH_INT_MASK_TXE | ETH_INT_MASK_TXB | ETH_INT_MASK_BUSY;

	//Enable Transmit, full duplex, and CRC appending
	ETH_MODER = ETH_MODER_TXEN | ETH_MODER_RXEN | ETH_MODER_FULLD | ETH_MODER_CRCEN | ETH_MODER_RECSMALL | ETH_MODER_PAD;

	initializing = 0;
//	in_use = 0;

	return 0;
}

/*
 * Opposite of svethlana_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long
svethlana_close (struct netif *nif)
{
	// disable RX and TX
	ETH_MODER = 0;

	//disable RX and RX error int sources BEFORE removing I6 handler
	ETH_INT_MASK = 0;

	c_conws("Svethlana down!\r\n");

	initializing = 1;

	return 0;
}

/*
 * This routine is responsible for enqueing a packet for later sending.
 * The packet it passed in `buf', the destination hardware address and
 * length in `hwaddr' and `hwlen' and the type of the packet is passed
 * in `pktype'.
 *
 * `hwaddr' is guaranteed to be of type nif->hwtype and `hwlen' is
 * garuanteed to be equal to nif->hwlocal.len.
 *
 * `pktype' is currently one of (definitions in if.h):
 *	PKTYPE_IP for IP packets,
 *	PKTYPE_ARP for ARP packets,
 *	PKTYPE_RARP for reverse ARP packets.
 *
 * These constants are equal to the ethernet protocol types, ie. an
 * Ethernet driver may use them directly without prior conversion to
 * write them into the `proto' field of the ethernet header.
 *
 * If the hardware is currently busy, then you can use the interface
 * output queue (nif->snd) to store the packet for later transmission:
 *	if_enqueue (&nif->snd, buf, buf->info).
 *
 * `buf->info' specifies the packet's delivering priority. if_enqueue()
 * uses it to do some priority queuing on the packets, ie. if you enqueue
 * a high priority packet it may jump over some lower priority packets
 * that were already in the queue (ie that is *no* FIFO queue).
 *
 * You can dequeue a packet later by doing:
 *	buf = if_dequeue (&nif->snd);
 *
 * This will return NULL if no more packets are left in the queue.
 *
 * The buffer handling uses the structure BUF that is defined in buf.h.
 * Basically a BUF looks like this:
 *
 * typedef struct {
 *	long buflen;
 *	char *dstart;
 *	char *dend;
 *	...
 *	char data[0];
 * } BUF;
 *
 * The structure consists of BUF.buflen bytes. Up until BUF.data there are
 * some header fields as shown above. Beginning at BUF.data there are
 * BUF.buflen - sizeof (BUF) bytes (called userspace) used for storing the
 * packet.
 *
 * BUF.dstart must always point to the first byte of the packet contained
 * within the BUF, BUF.dend points to the first byte after the packet.
 *
 * BUF.dstart should be word aligned if you pass the BUF to any MintNet
 * functions! (except for the buf_* functions itself).
 *
 * BUF's are allocated by
 *	nbuf = buf_alloc (space, reserve, mode);
 *
 * where `space' is the size of the userspace of the BUF you need, `reserve'
 * is used to set BUF.dstart = BUF.dend = BUF.data + `reserve' and mode is
 * one of
 *	BUF_NORMAL for calls from kernel space,
 *	BUF_ATOMIC for calls from interrupt handlers.
 *
 * buf_alloc() returns NULL on failure.
 *
 * Usually you need to pre- or postpend some headers to the packet contained
 * in the passed BUF. To make sure there is enough space in the BUF for this
 * use
 *	nbuf = buf_reserve (obuf, reserve, where);
 *
 * where `obuf' is the BUF where you want to reserve some space, `reserve'
 * is the amount of space to reserve and `where' is one of
 *	BUF_RESERVE_START for reserving space before BUF.dstart
 *	BUF_RESERVE_END for reserving space after BUF.dend
 *
 * Note that buf_reserve() returns pointer to a new buffer `nbuf' (possibly
 * != obuf) that is a clone of `obuf' with enough space allocated. `obuf'
 * is no longer existant afterwards.
 *
 * However, if buf_reserve() returns NULL for failure then `obuf' is
 * untouched.
 *
 * buf_reserve() does not modify the BUF.dstart or BUF.dend pointers, it
 * only makes sure you have the space to do so.
 *
 * In the worst case (if the BUF is to small), buf_reserve() allocates a new
 * BUF and copies the old one to the new one (this is when `nbuf' != `obuf').
 *
 * To avoid this you should reserve enough space when calling buf_alloc(), so
 * buf_reserve() does not need to copy. This is what MintNet does with the BUFs
 * passed to the output function, so that copying is never needed. You should
 * do the same for input BUFs, ie allocate the packet as eg.
 *	buf = buf_alloc (nif->mtu+sizeof (eth_hdr)+100, 50, BUF_ATOMIC);
 *
 * Then up to nif->mtu plus the length of the ethernet header bytes long
 * frames may ne received and there are still 50 bytes after and before
 * the packet.
 *
 * If you have sent the contents of the BUF you should free it by calling
 *	buf_deref (`buf', `mode');
 *
 * where `buf' should be freed and `mode' is one of the modes described for
 * buf_alloc().
 *
 * Functions that can be called from interrupt:
 *	buf_alloc (..., ..., BUF_ATOMIC);
 *	buf_deref (..., BUF_ATOMIC);
 *	if_enqueue ();
 *	if_dequeue ();
 *	if_input ();
 *	eth_remove_hdr ();
 *	addroottimeout (..., ..., 1);
 */


static long
svethlana_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF			*nbuf;
	int32			result;
//	unsigned char	littlemem;
//	static	uchar	message[100];

//	unsigned long	timeval;


	//c_conws("Output\r\n");		//debug

	//*ETH_REG = (*ETH_REG) | 0x80;	//enable LED2

	/*
	 * Attach eth header. MintNet provides you with the eth_build_hdr
	 * function that attaches an ethernet header to the packet in
	 * buf. It takes the BUF (buf), the interface (nif), the hardware
	 * address (hwaddr) and the packet type (pktype).
	 *
	 * Returns NULL if the header could not be attached (the passed
	 * buf is thrown away in this case).
	 *
	 * Otherwise a pointer to a new BUF with the packet and attached
	 * header is returned and the old buf pointer is no longer valid.
	 */
	nbuf = eth_build_hdr (buf, nif, hwaddr, pktype);
	if ( ((uint32)nbuf) == 0UL)
	{
		//c_conws("eth_build_hdr() failed!\r\n");
		nif->out_errors++;
		//*ETH_REG = (*ETH_REG) & 0x7F;	//disable LED2
		return ENOMEM;
	}
	nif->out_packets++;

	/*
	 * Here you should either send the packet to the hardware or
	 * enqueue the packet and send the next packet as soon as
	 * the hardware is finished.
	 *
	 * If you are done sending the packet free it with buf_deref().
	 *
	 * Before sending it pass it to the packet filter.
	 */
	if (nif->bpf)
		bpf_input (nif, nbuf);


	//in_use = 1;											//don't let interrupt disturb us now

	// Check available memory in LAN91C111
	//Eth_set_bank(0);
//	littlemem = (*LAN_MIR) & 0x00ff;
//	if (len == 0)
//	{
//		c_conws("Insufficient memory in LAN91C111!\r\n");
//	}

	//shut out TX and RX interrupt, so we can send our packet safely
//	ETH_INT_MASK = 0UL;
	//Turn off interrupts completely
	int_off();

	result = send_packet(nif, nbuf, BUF_NORMAL, slot_index, slot_index == (ETH_PKT_BUFFS-1) );

	int_on();

	if(result == 0L)
	{
		//Use the next TX slot for next packet to send
		slot_index = (slot_index + 1) & (ETH_PKT_BUFFS-1);
	}
	else if(result == -2L)
	{
//		//packet too large, just throw away
//		buf_deref(nbuf, BUF_NORMAL);

		//Turn on TX interrupt sources again
//		ETH_INT_MASK = ETH_INT_MASK_RXF | ETH_INT_MASK_RXE | ETH_INT_MASK_TXE | ETH_INT_MASK_TXB;

		c_conws("Sveth_output: too large\r\n");

//		int_on();
//		return EMSGSIZE;
		return E_OK;
	}
	else
	{
		//no free buffer, enqueue the nbuf and wait for TX interrupt
		if_enqueue (&nif->snd, nbuf, nbuf->info);
//		in_queue++;
	}

	//Turn on TX and RX interrupt sources again
//	ETH_INT_MASK = ETH_INT_MASK_RXF | ETH_INT_MASK_RXE | ETH_INT_MASK_TXE | ETH_INT_MASK_TXB;

	//*ETH_REG = (*ETH_REG) & 0x7F;	//disable LED2
	return E_OK;
}


/*This function sends a packet, but it should only be called by svethlana_output() or
  the svethlana_service() routine.
*/
//There is a risk that the TX ISR gets here too, when the svethlana_output() (working in usermode)
//is alredy writing a packet to the MAC controller. Therefore TX interrupt must be shut off before
//calling send_packet(). This assumes that a TX interrupt is not missed while the TXIE is disabled (check
//this in the MAC controller docs).
static long send_packet	(struct netif *nif, BUF *nbuf, long buf_alloc_type, uint32 slot, uint32 last_packet)
{
	uint32					 i;
	volatile	uint32 j;
	volatile	uint32 *datapnt;
	volatile	uint32 *eth_dst_pnt;
	uint32					 rounded_len;
	uint32					 origlen;
//	uchar	message[80];


	//calculate packet length
	origlen = (nbuf->dend) - (nbuf->dstart);

	//Round the packet length up to even 4 bytes
	rounded_len = (origlen + 3) & 0xFFFC;

	//If the packet is greater than 1536 we return error
	if (rounded_len > 1536UL)
	{
		//we have no intention to send this packet, so just free it
		buf_deref (nbuf, buf_alloc_type);
		return -2;
	}

	//Check the wanted TX BD slot if it's free
	if (((uint32)(eth_tx_bd[slot].len_ctrl & ETH_TX_BD_READY)) != 0UL)
		return -1;

	//Divide rounded len by 4
	rounded_len = rounded_len >> 2;

	//Write packet data
//	eth_dst_pnt = &ETH_DATA_BASE_PNT[slot * (1536/4)];
	eth_dst_pnt = (uint32*)(eth_tx_bd[slot].data_pnt);
	datapnt = (uint32*)nbuf->dstart;
	for(i=0; i < rounded_len; i++)
	{
		/*
		if (eth_dst_pnt == 0UL)
		{
			c_conws("Send_packet: eth_dst_pnt is 0\r\n");
			//Bconin(2);
		}
		if (datapnt == 0UL)
		{
			c_conws("Send_packet: datapnt is 0\r\n");
			//Bconin(2);
		}
		*/

		*eth_dst_pnt++ = *datapnt++;
	}

/*
	if ((((uint32)eth_dst_pnt) > (ETH_DATA_BASE_ADDR + (1536*2))) ||
		(((uint32)eth_dst_pnt) < ETH_DATA_BASE_ADDR))
	{
		c_conws("Send_packet: eth_dst_pnt outside tx slots!\r\n");
	}
	*/

	//Dummy loop to wait for the CT60 write FIFO to empty
	for (j = 0; j < 1000; j++)
	{
		asm("nop;");
	}

	//Write length of data, BD ready, BD CRC enable
	//We set the wrap bit if this packet is the last one in a sequence (may be just this packet too).
	if(last_packet)
		eth_tx_bd[slot].len_ctrl = (uint32)(origlen << 16) | ETH_TX_BD_READY | ETH_TX_BD_IRQ | ETH_TX_BD_CRC | ETH_TX_BD_WRAP;
	else
		eth_tx_bd[slot].len_ctrl = (uint32)(origlen << 16) | ETH_TX_BD_READY | ETH_TX_BD_IRQ | ETH_TX_BD_CRC;

	//ksprintf (message, "T%02lu 0x%08lx\r\n", slot, eth_tx_bd[slot].len_ctrl);
	//c_conws (message);

	buf_deref (nbuf, buf_alloc_type);						//free buf because contents of packet
																							//is now in the MAC controller

	return 0L;
}





/*
 * MintNet notifies you of some noteable IOCLT's. Usually you don't
 * need to act on them because MintNet already has done so and only
 * tells you that an ioctl happened.
 *
 * One useful thing might be SIOCGLNKFLAGS and SIOCSLNKFLAGS for setting
 * and getting flags specific to your driver. For an example how to use
 * them look at slip.c
 */
static long
svethlana_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;


//	c_conws("ioctl\r\n");		//debug


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

		case SIOCSIFOPT:
			/*
			 * Interface configuration, handled by dummy_config()
			 */
			ifr = (struct ifreq *) arg;
			return svethlana_config (nif, ifr->ifru.data);
	}

	return ENOSYS;
}




/*
 * Interface configuration via SIOCSIFOPT. The ioctl is passed a
 * struct ifreq *ifr. ifr->ifru.data points to a struct ifopt, which
 * we get as the second argument here.
 *
 * If the user MUST configure some parameters before the interface
 * can run make sure that dummy_open() fails unless all the necessary
 * parameters are set.
 *
 * Return values	meaning
 * ENOSYS		option not supported
 * ENOENT		invalid option value
 * 0			Ok
 */
static long
svethlana_config (struct netif *nif, struct ifopt *ifo)
{
//	c_conws("Config\r\n");

# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

	if (!STRNCMP ("hwaddr"))
	{
#ifdef SVETHLANA_DEBUG
		uchar *cp;
#endif
		/*
		 * Set hardware address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
#ifdef SVETHLANA_DEBUG
		cp = nif->hwlocal.adr.bytes;
		DEBUG (("dummy: hwaddr is %x:%x:%x:%x:%x:%x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
#endif
	}
	else if (!STRNCMP ("braddr"))
	{
#ifdef SVETHLANA_DEBUG
		uchar *cp;
#endif
		/*
		 * Set broadcast address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
#ifdef SVETHLANA_DEBUG
		cp = nif->hwbrcst.adr.bytes;
		DEBUG (("dummy: braddr is %x:%x:%x:%x:%x:%x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
#endif
	}
	else if (!STRNCMP ("debug"))
	{
		/*
		 * turn debuggin on/off
		 */
		if (ifo->valtype != IFO_INT)
			return ENOENT;
		DEBUG (("dummy: debug level is %ld", ifo->ifou.v_long));
	}
	else if (!STRNCMP ("log"))
	{
		/*
		 * set log file
		 */
		if (ifo->valtype != IFO_STRING)
			return ENOENT;
		DEBUG (("dummy: log file is %s", ifo->ifou.v_string));
	}

	return ENOSYS;
}



void Init_BD()
{
	uint32_t	i;
	char*			even_packet_base;

	packets_base = (char*) ct60_vmalloc(0, ETH_PKT_BUFFS * 2UL * 2048UL + 2048UL);
	even_packet_base = (char*)((((unsigned long)packets_base) + 2047UL) & 0xFFFFF800UL);	//Make it all start at even 2048 bytes

	//Set number of TX packet BDs and RX BDs
	ETH_TX_BD_NUM = ETH_PKT_BUFFS;

	//Init each BD ctrl longword
	//Init all RX slots to empty, so they can receive a packet
	//Init all data pointers, so they get 2048 bytes each.
	for (i = 0; i < ETH_PKT_BUFFS; i++)
	{
		if (i != (ETH_PKT_BUFFS-1))
		{
			eth_tx_bd[i].len_ctrl = ETH_TX_BD_CRC;																	// no wrap
			eth_rx_bd[i].len_ctrl = ETH_RX_BD_EMPTY | ETH_RX_BD_IRQ;								// no wrap
		}
		else
		{
			eth_tx_bd[i].len_ctrl = ETH_TX_BD_CRC   | ETH_TX_BD_WRAP;									// wrap on last TX BD
			eth_rx_bd[i].len_ctrl = ETH_RX_BD_EMPTY | ETH_RX_BD_IRQ | ETH_RX_BD_WRAP;	// wrap on last RX BD
		}
		eth_tx_bd[i].data_pnt = ((uint32_t)even_packet_base) + (i*2048UL);		//TX buffers come first
		eth_rx_bd[i].data_pnt = ((uint32_t)even_packet_base) + ((ETH_PKT_BUFFS+i)*2048UL);		//then all RX buffers
	}

}






/*
 * Initialization. This is called when the driver is loaded. If you
 * link the driver with main.o and init.o then this must be called
 * driver_init() because main() calles a function with this name.
 *
 * You should probe for your hardware here, setup the interface
 * structure and register your interface.
 *
 * This function should return 0 on success and != 0 if initialization
 * fails.
 */
long
driver_init (void)
{
//	char message[50];
	//static char eth_fname[128];

	long	ferror;
	short	fhandle;
	char	macbuf[13];

	c_conws("\r\n");
	c_conws("********************************\r\n");
	c_conws("*****   SVEthLANa driver   *****\r\n");
	c_conws("********************************\r\n");

	if (get_toscookie (COOKIE_SupV, NULL) != 0) {
		c_conws("\r\nThis driver requires SV_XBIOS.PRG!\r\n");
		Bconin(2);
		return -1;
	}

	// Check that the SV version is at least 10
	// Otherwise the FW doesn't have Ethernet DMA
	{
		uint32 sv_fw_version = SV_VERSION & 0x3FFUL;

		ksprintf (message, "SuperVidel FW version is %lu\r\n", sv_fw_version);
		c_conws  (message);

		if (sv_fw_version < 10)
		{
			c_conws( "\r\nThis driver needs at least SV FW version 10!\r\n" );
			Bconin(2);
			return -1;
		}
	}

	// Open svethlan.inf to read the MAC address
	if((ferror = Fopen("svethlan.inf",0)) < 0) { /* Try first in sysdir */
		short sysdrv = *((short *) 0x446);	/* get the boot drive number */
		char svethlan_inf[] = "A:\\SVETHLAN.INF";
		svethlan_inf[0] = DriveToLetter(sysdrv);

		ferror = Fopen(svethlan_inf,0);/* otherwise in boot drive's root */
	}

	if(ferror >= 0)
	{
		fhandle = (short)(ferror & 0xffff);
		memset(macbuf, 0, 13);
		ferror = Fread(fhandle,12,macbuf);
		if(ferror < 0)
		{
			c_conws ("\r\nError reading svethlan.inf!\r\n");
			Fclose(fhandle);
			return -1;
		}
		if(ferror < 12)
		{
			c_conws ("\r\nsvethlan.inf is less than 12 bytes long!\r\n");
			Fclose(fhandle);
			return -1;
		}
		Fclose(fhandle);

		//print what we read from ethernat.inf
		c_conws(macbuf);
		c_conws("\r\n");
	}
	else
	{
		c_conws("Could not open svethlan.inf\r\n");
		c_conws("Using default ethernet address 00:01:02:03:04:05\r\n");
		macbuf[0] = '0';
		macbuf[1] = '0';
		macbuf[2] = '0';
		macbuf[3] = '1';
		macbuf[4] = '0';
		macbuf[5] = '2';
		macbuf[6] = '0';
		macbuf[7] = '3';
		macbuf[8] = '0';
		macbuf[9] = '4';
		macbuf[10] = '0';
		macbuf[11] = '5';
	}

	macbuf[12] = 0;

	// Extract MAC address from macbuf
	if_svethlana.hwlocal.adr.bytes[0] = (uchar)(ch2i(macbuf[0]) * 16 + ch2i(macbuf[1]));
	if_svethlana.hwlocal.adr.bytes[1] = (uchar)(ch2i(macbuf[2]) * 16 + ch2i(macbuf[3]));
	if_svethlana.hwlocal.adr.bytes[2] = (uchar)(ch2i(macbuf[4]) * 16 + ch2i(macbuf[5]));
	if_svethlana.hwlocal.adr.bytes[3] = (uchar)(ch2i(macbuf[6]) * 16 + ch2i(macbuf[7]));
	if_svethlana.hwlocal.adr.bytes[4] = (uchar)(ch2i(macbuf[8]) * 16 + ch2i(macbuf[9]));
	if_svethlana.hwlocal.adr.bytes[5] = (uchar)(ch2i(macbuf[10]) * 16 + ch2i(macbuf[11]));



	/*
	 * The actual init of the hardware
	 *
	 */
	//Then write to the MODER register to reset the MAC controller
	ETH_MODER = ETH_MODER_RST;

	//Then release reset of the MAC controller
	ETH_MODER = 0L;

	//Set MAC address in MAC controller
	ETH_MAC_ADDR1 = (((uint32)(if_svethlana.hwlocal.adr.bytes[0])) <<  8) +
					(((uint32)(if_svethlana.hwlocal.adr.bytes[1])) <<  0);
	ETH_MAC_ADDR0 = (((uint32)(if_svethlana.hwlocal.adr.bytes[2])) << 24) +
					(((uint32)(if_svethlana.hwlocal.adr.bytes[3])) << 16) +
					(((uint32)(if_svethlana.hwlocal.adr.bytes[4])) <<  8) +
					(((uint32)(if_svethlana.hwlocal.adr.bytes[5])) <<  0);

	//Inter packet gap
	ETH_IPGT = 0x15UL;


	//Create longword MAC address for RX use
	mac_addr[0] = (((unsigned long)(if_svethlana.hwlocal.adr.bytes[0])) << 24) +
					  (((unsigned long)(if_svethlana.hwlocal.adr.bytes[1])) << 16) +
					  (((unsigned long)(if_svethlana.hwlocal.adr.bytes[2])) <<  8) +
					  (((unsigned long)(if_svethlana.hwlocal.adr.bytes[3])));

	mac_addr[1] = (((unsigned long)(if_svethlana.hwlocal.adr.bytes[4])) << 24) +
					  (((unsigned long)(if_svethlana.hwlocal.adr.bytes[5])) << 16);


	Init_BD();


	/*********************************************
	 * Here comes functions not using the hardware
	 *********************************************
	 */

	/*
	 * Set interface name
	 */
	strcpy (if_svethlana.name, "en");

	/*
	 * Set interface unit. if_getfreeunit("name") returns a yet
	 * unused unit number for the interface type "name".
	 */
	if_svethlana.unit = if_getfreeunit ("en");

	/*
	 * Alays set to zero
	 */
	if_svethlana.metric = 0;

	/*
	 * Initial interface flags, should be IFF_BROADCAST for
	 * Ethernet.
	 */
	if_svethlana.flags = IFF_BROADCAST;

	/*
	 * Maximum transmission unit, should be >= 46 and <= 1500 for
	 * Ethernet
	 */
	if_svethlana.mtu = 1500;

	/*
	 * Time in ms between calls to (*if_svethlana.timeout) ();
	 */
	if_svethlana.timer = 0;

	/*
	 * Interface hardware type
	 */
	if_svethlana.hwtype = HWTYPE_ETH;

	/*
	 * Hardware address length, 6 bytes for Ethernet
	 */
	if_svethlana.hwlocal.len =
	if_svethlana.hwbrcst.len = ETH_ALEN;

	/*
	 * Set interface broadcast address. For real ethernet
	 * drivers you must get them from the hardware of course!
	 */
	memcpy (if_svethlana.hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

	/*
	 * Set length of send and receive queue. IF_MAXQ is a good value.
	 */
	if_svethlana.rcv.maxqlen = IF_MAXQ;
	if_svethlana.snd.maxqlen = IF_MAXQ;

	/*
	 * Setup pointers to service functions
	 */
	if_svethlana.open = svethlana_open;
	if_svethlana.close = svethlana_close;
	if_svethlana.output = svethlana_output;
	if_svethlana.ioctl = svethlana_ioctl;

	/*
	 * Optional timer function that is called every 200ms.
	 */
	if_svethlana.timeout = NULL;

	/*
	 * Here you could attach some more data your driver may need
	 */
	if_svethlana.data = 0UL;

	/*
	 * Number of packets the hardware can receive in fast succession,
	 * 0 means unlimited.
	 */
	if_svethlana.maxpackets = ETH_PKT_BUFFS;

	/*
	 * Register the interface.
	 */
	if_register (&if_svethlana);

	/*
	 * And say we are alive...
	 */
	ksprintf (message, "SVEthLANa driver v0.9 (en%d)\r\n", if_svethlana.unit);
	c_conws (message);

	//print out all function pointers
//	if (!has_printed_functions)
//	{
//		ksprintf(message, "svethlana_open 0x%08lx\r\n", (uint32)(svethlana_open));
//		c_conws(message);
//		ksprintf(message, "svethlana_close 0x%08lx\r\n", (uint32)(svethlana_close));
//		c_conws(message);
//		ksprintf(message, "svethlana_output 0x%08lx\r\n", (uint32)(svethlana_output));
//		c_conws(message);
//		ksprintf(message, "svethlana_ioctl 0x%08lx\r\n", (uint32)(svethlana_ioctl));
//		c_conws(message);
//		ksprintf(message, "svethlana_config 0x%08lx\r\n", (uint32)svethlana_config);
//		c_conws(message);
//		ksprintf(message, "send_packet 0x%08lx\r\n", (uint32)(send_packet));
//		c_conws(message);
//		ksprintf(message, "svethlana_service 0x%08lx\r\n", (uint32)svethlana_service);
//		c_conws(message);

//		has_printed_functions = 1UL;
//	}



	// Install interrupt handler
	svethlana_install_int();

	//c_conws("Init succeeded\r\n");
	return 0;
}


static void
svethlana_install_int (void)
{
//	uint32	old_sp;

//	old_sp = Super(0L);

	old_i6_int = Setexc (0xC5, (long) interrupt_i6);

//	Super(old_sp);

	//c_conws("Installed ISR\r\n");
}




/*
 * Interrupt routine
 */
void _cdecl
svethlana_int (void)
{
	uint32	int_src;
	volatile uint16 temp;

	//Dummy read from motherboard to satisfy ABE-chip (should be done before interrupt
	//source is quenched? Interrupt is effectively shut off above)
	temp = *((volatile uint16*)0xffff8240);
        (void)temp;

	int_src = ETH_INT_SOURCE;

	//Clear all flags by writing 1 to them
	ETH_INT_SOURCE = 0x7FUL;

	svethlana_service(&if_svethlana, int_src);	//do the work
}


/*
//Returns 0 if there is a new packet received in slot 0, 1 if in slot 1 and -1 otherwise
int32 Check_Rx_Buffers()
{
	uint32 i;
	int32  retval = -1L;

	//We check two slots, starting at the slot not checked the last time
	//we were here.
	for (i = 0; i < 2; i++)
	{
		if((eth_rx_bd[cur_rx_slot].len_ctrl & ETH_RX_BD_EMPTY) == 0UL)
		{
			retval = (int32)cur_rx_slot;
			cur_rx_slot = (cur_rx_slot + 1) & 0x1UL;
			break;
		}
		cur_rx_slot = (cur_rx_slot + 1) & 0x1UL;
	}
	return retval;
}
*/

//Returns 0 if there is a new packet received in slot 0, 1 if in slot 1 and -1 otherwise
int32 Check_Rx_Buffers()
{
	int32  retval = -1L;
	uint32 tmp1;
	uint32 tmp2;

	//We check only one slot, which is the one not checked the last time
	//we were here. We only move to the other slot if we found a packet in the current slot.
	tmp1 = eth_rx_bd[cur_rx_slot].len_ctrl;
	if ((tmp1 & ETH_RX_BD_EMPTY) == 0UL)
	{
		retval = (int32)cur_rx_slot;
		cur_rx_slot = (cur_rx_slot + 1) & (ETH_PKT_BUFFS-1);
	}
	else
	{
		//No packet in the expected slot
		//Try again
		tmp2 = eth_rx_bd[cur_rx_slot].len_ctrl;
		if ((tmp2 & ETH_RX_BD_EMPTY) == 0UL)
		{
			ksprintf (message, "Slot %lu RX retried read: 0x%08lx then 0x%08lx\r\n", cur_rx_slot, tmp1, tmp2);
			c_conws (message);
			retval = (int32)cur_rx_slot;
			cur_rx_slot = (cur_rx_slot + 1) & (ETH_PKT_BUFFS-1);
		}
	}
	return retval;
}




/* Service function called by the interrupt handler
 * In here we check the interrupt bits, and do receive and transmit if necessary
 */
static void svethlana_service (struct netif * nif, uint32 int_src)
{
//	uchar	intstat;
//	uchar		oldpnr;
//	uchar	failpnr;
//	ushort	fifo_reg;
//	ushort	status, bytecount, longcnt;
	short	type, i;
//	char	packetnr;
//	char	message[80];
//	long	tmp, *dpnt;
	BUF		*b;
	volatile uint32	j;

//	long		timeval;



	if (int_src & ETH_INT_BUSY)
	{
		c_conws ("Busy\r\n");
	}

	if (int_src & ETH_INT_RXE)
	{
		//c_conws("RX error!\r\n");
		//printf("E");

		// Check which BD has the error and clear it
		for (i = 0; i < ETH_PKT_BUFFS; i++)
		{
			if((eth_rx_bd[i].len_ctrl & ETH_RX_BD_EMPTY) == 0UL)
			{
				if (eth_rx_bd[i].len_ctrl & (ETH_RX_BD_OVERRUN | ETH_RX_BD_INVSIMB | ETH_RX_BD_DRIBBLE |
											 ETH_RX_BD_TOOLONG | ETH_RX_BD_SHORT | ETH_RX_BD_CRCERR | ETH_RX_BD_LATECOL))
				{
					//At least one of the above error flags was set
					ksprintf (message, "Slot %d RX errorflags: 0x%08lx \r\n", i, eth_rx_bd[i].len_ctrl);
					c_conws (message);

					//Clear error flags
					eth_rx_bd[i].len_ctrl &= ~(ETH_RX_BD_OVERRUN | ETH_RX_BD_INVSIMB | ETH_RX_BD_DRIBBLE |
																		 ETH_RX_BD_TOOLONG | ETH_RX_BD_SHORT | ETH_RX_BD_CRCERR | ETH_RX_BD_LATECOL);

					//mark the buffer as empty and free to use
					eth_rx_bd[i].len_ctrl |= ETH_RX_BD_EMPTY;
					nif->in_errors++;
				}
			}
		}
	}


	// Check for received packets
	if (int_src & ETH_INT_RXB)
	{
		//printf("RX frame!\r\n");

		int32 slot = Check_Rx_Buffers();
		while (slot != -1)
		{
			uint32	*src;
			uint32	*dest;
			uint32	length, len_longs;

			//ksprintf (message, "R%02li 0x%08lx\r\n", slot, eth_rx_bd[slot].len_ctrl);
			//c_conws(message);

			length = ((eth_rx_bd[slot].len_ctrl) >> 16);	//length in upper word
			len_longs = (length + 3UL) >> 2;
//			src = &ETH_DATA_BASE_PNT[(1536UL / 4UL) * (slot+2L)];	//2 TX buffers before the RX buffers
			src = (uint32*)eth_rx_bd[slot].data_pnt;

//			b = buf_alloc (length+200, 100, BUF_ATOMIC);
			b = buf_alloc (1518UL + 128UL, 64UL, BUF_ATOMIC);
			if(b == 0)
			{
				nif->in_errors++;
				//ksprintf (message, "buf_alloc RX failed, %lu \r\n", 1518UL + 200UL);
				//c_conws(message);
			}
			else
			{
				//dstart must be on whole word, but we set to whole longword.
				//should make the 060 use longword accesses and not risk that
				//it is split into byte-word-byte.
				b->dstart = (char*)(((uint32)(b->dstart)) & 0xFFFFFFFCUL);
				b->dend = (char*)(((uint32)(b->dend)) & 0xFFFFFFFCUL);

				//Dummy loop to wait for the MAC write FIFO to empty
				for (j = 0; j < 1000; j++)
				{
					asm("nop;");
				}

				//read the data, rounded up to even longwords
				dest = (uint32*)(b->dstart);
				for(i=0; i < len_longs; i++)
				{
					*dest++ = *src++;
				}
//				b->dend += length - 4;							//TODO: should we subtract 4 here, to skip the CRC?
				b->dend += (uint32)(length - 4UL);				//TODO: should we subtract 4 here, to skip the CRC?
				if((b->dend) < (b->dstart))
				{
					c_conws("RX: dend < dstart!\r\n");
				}

				// Pass packet to upper layers
				if (nif->bpf)
					bpf_input (nif, b);

				type = eth_remove_hdr(b);

				// and enqueue packet
				if(!if_input(nif, b, 0UL, type))
					nif->in_packets++;
				else
				{
					nif->in_errors++;
					c_conws("input packet failed when receiving!\r\n");
				}
			}

			//now mark the buffer as empty and free to use
			eth_rx_bd[slot].len_ctrl |= ETH_RX_BD_EMPTY;

			slot = Check_Rx_Buffers();
		}
	}


	// Check for transmitted packets
	if ((int_src & (ETH_INT_TXB || ETH_INT_TXE)) != 0)	// Transmit complete or error
	{
		if(int_src & ETH_INT_TXE)				// TX eror set => failed!
		{
			nif->out_errors++;
		}

		//A Transmit Error means that the slot is free to be used.
		//TODO: Check first that the slot that is in turn to be used is free.
		//Then we dequeue a packet and send it, if one exists. Then we toggle the slot index variable.
		//Finally we check again if the new slot is free. If so we dequeue a packet again.
		if (((uint32)(eth_tx_bd[slot_index].len_ctrl & ETH_TX_BD_READY)) != 0UL)
			return;

		b = if_dequeue(&nif->snd);				//fetch a previously enqueued packet
		if( ((uint32)b) != 0UL)
		{
			//Possible errors returned are -1 and -2, meaning "no free slot" and "too large packet" respectively.
			//-1 isn't possible because of the check above, and -2 isn't possible either, because
			//we don't enqueue packets in svethlana_output() that are too large.
			send_packet(nif, b, BUF_ATOMIC, slot_index, slot_index == (ETH_PKT_BUFFS-1) );
			slot_index = (slot_index + 1) & (ETH_PKT_BUFFS-1);

			//in_queue--;

			//ksprintf (message, "Dequeued, %u left\r\n", nif->snd.qlen);
			//c_conws(message);
		}
	}

}



/* Convert one ASCII char to a hex nibble */
short ch2i(char c)
{
	if(c >= '0' && c <= '9')
		return (short)(c - '0');
	if(c >= 'A' && c <= 'F')
		return (short)(c - 'A' + 10);
	if(c >= 'a' && c <= 'f')
		return (short)(c - 'a' + 10);
	return 0;
}



// Convert an unsigned long to ASCII
void hex2ascii(ulong l, uchar* c)
{
	short	i;
	uchar	t;

	for(i=7; i!=0; i--)
	{
		t = (uchar)(l & 0xf);				// Keep only lower nibble
		if(t < 0xA)
			c[i] = t + '0';					// Output '0' to '9'
		else
			c[i] = t + 'A' - 0xA;			// Output 'A' to 'F'
		c[i] = '0';
		l = l >> 4;
	}
}


