/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 * 
 * Modified for FreeMiNT CVS
 * by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

/*
 * xdr.h functions dealing with the XDR standard
 */

# ifndef _xdr_h
# define _xdr_h


typedef void *             caddr_t;

# ifndef FALSE
# define FALSE 0
# endif
 
# ifndef TRUE
# define TRUE  1
# endif


# define BYTES_PER_XDR_UNIT   4

typedef int bool_t;
typedef int enum_t;


typedef struct
{
	int	op;	/* operation to perform on this xdr stream */
# define XDR_ENCODE	0
# define XDR_DECODE	1
# define XDR_FREE	2
	
	long	length;	/* number of bytes to go until end of buffer */
	char	*data;
	char	*current;
	void *	(*x_malloc)(long len);
	void	(*x_free)(void *addr);
} xdrs;


typedef char opaque;

typedef struct
{
	void *	(*alloc)(long len);
	void	(*free)(void *addr);
} MEMSVC;


typedef bool_t (*xdrproc_t)(xdrs *x, caddr_t data);


bool_t	xdr_init	(xdrs *x, char *buffer, long length, int op, MEMSVC *ms);
long	xdr_getpos	(xdrs *x);
bool_t	xdr_setpos	(xdrs *x, long pos);
long *	xdr_inline	(xdrs *x, long len);

bool_t	xdr_void	(xdrs *x, ...);
bool_t	xdr_long	(xdrs *x, long *val);
bool_t	xdr_enum	(xdrs *x, enum_t *val);
bool_t	xdr_bool	(xdrs *x, bool_t *val);
bool_t	xdr_ulong	(xdrs *x, unsigned long *val);
bool_t	xdr_string	(xdrs *x, char **cpp, long maxlen);
bool_t	xdr_opaque	(xdrs *x, opaque **opp, long *len, long maxlen);
bool_t	xdr_fixedopaq	(xdrs *x, opaque *val, long fixedlen);
bool_t	xdr_pointer	(xdrs *x, char **objpp, long objlen, xdrproc_t proc);


/* these are some inline functions for simple data types
 * they expect that the data is memory aligned.
 * Be careful with these macros, as the type of the buffer
 * HAS TO BE long* !!!!
 */
# define IXDR_GET_LONG(buf)       ((long) *(buf)++)
# define IXDR_GET_ULONG(buf)      ((unsigned long) *(buf)++)
# define IXDR_GET_ENUM(buf)       ((int) (IXDR_GET_ULONG (buf)))

# define IXDR_PUT_LONG(buf, val)  ((long) *(buf)++ = (long) val)
# define IXDR_PUT_ULONG(buf, val) ((unsigned long) *(buf)++ = (unsigned long) val)
# define IXDR_PUT_ENUM(buf, val)  (IXDR_PUT_ULONG (buf, (unsigned long) val))


# endif /* _xdr_h */
