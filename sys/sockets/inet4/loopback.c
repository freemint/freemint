
# include "loopback.h"

# include "mint/sockio.h"

# include "bpf.h"
# include "if.h"

# include "buf.h"


static long	loop_open	(struct netif *);
static long	loop_close	(struct netif *);
static long	loop_output	(struct netif *, BUF *, const char *, short, short);
static long	loop_ioctl	(struct netif *, short, long);

static struct netif if_loopback =
{
	name:		"lo",
	unit:		0,
	flags:		IFF_LOOPBACK | IFF_BROADCAST | IFF_IGMP,
	metric:		0,
	mtu:		2 * 8192,
	timer:		0,
	hwtype:		HWTYPE_NONE,
	
	rcv:		{ maxqlen:	IF_MAXQ },
	snd:		{ maxqlen:	IF_MAXQ },
	
	open:		loop_open,
	close:		loop_close,
	output:		loop_output,
	ioctl:		loop_ioctl,
	timeout:	NULL,
	
	data:		NULL
};


static long
loop_open (struct netif *nif)
{
	return 0;
}

static long
loop_close (struct netif *nif)
{
	return 0;
}

static long
loop_output (struct netif *nif, BUF *buf, const char *hwaddr, short hwlen, short pktype)
{
	long r;
	
	nif->out_packets++;
	if (nif->bpf && BUF_LEAD_SPACE (buf) >= 4)
	{
		/*
		 * Add 4 byte dummy header containing the address
		 * family for the packet type.
		 */
		long af = (pktype == PKTYPE_IP) ? AF_INET : AF_UNSPEC;
		
		buf->dstart -= 4;
		*(long *) buf->dstart = af;
		bpf_input (nif, buf);
		buf->dstart += 4;
	}
	
	r = if_input (&if_loopback, buf, 0, pktype);
	if (r)
		nif->in_errors++;
	else
		nif->in_packets++;
	
	return r;
}

static long
loop_ioctl (struct netif *nif, short cmd, long arg)
{
	switch (cmd)
	{
		case SIOCSIFFLAGS:
			return 0;
		
		case SIOCSIFADDR:
			nif->flags |= (IFF_UP|IFF_RUNNING|IFF_IGMP);
			return 0;
		
		case SIOCSIFNETMASK:
			return 0;
	}
	
	return ENOSYS;
}

void
loopback_init (void)
{
	if_register (&if_loopback);
}
