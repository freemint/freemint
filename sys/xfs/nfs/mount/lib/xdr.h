/*
 * Copyright 1993, 1994 by Ulrich KÅhn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * xdr.h functions dealing with the XDR standard
 *
 * Copyright Ulrich KÅhn, 1993 
 */

#ifndef XDR_H
#define XDR_H


#include <sys/types.h>



#ifndef NULL
#define NULL (void*)0L
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

typedef int bool_t;
typedef int enum_t;



#define BYTES_PER_XDR_UNIT   4


typedef struct
{
	int op;     /* operation to perform on this xdr stream */
#define XDR_ENCODE  0
#define XDR_DECODE  1
#define XDR_FREE    2 /* free a structure that was build via xdr functions */

	long length;   /* number of bytes to go until end of buffer */
	char *data;
	char *current;
	void *(*x_malloc)(long len);
	void (*x_free)(void *addr);
} xdrs;


typedef char opaque;

typedef struct
{
	void *(*alloc)(long len);
	void (*free)(void *addr);
} MEMSVC;


typedef bool_t (*xdrproc_t)(xdrs *x, caddr_t data);


bool_t xdr_init(xdrs *x, char *buffer, long length, int op, MEMSVC *ms);
long xdr_getpos(xdrs *x);
bool_t xdr_setpos(xdrs *x, long pos);
long *xdr_inline(xdrs *x, long len);

bool_t xdr_void(xdrs *x, ...);
bool_t xdr_long(xdrs *x, long *val);
bool_t xdr_enum(xdrs *x, enum_t *val);
bool_t xdr_bool(xdrs *x, bool_t *val);
bool_t xdr_ulong(xdrs *x, u_long *val);
bool_t xdr_string(xdrs *x, char **cpp, long maxlen);
bool_t xdr_opaque(xdrs *x, opaque **opp, long *len, long maxlen);
bool_t xdr_fixedopaq(xdrs *x, opaque *val, long fixedlen);
bool_t xdr_pointer(xdrs *x, char **objpp, long objlen, xdrproc_t proc);


/* these are some inline functions for simple data types
 * they expect that the data is memory aligned.
 * Be careful with these macros, as the type of the buffer
 * HAS TO BE long* !!!!
 */
#define IXDR_GET_LONG(buf)       ((long)*(buf)++)
#define IXDR_GET_ULONG(buf)      ((u_long)*(buf)++)
#define IXDR_GET_ENUM(buf)       ((int)(IXDR_GET_ULONG(buf)))

#define IXDR_PUT_LONG(buf, val)  ((long)*(buf)++ = (long)val)
#define IXDR_PUT_ULONG(buf, val) ((u_long)*(buf)++ = (u_long)val)
#define IXDR_PUT_ENUM(buf, val)  (IXDR_PUT_ULONG(buf, (u_long)val))


#endif
