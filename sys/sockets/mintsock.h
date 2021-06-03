/*
 *	Definitions for the MiNT specific ioctl() device driver interface
 *	to the socket network access protocol.
 *
 *	09/24/93, kay roemer.
 */

#ifndef _sockets_mintsock_h
#define _sockets_mintsock_h


#define SOCKDEV "U:\\dev\\socket"
#define UN_OFFSET ((short)(int)((struct sockaddr_un *)0)->sun_path)


/* perform a socket system call, via ioctl() */
#define SOCKETCALL	(('S' << 8) | 100)

/* socket call command names, passed in the `cmd' field of *_cmd structs */
enum so_cmd {
	SOCKET_CMD = 0,
	SOCKETPAIR_CMD,
	BIND_CMD,
	LISTEN_CMD,
	ACCEPT_CMD,
	CONNECT_CMD,
	GETSOCKNAME_CMD,
	GETPEERNAME_CMD,
	SEND_CMD,
	SENDTO_CMD,
	RECV_CMD,
	RECVFROM_CMD,
	SETSOCKOPT_CMD,
	GETSOCKOPT_CMD,
	SHUTDOWN_CMD,
	SENDMSG_CMD,
	RECVMSG_CMD
};

/* used to extract the `cmd' field from the *_cmd structs */
struct generic_cmd {
	short	cmd;
	char	data[10];
};

/* structures passed to the SOCKETCALL ioctl() as an argument */
struct socket_cmd {
	short	cmd;
	short	domain;
	short	type;
	short	protocol;
};

struct socketpair_cmd {
	short	cmd;
	short	domain;
	short	type;
	short	protocol;
};

struct bind_cmd {
	short	cmd;
	void	*addr;
	short	addrlen;
};
		
struct listen_cmd {
	short	cmd;
	short	backlog;
};

struct accept_cmd {
	short	cmd;
	void	*addr;
	short	*addrlen;
};

struct connect_cmd {
	short	cmd;
	void	*addr;
	short	addrlen;
};

struct getsockname_cmd {
	short	cmd;
	void	*addr;
	short	*addrlen;
};

struct getpeername_cmd {
	short	cmd;
	void	*addr;
	short	*addrlen;
};

struct send_cmd {
	short	cmd;
	const void *buf;
	long	buflen;
	short	flags;
};

struct sendto_cmd {
	short	cmd;
	const void *buf;
	long	buflen;
	short	flags;
	const void *addr;
	short	addrlen;
};

struct recv_cmd {
	short	cmd;
	void	*buf;
	long	buflen;	
	short	flags;
};

struct recvfrom_cmd {
	short	cmd;
	void	*buf;
	long	buflen;
	short	flags;
	void	*addr;
	short	*addrlen;
};

struct setsockopt_cmd {
	short	cmd;
	short	level;
	short	optname;
	void	*optval;
	long	optlen;
};

struct getsockopt_cmd {
	short	cmd;
	short	level;
	short	optname;
	void	*optval;
	long	*optlen;
};

struct shutdown_cmd {
	short	cmd;
	short	how;
};

struct sendmsg_cmd {
	short	cmd;
	const void *msg;
	short	flags;
};

struct recvmsg_cmd {
	short	cmd;
	void	*msg;
	short	flags;
};


#endif /* _sockets_mintsock_h */
