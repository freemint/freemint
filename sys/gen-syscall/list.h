/*
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
 * begin:	2000-01-01
 * last change:	2000-03-07
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
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

# ifndef _list_h
# define _list_h


# define STRMAX		255

# define INITIAL_DOS	100
# define INITIAL_BIOS	100
# define INITIAL_XBIOS	100


typedef struct systab	SYSTAB;
typedef struct syscall	SYSCALL;
typedef struct list	LIST;


extern SYSTAB *DOS;
extern SYSTAB *BIOS;
extern SYSTAB *XBIOS;


struct systab
{
	SYSCALL	**table;
	int	size;
	int	max;
	int	maxused;
};

struct syscall
{
	char	class [STRMAX];
	char	name [STRMAX];
	LIST	*args;
};

int	resize_tab	(SYSTAB *tab, int newsize);
int	add_tab		(SYSTAB *tab, int nr, const char *class, const char *name, LIST *p);


struct list
{
	LIST	*next;
	
	int	type;
# define TYPE_VOID	0
# define TYPE_INT	1
# define TYPE_CHAR	2
# define TYPE_SHORT	3
# define TYPE_LONG	4
# define TYPE_UNSIGNED	5
# define TYPE_UCHAR	6
# define TYPE_USHORT	7
# define TYPE_ULONG	8
# define TYPE_IDENT	9
	int	flags;
# define FLAG_CONST	0x01
# define FLAG_STRUCT	0x02
# define FLAG_UNION	0x04
# define FLAG_POINTER	0x08
# define FLAG_ARRAY	0x10
	int	ar_size;
	
	char	types [STRMAX];
	char	name [STRMAX];
};

void	add_list	(LIST **head, LIST *t);
LIST *	make_list	(int type, const char *s);


# endif /* _list_h */
