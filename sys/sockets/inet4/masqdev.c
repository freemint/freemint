/*
 *	This file implements /dev/masquerade, a device for controlling the IP
 *	masquerading features.
 *
 *	Started 10/5/1999 Mario Becroft.
 */

# include "masqdev.h"

# include "if.h"
# include "in.h"
# include "masquerade.h"
# include "route.h"

# include "dummydev.h"
# include "net.h"
# include "socket.h"
# include "util.h"

# include <mint/file.h>


static long masqdev_read  (FILEPTR *, char *, long);
static long masqdev_write (FILEPTR *, const char *, long);

static DEVDRV masqdev =
{
	open:		dummydev_open,
	write:		masqdev_write,
	read:		masqdev_read,
	lseek:		dummydev_lseek,
	ioctl:		dummydev_ioctl,
	datime:		dummydev_datime,
	close:		dummydev_close,
	select:		dummydev_select,
	unselect:	dummydev_unselect
};

static struct dev_descr masqdev_descr =
{
	driver:		&masqdev,
	fmode:		S_IFCHR | S_IRUSR | S_IWUSR
};

static char masqdev_name[] = "u:\\dev\\masquerade";

long
masqdev_init (void)
{
	return dummydev_init (masqdev_name, &masqdev_descr);
}

static int record = 0;
static PORT_DB_RECORD *redirection = NULL;
static PORT_DB_RECORD *prev_redir = NULL;

static long
masqdev_read (FILEPTR *fp, char *buf, long nbytes)
{
	switch (fp->pos)
	{
		case 0:
			if (nbytes >= sizeof (masq.magic))
			{
				memcpy (buf, &masq.magic, sizeof (masq.magic));
				return sizeof (masq.magic);
			}
			break;
		case 1:
			if (nbytes >= sizeof (masq.version))
			{
				memcpy (buf, &masq.version, sizeof (masq.version));
				return sizeof (masq.version);
			}
			break;
		case 2:
			if (nbytes >= sizeof (masq.addr))
			{
				memcpy (buf, &masq.addr, sizeof (masq.addr));
				return sizeof (masq.addr);
			}
			break;
		case 3:
			if (nbytes >= sizeof (masq.mask))
			{
				memcpy (buf, &masq.mask, sizeof (masq.mask));
				return sizeof (masq.mask);
			}
			break;
		case 4:
			if (nbytes >= sizeof (masq.flags))
			{
				memcpy (buf, &masq.flags, sizeof (masq.flags));
				return sizeof (masq.flags);
			}
			break;
		case 5:
			if (nbytes >= sizeof (masq.tcp_first_timeout))
			{
				memcpy (buf, &masq.tcp_first_timeout, sizeof (masq.tcp_first_timeout));
				return sizeof (masq.tcp_first_timeout);
			}
			break;
		case 6:
			if (nbytes >= sizeof (masq.tcp_ack_timeout))
			{
				memcpy (buf, &masq.tcp_ack_timeout, sizeof (masq.tcp_ack_timeout));
				return sizeof (masq.tcp_ack_timeout);
			}
			break;
		case 7:
			if (nbytes >= sizeof (masq.tcp_fin_timeout))
			{
				memcpy (buf, &masq.tcp_fin_timeout, sizeof (masq.tcp_fin_timeout));
				return sizeof (masq.tcp_fin_timeout);
			}
			break;
		case 8:
			if (nbytes >= sizeof (masq.udp_timeout))
			{
				memcpy (buf, &masq.udp_timeout, sizeof (masq.udp_timeout));
				return sizeof (masq.udp_timeout);
			}
			break;
		case 9:
			if (nbytes >= sizeof (masq.icmp_timeout))
			{
				memcpy (buf, &masq.icmp_timeout, sizeof (masq.icmp_timeout));
				return sizeof (masq.icmp_timeout);
			}
			break;
		case 50:
			if (nbytes >= sizeof (ulong))
			{
				ulong time;
				time = MASQ_TIME;
				memcpy (buf, &time, sizeof (ulong));
				return sizeof (ulong);
			}
		case 100:
			record = 0;
			fp->pos += 1;
		case 101:
			if (nbytes >= sizeof (PORT_DB_RECORD))
			{
				while (record < MASQ_NUM_PORTS && !masq.port_db[record])
					record += 1;
				if (record >= MASQ_NUM_PORTS)
					return 0;
				memcpy (buf, masq.port_db[record], sizeof (PORT_DB_RECORD));
				record += 1;
				return sizeof (PORT_DB_RECORD);
			}
			break;
		case 200:
			redirection = masq.redirection_db;
			fp->pos += 1;
		case 201:
			if (nbytes >= sizeof (PORT_DB_RECORD))
			{
				if (!redirection)
					return 0;
				memcpy (buf, redirection, sizeof (PORT_DB_RECORD));
				prev_redir = redirection;
				redirection = redirection->next_port;
				return sizeof (PORT_DB_RECORD);
			}
			break;
	}
	
	return 0;
}

static long
masqdev_write (FILEPTR *fp, const char *buf, long nbytes)
{
	switch (fp->pos)
	{
		case 2:
			if (nbytes == sizeof (masq.addr))
			{
				memcpy (&masq.addr, buf, sizeof (masq.addr));
				return sizeof (masq.addr);
			}
			break;
		case 3:
			if (nbytes == sizeof (masq.mask))
			{
				memcpy (&masq.mask, buf, sizeof (masq.mask));
				return sizeof (masq.mask);
			}
			break;
		case 4:
			if (nbytes == sizeof (masq.flags))
			{
				memcpy (&masq.flags, buf, sizeof (masq.flags));
				return sizeof (masq.flags);
			}
			break;
		case 5:
			if (nbytes == sizeof (masq.tcp_first_timeout))
			{
				memcpy (&masq.tcp_first_timeout, buf, sizeof (masq.tcp_first_timeout));
				return sizeof (masq.tcp_first_timeout);
			}
			break;
		case 6:
			if (nbytes == sizeof (masq.tcp_ack_timeout))
			{
				memcpy (&masq.tcp_ack_timeout, buf, sizeof (masq.tcp_ack_timeout));
				return sizeof (masq.tcp_ack_timeout);
			}
			break;
		case 7:
			if (nbytes == sizeof (masq.tcp_fin_timeout))
			{
				memcpy (&masq.tcp_fin_timeout, buf, sizeof (masq.tcp_fin_timeout));
				return sizeof (masq.tcp_fin_timeout);
			}
			break;
		case 8:
			if (nbytes == sizeof (masq.udp_timeout))
			{
				memcpy (&masq.udp_timeout, buf, sizeof (masq.udp_timeout));
				return sizeof (masq.udp_timeout);
			}
			break;
		case 9:
			if (nbytes == sizeof (masq.icmp_timeout))
			{
				memcpy (&masq.icmp_timeout, buf, sizeof (masq.icmp_timeout));
				return sizeof (masq.icmp_timeout);
			}
			break;
		case 102:
			if (nbytes == 5 && strncmp ("purge", buf, 5 ) == 0 )
			{
				purge_port_records ();
				return 5;
			}
		case 200:
			if (nbytes == sizeof (PORT_DB_RECORD) && (redirection = new_redirection()))
			{
				PORT_DB_RECORD *next;
				next = redirection->next_port;
				memcpy (redirection, buf, sizeof (PORT_DB_RECORD));
				redirection->next_port = next;
				return sizeof (PORT_DB_RECORD);
			}
			break;
		case 201:
			if (nbytes == sizeof (prev_redir->num) && prev_redir)
			{
				ushort port;
				
				memcpy (&port, buf, sizeof (port));
				
				if (port == prev_redir->num)
				{
					delete_redirection (prev_redir);
					return sizeof (port);
				}
			}
			break;
	}
	
	return 0;
}
