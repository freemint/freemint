/*
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
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
 * Started:      2000-05-02
 * 
 * Changes:
 * 
 * 2000-05-02:
 * 
 * - inital version
 * 
 */

# ifndef _crypt_h
# define _crypt_h

# include <sys/types.h>


void * make_key (char *passphrase, int cipher);
int encrypt_block (void *key, void *buf, int32_t bufsize, int32_t recno, int32_t p_secsize);
int decrypt_block (void *key, void *buf, int32_t bufsize, int32_t recno, int32_t p_secsize);


# endif /* _crypt_h */
