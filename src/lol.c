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
/* $Id: lol.c, v0.12 2016/11/11 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $" */
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
  {"ls",  lol_ls},
  {"rm",  lol_rm},
  {"cp",  lol_cp},
  {"df",  lol_df},
  {"cat", lol_cat},
  {NULL},
};
/* ****************************************************************** */
static const char  vers[] = "0.12";
static const char   usg[] = "<function> <parameter(s)>";
static const char  usg2[] = "           Type '%s -h' for help.\n";
static const char  copy[] = "Copyright (C) 2016, Niko Kiiskinen";
static const char*  lst[] =

{
  "Possible functions are:\n",
  "           ls  (Lists contents of a given container)",
  "           cp  (Copies files to and from a container)",
  "           rm  (Removes a file(s) from a container)",
  "           df  (Prints space usage of a container)",
  "           cat (prints contents of a file inside a container)\n",
  "           Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
void help() {
  int i = 0;
  while (lst[i]) {
    puts(lst[i++]);
  };
} // end help
/* ****************************************************************** */
int main (int argc, char* argv[])
{
  char **av = NULL;
  char   *p = NULL;
  int     a = argc - 1;
  int     i = 0;

  // Process standard --help & --version options.
  if (argc == 2) {
    if ((!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help"))) {
      printf ("%s v%s. %s\nUsage: %s %s\n", argv[0], vers, copy, argv[0], usg);
      help ();
      return 0;
    }
    if ((!strcmp(argv[1], "-v")) || (!strcmp(argv[1], "--version"))) {
	printf ("%s v%s %s\n", argv[0], vers, copy);
	return 0;
    }
  } // end if argc == 2

  if (argc == 1) {
      printf("Usage: %s %s\n", argv[0], usg);
      printf (usg2, argv[0]);
      return 0;
  }

  av = (char **)(&argv[1]);
  p  = argv[1];

  // Which function shall we use?
  while (i < N_LOLFUNCS) {
     if (!(strcmp(p, funcs[i].name))) {
         return funcs[i].func(a, av);
  }
     i++;
  } // end while funcs

  printf("%s: error: unrecognized function \'%s\'\n",
          argv[0], p);
  help();

  return -1;
} // end main
