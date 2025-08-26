/*
 * Ethernat driver for FreeMiNT.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2007 Henrik Gilda
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
 *	EtherNat packet driver.
 *
 *	Usage:
 *		ifconfig en0 addr u.v.w.x
 *		route add u.v.w.x en0
 *
 *		20050131		/Henrik and Torbj”rn Gild†
 *
 *
 *		Uses the I6 line for interrupt and does not use addroottimeout
 *
 *		20050820: Uses I6 interrupt, and allocation interrupt for sending
 *
 */

#define NEW_ETHERNAT

//#define USE_200Hz	1
#define USE_I6	1

#include "global.h"

#include "buf.h"
#include "inet4/if.h"
#include "inet4/ifeth.h"

#include "netinfo.h"
#include "mint/delay.h"
#include "mint/mdelay.h"
#include "mint/sockio.h"
#include "mint/endian.h"
#include "mint/arch/asm_spl.h" /* spl() */

#include "91c111.h"
#include "ethernat_200Hzint.h"

#include <mint/osbind.h>

#define DriveToLetter(d) ((d) < 26 ? 'A' + (d) : (d) - 26 + '1')

// Define the 91c111 hardware registers

pVUWORD const LAN_BANK = (pVUWORD) LANADDR_BANK;

pVUWORD const LAN_TCR = (pVUWORD) LANADDR_TCR;
pVUWORD const LAN_EPH_STATUS = (pVUWORD) LANADDR_EPH_STATUS;
pVUWORD const LAN_RCR = (pVUWORD) LANADDR_RCR;
pVUWORD const LAN_COUNTER = (pVUWORD) LANADDR_COUNTER;
pVUWORD const LAN_MIR = (pVUWORD) LANADDR_MIR;
pVUWORD const LAN_RPCR = (pVUWORD) LANADDR_RPCR;

pVUWORD const LAN_CONFIG = (pVUWORD) LANADDR_CONFIG;
pVUWORD const LAN_BASE = (pVUWORD) LANADDR_BASE;
pVUWORD const LAN_IA01 = (pVUWORD) LANADDR_IA01;
pVUWORD const LAN_IA23 = (pVUWORD) LANADDR_IA23;
pVUWORD const LAN_IA45 = (pVUWORD) LANADDR_IA45;
pVUWORD const LAN_GP = (pVUWORD) LANADDR_GP;
pVUWORD const LAN_CONTROL = (pVUWORD) LANADDR_CONTROL;

pVUWORD const LAN_MMU = (pVUWORD) LANADDR_MMU;
pVUBYTE const LAN_PNR = (pVUBYTE) LANADDR_PNR;
pVUBYTE const LAN_ARR = (pVUBYTE) (LANADDR_PNR+1);
pVUWORD const LAN_FIFO = (pVUWORD) LANADDR_FIFO;
pVUWORD const LAN_POINTER = (pVUWORD) LANADDR_POINTER;
pVULONG const LAN_DATA = (pVULONG) LANADDR_DATA;
pVUWORD const LAN_DATA_L = (pVUWORD) LANADDR_DATA;
pVUWORD const LAN_DATA_H = (pVUWORD) (LANADDR_DATA+1);		//second word of DATA reg
pVUBYTE const LAN_IST_ACK = (pVUBYTE) LANADDR_INTERRUPT;
pVUBYTE const LAN_INTMSK = (pVUBYTE) (LANADDR_INTERRUPT+1);

pVUWORD const LAN_MT01 = (pVUWORD) LANADDR_MT01;
pVUWORD const LAN_MT23 = (pVUWORD) LANADDR_MT23;
pVUWORD const LAN_MT45 = (pVUWORD) LANADDR_MT45;
pVUWORD const LAN_MT67 = (pVUWORD) LANADDR_MT67;
pVUWORD const LAN_MGMT = (pVUWORD) LANADDR_MGMT;
pVUWORD const LAN_REVISION = (pVUWORD) LANADDR_REVISION;
pVUWORD const LAN_ERCV = (pVUWORD) LANADDR_ERCV;

// Ethernat hardware register
#ifdef NEW_ETHERNAT
pVUBYTE const ETH_REG = (pVUBYTE) 0x80000023;
#else
pVUBYTE const ETH_REG = (pVUBYTE) 0x80000020;
#endif


// 200Hz hardware timer
pVULONG const TIMER200 = (pVULONG) 0x4ba;

/*
 * From main.c
 */
extern long driver_init (void);

// to make compiler happy
void _cdecl ethernat_int (void);


/*
 * Our interface structure
 */
static struct netif if_ethernat;

/*
 * Prototypes for our service functions
 */
static long	ethernat_open		(struct netif *);
static long	ethernat_close		(struct netif *);
static long	ethernat_output	(struct netif *, BUF *, const char *, short, short);
static long	ethernat_ioctl		(struct netif *, short, long);
static long	ethernat_config	(struct netif *, struct ifopt *);

//does actual sending of packets for ethernat_output() and ethernat_service()
static long send_packet			(struct netif *nif, BUF *nbuf);

// Service function to check for incoming and outgoing packets and allocation interrupts
static void ethernat_service	(struct netif *);

// For our interrupt
static void ethernat_install_int (void);

/* Convert single hex char to short */
short ch2i(char);

// Convert long to hex ascii
void hex2ascii(ulong l, uchar* c);

/* To know if the EtherNat is present through a bus error*/
extern void ethernat_probe_asm(void);
void ethernat_probe_c(void);
static int found = 0;

// Variable for locking out interrupt when accessing the hardware
static volatile int in_use = 0;

// If this variable equals 1, then the interrupt will do nothing until init is done (=0)
static volatile int initializing = 1;

static volatile int autoneg = 0;

// Keep track of when sending so we don't get fooled by TXEMPTY
static volatile int sending = 0;

//handle for all logging
//static ushort loghandle;


//global MAC address in longword format for RX use
static unsigned long mac_addr[2];


//static uchar upperleft[] = {27,'H',0};
//static uchar col40[] = {27,'Y',32,72,0};


/*
 * This gets called when someone makes an 'ifconfig up' on this interface
 * and the interface was down before.
 */
static long
ethernat_open (struct netif *nif)
{
//	short		tmp;
//	static char message[100];

	in_use = 1;

	c_conws("Ethernat up!\n\r");


	// Enable interrupts in the LAN91C111
/*
	ksprintf (message, "Enable interrupts in the LAN91C111... ");
	c_conws (message);
*/

#ifdef USE_I6
	Eth_set_bank(2);
	*LAN_INTMSK = (*LAN_INTMSK) | 0x1;					//enable RCV_INT
#endif

/*
	ksprintf (message, "OK\n\r");
	c_conws (message);
*/


	Eth_set_bank(0);
	*LAN_RCR = (*LAN_RCR) | 0x0001;						//receive enable

	initializing = 0;
	in_use = 0;

	return 0;
}

/*
 * Opposite of ethernat_open(), is called when 'ifconfig down' on this interface
 * is done and the interface was up before.
 */
static long
ethernat_close (struct netif *nif)
{
	c_conws("Ethernat down!\n\r");
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
ethernat_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	BUF				*nbuf;
	unsigned char	littlemem;
	UNUSED(littlemem);
//	static	uchar	message[100];

//	unsigned long	timeval;


//	c_conws("Output\n\r");		//debug


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
	if (nbuf == NULL)
	{
		c_conws("eth_build_hdr() failed!\n\r");
		nif->out_errors++;
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


	in_use = 1;											//don't let interrupt disturb us now

	// Check available memory in LAN91C111
	Eth_set_bank(0);
	littlemem = (*LAN_MIR) & 0x00ff;
//	if (len == 0)
//	{
//		c_conws("Insufficient memory in LAN91C111!\n\r");
//	}

	// Allocate buffer in 91C111
	Eth_set_bank(2);
	*LAN_MMU = 0x2000;								//allocate packet

#ifdef USE_200Hz
	timeval = *TIMER200;
	while(!((*LAN_IST_ACK) & 0x08))				//wait for ALLOC_INT
	{
		if(*TIMER200 > (timeval+1))
		{
			if (littlemem > 0)
			{
				c_conws("Waiting too long for ALLOC_INT but we had memory before!\n\r");
			}
			return ENOMEM;
		}
	}
	send_packet(nif, nbuf);
#endif
#ifdef USE_I6
	if((*LAN_IST_ACK) & 0x08)					//check ALLOC_INT only once
	{
		send_packet(nif, nbuf);
		*ETH_REG = (*ETH_REG) ^ 0x80;			// Toggle LED2
	}
	else
	{
		//no free buffer, enqueue the nbuf and wait for ALLOC_INT
		if_enqueue (&nif->snd, nbuf, nbuf->info);
		*LAN_INTMSK = (*LAN_INTMSK) | 0x08;			//enable ALLOC_INT
	}

#endif

	return E_OK;
}


/*This function sends a packet, but it should only be called by ethernat_output() or
  the ethernat_service() routine when servicing ALLOC_INT. This function assumes that
  an allocation for transmit packet has been done recently.
*/

static long
send_packet(struct netif *nif, BUF *nbuf)
{
	short				i, longcnt;
	long				*datapnt, tmp;
	unsigned short	bank;
	unsigned char	packetnr;
	unsigned short	len, origlen, bytecount;

//	c_conws("sendpacket\n\r");		//debug



	bank = Eth_set_bank(2);

	packetnr = (*LAN_ARR);							//read allocated packet nr (and status)
	if(packetnr & 0x80)								//allocation failed?
	{
		//no free buffer, destroy the nbuf (this should never happen)
		buf_deref (nbuf, BUF_NORMAL);
		c_conws("No free buffer in 91C111!\n\r");
		return ENOMEM;

	}

	*LAN_PNR = packetnr;
	*LAN_POINTER = 0x0040;							// offset = 0, autoinc = yes, write

	/*
	 * Write data into allocated buffer.
	 */

	//calculate packet length
	origlen = (nbuf->dend) - (nbuf->dstart);

	len = origlen;

	//Calculate longwords to read from nbuf and write to hardware
	longcnt = origlen >> 2;

	// Calculate bytecount to include in the controller's header
	bytecount = len + 6;
	tmp = (((long)bytecount) << 16) | 0x0000;

//	c_conws(upperleft);

//	ksprintf(message, "Status: 0x%08lx\n\r", cpu2le32(tmp));
//	c_conws(message);

	*LAN_DATA = cpu2le32(tmp);						//write statusword and bytecount

	//Write all data except the longword with the controlbyte
	datapnt = (long*)nbuf->dstart;
	for(i=0; i < longcnt; i++)
	{
		*LAN_DATA = *datapnt++;

//		tmp = *datapnt++;
//		*LAN_DATA = tmp;
//		ksprintf(message, "Sent: 0x%08lx\n\r", tmp);
//		c_conws(message);
	}

		//Write last odd bytes together with control byte (that signals odd/even data)
		switch(len & 0x3)
		{
			case 0:	tmp = 0x00100000;								//longword-even data
						break;
			case 1:	tmp = (*datapnt) & 0xff000000;
						tmp |= 0x00300000;							//one byte odd data
						break;
			case 2:	tmp = (*datapnt) & 0xffff0000;
						tmp |= 0x00000010;							//one word odd data
						break;
			case 3:	tmp = (*datapnt) & 0xffffff00;
						tmp |= 0x00000030;							//three bytes odd data
		}

//		ksprintf(message, "Last: 0x%08lx\n\r", tmp);
//		c_conws(message);

		*LAN_DATA = tmp;									//DONE!
//	}


	/*
	 *		Order the 91C111 to send the packet!
	 */

	Eth_set_bank(0);
	*LAN_TCR = (*LAN_TCR) | 0x0100;				//enable transmit

	Eth_set_bank(2);
	*LAN_MMU = 0xC000;								//enqueue packet into TX fifo

	//Clear TXINT flag
//	*LAN_IST_ACK = 0x02;

#ifdef USE_I6
	*LAN_INTMSK = (*LAN_INTMSK) | 0x2;			//enable TX_INT, but NOT TX_EMPTY_INT
#endif

	buf_deref (nbuf, BUF_NORMAL);					//free buf because contents of packet
															//is now in the 91C111 controller
	sending = 1;
	in_use = 0;

//	c_conws("Packet queued successfully!\n\r");
	Eth_set_bank(bank);

	return E_OK;
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
ethernat_ioctl (struct netif *nif, short cmd, long arg)
{
	struct ifreq *ifr;


//	c_conws("ioctl\n\r");		//debug


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
			return ethernat_config (nif, ifr->ifru.data);
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
ethernat_config (struct netif *nif, struct ifopt *ifo)
{
//	c_conws("Config\n\r");

# define STRNCMP(s)	(strncmp ((s), ifo->option, sizeof (ifo->option)))

	if (!STRNCMP ("hwaddr"))
	{
#ifdef DEBUG_INFO
		uchar *cp;
#endif
		/*
		 * Set hardware address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
#ifdef DEBUG_INFO
		cp = nif->hwlocal.adr.bytes;
		DEBUG (("dummy: hwaddr is %x:%x:%x:%x:%x:%x",
			cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
#endif
	}
	else if (!STRNCMP ("braddr"))
	{
#ifdef DEBUG_INFO
		uchar *cp;
#endif
		/*
		 * Set broadcast address
		 */
		if (ifo->valtype != IFO_HWADDR)
			return ENOENT;
		memcpy (nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
#ifdef DEBUG_INFO
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
//	static char message[100];
	//static char eth_fname[128];

	long	ferror;
	short	fhandle;
	char	macbuf[13];
	short sr;

//	c_conws("Driver init\n\r");

	// Lock out interrupt function
	initializing = 1;
	in_use = 1;

	// First check that the Ethernat card can be found
	sr = spl7 ();
	ethernat_probe_asm();
	spl (sr);

	if(!found)
	{
		c_conws ("\n\n\r\033pEtherNat not found!\033q\n\n\r");
		return -1;
	}

//	c_conws("Efter koll av ethernat\n\r");


	//c_conws("*********************************\n\r");
	//c_conws("******* EtherNat driver *********\n\r");
	//c_conws("*********************************\n\r");

	// Open ethernat.inf to read the MAC address
	if((ferror = Fopen("ethernat.inf",0)) < 0) { /* Try first in sysdir */
		short sysdrv = *((short *) 0x446);	/* get the boot drive number */
		char ethernat_inf[] = "A:\\ETHERNAT.INF";
		ethernat_inf[0] = DriveToLetter(sysdrv);

		ferror = Fopen(ethernat_inf,0);/* otherwise in boot drive's root */
	}

//	c_conws("Efter FOPEN\n\r");
	if(ferror >= 0)
	{
		fhandle = (short)(ferror & 0xffff);
		memset(macbuf, 0, 13);
		ferror = Fread(fhandle,12,macbuf);
//		c_conws("Efter FREAD\n\r");
		if(ferror < 0)
		{
			//ksprintf (message, "Error reading ethernat.inf!\n\r");
			//c_conws (message);
			c_conws ("Error reading ethernat.inf!\n\r");
			Fclose(fhandle);
			return -1;
		}
		if(ferror < 12)
		{
			//ksprintf (message, "ethernat.inf is less than 12 bytes long!\n\r");
			//c_conws (message);
			c_conws ("ethernat.inf is less than 12 bytes long!\n\r");
			Fclose(fhandle);
			return -1;
		}
		Fclose(fhandle);

		//print what we read from ethernat.inf
		c_conws (macbuf);
		//ksprintf(message, "\n\r");
		//c_conws(message);
	}
	else
	{
		//ksprintf (message, "Error opening ethernat.inf!\n\r");
		//c_conws (message);
		c_conws("Could not open ethernat.inf\n\r");
		c_conws("Using default ethernet address 00:01:02:03:04:05\n\r");
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
	if_ethernat.hwlocal.adr.bytes[0] = (uchar)(ch2i(macbuf[0]) * 16 + ch2i(macbuf[1]));
	if_ethernat.hwlocal.adr.bytes[1] = (uchar)(ch2i(macbuf[2]) * 16 + ch2i(macbuf[3]));
	if_ethernat.hwlocal.adr.bytes[2] = (uchar)(ch2i(macbuf[4]) * 16 + ch2i(macbuf[5]));
	if_ethernat.hwlocal.adr.bytes[3] = (uchar)(ch2i(macbuf[6]) * 16 + ch2i(macbuf[7]));
	if_ethernat.hwlocal.adr.bytes[4] = (uchar)(ch2i(macbuf[8]) * 16 + ch2i(macbuf[9]));
	if_ethernat.hwlocal.adr.bytes[5] = (uchar)(ch2i(macbuf[10]) * 16 + ch2i(macbuf[11]));



	/*
	 * The actual init of the hardware
	 *
	 */

	Eth_AutoNeg();

	// Set MAC address in the controller
	Eth_set_bank(1);
	*LAN_IA01 = (((short)(if_ethernat.hwlocal.adr.bytes[0])) << 8) +
					  (short)(if_ethernat.hwlocal.adr.bytes[1]);
	*LAN_IA23 = (((short)(if_ethernat.hwlocal.adr.bytes[2])) << 8) +
					  (short)(if_ethernat.hwlocal.adr.bytes[3]);
	*LAN_IA45 = (((short)(if_ethernat.hwlocal.adr.bytes[4])) << 8) +
					  (short)(if_ethernat.hwlocal.adr.bytes[5]);


	//Create longword MAC address for RX use
	mac_addr[0] = (((unsigned long)(if_ethernat.hwlocal.adr.bytes[0])) << 24) +
					  (((unsigned long)(if_ethernat.hwlocal.adr.bytes[1])) << 16) +
					  (((unsigned long)(if_ethernat.hwlocal.adr.bytes[2])) <<  8) +
					  (((unsigned long)(if_ethernat.hwlocal.adr.bytes[3])));

	mac_addr[1] = (((unsigned long)(if_ethernat.hwlocal.adr.bytes[4])) << 24) +
					  (((unsigned long)(if_ethernat.hwlocal.adr.bytes[5])) << 16);



	// Enable auto-release
	Eth_set_bank(1);
	*LAN_CONTROL = 0x105a;					//enable auto-release + RCV_BAD

	//Receive-settings
	Eth_set_bank(0);
//	*LAN_RCR = (*LAN_RCR) | 0x0002;		//enable strip CRC
	*LAN_RCR = (*LAN_RCR) | 0x0602;		//enable ALMUL, PRMS, strip CRC

	//Transmit-settings
	Eth_set_bank(0);
	*LAN_TCR = (*LAN_TCR) | 0x8000;		//Pad enable


	/*********************************************
	 * Here comes functions not using the hardware
	 *********************************************
	 */

	/*
	 * Set interface name
	 */
	strcpy (if_ethernat.name, "en");

	/*
	 * Set interface unit. if_getfreeunit("name") returns a yet
	 * unused unit number for the interface type "name".
	 */
	if_ethernat.unit = if_getfreeunit ("en");

	/*
	 * Alays set to zero
	 */
	if_ethernat.metric = 0;

	/*
	 * Initial interface flags, should be IFF_BROADCAST for
	 * Ethernet.
	 */
	if_ethernat.flags = IFF_BROADCAST;

	/*
	 * Maximum transmission unit, should be >= 46 and <= 1500 for
	 * Ethernet
	 */
	if_ethernat.mtu = 1500;

	/*
	 * Time in ms between calls to (*if_ethernat.timeout) ();
	 */
	if_ethernat.timer = 0;

	/*
	 * Interface hardware type
	 */
	if_ethernat.hwtype = HWTYPE_ETH;

	/*
	 * Hardware address length, 6 bytes for Ethernet
	 */
	if_ethernat.hwlocal.len =
	if_ethernat.hwbrcst.len = ETH_ALEN;

	/*
	 * Set interface broadcast address. For real ethernet
	 * drivers you must get them from the hardware of course!
	 */
	memcpy (if_ethernat.hwbrcst.adr.bytes, "\377\377\377\377\377\377", ETH_ALEN);

	/*
	 * Set length of send and receive queue. IF_MAXQ is a good value.
	 */
	if_ethernat.rcv.maxqlen = IF_MAXQ;
	if_ethernat.snd.maxqlen = IF_MAXQ;

	/*
	 * Setup pointers to service functions
	 */
	if_ethernat.open = ethernat_open;
	if_ethernat.close = ethernat_close;
	if_ethernat.output = ethernat_output;
	if_ethernat.ioctl = ethernat_ioctl;

	/*
	 * Optional timer function that is called every 200ms.
	 */
	if_ethernat.timeout = NULL;

	/*
	 * Here you could attach some more data your driver may need
	 */
	if_ethernat.data = 0;

	/*
	 * Number of packets the hardware can receive in fast succession,
	 * 0 means unlimited.
	 */
	if_ethernat.maxpackets = 0;

	/*
	 * Register the interface.
	 */
	if_register (&if_ethernat);

	/*
	 * And say we are alive...
	 */
	//ksprintf (message, "EtherNat driver v0.1 (en%d)\n\r", if_ethernat.unit);
	//c_conws (message);


	// Install interrupt handler
	//ksprintf (message, "Installing interrupt handler... ");
	//c_conws (message);

	ethernat_install_int();

	//ksprintf (message, "OK\n\r");
	//c_conws (message);


#ifdef USE_I6
	// Enable LAN interrupts in the Ethernat control register
	//ksprintf (message, "Enable LAN interrupts in the Ethernat... ");
	//c_conws (message);

	*ETH_REG = (*ETH_REG) | 0x82;			//Disable Led2, enable led 1

	//ksprintf (message, "OK\n\r");
	//c_conws (message);
#endif

	c_conws("Init succeeded\n\r");
	return 0;
}

void
ethernat_probe_c (void)
{
	if((*LAN_BANK & 0x00ff) != 0x0033)
		return;

	found = 1;
}

static void
ethernat_install_int (void)
{
	c_conws("install int\n\r");		//debug


#ifdef USE_I6
	old_i6_int = Setexc (0xC4, (long) interrupt_i6);
#endif
#ifdef USE_200Hz
	old_200Hz_int = Setexc (0x114/4, (long) interrupt_200Hz);
#endif
}




/*
 * Interrupt routine
 */
void _cdecl
ethernat_int (void)
{
	static ushort	banktmp, pnttmp;
	static uchar	pnrtmp;
	static uchar	inttmp;

#ifdef USE_200Hz
//	if (in_use)
//		return;
#endif

#ifdef USE_I6
	//Shut out all further interrupts from the LAN
	*ETH_REG = (*ETH_REG) & 0xFD;

	//Dummy read from motherboard to satisfy ABE-chip (should be done before interrupt
	//source is quenched? Interrupt is effectively shut off above)
	banktmp = *((volatile ushort*)0xffff8240);

	// Toggle LEDs
//	*ETH_REG = (*ETH_REG) ^ 0xC0;

	//Here the interrupt level should be set to 3 (or whatever SR is on the stack).
	//This to let in other interrupt sources.
	set_old_int_lvl();

	//Exit here if we're still initializing or still in autonegotiation
	if(initializing || !autoneg)
	{
		short datatmp = MII_read_reg(1); //read PHY register 1

		if((datatmp & 0x0020) && (datatmp & 0x0004))
			autoneg = 1;

		set_int_lvl6();
		*ETH_REG = (*ETH_REG) | 0x02;					//Enable LAN interrupt in the CPLD
		return;
	}
#endif

//	c_conws("Interrupt!\n\r");

	banktmp = Eth_set_bank(2);
	pnttmp = *LAN_POINTER;
	pnrtmp = *LAN_PNR;
	inttmp = *LAN_INTMSK;
	*LAN_INTMSK = 0x0;			// Mask interrupts

	ethernat_service(&if_ethernat);	//do the work

	Eth_set_bank(2);
	*LAN_POINTER = pnttmp;
	*LAN_PNR = pnrtmp;
	*LAN_INTMSK = inttmp;		// Restore interrupt mask state

	Eth_set_bank(banktmp);

#ifdef USE_I6
	//Here the interrupt level should be reset to 6 to prevent us from entering
	//this interrupt again before we have left it properly.
	set_int_lvl6();

	//Enable CPLD LAN interrupt again
	*ETH_REG = (*ETH_REG) | 0x02;
#endif

}




/* Service function called by the interrupt handler
 * In here we check the interrupt bits, and do receive and transmit if necessary
 */
static void ethernat_service	(struct netif * nif)
{
	uchar		intstat;
//	uchar		oldpnr;
	uchar		failpnr;
	ushort	fifo_reg;
	ushort	status, bytecount, longcnt;
	short		type, i;
	char		packetnr;
//	char		message[100];
	long		tmp, *dpnt;
	long		desttmp[2];
	BUF		*b;

//	long		timeval;

	(void) desttmp;


	/*
	 * When we enter here banknr, pointer reg, packetnr, intmask registers
	 * have been backed up.
	 *
	 */



	/*
	 * Check for received packets
	 */

	Eth_set_bank(2);
	intstat = *LAN_IST_ACK;
	while(intstat & 0x01)
	{
		packetnr = (unsigned char)((*LAN_FIFO) & 0x00ff);
/*
		ksprintf(message, "Packet nr: 0x%X\n", packetnr);
		c_conws(message);
*/
		*LAN_POINTER = 0x00e0;								//RCV, RD, AUTOINC

		// Read status word and bytecount at the same time
		tmp = le2cpu32(*LAN_DATA);

		status = (short)(tmp & 0xffff);
		bytecount = (short)(tmp >> 16);

//		col40[2] = 32;
//		c_conws(col40);


		//Look ahead at the destination address
		desttmp[0] = *LAN_DATA;						//first 4 bytes of dest address
		desttmp[1] = (*LAN_DATA) & 0xffff0000;	//last 2 bytes
		*LAN_POINTER = 0x04e0;		//reset back to where we were before checking


		//Check for errors
		if(status & 0xAC00)
		{
			nif->in_errors++;
/*			ksprintf(message, "Error in RCV packet: 0x%04hx\n\r", status);
			f_write(loghandle, strlen(message)-1, message);*/
		}
		else
		{
			b = buf_alloc (bytecount+100, 50, BUF_NORMAL);
			if(!b)
			{
				nif->in_errors++;
				c_conws("buf_alloc failed when receiving!\n\r");

				*LAN_MMU = 0x8000;									//release received packet
				while((*LAN_MMU) & 0x0100);						//wait till MMU not busy

				break;
			}

			//read the data, rounded up to even longwords
			longcnt = ((bytecount+3)>>2)-1;

			dpnt = (long*)(b->dstart);

			for(i=0; i < longcnt; i++)
			{
//				tmp = *LAN_DATA;
//				*dpnt++ = tmp;
//				ksprintf(message, "RX: 0x%08lx\n\r", tmp);
//				c_conws(message);

				*dpnt++ = *LAN_DATA;							//read a longword into our buffer
			}
/*
			ksprintf(message, "\n\n\r");
			f_write(loghandle, strlen(message)-1, message);
*/

			// Calc real frame length
			if(status & 0x1000)									// Oddfrm bit set
			{
				b->dend += bytecount-6+1;
			}
			else
			{
				b->dend += bytecount-6;
			}

			/* Pass packet to upper layers */
			if (nif->bpf)
				bpf_input (nif, b);

			type = eth_remove_hdr(b);

			/* and enqueue packet */
			if(!if_input(nif, b, 0, type))
				nif->in_packets++;
			else
				nif->in_errors++;

//			c_conws("Received packet\n\r");
		}

		*LAN_MMU = 0x8000;									//release received packet
		while((*LAN_MMU) & 0x0100);						//wait till MMU not busy

		intstat = *LAN_IST_ACK;								//check for more packets
	}


	/*
	 * Check for transmitted packets
	 */

	// Write acknowledge reg with TXEMPTY bit set
	Eth_set_bank(2);
	*LAN_IST_ACK = 0x04;

	//Read interrupt status bits
	intstat = *LAN_IST_ACK;

	// Keep TXINT and TXEMPTY bits
	intstat = intstat & 0x06;

	if(intstat == 0)			// Both clear, waiting for completion
	{
//		in_use = 0;
		return;
	}
	else if(intstat & 0x02)				// TXINT set => failed!
	{
		c_conws("Failed to transmit a packet, resending...\n\r");

		nif->out_errors++;

		// Read failed packet nr from TX FIFO
		fifo_reg = *LAN_FIFO;
		failpnr = (uchar)(fifo_reg >> 8);

		if(failpnr & 0x80)		// TX FIFO empty even though TXINT was set!
		{
			c_conws("No packets to resend!!!!\n\r");
//			in_use = 0;
			return;
		}

		// Keep only tx packet nr
		failpnr = failpnr & 0x3f;

		// Write failed packet nr to packet nr register
		*LAN_PNR = failpnr;

		// Write 0x6000 to pointer register (enables TX, RD, AUTOINC)
		*LAN_POINTER = 0x0060;

		// Read the packet's Status word from the data port
		status = *LAN_DATA_L;
/*
		ksprintf(message, "Status word: %X\n", status);
		c_conws(message);
*/

		// Acknowledge TXINT
		*LAN_IST_ACK = 0x02;

		// Resend packet
		Eth_set_bank(0);
		*LAN_TCR = (*LAN_TCR) | 0x0100;						//re-enable TXENA
	}
	else	// TXEMPTY
	{
		if (sending)
		{
//			c_conws("Successfully transmitted all queued packets!\n\r");
			sending = 0;
		}

		//Acknowledge TXEMPTY
		*LAN_IST_ACK = 0x04;
	}


	//Check ALLOC_INT, and send a packet that were previously enqueued
#ifdef USE_I6
	Eth_set_bank(2);
	if((*LAN_IST_ACK) & 0x08)					//check ALLOC_INT
	{
		b = if_dequeue(&nif->snd);				//fetch the previously enqueued packet
		if(b)
		{
			send_packet(nif, b);
			*ETH_REG = (*ETH_REG) ^ 0x40;			// Toggle LED1
		}
		else											//for some reason there was no packet enqueued
		{												//then we must deallocate the buffer in 91C111 manually.
			packetnr = (*LAN_ARR);				//read allocated packet nr (and status)
			*LAN_PNR = packetnr;

			*LAN_MMU = 0xA000;					//deallocate TX packet
			while((*LAN_MMU) & 0x0100);		//wait for MMU ready
		}
		*LAN_INTMSK = (*LAN_INTMSK) & (0xF7);			//disable ALLOC_INT
	}

#endif
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


