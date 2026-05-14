/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
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

#ifndef _libkern_ikbd_poll_h
#define _libkern_ikbd_poll_h

/*
 * Check if there is a pending interrupt request from the keyboard ACIA.
 * We use this while the CPU priority is set to 6, causing interrupts to
 * be disabled.  The major problem with this is that some keyboard/mouse
 * interrupt data is lost, which typically results in mouse movements
 * being interpreted as keyclicks, then repeating keys and other nasties.
 *
 * We call this routine to poll for ikbd interrupts, which are then serviced
 * by calling the keyboard interrupt routine ourselves.
 *
 * Returns != 0 if there is a pending interrupt request.
 */
static inline int ikbd_int_pending(void)
{
	unsigned char keyctl = *(volatile unsigned char *)0xFFFFFC00UL;
	return keyctl & 0x80;
}

/*
 * Synthesise a stack frame and jump through vector $118 (IKBD ACIA),
 * invoking whichever handler is currently installed.
 */
void fake_ikbd_int(void);

#endif /* _libkern_ikbd_poll_h */
