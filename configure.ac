# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: configure.ac  0.20 2016-12-04 08:10:42 niko $
#
# Author:   Niko Kiiskinen
# Modified: December 27, 2016
# This file defines the common autoconf macros for LOLFS
#
#
AC_PREREQ([2.69])
AC_INIT([lolfs], [0.20], [lolfs.bugs@gmail.com], [], [https://nkiiskin.github.io/lolfs])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_TESTDIR([tests])
# Use kernel-like CC instead of the usual verbose insanity.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_DEFINE([HAVE_CONFIG_H], [1], [Define that we have a config])
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_SED
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

#AC_CHECK_PROG(POD2MAN, pod2man, yes, no)
#AM_CONDITIONAL(POD, test x$ac_cv_prog_POD2MAN = xyes)
#AC_VISIBILITY
# Use a modern C dialect.
#AX_CHECK_COMPILE_FLAG(-std=gnu11, [CFLAGS="$CFLAGS -std=gnu11"],
# AX_CHECK_COMPILE_FLAG(-std=gnu99, [CFLAGS="$CFLAGS -std=gnu99"]))
# Enable warnings.
#AC_C_TRY_FLAG([-Wall])

m4_include([m4/m4_ax_compile_check_sizeof.m4])
## m4_include([m4/m4_ax_gnu_autotest.m4])
## m4_include([m4_ax_generate_changelog.m4])
LT_PREREQ([2.4.2])
LT_INIT
#LT_OBJDIR
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])
## AC_SUBST([tests/atlocal])
AC_CONFIG_FILES([Makefile lib/Makefile bin/Makefile src/Makefile include/Makefile man/Makefile])
# Checks for header files.
AC_CHECK_HEADER([linux/fs.h], [AC_DEFINE([HAVE_LINUX_FS_H], [1], [Building with raw device support])], [AC_MSG_WARN([Can not find <linux/fs.h> Unsupported build])])

AC_CHECK_HEADER([sys/ioctl.h], [AC_DEFINE([HAVE_SYS_IOCTL_H], [1], [ioctl support?])], [AC_MSG_WARN([Can not find <sys/ioctl.h> Unsupported build])])

AC_CHECK_HEADER([limits.h], [AC_DEFINE([HAVE_LIMITS_H], [1], [Need this for LONG_MAX and LONG_MIN])], [])
AC_CHECK_HEADER([unistd.h], [AC_DEFINE([HAVE_UNISTD_H], [1], [do we have unistd.h?])], [])
AC_CHECK_HEADER([ctype.h], [AC_DEFINE([HAVE_CTYPE_H], [1], [Need ctype for toupper])], [])
AC_CHECK_HEADERS([errno.h sys/types.h sys/stat.h time.h stdio.h stdlib.h string.h fcntl.h signal.h])

AX_COMPILE_CHECK_SIZEOF(char)
AX_COMPILE_CHECK_SIZEOF(short)
AX_COMPILE_CHECK_SIZEOF(int)
AX_COMPILE_CHECK_SIZEOF(long)
AX_COMPILE_CHECK_SIZEOF(unsigned char *)
AX_COMPILE_CHECK_SIZEOF(void *)
AX_COMPILE_CHECK_SIZEOF(size_t, $headers)
AX_COMPILE_CHECK_SIZEOF(ssize_t, $headers)
AX_COMPILE_CHECK_SIZEOF(off_t, $headers)

##AC_CHECK_FILE([/sbin/ldconfig], [], [AC_MSG_WARN([Can not find ldconfig script])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
# Checks for library functions.
AC_CHECK_FUNCS([memset strtol fsetpos sigaction sigemptyset sigfillset puts sprintf strcat ioctl toupper putchar])

## AX_GNU_AUTOTEST([tests], [testsuite], [Makefile], [yes], [no])
# Initialize the test suite.
##AC_CONFIG_TESTDIR(tests)
AC_CONFIG_FILES([tests/Makefile tests/atlocal])
AM_MISSING_PROG([AUTOM4TE], [autom4te])
##AX_GENERATE_CHANGELOG
AC_OUTPUT
