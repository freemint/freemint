/*
 * This file has been modified as part of the FreeMiNT project. See
 * the file Changes.MH for details and dates.
 */

# ifndef _tty_h
# define _tty_h

# include "mint/mint.h"
# include "mint/file.h"


# define CTRL(x)	((x) & 0x1f)

extern struct tty default_tty;

void tty_checkttou (FILEPTR *f, struct tty *tty);
long tty_read (FILEPTR *f, void *buf, long nbytes);
long tty_write (FILEPTR *f, const void *buf, long nbytes);
long tty_ioctl (FILEPTR *f, int mode, void *arg);
long tty_getchar (FILEPTR *f, int mode);
long tty_putchar (FILEPTR *f, long data, int mode);
long tty_select (FILEPTR *f, long proc, int mode);


# endif /* _tty_h */
