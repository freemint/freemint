/*
 * Ported to FreeMiNT by Alan Hourihane with additional inet4 hooks for IGMP support.
 * 
 * File from lwIP.
 */

/**
 * @file
 * IGMP - Internet Group Management Protocol
 *
 */

/*
 * Copyright (c) 2002 CITEL Technologies Ltd.
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
 * 3. Neither the name of CITEL Technologies Ltd nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY CITEL TECHNOLOGIES AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL CITEL TECHNOLOGIES OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is a contribution to the lwIP TCP/IP stack.
 * The Swedish Institute of Computer Science and Adam Dunkels
 * are specifically granted permission to redistribute this
 * source code.
*/

/*-------------------------------------------------------------
Note 1)
Although the rfc requires V1 AND V2 capability
we will only support v2 since now V1 is very old (August 1989)
V1 can be added if required

a debug print and statistic have been implemented to
show this up.
-------------------------------------------------------------
-------------------------------------------------------------
Note 2)
A query for a specific group address (as opposed to ALLHOSTS)
has now been implemented as I am unsure if it is required

a debug print and statistic have been implemented to
show this up.
-------------------------------------------------------------
-------------------------------------------------------------
Note 3)
The router alert rfc 2113 is implemented in outgoing packets
but not checked rigorously incoming
-------------------------------------------------------------
Steve Reynolds
------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * RFC 988  - Host extensions for IP multicasting                         - V0
 * RFC 1054 - Host extensions for IP multicasting                         -
 * RFC 1112 - Host extensions for IP multicasting                         - V1
 * RFC 2236 - Internet Group Management Protocol, Version 2               - V2  <- this code is based on this RFC (it's the "de facto" standard)
 * RFC 3376 - Internet Group Management Protocol, Version 3               - V3
 * RFC 4604 - Using Internet Group Management Protocol Version 3...       - V3+
 * RFC 2113 - IP Router Alert Option                                      - 
 *----------------------------------------------------------------------------*/

# include "igmp.h"

# include "if.h"
# include "inetutil.h"
# include "ip.h"


static long igmp_input (struct netif *, BUF *, ulong, ulong);
static long igmp_error (short, short, BUF *, ulong, ulong);

struct in_ip_ops igmp_ops =
{
	proto:	IPPROTO_IGMP,
	next:	NULL,
	error:	igmp_error,
	input:	igmp_input
};

static struct igmp_group *igmp_lookup_group(struct netif *ifp, ulong addr);
static long   igmp_remove_group(struct igmp_group *group);
static void   igmp_timeout( struct igmp_group *group);
static void   igmp_start_timer(struct igmp_group *group, char max_time);
static void   igmp_delaying_member(struct igmp_group *group, char maxresp);
static void   igmp_send(struct igmp_group *group, char type);

static struct igmp_group *igmp_group_list;

static struct igmp_group *
igmp_lookfor_group(struct netif *nif, ulong addr);

/* 
 * IGMP constants
 */
#define IGMP_TTL                       1
#define IGMP_MINLEN                    8
#define ROUTER_ALERT                   0x9404
#define ROUTER_ALERTLEN                4

/*
 * IGMP message types, including version number.
 */
#define IGMP_MEMB_QUERY                0x11 /* Membership query         */
#define IGMP_V1_MEMB_REPORT            0x12 /* Ver. 1 membership report */
#define IGMP_V2_MEMB_REPORT            0x16 /* Ver. 2 membership report */
#define IGMP_LEAVE_GROUP               0x17 /* Leave-group message      */

/* Group  membership states */
#define IGMP_GROUP_NON_MEMBER          0
#define IGMP_GROUP_DELAYING_MEMBER     1
#define IGMP_GROUP_IDLE_MEMBER         2

static ulong allsystems;
static ulong allrouters;

/** Set an IP address given by the four byte-parts */
#define IP4_ADDR(ipaddr, a,b,c,d) \
      ipaddr = ((ulong)((a) & 0xff) << 24) | \
               ((ulong)((b) & 0xff) << 16) | \
               ((ulong)((c) & 0xff) << 8)  | \
                (ulong)((d) & 0xff)


static long
igmp_input (struct netif *nif, BUF *buf, ulong saddr, ulong daddr)
{
	struct igmp_group *group, *groupref;
	struct igmp_dgram *igmph;
	short datalen;
	
	UNUSED(saddr);
	UNUSED(daddr);
	igmph = (struct igmp_dgram *) IP_DATA (buf);
	
	datalen = (long) buf->dend - (long)igmph;
	if (datalen & 1)
		*buf->dend = 0;
	
	if (chksum (igmph, (datalen+1)/2))
	{
		DEBUG (("igmp_input: bad checksum from 0x%lx %d 0x%lx", saddr,datalen,daddr));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}

	group = igmp_lookfor_group(nif, daddr);
	
	if (!group)
	{
		DEBUG (("igmp_input: IGMP frame not for us"));
		buf_deref (buf, BUF_NORMAL);
		return 0;
	}

	switch (igmph->type)
	{
		case IGMP_MEMB_QUERY:
			if ((daddr == allsystems) && group->group_address == INADDR_ANY)
			{
				if (igmph->maxresp == 0) 
				{
					igmph->maxresp = IGMP_V1_DELAYING_MEMBER_TMR;
				}

				groupref = igmp_group_list;

				while (groupref)
				{
					if ((groupref->nif == nif) && (!(groupref->group_address == allsystems))) 
					{
						igmp_delaying_member(groupref, igmph->maxresp);
					}

					groupref = groupref->next;
				}
			}
			else
			{
				if (group->group_address == INADDR_ANY)
				{
					if (daddr == allsystems)
					{
						ulong groupaddr;

						groupaddr = group->group_address;

						group = igmp_lookfor_group(nif, groupaddr);
					}

					if (group) 
					{
						igmp_delaying_member(group, igmph->maxresp);
					}
				}
			}
			break;
		case IGMP_V2_MEMB_REPORT:
			if (group->group_state == IGMP_GROUP_DELAYING_MEMBER)
			{
				group->timer = 0;
				group->group_state = IGMP_GROUP_IDLE_MEMBER;
				group->last_reporter_flag = 0;
			}
			break;
	}
	
	return -1;
}

static long
igmp_error (short type, short code, BUF *buf, ulong saddr, ulong daddr)
{
	UNUSED(type);
	UNUSED(code);
	UNUSED(saddr);
	UNUSED(daddr);
	TRACE (("igmp_error: cannot send igmp error message to %lx", daddr));
	buf_deref (buf, BUF_NORMAL);
	return 0;
}

long
igmp_start(struct netif *nif)
{
	struct igmp_group *group;

	group = igmp_lookup_group(nif, allsystems);

	if (group)
	{
		group->group_state = IGMP_GROUP_IDLE_MEMBER;
		group->use++;

		if (nif->igmp_mac_filter) 
		{
			nif->igmp_mac_filter(nif, allsystems, IGMP_ADD_MAC_FILTER);
		}

		return 0;
	}

	return EOPNOTSUPP;
}

void
igmp_report_groups(struct netif *nif)
{
	struct igmp_group *group = igmp_group_list;

	while (group)
	{
		if (group->nif == nif)
		{
			igmp_delaying_member(group, IGMP_JOIN_DELAYING_MEMBER_TMR);
		}
		group = group->next;
	}
}

long
igmp_stop(struct netif *nif)
{
	struct igmp_group *group = igmp_group_list;
	struct igmp_group *prev = NULL;
	struct igmp_group *next;

	while (group) 
	{
		next = group->next;
		if (group->nif == nif)
		{
			if (group == igmp_group_list)
			{
				igmp_group_list = next;
			}

			if (prev)
			{
				prev->next = next;
			}

			if (nif->igmp_mac_filter)
			{
				nif->igmp_mac_filter(nif, group->group_address, IGMP_DEL_MAC_FILTER);
			}

			kfree(group);
		}
		else
		{
			prev = group;
		}

		group = next;
	}

	return 0;
}

static struct igmp_group *
igmp_lookfor_group(struct netif *nif, ulong addr)
{
	struct igmp_group *group = igmp_group_list;

	while (group) 
	{
		if ((group->nif == nif) && (addr == group->group_address))
			return group;

		group = group->next;
	}

	return NULL;
}

static struct igmp_group *
igmp_lookup_group(struct netif *nif, ulong addr)
{
	struct igmp_group *group = igmp_group_list;

	group = igmp_lookfor_group(nif, addr);
	if (group)
		return group;

	group = (struct igmp_group *)kmalloc(sizeof(struct igmp_group));
	if (group) 
	{
		group->nif = nif;
		group->group_address = addr;
		group->timer = 0; /* not running */
		group->group_state = IGMP_GROUP_NON_MEMBER;
		group->last_reporter_flag = 0;
		group->use = 0;
		group->next = igmp_group_list;

		igmp_group_list = group;
	}
	
	return group;
}

static long
igmp_remove_group(struct igmp_group *group)
{
	int err = 0;

	if (igmp_group_list == group)
		igmp_group_list = group->next;
	else
	{
		struct igmp_group *tmpGroup;

		for (tmpGroup = igmp_group_list; tmpGroup != NULL; tmpGroup = tmpGroup->next) {
			if (tmpGroup->next == group) {
				tmpGroup->next = group->next;
				break;
			}
		}

		if (!tmpGroup)
			err = EOPNOTSUPP;
	}

	kfree(group);

	return err;
}

static void
igmp_tmr (PROC *proc, long arg)
{
	struct igmp_group *group = igmp_group_list;

	UNUSED(proc);
	UNUSED(arg);
	while (group) 
	{
		if (group->timer > 0) 
		{
			group->timer--;
			if (group->timer == 0)
			{
				igmp_timeout(group);
			}
		}

		group = group->next;
	}

	addroottimeout (IGMP_TMR_INTERVAL, igmp_tmr, 0);
}

static void
igmp_timeout(struct igmp_group *group)
{
	if (group->group_state == IGMP_GROUP_DELAYING_MEMBER)
	{
		igmp_send(group, IGMP_V2_MEMB_REPORT);
	}
}

static void
igmp_start_timer(struct igmp_group *group, char max_time)
{
        *(volatile unsigned long *)0x04baL;

	if (max_time == 0)
	{
		max_time = 1;
	}

	group->timer = (*(volatile unsigned long *)0x04baL) % max_time;
}

static void
igmp_delaying_member(struct igmp_group *group, char maxresp)
{
	if ((group->group_state == IGMP_GROUP_IDLE_MEMBER) ||
	   ((group->group_state == IGMP_GROUP_DELAYING_MEMBER) &&
	    ((group->timer == 0) || (maxresp < group->timer)))) 
	{
		igmp_start_timer(group, maxresp);

		group->group_state = IGMP_GROUP_DELAYING_MEMBER;
	}
}

static void
igmp_send(struct igmp_group *group, char type)
{
	BUF *buf;
	struct igmp_dgram *igmph;
	ulong src = ip_dst_addr(INADDR_ANY);
	ulong dst = 0;

	buf = buf_alloc (IGMP_MINLEN, 0, BUF_NORMAL);
	if (!buf)
	{
		DEBUG (("tcp_reset: no memory for reset segment"));
		return;
	}
	
	igmph = (struct igmp_dgram *) buf->dstart;

	if (type == IGMP_V2_MEMB_REPORT)
	{
		dst = group->group_address;
		igmph->group_address = group->group_address;
		group->last_reporter_flag = 1;
	}
	else
	{
		if (type == IGMP_LEAVE_GROUP)
		{
			dst = allrouters;
			igmph->group_address = group->group_address;
		}
	}

	if ((type == IGMP_V2_MEMB_REPORT) || (type == IGMP_LEAVE_GROUP))
	{
		igmph->type = type;
		igmph->maxresp = 0;
		igmph->chksum = 0;
		igmph->chksum = chksum (igmph, IGMP_MINLEN);
	
		buf->dend += IGMP_MINLEN;

		ip_send (src, dst, buf, IPPROTO_IGMP, 0, 0);
	} else {
		buf_deref (buf, BUF_NORMAL);
	}
}

static long 
igmp_joingroup_netif(struct netif *nif, ulong groupaddr){
	struct igmp_group *group;

	group = igmp_lookup_group(nif, groupaddr);

	if (group)
	{
		if (group->group_state != IGMP_GROUP_NON_MEMBER)
		{
			DEBUG(("Non member!"));
		}
		else
		{
			if ((group->use == 0) && nif->igmp_mac_filter)
			{
				nif->igmp_mac_filter(nif, groupaddr, IGMP_ADD_MAC_FILTER);
			}

			igmp_send(group, IGMP_V2_MEMB_REPORT);

			igmp_start_timer(group, IGMP_JOIN_DELAYING_MEMBER_TMR);

			group->group_state = IGMP_GROUP_DELAYING_MEMBER;
		}
		group->use++;
		return 0;
	}
	else
	{
		return EOPNOTSUPP;
	}
}

static long 
igmp_leavegroup_netif(struct netif *nif, ulong groupaddr){
	struct igmp_group *group;
	group = igmp_lookfor_group(nif, groupaddr);

	if (group)
	{
		if (group->use <= 1)
		{
			igmp_send(group, IGMP_LEAVE_GROUP);
		}

		if (nif->igmp_mac_filter)
		{
			nif->igmp_mac_filter(nif, groupaddr, IGMP_DEL_MAC_FILTER);
		}

		return igmp_remove_group(group);
	}
	else
	{
		group->use--;
		return 0;
	}
}


long
igmp_joingroup(ulong ifaddr, ulong groupaddr, unsigned short ifindex)
{
	struct netif *nif;

	if(!ifindex){
		struct ifaddr *ifa = NULL;
		long res, last_err = 0;
		for (nif = allinterfaces; nif; nif = nif->next)
		{
			ifa = if_af2ifaddr (nif, AF_INET);

			if ((nif->flags & IFF_IGMP) &&
				((ifa && ifaddr == ifa->adr.in.sin_addr.s_addr) || (ifaddr == INADDR_ANY))) 
			{
				res = igmp_joingroup_netif(nif, groupaddr);
				if(res){
					last_err = res;
				}
			}
			return last_err;
		}

	}
	else {

		for (nif = allinterfaces; nif; nif = nif->next)
		{
			if ((nif->flags & IFF_IGMP) && (nif->index == ifindex)) 
			{
				if (!IS_MULTICAST(groupaddr)){
					return EINVAL;
				}				
				return igmp_joingroup_netif(nif, groupaddr);
			}
		}

	}

	return EOPNOTSUPP;
}

long
igmp_leavegroup(ulong ifaddr, ulong groupaddr, unsigned short ifindex)
{
	struct netif *nif;

	if(!ifindex){
        struct ifaddr *ifa = NULL;
		long res, last_err = 0;

		for (nif = allinterfaces; nif; nif = nif->next)
		{
			ifa = if_af2ifaddr (nif, AF_INET);

			if ((nif->flags & IFF_IGMP) &&
				((ifa && ifaddr == ifa->adr.in.sin_addr.s_addr) || (ifaddr == INADDR_ANY))) 
			{
				res = igmp_leavegroup_netif(nif, groupaddr);
				if(res){
					last_err = res;
				}
			}
		}
		return last_err;
	}
	else {

		for (nif = allinterfaces; nif; nif = nif->next)
		{
			if ((nif->flags & IFF_IGMP) && (nif->index == ifindex)) 
			{
				if (!IS_MULTICAST(groupaddr)){
					return EINVAL;
				}
				return igmp_leavegroup_netif(nif, groupaddr);
			}
		}

	}
	return EOPNOTSUPP;
}

void
igmp_init (void)
{
	ip_register (&igmp_ops);

	IP4_ADDR(allsystems, 224, 0, 0, 1);
	IP4_ADDR(allrouters, 224, 0, 0, 2);

	addroottimeout (IGMP_TMR_INTERVAL, igmp_tmr, 0);
}
