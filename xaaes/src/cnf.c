/*
 * $Id$
 *
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999 Ralph Lowinski <ralph@aquaplan.de>
 * Copyright 2003 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef MINT_CNF_PARSER

# include "cnf.h"

#include <mint/mintbind.h>

#include "xa_types.h"
#include "xa_global.h"

#include "display.h"
#include "xa_fsel.h"
#include "xalloc.h"

#define XA_KEY 23
#define kmalloc(x) xmalloc(x, XA_KEY)
#define kfree free

#define boot_printf display
#define boot_print  display


/** from sys/info.c */
/* ------------ cnf.c ---------- */

const char *MSG_cnf_bad_drive = "bad drive '%c:' in alias";
const char *MSG_cnf_tos_error = "TOS error %ld while looking for '%s'";
const char *MSG_cnf_file_not_found = "file not found";
const char *MSG_cnf_error_executing = "error while attempting to execute";
const char *MSG_cnf_cannot_include = "cannot open include file '%s'";
# ifdef OLDTOSFS
const char *MSG_cnf_newfatfs = "\033pNEWFATFS active:\033q ";
# else
const char *MSG_cnf_newfatfs = "\033pNEWFATFS\033q is \033pdefault\033q filesystem for all drives!\r\n";
# endif
const char *MSG_cnf_vfat = "\033pVFAT active:\033q ";
const char *MSG_cnf_wbcache = "\033pWB CACHE active:\033q ";
const char *MSG_cnf_invalid_arg = "invalid argument line for 'set', skipped.";
const char *MSG_cnf_set_option = "option '-%c'";
const char *MSG_cnf_set_ignored = " ignored.";
const char *MSG_cnf_parser_skipped = ", skipped.";
const char *MSG_cnf_empty_file = "[%s] empty file, skipped.";
const char *MSG_cnf_cant_allocate = "[%s] can't allocate %li bytes, break.";
const char *MSG_cnf_reading_mintcnf = "\r\nReading `%s' ... ";
const char *MSG_cnf_not_successful = "not successful, break.\r\n\r\n";
const char *MSG_cnf_bytes_done = "%li bytes done.\r\n\r\n";
const char *MSG_cnf_unknown_variable = "unknown variable";
const char *MSG_cnf_syntax_error = "syntax error";
const char *MSG_cnf_keyword_not_supported = "keyword '%s' not supported yet";
const char *MSG_cnf_needs_equation = "variable '%s' needs equation";
const char *MSG_cnf_argument_for = "argument %i for '%s' ";
const char *MSG_cnf_missed = "missed";
const char *MSG_cnf_must_be_a_num = "must be a number";
const char *MSG_cnf_out_of_range = "out of range";
const char *MSG_cnf_must_be_a_bool = "must be of type boolean (y/n)";
const char *MSG_cnf_missing_quotation = "missing quotation";
const char *MSG_cnf_junk = "junk at end of line ignored.";
const char *MSG_cnf_unknown_tag = "!!! unknown tag type %04x for '%s'!!!\n";

/** sys/mint/fsops.h **/

# define NDRIVES     32
# define NUM_DRIVES  NDRIVES

# define O_RDONLY    0x00000000

/***/

union genarg
{
	int		_err;
	short		s;
	long		l;
	unsigned long	u;
	bool		b;
	char		*c;
};

const char *drv_list = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456";

/*============================================================================*/
/* The parser itself and its functions ...
 */

#define NUL '\0'
#define SGL '\''
#define DBL '"'
#define EOL '\n'
#define ESC '\\'   /* not 0x1B !!! */

enum { ARG_MISS, ARG_NUMB, ARG_RANG, ARG_BOOL, ARG_QUOT };


/*----------------------------------------------------------------------------*/
void
parser_msg(struct parsinf *inf, const char *msg)
{
	if (inf)
		boot_printf ("[%s:%i] ", inf->file, inf->line);
	else if (!msg)
		msg = MSG_cnf_parser_skipped;
	
	if (msg)
	{
		boot_print (msg);
		boot_print ("\r\n");
	}
}

/*----------------------------------------------------------------------------*/
static void
parse_spaces(struct parsinf *inf)
{
	char c;

	while (isspace(c = *(inf->src)) && c != '\n')
		inf->src++;
}

/*----------------------------------------------------------------------------*/
static union genarg
parse_token(struct parsinf *inf, bool upcase)
{
	char *dst = inf->dst, *src = inf->src;
	char delim = NUL;
	char c;
	union genarg ret;
	
	ret.c = inf->dst;

	do {
		c = *(src++);
		
		if (c == NUL)
		{
			/* correct overread zero */
			--src;
		}
		else if (c == SGL || c == DBL)
		{
			if      (delim == NUL) { delim  = c;   continue; }
			else if (delim == c)   { delim  = NUL; continue; }

		}
		else if (c == ESC  && (delim == DBL
					|| (delim == NUL  && (inf->opt & SET('C')))))
		{
			if (!*src || *src == EOL || (*src == '\r' && *(src+1) == EOL))
			{
				/* leading backslash honestly ignored :-) */
				c = NUL;
			}
			else if (delim != SGL) switch ((c = *(src++)))
			{
				case 't': c = '\t';   break;
				case 'n': c = '\n';   break;
				case 'r': c = '\r';   break;
				case 'e': c = '\x1B'; break; /* special: for nicer output */
			}

		}
		else if (c == EOL  || (isspace(c) && delim == NUL))
		{
			if (c == EOL) --src; /* correct overread eol */
			c = NUL;

		}
		else if (upcase)
		{
			c = toupper((int)c & 0xff);
		}
		*(dst++) = c;
	}
	while (c);

	if (src == inf->src)
	{
		dst = NULL; /* ARG_MISS */
	}
	else if (delim != NUL)
	{
		ret._err = ARG_QUOT;
		dst = NULL;
	}
	inf->dst = dst;
	inf->src = src;

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg
parse_line(struct parsinf *inf)
{
	union genarg ret;
	
	ret.c = inf->dst;

	while (*(inf->src) && *(inf->src) != EOL)
	{
		parse_token(inf, false);
		parse_spaces(inf);

		if (*(inf->src))
			*(inf->dst - 1) = ' ';
	}

	if (inf->dst == ret.c)
		inf->dst = ret.c + 1;

	*(inf->dst - 1) = NUL;

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg
parse_drvlst(struct parsinf *inf)
{
	union genarg ret;

	ret.u = 0;

	while (*(inf->src))
	{
		long drv = strchr(drv_list, toupper((int)*(inf->src) & 0xff)) - drv_list;

		if (drv < 0)
			break;

		if (drv >= NUM_DRIVES)
		{
			ret._err = ARG_RANG;
			break;
		}

		ret.l |= 1ul << drv;
		inf->src++;
		parse_spaces(inf);

		if (*(inf->src) != ',')
			break;

		inf->src++;
		parse_spaces(inf);
	}
	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg
parse_long(struct parsinf *inf, struct range *range)
{
	char *src = inf->src;
	int sign = 1;
	union genarg ret;

	ret.l = 0;

	if (*src == '-')
	{
		sign = -1;
		src++;
	}

	while (isdigit(*src))
		ret.l = (ret.l * 10) + (*(src++) - '0');

	if (src > inf->src)
	{
		if      (toupper((int)*src & 0xff) == 'K') { ret.l *= 1024l;      src++; }
		else if (toupper((int)*src & 0xff) == 'M') { ret.l *= 1024l*1024; src++; }
	}
	*(inf->dst++) = '\0'; /* not really necessary */

	if (src == inf->src || (*src && !isspace(*src)))
	{
		ret._err = ARG_NUMB;
		inf->dst = NULL;
	}
	else if (range  && (range->a > ret.l || ret.l > range->b))
	{
		ret._err = ARG_RANG;
		inf->dst = NULL;
	}
	else
		ret.l *= sign;

	inf->src = src;

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg
parse_short(struct parsinf *inf, struct range *range)
{
	struct range s_rng = {0x8000,0x7FFF};
	union genarg ret = parse_long(inf, (range ? range : &s_rng));

	if (inf->dst)
		ret.s = (short)ret.l;

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg
parse_bool(struct parsinf *inf)
{
	static const char *tab[] =
	{
		"\0""0","\1""1",
		"\0N","\1Y",
		"\0NO","\1YES",
		"\0OFF","\1ON",
		"\0FALSE","\1TRUE",
		NULL
	};
	char *token = (parse_token(inf, true)).c;
	union genarg ret;

	ret.b = false;

	if (!inf->dst)
	{
		ret._err = ((union genarg) token)._err;
	}
	else
	{
		const char **p = tab;

		while (*p && stricmp(&(*p)[1], token))
			p++;

		if (!*p)
		{
			ret._err = ARG_BOOL;
			inf->dst = NULL;
		}
		else
			ret.b = ((*p)[0] != '\0');
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
static void
parser(int fd, long f_size,
       struct parsinf *inf,
       struct parser_item *parser_tab)
{
	long b_len = f_size + 2;
	int  state = 1;	/* 0: skip, 1: key, 2: arg(s), 3: call */
	char *buf;

	if (f_size == 0)
	{
		if (!(inf->opt & SET('Q')))
		{
			boot_printf(MSG_cnf_empty_file, inf->file);
		}
		return;
	}

	buf = kmalloc(b_len);
	if (!buf)
	{
		boot_printf(MSG_cnf_cant_allocate, inf->file, b_len);
		return;
	}

	inf->src = buf + b_len - f_size - 1;

	if (!(inf->opt & SET('Q')))
	{
		boot_printf(MSG_cnf_reading_mintcnf, inf->file);
	}

	if (Fread(fd, f_size, inf->src) != f_size)
	{
		boot_print(MSG_cnf_not_successful);
		kfree(buf);
		return;
	}
	else
	{
		if (!(inf->opt & SET('Q')))
			boot_printf(MSG_cnf_bytes_done, f_size);
	}

	inf->src[f_size] = '\0';

	while (*(inf->src))
	{
		const struct parser_item *item    = NULL;
		union genarg             arg[2]   = { {0}, {0} };
		int                      arg_type = 0;
		int                      arg_num  = 0;

		if (state == 1) /*---------- process keyword */
		{
			char c;

			/*--- (1.1) skip leading spaces and empty or comment lines */
			while ((c = *(inf->src)))
			{
				if      (c == '#')    while ((c = *(++inf->src)) && c != EOL);
				if      (c == EOL)    inf->line++;
				else if (!isspace(c)) break;
				inf->src++;
			}
			if (*(inf->src) == NUL)
				break; /* <eof> */

			if (inf->opt & SET('V'))
			{
				char save, *end = inf->src;

				while (*end && *end != '\r' && *end != '\n')
					end++;

				save = *end;
				*end = NUL;
				boot_printf("%s\r\n", inf->src);
				*end = save;
			}

			/*--- (1.2) now read the keyword */
			inf->dst = buf;
			while ((c = toupper((int)*(inf->src) & 0xff)) && c != '='  && !isspace(c))
			{
				*(inf->dst++) = c;
				inf->src++;
			}
			*(inf->dst++) = '\0';

			/*--- (1.3) find item */
			item = parser_tab;
			while (item->key && strcmp(buf, item->key))
				item++;

			if (!item->key) /*--- (1.3.1) keyword not found */
			{
				parse_spaces(inf);   /* skip to next character */
				parser_msg(inf, NULL);
				boot_print(*(inf->src) == '=' ? MSG_cnf_unknown_variable : MSG_cnf_syntax_error);
				boot_printf(" '%s'", buf);
				parser_msg(NULL,NULL);
				state = 0;
			}
			else if (!item->cb) /*--- (1.3.2) found, but not supported */
			{
				parser_msg(inf, NULL);
				boot_printf(MSG_cnf_keyword_not_supported, item->key);
				parser_msg(NULL,NULL);
				state = 0;
			}
			else if (item->type & PI_V__) /*--- (1.3.3) check equation */
			{
				parse_spaces(inf);   /* skip to '=' */
				if (*(inf->src) != '=')
				{
					parser_msg(inf, NULL);
					boot_printf(MSG_cnf_needs_equation, item->key);
					parser_msg(NULL,NULL);
					state = 0;
				}
				else
					/* skip the '=' */
					inf->src++;
			}
			if (state) /*----- (1.3.4) success */
			{
				arg_type = item->type & 0x00FF;
				inf->dst = buf;
				state    = 2;
			}
		}

		while (state == 2) /*---------- process arg */
		{
			struct range *range = (item->dat ? (struct range *) &item->dat : NULL);
			char  *start;

			parse_spaces(inf); /*--- (2.1) skip leading spaces */

			start = inf->src;  /*--- (2.2) read argument */
			switch (arg_type & 0x0F)
			{
				case 0x01: arg[arg_num] = parse_short  (inf, range); break;
				case 0x02: arg[arg_num] = parse_long   (inf, range); break;
				case 0x03: arg[arg_num] = parse_bool   (inf);        break;
				case 0x04: arg[arg_num] = parse_token  (inf, false); break;
				case 0x05: arg[arg_num] = parse_line   (inf);        break;
				case 0x06: arg[arg_num] = parse_drvlst (inf);        break;
			}
			if (!inf->dst)     /*--- (2.3) argument failure */
			{
				const char *msg = MSG_cnf_missed;
				if (inf->src != start) switch (arg[arg_num]._err)
				{
					case ARG_NUMB: msg = MSG_cnf_must_be_a_num;		break;
					case ARG_RANG: msg = MSG_cnf_out_of_range;		break;
					case ARG_BOOL: msg = MSG_cnf_must_be_a_bool;		break;
					case ARG_QUOT: msg = MSG_cnf_missing_quotation;		break;
				}
				parser_msg(inf, NULL);
				boot_printf(MSG_cnf_argument_for, arg_num + 1, item->key);
				boot_print(msg);
				parser_msg(NULL,NULL);
				state = 0;

			}
			else               /*----- (2.4) success */
			{
				arg_num++;
				state = ((arg_type >>= 4) & 0x0F ? 2 : 3);
			}
		}

		if (state == 3) /*---------- handle the callback */
		{
			union { void *_v; PCB_Lx *l; PCB_Bx *b; PCB_Dx *u; PCB_Tx *c;
			        PCB_TTx *cc; PCB_0TT *_cc; PCB_ATK *ccl;
			} cb;
			cb._v = item->cb;

			/*--- (3.1) check for following characters */

			parse_spaces(inf);   /* skip following spaces */
			if (*(inf->src)  &&  *(inf->src) != '\n'  &&  *(inf->src) != '#')
			{
				parser_msg(inf, MSG_cnf_junk);
				state = 0;
			}
			switch (item->type & 0xF7FF) /*--- (3.2) do the call */
			{
				/* We allege that every callback can make use of the parser
				 * information and put it always on stack. If it is not necessary
				 * it will simply ignored.
				 */
				#define A0L arg[0].l
				#define A0B arg[0].b
				#define A0U arg[0].u
				#define A0C arg[0].c
				#define A1C arg[1].c
				case PI_C_L: case PI_V_L: (*cb.l  )(  A0L,             inf); break;
				case PI_C_B: case PI_V_B: (*cb.b  )(  A0B,             inf); break;
				case PI_C_D: case PI_V_D: (*cb.u  )(  A0U,             inf); break;
				case PI_C_T: case PI_V_T:
				case PI_C_A: case PI_V_A: (*cb.c  )(  A0C,             inf); break;
				case PI_C_TT:
				case PI_C_TA:             (*cb.cc )(  A0C,A1C,         inf); break;
				case PI_C_0TT:            (*cb._cc)(0,A0C,A1C             ); break;
				case PI_V_ATK:            (*cb.ccl)(  A0C,A1C,item->dat   ); break;
				/* references */
				case PI_R_S: *(short *)(cb._v) = arg[0].s;                   break;
				case PI_R_L: *(long  *)(cb._v) = arg[0].l;                   break;
				case PI_R_B: *(bool  *)(cb._v) = arg[0].b;                   break;
				/* string buf */
				case PI_R_T:
				{
					if (strlen(arg[0].c) < item->dat)
						strcpy((char *)(cb._v), arg[0].c); 
					break;
				}

				default: boot_printf(MSG_cnf_unknown_tag,
					             (int)item->type, item->key);
			}

			if (state) /*----- (3.3) success */
				state = 1;
		}

		if (state == 0) /*---------- skip to end of line */
		{
			while (*inf->src)
			{
				if (*(inf->src++) == '\n')
				{
					inf->line++;
					state = 1;
					break;
				}
			}
		}
	} /* end while */

	kfree(buf);
}

void
parse_include(const char *path, struct parsinf *inf, struct parser_item *parser_tab)
{
	long fd;

	fd = Fopen(path, O_RDONLY);
	if (fd > 0)
	{
		struct parsinf include =
		{
			inf->opt,
			path,
			1,
			inf->dst,
			inf->src,
			inf->data
		};
		struct xattr xattr;
		long ret;

		ret = Fxattr(0, path, &xattr);

		parser(fd, xattr.size, &include, parser_tab);
		Fclose(fd);
	}
	else
	{
		parser_msg(inf, NULL);
		boot_printf(MSG_cnf_cannot_include, path);
		parser_msg(NULL, NULL);
	}
}

void
parse_cnf(const char *path, struct parser_item *parser_tab, void *data)
{
	struct parsinf inf  = { 0ul, NULL, 1, NULL, NULL, data };
	long fd;

	fd = Fopen(inf.file = path, O_RDONLY);
	if (fd > 0)
	{
		struct xattr xattr;
		long ret;

		ret = Fxattr(0, path, &xattr);
 
		parser(fd, xattr.size, &inf, parser_tab);
		Fclose(fd);
	}
}

#endif /* MINT_CNF_PARSER */
