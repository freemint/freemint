/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * File: pmap_get.c
 *       communicate with the port mapper to set and reset mappings
 *       these are the client side functions
 */


#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>
#include <netinet/in.h>
#include "clnt.h"
#include "pmap.h"


static struct timeval timeout = { 5, 0 };
static struct timeval tottimeout = { 60, 0 };


u_long
pmap_getport(struct sockaddr_in *addr, u_long prog, u_long vers, u_long proto)
{
	CLIENT *cl;
	struct pmap map;
	u_long port;
	enum clnt_stat res;


	addr->sin_port = htons(PMAPPORT);
	cl = clnt_create(addr, PMAPPROG, PMAPVERS, timeout,
	                       RPCSMALLMSGSIZE, RPCSMALLMSGSIZE, NULL);
	if (!cl)
		return FALSE;

	map.pm_prog = prog;
	map.pm_vers = vers;
	map.pm_prot = proto;
	map.pm_port = 0;

	res = clnt_call(cl, PMAPPROC_GETPORT, (xdrproc_t)xdr_pmap, (caddr_t)&map,
	                         (xdrproc_t)xdr_ulong, (caddr_t)&port, tottimeout);
	clnt_destroy(cl);

	if (res != RPC_SUCCESS)
		return FALSE;

	return port;
}
