.TH Fcntl 2 "MiNT Programmer's Manual" "Version 1.08" "Aug. 5, 1993"
.SH NAME
Fcntl \- perform various control operations on a file
.SH SYNOPSIS
.nf
#include <filesys.h>

LONG Fcntl( WORD fh, LONG arg, WORD cmd);
.fi
.SH DESCRIPTION
.I Fcntl
performs various control operations on the open file with GEMDOS file
handle
.IR fh .
The specific command to perform is given by
.IR cmd ;
the possible commands are given by symbolic constants in filesys.h,
and are listed below.
.I arg
is an argument whose meaning depends on the command.
.PP
The following commands are applicable to any file descriptor:
.IP F_DUPFD
Return a duplicate for the file handle. The new (duplicate) handle will
be an integer >= arg and < 32. If no free handles exist in that range,
ENHNDL will be returned. The
.IR Fdup ( fh )
system call is equivalent to
.IR Fcntl ( fh ,
6L, F_DUPFD)
.IP F_GETFD
Return the noinherit flag for the file descriptor. This flag is 0 if child
processes started by
.I Pexec
should inherit this open file, and 1 if they should not.
.I arg
is ignored.
.IP F_SETFD
Set the noinherit flag for the file descriptor from the low order bit
of
.IR arg .
The default value of the flag is 0 for handles 0-5 (the "standard" handles)
and 1 for other (non-standard) handles. Note that the noinherit flag applies
only to this particular file descriptor; another descriptor obtained from
.I fh
by the
.I Fdup
system call or by use of the F_DUPFD option to
.I Fcntl
will have the noinherit flag set to the default. Also note that
these defaults are not the same as for the Unix operating system.
.IP F_GETFL
Returns the user-settable file descriptor flags. These flags are the same
as the mode passed to
.IR Fopen ,
unless they have been further modified by another
.I Fcntl
operation.
.I arg
is ignored.
.IP F_SETFL
Set user-settable file descriptor flags from the argument
.IR arg .
Only the user-settable bits in
.I arg
are considered; the settings of other bits are ignored, but should be 0
for future compatibility. Moreover, it is not possible to change a file's
read-write mode or sharing modes with this call; attempts to do so will
(silently) fail.
.IP F_GETLK
.I arg
is a pointer to an flock structure:
.nf
struct flock {
	short l_type;	/* type of lock */
#define F_RDLCK		0
#define F_WRLCK		1
#define F_UNLCK		3
	short l_whence;	/* 0:file start, 1:current, 2:EOF */
	long l_start;	/* offset of locked region */
	long l_len;	/* 0 for rest of file */
	short l_pid;	/* set by F_GETLK */
};
.fi
If a lock exists which would prevent this lock from being applied to the
file, the existing lock is copied into the structure and the l_pid
field is set to the process id of the locking process. Otherwise,
l_type is set to F_UNLCK. If a conflicting lock is held by a process on
a different machine on a network, then the l_pid field will be set to a
value defined by the network file system. This value will be in the range
0x1000 to 0xFFFF, and will therefore not conflict with any process id
since process id's must be less than 0x1000.
.IP F_SETLK
Set (if l_type is F_RDLCK or F_WRLCK) or clear (if l_type is F_UNLCK)
an advisory lock on a file. If the file is a FIFO, the whole file must be
locked or unlocked at once, i.e. l_whence, l_start, and l_len must be 0. If
this lock would conflict with a lock held by another process, ELOCKED is
returned. If an attempt is made to clear a non-existent lock, ENSLOCK
is returned. More than one read lock may be placed on the same region
of a file, but no write lock may overlap with any other sort of lock.
If a process holds locks on a file, then the locks are automatically
released whenever the process closes an open file handle referring to
that file, or when the process terminates.
.IP F_SETLKW
Like
.IR F_SETLK ,
but if the lock requested would conflict with a lock held by another process,
the calling process is suspended until all conflicting locks are released.
.IP FSTAT
.I arg
points to an XATTR structure, which is filled in with the appropriate
extended file attributes for the file to which
.I fd
refers just as though the
.I Fxattr
system call (q.v.) had been made on the file.
.IP FIONREAD
.I arg
points to a 32 bit integer, into which is written the number of bytes that
are currently available to be read from this descriptor; a read of this number
of bytes or less should not cause the process to block (wait for more input).
Note that for some files only an estimate can be provided, so the number is
not always completely accurate.
.IP FIONWRITE
.I arg
points to a 32 bit integer, into which is written the number of bytes that
may be written to the indicated file descriptor without causing the process
to block.
Note that for some files only an estimate can be provided, so the number is
not always completely accurate.
.PP
The following commands are valid for any terminal device, e.g. the
console or a pseudo-terminal:
.IP TIOCGETP
Get terminal parameters.
.I arg
is a pointer to a block of memory with the following structure:
.nf
struct sgttyb {
	char sg_ispeed;		/* reserved */
	char sg_ospeed;		/* reserved */
	char sg_erase;		/* erase character */
	char sg_kill;		/* line kill character */
	short sg_flags;		/* terminal control flags */
};
.fi
.IP TIOCSETP
Set terminal parameters from the struct sgttyb pointed to by
.IR arg .
.IP TIOCGETC
Get terminal control characters.
.I arg
is a pointer to the following structure:
.nf
struct tchars {
	char t_intrc;	/* raises SIGINT */
	char t_quitc;	/* raises SIGQUIT */
	char t_startc;	/* starts terminal output */
	char t_stopc;	/* stops terminal output */
	char t_eofc;	/* marks end of file */
	char t_brkc;	/* marks end of line */
};
.fi
.IP TIOCSETC
Set terminal control characters from the struct tchars pointed to by
.IR arg .
Setting any character to the value 0 causes the corresponding
function to become unavailable.
.IP TIOCGLTC
Get extended terminal control characters, and put them in the structure
pointed to by
.IR arg :
.nf
struct ltchars {
	char t_suspc;	/* raises SIGTSTP now */
	char t_dsuspc;	/* raises SIGTSTP when read */
	char t_rprntc;	/* redraws the input line */
	char t_flushc;	/* flushes output */
	char t_werasc;	/* erases a word */
	char t_lnextc;	/* quotes a character */
};
.fi
.IP TIOCSLTC
Set extended terminal control characters from the struct ltchars pointed to
by
.IR arg .
Setting any of the characters to 0 causes the corresponding function
to become unavailable.
.IP TIOCGWINSZ
.I arg
has type "struct winsize *". The current window size for this window is placed
in the structure pointed to by
.IR arg ,
which has the following fields:
.nf
struct winsize {
	short	ws_row;	/* # of rows of text in window*/
	short	ws_col;	/* # of columns of text */
	short	ws_xpixel; /* width of window in pixels */
	short	ws_ypixel; /* height of window in pixels */
};
.fi
If any fields in the structure are 0, this means that the corresponding value
is unknown.
.IP TIOCSWINSZ
.I arg
has type "struct winsize *". The current window size for the window is set from
the structure pointed to by
.IR arg .
Note that the kernel maintains the information but does
.B not
act upon it in any way; it is up to window managers to perform whatever
physical changes are necessary to alter the window size, and to raise the
SIGWINCH signal if necessary.
.IP TIOCGPGRP
.I arg
has type "long *"; the process group for the terminal is placed into the long
pointed to by it.
.IP TIOCSPGRP
.I arg
has type "long *"; the process group for the terminal is set from the long
pointed to by it. Processes in any other process group will be sent job
control signals if they attempt input or output to the terminal.
.IP TIOCSTART
Restart output to the terminal (as though the user typed control-Q) if it
was stopped by a control-S or TIOCSTOP command.
.I arg
is ignored.
.IP TIOCSTOP
Stop output to the terminal (as though the user typed control-S).
.I arg
is ignored.
.IP TIOCGXKEY
Get the definition of a function or cursor key.
.I arg
is a pointer to a structure with the following fields:
.nf
struct xkey {
	short	xk_num;	   /* function key number */
	char	xk_def[8]; /* associated string */
};
.fi
The
.I xk_num
field must be initialized with the number of the desired key:
.nf
  xk_num	 Key
   0-9		F1-F10
  10-19		F11-F20 (shift F1-shift F10)
   20		cursor up
   21		cursor down
   22		cursor right
   23		cursor left
   24		help
   25		undo
   26		insert
   27		clr/home
   28		shift+cursor up
   29		shift+cursor down
   30		shift+cursor right
   31		shift+cursor left
.fi
The string currently associated with the indicated key is copied into
xk_def; this string is always null-terminated.
.IP TIOCSXKEY
.I arg
is a structure of type struct xkey, as defined above. Both
the
.I xk_num
and the
.I xk_def
fields must be defined. After execution of this command, and if the
XKEY bit is set in the
.I sg_flags
field of the sgttyb structure associated with the terminal, then if the
indicated key is pressed on the affected terminal, any MiNT domain
process using
.I Fread
to read the key will get the string in
.I xk_def
instead of ASCII 0. Note that this translation occurs only for MiNT
domain processes and only for the
.I Fread
system call. Also note that the string in
.I xk_def
must be null terminated, and so at most 7 characters may be assigned
to any key.

.PP
The following commands are valid only for processes opened as files:
.IP PBASEADDR
.I arg
is a pointer to a 32 bit integer, into which the address of the process
basepage for the process to which
.I fh
refers is written.
.IP PPROCADDR
.I arg
is a pointer to a 32 bit integer, into which the address of the process
control structure for the process is written. Note that this control
structure differs from the one found in previous versions (before 0.93)
of MiNT; it no longer includes the process context, so that this
part of the structure may be changed without causing compatibility
problems. See the
.I PCTXTSIZE
command.
.IP PCTXTSIZE
.I arg
is a pointer to a 32 bit integer, into which the length of a process
context structure is written. There are two of these structures located
in memory just before the process control structure whose address
is returned by the
.I PPROCADDR
command. The first is the current process context; the second is the
saved context from the last system call.

.IP PGETFLAGS
.I arg
is a pointer to a 32 bit integer, into which the process memory
allocation flags are copied. These flags are the same ones found
in the `prgflags' field of GEMDOS executable programs, or as the
first parameter to
.I Pexec
mode 7.

.IP PSETFLAGS
.I arg
is a pointer to a 32 bit integer, from which the process memory
flags for the target process will be set. Note that only the low
order 16 bits are actually used right now, and not all of these
are valid. See the documentation for GEMDOS executable programs
for details on the meanings of the flags.

.IP PTRACEGFLAGS
.I arg
is a pointer to a 16 bit integer, into which the current trace
flags of the target process are copied. If the process is not being
traced, the flags will be 0.

.IP PTRACESFLAGS
.I arg
is a pointer to a 16 bit integer, the bits of which determine
how the target process will respond to signals. If bit #0
is set, the target process will respond to signals and other
exceptions by stopping, and the process which set the flags will
receive a SIGCHLD signal informing it of this fact; it may then use
the
.I Pwait3
system call to retreive information about why the process stopped,
and may use
.I Fread
and
.I Fwrite
to interrogate and possibly change the state of the process
before causing it to continue (see below). If bit #0 is clear,
then all process tracing will cease, and the process will respond
to signals in the normal way. All other bits are reserved and should
be set to 0 for now. If some other process has already used
.I PTRACESFLAGS
to set process tracing for the target process, then the call will
fail.

.IP PTRACEGO
Restarts a process that was being traced by the caller and which
stopped because of a signal.
.I arg
is a pointer to a 16 bit integer which is either 0 (in which case
all pending signals for the stopped process are cleared before it
is restarted) or the number of a signal which is to be delivered
to the process after it restarts. Typically, this will be the same
as the signal that stopped it.

.IP PTRACESTEP
Like
.IR PTRACEGO ,
except that the trace bit will be set in the status register of
the restarted process; thus, a SIGTRAP signal will be generated
in that process after 1 user instruction has been executed. Note
that it is not possible to trace processes that are executing
in the kernel; if the process was stopped while executing in
the kernel the trace bit will be set only when the process returns
from the kernel.

.IP PLOADINFO
Returns information about the parameters passed to Pexec to start this
process.
.I arg
should be a pointer to a structure with the following format:
.nf
struct ploadinfo {
    short fnamelen;	/* length of fname field */
    char *cmdlin;	/* must point to 128 bytes, to be filled in
			   with the process' command line */
    char *fname;	/* must point to "fnamelen" bytes, to be
			   filled in with the full path+name of
			   the file from which the process was
			   launched */
}
.fi
The data pointed to by the structure elements will be filled in with the
appropriate data for this
process. If the process's full name and path are too long to fit into
the provided space, ENAMETOOLONG will be returned.
On success, a 0 is returned.

.PP
The following commands are valid only for files which represent
shared memory.
.IP SHMSETBLK
.I arg
is a pointer to a block of memory previously allocated by
.IR Mxalloc .
The memory will be offered for sharing under the name of the file
represented by
.I fd
(which must be a file in the U:\\SHM subdirectory).
.IP SHMGETBLK
.I arg
must be 0, for future compatibility. Returns the address of the
block of memory previously associated with the file via
.IR SHMSETBLK ,
or a NULL pointer if an error occurs.
Note that different processes may see the shared memory block at
different addresses in their address spaces. Therefore, the
shared memory block should not contain any absolute pointers to
data.

.SH RETURNS
0 or a positive number if successful (for most commands; but see the
specific descriptions above).
.PP
EIHNDL if
.I fh
is not a valid GEMDOS open handle.
.PP
EINVFN if the specified command is not valid for this file handle
.PP
Some other (LONG) negative error number if an error occurs; different
commands may recognize different possible errors.
.SH "SEE ALSO"
.IR Fdup (2),
.IR Flock (2),
.IR Fopen (2),
.IR Fxattr (2),
.IR Pgetpgrp (2),
.IR Psetpgrp (2)

.SH BUGS
Very little error checking is done. In particular, ownership of terminals
is not properly checked, nor is read/write access to the files. Do not
rely on this bug; it will be fixed some day.
.PP
File locking is not yet implemented for some file systems.
