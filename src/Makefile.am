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
##AM_LDFLAGS = -L$(top_builddir)/lib

proj_heads = \
	$(top_builddir)/include/lolfs.h \
	$(top_builddir)/include/lol_file.h \
	$(top_builddir)/include/lol_internal.h

proj_lib = $(top_builddir)/lib/liblolfs.la
noinst_HEADERS = $(proj_heads)

bin_PROGRAMS =  \
	lol     \
	mkfs.lolfs


#lol_cat_SOURCES = lol_cat.c $(proj_heads)

# List all additionally required libraries
#lol_cat_LDADD = $(proj_lib)
## $(LOL_CATLIB_LIBS)


# List all dependency libraries
# similar to LDADD, but without -l, -L flags.
#lol_cat_DEPENDENCIES = $(proj_lib)
## $(LOL_CATLIB_DEPENDENCIES)

# The -rpath flag is used by libtool to make sure that the shared
# library is found (in the lib install directory) when we are using
# dynamic libraries.
#lol_cat_LDFLAGS = -rpath $(libdir)

#lol_cp_SOURCES      = lol_cp.c $(proj_heads)
#lol_cp_LDADD        = $(proj_lib)
#lol_cp_DEPENDENCIES = $(proj_lib)
#lol_cp_LDFLAGS      = -rpath $(libdir)

#lol_df_SOURCES      = lol_df.c $(proj_heads)
#lol_df_LDADD        = $(proj_lib)
#lol_df_DEPENDENCIES = $(proj_lib)
#lol_df_LDFLAGS      = -rpath $(libdir)

#lol_ls_SOURCES      = lol_ls.c $(proj_heads)
#lol_ls_LDADD        = $(proj_lib)
#lol_ls_DEPENDENCIES = $(proj_lib)
#lol_ls_LDFLAGS      = -rpath $(libdir)

#lol_rm_SOURCES      = lol_rm.c $(proj_heads)
#lol_rm_LDADD        = $(proj_lib)
#lol_rm_DEPENDENCIES = $(proj_lib)
#lol_rm_LDFLAGS      = -rpath $(libdir)

lol_SOURCES      = lol.c $(proj_heads)
lol_LDADD        = $(proj_lib)
lol_DEPENDENCIES = $(proj_lib)
lol_LDFLAGS      = -rpath $(libdir)

mkfs_lolfs_SOURCES      = mkfs.lolfs.c $(proj_heads)
mkfs_lolfs_LDADD        = $(proj_lib)
mkfs_lolfs_DEPENDENCIES = $(proj_lib)
mkfs_lolfs_LDFLAGS      = -rpath $(libdir)
