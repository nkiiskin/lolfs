/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
/*
 $Id: test2.c, v0.30 2016/12/27 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lolfs.h"
#include "../include/lol_internal.h"
/* ************************************************************************ */
/* LoL testsuite: test #2 (Copy files to container)                         */
/* ************************************************************************ */
char *args[] = {
  "cp",
  "../../../README.md",
  "../../../INSTALL",
  "../../../TODO",
  "../../../Makefile.am",
  "../../../AUTHORS",
  "../../../aclocal.m4",
  "../../../COPYING",
  "../../../Makefile.am",
  "../../test1.c",
  "../../test2.c",
  "../../../ChangeLog",
  "../../1.db",
  NULL
};
int main (int argc, char* argv[])
{
  const char *me = argv[0];
  // Process standard --help & --version options.
  if (argc > 1) {
    if (LOL_CHECK_HELP) {
        printf ("lol %s v%s. %s\n",
                me, lol_version, lol_copyright);
    } else {
       if (LOL_CHECK_VERSION) {
           printf ("lol %s v%s %s\n",
	         me, lol_version, lol_copyright);
       }
    }
  }
  if ((lol_cp (13, args))) {
      lol_error("Error, cannot copy files\n");
      return -1;
  }
  puts("Success, copied 11 files to container \'1.db\'");
  return 0;
}
/* ************************************************************************ */
