/*
 * Copyright 1993, 1994 by Ulrich Khn. All rights reserved.
 *
 * THIS PROGRAM COMES WITH ABSOLUTELY NO WARRANTY, NOT
 * EVEN THE IMPLIED WARRANTIES OF MERCHANTIBILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE. USE AT YOUR OWN
 * RISK.
 */

/*
 * xdr.c functions dealing with the XDR standard
 *
 * Copyright Ulrich Khn, 1993 
 */

# include "global.h"
# include "xdr.h"


static void *
dummy_alloc (long len)
{
	return NULL;
}

static void
dummy_free (void *addr)
{
	return;
}


bool_t
xdr_init (xdrs *s, char *buffer, long len, int op, MEMSVC *ms)
{
	s->data = s->current = buffer;
	s->length = len;
	s->op = op;
	
	if (ms)
	{
		s->x_malloc = ms->alloc;
		s->x_free = ms->free;
	}
	else
	{
		s->x_malloc = dummy_alloc;
		s->x_free = dummy_free;
	}
	
	return TRUE;
}

long
xdr_getpos (xdrs *x)
{
	if (x->length >= 0)
		return (long)(x->current) - (long)(x->data);
	
	return 0;
}

bool_t
xdr_setpos (xdrs *x, long pos)
{
	if (x->length <= pos)
		return FALSE;
	
	x->length -= pos;
	x->current += pos;
	
	return TRUE;
}


long *
xdr_inline (xdrs *x, long len)
{
	long *p;
	
	if (len <= x->length)
	{
		p = (long *) x->current;
		x->length -= len;
		x->current += len;
		return p;
	}
	
	return FALSE;
}

bool_t
xdr_void (xdrs *x, ...)
{
	/* nothing to do */
	if (x->length >= 0)
		return TRUE;
	
	return FALSE;
}

bool_t
xdr_long (xdrs *x, long *val)
{
	if (x->length < sizeof (long))
		return FALSE;
	
	if (XDR_DECODE == x->op)
		*val = *(long *) x->current;
	else if (XDR_ENCODE == x->op)
		*(long *) x->current = *val;
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
	
	x->current += sizeof (long);
	x->length -= sizeof (long);
	
	return TRUE;
}

bool_t
xdr_enum (xdrs *x, enum_t *val)
{
	if (x->length < sizeof (ulong))
		return FALSE;
	
	if (XDR_DECODE == x->op)
		*val = *(ulong *) x->current;
	else if (XDR_ENCODE == x->op)
		*(ulong *) x->current = *val;
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
	
	x->current += sizeof (ulong);
	x->length -= sizeof (ulong);
	
	return TRUE;
}

bool_t
xdr_bool (xdrs *x, bool_t *val)
{
	if (x->length < sizeof (ulong))
		return FALSE;
	
	if (XDR_DECODE == x->op)
		*val = *(ulong *) x->current;
	else if (XDR_ENCODE == x->op)
		*(ulong *) x->current = *val;
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
	
	x->current += sizeof (ulong);
	x->length -= sizeof (ulong);
	
	return TRUE;
}

bool_t
xdr_ulong (xdrs *x, ulong *val)
{
	if (x->length < sizeof (ulong))
		return FALSE;
	
	if (XDR_DECODE == x->op)
		*val = *(ulong *) x->current;
	else if (XDR_ENCODE == x->op)
		*(ulong *) x->current = *val;
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
	
	x->current += sizeof (ulong);
	x->length -= sizeof (ulong);
	
	return TRUE;
}

bool_t
xdr_string (xdrs *x, char **cpp, long maxlen)
{
	long rawlen;
	
	if (x->length < sizeof (ulong))
		return FALSE;
	
	if (XDR_DECODE == x->op)
	{
		ulong l = *(ulong *) x->current;

		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);
		rawlen = (l + 3) & ~0x03L;
		if (l > maxlen)
			return FALSE;
		if (x->length < rawlen)
			return FALSE;
		
 		memcpy (*cpp, x->current, l);
		*(*cpp + l) = '\0';
		x->current += rawlen;
		x->length -= rawlen;
		
		return TRUE;
	}
	else if (XDR_ENCODE == x->op)
	{
		ulong l = strlen (*cpp);

		*(ulong *) x->current = l;
		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);
		rawlen = (l + 3) & ~0x03L;
		if (l > maxlen)
			return FALSE;
		if (x->length < rawlen)
			return FALSE;

		if (rawlen > l)
		{
			/* clear the fringe at the end of the string.
			 * NOTE: we know here, that rawlen is at least 4!!
			 */
			*(long *)(x->current + rawlen - 4) = 0L;
		}
		
		memcpy (x->current, *cpp, l);
		x->current += rawlen;
		x->length -= rawlen;
		
		return TRUE;
	}
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
}

bool_t
xdr_opaque (xdrs *x, opaque **opp, long *len, long maxlen)
{
	long rawlen;
	
	if (x->length < sizeof (ulong))
		return FALSE;
	
	if (XDR_DECODE == x->op)
	{
		ulong l = *(ulong *) x->current;

		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);
		*len = l;
		rawlen = (l + 3) & ~0x03L;
		if (l > maxlen)
			return FALSE;
		if (x->length < rawlen)
			return FALSE;
		
		memcpy (*opp, x->current, l);
		x->current += rawlen;
		x->length -= rawlen;
		
		return TRUE;
	}
	else if (XDR_ENCODE == x->op)
	{
		ulong l = *len;
		
		*(ulong *) x->current = l;
		
		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);
		rawlen = (l + 3) & ~0x03L;
		if (l > maxlen)
			return FALSE;
		if (x->length < rawlen)
			return FALSE;
		
		if (rawlen > l)
		{
			/* clear the fringe at the end of the buffer
			 * NOTE: we know that rawlen is at least 4
			 */
			*(long *)(x->current + rawlen - 4) = 0L;
		}
		
		memcpy (x->current, *opp, l);
		x->current += rawlen;
		x->length -= rawlen;
		
		return TRUE;
	}
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
}

bool_t
xdr_fixedopaq (xdrs *x, opaque *val, long len)
{
	long rawlen;

	rawlen = (len + 3) & ~0x03;
	if (x->length < rawlen)
		return FALSE;
	
	if (XDR_DECODE == x->op)
	{
		memcpy (val, x->current, len);
	}
	else if (XDR_ENCODE == x->op)
	{
		if (rawlen > len)
		{
			/* clear the fringe at the end of the buffer
			 * NOTE: we know that rawlen is at least 4
			 */
			*(long *)(x->current + rawlen - 4) = 0L;
		}
		memcpy (x->current, val, len);
	}
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;
	
	x->current += rawlen;
	x->length -= rawlen;
	
	return TRUE;
}

bool_t
xdr_pointer (xdrs *x, char **objpp, long objlen, xdrproc_t proc)
{
	char *where = *objpp;
	bool_t more_data;
	
	more_data = (where != NULL);
	if (!xdr_bool (x, &more_data))
		return FALSE;
	if (!more_data)
	{
		*objpp = NULL;
		return TRUE;
	}
	
	return (*proc)(x, where);
}
