/*
 * Adopted to Mint-Net 1994, Kay Roemer.
 *
 * Modified to support Pure-C, Thorsten Otto.
 */

/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "stsocket.h"

/*
 * Expand compressed domain name 'comp_dn' to full domain name.
 * 'msg' is a pointer to the begining of the message,
 * 'eomorig' points to the first location after the message,
 * 'exp_dn' is a pointer to a buffer of size 'length' for the result.
 * Return size of compressed name or -1 if there was an error.
 */
int dn_expand(const uint8_t *msg, const uint8_t *eomorig, const uint8_t *comp_dn, char *exp_dn, int length)
{
	const uint8_t *cp;
	char *dn;
	int n;
	int c;
	char *eom;
	int len = -1;
	int checked = 0;

	dn = exp_dn;
	cp = comp_dn;
	eom = exp_dn + length;

	/*
	 * fetch next label in domain name
	 */
	while ((n = *cp++) != 0)
	{
		/*
		 * Check for indirection
		 */
		switch (n & INDIR_MASK)
		{
		case 0:
			if (dn != exp_dn)
			{
				if (dn >= eom)
					return -1;
				*dn++ = '.';
			}
			if (dn + n >= eom)
				return -1;
			checked += n + 1;
			while (--n >= 0)
			{
				if ((c = *cp++) == '.')
				{
					if (dn + n + 2 >= eom)
						return -1;
					*dn++ = '\\';
				}
				*dn++ = c;
				if (cp >= eomorig)		/* out of range */
					return -1;
			}
			break;

		case INDIR_MASK:
			if (len < 0)
				len = (int)(cp - comp_dn + 1);
			cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			if (cp < msg || cp >= eomorig)	/* out of range */
				return -1;
			checked += 2;
			/*
			 * Check for loops in the compressed name;
			 * if we've looked at the whole message,
			 * there must be a loop.
			 */
			if (checked >= eomorig - msg)
				return -1;
			break;

		default:
			return -1;				/* flag error */
		}
	}
	*dn = '\0';
	if (len < 0)
		len = (int)(cp - comp_dn);
	return len;
}


/*
 * Search for expanded name from a list of previously compressed names.
 * Return the offset from msg if found or -1.
 * dnptrs is the pointer to the first name on the list,
 * not the pointer to the start of the message.
 */
static int dn_find(const char *exp_dn, uint8_t *msg, uint8_t **dnptrs, uint8_t **lastdnptr)
{
	const char *dn;
	uint8_t *cp;
	uint8_t **cpp;
	int n;
	uint8_t *sp;

	for (cpp = dnptrs; cpp < lastdnptr; cpp++)
	{
		dn = exp_dn;
		sp = cp = *cpp;
		while ((n = *cp++) != 0)
		{
			/*
			 * check for indirection
			 */
			switch (n & INDIR_MASK)
			{
			case 0:					/* normal case, n == len */
				while (--n >= 0)
				{
					if (*dn == '.')
						goto next;
					if (*dn == '\\')
						dn++;
					if ((uint8_t) *dn++ != *cp++)
						goto next;
				}
				if ((n = *dn++) == '\0' && *cp == '\0')
					return (int)(sp - msg);
				if (n == '.')
					continue;
				goto next;

			default:					/* illegal type */
				return -1;

			case INDIR_MASK:			/* indirection */
				cp = msg + (((n & 0x3f) << 8) | *cp);
				break;
			}
		}
		if (*dn == '\0')
			return (int)(sp - msg);
	  next:;
	}
	return -1;
}


/*
 * Compress domain name 'exp_dn' into 'comp_dn'.
 * Return the size of the compressed name or -1.
 * 'length' is the size of the array pointed to by 'comp_dn'.
 * 'dnptrs' is a list of pointers to previous compressed names. dnptrs[0]
 * is a pointer to the beginning of the message. The list ends with NULL.
 * 'lastdnptr' is a pointer to the end of the arrary pointed to
 * by 'dnptrs'. Side effect is to update the list of pointers for
 * labels inserted into the message as we compress the name.
 * If 'dnptr' is NULL, we don't try to compress names. If 'lastdnptr'
 * is NULL, we don't update the list.
 */
int dn_comp(const char *exp_dn, uint8_t *comp_dn, int length, uint8_t **dnptrs, uint8_t **lastdnptr)
{
	uint8_t *cp;
	const char *dn;
	int c, l;
	uint8_t **cpp;
	uint8_t **lpp;
	uint8_t *sp;
	uint8_t *eob;
	uint8_t *msg;

#ifdef __GNUC__
	/* shut up compiler; both are only used when msg != NULL, and in this case are initialized */
	lpp = 0;
	cpp = 0;
#endif
	dn = exp_dn;
	cp = comp_dn;
	eob = cp + length;
	if (dnptrs != NULL)
	{
		if ((msg = *dnptrs++) != NULL)
		{
			for (cpp = dnptrs; *cpp != NULL; cpp++)
				;
			lpp = cpp;					/* end of list to search */
		}
	} else
	{
		msg = NULL;
	}
	for (c = *dn++; c != '\0';)
	{
		/* look to see if we can use pointers */
		if (msg != NULL)
		{
			if ((l = dn_find(dn - 1, msg, dnptrs, lpp)) >= 0)
			{
				if (cp + 1 >= eob)
					return -1;
				*cp++ = (l >> 8) | INDIR_MASK;
				*cp++ = l % 256;
				return (int)(cp - comp_dn);
			}
			/* not found, save it */
			if (lastdnptr != NULL && cpp < lastdnptr - 1)
			{
				*cpp++ = cp;
				*cpp = NULL;
			}
		}
		sp = cp++;						/* save ptr to length byte */
		do
		{
			if (c == '.')
			{
				c = *dn++;
				break;
			}
			if (c == '\\')
			{
				if ((c = *dn++) == '\0')
					break;
			}
			if (cp >= eob)
			{
				if (msg != NULL)
					*lpp = NULL;
				return -1;
			}
			*cp++ = c;
		} while ((c = *dn++) != '\0');
		/* catch trailing '.'s but not '..' */
		if ((l = (int)(cp - sp - 1)) == 0 && c == '\0')
		{
			cp--;
			break;
		}
		if (l <= 0 || l > NS_MAXLABEL)
		{
			if (msg != NULL)
				*lpp = NULL;
			return -1;
		}
		*sp = l;
	}
	if (cp >= eob)
	{
		if (msg != NULL)
			*lpp = NULL;
		return -1;
	}
	*cp++ = '\0';
	return (int)(cp - comp_dn);
}


/*
 * Skip over a compressed domain name. Return the size or -1.
 */
int __dn_skipname(const uint8_t *comp_dn, const uint8_t *eom)
{
	const uint8_t *cp;
	int n;

	cp = comp_dn;
	while (cp < eom && (n = *cp++) != 0)
	{
		/*
		 * check for indirection
		 */
		switch (n & INDIR_MASK)
		{
		case 0:						/* normal case, n == len */
			cp += n;
			continue;
		default:						/* illegal type */
			return -1;
		case INDIR_MASK:				/* indirection */
			cp++;
		}
		break;
	}
	return (int)(cp - comp_dn);
}


/*
 * Routines to insert/extract short/long's. Must account for byte
 * order and non-alignment problems. This code at least has the
 * advantage of being portable.
 *
 * used by sendmail.
 */

uint16_t _getshort(const uint8_t *msgp)
{
	const uint8_t *p = msgp;
	uint16_t u;

	u = *p++ << 8;
	return u | *p;
}


#if 0 /* unused */
uint32_t _getlong(const uint8_t *msgp)
{
	const uint8_t *p = msgp;
	uint32_t u;

	u = *p++;
	u <<= 8;
	u |= *p++;
	u <<= 8;
	u |= *p++;
	u <<= 8;
	return u | *p;
}
#endif


void __putshort(uint16_t s, uint8_t *msgp)
{
	msgp[1] = s;
	msgp[0] = s >> 8;
}


void __putlong(unsigned long l, uint8_t *msgp)
{
	msgp[3] = l;
	msgp[2] = (l >>= 8);
	msgp[1] = (l >>= 8);
	msgp[0] = l >> 8;
}


