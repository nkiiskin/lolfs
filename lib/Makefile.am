# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: Makefile.am library 0.13 2016-11-07 01:28:49Z niko $
#
# Author: Niko Kiiskinen 2016-11-07
#
# This file defines the common automake macros for LOLFS library
#
#
AUTOMAKE_OPTIONS = foreign
DEFAULT_INCLUDES = -I$(top_builddir)/include
AM_CFLAGS = -fPIC -Wall -I$(top_builddir)/include @AM_CFLAGS@
##AM_LDFLAGS = -lm


mandh = \
	$(top_builddir)/include/lolfs.h \
	$(top_builddir)/include/lol_file.h
addh  = $(top_builddir)/include/lol_internal.h

funcs = lol_ls.c  \
	lol_cp.c  \
	lol_rm.c  \
	lol_df.c  \
	lol_cat.c \
	lol_fs.c  \
	lol_cc.c

lib_LTLIBRARIES = liblolfs.la
##lib_LIBRARIES = liblolfs.a
##liblolfs_la_HEADERS = $(mandh)

liblolfs_la_SOURCES = \
	lolfs.c  \
	$(funcs) \
	$(mandh) \
	$(addh)

liblolfs_la_LDFLAGS = -lc -shared -Wl,-soname,liblolfs.so $(LT_LDFLAGS)

## -version-info 0:0:0

include_HEADERS = $(mandh)
noinst_HEADERS  = $(addh)

