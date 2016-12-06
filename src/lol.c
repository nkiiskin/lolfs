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
  {"ls",   lol_ls},
  {"rm",   lol_rm},
  {"cc",   lol_cc},
  {"cp",   lol_cp},
  {"df",   lol_df},
  {"cat",  lol_cat},
  {"fs",   lol_fs},
  {NULL,   NULL},
};
/* ****************************************************************** */
static const char params[] = "<function> <parameter(s)>";
static const char    hlp[] = "           Type \'%s -h\' for help.\n";
static const char*   lst[] =
{
  "Possible functions are:\n",
  "           cat  (Outputs a file inside a container)",
  "           cc   (Checks container for errors)",
  "           cp   (Copies files to and from a container)",
  "           df   (Prints space usage of a container)",
  "           fs   (Creates a new container file)",
  "           ls   (Lists contents of a given container)",
  "           rm   (Removes a file(s) from a container)\n",
  "           Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int main (int argc, char* argv[])
{
  char **av = NULL;
  char   *p = NULL;
  int     a = argc - 1;
  int     i = 0;

  // Process standard --help & --version options.
  if (argc == 2) {
    if (LOL_CHECK_HELP) {

        printf ("%s v%s. %s\nUsage: %s %s\n",
                argv[0], lol_version, lol_copyright,
                argv[0], params);


        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf ("%s v%s %s\n", argv[0],
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {

        printf("%s: unrecognized option \'%s\'\n",
               argv[0], argv[1]);
        printf (hlp, argv[0]);
        return -1;
    }
  } // end if argc == 2

  if (argc == 1) {

        printf ("%s v%s. %s\nUsage: %s %s\n",
                argv[0], lol_version, lol_copyright,
                argv[0], params);

        printf (hlp, argv[0]);
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

  printf("%s: unrecognized function \'%s\'\n",
          argv[0], p);

  printf (hlp, argv[0]);

  return -1;
} // end main
