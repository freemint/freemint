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

# include "blowfish.h"
# include "crypt.h"
# include "md5.h"

# include <assert.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>


/*
 * blow_cbc_encipher
 *
 * Encrypts a 64-bit-block in CBC mode (CipherBlock Chaining), where the
 * current plaintext is XORed with the last ciphertext before being encrypted.
 *
 * Input:
 * bfk: Pointer to the Blowfish-Key to use
 * lr: Pointer to the 64-bit-block (plaintext)
 * feedback: Pointer to the 64-bit-feedback, which will be updated for the next
 *           call
 */
static void
blow_cbc_encipher (BF_KEY *bfk, ulong *lr, ulong *feedback)
{
	lr[0] ^= feedback[0];
	lr[1] ^= feedback[1];
	Blowfish_encipher (bfk, lr, lr + 1);
	feedback[0] = lr[0];
	feedback[1] = lr[1];
}

/*
 * blow_cbc_decipher
 *
 * Decrypts a 64-bit-block in CBC mode (CipherBlock Chaining), where the
 * current plaintext is XORed with the last ciphertext after being decrypted.
 *
 * Input:
 * bfk: Pointer to the Blowfish-Key to use
 * lr: Pointer to the 64-bit-block (ciphertext)
 * feedback: Pointer to the 64-bit-feedback, which will be updated for the next
 *           call
 */
static void
blow_cbc_decipher (BF_KEY *bfk, ulong *lr, ulong *feedback)
{
	ulong	fb[2];

	fb[0] = feedback[0];
	fb[1] = feedback[1];
	feedback[0] = lr[0];
	feedback[1] = lr[1];
	Blowfish_decipher (bfk, lr, lr + 1);
	lr[0] ^= fb[0];
	lr[1] ^= fb[1];
}

static inline void
byte_copy (char *to, char *from, short bytes)
{
	/* This should produce dbra loop
	 */
	
	do {
		*to++ = *from++;
	}
	while (--bytes >= 0);
}

/*
 * blow_doblock
 * 
 * This is the function that is called from crypt_rwabs() to en- or decrypt one
 * or more disk blocks. Blowfish is used in CBC mode (CipherBlock Chaining),
 * with the physical record number as the initial value for the feedback
 * register. Each physical block is handled individually.
 *
 * Input:
 * bfk: Pointer to the Blowfish-Key to use
 * decipher: If non-zero, the block will be decrypted, encrypted otherwise
 * buf: Pointer to the buffer which is to be en/decrypted
 * size: How many bytes are in buf?
 * rec: The starting record of buf on the drive
 * p_secsize: physical sectorsize
 */
static void
blow_doblock (BF_KEY *bfk,
		void (*crypt)(BF_KEY *, ulong *, ulong *),
		char *buf, ulong size, u_int32_t rec, u_int32_t p_secsize)
{
	ulong feedback[2];
	ulong count;
	ulong block;
	ulong blocksize;
	
	/* Paranoia */
	assert (((size % p_secsize) == 0));
	
	/* Initialize local variables, and the CBC feedback register */
	feedback[0] = 0;
	feedback[1] = block = rec;
	count = 0;
	blocksize = p_secsize;
	
# ifdef __mc68000__
	if (((ulong) buf & 1L) /* && (mcpu < 20) */)
	{
		/* Unfortunately, the block isn't word aligned ...
		 */
		register ulong i;
		
		for (i = 0; i < size; i += 8)
		{
			ulong lr[2];
			
			byte_copy ((char *) lr, (char *) buf, 7);
			(*crypt)(bfk, lr, feedback);
			byte_copy ((char *) buf, (char *) lr, 7);
			
			buf += 8;
			count += 8;
			
			/* After each physical diskblock, re-initialize the
			 * feedback register
			 */
			if (count == blocksize)
			{
				count = feedback[0] = 0;
				feedback[1] = ++block;
			}
		}
	}
	else
# endif
	{
		/* The block is word aligned, or the CPU can handle misaligned
		 * long accesses
		 */
		register ulong *left = (ulong *) buf;
		register ulong i;
		
		for (i = 0; i < size; i += 8)
		{
			(*crypt)(bfk, left, feedback);
			left += 2;
			count += 8;
			
			/* After each physical diskblock, re-initialize the
			 * feedback register
			 */
			if (count == blocksize)
			{
				count = feedback[0] = 0;
				feedback[1] = ++block;
			}
		}
	}
}

static void
blow_keyinit (BF_KEY *key, char *passphrase)
{
	struct MD5Context md5sum;
	char hash [16];
	
	MD5Init (&md5sum);
	MD5Update (&md5sum, passphrase, strlen (passphrase));
	MD5Final (hash, &md5sum);
	
	InitializeBlowfish (key, hash, 16);
}


typedef struct key KEY;
struct key
{
	int	cipher;
	BF_KEY	blowfish;
};

void *
make_key (char *passphrase, int cipher)
{
	KEY *key = malloc (sizeof (*key));
	
	if (key)
	{
		key->cipher = cipher;
		
		switch (cipher)
		{
			/* blowfish */
			case 0:
			{
				blow_keyinit (&(key->blowfish), passphrase);
				break;
			}
			
			/* unknown */
			default:
			{
				free (key);
				key = NULL;
				
				break;
			}
		}
	}
	
	return key;
}

int
encrypt_block (void *_key, void *_buf, int32_t bufsize, int32_t recno, int32_t p_secsize)
{
	KEY *key = _key;
	char *buf = _buf;
	
	assert (bufsize > p_secsize);
	
	if (recno == 0)
	{
		/* skip first sector */
		recno++;
		buf += p_secsize;
		bufsize -= p_secsize;
	}
	
	if (bufsize)
	{
		if (key->cipher)
			return -1;
		
		blow_doblock (&(key->blowfish), blow_cbc_encipher,
				buf, bufsize, recno, p_secsize);
	}
	
	return 0;
}

int
decrypt_block (void *_key, void *_buf, int32_t bufsize, int32_t recno, int32_t p_secsize)
{
	KEY *key = _key;
	char *buf = _buf;
	
	assert (bufsize > p_secsize);
	
	if (recno == 0)
	{
		/* skip first sector */
		recno++;
		buf += p_secsize;
		bufsize -= p_secsize;
	}
	
	if (bufsize)
	{
		if (key->cipher)
			return -1;
		
		blow_doblock (&(key->blowfish), blow_cbc_decipher,
				buf, bufsize, recno, p_secsize);
	}
	
	return 0;
}
