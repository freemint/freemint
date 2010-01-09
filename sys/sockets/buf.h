
# ifndef _buf_h
# define _buf_h

# include "global.h"


# define BUF_NORMAL		0
# define BUF_ATOMIC		1

# define BUF_RESERVE_START	1
# define BUF_RESERVE_END	2

# define BUF_LEAD_SPACE(b)	((long)(b)->dstart - (long)(b)->data)
# define BUF_TRAIL_SPACE(b)	((long)(b) + (b)->buflen - (long)(b)->dend)

typedef struct buf BUF;
struct buf
{
	ulong	buflen;		/* buffer len, including header */
	char	*dstart;	/* start of data */
	char	*dend;		/* end of data */
	BUF	*next;		/* next message */
	BUF	*prev;		/* previous message */
	BUF	*link3;		/* another next pointer */
	short	links;		/* usage counter */
	long	info;		/* aux info */

	BUF	*_n;		/* next buf in memory */
	BUF	*_p;		/* previous buf memory */
	BUF	*_nfree;	/* next free buf of same size */
	BUF	*_pfree;	/* previous free buf of same size */
	char	data[0];
};


long	buf_init (void);

BUF *	buf_alloc (ulong, ulong, short);
void	buf_free (BUF *, short);
BUF *	buf_reserve (BUF *, long, short);
void	buf_deref (BUF *, short);
BUF *	buf_clone (BUF *, short);

INLINE void
buf_ref (BUF *buf)
{
	buf->links++;
}


# endif /* _buf_h */
