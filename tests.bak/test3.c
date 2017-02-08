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
 $Id: test3.c, v0.30 2016/12/27 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/lolfs.h"
#include "../include/lol_internal.h"
/* ************************************************************************ */
/* LoL testsuite: test #3 (Modify a file inside a container)                */
/* ************************************************************************ */
char lolfile[] = "../../1.db:/ChangeLog";
char newline[] = "\nJust a wew line in ChangeLog... testing\n";

int main (int argc, char* argv[])
{
  lol_FILE *fp;
  const char *me = argv[0];
  int ret = -1;

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
  fp = lol_fopen(lolfile, "a");
  if (!(fp)) {
     lol_error("cannot open file %s\n", lolfile);
     return -1;
  }
  if ((lol_fwrite((char *)newline, strlen(newline), 1, fp)) != 1) {
       lol_error("lol_fwrite: cannot write to file \'%s\'\n", lolfile);
       goto error;
  }
  else {
    printf("Success, modified file \'%s\'\n", lolfile);
    ret = 0;
  }

 error:
  lol_fclose(fp);
  return ret;
}
/* ************************************************************************ */
