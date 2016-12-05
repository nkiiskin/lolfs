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
 $Id: lol_cp.c, v0.13 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"

*/
/* ************************************************************************** */
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
 * This program copies file(s) to and from a
 * given container.
 * It became quite big but we have to check
 * every little detail because we are copying
 * files (which may actually be important!!!)
 *
 */


/*
 *
 *  TODO: This program does not check if BOTH source
 *        and destination file(s) are inside a container.
 *
 */

/* ************************************************************** */
static int can_replace (const size_t src_s, const size_t dst_s,
                 const long free_bytes, const long bs)  {

  // Here we KNOW we are replacing an existing file, which size is
  // dst_s. free_bytes may be 0, however there may be free space in the
  // last block of the file.

  long dst_size = (long)dst_s;
  long src_size = (long)src_s;
  long available_bytes = 0;
  long margin;

  if ((free_bytes < 0) || (bs <= 0) ||
      (dst_size < 0)   || (src_size < 0)) {
#if LOL_TESTING
  printf("src = %ld, dst = %ld, freeb = %ld, bs = %ld\n", src_size, dst_size,
	 free_bytes, bs);
#endif

    return -1;
  }

  if (src_size <= dst_size)
      return 0;

#if TESTING
    printf("src = %ld, dst = %ld, freeb = %ld, bs = %ld\n", src_size, dst_size,
      free_bytes, bs);
#endif

  // How much room in last block?
  margin = dst_size % bs;
  if (margin) {
      available_bytes = bs - margin;
  }
  // Add free blocks to it
  available_bytes += free_bytes;
  // Add the existing file size
  available_bytes += dst_size;

  if (src_size > available_bytes)
    return -1;

  return 0;
} // end can_replace
/* ************************************************************** */
int copy_from_disk_to_lolfile(int argc, char *argv[]) {

  char temp[4096];
  char name[1024];
  char base[1024];

  size_t len, ln;
  size_t src_size;
  size_t last_bytes;
  long dst_size = 0;
  ino_t cont_ino;

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
  vdisk     = argv[num_files];
  len       = strlen (vdisk);

  if (stat (vdisk, &sta)) {
	printf("lol %s: error in container %s\n", argv[0], vdisk);
        return -1;
  }

  cont_ino = sta.st_ino;

#if 0

    dst_size = lol_free_space (vdisk);
    if (dst_size < 0)
      return -1;
#endif


  for (i = 1; i < num_files; i++) {

    if (stat (argv[i], &st)) {
	printf("lol %s: cannot copy to file %s\n", argv[0], argv[i]);
        continue;
    }

    if (st.st_ino == cont_ino) {
      //printf("Warning: Skipping file %s\n", argv[i]);
        continue;
    }

    if (!(S_ISREG(st.st_mode))) { // We copy only regular files
                                  // into container
      if ((S_ISDIR(st.st_mode))) {
	   printf("lol %s: cannot copy directory %s\n", argv[0], argv[i]);
      }
      else {
	   printf("lol %s: %s is not a regular file\n", argv[0], argv[i]);
      }
      continue;
    }

    // Create destination name of form lolfile:/filename
    // (Should create an interface function to do this common task !! )

    src_size = (size_t)st.st_size;
    memset((char *)name, 0, 1024);
    strcat(name, vdisk);
    name[len] = ':';
    name[len+1] = '/';

    ln = strlen(argv[i]);
    if (!(ln))
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
	   printf("lol %s: file name \"%s\"\n", argv[0], argv[i]);
	     puts("        too long, truncating");
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

    dst_size = lol_free_space(vdisk, LOL_SPACE_BYTES);
    if (dst_size < 0) {
	printf("lol %s: error in container %s\n", argv[0], vdisk);
        return -1;
    }
#if LOL_TESTING
    printf("DEBUG: Bytes in unused blocks = %ld\n", (long)dst_size);
#endif

    answer = 'n';

    //lol_errno = 0;

    if (!(lol_stat(name, &sta))) {
      // Exixting file !
       printf("lol %s: %s exists. Replace [y/n]? ", argv[0], name);
       answer = (char)getchar();
       (void)getchar();
       if (answer != 'y')
	 continue;
       // Ok, user wants to replace, do we have room?

       if (can_replace (((size_t)(src_size)), ((size_t)(sta.st_size)),
		       ((long)(dst_size)), (long)(sta.st_blksize))) {

	     printf("lol %s: not enough room for %s\n", argv[0], argv[i]);
             continue;

        } // end if can_replace

    }
    else {
      // Error or file does not exist.
#if LOL_TESTING
      if (lol_errno) {
	printf("DEBUG: lol_errno = %d\n", lol_errno);
      }
#endif

      if (src_size > dst_size) {

	     printf("lol %s: not enough room for %s\n", argv[0], argv[i]);
             continue;

      } // end if src_size >

    } // end else


    if (!(dest = lol_fopen(name, "w"))) {
	  printf("lol %s: cannot copy to file %s\n", argv[0], base);
          continue;
    }

    //if source size is zero, just close and continue
    if (!(src_size)) {
        if (lol_fclose(dest)) {
            printf("lol %s: I/O error\n", argv[0]);
            return -1;
        }
	continue;
    } // end if !src_size

    //printf("DEBUG: src size = %d, dest size = %d\n", (int)src_size, (int)dst_size);

    if (!(src = fopen(argv[i], "r"))) {
	 printf("lol %s: cannot read file %s\n", argv[0], argv[i]);
	 if (lol_fclose(dest)) {
	     printf("lol %s: I/O error\n", argv[0]);
	     return -1;
	 }

         continue;
    }

    memset(temp, 0, 4096);
    loops = src_size / 4096;
    last_bytes = src_size % 4096;

    for (j = 0; j < loops; j++) {

      if ((lol_fio((char *)temp, 4096, src, LOL_READ)) != 4096)
      {
	  printf("lol %s: cannot read file %s\n", argv[0], argv[i]);
	  last_bytes = 0; break;
      }
      if ((lol_fwrite((char *)temp, 4096, 1, dest)) != 1)
      {
	  printf("lol %s: cannot copy to file %s\n", argv[0], name);
	  last_bytes = 0; break;
      }
    } // end for j

  do {
      if (last_bytes) {
        if ((lol_fio((char *)temp, last_bytes, src, LOL_READ)) != last_bytes)
        {
	    printf("lol %s: cannot read file %s\n", argv[0], argv[i]);
	    break;
        }
#if LOL_TESTING
	printf("DEBUG: Trying to write %ld bytes to container\n", (long)last_bytes);
#endif

        if ((lol_fwrite((char *)temp, last_bytes, 1, dest)) != 1)
        {
	  printf("lol %s: cannot copy to file %s\n", argv[0], name);

#if LOL_TESTING
	  printf("DEBUG: lol_errno = %d\n", lol_errno);
	  printf("DEBUG: = lol_ferror = %d\n", (lol_ferror(dest)));
#endif

        }
      } // end if last_bytes
    } while (0);

  if (lol_fclose(dest)) {
      fclose(src);
      printf("lol %s: I/O error\n", argv[0]);
      return -1;
  }
  if (fclose(src)) {
      printf("lol %s: I/O error\n", argv[0]);
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
	  printf("lol %s: cannot find directory %s\n", argv[0], dir);
          return -1;
      }

      if (!(S_ISDIR(st.st_mode))) {
	  printf("lol %s: syntax error\n", argv[0]);
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
	  printf("lol %s: %s: no such file or directory\n", argv[0], dir);
	  return -1;
	}

      } // end if !i

    } // end else is_dir

  } // end else num_files > 1


  len = strlen (dir);

  for(i = 1; i < num_files; i++) {

    if (!(src = lol_fopen(argv[i], "r"))) {
	printf("lol %s: cannot read file %s\n", argv[0], argv[i]);
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
          printf("lol %s: %s exists. Replace [y/n]? ", argv[0], name);
                temp[0] = (char)getchar();
		(void)getchar();
                if (temp[0] != 'y') {
		    if (lol_fclose(src)) {
                      printf("lol %s: I/O error\n", argv[0]);
		      return -1;
		    }
		    continue;
                }
      }

    if (!(dest = fopen(name, "w"))) {
	  printf("lol %s: cannot copy to file %s\n", argv[0], name);
		    if (lol_fclose(src)) {
                      printf("lol %s: I/O error\n", argv[0]);
		      return -1;
		    }

          continue;
    }

    src_size = (size_t)src->nentry.file_size;

    // Don't copy if zero size, just close
    if (src_size) {

      memset(temp, 0, 4096);
      loops = src_size / 4096;
      last_bytes = src_size % 4096;

       for (j = 0; j < loops; j++) {

	      if ((lol_fread((char *)temp, 4096, 1, src)) != 1) {
	          printf("lol %s: cannot read file %s\n", argv[0], argv[i]);
		  last_bytes = 0; break;
	      }
              if ((lol_fio((char *)temp, 4096, dest, LOL_WRITE)) != 4096) {
	          printf("lol %s: cannot copy to file %s\n", argv[0], name);
		  last_bytes = 0; break;
              }

       } // end for j

       do {
           if (last_bytes) {
	       if ((lol_fread((char *)temp, last_bytes, 1, src)) != 1) {
	           printf("lol %s: cannot read file %s\n", argv[0], argv[i]);
		   break;
	       }
               if ((lol_fio((char *)temp, last_bytes, dest, LOL_WRITE)) != last_bytes) {
	           printf("lol %s: cannot copy to file %s\n", argv[0], name);
               }
           }
       } while (0);

    } // end if src_size

  if (lol_fclose(src)) {
      fclose(dest);
      printf("lol %s: I/O error\n", argv[0]);
      return -1;
  }
  if (fclose(dest)) {
      printf("lol %s: I/O error\n", argv[0]);
      return -1;
  }

 } // end for i

  return 0;

} // end copy_from_lolfile_to_disk
/* ****************************************************************** */
static const char params[] = "file(s)  destination";
static const char    hlp[] = "       Type 'lol %s -h' for help.\n";
static const char*   lst[] =
{
  "  Example 1:\n",
  "            lol cp lol.db:/memo.txt memo.bak",
  "            This copies the file \'memo.txt\'",
  "            which is inside a container file \'lol.db\'",
  "            to current directory as \'memo.bak\'.\n",
  "  Example 2:\n",
  "            lol cp src/* ~/lol.db",
  "            This copies all the files in the",
  "            directory \'src\' to container file",
  "            \'lol.db\' which is in the home",
  "            directory of the current user.\n",
  "          Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int lol_cp (int argc, char* argv[]) {

  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {

        printf (LOL_USAGE_FMT, argv[0], lol_version,
                lol_copyright, argv[0], params);

        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {
	printf (LOL_VERSION_FMT, argv[0],
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {
        printf(LOL_WRONG_OPTION, argv[0], argv[1]);
        printf (hlp, argv[0]);
        return -1;
    }
  } // end if argc == 2

  if (argc < 3) {

        printf (LOL_USAGE_FMT, argv[0], lol_version,
                lol_copyright, argv[0], params);

        puts  ("       Copies file(s) to and from a container.");
        printf (hlp, argv[0]);
        return 0;
  }

  // Do we copy TO or FROM lolfile?
  if (lol_is_validfile (argv[argc-1])) {
    // dir is a container, so we copy from disk to it
    if (copy_from_disk_to_lolfile(argc, argv) < 0) {
      // printf("%s: No files were copied\n", argv[0]);
	return -1;
    }

     return 0;
  }

    // So, we copy files from a container to disk..
    return copy_from_lolfile_to_disk(argc, argv);

} // end lol_cp
