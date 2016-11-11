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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lolfs.h>
#include <lol_internal.h>

static const char version[] = "0.12";
static const char usage[] = "<function> <parameter(s)>";
static const char copy[] = "Copyright (C) 2016, Niko Kiiskinen";

int main (int argc, char* argv[])
{

  // Process standard --help & --version options.
  if (argc == 2) {
      if ((!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help"))) {
	printf ("%s v%s:\nUsage: %s %s\n", argv[0], version, argv[0], usage);
	return 0;
      }
      if ((!strcmp(argv[1], "-v")) || (!strcmp(argv[1], "--version"))) {
	printf ("%s v%s %s\n", argv[0], version, copy);
	return 0;
      }
  } // end if argc == 2

  if (argc == 1) {
      printf("Usage: %s %s\n", argv[0], usage);
      return 0;
  }

  // Which function shall we use?
  if ((!strcmp(argv[1], "ls"))) {
     return lol_ls(argc - 1, &argv[1]);
  }

  if ((!strcmp(argv[1], "rm"))) {
     return lol_rm(argc - 1, &argv[1]);
  }

  if ((!strcmp(argv[1], "cp"))) {
     return lol_cp(argc - 1, &argv[1]);
  }

  if ((!strcmp(argv[1], "df"))) {
     return lol_df(argc - 1, &argv[1]);
  }

  if ((!strcmp(argv[1], "cat"))) {
     return lol_cat(argc - 1, &argv[1]);
  }

  printf("%s: error: unrecognized command line option \'%s\'\n", argv[0], argv[1]);
  return 0;

} // end main
