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
 $Id: test1.c, v0.30 2016/12/27 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lolfs.h>
#include <lol_internal.h>
/* ************************************************************************ */
/* LoL testsuite: test #1 (Create a container)                              */
/* ************************************************************************ */
int main (int argc, char* argv[])
{
  const char *me = argv[0];
  // Process standard --help & --version options.
  if (argc > 1) {
    if (LOL_CHECK_HELP) {
        printf ("%s v%s. %s\n",
                me, lol_version, lol_copyright);

    } else {
       if (LOL_CHECK_VERSION) {
         printf ("%s v%s %s\n",
	         me, lol_version, lol_copyright);
       }
    }
  }
  if ((lol_mkfs ("-s", "5M", 0, 0, "../../test.db"))) {
      lol_error("lol_mkfs: Error, cannot create container\n");
      return -1;
  }
  puts("Success, created container");
  return 0;
}
/* ************************************************************************ */
