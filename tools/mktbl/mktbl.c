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

# include <ctype.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

/* Own getdelim(). The `n' buffer size must definitely be bigger than 0!
 */
static int
mktbl_getdelim(char **lineptr, size_t *n, int terminator, FILE *stream)
{
	int ch;
	char *buf = *lineptr;
	size_t len = 0;

	while ((ch = fgetc(stream)) != -1)
	{
		if ((len + 1) >= *n)
		{
			buf = realloc(buf, len + 256L);
			*n += 256L;
			*lineptr = buf;
		}
		buf[len++] = (char)ch;

		if (ch == terminator)
			break;
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
	if (ch == -1)
	{
		if (len == 0)	/* Nothing read before EOF in this line */
			return -1;
		/* Pretend success otherwise */
	}

	return 0;
}

int
main(int argc, char **argv)
{
	char *line, *ln, *outname, *o;
	long r, n = 1, num, flen;
	short w, warn = 0;
	size_t buf;
	FILE *fd, *out;

	if (argc <= 2)
	{
		if (argc < 2 || (argc >= 2 && (strcmp(argv[1], "--help") == 0)))
		{
			printf("Usage: %s src-file [tbl-file]\n", argv[0]);

			return 1;
		}

		flen = strlen(argv[1]);
		outname = malloc(flen + 1);
		if (!outname)
		{
			fprintf(stderr, "Out of RAM\n");
			return 10;
		}
		strcpy(outname, argv[1]);
		o = strrchr(outname, '.');
		if (o == NULL)
			strcat(outname, ".tbl");
		else
			strcpy(o, ".tbl");

		printf("%s: output to %s\n", argv[1], outname);
	}
	else
		outname = argv[2];

	buf = 1024;
	line = malloc(buf);	/* should be plenty */
	if (!line)
		return 2;

	fd = fopen(argv[1], "r");
	if (!fd)
		return 3;

	out = fopen(outname, "wb");
	if (!fd)
		return 4;

	do
	{
		r = mktbl_getdelim(&line, &buf, '\n', fd);
		if (r >= 0 && line[0] != ';' && (ln = strstr(line, "dc.")))
		{
			w = strncmp(ln, "dc.b", 4);
			ln += 4;

			while (*ln && *ln != '$' && *ln != '\'')
				ln++;

			while (*ln)
			{
				if (ln[0] == '\'')
				{
					if (ln[2] != '\'')
					{
						fprintf(stderr, "%s: error, unmatched quotes in line %ld: %s\n", \
								argv[1], n, line);
						r = 5;
						goto error;
					}
					if (w)
						fputc(0, out);
					fputc(ln[1], out);
					ln += 3;
				}
				else if (ln[0] == '$')
				{
					if (!isxdigit(ln[1]))
					{
						fprintf(stderr, "%s: error, '%c' is not a hex number in line %ld: %s\n", \
								argv[1], ln[1], n, line);
						r = 6;
						goto error;
					}

					ln++;
					num = strtoul(ln, &ln, 16);
					if ((w && num > 65535) || (!w && num > 255))
						fprintf(stderr, "%s: warning, number %ld in line %ld is too big\n", \
							argv[1], num, n);
					if (w)
						fputc(num >> 8, out);
					fputc(num & 0xff, out);
				}
				else if (ln[0] == '0' && ln[1] == 'x')
				{
					if (!isxdigit(ln[3]))
					{
						fprintf(stderr, "%s: error, '%c' is not a hex number in line %ld: %s\n", \
								argv[1], ln[3], n, line);
						r = 7;
						goto error;
					}

					ln += 2;
					num = strtoul(ln, &ln, 16);
					if ((w && num > 65535) || (!w && num > 255))
						fprintf(stderr, "%s: warning, number %ld in line %ld is too big\n", \
							argv[1], num, n);
					if (w)
						fputc(num >> 8, out);
					fputc(num & 0xff, out);
				}
				else if (isdigit(ln[0]))
				{
					num = strtoul(ln, &ln, 10);
					if ((w && num > 65535) || (!w && num > 255))
						fprintf(stderr, "%s: warning, number %ld in line %ld is too big\n", \
							argv[1], num, n);
					if (w)
						fputc(num >> 8, out);
					fputc(num & 0xff, out);
				}
				else if (ln[0] == ';' || ln[0] == ',' || ln[0] == '\n')
					ln++;
				else if (ln[0] == '\r' && ln[1] == '\n')
				{
					if (!warn)
					{
						fprintf(stderr, "%s: warning, CR/LF line endings\r\n", argv[1]);
						warn = 1;
					}
					ln += 2;
				}
				else
				{
					fprintf(stderr, "%s: error, unexpected '%c' in line %ld: %s\n", \
						argv[1], ln[0], n, line);
					r = 8;
					goto error;
				}
			}
		}
		n++;
	}
	while (r != -1);

	r = 0;

error:
	fclose(fd);
	fclose(out);

	if (r)
		unlink(outname);

	return r;
}

/* EOF */
