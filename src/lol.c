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
 $Id: lol.c, v0.13 2016/11/11 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"

*/
/* ****************************************************************** */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lolfs.h>
#include <lol_internal.h>
/* ****************************************************************** */
typedef int (*lol_func)(int, char**);
struct lfuncs
{
  char*    name;
  lol_func func;
};
/* ****************************************************************** */
static const struct lfuncs funcs[] =
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
static const char    hlp[] = "           Type \'%s -h\' for help.\n";
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
  "           Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int main (int argc, char* argv[])
{
  char **av = NULL;
  char  *me = NULL;
  char   *p = NULL;
  int     a = argc - 1;
  int     i = 0;


  me = argv[0];
  // Process standard --help & --version options.
  if (argc == 2) {
    if (LOL_CHECK_HELP) {

        printf ("%s v%s. %s\nUsage: %s %s\n",
                me, lol_version, lol_copyright,
                me, params);

        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf ("%s v%s %s\n", me,
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {

        printf("%s: unrecognized option \'%s\'\n",
               me, argv[1]);
        printf (hlp, me);
        return -1;
    }
  } // end if argc == 2

  if (argc == 1) {

        printf ("%s v%s. %s\nUsage: %s %s\n",
                me, lol_version, lol_copyright,
                me, params);

        printf (hlp, me);
        return 0;
  }

  av = (char **)(&argv[1]);
  p  = argv[1];

  // Which function shall we use?
  while (funcs[i].name) {
     if (!(strcmp(p, funcs[i].name))) {
         return funcs[i].func(a, av);
     }
     i++;
  } // end while funcs

  printf("%s: unrecognized function \'%s\'\n", me, p);
  printf (hlp, me);

  return -1;
} // end main
