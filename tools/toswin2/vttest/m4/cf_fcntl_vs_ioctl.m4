dnl ---------------------------------------------------------------------------
dnl CF_FCNTL_VS_IOCTL version: 3 updated: 2004/08/03 20:29:33
dnl -----------------
dnl Test if we have a usable ioctl with FIONREAD, or if fcntl.h is preferred.
AC_DEFUN([CF_FCNTL_VS_IOCTL],
[
AC_MSG_CHECKING(if we may use FIONREAD)
AH_TEMPLATE([USE_FIONREAD], [define this if we can use ioctl(,FIONREAD,)])
AC_CACHE_VAL(cf_cv_use_fionread,[
	AC_TRY_COMPILE([
#if HAVE_SYS_FILIO_H
#  include <sys/filio.h>	/* FIONREAD */
#endif
	],[
int l1;
ioctl (0, FIONREAD, &l1);
	],
	[cf_cv_use_fionread=yes],
	[cf_cv_use_fionread=no])
])
AC_MSG_RESULT($cf_cv_use_fionread)
test $cf_cv_use_fionread = yes && AC_DEFINE(USE_FIONREAD)
])dnl
