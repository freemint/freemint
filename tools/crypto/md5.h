/*
 * md5.h dated 99/09/26
 *
 * Constants and prototypes for the MD5-message digest algorithm. Original file
 * by Colin Plumb, with minor modifications.
 */

# ifndef _md5_h
# define _md5_h

# include "mytypes.h"


struct MD5Context
{
	ulong	buf[4];
	ulong	bits[2];
	uchar	in[64];
};

void MD5Init		(struct MD5Context *context);
void MD5Update		(struct MD5Context *context, uchar const *buf, ushort len);
void MD5Final		(uchar digest[16], struct MD5Context *context);
void MD5Transform	(ulong buf[4], ulong const in[16]);


# endif /* _md5_h */
