/*
 * IP masquerading support for MiNTNet.
 *
 * Started 10/5/1999 Mario Becroft.
 */

# ifndef _masquerade_h
# define _masquerade_h

# include "global.h"

# include "buf.h"
# include "if.h"


# define MASQ_NUM_PORTS 	4096
# define MASQ_BASE_PORT 	60000

# define MASQ_MAGIC		0x4D415351
# define MASQ_VERSION		0x00000001

# define MASQ_TIME		(*((ulong * volatile) 0x4BA))

typedef struct port_db_record  PORT_DB_RECORD;
struct port_db_record
{
	ushort	num;
	ulong	masq_addr;
	ushort	masq_port; /* src port of outgoing packets of masqueraded machine             */
	ushort	dst_port;  /* dst port of outgoing packets of masqueraded machine (tcp only!) */
	uchar	proto;
	ulong	modified;
	ulong	timeout;
	long	offs;
	long	prev_offs;
	ulong	seq;
	
	PORT_DB_RECORD *next_port;
};

typedef struct
{
	ulong	magic;
	ulong	version;
	ulong	addr;
	ulong	mask;
	ulong	flags;
	ulong	tcp_first_timeout;
	ulong	tcp_ack_timeout;
	ulong	tcp_fin_timeout;
	ulong	udp_timeout;
	ulong	icmp_timeout;
	PORT_DB_RECORD *port_db[MASQ_NUM_PORTS];
	PORT_DB_RECORD *redirection_db;
} MASQ_GLOBAL_INFO;

# define MASQ_ENABLED		0x01
# define MASQ_LOCAL_PACKETS	0x02
# define MASQ_DEFRAG_ALL	0x04


extern MASQ_GLOBAL_INFO masq;

long			masqdev_init (void);

void			masq_init (void);
BUF *			masq_ip_input (struct netif *nif, BUF *buf);
PORT_DB_RECORD *	find_port_record (ulong addr, ushort src_port, ushort dst_port, uchar proto);
PORT_DB_RECORD *	new_port_record (void);
void			delete_port_record (PORT_DB_RECORD *record);
void			purge_port_records (void);
PORT_DB_RECORD *	new_redirection (void);
void			delete_redirection (PORT_DB_RECORD *record);
PORT_DB_RECORD *	find_redirection (ushort port);
PORT_DB_RECORD *	find_redirection_by_masq_port (ushort port);


# endif /* _masquerade_h */
