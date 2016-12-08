# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: Makefile.am 0.12 2016-11-07 01:28:49Z niko $
#
# Author: Niko Kiiskinen 2016-11-07
#
# This file defines the common automake macros for LOLFS tools
#
#
AUTOMAKE_OPTIONS = foreign
DEFAULT_INCLUDES = -I$(top_builddir)/include
AM_CFLAGS = -Wall -I$(top_builddir)/include @AM_CFLAGS@
AM_LDFLAGS = -L$(top_builddir)/lib

proj_heads = \
	$(top_builddir)/include/lolfs.h \
	$(top_builddir)/include/lol_file.h \
	$(top_builddir)/include/lol_internal.h

proj_lib = $(top_builddir)/lib/liblolfs.la
noinst_HEADERS = $(proj_heads)

bin_PROGRAMS =  \
	lol     \
	mkfs.lolfs \
	fsck.lolfs

lol_SOURCES      = $(top_builddir)/src/lol.c $(proj_heads)
lol_LDADD        = $(proj_lib)
lol_DEPENDENCIES = $(proj_lib)
lol_LDFLAGS      = -rpath $(libdir)

mkfs_lolfs_SOURCES      = $(top_builddir)/src/mkfs.lolfs.c $(proj_heads)
mkfs_lolfs_LDADD        = $(proj_lib)
mkfs_lolfs_DEPENDENCIES = $(proj_lib)
mkfs_lolfs_LDFLAGS      = -rpath $(libdir)

fsck_lolfs_SOURCES      = $(top_builddir)/src/fsck.lolfs.c $(proj_heads)
fsck_lolfs_LDADD        = $(proj_lib)
fsck_lolfs_DEPENDENCIES = $(proj_lib)
fsck_lolfs_LDFLAGS      = -rpath $(libdir)
