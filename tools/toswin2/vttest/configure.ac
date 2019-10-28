dnl Process this file with 'autoconf' to produce a 'configure' script
AC_PREREQ(2.62)

AC_INIT([vttest],[20190710])

AC_CONFIG_SRCDIR([esc.c])
AC_CONFIG_HEADER(config.h:config.hin)
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE

AC_ARG_PROGRAM
AC_PROG_CC
CF_PROG_CC

AC_PROG_INSTALL
CF_MAKE_TAGS

CF_WITH_MAN2HTML
CF_DISABLE_ECHO
CF_WITH_WARNINGS
CF_GNU_SOURCE

###	extra things that we'll substitute in the makefile
AC_TYPE_SIGNAL
AC_STDC_HEADERS
AC_HAVE_HEADERS( \
fcntl.h \
ioctl.h \
sgtty.h \
stdlib.h \
string.h \
sys/filio.h \
sys/ioctl.h \
termio.h \
termios.h \
unistd.h \
)

AC_HAVE_FUNCS([alarm rdchk tcgetattr usleep])

###	special configuration tests
CF_POSIX_VDISABLE
CF_FCNTL_VS_IOCTL

###	debugging/testing

###	output makefile and config.h
AC_OUTPUT(Makefile)
