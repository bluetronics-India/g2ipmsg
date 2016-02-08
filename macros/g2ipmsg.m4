dnl Process this file with autoconf to produce a configure script.

dnl from ac-archives
dnl
dnl In Autoconf 2.60, ${datadir} refers to ${datarootdir}, which in turn
dnl refers to ${prefix}.  Thus we have to use `eval' twice.
dnl
AC_DEFUN([AC_DEFINE_DIR], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix="$prefix"
  ac_define_dir=`eval echo $2`
  ac_define_dir=`eval echo $ac_define_dir`
  AC_MSG_CHECKING(directory configuration for $1)
  AC_MSG_RESULT($ac_define_dir)
  $1="$ac_define_dir"
  AC_SUBST($1)
  ifelse($3, ,
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir"),
    AC_DEFINE_UNQUOTED($1, "$ac_define_dir", $3))
])

