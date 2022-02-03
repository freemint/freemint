
# ifndef _netinfo_h
# define _netinfo_h

# include "buf.h"
# include "inet4/if.h"


struct netinfo
{
	BUF *	(*_buf_alloc) (ulong, ulong, short);
	void	(*_buf_free) (BUF *, short);
	BUF *	(*_buf_reserve) (BUF *, long, short);
	void	(*_buf_deref) (BUF *, short);
	
	short	(*_if_enqueue) (struct ifq *, BUF *, short);
	BUF *	(*_if_dequeue) (struct ifq *);
	long	(*_if_register) (struct netif *);
	short	(*_if_input) (struct netif *, BUF *, long, short);
	void	(*_if_flushq) (struct ifq *);
	
	short	(*_in_chksum) (void *, short);
	short	(*_if_getfreeunit) (char *);
	
	BUF *	(*_eth_build_hdr) (BUF *, struct netif *, const char *, short);
	short	(*_eth_remove_hdr) (BUF *);
	
	const char *fname;
	
	long	(*_bpf_input) (struct netif *, BUF *);

	/* added for hotplug, i.e. USB */
	long	(*_if_deregister) (struct netif *);

	/* used by MagiCNet */
	void *slip_pd;

	long	reserved[3];
};

# ifndef NETINFO
# define NETINFO netinfo

extern struct netinfo *NETINFO;

# define buf_alloc	(*NETINFO->_buf_alloc)
# define buf_free	(*NETINFO->_buf_free)
# define buf_reserve	(*NETINFO->_buf_reserve)
# define buf_deref	(*NETINFO->_buf_deref)

# define if_enqueue	(*NETINFO->_if_enqueue)
# define if_dequeue	(*NETINFO->_if_dequeue)
# define if_register	(*NETINFO->_if_register)
# define if_input	(*NETINFO->_if_input)
# define if_flushq	(*NETINFO->_if_flushq)
# define if_getfreeunit	(*NETINFO->_if_getfreeunit)

# define in_chksum	(*NETINFO->_in_chksum)

# define eth_build_hdr	(*NETINFO->_eth_build_hdr)
# define eth_remove_hdr	(*NETINFO->_eth_remove_hdr)

# define bpf_input	(*NETINFO->_bpf_input)
# define if_deregister	(*NETINFO->_if_deregister)
# endif


# endif /* _netinfo_h */
