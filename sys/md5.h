/*
 * $Id$
 * 
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 * 
 * 
 * md5.h dated 99/09/26
 * 
 * Constants and prototypes for the MD5-message digest algorithm. Original file
 * by Colin Plumb, with minor modifications.
 * 
 */

# ifndef _md5_h
# define _md5_h

# include "mint/mint.h"


# ifdef CRYPTO_CODE

struct MD5Context
{
	__u32	buf[4];
	__u32	bits[2];
	uchar	in[64];
};

void MD5Init		(struct MD5Context *context);
void MD5Update		(struct MD5Context *context, uchar const *buf, ushort len);
void MD5Final		(uchar digest[16], struct MD5Context *context);
void MD5Transform	(__u32 buf[4], __u32 const in[16]);

# endif /* CRYPTO_CODE */


# endif /* _md5_h */
