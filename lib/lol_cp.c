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
/* $Id: lol_cp.c, v0.12 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $" */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif

/*
 *
 *  TODO: This program does not check if BOTH source
 *        and destination are lol files.
 *
 */




int copy_from_disk_to_lolfile(int argc, char *argv[]) {

  char temp[4096];
  char name[1024];
  char base[1024];

  size_t len, ln;
  // size_t bytes;
  size_t src_size;
  size_t last_bytes;
  long dst_size = 0;

      FILE *src;  // We copy from here
  lol_FILE *dest; // ...to here

  struct stat st, sta;
  int j, i, c, p;
  int loops;
  int num_files;
  char *vdisk;
  char answer;

  if (argc < 3)
      return -1;

  num_files = argc - 1;
  vdisk = argv[num_files];
  len = strlen (vdisk);

    dst_size = lol_free_space (vdisk);

    if (dst_size < 0)
      return -1;

  for (i = 1; i < num_files; i++) {

    if (stat (argv[i], &st))
      continue;

    if (!(strcmp(argv[i], vdisk))) {
       printf("Warning: Skipping file %s\n", argv[i]);
       continue;
    }

    if (!(S_ISREG(st.st_mode))) { // We copy only regular files
                                  // into container

      printf("%s is not a regular file, cannot copy it\n", argv[i]);
      continue;
    }

    // Create destination name of form lolfile:/filename
    // (Should create an interface function to do this common task !! )

    src_size = st.st_size;
    dst_size = lol_free_space(vdisk);
    if (dst_size < 0) {
      printf("Error in container file %s\n", vdisk);
      return -1;
    }
    if (!dst_size) {
        printf("Container file %s is full\n", vdisk);
        return -1;
    }

    if (src_size > dst_size) {
        printf("Warning: Not enough room for %s\n", argv[i]);
        continue;
    }
    memset((char *)name, 0, 1024);
    strcat(name, vdisk);
    name[len] = ':';
    name[len+1] = '/';

    ln = strlen(argv[i]);
    if (!ln)
       continue;
    if (argv[i][ln-1] == '/')
        continue;

    p = 0;
    for (j = ln - 1; j >= 0; j--) { // Count length of basename

         if (argv[i][j] == '/') {
	    break;
         }
	 p++;

    } // end for j

    memset (base, 0, 1024);
    if (p >= LOL_FILENAME_MAX) {
        printf("Warning: file name \"%s\" too long, truncating..\n", argv[i]);
    }

    if (p != ln) {

       p = 0;
       for (c = j + 1; c < ln; c++) {

	    base[p++] = argv[i][c];
	    if (p == (LOL_FILENAME_MAX-1))
	      break;

       } // end for c

    } // end if c
    else {

      if (ln >= LOL_FILENAME_MAX)
	ln = LOL_FILENAME_MAX - 1;
        memcpy (base, argv[i], ln);

    }

    strcat (name, base);
    if (!(lol_stat(name, &sta))) {
       printf("The file %s exists. Overwrite [y/n]? ", name);
       answer = (char)getchar();
       (void)getchar();
       if (answer != 'y')
	 continue;
    }

    if (!(dest = lol_fopen(name, "w"))) {
          printf("Cannot copy to file %s\n", base);
          continue;
    }

    if (!(src = fopen(argv[i], "r"))) {
         printf("Cannot read from file %s\n", argv[i]);
	 if (lol_fclose(dest)) {
	     puts("I/O error, aborting..");
	     return -1;
	 }

         continue;
    }

    memset(temp, 0, 4096);
    loops = src_size / 4096;
    last_bytes = src_size % 4096;

    for (j = 0; j < loops; j++) {
      if ((lol_fio((char *)temp, 4096, src, LOL_READ)) != 4096) {
          printf("Cannot read from file %s\n", argv[i]);
	  last_bytes = 0; break;
      }
      if ((lol_fwrite((char *)temp, 4096, 1, dest)) != 1) {
	  printf("Cannot copy to file %s\n", name);
	  last_bytes = 0; break;
      }

    } // end for j

    do {
        if (last_bytes) {
            if ((lol_fio((char *)temp, last_bytes, src, LOL_READ)) != last_bytes) {
	        printf("Cannot read from file %s\n", argv[i]);
	        break;
            }
            if ((lol_fwrite((char *)temp, last_bytes, 1, dest)) != 1) {
	         printf("Cannot copy to file %s\n", name);
            }
        } // end if last_bytes
    } while (0);

  if (lol_fclose(dest)) {
      fclose(src);
      puts("I/O error, aborting..");
      return -1;
  }
  if (fclose(src)) {
      puts("I/O error, aborting..");
      return -1;
  }

 } // end for i

  return 0;

} // end copy_from_disk_to_lolfile
/* ********************************************************************* */
int copy_from_lolfile_to_disk(int argc, char *argv[]) {


  struct stat st;
  char temp[4096];
  char name[1024];
  size_t len;
  size_t src_size;
  size_t last_bytes;

  lol_FILE  *src; // We copy from here
      FILE *dest; // ...to here

  int j, i, loops;
  int num_files;
  int dest_is_dir = 0;
  char *dir = 0;


  if (argc < 3)
    return -1;

  num_files = argc - 1;
  dir = argv[num_files];
  memset((char *)&st, 0, sizeof(st));
  i = stat(dir, &st);

  if (num_files > 2) {

                       if (i) {
                           printf("The directory %s does no exist\n", dir);
                           return -1;
                       }

                       if (!(S_ISDIR(st.st_mode))) {
                           puts("Syntax error");
                           return -1;
                       }
                       else {
			     dest_is_dir = 1;
		       }

  } // end if num_files > 1
  else {
    // So, just one file. Is the destination
    // a directory or a file?

    if ((S_ISDIR(st.st_mode))) {
        dest_is_dir = 1;
    }
    else {

      // Not a directory.
      // Does it exist already?
      if (!i) {

	// It does, but is it a regular file?
            if (!(S_ISREG(st.st_mode))) {

      	      // Not a file or directory. Can't use it!
               printf("Error: %s is not a file or directory\n", dir);
	       return -1;
	    }

      } // end if !i

    } // end else is_dir

  } // end else num_files > 1


  len = strlen (dir);

  for(i = 1; i < num_files; i++) {

    if (!(src = lol_fopen(argv[i], "r"))) {
      printf("Cannot copy lol-file %s\n", argv[i]);
      continue;
    }

    // Create destination from lolfile:/filename --> filename

    memset((char *)name, 0, 1024);

    if (dest_is_dir) {
        strcat(name, dir);
        if (name[len-1] != '/') // append slash if it's not there
            name[len] = '/';
        strcat(name, src->vdisk_file);
    }
    else {

          strcat(name, dir);
      }

      if (!(stat(name, &st))) {
	// Prompt
                printf("The file %s exists. Overwrite [y/n]? ", name);
                temp[0] = (char)getchar();
		(void)getchar();
                if (temp[0] != 'y') {
		    if (lol_fclose(src)) {
		      puts("I/O error, aborting..");
		      return -1;
		    }
		    continue;
                }
      }

    if (!(dest = fopen(name, "w"))) {
           printf("Cannot copy to file %s\n", name);
		    if (lol_fclose(src)) {
		      puts("I/O error, aborting..");
		      return -1;
		    }

          continue;
    }

    src_size = (size_t)src->nentry.file_size;
    memset(temp, 0, 4096);
    loops = src_size / 4096;
    last_bytes = src_size % 4096;


       for (j = 0; j < loops; j++) {

	      if ((lol_fread((char *)temp, 4096, 1, src)) != 1) {
	          printf("Cannot read from file %s\n", argv[i]);
		  last_bytes = 0; break;
	      }
              if ((lol_fio((char *)temp, 4096, dest, LOL_WRITE)) != 4096) {
	          printf("Cannot copy to file %s\n", name);
		  last_bytes = 0; break;
              }

       } // end for j

       do {
           if (last_bytes) {
	       if ((lol_fread((char *)temp, last_bytes, 1, src)) != 1) {
	           printf("Cannot read from file %s\n", argv[i]);
		   break;
	       }
               if ((lol_fio((char *)temp, last_bytes, dest, LOL_WRITE)) != last_bytes) {
	           printf("Cannot copy to file %s\n", name);
               }
           }
       } while (0);

  if (lol_fclose(src)) {
      fclose(dest);
      puts("I/O error, aborting..");
      return -1;
  }
  if (fclose(dest)) {
      puts("I/O error, aborting..");
      return -1;
  }

 } // end for i

  return 0;

} // end copy_from_lolfile_to_disk
/* ********************************************************************* */
int lol_cp (int argc, char* argv[]) {

  // struct stat st;
  // int i, n;
  char *dir;

  if (argc < 3) {

      printf("lol cp: missing file operand\n");
      return 0;

  }

  dir = argv[argc-1];

  // Do we copy TO or FROM lolfile?
  if (lol_is_validfile (dir)) {
    // dir is a container, so we copy from disk to it
    if (copy_from_disk_to_lolfile(argc, argv) < 0) {
        printf("%s: Target cannot be the same as the source\n", argv[0]);
	return -1;
    }

     return 0;
  }

  // So, we copy files from a container to disk..
  // Check that the target is a directory.

#if 0
  n = argc - 1;

  for (i = 1; i < n; i++) {
  if (stat (dir, &st)) {
      printf("Cannot copy to directory %s\n", dir);
      return -1;
  }
  }

  if (!(S_ISDIR(st.st_mode))) {
      printf("Error: target must be a directory\n");
      return -1;
  }

#endif


  // If ok, copy
    return copy_from_lolfile_to_disk(argc, argv);

} // end main

