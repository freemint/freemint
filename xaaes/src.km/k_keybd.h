/*
 * $Id$
 *
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Kernal Message Handler
 */

#ifndef _k_keybd_h
#define _k_keybd_h

#include "global.h"
#include "xa_types.h"


#define KEQ_L 64

struct key_q
{
	struct xa_client *client;
	struct xa_client *locked;
	struct rawkey k;
};
struct key_queue
{
	int cur;
	int last;
	struct key_q q[KEQ_L];
};
extern struct key_queue pending_keys;

short key_conv( struct xa_client *client, short key );
//void keybd_event(enum locks lock, struct xa_client *client, struct rawkey *key);
void cancel_keyqueue	(struct xa_client *client);
void queue_key		(struct xa_client *client, const struct rawkey *key);
bool unqueue_key	(struct xa_client *client, struct rawkey *key);

int switch_keyboard( char *tbname );

#if REMOTE_KBD

typedef struct
{
	unsigned char *unshift; 		/* Table of 'normal' key presses	*/
	unsigned char *shift; 		/* Table of Shift key presses 	*/
	unsigned char *capslock;		/* Table of Capslock key presses	*/
	unsigned char *altunshift;		/* From TOS 4.00, undocumented! */
	unsigned char *altshift;		/* From TOS 4.00, undocumented! */
	unsigned char *altcapslock; 	/* From TOS 4.00, undocumented! */
	unsigned char *altgr; 		/* From TOS 4.00, undocumented! */
} KEYTAB;

#define NKEYCODES 256
#define NALTCODES 16

typedef struct
{
	unsigned char unshift[NKEYCODES];
	unsigned char shift[NKEYCODES];
	unsigned char alt[NALTCODES*2];
	unsigned char altshift[NALTCODES*2];
} SCANTAB;

extern SCANTAB *scantab;
#define CTRL_Z 0x1A
#endif

void keyboard_input	(enum locks lock, int dev, bool remote);

#endif /* _k_keybd_h */
