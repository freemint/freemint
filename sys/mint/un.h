/*
 *	Definitions for the UNIX communication domain.
 *
 *	09/26/93, kay roemer.
 */

# ifndef _un_h
# define _un_h


struct sockaddr_un
{
	short	sun_family;
	char	sun_path[128];
};

# define UN_PATH_OFFSET	((short)((struct sockaddr_un *) 0)->sun_path)
# define UN_ADDR_SIZE	(sizeof (struct sockaddr_un))

/* Possible unix socket flags */
# define UN_ISBOUND	0x0001
# define UN_ISCONNECTED	0x0002


# endif /* _un_h */
