.TH Fputchar 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fputchar \- write a character to a file
.SH SYNOPSIS
.nf
LONG Fputchar( WORD fh, LONG ch, WORD mode );
.fi
.SH DESCRIPTION
.I Fputchar
outputs a character to the GEMDOS file whose handle is
.IR fh .
The parameter
.I mode
has an effect only if the open file is a terminal or pseudo-terminal,
in which case the bits of
.I mode
have the following meanings:
.IP 0x0001
Cooked mode; special control characters (control-C and control-Z) are
checked for and interpreted if found (they cause SIGINT and SIGTSTP,
respectively, to be raised); also, flow control with control-S and
control-Q is activated.
.PP
If the file receiving output is a pseudo-terminal, then all 4 bytes of
.I ch
are recorded in the write, and may be retreived by a
.I Fputchar
call on the other side of the pseudo-terminal; this allows programs to
pass simulated BIOS scan codes and shift key status through the
pseudo-terminal.
.PP
If the file receiving output is not a terminal, then only the low order
byte of
.I ch
is written to the file.
.SH RETURNS
4 (the number of bytes of data transferred) if the write was to a terminal
.PP
1 if the write was not to a terminal and was successful
.PP
0 if the bytes could not be output (for example, because of flow control)
.PP
EIHNDL if
.I fh
is not a valid handle for an open file.
.PP
A (long) negative BIOS error code if an error occured during physical I/O.
.SH "SEE ALSO"
.IR Bconout (2),
.IR Cauxout (2),
.I Cconout(2),
.IR Crawio (2),
.IR Fgetchar (2),
.IR Fwrite (2)
