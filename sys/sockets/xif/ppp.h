/*
 *	PPP specific ioctl's and other stuff.
 *
 *	06/26/94, Kay Roemer.
 *
 */

# ifndef _ppp_h
# define _ppp_h


# define PPPIOCGFLAGS	(('p' << 8) | 100)	/* set config flags */
# define PPPIOCSFLAGS	(('p' << 8) | 101)	/* get config flags */
# define PPPIOCSRMAP	(('p' << 8) | 102)	/* set rcv asynch. char map */
# define PPPIOCGRMAP	(('p' << 8) | 103)	/* get rcv asynch. char map */
# define PPPIOCSXMAP	(('p' << 8) | 104)	/* set snd asynch. char map */
# define PPPIOCGXMAP	(('p' << 8) | 105)	/* get snd asynch. char map */
# define PPPIOCGMTU	(('p' << 8) | 106)	/* get max. send unit */
# define PPPIOCSMTU	(('p' << 8) | 107)	/* set max. send unit */
# define PPPIOCGMRU	(('p' << 8) | 108)	/* get max. recv unit */
# define PPPIOCSMRU	(('p' << 8) | 109)	/* set max. recv unit */
# define PPPIOCGPGRP	(('p' << 8) | 110)	/* get pid to send SIGIO to */
# define PPPIOCSPGRP	(('p' << 8) | 111)	/* set pid to send SIGIO to */
# define PPPIOCSSLOTS	(('p' << 8) | 112)	/* set # of VJ comp. slots */

/*
 * Option codes for PPPIOC[SG]FLAGS.
 */
# define PPPO_PROT_COMP	0x00000001	/* enable protocol compression */
# define PPPO_ADDR_COMP	0x00000002	/* enable addr/control compression */
# define PPPO_IP_DOWN	0x00000004	/* send IP pack. to /dev/ppp? */
# define PPPO_COMPRESS	0x00000008	/* enable VJ compression */
# define PPPO_AUTOCOMP	0x00000010	/* enable comp. on TCP_UNCOMP. frame */
# define PPPO_COMPCID	0x00000020	/* turn on conn. ID compression */

/*
 * Some notable PPP protocols.
 */
# define PPPPROTO_IP		0x0021	/* IP */
# define PPPPROTO_LCP		0xc021	/* Link Control Protocol */
# define PPPPROTO_IPCP		0x8021	/* IP Control Protocol */
# define PPPPROTO_COMP_TCP 	0x002d	/* VJ compressed TCP */
# define PPPPROTO_UNCOMP_TCP	0x002f	/* VJ uncompressed TCP */


# endif /* _ppp_h */
