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
 * xdr.c functions dealing with the XDR standard
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
xdr_string (xdrs *x, const char **cpp, long maxlen)
{
	long rawlen;
	union { const char **cc; char **c; } cp;

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

		cp.cc = cpp;
		memcpy(*cp.c, x->current, l);
		*(*cp.c + l) = '\0';
#if 0
 		memcpy (*cpp, x->current, l);
		*(*cpp + l) = '\0';
#endif
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
		cp.cc = cpp;
		memcpy (x->current, *cp.c/* *cpp */, l);
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
xdr_opaque (xdrs *x, const opaque **opp, long *len, long maxlen)
{
	long rawlen;

	if (x->length < sizeof (ulong))
		return FALSE;

	if (XDR_DECODE == x->op)
	{
		union { const char **cc; char **c; } cp;
		ulong l = *(ulong *) x->current;

		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);
		*len = l;
		rawlen = (l + 3) & ~0x03L;
		if (l > maxlen)
			return FALSE;
		if (x->length < rawlen)
			return FALSE;

		cp.cc = opp;
		memcpy (*cp.c/* *opp */, x->current, l);
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

/* 64 bit integers.
 *
 * NFS3 uses these for file sizes, offsets, file ids, fsids and directory
 * cookies. XDR transmits them as two 32 bit words, most significant word
 * first (RFC 4506, "Hyper Integer and Unsigned Hyper Integer").
 */
bool_t
xdr_uint64 (xdrs *x, uint64 *val)
{
	ulong *p;

	if (x->length < 2 * (long) sizeof (ulong))
		return FALSE;

	p = (ulong *) x->current;

	if (XDR_DECODE == x->op)
	{
		*val = ((uint64) p[0] << 32) | (uint64) p[1];
	}
	else if (XDR_ENCODE == x->op)
	{
		p[0] = (ulong) (*val >> 32);
		p[1] = (ulong) (*val & 0xffffffffUL);
	}
	else if (XDR_FREE == x->op)
		return TRUE;
	else
		return FALSE;

	x->current += 2 * sizeof (ulong);
	x->length -= 2 * sizeof (ulong);

	return TRUE;
}

bool_t
xdr_int64 (xdrs *x, int64 *val)
{
	union { int64 *s; uint64 *u; } p;

	p.s = val;
	return xdr_uint64 (x, p.u);
}

/* Variable sized opaque data that is stored in a fixed size buffer
 * supplied by the caller (used for nfs_fh3). On decoding the length is
 * checked against maxlen, on encoding the caller has to provide it.
 */
bool_t
xdr_varopaque (xdrs *x, opaque *val, ulong *len, long maxlen)
{
	long rawlen;

	if (x->length < (long) sizeof (ulong))
		return FALSE;

	if (XDR_DECODE == x->op)
	{
		ulong l = *(ulong *) x->current;

		if (l > (ulong) maxlen)
			return FALSE;

		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);

		rawlen = XDR_ROUNDUP (l);
		if (x->length < rawlen)
			return FALSE;

		memcpy (val, x->current, l);
		*len = l;

		x->current += rawlen;
		x->length -= rawlen;

		return TRUE;
	}
	else if (XDR_ENCODE == x->op)
	{
		ulong l = *len;

		if (l > (ulong) maxlen)
			return FALSE;

		*(ulong *) x->current = l;
		x->current += sizeof (ulong);
		x->length -= sizeof (ulong);

		rawlen = XDR_ROUNDUP (l);
		if (x->length < rawlen)
			return FALSE;

		if (rawlen > (long) l)
		{
			/* clear the fringe at the end of the buffer
			 * NOTE: we know that rawlen is at least 4
			 */
			*(long *)(x->current + rawlen - 4) = 0L;
		}

		memcpy (x->current, val, l);

		x->current += rawlen;
		x->length -= rawlen;

		return TRUE;
	}
	else if (XDR_FREE == x->op)
		return TRUE;

	return FALSE;
}

/* Skip over `len' bytes of the stream. Needed to step over the parts of
 * an NFS3 reply we are not interested in (e.g. the write verifier).
 */
bool_t
xdr_skip (xdrs *x, long len)
{
	if (x->length < len)
		return FALSE;

	x->current += len;
	x->length -= len;

	return TRUE;
}
