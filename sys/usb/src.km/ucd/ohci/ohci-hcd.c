/*
 * Copyright (c) 2026 The FreeMiNT development team
 *
 * Ported to ATARI by Didier Mequignon.
 *
 * URB OHCI HCD (Host Controller Driver) for USB and PCI bus.
 *
 * Interrupt support is added. Now, it has been tested
 * on ULI1575 chip and works well with USB keyboard.
 *
 * (C) Copyright 2007
 * Zhang Wei, Freescale Semiconductor, Inc. <wei.zhang@freescale.com>
 *
 * (C) Copyright 2003
 * Gary Jennejohn, DENX Software Engineering <garyj@denx.de>
 *
 * Note: Much of this code has been derived from Linux 2.4
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell
 *
 * Modified for the MP2USB by (C) Copyright 2005 Eric Benard
 * ebenard@eukrea.com - based on s3c24x0's driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
/*
 * IMPORTANT NOTE
 * this driver is intended for use with USB Mass Storage Devices
 * (BBB) and USB keyboard. There is NO support for Isochronous pipes!
 */

#ifndef TOSONLY
#if 0
# define DEV_DEBUG
#endif
#endif

#include <stddef.h>
#ifndef TOSONLY
#include "mint/mint.h"
#endif
#include "mint/dcntl.h"
#include "mint/ssystem.h"
#include "mint/pcibios.h"
#include "mint/pci_ids.h"

#include "../../global.h"

#ifndef TOSONLY
#include "mint/time.h"
#include "arch/timer.h"
#include "mint/mdelay.h"
#endif

#include "mint/endian.h"
#include "../../usb.h"
#include "../../usb_api.h"

#include "ohci.h"

#define VER_MAJOR	0
#define VER_MINOR	1

#ifdef TOSONLY
#define MSG_VERSION	"TOS"
#else
#define MSG_VERSION	"FreeMiNT"
#endif
#define MSG_BUILDDATE	__DATE__

#define MSG_BOOT	\
	"\033p OHCI USB controller driver " MSG_VERSION " \033q\r\n"

#define MSG_GREET	\
	"Ported by David Galvez.\r\n" \
	"Compiled " MSG_BUILDDATE ".\r\n\r\n"

/*
 * Kernel interface
 */

#ifndef TOSONLY
struct kentry	*kentry;
#else
extern long pcibios_installed;
extern int pcibios_init(void);
extern unsigned long _PgmSize;

extern void cpush(void *base, long size);
#endif
struct usb_module_api	*api;

#ifdef TOSONLY
machine_type machine;

/* Global variables for tosdelay.c. Should be defined here to avoid
 * "multiple definition" errors from the linker with -fno-common.
 */
unsigned long loopcount_1_msec;
unsigned long delay_1usec;

static void is_firebee(void)
{
	long mch = 0, cf = 0;

	if (!getcookie(COOKIE__MCH, &mch))
		(void)Cconws("USB: Failed to get _MCH cookie\r\n");

	if ((mch == FALCON) && getcookie(COOKIE__CF_, &cf))
		machine = machine_firebee;
}
#endif

/*
 * USB controller interface
 */

static long _cdecl	ohci_open	(struct ucdif *);
static long _cdecl	ohci_close	(struct ucdif *);
static long _cdecl	ohci_ioctl	(struct ucdif *, short, long);

static char lname[] = "OHCI-PCI USB driver\0";

/*
 * Function prototypes
 */

/* ramtop system variable - top of ST-RAM */
#define ramtop ((unsigned long *)0x05a4L)

long submit_bulk_msg(struct usb_device *, unsigned long, void *, long, long, unsigned long);
long submit_control_msg(struct usb_device *, unsigned long, void *, long, struct devrequest *);
long submit_int_msg(struct usb_device *, unsigned long, void *, long, long);

long usb_lowlevel_init(void *ucd_priv);
long usb_lowlevel_stop(void *ucd_priv);

long ohci_alloc_ucdif(struct ucdif **u);
long ohci_pci_probe(void);
int rh_check_port_status(ohci_t *controller);
#ifndef TOSONLY
long ohci_interrupt_handle(long param, long biosparam);
#endif

static void flush_dcache_all(void) __attribute__((unused));
static void flush_dcache_range(void *, long);
static void invalidate_dcache_range(void *, long);
#ifndef TOSONLY
static void *bounce_alloc(ohci_t *ohci, long size);
static void bounce_free(void *ptr);
#endif
#ifdef TOSONLY
static void ohci_mdelay_1_ikbd(void);
#endif

extern void ohci_int_handle_asm(void);

#undef OHCI_USE_NPS		/* force NoPowerSwitching mode */
#undef OHCI_VERBOSE_DEBUG	/* not always helpful */
#undef OHCI_FILL_TRACE

/* For initializing controller (mask in an HCFS mode too) */
#define OHCI_CONTROL_INIT (OHCI_CTRL_CBSR & 0x3) | OHCI_CTRL_IE | OHCI_CTRL_PLE

/* OHCI is a little-endian PCI device; swap on register access */
#define readl(a)	le2cpu32(*(volatile unsigned long *)(a))
#define writel(a, b)	(*(volatile unsigned long *)(b) = cpu2le32((volatile unsigned long)(a)))

#define min_t(type, x, y) ((type)(x) < (type)(y) ? (type)(x) : (type)(y))

struct pci_device_id ohci_usb_pci_table[] = {
	{ PCI_VENDOR_ID_AL, PCI_DEVICE_ID_AL_M5237, PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_SERIAL_USB_OHCI, 0, 0 }, /* ULI1575 */
	{ PCI_VENDOR_ID_NEC, PCI_DEVICE_ID_NEC_USB, PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_SERIAL_USB_OHCI, 0, 0 }, /* NEC */
	{ PCI_VENDOR_ID_PHILIPS, PCI_DEVICE_ID_PHILIPS_ISP1561, PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_SERIAL_USB_OHCI, 0, 0 }, /* Philips 1561 */
	/* Please add supported PCI OHCI controller ids here */
	{ 0, 0, 0, 0, 0, 0, 0 }
};

static inline unsigned long roothub_a(ohci_t *ohci)		{ return readl(&ohci->regs->roothub.a); }
static inline unsigned long roothub_b(ohci_t *ohci)		{ return readl(&ohci->regs->roothub.b); }
static inline unsigned long roothub_status(ohci_t *ohci)		{ return readl(&ohci->regs->roothub.status); }
static inline unsigned long roothub_portstatus(ohci_t *ohci, int i) { return readl(&ohci->regs->roothub.portstatus[i]); }

/* forward declarations */

static int hc_interrupt(ohci_t *ohci);
static void td_submit_job(ohci_t *ohci, struct usb_device *dev, unsigned long pipe,
 void *buffer, int transfer_len, struct devrequest *setup, urb_priv_t *urb, int interval);


/*-------------------------------------------------------------------------*
 * cache coherency and DMA bounce buffer helpers
 *-------------------------------------------------------------------------*/

static void *bounce_alloc(ohci_t *ohci, long size)
{
#ifdef TOSONLY
	if (size > OHCI_TOS_BOUNCE_SIZE)
	{
		ALERT(("OHCI: static bounce too small (need %ld, have %ld)",
		       size, (long)OHCI_TOS_BOUNCE_SIZE));
		return NULL;
	}
	return (void *)(((unsigned long)ohci->tos_bounce
	                 + (M68K_CACHE_LINE_SIZE - 1))
	                & ~((unsigned long)M68K_CACHE_LINE_SIZE - 1));
#else
	(void)ohci;
	return (void *)kmalloc(size);
#endif
}

static void bounce_free(void *ptr)
{
#ifdef TOSONLY
	(void)ptr;
#else
	kfree(ptr);
#endif
}

/* Locking functions used to protect the single TOS bounce buffer */

inline static char lock_usb(char *lock)
{
	char ret = 0;

	__asm("tas %1\n\t"
	      "seq %0"
	      : "=d" (ret)
	      : "m"  (*lock)
	      : "cc");

	return ret;
}

inline static void unlock_usb(char *lock)
{
	*lock = 0;
#ifdef __mcoldfire__
	/* tas bypasses the D-cache on ColdFire (MCF547x RM: a tas not
	 * hitting a RAMBAR is non-cacheable and precise), so this cached
	 * write to *lock is invisible to the next tas until the dirty line
	 * reaches RAM. Push it now via cpushl on the cache line containing
	 * the lock byte. The D-cache is 4-way set-associative and cpushl
	 * encodes the way in bits 1:0 of its operand, so we issue four
	 * cpushls (at aligned+0/+1/+2/+3) to walk all four ways.
	 */
	{
		unsigned long line = ((unsigned long)lock) & ~15UL;
		__asm__ __volatile__(
			"move.l %0,%%a0\n\t"
			"nop\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 0 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 1 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 2 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)"       /* way 3 */
			: : "g"(line)
			: "a0", "memory");
	}
#endif
}

/* Push and invalidate every line in the D-cache. On 68040/060 a single
 * cpusha %dc does this. On ColdFire V4e there is no cpusha, so we walk
 * every set/way index by hand. Kept available for callers that need a
 * whole-cache push without enumerating ranges; not currently used.
 */

static void flush_dcache_all(void)
{
#if defined(__mc68040__) || defined(__mc68060__)
	__asm__ __volatile__("cpusha %%dc" : : : "memory");
#elif defined(__mcoldfire__)
	__asm__ __volatile__(
		"nop\n\t"
		"move.w  %%sr, %%d2\n\t"    /* save IRQ state */
		"move.w  #0x2700, %%sr\n\t" /* mask IRQs so handlers can't dirty
		                              cache lines mid-walk */
		"moveq.l #0, %%d0\n\t"      /* way counter */
		"moveq.l #0, %%d1\n\t"      /* set counter */
		"move.l  %%d0, %%a0\n"      /* a0 starts at way index */
	"1:\n\t"
		"cpushl  %%dc, (%%a0)\n\t"
		"add.l   #0x10, %%a0\n\t"   /* next set */
		"addq.l  #1, %%d1\n\t"
		"cmpi.l  #512, %%d1\n\t"
		"bne     1b\n\t"
		"moveq.l #0, %%d1\n\t"
		"addq.l  #1, %%d0\n\t"
		"move.l  %%d0, %%a0\n\t"    /* next way, set back to 0 */
		"cmpi.l  #4, %%d0\n\t"
		"bne     1b\n\t"
		"move.w  %%d2, %%sr\n\t"    /* restore IRQ state */
		: : : "d0", "d1", "d2", "a0", "memory"
	);
#endif
}

static void flush_dcache_range(void *begin, long size)
{
#if defined(__mc68040__) || defined(__mc68060__)
	cpush(begin, size);
#elif defined(__mcoldfire__)
	/* On ColdFire the kernel-exported cpush() ignores the range arguments
	 * entirely and flushes the whole D+I cache, so we provide our own
	 * range-aware version here.
	 */
	unsigned long p = (unsigned long)begin & ~15UL;
	unsigned long end = (unsigned long)begin + (unsigned long)size;
	unsigned short saved_sr;
	/* Mask IRQs across the cpushl loop so a handler can't dirty
	 * cache lines (or evict ours) mid-flush. The D-cache is 4-way
	 * set-associative and cpushl encodes the way in bits 1:0 of its
	 * operand, so each line needs four cpushls (at +0/+1/+2/+3) to
	 * cover all four ways. A single cpushl at the aligned line address
	 * would only flush way 0 and miss the line if it sits in a
	 * different way.
	 */
	__asm__ __volatile__("move.w %%sr,%0\n\t"
	                     "move.w #0x2700,%%sr"
	                     : "=d"(saved_sr) : : "memory");
	__asm__ __volatile__("nop" : : : "memory");
	while (p < end)
	{
		__asm__ __volatile__(
			"move.l %0,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 0 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 1 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 2 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)"       /* way 3 */
			: : "g"(p) : "a0", "memory");
		p += 16;
	}
	__asm__ __volatile__("move.w %0,%%sr" : : "d"(saved_sr) : "memory");
#endif
}

/* The kernel doesn't export a function in kentry to invalidate caches,
 * so until then we have our own function.
 */
static void invalidate_dcache_range(void *begin, long size)
{
#if defined(__mc68040__) || defined(__mc68060__)
	/* cinvl invalidates without pushing dirty data to RAM.
	 * Caller must ensure flush_dcache_range() was called on this range
	 * before DMA started, otherwise dirty lines are silently discarded.
	 */
	unsigned long p = (unsigned long)begin & ~15UL;
	unsigned long end = (unsigned long)begin + (unsigned long)size;
	while (p < end)
	{
		__asm__ __volatile__("cinvl %%dc,(%0)" : : "a"(p) : "memory");
		p += 16;
	}
#elif defined(__mcoldfire__)
	/* ColdFire has no invalidate-without-push instruction. cpushl pushes
	 * any dirty lines to RAM before invalidating. If the cache has dirty
	 * data in this range it will overwrite what DMA wrote to RAM.
	 * Caller MUST call flush_dcache_range() on this range before DMA
	 * starts and must not write to it again until DMA completes. The
	 * D-cache is 4-way set-associative and cpushl encodes the way in
	 * bits 1:0 of its operand, so each line needs four cpushls (at
	 * +0/+1/+2/+3) to cover all four ways. A single cpushl at the
	 * aligned line address would only flush way 0 and miss the line if
	 * it sits in a different way.
	 */
	unsigned long p = (unsigned long)begin & ~15UL;
	unsigned long end = (unsigned long)begin + (unsigned long)size;
	__asm__ __volatile__("nop" : : : "memory");
	while (p < end)
	{
		__asm__ __volatile__(
			"move.l %0,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 0 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 1 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)\n\t"   /* way 2 */
			"addq.l #1,%%a0\n\t"
			"cpushl %%dc,(%%a0)"       /* way 3 */
			: : "g"(p) : "a0", "memory");
		p += 16;
	}
#endif
}

/*-------------------------------------------------------------------------*
 * URB support functions
 *-------------------------------------------------------------------------*/

static void urb_free_priv(urb_priv_t *urb)
{
	int i;
	struct td *td;
	int last = urb->length - 1;
	if(last >= 0)
	{
		for(i = 0; i <= last; i++)
		{
			td = urb->td[i];
			if(td)
			{
				td->usb_dev = NULL;
				urb->td[i] = NULL;
			}
		}
	}
	/* Bounce buffer lives for the lifetime of the URB — see the
	 * comment on urb_priv_t. Freeing here means Model A auto-requeue,
	 * which keeps the URB alive across completions, also keeps the
	 * buffer alive. bounce_free is a no-op on TOSONLY.
	 */
	if(urb->bounce_buf) bounce_free(urb->bounce_buf);
#ifndef TOSONLY
	/* On TOSONLY the URB is the preallocated tos_urb slot; skip Mfree.
	 * See the ohci_t comment on tos_urb for the rationale. */
	kfree(urb);
#endif
}

/*-------------------------------------------------------------------------*/

/* free HCD-private data associated with this URB */

#ifdef DEV_DEBUG
static int sohci_get_current_frame_number(ohci_t *ohci, struct usb_device *dev);

/* debug| print the main components of an URB
 * small: 0) header + data packets 1) just header */

static void pkt_print(ohci_t *ohci, urb_priv_t *purb, struct usb_device *dev,
		      unsigned long pipe, void *buffer, int transfer_len,
		      struct devrequest *setup, char *str, int small)
{
	DEBUG(("%s URB:[%4x] dev:%2lu,ep:%2lu-%c,type:%s,len:%d/%d stat:%#lx",
			str,
			sohci_get_current_frame_number(ohci, dev),
			usb_pipedevice(pipe),
			usb_pipeendpoint(pipe),
			usb_pipeout(pipe)? 'O': 'I',
			usb_pipetype(pipe) < 2 ? \
				(usb_pipeint(pipe)? "INTR": "ISOC"): \
				(usb_pipecontrol(pipe)? "CTRL": "BULK"),
			(purb ? purb->actual_length : 0),
			transfer_len, dev->status));
#ifdef OHCI_VERBOSE_DEBUG
	if(!small)
	{
		int i, len;
		if(usb_pipecontrol(pipe))
		{
			FORCE(("%s: cmd(8):", __FILE__));
			for(i = 0; i < 8 ; i++)
				FORCE((" %02x", ((unsigned char *)setup)[i]));
			FORCE(("\r\n"));
		}
		if(transfer_len > 0 && buffer)
		{
			FORCE(("%s: data(%d/%d):", __FILE__, (purb ? purb->actual_length : 0), transfer_len));
			len = usb_pipeout(pipe)? transfer_len : (purb ? purb->actual_length : 0);
			for(i = 0; i < 16 && i < len; i++)
				FORCE((" %02x", ((unsigned char *)buffer)[i]));
			FORCE(("%s\r\n", i < len? "...": ""));
		}
	}
#endif
}

/* just for debugging; prints non-empty branches of the int ed tree
 * inclusive iso eds */
static void ep_print_int_eds(ohci_t *ohci, char *str)
{
	int i, j;
	unsigned long *ed_p;
	for(i = 0; i < 32; i++)
	{
		j = 5;
		ed_p = &(ohci->hcca->int_table[i]);
		if(*ed_p == 0)
			continue;
		FORCE(("%s: %s branch int %2d(%2x):", __FILE__, str, i, i));
		while(*ed_p != 0 && j--)
		{
			ed_t *ed = (ed_t *)cpu2le32((unsigned long)ed_p);
			FORCE((" ed: %4lx", ed->hwINFO));
			ed_p = &ed->hwNextED;
		}
		FORCE(("\r\n"));
	}
}

static void ohci_dump_intr_mask(char *label, unsigned long mask)
{
	DEBUG(("%s: 0x%08lx%s%s%s%s%s%s%s%s%s",
		label, mask,
		(mask & OHCI_INTR_MIE) ? " MIE" : "",
		(mask & OHCI_INTR_OC) ? " OC" : "",
		(mask & OHCI_INTR_RHSC) ? " RHSC" : "",
		(mask & OHCI_INTR_FNO) ? " FNO" : "",
		(mask & OHCI_INTR_UE) ? " UE" : "",
		(mask & OHCI_INTR_RD) ? " RD" : "",
		(mask & OHCI_INTR_SF) ? " SF" : "",
		(mask & OHCI_INTR_WDH) ? " WDH" : "",
		(mask & OHCI_INTR_SO) ? " SO" : ""
		));
}

static void maybe_print_eds(ohci_t *controller, char *label, unsigned long value)
{
	ed_t *edp;
	value += controller->dma_offset;
	edp = (ed_t *)value;
	if(value && (value < 0xDFFFF0))
	{
		DEBUG(("%s %08lx", label, value));
		DEBUG(("%08lx", edp->hwINFO));
		DEBUG(("%08lx", edp->hwTailP));
		DEBUG(("%08lx", edp->hwHeadP));
		DEBUG(("%08lx", edp->hwNextED));
	}
}

static char *hcfs2string(int state)
{
	switch(state)
	{
		case OHCI_USB_RESET:	return "reset";
		case OHCI_USB_RESUME:	return "resume";
		case OHCI_USB_OPER:	return "operational";
		case OHCI_USB_SUSPEND:	return "suspend";
	}
	return "?";
}

static void ohci_dump_status(ohci_t *controller)
{
	struct ohci_regs *regs = controller->regs;
	unsigned long temp = readl(&regs->revision) & 0xff;
	if(temp != 0x10)
		DEBUG(("spec %d.%d", (int)(temp >> 4), (int)(temp & 0x0f)));
	temp = readl(&regs->control);
	DEBUG(("control: 0x%08lx%s%s%s HCFS=%s%s%s%s%s CBSR=%d", temp,
		(temp & OHCI_CTRL_RWE) ? " RWE" : "",
		(temp & OHCI_CTRL_RWC) ? " RWC" : "",
		(temp & OHCI_CTRL_IR) ? " IR" : "",
		hcfs2string(temp & OHCI_CTRL_HCFS),
		(temp & OHCI_CTRL_BLE) ? " BLE" : "",
		(temp & OHCI_CTRL_CLE) ? " CLE" : "",
		(temp & OHCI_CTRL_IE) ? " IE" : "",
		(temp & OHCI_CTRL_PLE) ? " PLE" : "",
		(int)(temp & OHCI_CTRL_CBSR)
		));
	temp = readl(&regs->cmdstatus);
	DEBUG(("cmdstatus: 0x%08lx SOC=%d%s%s%s%s", temp,
		(int)((temp & OHCI_SOC) >> 16),
		(temp & OHCI_OCR) ? " OCR" : "",
		(temp & OHCI_BLF) ? " BLF" : "",
		(temp & OHCI_CLF) ? " CLF" : "",
		(temp & OHCI_HCR) ? " HCR" : ""
		));
	ohci_dump_intr_mask("intrstatus", readl(&regs->intrstatus));
	ohci_dump_intr_mask("intrenable", readl(&regs->intrenable));
	maybe_print_eds(controller, "ed_periodcurrent", readl(&regs->ed_periodcurrent));
	maybe_print_eds(controller, "ed_controlhead", readl(&regs->ed_controlhead));
	maybe_print_eds(controller, "ed_controlcurrent", readl(&regs->ed_controlcurrent));
	maybe_print_eds(controller, "ed_bulkhead", readl(&regs->ed_bulkhead));
	maybe_print_eds(controller, "ed_bulkcurrent", readl(&regs->ed_bulkcurrent));
	maybe_print_eds(controller, "donehead", readl(&regs->donehead));
}

static void ohci_dump_roothub(ohci_t *controller, int verbose)
{
	unsigned long temp, ndp, i;
	temp = roothub_a(controller);
	ndp = controller->ndp;
	if(verbose)
	{
		DEBUG(("roothub.a: %08lx POTPGT=%d%s%s%s%s%s NDP=%d", temp,
			(int)(((temp & RH_A_POTPGT) >> 24) & 0xff),
			(temp & RH_A_NOCP) ? " NOCP" : "",
			(temp & RH_A_OCPM) ? " OCPM" : "",
			(temp & RH_A_DT) ? " DT" : "",
			(temp & RH_A_NPS) ? " NPS" : "",
			(temp & RH_A_PSM) ? " PSM" : "",
			(int)ndp
			));
		temp = roothub_b(controller);
		DEBUG(("roothub.b: %08lx PPCM=%04x DR=%04x",
			temp,
			(int)((temp & RH_B_PPCM) >> 16),
			(int)(temp & RH_B_DR)
			));
		temp = roothub_status(controller);
		DEBUG(("roothub.status: %08lx%s%s%s%s%s%s",
			temp,
			(temp & RH_HS_CRWE) ? " CRWE" : "",
			(temp & RH_HS_OCIC) ? " OCIC" : "",
			(temp & RH_HS_LPSC) ? " LPSC" : "",
			(temp & RH_HS_DRWE) ? " DRWE" : "",
			(temp & RH_HS_OCI) ? " OCI" : "",
			(temp & RH_HS_LPS) ? " LPS" : ""
			));
	}
	for(i = 0; i < ndp; i++)
	{
		temp = roothub_portstatus(controller, i);
		DEBUG(("roothub.portstatus [%d] = 0x%08lx%s%s%s%s%s%s%s%s%s%s%s%s",
			(int)i, temp,
			(temp & RH_PS_PRSC) ? " PRSC" : "",
			(temp & RH_PS_OCIC) ? " OCIC" : "",
			(temp & RH_PS_PSSC) ? " PSSC" : "",
			(temp & RH_PS_PESC) ? " PESC" : "",
			(temp & RH_PS_CSC) ? " CSC" : "",
			(temp & RH_PS_LSDA) ? " LSDA" : "",
			(temp & RH_PS_PPS) ? " PPS" : "",
			(temp & RH_PS_PRS) ? " PRS" : "",
			(temp & RH_PS_POCI) ? " POCI" : "",
			(temp & RH_PS_PSS) ? " PSS" : "",
			(temp & RH_PS_PES) ? " PES" : "",
			(temp & RH_PS_CCS) ? " CCS" : ""
			));
	}
}

/* dumps some of the state we know about */
static void ohci_dump(ohci_t *ohci, int verbose)
{
	DEBUG(("OHCI controller usb-%s-%c state", ohci->slot_name, (char)ohci->ctrl_num + '0'));
	ohci_dump_status(ohci);
	if(verbose)
		ep_print_int_eds(ohci, "hcca");
	DEBUG(("hcca frame #%04x", ohci->hcca->frame_no));
	ohci_dump_roothub(ohci, 1);
}
#endif /* DEV_DEBUG */

/*-------------------------------------------------------------------------*
 * Interface functions (URB)
 *-------------------------------------------------------------------------*/

/* get a transfer request */

static int sohci_submit_job(ohci_t *ohci, urb_priv_t *urb, struct devrequest *setup)
{
	ed_t *ed;
	urb_priv_t *purb_priv = urb;
	int i, size = 0;
	struct usb_device *dev = urb->dev;
	unsigned long pipe = urb->pipe;
	void *buffer = urb->transfer_buffer;
	int transfer_len = urb->transfer_buffer_length;
	int interval = urb->interval;
	/* when controller's hung, permit only roothub cleanup attempts
	 * such as powering down ports */
	if(ohci->disabled)
	{
		urb_free_priv(purb_priv);
		ALERT(("sohci_submit_job: EPIPE"));
		return -1;
	}
	/* we're about to begin a new transaction here so mark the
	 * URB unfinished */
	urb->finished = 0;
	/* every endpoint has a ed, locate and fill it */
	ed = ep_add_ed(ohci, dev, pipe, interval, 1);
	if(!ed)
	{
		urb_free_priv(purb_priv);
		ALERT(("sohci_submit_job: ENOMEM"));
		return -1;
	}
	/* for the private part of the URB we need the number of TDs (size) */
	switch(usb_pipetype(pipe))
	{
		case PIPE_BULK: /* one TD for every 4096 Byte */
			size = (transfer_len - 1) / 4096 + 1;
			break;
		case PIPE_CONTROL:/* 1 TD for setup, 1 for ACK and 1 for every 4096 B */
			size = (transfer_len == 0) ? 2: (transfer_len - 1) / 4096 + 3;
			break;
		case PIPE_INTERRUPT: /* 1 TD */
			size = 1;
			break;
	}
	ed->purb = urb;
	if(size >= (N_URB_TD - 1))
	{
		urb_free_priv(purb_priv);
		ALERT(("need %d TDs, only have %d", size, N_URB_TD));
		return -1;
	}
	purb_priv->pipe = pipe;
	/* fill the private part of the URB */
	purb_priv->length = size;
	purb_priv->ed = ed;
	purb_priv->actual_length = 0;
	/* allocate the TDs */
	/* note that td[0] was allocated in ep_add_ed */
	for(i = 0; i < size; i++)
	{
		purb_priv->td[i] = td_alloc(ohci, dev);
		if(!purb_priv->td[i])
		{
			purb_priv->length = i;
			urb_free_priv(purb_priv);
			ALERT(("sohci_submit_job: ENOMEM"));
			return -1;
		}
	}
	if(ed->state == ED_NEW || (ed->state & ED_DEL))
	{
		urb_free_priv(purb_priv);
		ALERT(("sohci_submit_job: EINVAL"));
		return -1;
	}
	/* link the ed into a chain if is not already */
	if(ed->state != ED_OPER)
		ep_link(ohci, ed);
	/* fill the TDs and link it to the ed */
	td_submit_job(ohci, dev, pipe, buffer, transfer_len, setup, purb_priv, interval);
	return 0;
}

/* Interrupt endpoints can be driven in two ways here:
 *
 *   Model A ("fire and forget"): caller submits once via submit_int_msg
 *   with the endpoint's bInterval, registers dev->irq_handle, and lets
 *   the HC re-arm the ED after every completion. The callback fires each
 *   time data arrives. Selected by interval > 0.
 *
 *   Model B ("caller polls"): . Caller submits synchronously each iteration
 *   (usually via submit_bulk_msg, interval == 0). The HC just returns the
 *   completion; the caller's poll thread re-submits. All in-tree device
 *   drivers use this model. This is what our mouse and keyboard drivers do.
 */
static inline int sohci_return_job(ohci_t *ohci, urb_priv_t *urb)
{
	struct ohci_regs *regs = ohci->regs;
	switch(usb_pipetype(urb->pipe))
	{
		case PIPE_INTERRUPT:
			if((urb->dev->irq_handle != NULL) && (urb->dev->irq_act_len = urb->actual_length))
			{
				if(urb->interval > 0)
				{
					/* Model A: keep WDH enabled across the callback so a
					 * Model A consumer that drives more USB traffic from
					 * irq_handle still sees ISR-path completions.
					 */
					writel(OHCI_INTR_WDH, &regs->intrenable);
					readl(&regs->intrenable); /* PCI posting flush */
					urb->dev->irq_handle(urb->dev);
					writel(OHCI_INTR_WDH, &regs->intrdisable);
					readl(&regs->intrdisable); /* PCI posting flush */
				}
				else
				{
					/* Model B: just fire the callback as a notification.
					 * Enabling WDH here would race with the poll loop —
					 * the completion's WDH bit is still latched in
					 * intrstatus and the ISR would immediately fire.
					 */
					urb->dev->irq_handle(urb->dev);
				}
			}
			if(urb->interval > 0)
			{
				/* Model A: HC re-arms the ED for the next arrival. */
				urb->actual_length = 0;
				td_submit_job(ohci, urb->dev, urb->pipe, urb->transfer_buffer, urb->transfer_buffer_length, NULL, urb, urb->interval);
			}
			/* Model B: caller re-submits synchronously — nothing to do. */
			break;
		case PIPE_CONTROL:
		case PIPE_BULK:
			break;
		default:
			return 0;
	}
	return 1;
}

/*-------------------------------------------------------------------------*/

#ifdef DEV_DEBUG
/* tell us the current USB frame number */
static int sohci_get_current_frame_number(ohci_t *ohci, struct usb_device *usb_dev)
{
	return cpu2le16(ohci->hcca->frame_no);
}
#endif

/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/

/* search for the right branch to insert an interrupt ed into the int tree
 * do some load ballancing;
 * returns the branch and
 * sets the interval to interval = 2^integer (ld (interval)) */
static int ep_int_ballance(ohci_t *ohci, int interval, int load)
{
	int i, branch = 0;
	/* search for the least loaded interrupt endpoint
	 * branch of all 32 branches
	 */
	for(i = 0; i < 32; i++)
	{
		if(ohci->ohci_int_load[branch] > ohci->ohci_int_load[i])
			branch = i;
	}
	branch = branch % interval;
	for(i = branch; i < 32; i += interval)
		ohci->ohci_int_load[i] += load;
	return branch;
}

/*-------------------------------------------------------------------------*/

/*  2^int( ld (inter)) */

static int ep_2_n_interval(int inter)
{
	int i;
	for(i = 0; ((inter >> i) > 1) && (i < 5); i++);
	return 1 << i;
}

/*-------------------------------------------------------------------------*/

/* the int tree is a binary tree
 * in order to process it sequentially the indexes of the branches have to
 * be mapped the mapping reverses the bits of a word of num_bits length */
static int ep_rev(int num_bits, int word)
{
	int i, wout = 0;
	for(i = 0; i < num_bits; i++)
		wout |= (((word >> i) & 1) << (num_bits - i - 1));
	return wout;
}

/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/

/* link an ed into one of the HC chains */

static int ep_link(ohci_t *ohci, ed_t *edi)
{
	volatile ed_t *ed = edi;
	int int_branch;
	int i;
	int inter;
	int interval;
	int load;
	unsigned long *ed_p;
	ed->state = ED_OPER;
	ed->int_interval = 0;
	switch(ed->type)
	{
		case PIPE_CONTROL:
			ed->hwNextED = 0;
			if(ohci->ed_controltail == NULL)
				writel((unsigned long)ed - ohci->dma_offset, &ohci->regs->ed_controlhead);
			else
				ohci->ed_controltail->hwNextED = cpu2le32((unsigned long)ed - ohci->dma_offset);
			ed->ed_prev = ohci->ed_controltail;
			if(!ohci->ed_controltail && !ohci->ed_rm_list[0] && !ohci->ed_rm_list[1] && !ohci->sleeping)
			{
				ohci->hc_control |= OHCI_CTRL_CLE;
				writel(ohci->hc_control, &ohci->regs->control);
			}
			ohci->ed_controltail = edi;
			break;
		case PIPE_BULK:
			ed->hwNextED = 0;
			if(ohci->ed_bulktail == NULL)
				writel((unsigned long)ed - ohci->dma_offset, &ohci->regs->ed_bulkhead);
			else
				ohci->ed_bulktail->hwNextED = cpu2le32((unsigned long)ed - ohci->dma_offset);
			ed->ed_prev = ohci->ed_bulktail;
			if(!ohci->ed_bulktail && !ohci->ed_rm_list[0] && !ohci->ed_rm_list[1] && !ohci->sleeping)
			{
				ohci->hc_control |= OHCI_CTRL_BLE;
				writel(ohci->hc_control, &ohci->regs->control);
			}
			ohci->ed_bulktail = edi;
			break;
		case PIPE_INTERRUPT:
			load = ed->int_load;
			interval = ep_2_n_interval(ed->int_period);
			ed->int_interval = interval;
			int_branch = ep_int_ballance(ohci, interval, load);
			ed->int_branch = int_branch;
			for(i = 0; i < ep_rev(6, interval); i += inter)
			{
				inter = 1;
				for(ed_p = &(ohci->hcca->int_table[ep_rev(5, i) + int_branch]);
				 (*ed_p != 0) && (((ed_t *)ed_p)->int_interval >= interval);
				 ed_p = &(((ed_t *)ed_p)->hwNextED))
					inter = ep_rev(6, ((ed_t *)ed_p)->int_interval);
				ed->hwNextED = *ed_p;
				*ed_p = cpu2le32((unsigned long)ed - ohci->dma_offset);
				flush_dcache_range((void *)ed_p, sizeof(*ed_p));
			}
			break;
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

/* scan the periodic table to find and unlink this ED */
static void periodic_unlink(struct ohci *ohci, volatile struct ed *ed, unsigned index, unsigned period)
{
	for( ;index < NUM_INTS; index += period)
	{
		unsigned long *ed_p = &ohci->hcca->int_table[index];
		while(*ed_p != 0)
		{
			if((unsigned long)*ed_p == cpu2le32((unsigned long)ed - ohci->dma_offset))
			{
				*ed_p = ed->hwNextED;
				break;
			}
			ed_p = &(((struct ed *)ed_p)->hwNextED);
		}
	}
}

/* unlink an ed from one of the HC chains.
 * just the link to the ed is unlinked.
 * the link from the ed still points to another operational ed or 0
 * so the HC can eventually finish the processing of the unlinked ed */

static int ep_unlink(ohci_t *ohci, ed_t *edi)
{
	volatile ed_t *ed = edi;
	int i;
	ed->hwINFO |= cpu2le32(OHCI_ED_SKIP);
	switch(ed->type)
	{
		case PIPE_CONTROL:
			if(ed->ed_prev == NULL)
			{
				if(!ed->hwNextED)
				{
					ohci->hc_control &= ~OHCI_CTRL_CLE;
					writel(ohci->hc_control, &ohci->regs->control);
				}
				writel(cpu2le32(*((unsigned long *)&ed->hwNextED)), &ohci->regs->ed_controlhead);
			}
			else
				ed->ed_prev->hwNextED = ed->hwNextED;
			if(ohci->ed_controltail == ed)
				ohci->ed_controltail = ed->ed_prev;
			else
				((ed_t *)(cpu2le32(*((unsigned long *)&ed->hwNextED)) + ohci->dma_offset))->ed_prev = ed->ed_prev;
			break;
		case PIPE_BULK:
			if(ed->ed_prev == NULL)
			{
				if(!ed->hwNextED)
				{
					ohci->hc_control &= ~OHCI_CTRL_BLE;
					writel(ohci->hc_control, &ohci->regs->control);
				}
				writel(cpu2le32(*((unsigned long *)&ed->hwNextED)), &ohci->regs->ed_bulkhead);
			}
			else
				ed->ed_prev->hwNextED = ed->hwNextED;
			if(ohci->ed_bulktail == ed)
				ohci->ed_bulktail = ed->ed_prev;
			else
				((ed_t *)(cpu2le32(*((unsigned long *)&ed->hwNextED)) + ohci->dma_offset))->ed_prev = ed->ed_prev;
			break;
		case PIPE_INTERRUPT:
			periodic_unlink(ohci, ed, 0, 1);
			for(i = ed->int_branch; i < 32; i += ed->int_interval)
				ohci->ohci_int_load[i] -= ed->int_load;
			break;
	}
	ed->state = ED_UNLINK;
	return 0;
}

/*-------------------------------------------------------------------------*/

/* add/reinit an endpoint; this should be done once at the
 * usb_set_configuration command, but the USB stack is a little bit
 * stateless so we do it at every transaction if the state of the ed
 * is ED_NEW then a dummy td is added and the state is changed to
 * ED_UNLINK in all other cases the state is left unchanged the ed
 * info fields are setted anyway even though most of them should not
 * change
 */
static ed_t *ep_add_ed(ohci_t *ohci, struct usb_device *usb_dev, unsigned long pipe, int interval, int load)
{
	td_t *td;
	ed_t *ed_ret;
	volatile ed_t *ed;
	struct ohci_device *ohci_dev = ohci->ohci_dev;
	ed = ed_ret = &ohci_dev->ed[(usb_pipeendpoint(pipe) << 1) | (usb_pipecontrol(pipe)? 0: usb_pipeout(pipe))];
	if((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
	{
		ALERT(("ep_add_ed: pending delete"));
		return NULL;
	}
	if(ed->state == ED_NEW)
	{
		/* dummy td; end of td list for ed */
		td = td_alloc(ohci, usb_dev);
		ed->hwTailP = cpu2le32((unsigned long)td - ohci->dma_offset);
		ed->hwHeadP = ed->hwTailP;
		ed->state = ED_UNLINK;
		ed->type = usb_pipetype(pipe);
		ohci_dev->ed_cnt++;
	}
	ed->hwINFO = cpu2le32(usb_pipedevice(pipe)
			| usb_pipeendpoint(pipe) << 7
			| (usb_pipeisoc(pipe)? 0x8000: 0)
			| (usb_pipecontrol(pipe)? 0: (usb_pipeout(pipe)? 0x800: 0x1000))
			| usb_pipeslow(pipe) << 13
			| usb_maxpacket(usb_dev, pipe) << 16);
	if(ed->type == PIPE_INTERRUPT && ed->state == ED_UNLINK)
	{
		ed->int_period = interval;
		ed->int_load = load;
	}
	return ed_ret;
}

/*-------------------------------------------------------------------------*
 * TD handling functions
 *-------------------------------------------------------------------------*/

/* enqueue next TD for this URB (OHCI spec 5.2.8.2) */

static void td_fill(ohci_t *ohci, unsigned long info, void *data, int len,
 struct usb_device *dev, int index, urb_priv_t *urb_priv)
{
	volatile td_t *td, *td_pt;
	if(index > urb_priv->length)
	{
		ALERT(("index > length"));
		return;
	}
	/* use this td as the next dummy */
	td_pt = urb_priv->td[index];
	td_pt->hwNextTD = 0;
	/* fill the old dummy TD */
	td = urb_priv->td[index] = (td_t *)((cpu2le32(urb_priv->ed->hwTailP) & ~0xf) + ohci->dma_offset);
	td->ed = urb_priv->ed;
	td->next_dl_td = NULL;
	td->index = index;
	td->data = (unsigned long)data;
	if(!len)
		data = NULL;
	td->hwINFO = cpu2le32(info);
	if(data != NULL)
	{
		td->hwCBP = cpu2le32((unsigned long)data - ohci->dma_offset);
		td->hwBE = cpu2le32((unsigned long)((unsigned char *)data + len - 1) - ohci->dma_offset);
	}
	else
	{
		td->hwCBP = 0;
		td->hwBE = 0;
	}
	td->hwNextTD = cpu2le32((unsigned long)td_pt - ohci->dma_offset);
	/* append to queue */
	td->ed->hwTailP = td->hwNextTD;
	/* push TD, dummy, ED, and any OUT/SETUP data to memory before HC reads them */
	flush_dcache_range((void *)td, sizeof(td_t));
	flush_dcache_range((void *)td_pt, sizeof(td_t));
	flush_dcache_range((void *)td->ed, sizeof(ed_t));
	if(data != NULL)
		flush_dcache_range(data, len);
}

/*-------------------------------------------------------------------------*/

/* prepare all TDs of a transfer */

static void td_submit_job(ohci_t *ohci, struct usb_device *dev, unsigned long pipe,
 void *buffer, int transfer_len, struct devrequest *setup, urb_priv_t *urb, int interval)
{
	int data_len = transfer_len;
	unsigned char *data;
	int cnt = 0;
	unsigned long info = 0;
	unsigned long toggle = 0;
	/* OHCI handles the DATA-toggles itself, we just use the USB-toggle
	 * bits for reseting */
	if(usb_gettoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe)))
		toggle = TD_T_TOGGLE;
	else
	{
		toggle = TD_T_DATA0;
		usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 1);
	}
	urb->td_cnt = 0;
	if(data_len)
		data = (unsigned char *)buffer;
	else
		data = NULL;
	switch(usb_pipetype(pipe))
	{
		case PIPE_BULK:
			info = usb_pipeout(pipe) ? TD_CC | TD_DP_OUT : TD_CC | TD_DP_IN ;
			while(data_len > 4096)
			{
				td_fill(ohci, info | (cnt? TD_T_TOGGLE : toggle), data, 4096, dev, cnt, urb);
				data += 4096; data_len -= 4096; cnt++;
			}
			info = usb_pipeout(pipe) ? TD_CC | TD_DP_OUT : TD_CC | TD_R | TD_DP_IN ;
			td_fill(ohci, info | (cnt? TD_T_TOGGLE:toggle), data, data_len, dev, cnt, urb);
			cnt++;
			if(!ohci->sleeping)	/* start bulk list */
				writel(OHCI_BLF, &ohci->regs->cmdstatus);
			break;
		case PIPE_CONTROL:
			/* Setup phase */
			info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
			td_fill(ohci, info, setup, 8, dev, cnt++, urb);
			/* Optional Data phase */
			if(data_len > 0)
			{
				info = usb_pipeout(pipe) ? TD_CC | TD_R | TD_DP_OUT | TD_T_DATA1 : TD_CC | TD_R | TD_DP_IN | TD_T_DATA1;
				/* NOTE:  mishandles transfers >8K, some >4K */
				td_fill(ohci, info, data, data_len, dev, cnt++, urb);
			}
			/* Status phase */
			info = usb_pipeout(pipe) ? TD_CC | TD_DP_IN | TD_T_DATA1 : TD_CC | TD_DP_OUT | TD_T_DATA1;
			td_fill(ohci, info, data, 0, dev, cnt++, urb);
			if(!ohci->sleeping) /* start Control list */
				writel(OHCI_CLF, &ohci->regs->cmdstatus);
			break;
		case PIPE_INTERRUPT:
			info = usb_pipeout(urb->pipe) ? TD_CC | TD_DP_OUT | toggle : TD_CC | TD_R | TD_DP_IN | toggle;
			td_fill(ohci, info, data, data_len, dev, cnt++, urb);
			break;
	}
#ifdef DEV_DEBUG
	if(urb->length != cnt)
		DEBUG(("TD LENGTH %d != CNT %d", urb->length, cnt));
#endif
}

/*-------------------------------------------------------------------------*
 * Done List handling functions
 *-------------------------------------------------------------------------*/

/* calculate the transfer length and update the urb */

static void dl_transfer_length(ohci_t *ohci, td_t *td)
{
	unsigned long tdBE, tdCBP;
	urb_priv_t *lurb_priv = td->ed->purb;
	tdBE   = cpu2le32(td->hwBE);
	tdCBP  = cpu2le32(td->hwCBP);
	if(tdBE)
		tdBE += ohci->dma_offset;
	if(tdCBP)
		tdCBP += ohci->dma_offset;
	if(!(usb_pipecontrol(lurb_priv->pipe) && ((td->index == 0) || (td->index == lurb_priv->length - 1))))
	{
		if(tdBE != 0)
		{
			if(td->hwCBP == 0)
				lurb_priv->actual_length += (tdBE - td->data + 1);
			else
				lurb_priv->actual_length += (tdCBP - td->data);
		}
	}
}

/*-------------------------------------------------------------------------*/

static void check_status(ohci_t *ohci, td_t *td_list)
{
	urb_priv_t *lurb_priv = td_list->ed->purb;
	int urb_len = lurb_priv->length;
	unsigned long *phwHeadP = &td_list->ed->hwHeadP;
	int cc = TD_CC_GET(cpu2le32(td_list->hwINFO));
	if(cc)
	{
		ALERT(("OHCI usb-%s-%c error: %s (%x)", ohci->slot_name, (char)ohci->ctrl_num + '0', cc_to_string[cc], cc));
		if(*phwHeadP & cpu2le32(0x1))
		{
			if(lurb_priv && ((td_list->index + 1) < urb_len))
			{
				*phwHeadP = (lurb_priv->td[urb_len - 1]->hwNextTD & cpu2le32(0xfffffff0)) | (*phwHeadP & cpu2le32(0x2));
				lurb_priv->td_cnt += urb_len - td_list->index - 1;
			}
			else
				*phwHeadP &= cpu2le32(0xfffffff2);
		}
	}
}

/* replies to the request have to be on a FIFO basis so
 * we reverse the reversed done-list */
static td_t *dl_reverse_done_list(ohci_t *ohci)
{
	unsigned long td_list_hc;
	td_t *td_rev = NULL;
	td_t *td_list = NULL;
	td_list_hc = cpu2le32(ohci->hcca->done_head) & ~0xf;
	if(td_list_hc)
		td_list_hc += ohci->dma_offset;
	ohci->hcca->done_head = 0;
	flush_dcache_range(&ohci->hcca->done_head, sizeof(ohci->hcca->done_head));
	while(td_list_hc)
	{
		td_list = (td_t *)td_list_hc;
		invalidate_dcache_range(td_list, sizeof(td_t));
		check_status(ohci, td_list);
		td_list->next_dl_td = td_rev;
		td_rev = td_list;
		td_list_hc = cpu2le32(td_list->hwNextTD) & ~0xf;
		if(td_list_hc)
			td_list_hc += ohci->dma_offset;
	}
	return td_list;
}

/*-------------------------------------------------------------------------*/

static void finish_urb(ohci_t *ohci, urb_priv_t *urb, int status)
{
	if((status & (ED_OPER | ED_UNLINK)) && (urb->state != URB_DEL))
		urb->finished = sohci_return_job(ohci, urb);
	else
		DEBUG(("finish_urb: strange.., ED state %x\r\n", status));
}

/* Used to take back a TD from the host controller. This would be called
 * from the interrupt service routine.
 */
static int takeback_td(ohci_t *ohci, td_t *td_list)
{
	ed_t *ed;
	int cc;
	int stat = 0;
	/* urb_t *urb; */
	urb_priv_t *lurb_priv;
	unsigned long tdINFO, edHeadP, edTailP;
	tdINFO = cpu2le32(td_list->hwINFO);
	ed = td_list->ed;
	if(ed == NULL)
	{
		ALERT(("OHCI usb-%s-%c cannot get error code ED is null", ohci->slot_name, (char)ohci->ctrl_num + '0'));
		return stat;
	}
	lurb_priv = ed->purb;
	dl_transfer_length(ohci, td_list);
	lurb_priv->td_cnt++;
	/* error code of transfer */
	cc = TD_CC_GET(tdINFO);
	if(cc)
	{
		ALERT(("OHCI usb-%s-%c error: %s (%x)", ohci->slot_name, (char)ohci->ctrl_num + '0', cc_to_string[cc], cc));
		stat = cc_to_error[cc];
	}
	/* see if this done list makes for all TD's of current URB,
	 * and we can return them to the controller
	 */
	if(lurb_priv->td_cnt == lurb_priv->length)
		finish_urb(ohci, lurb_priv, ed->state);
#ifdef DEV_DEBUG
	DEBUG(("dl_done_list: processing TD %x, len %x", lurb_priv->td_cnt, lurb_priv->length));
#endif
	if(ed->state != ED_NEW && (!usb_pipeint(lurb_priv->pipe)))
	{
		edHeadP = cpu2le32(ed->hwHeadP) & ~0xf;
		edTailP = cpu2le32(ed->hwTailP);
		if((edHeadP == edTailP) && (ed->state == ED_OPER))
			ep_unlink(ohci, ed);
	}
	if(cc && (ed->type == PIPE_INTERRUPT))
		ep_unlink(ohci, ed);
	return stat;
}

static int dl_done_list(ohci_t *ohci)
{
	int stat = 0;
	td_t *td_list = dl_reverse_done_list(ohci);
	while(td_list)
	{
		td_t *td_next = td_list->next_dl_td;
		stat = takeback_td(ohci, td_list);
		td_list = td_next;
	}
	return stat;
}

/*-------------------------------------------------------------------------*
 * Virtual Root Hub
 *-------------------------------------------------------------------------*/

/* Device descriptor */
static unsigned char root_hub_dev_des[] =
{
	0x12,	    /*	unsigned char  bLength; */
	0x01,	    /*	unsigned char  bDescriptorType; Device */
	0x10,	    /*	unsigned short bcdUSB; v1.1 */
	0x01,
	0x09,	    /*	unsigned char  bDeviceClass; HUB_CLASSCODE */
	0x00,	    /*	unsigned char  bDeviceSubClass; */
	0x00,	    /*	unsigned char  bDeviceProtocol; */
	0x08,	    /*	unsigned char  bMaxPacketSize0; 8 Bytes */
	0x00,	    /*	unsigned short idVendor; */
	0x00,
	0x00,	    /*	unsigned short idProduct; */
	0x00,
	0x00,	    /*	unsigned short bcdDevice; */
	0x00,
	0x00,	    /*	unsigned char  iManufacturer; */
	0x01,	    /*	unsigned char  iProduct; */
	0x00,	    /*	unsigned char  iSerialNumber; */
	0x01	    /*	unsigned char  bNumConfigurations; */
};

/* Configuration descriptor */
static unsigned char root_hub_config_des[] =
{
	0x09,	    /*	unsigned char  bLength; */
	0x02,	    /*	unsigned char  bDescriptorType; Configuration */
	0x19,	    /*	unsigned short wTotalLength; */
	0x00,
	0x01,	    /*	unsigned char  bNumInterfaces; */
	0x01,	    /*	unsigned char  bConfigurationValue; */
	0x00,	    /*	unsigned char  iConfiguration; */
	0x40,	    /*	unsigned char  bmAttributes; */
	0x00,	    /*	unsigned char  MaxPower; */

	/* interface */
	0x09,	    /*	unsigned char  if_bLength; */
	0x04,	    /*	unsigned char  if_bDescriptorType; Interface */
	0x00,	    /*	unsigned char  if_bInterfaceNumber; */
	0x00,	    /*	unsigned char  if_bAlternateSetting; */
	0x01,	    /*	unsigned char  if_bNumEndpoints; */
	0x09,	    /*	unsigned char  if_bInterfaceClass; HUB_CLASSCODE */
	0x00,	    /*	unsigned char  if_bInterfaceSubClass; */
	0x00,	    /*	unsigned char  if_bInterfaceProtocol; */
	0x00,	    /*	unsigned char  if_iInterface; */

	/* endpoint */
	0x07,	    /*	unsigned char  ep_bLength; */
	0x05,	    /*	unsigned char  ep_bDescriptorType; Endpoint */
	0x81,	    /*	unsigned char  ep_bEndpointAddress; IN Endpoint 1 */
	0x03,	    /*	unsigned char  ep_bmAttributes; Interrupt */
	0x02,	    /*	unsigned short ep_wMaxPacketSize; ((MAX_ROOT_PORTS + 1) / 8 */
	0x00,
	0xff	    /*	unsigned char  ep_bInterval; 255 ms */
};

static unsigned char root_hub_str_index0[] =
{
	0x04,	/*  unsigned char  bLength; */
	0x03,	/*  unsigned char  bDescriptorType; String-descriptor */
	0x09,	/*  unsigned char  lang ID */
	0x04,	/*  unsigned char  lang ID */
};

static unsigned char root_hub_str_index1[] =
{
	28,	/*  unsigned char  bLength; */
	0x03,	/*  unsigned char  bDescriptorType; String-descriptor */
	'O', 0, 'H', 0, 'C', 0, 'I', 0, ' ', 0,
	'R', 0, 'o', 0, 'o', 0, 't', 0, ' ', 0,
	'H', 0, 'u', 0, 'b', 0,
};

/*-------------------------------------------------------------------------*/

#define OK(x)			len = (x); break
#ifdef DEV_DEBUG
#define WR_RH_STAT(x)		{ DEBUG(("WR:status %#8lx", (unsigned long)(x))); writel((x), &ohci->regs->roothub.status); }
#define WR_RH_PORTSTAT(x)	{ DEBUG(("WR:portstatus[%d] %#8lx", wIndex-1, (unsigned long)(x))); writel((x), &ohci->regs->roothub.portstatus[wIndex-1]); }
#else
#define WR_RH_STAT(x)		{ writel((x), &ohci->regs->roothub.status); }
#define WR_RH_PORTSTAT(x)	{ writel((x), &ohci->regs->roothub.portstatus[wIndex-1]); }
#endif
#define RD_RH_STAT		roothub_status(ohci)
#define RD_RH_PORTSTAT		roothub_portstatus(ohci, wIndex-1)

int rh_check_port_status(ohci_t *controller)
{
	unsigned long temp, ndp, i;
	int res = -1;
	temp = roothub_a(controller);
	ndp = controller->ndp;
	for(i = 0; i < ndp; i++)
	{
		temp = roothub_portstatus(controller, i);
		if(((temp & (RH_PS_PESC | RH_PS_CSC)) == (RH_PS_PESC | RH_PS_CSC)) && ((temp & RH_PS_CCS) == 0))
		{
			res = i;
			break;
		}
		if((temp & RH_PS_CSC) && (temp & RH_PS_CCS))
		{
			res = i;
			break;
		}
	}
	return res;
}

/* request to virtual root hub */
static int ohci_submit_rh_msg(ohci_t *ohci, struct usb_device *dev, unsigned long pipe,
 void *buffer, int transfer_len, struct devrequest *cmd)
{
	void *data = buffer;
	int leni = transfer_len;
	int len = 0;
	int stat = 0;
	unsigned long datab[4];
	unsigned char *data_buf = (unsigned char *)datab;
	unsigned short bmRType_bReq;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
#ifdef DEV_DEBUG
	pkt_print(ohci, NULL, dev, pipe, buffer, transfer_len, cmd, "SUB(rh)", usb_pipein(pipe));
#else
	if(ohci->irq)
		mdelay(1);
#endif
	if(usb_pipeint(pipe))
	{
		DEBUG(("Root-Hub submit IRQ: NOT implemented"));
		return 0;
	}
	bmRType_bReq  = cmd->requesttype | (cmd->request << 8);
	wValue	      = le2cpu16(cmd->value);
	wIndex	      = le2cpu16(cmd->index);
	wLength	      = le2cpu16(cmd->length);
	DEBUG(("Root-Hub: adr: %2lx cmd(%1x): %04x %04x %04x %04x", dev->devnum, 8, bmRType_bReq, wValue, wIndex, wLength));
	/* Request Destination:
	       without flags: Device,
	       RH_INTERFACE: Interface,
	       RH_ENDPOINT: Endpoint,
	       RH_CLASS means HUB here,
	       RH_OTHER | RH_CLASS almost ever means HUB_PORT here
	*/
#ifdef TOSONLY
	unsigned long oldmode = (Super(1L) ? 0L: Super(0L));
#endif
	switch(bmRType_bReq)
	{
		case RH_GET_STATUS:
			*(unsigned short *)data_buf = cpu2le16(1);
			OK(2);
		case RH_GET_STATUS | RH_INTERFACE:
			*(unsigned short *)data_buf = cpu2le16(0);
			OK(2);
		case RH_GET_STATUS | RH_ENDPOINT:
			*(unsigned short *)data_buf = cpu2le16(0);
			OK(2);
		case RH_GET_STATUS | RH_CLASS:
			*(unsigned long *)data_buf = cpu2le32(RD_RH_STAT & ~(RH_HS_CRWE | RH_HS_DRWE));
			OK(4);
		case RH_GET_STATUS | RH_OTHER | RH_CLASS:
			*(unsigned long *)data_buf = cpu2le32(RD_RH_PORTSTAT);
			OK(4);
		case RH_CLEAR_FEATURE | RH_ENDPOINT:
			switch(wValue)
			{
				case (RH_ENDPOINT_STALL): OK(0);
			}
			break;
		case RH_CLEAR_FEATURE | RH_CLASS:
			switch(wValue)
			{
				case RH_C_HUB_LOCAL_POWER: OK(0);
				case (RH_C_HUB_OVER_CURRENT): WR_RH_STAT(RH_HS_OCIC); OK(0);
			}
			break;
		case RH_CLEAR_FEATURE | RH_OTHER | RH_CLASS:
			switch(wValue)
			{
				case (RH_PORT_ENABLE):         WR_RH_PORTSTAT(RH_PS_CCS);  OK(0);
				case (RH_PORT_SUSPEND):        WR_RH_PORTSTAT(RH_PS_POCI); OK(0);
				case (RH_PORT_POWER):          WR_RH_PORTSTAT(RH_PS_LSDA); OK(0);
				case (RH_C_PORT_CONNECTION):   WR_RH_PORTSTAT(RH_PS_CSC);  OK(0);
				case (RH_C_PORT_ENABLE):       WR_RH_PORTSTAT(RH_PS_PESC); OK(0);
				case (RH_C_PORT_SUSPEND):      WR_RH_PORTSTAT(RH_PS_PSSC); OK(0);
				case (RH_C_PORT_OVER_CURRENT): WR_RH_PORTSTAT(RH_PS_OCIC); OK(0);
				case (RH_C_PORT_RESET):        WR_RH_PORTSTAT(RH_PS_PRSC); OK(0);
			}
			break;
		case RH_SET_FEATURE | RH_OTHER | RH_CLASS:
			switch(wValue)
			{
				case (RH_PORT_SUSPEND):
					WR_RH_PORTSTAT(RH_PS_PSS);
					OK(0);
				case (RH_PORT_RESET):
					if(RD_RH_PORTSTAT & RH_PS_CCS)
						WR_RH_PORTSTAT(RH_PS_PRS);
					OK(0);
				case (RH_PORT_POWER):
					WR_RH_PORTSTAT(RH_PS_PPS);
					mdelay(100);
					OK(0);
				case (RH_PORT_ENABLE):
					if(RD_RH_PORTSTAT & RH_PS_CCS)
						WR_RH_PORTSTAT(RH_PS_PES);
					OK(0);
			}
			break;
		case RH_SET_ADDRESS:
			ohci->rh.devnum = wValue;
			OK(0);
		case RH_GET_DESCRIPTOR:
			switch((wValue & 0xff00) >> 8)
			{
				case(0x01): /* device descriptor */
					len = min_t(unsigned int, leni, min_t(unsigned int, sizeof(root_hub_dev_des), wLength));
					data_buf = root_hub_dev_des;
					OK(len);
				case(0x02): /* configuration descriptor */
					len = min_t(unsigned int, leni, min_t(unsigned int, sizeof(root_hub_config_des), wLength));
					data_buf = root_hub_config_des;
					OK(len);
				case(0x03): /* string descriptors */
					if(wValue == 0x0300)
					{
						len = min_t(unsigned int, leni, min_t(unsigned int, sizeof(root_hub_str_index0), wLength));
						data_buf = root_hub_str_index0;
						OK(len);
					}
					if(wValue == 0x0301)
					{
						len = min_t(unsigned int, leni, min_t(unsigned int, sizeof(root_hub_str_index1), wLength));
						data_buf = root_hub_str_index1;
						OK(len);
					}
				default:
					stat = USB_ST_STALLED;
			}
			break;
		/* Hub class-specific descriptor is constructed dynamically */
		case RH_GET_DESCRIPTOR | RH_CLASS:
		{
			unsigned long temp = roothub_a(ohci);
			data_buf[0] = 9;
			data_buf[1] = 0x29;
			data_buf[2] = (unsigned char)ohci->ndp;
			data_buf[3] = 0;
			if(temp & RH_A_PSM)
				data_buf[3] |= 0x1;
			if(temp & RH_A_NOCP)
				data_buf[3] |= 0x10;
			else if(temp & RH_A_OCPM)
				data_buf[3] |= 0x8;
			datab[1] = 0;
			data_buf[5] = (temp & RH_A_POTPGT) >> 24;
			temp = roothub_b(ohci);
			data_buf[7] = temp & RH_B_DR;
			if(data_buf[2] < 7)
				data_buf[8] = 0xff;
			else
			{
				data_buf[0] += 2;
				data_buf[8] = (temp & RH_B_DR) >> 8;
				data_buf[10] = data_buf[9] = 0xff;
			}
			len = min_t(unsigned int, leni, min_t(unsigned int, data_buf[0], wLength));
			OK(len);
		}
		case RH_GET_CONFIGURATION: *(unsigned char *) data_buf = 0x01; OK(1);
		case RH_SET_CONFIGURATION: WR_RH_STAT(0x10000); OK(0);
		default:
			DEBUG(("unsupported root hub command"));
			stat = USB_ST_STALLED;
	}
#ifdef DEV_DEBUG
	ohci_dump_roothub(ohci, 1);
#else
	if(ohci->irq)
		mdelay(1);
#endif
	len = min_t(int, len, leni);
	if(data != data_buf)
		memcpy(data, data_buf, len);
	dev->act_len = len;
	dev->status = stat;
#ifdef DEV_DEBUG
	pkt_print(ohci, NULL, dev, pipe, buffer, transfer_len, cmd, "RET(rh)", 0);
#else
	if(ohci->irq)
		mdelay(1);
#endif

#ifdef TOSONLY
	if (oldmode) SuperToUser(oldmode);
#endif

	return stat;
}

/*-------------------------------------------------------------------------*/

/* Cancel any TDs queued on this ED. Used on interrupt-transfer timeout: the ED
 * stays on the periodic list forever, and hwCBP/hwBE still point at the
 * bounce buffer we're about to free — without emptying the queue first, the
 * HC corrupts memory when data eventually arrives.
 */
static void ed_cancel_queued_tds(ed_t *ed)
{
	unsigned long carry;

	ed->hwINFO |= cpu2le32(OHCI_ED_SKIP);
	flush_dcache_range((void *)ed, sizeof(ed_t));

	/* Give the HC a frame to observe SKIP before we touch hwHeadP. */
#ifdef TOSONLY
	ohci_mdelay_1_ikbd();
#else
	mdelay(1);
#endif

	/* Empty the queue; preserve the toggle carry (bit 1 of hwHeadP). */
	carry = ed->hwHeadP & cpu2le32(0x2);
	ed->hwHeadP = ed->hwTailP | carry;

	ed->hwINFO &= ~cpu2le32(OHCI_ED_SKIP);
	flush_dcache_range((void *)ed, sizeof(ed_t));
}

#ifdef TOSONLY
/* Synthesize an IKBD interrupt (jumps to *0x118)
 * Implemented in ohci.S.
 */
extern void fake_ikbd_int(void);

/* Read bit 7 of the IKBD ACIA control register — set when the ACIA
 * has data waiting.
 */
static inline int ikbd_int_pending(void)
{
	return *(volatile unsigned char *)0xFFFFFC00UL & 0x80;
}

/* Read the current IPL from SR. Supervisor-only instruction; safe here
 * because submit_common_msg enters supervisor mode via Super() first.
 */
static inline unsigned int get_int_lvl(void)
{
	unsigned int sr;
	__asm("move %%sr,%0" : "=d"(sr));
	return (sr >> 8) & 7;
}

/* 1 ms wait that services the IKBD ACIA every µs while spinning. Used
 * in place of plain mdelay(1), so legacy IKBD mouse bytes don't overrun
 * the 1-byte ACIA FIFO. Only drains when we're at IPL >= 6 (i.e., called
 * from the timer C ISR chain); otherwise falls through to plain
 * mdelay(1).
 */
static void ohci_mdelay_1_ikbd(void)
{
	if (get_int_lvl() >= 6) {
		int i;
		for (i = 0; i < 1000; i++) {
			int drain_limit = 20;
			while (drain_limit-- && ikbd_int_pending())
				fake_ikbd_int();
			udelay(1);
		}
	} else {
		mdelay(1);
	}
}

#endif /* TOSONLY */

/* common code for handling submit messages - used for all but root hub accesses. */
static int submit_common_msg(ohci_t *ohci, struct usb_device *dev, unsigned long pipe, void *buffer,
 int transfer_len, struct devrequest *setup, int interval)
{
	int stat = 0;
	int maxsize = usb_maxpacket(dev, pipe);
	int timeout;
	int timed_out = 0;
	int ret = 0;
	void *orig_buffer = buffer;
	urb_priv_t *urb;
#ifdef TOSONLY
	/* Enter supervisor mode for the whole submission. unlock_usb's
	 * cpushl is a privileged instruction on ColdFire; keeping the
	 * entire critical section supervisor-side means every early-exit
	 * unlock is safe too.
	 */
	unsigned long oldmode = (Super(1L) ? 0L : Super(0L));
#endif

	if (!lock_usb(&ohci->job_in_progress))
	{
		DEBUG(("submit_common_msg: another USB job in progress"));
		dev->status = USB_ST_BUF_ERR;
#ifdef TOSONLY
		if (oldmode) SuperToUser(oldmode);
#endif
		return -1;
	}

#ifdef TOSONLY
	/* Preallocated URB slot; kmalloc is GEMDOS Malloc (TRAP #1) and is
	 * unsafe from the timer C ISR path. job_in_progress serializes us. */
	urb = &ohci->tos_urb;
#else
	urb = (urb_priv_t *)kmalloc(sizeof(urb_priv_t));
	if(urb == NULL)
	{
		ALERT(("submit_common_msg malloc failed"));
		unlock_usb(&ohci->job_in_progress);
		return -1;
	}
#endif
	memset(urb, 0, sizeof(urb_priv_t));
	urb->dev = dev;
	urb->pipe = pipe;
	urb->transfer_buffer = buffer;
	urb->transfer_buffer_length = transfer_len;
	urb->interval = interval;
	/* device pulled? Shortcut the action. */
	if(ohci->devgone == dev)
	{
		dev->status = USB_ST_CRC_ERR;
		kfree(urb);
		unlock_usb(&ohci->job_in_progress);
#ifdef TOSONLY
		if (oldmode) SuperToUser(oldmode);
#endif
		return 0;
	}
#ifdef DEV_DEBUG
	urb->actual_length = 0;
	pkt_print(ohci, urb, dev, pipe, buffer, transfer_len, setup, "SUB", usb_pipein(pipe));
#else
	if(ohci->irq)
#ifdef TOSONLY
		ohci_mdelay_1_ikbd();
#else
		mdelay(1);
#endif
#endif
	if(!maxsize)
	{
		ALERT(("submit_common_message: pipesize for pipe %lx is zero", pipe));
		kfree(urb);
		unlock_usb(&ohci->job_in_progress);
#ifdef TOSONLY
		if (oldmode) SuperToUser(oldmode);
#endif
		return -1;
	}
	/* Allocate a temporary buffer when needed so that the transfer data are
	 * cache-line aligned and padded to whole cache lines.
	 *
	 * This ensures the transfer data occupy cache lines exclusively owned by
	 * this buffer, so the pre-DMA cache flush and post-DMA cache invalidate
	 * operations affect only those cache lines.
	 */
	{
		long req_pad = (setup != NULL) ? M68K_CACHE_LINE_SIZE : 0;
		long buf_pad = 0;
		int need_buf_bounce = (buffer != NULL && transfer_len > 0
		                       && (((unsigned long)buffer & (M68K_CACHE_LINE_SIZE - 1))
		                           || ((unsigned long)transfer_len & (M68K_CACHE_LINE_SIZE - 1))));
		if (need_buf_bounce)
			buf_pad = ((long)transfer_len + (M68K_CACHE_LINE_SIZE - 1))
			          & ~((long)M68K_CACHE_LINE_SIZE - 1);
		if (req_pad || buf_pad)
		{
			char *base;
			urb->bounce_buf = bounce_alloc(ohci, req_pad + buf_pad + M68K_CACHE_LINE_SIZE);
			if (urb->bounce_buf == NULL)
			{
				ALERT(("submit_common_msg: bounce alloc failed"));
				kfree(urb);
				unlock_usb(&ohci->job_in_progress);
#ifdef TOSONLY
				if (oldmode) SuperToUser(oldmode);
#endif
				return -1;
			}
			base = (char *)(((unsigned long)urb->bounce_buf
			                 + (M68K_CACHE_LINE_SIZE - 1))
			                & ~((unsigned long)M68K_CACHE_LINE_SIZE - 1));
			if (req_pad)
			{
				memcpy(base, setup, sizeof(*setup));
				setup = (struct devrequest *)base;
				base += req_pad;
			}
			if (buf_pad)
			{
				if (!usb_pipein(pipe))
					memcpy(base, orig_buffer, transfer_len);
				buffer = base;
				urb->transfer_buffer = base;
			}
		}
	}
	if(sohci_submit_job(ohci, urb, setup) < 0)
	{
		ALERT(("sohci_submit_job failed"));
		/* sohci_submit_job's failure paths call urb_free_priv,
		 * which also frees urb->bounce_buf.
		 */
		ret = -1;
		goto out;
	}
/* allow more time for a BULK device to react - some are slow */
#define BULK_TO	5000	/* timeout in milliseconds */
	if(usb_pipebulk(pipe))
		timeout = BULK_TO;
	else
	/* Interrupt pipes: the poll loop below uses mdelay(1), so this timeout is
	 * in milliseconds of busy waiting. Since HID poll callers (mouse, keyboard)
	 * run from the etv_timer system vector chain, a long timeout would stall the
	 * timer chain on every idle NAK.
	 */
		timeout = 5;
	/* NOTE: we're not interrupt driven and
	 * the EDs and TDs are not on DMA-coherent memory.
	 */
	while(ohci->irq)
	{
		invalidate_dcache_range(ohci->hcca, sizeof(struct ohci_hcca));
		stat = hc_interrupt(ohci);
		if(stat < 0)
		{
			stat = USB_ST_CRC_ERR;
			break;
		}
		/* 0xff is returned for an SF-interrupt */
		if((stat >= 0) && (stat != 0xff) && (urb->finished))
			break;
		if(--timeout)
		{
#ifdef TOSONLY
		ohci_mdelay_1_ikbd();
#else
		mdelay(1);
#endif
		}
		else
		{
			DEBUG(("OHCI usb-%s-%c CTL:TIMEOUT", ohci->slot_name, (char)ohci->ctrl_num + '0'));
			DEBUG(("submit_common_msg: TO status %x\r\n", stat));
			urb->finished = 1;
			stat = USB_ST_CRC_ERR;
			timed_out = 1;
			break;
		}
	}
	/* On timeout for an interrupt pipe, empty the ED's TD queue before we
	 * free the bounce buffer. The interrupt ED stays on the periodic list
	 * and the HC will keep polling it; if hwCBP still points at the freed
	 * buffer, the HC corrupts memory when data eventually arrives.
	 */
	if(timed_out && usb_pipeint(pipe) && urb->ed)
		ed_cancel_queued_tds(urb->ed);
	if(usb_pipein(pipe) && buffer && transfer_len > 0)
	{
		invalidate_dcache_range(buffer, transfer_len);
		if (buffer != orig_buffer)
			memcpy(orig_buffer, buffer, transfer_len);
	}
	/* Model A (interrupt pipe with interval > 0)leaves the URB alive across
	 * auto-requeues, so the buffer stays valid for the HC's continuing DMA
	 * into it.
	 */
	dev->status = stat;
	dev->act_len = transfer_len;
#ifdef DEV_DEBUG
	pkt_print(ohci, urb, dev, pipe, buffer, transfer_len, setup, "RET(ctlr)", usb_pipeout(pipe));
#else
	if(ohci->irq)
#ifdef TOSONLY
		ohci_mdelay_1_ikbd();
#else
		mdelay(1);
#endif
#endif
	/* Free TDs in urb_priv. Under Model A (interrupt pipe with
	 * interval > 0) sohci_return_job auto-requeued the TD and still
	 * holds a live reference to the URB, so we must not free it
	 * unless we timed out. Under Model B (interrupt pipe with
	 * interval == 0) and for control/bulk, the URB is done here.
	 */
	if(!usb_pipeint(pipe) || timed_out || urb->interval == 0)
		urb_free_priv(urb);
out:
	/* NOTE: on Model A interrupt pipes (interval > 0, no timeout) the
	 * URB is not freed above and the HC will keep DMAing into the
	 * bounce buffer. Releasing the lock here lets a subsequent submit
	 * hand out the same buffer. Our TOSONLY device drivers use Model B,
	 * so this is currently latent.
	 */
	unlock_usb(&ohci->job_in_progress);
#ifdef TOSONLY
	if (oldmode) SuperToUser(oldmode);
#endif
	return ret;
}

/* FreeMiNT USB API submit functions */

long submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
 long transfer_len, long flags, unsigned long timeout)
{
	ohci_t *ohci = (ohci_t *)dev->controller->ucd_priv;
	DEBUG(("submit_bulk_msg dev 0x%p ohci 0x%p buffer 0x%p len %ld", dev, ohci, buffer, transfer_len));
	return submit_common_msg(ohci, dev, pipe, buffer, (int)transfer_len, NULL, 0);
}

long submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
 long transfer_len, struct devrequest *setup)
{
	ohci_t *ohci = (ohci_t *)dev->controller->ucd_priv;
	int maxsize = usb_maxpacket(dev, pipe);
	DEBUG(("submit_control_msg dev 0x%p ohci 0x%p", dev, ohci));
#ifdef DEV_DEBUG
	pkt_print(ohci, NULL, dev, pipe, buffer, (int)transfer_len, setup, "SUB", usb_pipein(pipe));
#else
	if(ohci->irq)
		mdelay(1);
#endif
	if(!maxsize)
	{
		ALERT(("submit_control_message: pipesize for pipe %lx is zero", pipe));
		return -1;
	}
	if(((pipe >> 8) & 0x7f) == ohci->rh.devnum)
	{
		ohci->rh.dev = dev;
		return ohci_submit_rh_msg(ohci, dev, pipe, buffer, (int)transfer_len, setup);
	}
	return submit_common_msg(ohci, dev, pipe, buffer, (int)transfer_len, setup, 0);
}

long submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
 long transfer_len, long interval)
{
	ohci_t *ohci = (ohci_t *)dev->controller->ucd_priv;
	DEBUG(("submit_int_msg dev 0x%p ohci 0x%p buffer 0x%p len %ld", dev, ohci, buffer, transfer_len));
	return submit_common_msg(ohci, dev, pipe, buffer, (int)transfer_len, NULL, (int)interval);
}

/*-------------------------------------------------------------------------*
 * HC functions
 *-------------------------------------------------------------------------*/

/* reset the HC and BUS */
static int hc_reset(ohci_t *ohci)
{
	int timeout = 30;
	int smm_timeout = 50; /* 0.5 sec */
	DEBUG(("%s\r\n", __FUNCTION__));
	/* ISP1561 multi-function: reset the EHCI companion via PCI scan */
	if((ohci->ent->vendor == PCI_VENDOR_ID_PHILIPS)
	 && (ohci->ent->device == PCI_DEVICE_ID_PHILIPS_ISP1561))
	{
		short index = 0;
		long handle;
		do
		{
			handle = Find_pci_device(0x0000FFFFL, index++);
			if(handle >= 0)
			{
				unsigned long id = 0;
				long error = Read_config_longword(handle, PCIIDR, &id);
				if((error >= 0) && (PCI_VENDOR_ID_PHILIPS == (id & 0xFFFF))
				 && (PCI_DEVICE_ID_PHILIPS_ISP1561_2 == (id >> 16)))
				{
					int ehci_timeout = 1000;
					unsigned long usb_base_addr = 0xFFFFFFFF;
					PCI_RSC_DESC *pci_rsc_desc;
					pci_rsc_desc = (PCI_RSC_DESC *)Get_resource(handle);
					if((long)pci_rsc_desc >= 0)
					{
						unsigned short flags;
						do
						{
							if(!(pci_rsc_desc->flags & FLG_IO))
							{
								if(usb_base_addr == 0xFFFFFFFF)
								{
									unsigned long base = pci_rsc_desc->offset + pci_rsc_desc->start;
									usb_base_addr = pci_rsc_desc->start;
									writel(readl(base + EHCI_USBCMD_OFF) | EHCI_USBCMD_HCRESET, base + EHCI_USBCMD_OFF);
									while(readl(base + EHCI_USBCMD_OFF) & EHCI_USBCMD_HCRESET)
									{
										if(ehci_timeout-- <= 0)
										{
											ALERT(("USB RootHub reset timed out!\r\n"));
											break;
										}
										udelay(1);
									}
								}
							}
							flags = pci_rsc_desc->flags;
							pci_rsc_desc = (PCI_RSC_DESC *)((unsigned long)pci_rsc_desc->next + (unsigned long)pci_rsc_desc);
						}
						while(!(flags & FLG_LAST));
					}
				}
			}
		}
		while(handle >= 0);
	}
	/* NEC uPD720101: EXT2 register (config offset E4h) selects the 48 MHz
	 * oscillator. Only function 0 has E4h; function 1's register map ends at
	 * 47h. The clock is chip-wide so one write from function 0 covers both
	 * OHCI ports.
	 */
	if ((machine == machine_firebee)
	 && (ohci->ent->vendor == PCI_VENDOR_ID_NEC)
	 && (ohci->ctrl_num == 0))
		Write_config_longword(ohci->handle, 0xE4, 0x20UL);
	if(readl(&ohci->regs->control) & OHCI_CTRL_IR)
	{
		writel(OHCI_OCR, &ohci->regs->cmdstatus);
		DEBUG(("USB HC TakeOver from SMM"));
		while(readl(&ohci->regs->control) & OHCI_CTRL_IR)
		{
			mdelay(10);
			if(--smm_timeout == 0)
			{
				ALERT(("USB HC TakeOver failed!"));
				return -1;
			}
		}
	}
	writel(OHCI_INTR_MIE, &ohci->regs->intrdisable);
	DEBUG(("USB OHCI HC reset_hc usb-%s-%c: ctrl = 0x%08lx", ohci->slot_name, (char)ohci->ctrl_num + '0', readl(&ohci->regs->control)));
	ohci->hc_control = 0;
	writel(ohci->hc_control, &ohci->regs->control);
	mdelay(50);
	writel(OHCI_HCR, &ohci->regs->cmdstatus);
	while((readl(&ohci->regs->cmdstatus) & OHCI_HCR) != 0)
	{
		if(--timeout == 0)
		{
			ALERT(("USB HC reset timed out!"));
			return -1;
		}
		udelay(1);
	}
	return 0;
}

/*-------------------------------------------------------------------------*/

/* Start an OHCI controller, set the BUS operational
 * enable interrupts
 * connect the virtual root hub
 */
static int hc_start(ohci_t *ohci)
{
	unsigned long mask;
	unsigned long fminterval;
	ohci->disabled = 1;
	writel(0, &ohci->regs->ed_controlhead);
	writel(0, &ohci->regs->ed_bulkhead);
	writel((unsigned long)ohci->hcca - ohci->dma_offset, &ohci->regs->hcca);
	fminterval = 0x2edf;
	writel((fminterval * 9) / 10, &ohci->regs->periodicstart);
	fminterval |= ((((fminterval - 210) * 6) / 7) << 16);
	writel(fminterval, &ohci->regs->fminterval);
	writel(0x628, &ohci->regs->lsthresh);
	ohci->hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
	ohci->disabled = 0;
	writel(ohci->hc_control, &ohci->regs->control);
	mask = (OHCI_INTR_SO | OHCI_INTR_WDH | OHCI_INTR_SF | OHCI_INTR_RD |
			OHCI_INTR_UE | OHCI_INTR_FNO | OHCI_INTR_RHSC | OHCI_INTR_OC | OHCI_INTR_MIE);
	writel(mask, &ohci->regs->intrdisable);
	mask &= ~OHCI_INTR_MIE;
	writel(mask, &ohci->regs->intrstatus);
	mask = OHCI_INTR_RHSC | OHCI_INTR_UE | OHCI_INTR_SO;
	writel(mask, &ohci->regs->intrenable);
#if !defined(OHCI_POLL) && !defined(TOSONLY)
	writel(OHCI_INTR_MIE, &ohci->regs->intrenable);
#endif
	ohci->ndp = roothub_a(ohci);
#ifdef OHCI_USE_NPS
	writel((ohci->ndp | RH_A_NPS) & ~RH_A_PSM, &ohci->regs->roothub.a);
	writel(RH_HS_LPSC, &ohci->regs->roothub.status);
#endif
	mdelay((ohci->ndp >> 23) & 0x1fe);
	ohci->ndp &= RH_A_NDP;
	ohci->rh.devnum = 0;
	return 0;
}

/*-------------------------------------------------------------------------*/

static int hc_interrupt(ohci_t *ohci)
{
	struct ohci_regs *regs = ohci->regs;
	int ints, stat = -1;
	if((ohci->hcca->done_head != 0) && !(cpu2le32(ohci->hcca->done_head) & 0x01))
		ints = OHCI_INTR_WDH;
	else
	{
		ints = readl(&regs->intrstatus);
		if(ints == ~(unsigned long)0)
		{
			ohci->disabled++;
			ALERT(("OHCI usb-%s-%c device removed!", ohci->slot_name, (char)ohci->ctrl_num + '0'));
			return -1;
		}
		else
		{
			ints &= readl(&regs->intrenable);
			if(ints == 0)
				return 0xff;
		}
	}
#ifdef DEV_DEBUG
	DEBUG(("Interrupt: 0x%x frame: 0x%x bus: %d", ints, le2cpu16(ohci->hcca->frame_no), ohci->ctrl_num));
#endif
	if(ints & OHCI_INTR_RHSC)
	{
		stat = 0xff;
	}
	if(ints & OHCI_INTR_UE)
	{
		unsigned short status __attribute__((unused)) = Fast_read_config_word(ohci->handle, PCISR);
		ALERT(("OHCI Unrecoverable Error, controller usb-%s-%c disabled\r\n(SR:0x%04X%s%s%s%s%s%s)",
		 ohci->slot_name, (char)ohci->ctrl_num + '0', status & 0xFFFF,
		 status & 0x8000 ? ", Parity error" : "",
		 status & 0x4000 ? ", Signaled system error" : "",
		 status & 0x2000 ? ", Received master abort" : "",
		 status & 0x1000 ? ", Received target abort" : "",
		 status & 0x800 ? ", Signaled target abort" : "",
		 status & 0x100 ? ", Data parity error" : ""));
		ohci->disabled++;
#ifdef DEV_DEBUG
		ohci_dump(ohci, 1);
#else
		if(ohci->irq)
#ifdef TOSONLY
			ohci_mdelay_1_ikbd();
#else
			mdelay(1);
#endif
#endif
		ohci->hc_control = 0;
		writel(ohci->hc_control, &ohci->regs->control);
		return -1;
	}
	if(ints & OHCI_INTR_WDH)
	{
		if(ohci->irq)
#ifdef TOSONLY
			ohci_mdelay_1_ikbd();
#else
			mdelay(1);
#endif
		stat = dl_done_list(ohci);
	}
	if(ints & OHCI_INTR_SO)
	{
		DEBUG(("USB Schedule overrun\r\n"));
		writel(OHCI_INTR_SO, &regs->intrenable);
		stat = -1;
	}
	/* FIXME: this assumes SOF (1/ms) interrupts don't get lost... */
	if(ints & OHCI_INTR_SF)
	{
		unsigned int frame = cpu2le16(ohci->hcca->frame_no) & 1;
		if(ohci->irq)
#ifdef TOSONLY
			ohci_mdelay_1_ikbd();
#else
			mdelay(1);
#endif
		writel(OHCI_INTR_SF, &regs->intrdisable);
		if(ohci->ed_rm_list[frame] != NULL)
			writel(OHCI_INTR_SF, &regs->intrenable);
		stat = 0xff;
	}
	writel(ints, &regs->intrstatus);
	return stat;
}

#ifndef TOSONLY
static struct ucdif *ohci_pending_wakeup = NULL;

static void _cdecl ohci_int_handle_tophalf(PROC *process, long arg)
{
	if(ohci_pending_wakeup)
		usb_rh_wakeup(ohci_pending_wakeup);
	ohci_pending_wakeup = NULL;
}

long ohci_interrupt_handle(long param, long biosparam)
{
	ohci_t *ohci = (ohci_t *)param;
	int ints;
	static int irq_count = 0;

	if (irq_count < 10)
	{
		irq_count++;
	}

	invalidate_dcache_range(ohci->hcca, sizeof(struct ohci_hcca));

	ints = readl(&ohci->regs->intrstatus);
	if(ints == ~(unsigned long)0)
		return biosparam; /* device removed */

	ints &= readl(&ohci->regs->intrenable);
	if(ints == 0)
		return biosparam; /* not our interrupt */

	writel(ints, &ohci->regs->intrstatus);

	if(ints & OHCI_INTR_RHSC)
	{
		if(rh_check_port_status(ohci) >= 0)
		{
			ohci_pending_wakeup = ohci->controller_if;
			addroottimeout(0L, ohci_int_handle_tophalf, 0x1);
		}
	}

	return 1; /* PCI_BIOS specification: interrupt was ours */
}
#endif /* TOSONLY */

/*-------------------------------------------------------------------------*/

/* De-allocate all resources.. */
static void hc_release_ohci(ohci_t *ohci)
{
	DEBUG(("USB HC release OHCI usb-%s-%c", ohci->slot_name, (char)ohci->ctrl_num + '0'));
	if(!ohci->disabled)
		hc_reset(ohci);
}

static void hc_free_buffers(ohci_t *ohci)
{
	if(ohci->td_unaligned != NULL)
	{
		kfree(ohci->td_unaligned);
		ohci->td_unaligned = NULL;
	}
	if(ohci->ohci_dev_unaligned != NULL)
	{
		kfree(ohci->ohci_dev_unaligned);
		ohci->ohci_dev_unaligned = NULL;
	}
	if(ohci->hcca_unaligned != NULL)
	{
		kfree(ohci->hcca_unaligned);
		ohci->hcca_unaligned = NULL;
	}
}

/*-------------------------------------------------------------------------*/

/* low level initalisation routine, called from usb.c */
long usb_lowlevel_init(void *ucd_priv)
{
	unsigned long usb_base_addr = 0xFFFFFFFF;
	PCI_RSC_DESC *pci_rsc_desc;
	long ret = 0;
#ifdef TOSONLY
	unsigned long oldmode = 0;
#endif

	long saved_handle;
	const struct pci_device_id *saved_ent;
	int saved_ctrl_num;
	struct ucdif *saved_controller_if;

	ohci_t *ohci = (ohci_t *)ucd_priv;
	if(!ohci->handle)
		return -1;

#ifdef TOSONLY
	/* From here on we touch OHCI PCI MMIO at ohci->regs; on TOSONLY
	 * that window is supervisor-only
	 */
	oldmode = (Super(1L) ? 0L: Super(0L));
#endif

	pci_rsc_desc = (PCI_RSC_DESC *)Get_resource(ohci->handle);

	/* save PCI-specific fields before zeroing transient state */
	saved_handle        = ohci->handle;
	saved_ent           = ohci->ent;
	saved_ctrl_num      = ohci->ctrl_num;
	saved_controller_if = ohci->controller_if;

	memset(ohci, 0, sizeof(ohci_t));

	ohci->handle        = saved_handle;
	ohci->ent           = saved_ent;
	ohci->ctrl_num      = saved_ctrl_num;
	ohci->controller_if = saved_controller_if;

	/* this must be aligned to a 256 byte boundary */
	ohci->hcca_unaligned = (struct ohci_hcca *)kmalloc(sizeof(struct ohci_hcca) + 256);
	if(ohci->hcca_unaligned == NULL)
	{
		ALERT(("HCCA malloc failed"));
		ret = -1;
		goto out;
	}
	ohci->hcca = (struct ohci_hcca *)(((unsigned long)ohci->hcca_unaligned + 255) & ~255);
	memset(ohci->hcca, 0, sizeof(struct ohci_hcca));
	flush_dcache_range(ohci->hcca, sizeof(struct ohci_hcca));
	DEBUG(("aligned hcca 0x%p", ohci->hcca));

	ohci->ohci_dev_unaligned = (struct ohci_device *)kmalloc(sizeof(struct ohci_device) + 15);
	if(ohci->ohci_dev_unaligned == NULL)
	{
		ALERT(("EDs malloc failed"));
		hc_free_buffers(ohci);
		ret = -1;
		goto out;
	}
	ohci->ohci_dev = (struct ohci_device *)(((unsigned long)ohci->ohci_dev_unaligned + 15) & ~15);
	memset(ohci->ohci_dev, 0, sizeof(struct ohci_device));
	flush_dcache_range(ohci->ohci_dev, sizeof(struct ohci_device));
	DEBUG(("aligned EDs 0x%p", ohci->ohci_dev));

	ohci->td_unaligned = (td_t *)kmalloc(sizeof(td_t) * NUM_TD + 15);
	if(ohci->td_unaligned == NULL)
	{
		ALERT(("TDs malloc failed"));
		hc_free_buffers(ohci);
		ret = -1;
		goto out;
	}
	ohci->ptd = (td_t *)(((unsigned long)ohci->td_unaligned + 15) & ~15);
	memset(ohci->ptd, 0, sizeof(td_t) * NUM_TD);
	flush_dcache_range(ohci->ptd, sizeof(td_t) * NUM_TD);
	DEBUG(("aligned TDs 0x%p", ohci->ptd));

	ohci->disabled = 1;
	ohci->sleeping = 0;
	ohci->irq = -1;

	if((long)pci_rsc_desc >= 0)
	{
		unsigned short flags;
		do
		{
			DEBUG(("PCI USB descriptors: flags 0x%04x start 0x%08lx \r\n offset 0x%08lx dmaoffset 0x%08lx length 0x%08lx",
			 pci_rsc_desc->flags, pci_rsc_desc->start, pci_rsc_desc->offset, pci_rsc_desc->dmaoffset, pci_rsc_desc->length));
			if(!(pci_rsc_desc->flags & FLG_IO))
			{
				if(usb_base_addr == 0xFFFFFFFF)
				{
					usb_base_addr = pci_rsc_desc->start;
					ohci->offset = pci_rsc_desc->offset;
					ohci->regs = (void *)(pci_rsc_desc->offset + pci_rsc_desc->start);
					ohci->dma_offset = pci_rsc_desc->dmaoffset;
					if((pci_rsc_desc->flags & FLG_ENDMASK) == ORD_MOTOROLA)
						ohci->big_endian = 0;
					else
						ohci->big_endian = 1;
				}
			}
			flags = pci_rsc_desc->flags;
			pci_rsc_desc = (PCI_RSC_DESC *)((unsigned long)pci_rsc_desc->next + (unsigned long)pci_rsc_desc);
		}
		while(!(flags & FLG_LAST));
	}
	else
	{
		hc_free_buffers(ohci);
		ret = -1;
		goto out;
	}
	if(usb_base_addr == 0xFFFFFFFF)
	{
		hc_free_buffers(ohci);
		ret = -1;
		goto out;
	}

	switch(ohci->ent->vendor)
	{
		case PCI_VENDOR_ID_AL:     ohci->slot_name = "uli1575"; break;
		case PCI_VENDOR_ID_NEC:    ohci->slot_name = "uPD720101"; ohci->flags |= OHCI_FLAGS_NEC; break;
		case PCI_VENDOR_ID_PHILIPS: ohci->slot_name = "isp1561"; break;
		default:                    ohci->slot_name = "generic"; break;
	}

	DEBUG(("OHCI usb-%s-%c, regs address 0x%08lx, PCI handle 0x%lx",
	 ohci->slot_name, (char)ohci->ctrl_num + '0', (unsigned long)ohci->regs, ohci->handle));

	if(hc_reset(ohci) < 0)
	{
		ALERT(("Can't reset OHCI usb-%s-%c", ohci->slot_name, (char)ohci->ctrl_num + '0'));
		hc_release_ohci(ohci);
		hc_free_buffers(ohci);
		ret = -1;
		goto out;
	}
	if(hc_start(ohci) < 0)
	{
		ALERT(("Can't start OHCI usb-%s-%c", ohci->slot_name, (char)ohci->ctrl_num + '0'));
		hc_release_ohci(ohci);
		hc_free_buffers(ohci);
		ret = -1;
		goto out;
	}
#ifdef DEV_DEBUG
	ohci_dump(ohci, 1);
#endif
#ifndef TOSONLY
	Hook_interrupt(ohci->handle, (void *)ohci_int_handle_asm, (unsigned long *)ohci);
#endif
	ohci->ohci_inited = 1;
out:
#ifdef TOSONLY
	if (oldmode) SuperToUser(oldmode);
#endif
	return ret;
}

long usb_lowlevel_stop(void *ucd_priv)
{
	/* this gets called really early - before the controller has been properly initialized */
	ohci_t *ohci = (ohci_t *)ucd_priv;
	if(!ohci->ohci_inited)
		return 0;
#ifndef TOSONLY
	/* Unhook_interrupt(ohci->handle); */
#endif
	hc_reset(ohci);
	hc_free_buffers(ohci);
	ohci->ohci_inited = 0;
	return 0;
}

/*-------------------------------------------------------------------------*
 * Init functions
 *-------------------------------------------------------------------------*/

static long _cdecl
ohci_open(struct ucdif *u)
{
	return E_OK;
}

static long _cdecl
ohci_close(struct ucdif *u)
{
	return E_OK;
}

static long _cdecl
ohci_ioctl(struct ucdif *u, short cmd, long arg)
{
	long ret = E_OK;

	switch(cmd)
	{
		case FS_INFO:
		{
			*(long *)arg = (((long)VER_MAJOR << 16) | VER_MINOR);
			break;
		}
		case LOWLEVEL_INIT:
		{
			ret = usb_lowlevel_init(u->ucd_priv);
			break;
		}
		case LOWLEVEL_STOP:
		{
			ret = usb_lowlevel_stop(u->ucd_priv);
			break;
		}
		case SUBMIT_CONTROL_MSG:
		{
			struct control_msg *ctrl_msg = (struct control_msg *)arg;
			ret = submit_control_msg(ctrl_msg->dev, ctrl_msg->pipe,
						 ctrl_msg->data, ctrl_msg->size,
						 ctrl_msg->setup);
			break;
		}
		case SUBMIT_BULK_MSG:
		{
			struct bulk_msg *bulk_msg = (struct bulk_msg *)arg;
			ret = submit_bulk_msg(bulk_msg->dev, bulk_msg->pipe,
					      bulk_msg->data, bulk_msg->len,
					      bulk_msg->flags, bulk_msg->timeout);
			break;
		}
		case SUBMIT_INT_MSG:
		{
			struct int_msg *int_msg = (struct int_msg *)arg;
			ret = submit_int_msg(int_msg->dev, int_msg->pipe,
					     int_msg->buffer, int_msg->transfer_len,
					     int_msg->interval);
			break;
		}
		default:
		{
			return ENOSYS;
		}
	}
	return ret;
}

char fail_kentry[] = "wrong kentry version";
char fail_probe[] = "probe failed";

long
ohci_alloc_ucdif(struct ucdif **u)
{
	struct ucdif *ohci_uif;

	if(!(*u = (struct ucdif *)kmalloc(sizeof(struct ucdif))))
		return -1;
	ohci_uif = *u;

	ohci_uif->next = 0;
	ohci_uif->api_version = USB_API_VERSION;
	ohci_uif->class = USB_CONTRLL;
	ohci_uif->lname = lname;
	ohci_uif->unit = 0;
	ohci_uif->flags = 0;
	ohci_uif->open = ohci_open;
	ohci_uif->close = ohci_close;
	ohci_uif->resrvd1 = 0;
	ohci_uif->ioctl = ohci_ioctl;
	ohci_uif->resrvd2 = 0;
	strcpy(ohci_uif->name, "ohci-pci");

	{
		/* ohci_t is allocate-once for the controller's lifetime (never
		 * kfree'd), so we over-allocate and align up without keeping
		 * the raw pointer. Cache-line alignment matters for two things:
		 * (a) unlock_usb's cpushl on the line containing job_in_progress
		 * only touches memory we own, (b) the tos_bounce array starts on
		 * a cache line boundary.
		 */
		long ohci_padded = (sizeof(ohci_t) + M68K_CACHE_LINE_SIZE - 1)
		                   & ~((long)M68K_CACHE_LINE_SIZE - 1);
		void *raw = (void *)kmalloc(ohci_padded + M68K_CACHE_LINE_SIZE);
		if (raw == NULL)
		{
			kfree(ohci_uif);
			*u = NULL;
			return -1;
		}
		ohci_uif->ucd_priv = (void *)(((unsigned long)raw
		                               + M68K_CACHE_LINE_SIZE - 1)
		                              & ~((unsigned long)M68K_CACHE_LINE_SIZE - 1));
	}
	memset(ohci_uif->ucd_priv, 0, sizeof(ohci_t));

	return 0;
}

long
ohci_pci_probe(void)
{
	short index;
	long err;
	long handle;
	struct pci_device_id *board;
	struct ucdif *ohci_uif = NULL;
	int ctrl_num = 0;

#ifdef TOSONLY
	if (pcibios_init())
		return -1;

	/* Check if we are in a FireBee */
	is_firebee();
#endif

	if(pcibios_installed == 0)
	{
		ALERT(("PCI-BIOS not found. You need a PCI-BIOS to use this driver"));
		return -1;
	}

	index = 0;
	do
	{
		handle = Find_pci_device(0x0000FFFFL, index++);
		if(handle >= 0)
		{
			unsigned long id = 0;
			err = Read_config_longword(handle, PCIIDR, &id);
			if(err >= 0)
			{
				unsigned long class;
				if(Read_config_longword(handle, PCIREV, &class) >= 0
				   && ((class >> 16) == PCI_CLASS_SERIAL_USB))
				{
					if((class >> 8) == PCI_CLASS_SERIAL_USB_OHCI)
					{
						board = ohci_usb_pci_table;
						while(board->vendor)
						{
							if((board->vendor == (id & 0xFFFF))
							    && (board->device == (id >> 16)))
							{
								struct usb_device *root_hub_dev = NULL;
								err = ohci_alloc_ucdif(&ohci_uif);
								if(err < 0)
									break;

								ohci_t *ohci = (ohci_t *)ohci_uif->ucd_priv;
								ohci->handle = handle;
								ohci->ent = board;
								ohci->ctrl_num = ctrl_num++;
								ohci->controller_if = ohci_uif;

								err = ucd_register(ohci_uif, &root_hub_dev);
								if(err)
								{
									DEBUG(("%s: ucd register failed!", __FILE__));
									break;
								}
								DEBUG(("%s: ucd register ok", __FILE__));
							}
							board++;
						}
					}
				}
			}
		}
	}
	while(handle >= 0);

	return 0;
}

/* Entry function */

#ifdef TOSONLY
int init(int argc, char **argv, char **env);

int
init(int argc, char **argv, char **env)
#else

long _cdecl init_ucd(struct kentry *k, struct usb_module_api *uapi, char **reason)
#endif
{
	long ret;
#ifndef TOSONLY
	kentry = k;
	api = uapi;

	if(check_kentry_version())
	{
		*reason = fail_kentry;
		return -1;
	}
#else
	if(!getcookie(_USB, (long *)&api))
	{
		(void)Cconws("OHCI: failed to get _USB cookie\r\n");
		return -1;
	}
	set_tos_delay();
#endif
	ret = ohci_pci_probe();
	if(ret < 0)
	{
#ifndef TOSONLY
		*reason = fail_probe;
#endif
		return -1;
	}

	c_conws(MSG_BOOT);
	c_conws(MSG_GREET);

#ifdef TOSONLY
	c_conws("OHCI USB driver installed.\r\n");
	Ptermres(_PgmSize, 0);
#endif

	return 0;
}
