# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: Makefile.am main 0.30 2016-11-07 01:28:49Z niko $
#
# Author: Niko Kiiskinen 2016-11-07
#
# This file defines the common automake macros for LOLFS build
#
#
# ACLOCAL_AMFLAGS will be deprecated,
# We include our macros from configure.ac
ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
DIST_SUBDIRS = lib bin src include man tests
SUBDIRS = lib bin src include man tests
EXTRA_DIST = README.md gpl3.txt
DEFS = @DEFS@

# CHANGELOG_START = 0.30^
# CHANGELOG_START = 633f37efc5743504fa094fc91f18de0f2df7f3b9
# CHANGELOG_GIT_FLAGS = --pretty=format:"%h %ad | %s%d (nk)" --graph --date=short
# CHANGELOG_GIT_DIR = $(top_srcdir)/.git
# AX_GNU_AUTOTEST_DEFAULT: @AX_GNU_AUTOTEST_DEFAULT@
# @GENERATE_CHANGELOG_RULES@

# clean:
#	rm -f tests/1.db tests/2.db tests/ChangeLog tests/TODO

all-local:
	@echo '******************************'
	@echo '*                            *'
	@echo '*         Success!           *'
	@echo '*                            *'
	@echo '******************************'
	@echo ''

dist-hook:
	chmod u+w $(distdir)/README.md
	sed -i "/Distribution date/c\Distribution date: `date`" README.md
	rm -f *~ bin/*~ src/*.o src/*~ lib/*.o lib/*.so lib/*~ man/*~
	rm -fr autom4te.cache
	mkdir -p $(distdir)/m4
	mkdir -p $(distdir)/tests/m4
	cp -f $(srcdir)/tests/m4/* $(distdir)/tests/m4

## dist-ChangeLog

install-data-local:
	@mkdir -p ${datarootdir}/man/man3

# Patch lol_file.h since local install won't have LOL_FILENAME_MAX definition
# (It is defined while making the lib & bins and the struct lol_FILE is
#  meant to have only private members).
install-exec-local:
	@cd $(DESTDIR)$(includedir) && \
	sed -i "s/#include <lol_config.h>//;" lol_file.h && \
	sed -i "s/#include \"..\/config.h\"//;" lol_file.h && \
	sed -i "s/name\[LOL_FILENAME_MAX\]/\*name/;" lol_file.h && \
	sed -i "s/file\[LOL_FILENAME_MAX\]/\*file/;" lol_file.h && \
	sed -i "s/cont\[LOL_DEVICE_MAX\]/\*cont/;" lol_file.h
	@echo '******************************'
	@echo '*                            *'
	@echo '* The manual has been instal-*'
	@echo '* led under:                 *'
	@echo '*' ${datarootdir}/man '      *'
	@echo '* To use it, add it to the   *'
	@echo '* MANPATH variable.          *'
	@echo '*                            *'
	@echo '* Thank you for using LOLFS! *'
	@echo '*                            *'
	@echo '******************************'
