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
/* $Id: mkfs.lolfs.c, v0.12 2016/11/02 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $" */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lolfs.h>

static const char version[] = "0.12";
static const char usage[] = "<block size (in bytes)>  <number of blocks>  <filename>";
static const char copy[] = "Copyright (C) 2016, Niko Kiiskinen";

// Create container
int main (int argc, char* argv[])
{
  DWORD bs;
  DWORD nb;
  long size;

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
      printf("%s: error: unrecognized command line option \'%s\'\n", argv[0], argv[1]);
      return 0;

  } // end if argc == 2

  if (argc != 4) {

      printf("Usage: %s  <block size (in bytes)>  <number of blocks>  <filename>\n", argv[0]);
      return 0;
  }

  // We need 2 numbers, block size and the number of blocks
  bs   = (DWORD)strtol(argv[1], NULL, 10);
  nb   = (DWORD)strtol(argv[2], NULL, 10);
  size = bs * nb;

  if (lol_mkfs(bs, nb, argv[3])) {
       printf("%s: Cannot create container %s, size %ld bytes, (%ld Kb)\n",
              argv[0], argv[3], size, (size / 1024));
       puts("Please check if the directory is write protected");
  }
  else {
         puts("Container created");
  }

  return 0;
} // end main
