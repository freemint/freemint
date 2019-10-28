/* $Id: ttymodes.c,v 1.23 2018/07/26 00:38:24 tom Exp $ */

#include <vttest.h>
#include <ttymodes.h>
#include <esc.h>  /* inflush() */

#undef tabs

#ifdef TAB3
# define tabs TAB3
#else
# ifdef XTABS
#  define tabs XTABS
# else
#  ifdef OXTABS
#   define tabs OXTABS
#  else
#   define tabs 0
#  endif
# endif
#endif

static TTY old_modes, new_modes;
/* *INDENT-OFF* */
static struct {
  int name;
  int code;
} speeds[] = {
  {B0,           0},
  {B50,         50},
  {B75,         75},
  {B110,       110},
  {B134,       134},
  {B150,       150},
  {B200,       200},
  {B300,       300},
  {B600,       600},
  {B1200,     1200},
  {B1800,     1800},
  {B2400,     2400},
  {B4800,     4800},
  {B9600,     9600},
#ifdef B19200
  {B19200,   19200},
#else
#ifdef EXTA
  {EXTA,     19200},
#endif
#endif
#ifdef B38400
  {B38400,   38400},
#else
#ifdef EXTB
  {EXTB,     38400},
#endif
#endif
#ifdef B57600
  {B57600,   57600},
#endif
#ifdef B115200
  {B115200, 115200},
#endif
};
/* *INDENT-ON* */

#if !USE_POSIX_TERMIOS && !USE_TERMIO && USE_SGTTY
static struct tchars old_tchars;
static struct ltchars old_ltchars;
#endif

#if USE_POSIX_TERMIOS || USE_TERMIO
static void
disable_control_chars(TTY * modes)
{
# if USE_POSIX_TERMIOS
  int n;
  int temp;
#   ifdef HAVE_POSIX_VDISABLE
  temp = _POSIX_VDISABLE;
#   else
  errno = 0;
  temp = fpathconf(0, _PC_VDISABLE);
  if (temp == -1) {
    if (errno != 0) {
      restore_ttymodes();
      fprintf(stderr, "Cannot disable special characters!\n");
      exit(EXIT_FAILURE);
    }
    temp = 0377;
  }
#   endif
  for (n = 0; n < NCCS; n++)
    if (n != VINTR)
      modes->c_cc[n] = (unsigned char) temp;
# else /* USE_TERMIO */
#   ifdef       VSWTCH
  modes->c_cc[VSWTCH] = VDISABLE;
#   endif
  modes->c_cc[VSUSP] = VDISABLE;
#   if defined (VDSUSP) && defined(NCCS) && VDSUSP < NCCS
  modes->c_cc[VDSUSP] = VDISABLE;
#   endif
  modes->c_cc[VSTART] = VDISABLE;
  modes->c_cc[VSTOP] = VDISABLE;
# endif
  modes->c_cc[VMIN] = 1;
  modes->c_cc[VTIME] = 0;
}
#endif

static void
set_ttymodes(TTY * modes)
{
# if USE_POSIX_TERMIOS
  fflush(stdout);
  tcsetattr(0, TCSAFLUSH, modes);
# else
#   if USE_TERMIO
  tcsetattr(0, TCSETAF, modes);
#   else /* USE_SGTTY */
  stty(0, modes);
#   endif
# endif
}

#ifndef log_ttymodes
void
log_ttymodes(char *file, int line)
{
  if (LOG_ENABLED)
    fprintf(log_fp, "%s @%d\n", file, line);
}
#endif

#ifndef dump_ttymodes
void
dump_ttymodes(char *tag, int flag)
{
#ifdef UNIX
  TTY tmp_modes;
  if (LOG_ENABLED) {
    fprintf(log_fp, "%s (%d):\n", tag, flag);
# if USE_POSIX_TERMIOS || USE_TERMIO
    tcgetattr(0, &tmp_modes);
    fprintf(log_fp, " iflag %08o\n", tmp_modes.c_iflag);
    fprintf(log_fp, " oflag %08o\n", tmp_modes.c_oflag);
    fprintf(log_fp, " lflag %08o\n", tmp_modes.c_lflag);
    if (!tmp_modes.c_lflag & ICANON) {
      fprintf(log_fp, " %d:min  =%d\n", VMIN, tmp_modes.c_cc[VMIN]);
      fprintf(log_fp, " %d:time =%d\n", VTIME, tmp_modes.c_cc[VTIME]);
    }
# else
    gtty(0, &tmp_modes);
    fprintf(log_fp, " flags %08o\n", tmp_modes.sg_flags);
# endif
  }
#endif
}
#endif

void
close_tty(void)
{
  restore_ttymodes();
}

void
init_ttymodes(int pn)
{
  dump_ttymodes("init_ttymodes", pn);

#ifdef UNIX
  if (pn == 0) {
    int speed_code, n;

    fflush(stdout);
# if USE_POSIX_TERMIOS || USE_TERMIO
    tcgetattr(0, &old_modes);
    speed_code = (int) cfgetospeed(&old_modes);
# else
#   if USE_SGTTY
    gtty(0, &old_modes);
    ioctl(0, TIOCGETC, &old_tchars);
    ioctl(0, TIOCGLTC, &old_ltchars);
    speed_code = old_modes.sg_ospeed;
#   endif
# endif
    new_modes = old_modes;
    for (n = 0; n < TABLESIZE(speeds); n++) {
      if (speeds[n].name == speed_code) {
        tty_speed = speeds[n].code;
        break;
      }
    }
  } else {
    putchar(BEL);
    fflush(stdout);
    inflush();
    new_modes = old_modes;
    sleep(2);
  }
# if USE_POSIX_TERMIOS || USE_TERMIO
  new_modes.c_iflag = BRKINT | old_modes.c_iflag;
  new_modes.c_oflag &= (unsigned) ~tabs;
# else /* USE_SGTTY */
  new_modes.sg_flags = old_modes.sg_flags | CBREAK;
# endif
  set_ttymodes(&new_modes);
# ifdef HAVE_FCNTL_H
#  ifndef O_NDELAY
#  define O_NDELAY O_NONBLOCK   /* O_NONBLOCK is POSIX */
#  endif
  {
  	const char *tty = ttyname(2);
  	if (tty == NULL)
  	  tty = "/dev/tty";
  close(2);
  if (open(tty, O_RDWR | O_NDELAY) < 0)
  {
  	int err = errno;
  	restore_ttymodes();
  	fflush(stdout);
  	dup(0);
  	fprintf(stderr, "can't open %s: %s\n", tty, strerror(err));
  	exit(1);
  }
  }
# endif
#endif /* UNIX */
  dump_ttymodes("...init_ttymodes", pn);
}

void
restore_ttymodes(void)
{
  dump_ttymodes("restore_ttymodes", -1);
#ifdef UNIX
  set_ttymodes(&old_modes);
#endif
  dump_ttymodes("...restore_ttymodes", -1);
}

void
set_tty_crmod(int enabled)
{
  dump_ttymodes("set_tty_crmod", enabled);
#ifdef UNIX
# if USE_POSIX_TERMIOS || USE_TERMIO
#   if USE_POSIX_TERMIOS
#     define MASK_CRMOD ((unsigned) (ICRNL | IXON))
#   else
#     define MASK_CRMOD ((unsigned) (ICRNL))
#   endif
  if (enabled) {
    new_modes.c_iflag |= MASK_CRMOD;
    new_modes.c_lflag |= ICANON;
    memcpy(new_modes.c_cc, old_modes.c_cc, sizeof(new_modes.c_cc));
  } else {
    new_modes.c_iflag &= ~MASK_CRMOD;
    new_modes.c_lflag &= (unsigned) ~ICANON;
    disable_control_chars(&new_modes);
  }
# else
  if (enabled)
    new_modes.sg_flags |= CRMOD;
  else
    new_modes.sg_flags &= ~CRMOD;
# endif
  set_ttymodes(&new_modes);
#endif
  dump_ttymodes("...set_tty_crmod", enabled);
}

void
set_tty_echo(int enabled)
{
  dump_ttymodes("set_tty_echo", enabled);
#ifdef UNIX
# if USE_POSIX_TERMIOS || USE_TERMIO
  if (enabled)
    new_modes.c_lflag |= ECHO;
  else
    new_modes.c_lflag &= (unsigned) ~ECHO;
# else /* USE_SGTTY */
  if (enabled)
    new_modes.sg_flags |= ECHO;
  else
    new_modes.sg_flags &= (unsigned) ~ECHO;
# endif
  set_ttymodes(&new_modes);
#endif
  dump_ttymodes("...set_tty_echo", enabled);
}

void
set_tty_raw(int enabled)
{
  dump_ttymodes("set_tty_raw", enabled);
  if (enabled) {
#ifdef UNIX
# if USE_POSIX_TERMIOS || USE_TERMIO
    /* *INDENT-EQLS* */
    new_modes.c_iflag      = 0;
    new_modes.c_lflag      = 0;
    new_modes.c_cc[VMIN]   = 1;
    new_modes.c_cc[VTIME]  = 0;
    set_ttymodes(&new_modes);
    set_tty_crmod(FALSE);
# else /* USE_SGTTY */
#   ifdef HAVE_FCNTL_H
    new_modes.sg_flags     &= ~CBREAK;
#   endif
    new_modes.sg_flags     |= RAW;
    set_ttymodes(&new_modes);
    {
      struct tchars tmp_tchars;
      struct ltchars tmp_ltchars;
      memset(&tmp_tchars, -1, sizeof(tmp_tchars));
      memset(&tmp_ltchars, -1, sizeof(tmp_ltchars));
      ioctl(0, TIOCSETC, &tmp_tchars);
      ioctl(0, TIOCSLTC, &tmp_ltchars);
    }
# endif
#endif
  } else {
#ifdef UNIX
# if USE_POSIX_TERMIOS || USE_TERMIO
    new_modes              = old_modes;
# else /* USE_SGTTY */
    new_modes.sg_flags     &= ~RAW;
#   ifdef HAVE_FCNTL_H
    new_modes.sg_flags     |= CBREAK;
#   endif
    ioctl(0, TIOCSETC, &old_tchars);
    ioctl(0, TIOCSLTC, &old_ltchars);
# endif
    set_ttymodes(&new_modes);
#endif
  }
  dump_ttymodes("...set_tty_raw", enabled);
}
