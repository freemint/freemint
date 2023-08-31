/*
 *	Loadable MintNet network interface startup code.
 *
 *	06/22/94, Kay Roemer.
 */

# include "global.h"
# include "netinfo.h"


long init (struct kerinfo *ker, struct netinfo *net) __asm__("init");
long driver_init(void);


struct kerinfo *KERNEL;
struct netinfo *NETINFO;


long
init (struct kerinfo *k, struct netinfo *n)
{
	KERNEL = k;
	NETINFO = n;
	
	return (driver_init () != 0) ? 1 : 0;
}
