/*
 *	This file contains definitions and prototypes for the structures
 *	and functions used throughout TCP.
 *
 *	04/17/94, Kay Roemer.
 */

# ifndef _tcp_h
# define _tcp_h

# include "global.h"

# include "buf.h"
# include "iov.h"
# include "timer.h"

# include "inet.h"


/*
 * Default and max. maximum segment size.
 */
# define TCP_MSS	1460	/* was 536 */
# define TCP_MAXMSS	2000
# define TCP_MINMSS	100

/*
 * Maximum segment lifetime. We chose to use 30 seconds, because it turns
 * out to be very unpleasant to wait 2 minutes (2*MSL when using 60 sec
 * as MSL) before the timewaiting socket goes away when you want to bind
 * a new socket to the same address.
 */
# define TCP_MSL	(30000/EVTGRAN)

/*
 * Minimum timeout between (and before) retransmissions. This has to be
 * greater than or equal to 200ms because of BSD's delayed acks.
 */
# define TCP_MINRETRANS	(200/EVTGRAN)

/*
 * Maximum retransmission timeout, should be greater than or equal to MSL.
 */
# define TCP_MAXRETRANS	(100000/EVTGRAN)

/*
 * Maximum timeout between window probes.
 */
# define TCP_MAXPROBE	(30000/EVTGRAN)

/*
 * Maximum number of retransmissions before giving up with ETIMEDOUT.
 */
# define TCP_MAXRETRY	15

/*
 * Time to delay acks. This is currently 15ms. Everything greater than
 * 20ms leads to *immense* performance losses over lo0 due to the way
 * MintNet handles input packets.
 */
# define TCP_ACKDELAY	(15/EVTGRAN)

/*
 * When no response from peer (when he is to respond) for this time
 * then close connection.
 */
# define TCP_CONN_TMOUT	(10L*60L*1000L/EVTGRAN)

/*
 * Dupack threshhold. When the TCP receives this many completely duplicate
 * acks it starts to resend some segments to keep the congestion window open.
 */
# define TCP_DUPTHRESH	3

/*
 * Dropped segments threshhold. When the TCP encounters this many dropped
 * retransmitted segments it resets the backoff. (because a line error
 * caused the packet to time out, not a too small rtt).
 */
# define TCP_DROPTHRESH	1

# define TCP_RESERVE	140
# define TCP_MINLEN	(sizeof (struct tcp_dgram))

# define TCP_DATA(th)	((char *)(th) + (th)->hdrlen * 4)
# define TCP_HDRLEN(th)	((th)->hdrlen * 4)

/* TCP datagram header */
struct tcp_dgram
{
	ushort		srcport;	/* source port */
	ushort		dstport;	/* destination port */
	long		seq;		/* sequence number */
	long		ack;		/* acknowledge number */
	ushort		hdrlen:4;	/* TCP header len in longs */
	ushort		flags:12;	/* segment flags */
# define TCPF_URG	0x020		/* segment contains urgent data */
# define TCPF_ACK	0x010		/* segment contains acknowledgement */
# define TCPF_PSH	0x008		/* push function */
# define TCPF_RST	0x004		/* reset connection */
# define TCPF_SYN	0x002		/* syncronice sequence numbers */
# define TCPF_FIN	0x001		/* finish connection */
# define TCPF_FREEME	0x800
	ushort		window;		/* window size */
	short		chksum;		/* checksum */
	ushort		urgptr;		/* urgent data offset rel to seq */
	char		data[0];	/* options/user data */
};

/* TCP options */
# define TCPOPT_EOL	0	/* end of option list */
# define TCPOPT_NOP	1	/* no operation */
# define TCPOPT_MSS	2	/* maximum segment size */

/* TCP setsockopt options */
# define TCP_NODELAY	1	/* disable Nagle algorithm */

/* Sequence space comparators */
# define SEQEQ(x, y)	((long)(x) == (long)(y))	/* (x == y) mod 2^32 */
# define SEQNE(x, y)	((long)(x) != (long)(y))	/* (x != y) mod 2^32 */
# define SEQLT(x, y)	((long)(x) - (long)(y) <  0)	/* (x <  y) mod 2^32 */
# define SEQLE(x, y)	((long)(x) - (long)(y) <= 0)	/* (x <= y) mod 2^32 */
# define SEQGT(x, y)	((long)(x) - (long)(y) >  0)	/* (x >  y) mod 2^32 */
# define SEQGE(x, y)	((long)(x) - (long)(y) >= 0)	/* (x >= y) mod 2^32 */

/* TCB states */
# define TCBS_CLOSED		0
# define TCBS_LISTEN		1
# define TCBS_SYNSENT		2
# define TCBS_SYNRCVD		3
# define TCBS_ESTABLISHED	4
# define TCBS_FINWAIT1		5
# define TCBS_FINWAIT2		6
# define TCBS_CLOSEWAIT		7
# define TCBS_LASTACK		8
# define TCBS_CLOSING		9
# define TCBS_TIMEWAIT		10

/* TCB output states */
# define TCBOS_IDLE		0
# define TCBOS_RETRANS		1
# define TCBOS_PERSIST		2
# define TCBOS_XMIT		3

/* TCB output events */
# define TCBOE_SEND		0
# define TCBOE_ACKRCVD		1
# define TCBOE_WNDOPEN		2
# define TCBOE_WNDCLOSE		3
# define TCBOE_TIMEOUT		4
# define TCBOE_DUPACK		5

# define TCB_OSTATE(t, e)	((*tcb_ostate[(t)->ostate]) (t, e))

/* TCP control block */
struct tcb
{
	struct in_data	*data;		/* backlink to inet socket */
	short		state;		/* TCB state, TCBS_* */
	short		ostate;		/* TCB output state, TCBOS_* */
	short		flags;		/* misc flags */
# define TCBF_PSH	0x01		/* PUSH seen */
# define TCBF_FIN	0x02		/* FIN seen */
# define TCBF_PASSIVE	0x04		/* tcb is forked off a server */
# define TCBF_DORTT	0x08		/* measuring RTT */
# define TCBF_NDELAY	0x10		/* disable nagle algorithm */
# define TCBF_DELACK	0x20		/* need delayed ack */
# define TCBF_ACKVALID	0x40		/* last_ack field valid */

	long		snd_isn;	/* initial send sequence number */
	long		snd_una;	/* oldest unacknowledged seq number */
	long		snd_nxt;	/* next seq number to send */
	long		snd_max;	/* max sent seq number */
	long		snd_wnd;	/* send window size */
	long		snd_cwnd;	/* congestion window size */
	long		snd_thresh;	/* cong. window threshold */
	long		snd_wndseq;	/* seq number of last window update */
	long		snd_wndack;	/* ack number of last window update */
	long		snd_wndmax;	/* max. send window seen on conn. */
	short		snd_ppw;	/* # of mss sized pkts that fit into
					   snd_wndmax */
	long		snd_mss;	/* send max segment size */
	long		snd_urg;	/* send urgent pointer */

	long		rcv_isn;	/* initial recv sequence number */
	long		rcv_nxt;	/* next seq number to recv */
	long		rcv_wnd;	/* receive window size */
	long		rcv_mss;	/* recv max segment size */
	long		rcv_urg;	/* receive urgent pointer */

	long		seq_psh;	/* sequence number of PUSH */
	long		seq_fin;	/* sequence number of FIN */
	long		seq_read;	/* sequence of next byte to read() */
	long		seq_uread;	/* sequence of next urg byte to read */
	long		seq_write;	/* sequence of next byte to write */

	long		rttseq;		/* calc RTT when this gets acked */
	long		rtt;		/* estimated round trip time */
	long		rttdev;		/* deviation in round trip time */
	short		backoff;	/* timer backoff */

	short		dupacks;	/* # of duplicate acks */

	short		nretrans;	/* number of retransmits */

	long		retrans_tmo;	/* retransmit timeout value */
	long		persist_tmo;	/* persist timeout value */
	long		keep_tmo;	/* keepalive timeout value */

	struct event	timer_evt;	/* persist/retransmit timer event */
	struct event	delete_evt;	/* tcb deletion timer event */
	struct event	ack_evt;	/* delayed ack event */

	long		last_recv;	/* time of last receive */
	long		last_ack;	/* time of last ack */
	long		artt, arttdev;
};

extern struct in_proto tcp_proto;


void	tcp_init	(void);
long	tcp_canread	(struct in_data *data);
long	tcp_canwrite	(struct in_data *data);


# endif /* _tcp_h */
