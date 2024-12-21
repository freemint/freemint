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
	static char states[] = { 0xff, 0x01, 0x20, 0x21, 0x22, 0x02, 0x24 };
	int i;
	
	for (i = 0; i < 7; i++)
		if (states[i] == state)
			return state_names[i];

	return NULL;
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

#ifdef PROC_0_1_FIX
			/* Workaround for hidden proc 0 & 1 in /proc */
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
				return error(err, "Fcntl failed: %d\n", err);

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
					char *c = strrchr(pl.fname, '\\');
					
					if (!c)
						c = strrchr(pl.fname, '/');
					
					if (!c)
						c = pl.fname;
					else
						c += 1;
					
					strncpy(procname, c, 13);
				}
#endif	
				if (strchr(procname, '.'))
					*strchr(procname, '.') = '\0';
			}

			if (!strcmp(pl.cmdlin, pl.fname)) /* Workaround for kernel-threads */
				pl.cmdlin[1] = '\0';

			state = state_name(attr.st_attr == 0 ? 0xff : attr.st_attr);

			{
				long time, hour, min, sec, frac;
				
				time = pcb.systime + pcb.usrtime;
				hour = (time / 1000 / 60 / 60);
				min  = (time / 1000 / 60) % 60;
				sec  = (time / 1000) % 60;
				frac = (time % 1000) / 10;

				if (hour)
				{
					printf("%03d  %03d %3d   %3d   %s %8ld %02d:%02d:%02d  %s %s\n",
						pcb.pid, pcb.ppid, pcb.pri, pcb.curpri, state ? state : "#ERR#",
						attr.st_size,
						(int) hour, (int) min, (int) sec,
						fullpath ? pl.fname : procname,
						&pl.cmdlin[1]
					);
				}
				else
				{
					printf("%03d  %03d %3d   %3d   %s %8ld %02d:%02d.%02d  %s %s\n",
						pcb.pid, pcb.ppid, pcb.pri, pcb.curpri, state ? state : "#ERR#",
						attr.st_size,
						(int) min, (int) sec, (int) frac,
						fullpath ? pl.fname : procname,
						&pl.cmdlin[1]
					);
				}
			}

			Fclose(f);
		}
	}
	
	Dclosedir(d);
	return 0;
}
