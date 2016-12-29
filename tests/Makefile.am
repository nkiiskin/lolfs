# Test Makefile.am
# GNU Autotest rules
# the make rules provided here. Substitute this in a separate line.
#AX_GNU_AUTOTEST_RULES:
#	@AX_GNU_AUTOTEST_DEFAULT@
#
# the list of files to be added to DISTCLEANFILES
#AX_GNU_AUTOTEST_DCLEAN
#
# the list of files to be added to EXTRA_DIST
#AX_GNU_AUTOTEST_DIST: testsuite
#
# includes all other substitutions and uses them in a "default" way.
#AX_GNU_AUTOTEST_DEFAULT: @AX_GNU_AUTOTEST_DEFAULT@
#

AUTOMAKE_OPTIONS = foreign
DEFAULT_INCLUDES = -I$(top_builddir)/include
AM_CFLAGS = -O2 -Wall -I$(top_builddir)/include @AM_CFLAGS@
AM_LDFLAGS = -L$(top_builddir)/lib
check_PROGRAMS = foo

proj_heads = \
	$(top_builddir)/include/lol_config.h \
	$(top_builddir)/include/lolfs.h \
	$(top_builddir)/include/lol_file.h \
	$(top_builddir)/include/lol_internal.h

proj_lib = $(top_builddir)/lib/liblolfs.la
## -L$(top_builddir)/lib -llolfs $(top_builddir)/lib/liblolfs.la

noinst_HEADERS   = $(proj_heads)
foo_DEPENDENCIES = $(proj_lib)
foo_SOURCES      = $(top_builddir)/tests/foo.c $(proj_heads)
foo_LDADD        = $(proj_lib)
foo_LDFLAGS      = -L$(top_builddir)/lib
## -rpath $(libdir)

## LDADD = -L$(top_builddir)/lib -llolfs $(top_builddir)/lib/liblolfs.la
## foo_LDFLAGS = -L$(top_builddir)/lib
## foo_LDADD = $(LDADD)

## @PTHREAD_LIBS@
EXTRA_DIST = $(TESTSUITE_AT) testsuite.at testsuite package.m4 \
	atlocal.in fail.sh success.sh
DISTCLEANFILES       = atconfig $(check_SCRIPTS)
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE) package.m4

#dist-hook:
#	mkdir $(distdir)/vt.in
#	cp -p $(srcdir)/vt.in/* $(distdir)/vt.in
#	mkdir $(distdir)/vt.out
#	cp -p $(srcdir)/vt.out/* $(distdir)/vt.out
#

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

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): package.m4 testsuite.at $(TESTSUITE_AT)
	$(AUTOTEST) -I $(srcdir) $@.at -o $@.tmp
	mv $@.tmp $@

TESTSUITE_AT =
