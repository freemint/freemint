/*
 *	Loadable MintNet network interface startup code.
 *
 *	06/22/94, Kay Roemer.
 */

# include "global.h"
# include "netinfo.h"

# include <mint/file.h>


long init (struct kerinfo *ker, struct netinfo *net);
long driver_init (void);


struct kerinfo *KERNEL;
struct netinfo *NETINFO;

ulong memory = 0;


long
init (struct kerinfo *k, struct netinfo *n)
{
	KERNEL = k;
	NETINFO = n;
	
	return (driver_init () != 0) ? 1 : 0;
}
