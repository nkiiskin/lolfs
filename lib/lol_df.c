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
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_SYS_STAT_H
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#endif
#ifdef HAVE_STDIO_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
#endif
#ifdef HAVE_STRING_H
#ifndef _STRING_H
#include <string.h>
#endif
#endif
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

  char temp[512 * NAME_ENTRY_SIZE];
  char number[LOL_DF_MESSAGE];
  char before[LOL_DF_MESSAGE];
  char  after[LOL_DF_MESSAGE];
  struct lol_name_entry *name_e;
  struct stat st;
  struct lol_super sb;
  FILE *fp;
  char *me;
  char *cont;
  struct lol_name_entry *buffer;
  DWORD i, nf;
  long         frac = 0;
  size_t        len = 0;
  long           io = 0;
  long    data_size = 0;
  long           nb = 0;
  long           bs = 0;
  long        times = 0;
  long  used_blocks = 0;
  long   free_space = 0;
  long        files = 0;
  long  terminators = 0;
  long    unused_bl = 0;
  long   occupation = 0;
  long   used_space = 0;
  long estim_blocks = 0;
  long         doff = 0;
  alloc_entry *buf = 0;
  alloc_entry entry = 0;
  float   available = 0;
  int    n_garbages = 0;
  int        silent = 0;
  int           err = 0;
  int             j = 0;
  int             k = 0;
  int         loops = 1;
  int         alloc = 0;

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
      return -1;
  }
  if (LOL_INVALID_MAGIC) {
      lol_error("lol %s: invalid file id [0x%x, 0x%x].\n",
	   me, (int)sb.reserved[0], (int)sb.reserved[1]);
      lol_error(LOL_FSCK_FMT);
      return -1;
  }

  nb = (long)sb.num_blocks;
  bs = (long)sb.block_size;
  if ((!(nb)) || (!(bs))) {
      lol_error(cantuse, me, cont);
      return -1;
  }

  free_space = (long)(nb * bs);
  memset((char *)number, 0, LOL_DF_MESSAGE);
  lol_ltostr((unsigned long)free_space, number);
  lol_fd_clear(before, after);
  sprintf(before, "Total  space: %s ", number);
  sprintf(after, "[%ld blocks, each %ld bytes]\n", nb, bs);
  lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);
  nf = sb.num_files;

  data_size  = (long)NAME_ENTRY_SIZE;
  data_size *= (long)(nb);
  io = lol_get_io_size(data_size, (long)NAME_ENTRY_SIZE);
  if (io <= 0) {
      lol_debug("lol_df: Internal error: io <= 0");
      return -1;
  }
  if (!(buffer = (struct lol_name_entry *)lol_malloc((size_t)(io)))) {
        buffer = (struct lol_name_entry *)temp;
        io     = 512 * NAME_ENTRY_SIZE;
  }
  else {
     alloc = 1;
  }

  times = data_size / io;
  frac  = (size_t)(data_size % io);
  k = io / NAME_ENTRY_SIZE;

  if (!(fp = fopen(cont, "r"))) {
       lol_error(cantread, me, cont);
       goto just_free;
  }

  doff = LOL_DENTRY_OFFSET_EXT(nb, bs);
  if (fseek (fp, doff, SEEK_SET)) {
      lol_error(cantread, me, cont);
      goto closefree;
  }

 dentry_loop:
  for (i = 0; i < times; i++) {

    if ((lol_fio((char *)buffer, io, fp, LOL_READ)) != io) {
         lol_error(cantread, me, cont);
         goto closefree;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...
       name_e = &buffer[j];
       if (!(name_e->filename[0])) {
	   continue;
       }
       files++;
       if ((name_e->i_idx < 0) || (name_e->i_idx >= nb)) {
	    err = 1;
       }
       if (name_e->file_size) {
            used_space += name_e->file_size;
	    estim_blocks += (name_e->file_size / bs);
            if (name_e->file_size % bs)
	        estim_blocks++;
       }
       else {
	  estim_blocks++; // Zero size occupy 1 block
       }
       // Do some checking while we are here..
       if ((lol_garbage_filename((char *)(name_e->filename)))) {
	     n_garbages++;
       } // end if suspicious name
       len = strlen((char *)(name_e->filename));
       if (len > LOL_FILENAME_MAXLEN) {
	   err = 1;
       }
    } // end for j
  } // end for i

  // Now the fractional data
  if ((frac) && (loops)) {
      times = 1;
      io = frac;
       k = io / NAME_ENTRY_SIZE;
      loops = 0;
      goto dentry_loop;
  } // end if frac

  // Read also the reserved blocks.
  data_size  = (long)ENTRY_SIZE;
  data_size *= (long)(nb);
  io = lol_get_io_size(data_size, (long)ENTRY_SIZE);
  times = data_size / io;
  frac  = (size_t)(data_size % io);
  k = io / ENTRY_SIZE;
  buf = (alloc_entry *)buffer;
  loops = 1;

 index_loop:
  for (i = 0; i < times; i++) {
    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         lol_error(cantread, me, cont);
         goto closefree;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...
        entry = buf[j];
       if (entry == LAST_LOL_INDEX) {
           terminators++;
           used_blocks++;
           continue;
       }
       if (entry == FREE_LOL_INDEX) {
           continue;
       }
       used_blocks++;
       if ((entry >= 0) && (entry < nb)) {
           continue;
       }
       err = 1;
    } // end for j
  } // end for i

  // Now the fractional data
  if ((frac) && (loops)) {
      times = 1;
      io = frac;
       k = io / ENTRY_SIZE;
      loops = 0;
      goto index_loop;
  } // end if frac

  if (alloc) {
    lol_index_free(LOL_STORAGE_ALL);
  }
  lol_try_fclose(fp);


  if ((terminators != nf) || (terminators != files)) {
#if LOL_TESTING
    printf("lol df: DEGUG: ERROR: nf = %u, files = %ld, terms = %ld\n", nf, files, terminators);
#endif
    err = 1;
  }

  occupation = (long)(used_blocks * bs);
  free_space = (long)(nb - used_blocks);
  unused_bl  = (long)(free_space);
  available  = (float)(free_space);
  free_space *= bs;
  available  /= ((float)(nb));
  available  *= ((float)(100.0));

  memset((char *)number, 0, LOL_DF_MESSAGE);
  lol_ltostr((unsigned long)free_space, number);
  lol_fd_clear(before, after);
  sprintf(before, "Unused space: %s ", number);
  sprintf(after, "[%ld blocks, %2.1f%% free]\n",
          unused_bl, available);
  lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);

  if (files) {

     available = 100.0 - available;
     memset((char *)number, 0, LOL_DF_MESSAGE);
     lol_ltostr((unsigned long)(occupation), number);
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
      (used_blocks > nb) || (nf != files)) {

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

closefree:
 lol_try_fclose(fp);
just_free:
 if (alloc) {
    lol_index_free(LOL_STORAGE_ALL);
 }
 return -1;
} // end lol_df
