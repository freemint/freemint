.TH Fpipe 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fpipe \- create a pipe
.SH SYNOPSIS
.nf
WORD Fpipe( WORD usrh[2] );
.fi
.SH DESCRIPTION
.I Fpipe
creates a pipe that may be used for interprocess communication. If
it is successful, two GEMDOS handles are returned in usrh[0] and
usrh[1]. usrh[0] will contain a handle for the read end of the pipe,
(opened for reading only), and usrh[1] will contain a handle for
the write end of the pipe (opened for writing only).
.PP
The created pipe has the name "sys$pipe.xxx", where "xxx" is a
three digit integer.
.SH USAGE
Typically used by shells; the shell redirects its standard input (or
standard output) to the read (or write) end of the pipe using 
.I Fdup
and
.I Fforce
before launching a child; the child will then read from (or write to) the
pipe by default.
.SH RETURNS
0 if successful
.PP
ENHNDL if there are not 2 free handles to allocate for the pipe
.PP
ENSMEM if there is not enough free memory to create the pipe
.PP
EACCDN if too many pipes already exist in the system
.SH BUGS
There cannot be more than 999 open pipes in the system at one time.
