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
/* $Id: lol_cat.c, v0.11 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $" */

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

int lol_cat (int argc, char *argv[]) {

  struct stat st;
  char ptr[4096];
  lol_FILE *fp;
  FILE     *dest;
  size_t i, r, t;
  size_t v, size;
  int ret = -1;


  if (argc != 2) {

      printf("Usage: lol %s <container:/file>\n", argv[0]);
      puts  ("       Prints contents of a file to standard output.");
      return 0;
  }
  if ((lol_stat(argv[1], &st))) {
      printf(LOL_NOT_FOUND, argv[0], argv[1]);
      return -1;
  }
  if (!(st.st_size))
     return 0;
  if (!(fp = lol_fopen(argv[1], "r"))) {
      printf(LOL_NOT_FOUND, argv[0], argv[1]);
      return -1;
  }
  size = (size_t)fp->nentry.file_size;
  if (size != st.st_size) {
      printf(LOL_BUG_FOUND, argv[0], argv[1]);
      goto errlol;
  }
  if (!(dest = fopen("/dev/stdout", "w")))
      goto errlol;

  t = size / 4096;
  r = size % 4096;

  for (i = 0; i < t; i ++) {

     v = lol_fread((char *)ptr, 4096, 1, fp);
     if ((lol_ferror(fp)) || (v != 1)) {
        goto error; 
     }
     if (lol_fio((char *)ptr, 4096, dest, LOL_WRITE) != 4096) {
        goto error;
     }
  } // end for i

  if (r) {

     v = lol_fread((char *)ptr, r, 1, fp);
     if ((lol_ferror(fp)) || (v != 1)) {
        goto error; 
     }
     if (lol_fio((char *)ptr, r, dest, LOL_WRITE) != r) {
  	 goto error;
     }

  } // end if r

  ret = 0;

error:

  fclose(dest);

errlol:

  lol_fclose(fp);
  return ret;
} // end main
