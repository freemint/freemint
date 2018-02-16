/*
 * This file belongs to FreeMiNT.  It's not in the original MiNT 1.12
 * distribution.  See the file Changes.MH for a detailed log of changes.
 *
 *
 * Copyright 1999, 2000 Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
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
 * Author: Thomas Binder <gryf@hrzpub.tu-darmstadt.de>
 * Started: 1999-09-27
 *
 * please send suggestions or bug reports to me or
 * the MiNT mailing list
 *
 *
 * Routines to de/encrypt blocks read/written in block_IO.c. The actual
 * cryptographic code (Blowfish) is in blowfish.c.
 *
 * History:
 * 2000-05-29: - crypt_block() now gets the physical record number, adapted
 *               calls to it accordingly (Gryf)
 *             - rwabs_crypto() didn't properly skip the bootsector (forgot to
 *               increment the record number, which is used for the feedback in
 *               CBC mode) (Gryf)
 * 2000-02-02: - Added additional parameter to d_setkey, which selects the
 *               cipher to use. Currently, only a value of zero (Blowfish) is
 *               supported, other values are reserved for future expansions
 *               (Gryf)
 * 1999-10-13: - After disabling ciphering for a drive, the used key structure
 *               is overwritten (Gryf)
 *             - Disabling ciphering only works for drives where it is active
 *               (Gryf)
 * 1999-09-29: - Removed initial password prompts and adapted to new
 *               integration into the block IO layer (Frank)
 * 1999-09-19-
 * 1999-09-27: - Creation (Gryf)
 */

# include "crypt_IO.h"
# include "global.h"

# include "libkern/blowfish.h"
# include "libkern/libkern.h"
# include "libkern/md5.h"
# include "mint/proc.h"

# include "block_IO.h"
# include "dosdir.h"
# include "info.h"
# include "init.h"
# include "k_prot.h"
# include "kmemory.h"
# include "proc.h"


void
init_crypt_IO (void)
{
# ifdef CRYPTO_CODE
	boot_print (crypto_greet);
# endif
}

# ifdef CRYPTO_CODE

/*
 * cbc_encipher
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
cbc_encipher (BF_KEY *bfk, ulong *lr, ulong *feedback)
{
	lr[0] ^= feedback[0];
	lr[1] ^= feedback[1];
	Blowfish_encipher (bfk, lr, lr + 1);
	feedback[0] = lr[0];
	feedback[1] = lr[1];
}

/*
 * cbc_decipher
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
cbc_decipher (BF_KEY *bfk, ulong *lr, ulong *feedback)
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

# ifdef M68000
INLINE void _cdecl
byte_copy (char *to, char *from, short bytes)
{
	/* This should produce dbra loop
	 */

	do {
		*to++ = *from++;
	}
	while (--bytes >= 0);
}
# endif

/*
 * crypt_block
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
 * rec: The (physical!) starting record of buf on the drive
 * DI: The diskinfo structure of the drive
 */
static void
crypt_block (BF_KEY *bfk, void (*crypt)(BF_KEY *, ulong *, ulong *), char *buf, ulong size, ulong rec, DI *di)
{
	ulong feedback[2];
	ulong count;
	ulong block;
	ulong blocksize;

	/* Paranoia */
	assert (((size % 8) == 0));

	/* Initialize local variables, and the CBC feedback register */
	feedback[0] = 0;
	feedback[1] = block = rec;
	count = 0;
	blocksize = di->pssize;

# ifdef M68000	/* No, bez jaj... */
	if (((ulong) buf & 1L) && (mcpu < 20))
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
# endif	/* M68000 */
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

static long _cdecl
rwabs_crypto (DI *di, ushort rw, void *buf, ulong size, ulong rec)
{
	void *crypt_buf = buf;
	ulong crypt_size = size;
	ulong crypt_rec = rec << di->lshift;
	long r;

	/* Record 0 (the bootsector) must not be ciphered, as otherwise
	 * Getbpb() will fail/return nonsense, making it impossible to
	 * use a crypted drive not under control of XHDI (e.g. disks)
	 */
	if (rec == 0)
	{
		crypt_buf = (char *) buf + di->pssize;
		crypt_size -= di->pssize;
		crypt_rec++;
		if (crypt_size == 0)
			return (*di->crypt.rwabs)(di, rw, buf, size, rec);
	}

	/* Encipher the data before writing the buffer */
	if (rw & 1)
		crypt_block (di->crypt.key, cbc_encipher, crypt_buf, crypt_size, crypt_rec, di);

	/* Do the real read/write operation */
	r = (*di->crypt.rwabs)(di, rw, buf, size, rec);
	if (r == 0)
	{
		/* On success, always decipher the buffer, as we have
		 * crypted data in it, anyway
		 */
		crypt_block (di->crypt.key, cbc_decipher, crypt_buf, crypt_size, crypt_rec, di);
	}

	return r;
}

# endif

/*
 * d_setkey
 *
 * GEMDOS call to set the key/passphrase for a drive. As changing the key
 * directly influences data access, the drive is Dlock()ed during this process
 * to prevent data loss.
 *
 * Input:
 * dev: Drive (will be dereferenced, if it is an alias)
 * key: Pointer to the new passphrase; if this is an empty string, ciphering
 *      will be disabled for drv; it it is a NULL pointer, test current
 *      ciphering mode
 * cipher: Cipher type to use. Currently, only zero (Blowfish) is supported.
 *         Other values are reserved for future expansion.
 *
 * Returns:
 * (for key != NULL)
 *  E_OK: All right
 *  EDRIVE: Invalid value for dev
 *  EINVAL: Invalid value for cipher
 *  else: Any GEMDOS error code returned by Dlock()
 * (for key == NULL)
 * 0: Ciphering disabled on dev
 * else: Ciphering enabled on dev
 */
long _cdecl
sys_d_setkey (llong dev, char *key, int cipher)
{
# ifdef CRYPTO_CODE
	DI *di;
	long r;

	DEBUG (("Dsetkey(%li, %d)", (long) dev, cipher));

	if (!suser (get_curproc()->p_cred->ucr))
		return EPERM;

	switch (cipher)
	{
	    case 0:
		/* Blowfish */
		break;
	    default:
		return EINVAL;
	}

	if ((r = sys_d_lock (1, dev)) != E_OK)
		return r;

	di = bio.res_di (dev);
	if (!di)
	{
		(void) sys_d_lock (0, dev);
		return ENXIO;
	}

	/* return whether encryption is enabled or not */
	if (!key)
	{
		r = di->mode & BIO_ENCRYPTED;

		bio.free_di (di);
		(void) sys_d_lock (0, dev);

		return r;
	}

	if (*key)
	{
		struct MD5Context md5sum;
		uchar hash [16];

		MD5Init (&md5sum);
		MD5Update (&md5sum, (unsigned char *)key, strlen (key));
		MD5Final ((unsigned char *)hash, &md5sum);

		if (!(di->mode & BIO_ENCRYPTED))
		{
			di->crypt.key = kmalloc (sizeof (*(di->crypt.key)));
			if (!di->crypt.key)
			{
				bio.free_di (di);
				(void) sys_d_lock (0, dev);

				return ENOMEM;
			}

			di->rrwabs = &di->crypt.rwabs;

			di->crypt.rwabs = di->rwabs;
			di->rwabs = rwabs_crypto;

			di->mode |= BIO_ENCRYPTED;
		}

		Blowfish_initialize (di->crypt.key, (unsigned char *)hash, 16);
	}
	else
	{
		if (di->mode & BIO_ENCRYPTED)
		{
			di->mode &= ~BIO_ENCRYPTED;
			di->rwabs = di->crypt.rwabs;
			di->rrwabs = &di->rwabs;

			mint_bzero (di->crypt.key, sizeof(*(di->crypt.key)));
			kfree (di->crypt.key);
		}
	}

	bio.free_di (di);
	return sys_d_lock (0, dev);
# else
	return ENOSYS;
# endif
}
