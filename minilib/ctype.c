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

#define _CTc _IScntrl
#define _CTd _ISdigit
#define _CTu _ISupper
#define _CTl _ISlower
#define _CTs _ISspace
#define _CTp _ISpunct
#define _CTx _ISxdigit
#define _CTP _ISprint

unsigned char const __libc_ctype[UCHAR_MAX + 1] =
{
	_CTc, _CTc, _CTc, _CTc,					/* 0x00..0x03 */
	_CTc, _CTc, _CTc, _CTc,					/* 0x04..0x07 */
	_CTc, _CTc|_CTs, _CTc|_CTs, _CTc|_CTs,	/* 0x08..0x0B */
	_CTc|_CTs, _CTc|_CTs, _CTc, _CTc,		/* 0x0C..0x0F */

	_CTc, _CTc, _CTc, _CTc,				/* 0x10..0x17 */
	_CTc, _CTc, _CTc, _CTc,				/* 0x14..0x17 */
	_CTc, _CTc, _CTc, _CTc,				/* 0x18..0x1B */
	_CTc, _CTc, _CTc, _CTc,				/* 0x1C..0x1F */

	_CTs|_CTP, _CTp|_CTP, _CTp|_CTP, _CTp|_CTP,			/* 0x20..0x23 */
	_CTp|_CTP, _CTp|_CTP, _CTp|_CTP, _CTp|_CTP,			/* 0x24..0x27 */
	_CTp|_CTP, _CTp|_CTP, _CTp|_CTP, _CTp|_CTP,			/* 0x28..0x2B */
	_CTp|_CTP, _CTp|_CTP, _CTp|_CTP, _CTp|_CTP,			/* 0x2C..0x2F */

	_CTd|_CTx|_CTP, _CTd|_CTx|_CTP,
	_CTd|_CTx|_CTP, _CTd|_CTx|_CTP,	/* 0x30..0x33 */
	_CTd|_CTx|_CTP, _CTd|_CTx|_CTP,
	_CTd|_CTx|_CTP, _CTd|_CTx|_CTP,	/* 0x34..0x37 */
	_CTd|_CTx|_CTP, _CTd|_CTx|_CTP,
	_CTp|_CTP, _CTp|_CTP,			/* 0x38..0x3B */
	_CTp|_CTP, _CTp|_CTP,
	_CTp|_CTP, _CTp|_CTP,			/* 0x3C..0x3F */

	_CTp|_CTP, _CTu|_CTx|_CTP,
	_CTu|_CTx|_CTP, _CTu|_CTx|_CTP,	/* 0x40..0x43 */
	_CTu|_CTx|_CTP, _CTu|_CTx|_CTP,
	_CTu|_CTx|_CTP, _CTu|_CTP,		/* 0x44..0x47 */
	_CTu|_CTP, _CTu|_CTP,
	_CTu|_CTP, _CTu|_CTP,			/* 0x48..0x4B */
	_CTu|_CTP, _CTu|_CTP,
	_CTu|_CTP, _CTu|_CTP,			/* 0x4C..0x4F */

	_CTu|_CTP, _CTu|_CTP,
	_CTu|_CTP, _CTu|_CTP,			/* 0x50..0x53 */
	_CTu|_CTP, _CTu|_CTP,
	_CTu|_CTP, _CTu|_CTP,			/* 0x54..0x57 */
	_CTu|_CTP, _CTu|_CTP,
	_CTu|_CTP, _CTp|_CTP,			/* 0x58..0x5B */
	_CTp|_CTP, _CTp|_CTP,
	_CTp|_CTP, _CTp|_CTP,			/* 0x5C..0x5F */

	_CTp|_CTP, _CTl|_CTx|_CTP,
	_CTl|_CTx|_CTP, _CTl|_CTx|_CTP,	/* 0x60..0x63 */
	_CTl|_CTx|_CTP, _CTl|_CTx|_CTP,
	_CTl|_CTx|_CTP, _CTl|_CTP,		/* 0x64..0x67 */
	_CTl|_CTP, _CTl|_CTP,
	_CTl|_CTP, _CTl|_CTP,			/* 0x68..0x6B */
	_CTl|_CTP, _CTl|_CTP,
	_CTl|_CTP, _CTl|_CTP,			/* 0x6C..0x6F */

	_CTl|_CTP, _CTl|_CTP,
	_CTl|_CTP, _CTl|_CTP,			/* 0x70..0x73 */
	_CTl|_CTP, _CTl|_CTP,
	_CTl|_CTP, _CTl|_CTP,			/* 0x74..0x77 */
	_CTl|_CTP, _CTl|_CTP,
	_CTl|_CTP, _CTp|_CTP,			/* 0x78..0x7B */
	_CTp|_CTP, _CTp|_CTP,
	_CTp|_CTP, _CTc,				/* 0x7C..0x7F */

	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0x87 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0x8F */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0x97 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0x9F */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xA7 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xAF */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xB7 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xBF */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xC7 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xCF */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xD7 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xDF */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xE7 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xEF */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc, /* 0x80..0xF7 */
	_CTc, _CTc, _CTc, _CTc, _CTc, _CTc, _CTc,
	0 /* EOF */
};
