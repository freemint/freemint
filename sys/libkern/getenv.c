/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2003 Konrad Kokoszkiewicz <draco@atari.org>
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
 * begin:	2003-02-16
 * last change:	2003-02-16
 * 
 * Author:	Konrad Kokoszkiewicz <draco@atari.org>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * changes since last version:
 * 
 * known bugs:
 * 
 * todo:
 * 
 * optimizations:
 * 
 */

# include "mint/basepage.h"
# include "libkern.h"

char * _cdecl
_mint_getenv(BASEPAGE *bp, const char *var)
{
	char *env_str = bp->p_env;
	long len = strlen(var);

	if (env_str && len)
	{
		while (*env_str)
		{
			if ((strncmp(env_str, var, len) == 0) && (env_str[len] == '='))
				return env_str + len + 1;
			while (*env_str)
				env_str++;
			env_str++;
		}
	}

	return NULL;
}

/* EOF */
