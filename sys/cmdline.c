/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Guido Flohr <guido@freemint.de>
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
 * Author: Guido Flohr <guido@freemint.de>
 * Started: 1999-11-21
 *
 * please send suggestions, patches or bug reports to me or
 * the MiNT mailing list
 *
 *
 * Build argument vector for all processes.
 *
 */

# include "cmdline.h"

# include "libkern/libkern.h"

# include "proc.h"
# include "kmemory.h"


/* Construct a usable argument vector and argument buffer for the
 * newborn process P.  Returns zero on success or a negative error
 * number on failure.  The only possible error condition is ENOMEM.
 *
 * The function knows about various calling conventions, notably
 * the extended ARGV technique with a possible list of NULL arguments
 * (indicated by the value "NULL:<LIST>" for the ARGV envariable)
 * and also about really obsoleted argument passing via the p_cmdlin
 * member of the basepage structure.  For such obsolete stuff it
 * even checks for the pseudo-quoting done by some buggy desktops.
 *
 * It puts the result into one single buffer inside the process
 * structure.  The layout of this buffer may look strange at first
 * glance but it is optimized for two purposes:  First, the sys
 * pseudo-filesystem can use the information to serve read requests
 * from the special file /sys/<PID>/cmdline, second, for future
 * enhancements of the MiNT kernel, the buffer can be used to
 * pass a standard argument vector to the new process (no matter
 * how this is done.
 *
 * The layout is as follows:
 *
 * 1. The first four bytes store the length of the entire buffer,
 *    including the double null-termination byte at the end.
 * 2. The number of arguments ARGC (as an unsigned long).
 * 3. A NULL-terminated array of offsets into the buffer counted
 *    from the address of this array (resp. its first member)
 *    itself.
 * 4. A null-terminated buffer with the arguments itself.  An
 *    extra null-byte is appended to the end of the buffer to
 *    denote the end of arguments.  The buffer is therefore always
 *    terminated by a double null-byte but you may also encounter
 *    two or more subsequent null-bytes in the middle of the buffer
 *    caused by empty arguments.  Use ARGC and the offsets
 *    in the ARGV vector to find the end of arguments.	Note that
 *    the special files /sys/<PID>/cmdline will NOT contains
 *    the last two null-bytes (because the end of the buffer is
 *    always terminated by EOF.
 *
 * If this argument vector and the argument buffer should be passed
 * to the process in some future MiNT version, you should proceed
 * as follows:	First negotiate the required buffer size with the
 * child process.  The required size can be found at offset 0 (i. e.
 * it denote the amount of memory the process should allocate for
 * the argument vector/buffer).  ARGC can be passed as an lvalue
 * and the rest of the kernel buffer can then be copied into the
 * user buffer.  The user program should then add the address of
 * the first argument to all non-NULL members of the ARGV array
 * and can then pass this to the main() function (note that one
 * malloc() is sufficient for that).
 *
 * The sys pseudo-filesystem has to do slightly more calculations
 * but this is still tolerable.
 */

# define SINGLE_QUOTE	'\''

int
make_real_cmdline(struct proc *p)
{
	char *to;		/* General purpose cursor over arrays */
	char *from;
	char *env;
	char *raw_cmdline = NULL;
	char *end_parse = NULL;
	int silly_quotes = 0;
	int translate_space = 0;
	int copy_fname = 0;
	char *null_list = NULL;
	long max_length = 0;
	long real_length = 0;
	long buffer_length;
	unsigned long argc = 0;
	char *cmdline;
	int quoted = 0;
	char **raw_argv = NULL;
	int opt;

	assert(p->p_mem);
	env = p->p_mem->base->p_env;

	if (p->real_cmdline != NULL)
	{
		kfree (p->real_cmdline);
		p->real_cmdline = NULL;
	}

	TRACE (("make_real_cmdline: fname: %s", p->fname));
	if (env != NULL)
	{
		char* argv = NULL;
		from = env;

		for (;;)
		{
# ifndef M68000
			if (*(ushort *)from == 0)
				break;
# else
			if (from[0] == '\0' && from[1] == '\0')
				break;
# endif
			if (from[0] == 'A' && from[1] == 'R' && from[2] == 'G' \
				&& from[3] == 'V' && from[4] == '=' \
				&& (from == env || from[-1] == '\0'))
			{
				argv = from;
				break;
			}
			from++;
		}

		if (argv != NULL)
		{
			TRACE (("make_real_cmdline: ARGV found"));
			from += (sizeof("ARGV=") - 1);

			/* Check for null list */
			if (from[0] == 'N' && from[1] == 'U' \
				&& from[2] == 'L' && from[3] == 'L' && from[4] == ':')
			{
				from += (sizeof("NULL:") - 1);
				null_list = from;
			}
			while (*from++)
				;

			raw_cmdline = end_parse = from;

			/* Determine the maximum number of arguments
			 * and the length of the argument list.
			 */
			for (;;)
			{
				   if (*end_parse == '\0')
				   {
					    argc++;
					if (end_parse[1] == '\0')
						     break;
				   }

				end_parse++;
				max_length++;
			}
		}
	}

	if (raw_cmdline == NULL)
	{
		/* Called by a broken program, no argument vector available.
		 * Try to make one out of the command line.  In round one
		 * we try to have a good guess at the maximum size of the
		 * command line we need.
		 */
		int i;

		TRACE (("make_real_cmdline: fallback"));

		max_length = strlen (p->cmdlin + 1);
		translate_space = 1;
		copy_fname = 1;

		for (raw_cmdline = p->cmdlin + 1;
		     *raw_cmdline == ' ' && max_length > 0;
		     raw_cmdline++, max_length--)
			;

		/* This ain't no joke.  If the first character of the
		 * command line is a single quote whitespace between
		 * single quotes has to be interpreted verbatim.  The
		 * missing possibility to quote a single quote shows
		 * the expertise of the designer.
		 */
		if (*raw_cmdline == SINGLE_QUOTE)
			silly_quotes = 1;

		/* Try to get an idea of the maximum number of arguments
		 * we have to expect.
		 */
		end_parse = raw_cmdline;
		for (i = 0; i < max_length && raw_cmdline[i]; i++, end_parse++)
			if (raw_cmdline[i] == ' ')
				  argc++;

		/* Don't forget that we add argv[0] */
		argc++;
	}

	/* Now we have a vague idea of how much memory to allocate.  In
	 * fact our idea is good enough to just allocate it and forget
	 * about the little rest for now.  We will copy that over later.
	 */
	if (copy_fname)
		max_length += strlen (p->fname) + 1;

	max_length += 2;     /* Two null-bytes */

	cmdline = kmalloc (max_length);
	if (cmdline == NULL)
	{
		DEBUG (("make_real_cmdline: fatal: out of memory"));
		return ENOMEM;
	}

	/* Allocate space for the argument vector */
	raw_argv = kmalloc ((argc + 1) * sizeof (char *));
	if (raw_argv == NULL)
	{
		kfree (cmdline);
		DEBUG (("make_real_cmdline: fatal: out of memory"));
		return ENOMEM;
	}

	/* Reset argc because we want the real value now */
	argc = 0;

	/* Now fill the argument vector */
	to = cmdline;

	if (copy_fname)
	{
		from = p->fname;
		do {
			*to++ = *from;
			real_length++;
		}
		while (*from++);

		argc++;
		raw_argv[0] = cmdline;
	}

	/* Only evaluate one single byte at once so that our
	 * break condition won't lose!
	 */
	for (from = raw_cmdline; from < end_parse; from++)
	{
		int is_null = 0;

		/* This flag indicates whether the currently
		 * read byte should be treated as a null-byte
		 * terminating an argument.  We cannot
		 * write a null-byte to that location because
		 * our source has to be considered
		 * const (it either belongs to the basepage
		 * or the environment).
		 */

		if (*from == SINGLE_QUOTE)
		{
			if (quoted)
			{
				quoted = 0;
				continue;
			}
			else if (silly_quotes)
			{
				quoted = 1;
				continue;
			}
		}

		if (translate_space && !quoted && *from == ' ')
		{
			if (from + 1 == end_parse)
			{
				argc++;
				continue;
			}

			if (from[1] == ' ')
				continue;

			is_null = 1;
		}

		if (!translate_space && *from == '\0')
		{
			/* FIXME: Check null_list */
			is_null = 1;
		}

		if (is_null)
		{
			/* Attention: If we read an old-style command
			 * line we had a spce here.
			 */
			argc++;
			*to++ = '\0';
			real_length++;
			continue;
		}

		if (from == raw_cmdline || to[-1] == '\0')
			raw_argv[argc] = to;

		*to++ = *from;
		real_length++;
	}

	if (!translate_space && argc != 0)
		 argc++;  /* We had missed this one! */

	*to++ = '\0';
	*to++ = '\0';
	real_length += 2;

	/* Evaluate a possible list of empty parameters.  I'm not
	 * completely sure if I have understood this.	As far as I
	 * can tell from crtinit.c of the MiNTLib something like
	 * "ARGV=NULL:2,4,5" would tell us to set argv[2], argv[4],
	 * and argv[5] to NULL.  This is potentially unsafe because
	 * most people will simply do a "for (i = 1; i < argc; i++)"
	 * and not "for (i = 1; i < argc && argv[i]; i++)" when
	 * parsing their arguments.
	 *
	 * The other problem: How do we tell the casual onlooker on
	 * "/sys/[PID]/cmdline" that an argument is empty?  The
	 * "natural" solution is to display an empty string
	 * at the risk of being misunderstood as end of options.
	 *
	 * Once decided that this is the preferable solution we
	 * face the next problem that our command line that we
	 * have built some ticks ago is now wrong.  We have to
	 * zero-length the arguments and then move the rest
	 * of our buffer to the front.  Sigh!
	 */
	if (null_list)
	{
		from = null_list;

		while (*from)
		{
			int valid = (*from >= '0' && *from <= '9') ? 1 : 0;
			ulong i;
			opt = 0;

			while (*from >= '0' && *from <= '9')
			{
				opt = (opt << 3) + (opt << 1);
				opt += (*from - '0');
				from++;
			}

			if (valid && *from != ',' && *from != '\0')
				   valid = 0;

			if (opt >= argc)
				   valid = 0;

			if (raw_argv[opt] == NULL)
				   valid = 0;

			if (valid)
			{
				char* from1 = raw_argv[opt + 1];
				char* to1 = raw_argv[opt];
				ulong deleted = 0;

				TRACE (("make_real_cmdline: zero out argv[%u] (%s)", opt, raw_argv[opt]));
				if (from1 == NULL)
				{
					from1 = raw_argv[opt];
					while (*from1)
						from1++;
					from1 += 2;
				}

				*to1++ = '\0';
				*to1++ = '\0';

				deleted = from1 - to1;

				for (; from1 < raw_argv[0] + real_length; )
					    *to1++ = *from1++;
				real_length -= deleted;

				for (i = opt + 1; i < argc; i++)
					    raw_argv[i] -= deleted;
			}

			while (*from != ',' && *from)
				   from++;

			if (*from == ',')
				   from++;
		}
	}

	/* Now correct the argument vector by the size of the vector
	 * itself.  We need one extra slot for the terminating NULL
	 * pointer.
	 */
	for (opt = 0; opt < argc; opt++)
		 raw_argv[opt] = (char*) (raw_argv[opt] - cmdline
					 + ((argc + 1) * sizeof raw_argv[0]));
	raw_argv[argc] = NULL;

	/* Allocate the final space */
	p->real_cmdline = kmalloc (sizeof real_length + sizeof argc
				   + real_length
				   + (argc + 1) * sizeof raw_argv[0]);

	if (p->real_cmdline == NULL)
	{
		kfree (raw_argv);
		kfree (cmdline);

		DEBUG (("make_real_cmdline: fatal: out of memory"));
		return ENOMEM;
	}

	buffer_length = real_length + sizeof argc + (argc + 1) * sizeof raw_argv[0];
	to = p->real_cmdline;
	memcpy (to, &buffer_length, sizeof buffer_length);
	to += sizeof buffer_length;

	memcpy (to, &argc, sizeof argc);
	to += sizeof argc;
	memcpy (to, raw_argv, (argc + 1) * sizeof raw_argv[0]);
	to += (argc + 1) * sizeof raw_argv[0];
	memcpy (to, cmdline, real_length);

	/* Cleanup */
	kfree (raw_argv);
	kfree (cmdline);

	return 0;
}
