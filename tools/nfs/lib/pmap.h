/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

#ifndef PMAP_H
#define PMAP_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "xdr.h"


/* the actual xdr stuff */

#define PMAPPORT  ((u_short)111)
#define PMAPPROG  ((u_long)100000)
#define PMAPVERS  2

#define PMAPPROC_NULL     0
#define PMAPPROC_SET      1
#define PMAPPROC_UNSET    2
#define PMAPPROC_GETPORT  3
#define PMAPPROC_DUMP     4
#define PMAPPROC_CALLIT   5


struct pmap
{
	u_long pm_prog;
	u_long pm_vers;
	u_long pm_prot;
	u_long pm_port;
};

extern bool_t xdr_pmap(xdrs *x, struct pmap *p);


/* the programmer interface */

bool_t pmap_set(u_long prog, u_long vers, u_long proto, u_long port);
bool_t pmap_unset(u_long prog, u_long vers);
u_long pmap_getport(struct sockaddr_in *addr,
                      u_long prog, u_long vers, u_long proto);


#endif
