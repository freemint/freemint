/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * The CLIPBRD access driver - NF API definitions.
 *
 * Copyright (c) 2006 Standa Opichal of ARAnyM dev team.
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

#ifndef _CLIPBRD_NFAPI_H
#define _CLIPBRD_NFAPI_H

/*
 * general CLIPBRD driver version
 */
#define CLIPBRD_DRIVER_VERSION   0
#define BETA

/* if you change anything in the enum {} below you have to increase
   this CLIPBRD_NFAPI_VERSION!
*/
#define CLIPBRD_NFAPI_VERSION    1

enum {
	GET_VERSION = 0,	/* subID = 0 */
	CLIP_OPEN,	        /* open clipboard */
	CLIP_CLOSE,	        /* close clipboard */
	CLIP_READ,	        /* read clipboard data */
	CLIP_WRITE	        /* write data to clipboard */
};

extern unsigned long nf_clipbrd_id;
extern long __CDECL (*nf_call)(long id, ...);

#define NFCLIP(a)	(nf_clipbrd_id + a)

#endif /* _CLIPBRD_NFAPI_H */

