#ifndef _SLIP_H
#define _SLIP_H

/*
 * Slip flags, settable via SIOCSLNKFLAGS
 */
#define	SLF_ESC		0x01		/* next char is escaped */
#define SLF_DROP	0x02		/* drop this packet */
#define SLF_LINKED	0x04		/* interface is linked to device */
#define SLF_COMPRESS	0x08		/* turn on VJ compression */
#define SLF_AUTOCOMP	0x10		/* enable comp. on TCP_UNCOMP. frame */
#define SLF_COMPCID	0x20		/* enable CID compression */
#define SLF_USRMASK	(SLF_COMPRESS|SLF_AUTOCOMP|SLF_COMPCID)

/*
 * 16 byte pseudo header for BPF
 */
#define SLIP_HDRLEN	16

/* Offsets to special fields in the header */
#define SLX_DIR		0		/* SLIPDIR_IN or SLIPDIR_OUT */
#define SLX_CHDR	1		/* 15 bytes compressed header */
#define CHDR_LEN	15		/* comp header length */

#define SLIPDIR_IN	0
#define SLIPDIR_OUT	1

#endif
