/*
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
#include "xa_scrp.h"
#include "xa_global.h"

/*
 * Scrap/clipboard directory stuff
 */

unsigned long
XA_scrp_read(int lock, struct xa_client *client, AESPB *pb)
{
	CONTROL(0,1,1)

	pb->intout[0] = 0;
	if (pb->addrin[0])
	{
		if (*cfg.scrap_path) /* return 0 if not initialized */
		{
			strcpy((char*)pb->addrin[0], cfg.scrap_path);
			pb->intout[0] = 1;
		}
	}

	return XAC_DONE;
}

unsigned long
XA_scrp_write(int lock, struct xa_client *client, AESPB *pb)
{
	const char *new_path = (const char *)pb->addrin[0];

	CONTROL(0,1,1)

	if (new_path && strcmp( new_path, cfg.scrap_path) && strlen(new_path) < 128)
	{
		strcpy(cfg.scrap_path, new_path);
	}

	pb->intout[0] = 1;
	return XAC_DONE;
}
