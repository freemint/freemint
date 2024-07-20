/*
 *	Internet Control Message Protocol, RFC 792
 *
 *	02/05/94, Kay Roemer.
 */

# ifndef _icmp_h
# define _icmp_h

# include "global.h"

# include "buf.h"


/* ICMP message types */
# define ICMPT_ECHORP	0		/* echo reply */
# define ICMPT_DSTUR	3		/* destination unreachable */
# define	ICMPC_NETUR	0	/* net unreachable */
# define	ICMPC_HOSTUR	1	/* host unreachable */
# define	ICMPC_PROTOUR	2	/* protocol unreachable */
# define	ICMPC_PORTUR	3	/* port unreachable */
# define	ICMPC_FNDF	4	/* fragmentation needed, but DF set */
# define	ICMPC_SRCRT	5	/* source route failed */
# define	ICMPC_NET_UNKNOWN	6 /* Destination network unknown */
# define	ICMPC_HOST_UNKNOWN	7 /* Destination host unknown */
# define	ICMPC_HOST_ISOLATED 8 /* Source host isolated */
# define	ICMPC_NET_ANO		9 /* Network administratively prohibited */
# define	ICMPC_HOST_ANO		10 /* Host administratively prohibited */
# define	ICMPC_NET_UNR_TOS	11 /* Network unreachable for ToS */
# define	ICMPC_HOST_UNR_TOS	12 /* Host unreachable for ToS */
# define	ICMPC_PKT_FILTERED	13 /* Communication administratively filtered */
# define	ICMPC_PREC_VIOLATION	14	/* Host Precedence violation */
# define	ICMP_PREC_CUTOFF	15	/* Precedence cut off in effect */

# define ICMPT_SRCQ	4		/* source quench */
# define ICMPT_REDIR	5		/* redirect route */
# define	ICMPC_NETRD	0	/* redirect net */
# define	ICMPC_HOSTRD	1	/* redirect host */
# define	ICMPC_TOSNRD	2	/* redirect net for type of service */
# define	ICMPC_TOSHRD	3	/* redirect host for type of service */

# define ICMPT_ECHORQ	8		/* echo request */
# define ICMPT_ROUTER_ADVERT 9	/* Router Advertisement */
# define ICMPT_ROUTER_SOLICIT 10	/* Router discovery/selection/solicitation */

# define ICMPT_TIME_EXCEEDED	11	/* Time Exceeded		*/
# define ICMPT_TIMEX	11		/* time exeeded */
# define	ICMPC_TTLEX	0	/* ttl exeeded */
# define	ICMPC_FRAGEX	1	/* fragmentation reassambly timex */

# define ICMPT_PARAMP	12		/* parameter problem */
# define ICMPT_TIMERQ	13		/* timestamp request */
# define ICMPT_TIMERP	14		/* timestamp reply */
# define ICMPT_INFORQ	15		/* information request */
# define ICMPT_INFORP	16		/* information reply */
# define ICMPT_MASKRQ	17		/* mask request */
# define ICMPT_MASKRP	18		/* mask reply */

struct icmp_dgram
{
	char		type;		/* ICMP message type */
	char		code;		/* ICMP message code */
	short		chksum;		/* checksum */
	union {
		struct {		/* ICMPT_ECHOR? */
			short	id;	/* echo id */
			short	seq;	/* echo sequence number */
		} echo;
		struct {		/* ICMPT_PARAMP */
			char	ptr;	/* parameter problem pointer */
			char	pad[3];	/* padding */
		} param;
		ulong		redir_gw; /* ICMPT_REDIR, gateway address */
		long		zero;	/* must be zero */
	} u;
	union
	{
		char		bytes[0];	/* start of data */
		long		longs[0];
	} data;
};

long	icmp_errno (short, short);
long	icmp_send (short, short, ulong, BUF *, BUF *);
void	icmp_init (void);
short	icmp_dontsend (short, BUF *);


# endif /* _icmp_h */
