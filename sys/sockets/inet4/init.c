/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
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
 * 
 * 
 * begin:	2001-01-15
 * last change:	2001-01-15
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "init.h"

# include "mint/sockio.h"

# include "bpf.h"
# include "if.h"
# include "icmp.h"
# include "inet.h"
# include "inetutil.h"
# include "masquerade.h"
# include "rawip.h"
# include "route.h"
# include "tcp.h"
# include "udp.h"
# include "igmp.h"


void
inet4_init (void)
{
	/* install packetfilter */
	bpf_init ();
	
	/* load all interfaces */
	if_init ();
	
	/* initialize IP router & control device */
	route_init ();
	
	/* initialize raw IP driver; must be first */
	rip_init ();
	
	/* initialize masquerade support */
	masq_init ();
	
	/* initialize ICMP protocol */
	icmp_init ();
	/* initialize UDP protocol */
	udp_init ();
	/* initialize TCP protocol */
	tcp_init ();
	/* initialize IGMP protocol */
	igmp_init ();
	
	/* initialize lo0 */
	{
		struct sockaddr_in in;
		struct netif *nif;
		struct ifaddr *ifa;
		
		nif = if_name2if ("lo0");
		if (!nif)
		{
			ALERT (("inet4: No such interface lo0 ???"));
		} else
		{
			/* addr 127.0.0.1 */

			_mint_bzero(&in, sizeof(in));
			in.sin_family = AF_INET;
			in.sin_addr.s_addr = INADDR_LOOPBACK;
			in.sin_port = 0;

			if_setifaddr (nif, (struct sockaddr *) &in);

			ifa = if_af2ifaddr (nif, AF_INET);
			if (!ifa)
			{
				ALERT (("inet4: initializing lo0 failed ???"));
			} else
			{
				/* netmask 255.255.255.0 */

				ifa->subnetmask = IN_CLASSA_NET;
				ifa->subnet = ifa->subnetmask;

				/* broadaddr 127.0.0.255 */

				in.sin_family = AF_INET;
				in.sin_addr.s_addr = INADDR_LOOPBACK | IN_CLASSA_HOST;
				in.sin_port = 0;

				ifa->ifu.broadadr.in = in;

				/* up & running */

				nif->ioctl(nif, SIOCSIFADDR, 0);
			}
		}
	}

	/* register our domain */
	inet_init ();
}
