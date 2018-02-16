/*
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
 * All rights reserved.
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
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
 */

# ifndef _mint_sem_h
# define _mint_sem_h

# include "ktypes.h"
# include "ipc.h"

union __semun;

struct __sem
{
	ushort		semval;		/* semaphore value */
	short		sempid;		/* pid of last operation */
	ushort		semncnt;	/* # awaiting semval > cval */
	ushort		semzcnt;	/* # awaiting semval = 0 */
};

struct semid_ds
{
	struct ipc_perm	sem_perm;	/* operation permission struct */
	struct __sem	*sem_base;	/* pointer to first semaphore in set */
	ushort		sem_nsems;	/* number of sems in set */
	
	long		sem_otime;	/* last operation time */
	long		sem_pad1;	/* SVABI/386 says I need this here */
	
	long		sem_ctime;	/* last change time */
    					/* Times measured in secs since */
    					/* 00:00:00 GMT, Jan. 1, 1970 */
	long		sem_pad2;	/* SVABI/386 says I need this here */
	
	long		sem_pad3[4];	/* SVABI/386 says I need this here */
};

/*
 * semop's sops parameter structure
 */
struct sembuf
{
	ushort		sem_num;	/* semaphore # */
	short		sem_op;		/* semaphore operation */
	short		sem_flg;	/* operation flags */
};
#define SEM_UNDO	010000		/* undo changes on process exit */

/*
 * commands for semctl
 */
# define GETNCNT	3		/* Return the value of semncnt {READ} */
# define GETPID		4		/* Return the value of sempid {READ} */
# define GETVAL		5		/* Return the value of semval {READ} */
# define GETALL		6		/* Return semvals into arg.array {READ} */
# define GETZCNT	7		/* Return the value of semzcnt {READ} */
# define SETVAL		8		/* Set the value of semval to arg.val {ALTER} */
# define SETALL		9		/* Set semvals from arg.array {ALTER} */

/*
 * semaphore info struct
 */
struct seminfo
{
	long		semmap;		/* # of entries in semaphore map */
	long		semmni;		/* # of semaphore identifiers */
	long		semmns;		/* # of semaphores in system */
	long		semmnu;		/* # of undo structures in system */
	long		semmsl;		/* max # of semaphores per id */
	long		semopm;		/* max # of operations per semop call */
	long		semume;		/* max # of undo entries per process */
	long		semusz;		/* size in bytes of undo structure */
	long		semvmx;		/* semaphore maximum value */
	long		semaem;		/* adjust on exit max value */
};

/*
 * Internal "mode" bits.  The first of these is used by ipcs(1), and so
 * is defined outside the kernel as well.
 */
# define SEM_ALLOC	01000		/* semaphore is allocated */


# endif /* _mint_sem_h */
