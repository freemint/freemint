/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Modified for FreeMiNT by Frank Naumann <fnaumann@freemint.de>
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 * 
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sysctl.h	8.1 (Berkeley) 6/2/93
 */

# ifndef _mint_sysctl_h
# define _mint_sysctl_h


/*
 * Definitions for sysctl call.  The sysctl call uses a hierarchical name
 * for objects that can be examined or modified.  The name is expressed as
 * a sequence of integers.  Like a file path name, the meaning of each
 * component depends on its place in the hierarchy.  The top-level and kern
 * identifiers are defined here, and other identifiers are defined in the
 * respective subsystem header files.
 */

# define CTL_MAXNAME	12	/* largest number of components supported */


/*
 * Each subsystem defined by sysctl defines a list of variables
 * for that subsystem. Each name is either a node with further
 * levels defined below it, or it is a leaf of some particular
 * type given below. Each sysctl level defines a set of name/type
 * pairs to be used by sysctl(1) in manipulating the subsystem.
 */
struct ctlname
{
	const char *ctl_name;	/* subsystem name */
	int	ctl_type;	/* type of name */
};
# define CTLTYPE_NODE	1	/* name is a node */
# define CTLTYPE_INT	2	/* name describes an integer */
# define CTLTYPE_STRING	3	/* name describes a string */
# define CTLTYPE_QUAD	4	/* name describes a 64-bit number */
# define CTLTYPE_STRUCT	5	/* name describes a structure */


/*
 * Top-level identifiers
 */
# define CTL_UNSPEC	0		/* unused */
# define CTL_KERN	1		/* "high kernel" */
# define CTL_HW		2		/* generic cpu/io */
# define CTL_MACHDEP	3		/* machine dependent */
# define CTL_DEBUG	4		/* debugging parameters */
# define CTL_MAXID	5		/* number of valid top-level ids */

# define CTL_NAMES \
{ \
	{ 0, 0 }, \
	{ "kern", CTLTYPE_NODE }, \
	{ "hw", CTLTYPE_NODE }, \
	{ "machdep", CTLTYPE_NODE }, \
	{ "debug", CTLTYPE_NODE }, \
}


/*
 * CTL_KERN identifiers
 */
# define KERN_OSTYPE	 	 1	/* string: system version */
# define KERN_OSRELEASE	 	 2	/* string: system release */
# define KERN_OSREV	 	 3	/* int: system revision */
# define KERN_VERSION	 	 4	/* string: compile time info */
# define KERN_HOSTNAME		 5	/* string: hostname */
# define KERN_DOMAINNAME	 6	/* string: (YP) domainname */
# define KERN_SECURELVL	 	 7	/* int: system security level */
# define KERN_MAXPROC	 	 8	/* int: max processes */
# define KERN_MAXFILES	 	 9	/* int: max open files */
# define KERN_NGROUPS		10	/* int: # of supplemental group ids */
# define KERN_IOV_MAX		11	/* int: max iovec's for readv(2) etc. */
# define KERN_LOGIN_NAME_MAX	12	/* int: max length login name + NUL */
# define KERN_BOOTTIME		13	/* struct: time kernel was booted */
# define KERN_INITIALTPA	14	/* int: max TPA size of a process */
# define KERN_MAXID		15	/* number of valid kern ids */

# define CTL_KERN_NAMES \
{ \
	{ 0, 0 }, \
	{ "ostype", CTLTYPE_STRING }, \
	{ "osrelease", CTLTYPE_STRING }, \
	{ "osrevision", CTLTYPE_INT }, \
	{ "version", CTLTYPE_STRING }, \
	{ "hostname", CTLTYPE_STRING }, \
	{ "domainname", CTLTYPE_STRING }, \
	{ "securelevel", CTLTYPE_INT }, \
	{ "maxproc", CTLTYPE_INT }, \
	{ "maxfiles", CTLTYPE_INT }, \
	{ "ngroups", CTLTYPE_INT }, \
	{ "iov_max", CTLTYPE_INT }, \
	{ "login_name_max", CTLTYPE_INT }, \
	{ "boottime", CTLTYPE_STRUCT }, \
	{ "initialtpa", CTLTYPE_INT }, \
}


/*
 * CTL_HW identifiers
 */
# define HW_MACHINE	 	1	/* string: machine class */
# define HW_MACHINE_ARCH	2	/* string: machine architecture */
# define HW_MODEL	 	3	/* string: specific machine model */
# define HW_NCPU	 	4	/* int: number of cpus */
# define HW_BYTEORDER	 	5	/* int: machine byte order */
# define HW_PAGESIZE	 	6	/* int: software page size */
# define HW_FREEPHYSMEM		7	/* int: free physical memory */
# define HW_MAXID		8	/* number of valid hw ids */

# define CTL_HW_NAMES \
{ \
	{ 0, 0 }, \
	{ "machine", CTLTYPE_STRING }, \
	{ "machine_arch", CTLTYPE_STRING }, \
	{ "model", CTLTYPE_STRING }, \
	{ "ncpu", CTLTYPE_INT }, \
	{ "byteorder", CTLTYPE_INT }, \
	{ "pagesize", CTLTYPE_INT }, \
	{ "freephysmem", CTLTYPE_INT }, \
}


/*
 * CTL_DEBUG definitions
 *
 * Second level identifier specifies which debug variable.
 * Third level identifier specifies which stucture component.
 */
# define CTL_DEBUG_NAME		0	/* string: variable name */
# define CTL_DEBUG_VALUE	1	/* int: variable value */
# define CTL_DEBUG_MAXID	20


# ifndef __KERNEL__

int __sysctl (int *name, unsigned long namelen, void *old, unsigned long *oldlenp,
              const void *new, unsigned long newlen);
int sysctl (int *name, unsigned long namelen, void *old, unsigned long *oldlenp,
            const void *new, unsigned long newlen);

# endif


# endif /* _mint_sysctl_h */
