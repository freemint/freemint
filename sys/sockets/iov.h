/*
 *	Definitions to handle io vectors.
 *
 *	11/13/93, kay roemer.
 */

# ifndef _mint_iov_h
# define _mint_iov_h


/* Maximum number of elements in an io vector */
# define IOV_MAX	16

struct iovec
{
	char	*iov_base;
	long	iov_len;
};

long iov2buf_cpy (char *, long, struct iovec *, short, long);
long buf2iov_cpy (char *, long, struct iovec *, short, long);

static inline long
iov_size (struct iovec *iov, short n)
{
	register long size;

	if (n <= 0 || n > IOV_MAX)
		return -1;
	
	for (size = 0; n; ++iov, --n)
	{
		if (iov->iov_len < 0) return -1;
		size += iov->iov_len;
	}
	
	return size;
}


# endif /* _mint_iov_h */
