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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


#define TAB_UNSHIFT   0
#define TAB_SHIFT     1
#define TAB_CAPS      2
#define TAB_ALTGR     3
#define TAB_SHALTGR   4
#define TAB_CAPSALTGR 5
#define TAB_ALT       6
#define TAB_SHALT     7
#define TAB_CAPSALT   8
#define N_KEYTBL      9
#define TAB_DEADKEYS  N_KEYTBL

#define MAX_SCANCODE 128
static unsigned char keytab[N_KEYTBL][MAX_SCANCODE];

#define MAX_DEADKEYS 2048
static unsigned char deadkeys[MAX_DEADKEYS];
static int tabsize[N_KEYTBL + 1];
static int copyfrom[N_KEYTBL];
static const char *tblname = "xx";
static const char *progname;

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
	0xb8,  /* U+00b8 Cedilla (latin-2) */
	0xb7,  /* U+00b7 Middle dot (latin-1) */
	       /* U+201a Single low-9 quotation mark */
	0
};

#define FORMAT_NONE     0
#define FORMAT_MAGIC    1
#define FORMAT_MINT     2
#define FORMAT_C_SOURCE 3

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


static int is_deadkey(unsigned char c, int deadkeys_format)
{
	int i;
	unsigned char deadchars[256 + 1];
	int n_deadchars;

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


static void print_char(FILE *out, unsigned char c)
{
	if (c >= 0x20 && c < 0x7f && c != 0x27 && c != 0x5c)
		fprintf(out, " '%c'", c);
	else
		fprintf(out, "0x%02x", c);
}


static void write_c_tbl(FILE *out, int tab, const char *suffix, int deadkeys_format)
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
		else if (deadkeys_format != FORMAT_NONE && (d = is_deadkey(c, deadkeys_format)) >= 0)
			fprintf(out, "DEAD(%d)", d);
		else
			fputc(' ', out), print_char(out, c);
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


static void write_c_alt(FILE *out, int tab, const char *suffix)
{
	int i;
	unsigned char c;

	fprintf(out, "static const UBYTE keytbl_%s_%s[] = {\n", tblname, suffix);

	for (i = 0; i < tabsize[tab]; i++)
	{
		c = keytab[tab][i];
		if (c == 0)
			continue;
		fprintf(out, "    0x%02x, ", i);
		print_char(out, c);
		fprintf(out, ",\n");
	}
	fprintf(out, "    0\n");
	fprintf(out, "};\n\n");
}


static void write_deadkeys(FILE *out, int deadkeys_format)
{
	int i, j;
	char deadchars[256 + 1];
	int n_deadchars;

	switch (deadkeys_format)
	{
	case FORMAT_MINT:
		n_deadchars = 0;
		deadchars[0] = 0;
		for (i = 0; i < tabsize[TAB_DEADKEYS] && deadkeys[i] != 0; i += 3)
		{
			if (strchr(deadchars, deadkeys[i]) == NULL)
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
					print_char(out, deadkeys[i + 1]);
					fprintf(out, ", ");
					print_char(out, deadkeys[i + 2]);
					fprintf(out, ",\n");
				}
			}
			/* deadkey + space always gives deadkey */
			fprintf(out, "    ' ', ");
			print_char(out, deadchars[j]);
			fprintf(out, ",\n");
			/* deadkey + deadkey always gives deadkey */
			fprintf(out, "    DEAD(%d), ", j);
			print_char(out, deadchars[j]);
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
				print_char(out, deadkeys[i + 1]);
				fprintf(out, ", ");
				print_char(out, deadkeys[i + 2]);
				fprintf(out, ",\n");
			}
			/* deadkey + space always gives deadkey */
			fprintf(out, "    ' ', ");
			print_char(out, deadchars[j]);
			fprintf(out, ",\n");
			/* deadkey + deadkey always gives deadkey */
			fprintf(out, "    DEAD(%d), ", j);
			print_char(out, deadchars[j]);
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

	fprintf(out, "static const UBYTE * keytbl_%s_dead[] = {\n", tblname);
	for (j = 0; j < n_deadchars; j++)
	{
		fprintf(out, "    keytbl_%s_dead%d%s\n", tblname, j, j + 1 < n_deadchars ? "," : "");
	}
	fprintf(out, "};\n\n");
}


static void write_c_src(FILE *out, int deadkeys_format)
{
	write_c_tbl(out, TAB_UNSHIFT, "norm", deadkeys_format);
	write_c_tbl(out, TAB_SHIFT, "shft", deadkeys_format);
	write_c_tbl(out, TAB_CAPS, "caps", deadkeys_format);
	write_c_alt(out, TAB_ALT, "altnorm");
	write_c_alt(out, TAB_SHALT, "altshft");
	write_c_alt(out, TAB_CAPSALT, "altcaps");
	if (deadkeys_format != FORMAT_NONE)
		write_deadkeys(out, deadkeys_format);

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
	fprintf(out, "    0\n");
	fprintf(out, "};\n");
}


static void usage(void)
{
	printf("Usage: %s [-c] src-file [tbl-file]\n", progname);
}


int main(int argc, char **argv)
{
	char *line;
	char *ln;
	char *outname = NULL;
	char *o;
	int r;
	long lineno;
	long num;
	int scancode = 0;
	long flen;
	short w;
	size_t buf;
	FILE *fd;
	FILE *out = 0;
	int tab, newtab;
	int loop;
	const char *filename;
	int is_mint = 0;
	int mint_expected = 0;
	short mint_magics[4];
	int output_format = FORMAT_NONE;
	int deadkeys_format;

	/*
	 * very minimalistic option processing
	 */
	argc--;
	progname = *argv++;
	while (argc > 0 && argv[0][0] == '-')
	{
		if (strcmp(argv[0], "-c") == 0)
		{
			output_format = FORMAT_C_SOURCE;
			argv++;
			argc--;
		} else if (strcmp(argv[0], "--help") == 0)
		{
			usage();
			return 0;
		} else
		{
			fprintf(stderr, "%s: unknown option %s\n", progname, argv[0]);
			return 1;
		}
	}

	if (argc == 0)
	{
		usage();
		return 1;
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
		return 1;
	}

	buf = 1024;
	line = malloc(buf);					/* should be plenty */
	assert(line);

	fd = fopen(filename, "r");
	if (!fd)
	{
		fprintf(stderr, "%s: %s\n", filename, strerror(errno));
		return 3;
	}

	for (tab = 0; tab < N_KEYTBL; tab++)
		copyfrom[tab] = -1;
	lineno = 0;
	tab = -1;
	w = 0;
	
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
		} else if (strcmp(ln, labels[TAB_SHALT]) == 0)
		{
			newtab = TAB_SHALT;
		} else if (strcmp(ln, labels[TAB_CAPSALT]) == 0)
		{
			newtab = TAB_CAPSALT;
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
			fprintf(stderr, "%s:%ld: warning: unknown statement: %s\n", filename, lineno, line);
			continue;
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
					fprintf(stderr, "%s:%ld: error: unmatched quotes: %s\n", filename, lineno, line);
					r = 5;
					goto error;
				}
				num = (unsigned char)ln[1];
				ln += 3;
			} else if (ln[0] == '$')
			{
				if (!isxdigit(ln[1]))
				{
					fprintf(stderr, "%s:%ld: error: '%c' is not a hex number: %s\n", filename, lineno, ln[1], line);
					r = 6;
					goto error;
				}

				ln++;
				num = strtoul(ln, &ln, 16);
			} else if (ln[0] == '0' && ln[1] == 'x')
			{
				if (!isxdigit(ln[3]))
				{
					fprintf(stderr, "%s:%ld: error: '%c' is not a hex number: %s\n", filename, lineno, ln[3], line);
					r = 7;
					goto error;
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
				fprintf(stderr, "%s:%ld: error: unexpected '%c': %s\n", filename, lineno, ln[0], line);
				r = 8;
				goto error;
			}

			if (w)
			{
				/*
				 * dc.w only used for MiNT to specify magics at filestart
				 */
				if (num > 65535L)
				{
					fprintf(stderr, "%s:%ld: error: number %ld is too big\n", filename, lineno, num);
					r = 1;
					goto error;
				}
				if (tab >= 0)
				{
					fprintf(stderr, "%s:%ld: error: too many magics: %s\n", filename, lineno, line);
					r = 1;
					goto error;
				}
				if (is_mint == 0)
				{
					switch ((unsigned int) num)
					{
					case 0x2771:
						mint_expected = 1;
						fprintf(stderr, "%s:%ld: warning: old format without AKP code: %s\n", filename, lineno, line);
						break;
					case 0x2772:
						mint_expected = 2;
						break;
					case 0x2773:
						mint_expected = 4;
						break;
					default:
						fprintf(stderr, "%s:%ld: error: invalid magic: %s\n", filename, lineno, line);
						r = 1;
						goto error;
					}
				}
				if (is_mint == mint_expected)
				{
					/*
					 * MinT keyboard tables start with 1, 2 or 4 words
					 */
					fprintf(stderr, "%s:%ld: error: too many magics: %s\n", filename, lineno, line);
					r = 1;
					goto error;
				}
				mint_magics[is_mint] = num;
				is_mint++;
				if (is_mint == mint_expected)
					tab = TAB_UNSHIFT;
			} else
			{
				if (num > 255)
					fprintf(stderr, "%s:%ld: warning: number %ld is too big\n", filename, lineno, num);
				if (tab < 0)
				{
					fprintf(stderr, "%s:%ld: error: no table: %s\n", filename, lineno, line);
					r = 1;
					goto error;
				}
				if (tab == TAB_DEADKEYS)
				{
					if (tabsize[tab] >= MAX_DEADKEYS)
					{
						fprintf(stderr, "%s:%ld: error: too many dead keys: %s\n", filename, lineno, line);
						r = 1;
						goto error;
					}
					deadkeys[tabsize[tab]] = num;
					tabsize[tab]++;
				} else if (tab > TAB_DEADKEYS)
				{
					fprintf(stderr, "%s:%ld: error: too many tables: %s\n", filename, lineno, line);
					r = 1;
					goto error;
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
							fprintf(stderr, "%s:%ld: error: too many keys: %s\n", filename, lineno, line);
							r = 1;
							goto error;
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
									tab = TAB_SHALT;
									break;
								case TAB_SHALT:
									tab = TAB_CAPSALT;
									break;
								case TAB_CAPSALT:
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
							} else if (num <= 0 || num >= MAX_SCANCODE)
							{
								fprintf(stderr, "%s:%ld: error: illegal scancode $%02x: %s\n", filename, lineno, (int)num, line);
								r = 1;
								goto error;
							}
						} else
						{
							/*
							 * 2nd value: ascii code
							 */
							if (num == 0)
							{
								fprintf(stderr, "%s:%ld: error: illegal ascii code $%02x: %s\n", filename, lineno, (int)num, line);
								r = 1;
								goto error;
							}
							if (scancode >= tabsize[tab])
								tabsize[tab] = scancode + 1;
							if (keytab[tab][scancode] != 0)
							{
								fprintf(stderr, "%s:%ld: warning: duplicate scancode $%02x: %s\n", filename, lineno, (int)scancode, line);
							}
							keytab[tab][scancode] = num;
							scancode = 0;
						}
					} else
					{
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

	/*
	 * the altgr table is missing from a lot of sources
	 */
	if (is_mint && tabsize[TAB_ALTGR] == 0)
	{
		keytab[TAB_ALTGR][0] = 0;
		tabsize[TAB_ALTGR] = 1;
	}

	for (tab = 0; tab < N_KEYTBL; tab++)
	{
		if (tab <= TAB_CAPS || is_mint == 0)
		{
			if (tabsize[tab] == 0)
				fprintf(stderr, "%s: warning: missing table for %s\n", filename, labels[tab]);
			else if (tab <= TAB_CAPS && tabsize[tab] != MAX_SCANCODE)
				fprintf(stderr, "%s: warning: incomplete table for %s\n", filename, labels[tab]);
		}
	}

	deadkeys_format = FORMAT_NONE;
	if (tabsize[TAB_DEADKEYS] > 0)
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
			fprintf(stderr, "%s: warning: missing terminating zero in dead keys\n", filename);
			deadkeys[tabsize[TAB_DEADKEYS]] = 0;
			tabsize[TAB_DEADKEYS]++;
		}
	}
	
	if (output_format == FORMAT_NONE)
	{
		output_format = is_mint ? FORMAT_MINT : FORMAT_MAGIC;
	}
	
	if (outname == NULL)
	{
		const char *ext = output_format == FORMAT_C_SOURCE ? ".h" : output_format == FORMAT_MINT ? ".tbl" : ".sys";

		flen = strlen(filename);
		outname = malloc(flen + 6);
		if (!outname)
		{
			fprintf(stderr, "Out of RAM\n");
			return 1;
		}
		strcpy(outname, filename);
		o = strrchr(outname, '.');
		if (o == NULL)
			strcat(outname, ext);
		else
			strcpy(o, ext);

		printf("%s: output to %s\n", filename, outname);
	}

	out = fopen(outname, "wb");
	if (!out)
	{
		fprintf(stderr, "%s: %s\n", outname, strerror(errno));
		return 4;
	}

	r = 0;

	switch (output_format)
	{
	case FORMAT_C_SOURCE:
		write_c_src(out, deadkeys_format);
		break;

	case FORMAT_MINT:
		if (is_mint < 2)
		{
			mint_magics[0] = 0x2772;
			mint_magics[1] = 0x0003; /* AKP code: has to be fixed later */
			is_mint = 2;
		}
		for (loop = 0; loop < is_mint; loop++)
		{
			num = mint_magics[loop];
			fputc((int)(num >> 8), out);
			fputc((int)(num & 0xff), out);
		}
		for (tab = 0; tab <= TAB_CAPS; tab++)
			if (fwrite(keytab[tab], MAX_SCANCODE, 1, out) != 1)
				;
		write_alttab(out, TAB_ALT);
		write_alttab(out, TAB_SHALT);
		write_alttab(out, TAB_CAPSALT);
		write_alttab(out, TAB_ALTGR);
		if (deadkeys_format != FORMAT_NONE)
		{
			if (deadkeys_format != FORMAT_MINT)
			{
				fprintf(stderr, "%s: unexpected format of dead keys\n", filename);
				/* TODO: convert the format */
				r = 1;
			}
		}
		break;

	case FORMAT_MAGIC:
		if (tabsize[TAB_DEADKEYS] == 0)
			tabsize[TAB_DEADKEYS] = 2;
		for (tab = 0; tab < N_KEYTBL; tab++)
			if (fwrite(keytab[tab], MAX_SCANCODE, 1, out) != 1)
				;
		if (deadkeys_format != FORMAT_NONE)
		{
			if (deadkeys_format != FORMAT_MAGIC)
			{
				fprintf(stderr, "%s: unexpected format of dead keys\n", filename);
				/* TODO: convert the format */
				r = 1;
			}
		}
		break;
	}
	/*
	 * deadkeys are interpreted differently, but are just written as is
	 */
	if (tabsize[TAB_DEADKEYS] != 0 && fwrite(deadkeys, tabsize[TAB_DEADKEYS], 1, out) != 1)
		;

error:
	fclose(fd);
	if (out)
		fclose(out);

	if (r && outname)
		unlink(outname);

	return r;
}
