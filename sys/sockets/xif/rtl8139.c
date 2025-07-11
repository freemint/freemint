/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
        A RealTek RTL-8139 Fast Ethernet driver for Linux.

        Maintained by Jeff Garzik <jgarzik@mandrakesoft.com>
        Copyright 2000,2001 Jeff Garzik

        Much code comes from Donald Becker's rtl8139.c driver,
        versions 1.13 and older.  This driver was originally based
        on rtl8139.c version 1.07.  Header of rtl8139.c version 1.13:

        -----<snip>-----

                Written 1997-2001 by Donald Becker.
                This software may be used and distributed according to the
                terms of the GNU General Public License (GPL), incorporated
                herein by reference.  Drivers based on or derived from this
                code fall under the GPL and must retain the authorship,
                copyright and license notice.  This file is not a complete
                program and may only be used when the entire operating
                system is licensed under the GPL.

                This driver is for boards based on the RTL8129 and RTL8139
                PCI ethernet chips.

                The author may be reached as becker@scyld.com, or C/O Scyld
                Computing Corporation 410 Severn Ave., Suite 210 Annapolis
                MD 21403
                Support and updates available at
                http://www.scyld.com/network/rtl8139.html
                Twister-tuning table provided by Kinston
                <shangh@realtek.com.tw>.

        -----<snip>-----
        This software may be used and distributed according to the terms
        of the GNU General Public License, incorporated herein by reference.

        Contributors:

                Donald Becker - he wrote the original driver, kudos to him!
                (but please don't e-mail him for support, this isn't his driver)

                Tigran Aivazian - bug fixes, skbuff free cleanup

                Martin Mares - suggestions for PCI cleanup

                David S. Miller - PCI DMA and softnet updates

                Ernst Gill - fixes ported from BSD driver

                Daniel Kobras - identified specific locations of
                        posted MMIO write bugginess

                Gerard Sharp - bug fix, testing and feedback

                David Ford - Rx ring wrap fix

                Dan DeMaggio - swapped RTL8139 cards with me, and allowed me
                to find and fix a crucial bug on older chipsets.

                Donald Becker/Chris Butterworth/Marcus Westergren -
                Noticed various Rx packet size-related buglets.

                Santiago Garcia Mantinan - testing and feedback

                Jens David - 2.2.x kernel backports

                Martin Dennett - incredibly helpful insight on undocumented
                features of the 8139 chips

                Jean-Jacques Michel - bug fix

                Tobias Ringstr�m - Rx interrupt status checking suggestion

                Andrew Morton - Clear blocked signals, avoid
                buffer overrun setting current->comm.

--------------------------------------------------------------------------- */

# include "global.h"

# include "buf.h"
# include "inet4/if.h"
# include "inet4/ifeth.h"

# include "netinfo.h"

# include "mint/arch/delay.h"
# include "mint/mdelay.h"
# include "mint/sockio.h"
# include "mint/pcibios.h"
# include "mint/pci_ids.h"

# include "libkern/libkern.h"

/* XXX: shouldn't be used in a kernel driver... */
# include <mint/osbind.h>
# include <mint/ssystem.h>

# include "rtl8139.h"

/* ------------------------ Defines --------------------------------------- */
#define ARRAY_SIZE(arr) (sizeof(arr) / (sizeof(arr)[0]))

#ifndef OSBIND_CLOBBER_LIST
#define OSBIND_CLOBBER_LIST __CLOBBER_RETURN("d0") "d1", "d2", "a0", "a1", "a2"    /* clobbered regs */	\
    AND_MEMORY
#endif

#ifndef trap_14_wlw
#define trap_14_wlw(n, a, b)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	long  _a = (long) (a);	\
	short _b = (short) (b);	\
	\
	__asm__ volatile (	\
		"movw	%3,%%sp@-\n\t"	\
		"movl	%2,%%sp@-\n\t"	\
		"movw	%1,%%sp@-\n\t"	\
		"trap	#14\n\t"	\
		"lea	%%sp@(8),%%sp"	\
		: "=r"(retvalue)	\
		: "g"(n), "r"(_a), "r"(_b)	\
		: OSBIND_CLOBBER_LIST	\
	);	\
	retvalue;	\
})
#endif
#ifndef trap_14_wll
#define trap_14_wll(n, a, b)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	long _a = (long) (a);	\
	long _b = (long) (b);	\
	\
	__asm__ volatile (	\
		"movl	%3,%%sp@-\n\t"	\
		"movl	%2,%%sp@-\n\t"	\
		"movw	%1,%%sp@-\n\t"	\
		"trap	#14\n\t"	\
		"lea	%%sp@(10),%%sp"	\
		: "=r"(retvalue)	\
		: "g"(n), "r"(_a), "r"(_b)	\
		: OSBIND_CLOBBER_LIST	\
	);	\
	retvalue;	\
})
#endif
#ifndef trap_14_wlww
#define trap_14_wlww(n, a, b, c)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	long  _a = (long) (a);	\
	short _b = (short) (b);	\
	short _c = (short) (c);	\
	\
	__asm__ volatile (	\
		"movw	%4,%%sp@-\n\t"	\
		"movw	%3,%%sp@-\n\t"	\
		"movl	%2,%%sp@-\n\t"	\
		"movw	%1,%%sp@-\n\t"	\
		"trap	#14\n\t"	\
		"lea	%%sp@(10),%%sp"	\
		: "=r"(retvalue)	\
		: "g"(n), "r"(_a), "r"(_b), "r"(_c)	\
		: OSBIND_CLOBBER_LIST	\
	);	\
	retvalue;	\
})
#endif
#ifndef trap_14_wlwl
#define trap_14_wlwl(n, a, b, c)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	long  _a = (long) (a);	\
	short _b = (short) (b);	\
	long  _c = (long) (c);	\
	\
	__asm__ volatile (	\
		"movl	%4,%%sp@-\n\t"	\
		"movw	%3,%%sp@-\n\t"	\
		"movl	%2,%%sp@-\n\t"	\
		"movw	%1,%%sp@-\n\t"	\
		"trap	#14\n\t"	\
		"lea	%%sp@(12),%%sp"	\
		: "=r"(retvalue)	\
		: "g"(n), "r"(_a), "r"(_b), "r"(_c)	\
		: OSBIND_CLOBBER_LIST	\
	);	\
	retvalue;	\
})
#endif
#ifndef trap_14_wlll
#define trap_14_wlll(n, a, b, c)	\
__extension__	\
({	\
	register long retvalue __asm__("d0");	\
	long  _a = (long) (a);	\
	long  _b = (long) (b);	\
	long  _c = (long) (c);	\
	\
	__asm__ volatile (	\
		"movl	%4,%%sp@-\n\t"	\
		"movl	%3,%%sp@-\n\t"	\
		"movl	%2,%%sp@-\n\t"	\
		"movw	%1,%%sp@-\n\t"	\
		"trap	#14\n\t"	\
		"lea	%%sp@(14),%%sp"	\
		: "=r"(retvalue)	\
		: "g"(n), "r"(_a), "r"(_b), "r"(_c)	\
		: OSBIND_CLOBBER_LIST	\
	);	\
	retvalue;	\
})
#endif

/* PCI BIOS TODO: make accessible via KERINFO */
#define find_pci_device(id,index) (long)trap_14_wlw((short)(300),(unsigned long)(id),(unsigned short)(index))
#define find_pci_classcode(classcode,index) (long)trap_14_wlw((short)(301),(unsigned long)(classcode),(unsigned short)(index))
#define read_config_byte(handle,reg,address) (long)trap_14_wlwl((short)(302),(long)(handle),(unsigned short)(reg),(unsigned char *)(address))
#define read_config_word(handle,reg,address) (long)trap_14_wlwl((short)(303),(long)(handle),(unsigned short)(reg),(unsigned short *)(address))
#define read_config_longword(handle,reg,address) (long)trap_14_wlwl((short)(304),(long)(handle),(unsigned short)(reg),(unsigned long *)(address))
#define fast_read_config_byte(handle,reg) (unsigned char)trap_14_wlw((short)(305),(long)(handle),(unsigned short)(reg))
#define fast_read_config_word(handle,reg) (unsigned short)trap_14_wlw((short)(306),(long)(handle),(unsigned short)(reg))
#define fast_read_config_longword(handle,reg) (unsigned long)trap_14_wlw((short)(307),(long)(handle),(unsigned short)(reg))
#define write_config_byte(handle,reg,data) (long)trap_14_wlww((short)(308),(long)(handle),(unsigned short)(reg),(unsigned short)(data))
#define write_config_word(handle,reg,data) (long)trap_14_wlww((short)(309),(long)(handle),(unsigned short)(reg),(unsigned short)(data))
#define write_config_longword(handle,reg,data) (long)trap_14_wlwl((short)(310),(long)(handle),(unsigned short)(reg),(unsigned long)(data))
#define hook_interrupt(handle,routine,parameter) (long)trap_14_wlll((short)(311),(long)(handle),(unsigned long *)(routine),(unsigned long *)(parameter))
#define unhook_interrupt(handle) (long)trap_14_wl((short)(312),(long)(handle))
#define special_cycle(bus_number,special_cycle) (long)trap_14_wwl((short)(313),(unsigned short)(bus_number),(unsigned long)(special_cycle))
#define get_routing(handle) (long)trap_14_wl((short)(314),(long)(handle))
#define set_interrupt(handle,mode) (long)trap_14_wlw((short)(315),(long)(handle),(short)(mode))
#define get_resource(handle) (long)trap_14_wl((short)(316),(long)(handle))
#define get_card_used(handle,callback) (long)trap_14_wll((short)(317),(long)(handle),(long *)(address))
#define set_card_used(handle,callback) (long)trap_14_wll((short)(318),(long)(handle),(long *)(callback))
#define read_mem_byte(handle,offset,address) (long)trap_14_wlll((short)(319),(long)(handle),(unsigned long)(offset),(unsigned char *)(address))
#define read_mem_word(handle,offset,address) (long)trap_14_wlll((short)(320),(long)(handle),(unsigned long)(offset),(unsigned short *)(address))
#define read_mem_longword(handle,offset,address) (long)trap_14_wlll((short)(321),(long)(handle),(unsigned long)(offset),(unsigned long *)(address))
#define fast_read_mem_byte(handle,offset) (unsigned char)trap_14_wll((short)(322),(long)(handle),(unsigned long)(offset))
#define fast_read_mem_word(handle,offset) (unsigned short)trap_14_wll((short)(323),(long)(handle),(unsigned long)(offset))
#define fast_read_mem_longword(handle,offset) (unsigned long)trap_14_wll((short)(324),(long)(handle),(unsigned long)(offset))
#define write_mem_byte(handle,offset,data) (long)trap_14_wllw((short)(325),(long)(handle),(unsigned long)(offset),(unsigned short)(data))
#define write_mem_word(handle,offset,data) (long)trap_14_wllw((short)(326),(long)(handle),(unsigned long)(offset),(unsigned short)(data))
#define write_mem_longword(handle,offset,data) (long)trap_14_wlll((short)(327),(long)(handle),(unsigned long)(offset),(unsigned long)(data))
#define read_io_byte(handle,offset,address) (long)trap_14_wlll((short)(328),(long)(handle),(unsigned long)(offset),(unsigned char *)(address))
#define read_io_word(handle,offset,address) (long)trap_14_wlll((short)(329),(long)(handle),(unsigned long)(offset),(unsigned short *)(address))
#define read_io_longword(handle,offset,address) (long)trap_14_wlll((short)(330),(long)(handle),(unsigned long)(offset),(unsigned long *)(address))
#define fast_read_io_byte(handle,offset) (unsigned char)trap_14_wll((short)(331),(long)(handle),(unsigned long)(offset))
#define fast_read_io_word(handle,offset) (unsigned short)trap_14_wll((short)(332),(long)(handle),(unsigned long)(offset))
#define fast_read_io_longword(handle,offset) (unsigned long)trap_14_wll((short)(333),(long)(handle),(unsigned long)(offset))
#define write_io_byte(handle,offset,data) (long)trap_14_wllw((short)(334),(long)(handle),(unsigned long)(offset),(unsigned short)(data))
#define write_io_word(handle,offset,data) (long)trap_14_wllw((short)(335),(long)(handle),(unsigned long)(offset),(unsigned short)(data))
#define write_io_longword(handle,offset,data) (long)trap_14_wlll((short)(336),(long)(handle),(unsigned long)(offset),(unsigned long)(data))
#define get_machine_id() (long)trap_14_w((short)(337))
#define get_pagesize() (long)trap_14_w((short)(338))
#define virt_to_bus(handle,address,pointer) (long)trap_14_wlll((short)(339),(long)(handle),(unsigned long)(address),(unsigned long *)(pointer))
#define bus_to_virt(handle,address,pointer) (long)trap_14_wlll((short)(340),(long)(handle),(unsigned long)(address),(unsigned long *)(pointer))
#define virt_to_phys(address,pointer) (long)trap_14_wll((short)(341),(unsigned long)(address),(unsigned long *)(pointer))
#define phys_to_virt(address,pointer) (long)trap_14_wll((short)(342),(unsigned long)(address),(unsigned long *)(pointer))

/* CTPCI extensions */
#define dma_setbuffer(pci_address,local_address,size) (long)trap_14_wlll((short)(350),(unsigned long)(pci_address),(unsigned long)(local_address),(unsigned long)(size))
#define dma_buffoper(mode) (long)trap_14_ww((short)(351),(short)(mode))
#define read_mailbox(mailbox,pointer)  (long)trap_14_wwl((short)(352),(short)(mailbox),(unsigned long *)(pointer))
#define write_mailbox(mailbox,data) (long)trap_14_wwl((short)(353),(short)(mailbox),(unsigned long)(data))
#define dma_alloc(size) (long)trap_14_wl((short)(354),(unsigned long)(size))
#define dma_free(addr) (long)trap_14_wl((short)(355),(unsigned long)(addr))
#define dma_lock(mode) (long)trap_14_ww((short)(356),(short)(mode))

static int rtl8139_read_eeprom(void *ioaddr, int location, int addr_len);
static int rtl8139_mdio_read(struct rtl8139_private *rtl_8139, int phy_id, int location);
static void rtl8139_mdio_write(struct rtl8139_private *rtl_8139, int phy_id, int location, int val);

/* Maximum events (Rx packets, etc.) to handle at each interrupt. */
static int rtl8139_max_interrupt_work = 10;
static unsigned char rtl8139_ip_addr[2][4];
static int use_dma_alloc = 0;
static int rtl8139_n_filters = 0;
static unsigned char rtl8139_opened = 0;
static unsigned char mintnet_opened = 0;
static struct rtl8139_private rtl8139_tp;

static int splx(int level)
{
  register int old_level __asm__("d0");
  asm __volatile__ (
      " move.l %1, %%d6\n\t"     /* get argument */
      " move.w %%sr, %%d7\n\t"     /* current SR */
      " move.l %%d7, %%d0\n\t"     /* prepare return value */
      " and.l #0x700, %%d0\n\t"  /* mask out IPL */
      " lsr.l #8, %%d0\n\t"      /* IPL */
      " and.l #7, %%d6\n\t"      /* least significant three bits */
      " lsl.l #8, %%d6\n\t"      /* move over to make mask */
      " and.l #0xF8FF, %%d7\n\t" /* zero out current IPL */
      " or.l %%d6, %%d7\n\t"       /* place new IPL in SR */
      " move.w %%d7, %%sr"
      : "=d" (old_level)
      : "d" (level)
      : "d6", "d7", "cc"
  );
  return(old_level);
}

static void *rtl8139_nbuf_alloc(long size)
{
  void *ret;
  if(use_dma_alloc)
  {
    ret = (void *)dma_alloc(size);
	if((long)ret >= s_system(S_GETLVAL, 0x436, 0))	/* memtop */
      return(ret);
    else
      use_dma_alloc = 0;
  }
  return((void *)_m_xalloc(size, 0)); /* ST-RAM cache WT */
}

static void rtl8139_nbuf_free(void *addr)
{
  if(use_dma_alloc)
    dma_free(addr);
  _m_free(addr);
}

static int rtl8139_set_ip_filter(long ipaddr)
{
  if(rtl8139_n_filters <= 1)
  {
    rtl8139_ip_addr[rtl8139_n_filters][3] = ipaddr & 0x000000ffUL;
    rtl8139_ip_addr[rtl8139_n_filters][2] = (ipaddr >> 8) & 0x000000ffUL;
    rtl8139_ip_addr[rtl8139_n_filters][1] = (ipaddr >> 16) & 0x000000ffUL;
    rtl8139_ip_addr[rtl8139_n_filters][0] = (ipaddr >> 24) & 0x000000ffUL;
    rtl8139_n_filters++;
    return(0);
  }
  DEBUG(("RTL8139: You cannot set more than 2 IP filters !!"));
  return(-1);
}

/* The data sheet doesn't describe the Rx ring at all, so I'm guessing at the field alignments and semantics. */
static void rtl8139_rx_interrupt(struct rtl8139_private *rtl_8139_tp)
{
  unsigned char *rx_ring;
  unsigned long cur_rx;
  unsigned char mine = 1, multicast_packet= 1, arp_request_for_me = 1;
  void *ioaddr = rtl_8139_tp->mmio_addr;
  int i, j;
  rx_ring = rtl_8139_tp->rx_ring;
  cur_rx = rtl_8139_tp->cur_rx;
  while((RTL_R8(ChipCmd) & RxBufEmpty) == 0)
  {
    long ring_offset = cur_rx % RX_BUF_LEN;
    unsigned long rx_status;
    unsigned long rx_size, pkt_size;
    unsigned char *skb;
    /* read size+status of next frame from DMA ring buffer */
    rx_status = le32_to_cpu(*(unsigned long *)(rx_ring + ring_offset));
    rx_size = rx_status >> 16;
    pkt_size = rx_size - 4;
    if(rx_size == 0xfff0UL)
      break;
    if((rx_size > (MAX_ETH_FRAME_SIZE+4)) || (!(rx_status & RxStatusOK)))
      return;
    skb = &rx_ring[ring_offset+4];
    for(i = 0; i < 6; i++)
    {
      if(skb[i] == rtl_8139_tp->dev_addr[i])
        continue;
      else
      {
        mine = 0;
        break;
      }
    }
    if(mine)
      goto accept_frame;
    if((skb[12] == 0x08) && (skb[13] == 0x06))
    {
      for(j = 0; j < rtl8139_n_filters; j++)
      {
        for(i = 0; i < 4;i++)
        {
          if(skb[38+i] == rtl8139_ip_addr[j][i])
            continue;
          else
          {
            arp_request_for_me = 0;
            break;
          }
        }
      }
    }
    else
      arp_request_for_me = 0;
    for(i = 0; i < 6; i++)
    {
      if(skb[i] == 0xff)
        continue;
      else
      {
        multicast_packet = 0;
        break;
      }
    }
accept_frame:
    if(mine || (multicast_packet && arp_request_for_me))
    {
      if(!pkt_size)
        rtl_8139_tp->nif->in_errors++;
      else
      {
        BUF *b = buf_alloc(pkt_size+100, 50, BUF_ATOMIC);
        if(b == NULL)
          rtl_8139_tp->nif->in_errors++;
        else
        {
	        b->dend += pkt_size;
	        memcpy(b->dstart, skb, (int)pkt_size);
          /* Pass packet to upper layers */
          if(rtl_8139_tp->nif->bpf)
		        bpf_input(rtl_8139_tp->nif, b);
          /* and enqueue packet */
          if(!if_input(rtl_8139_tp->nif, b, 0, eth_remove_hdr(b)))
            rtl_8139_tp->nif->in_packets++;
          else
            rtl_8139_tp->nif->in_errors++;
        }
      }
    }
    cur_rx = (cur_rx + rx_size + 4 + 3) & ~3;
    RTL_W16_F(RxBufPtr, cur_rx - 16);
    mine = multicast_packet = arp_request_for_me = 1;
  }
  rtl_8139_tp->cur_rx = cur_rx;
}

static void rtl8139_tx_interrupt(struct rtl8139_private *rtl_8139_tp)
{
  void *ioaddr = rtl_8139_tp->mmio_addr;
  unsigned long dirty_tx, tx_left;
  dirty_tx = rtl_8139_tp->dirty_tx;
  tx_left = rtl_8139_tp->cur_tx - dirty_tx;
  while(tx_left > 0)
  {
    long entry = dirty_tx % NUM_TX_DESC;
    long txstatus = RTL_R32(TxStatus0 + (entry * sizeof(unsigned long)));
    if (!(txstatus & (TxStatOK | TxUnderrun | TxAborted)))
      break;  /* It still hasn't been Txed */
    /* Note: TxCarrierLost is always asserted at 100mbps. */
    if(txstatus & (TxOutOfWindow | TxAborted))
    {
      /* There was an major error, log it. */
//      DEBUG(("RTL8139: Transmit error, Tx status %8.8x.\r\n", txstatus));
      if(txstatus & TxAborted)
      {
        rtl_8139_tp->nif->out_errors++;
        RTL_W32(TxConfig, TxClearAbt | (TX_DMA_BURST << TxDMAShift));
      }
      if(txstatus & TxCarrierLost)
        rtl_8139_tp->nif->out_errors++;
      if(txstatus & TxOutOfWindow)
        rtl_8139_tp->nif->out_errors++;
//      if((txstatus & 0x0f000000) == 0x0f000000UL)
//        collisions16++;
    }
    else
    {
      if(txstatus & TxUnderrun)
      {
        /* Add 64 to the Tx FIFO threshold. */
        if(rtl_8139_tp->tx_flag < 0x00300000UL)
          rtl_8139_tp->tx_flag += 0x00020000UL;
        rtl_8139_tp->nif->out_errors++;
      }
	    rtl_8139_tp->nif->collisions += (txstatus >> 24) & 15;
	    rtl_8139_tp->nif->out_packets++;
    }
    /* Free the original skb. */
    dirty_tx++;
    tx_left--;
  }
#ifdef RTL8139_DEBUG
  if(rtl_8139_tp->cur_tx - dirty_tx > (long)NUM_TX_DESC)
  {
//    DEBUG(("RTL8139:  Out-of-sync dirty pointer, %ld vs. %ld.\r\n", dirty_tx, rtl_8139_tp->cur_tx));
    dirty_tx += (long)NUM_TX_DESC;
  }
#endif /* RTL8139_DEBUG */
  /* only wake the queue if we did work, and the queue is stopped */
  if(rtl_8139_tp->dirty_tx != dirty_tx)
    rtl_8139_tp->dirty_tx = dirty_tx;
}

/* The interrupt handler does all of the Rx thread work and cleans up after the Tx thread. */
static int rtl8139_interrupt(struct rtl8139_private *rtl_8139_tp)
{
  int boguscnt = rtl8139_max_interrupt_work;
  void *ioaddr = rtl_8139_tp->mmio_addr;
  unsigned short status = 0, link_changed = 0; /* avoid bogus "uninit" warning */
  do
  {
    if(rtl_8139_tp->ctpci_dma_lock != NULL)
    {
      int i = 0;
      while((i <= 100) && rtl_8139_tp->ctpci_dma_lock(1))
      {
        udelay(1); /* try to fix CTPCI freezes */
        i++;
      }
    }
    else
      udelay(100); /* try to fix CTPCI freezes */
    status = RTL_R16(IntrStatus);
    /* h/w no longer present (hotplug?) or major error, bail */
    if(status == 0xFFFF)
    {
      if(rtl_8139_tp->ctpci_dma_lock != NULL)
        rtl_8139_tp->ctpci_dma_lock(0);
      break;
    }
    /* Acknowledge all of the current interrupt sources ASAP, but an first get an additional status bit from CSCR. */
    if(status & RxUnderrun)
      link_changed = RTL_R16 (CSCR) & CSCR_LinkChangeBit;
	UNUSED (link_changed);

    RTL_W16_F(IntrStatus, (status & RxFIFOOver) ? (status | RxOverflow) : status);
    if((status & (PCIErr | PCSTimeout | RxUnderrun | RxOverflow | RxFIFOOver | TxErr | TxOK | RxErr | RxOK)) == 0)
    {
      if(rtl_8139_tp->ctpci_dma_lock != NULL)
        rtl_8139_tp->ctpci_dma_lock(0);
      break;
    }
    if(status & (RxOK | RxUnderrun | RxOverflow | RxFIFOOver))
      rtl8139_rx_interrupt(rtl_8139_tp);
    if(status & (TxOK | TxErr))
      rtl8139_tx_interrupt(rtl_8139_tp);
    if(rtl_8139_tp->ctpci_dma_lock != NULL)
      rtl_8139_tp->ctpci_dma_lock(0);
    boguscnt--;
  }
  while(boguscnt > 0);
  if((status != 0xFFFF) && (status & PCIErr))
  {
//    static char message[100];
    short sr = fast_read_config_word(rtl_8139_tp->handle, PCISR);
    write_config_word(rtl_8139_tp->handle, PCISR, sr);
//    ksprintf(message, "RTL8139: PCI ERROR SR %#04x\r\n", sr);
//    c_conws(message);
  }
  if(boguscnt <= 0)
  {
//    DEBUG(("RTL8139:  Too much work at interrupt, IntrStatus=0x%04x.\r\n", status));
    /* Clear all interrupt sources. */
    RTL_W16(IntrStatus, 0xffff);
  }
  return(1);
}

/* Syncronize the MII management interface by shifting 32 one bits out. */
static void rtl8139_mdio_sync(void *mdio_addr)
{
  int i;
  for(i = 32; i >= 0; i--)
  {
    writeb(MDIO_WRITE1, mdio_addr);
    rtl8139_mdio_delay(mdio_addr);
    writeb(MDIO_WRITE1 | MDIO_CLK, mdio_addr);
    rtl8139_mdio_delay(mdio_addr);
  }
}

static int rtl8139_mdio_read(struct rtl8139_private *rtl_8139_tp, int phy_id, int location)
{
  void *mdio_addr = (void *)((unsigned long)rtl_8139_tp->mmio_addr + Config4);
  long mii_cmd = (0xf6L << 10) | ((long)phy_id << 5) | (long)location;
  int retval = 0;
  int i;
  if(phy_id > 31)
    /* Really a 8139.  Use internal registers. */
    return location < 8 && mii_2_8139_map[location] ? readw((unsigned long)rtl_8139_tp->mmio_addr + mii_2_8139_map[location]) : 0;
   rtl8139_mdio_sync(mdio_addr);
   /* Shift the read command bits out. */
   for(i = 15; i >= 0; i--)
   {
     int dataval = (mii_cmd & (1 << i)) ? MDIO_DATA_OUT : 0;
     writeb(MDIO_DIR | dataval, mdio_addr);
     rtl8139_mdio_delay(mdio_addr);
     writeb(MDIO_DIR | dataval | MDIO_CLK, mdio_addr);
     rtl8139_mdio_delay(mdio_addr);
   }
   /* Read the two transition, 16 data, and wire-idle bits. */
   for(i = 19; i > 0; i--)
   {
     writeb(0, mdio_addr);
     rtl8139_mdio_delay(mdio_addr);
     retval = (retval << 1) | ((readb((unsigned long)mdio_addr) & MDIO_DATA_IN) ? 1 : 0);
     writeb(MDIO_CLK, mdio_addr);
     rtl8139_mdio_delay(mdio_addr);
   }
   return((retval >> 1) & 0xffff);
}

static void rtl8139_mdio_write(struct rtl8139_private *rtl_8139_tp, int phy_id, int location, int value)
{
  void *mdio_addr = (void *)((unsigned long)rtl_8139_tp->mmio_addr + Config4);
  long mii_cmd = (0x5002L << 16) | ((long)phy_id << 23) | ((long)location << 18) | (long)value;
  int i;
  if(phy_id > 31)
  {      /* Really a 8139.  Use internal registers. */
    void *ioaddr = rtl_8139_tp->mmio_addr;
    if(location == 0)
    {
      RTL_W8_F(Cfg9346, Cfg9346_Unlock);
      switch(rtl_8139_tp->AutoNegoAbility)
      {
        case 1: RTL_W16(NWayAdvert, AutoNegoAbility10half); break;
        case 2: RTL_W16(NWayAdvert, AutoNegoAbility10full); break;
        case 4: RTL_W16(NWayAdvert, AutoNegoAbility100half); break;
        case 8: RTL_W16(NWayAdvert, AutoNegoAbility100full); break;
        default: break;
      }
      RTL_W16_F(BasicModeCtrl, AutoNegotiationEnable|AutoNegotiationRestart);
      RTL_W8_F(Cfg9346, Cfg9346_Lock);
    }
    else if(location < 8 && mii_2_8139_map[location])
      RTL_W16_F(mii_2_8139_map[location], value);
  }
  else
  {
    rtl8139_mdio_sync(mdio_addr);
    /* Shift the command bits out. */
    for(i = 31; i >= 0; i--)
    {
      int dataval = (mii_cmd & (1 << i)) ? MDIO_WRITE1 : MDIO_WRITE0;
      writeb(dataval, mdio_addr);
      rtl8139_mdio_delay(mdio_addr);
      writeb(dataval | MDIO_CLK, mdio_addr);
      rtl8139_mdio_delay(mdio_addr);
    }
    /* Clear out extra bits. */
    for(i = 2; i > 0; i--)
    {
      writeb(0, mdio_addr);
      rtl8139_mdio_delay(mdio_addr);
      writeb(MDIO_CLK, mdio_addr);
      rtl8139_mdio_delay(mdio_addr);
    }
  }
}

/* Serial EEPROM section. */

/*  EEPROM_Ctrl bits. */
#define EE_SHIFT_CLK    0x04    /* EEPROM shift clock. */
#define EE_CS                   0x08    /* EEPROM chip select. */
#define EE_DATA_WRITE   0x02    /* EEPROM chip data in. */
#define EE_WRITE_0              0x00
#define EE_WRITE_1              0x02
#define EE_DATA_READ    0x01    /* EEPROM chip data out. */
#define EE_ENB                  (0x80 | EE_CS)

/* Delay between EEPROM clock transitions.
   No extra delay is needed with 33Mhz PCI, but 66Mhz may change this.
 */

#define rtl8139_eeprom_delay()  readl(ee_addr)

/* The EEPROM commands include the alway-set leading bit. */
#define EE_WRITE_CMD    (5)
#define EE_READ_CMD     (6)
#define EE_ERASE_CMD    (7)

static int rtl8139_read_eeprom(void *ioaddr, int location, int addr_len)
{
  int i;
  unsigned retval = 0;
  void *ee_addr = (void *)((unsigned long)ioaddr + Cfg9346);
  int read_cmd = location | (EE_READ_CMD << addr_len);
  writeb(EE_ENB & ~EE_CS, ee_addr);
  writeb(EE_ENB, ee_addr);
  rtl8139_eeprom_delay();
  /* Shift the read command bits out. */
  for(i = 4 + addr_len; i >= 0; i--)
  {
    int dataval = (read_cmd & (1 << i)) ? EE_DATA_WRITE : 0;
    writeb(EE_ENB | dataval, ee_addr);
    rtl8139_eeprom_delay();
    writeb(EE_ENB | dataval | EE_SHIFT_CLK, ee_addr);
    rtl8139_eeprom_delay();
  }
  writeb(EE_ENB, ee_addr);
  rtl8139_eeprom_delay();
  for(i = 16; i > 0; i--)
  {
    writeb(EE_ENB | EE_SHIFT_CLK, ee_addr);
    rtl8139_eeprom_delay();
    retval = (retval << 1) | ((readb(ee_addr) & EE_DATA_READ) ? 1 : 0);
    writeb(EE_ENB, ee_addr);
    rtl8139_eeprom_delay();
  }
  /* Terminate the EEPROM access. */
  writeb(~EE_CS, ee_addr);
  rtl8139_eeprom_delay();
  return(retval);
}

/* Start the hardware at open or resume. */
static void rtl8139_hw_start(struct rtl8139_private *rtl_8139_tp)
{
  void *ioaddr = rtl_8139_tp->mmio_addr;
  static char message[256];
  unsigned long i;
  unsigned char tmp;
  unsigned long* ptmp_long;
  int rx_mode;
  /* Soft reset the chip. */
  RTL_W8(ChipCmd, (RTL_R8(ChipCmd) & ChipCmdClear) | CmdReset);
  mdelay(100);
  /* Check that the chip has finished the reset. */
  for(i = 1000; i > 0; i--)
  {
    if((RTL_R8(ChipCmd) & CmdReset) == 0)
      break;
  }
  /* unlock Config[01234] and BMCR register writes */
  RTL_W8_F(Cfg9346, Cfg9346_Unlock);
  /* Restore our idea of the MAC address. */
  ptmp_long = (unsigned long *)(rtl_8139_tp->dev_addr + 0);
  RTL_W32_F(MAC0 + 0, cpu_to_le32(*ptmp_long));
  ptmp_long = (unsigned long *)(rtl_8139_tp->dev_addr + 4);
  RTL_W32_F(MAC0 + 4, cpu_to_le32(*ptmp_long));
  /* Must enable Tx/Rx before setting transfer thresholds! */
  RTL_W8_F(ChipCmd, (RTL_R8(ChipCmd) & ChipCmdClear) | CmdRxEnb | CmdTxEnb);
  RTL_W32_F(RxConfig, rtl8139_rx_config | (RTL_R32(RxConfig) & rtl_chip_info[rtl_8139_tp->chipset].RxConfigMask));
  /* Check this value: the documentation for IFG contradicts ifself. */
  RTL_W32(TxConfig, (TX_DMA_BURST << TxDMAShift));
  rtl_8139_tp->cur_rx = 0;
  /* This is check_duplex() */
  if((rtl_8139_tp->phys[0] >= 0) || (rtl_8139_tp->drv_flags & HAS_MII_XCVR))
  {
    unsigned short mii_reg5 = rtl8139_mdio_read(rtl_8139_tp, rtl_8139_tp->phys[0], 5);
    if(mii_reg5 == 0xffff); /* Not there */
    else if(((mii_reg5 & 0x0100) == 0x0100) || ((mii_reg5 & 0x00C0) == 0x0040))
      rtl_8139_tp->full_duplex = 1;
    ksprintf(message, "RTL8139: Setting %s%s-duplex based on auto-negotiated partner ability %04x.\r\n", mii_reg5 == 0 ? "" :
     (mii_reg5 & 0x0180) ? "100mbps " : "10mbps ", rtl_8139_tp->full_duplex ? "full" : "half", mii_reg5);
    c_conws(message);
  }
  if(rtl_8139_tp->chipset >= CH_8139A)
  {
    tmp = RTL_R8(Config1) & Config1Clear;
    tmp |= Cfg1_Driver_Load;
    tmp |= (rtl_8139_tp->chipset == CH_8139B) ? 3 : 1; /* Enable PM/VPD */
    RTL_W8_F(Config1, tmp);
  }
  else
  {
    unsigned char foo = RTL_R8(Config1) & Config1Clear;
    RTL_W8(Config1, rtl_8139_tp->full_duplex ? (foo|0x60) : (foo|0x20));
  }
  if(rtl_8139_tp->chipset >= CH_8139B)
  {
    tmp = RTL_R8(Config4) & ~(1<<2);
    /* chip will clear Rx FIFO overflow automatically */
    tmp |= (1<<7);
    RTL_W8(Config4, tmp);
    /* disable magic packet scanning, which is enabled when PM is enabled above (Config1) */
    RTL_W8(Config3, RTL_R8(Config3) & ~(1<<5));
  }
  /* Lock Config[01234] and BMCR register writes */
  RTL_W8_F(Cfg9346, Cfg9346_Lock);
  mdelay(10);
  /* init Rx ring buffer DMA address */
  RTL_W32_F(RxBuf, rtl_8139_tp->rx_ring_dma);
  /* init Tx buffer DMA addresses */
  for(i = 0; i < NUM_TX_DESC; i++)
    RTL_W32_F(TxAddr0 + (i * 4), rtl_8139_tp->tx_bufs_dma + (rtl_8139_tp->tx_buf[i] - rtl_8139_tp->tx_bufs));
  RTL_W32_F(RxMissed, 0);
  /* Set rx mode */
  rx_mode = AcceptBroadcast | AcceptMyPhys | AcceptAllPhys;
  /* We can safely update without stopping the chip. */
  RTL_W32_F(RxConfig, rtl8139_rx_config | rx_mode | (RTL_R32(RxConfig) & rtl_chip_info[rtl_8139_tp->chipset].RxConfigMask));
  /* no early-rx interrupts */
  RTL_W16(MultiIntr, RTL_R16(MultiIntr) & MultiIntrClear);
  /* make sure RxTx has started */
  RTL_W8_F(ChipCmd, (RTL_R8(ChipCmd) & ChipCmdClear) | CmdRxEnb | CmdTxEnb);
  /* Enable all known interrupts by setting the interrupt mask. */
  RTL_W16_F(IntrMask, rtl8139_intr_mask);
  rtl_8139_tp->trans_start = jiffies;
}

static void rtl8129_tx_timeout(struct rtl8139_private *rtl_8139_tp)
{
  void *ioaddr = rtl_8139_tp->mmio_addr;
  /* Disable interrupts by clearing the interrupt mask. */
  RTL_W16(IntrMask, 0x0000);
  rtl_8139_tp->dirty_tx = rtl_8139_tp->cur_tx = 0;
  rtl8139_hw_start(rtl_8139_tp);
}

static int rtl8139_send_packet(const char *buffer, long size)
{
  void *ioaddr = rtl8139_tp.mmio_addr;
  unsigned char *buff;
  int entry, level;
  if((rtl8139_tp.cur_tx - rtl8139_tp.dirty_tx) >= (long)NUM_TX_DESC)
  {
    if((jiffies - rtl8139_tp.trans_start) > TX_TIMEOUT)
      rtl8129_tx_timeout(&rtl8139_tp); /* timeout */
    else
      return(0);
  }
  level = splx(7); /* mask interrupts */
  if((rtl8139_tp.ctpci_dma_lock != NULL) && rtl8139_tp.ctpci_dma_lock(1))
  {
    splx(level); /* restore interrupts */
    return(0);
  }
  /* Calculate the next Tx descriptor entry. */
  entry = rtl8139_tp.cur_tx % NUM_TX_DESC;
  buff = rtl8139_tp.tx_buf[entry];
  if(buff != NULL)
    memcpy(buff, buffer, size);
  RTL_W32(TxAddr0 + (entry * 4), rtl8139_tp.tx_buf[entry]);
  rtl8139_tp.cur_tx++;
  /* Note: the chip doesn't have auto-pad! */
  RTL_W32(TxStatus0 + (entry * sizeof(unsigned long)), rtl8139_tp.tx_flag | (size >= ETH_ZLEN ? size : ETH_ZLEN));
  rtl8139_tp.trans_start = jiffies;
  if(rtl8139_tp.ctpci_dma_lock != NULL)
    rtl8139_tp.ctpci_dma_lock(0);
  splx(level); /* restore interrupts */
  return(size);
}

static int rtl8139_eth_start(void)
{
  static char message[256];
  unsigned long pio_start = 0, pio_len = 0;
  unsigned long pio_base_addr = 0xFFFFFFFFUL;
  unsigned long mmio_start = 0, mmio_len = 0;
  unsigned long mmio_base_addr = 0xFFFFFFFFUL;
  unsigned long dma_start = 0, dma_len = 0, dma_offset = 0;
  unsigned long dma_base_addr = 0xFFFFFFFFUL;
  PCI_RSC_DESC *pci_rsc_desc;
  unsigned char tmp8;
  int i, option, addr_len;
  short idx = 0;
  long handle, handle_bridge = -1, err;
  static int board_idx = 0;
  unsigned long tmp, id;
  void *ioaddr = NULL;
  do
  {
    handle = find_pci_device(0x0000FFFFL, idx++);
    if(handle >= 0)
    {
      err = read_config_longword(handle, PCIIDR, &id);
      if((err >= 0) && ((id & 0xFFFFUL) == 0x10B5) && ((id >> 16) == 0x9054)) /* bridge PLX 9054 CTPCI */
        handle_bridge = handle;
      if((err >= 0) && ((id & 0xFFFFUL) == RTL8139_VENDOR_ID) && ((id >> 16) == RTL8139_DEVICE_ID))
        break;
    }
  }
  while(handle >= 0);
  if(handle < 0)
    return(-1);
  if(handle_bridge >= 0)
  {
    pci_rsc_desc = (PCI_RSC_DESC *)get_resource(handle_bridge);
    if((long)pci_rsc_desc >= 0)
    {
      unsigned short flags;
	  unsigned long memtop = s_system(S_GETLVAL, 0x436, 0);
      do
      {
        if(!(pci_rsc_desc->flags & FLG_IO))
        {
          if(dma_base_addr == 0xFFFFFFFFUL)
          {
            dma_base_addr = pci_rsc_desc->start;
            dma_start = pci_rsc_desc->offset + pci_rsc_desc->start;
            dma_len = pci_rsc_desc->length;
            if(!((dma_start == 0) && (memtop < (dma_start+dma_len))))
              use_dma_alloc = 1;
            DEBUG(("RTL8139: PCI BRIDGE DMA start 0x%08lx length 0x%08lx\r\n", dma_start, dma_len));
          }
        }
        flags = pci_rsc_desc->flags;
        pci_rsc_desc = (PCI_RSC_DESC *)((unsigned long)pci_rsc_desc->next + (unsigned long)pci_rsc_desc);
      }
      while(!(flags & FLG_LAST));
    }
  }
  pci_rsc_desc = (PCI_RSC_DESC *)get_resource(handle);
  if((long)pci_rsc_desc >= 0)
  {
    unsigned short flags;
    do
    {
/*      DEBUG(("RTL8139: PCI RTL descriptors: flags 0x%04x start 0x%08lx \r\n offset 0x%08lx dmaoffset 0x%08lx length 0x%08lx",
      pci_rsc_desc->flags, pci_rsc_desc->start, pci_rsc_desc->offset, pci_rsc_desc->dmaoffset, pci_rsc_desc->length)); */
      if(!(pci_rsc_desc->flags & FLG_IO))
      {
        if(mmio_base_addr == 0xFFFFFFFFUL)
        {
          mmio_base_addr = pci_rsc_desc->start;
          mmio_start = pci_rsc_desc->offset + pci_rsc_desc->start;
          mmio_len = pci_rsc_desc->length;
          dma_offset = pci_rsc_desc->dmaoffset;
        }
      }
      else
      {
        if(pio_base_addr == 0xFFFFFFFFUL)
        {
          pio_base_addr = pci_rsc_desc->start;
          pio_start = pci_rsc_desc->offset + pci_rsc_desc->start;
          pio_len = pci_rsc_desc->length;
        }
      }
      flags = pci_rsc_desc->flags;
      pci_rsc_desc = (PCI_RSC_DESC *)((unsigned long)pci_rsc_desc->next + (unsigned long)pci_rsc_desc);
    }
    while(!(flags & FLG_LAST));
  }
  if((pio_base_addr == 0xFFFFFFFFUL) || (mmio_base_addr == 0xFFFFFFFFUL))
    return(-1);
  UNUSED (pio_start);
  ioaddr = (void *)mmio_start;
  memset(&rtl8139_tp, 0, sizeof(struct rtl8139_private));
  rtl8139_tp.handle = handle;
  rtl8139_tp.mmio_len = mmio_len;
  /* set this immediately, we need to know before we talk to the chip directly */
  if(pio_len == RTL8139B_IO_SIZE)
    rtl8139_tp.chipset = CH_8139B;
  /* check for weird/broken PCI region reporting */
  if((pio_len < RTL_MIN_IO_SIZE) || (mmio_len < RTL_MIN_IO_SIZE))
  {
    DEBUG(("RTL8139: Invalid PCI region size(s), aborting\r\n"));
    return(-1);
  }
  err = dma_lock(-1); /* CTPCI */
  if((err == 0) || (err == 1))
    rtl8139_tp.ctpci_dma_lock = (void *)dma_lock(-2); /* function exist */
  /* Soft reset the chip. */
  RTL_W8(ChipCmd, (RTL_R8(ChipCmd) & ChipCmdClear) | CmdReset);
  /* Check that the chip has finished the reset. */
  for(i = 1000; i > 0; i--)
  {
    if((RTL_R8(ChipCmd) & CmdReset) == 0)
      break;
    else
      udelay(10);
  }
  /* Bring the chip out of low-power mode. */
  if(rtl8139_tp.chipset == CH_8139B)
  {
    RTL_W8(Config1, RTL_R8(Config1) & ~(1<<4));
    RTL_W8(Config4, RTL_R8(Config4) & ~(1<<2));
  }
  else
  {
    /* handle RTL8139A and RTL8139 cases */
    /* XXX from becker driver. is this right?? */
    RTL_W8(Config1, 0);
  }
  /* make sure chip thinks PIO and MMIO are enabled */
  tmp8 = RTL_R8(Config1);
  if((tmp8 & Cfg1_PIO) == 0)
  {
    DEBUG(("RTL8139: PIO not enabled, Cfg1=%02X, aborting\r\n", tmp8));
    return(-1);
  }
  if((tmp8 & Cfg1_MMIO) == 0)
  {
    DEBUG(("RTL8139: MMIO not enabled, Cfg1=%02X, aborting\r\n", tmp8));
    return(-1);
  }
  /* identify chip attached to board */
  /* tmp = RTL_R8(ChipVersion); */
  tmp = RTL_R32(TxConfig);
  tmp = ((tmp & 0x7c000000UL) + ((tmp & 0x00800000UL) << 2)) >> 24;
  rtl8139_tp.drv_flags = board_info[0].hw_flags;
  rtl8139_tp.mmio_addr = ioaddr;
  for(i = ARRAY_SIZE(rtl_chip_info) - 1; i >= 0; i--)
  {
    if(tmp == rtl_chip_info[i].version)
       rtl8139_tp.chipset = i;
  }
  if(rtl8139_tp.chipset > (ARRAY_SIZE(rtl_chip_info) - 2))
    rtl8139_tp.chipset = ARRAY_SIZE(rtl_chip_info) - 2;
  /* Find the connected MII xcvrs. */
  if(rtl8139_tp.drv_flags & HAS_MII_XCVR)
  {
    int phy, phy_idx = 0;
    for(phy = 0; phy < 32 && phy_idx < sizeof(rtl8139_tp.phys); phy++)
    {
      int mii_status = rtl8139_mdio_read(&rtl8139_tp, phy, 1);
      if(mii_status != 0xffff && mii_status != 0x0000)
      {
        rtl8139_tp.phys[phy_idx++] = phy;
        rtl8139_tp.advertising = rtl8139_mdio_read(&rtl8139_tp, phy, 4);
        DEBUG(("RTL8139: MII transceiver %d status 0x%4.4x advertising %4.4x.\r\n", phy, mii_status, rtl8139_tp.advertising));
      }
    }
    if(phy_idx == 0)
    {
      DEBUG(("RTL8139: No MII transceivers found!  Assuming SYM transceiver.\r\n"));
      rtl8139_tp.phys[0] = 32;
    }
  }
  else
    rtl8139_tp.phys[0] = 32;
  /* Put the chip into low-power mode. */
  RTL_W8_F(Cfg9346, Cfg9346_Unlock);
  tmp = RTL_R8(Config1) & Config1Clear;
  tmp |= (rtl8139_tp.chipset == CH_8139B) ? 3 : 1; /* Enable PM/VPD */
  RTL_W8_F(Config1, tmp);
  RTL_W8_F(HltClk, 'H'); /* 'R' would leave the clock running. */
  /* The lower four bits are the media type. */
  option = (board_idx >= MAX_UNITS) ? 0 : media[board_idx];
  rtl8139_tp.AutoNegoAbility = option&0xF;
  if(option > 0)
  {
    rtl8139_tp.full_duplex = (option & 0x210) ? 1 : 0;
    rtl8139_tp.default_port = option & 0xFF;
    if(rtl8139_tp.default_port)
      rtl8139_tp.medialock = 1;
  }
  if(board_idx < MAX_UNITS  &&  full_duplex[board_idx] > 0)
      rtl8139_tp.full_duplex = full_duplex[board_idx];
  if(rtl8139_tp.full_duplex)
  {
    c_conws("RTL8139: Media type forced to Full Duplex.\r\n");
    /* Changing the MII-advertised media because might prevent  re-connection. */
    rtl8139_tp.duplex_lock = 1;
  }
  if(rtl8139_tp.default_port)
  {
    ksprintf(message, "RTL8139: Forcing %dMbs %s-duplex operation.\r\n", (option & 0x0C ? 100 : 10), (option & 0x0A ? "full" : "half"));
    c_conws(message);
    rtl8139_mdio_write(&rtl8139_tp, rtl8139_tp.phys[0], 0, ((option & 0x20) ? 0x2000 : 0) | /* 100mbps? */ ((option & 0x10) ? 0x0100 : 0)); /* Full duplex? */
  }
  addr_len = rtl8139_read_eeprom(ioaddr, 0, 8) == 0x8129 ? 8 : 6;
  for(i = 0; i < 3; ((unsigned short *)(rtl8139_tp.dev_addr))[i] = le16_to_cpu(rtl8139_read_eeprom(ioaddr, i + 7, addr_len)), i++);
  ksprintf(message, "RTL8139: %s at 0x%lx, %02x:%02x:%02x:%02x:%02x:%02x\r\n", board_info[0].name, (long)rtl8139_tp.mmio_addr,
   rtl8139_tp.dev_addr[0], rtl8139_tp.dev_addr[1], rtl8139_tp.dev_addr[2], rtl8139_tp.dev_addr[3], rtl8139_tp.dev_addr[4], rtl8139_tp.dev_addr[5]);
  c_conws(message);
  hook_interrupt(handle, rtl8139_interrupt, &rtl8139_tp);
  rtl8139_tp.must_free_irq = 1;
  rtl8139_tp.tx_bufs = rtl8139_nbuf_alloc(TX_BUF_TOT_LEN);
  rtl8139_tp.rx_ring = rtl8139_nbuf_alloc(RX_BUF_TOT_LEN);
  if((rtl8139_tp.tx_bufs == NULL) || (rtl8139_tp.rx_ring == NULL))
  {
    unhook_interrupt(handle);
    if(rtl8139_tp.tx_bufs)
      rtl8139_nbuf_free(rtl8139_tp.tx_bufs);
    if(rtl8139_tp.rx_ring)
     rtl8139_nbuf_free(rtl8139_tp.rx_ring);
    DEBUG(("RTL8139: EXIT, out of memory\r\n"));
    return(-1);
  }
  ksprintf(message, "RTL8139: TX buffers at 0x%lx, RX buffers at 0x%lx, DMA offset 0x%lx\r\n", (long)rtl8139_tp.tx_bufs, (long)rtl8139_tp.rx_ring, dma_offset);
  c_conws(message);
  rtl8139_tp.tx_bufs_dma = (unsigned long)rtl8139_tp.tx_bufs - dma_offset;
  rtl8139_tp.rx_ring_dma = (unsigned long)rtl8139_tp.rx_ring - dma_offset;
  rtl8139_tp.full_duplex = rtl8139_tp.duplex_lock;
  rtl8139_tp.tx_flag = (TX_FIFO_THRESH << 11) & 0x003f0000UL;
  rtl8139_tp.twistie = 1;
/* Initialize the Rx and Tx rings */
  rtl8139_tp.cur_rx = 0;
  rtl8139_tp.cur_tx = 0;
  rtl8139_tp.dirty_tx = 0;
  for(i = 0; i < NUM_TX_DESC; i++)
    rtl8139_tp.tx_buf[i] = &rtl8139_tp.tx_bufs[i * TX_BUF_SIZE];
  rtl8139_hw_start(&rtl8139_tp);
  ksprintf(message, "RTL8139: ioaddr 0x%lx handle 0x%lx %s-duplex.\r\n", mmio_start, handle, rtl8139_tp.full_duplex ? "full" : "half");
  c_conws(message);
  rtl8139_opened = 1;
  return(0);
}

static void rtl8139_eth_stop(void)
{
  void *ioaddr = rtl8139_tp.mmio_addr;
  int level = splx(7); /* mask interrupts */
  /* Stop the chip's Tx and Rx DMA processes. */
  RTL_W8(ChipCmd, (RTL_R8(ChipCmd) & ChipCmdClear));
  /* Disable interrupts by clearing the interrupt mask. */
  RTL_W16(IntrMask, 0x0000);
  RTL_W32(RxMissed, 0);
  if(rtl8139_tp.must_free_irq)
  {
    unhook_interrupt(rtl8139_tp.handle);
    rtl8139_tp.must_free_irq = 0;
  }
  rtl8139_tp.cur_tx = 0;
  rtl8139_tp.dirty_tx = 0;
  if(rtl8139_tp.rx_ring != NULL)
  {
    rtl8139_nbuf_free(rtl8139_tp.rx_ring);
    rtl8139_tp.rx_ring = NULL;
  }
  if(rtl8139_tp.tx_bufs != NULL)
  {
    rtl8139_nbuf_free(rtl8139_tp.tx_bufs);
    rtl8139_tp.tx_bufs = NULL;
  }
  /* Green! Put the chip in low-power mode. */
  RTL_W8(Cfg9346, Cfg9346_Unlock);
  RTL_W8(Config1, 0x03);
  RTL_W8(HltClk, 'H');   /* 'R' would leave the clock running. */
  splx(level); /* restore interrupts */
  rtl8139_opened = 0;
}

/* MiNTnet interface */

static long rtl8139_open(struct netif *nif)
{
  long ret = 0;
  if(!rtl8139_n_filters)
    rtl8139_set_ip_filter(*(long *)&rtl8139_tp.nif->addrlist->adr.sa.sa_data[2]);
  if(!rtl8139_opened)
    ret = rtl8139_eth_start();
  if(!ret && rtl8139_opened)
    mintnet_opened = 1;
  return(0);
}

static long rtl8139_close(struct netif *nif)
{
  rtl8139_eth_stop();
  rtl8139_n_filters = 0;
  mintnet_opened = 0;
  return(0);
}

static long rtl8139_output(struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	unsigned short len;
  BUF *nbuf = eth_build_hdr(buf, nif, hwaddr, pktype);
  if(nbuf == NULL)
  {
    DEBUG(("RTL8139: eth_build_hdr failed, out of memory!"));
    nif->out_errors++;
    return(ENOMEM);
  }
  /* pass to upper layer */
  if(nif->bpf)
    bpf_input(nif, nbuf);
  len = nbuf->dend - nbuf->dstart;
  if(rtl8139_send_packet((const char *)nbuf->dstart, (long)len) == 0)
  {
    long rc = if_enqueue(&nif->snd, nbuf, nbuf->info);
    if(rc)
    {
      DEBUG(("RTL8139: TX buffer full or DMA locked! Buffer lost."));
      nif->out_errors++;
    }
    else
    {
      DEBUG(("RTL8139: TX buffer full or DMA locked! Buffer pushed inside queue."));
    }
    return(rc);
	}
	buf_deref(nbuf, BUF_NORMAL);
//	nif->out_packets++; /* moved inside interrupt */
  return(0);
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
 * Return values  meaning
 * ENOSYS    option not supported
 * ENOENT    invalid option value
 * 0      Ok
 */
static long rtl8139_config(struct netif *nif, struct ifopt *ifo)
{
  if(!strncmp("hwaddr", ifo->option, sizeof(ifo->option)))
  {
    unsigned char *cp;
    /* Set hardware address */
    if(ifo->valtype != IFO_HWADDR)
      return(ENOENT);
    memcpy(nif->hwlocal.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
    cp = nif->hwlocal.adr.bytes;
	UNUSED(cp);
    DEBUG(("RTL8139: hwaddr is %x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
  }
  else if(!strncmp("braddr", ifo->option, sizeof(ifo->option)))
  {
    unsigned char *cp;
    /* Set broadcast address */
    if(ifo->valtype != IFO_HWADDR)
      return(ENOENT);
    memcpy(nif->hwbrcst.adr.bytes, ifo->ifou.v_string, ETH_ALEN);
    cp = nif->hwbrcst.adr.bytes;
	UNUSED(cp);
    DEBUG(("RTL8139: braddr is %x:%x:%x:%x:%x:%x", cp[0], cp[1], cp[2], cp[3], cp[4], cp[5]));
  }
  else if(!strncmp("debug", ifo->option, sizeof(ifo->option)))
  {
    /* turn debuggin on/off */
    if (ifo->valtype != IFO_INT)
      return(ENOENT);
    DEBUG(("RTL8139: debug level is %ld", ifo->ifou.v_long));
  }
  else if(!strncmp("log", ifo->option, sizeof(ifo->option)))
  {
    /* set log file  */
    if(ifo->valtype != IFO_STRING)
      return(ENOENT);
    DEBUG(("RTL8139: log file is %s", ifo->ifou.v_string));
  }
  return(ENOSYS);
}

static long rtl8139_ioctl(struct netif *nif, short cmd, long arg)
{
  switch(cmd)
  {
    case SIOCSIFNETMASK:
    case SIOCSIFFLAGS:
    case SIOCSIFADDR:
      return(0);
    case SIOCSIFMTU:
      /* Limit MTU to 1500 bytes. MintNet has already set nif->mtu to the new value, we only limit it here */
       if(nif->mtu > ETH_MAX_DLEN)
         nif->mtu = ETH_MAX_DLEN;
      return(0);
    case SIOCSIFOPT: /* Interface configuration */
      return(rtl8139_config(nif, ((struct ifreq *)arg)->ifru.data));
  }
  return(ENOSYS);
}

static struct netif if_rtl8139 =
{
  name:  "rtk",
  unit: 0,
  flags: IFF_BROADCAST,
  metric:  0,
  mtu: 1500,
  timer: 0,
  hwtype: HWTYPE_ETH,
  hwlocal: { len: ETH_ALEN },
  hwbrcst: { len: ETH_ALEN },
  snd: { maxqlen: IF_MAXQ },
  rcv: { maxqlen: IF_MAXQ },
  open:  rtl8139_open,
  close: rtl8139_close,
  output: rtl8139_output,
  ioctl: rtl8139_ioctl,
  timeout: NULL,
  data:  &rtl8139_tp,
  maxpackets: 0
};

long driver_init(void);
long driver_init(void)
{
  int i, ret = rtl8139_eth_start();
  if(ret)
    return(1);
  rtl8139_tp.nif = &if_rtl8139;
  for(i = 0; i < ETH_ALEN; i++)
  {
    if_rtl8139.hwlocal.adr.bytes[i] = rtl8139_tp.dev_addr[i];
    if_rtl8139.hwbrcst.adr.bytes[i] = 0xff;
  }
	if_register(&if_rtl8139);
	return(0);
}
