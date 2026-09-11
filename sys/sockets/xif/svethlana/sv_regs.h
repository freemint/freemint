/*
 * SVEthlana platform description.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * This is the FreeMiNT counterpart of the CONFIG_ATARI_SVETHLANA section
 * of Linux arch/m68k/atari/config.c: where the OpenCores MAC sits in the
 * SuperVidel address space, which vector it raises and how much DDR the
 * packet buffers take. The MAC itself is driven by the ethoc port in
 * svethlana.c.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef _sv_regs_h
#define _sv_regs_h

/* ethoc-regs: 0x800 bytes of MAC registers followed by the descriptors */
#define ATARI_SVETHLANA_PHYS_ADDR	0x80012000UL

/* Linux IRQ 141, vector 0xc5 */
#define ATARI_SVETHLANA_VECTOR		0xc5

/* SuperVidel firmware version register, low 10 bits are the version */
#define ATARI_SV_VERSION_PHYS_ADDR	0x8001007cUL
#define ATARI_SV_VERSION_MASK		0x3ffUL
/* Firmware 10 is the first with the Ethernet DMA */
#define ATARI_SV_MIN_VERSION		10

/*
 * ethoc-buf: 128 packet buffers of 1536 bytes each, the maximum ethoc
 * supports. The MAC can only DMA within the SuperVidel DDR RAM. Linux
 * takes the top of the DDR for this; here the memory is requested from the
 * SuperVidel XBIOS, which owns the DDR under TOS and MiNT, so the
 * framebuffer and the packet buffers cannot collide.
 */
#define SVETHLANA_BUF_SIZE		(128UL * 1536UL)

/*
 * Linux places the buffers 64 KB aligned, which with 1536 byte spacing
 * makes every buffer at least 512 byte aligned. Aligning the allocation
 * to 2048 gives the same alignment for every buffer.
 */
#define SVETHLANA_BUF_ALIGN		2048UL

/* CT60/SuperVidel XBIOS: mode 0 allocates from DDR, mode 1 frees */
#define ct60_vmalloc(mode, value) \
	((long) trap_14_wwl ((short) 0xc60e, (short) (mode), (long) (value)))

#endif /* _sv_regs_h */
