.TH Dcntl 2 "MiNT Programmer's Manual" "Version 1.08" "Aug. 5, 1993"
.SH NAME
Dcntl \- perform a file system specific operation on a file or directory
.SH SYNOPSIS
.nf
#include <filesys.h>

LONG Dcntl( WORD cmd, char *name, LONG arg);
.fi
.SH DESCRIPTION
.I Dcntl
performs a file system specific command, given by
.IR cmd ,
upon the file or directory specified by
.IR name .
The exact nature of the operation performed depends upon the file
system on which
.I name
resides. The interpretation of the third argument
.I arg
depends upon the specific command.
.PP
The only built-in file systems which support
.I Dcntl
operations are the device file system U:\\DEV and the unified
file system U:\\.

.PP
U:\\ understands the following commands:

.IP FS_INSTALL
Installs a new file system.
.I name
must be U:\\, and
.I arg
is a pointer to an fs_descr structure, which has the following format:
.nf

struct fs_descr
{
/* points to the file system structure for this file system */
	FILESYS *file_system;
/* device number for a particular mount point; filled in when an FS_MOUNT
 * Dcntl call is made
 */
	short dev_no;
/* "flags" may be used by the file system for whatever it wants */
	long flags;
/* the next 4 long words are reserved by the kernel */
	long reserved[4];
};


.fi
If the FS_INSTALL call is successful, a pointer to a kernel
information structure (struct kerinfo) is returned; otherwise, a
NULL pointer is returned. Note that even if the call is successful and
the file system is installed, it is still not accessible until it has
been given a name with FS_MOUNT.
.IP FS_MOUNT
Mounts an already existing file system into drive U:\\.
.I arg
is a pointer to an fs_descr structure describing the file system,
and
.I name
gives the name under which the file system may be accessed. After the
mount, the file systems "dev_no" field will be updated to reflect the
new, kernel-assigned device number for this instance of the file system.
This allows file systems to be mounted multiple times. On success, the
(positive) device number is returned; on failure, a negative error
number is returned.

.IP FS_UNMOUNT
Removes a mount created by FS_MOUNT.
.I arg
points to the fs_descr structure describing the file system, and
.I name
is the name under which the system is mounted.

.IP FS_UNINSTALL
Removes a file system completely from the kernel. This call is allowed
only if no remaining names exist for the file system, i.e. if all
mount points have been removed by FS_UNMOUNT>
.I arg
points to the fs_descr structure describing the file system, and
.I name
must be U:\\.

.PP
U:\\DEV understands the following commands:

.IP DEV_INSTALL
Installs a new device driver whose name is given by
.I name
(which must be the complete path and file name, e.g.
U:\\DEV\\FOO). The device may be accessed only through
GEMDOS file operations; the BIOS will not be aware of it.
.I arg
is a pointer to a device descriptor structure:
.nf

struct dev_descr {
 /* pointer to device driver structure */
	DEVDRV	*driver;
 /* placed in the "aux" field of file cookies */
	short	dinfo;
 /* either 0 or O_TTY */
	short	flags;
 /* if flags&O_TTY, this points to the tty structure associated
  * with the device
  */
	struct tty *tty;
 /* reserved for future expansion -- set to 0 */
	long	reserved[4];
};
.fi

If the attempt to install the device is successful,
.I Dcntl
will return a pointer to a structure with type "struct kerinfo"
that describes the kernel (see the documentation for loadable
file systems for more information on this structure). This
structure is in protected memory and can be accessed only
in supervisor mode. Moreover, the structure is read only;
attempts to write to it will produce unpredictable errors.
If there is not enough memory to install the new device,
a null pointer will be returned.
.IP DEV_NEWTTY
Installs a driver for a new BIOS terminal device whose name is
.I name
(this must be the full path name, e.g.
U:\\DEV\\BAR).
.I arg
is the BIOS device number of this device. Note that the BIOS
must have been told about the device already via the
.I Bconmap
system call or some similar mechanism; otherwise, any attempt
to access the file will result in an unknown device error
(EUNDEV). This command simply informs the kernel about a BIOS
device that already exists, unlike DEV_INSTALL which actually
creates a new device.

.IP DEV_NEWBIOS
Installs a driver for a new BIOS non-terminal device whose name is
.I name
(this must be the full path name, e.g.
U:\\DEV\\BAR).
.I arg
is the BIOS device number of this device. Note that the BIOS
must have been told about the device already via the
.I Bconmap
system call or some similar mechanism; otherwise, any attempt
to access the file will result in an unknown device error
(EUNDEV). Like DEV_NEWTTY, this command informs the kernel
of the existence of a BIOS device. The difference between
the two commands is that DEV_NEWTTY should be used for devices
which may be used as terminal devices (e.g. serial lines),
whereas DEV_NEWBIOS is useful for devices for which data must
be always transmitted "raw" (e.g. a SCSI tape device, or perhaps
a LAN device).
 
.SH RETURNS
The value returned depends on the specific operation requested and
the file system involved. Generally, a 0 or positive return value
should mean success, and a negative one failure. An exception is
the value returned by DEV_INSTALL, which is either a pointer
to a kernel information structure, or null on failure.

.SH "SEE ALSO"
.IR Bconmap (2),
.IR Fcntl (2)
