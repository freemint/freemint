Appendix B: Debugging

------------------------------------------------------------------------------
2001-07-31

These interface is now superseeded by the new Ptrace() system call.
The Ptrace() system call is BSD/Linux compatible, much easier and safer
to use. It's recommended to use the Ptrace() interface in new applications.
The old interface is included for backward compatibility.

Please see in the Ptrace() documentation for more details.

Frank Naumann <fnaumann@freemint.de>
------------------------------------------------------------------------------


MiNT provides a special pseudo-drive, U:\PROC, in which processes appear
as files. This drive may be used by debuggers much as the Unix /proc
file system is used, although the specific details of implementation will
be different.

The entry for a process in U:\PROC has a name like SOMEPROG.nnn, where
"nnn" is a 3 digit number which is the (decimal) process ID of the process,
and "SOMEPROG" is the name of the process. When opening a process, only
the process ID matters; a process does *not* need to know another process'
name in order to open it, only its process id. So, for example,
   fd = Fopen("U:\\PROC\\FOO.032",2);
   fd = Fopen("U:\\PROC\\.032",2);
and
   fd = Fopen("U:\\PROC\\TCSH.032",2);
will all open the process with process id #32 for reading and writing,
regardless of the actual name of that process.

Also, note that a process id of -1 refers to the current process, and a
process id of -2 refers to the parent of the current process; thus,
   fd = Fopen("U:\\PROC\\.-1",2);
may always be used to open oneself.

Before a process may be debugged (or "traced") it must first be marked
as a traced process, and a specific process (often its parent) must
be marked as the "tracer", the program which will be notified when the
traced process receives signals.

(1) If the process to be traced is started with Pexec(), and the high bit
    of the Pexec() mode is set, then the child process begins
    as a "traced" process automatically, with its parent as tracer.

Example:
    childpid = Pexec(0x8000|100, "foo.prg", "", 0L);

(2) The traced process may indicate that it wishes to be traced by opening
    its own entry in U:\PROC and performing an Fcntl(...PTRACESFLAGS) call
    which enables tracing. In this case, the child's parent will become
    the tracer automatically. Note that if the parent is not prepared to
    perform debugging functions, there could be undesireable results, so
    the child must be *certain* that the parent wishes to debug it; for
    example, this is the case if the child and parent are both executing
    from the same program image (e.g. the child was started with the
    Pvfork() system call). This method of indicating tracing is quite
    similar to Unix's ptrace(0,...) function.

Example:
    if ((pid = Pvfork()) == 0) {
        short flag = 1;
	int fd;

/* here we are in the child */
/* open self; see notes above */
 	fd = Fopen("U:\\PROC\\A.-1",2);
/* perform Fcntl */
	Fcntl(fd, &flag, PTRACESFLAGS);
/* close self, we don't need the handle any more */
	Fclose(fd);
/* now overlay the child with a new program image */
	Pexec(200, "foo.prg", "", 0L);
    }

(3) A process may force another process to be traced by opening that process'
    entry in U:\PROC, and doing an Fcntl(...PTRACESFLAGS) on it. This is
    the only way in which to trace a process that is not the
    child of the debugging process.

Example:
    short flag = 1;

/* open process "pid" for tracing */
    sprintf(name, "U:\\PROC\\A.%03d", pid);
    fd = Fopen(name, 2);
    Fcntl(fd, &flag, PTRACESFLAGS);
/* leave fd open so that we can read and write it... */


What Happens When A Traced Process Receives a Signal

If a traced process receives a signal (for example, a bus error), then
it is placed into a STOP condition and a SIGCHLD signal is sent to
the tracer. If the tracer performs a Pwait3() or Pwaitpid() system call,
it can then retrieve the pid of the stopped process, and the signal which
caused that process to stop. For example:

#define WIFSTOPPED(x)	(((int)((x) & 0xFF) == 0x7F) && ((int)(((x) >> 8) & 0xFF) != 0))
#define WSTOPSIG(x)	((int)(((x) >> 8) & 0xFF))
    unsigned long r;

/* 0x2 is a flag that indicates that we want to see stopped processes */
    r = Pwait3(0x2, 0L);
    if (WIFSTOPPED(r)) {
	pid = r >> 16;
	signal = WSTOPSIG(r);
    }

The tracer can then examine the stopped process' address space and registers
(using the Fread(), Fwrite(), and Fcntl() system calls; see below) and/or make
modifications to the stopped process' state. It can then restart that
process with either the PTRACEGO, PTRACEFLOW, or PTRACESTEP Fcntl commands.
The PTRACEGO command Fcntl(fd, &sig, PTRACEGO), where "sig" is a 16 bit
integer, restarts the process. If sig == 0, then all pending signals in
the traced process are cleared; otherwise, the signal represented by
"sig" will be delivered to the process after it starts again. Normally,
this signal would be the same one which caused the process to stop. Note
that this second time the signal is delivered it will not cause the
process to stop, but rather will cause whatever action is normally associated
with the signal (for example, SIGKILL will kill the process).

PTRACEFLOW and PTRACESTEP are similar to PTRACEGO, but they also set some
bits in the status register of the stopped process which will cause a
SIGTRAP (trace trap) signal to be raised either on the next instruction
to be executed (PTRACESTEP) or the next branch or jump instruction to
be executed (PTRACEFLOW; this only works on a 68030 or 68040 processor).
Note that if the traced process was executing a system call at the time
it stopped, the trace trap signal will not take effect until the process
leaves the kernel. PTRACESTEP, then, makes it possible to single step
through the traced program.


Reading and Writing the Process' Address Space, and Setting Breakpoints

Traditional debuggers for the ST have directly accessed the address space
of the debugged process. This is undesireable because it assumes that
both processes share a common address space, something which may not be
the case if memory protection is enabled; also, in a virtual memory
system logical addresses in the child and parent may be translated
differently. The U:\PROC file system may be used to avoid both of these
difficulties. If the debugger opens the process to be debugged for
reading and writing, it may then use Fread() and Fwrite() system calls to
transfer data (e.g. breakpoint instructions) to and from the debugged
process' address space.

To read 100 bytes from address 0x0123456 in the address space of
process 12, for example, one would do:
    char buf[100];

    fd = Fopen("U:\\PROC\\A.012", 2);
    Fseek(0x0123456L, fd, 0);
    Fread(fd, 100L, buf);
(Note that error checking should be performed in any real application;
for example, the Fopen() could very well fail if the process being
opened is another user's process.)


Reading and Writing the Process' Registers

The registers of the stopped process are also available for inspection.
The PPROCADDR and PCTXTSIZE Fcntl commands are used to find the
address of the process context block in the traced process' address
space; the registers of the process can then be read from the address
space with Fread, or written to the address space with Fwrite.

Example:
    long curprocaddr;
    long ctxtsize;
    struct context {
	long regs[15];	/* registers d0-d7, a0-a6 */
	long usp;	/* user stack pointer */
	short sr;	/* status register */
	long pc;	/* program counter */
	long ssp;	/* supervisor stack pointer */
	long tvec;	/* GEMDOS terminate vector */
	char fstate[216]; /* internal FPU state */
	long fregs[3*8];  /* registers fp0-fp7 */
	long fctrl[3];	/* FPCR/FPSR/FPIAR */
/* there are some more, undocumented, fields, in the
 * MiNT context structure
 */
    } c;

/* get the address of the process control structure */
    Fcntl(fd, &curprocaddr, PPROCADDR);

/* get the size of the process context structure.
 * there are 2 of these, located immediately before
 * the process control structure. The first one
 * is the one we're interested in (the second one
 * is used by MiNT, and should never be modified).
 */
    Fcntl(fd, &ctxtsize, PCTXTSIZE);

/* make curprocaddr point to the first context struct */
    curprocaddr -= 2*ctxtsize;

/* now read the context structure */
    Fseek(curprocaddr, fd, 0);
    Fread(fd, (long)sizeof(struct context), &c);

/* the various fields of c are now set up properly */
   ... /* code omitted */

/* now write back the context to reflect whatever changes
 * we made
 */
    Fseek(curprocaddr, fd, 0);
    Fwrite(fd, (long)sizeof(struct context), &c);

