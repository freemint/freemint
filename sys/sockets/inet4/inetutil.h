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
 * begin:	2000-04-18
 * last change:	2000-04-18
 * 
 * Author:	Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# ifndef _inetutil_h
# define _inetutil_h

# include "global.h"

# include "inet.h"


void			in_proto_register (short, struct in_proto *);
struct in_proto *	in_proto_lookup (short);

struct in_data	*	in_data_create (void);
long			in_data_destroy (struct in_data *, short);
void			in_data_flush (struct in_data *);

short			in_data_find (short, struct in_data *);
void			in_data_put (struct in_data *);
void			in_data_remove (struct in_data *);
struct in_data *	in_data_lookup (struct in_data *,
				ulong, ushort,
				ulong, ushort);

struct in_data *	in_data_lookup_next (struct in_data *,
				ulong, ushort,
				ulong, ushort, short);

short			chksum (void *, short);
void			sa_copy (struct sockaddr *, struct sockaddr *);


# endif /* _inetutil_h */
