/*
 *	Functions for copying data from/to io vectors to/from one
 *	continuous memory block.
 *
 *	PROBLEM: Should buf2iov_cpy change `iov_size' of the
 *	last element of iov[] to reflect the number of bytes
 *	actually copied into this buffer ??
 *
 *	01/01/94, kay roemer.
 */

# include "global.h"

# include "iov.h"


long
iov2buf_cpy (char *buf, long nbytes, struct iovec *iov, short niov, long skip)
{
	long cando, todo = nbytes;
	
	if (niov <= 0 || todo <= 0 || skip < 0)
		return 0;
	
	for (; skip > 0 && niov; ++iov, --niov)
		skip -= iov->iov_len;
	
	if (skip < 0)
	{
		cando = MIN (-skip, todo);
		memcpy (buf, iov[-1].iov_base + skip + iov[-1].iov_len, cando);
		buf  += cando;
		todo -= cando;
	}
	
	for (; todo > 0 && niov; ++iov, --niov)
	{
		cando = MIN (todo, iov->iov_len);
		memcpy (buf, iov->iov_base, cando);
		todo -= cando;
		buf  += cando;
	}
	
	return (nbytes - todo);
}

long
buf2iov_cpy (char *buf, long nbytes, struct iovec *iov, short niov, long skip)
{
	long cando, todo = nbytes;
	
	if (niov <= 0 || todo <= 0 || skip < 0)
		return 0;
	
	for (; skip > 0 && niov; ++iov, --niov)
		skip -= iov->iov_len;
	
	if (skip < 0)
	{
		cando = MIN (-skip, todo);
		memcpy (iov[-1].iov_base + skip + iov[-1].iov_len, buf, cando);
		buf  += cando;
		todo -= cando;
	}
	
	for (; todo > 0 && niov; ++iov, --niov)
	{
		cando = MIN (todo, iov->iov_len);
		memcpy (iov->iov_base, buf, cando);
		todo -= cando;
		buf  += cando;
	}
	
	return (nbytes - todo);
}
