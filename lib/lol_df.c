/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

/*
 * $Id: lol_df.c, v0.20 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
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
#define LOL_DF_TEMP 256
int lol_df (int argc, char* argv[])
{
  char   temp[LOL_DF_TEMP * NAME_ENTRY_SIZE];
  char number[LOL_DF_MESSAGE];
  char before[LOL_DF_MESSAGE];
  char  after[LOL_DF_MESSAGE];
  struct lol_super sb;
  struct      stat st;
  size_t         mem = 0;
  lol_nentry *buffer = 0;
  lol_nentry *nentry = 0;
  FILE           *fp = 0;
  char *cont =0, *me = 0;
  alloc_entry   *buf = 0;
  alloc_entry  entry = 0;
  float        avail = 0;
  long  i=0,io=0, nb = 0;
  long  frac = 0, bs = 0;
  long       nb_used = 0;
  long      nb_estim = 0;
  long     nb_unused = 0;
  long    free_space = 0;
  long    used_space = 0;
  long     data_size = 0;
  long         terms = 0;
  long         files = 0;
  long         times = 0;

  int   j=0, k=0, nf = 0;
  int           susp = 0;
  int         silent = 0;
  int            err = 0;
  int          loops = 1;
  int          alloc = 0;

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

  nb = (long)sb.nb;
  bs = (long)sb.bs;
  if ((!(nb)) || (!(bs))) {
      lol_error(cantuse, me, cont);
      return -1;
  }

  free_space = (long)(nb * bs);
  memset((char *)number, 0, LOL_DF_MESSAGE);
  lol_ltostr(free_space, number);
  lol_fd_clear(before, after);
  sprintf(before, "Total  space: %s ", number);
  sprintf(after, "[%ld blocks, each %ld bytes]\n", nb, bs);
  lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);
  nf = (int)sb.nf;

  data_size  = (long)NAME_ENTRY_SIZE;
  data_size *= (long)(nb);
  io = lol_get_io_size(data_size, (long)NAME_ENTRY_SIZE);
  if (io <= 0) {
      lol_debug("lol_df: Internal error: io <= 0");
      return -1;
  }
  if (!(buffer = (lol_nentry *)lol_malloc((size_t)(io)))) {
        buffer = (lol_nentry *)temp;
        io     = LOL_DF_TEMP * NAME_ENTRY_SIZE;
  }
  else {
     mem = (size_t)(io);
     alloc = 1;
  }

  times = data_size / io;
  frac  = (size_t)(data_size % io);
  k = io / NAME_ENTRY_SIZE;

  if (!(fp = fopen(cont, "r"))) {
       lol_error(cantread, me, cont);
       goto just_free;
  }
  i = LOL_DENTRY_OFFSET_EXT(nb, bs);
  if (fseek (fp, i, SEEK_SET)) {
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
       nentry = &buffer[j];
       if (!(nentry->filename[0])) {
	   continue;
       }
       files++;
       if ((nentry->i_idx < 0) || (nentry->i_idx >= nb)) {
	    err = 1;
       }
       if (nentry->file_size) {
            used_space += nentry->file_size;
	    nb_estim += (nentry->file_size / bs);
            if (nentry->file_size % bs)
	        nb_estim++;
       }
       else {
	  nb_estim++; // Zero size occupy 1 block
       }
       // Do some checking while we are here..
       if ((lol_garbage_filename((char *)(nentry->filename)))) {
	     susp++;
       } // end if suspicious name
       if ((strlen((char *)(nentry->filename)))
                    >= LOL_FILENAME_MAX) {
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
           terms++;
           nb_used++;
           continue;
       }
       if (entry == FREE_LOL_INDEX) {
           continue;
       }
       nb_used++;
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
     lol_free(mem);
  }
  lol_try_fclose(fp);

  if ((terms != nf) || (terms != files)) {
       err = 1;
  }

  io = (long)(nb_used * bs);
  free_space = (long)(nb - nb_used);
  nb_unused  = (long)(free_space);
  avail  = (float)(free_space);
  free_space *= bs;
  avail  /= ((float)(nb));
  avail  *= ((float)(100.0));

  memset((char *)number, 0, LOL_DF_MESSAGE);
  lol_ltostr(free_space, number);
  lol_fd_clear(before, after);
  sprintf(before, "Unused space: %s ", number);
  sprintf(after, "[%ld blocks, %2.1f%% free]\n",
          nb_unused, avail);
  lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);

  if (files) {

     avail = 100.0 - avail;
     memset((char *)number, 0, LOL_DF_MESSAGE);
     lol_ltostr(io, number);
     lol_fd_clear(before, after);
     sprintf(before, "Used   space: %s ", number);
     sprintf(after, "[%ld bytes in %ld blocks, %2.1f%% used]\n",
             used_space, nb_used, avail);
     lol_align(before, after, LOL_DF_ALIGN, LOL_STDOUT);
  }
  else {
    puts("No files");
  }

  if (nb_used) {
    if (!(files))
        err = 1;
  }
  if (nb_estim != nb_used) {
      err = 1;
  }
  if ((used_space > io)  || (err) || (nf > nb_used) ||
      (nb_used > nb) || (nf != files)) {
      lol_error("lol %s: container \'%s\' has errors.\n", me, cont);
      lol_error(LOL_FSCK_FMT);
      return -1;
  }
  if (!(silent)) {
     if (susp > 5) {

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
    lol_free(mem);
 }
 return -1;
} // end lol_df
#undef LOL_DF_TEMP
