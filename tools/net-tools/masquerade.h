/*
 * IP masquerading support for MiNTNet.
 *
 * Started 10/5/1999 Mario Becroft.
 */

#define MASQ_NUM_PORTS 4096
#define MASQ_BASE_PORT 60000

#define MASQ_MAGIC	0x4D415351
#define MASQ_VERSION	0x00000001

#define MASQ_TIME	(*((unsigned long * volatile)0x4BA))

typedef struct port_db_record
{
	unsigned short num;
	unsigned long masq_addr;
	unsigned short masq_port; /* src port of outgoing packets of masqueraded machine             */
	unsigned short dst_port;  /* dst port of outgoing packets of masqueraded machine (tcp only!) */
	unsigned char proto;
	unsigned long modified;
	unsigned long timeout;
	         long offs;
	         long prev_offs;
	unsigned long seq;
	
	struct port_db_record *next_port;
} PORT_DB_RECORD;

typedef struct
{
	unsigned long magic;
	unsigned long version;
	unsigned long addr;
	unsigned long mask;
	unsigned long flags;
	unsigned long tcp_first_timeout;
	unsigned long tcp_ack_timeout;
	unsigned long tcp_fin_timeout;
	unsigned long udp_timeout;
	unsigned long icmp_timeout;
	PORT_DB_RECORD *port_db[MASQ_NUM_PORTS];
	PORT_DB_RECORD *redirection_db;
} MASQ_GLOBAL_INFO;

#define MASQ_ENABLED	0x01
#define MASQ_LOCAL_PACKETS	0x02
#define MASQ_DEFRAG_ALL	0x04
