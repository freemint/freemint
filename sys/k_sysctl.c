/*
 * $Id$
 * 
 * This file belongs to FreeMiNT. It's not in the original MiNT 1.12
 * distribution. See the file CHANGES for a detailed log of changes.
 * 
 * 
 * Copyright 2000 Frank Naumann <fnaumann@freemint.de>
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
 * Author: Frank Naumann <fnaumann@freemint.de>
 * Started: 2001-04-30
 * 
 * Please send suggestions, patches or bug reports to me or
 * the MiNT mailing list.
 * 
 */

# include "k_sysctl.h"

# include "arch/info_mach.h"
# include "buildinfo/version.h"
# include "libkern/libkern.h"
# include "mint/endian.h"
# include "mint/iov.h"
# include "mint/proc.h"
# include "mint/sysctl.h"
# include "sys/param.h"

# include "info.h"
# include "memory.h"
# include "proc.h"
# include "time.h"


long kern_sysctl (long *name, ulong namelen, void *oldp, ulong *oldlenp, const void *newp, ulong newlen, struct proc *p);
long hw_sysctl (long *name, ulong namelen, void *oldp, ulong *oldlenp, const void *newp, ulong newlen, struct proc *p);

long _cdecl
sys_p_sysctl (long *name, ulong namelen, void *old, ulong *oldlenp, const void *new, ulong newlen)
{
	struct proc *p = curproc;

	long ret;
	sysctlfn *fn;


	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (namelen > CTL_MAXNAME || namelen < 2)
		return EINVAL;

	/*
	 * For all but CTL_PROC, must be root to change a value.
	 * For CTL_PROC, must be root, or owner of the proc (and not suid),
	 * this is checked in proc_sysctl() (once we know the targer proc).
	 */
	if (new != NULL
# ifdef CTL_PROC
		&& name[0] != CTL_PROC
# endif
	)
	{
		if (p->euid)
			return EPERM;
	}

	switch (name[0])
	{
		case CTL_KERN:
			fn = kern_sysctl;
			break;
		case CTL_HW:
			fn = hw_sysctl;
			break;
		case CTL_MACHDEP:
			return EINVAL;
			// XXX todo
			// fn = cpu_sysctl;
			break;
# ifdef DEBUG
		case CTL_DEBUG:
			return EINVAL;
			// XXX todo
			// fn = debug_sysctl;
			break;
# endif
		default:
			return EOPNOTSUPP;
	}

	ret = (*fn)(name + 1, namelen - 1, old, oldlenp, new, newlen, p);
	return ret;
}


/*
 * Attributes stored in the kernel.
 */
char hostname[MAXHOSTNAMELEN] = "(none)";
long hostnamelen;

char domainname[MAXHOSTNAMELEN] = "(none)";
long domainnamelen;

long securelevel = 0;

/*
 * kernel related system variables.
 */
long
kern_sysctl (long *name, ulong namelen, void *oldp, ulong *oldlenp, const void *newp, ulong newlen, struct proc *p)
{
	long ret;

	/* All sysctl names at this level, except for a few, are terminal */
	switch (name[0])
	{
		/* case : */
			/* Not terminal */
			/* break; */
		default:
			if (namelen != 1)
				/* overloaded */
				return ENOTDIR;
	}

	switch (name[0])
	{
		case KERN_OSTYPE:
			return sysctl_rdstring (oldp, oldlenp, newp, ostype);

		case KERN_OSRELEASE:
			return sysctl_rdstring (oldp, oldlenp, newp, osrelease);

		case KERN_OSREV:
			return sysctl_rdlong (oldp, oldlenp, newp, PATCH_LEVEL);

		case KERN_VERSION:
			return sysctl_rdstring (oldp, oldlenp, newp, version);

		case KERN_HOSTNAME:
			ret = sysctl_string (oldp, oldlenp, newp, newlen, hostname, sizeof (hostname));
			if (newp && !ret)
				hostnamelen = newlen;
			return ret;

		case KERN_DOMAINNAME:
			ret = sysctl_string (oldp, oldlenp, newp, newlen, domainname, sizeof (domainname));
			if (newp && !ret)
				domainnamelen = newlen;
			return ret;

		case KERN_MAXPROC:
			return sysctl_rdlong (oldp, oldlenp, newp, UNLIMITED);

		case KERN_MAXFILES:
			return sysctl_rdlong (oldp, oldlenp, newp, UNLIMITED);

# if 0
		case KERN_SECURELVL:
			level = securelevel;
			ret = sysctl_long (oldp, oldlenp, newp, newlen, &level);
			if (ret || newp == NULL)
				return ret;
			if (level < securelevel && p->p_pid != 1)
				return EPERM;
			securelevel = level;
			return 0;
# endif

		case KERN_NGROUPS:
			return sysctl_rdlong (oldp, oldlenp, newp, NGROUPS_MAX);

		case KERN_IOV_MAX:
			return sysctl_rdlong (oldp, oldlenp, newp, IOV_MAX);

//		case KERN_LOGIN_NAME_MAX:
//			return sysctl_rdlong (oldp, oldlenp, newp, LOGIN_NAME_MAX);

		case KERN_BOOTTIME:
			return sysctl_rdstruct (oldp, oldlenp, newp, &boottime, sizeof (struct timeval));

		case KERN_INITIALTPA:
			return sysctl_long (oldp, oldlenp, newp, newlen, &initialmem);
	}

	return EOPNOTSUPP;
}

/*
 * hardware related system variables.
 */
long
hw_sysctl (long *name, ulong namelen, void *oldp, ulong *oldlenp, const void *newp, ulong newlen, struct proc *p)
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		/* overloaded */
		return ENOTDIR;

	switch (name[0])
	{
		case HW_MACHINE:
			return sysctl_rdstring (oldp, oldlenp, newp, machine);

		case HW_MACHINE_ARCH:
			return sysctl_rdstring (oldp, oldlenp, newp, machine_arch);

		case HW_MODEL:
			return sysctl_rdstring (oldp, oldlenp, newp, cpu_model);

		case HW_NCPU:
			return sysctl_rdlong (oldp, oldlenp, newp, 1);

		case HW_BYTEORDER:
			return sysctl_rdlong (oldp, oldlenp, newp, BYTE_ORDER);

		case HW_PAGESIZE:
			return sysctl_rdlong (oldp, oldlenp, newp, QUANTUM);

		case HW_FREEPHYSMEM:
			return sysctl_rdlong (oldp, oldlenp, newp, freephysmem ());
	}

	return EOPNOTSUPP;
}


static long copyout(const void *src, void *dst, ulong len) { memcpy (dst, src, len); return 0; }
static long copyin(const void *src, void *dst, ulong len) { memcpy (dst, src, len); return 0; }

/*
 * Convenience macros.
 */

# define SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, valp, len) 	\
	if (oldlenp) {						\
		if (!oldp)					\
			*oldlenp = len;				\
		else {						\
			if (*oldlenp < len)			\
				return ENOMEM;			\
			*oldlenp = len;				\
			error = copyout (valp, oldp, len);	\
		}						\
	}

# define SYSCTL_SCALAR_CORE_TYP(oldp, oldlenp, valp, typ)	\
	SYSCTL_SCALAR_CORE_LEN(oldp, oldlenp, valp, sizeof(typ))

# define SYSCTL_SCALAR_NEWPCHECK_LEN(newp, newlen, len)		\
	if (newp && newlen != len)				\
		return EINVAL;

# define SYSCTL_SCALAR_NEWPCHECK_TYP(newp, newlen, typ)		\
	SYSCTL_SCALAR_NEWPCHECK_LEN(newp, newlen, sizeof(typ))

# define SYSCTL_SCALAR_NEWPCOP_LEN(newp, valp, len)		\
	if (error == 0 && newp)					\
		error = copyin (newp, valp, len);

# define SYSCTL_SCALAR_NEWPCOP_TYP(newp, valp, typ)      	\
	SYSCTL_SCALAR_NEWPCOP_LEN(newp, valp, sizeof(typ))

static long
SYSCTL_STRING_CORE (void *oldp, ulong *oldlenp, const char *str)
{
	long error = 0;
	
	if (oldlenp)
	{
		long len = strlen (str) + 1;
		
		if (oldp)
		{
			long err2 = 0;
			
			if (*oldlenp < len)
			{
				err2 = ENOMEM;
				len = *oldlenp;
			}
			else
				*oldlenp = len;
			
			error = copyout (str, oldp, len);
			if (error == 0)
				error = err2;
		}
		else
			*oldlenp = len;
	}
	
	return error;
}

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
long
sysctl_long (void *oldp, ulong *oldlenp, const void *newp, ulong newlen, long *valp)
{
	long error = 0;
	
	SYSCTL_SCALAR_NEWPCHECK_TYP (newp, newlen, long)
	SYSCTL_SCALAR_CORE_TYP (oldp, oldlenp, valp, long)
	SYSCTL_SCALAR_NEWPCOP_TYP (newp, valp, long)
	
	return error;
}


/*
 * As above, but read-only.
 */
long
sysctl_rdlong (void *oldp, ulong *oldlenp, const void *newp, long val)
{
	long error = 0;
	
	if (newp)
		return EPERM;
	
	SYSCTL_SCALAR_CORE_TYP (oldp, oldlenp, &val, long)
	
	return error;
}

/*
 * Validate parameters and get old / set new parameters
 * for an quad-valued sysctl function.
 */
long
sysctl_quad (void *oldp, ulong *oldlenp, const void *newp, ulong newlen, llong *valp)
{
	long error = 0;
	
	SYSCTL_SCALAR_NEWPCHECK_TYP (newp, newlen, llong)
	SYSCTL_SCALAR_CORE_TYP (oldp, oldlenp, valp, llong)
	SYSCTL_SCALAR_NEWPCOP_TYP (newp, valp, llong)
	
	return error;
}

/*
 * As above, but read-only.
 */
long
sysctl_rdquad (void *oldp, ulong *oldlenp, const void *newp, llong val)
{
	long error = 0;
	
	if (newp)
		return EPERM;
	
	SYSCTL_SCALAR_CORE_TYP (oldp, oldlenp, &val, llong)
	
	return error;
}

/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
long
sysctl_string (void *oldp, ulong *oldlenp, const void *newp, ulong newlen, char *str, long maxlen)
{
	long error;
	
	if (newp && newlen >= maxlen)
		return EINVAL;
	
	error = SYSCTL_STRING_CORE (oldp, oldlenp, str);
	if (error == 0 && newp)
	{
		error = copyin (newp, str, newlen);
		str[newlen] = 0;
	}
	
	return error;
}

/*
 * As above, but read-only.
 */
long
sysctl_rdstring (void *oldp, ulong *oldlenp, const void *newp, const char *str)
{
	if (newp)
		return EPERM;
	
	return SYSCTL_STRING_CORE (oldp, oldlenp, str);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
long
sysctl_struct (void *oldp, ulong *oldlenp, const void *newp, ulong newlen, void *sp, long len)
{
	long error = 0;
	
	SYSCTL_SCALAR_NEWPCHECK_LEN (newp, newlen, len)
	SYSCTL_SCALAR_CORE_LEN (oldp, oldlenp, sp, len)
	SYSCTL_SCALAR_NEWPCOP_LEN (newp, sp, len)
	
	return error;
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
long
sysctl_rdstruct (void *oldp, ulong *oldlenp, const void *newp, const void *sp, long len)
{
	long error = 0;
	
	if (newp)
		return EPERM;
	
	SYSCTL_SCALAR_CORE_LEN (oldp, oldlenp, sp, len)
	
	return error;
}

/*
 * As above, but can return a truncated result.
 */
long
sysctl_rdminstruct (void *oldp, ulong *oldlenp, const void *newp, const void *sp, long len)
{
	long error = 0;
	
	if (newp)
		return EPERM;
	
	len = MIN (*oldlenp, len);
	SYSCTL_SCALAR_CORE_LEN (oldp, oldlenp, sp, len)
	
	return error;
}
