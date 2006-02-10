/*
 * $Id$
 * 
 * HypView - (c)      - 2006 Philipp Donze
 *               2006 -      Philipp Donze & Odd Skancke
 *
 * A replacement hypertext viewer
 *
 * This file is part of HypView.
 *
 * HypView is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HypView is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HypView; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __GNUC__
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <mintbind.h>
#include <fcntl.h>
#include <mt_gem.h>
#include <stdio.h>
#include <macros.h>

#include "include/types.h"
#include "include/scancode.h"
#include "diallib.h"
#include "hyp.h"
#else
#include <stdio.h>
#include <string.h>
#include <tos.h>
#include <aes.h>
#include "diallib.h"
#include SPEC_DEFINITION_FILE
#endif

void FileError(char *path,char *str)
{
char *filename;
char ZStr[255];

	filename=strrchr(path,dir_separator);
	if(!filename++)
		filename=path;

	sprintf ( ZStr, "[3][ File <%s>: | %s ][ OK ]", filename, str );
	form_alert ( 1, ZStr );
}

void FileErrorNr(char *path,long ret)
{
char *filename;
	filename=strrchr(path,dir_separator);
	if(!filename++)
		filename = path;
	if((ret == ENOENT/*EFILNF*/) || (ret == ENOTDIR/*EPTHNF*/))
		Debug("ERROR: File <%s>: file not found",filename);
	else
		Debug("ERROR: File <%s>: error %ld",filename,ret);
}
