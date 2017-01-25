/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 $Id: lol_cat.c, v0.30 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_STDIO_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
#endif
#ifdef HAVE_STRING_H
#ifndef _STRING_H
#include <string.h>
#endif
#endif
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ****************************************************************** */
static const char params[] = "<container:/filename>";
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

  char ptr[LOL_DEFBUF];
  struct stat st;
  lol_pinfo    p;
  const char *me = argv[0];
  lol_FILE   *fp;
  FILE     *dest;
  char     *name;
  size_t   csize;
  size_t i, r, t;
  size_t v, size;
  int    ret = 0;

  // Process standard --help & --version options.
  if (argc == 2) {

     if (LOL_CHECK_HELP) {
         lol_show_usage(me);
         lol_help(lst);
         return 0;
     }
     if (LOL_CHECK_VERSION) {
         lol_show_version(me);
	 return 0;
     }
     if (argv[1][0] == '-') {
	if ((stat(argv[1], &st))) {
           lol_errfmt2(LOL_2E_OPTION, me, argv[1]);
	   lol_ehelpf(me);
           return -1;
	}
     }
  } // end if argc == 2
  if (argc != 2) {

      lol_show_usage(me);
      puts  ("       Prints content of a file to standard output");
      lol_helpf(me);
      return 0;
  }
  name = argv[1];

  p.fullp = name;
  p.cont  = ptr;
  p.func  = LOL_CONTPATH;
  if ((lol_pathinfo(&p))) {
      lol_errfmt2(LOL_2E_NOSUCHF, me, name);
      return -1;
  }
  csize = lol_validcont(ptr, NULL, NULL);
  if (!(csize)) {
     lol_errfmt2(LOL_2E_CORRCONT, me, ptr);
     lol_errfmt(LOL_0E_USEFSCK);
     return -1;
  }
  if ((lol_stat(name, &st))) {
     lol_errfmt2(LOL_2E_INVSRC, me, name);
     return -1;
  }
  if (!(fp = lol_fopen(name, "r"))) {
     lol_errfmt2(LOL_2E_CANTREAD, me, name);
     return -1;
  }
  size = (size_t)fp->nentry.fs;
  if (size != st.st_size) {
      lol_fclose(fp);
      lol_errfmt2(LOL_2E_FIOERR, me, name);
      return -1;
  }
  if (!(dest = fopen("/dev/stdout", "w"))) {
      lol_fclose(fp);
      lol_errfmt2(LOL_2E_FIOERR, me, name);
      return -1;
  }
  t = size / LOL_DEFBUF;
  r = size % LOL_DEFBUF;
  size = LOL_DEFBUF;

read_loop:
  for (i = 0; i < t; i ++) {
     v = lol_fread((char *)ptr, size, 1, fp);
     if ((fp->err) || (v != 1)) {
        lol_errfmt2(LOL_2E_CANTREAD, me, name);
        ret = -1; r = 0; break;
     }
     if ((fwrite((char *)ptr, size, 1, dest)) != 1) {
         lol_errfmt2(LOL_2E_CANTWRITE, me, name);
         ret = -1; r = 0; break;
     }
  } // end for i

  if (r) {

     t = 1;
     size = r;
     r = 0;
     goto read_loop;

  }

  lol_fclose(fp);
  fclose(dest);
  return ret;
} // end lol_cat
