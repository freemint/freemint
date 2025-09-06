/*

	ps.c: A replacement for the old MiNTOS ps.
	
	Works and looks exactly the same, except that it displays the
	full path of the command if the parameter "-f" is used.

	19/12-2024, joska

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <mintbind.h>

#ifndef E_OK
#define E_OK 0
#endif
#ifndef FO_READ
#define FO_READ 0
#endif
#ifndef false
#define false 0
#define true 1
#endif
#ifndef NULL
#define NULL 0
#endif

/* structure for Fxattr */
#ifndef __XATTR
#define __XATTR
typedef struct xattr		XATTR;

struct xattr
{
	unsigned short	mode;
	long	index;
	unsigned short	dev;
	unsigned short	rdev;		/* "real" device */
	unsigned short	nlink;
	unsigned short	uid;
	unsigned short	gid;
	long	st_size;
	long	blksize;
	long	nblocks;
	unsigned short	mtime, mdate;
	unsigned short	atime, adate;
	unsigned short	ctime, cdate;
	short	st_attr;
	short	reserved2;
	long	reserved3[2];
};
#endif

/* Dreaddir does not return anything for pid's 0 and 1, but they
   can still be opened. This fix opens these pids specifically. */
#define PROC_0_1_FIX

static char *state_name(char state)
{
	static char *state_names[] = {
		"Run   ", "Ready ", "Wait  ", "I/O   ", "Zombie", "TSR   ", "Stop  " };
	static char states[] = {
		0,
		0x01, /* FA_RDONLY */
		0x20, /* FA_CHANGED */
		0x21, /* FA_CHANGED | FA_RDONLY */
		0x22, /* FA_CHANGED | FA_HIDDEN */
		0x02, /* FA_HIDDEN */
		0x24  /* FA_CHANGED | FA_SYSTEM */
	};
	int i;
	
	for (i = 0; i < 7; i++)
		if (states[i] == state)
			return state_names[i];

	return "#ERR#";
}

/* Process Control Block - part of MiNT's proc struct */
struct PCB {
	unsigned long magic; /* 0xabcdef98 */
	void * _base;
	short pid, ppid, pgrp;
	short _ruid, _rgid, _euid, _egid;
	unsigned short modeflags;
	short pri;
	short wait_q;
	long wait_cond;
	unsigned long systime, usrtime, chldstime, chldutime;
	unsigned long maxmem, maxdata, maxcore, maxcpu;
	short domain, curpri, _suid, _sgid;
};

struct ploadinfo {
	short fnamelen;
	char *cmdlin, *fname;
};

static int error(int err, const char *s, ...)
{
	va_list args;
	(void)err;
	va_start(args, s);
	vprintf(s, args);
	va_end(args);
	return 1;
}

int main(int argc, char *argv[])
{
	long d;
	int fullpath = false;
	int err;

	if (Pdomain(1) < 0)
		return error(0, "MiNT not available.\n");

	if (argc > 1)
	{
		if (!strcmp("-f", argv[1]))
			fullpath = true;
		else
			return error(0, "-f: Show full path of command.\n");
	}

	d = Dopendir("u:\\proc\\", 1); /* /proc is always 8+3 anyway */

	if (d >= 0 )
	{
#ifdef PROC_0_1_FIX
		int found_0 = false, found_1 = false;
#endif
		char procfullname[22];
		char procname[14];

		printf("PID  PPID PRI CURPRI STATUS    SIZE    TIME    COMMAND\n");

		while (Dreaddir(14, d, procname) == E_OK)
		{
			int f;
			char *state;
			XATTR attr;
			struct PCB *pcbptr, pcb;
			static char cmd[128], name[128];
			struct ploadinfo pl = { 127, cmd, name };
			char *c;

#ifdef PROC_0_1_FIX
			/*
			 * Workaround for hidden proc 0 & 1 in /proc:
			 * - old kernels had a bug that caused Dreaddir only to return
			 *   the process list, but no "." or ".." entries
			 * - kernels from around 2020-2025 had a bug that returned those
			 *   entries, but skipped the first 2 processes instead
			 * - in recent kernels, Dreaddir() returns all entries,
			 *   including "." and ".."
			 */
			if (!strcmp(procname, "."))
				strcpy(procname, "MiNT.000");

			if (!strcmp(procname, ".."))
				strcpy(procname, ".001");
#else
			if (*procname == '.')
				continue;
#endif
			sprintf(procfullname, "u:\\proc\\%s", procname);

			f = (int) Fopen(procfullname, FO_READ);

			if (f < 0)
				return error(f, "Could not open %s: %d.\n", procfullname, f);

			if ((err = Fcntl(f, &pcbptr, 0x5001)) < 0) /* PPROCADDR */
				return error(err, "Fcntl %s failed: %d\n", procname, err);

			if (Fseek((long) pcbptr, f, 0) < 0)
				return error(0, "Fseek failed.\n");

			if (Fread(f, sizeof(struct PCB), &pcb) != sizeof(struct PCB))
				return error(0, "Fread failed.\n");

			if ((err = Fcntl(f, &attr, 0x4600)) < 0) /* FSTAT */
				return error(0, "FSTAT failed: %d\n", err);

			if ((err = Fcntl(f, &pl,   0x500c)) < 0) /* PLOADINFO */
				return error(err, "PLOADINFO failed: %d\n", err);

			if (!fullpath)
			{
#ifdef PROC_0_1_FIX
				/* Workaround for hidden proc 0 & 1 in /proc */
				if (*procname == '.')
				{
					c = strrchr(pl.fname, '\\');
					
					if (!c)
						c = strrchr(pl.fname, '/');
					
					if (!c)
						c = pl.fname;
					else
						c += 1;
					
					strncpy(procname, c, 13);
				}
#endif	
			}
			if ((c = strchr(procname, '.')) != NULL)
				*c = '\0';

			/* Workaround for kernel-threads */
			if (!strcmp(pl.cmdlin, pl.fname))
				pl.cmdlin[1] = '\0';

			/*
			 * Workaround for SLBs, which have their filename at the first byte of the cmdline
			 * and also return the name of the loading application in the filename
			 */
			if (pl.cmdlin[0] == '\0' && pl.cmdlin[1] == ':')
				pl.cmdlin[1] = '\0';
			if (!(pl.cmdlin[0] >= 'A' && pl.cmdlin[0] <= 'Z' && pl.cmdlin[1] == ':'))
			{
				strcpy(pl.cmdlin, pl.cmdlin + 1);
			} else
			{
				if (fullpath)
					strcpy(pl.fname, pl.cmdlin);
				pl.cmdlin[0] = '\0';
			}

			state = state_name(attr.st_attr);

#ifdef PROC_0_1_FIX
			if ( !((pcb.pid == 0 && found_0) || (pcb.pid == 1 && found_1)) )
#endif
			{
				long time;
				int hour, min, sec, frac;
				char sep;

				time = pcb.systime + pcb.usrtime;
				hour = (int)((time / 1000 / 60 / 60));
				min  = (int)((time / 1000 / 60) % 60);
				sec  = (int)((time / 1000) % 60);
				frac = (int)((time % 1000) / 10);

#ifdef PROC_0_1_FIX
				if (pcb.pid == 0)
					found_0 = true;
				
				if (pcb.pid == 1)
					found_1 = true;
#endif

				sep = ':';
				if (hour == 0)
				{
					hour = min;
					min = sec;
					sec = frac;
					sep = '.';
				}
				printf("%03d  %03d %3d   %3d   %s %8ld %02d:%02d%c%02d  %s %s\n",
					pcb.pid, pcb.ppid, pcb.pri, pcb.curpri, state,
					attr.st_size,
					hour, min, sep, sec,
					fullpath ? pl.fname : procname,
					pl.cmdlin
				);
			}

			Fclose(f);
		}
	}
	
	Dclosedir(d);
	return 0;
}
