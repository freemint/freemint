.TH Fgetchar 2 "MiNT Programmer's Manual" "Version 1.0" "Feb. 1, 1993"
.SH NAME
Fgetchar \- read a character from a file
.SH SYNOPSIS
.nf
LONG Fgetchar( WORD fh, WORD mode );
.fi
.SH DESCRIPTION
.I Fgetchar
reads a character from the open file whose handle is
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
.IP 0x0002
Echo mode; characters read are echoed back to the terminal.
.PP
The ASCII value of the character read is put in the low byte of the long word
that is returned. If the file is a terminal or pseudo-terminal, the scan code
of the character pressed and (possibly) the shift key status are also returned
in the long word, just as with the BIOS
.I Bconin
system call.
.SH RETURNS
The character read, if successful.
.PP
0x0000FF1A if end of file is detected.
.PP
EIHNDL if
.I fh
is not a valid handle for an open file.
.SH "SEE ALSO"
.IR Bconin (2),
.IR Cconin (2),
.IR Cauxin (2),
.IR Fputchar (2),
.IR Fread (2)
