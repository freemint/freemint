/*
 * The Host OS filesystem access driver - device IO.
 *
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 *
 * Copyright (c) 2002-2006 Standa of ARAnyM dev team.
 * Copyright 1998, 1999, 2001 by Markus Kohm <Markus.Kohm@gmx.de>.
 * Modified by Chris Felsch <C.Felsch@gmx.de>.
 *
 * Originally taken from the STonX CVS repository.
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
 */


#include "global.h"
#include "hostfs_xfs.h"
#include "hostfs_dev.h"
#include "hostfs_nfapi.h"
#if __KERNEL__ == 1
#include "filesys.h"
#include "proc.h"
#include "kmemory.h"
#define p_getpid() get_curproc()->pid
#else
#include "libkern/kernel_xfs_xdd.h"
#endif

/* from ../natfeat/natfeat.c */
extern long __CDECL (*nf_call)(long id, ...);

long _cdecl hostfs_fs_dev_open     (FILEPTR *f);
long _cdecl hostfs_fs_dev_write    (FILEPTR *f, const char *buf, long bytes);
long _cdecl hostfs_fs_dev_read     (FILEPTR *f, char *buf, long bytes);
long _cdecl hostfs_fs_dev_lseek    (FILEPTR *f, long where, int whence);
long _cdecl hostfs_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf);
long _cdecl hostfs_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag);
long _cdecl hostfs_fs_dev_close    (FILEPTR *f, int pid);
long _cdecl hostfs_fs_dev_select   (FILEPTR *f, long proc, int mode);
void _cdecl hostfs_fs_dev_unselect (FILEPTR *f, long proc, int mode);



long _cdecl hostfs_fs_dev_open     (FILEPTR *f) {
	return nf_call(HOSTFS(DEV_OPEN), f);
}

long _cdecl hostfs_fs_dev_write    (FILEPTR *f, const char *buf, long bytes) {
	return nf_call(HOSTFS(DEV_WRITE), f, buf, bytes);
}

long _cdecl hostfs_fs_dev_read     (FILEPTR *f, char *buf, long bytes) {
	return nf_call(HOSTFS(DEV_READ), f, buf, bytes);
}

long _cdecl hostfs_fs_dev_lseek    (FILEPTR *f, long where, int whence) {
	return nf_call(HOSTFS(DEV_LSEEK), f, where, (long)whence);
}

long _cdecl hostfs_fs_dev_ioctl    (FILEPTR *f, int mode, void *buf) {
#ifndef ARAnyM_MetaDOS
	/*
	 * The hostfs part in the emulator will never be able to
	 * emulate file locking, since it does not have any notice of
	 * MiNT processes, so these calls have to be handled here.
	 * We need some help here however: the per-file lock list ptr
	 * must be initialized.
	 */
	switch (mode)
	{
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
		{
			LOCK *hostfs_lock;
			LOCK **locks = &hostfs_lock;
			struct flock *fl = (struct flock *) buf;
			LOCK t;
			LOCK *lck;
			long r;
			int cpid;		/* Current proc pid */
			
			r = nf_call(HOSTFS(DEV_IOCTL), f, (long)F_GETLK, locks);
			if (r < 0)
				return r;
			t.l = *fl;

			switch (t.l.l_whence)
			{
				case SEEK_SET:
				{
					break;
				}
				case SEEK_CUR:
				{
					r = hostfs_fs_dev_lseek (f, 0L, SEEK_CUR);
					t.l.l_start += r;
					break;
				}
				case SEEK_END:
				{
					r = hostfs_fs_dev_lseek (f, 0L, SEEK_CUR);
					t.l.l_start = hostfs_fs_dev_lseek (f, t.l.l_start, SEEK_END);
					(void) hostfs_fs_dev_lseek (f, r, SEEK_SET);
					break;
				}
				default:
				{
					DEBUG (("hostfs_ioctl: invalid value for l_whence"));
					return ENOSYS;
				}
			}

			if (t.l.l_start < 0) t.l.l_start = 0;
			t.l.l_whence = 0;

			cpid = p_getpid ();

			if (mode == F_GETLK)
			{
				lck = denylock (cpid, *locks, &t);
				if (lck)
					*fl = lck->l;
				else
					fl->l_type = F_UNLCK;

				return E_OK;
			}

			if (t.l.l_type == F_UNLCK)
			{
				/* try to find the lock */
				LOCK **lckptr = locks;

				lck = *lckptr;
				while (lck)
				{
					if (lck->l.l_pid == cpid
					    && ((lck->l.l_start == t.l.l_start && lck->l.l_len == t.l.l_len) ||
						    (lck->l.l_start >= t.l.l_start && t.l.l_len == 0)))
					{
						/* found it -- remove the lock */
						*lckptr = lck->next;
						DEBUG (("hostfs_ioctl: unlocked #%li: %ld + %ld", f->fc.index, t.l.l_start, t.l.l_len));
						
						/* wake up anyone waiting on the lock */
						wake (IO_Q, (long) lck);
						kfree (lck);

						nf_call(HOSTFS(DEV_IOCTL), f, (long)F_SETLK, locks);
						return E_OK;
					}

					lckptr = &(lck->next);
					lck = lck->next;
				}

				return ENSLOCK;
			}

			DEBUG (("hostfs_ioctl: lock #%li: %ld + %ld", f->fc.index, t.l.l_start, t.l.l_len));

			/* see if there's a conflicting lock */
			while ((lck = denylock (cpid, *locks, &t)) != 0)
			{
				DEBUG (("hostfs_ioctl: lock conflicts with one held by %d", lck->l.l_pid));
				if (mode == F_SETLKW)
				{
					/* sleep a while */
					sleep (IO_Q, (long) lck);
				}
				else
				{
					return ELOCKED;
				}
			}

			/* if not, add this lock to the list */
			lck = kmalloc (sizeof (*lck));
			if (!lck)
			{
				/* KERNEL_ALERT ("HostFS: kmalloc fail in: hostfs_ioctl: #%li", f->fc.index); */
				return ENOMEM;
			}

			lck->l = t.l;
			lck->l.l_pid = cpid;

			lck->next = *locks;
			*locks = lck;

			/* mark the file as being locked */
			f->flags |= O_LOCK;
			nf_call(HOSTFS(DEV_IOCTL), f, (long)F_SETLK, locks);
			return E_OK;
		}
	}
#endif /* ARAnyM_MetaDOS */
		
	return nf_call(HOSTFS(DEV_IOCTL), f, (long)mode, buf);
}

long _cdecl hostfs_fs_dev_datime   (FILEPTR *f, ushort *timeptr, int rwflag) {
	return nf_call(HOSTFS(DEV_DATIME), f, timeptr, (long)rwflag);
}

long _cdecl hostfs_fs_dev_close    (FILEPTR *f, int pid) {
#ifndef ARAnyM_MetaDOS
	if (f->flags & O_LOCK)
	{
		LOCK *lock;
		LOCK **oldlock;
		
		LOCK *hostfs_lock;
		LOCK **locks = &hostfs_lock;
		long r;
		
		r = nf_call(HOSTFS(DEV_IOCTL), f, (long)F_GETLK, locks);
		if (r == 0)
		{
			oldlock = locks;
			lock = *oldlock;
			
			while (lock)
			{
				if (lock->l.l_pid == pid)
				{
					*oldlock = lock->next;
					wake (IO_Q, (long) lock);
					kfree (lock);
				}
				else
				{
					oldlock = &lock->next;
				}
				lock = *oldlock;
			}
			r = nf_call(HOSTFS(DEV_IOCTL), f, (long)F_SETLK, locks);
		}
	}
#endif /* ARAnyM_MetaDOS */
	return nf_call(HOSTFS(DEV_CLOSE), f, (long)pid);
}

long _cdecl hostfs_fs_dev_select   (FILEPTR *f, long proc, int mode) {
	return nf_call(HOSTFS(DEV_SELECT), f, proc, (long)mode);
}

void _cdecl hostfs_fs_dev_unselect (FILEPTR *f, long proc, int mode) {
	nf_call(HOSTFS(DEV_UNSELECT), f, proc, (long)mode);
}



DEVDRV hostfs_fs_devdrv =
{
    hostfs_fs_dev_open, hostfs_fs_dev_write, hostfs_fs_dev_read, hostfs_fs_dev_lseek,
    hostfs_fs_dev_ioctl, hostfs_fs_dev_datime, hostfs_fs_dev_close, hostfs_fs_dev_select,
    hostfs_fs_dev_unselect,
    NULL, NULL /* writeb, readb not needed */
};
