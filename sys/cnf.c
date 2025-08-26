/*
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

#include "cnf.h"
#include "global.h"

#include "libkern/libkern.h"

#include "mint/filedesc.h"
#include "mint/proc.h"
#include "mint/stat.h"

#include "info.h"						/* messages */
#include "init.h"
#include "k_fds.h"
#include "kmemory.h"


union genarg
{
	int _err;
	short s;
	long l;
	unsigned long u;
	bool b;
	char *c;
};


/*============================================================================*/
/* The parser itself and its functions ...
 */

#define EOL '\n'

enum { ARG_MISS, ARG_NUMB, ARG_RANG, ARG_BOOL, ARG_QUOT };


/*----------------------------------------------------------------------------*/
void parser_msg(struct parsinf *inf, const char *msg)
{
	if (inf)
		boot_printf("[%s:%i] ", inf->file, inf->line);
	else if (!msg)
		msg = MSG_cnf_parser_skipped;

	if (msg)
	{
		boot_print(msg);
		boot_print("\r\n");
	}
}

/*----------------------------------------------------------------------------*/
static void parse_spaces(struct parsinf *inf)
{
	char c;

	while (isspace(c = *(inf->src)) && c != '\n')
		inf->src++;
}

/*----------------------------------------------------------------------------*/
static union genarg parse_token(struct parsinf *inf, bool upcase, bool envexpand, char *end)
{
	char *dst = inf->dst;
	char *src = inf->src;
	char delim = '\0';
	unsigned char c;
	union genarg ret;

	ret.c = inf->dst;

	for (;;)
	{
		c = *(src++);

		if (c == '\0')
		{
			/* correct overread zero */
			--src;
			*dst++ = '\0';
			break;
		}
		if (c == '\'' || c == '"')
		{
			if (delim == '\0')
			{
				delim = c;
				continue;
			} else if (delim == c)
			{
				delim = '\0';
				continue;
			}
		} else if (c == '\\' && (delim == '"' || (delim == '\0' && (inf->opt & INF_CONTROL))))
		{
			if (!*src || *src == EOL || (*src == '\r' && *(src + 1) == EOL))
			{
				/* trailing backslash honestly ignored :-) */
				*dst++ = '\0';
				break;
			}
			if (delim != '\'')
			{
				c = *src++;
				if (c == '\0')
				{
					/* correct overread zero */
					--src;
					*dst++ = '\0';
					break;
				}
				switch (c)
				{
				case 't':
					c = '\t';
					break;
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				case 'e':
					c = '\x1B';
					break;				/* special: for nicer output */
				case '\\':
				case '$':
				case '"':
				case '\'':
					break;
				default:
					--src;
					c = '\\';
					break;
				}
			}
		} else if (envexpand && c == '$' && (delim == '"' || delim == '\0'))
		{
			char brace;
			char *srcstart;
			char *varstart;
			char *varend;
			char savec;
			const char *value;

			srcstart = src;
			brace = '\0';
			if (*src == '{')
				brace = *src++;
			varstart = src;
			while (isalnum(*src) || *src == '_' || (brace != '\0' && *src != '\0' && !isspace(*src) && *src != '}'))
				src++;
			varend = src;
			if (brace != '\0')
			{
				if (*src == '}')
				{
					src++;
				} else
				{
					parser_msg(inf, NULL);
					boot_printf(MSG_cnf_missing_brace);
				}
			}
			if (varend == varstart)
			{
				/* empty variable name ? back up */
				src = srcstart;
			} else
			{
				savec = *varend;
				*varend = '\0';
				value = _mint_getenv(_base, varstart);
				if (value == NULL)
				{
					value = "";
					if (inf->opt & INF_UNSET)
					{
						parser_msg(inf, NULL);
						boot_printf(MSG_cnf_unset_variable, varstart);
					}
				}
				*varend = savec;
				while (*value != '\0' && dst < end)
				{
					c = *value++;
					if (upcase)
						c = toupper(c);
					*dst++ = c;
				}
				continue;
			}
		} else if (c == EOL || (isspace(c) && delim == '\0'))
		{
			if (c == EOL)
				--src;					/* correct overread eol */
			*dst++ = '\0';
			break;
		} else if (upcase)
		{
			c = toupper(c);
		}
		if (dst < end)
			*(dst++) = c;
	}

	if (src == inf->src)
	{
		/* ret._err = ARG_MISS */
		dst = NULL;
	} else if (delim != '\0')
	{
		ret._err = ARG_QUOT;
		dst = NULL;
	}
	inf->dst = dst;
	inf->src = src;

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg parse_line(struct parsinf *inf, char *end)
{
	union genarg ret;

	ret.c = inf->dst;

	while (*(inf->src) && *(inf->src) != EOL)
	{
		parse_token(inf, false, true, end);
		parse_spaces(inf);

		if (*(inf->src))
			*(inf->dst - 1) = ' ';
	}

	if (inf->dst == ret.c)
		inf->dst = ret.c + 1;

	if (inf->dst)
		*(inf->dst - 1) = '\0';

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg parse_drvlst(struct parsinf *inf)
{
	union genarg ret;

	ret.u = 0;

	while (*(inf->src))
	{
		int drv = DriveFromLetter(*(inf->src));

		if (drv < 0)
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
static union genarg parse_long(struct parsinf *inf, const struct range *range)
{
	char *src = inf->src;
	int sign = 1;
	unsigned char c;
	union genarg ret;

	ret.l = 0;

	if (*src == '-')
	{
		sign = -1;
		src++;
	}

	if (src[0] == '0' && (src[1] == 'x' || src[1] == 'X'))
	{
		src += 2;
		c = *src++;
		c = toupper(c);

		while (isdigit(c) || (c >= 'A' && c <= 'F'))
		{
			ret.l <<= 4;
			if (c < 'A')
				ret.l |= c - '0';
			else
			{
				ret.l |= (c - ('A' - 10)) & 0xf;
			}
			c = *src++;
			c = toupper(c);
		}
		src--;
	} else
	{
		while (isdigit(*src))
			ret.l = (ret.l * 10) + (*(src++) - '0');
	}

	if (src > inf->src)
	{
		c = *src;
		c = toupper(c);
		if (c == 'K')
		{
			ret.l *= 1024l;
			src++;
		} else if (c == 'M')
		{
			ret.l *= 1024l * 1024;
			src++;
		}
	}

	if (src == inf->src || (*src && !isspace(*src)))
	{
		ret._err = ARG_NUMB;
		inf->dst = NULL;
	} else if (range && (range->a > ret.l || ret.l > range->b))
	{
		ret._err = ARG_RANG;
		inf->dst = NULL;
	} else
	{
		ret.l *= sign;
	}

	inf->src = src;

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg parse_short(struct parsinf *inf, const struct range *range)
{
	struct range s_rng = { 0x8000, 0x7FFF };
	union genarg ret = parse_long(inf, range ? range : &s_rng);

	if (inf->dst)
		ret.s = (short) ret.l;

	return ret;
}

static union genarg parse_ushort(struct parsinf *inf, const struct range *range)
{
	union genarg ret = parse_long(inf, NULL);

	if (inf->dst)
	{
		unsigned long min, max;

		if (range)
			min = (unsigned short) range->a, max = (unsigned short) range->b;
		else
			min = 0L, max = 0x0000ffffL;

		if ((unsigned long) ret.l >= min && (unsigned long) ret.l <= max)
			ret.s = (unsigned short) ret.l;
		else
		{
			ret._err = ARG_RANG;
			inf->dst = NULL;
		}
	}

	return ret;
}

/*----------------------------------------------------------------------------*/
static union genarg parse_bool(struct parsinf *inf, char *end)
{
	static const char *const tab[] = {
		"0" "0", "1" "1",
		"0N", "1Y",
		"0NO", "1YES",
		"0OFF", "1ON",
		"0FALSE", "1TRUE",
		NULL
	};
	union genarg token = parse_token(inf, true, false, end);
	union genarg ret;

	ret.b = false;

	if (!inf->dst)
	{
		ret._err = token._err;
	} else
	{
		const char *const *p = tab;

		while (*p && strcmp(&(*p)[1], token.c) != 0)
			p++;

		if (!*p)
		{
			ret._err = ARG_BOOL;
			inf->dst = NULL;
		} else
		{
			ret.b = (*p)[0] - '0';
		}
	}

	return ret;
}

#define CNF_LINESIZE 512

/*----------------------------------------------------------------------------*/
static void parser(FILEPTR *f, long f_size, struct parsinf *inf, struct parser_item *parser_tab)
{
	long b_len = f_size + 2;
	int state = 1;						/* 0: skip, 1: key, 2: arg(s), 3: call */
	char *buf;
	char linebuf[2][CNF_LINESIZE];
	char *end;

	if (f_size == 0)
	{
		if (!(inf->opt & INF_QUIET))
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
	end = &linebuf[0][CNF_LINESIZE - 1];

	if (!(inf->opt & INF_QUIET))
	{
		boot_printf(MSG_cnf_reading_mintcnf, inf->file);
	}

	if ((*f->dev->read) (f, inf->src, f_size) != f_size)
	{
		boot_print(MSG_cnf_not_successful);
		kfree(buf);
		return;
	} else
	{
		if (!(inf->opt & INF_QUIET))
			boot_printf(MSG_cnf_bytes_done, f_size);
	}

	inf->src[f_size] = '\0';

	while (*(inf->src))
	{
		const struct parser_item *item = NULL;
		union genarg arg[2] = { {0}, {0} };
		int arg_type = 0;
		int arg_num = 0;

		if (state == 1)	/*---------- process keyword */
		{
			unsigned char c;

			/*--- (1.1) skip leading spaces and empty or comment lines */
			while ((c = *(inf->src)) != '\0')
			{
				if (c == '#')
				{
					while ((c = *(++inf->src)) && c != EOL)
						;
				}
				if (c == EOL)
					inf->line++;
				else if (!isspace(c))
					break;
				inf->src++;
			}
			if (*(inf->src) == '\0')
				break;					/* <eof> */

			if (inf->opt & INF_VERBOSE)
			{
				char save;
				char *srcend = inf->src;

				while (*srcend && *srcend != '\r' && *srcend != '\n')
					srcend++;

				save = *srcend;
				*srcend = '\0';
				boot_printf("%s\r\n", inf->src);
				*srcend = save;
			}

			/*--- (1.2) now read the keyword */
			inf->dst = &linebuf[0][0];
			while ((c = toupper((int) *(inf->src) & 0xff)) && c != '=' && !isspace(c))
			{
				if (inf->dst < end)
					*(inf->dst++) = c;
				inf->src++;
			}
			*(inf->dst) = '\0';

			/*--- (1.3) find item */
			item = parser_tab;
			while (item->key && strcmp(linebuf[0], item->key) != 0)
				item++;

			inf->dst = &linebuf[0][0];
			if (!item->key)
			{
				/*--- (1.3.1) keyword not found */
				parse_spaces(inf);		/* skip to next character */
				parser_msg(inf, NULL);
				boot_print(*(inf->src) == '=' ? MSG_cnf_unknown_variable : MSG_cnf_syntax_error);
				boot_printf(" '%s'", linebuf[0]);
				parser_msg(NULL, NULL);
				state = 0;
			} else if (!item->cb)
			{
				/*--- (1.3.2) found, but not supported */
				parser_msg(inf, NULL);
				boot_printf(MSG_cnf_keyword_not_supported, item->key);
				parser_msg(NULL, NULL);
				state = 0;
			} else if (item->type & PI_V__)
			{
				/*--- (1.3.3) check equation */
				parse_spaces(inf);		/* skip to '=' */
				if (*(inf->src) != '=')
				{
					parser_msg(inf, NULL);
					boot_printf(MSG_cnf_needs_equation, item->key);
					parser_msg(NULL, NULL);
					state = 0;
				} else
				{
					/* skip the '=' */
					inf->src++;
				}
			}
			if (state)
			{
				/*----- (1.3.4) success */
				arg_type = item->type & 0x00FF;
				state = 2;
			}
		}

		while (state == 2) /*---------- process arg */
		{
			const struct range *range = item->dat.dat ? &item->dat.range : NULL;
			char *start;

			/*--- (2.1) skip leading spaces */
			parse_spaces(inf);

			inf->dst = &linebuf[arg_num][0];
			end = &linebuf[arg_num][CNF_LINESIZE - 1];
			start = inf->src; 
			/*--- (2.2) read argument */
			switch (arg_type & 0x0F)
			{
			case PI_C_S:
				arg[arg_num] = parse_short(inf, range);
				break;
			case PI_C_L:
				arg[arg_num] = parse_long(inf, range);
				break;
			case PI_C_B:
				arg[arg_num] = parse_bool(inf, end);
				break;
			case PI_C_T:
				arg[arg_num] = parse_token(inf, false, true, end);
				break;
			case PI_C_A:
				arg[arg_num] = parse_line(inf, end);
				break;
			case PI_C_D:
				arg[arg_num] = parse_drvlst(inf);
				break;
			case PI_C_US:
				arg[arg_num] = parse_ushort(inf, range);
				break;
			}
			if (!inf->dst)
			{
				/*--- (2.3) argument failure */
				const char *msg = MSG_cnf_missed;

				if (inf->src != start)
				{
					switch (arg[arg_num]._err)
					{
					case ARG_NUMB:
						msg = MSG_cnf_must_be_a_num;
						break;
					case ARG_RANG:
						msg = MSG_cnf_out_of_range;
						break;
					case ARG_BOOL:
						msg = MSG_cnf_must_be_a_bool;
						break;
					case ARG_QUOT:
						msg = MSG_cnf_missing_quotation;
						break;
					}
				}
				parser_msg(inf, NULL);
				boot_printf(MSG_cnf_argument_for, arg_num + 1, item->key);
				boot_print(msg);
				parser_msg(NULL, NULL);
				state = 0;
			} else
			{
				/*----- (2.4) success */
				arg_num++;
				state = arg_num < 2 && ((arg_type >>= 4) & 0x0F) ? 2 : 3;
			}
		}

		if (state == 3)	/*---------- handle the callback */
		{
			union
			{
				void *_v;
				PCB_Lx *l;
				PCB_Bx *b;
				PCB_Dx *u;
				PCB_Tx *c;
				PCB_TTx *cc;
				PCB_0TT *_cc;
				PCB_ATK *ccl;
			} cb;

			cb._v = item->cb;

			/*--- (3.1) check for following characters */

			parse_spaces(inf);			/* skip following spaces */
			if (*(inf->src) && *(inf->src) != '\n' && *(inf->src) != '#')
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
			case PI_C_L:
			case PI_V_L:
				(*cb.l) (arg[0].l, inf);
				break;
			case PI_C_B:
			case PI_V_B:
				(*cb.b) (arg[0].b, inf);
				break;
			case PI_C_D:
			case PI_V_D:
				(*cb.u) (arg[0].u, inf);
				break;
			case PI_C_T:
			case PI_V_T:
			case PI_C_A:
			case PI_V_A:
				(*cb.c) (arg[0].c, inf);
				break;
			case PI_C_TT:
			case PI_C_TA:
				(*cb.cc) (arg[0].c, arg[1].c, inf);
				break;
			case PI_C_0TT:
				(*cb._cc) (0, arg[0].c, arg[1].c);
				break;
			case PI_V_ATK:
				(*cb.ccl) (arg[0].c, arg[1].c, item->dat.dat);
				break;
				/* references */
			case PI_R_S:
				*(short *) (cb._v) = arg[0].s;
				break;
			case PI_R_L:
				*(long *) (cb._v) = arg[0].l;
				break;
			case PI_R_B:
				*(bool *) (cb._v) = arg[0].b;
				break;
			case PI_R_US:
				*(unsigned short *) (cb._v) = arg[0].s;
				break;
			case PI_R_T:
				/* string buf */
				if (strlen(arg[0].c) < item->dat.dat)
				{
					strcpy((char *) (cb._v), arg[0].c);
				} else
				{
					parser_msg(inf, NULL);
					boot_printf(MSG_cnf_string_too_long, arg[0].c, item->dat.dat);
				}
				break;

			default:
				ALERT(MSG_cnf_unknown_tag, (int) item->type, item->key);
				break;
			}

			if (state)
			{
				/*----- (3.3) success */
				state = 1;
			}
		}

		if (state == 0)	/*---------- skip to end of line */
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
	}

	kfree(buf);
}

long parse_include(const char *path, struct parsinf *inf, struct parser_item *parser_tab)
{
	XATTR xattr;
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC(rootproc, &fp);
	if (ret)
		return ret;

	ret = do_open(&fp, path, O_RDONLY, 0, &xattr);
	if (!ret)
	{
		struct parsinf include = {
			inf->opt,
			path,
			1,
			inf->dst,
			inf->src,
			inf->data
		};

		parser(fp, xattr.size, &include, parser_tab);
		do_close(rootproc, fp);
	} else
	{
		fp->links = 0;
		FP_FREE(fp);

		parser_msg(inf, NULL);
		boot_printf(MSG_cnf_cannot_include, path);
		parser_msg(NULL, NULL);
	}
	return ret;
}

long parse_cnf(const char *path, struct parser_item *parser_tab, void *data, unsigned long options)
{
	struct parsinf inf = { options, NULL, 1, NULL, NULL, data };
	XATTR xattr;
	FILEPTR *fp;
	long ret;

	ret = FP_ALLOC(rootproc, &fp);
	if (ret)
		return ret;

	ret = do_open(&fp, inf.file = path, O_RDONLY, 0, &xattr);
	if (!ret)
	{
		parser(fp, xattr.size, &inf, parser_tab);
		do_close(rootproc, fp);
	} else
	{
		fp->links = 0;
		FP_FREE(fp);

		if (!(options & INF_OPTIONAL))
			ALERT(MSG_cnf_cant_open, path);
	}
	return ret;
}
