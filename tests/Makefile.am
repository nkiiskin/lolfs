# Test Makefile.am
# GNU Autotest rules
# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: Makefile.am tests 0.40 2017-01-08 23:17:15Z niko $
#
# Author: Niko Kiiskinen 2016-11-07
#
# This file defines the rules for LOLFS testsuite
#
#
AUTOMAKE_OPTIONS = foreign
DEFAULT_INCLUDES = -I$(top_builddir)/include -I$(top_builddir)/tests/include
AM_CFLAGS = -O2 -Wall -I$(top_builddir)/include \
	-I$(top_builddir)/tests/include @AM_CFLAGS@
AM_LDFLAGS = -L$(top_builddir)/lib

testhdr = $(top_builddir)/tests/include/lol_tests.h

check_PROGRAMS = test1 test2 test3 test4 test5 test6

proj_heads = \
	$(top_builddir)/include/lol_config.h \
	$(top_builddir)/include/lolfs.h \
	$(top_builddir)/include/lol_file.h \
	$(top_builddir)/include/lol_internal.h

proj_lib = $(top_builddir)/lib/liblolfs.la
tst_libd = $(top_builddir)/tests
tst_lib = $(tst_libd)/libloltests.la
## -L$(top_builddir)/lib -llolfs $(top_builddir)/lib/liblolfs.la

noinst_HEADERS   = $(proj_heads) $(testhdr)

lib_LTLIBRARIES = libloltests.la
libloltests_la_SOURCES = $(top_builddir)/tests/src/testfuncs.c $(testhdr) $(proj_heads)

libloltests_la_LIBADD = $(proj_lib)
libloltests_la_LDFLAGS = -L$(top_builddir)/lib -lc -shared \
	-Wl,-soname,libloltests.so $(LT_LDFLAGS)

test1_DEPENDENCIES = $(proj_lib)
test1_SOURCES      = $(top_builddir)/tests/src/test1.c $(proj_heads)
test1_LDADD        = $(proj_lib)
test1_LDFLAGS      = -L$(top_builddir)/lib

test2_DEPENDENCIES = $(proj_lib)
test2_SOURCES      = $(top_builddir)/tests/src/test2.c $(proj_heads)
test2_LDADD        = $(proj_lib)
test2_LDFLAGS      = -L$(top_builddir)/lib

test3_DEPENDENCIES = $(proj_lib)
test3_SOURCES      = $(top_builddir)/tests/src/test3.c $(proj_heads)
test3_LDADD        = $(proj_lib)
test3_LDFLAGS      = -L$(top_builddir)/lib

test4_DEPENDENCIES = $(proj_lib)
test4_SOURCES      = $(top_builddir)/tests/src/test4.c $(proj_heads)
test4_LDADD        = $(proj_lib)
test4_LDFLAGS      = -L$(top_builddir)/lib

test5_DEPENDENCIES = $(proj_lib)
test5_SOURCES      = $(top_builddir)/tests/src/test5.c $(proj_heads)
test5_LDADD        = $(proj_lib)
test5_LDFLAGS      = -L$(top_builddir)/lib

test6_DEPENDENCIES = $(proj_lib)
test6_SOURCES      = $(top_builddir)/tests/src/test6.c $(proj_heads) $(testheads)
test6_LDADD        = $(proj_lib) $(tst_lib)
test6_LDFLAGS      = -L$(top_builddir)/lib -L$(tst_libd)

EXTRA_DIST = $(TESTSUITE_AT) testsuite.at testsuite package.m4 \
	atlocal.in fail.sh success.sh m4 m4/1.at m4/2.at m4/3.at m4/4.at \
	m4/5.at m4/6.at

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
	  echo 'm4_define([AT_PACKAGE_URL],       [@PACKAGE_URL@])'; \
	  echo 'm4_define([AT_TESTS], [$(TESTSUITE_AT)])'; \
	} >$(srcdir)/package.m4

TESTSUITE = $(srcdir)/testsuite
check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) $(TESTSUITE)

clean-local:
	[ ! -x $(TESTSUITE) ] || $(SHELL) $(TESTSUITE) --clean
	rm -f $(top_builddir)/tests/ChangeLog
	rm -f $(top_builddir)/tests/TODO $(top_builddir)/tests/foo
	rm -f $(top_builddir)/2.db $(top_builddir)/tests/1.db $(top_builddir)/tests/2.db
	rm -f $(top_builddir)/tests/*.o
	rm -f $(top_builddir)/tests/*~
	rm -f $(top_builddir)/tests/src/*~
	rm -f $(top_builddir)/tests/include/*~


AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 testsuite.at $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) $@.at -o $@.tmp
	mv $@.tmp $@

TESTSUITE_AT =
