/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2003 Konrad M. Kokoszkiewicz <draco@atari.org>
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
 *
 * begin:	2003-02-25
 * last change:	2000-02-25
 *
 * Author:	Konrad M. Kokoszkiewicz <draco@atari.org>
 *          Thorsten Otto <admin@tho-otto.de>
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

/* Make a keyboard translation table out of the given source file.
 *
 * The source file must consist of text lines. A text line must either
 * begin with a semicolon (;), or contain one of two directives:
 *
 * dc.b	- begins a sequence of bytes
 * dc.w - begins a sequence of words
 *
 * The data may be given as hex numbers (in either asm or C syntax),
 * dec numbers or ASCII characters. A hex number begins with $ or 0x,
 * (e.g. $2735 or 0x2735), an ASCII character is quoted (e.g. 'a'),
 * and a dec number has no prefix (e.g. 1 or 48736 is fine).
 *
 * If a number exceeds the desired limit, e.g. when you do
 *
 * dc.b 100000
 *
 * Only the lowest eight (or sixteen in case of dc.w) of such a
 * number will be taken into account, and a warning message is
 * printed.
 *
 * The data may be separated with commas or semicolons.
 *
 * Examples:
 *
 * ; This is a comment
 *
 *	dc.b 'a',$00,0x55,76
 *	dc.w $2334,4,0x12,'d'
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#ifdef WITH_GUI
#ifdef __PUREC__
#include <aes.h>
#else
#include <gem.h>
#endif
#endif
#ifdef __GNUC__
#include <unistd.h>
#endif
#include "mktbl.h"
#include "country.h"

#ifndef FALSE
#  define FALSE 0
#  define TRUE  1
#endif

#if !defined(__GNUC__) && !defined(__attribute__)
#define __attribute__(x)
#endif


unsigned char keytab[N_KEYTBL][MAX_SCANCODE];
unsigned char deadkeys[MAX_DEADKEYS];
int tabsize[N_KEYTBL + 1];
int keytab_format;
int deadkeys_format;
int keytab_ctry_code;
int keytab_codeset;

static const char *tblname = "xx";

static const char *const labels[N_KEYTBL + 1] = {
	"tab_unshift:",
	"tab_shift:",
	"tab_caps:",
	"tab_altgr:",
	"tab_shaltgr:",
	"tab_capsaltgr:",
	"tab_alt:",
	"tab_shalt:",
	"tab_capsalt:",
	"tab_dead:"
};

static unsigned char const possible_dead_keys[] = {
	0x5e,  /* U+005E Circumflex accent */
	0x60,  /* U+0060 Grave accent */
	0x7e,  /* U+007E Tilde */
	0xf8,  /* U+00B0 Degree sign (latin-1) */
	0xb7,  /* U+02C7 Caron (latin-2) */
	0xa2,  /* U+02D8 Breve (latin-2) */
	0xb0,  /* U+00B0 Degree sign (latin-2) */
	0xb2,  /* U+02DB Ogonek (latin-2) */
	0xff,  /* U+02D9 Dot above (latin-2) */
	0xba,  /* U+00B4 Acute accent (latin-1) */
	0xb4,  /* U+00B4 Acute accent (latin-2) */
	0xbd,  /* U+02DD Double acute accent (latin-2) */
	0xb9,  /* U+00A8 Diaresis (latin-1) */
	0xa8,  /* U+00A8 Diaresis (latin-2) */
	0xba,  /* U+00b4 Acute accent (greek-atarist) */
	0xb8,  /* U+00b8 Cedilla (latin-2) */
	0xb7,  /* U+00b7 Middle dot (latin-1) */
	       /* U+201a Single low-9 quotation mark */
	0
};

#include "unitable.h"

/*
 * If you add entries here,
 * also add tables to atari_toupper[] and atari_to_unicode[]
 * (and maybe also missing glyphs to unifont.txt).
 * The values here are the ones that are specified in the definition
 * files, and must not be changed.
 */
unsigned short const keytab_codesets[] = {
	0, /* atarist */
	1, /* iso8859-1 */
	2, /* iso8859-2 */
	3, /* iso8859-3 */
	4, /* iso8859-4 */
	5, /* iso8859-5 */
	6, /* iso8859-6 */
	7, /* iso8859-7 */
	8, /* iso8859-8 */
	9, /* iso8859-9 */
	10, /* iso8859-10 */
	737, /* cp737, with atari-specific changes */
	1251, /* cp1251, with atari-specific changes */
};

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

/* Own getdelim(). The `n' buffer size must definitely be bigger than 0!
 */
static int mktbl_getdelim(char **lineptr, size_t *n, FILE *stream)
{
	int ch;
	char *buf = *lineptr;
	size_t len = 0;

	while ((ch = fgetc(stream)) != EOF)
	{
		if ((len + 1) >= *n)
		{
			buf = realloc(buf, len + 256L);
			assert(buf);
			*n += 256L;
			*lineptr = buf;
		}

		if (ch == 0x0a)
			break;
		if (ch == 0x0d)
		{
			ch = fgetc(stream);
			if (ch != 0x0a && ch != EOF)
				ungetc(ch, stream);
			break;
		}
		buf[len++] = (char) ch;
	}

	buf[len] = 0;

	/* A problem here: returning -1 on EOF may cause data loss
	 * if there is no terminator character at the end of the file
	 * (in this case all the previously read characters of the
	 * line are discarded). At the other hand returning 0 at
	 * first EOF and -1 at the other creates an additional false
	 * empty line, if there *was* terminator character at the
	 * end of the file. So the check must be more extensive
	 * to behave correctly.
	 */
	if (ch == EOF)
	{
		if (len == 0)					/* Nothing read before EOF in this line */
			return -1;
		/* Pretend success otherwise */
	}

	return 0;
}

/* -------------------------------------------------------------------------- */

__attribute__((format(printf, 4, 5)))
static void error_in_line(const char *filename, long lineno, int error, const char *message, ...)
{
	va_list args;
#ifdef WITH_GUI
	char buf[160];
	char msgbuf[80];
	va_start(args, message);
	vsprintf(msgbuf, message, args);
	va_end(args);
	if (filename && lineno)
		sprintf(buf, "[1][%s in line %ld:|%s][%s]", error ? "Error" : "Warning", lineno, msgbuf, error ? "Abort" : "Continue");
	else
		sprintf(buf, "[1][%s:|%s][%s]", error ? "Error" : "Warning", msgbuf, error ? "Abort" : "Continue");
	form_alert(1, buf);
#else
	va_start(args, message);
	if (filename && lineno)
		fprintf(stderr, "%s:%ld: ", filename, lineno);
	fprintf(stderr, "%s: ", error ? "error" : "warning");
	vfprintf(stderr, message, args);
	fputc('\n', stderr);
	va_end(args);
#endif
}

/* -------------------------------------------------------------------------- */

#if 0
static unsigned char *tbl_scan_fwd(int tab, const unsigned char *src)
{
	unsigned char *dst;
	unsigned char *s;

	s = src;
	dst = keytab[tab];
	while (*src)
	{
		*dst++ = *src++;
	}
	*dst = '\0';
	++src;
	tabsize[tab] = (int)(src - s);
	return src;
}
#endif

/* -------------------------------------------------------------------------- */

int lookup_codeset(unsigned short magic)
{
	unsigned int i;
	
	for (i = 0; i < ARRAY_SIZE(keytab_codesets); i++)
		if (magic == keytab_codesets[i])
			return i;
	return -1;
}

/* -------------------------------------------------------------------------- */

static int mktbl_load_mint(FILE *fd, const char *filename)
{
	long size;
	const unsigned char *src;
	unsigned short magic;
	long consumed;
	int codeset;

	/*
	 * the deadkeys buffer should be large enough to hold the normal tables
	 */
	assert(sizeof(deadkeys) >= N_KEYTBL * MAX_SCANCODE);
	size = fread(deadkeys, 1, sizeof(deadkeys) - 2, fd);

	/* This is 128+128+128 for unshifted, shifted and caps
	 * tables respectively; plus 3 bytes for three alt ones,
	 * plus two bytes magic header, gives 389 bytes minimum.
	 */
	if (size < 389)
	{
		error_in_line(filename, 0, 1, "invalid size");
		return FALSE;
	}
	/* Append two zeros in case the altgr + deadkey tables are missing from the file. */
	deadkeys[size] = 0;
	deadkeys[size + 1] = 0;
	src = deadkeys;

	magic = (src[0] << 8) | src[1];
	switch (magic)
	{
	case 0x2771:	/* magic word for std format */
		src += 2;
		break;
	case 0x2772:	/* magic word for ext format */
		/* The extended format is identical as the old one
		 * with exception that the second word of the data
		 * contains the AKP code for the keyboard table
		 * loaded.
		 */
		magic = (src[2] << 8) | src[3];
		/* -1 for the AKP field means "ignore" */
		if (magic <= MAXAKP)
			keytab_ctry_code = magic;
		src += 4;
		break;
	case 0x2773:		/* the ISO format (as of 30.VII.2004) */
		/* The ISO format header consists of:
		 * 0x2773, the AKP code as above, and the longword containing
		 * ISO-8859 code for the keyboard (0 - undefined).
		 */
		magic = (src[2] << 8) | src[3];
		/* -1 for the AKP field means "ignore" */
		if (magic <= MAXAKP)
			keytab_ctry_code = magic;
		magic = (src[6] << 8) | src[7];
		if (magic == 0 || (codeset = lookup_codeset(magic)) < 0)
		{
			error_in_line(filename, 0, 1, "invalid ISO code %u", magic);
			return FALSE;
		}
		keytab_codeset = codeset;
		src += 8;
		break;
	default:
		error_in_line(filename, 0, 1, "unknown format 0x%x", magic);
		return FALSE;
	}

	/* Setup the standard vectors */
	memcpy(keytab[TAB_UNSHIFT], src, MAX_SCANCODE * sizeof(keytab[0][0]));
	src += MAX_SCANCODE;
	tabsize[TAB_UNSHIFT] = MAX_SCANCODE;
	memcpy(keytab[TAB_SHIFT], src, MAX_SCANCODE * sizeof(keytab[0][0]));
	src += MAX_SCANCODE;
	tabsize[TAB_SHIFT] = MAX_SCANCODE;
	memcpy(keytab[TAB_CAPS], src, MAX_SCANCODE * sizeof(keytab[0][0]));
	src += MAX_SCANCODE;
	tabsize[TAB_CAPS] = MAX_SCANCODE;
	/* and the AKP vectors */
	src = conv_table(TAB_ALT, src);
	src = conv_table(TAB_ALTSHIFT, src);
	src = conv_table(TAB_ALTCAPS, src);
	src = conv_table(TAB_ALTGR, src);
	
	/* and the deadkeys */
	consumed = (long)(src - deadkeys);
	if (consumed > size)
		consumed = size;
	memmove(deadkeys, &deadkeys[consumed], size - consumed);
	size -= consumed;
	size += fread(&deadkeys[size], 1, sizeof(deadkeys) - 2 - size, fd);
	if (size == 0 || deadkeys[size - 1] != 0)
	{
		if (size != 0)
			error_in_line(filename, 0, 0, "missing terminating zero in dead keys");
		deadkeys[size] = 0;
		size++;
	}
	tabsize[TAB_DEADKEYS] = (int)size;

	memcpy(keytab[TAB_SHALTGR], keytab[TAB_ALTSHIFT], MAX_SCANCODE * sizeof(keytab[0][0]));
	memcpy(keytab[TAB_CAPSALTGR], keytab[TAB_ALTCAPS], MAX_SCANCODE * sizeof(keytab[0][0]));

	keytab_format = FORMAT_MINT;
	deadkeys_format = FORMAT_MINT;

	return TRUE;
}

/* -------------------------------------------------------------------------- */

static int mktbl_load_magic(FILE *fd, const char *filename)
{
	long size;

	size = fread(keytab, 1, N_KEYTBL * MAX_SCANCODE, fd);
	if (size != N_KEYTBL * MAX_SCANCODE)
	{
		error_in_line(filename, 0, 1, "invalid size");
		return FALSE;
	}

	size = fread(deadkeys, 1, sizeof(deadkeys) - 2, fd);
	if (size < 0)
		return FALSE;
	if (size == 0 || deadkeys[size - 1] != 0)
	{
		if (size != 0)
			error_in_line(filename, 0, 0, "missing terminating zero in dead keys");
		deadkeys[size] = 0;
		size++;
	}
	tabsize[TAB_DEADKEYS] = (int)size;

	keytab_format = FORMAT_MAGIC;
	deadkeys_format = FORMAT_MAGIC;

	return TRUE;
}

/* -------------------------------------------------------------------------- */

static int mktbl_parse_source(FILE *fd, const char *filename)
{
	int scancode = 0;
	size_t buf;
	char *line;
	char *ln;
	int is_mint = 0;
	int mint_expected = 0;
	short w;
	int tab, newtab;
	long lineno;
	int r;
	int loop;
	unsigned long num;
	int copyfrom[N_KEYTBL];

	for (tab = 0; tab < N_KEYTBL; tab++)
	{
		copyfrom[tab] = -1;
	}
	lineno = 0;
	tab = -1;
	w = 0;

	buf = 1024;
	line = malloc(buf);					/* should be plenty */
	assert(line);

	for (;;)
	{
		lineno++;

		r = mktbl_getdelim(&line, &buf, fd);
		if (r < 0)
			break;
		ln = line;
		while (*ln == ' ' || *ln == '\t')
			ln++;
		if (ln[0] == ';' || ln[0] == '\0')
			continue;
		newtab = -1;
		if (strncmp(ln, "dc.b", 4) == 0)
		{
			w = 0;
		} else if (strncmp(ln, "dc.w", 4) == 0)
		{
			w = 1;
		} else if (strcmp(ln, labels[TAB_SHIFT]) == 0)
		{
			newtab = TAB_SHIFT;
		} else if (strcmp(ln, labels[TAB_UNSHIFT]) == 0)
		{
			newtab = TAB_UNSHIFT;
		} else if (strcmp(ln, labels[TAB_CAPS]) == 0)
		{
			newtab = TAB_CAPS;
		} else if (strcmp(ln, labels[TAB_ALTGR]) == 0)
		{
			newtab = TAB_ALTGR;
		} else if (strcmp(ln, labels[TAB_SHALTGR]) == 0)
		{
			newtab = TAB_SHALTGR;
		} else if (strcmp(ln, labels[TAB_CAPSALTGR]) == 0)
		{
			newtab = TAB_CAPSALTGR;
		} else if (strcmp(ln, labels[TAB_ALT]) == 0)
		{
			newtab = TAB_ALT;
		} else if (strcmp(ln, labels[TAB_ALT]) == 0)
		{
			newtab = TAB_ALT;
		} else if (strcmp(ln, labels[TAB_ALTSHIFT]) == 0)
		{
			newtab = TAB_ALTSHIFT;
		} else if (strcmp(ln, labels[TAB_ALTCAPS]) == 0)
		{
			newtab = TAB_ALTCAPS;
		} else if (strcmp(ln, labels[TAB_DEADKEYS]) == 0)
		{
			newtab = TAB_DEADKEYS;
		} else if (strncmp(ln, "IFNE", 4) == 0)
		{
			continue;
		} else if (strncmp(ln, "ENDC", 4) == 0)
		{
			continue;
		} else
		{
			error_in_line(filename, lineno, 1, "illegal file format");
			free(line);
			return FALSE;
		}

		if (newtab >= 0)
		{
			if (tab >= 0 && tab < N_KEYTBL && tabsize[tab] == 0)
				copyfrom[tab] = newtab;
			tab = newtab;
			continue;
		}

		ln += 4;
		while (*ln == ' ' || *ln == '\t')
			ln++;

		while (*ln)
		{
			if (ln[0] == ';')
				break;

			if (ln[0] == '\'')
			{
				if (ln[2] != '\'')
				{
					error_in_line(filename, lineno, 1, "unmatched quotes");
					free(line);
					return FALSE;
				}
				num = (unsigned char)ln[1];
				ln += 3;
			} else if (ln[0] == '$')
			{
				if (!isxdigit(ln[1]))
				{
					error_in_line(filename, lineno, 1, "'%c' not a hex number", ln[1]);
					free(line);
					return FALSE;
				}

				ln++;
				num = strtoul(ln, &ln, 16);
			} else if (ln[0] == '0' && ln[1] == 'x')
			{
				if (!isxdigit(ln[3]))
				{
					error_in_line(filename, lineno, 1, "'%c' not a hex number", ln[3]);
					free(line);
					return FALSE;
				}

				ln += 2;
				num = strtoul(ln, &ln, 16);
			} else if (isdigit(ln[0]))
			{
				num = strtoul(ln, &ln, 10);
			} else if (strncmp(ln, "XXX", 3) == 0)
			{
				num = 0;
				ln += 3;
			} else if (strncmp(ln, "YYY", 3) == 0)
			{
				num = 0;
				ln += 3;
			} else if (strncmp(ln, "U2B", 3) == 0)
			{
				num = 0x7e;
				ln += 3;
			} else if (strncmp(ln, "S2B", 3) == 0)
			{
				num = 0x7c;
				ln += 3;
			} else if (strncmp(ln, "S29", 3) == 0)
			{
				num = 0x5e;
				ln += 3;
			} else
			{
				error_in_line(filename, lineno, 1, "unexpected '%c'", ln[0]);
				free(line);
				return FALSE;
			}

			if (w)
			{
				/*
				 * dc.w only used for MiNT to specify magics at filestart
				 */
				if (num > 65535L)
				{
					error_in_line(filename, lineno, 1, "number %ld is too big", num);
					free(line);
					return FALSE;
				}
				if (tab >= 0)
				{
					error_in_line(filename, lineno, 1, "too many magics");
					free(line);
					return FALSE;
				}
				if (is_mint == 0)
				{
					switch ((unsigned int) num)
					{
					case 0x2771:
						mint_expected = 1;
						error_in_line(filename, lineno, 0, "old format without AKP code");
						break;
					case 0x2772:
						mint_expected = 2;
						break;
					case 0x2773:
						mint_expected = 4;
						break;
					default:
						error_in_line(filename, lineno, 1, "invalid magic");
						free(line);
						return FALSE;
					}
				}
				if (is_mint == mint_expected)
				{
					/*
					 * MiNT keyboard tables start with 1, 2 or 4 words
					 */
					error_in_line(filename, lineno, 1, "too many magics");
					free(line);
					return FALSE;
				}
				if (is_mint == 1)
				{
					if (num > MAXAKP)
						error_in_line(filename, lineno, 0, "unknown country code %ld", num);
					else
						keytab_ctry_code = (int)num;
				}
				if (is_mint == 3)
				{
					int codeset = lookup_codeset((unsigned short) num);
					if (num == 0 || codeset < 0)
					{
						error_in_line(filename, lineno, 1, "invalid ISO code %u", (unsigned short) num);
						return FALSE;
					}
					keytab_codeset = codeset;
				}
				is_mint++;
				if (is_mint == mint_expected)
					tab = TAB_UNSHIFT;
			} else
			{
				if (num > 255)
					error_in_line(filename, lineno, 0, "number %ld is too big", num);
				if (tab < 0)
				{
					error_in_line(filename, lineno, 1, "no table");
					free(line);
					return FALSE;
				}
				if (tab == TAB_DEADKEYS)
				{
					if (tabsize[tab] >= MAX_DEADKEYS)
					{
						error_in_line(filename, lineno, 1, "too many dead keys");
						free(line);
						return FALSE;
					}
					if (atari_to_unicode[keytab_codeset][(unsigned char) num] == UNDEF)
						error_in_line(filename, lineno, 0, "character $%02x is undefined", (unsigned char) num);
					deadkeys[tabsize[tab]] = num;
					tabsize[tab]++;
				} else if (tab > TAB_DEADKEYS)
				{
					error_in_line(filename, lineno, 1, "too many tables");
					free(line);
					return FALSE;
				} else
				{
					if (tabsize[tab] >= MAX_SCANCODE)
					{
						if (is_mint && tab <= TAB_CAPS)
						{
							/* no keywords in mint tables; just switch to next one */
							tab++;
							if (tab == TAB_ALTGR)
								tab = TAB_ALT;
						} else if (!is_mint || tab <= TAB_CAPS)
						{
							error_in_line(filename, lineno, 1, "too many keys");
							free(line);
							return FALSE;
						}
					}
					if (is_mint && tab >= TAB_ALTGR)
					{
						/*
						 * in MiNT sources, all alt-tables are a scancode/value pair
						 */
						if (scancode == 0)
						{
							/*
							 * first value: scancode
							 */
							scancode = (unsigned char)num;
							if (num == 0)
							{
								/*
								 * end of table; automatically switch to next
								 */
								switch (tab)
								{
								case TAB_ALT:
									tab = TAB_ALTSHIFT;
									break;
								case TAB_ALTSHIFT:
									tab = TAB_ALTCAPS;
									break;
								case TAB_ALTCAPS:
									tab = TAB_ALTGR;
									break;
								case TAB_ALTGR:
									tab = TAB_DEADKEYS;
									break;
								case TAB_DEADKEYS:
									tab = TAB_DEADKEYS + 1;
									break;
								default:
									assert(0);
									break;
								}
							} else if (num >= MAX_SCANCODE)
							{
								error_in_line(filename, lineno, 1, "illegal scancode $%02x", (int)num);
								free(line);
								return FALSE;
							}
						} else
						{
							/*
							 * 2nd value: ascii code
							 */
							if (num == 0)
							{
								error_in_line(filename, lineno, 1, "illegal ascii code $%02x", (int)num);
								free(line);
								return FALSE;
							}
							if (scancode >= tabsize[tab])
								tabsize[tab] = scancode + 1;
							if (keytab[tab][scancode] != 0)
							{
								error_in_line(filename, lineno, 0, "duplicate scancode $%02x", (int)scancode);
							}
							if (atari_to_unicode[keytab_codeset][(unsigned char) num] == UNDEF)
								error_in_line(filename, lineno, 0, "character $%02x for scancode $%02x is undefined", (unsigned char) num, scancode);
							keytab[tab][scancode] = num;
							scancode = 0;
						}
					} else
					{
						if (atari_to_unicode[keytab_codeset][(unsigned char) num] == UNDEF)
							error_in_line(filename, lineno, 0, "character $%02x for scancode $%02x is undefined", (unsigned char) num, tabsize[tab]);
						keytab[tab][tabsize[tab]] = num;
						tabsize[tab]++;
					}
				}
			}

			while (*ln == ' ' || *ln == '\t')
				ln++;
			if (ln[0] == ',')
			{
				ln++;
				while (*ln == ' ' || *ln == '\t')
					ln++;
			}
		}
	}

	for (loop = 0; loop < N_KEYTBL; loop++)
	{
		for (tab = 0; tab < N_KEYTBL; tab++)
		{
			if (copyfrom[tab] >= 0 && tabsize[copyfrom[tab]] > 0)
			{
				memcpy(keytab[tab], keytab[copyfrom[tab]], MAX_SCANCODE);
				tabsize[tab] = tabsize[copyfrom[tab]];
			}
		}
	}

	keytab_format = is_mint ? FORMAT_MINT : FORMAT_MAGIC;

	free(line);
	return TRUE;
}

/* -------------------------------------------------------------------------- */

int mktbl_parse(FILE *fd, const char *filename)
{
	int tab;
	int r;
	int loop;

	for (tab = 0; tab < N_KEYTBL; tab++)
	{
		tabsize[tab] = 0;
		memset(keytab[tab], 0, MAX_SCANCODE);
	}
	tabsize[TAB_DEADKEYS] = 0;
	memset(deadkeys, 0, MAX_DEADKEYS);
	keytab_ctry_code = -1;
	keytab_codeset = 0;
	keytab_format = FORMAT_NONE;
	deadkeys_format = FORMAT_NONE;

	/*
	 * quick check for binary formats first
	 */
	if (getc(fd) == 0x27 &&
		((r = getc(fd)) == 0x71 || r == 0x72 || r == 0x73))
	{
		fseek(fd, 0, SEEK_SET);
		r = mktbl_load_mint(fd, filename);
	} else
	{
		fseek(fd, 0, SEEK_SET);
		/*
		 * MagiC *.sys don't have a magic number, but start with a
		 * full table, who's first two entries are 0x00, 0x1b in all cases
		 */
		if (getc(fd) == 0 && getc(fd) == 0x1b)
		{
			fseek(fd, 0, SEEK_SET);
			r = mktbl_load_magic(fd, filename);
		} else
		{
			/*
			 * no such luck. try parse text format
			 */
			fseek(fd, 0, SEEK_SET);
			r = mktbl_parse_source(fd, filename);
		}
	}
	if (r == FALSE)
		return FALSE;

	/*
	 * the altgr table is missing from a lot of sources
	 */
	if (keytab_format == FORMAT_MINT && tabsize[TAB_ALTGR] == 0)
	{
		keytab[TAB_ALTGR][0] = 0;
		tabsize[TAB_ALTGR] = 1;
	}

	for (tab = 0; tab < N_KEYTBL; tab++)
	{
		if (tab <= TAB_CAPS || keytab_format != FORMAT_MINT)
		{
			if (tabsize[tab] == 0)
				error_in_line(filename, 0, 0, "missing table for %s", labels[tab]);
			else if (tab <= TAB_CAPS && tabsize[tab] != MAX_SCANCODE)
				error_in_line(filename, 0, 0, "incomplete table for %s", labels[tab]);
		}
	}

	if (tabsize[TAB_DEADKEYS] > 0 && deadkeys_format == FORMAT_NONE)
	{
		deadkeys_format = FORMAT_MAGIC;
		/*
		 * Magic uses a list of only the used deadkeys first
		 */
		for (loop = 0; loop < tabsize[TAB_DEADKEYS] && deadkeys[loop] != 0; loop++)
		{
			if (strchr((const char *)possible_dead_keys, deadkeys[loop]) == NULL)
			{
				deadkeys_format = FORMAT_MINT;
				break;
			}
		}
		if (deadkeys[tabsize[TAB_DEADKEYS] - 1] != 0)
		{
			error_in_line(filename, 0, 0, "missing terminating zero in dead keys");
			deadkeys[tabsize[TAB_DEADKEYS]] = 0;
			tabsize[TAB_DEADKEYS]++;
		}
	}

	/* they have already been converted */
	for (tab = 0; tab < N_KEYTBL; tab++)
		tabsize[tab] = MAX_SCANCODE;

	if (keytab_ctry_code < 0)
	{
		static unsigned char const keytbl_us_norm[] = {
		       0, 0x1b,  '1',  '2',  '3',  '4',  '5',  '6',
		     '7',  '8',  '9',  '0',  '-',  '=',    8, 0x09,
		     'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
		     'o',  'p',  '[',  ']', 0x0d,    0,  'a',  's',
		     'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
		    '\'',  '`',    0, '\\',  'z',  'x',  'c',  'v',
		     'b',  'n',  'm',  ',',  '.',  '/',    0,    0,
		       0,  ' ',    0,    0,    0,    0,    0,    0,
		       0,    0,    0,    0,    0,    0,    0,    0,
		       0,    0,  '-',    0,    0,    0,  '+',    0,
		       0,    0,    0, 0x7f,    0,    0,    0,    0,
		       0,    0,    0,    0,    0,    0,    0,    0,
		       0,    0,    0,  '(',  ')',  '/',  '*',  '7',
		     '8',  '9',  '4',  '5',  '6',  '1',  '2',  '3',
		     '0',  '.', 0x0d,    0,    0,    0,    0,    0,
		       0,    0,    0,    0,    0,    0,    0,    0,
		};

		/*
		 * MagiC does not have a country code. Guess one.
		 */
		if (keytab[TAB_UNSHIFT][0x2b] == 0xf2 || keytab[TAB_UNSHIFT][0x28] == 0x9f)
		{
			keytab_ctry_code = COUNTRY_CZ;
			keytab_codeset = lookup_codeset(2);
		} else if (keytab[TAB_UNSHIFT][0x1a] == 0x81 && keytab[TAB_UNSHIFT][0x1b] == 0x2b)
		{
			keytab_ctry_code = COUNTRY_DE;
		} else if (keytab[TAB_UNSHIFT][0x1a] == 0x81 && keytab[TAB_UNSHIFT][0x1b] == 0xb9)
		{
			keytab_ctry_code = COUNTRY_SG;
		} else if (keytab[TAB_UNSHIFT][0x1a] == 0x8a && keytab[TAB_UNSHIFT][0x1b] == 0xb9)
		{
			keytab_ctry_code = COUNTRY_SF;
		} else if (keytab[TAB_UNSHIFT][0x28] == 0x97)
		{
			keytab_ctry_code = COUNTRY_FR;
		} else if (keytab[TAB_UNSHIFT][0x28] == 0x85)
		{
			keytab_ctry_code = COUNTRY_IT;
		} else if (keytab[TAB_UNSHIFT][0x27] == 0xa4)
		{
			keytab_ctry_code = COUNTRY_ES;
		} else if (keytab[TAB_UNSHIFT][0x28] == 0xb3)
		{
			keytab_ctry_code = COUNTRY_DK;
		} else if (keytab[TAB_UNSHIFT][0x28] == 0x84)
		{
			keytab_ctry_code = COUNTRY_FI; /* or swedish; they are identical */
		} else if (keytab[TAB_UNSHIFT][0x28] == 0x91)
		{
			keytab_ctry_code = COUNTRY_NO;
		} else if (keytab[TAB_UNSHIFT][0x28] == 0xa7)
		{
			keytab_ctry_code = COUNTRY_TR;
		} else if (keytab[TAB_UNSHIFT][0x28] == 0xe1)
		{
			keytab_ctry_code = COUNTRY_HU;
			keytab_codeset = lookup_codeset(2);
		} else if (keytab[TAB_SHIFT][0x04] == 0x9c)
		{
			keytab_ctry_code = COUNTRY_UK; /* or dutch; they are identical */
		} else if (keytab[TAB_ALT][0x18] == 0xf3)
		{
			keytab_ctry_code = COUNTRY_PL;
			keytab_codeset = lookup_codeset(2);
		} else if (keytab[TAB_ALT][0x28] == 0xfd || keytab[TAB_UNSHIFT][0x28] == 0xfd)
		{
			keytab_ctry_code = COUNTRY_RU;
			keytab_codeset = lookup_codeset(1251);
		} else if (keytab[TAB_ALT][0x30] == 0x99 || keytab[TAB_UNSHIFT][0x30] == 0x99)
		{
			keytab_ctry_code = COUNTRY_GR;
			keytab_codeset = lookup_codeset(737);
		} else if (memcmp(keytab[TAB_UNSHIFT], keytbl_us_norm, sizeof(keytbl_us_norm)) == 0)
		{
			keytab_ctry_code = COUNTRY_US;
		}
	}

	if (keytab_ctry_code == COUNTRY_GR && keytab_codeset == 0)
		keytab_codeset = lookup_codeset(737);
	if (keytab_ctry_code == COUNTRY_RU && keytab_codeset == 0)
		keytab_codeset = lookup_codeset(1251);
	if (keytab_codeset < 0)
		keytab_codeset = 0;

	return TRUE;
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

int is_deadkey(unsigned char c)
{
	int i;
	unsigned char deadchars[256 + 1];
	int n_deadchars;

	switch (deadkeys_format)
	{
	case FORMAT_MINT:
		/*
		 * The deadkey table structure is as follows:
		 * dd,bb,aa,dd,bb,aa,...,aa,bb,aa,0
		 * Where dd is the deadkey character, aa is the base
		 * character and aa the accented character.
		 * So '^','a',$83 means that '^' followed by 'a' results
		 * in $83.
		 */
		n_deadchars = 0;
		deadchars[0] = 0;
		for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 3)
		{
			if (strchr((char *) deadchars, deadkeys[i]) == NULL)
			{
				deadchars[n_deadchars++] = deadkeys[i];
				deadchars[n_deadchars] = 0;
			}
		}
		for (i = 0; i < n_deadchars; i++)
			if (c == deadchars[i])
				return i;
		break;
	case FORMAT_MAGIC:
		for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i++)
			if (c == deadkeys[i])
				return i;
		break;
	}
	return -1;
}

/* -------------------------------------------------------------------------- */

static void print_char(FILE *out, unsigned char c, int space)
{
	if (c >= 0x20 && c < 0x7f && c != 0x27 && c != 0x5c)
	{
		if (space)
			putc(' ', out);
		fprintf(out, "'%c'", c);
	} else
	{
		fprintf(out, "0x%02x", c);
	}
}

/* -------------------------------------------------------------------------- */

static void write_c_tbl(FILE *out, int tab, const char *suffix)
{
	int i;
	unsigned char c;
	int d;

	fprintf(out, "static const UBYTE keytbl_%s_%s[] = {\n", tblname, suffix);
	for (i = 0; i < MAX_SCANCODE; )
	{
		c = keytab[tab][i];
		if ((i % 8) == 0)
			fprintf(out, "    ");
		if (c == 0)
			fprintf(out, "   0");
		else if (deadkeys_format != FORMAT_NONE && (d = is_deadkey(c)) >= 0)
			fprintf(out, "D(%d)", d);
		else
			print_char(out, c, TRUE);
		++i;
		if (i != MAX_SCANCODE)
			fputc(',', out);
		if ((i % 8) == 0)
			fputc('\n', out);
		else
			fputc(' ', out);
	}
	fprintf(out, "};\n\n");
}

/* -------------------------------------------------------------------------- */

static void write_c_alt(FILE *out, int tab, const char *suffix)
{
	int i;
	unsigned char c;
	unsigned short unicode;

	fprintf(out, "static const UBYTE keytbl_%s_%s[] = {\n", tblname, suffix);

	for (i = 0; i < tabsize[tab]; i++)
	{
		c = keytab[tab][i];
		if (c == 0)
			continue;
		fprintf(out, "    0x%02x, ", i);
		print_char(out, c, TRUE);
		fputc(',', out);
		/* if (c >= 0x80) */
		{
			unicode = atari_to_unicode[keytab_codeset][c];
			fprintf(out, "    /* U+%04x %s */", unicode, unicode_name(unicode));
		}
		fputc('\n', out);
	}
	fprintf(out, "    0\n");
	fprintf(out, "};\n\n");
}

/* -------------------------------------------------------------------------- */

static void write_c_deadkeys(FILE *out)
{
	int i, j;
	unsigned char deadchars[256 + 1];
	int n_deadchars;
	unsigned short unicode;

	switch (deadkeys_format)
	{
	case FORMAT_MINT:
		n_deadchars = 0;
		deadchars[0] = 0;
		for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 3)
		{
			if (strchr((char *) deadchars, deadkeys[i]) == NULL)
			{
				deadchars[n_deadchars++] = deadkeys[i];
				deadchars[n_deadchars] = 0;
			}
		}
		for (j = 0; j < n_deadchars; j++)
		{
			fprintf(out, "static const UBYTE keytbl_%s_dead%d[] = {\n", tblname, j);
			for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 3)
			{
				if (deadkeys[i] == deadchars[j])
				{
					fprintf(out, "    ");
					print_char(out, deadkeys[i + 1], TRUE);
					fprintf(out, ", ");
					print_char(out, deadkeys[i + 2], TRUE);
					unicode = atari_to_unicode[keytab_codeset][deadkeys[i + 2]];
					fprintf(out, ",    /* U+%04x %s */\n", unicode, unicode_name(unicode));
				}
			}
			/* deadkey + space always gives deadkey */
			fprintf(out, "     ' ', ");
			print_char(out, deadchars[j], TRUE);
			fprintf(out, ",\n");
			/* deadkey + deadkey always gives deadkey */
			fprintf(out, "    D(%d), ", j);
			print_char(out, deadchars[j], TRUE);
			fprintf(out, ",\n");
			fprintf(out, "    0\n");
			fprintf(out, "};\n\n");
		}
		break;
	case FORMAT_MAGIC:
		n_deadchars = 0;
		deadchars[0] = 0;
		for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i++)
		{
			deadchars[n_deadchars++] = deadkeys[i];
		}
		i++;
		for (j = 0; j < n_deadchars; j++)
		{
			fprintf(out, "static const UBYTE keytbl_%s_dead%d[] = {\n", tblname, j);
			for (; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 2)
			{
				fprintf(out, "    ");
				print_char(out, deadkeys[i + 0], TRUE);
				fprintf(out, ", ");
				print_char(out, deadkeys[i + 1], TRUE);
				unicode = atari_to_unicode[keytab_codeset][deadkeys[i + 1]];
				fprintf(out, ",    /* U+%04x %s */\n", unicode, unicode_name(unicode));
			}
			/* deadkey + space always gives deadkey */
			fprintf(out, "     ' ', ");
			print_char(out, deadchars[j], TRUE);
			fprintf(out, ",\n");
			/* deadkey + deadkey always gives deadkey */
			fprintf(out, "    D(%d), ", j);
			print_char(out, deadchars[j], TRUE);
			fprintf(out, ",\n");
			fprintf(out, "    0\n");
			fprintf(out, "};\n\n");
			i++;
		}
		break;
	default:
		n_deadchars = 0;
		break;
	}

	fprintf(out, "static const UBYTE * const keytbl_%s_dead[] = {\n", tblname);
	for (j = 0; j < n_deadchars; j++)
	{
		fprintf(out, "    keytbl_%s_dead%d%s\n", tblname, j, j + 1 < n_deadchars ? "," : "");
	}
	fprintf(out, "};\n\n");
}

/* -------------------------------------------------------------------------- */

void mktbl_write_c_src(FILE *out)
{
	int dual_keyboard = 0;

	if (deadkeys_format == FORMAT_MAGIC)
		conv_magic_deadkeys(deadkeys);
	if (deadkeys_format != FORMAT_NONE)
	{
		fprintf(out, "#define D(i) DEAD(i)\n\n");
	}

	switch (keytab_ctry_code)
	{
		case COUNTRY_US: tblname = "us"; break;
		case COUNTRY_DE: tblname = "de"; break;
		case COUNTRY_FR: tblname = "fr"; break;
		case COUNTRY_UK: tblname = "uk"; break;
		case COUNTRY_ES: tblname = "es"; break;
		case COUNTRY_IT: tblname = "it"; break;
		case COUNTRY_SE: tblname = "se"; break;
		case COUNTRY_SF: tblname = "sf"; break;
		case COUNTRY_SG: tblname = "sg"; break;
		case COUNTRY_TR: tblname = "tr"; break;
		case COUNTRY_FI: tblname = "fi"; break;
		case COUNTRY_NO: tblname = "no"; break;
		case COUNTRY_DK: tblname = "dk"; break;
		case COUNTRY_SA: tblname = "sa"; break;
		case COUNTRY_NL: tblname = "nl"; break;
		case COUNTRY_CZ: tblname = "cz"; break;
		case COUNTRY_HU: tblname = "hu"; break;
		case COUNTRY_PL: tblname = "pl"; break;
		case COUNTRY_LT: tblname = "lt"; break;
		case COUNTRY_RU: tblname = "ru"; dual_keyboard = 1; break;
		case COUNTRY_EE: tblname = "ee"; break;
		case COUNTRY_BY: tblname = "by"; break;
		case COUNTRY_UA: tblname = "ua"; break;
		case COUNTRY_SK: tblname = "sk"; break;
		case COUNTRY_RO: tblname = "ro"; break;
		case COUNTRY_BG: tblname = "bg"; break;
		case COUNTRY_SI: tblname = "si"; break;
		case COUNTRY_HR: tblname = "hr"; break;
		case COUNTRY_RS: tblname = "rs"; break;
		case COUNTRY_ME: tblname = "me"; break;
		case COUNTRY_MK: tblname = "mk"; break;
		case COUNTRY_GR: tblname = "gr"; dual_keyboard = 1; break;
		case COUNTRY_LV: tblname = "lv"; break;
		case COUNTRY_IL: tblname = "il"; break;
		case COUNTRY_ZA: tblname = "za"; break;
		case COUNTRY_PT: tblname = "pt"; break;
		case COUNTRY_BE: tblname = "be"; break;
		case COUNTRY_JP: tblname = "jp"; break;
		case COUNTRY_CN: tblname = "cn"; break;
		case COUNTRY_KR: tblname = "kr"; break;
		case COUNTRY_VN: tblname = "vn"; break;
		case COUNTRY_IN: tblname = "in"; break;
		case COUNTRY_IR: tblname = "ir"; break;
		case COUNTRY_MN: tblname = "mn"; break;
		case COUNTRY_NP: tblname = "np"; break;
		case COUNTRY_LA: tblname = "la"; break;
		case COUNTRY_KH: tblname = "kh"; break;
		case COUNTRY_ID: tblname = "id"; break;
		case COUNTRY_BD: tblname = "bd"; break;
		case -1:
			error_in_line(NULL, 0, 0, "country code could not be determined");
			break;
		default:
			error_in_line(NULL, 0, 0, "unknown country code %d", keytab_ctry_code);
			break;
	}

	write_c_tbl(out, TAB_UNSHIFT, "norm");
	write_c_tbl(out, TAB_SHIFT, "shft");
	write_c_tbl(out, TAB_CAPS, "caps");
	if (dual_keyboard)
	{
		write_c_tbl(out, TAB_ALT, "altnorm");
		write_c_tbl(out, TAB_ALTSHIFT, "altshft");
		write_c_tbl(out, TAB_ALTCAPS, "altcaps");
	} else
	{
		write_c_alt(out, TAB_ALT, "altnorm");
		write_c_alt(out, TAB_ALTSHIFT, "altshft");
		write_c_alt(out, TAB_ALTCAPS, "altcaps");
	}
	if (deadkeys_format != FORMAT_NONE)
		write_c_deadkeys(out);

	fprintf(out, "\n");
	fprintf(out, "static const struct keytbl keytbl_%s = {\n", tblname);
	fprintf(out, "    keytbl_%s_norm,\n", tblname);
	fprintf(out, "    keytbl_%s_shft,\n", tblname);
	fprintf(out, "    keytbl_%s_caps,\n", tblname);
	fprintf(out, "    keytbl_%s_altnorm,\n", tblname);
	fprintf(out, "    keytbl_%s_altshft,\n", tblname);
	fprintf(out, "    keytbl_%s_altcaps,\n", tblname);
	if (deadkeys_format != FORMAT_NONE)
		fprintf(out, "    keytbl_%s_dead,\n", tblname);
	else
		fprintf(out, "    NULL,\n");
	fprintf(out, "    %s\n", dual_keyboard ? "DUAL_KEYBOARD" : "0");
	fprintf(out, "};\n");
	if (deadkeys_format != FORMAT_NONE)
	{
		fprintf(out, "\n#undef D\n");
	}
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

static void fputm(unsigned short num, FILE *out)
{
	fputc(num >> 8, out);
	fputc(num, out);
}

/* -------------------------------------------------------------------------- */

static void write_alttab(FILE *out, int tab)
{
	int i;
	unsigned char c;

	for (i = 0; i < tabsize[tab]; i++)
	{
		c = keytab[tab][i];
		if (c != 0)
		{
			fputc(i, out);
			fputc(c, out);
		}
	}
	fputc(0, out);
}

/* -------------------------------------------------------------------------- */

int mktbl_write_mint(FILE *out)
{
	int tab;

	if (deadkeys_format == FORMAT_MAGIC)
		conv_magic_deadkeys(deadkeys);
	if (keytab_codeset > 0)
	{
		fputm(0x2773, out);
		fputm(keytab_ctry_code, out);
		fputm(0, out);
		fputm(keytab_codesets[keytab_codeset], out);
	} else
	{
		fputm(0x2772, out);
		fputm(keytab_ctry_code, out);
	}
	for (tab = 0; tab <= TAB_CAPS; tab++)
		if (fwrite(keytab[tab], MAX_SCANCODE, 1, out) != 1)
			{}
	write_alttab(out, TAB_ALT);
	write_alttab(out, TAB_ALTSHIFT);
	write_alttab(out, TAB_ALTCAPS);
	write_alttab(out, TAB_ALTGR);
	if (tabsize[TAB_DEADKEYS] != 0 && fwrite(deadkeys, tabsize[TAB_DEADKEYS], 1, out) != 1)
		return FALSE;
	return TRUE;
}

/* -------------------------------------------------------------------------- */

int mktbl_write_magic(FILE *out)
{
	int tab;
	const unsigned char *dead;
	int deadsize;
	unsigned char tmp[MAX_DEADKEYS];

	if (deadkeys_format == FORMAT_MINT)
	{
		deadsize = conv_mint_deadkeys(tmp);
		dead = tmp;
	} else if (deadkeys_format == FORMAT_MAGIC)
	{
		dead = deadkeys;
		deadsize = tabsize[TAB_DEADKEYS];
		if (deadsize == 0)
		{
			deadkeys[0] = 0;
			deadkeys[1] = 0;
			deadsize = 2;
		}
	} else
	{
		dead = deadkeys;
		deadsize = 0;
	}
	for (tab = 0; tab < N_KEYTBL; tab++)
		if (fwrite(keytab[tab], MAX_SCANCODE, 1, out) != 1)
			return FALSE;
	if (deadsize != 0 && fwrite(dead, deadsize, 1, out) != 1)
		return FALSE;
	return TRUE;
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

static void print_asm_char(FILE *out, unsigned char c)
{
	if (c >= 0x20 && c < 0x7f && c != 0x27 && c != 0x5c)
	{
		fprintf(out, "'%c'", c);
	} else
	{
		fprintf(out, "$%02x", c);
	}
}

/* -------------------------------------------------------------------------- */

static void write_asm_tbl(FILE *out, int tab, const char *name)
{
	int i;
	unsigned char c;

	fprintf(out, "%s\n", name);
	for (i = 0; i < MAX_SCANCODE; )
	{
		c = keytab[tab][i];
		if ((i % 8) == 0)
			fprintf(out, "\tdc.b\t");
		else
			fputc(',', out);
		print_asm_char(out, c);
		++i;
		if ((i % 8) == 0)
			fputc('\n', out);
	}
	fprintf(out, "\n");
}

/* -------------------------------------------------------------------------- */

static void write_asm_alt(FILE *out, int tab, const char *name)
{
	int i;
	unsigned char c;
	unsigned short unicode;

	fprintf(out, "%s\n", name);
	for (i = 0; i < tabsize[tab]; i++)
	{
		c = keytab[tab][i];
		if (c == 0)
			continue;
		fprintf(out, "\tdc.b\t$%02x,", i);
		print_asm_char(out, c);
		/* if (c >= 0x80) */
		{
			unicode = atari_to_unicode[keytab_codeset][c];
			fprintf(out, "    ; U+%04x %s", unicode, unicode_name(unicode));
		}
		fputc('\n', out);
	}
	fprintf(out, "\tdc.b\t0\n");
	fprintf(out, "\n");
}

/* -------------------------------------------------------------------------- */

static void write_asm_deadkeys(FILE *out, const char *name, int format, const unsigned char *src, int size)
{
	int i, j;
	int n_deadchars;
	unsigned char deadchars[256 + 1];
	unsigned short unicode;

	fprintf(out, "%s\n", name);
	switch (format)
	{
	case FORMAT_MINT:
		n_deadchars = 0;
		deadchars[0] = 0;
		for (i = 0; i < size && src[i] != 0; i += 3)
		{
			if (strchr((char *) deadchars, src[i]) == NULL)
			{
				deadchars[n_deadchars++] = src[i];
				deadchars[n_deadchars] = 0;
			}
		}
		for (j = 0; j < n_deadchars; j++)
		{
			for (i = 0; i < size && src[i] != 0; i += 3)
			{
				if (src[i] == deadchars[j])
				{
					fprintf(out, "\tdc.b\t");
					print_asm_char(out, src[i + 0]);
					fprintf(out, ",");
					print_asm_char(out, src[i + 1]);
					fprintf(out, ",");
					print_asm_char(out, src[i + 2]);
					unicode = atari_to_unicode[keytab_codeset][src[i + 2]];
					fprintf(out, "    ; U+%04x %s\n", unicode, unicode_name(unicode));
				}
			}
			fprintf(out, "\n");
		}
		fprintf(out, "\tdc.b\t0\n");
		break;
	case FORMAT_MAGIC:
		n_deadchars = 0;
		fprintf(out, "\tdc.b\t");
		for (i = 0; i < size && src[i] != 0; i++)
		{
			print_asm_char(out, src[i + 0]);
			fprintf(out, ",");
			n_deadchars++;
		}
		fprintf(out, "0\n");
		i++;
		for (j = 0; j < n_deadchars; j++)
		{
			fprintf(out, "\n");
			for (; i < size && src[i] != 0; i += 2)
			{
				fprintf(out, "\tdc.b\t");
				print_asm_char(out, src[i + 0]);
				fprintf(out, ",");
				print_asm_char(out, src[i + 1]);
				unicode = atari_to_unicode[keytab_codeset][src[i + 1]];
				fprintf(out, "    ; U+%04x %s\n", unicode, unicode_name(unicode));
			}
			fprintf(out, "\tdc.b\t0\n");
			i++;
		}
		break;
	default:
		n_deadchars = 0;
		break;
	}
}

/* -------------------------------------------------------------------------- */

int mktbl_write_mint_source(FILE *out)
{
	if (deadkeys_format == FORMAT_MAGIC)
		conv_magic_deadkeys(deadkeys);
	fprintf(out, "; AKP %d\n", keytab_ctry_code);
	fprintf(out, "\n");
	if (keytab_codeset > 0)
	{
		fprintf(out, "\tdc.w\t$%04x\n", 0x2773);
		fprintf(out, "\tdc.w\t$%04x\n", keytab_ctry_code);
		fprintf(out, "\tdc.w\t%u,%u\n", 0, keytab_codesets[keytab_codeset]);
	} else
	{
		fprintf(out, "\tdc.w\t$%04x\n", 0x2772);
		fprintf(out, "\tdc.w\t$%04x\n", keytab_ctry_code);
	}
	fprintf(out, "\n");
	write_asm_tbl(out, TAB_UNSHIFT, "; Unshifted\n");
	write_asm_tbl(out, TAB_SHIFT, "; Shifted\n");
	write_asm_tbl(out, TAB_CAPS, "; Caps\n");
	write_asm_alt(out, TAB_ALT, "; Alternate\n");
	write_asm_alt(out, TAB_ALTSHIFT, "; Alt + shift\n");
	write_asm_alt(out, TAB_ALTCAPS, "; Alt + caps\n");
	write_asm_alt(out, TAB_ALTCAPS, "; AltGr\n");
	if (deadkeys_format != FORMAT_NONE)
		write_asm_deadkeys(out, "; Deadkeys\n", deadkeys_format, deadkeys, tabsize[TAB_DEADKEYS]);

	return TRUE;
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

int mktbl_write_magic_source(FILE *out)
{
	int tab;
	unsigned char tmp[MAX_DEADKEYS];
	int deadsize;

	fprintf(out, "; AKP %d\n", keytab_ctry_code);
	fprintf(out, "\n");
	for (tab = 0; tab < N_KEYTBL; tab++)
		write_asm_tbl(out, tab, labels[tab]);
	fprintf(out, " IFNE DEADKEYS\n");
	if (deadkeys_format == FORMAT_MINT)
	{
		deadsize = conv_mint_deadkeys(tmp);
		write_asm_deadkeys(out, labels[TAB_DEADKEYS], FORMAT_MAGIC, tmp, deadsize);
	} else if (deadkeys_format == FORMAT_MAGIC)
	{
		write_asm_deadkeys(out, labels[TAB_DEADKEYS], deadkeys_format, deadkeys, tabsize[TAB_DEADKEYS]);
	} else
	{
		tmp[0] = 0;
		write_asm_deadkeys(out, labels[TAB_DEADKEYS], FORMAT_MAGIC, tmp, 1);
	}
	fprintf(out, " ENDC\n");

	return FALSE;
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

const unsigned char *conv_table(int tab, const unsigned char *src)
{
	unsigned char *table = keytab[tab];
	const unsigned char *s = src;

	while (*src != 0)
	{
		unsigned char scan = *src++;
		if (scan >= MAX_SCANCODE)
			return src;
		table[scan] = *src++;
	}
	src++;
	tabsize[tab] = (int)(src - s);
	return src;
}

/* -------------------------------------------------------------------------- */

/*
 * convert deadkeys from MagiC format to MiNT
 */
void conv_magic_deadkeys(const unsigned char *src)
{
	unsigned char *table = deadkeys;
	const unsigned char *deadkey_chars;
	unsigned char tmp[MAX_DEADKEYS];

	if (src == deadkeys)
	{
		memcpy(tmp, deadkeys, tabsize[TAB_DEADKEYS]);
		src = tmp;
	}
	deadkey_chars = src;
	while (*src != 0)
		src++;
	src++;
	while (*deadkey_chars != 0)
	{
		while (*src != 0)
		{
			if (table < &deadkeys[MAX_DEADKEYS - 4])
			{
				*table++ = *deadkey_chars;
				*table++ = *src++;
				*table++ = *src++;
			}
		}
		src++;
		deadkey_chars++;
	}
	*table++ = 0;
	tabsize[TAB_DEADKEYS] = (int)(table - deadkeys);
	deadkeys_format = FORMAT_MINT;
}

/* -------------------------------------------------------------------------- */

/*
 * convert deadkeys from MiNT format to MagiC
 */
int conv_mint_deadkeys(unsigned char *tmp)
{
	int i, j;
	int n_deadchars;
	int deadsize;

	/*
	 * collect a set of unique dead keys
	 */
	deadsize = 0;
	tmp[0] = 0;
	for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 3)
	{
		if (strchr((char *) tmp, deadkeys[i]) == NULL)
		{
			tmp[deadsize++] = deadkeys[i];
			tmp[deadsize] = 0;
		}
	}
	n_deadchars = deadsize;
	tmp[deadsize++] = 0;
	for (j = 0; j < n_deadchars; j++)
	{
		for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 3)
		{
			if (deadkeys[i] == tmp[j])
			{
				tmp[deadsize++] = deadkeys[i + 1];
				tmp[deadsize++] = deadkeys[i + 2];
			}
		}
		tmp[deadsize++] = 0;
	}
	return deadsize;
}

/* -------------------------------------------------------------------------- */

void copy_mint_deadkeys(const unsigned char *src)
{
	unsigned char *table = deadkeys;

	while (*src != 0 && table < &deadkeys[MAX_DEADKEYS - 4])
	{
		*table++ = *src++;
		*table++ = *src++;
		*table++ = *src++;
	}
	*table++ = 0;
	tabsize[TAB_DEADKEYS] = (int)(table - deadkeys);
}

/******************************************************************************/
/* -------------------------------------------------------------------------- */
/******************************************************************************/

#ifndef NO_MAIN

#ifdef WITH_GUI
# error "GUI for main program not supported yet"
#endif

static char const progname[] = "mktbl";

static void usage(FILE *fp)
{
	fprintf(fp, "Usage: %s [-c|-m|-M|-a|-A] src-file [tbl-file]\n", progname);
}


int main(int argc, char **argv)
{
	char *outname = NULL;
	FILE *infile;
	FILE *out;
	const char *filename;
	int output_format = FORMAT_NONE;
	int r;

	/*
	 * very minimalistic option processing
	 */
	argc--;
	argv++;
	while (argc > 0 && argv[0][0] == '-')
	{
		if (argv[0][1] == '\0')
		{
			break;
		} else if (strcmp(argv[0], "-c") == 0)
		{
			output_format = FORMAT_C_SOURCE;
			argv++;
			argc--;
		} else if (strcmp(argv[0], "-m") == 0)
		{
			output_format = FORMAT_MINT;
			argv++;
			argc--;
		} else if (strcmp(argv[0], "-M") == 0)
		{
			output_format = FORMAT_MINT_SOURCE;
			argv++;
			argc--;
		} else if (strcmp(argv[0], "-a") == 0)
		{
			output_format = FORMAT_MAGIC;
			argv++;
			argc--;
		} else if (strcmp(argv[0], "-A") == 0)
		{
			output_format = FORMAT_MAGIC_SOURCE;
			argv++;
			argc--;
#ifdef WITH_CHECKS
		} else if (strcmp(argv[0], "--check") == 0)
		{
			int ret = unitable_selfcheck();
#if WITH_CHECKS >= 2
			ret &= unifont_check();
#endif
			return ret ? EXIT_SUCCESS : EXIT_FAILURE;
#endif
		} else if (strcmp(argv[0], "--help") == 0)
		{
			usage(stdout);
			return EXIT_SUCCESS;
		} else
		{
			fprintf(stderr, "%s: unknown option %s\n", progname, argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (argc == 0)
	{
		usage(stderr);
		return EXIT_FAILURE;
	} else if (argc == 1)
	{
		filename = argv[0];
	} else if (argc == 2)
	{
		filename = argv[0];
		outname = argv[1];
	} else
	{
		fprintf(stderr, "%s: too many arguments\n", progname);
		return EXIT_FAILURE;
	}

	if (strcmp(filename, "-") == 0)
	{
		infile = stdin;
		filename = "<stdin>";
	} else
	{
		infile = fopen(filename, "rb");
	}
	if (infile == NULL)
	{
		fprintf(stderr, "%s: %s\n", filename, strerror(errno));
		return EXIT_FAILURE;
	}

	r = mktbl_parse(infile, filename);
	if (infile != stdin)
		fclose(infile);

	if (r == FALSE)
	{
		return EXIT_FAILURE;
	}

	if (output_format == FORMAT_NONE)
	{
		output_format = keytab_format;
	}

	if (outname == NULL && infile != stdin)
	{
		const char *ext =
			output_format == FORMAT_C_SOURCE ? ".h" : 
			output_format == FORMAT_MINT ? ".tbl" :
			output_format == FORMAT_MINT_SOURCE ? ".src" :
			output_format == FORMAT_MAGIC ? ".sys" :
			output_format == FORMAT_MAGIC_SOURCE ? ".inc" :
			".xxx";
		char *o;
		long flen;

		flen = strlen(filename);
		outname = malloc(flen + 6);
		if (outname == NULL)
		{
			fprintf(stderr, "%s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		strcpy(outname, filename);
		o = strrchr(outname, '.');
		if (o == NULL)
			strcat(outname, ext);
		else
			strcpy(o, ext);

		printf("%s: output to %s\n", filename, outname);
	}

	if (outname == NULL || strcmp(outname, "-") == 0)
	{
		out = stdout;
		if ((output_format == FORMAT_MINT || output_format == FORMAT_MAGIC) &&
			isatty(fileno(out)))
		{
			fprintf(stderr, "%s: refusing to output binary data to a terminal\n", progname);
			return EXIT_FAILURE;
		}
	} else
	{
		out = fopen(outname, "wb");
	}
	if (out == NULL)
	{
		fprintf(stderr, "%s: %s\n", outname, strerror(errno));
		return EXIT_FAILURE;
	}

	switch (output_format)
	{
	case FORMAT_C_SOURCE:
		mktbl_write_c_src(out);
		break;

	case FORMAT_MINT:
		mktbl_write_mint(out);
		break;

	case FORMAT_MAGIC:
		mktbl_write_magic(out);
		break;

	case FORMAT_MINT_SOURCE:
		mktbl_write_mint_source(out);
		break;

	case FORMAT_MAGIC_SOURCE:
		mktbl_write_magic_source(out);
		break;
	}

	fflush(out);
	if (out != stdout)
		fclose(out);

	return EXIT_SUCCESS;
}

#endif /* MAIN */
