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
 $Id: lol_cc.c, v0.40 2016/12/06 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
#ifdef HAVE_CONFIG_H
#include "../config.h"
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

#define LOL_FSCK_INDEX_BLOCK 1024
#define CONT_NOT_FOUND "cannot find container \'%s\'"
static const char      lol_fsck_cc[] = "lol cc";
static const char   lol_fsck_lolfs[] = "fsck.lolfs";
// Error messages
static const char   lol_fsck_susp_name[] = "suspicious filename: ";
static const char  lol_fsck_susp_names[] = "found %2.2f%% non-standard filenames";
static const char   lol_fsck_nentry_io[] = "cannot read entry number ";
static const char   lol_fsck_err_super[] = "cannot read header metadata";
static const char  lol_fsck_corr_super[] = "corrupted header metadata";
static const char   lol_fsck_incons_sb[] = "inconsistent container size";
static const char  lol_fsck_incons_dmd[] = "inconsistent directory metadata";
static const char  lol_fsck_incons_dir[] = "inconsistent directory size";
static const char   lol_fsck_wrong_dev[] = "unsupported media type";
static const char  lol_fsck_corr_entry[] = "corrupted file entry at: ";
static const char  lol_fsck_fatal_exit[] = "cannot continue, exiting..";
static const char   lol_fsck_exit_info[] = "must exit, run again with \'-d\' option";
static const char          lol_fsck_io[] = "I/O error";
static lol_indref  fsck_indref[LOL_FSCK_INDEX_BLOCK];
static long  res_blocks_by_size = 0;
static long res_blocks_by_count = 0;
static const size_t indref_siz = (sizeof(struct lol_indref_t) *
                                  LOL_FSCK_INDEX_BLOCK);
/* ****************************************************************
 * TEST #1:
 * lol_fsck_sb: verify the superblock/metadata self-consistency.
 *
 ****************************************************************** */
static int lol_fsck_sb(FILE *fp, const struct lol_super *sb,
                       const struct stat *st,
                       const char* cont, const char *me, const int v)
{

  char message[512];
  const DWORD bs = sb->bs;
  const DWORD nb = sb->nb;
  const DWORD nf = sb->nf;
  int  ret = 0;

  if (!(nb)) {
       return lol_status_msg(me, "no data blocks found", LOL_FSCK_FATAL);
  }
  if (!(bs)) {
       return lol_status_msg(me, "data block size is zero", LOL_FSCK_FATAL);
  }
  if (LOL_INVALID_MAGIC_PTR) {
      ret = LOL_FSCK_ERROR;
      memset((char *)message, 0, 512);
      sprintf(message, "invalid file id [0x%x, 0x%x]",
	      (int)sb->reserved[0], (int)sb->reserved[1]);
      lol_status_msg(me, message, ret);
  }
  if (nf > nb) {
     if (ret < LOL_FSCK_WARN)
         ret = LOL_FSCK_WARN;
      memset((char *)message, 0, 512);
      sprintf(message, "too many (%u) files found",
	     (unsigned int)(nf));
      lol_status_msg(me, message, LOL_FSCK_WARN);
      if (v) {
         memset((char *)message, 0, 512);
         sprintf(message, "          Maximum capacity is %u",
	        (unsigned int)(nb));
         lol_status_msg(me, message, LOL_FSCK_INFO);
      }
  }
  return (-ret);
} // end lol_fsck_sb
/* ******************************************************************
 * TEST #2:
 * lol_fsck_geom: compare the superblock/metadata information about
 *                the container's actual size.
 *
 ****************************************************************** */
static int lol_fsck_geom(FILE *fp, const struct lol_super *sb,
                         const struct stat *st,
                         const char* cont, const char *me, const int v)
{

  char message[512];
  char gsize_s[128];
  char  size_s[128];
  const  DWORD bs = sb->bs;
  const  DWORD nb = sb->nb;
  long   size = 0;
  long g_size = 0;
  mode_t mode = 0;
  int     ret = 0;

  size = lol_getsize((char *)cont,
                           (struct lol_super *)sb,
                           NULL, USE_SB_INFO);

  if (size < LOL_THEOR_MIN_DISKSIZE) {
      return lol_status_msg(me, lol_fsck_incons_sb,
                            LOL_FSCK_FATAL);
  }

  memset ((char *)gsize_s, 0, 128);
  memset ((char *)size_s,  0, 128);
  g_size = (long)LOL_DEVSIZE(nb, bs);
  mode = st->st_mode;
  lol_ltostr((ULONG)(g_size), gsize_s);
  lol_ltostr((ULONG)(size), size_s);

  if (S_ISREG(mode)) {
    // In this case the calculated size must be
    // _exactly_ the same as the sb data says.
    if (g_size != size) {
	ret = LOL_FSCK_ERROR;
        memset((char *)message, 0, 512);
        sprintf(message,
        "inconsistent container size (%s)\n          Should be %s",
        size_s, gsize_s);
        lol_status_msg(me, message, ret);
    }

  } // end if regular file
  else {
    // So, it is not a regular file
#ifdef HAVE_LINUX_FS_H
    // In Linux, the container may be a block device
    // and it may be bigger than the sb says
       if (!(S_ISBLK(mode))) {
	   ret = LOL_FSCK_FATAL;
	   lol_status_msg(me, lol_fsck_wrong_dev, ret);
       }
       else {
	  // It is a Linux block device
	 if (g_size > size) {

	     ret = LOL_FSCK_ERROR;
             memset((char *)message, 0, 512);
	     sprintf(message,
              "inconsistent container size (%s)", gsize_s);
	     lol_status_msg(me, message, ret);
	     if (v) {
                 memset((char *)message, 0, 512);
	         sprintf(message,
                 "          Should not be bigger than %s", size_s);
                 lol_status_msg(me, message, LOL_FSCK_INFO);
	     }
	 }
	 else {
	   if (v) {
              memset ((char *)message,  0, 512);
	      sprintf(message, "device \'%s\' geometry test passed", cont);
              lol_status_msg(me, message, LOL_FSCK_OK);
	   }
	 }

       } // end else not block dev

#else
    // Not Linux and not a regular file -> fatal
    ret = LOL_FSCK_FATAL;
    lol_status_msg(me, lol_fsck_wrong_dev, ret);

#endif
  } // end else regular file

  return (-ret);
} // end lol_fsck_geom
/* ******************************************************************
 * TEST #3:
 * lol_fsck_nent: directory name entry self-consistency check.
 *
 ****************************************************************** */
static int lol_fsck_nent(FILE *fp, const struct lol_super *sb,
                         const struct stat *st,
                         const char* cont, const char *me, const int v)
{

  char message[512];
  struct lol_name_entry nentry;
  const long bs = (long)sb->bs;
  const long nb = (long)sb->nb;
  const long nf = (long)sb->nf;
  size_t              len = 0;
  long         dentry_off = 0;
  long          cont_size = 0;
  long        dentry_area = 0;
  long        dentry_size = 0;
  long       num_dentries = 0;
  long              fract = 0;
  long         i = 0, idx = 0;
  long      num_corrupted = 0;
  long    assumed_fblocks = 0;
  long     actual_fblocks = 0;
  long            n_files = 0;
  long       n_suspicious = 0;
  float             total = 0;
  float            suspic = 0;
  float             ratio = 0;
  int       rval = 0, ret = 0;

  // Let's make some aliases
  cont_size   = (long)st->st_size;
  dentry_size = (long)(NAME_ENTRY_SIZE);
  dentry_off  = LOL_DENTRY_OFFSET_EXT(nb, bs);
  dentry_area = (long)(dentry_off + dentry_size);

  // We MUST have at least 1 entry to analyze.
  if (cont_size < (dentry_area + ENTRY_SIZE)) {
    // an earlier check has propably warned about this.
     return lol_status_msg(me, lol_fsck_incons_dir,
                           LOL_FSCK_FATAL);
  } // end if wrong size

  // Calculate how much we have dentry data
  dentry_area = cont_size - dentry_off;
  // subtract index area
  dentry_area -= (long)(nb * (long)(ENTRY_SIZE));
  // Now, the dentry area should be > 0 and an integer
  // multiple of NAME_ENTRY_SIZE
  if (dentry_area < dentry_size) {
      return lol_status_msg(me,
             "no files: directory corrupted",
             LOL_FSCK_FATAL);
  } // end if wrong size

  // How many are there?
  num_dentries = dentry_area / dentry_size;
  fract = dentry_area % dentry_size;

  if ((fract) || (num_dentries != nb)) {
     // at least dentry storage is corrupted
    if (ret < LOL_FSCK_ERROR)
        ret = LOL_FSCK_ERROR;
        lol_status_msg(me,
        lol_fsck_incons_dir, LOL_FSCK_ERROR);
  } // end if fract

  if ((fseek(fp, dentry_off, SEEK_SET))) {
       return lol_status_msg(me, lol_fsck_io,
                             LOL_FSCK_FATAL);
  }

  num_corrupted = 0;
  // Let's read and see what we have
  for (i = 0; i < num_dentries; i++) {

    if ((fread((char *)&nentry,
               (size_t)(NAME_ENTRY_SIZE), 1, fp)) != 1) {
      if (v) {
          memset((char *)message, 0, 512);
	  sprintf(message, "%s%ld", lol_fsck_nentry_io, i);
          lol_status_msg(me, message, LOL_FSCK_ERROR);
      }
          if (ret < LOL_FSCK_ERROR)
              ret = LOL_FSCK_ERROR;
	  continue;

     } // end if fread

    // Got one!
    idx = (long)(nentry.i_idx);
    // Check 1: if no name but file_size > 0   => error
    // Check 2: if no name but allocated index => error
    if (!(nentry.name[0])) {
        if ((nentry.fs) || (idx)) {
           // This is f*cked up, continue
	  if (v) {
             memset((char *)message, 0, 512);
   	     sprintf(message, "%s%ld", lol_fsck_corr_entry, i);
             lol_status_msg(me, message, LOL_FSCK_ERROR);
	  }
           if (ret < LOL_FSCK_ERROR)
               ret = LOL_FSCK_ERROR;
           num_corrupted++;
	  // TODO: Prompt for fix here.. this ghost file
      	  // Or auto-correct the error...

        } // end if file_size or index

	  // Seems to be fine but if there is no name
	  // there should not be reserved index either
	  // Also, the file_size should be 0

	continue;

    } // end if no name

    // So, there is a file
    n_files++;
    // Check if it seems to be ok
    // How about the size of the file?
    assumed_fblocks = (long)nentry.fs;
    if (assumed_fblocks) {
        assumed_fblocks /= bs;
        if (((long)nentry.fs) % bs)
            assumed_fblocks++;
    }
    else {
      assumed_fblocks = 1; // size 0 files reserve 1 block
    } // end else

    if ((assumed_fblocks > 0) && (assumed_fblocks <= nb)) {
        res_blocks_by_size += assumed_fblocks;
    }

    len = strlen((char *)nentry.name);

    if (len > LOL_FILENAME_MAXLEN) {
    // TODO & THINK: This is actually very severe error!
    // It may corrupt the i_idx and file_size fields...

       if (v) {
          nentry.name[LOL_FILENAME_MAXLEN] = '\0';
          memset((char *)message, 0, 512);
	  sprintf(message, "%s\'%s\'", lol_fsck_susp_name,
                                  (char *)(nentry.name));
          lol_status_msg(me, message, LOL_FSCK_ERROR);
       }
       if (ret < LOL_FSCK_ERROR)
           ret = LOL_FSCK_ERROR;
	 // TODO: prompt for fix. name is easy to truncate.
	 //       We should check the chain and file size too though..
         num_corrupted++;
	 continue;
    }

    if ((lol_garbage_filename((char *)(nentry.name)))) {
         n_suspicious++;
	 if (v) {

            memset((char *)message, 0, 512);
	    sprintf(message, "%s\'%s\'", lol_fsck_susp_name,
                                  (char *)(nentry.name));
	    // Give warning but we are not sure
	    // if this is actually corrupted.
            lol_status_msg(me, message, LOL_FSCK_WARN);
	 }
         if (ret < LOL_FSCK_WARN)
             ret = LOL_FSCK_WARN;

    } // end if garbage name


    if ((((long)(nentry.fs)) < 0) ||
          (idx < 0) || (idx >= nb))
    {
         num_corrupted++;
	 if (v) {
            memset((char *)message, 0, 512);
	    sprintf(message, "invalid file \'%s\' found",
                    (char *)(nentry.name));
            lol_status_msg(me, message, LOL_FSCK_ERROR);
	 }
         if (ret < LOL_FSCK_ERROR)
             ret = LOL_FSCK_ERROR;
         continue;
    }

    // reserved_blocks is the value of total reserved
    // blocks there SHOULD be according to metadata.

    if ((assumed_fblocks > nb)) {

        num_corrupted++;
	if (v) {
           memset((char *)message, 0, 512);
	   sprintf(message, "invalid file '%s\' found",
                             (char *)(nentry.name));
           lol_status_msg(me, message, LOL_FSCK_ERROR);
	}
        if (ret < LOL_FSCK_ERROR)
            ret = LOL_FSCK_ERROR;
       continue;
    }

    // Compare this to the number of blocks we find
    // TODO: We could actually fix the chain here by settin the flag
    rval = (long)lol_count_file_blocks(fp, sb,
	    nentry.i_idx, cont_size, &actual_fblocks, 0);

      if (!(rval)) { /* not error? */

         res_blocks_by_count += actual_fblocks;

         if (assumed_fblocks != actual_fblocks) {
	   // This should not happen, I think, because
	   // rval is 0, which means no error...
	    num_corrupted++;
	    if (v) {
               memset((char *)message, 0, 512);
	       sprintf(message, "invalid file '%s\' found",
                                (char *)(nentry.name));
               lol_status_msg(me, message, LOL_FSCK_ERROR);
	    }
            if (ret < LOL_FSCK_ERROR)
                ret = LOL_FSCK_ERROR;

          } // end if assumed_fblocks

         continue;

       } // end if ! rval
       else {
        // So, rval is -1 or 1 possibly. Either case, the file is
        // messed up...
          num_corrupted++;
	  if (v) {
             memset((char *)message, 0, 512);
             sprintf(message, "found corrupted file \'%s\'",
                              (char *)(nentry.name));
             lol_status_msg(me, message, LOL_FSCK_ERROR);
	  }
          if (ret < LOL_FSCK_ERROR)
              ret = LOL_FSCK_ERROR;
       } // end else !rval

    } // end for i

    if ((n_files > 3) && (n_suspicious)) {
       // Calculate how many filenames were suspicious
        suspic = (float)(n_suspicious);
        total  = (float)(n_files);
        ratio  = suspic / total;
	// We want to be silent, so let's not
	// bother the user unless we found
	// over 50% of files suspicious.
	rval = LOL_FSCK_OK;
        do {
	    if (ratio > 0.5) {
	        rval = LOL_FSCK_WARN;
		break;
	    } // end if over 50%
	    if (ratio > 0.3) {
	        rval = LOL_FSCK_INFO;
		break;
	    }
	} while (0);

	if (rval != LOL_FSCK_OK) {
	   if (v) {
              memset((char *)message, 0, 512);
	      ratio *= 100.0;
	      sprintf(message, lol_fsck_susp_names, ratio);
	      // Give warning but we are not sure if this is corrupted
              lol_status_msg(me, message, rval);
	   }
           if (ret < rval)
               ret = rval;
	} // end if we warn

    } // end if there were suspicious files

    if (num_corrupted) {
          memset((char *)message, 0, 512);
	  sprintf(message, "found %ld corrupted files", num_corrupted);
          lol_status_msg(me, message, LOL_FSCK_INFO);
    }

    // TODO: Super block is easy to fix, we just warn for now..
    if (nf != n_files) {
      // The number of files recorded into the header does not
      // match the number of files we found.
        return lol_status_msg(me,
                lol_fsck_incons_dmd, LOL_FSCK_ERROR);
    }

    return (-ret);
} // end lol_fsck_nent
/* *************************************************************** **
 * lol_fsck_index_crossref: helper function to lol_fsck_index
 *
 ****************************************************************** */
static long lol_fsck_index_crossref(alloc_entry *buf,
                                   const size_t num_ind, const long nb)
{

  int i, j, count;
  int ref_count = 0;
  long corr_ind = 0;
  alloc_entry a;
  const int num = (int)(num_ind);
  const int max = num - 1;

  for (i = 0; i < max; i++) {

    a = buf[i];
    count = 0;

    for (j = i+1; j < num; j++) {

      if (a == buf[j]) {

	  if ((a >= 0) && (a < nb)) {
	     count++;
	     // Make a record of this error
#if LOL_TESTING
	     printf("CHECK i = %d, j = %d, ref_count = %d\n", i, j, ref_count);
#endif
             fsck_indref[ref_count].i   = i;
             fsck_indref[ref_count].j   = j;
             fsck_indref[ref_count].val = a;
	     buf[i] = buf[j] = FREE_LOL_INDEX;
	     ref_count++;

	  } // end if duplicate was also allocated
	  //	} // end if i != j, found duplicate in different position
      } // end if a, found potential duplicate?
    } // end for inner loop

    // How many duplicated did we find?
    if (count) {
#if LOL_TESTING
      printf("Setting .num to value %d to ref_count %d\n",
             (int)count, (int)(ref_count - count));
#endif
      // Record the number of duplicates
      fsck_indref[(ref_count - count)].num = count;
      corr_ind++;
    } // end if count

  } // end for i

  return corr_ind;

} // end lol_fsck_index_crossref
/* *************************************************************** **
 * lol_fsck_calc_indexes: helper function to lol_fsck_index
 *
 ****************************************************************** */
static void lol_fsck_calc_indexes(const alloc_entry *buf, long *nf,
                                  long *nl, long *nc, long *nr,
                                  const long nb, const size_t num_ind)
{

  const int num = (int)(num_ind);
  int i;
  alloc_entry idx;

  for (i = 0; i < num; i++) {

    idx = buf[i];

    if ((idx >= 0) && (idx < nb)) {
       (*nr)++;
       continue;
    }
    if (idx == LAST_LOL_INDEX) {
       (*nl)++;
       continue;
    }
    if (idx == FREE_LOL_INDEX) {
       (*nf)++;
       continue;
    }

    (*nc)++;

  } // end for i

} // end lol_fsck_calc_indexes
/* *************************************************************** **
 * lol_fsck_index_report: report duplicate indices for lol_fsck_index
 *
 ****************************************************************** */
static void lol_cc_index_report(const long corr_ind,
                                const size_t io_bl, const long base)
{

  const long max = (long)(io_bl);
  long i, j, r, s = 0;
  long k = 0;
  long curr_off;
  long count = 0;
  int n;

  /*
   * corr_ind: has the number of indices which
   * have duplicates.
   *
   * io_bl: is the number of indices in one i/o block
   *    nb: is the number of blocks in container
   *
   *  int         num;
   *  alloc_entry   i;
   *  alloc_entry   j;
   *  alloc_entry val;
   */

  for (i = 0; i < max;) {

    n = fsck_indref[i].num; // This is how many copies of this
                            // index we have...
    if (count)
      printf("\n");

    curr_off = (long)(fsck_indref[i].i);
    curr_off += base;

    printf("Index at offset %ld has %d duplicate values (%d)\n",
	   curr_off, n, fsck_indref[i].val);

    printf("Duplicates are at offset(s): ");
    s = 0; r = n + i;
    for (j = i; j < r; j++) { // Read all copies

      k = (long)(fsck_indref[j].j);
      k += base;
      printf("[%ld] ", k);
      if (++s > 3) {
	printf("\n                             ");
        s = 0;
      }
    } // end for j

    i += n;
    //i++;
    count++;
    if (count >= corr_ind)
      break;

  } // end for i

 if (count)
   printf("\n");

} // lol_cc_index_report
/* *************************************************************** **
 * TEST #4:
 * lol_fsck_index: verify data index area self-consistency
 *                 and cross reference with other values.
 *
 ****************************************************************** */
static int lol_fsck_index(FILE *fp, const struct lol_super *sb,
                          const struct stat *st,
                          const char* cont, const char *me, const int v)
{
  const size_t io = LOL_FSCK_INDEX_BLOCK * ENTRY_SIZE;
  alloc_entry buffer[io];
  alloc_entry backup[io];
  char       message[512];
  char      size_str[128];

  const long   bs = (long)sb->bs;
  const long   nb = (long)sb->nb;
  const long   nf = (long)sb->nf;

  long n_dup_idx = 0;
  long data_size = 0;
  long real_size = 0;
  long     times = 0;
  long    offset = 0;
  long  base_off = 0;
  long  num_free = 0;
  long  num_last = 0;
  long  num_corr = 0;
  long   num_res = 0;
  long files_res = 0;
  ULONG  f_alloc = 0;
  long         i = 0;
  size_t    frac = 0;
  long  corr_ind = 0;
  int        ret = 0;
  int has_errors = 0;

  real_size = (long)st->st_size;
  if (real_size > LOL_TERABYTE) {
      ret = LOL_FSCK_INFO;
      if (v) {
         memset((char *)message, 0, 512);
         sprintf(message,
         "container \'%s\' is suspiciously large", cont);
         lol_status_msg(me, message, ret);
      }
  } // end if real_size

  real_size -= DISK_HEADER_SIZE;
  real_size -= (nb * bs);
  real_size -= (nb * ((long)(NAME_ENTRY_SIZE)));
  data_size  = (long)ENTRY_SIZE;
  data_size *= nb;

  if (data_size != real_size) {
      return lol_status_msg(me,
             "the index storage is corrupted",
             LOL_FSCK_FATAL);
  } // end if data_size

  // Ok, at least we have enough indices

  times = data_size / io;
  frac  = (size_t)(data_size % io);
  if (frac % ENTRY_SIZE)
        return lol_status_msg(me,
               lol_msg_list0[LOL_0E_INTERERR], LOL_FSCK_FATAL);

  offset = LOL_TABLE_START_EXT(nb, bs);
  if ((fseek(fp, offset, SEEK_SET))) {
      return lol_status_msg(me,
             lol_fsck_io, LOL_FSCK_FATAL);
  } // end if fseek

  for (i = 0; i < times; i++) {
     // Read LOL_FSCK_INDEX_BLOCK indices at a time
     // (This is most likely 4k or 8k of data)
     if ((lol_fio((char *)buffer, io, fp, LOL_READ)) != io) {
         return lol_status_msg(me,
                lol_fsck_io, LOL_FSCK_FATAL);
     } // end if cannot read

     // Cross reference this block of indices
     memset((char *)fsck_indref, 0, indref_siz);
     LOL_MEMCPY(backup, buffer, io);
     corr_ind = lol_fsck_index_crossref(buffer,
                LOL_FSCK_INDEX_BLOCK, nb);
     n_dup_idx += corr_ind;

     if ((corr_ind) && (v)) {
         printf("Found %ld indices with duplicates in block %ld\n", corr_ind, i);
         lol_cc_index_report(corr_ind, LOL_FSCK_INDEX_BLOCK, base_off);
     } // end if corr_ind

     // Calculate different types of indices
     lol_fsck_calc_indexes(backup, &num_free, &num_last, &num_corr,
                           &num_res, nb, LOL_FSCK_INDEX_BLOCK);
     base_off += LOL_FSCK_INDEX_BLOCK;
  } // end for i

  if (frac) {

    if ((lol_fio((char *)buffer, frac, fp, LOL_READ)) != frac) {
        return lol_status_msg(me, lol_fsck_io, LOL_FSCK_FATAL);
    } // end if cannot read

    frac /= ENTRY_SIZE;

    // Cross reference this block of indices
    memset((char *)fsck_indref, 0, indref_siz);
    LOL_MEMCPY(backup, buffer, io);
    corr_ind = lol_fsck_index_crossref(buffer, frac, nb);

    n_dup_idx += corr_ind;

    if ((corr_ind) && (v)) {
        printf("Found %ld indices with duplicates.\n", corr_ind);
        lol_cc_index_report(corr_ind, frac, base_off);
    } // end if corr_ind
    // Calculate different types of indices
    lol_fsck_calc_indexes(backup, &num_free, &num_last,
                          &num_corr, &num_res, nb, frac);
  } // end if frac

  /* Now, Let's report our findings... */

  // If everything is OK, then (num_res + num_last) should be equal
  // to the number of blocks found reserved when lol_fsck_nentry
  // calculated the file sizes.

  files_res = num_res + num_last;
  f_alloc = (ULONG)files_res;
  f_alloc *= bs;

  if (v) {
     lol_status_msg(me, "Index report:", LOL_FSCK_INFO);
  }
  if ((nf != num_last) || (n_dup_idx) ||
      (num_corr) || (files_res != res_blocks_by_count) ||
      (files_res != res_blocks_by_size)) {
      has_errors = LOL_FSCK_WARN;
    if (v) {
      lol_status_msg(me, "detected errors", has_errors);
    }
  } // end if nf

  if (v) {
    memset((char *)message, 0, 512);
    memset((char *)size_str, 0, 128);
    lol_ltostr(f_alloc, size_str);
    sprintf(message, "files reserve %s by index storage", size_str);
    lol_status_msg(me, message, LOL_FSCK_INFO);
  }

  if (files_res != res_blocks_by_count) {
      has_errors = LOL_FSCK_ERROR;
      if (v) {
         memset((char *)message, 0, 512);
	 memset((char *)size_str, 0, 128);
	 f_alloc = (ULONG)res_blocks_by_count;
	 f_alloc *= bs;
         lol_ltostr(f_alloc, size_str);
         sprintf(message,
		 "they actually reserve %s", size_str);
         lol_status_msg(me, message, has_errors);
      }
  } // end if files_res

  if ((files_res != res_blocks_by_size) &&
      (res_blocks_by_size != res_blocks_by_count)) {
       has_errors = LOL_FSCK_ERROR;
     if (v) {
         memset((char *)message, 0, 512);
	 memset((char *)size_str, 0, 128);
	 f_alloc = (ULONG)res_blocks_by_size;
	 f_alloc *= bs;
         lol_ltostr(f_alloc, size_str);
         sprintf(message,
		 "they should reserve %s", size_str);
         lol_status_msg(me, message, has_errors);
     }
  } // end if files_res

  if (num_corr) {
         has_errors = LOL_FSCK_ERROR;
     if (v) {
         memset((char *)message, 0, 512);
         sprintf(message, "found %ld corrupted indices", num_corr);
         lol_status_msg(me, message, has_errors);
     }
  } // end if num_corr

  if (n_dup_idx) {
    if (has_errors < LOL_FSCK_WARN)
        has_errors = LOL_FSCK_WARN;
      if (v) {
         memset((char *)message, 0, 512);
         sprintf(message, "found %ld duplicate indices", n_dup_idx);
         lol_status_msg(me, message, LOL_FSCK_WARN);
      }
  } // end if n_dup_idx

  if ((has_errors) && (!(v))) {
    // Recommend verbose mode
     lol_status_msg(me,
     "found errors, run again with option \'-d\'",
     LOL_FSCK_ERROR);
  } // end if

  if (has_errors > ret)
    ret = has_errors;

  return (-ret);

} // end lol_fsck_index
/* ****************************************************************** */
typedef int (*lol_check_t)(FILE *, const struct lol_super *,
                           const struct stat *, const char *,
                           const char *, const int);
/* ****************************************************************** */
typedef struct lol_check_func_t
{
  lol_check_t      func;
         char    *alias;
         char    *descr;

} lol_check_func;
/* ****************************************************************** */
static const lol_check_func lol_check_funcs[] =
{
    {lol_fsck_sb,    "header   ", "header metadata self consistency:"},
    {lol_fsck_geom,  "geometry ", "container geometry:"},
    {lol_fsck_nent,  "directory", "directory consistency:"},
    {lol_fsck_index, "index    ", "index cross reference:"},
    {NULL, NULL, NULL},
};
/* ****************************************************************** */
static const char params[] = "<container>";
static const char*   lst[] =
{
  /* If you EDIT this, you MUST edit lol_fsck_help too! */

  "\n  Example:",
  "          \'%s lol.db\'\n",
  "          This checks the container",
  "          \'lol.db\' for errors.\n",
  "  Use option \'-d\' to show more details",
  "  about container inspection like:",
  "          \'%s -d lol.db\'\n\n",
  "  There are 5 levels of information output:\n",
  "         - OK    (one check has has been passed).",
  "         - INFO  (some information, not error).",
  "         - WARN  (warning, something is may be wrong).",
  "         - ERROR (something that needs repair).",
  "         - FATAL (needs immediate repair).\n",
  "          Type: 'man %s' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
void lol_fsck_help(char *me) {
  const char lol[] = "lol";
  int i;

  puts(lst[0]);
  printf(lst[1], me);
  for (i = 2; i < 6; i++) {
    puts(lst[i]);
  }
  printf(lst[6], me);
  for (i = 7; i < 13; i++) {
    puts(lst[i]);
  }

  if (me[0] == 'l') {
     printf(lst[13], lol);
  }
  else {
     printf(lst[13], me);
  }
} // end lol_fsck_help
/* ******************************************************************
 * lol_cc: checks if a container has errors.
 ******************************************************************** */
int lol_cc (int argc, char *argv[]) {

  char       msg[512];
  struct      stat st;
  struct lol_super sb;

  FILE       *fp = 0;
  char       *me = argv[0];
  char     *cont = 0;
  char     *desc = 0;
  char     *curr = 0;
  int          i = 0;
  int         rv = 0;
  int        ret = 0;
  int    verbose = 0;
  char       a, b;

  a = me[0]; b = me[1];
  // We must have a proper name
  if ((!(a)) || (!(b))) {
      lol_errfmt(LOL_0E_INTERERR);
      return -1;
  }
  // Let's see who called us
  if ((a == 'c') && (b == 'c'))
    me = (char *)lol_fsck_cc;
  else
    me = (char *)lol_fsck_lolfs;


  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {
        lol_show_usage2(me);
        lol_fsck_help(me);
        return 0;
    }
    if (LOL_CHECK_VERSION) {
        lol_show_version2(me);
	return 0;
    }
    if (LOL_CHECK_DETAILS) {
       lol_errfmt2(LOL_2E_ARGMISS2, me, params);
       return -1;
    }
    if (argv[1][0] == '-') {
      if ((stat(argv[1], &st))) {
	 lol_errfmt2(LOL_2E_OPTION2, me, argv[1]);
         lol_ehelpf2(me);
         return -1;
      }
    }
  } // end if argc == 2

  if (argc == 3) {
    if (LOL_CHECK_DETAILS) {
       verbose = 1;
       cont = argv[2];
       goto action;
    }
  } // end if argc == 3

  if (argc != 2) {
      lol_show_usage2(me);
      puts("       Checks container for errors");
      lol_helpf2(me);
      return 0;
  }

  cont = argv[1];

action:

  memset((char *)msg, 0, 512);
  if ((stat(cont, &st))) {

      sprintf(msg, CONT_NOT_FOUND, cont);
      return lol_status_msg(me, msg, LOL_FSCK_FATAL);
  }
  if (st.st_size < LOL_THEOR_MIN_DISKSIZE) {
      sprintf(msg, "\'%s\' is too small to be a container", cont);
      return lol_status_msg(me, msg, LOL_FSCK_FATAL);
  }
  if (!(fp = fopen(cont, "r"))) {
      sprintf(msg, CONT_NOT_FOUND, cont);
      return lol_status_msg(me, msg, LOL_FSCK_FATAL);
  }
  if ((fread((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
       fclose(fp);
       return lol_status_msg(me, lol_fsck_io, LOL_FSCK_FATAL);
  } // end if fread

  while (lol_check_funcs[i].func) {

    desc = lol_check_funcs[i].descr;
    curr = lol_check_funcs[i].alias;

    rv = lol_check_funcs[i].func(fp, &sb, &st,
                                 cont, curr, verbose);
    if (!(rv)) {
       lol_status_msg(me, desc, LOL_FSCK_OK);
    }
    else {
       if (rv >= LOL_FSCK_FATAL) {
           fclose(fp);
           if (verbose) {
               return lol_status_msg(me, lol_fsck_fatal_exit, rv);
           }
           return lol_status_msg(me, lol_fsck_exit_info, rv);
       }
       // If it was not a fatal or internal error, we try to continue...

    } // end else no errors

    i++;
  } // end while funcs

  fclose(fp);
  return ret;
} // end lol_cc
