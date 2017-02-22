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
 $Id: test5.c, v0.40 2017/01/29 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lolfs.h"
#include "../include/lol_internal.h"
/* ************************************************************************ */
/* LoL testsuite: test #5 (Using 'lol' command to try miscallaneus tests)   */
/* ************************************************************************ */
char *args1[] = {
  "lol",
  "cp",
  "../../1.db:/ChangeLog",
  "../../2.db:/2.db",
  NULL
};
char *args2[] = {
  "lol",
  "cp",
  "../../2.db:/2.db",
  "../../2.db:/3.db",
  NULL
};
// This test SHOULD fail!
char *args3[] = {
  "lol",
  "cp",
  "../../2.db:/2.db",
  "../..",
  NULL
};
char *args4[] = {
  "lol",
  "cp",
  "../../2.db:/2.db",
  "../../..",
  NULL
};
// This test SHOULD fail!
char *args5[] = {
  "lol",
  "cp",
  "../../../2.db",
  "../..",
  NULL
};
char *args6[] = {
  "lol",
  "rm",
  "../../2.db:/2.db",
  NULL
};
char *args7[] = {
  "lol",
  "cp",
  "../../../2.db",
  "../../2.db",
  NULL
};


int main (int argc, char* argv[])
{
  const char *me = argv[0];
  int rv;
  int ret = 0;

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

  puts ("Copying 1 file from the container 1 to 2");
  rv = lol_cmd (4, args1);
  if (!(rv)) {
    puts ("Succes");
  }
  else {
    puts("ERROR: bundle 5, test 1 failed");
    ret = -1;
  }

  puts ("Copying 1 file from the container 2 to 2 (renaming)");
  rv = lol_cmd (4, args2);
  if (!(rv)) {
    puts ("Succes: copied 1 file from container 2 to 2");
  }
  else {
    puts("ERROR: bundle 5, test 2 failed");
    ret = -1;
  }

  puts ("Bundle 5, test 3: Trying to generate an error");
  rv = lol_cmd (4, args3);
  if (rv) {
    puts ("Succes: error as expected!");
  }
  else {
    puts("ERROR: bundle 5, test 3 failed, did NOT generate error!");
    ret = -1;
  }

  puts ("Copying 1 file from container 2 to parent dir");
  rv = lol_cmd (4, args4);
  if (!(rv)) {
    puts ("Succes: copied 1 file from container 2");
  }
  else {
    puts("ERROR: bundle 5, test 4: failed to copy 1 file from container 2");
    ret = -1;
  }

  puts ("Bundle 5, test 5: Trying to generate an error");
  rv = lol_cmd (4, args5);
  if (rv) {
    puts ("Succes: error as expected!");
  }
  else {
    puts("ERROR: bundle 5, test 5 failed, did NOT generate error!");
    ret = -1;
  }

  puts ("Bundle 5, test 6: deleting a file from container 2");
  rv = lol_cmd (3, args6);
  if (!(rv)) {
    puts ("Succes: deleted 1 file from container 2");
  }
  else {
    puts("ERROR: bundle 5, test 6: failed to delete 1 file from container 2");
    ret = -1;
  }

  puts ("Bundle 5, test 7: copying 1 file from parent dir to same name as container");
  rv = lol_cmd (4, args7);
  if (!(rv)) {
    puts ("Succes, bundle 5, test 7");
  }
  else {
    puts("ERROR: bundle 5, test 7 failed, could not copy 1 file.");
    ret = -1;
  }

  if (!(ret))
   puts("Success, all 7 tests of test bundle 5 succeed!");
  else
   puts("Some tests of test bundle 5 failed!");

  return ret;
}
/* ************************************************************************ */
