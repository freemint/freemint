# ifndef _sockaddr_in_h_
# define _sockaddr_in_h_

/* internet address */
struct in_addr
{
        ulong s_addr;
};
/* structure describing an Internet socket address */
struct sockaddr_in
{
        short           sin_family;
        unsigned short  sin_port;
        struct in_addr  sin_addr;
        char            sin_zero[8];
};

# endif /* _sockaddr_in_h_ */
