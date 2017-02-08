# Test Makefile.am
# GNU Autotest rules
# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: Makefile.am tests 0.30 2017-01-08 23:17:15Z niko $
#
# Author: Niko Kiiskinen 2016-11-07
#
# This file defines the rules for LOLFS testsuite
#
#
AUTOMAKE_OPTIONS = foreign
DEFAULT_INCLUDES = -I$(top_builddir)/include
AM_CFLAGS = -O2 -Wall -I$(top_builddir)/include @AM_CFLAGS@
AM_LDFLAGS = -L$(top_builddir)/lib
check_PROGRAMS = test1 test2 test3 test4

proj_heads = \
	$(top_builddir)/include/lol_config.h \
	$(top_builddir)/include/lolfs.h \
	$(top_builddir)/include/lol_file.h \
	$(top_builddir)/include/lol_internal.h

proj_lib = $(top_builddir)/lib/liblolfs.la
## -L$(top_builddir)/lib -llolfs $(top_builddir)/lib/liblolfs.la

noinst_HEADERS   = $(proj_heads)
test1_DEPENDENCIES = $(proj_lib)
test1_SOURCES      = $(top_builddir)/tests/test1.c $(proj_heads)
test1_LDADD        = $(proj_lib)
test1_LDFLAGS      = -L$(top_builddir)/lib

test2_DEPENDENCIES = $(proj_lib)
test2_SOURCES      = $(top_builddir)/tests/test2.c $(proj_heads)
test2_LDADD        = $(proj_lib)
test2_LDFLAGS      = -L$(top_builddir)/lib

test3_DEPENDENCIES = $(proj_lib)
test3_SOURCES      = $(top_builddir)/tests/test3.c $(proj_heads)
test3_LDADD        = $(proj_lib)
test3_LDFLAGS      = -L$(top_builddir)/lib

test4_DEPENDENCIES = $(proj_lib)
test4_SOURCES      = $(top_builddir)/tests/test4.c $(proj_heads)
test4_LDADD        = $(proj_lib)
test4_LDFLAGS      = -L$(top_builddir)/lib

EXTRA_DIST = $(TESTSUITE_AT) testsuite.at testsuite package.m4 \
	atlocal.in fail.sh success.sh m4 m4/1.at m4/2.at m4/3.at m4/4.at

DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE) package.m4


## ------------ ##
## package.m4.  ##
## ------------ ##

$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	{				      \
	  echo '# Signature of the current package.'; \
	  echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])'; \
	  echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])'; \
	  echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])'; \
	  echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])'; \
	  echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	  echo 'm4_define([AT_TESTS], [$(TESTSUITE_AT)])'; \
	} >$(srcdir)/package.m4

TESTSUITE = $(srcdir)/testsuite
check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE)

clean-local:
	[ ! -x $(TESTSUITE) ] || $(SHELL) $(TESTSUITE) --clean
	rm -f 1.db 2.db ChangeLog TODO

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 testsuite.at $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) $@.at -o $@.tmp
	mv $@.tmp $@

TESTSUITE_AT =
