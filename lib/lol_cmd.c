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
 $Id: lol_cmd.c, v0.30 2016/11/11 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"

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
typedef int (*lol_func)(int, char**);
struct lol_function
{
  char*    n;
  lol_func f;
};
/* ****************************************************************** */
static const struct lol_function funcs[] =
{
  // Sorted by assumed popularity..
  {"ls",   lol_ls},
  {"cp",   lol_cp},
  {"rm",   lol_rm},
  {"df",   lol_df},
  {"cat",  lol_cat},
  {"fs",   lol_fs},
  {"rs",   lol_rs},
  {"cc",   lol_cc},
  {NULL,   NULL},
};
/* ****************************************************************** */
static const char params[] = "<function> <parameter(s)>";
static const char*   lst[] =
{
  "Possible functions are:\n",
  "           ls   - lists files inside a container",
  "           cp   - copies  file(s) to and from a container",
  "           rm   - removes file(s) from a container",
  "           df   - shows space usage of a container",
  "           cat  - outputs a file inside a container",
  "           fs   - creates a new container file",
  "           rs   - resizes a container",
  "           cc   - checks container for errors\n",
  "           Type: 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int lol_cmd (int argc, char* argv[])
{
  char  *me = argv[0];
  char **av = NULL;
  char   *p = NULL;
  int     a = argc - 1;
  int     i = 0;

  // Process standard --help & --version options.
  if (argc == 2) {

     if (LOL_CHECK_HELP) {
         lol_show_usage2(me);
         lol_help(lst);
         return 0;
     }
     if (LOL_CHECK_VERSION) {
         lol_show_version2(me);
	 return 0;
     }
     if (argv[1][0] == '-') {
         lol_errfmt2(LOL_2E_OPTION2, me, argv[1]);
	 lol_ehelpf2(me);
         return -1;
     }
  } // end if argc == 2
  if (argc == 1) {

     lol_show_usage2(me);
     lol_helpf2(me);
     return 0;
  }

  av = (char **)(&argv[1]);
  p  = argv[1];

  // Which function shall we use?
  while (funcs[i].n) {
     if (!(strcmp(p, funcs[i].n))) {
         return funcs[i].f(a, av);
     }
     i++;
  } // end while funcs

  lol_errfmt2(LOL_2E_INVFUNC, me, p);
  lol_ehelpf2(me);

  return -1;
} // end lol_cmd
