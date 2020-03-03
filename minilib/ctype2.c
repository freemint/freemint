/*
 *  CTYPE.C	Character classification and conversion
 */

/* Modified by Guido Flohr <guido@freemint.de>:
 * - Characters > 128 are control characters.
 * - iscntrl(EOF) should return false, argh, stupid but that's the
 *   the opinion of the majority.
 */

#define __NO_CTYPE

#include <ctype.h>
#include <limits.h>

#define _CTb _ISblank
#define _CTg _ISgraph

unsigned char const __libc_ctype2[UCHAR_MAX + 1] =
{
	0, 0, 0, 0,				/* 0x00..0x03 */
	0, 0, 0, 0,				/* 0x04..0x07 */
	0, _CTb, 0, 0,			/* 0x08..0x0B */
	0, 0, 0, 0,				/* 0x0C..0x0F */

	0, 0, 0, 0,				/* 0x10..0x17 */
	0, 0, 0, 0,				/* 0x14..0x17 */
	0, 0, 0, 0,				/* 0x18..0x1B */
	0, 0, 0, 0,				/* 0x1C..0x1F */

	_CTb, _CTg, _CTg, _CTg,			/* 0x20..0x23 */
	_CTg, _CTg, _CTg, _CTg,			/* 0x24..0x27 */
	_CTg, _CTg, _CTg, _CTg,			/* 0x28..0x2B */
	_CTg, _CTg, _CTg, _CTg,			/* 0x2C..0x2F */

	_CTg, _CTg,
	_CTg, _CTg,			/* 0x30..0x33 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x34..0x37 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x38..0x3B */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x3C..0x3F */

	_CTg, _CTg,
	_CTg, _CTg,			/* 0x40..0x43 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x44..0x47 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x48..0x4B */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x4C..0x4F */

	_CTg, _CTg,
	_CTg, _CTg,			/* 0x50..0x53 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x54..0x57 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x58..0x5B */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x5C..0x5F */

	_CTg, _CTg,
	_CTg, _CTg,			/* 0x60..0x63 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x64..0x67 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x68..0x6B */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x6C..0x6F */

	_CTg, _CTg,
	_CTg, _CTg,			/* 0x70..0x73 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x74..0x77 */
	_CTg, _CTg,
	_CTg, _CTg,			/* 0x78..0x7B */
	_CTg, _CTg,
	_CTg, 0,			/* 0x7C..0x7F */

	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0x87 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0x8F */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0x97 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0x9F */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xA7 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xAF */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xB7 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xBF */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xC7 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xCF */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xD7 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xDF */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xE7 */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xEF */
	0, 0, 0, 0, 0, 0, 0, 0, /* 0x80..0xF7 */
	0, 0, 0, 0, 0, 0, 0,
	0 /* EOF */
};
