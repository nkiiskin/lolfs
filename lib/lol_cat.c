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
 $Id: lol_cat.c, v0.20 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/

#include <stdio.h>
#include <string.h>

#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif

#define LOL_NOT_FOUND "lol %s: %s: No such file or directory\n"
#define LOL_BUG_FOUND "lol %s: %s: I/O error\n"
/* ****************************************************************** */
static const char params[] = "<container:/filename>";
static const char    hlp[] = "       Type: 'lol %s -h' for help.\n";
static const char*   lst[] =
{
  "  Example:\n",
  "          lol cat lol.db:/memo.txt",
  "          This writes the contents of the file \'memo.txt\'",
  "          which is inside a container file \'lol.db\' to",
  "          the standard output.\n",
  "          Type: 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int lol_cat (int argc, char *argv[]) {

  struct stat st;
  char ptr[4096];
  lol_FILE   *fp;
  char       *me;
  char    *lfile;
  FILE     *dest;
  size_t i, r, t;
  size_t v, size;
  int ret = -1;

  me = argv[0];
  // Process standard --help & --version options.
  if (argc == 2) {
    if (LOL_CHECK_HELP) {

        printf (LOL_USAGE_FMT, me, lol_version,
                lol_copyright, me, params);

        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf (LOL_VERSION_FMT, me,
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {

      if ((stat(argv[1], &st))) {
          lol_error(LOL_WRONG_OPTION, me, argv[1]);
	  lol_error(hlp, me);
          return -1;
      }
    }
  } // end if argc == 2

  if (argc != 2) {

        printf (LOL_USAGE_FMT, me, lol_version,
                lol_copyright, me, params);

        printf (hlp, me);
        return 0;
  }

  lfile = argv[1];
  if ((lol_stat(lfile, &st))) {
      lol_error(LOL_NOT_FOUND, me, lfile);
      return -1;
  }
  if (!(st.st_size))
     return 0;
  if (!(fp = lol_fopen(lfile, "r"))) {
      lol_error(LOL_NOT_FOUND, me, lfile);
      return -1;
  }
  size = (size_t)fp->nentry.file_size;
  if (size != st.st_size) {
      lol_error(LOL_BUG_FOUND, me, lfile);
      goto errlol;
  }
  if (!(dest = fopen("/dev/stdout", "w")))
      goto errlol;

  t = size / 4096;
  r = size % 4096;

  for (i = 0; i < t; i ++) {

     v = lol_fread((char *)ptr, 4096, 1, fp);
     if ((lol_ferror(fp)) || (v != 1)) {
        lol_error(E_FILE_READ, me, lfile);
        goto error; 
     }
     if (lol_fio((char *)ptr, 4096, dest, LOL_WRITE) != 4096) {
        lol_error(E_FILE_READ, me, lfile);
        goto error;
     }
  } // end for i

  if (r) {

     v = lol_fread((char *)ptr, r, 1, fp);
     if ((lol_ferror(fp)) || (v != 1)) {
        lol_error(E_FILE_READ, me, lfile);
        goto error; 
     }
     if (lol_fio((char *)ptr, r, dest, LOL_WRITE) != r) {
         lol_error(E_FILE_READ, me, lfile);
  	 goto error;
     }

  } // end if r

  ret = 0;

error:

  fclose(dest);

errlol:
#if LOL_TESTING
  if (lol_errno)
    lol_error("lol_errno = %d\n", lol_errno);
#endif

  lol_fclose(fp);
  return ret;
} // end lol_cat
