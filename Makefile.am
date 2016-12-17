# Copyright (C) 2016 Niko Kiiskinen.
# All Rights Reserved.
# This file is distributed under the GPLv2 License.
#
## $Id: Makefile.am main 0.13 2016-11-07 01:28:49Z niko $
#
# Author: Niko Kiiskinen 2016-11-07
#
# This file defines the common automake macros for LOLFS build
#
#
ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
DIST_SUBDIRS = lib bin src include man
SUBDIRS = lib bin src include man
EXTRA_DIST = README.md gpl3.txt
DEFS = @DEFS@

dist-hook:
	chmod u+w $(distdir)/README.md
	sed -i "/Distribution date/c\Distribution date: `date`" README.md
	rm -f *~ bin/*~ src/*.o src/*~ lib/*.o lib/*.so lib/*~ man/*~
	mkdir -p $(distdir)/m4

install-data-local:
	@mkdir -p ${datarootdir}/man/man3

install-exec-local:
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
