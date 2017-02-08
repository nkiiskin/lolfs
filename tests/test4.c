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
 $Id: test4.c, v0.30 2017/01/29 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lolfs.h"
#include "../include/lol_internal.h"
/* ************************************************************************ */
/* LoL testsuite: test #4 (Using 'lol' command to try miscallaneus tests)   */
/* ************************************************************************ */
char *args1[] = {
  "lol",
  "cp",
  "../../1.db:/ChangeLog",
  "../../1.db:/TODO",
  "../..",
  NULL
};
char *args2[] = {
  "lol",
  "cp",
  "../../1.db:/README.md",
  "../../1.db:/INSTALL",
  "../../1.db:/TODO",
  "../../1.db:/AUTHORS",
  "../../1.db:/aclocal.m4",
  "../../1.db:/COPYING",
  "../../test1.c",
  "../../test2.c",
  "../../../ChangeLog",
  "../../2.db",
  NULL
};
char *args3[] = {
  "lol",
  "cp",
  "../../1.db:/COPYING",
  "../../2.db:/2.db",
  NULL
};

// This test SHOULD fail!
char *args4[] = {
  "lol",
  "cp",
  "../../2.db:/2.db",
  "../..",
  NULL
};


int main (int argc, char* argv[])
{
  const char *me = argv[0];
  int rv;

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

  puts ("Copying 2 files from the container to tests/ directory");
  rv = lol_cmd (5, args1);
  if (!(rv)) {
    puts ("Succes: copied 2 files from the container");
  }
  else {
    puts("failed");
    return -1;
  }


  puts ("Copying files from the container 1 to 2");
  rv = lol_cmd (12, args2);
  if (!(rv)) {
    puts ("Succes: copied 11 files to container 2");
  }
  else {
    puts("failed");
    return -1;
  }

  puts ("Copying between containers and renaming");
  rv = lol_cmd (4, args3);
  if (!(rv)) {
    puts ("Succes: copied and renamed a file");
  }
  else {
    puts("failed");
    return -1;
  }

  puts ("Trying to generate an error");
  rv = lol_cmd (4, args4);
  if (rv) {
    puts ("Succes: test failed as it should have");
  }
  else {
    puts("failed, test passed, which was not supposed to happen");
    return -1;
  }

  puts("Success, all tests of test 4 succeed!");
  return 0;
}
/* ************************************************************************ */
