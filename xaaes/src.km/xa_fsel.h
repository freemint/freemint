/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *                                        2004 F.Naumann & O.Skancke
 *
 * A multitasking AES replacement for FreeMiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _xa_fsel_h
#define _xa_fsel_h

#include "global.h"
#include "xa_types.h"

#define DRV_MAX 32
#define NAME_MAX 128

struct fsel_data;
typedef void fsel_handler(int lock, struct fsel_data *fs, const char *path, const char *file);
struct fsel_data
{
	struct xa_window *wind;
	XA_TREE *form;
	XA_TREE *menu;
	void *data;
	struct xa_client *owner;
	short sort;
	struct scroll_entry *selected_dir;
	struct scroll_entry *selected_file;
	fsel_handler	*selected;
	fsel_handler	*canceled;
	char root[PATH_MAX * 2]; //Path root;
	char path[PATH_MAX * 2]; //Path path;
	char fslash[2];
	char fs_pattern[NAME_MAX * 2];
	char fs_origpattern[NAME_MAX];
	char filter[NAME_MAX * 2];
	char file  [NAME_MAX + 2];		/* Is the tedindo->te_ptext of FS_FILE */
	char ofile [NAME_MAX + 2];
	long fcase,trunc;
	int drives;
	int curr_longest;
	int ok;
#define FS_INIT           1
#define FS_CREATE_FOLDER  2
#define FS_RENAME_FILE    4
	short rtflags;	/* runtime-flags */
	short fntinc;	/* increment/decrement font-size for file-selector */
	short point;	/* font-size */
	bool tfile;
	bool kbdnav;
	bool treeview;
	bool rtbuild;
};

struct fs_data{
	short SortDir;
	short fs_sort;
	bool treeview;
	bool rtbuild;
	short fs_x, fs_y, fs_width, fs_height;
	short fs_point;

	char fs_paths[DRV_MAX][NAME_MAX+2];
	char fs_patterns[FS_NPATTERNS][FS_PATLEN];

	/* runtime only */
	short fs_file_w; /* max. width for edit-field */
	short fs_num;
	int prover, provermin;
/* fsel opened by launcher */
	struct xa_client *aes_has_fsel;
	void *fs;
};

extern struct fs_data fs_data;

void open_fileselector(int lock, struct xa_client *client, struct fsel_data *fs,
		       char *path, const char *file, const char *title,
		       fsel_handler *s, fsel_handler *c, void *data);

void fs_save(struct fsel_data *fs);
void close_fileselector(int lock, struct fsel_data *fs);

void init_fsel(void);

AES_function
	XA_fsel_input,
	XA_fsel_exinput;

/* exported helper functions */
bool
sortbyname(struct scroll_info *list, struct scroll_entry *e1, struct scroll_entry *e2);	/* xa_fsel.c */

#endif /* _xa_fsel_h */
