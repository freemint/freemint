dnl ---------------------------------------------------------------------------
dnl CF_POSIX_VDISABLE version: 6 updated: 2010/10/23 15:52:32
dnl -----------------
dnl Special test to workaround gcc 2.6.2, which cannot parse C-preprocessor
dnl conditionals.
dnl
dnl AC_CHECK_HEADERS(termios.h unistd.h)
dnl AC_CHECK_FUNCS(tcgetattr)
dnl
AC_DEFUN([CF_POSIX_VDISABLE],
[
AC_MSG_CHECKING(if POSIX VDISABLE symbol should be used)
AH_TEMPLATE([HAVE_POSIX_VDISABLE], [define this if the POSIX VDISABLE symbol is defined, not equal to -1])
AC_CACHE_VAL(cf_cv_posix_vdisable,[
	AC_TRY_RUN([
#if defined(HAVE_TERMIOS_H) && defined(HAVE_TCGETATTR)
#include <termios.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(_POSIX_VDISABLE)
int main() { ${cf_cv_main_return:-return}(_POSIX_VDISABLE == -1); }
#endif],
	[cf_cv_posix_vdisable=yes],
	[cf_cv_posix_vdisable=no],
	[AC_TRY_COMPILE([
#if defined(HAVE_TERMIOS_H) && defined(HAVE_TCGETATTR)
#include <termios.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif],[
#if defined(_POSIX_VDISABLE) && (_POSIX_VDISABLE != -1)
int temp = _POSIX_VDISABLE;
#else
this did not work
#endif],
	[cf_cv_posix_vdisable=yes],
	[cf_cv_posix_vdisable=no],
	)])
])
AC_MSG_RESULT($cf_cv_posix_vdisable)
test $cf_cv_posix_vdisable = yes && AC_DEFINE(HAVE_POSIX_VDISABLE)
])dnl
