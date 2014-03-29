/*
 * $Id$
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 *
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2000-10-30
 *
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 *
 */

# ifndef _mint_filedesc_h
# define _mint_filedesc_h

# include "ktypes.h"
# include "file.h"


struct file;

# define NDFILE		32
# define NDEXTENT	50

struct filedesc
{
	struct file	**ofiles;	/* file structures for open files */
	uchar		*ofileflags;	/* per-process open file flags */
	short		nfiles;		/* number of open files allocated */
	short		pad2;
// XXX	short		lastfile;	/* high-water mark of ofiles */
// XXX	short		freefile;	/* approx. next free file */
	long		links;		/* reference count */


	DIR		*searches;	/* open directory searches	*/

	/* TOS emulation */

	DTABUF	*dta;			/* current DTA			*/
# define NUM_SEARCH	10		/* max. number of searches	*/
	DTABUF *srchdta[NUM_SEARCH];	/* for Fsfirst/next		*/
	DIR	srchdir[NUM_SEARCH];	/* for Fsfirst/next		*/
	long	srchtim[NUM_SEARCH];	/* for Fsfirst/next		*/

	/* XXX total crap
	 * there are something like this "ofiles[-3]" over the src
	 * before we dynamically alloc the ofiles we have
	 * to fix all the places
	 */
	short		pad1;
	short		bconmap;	/* Bconmap mapping */
	struct file	*midiout;	/* MIDI output */
	struct file	*midiin;	/* MIDI input */
	struct file	*prn;		/* printer */
	struct file	*aux;		/* auxiliary tty */
	struct file	*control;	/* control tty */

	struct file	*dfiles [NDFILE];
	uchar		dfileflags [NDFILE];
};

struct cwd
{
	long		links;		/* reference count */
	ushort		cmask;		/* mask for file creation */
	ushort		pad;

	fcookie		currdir;	/* current directory */
	fcookie		rootdir;	/* root directory */
	char		*root_dir;	/* XXX chroot emulation */
#if STORE_FILENAMES
	char     fullpath[PATH_MAX+1];
#endif
	/* DOS emulation
	 */
	ushort		curdrv;		/* current drive */
	ushort		pad2;
	fcookie 	root[NDRIVES];	/* root directories */
	fcookie		curdir[NDRIVES];/* current directory */
};


# endif /* _mint_filedesc_h */
