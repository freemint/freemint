/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File: pmap_set.c
 *       communicate with the port mapper to set and reset mappings
 *       these are the server side functions
 */


#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>
#include <netinet/in.h>
#include "clnt.h"
#include "pmap.h"


static struct timeval pmap_timeout = { 5, 0 };
static struct timeval pmap_tottimeout = { 60, 0 };


bool_t
pmap_set(u_long prog, u_long vers, u_long proto, u_long port)
{
	struct sockaddr_in addr;
	CLIENT *cl;
	struct pmap map;
	bool_t pmres;
	enum clnt_stat res;

	/* we want to talk to ourselves */
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(PMAPPORT);
	cl = clnt_create(&addr, PMAPPROG, PMAPVERS, pmap_timeout,
	                       RPCSMALLMSGSIZE, RPCSMALLMSGSIZE, NULL);
	if (!cl)
		return FALSE;

	map.pm_prog = prog;
	map.pm_vers = vers;
	map.pm_prot = proto;
	map.pm_port = port;

	res = clnt_call(cl, PMAPPROC_SET, (xdrproc_t)xdr_pmap, (caddr_t)&map,
	                    (xdrproc_t)xdr_bool, (caddr_t)&pmres, pmap_tottimeout);
	clnt_destroy(cl);

	if (res != RPC_SUCCESS)
		return FALSE;
	
	return pmres;
}


bool_t
pmap_unset(u_long prog, u_long vers)
{
	struct sockaddr_in addr;
	CLIENT *cl;
	struct pmap map;
	bool_t pmres;
	enum clnt_stat res;

	/* we want to talk to ourselves */
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(PMAPPORT);
	cl = clnt_create(&addr, PMAPPROG, PMAPVERS, pmap_timeout,
	                       RPCSMALLMSGSIZE, RPCSMALLMSGSIZE, NULL);
	if (!cl)
		return FALSE;

	map.pm_prog = prog;
	map.pm_vers = vers;
	map.pm_prot = 0;
	map.pm_port = 0;

	res = clnt_call(cl, PMAPPROC_UNSET, (xdrproc_t)xdr_pmap, (caddr_t)&map,
	                    (xdrproc_t)xdr_bool, (caddr_t)&pmres, pmap_tottimeout);
	clnt_destroy(cl);

	if (res != RPC_SUCCESS)
		return FALSE;

	return pmres;
}
