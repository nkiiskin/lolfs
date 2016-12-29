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
 $Id: lol_df.c, v0.20 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ****************************************************************** */
static const char  cantuse[] = "lol %s: cannot use %s\n";
static const char cantread[] = "lol %s: cannot read %s\n";
static const char   params[] = "[-s] <container>";
static const char      hlp[] = "       Type: 'lol %s -h' for help.\n";
static const char*     lst[] =
{
  "  Example:\n",
  "          lol df lol.db",
  "          This shows the space usage of",
  "          container file \'lol.db\'\n",
  "  Use option \'-s\' to suppress warnings",
  "  about suspicious filenames like:",
  "          lol df -s lol.db\n",
  "          Type: 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
#define LOL_DF_MESSAGE 1024
#define LOL_DF_ALIGN 25
static void lol_fd_clear(char *a, char *b) {
  memset((char *)a, 0, LOL_DF_MESSAGE);
  memset((char *)b, 0, LOL_DF_MESSAGE);
}
/* ****************************************************************** */
int lol_df (int argc, char* argv[])
{

  char number[LOL_DF_MESSAGE];
  char before[LOL_DF_MESSAGE];
  char  after[LOL_DF_MESSAGE];
  struct lol_name_entry name_e;
  struct stat st;
  struct lol_super sb;
  FILE *vdisk;
  char *me;
  char *cont;
  DWORD i, nf;
  size_t      len   = 0;
  long num_blocks   = 0;
  long block_size   = 0;
  long used_blocks  = 0;
  long free_space   = 0;
  long files        = 0;
  long terminators  = 0;
  long unused_bl    = 0;
  long occupation   = 0;
  long used_space   = 0;
  long estim_blocks = 0;
  long doff         = 0;
  alloc_entry entry = 0;
  float available   = 0;
  int    n_garbages = 0;
  int        silent = 0;
  int err = 0;

  if (!(argv[0])) {

      lol_error(LOL_INTERERR_FMT);
      return -1;
  }
  me = argv[0];

  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {

        printf (LOL_USAGE_FMT, me, lol_version,
                lol_copyright, me, params);

        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf (LOL_VERSION_FMT, me,
                lol_version, lol_copyright);
	return 0;
    }
    if (LOL_CHECK_SILENT) {
        lol_error(LOL_MISSING_ARG_FMT, me, "<container>");
        return -1;
    }
    if (argv[1][0] == '-') {

      if ((stat(argv[1], &st))) {
          lol_error(LOL_WRONG_OPTION, me, argv[1]);
	  lol_error(hlp, me);
          return -1;
      }
    }

  } // end if argc == 2

  if (argc == 3) {
    if (LOL_CHECK_SILENT) {
        silent = 1;
        cont   = argv[2];
	goto action;
    }
  } // end if argc == 3

  if (argc != 2) {

        printf (LOL_USAGE_FMT, me, lol_version,
                lol_copyright, me, params);
        puts  ("       Shows container space usage.");
        printf (hlp, me);
        return 0;
  }

  cont = argv[1];

action:


  free_space = lol_get_vdisksize(cont, &sb,
               NULL, RECUIRE_SB_INFO);
  if (free_space < LOL_THEOR_MIN_DISKSIZE) {

       lol_error(cantuse, me, cont);
       //printf(cantuse, me, cont);
       return -1;
  }
  if (LOL_INVALID_MAGIC) {


      lol_error("lol %s: invalid file id [0x%x, 0x%x].\n",
	   me, (int)sb.reserved[0], (int)sb.reserved[1]);
      lol_error(LOL_FSCK_FMT);
      return -1;
  }

  num_blocks = (long)sb.num_blocks;
  block_size = (long)sb.block_size;
  if ((!(num_blocks)) || (!(block_size))) {

      lol_error(cantuse, me, cont);
      return -1;
  }

  free_space = (long)(num_blocks * block_size);
  memset((char *)number, 0, LOL_DF_MESSAGE);
  lol_size_to_str((unsigned long)free_space, number);
  lol_fd_clear(before, after);
  sprintf(before, "Total  space: %s ", number);
  sprintf(after, "[%ld blocks, each %ld bytes]\n", num_blocks, block_size);
  lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);

  nf = sb.num_files;
  if (!(vdisk = fopen(cont, "r"))) {

       lol_error(cantread, me, cont);
       return -1;
  }

  doff = LOL_DENTRY_OFFSET_EXT(num_blocks, block_size);
  if (fseek (vdisk, doff, SEEK_SET)) {

       fclose(vdisk);
       lol_error(cantread, me, cont);
       return -1;
  }

  // Read the name entries
  for (i = 0; i < num_blocks; i++) {

    if ((fread((char *)&name_e, (size_t)(NAME_ENTRY_SIZE), 1, vdisk)) != 1) {

       fclose(vdisk);
       lol_error(cantread, me, cont);
       return -1;
    }
    if (name_e.filename[0]) { // Should check more but this will do now
        files++;
	if ((name_e.i_idx < 0) || (name_e.i_idx >= num_blocks)) {
	  err = 1;
	}

        if (name_e.file_size) {
            used_space += name_e.file_size;
	    estim_blocks += (name_e.file_size / block_size);
            if (name_e.file_size % block_size)
	        estim_blocks++;
	}
        else {
	  estim_blocks++; // Zero size occupy 1 block
	}

	// Do some checking while we are here..
        if ((lol_garbage_filename((char *)(name_e.filename)))) {
	     n_garbages++;
	} // end if suspicious name
	len = strlen((char *)(name_e.filename));
	if (len > LOL_FILENAME_MAXLEN) {
	    err = 1;
	}
    }
  } // end for i

  // Read also the reserved blocks.
  for (i = 0; i < num_blocks; i++) {

    // We could propably suck all the indexes into
    // a buffer with a few reads, something todo...

    if ((fread((char *)&entry, (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
         fclose(vdisk);
         lol_error(cantread, me, cont);
         return -1;
    }
    if (entry == LAST_LOL_INDEX) {
        terminators++;
        used_blocks++;
        continue;
    }
    if (entry == FREE_LOL_INDEX) {
        continue;
    }
    if (entry < 0) {
        err = 1;
        used_blocks++;
        continue;
    }
    if (entry >= num_blocks) {
#if LOL_TESTING
      printf("lol df: Found invalid index %d at entry %u\n",
             entry, i);
#endif
        used_blocks++;
	err = 1;
	continue;
    }

    used_blocks++;

  } // end for i

  fclose(vdisk);
  if ((terminators != nf) || (terminators != files)) {
#if LOL_TESTING

    printf("lol df: DEGUG: ERROR: nf = %u, files = %ld, terms = %ld\n", nf, files, terminators);

#endif
    err = 1;
  }

  occupation = (long)(used_blocks * block_size);
  free_space = (long)(num_blocks - used_blocks);
  unused_bl  = (long)(free_space);
  available  = (float)(free_space);
  free_space *= block_size;
  available  /= ((float)(num_blocks));
  available  *= ((float)(100.0));

  memset((char *)number, 0, LOL_DF_MESSAGE);
  lol_size_to_str((unsigned long)free_space, number);
  lol_fd_clear(before, after);
  sprintf(before, "Unused space: %s ", number);
  sprintf(after, "[%ld blocks, %2.1f%% free]\n",
          unused_bl, available);
  lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);

  if (files) {

     available = 100.0 - available;
     memset((char *)number, 0, LOL_DF_MESSAGE);
     lol_size_to_str((unsigned long)(occupation), number);
     lol_fd_clear(before, after);
     sprintf(before, "Used   space: %s ", number);
     sprintf(after, "[%ld bytes in %ld blocks, %2.1f%% used]\n",
             used_space, used_blocks, available);
     lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);
  }
  else {
    puts("No files");
  }

  if (used_blocks) {
    if (!(files))
        err = 1;
  }

  if (estim_blocks != used_blocks) {
      err = 1;
  }

  if ((used_space > occupation)  || (err) || (nf > used_blocks) ||
      (used_blocks > num_blocks) || (nf != files)) {

      lol_error("lol %s: container \'%s\' has errors.\n", me, cont);
      lol_error(LOL_FSCK_FMT);
      return -1;
  }

  if (!(silent)) {
     if (n_garbages > 5) {

         printf("lol %s: container \'%s\' has suspicious filenames.\n",
                 me, cont);
         puts("        This may be if they have Unicode characters");
         puts("        To suspend this message, use option \'-s\'");
     }
  } // end if not silent

  return 0;
} // end lol_df
