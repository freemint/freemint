/*
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 *
 * NFS version 3 (RFC 1813) support, derived from the NFS version 2
 * driver. See the file COPYING for copying and using conditions.
 */

/*
 * xdr.h functions dealing with the XDR standard (RFC 4506)
 *
 * Differences to the NFS2 version of this file:
 *
 * - bool_t and enum_t are 32 bit types now. The kernel is compiled with
 *   -mshort, so `int' is only 16 bits wide, which silently truncated every
 *   enum that does not fit into a short. NFS3 needs that room: the error
 *   codes of RFC 1813 start at 10001 and the cookie verifier handling
 *   compares full 32 bit words.
 * - xdr_uint64()/xdr_int64() have been added, as NFS3 uses 64 bit file
 *   sizes, offsets, file ids and directory cookies.
 * - xdr_varopaque() has been added for the variable sized opaque data of
 *   nfs_fh3.
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

typedef long bool_t;
typedef long enum_t;

/* 64 bit quantities as used all over the NFS3 protocol */
typedef unsigned long long uint64;
typedef long long          int64;


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
bool_t	xdr_uint64	(xdrs *x, uint64 *val);
bool_t	xdr_int64	(xdrs *x, int64 *val);
bool_t	xdr_string	(xdrs *x, const char **cpp, long maxlen);
bool_t	xdr_opaque	(xdrs *x, const opaque **opp, long *len, long maxlen);
bool_t	xdr_varopaque	(xdrs *x, opaque *val, unsigned long *len, long maxlen);
bool_t	xdr_fixedopaq	(xdrs *x, opaque *val, long fixedlen);
bool_t	xdr_skip	(xdrs *x, long len);
bool_t	xdr_pointer	(xdrs *x, char **objpp, long objlen, xdrproc_t proc);


/* number of bytes an XDR string/opaque of `len' bytes payload occupies
 * on the wire, including its length word
 */
# define XDR_ROUNDUP(len)	(((len) + 3L) & ~3L)
# define XDR_STRSIZE(len)	(BYTES_PER_XDR_UNIT + XDR_ROUNDUP(len))


/* these are some inline functions for simple data types
 * they expect that the data is memory aligned.
 * Be careful with these macros, as the type of the buffer
 * HAS TO BE long* !!!!
 */
# define IXDR_GET_LONG(buf)       ((long) *(buf)++)
# define IXDR_GET_ULONG(buf)      ((unsigned long) *(buf)++)
# define IXDR_GET_ENUM(buf)       ((enum_t) (IXDR_GET_ULONG (buf)))

# define IXDR_PUT_LONG(buf, val) { union { void *v; long *l; } p; p.v = buf; *p.l++ = (long)val; buf = p.v;}
# define IXDR_PUT_ULONG(buf, val) { union { void *v; unsigned long *l; } p; p.v = buf; *p.l++ = (unsigned long)val; buf = p.v;}
# define IXDR_PUT_ENUM(buf, val)  (IXDR_PUT_ULONG (buf, (unsigned long) val))


# endif /* _xdr_h */
