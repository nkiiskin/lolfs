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
/* $Id: lol_rm.c, v0.10 2016/04/14 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $" */

#include <stdio.h>
#ifndef _LOLFS_H
#include <lolfs.h>
#endif


// lol rm: Deletes a file which is inside a lol container
//         Use like: lol rm my_container:/my_file.txt

int lol_rm (int argc, char* argv[])
{
  int i = 1;
  int rm = 0;

  if (argc < 2)
  {
     printf("Usage: lol %s: <container:/file> ...\n", argv[0]);
     puts  ("        Removes file(s) from a container.");
     return 0;
  }

  while (i < argc)
  {
     if (lol_unlink(argv[i])) {
       printf("lol %s: cannot remove '%s'\n", argv[0], argv[i]);
     }
     else {
       rm++;
     }
     i++;
  }

  switch (rm) {
  case 0 :
    puts("No files deleted");
    break;
  case 1 :
    puts("One file deleted");
    break;
  default:
    printf("%d files deleted\n", rm);
    break;
  } // end switch

  return 0;

} // end lol_rm
