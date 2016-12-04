# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: configure.ac  0.13 2016-12-04 08:10:42 niko $
#
# Author: Niko Kiiskinen 2016-12-04
#
# This file defines the common autoconf macros for LOLFS
#
#
AC_PREREQ([2.69])
AC_INIT([lolfs], [0.13], [nkiiskin@yahoo.com])
AM_INIT_AUTOMAKE([1.9 foreign])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_CONFIG_MACRO_DIR([m4])
LT_PREREQ([2.4.2])
LT_INIT
#LT_OBJDIR
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])
AC_CONFIG_FILES([Makefile lib/Makefile bin/Makefile src/Makefile include/Makefile man/Makefile])
# Checks for header files.
AC_CHECK_HEADER([linux/fs.h], [AC_DEFINE([HAVE_LINUX_FS_H], [1], [Building with raw device support])], [AC_MSG_WARN([Can not find <linux/fs.h> Unsupported build])])

AC_CHECK_HEADERS([errno.h sys/types.h sys/stat.h unistd.h time.h stdio.h stdlib.h string.h fcntl.h signal.h])
AC_CHECK_FILE([/sbin/ldconfig], [], [AC_MSG_WARN([Can not find ldconfig script])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
# Checks for library functions.
AC_CHECK_FUNCS([memset strtol fsetpos sigaction sigemptyset sigfillset puts])
AC_OUTPUT
