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
 $Id: lol_rm.c, v0.40 2016/04/14 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ****************************************************************** */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_STDIO_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
#endif
#ifndef  _LOLFS_H
#include <lolfs.h>
#endif
#ifndef  _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ****************************************************************** */
static const char params[] = "<container:/file1> <container:/file2> ...";
static const char*   lst[] =
{
  "  Example:\n",
  "          lol rm lol.db:/memo.txt",
  "          This deletes the file \'memo.txt\'",
  "          which is inside container file \'lol.db\'\n",
  "          Type: 'man lol' to read the manual.\n",
  NULL
};
/* **************************************************************** *
 * lol rm: Deletes a file which is inside a lol container
 *         Use like: lol rm my_container:/my_file.txt
 ****************************************************************** */
int lol_rm (int argc, char* argv[])
{
  const int nf = argc - 1;
  char *me = argv[0];
  int    i = 1;
  int errs = 0;

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
  } // end if argc == 2
  if (argc < 2) {
        lol_show_usage(me);
        puts  ("       Removes file(s) from a container.");
        lol_helpf(me);
        return 0;
  }
  // Just loop through files and unlink..
  while (i < argc) {
    if (lol_unlink(argv[i])) {
        lol_errfmt2(LOL_2E_NOSUCHF, me, argv[i]);
	errs++;
    }
    i++;
  }

  // How many files failed?
  // If over 50% --> return error
  if (errs) {
    if ((nf / errs) < 2) {
      return -1;
    }
  }
  return 0;
} // end lol_rm
