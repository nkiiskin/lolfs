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

/* $Id: lolfs.c, v0.30 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $" */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_LIMITS_H
#ifndef _LIMITS_H
#include <limits.h>
#endif
#endif
#ifdef HAVE_ERRNO_H
#ifndef _ERRNO_H
#include <errno.h>
#endif
#endif
#ifdef HAVE_SYS_TYPES_H
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
#endif
#ifdef HAVE_SYS_STAT_H
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#endif
#ifdef HAVE_UNISTD_H
#ifndef _UNISTD_H
#include <unistd.h>
#endif
#endif
#ifdef HAVE_TIME_H
#ifndef _TIME_H
#include <time.h>
#endif
#endif
#ifdef HAVE_STDIO_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
#endif
#ifdef HAVE_STDLIB_H
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#endif
#ifdef HAVE_STRING_H
#ifndef _STRING_H
#include <string.h>
#endif
#endif
#ifdef HAVE_FCNTL_H
#ifndef _FCNTL_H
#include <fcntl.h>
#endif
#endif
#ifdef HAVE_SIGNAL_H
#ifndef _SIGNAL_H
#include <signal.h>
#endif
#endif
#ifdef HAVE_SYS_IOCTL_H
#ifndef _SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#endif
#ifdef HAVE_LINUX_FS_H
#ifndef _LINUX_FS_H
#include <linux/fs.h>
#endif
#endif
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif

// TODO: FIX lol_remove_nentry: It reads the index from the disk!
// TODO: Make use of new lol_FILE members: n_off, p_len and f_len
// TODO: add global lol_FILE[] buffer instead of mallocing mem for files.
// TODO: lolfs.c:    buffer lol_read_nentry !!! (SLOWS DOWN lol_fopen... !)
// TODO: lol_getsize should return some information when error, not just -1
// TODO: inner loop counters to int
// TODO: Fix lol_cc.c --> buffer the i/o
// TODO: lol_file.h: add last free nentry index to sb
// TODO: lol_cc.c:   When invalid file found -> get details
/* ********************************************************** */
// Some globals
/* ********************************************************** */
int lol_errno = 0;
int lol_buffer_lock = 0;
const char lol_mode_ro[] = "r";
const char lol_mode_rw[] = "r+";
const char* lol_prefix_list[] = {
  ": ",
  ": info, ",
  ": warning, ",
  ": error, ",
  ": fatal error, ",
  ": INTERNAL ERROR, ",
  NULL,
};
const char* lol_tag_list[] = {
  "\x1b[32m[OK]\x1b[0m\n",
  "\x1b[34m[INFO]\x1b[0m\n",
  "\x1b[35m[WARNING]\x1b[0m\n",
  "\x1b[31m[ERROR]\x1b[0m\n",
  "\x1b[31m[FATAL]\x1b[0m\n",
  "\x1b[31m[INTERNAL]\x1b[0m\n",
  NULL,
};
const lol_size lol_sizes[] = {
    {LOL_BYTEBYTE, "Bytes"},
    {LOL_KILOBYTE, "Kb"},
    {LOL_MEGABYTE, "Mb"},
    {LOL_GIGABYTE, "Gb"},
    {LOL_TERABYTE, "Tb"},
};

const char *lol_msg_list0[] = {
  LOL_FSCK_FMT,
  LOL_INTERERR_FMT,
  NULL,
};

const char *lol_msg_list1[] = {
  LOL_OOM_FMT,
  LOL_IOERR_FMT,
  LOL_SYNTAX_FMT,
  LOL_IMAGIC_FMT,
  LOL_HELP_FMT,
  LOL_HELP2_FMT,
  LOL_INTERE_FMT,
  NULL,
};
const char *lol_msg_list2[] = {

  LOL_EREAD_FMT,
  LOL_EWRITE_FMT,
  LOL_EUSE_FMT,
  LOL_ECP_FMT,
  LOL_ENODIR_FMT,
  LOL_ENOENT_FMT,
  LOL_EOPT_FMT,
  LOL_EARG_FMT,
  LOL_ECONT_FMT,
  LOL_NOSPC_FMT,
  LOL_OWPMT_FMT,
  LOL_EFULL_FMT,
  LOL_EIO_FMT,
  LOL_EACCES_FMT,
  LOL_INVSRC_FMT,
  LOL_OWCONT_FMT,
  LOL_EFBIG_FMT,
  LOL_EFUNC_FMT,
  LOL_EARG2_FMT,
  LOL_EOPT2_FMT,
  NULL,
};

const char *lol_msg_list3[] = {

  LOL_VERS_FMT,
  LOL_VERS2_FMT,
  NULL,

};
const char *lol_msg_list5[] = {
  LOL_USAGE_FMT,
  LOL_USAGE2_FMT,
  NULL,
};


const char lol_version[] = LOLFS_VERSION;
const char lol_copyright[] = LOLFS_COPYRIGHT;
/** **********************************************************
 *   TODO: Should make a better check for all these possible
 *        combinations... Not even sure if these are correct
 *
 * ******************************************************* **/
char lol_mode_combinations[MAX_LOL_OPEN_MODES][14][5] = {
  {{"r"},{"rb"},{"br"},{"\0"}},
  {{"r+"},{"rw"},{"wr"},{"rb+"},
   {"r+b"},{"br+"},{"b+r"},{"rwb"},
   {"rbw"},{"wrb"},{"wbr"},{"brw"},
   {"bwr"},{"\0"}},
  {{"w"},{"wb"},{"bw"},{"\0"}},
  {{"w+"},{"wb+"},{"w+b"},{"\0"}},
  {{"a"}, {"ab"},{"ba"},{"\0"}},
  {{"a+"},{"ab+"},{"a+b"},{"ar+"},{"a+r"},
   {"arb+"},{"abr+"},{"a+rb"},{"a+br"},{"\0"}},
};
/* ********************************************************** */
const struct lol_open_mode lol_open_modes[] =
{
    { .n = 0, .m = "r"  },
    { .n = 1, .m = "r+" },
    { .n = 2, .m = "r+" },
    { .n = 3, .m = "r+" },
    { .n = 4, .m = "r+" },
    { .n = 5, .m = "r+" },
};
/* ********************************************************** */
alloc_entry *lol_index_buffer = 0;
alloc_entry  lol_storage_buffer[LOL_STORAGE_SIZE+1];
const char lol_valid_filechars[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.- _~:!+#%[]{}?,;&()=@";
struct sigaction lol_sighand_store[LOL_NUM_SIGHANDLERS+1];
static const int       lol_signals[LOL_NUM_SIGHANDLERS + 1] = {
                       SIGHUP, SIGINT, SIGTERM, SIGSEGV, 0
};
/* ********************************************************** */
int lol_memset_indexbuffer(const alloc_entry value,
                            const size_t num)   {

  alloc_entry i = 0;
  if (!(lol_index_buffer)) {
#if LOL_TESTING
    lol_error("internal error in lol_memset_indexfuffer\n");
    lol_debug("buffer is NULL");
#endif
     return -1;
  }
  for (; i < num; i++) {
       lol_index_buffer[i] = value;
  }
  return 0;
} // end lol_memset_indexbuffer
/* ********************************************************** */
void lol_restore_sighandlers() {
  int i, j, ret;

   for (i = 0; i < LOL_NUM_SIGHANDLERS; i++) {
        j = 0;
        do {
           ret = sigaction(lol_signals[i],
                 &lol_sighand_store[i],  NULL);

           if (!(ret)) {
#if LOL_TESTING
	   printf("Restored signal %d\n", i);
#endif
	   break;
	  }
        } while (++j < 5); // if fails, try 4 more times
    } // end for i
} // end lol_restore_sighandlers
/* ********************************************************** */
static void lol_freemem_hdl(int sig, siginfo_t *so, void *c)
{
  int status = 0;
  if (so) {
     status = so->si_status;
  }
  if ((lol_index_buffer) &&
      (lol_buffer_lock == 2)) {
       free (lol_index_buffer);
  }
  fflush(0);
  exit (status);
}
/* ********************************************************** */
int lol_setup_sighandlers() {

    int i, j, k;
    int ret;
    struct sigaction sa;

    memset (&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = &lol_freemem_hdl;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigfillset(&sa.sa_mask);

    for (i = 0; i < LOL_NUM_SIGHANDLERS; i++) {
          memset (&lol_sighand_store[i], 0, sizeof(sa));

          if ( sigaction(lol_signals[i], &sa, &lol_sighand_store[i]) ) {
	      // Cannot handle this signal
	      // Rollback all...
	      for (j = 0; j < i; j++) {
		k = 0;
		do {
                     ret = sigaction(lol_signals[j],
                           &lol_sighand_store[j],  NULL);

		     if (!(ret)) {
		       break;
		     }
		} while (++k < 4);
	      } // end for j
              return -1;
          }
#if LOL_TESTING
	  printf("Set signal %d\n", i);
#endif
    }
  return 0;
} // end lol_setup_sighandlers
/* **********************************************************
 * lol_rgetsize: PRIVATE FUNCTION, USE WITH CAUTION!
 * Return the size (in bytes) of a given raw block device
 * or regular file or link to a regular file or -1, if error
 * or if size is too small to be a container.
 * Most likely must be run as root if it is ablock device.
 *
 *  NOTE:
 *  The function may be called with or without 'sb' and/or 'sta'
 *  if or if not those information(s) are needed.
 * ********************************************************** */
long lol_rgetsize(const char *device, lol_meta *sb, struct stat *sta) {

  struct stat st;
  mode_t mode;
  long ret = -1;
  long stsize;
#ifdef HAVE_LINUX_FS_H
  long size = 0;
#endif
  int fd;

  if (!(device))
    return -1;
  if (stat(device, &st))
    return LOL_ERR_ENOENT;

  stsize = (long)st.st_size;
  if (stsize < LOL_THEOR_MIN_DISKSIZE) {
     return -1;
  }
  if (sta) {
    LOL_MEMCPY(sta, &st, sizeof(st));
  }
  mode = st.st_mode;
  if (!(sb)) { // Return fast if sb info is not recuired
    if (S_ISREG(mode)) {
       return stsize;
    }
#ifdef HAVE_LINUX_FS_H
    if (!(S_ISBLK(mode))) {
       return -1;
    }
#else
      return -1;
#endif
  } // end if sb info is not needed

  fd = open(device, O_RDONLY);
  if (fd < 0) {
    return -1;
  }
  if (sb) {
    if ((read(fd, (char *)sb, DISK_HEADER_SIZE)) !=
       DISK_HEADER_SIZE) {
       goto error;
    }
  }
  if (S_ISREG(mode)) {
       ret = stsize;
       goto error;
  }
#ifdef HAVE_LINUX_FS_H
  if (!(S_ISBLK(mode))) {
      goto error;
  }
  if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
      goto error;
  }
  if (size < LOL_THEOR_MIN_DISKSIZE) {
      goto error;
  }
  ret = size;
#else
  ret = -1;
#endif
error:
  close(fd);
  return ret;
} // end lol_rgetsize
/* **********************************************************
 * lol_getsize: PRIVATE FUNCTION, USE WITH CAUTION!
 * Return the size (in bytes) of a given lol container.
 * NOTE: This returns the 'outer' size of the container
 * file or device, not the 'inner' size, which is available
 * to store files.
 *
 * NOTE:
 * In input, only the parameter 'sta' is not mandatory,
 * all the other fields are needed and may not be NULL.
 * The parameter func must be either USE_SB_INFO or
 * RECUIRE_SB_INFO. In later case the parameter 'sb'
 * will be filled with values obtained from the container.
 * In the former case, the function will use the parame-
 * ters given in 'sb' to evaluate the size.
 *
 * Return value:
 * < 0  : error
 * >= 0 : the size allocated for the container file or device.
 * ********************************************************** */
long lol_getsize(const char *name, lol_meta *sb,
		 struct stat *sta, int func)

 {
  struct stat st;
  long size;
  long bs;
  long nb;
  long min_size;

  if ((!(name)) || (!(sb)))
     return -1;

  switch (func) {

          case RECUIRE_SB_INFO:
	    size = lol_rgetsize(name, sb, &st);
	    break;
          case USE_SB_INFO:
	    size = lol_rgetsize(name, NULL, &st);
	    break;

          default:
            return -1;

  } // end switch

    if (size == LOL_ERR_ENOENT) {
        return LOL_ERR_ENOENT;
    }
    if (size < LOL_THEOR_MIN_DISKSIZE) {
        return -1;
    }
    bs = (long)sb->bs;
    nb = (long)sb->nb;
    if ((bs < 1) || (nb < 1)) {
        return -1;
    }
    if (LOL_INVALID_MAGIC_PTR) {
       return -1;
    }
    min_size = (long)LOL_DEVSIZE(nb, bs);
    if (sta) {
       LOL_MEMCPY(sta, &st, sizeof(st));
    }
    if ((S_ISREG(st.st_mode))) {
       if (size == min_size) {
           return size;
       }
       else {
         return -1;
       }
    } // end if regular file

#ifdef HAVE_LINUX_FS_H
    else {

        if ((S_ISBLK(st.st_mode))) {
            if (size >= min_size) {
	       return min_size;
            }

            else {
              return -1;
            }

          } // end if block device
    }
#endif

    /* If the file is not a regular file
       or a block device, we can't use it! */
   return -1;
} // end lol_getsize
/* **********************************************************
 * lol_valid_sb: PRIVATE FUNCTION
 * Fast check if the sb seems to be consistent.
 *
 * Return value:
 * <0 : error
 *  0 : success
 * ********************************************************** */
int lol_valid_sb(const lol_FILE *op) {

  ULONG size;

  if (!(op))
    return -1;
  if ((!(op->sb.nb)) || (!(op->sb.bs)))
    return -2;
  if (op->cs < LOL_THEOR_MIN_DISKSIZE)
    return -3;
  size = (ULONG)LOL_DEVSIZE(op->sb.nb, op->sb.bs);
  if (op->cs != size)
     return -4;
  if (size != op->cinfo.st_size)
    return -5;
  if ((LOL_CHECK_MAGIC(op)))
    return -6;
  if ((op->sb.nf > op->sb.nb))
    return -7;

  return 0;
} // end lol_valid_sb
/* **********************************************************
 * lol_check_corr: PRIVATE FUNCTION
 * Fast check of possible file/container corruption
 * Param mode is or'd value of LOL_CHECK_SB / LOL_CHECK_FILE
 * NOTE: The file is assumed to be open. If it's not, then
 *       error will be returned.
 *
 * Return value:
 * <0 : error
 *  0 : success
 * ********************************************************** */
int lol_check_corr(const lol_FILE *op, const int mode) {

  DWORD bs;
  DWORD nb;
  DWORD nf;
  ULONG fs;
  ULONG space;
  alloc_entry f_idx;
  alloc_entry n_idx;

  if (!(op))
    return LOL_ERR_PTR;
  if (!(op->dp))
    return LOL_ERR_BADFD;
  if ((mode & LOL_CHECK_SB)) {
    if ((lol_valid_sb(op)))
       return LOL_ERR_CORR;
  } // end if LOL_CHECK_SB

  if ((mode & LOL_CHECK_FILE)) {

    bs = op->sb.bs;
    nb = op->sb.nb;
    fs = op->nentry.fs;

    space = (ULONG)(((ULONG)(nb)) * ((ULONG)(bs)));
    if (fs > space)
      return LOL_ERR_CORR;
    if (!(op->file)) // unnecessary?
      return LOL_ERR_CORR;
    if (!(op->file[0]))
      return LOL_ERR_CORR;
    if (!(op->nentry.name[0]))
      return LOL_ERR_CORR;

    f_idx = op->nentry.i_idx;
    n_idx = op->n_idx;

    if ((f_idx < 0) || (f_idx >= nb))
      return LOL_ERR_CORR;
    if ((n_idx < 0) || (n_idx >= nb))
      return LOL_ERR_CORR;
    nf = op->sb.nf;
    if ((!(nf)) || (nf > nb))
      return LOL_ERR_CORR;
  } // end if LOL_CHECK_FILE

  return LOL_OK;
} // end lol_check_corr
/* **********************************************************
 * lol_remove_nentry: PRIVATE FUNCTION
 * Remove a name entry from the directory.
 * (Does not touch index chain, except the first index!)
 * 
 * Return value:
 * -1 : error
 *  0 : success
 * ********************************************************** */
int lol_remove_nentry(lol_FILE *op,
                      const DWORD nentry, int remove_idx) {

  struct lol_name_entry p;
  FILE *f = op->dp;
  const DWORD nb = op->sb.nb;
  const DWORD bs = op->sb.bs;

  fpos_t old_pos, tmp;
  long offset = op->dir;
  long off = NAME_ENTRY_SIZE;
  alloc_entry idx, v;
  int ret = -1;

  if (nentry >= nb)
      return -1;
  if (fgetpos(f, &old_pos))
      return -1;

  off *= nentry;
  offset += off;
  if (fseek(f, offset, SEEK_SET)) {
      goto error;
  }
  if (remove_idx) {
    // Free the index also
    if (fgetpos(f, &tmp))
        goto error;
    if (fread((char *)&p, NAME_ENTRY_SIZE, 1, f) != 1)
      goto error;
    idx = p.i_idx;
    if ((idx < 0) || (idx >= nb)) {
      // invalid index, we remove only the name
	goto skip_index;
    }
    offset = (long)LOL_INDEX_OFFSET(nb, bs, idx);
    if (fseek(f, offset, SEEK_SET))
        goto error;
    v = FREE_LOL_INDEX;
    if (fwrite((char *)&v, ENTRY_SIZE, 1, f) != 1)
        goto error;
  skip_index:
    if (fsetpos(f, &tmp))
        goto error;
  } // end if delete index too

  memset((char *)&p, 0, NAME_ENTRY_SIZE);
  if (fwrite((char *)&p, NAME_ENTRY_SIZE, 1, f) != 1)
      goto error;

  ret = 0;
error:
  fsetpos(f, &old_pos);
  return ret;
} // end lol_remove_nentry
/* **********************************************************
 * lol_get_index_value: PRIVATE FUNCTION
 * Return value of an index.
 * 
 * Return value:
 * LOL_FALSE if error, otherwise the index
 * ********************************************************** */
alloc_entry lol_get_index_value(FILE *f, const DWORD nb,
				const DWORD bs,
                                const alloc_entry idx)
{
  fpos_t p;
  long      off = 0;
  alloc_entry v = 0;
  alloc_entry ret = LOL_FALSE;

  if ((!(f)) || (idx < 0) || (idx >= nb) ||
      (!(nb)) || (!(bs))) {
      return LOL_FALSE;
  }
  if (fgetpos(f, &p))
      return LOL_FALSE;
  off = (long)LOL_INDEX_OFFSET(nb, bs, idx);
  if (fseek(f, off, SEEK_SET))
      goto error;
  if ((fread((char *)&v, ENTRY_SIZE, 1, f)) != 1)
      goto error;
  ret = v;
error:
    fsetpos(f, &p);
    return ret;
} // end lol_get_index_value
/* **********************************************************
 * lol_set_index_value: PRIVATE FUNCTION
 * Set value of an index.
 * 
 * Return value:
 * -1 : error
 *  0 : success
 * ********************************************************** */
int lol_set_index_value(lol_FILE *op, const alloc_entry val)
{
  FILE *f = op->dp;
  fpos_t p;
  long off;
  const DWORD nb = op->sb.nb;
 // const DWORD bs = op->sb.bs;
  const alloc_entry idx = op->nentry.i_idx;
  int  ret = -1;

  // Don't allow illegal values, sort them out..
  if ((val == LAST_LOL_INDEX) ||
      (val == FREE_LOL_INDEX)) {
      goto legal;
  }
  if ((val < 0) || (val >= nb))
      return -1;
legal:
    if (fgetpos(f, &p))
        return -1;
    off = op->idxs;
    off += (idx * ENTRY_SIZE);
    if (fseek(f, off, SEEK_SET))
        goto error;
    if (fwrite((char *)&val, ENTRY_SIZE, 1, f) != 1)
        goto error;

    ret = 0;
error:
    fsetpos(f, &p);
    return ret;
} // end lol_set_index_value
/* **********************************************************
 * lol_index_malloc: PRIVATE FUNCTION
 * Allocate a buffer for indexes in i/o -operations.
 * NOTE: sets lol_errno
 * Return value:
 * <0 : error code
 *  0 : success
 * ********************************************************** */
int lol_index_malloc(const size_t num) {

  const size_t es = (size_t)(ENTRY_SIZE);
  const size_t  p = num + 1;
  // size_t i;
  alloc_entry *lsb;
  size_t bytes;

#if LOL_TESTING
   printf("lol_index_malloc: lol_buffer_lock = %d\n", lol_buffer_lock);
#endif


  if (lol_buffer_lock)
     LOL_ERRET(EBUSY, LOL_ERR_BUSY);

  // Ok, we can allocate
  if (!(num))
     LOL_ERRET(EINVAL, LOL_ERR_PARAM);

  if (num <= (LOL_STORAGE_SIZE)) {
      // Clear first (Fake clear)

      lsb = lol_storage_buffer;
      lsb[LOL_STORAGE_SIZE] = LAST_LOL_INDEX;
      lsb[num-1] = lsb[num] = LAST_LOL_INDEX;

      /*
      for (i = 0; i < p; i++) {
        lol_storage_buffer[i] = LAST_LOL_INDEX;
      }
      */

      lol_index_buffer = lol_storage_buffer;
      lol_buffer_lock = 1;
      return LOL_OK;
  } // end if small enough

  // So, we must allocate dynamic mem
   bytes  = p * es;
#if LOL_TESTING
   printf("lol_index_malloc: trying to allocate %ld bytes\n", (long)(bytes));
#endif
   if (!(lol_index_buffer = (alloc_entry *)malloc(bytes)))
      LOL_ERRET(ENOMEM, LOL_ERR_MEM);

#if LOL_TESTING
   puts("lol_index_malloc: Succes!");
#endif

   // Set handlers
   if ((lol_setup_sighandlers())) {
        free (lol_index_buffer);
        lol_errno = EAGAIN;
        return LOL_ERR_SIG;
   }

   // Clear memory (fake)
    lol_index_buffer[p-1] = LAST_LOL_INDEX;
    lol_index_buffer[p-2] = LAST_LOL_INDEX;

    /*
   for (i = 0; i < p; i++) {
     lol_index_buffer[i] = LAST_LOL_INDEX;
   }
    */

   lol_buffer_lock = 2;
   return LOL_OK;
} // end lol_index_malloc
/* **********************************************************
 * lol_index_free: PRIVATE FUNCTION
 * Free buffer allocated by lol_index_malloc
 * NOTE: sets lol_errno
 * Return value:
 * N/A
 * ********************************************************** */
void lol_index_free(const size_t amount) {
 if (!(lol_buffer_lock))
     return;
 if (amount > (LOL_STORAGE_SIZE)) {
   if ((lol_index_buffer) && (lol_buffer_lock == 2)) {
         free (lol_index_buffer);
         lol_restore_sighandlers();
   }
 } // end if it was dynamic mem
 // So, it must be non-dynamic mem
 lol_buffer_lock = 0;
} // end lol_index_free
/* **********************************************************
 * lol_malloc:
 *
 * Allocate a buffer like malloc but uses preallocated.
 * buffer if the request is small enough.
 * Also sets signal handlers (if needed).
 *
 * NOTE: Free this memory using lol_free
 *
 * Return value:
 * A pointer to the allocated memory.
 * NULL if error
 * ********************************************************** */
void* lol_malloc(const size_t bytes) {
  size_t entries = 0;
  size_t frac    = 0;
  const size_t e_size = (size_t)(ENTRY_SIZE);

  if ((lol_buffer_lock) || (!(bytes)))
    return NULL;
  // We use lol_index_malloc, which allocates
  // memory in ENTRY_SIZE sized units.
  LOL_CALC_ENTRIES(entries, bytes, e_size, frac);

#if LOL_TESTING
  printf("lol_malloc: trying to lol_index_malloc(%ld)\n",
         (long)(entries));
#endif
  if ((lol_index_malloc(entries)))
     return NULL;

  return (void *)(lol_index_buffer);
} // end lol_malloc
/* ********************************************************** */
void lol_free(const size_t size) {
  if ((!(lol_buffer_lock)) || (!(size)))
     return;
  if (lol_buffer_lock == 2) {
      lol_index_free(LOL_STORAGE_ALL);
      return;
  }
  lol_index_free(1);
} // end lol_free
/* ********************************************************* *
 * lol_ifcopy:
 * Write given index value to file stream.
 * (The argument 'times' tells how many times we write
 *  the index value 'val' to the stream).
 *
 * Return value:
 * The number of times the 'val' was written.
 *********************************************************** */
size_t lol_ifcopy(const alloc_entry val, const size_t times, FILE *s)
{
  alloc_entry buf[LOL_DEFBUF];
  const size_t es = (size_t)(ENTRY_SIZE);
  const size_t bytes = times * es;
  size_t i, tm, fr, ret = 0;

  if ((!(s)) || (!(times)))
     return 0;
  for (i = 0; i < LOL_DEFBUF; i++) {
     buf[i] = val;
  }
  tm = bytes / LOL_DEFBUF;
  fr = bytes % LOL_DEFBUF;
  for (i = 0; i < tm; i++) {
    if ((fwrite(buf, LOL_DEFBUF, 1, s)) != 1)
      goto calc; // return ret;
    ret += LOL_DEFBUF;
  }
  if (fr) {
    if ((fwrite(buf, fr, 1, s)) != 1)
      goto calc; // return ret;
    ret += fr;
  }

 calc:
  if (!(ret))
    return 0;
  fr = ret % es;
  ret /= es;
  if (fr)
    ret++;
  return ret;
} // end lol_ifcopy
/* ********************************************************** */
size_t lol_fclear(const size_t bytes, FILE *s)
{
  // Can't use lol_ifcopy because it fills in
  // alloc_entry -sized chuncks
  size_t i, last, times, ret = 0;
  char buf[LOL_DEFBUF];
  if ((!(s)) || (!(bytes)))
    return 0;
  last  = bytes % LOL_DEFBUF;
  times = bytes / LOL_DEFBUF;
  memset(buf, 0, LOL_DEFBUF);
  for (i = 0; i < times; i++) {
      if (fwrite(buf, LOL_DEFBUF, 1, s) != 1) {
               return ret;
      }
      ret += LOL_DEFBUF;
  }
  if (last) {
       if (fwrite(buf, last, 1, s) != 1) {
           return ret;
       }
       ret += last;
  }
  return ret;
} // end lol_fclear
/* **********************************************************
 *  lol_fio: File I/O. Read/Write from/to given file.
 *  Return value:
 *  number of bytes read or written succesfully.
 *********************************************************** */
size_t lol_fio(char *ptr, const size_t bytes,
               void *fp, const int func)

{
  size_t i, t, r;
  size_t n = 0;
  lol_io_func io = NULL;

  if ((!(ptr)) || (bytes <= 0) || (!(fp)))
    return 0;

  switch (func) {
       case LOL_READ :
	    io = (lol_io_func)&fread;
	    break;
       case LOL_WRITE :
	    io = (lol_io_func)&fwrite;
	    break;
       case LOL_READ_CONT :
	    io = (lol_io_func)&lol_fread;
	    break;
       case LOL_WRITE_CONT :
	    io = (lol_io_func)&lol_fwrite;
	    break;
          default:
            return 0;
  } // end switch

  r = bytes % LOL_DEFBUF;
  t = bytes / LOL_DEFBUF;
  for (i = 0; i < t; i++) {
        if (io((char *)&ptr[n], LOL_DEFBUF, 1, fp) != 1)
             return n;
        n += LOL_DEFBUF;
  }
  if (r) {
    if ((io((char *)&ptr[n], r, 1, fp)) != 1)
          return n;
        n += r;
  }
  return n;
} // end lol_fio
/* ********************************************************** */
size_t lol_validcont(const char *name, lol_meta *usb, struct stat *st) {
  lol_meta sb;
  lol_meta *x;
  long s;
  if (!(name)) {
       return 0;
  }
  if (usb) {
    x = usb;
  }
  else {
    x = &sb;
  }
  s = lol_getsize(name, x, st, RECUIRE_SB_INFO);
  if (s == LOL_ERR_ENOENT) {
    if (st) {
      st->st_size = 0;
    }
    return 0;
  }
  if (s < LOL_THEOR_MIN_DISKSIZE) {
     return 0;
  }
  //if (sb.nf > sb.nb) { lol_getsize does not check this
  return (size_t)(s);
} // end lol_validcont
/* **********************************************************
 * lol_help:
 * Outputs a string array.
 *
 * Return value:
 * N/A
 *********************************************************** */
void lol_help(const char* lst[]) {
  int i = 0;
  if (!(lst))
    return;
  while (lst[i]) {
    puts(lst[i++]);
  };
} // end lol_help
/* **********************************************************
 * lol_ltostr:
 * Converts given amount of bytes to a C string like:
 * 1120 --> "1.10 Kb"
 *
 * -1 : error
 *  0 : success (C-string value will be in 's')
 *********************************************************** */
int lol_ltostr(const long y, char *s) {
 long  a = y;
 float x = (float)(y);
 int   i = 1;
 if (!(s))
   return -1;
 if (y < 0)
   a = -y;
 while (i < LOL_MAX_LOLSIZES)   {
   if (((float)(a)) < lol_sizes[i].s) {
        break;
    }
    i++;
 }
 x /= lol_sizes[i-1].s;
 sprintf(s, "%2.2f ", x);
 strcat(s, lol_sizes[i-1].n);
 return 0;
} // end lol_ltostr
/* **********************************************************
 * lol_count_file_blocks:
 * Follow the chain and count reserved VALID blocks of a file.
 * Return:
 * -1 : I/O-, user,- or other error (maybe corruption too).
 *  0 : success
 *  1 : Corrupted chain.
 * ********************************************************** */
int lol_count_file_blocks(FILE *fp, const struct lol_super *sb,
                          const alloc_entry first_index,
                          const long dsize, long *count,
                          const int terminate)
{

  const long ents = (long)(ENTRY_SIZE);
  const long ments = -ents;
  const size_t sents = (size_t)ents;

  alloc_entry c_idx;
  long  nb;
  long  bs;
  fpos_t old_pos;
  long i, size;
  long skip, offset;
  int ret = -1;

  if ((!(fp)) || (!(sb)) || (!(count)))
    return -1;

  c_idx = first_index;
  nb    = (long)sb->nb;
  bs = (long)sb->bs;
  *count     = 0;
  if ((!(nb)) || (!(bs)))
      return -1;
  if ((c_idx < 0) || (c_idx >= nb))
      return 1;
  if (dsize < LOL_THEOR_MIN_DISKSIZE)
      return -1;
  *count = 1;
  size   = (long)LOL_DEVSIZE(nb, bs);
  if (dsize != size)
     return -1;
  skip = LOL_TABLE_START_EXT(nb, bs);
  if (fgetpos(fp, &old_pos))
      return -1;
  for (i = 0; i < nb; i++) {
        offset = skip + c_idx * ents;
        if ((fseek(fp, offset, SEEK_SET))) {
	   *count = i + 1;
            goto error;
        }
	if ((fread((char *)&c_idx, sents, 1, fp)) != 1) {
	     *count = i + 1;
              goto error;
        }
        if (c_idx == LAST_LOL_INDEX) {
	    *count = i + 1;
	    ret    = 0;
	    goto error;
        }
        if ((c_idx >= nb) || (c_idx < 0)) {
	  if (terminate) {
	         *count = i + 1;
                 if ((fseek(fp, ments, SEEK_CUR))) {
                      goto error;
                 }
		 c_idx = LAST_LOL_INDEX;
	         if ((fwrite((char *)&c_idx, sents, 1, fp)) != 1) {
                      goto error;
                 }
	 	 ret = 0;
		 goto error;
	  } // end if terminate

	    *count = i + 1;
	     ret   = 1;
	     goto error;
        }
  } // end for i
  if (terminate) {
      *count = i;
      if ((fseek(fp, ments, SEEK_CUR))) {
           goto error;
      }
      c_idx = LAST_LOL_INDEX;
      if ((fwrite((char *)&c_idx, sents, 1, fp)) != 1) {
           goto error;
      }
      ret = 0;
      goto error;
  } // end if terminate
  *count = i;
     ret = 1;
error:
    fsetpos(fp, &old_pos);
    return ret;
} // end lol_count_file_blocks
/* **********************************************************
 * lol_supermod: PRIVATE FUNCTION!
 * Increment & decrement the superblock file counter.
 * (Autocorrects superblock also)
 * Return value:
 * -1 : error
 *  0 : success
 * ********************************************************** */
int lol_supermod(FILE *fp, const lol_meta *sb, const int func) {

  DWORD nb;
  DWORD bs;
  int   nf;
  fpos_t pos;
  lol_meta s;
  int fix  = 0;
  int ret = -1;

  if ((!(fp)) || (!(sb)))
     return -1;
  nb  = sb->nb;
  bs  = sb->bs;
  nf  = (int)sb->nf;

  if ((!(nb)) || (!(bs)))
    return -1;
  switch (func) {

    case  LOL_INCREASE:
      if (nf == nb)
	return -1;
      if (nf > nb)
	fix = 1;
      break;

    case  LOL_DECREASE:
      if (!(nf))
	return -1;
      if (nf < 0)
	fix = 1;
      break;

    default:
    return -1;
  } // end switch
  if (fgetpos(fp, &pos))
      return -1;
  if (fseek(fp, 0, SEEK_SET))
      goto error;
  if ((fread((char *)&s, (size_t)(DISK_HEADER_SIZE),
       1, fp)) != 1)
      goto error;
  if (fseek(fp, 0, SEEK_SET))
      goto error;
  if (func == LOL_INCREASE) {
      if (fix)
	s.nf = nb;
      else
        s.nf++;
  }
  else {
    if (fix)
      s.nf = 0;
    else
     s.nf--;
  } // end else
  if ((fwrite((const char *)&s,
       (size_t)(DISK_HEADER_SIZE), 1, fp)) != 1)
       goto error;
  if (fix)
    goto error;
    ret = 0;
 error:
  fsetpos(fp, &pos);
  return ret;
} // end lol_supermod
/* **********************************************************
 * lol_inc_files: PRIVATE FUNCTION!
 * Increment the superblock file counter.
 * ********************************************************** */
int lol_inc_files(lol_FILE *op) {
    return lol_supermod(op->dp, &(op->sb), LOL_INCREASE);
} // end lol_inc_files
/* **********************************************************
 * lol_dec_files: PRIVATE FUNCTION!
 * Decrement the superblock file counter.
 * ********************************************************** */
int lol_dec_files(lol_FILE *op) {
    return lol_supermod(op->dp, &(op->sb), LOL_DECREASE);
} // end lol_dec_files
/* **********************************************************
 * delete_chain_from: PRIVATE FUNCTION!
 * Delete index chain of the argument file.
 * If flags is set, then it will save the first allocated block
 * and delete all the rest. (handy when truncating a file)
 *
 * Does not do anything else. op->curr_pos, op->pos,
 * op->nentry etc should be updated after calling this...
 *
 * Return value:
 * < 0 : error
 * = 0 : succes
 * ********************************************************** */
int lol_delete_chain_from(lol_FILE *op, const int flags) {

  const alloc_entry empty = FREE_LOL_INDEX;
  const alloc_entry last  = LAST_LOL_INDEX;
  alloc_entry current_index;
  FILE *fp;
  DWORD nb;
  DWORD bs;
  ULONG fs;
  DWORD fblocks;
  long  skip;
  long  offset;
  DWORD i = 0;
  int   j = 1;

    fp = op->dp;
    if (!(fp))
      return LOL_ERR_USER;

    current_index = op->nentry.i_idx;
    nb = op->sb.nb;
    bs = op->sb.bs;
    fs = op->nentry.fs;

    // Quick check
    if ((current_index >= nb) ||
	(fs > (nb * bs)) ||
	(!(nb)) || (!(bs))) {
      return LOL_ERR_CORR;
    }
    // How many blocks in the file?
    LOL_FILE_BLOCKS(fs, bs, fblocks);
    if (fblocks > nb) {
        return LOL_ERR_CORR;
    }
    if (flags) {
        j = 0;
    }
    skip = LOL_TABLE_START(op);
    while (1) {

      offset = skip + current_index * ENTRY_SIZE;
#if LOL_TESTING
      printf("delete_chain_from: Seeking to index %d\n", current_index);
#endif

      if ((fseek(fp, offset, SEEK_SET))) {
          goto error;
      }

      if (++i > fblocks) {
#if LOL_TESTING
          puts("delete_chain_from: I am returning -6 now (7)");
#endif
	  return LOL_ERR_CORR;
      }

      if ((fread((char *)&current_index,
                 (size_t)(ENTRY_SIZE), 1, fp)) != 1) {
           goto error;

      } // end if fread
      // Clear and read next entry
      if ((fseek(fp, -(ENTRY_SIZE), SEEK_CUR))) {
           goto error;
      }
      if (j) {

	  if ((fwrite((const char *)&empty,
                      (size_t)(ENTRY_SIZE), 1, fp)) != 1) {
               goto error;

	  } // end if fwrite
      }
      else {
	  if ((fwrite((const char *)&last,
                      (size_t)(ENTRY_SIZE), 1, fp)) != 1) {
               goto error;
	  } // end if fwrite

          j = 1;

      } // end else
      if (current_index == LAST_LOL_INDEX) {
          return LOL_OK;
      }

      // Next offset

  } // end while

return LOL_ERR_INTRN;

 error:

if (errno)
   LOL_ERR_RETURN(errno , LOL_ERR_IO)
else
   LOL_ERR_RETURN(EIO , LOL_ERR_CORR)
} // end lol_delete_chain_from
/* **********************************************************
 * lol_read_nentry: PRIVATE helper function exclusively
 * for lol_fopen's use.
 * Checks if name given in op->file exists.
 * ret <  0 : error
 * ret =  0 : does not exist
 * ret =  1 : exists
 *
 * The op->sb must be set before calling.
 * op->nentry and op->n_idx will be overwritten
 * ********************************************************** */
#define LOL_READNENTRY_TMP 256
int lol_read_nentry(lol_FILE *op) {

  const long  nes =  ((long)(NAME_ENTRY_SIZE));
  const long temp_mem = nes * LOL_READNENTRY_TMP;
  char temp[temp_mem];
  const char  *name = (char *)op->file;
  char  *ename;
  size_t mem = 0;
  FILE   *fp = op->dp;
//  long    nb = (long)op->sb.nb;
//  long    bs = (long)op->sb.bs;
  long  base = op->dir;
  long    ds = op->dir_s;
  fpos_t      pos;
  lol_nentry *buf;
  lol_nentry *entry;
  long      times;
  long       frac;
  long       full;
  long         io;

  int  flen = op->f_len;
  int alloc = 0;
  int loops = 1;
  int  ret = -1;
  int     i = 0;
  int   j, k, z;

  io = lol_get_io_size(ds, nes);
  if (io <= 0) {
      lol_debug("lol_read_nentry: Internal error: io <= 0\n");
      lol_errno = EIO; return -1;
  }
  if (io > temp_mem) {
    if (!(buf = (lol_nentry *)lol_malloc((size_t)(io)))) {
          buf = (lol_nentry *)temp;
          io  = temp_mem;
    } else {
       mem = (size_t)(io);
       alloc = 1;
    }
  } else {
    buf = (lol_nentry *)temp;
    io  = temp_mem;
  }

  times = ds / io;
  frac  = ds % io;
  k     = (int)(io / nes);
  full  = ds - frac;

  if ((fgetpos(fp, &pos))) {
      LOL_ERRNO(EIO);
      goto freemem;
  }
  if (fseek (fp, base, SEEK_SET)) {
      lol_errno = EIO;
      goto position;
  }

dentry_loop:

  for (; i < times; i++) {

    // Read a bunch of name entries
    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         lol_errno = EIO;
         goto position;
    }
    // Now check these entries
    for (j = 0; j < k; j++) { // foreach entry...

        if (!(buf[j].name[0])) {
	   continue;
        }
	else {

           entry = &buf[j];
	   ename = (char *)entry->name;

	   // Now compare 'name' with 'entry->name'
#if LOL_TESTING
	   printf("lol_read_nentry: comparing: \'%s\' and \'%s\'\n",
                  (char *)name, (char *)ename);
#endif

#ifdef LOL_INLINE_MEMCPY
	   // inline strcmp
           for (z = 0; z < flen; z++) {

	      if (ename[z] != name[z]) {
	          break;
	      }

	   }
	   if (z != flen) {
	      continue;
	   }
	   if (ename[flen]) {
	      continue;
	   }

#else
           if ((strcmp((char *)name, (char *)ename))) {
	       continue;
           }

#endif
	   // Gotcha! -> Save the entry
           LOL_MEMCPY(&op->nentry, entry, nes);

	   if (loops) {
	       ds = io;
	       ds *= i;
	   }
	   else {
	       ds = full;
	   }

	   j *= nes;
           ds += j;
	   op->n_idx = ds / nes;
           op->n_off = base;
	   op->n_off += ds;
           ret = 1;

	   goto position;

        } // end else found it

    } // end for j
  } // end for i
  // Now the fractional data
  if ((frac) && (loops)) {
       times++;
       io = frac;
       k = (int)(io / nes);
       loops = 0;
      goto dentry_loop;
  } // end if frac

  ret = 0;

position:
  fsetpos(fp, &pos);
freemem:
  if (alloc) {
    lol_index_free(mem);
  }
  return ret;
} // end lol_read_nentry
#undef LOL_READNENTRY_TMP
/* ********************************************************** */
lol_FILE *new_lol_FILE() {
  lol_FILE *fp = LOL_ALLOC(struct _lol_FILE);
  if (!(fp))
    return NULL;
  memset((char *)fp, 0, LOL_FILE_SIZE);
  fp->opm = -1;
  return fp;
} // end new_lol_FILE
/* ********************************************************** */
void delete_lol_FILE(lol_FILE *fp) {
  if (fp) {
     free(fp);
     fp = NULL;
  }
} // end delete lol_FILE
/* **********************************************************
 * lol_pathinfo:
 * Dissect full lolfile path like: "/path/to/my.db:/image.jpg"
 * and receive it's
 *                  - container path: "/path/to/my.db"
 *                  - container name: "my.db"
 *                  - file name: "image.jpg"
 *                  - length of the full path (as with strlen)
 *                  - length of the container path
 *                  - length of the container name
 *                  - length of the file name
 *
 * You may request multiple choices at the same time with
 * the following exception:
 * Request either container path or container name, not both!
 *
 * Setup lol_pinfo structure first. Mandatory fields are
 * - 'fullp' full lolfile path to analyze
 * - 'func'  or'd combinations of requested info:
 *    LOL_PATHLEN   (full path length)
 *    LOL_FILELEN   (length of the file part)
 *    LOL_FILENAME  (ptr to file name part)
 *    LOL_CONTPATH  (ptr to container path part)
 *    LOL_CONTNAME  (ptr to container's name)
 * (Container path length or containers name length will
 *  be always stored automatically to 'clen' field when
 *  you request either LOL_CONTPATH or LOL_CONTNAME).
 * - naturally allocate all the pointers for which you
 *   request results too..
 *
 * Return value:
 * <0 : error
 *  0 : success
 * ********************************************************** */
int lol_pathinfo(lol_pinfo *p) {

  char *path;
  char *cont;
#ifdef LOL_INLINE_MEMCPY
  char *src;
  int x;
#endif
  int i, k, len = 0;
  int y, f;

  if (!(p))
    return -1;
  if (!(p->fullp))
    return -2;
  path = p->fullp;
  f = p->func;
  if ((f <= 0) || (f > 23)) {
     return -3;
  }
#ifdef LOL_INLINE_MEMCPY
  for (; path[len]; len++);
#else
  len = (int)strlen(path);
#endif
  if ((len < 4) || (len > LOL_PATH_MAX)) {
    return -4;  // <-- Don't touch this value, lol_fopen
                // depends on it!
  }
  if (path[len-1] == '/') {
    return -5;
  }
  if (f & LOL_PATHLEN) {
     p->plen = len;
     if (f == LOL_PATHLEN) {
       return 0;
     }
  }
  // file's length = len - (i+1)
  for (i = len-2; i > 1; i--) {
    if (path[i] == '/') {
      if (path[i-1] == ':') {
        if (path[i-2] == '/') {
	   return -6;
        }
	if ((f == LOL_FILELEN) &&
            (p->flen == LOL_PATHMAGIC)) {
	    return 0;
	}
	// Ok, found it, copy the parts
	k = i + 1;
        y = len - k;
	if (y >= LOL_FILENAME_MAX) {
          // y  = LOL_FILENAME_MAXLEN;
           return -7;
        }
	if (f & LOL_FILELEN) {
	    p->flen = y;
	    if (f < LOL_FILENAME) {
	      return 0;
	    }
        }
	if (f & LOL_FILENAME) {
#ifdef LOL_INLINE_MEMCPY
	   src = (char *)&path[k];
	   for (x = 0; x < y; x++) {
	       p->file[x] = src[x];
	   }
#else
	   LOL_MEMCPY(p->file, &path[k], y);
#endif
	   p->file[y] = '\0';
	   if (f < LOL_CONTPATH) {
	      return 0;
	   }
        } // end if get file name

        cont = p->cont;
        if (f & LOL_CONTPATH) {
            y = i - 1;
#ifdef LOL_INLINE_MEMCPY
	    for (x = 0; x < y; x++) {
	         cont[x] = path[x];
	    }
#else
	    LOL_MEMCPY(cont, path, y);
#endif
	    cont[y] = '\0';
	    p->clen = y;
	    return 0;
	} // end if full path
	if (f & LOL_CONTNAME) {
	   if (i == 2) { // Special case -> return fast
	      cont[0] = path[0]; cont[1] = '\0';
	      p->clen = 1;
	      return 0;
	   } // end if

           len = i - 1;
	   for (i = len-1; i >= 0; i--) {
	       if (path[i] == '/') {
	           k = i + 1;
                   y = len - k;
#ifdef LOL_INLINE_MEMCPY
	           src = (char *)&path[k];
	           for (x = 0; x < y; x++) {
	               cont[x] = src[x];
	           }
#else
	           LOL_MEMCPY(cont, &path[k], y);
#endif
		   cont[y] = '\0';
	           p->clen = y;
	           return 0;
	       } // end if slash
	   } // end for i
	   // No slashes found -> copy all and return
#ifdef LOL_INLINE_MEMCPY
	   for (x = 0; x < len; x++) {
	        cont[x] = path[x];
	   }
#else
	   LOL_MEMCPY(cont, path, len);
#endif
	   cont[len] = '\0';
	   p->clen = len;
	   return 0;
	} // end if cont name only
      } // end if ':'
      else {
	return -8;
      }
    } // end if '/'
  } // end for i
  return -9;
} // end lol_pathinfo
/* **********************************************************
 * lol_validpath:
 * Check if given lolpath ("/path/to/db:/foo") is formally
 * correct.
 *
 * Return value:
 * 1 : correct
 * 0 : incorrect
 * ********************************************************** */
BOOL lol_validpath(char *path) {
 lol_pinfo p;
 int r;
 p.fullp = path;
 p.func = LOL_SIMULATE;
 p.flen = LOL_PATHMAGIC;
 r = lol_pathinfo(&p);
 if (!(r)) {
   return 1;
 }
 return 0;
} // end lol_validpath
/* **********************************************************
 * lol_getfname: // PRIVATE function.
 * Takes a file path given in 'path 'inside a container
 * like: "/home/my.db:/file.txt" and copies the filename
 * part ("file.txt") to param 'fname'.
 *
 * Return value:
 * <0 : error
 *  0 : success
 * ********************************************************** */
int lol_getfname(const char *path, char *fname)
{
  lol_pinfo p;
  if ((!(path)) || (!(fname))) {
    return -1;
  }
  p.fullp = (char *)path;
  p.file  = fname;
  p.func  = LOL_FILENAME;
  return lol_pathinfo(&p);
} // end lol_getfname
/* **********************************************************
 * lol_fnametolol: // PRIVATE function.
 * Takes a normal file path (like: /path/to/dir/README)
 * and a container name - including optional path part
 * (like: /home/my.db) and joins them to a lol_FILE name
 * like: /home/my.db:/README
 *
 * Return value:
 * <0 : error
 *  0 : success
 * ********************************************************** */
int lol_fnametolol(const char *src, const char *cont,
                   char *out, const size_t cont_len)
{
  size_t ln;
  int i, len;

  LOL_MEMCPY(out, cont, cont_len);
  len = (int)(cont_len);
  out[len] = ':';
  out[len + 1] = '/';

  ln = strlen(src);

  if ((ln > LOL_PATH_MAX))
    return -1;
  if (src[ln-1] == '/')
    return -1;

  len += 2;

  for (i = ln-1; i >= 0; i--) { // any slashes in the path?
    if (src[i] == '/') {
       i++;
       ln -= i;
       LOL_MEMCPY(&out[len], &src[i], ln);
       ln += len;
       out[ln] = '\0';
       return 0;
    } // end if slash
  } // end for i

  // No slashes -> copy
  LOL_MEMCPY(&out[len], src, ln);
  ln += len;
  out[ln] = '\0';
  return 0;
} // end lol_fnametolol
/* ********************************************************** */
int lol_is_writable(const lol_FILE *op) {

  DWORD nb;
  DWORD bs;
  ULONG ds;
  UCHAR fe;

  if (!(op))
    return -1;
  nb   = op->sb.nb;
  bs   = op->sb.bs;
  fe   = op->file[0];

  if ((!(nb)) || (!(bs)) || (!(fe)))
    return -2;
  if (op->opm < 1) // Opened "r"?
     return -3;
  ds = (ULONG)LOL_DEVSIZE(nb, bs);
  if (op->cs != ds)
     return -4;
  return 0;
} // end lol_is_writable
/* ********************************************************** *
 * lol_exit:
 * Cleanup & exit
 *
 * Return value:
 * N/A
 * ********************************************************** */
void lol_exit(int status) {
  // There may be open file handles but
  // let the OS take care of them.
  // Just try to free memory if allocated..
  if (lol_buffer_lock == 2) {
      if (lol_index_buffer) {
          free (lol_index_buffer);
          lol_restore_sighandlers();
      }
      lol_index_buffer = lol_storage_buffer;
  }
  exit (status);
} // end lol_exit
/* ********************************************************** *
 * lol_getsb:  PRIVATE function!!
 * Read container metadata to the argument 'sb'.
 *
 * Return value:
 * <0 : if error
 *  0 : if success
 * ********************************************************** */
int lol_getsb(FILE *fp, lol_meta *sb) {
  const size_t sbs = (size_t)(DISK_HEADER_SIZE);
  fpos_t pos;
  int rv = -1;
  if ((!(fp)) || (!(sb)))
      return -1;
  if ((fgetpos(fp, &pos)))
     return -1;
  if ((fseek(fp, 0, SEEK_SET)))
     goto error;
  if ((fread((char *)sb, sbs, 1, fp)) != 1)
     goto error;
  rv = 0;
 error:
  fsetpos(fp, &pos);
  return rv;
} // end lol_getsb
/* ********************************************************** *
 * lol_get_free_index:
 * Find the first free index in list and set it to value *idx
 * if the flag is set. The value in *idx will be replaced with
 * the free index.
 *
 * Return value:
 * < 0 : error
 *   0 : success
 * ********************************************************** */
#define LOL_FREE_IDX_BUFF 1024
int lol_get_free_index(lol_FILE *op, alloc_entry *idx, int flag)
{
  FILE *fp = op->dp;
  lol_meta *sb = &op->sb;
  alloc_entry temp[LOL_FREE_IDX_BUFF];
  const long idx_s = (long)(ENTRY_SIZE);
  const long temp_mem = idx_s * LOL_FREE_IDX_BUFF;
  alloc_entry     newe = *idx;
  fpos_t           pos;
  size_t       mem = 0;
  size_t      data;
  long          nb = (long)sb->nb;
 // long          bs = (long)sb->bs;
  alloc_entry *buf;
  long           i;
  long           j;
  long          io;
  long    idx_area = op->idxs_s;
  long      offset;
  long       times;
  long       n_idx;
  long        frac;
  long        full;
  int        alloc = 0;
  int         ret = -1;
  int   doing_blk  = 1;
  alloc_entry what;

  if (((newe >= 0) && (newe < nb)) ||
       (newe == LAST_LOL_INDEX)    ||
       (newe == FREE_LOL_INDEX)) {
    goto legal;
  }
  return -1;

 legal:

  io = lol_get_io_size(idx_area, idx_s);
  if (io <= 0)
     return -1;

  // We try to suck in all the indices
  // at the same time
  if (io > temp_mem) {

     if (!(buf = (alloc_entry *)lol_malloc((size_t)(io)))) {
           buf = temp;
           io  = temp_mem;
     } else {
        mem = (size_t)(io);
        alloc = 1;
     }

  } else {
      buf = (alloc_entry *)temp;
      io  = temp_mem;
  }

  offset = op->idxs;
  times  = idx_area / io;
  frac   = idx_area % io;
  full   = idx_area - frac;
  full  /= idx_s;

  if ((fgetpos(fp, &pos)))
       goto err;
  if ((fseek(fp, offset, SEEK_SET)))
       goto error;
  n_idx = io / idx_s;
  if ((io % idx_s) || (frac % idx_s)) {
    lol_debug("lol_get_free_index: internal error, sorry!");
       goto error;
  }
  data = (size_t)(io);

  for (i = 0; i < times; i++) {
   if ((fread((char *)buf, data, 1, fp)) != 1)
       goto error;
  check_loop:
      for (j = 0; j < n_idx; j++) {
          if (buf[j] == FREE_LOL_INDEX) {
	     if (flag) {
	        offset = -(n_idx);
	        offset += j;
	        offset *= idx_s;
                if ((fseek(fp, offset, SEEK_CUR)))
                    goto error;
	        if ((fwrite((const char *)&newe, (size_t)(idx_s),
		       1, fp)) != 1)
	               goto error;
	     }
	     if (doing_blk)
                what = (i * n_idx);
	     else
	        what = full;

	     *idx = (alloc_entry)(what + j);
	     ret = 0;
	     goto done; // OK, --> return

          } // end if found free index
      } // end for j
  } // end for i
  if ((frac) && (doing_blk)) {
      n_idx = frac / idx_s;
      if ((lol_fio((char *)buf, frac, fp, LOL_READ)) != frac)
          goto error;
      doing_blk = 0;
      goto check_loop;
  } // end if frac

 error:
 done:
  fsetpos(fp, &pos);
 err:
  if (alloc) {
     lol_index_free(mem);
  }
  return ret;
} // end lol_get_free_index
#undef LOL_FREE_IDX_BUFF
/* ********************************************************** *
 * lol_get_free_nentry:  PRIVATE function!!
 * Find the first free name entry in list and fill it
 * with 'info->ne' if it's not NULL.
 * Fill all other fields of 'info' too.
 *
 * Return value:
 *     <0 : if error
 * offset : if success (offset of the directory entry from abs 0)
 * ********************************************************** */
#define LOL_FREEDENTRY_TMP 256
long lol_get_free_nentry(lol_FILE *op, lol_ninfo *info) {

  FILE *fp = op->dp;
//  lol_meta *sb = &op->sb;
  const long  nes =  ((long)(NAME_ENTRY_SIZE));
 // const long  ms  =  ((long)(DISK_HEADER_SIZE));
  const long temp_mem = nes * LOL_FREEDENTRY_TMP;
  char temp[temp_mem];
  fpos_t      pos;
  lol_nentry *buf;
  size_t mem = 0;
  long io;
  long base = op->dir;
 // long nb = (long)sb->nb;
//  long bs = (long)sb->bs;
  long ds = op->dir_s;
  long  frac;
  long  full;
  long ret = -1;
  int j, k, times;
  int     i = 0;
  int alloc = 0;
  int loops = 1;

  io = lol_get_io_size(ds, nes);
  if (io <= 0) {
      lol_debug("lol_df: Internal error: io <= 0");
      lol_errno = EIO; return -1;
  }
  if (io > temp_mem) {
    if (!(buf = (lol_nentry *)lol_malloc((size_t)(io)))) {
          buf = (lol_nentry *)temp;
          io  = temp_mem;
    } else {
       mem = (size_t)(io);
       alloc = 1;
    }
  } else {
    buf = (lol_nentry *)temp;
    io  = temp_mem;
  }

  times = (int)(ds / io);
  frac  = ds % io;
  k     = (int)(io / nes);
  full  = ds - frac;

  if ((fgetpos(fp, &pos))) {
      goto freemem;
  }
  if (fseek (fp, base, SEEK_SET)) {
      goto position;
  }

 dentry_loop:
  for (; i < times; i++) {

    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         goto position;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...

        if (buf[j].name[0]) {
	   continue;
        }
	else {

	    // Found it, calculate offset
	    // and index for param 'info'
	    if (loops) {
	        ret = i * io;
	    }
	    else {
	        ret = full;
	    }
            ret += (j * nes);
	    info->idx = ret / nes;
	    ret += base;
            info->off = ret;
	    info->res = 0;

	    // update nentry if ne != NULL
	    if (info->ne) {
	       if ((fseek (fp, ret, SEEK_SET))) {
		    ret = -1;
	            goto position;
	       }
	       if ((fwrite((const char *)info->ne,
                           (size_t)(nes), 1, fp)) != 1) {
		    ret = -1;
	            goto position;
	       }
	       info->res = 1;
	    } // end if patch too

	    // All well, --> return
	    goto position;
	} // end else

    } // end for j
  } // end for i
  // Now the fractional data
  if ((frac) && (loops)) {
       times++;
       io = frac;
       k = (int)(io / nes);
       loops = 0;
      goto dentry_loop;
  } // end if frac
position:
  fsetpos(fp, &pos);
freemem:
  if (alloc) {
     lol_index_free(mem);
  }
  return ret;
} // end lol_get_free_nentry
#undef LOL_FREEDENTRY_TMP
/* ********************************************************** *
 * lol_touch_file:  PRIVATE function!!
 * Create a new file of zero size.
 * Assumes that the file name is already in op->file.
 * NOTE: DOES NOT RETURN ERROR IF FILE EXISTS ALREADY!
 * (This is basically a helper function for lol_fopen).
 *
 * Return < 0 if error.
 * ********************************************************** */
#define LOL_TOUCH_ERROR(x,y) { op->err = lol_errno = (x); ret = (y); goto error; }
int lol_touch_file(lol_FILE *op) {

  lol_ninfo info;
  FILE *fp = op->dp;
  char *name = op->file;
#ifdef LOL_INLINE_MEMCPY
  char *tmp;
  int x;
#endif
  fpos_t pos;
  alloc_entry idx = LAST_LOL_INDEX;
  int  len, ret = -1;
  long off;
 // Make some aliases
  DWORD nb   = op->sb.nb;
 // DWORD bs   = op->sb.bs;
  DWORD nf   = op->sb.nf;

  // Make some checks
  if (nf >= nb)
     LOL_ERR_RETURN(ENOSPC, -4);
 // if(!(name[0]))
//     LOL_ERR_RETURN(EIO, -5);
  len = op->f_len;
  if (len >= LOL_FILENAME_MAX) {
      len = LOL_FILENAME_MAX - 1;
      name[len] = '\0';
  }
  if (fgetpos(fp, &pos))
     LOL_ERR_RETURN(errno, -7);

  // Ok, action begins here...
  // Find a free index and reserve it
  if ((lol_get_free_index(op, &idx, LOL_MARK_USED)))
      LOL_TOUCH_ERROR(ENOSPC, -10);

  // create a zero sized file
#ifdef LOL_INLINE_MEMCPY
  tmp = (char *)op->nentry.name;
  for (x = 0; x < len; x++) {
       tmp[x] = name[x];
  }
#else
  LOL_MEMCPY(op->nentry.name, name, len);
#endif
  op->nentry.name[len] = '\0';
  op->nentry.created   = time(NULL);
  op->nentry.i_idx     = idx;
  op->nentry.fs = 0;
  info.ne = &op->nentry;
  off = lol_get_free_nentry(op, &info);

  if (off < 0) { // Failed, free the index
      lol_set_index_value(op, FREE_LOL_INDEX);
      LOL_TOUCH_ERROR(EIO, -9);
  }
  if (lol_inc_files(op)) {
     // Try to rollback...
     if (!(lol_remove_nentry(op,(DWORD)(info.idx), 1))) {
	   lol_set_index_value(op, FREE_LOL_INDEX);
     }
     LOL_TOUCH_ERROR(EIO, -13);
  }
  op->curr_pos = 0;
  if (fsetpos(fp, &pos))
      goto error;
  op->n_idx = (alloc_entry)info.idx;
  return 0;
error:
  fsetpos(fp, &pos);
  return ret;
} // end lol_touch_file
#undef LOL_TOUCH_ERROR
/* ********************************************************** */
int lol_truncate_file(lol_FILE *op) {

  lol_nentry *entry;
  fpos_t pos;
  int ret = -1;

  if (!(op->nentry.fs)) {
      return 0;
  }
  // So, the file is >= 1 bytes, --> truncate to zero
  if (fgetpos(op->dp, &pos))
      return -2;
  // Truncate index chain.
  if ((lol_delete_chain_from(op,
       LOL_SAVE_FIRST_BLOCK)))  {
       ret = -3;
       goto error;
  }
  if (LOL_GOTO_DENTRY(op)) {
      ret = -4;
      goto error;
  }
  entry = &op->nentry;
  // Set zero size
  entry->fs = 0;
  entry->created = time(NULL);
  // update name entry
  if ((fwrite((const char *)entry,
       NAME_ENTRY_SIZE, 1, op->dp)) != 1) {
       ret = -5;
       goto error;
  }
  ret = 0;
 error:
  fsetpos(op->dp, &pos);
  return ret;
} // end lol_truncate_file
/* **********************************************************
 *  lol_getmode:
 *  Return the associated integer of the given open mode.
 *  
 *  < 0 : if illegal mode.
 *
 * ********************************************************** */
int lol_getmode(const char *mode) {
  int i, j;
  if (!(mode))
     return -1;
  if (!(mode[0]))
     return -1;
  for (i = 0; i < MAX_LOL_OPEN_MODES; i++) {
     for (j = 0; lol_mode_combinations[i][j][0]; j++) {
	 if (!(strcmp(mode, lol_mode_combinations[i][j]))) {
	     return i;
	 }
     } // end for j
  } // end for i
  return -1;
} // end lol_getmode
/* **********************************************************
 * lol_can_replace:
 * Check if an existing file inside a container may be
 * overwritten by another file (same name usually but
 * different size).
 *
 * Arguments:
 *    src_fs = size of the source file
 *    dst_fs = size of the file to be replaced in container
 *    freebl = unused/free blocks (in the target container)
 *    bs     = block size in the target container.
 *
 * NOTE: Even if freebl is zero, there may be unused space
 *       in the last block of the file.
 *
 * Return value:
 *  <0 : error / cannot be overwritten
 *   0 : succes (the file may be overwritten)
 * ********************************************************** */
int lol_can_replace (const long src_fs, const long dst_fs,
                     const long freebl, const long bs)  {

  const long bfree = freebl * bs;
  long availab = 0;
  long frac;
  if ((bfree < 0) || (bs <= 0) ||
     (dst_fs < 0) || (src_fs < 0)) {
      return -1;
  }
  if (src_fs <= dst_fs) {
      return 0;
  }
  // How much room in last block?
  frac = dst_fs % bs;
  if (frac) {
      availab = bs - frac;
  }
  // Add free blocks to it
  availab += bfree;
  // Add the existing file size
  availab += dst_fs;
  if (src_fs > availab) {
     return -1;
  }
  return 0;
} // end lol_can_replace
/* ********************************************************** */
int lol_update_nentry(lol_FILE *op) {
  if (LOL_GOTO_DENTRY(op)) {
    goto error;
  }
  if ((fwrite((const char *)&op->nentry,
       (size_t)(NAME_ENTRY_SIZE), 1, op->dp)) != 1) {
       goto error;
  }
  return 0;
 error:
  return -1;
} // end lol_update_nentry
/* ********************************************************** */
int lol_update_ichain(lol_FILE *op, const long olds,
                 const long news, const alloc_entry last_old) {

  int  mode;
  size_t bytes;
  alloc_entry idx;
  long start;
  long i, offset;
  char *name;

  if (!(op))
    return -1;
  if (!(lol_index_buffer))
    return -2;
  if (last_old < 0)
    return -3;
  if (!(news))
    return 0;
  if (lol_is_writable(op)) {
    return -4;
  }

  mode = op->opm;
  name = op->file;
  if (mode < 1) // Opened "r"?
    return -5;
  if(!(name[0]))
    return -6;

  start = (long)LOL_TABLE_START(op);
  i = olds;
  // Join old and new chains first
  if (i) {
    // Take last index of old chain
    // and make it point to first index of the new chain
     idx = lol_index_buffer[i-1];
     offset = start + idx * ENTRY_SIZE;
     if (fseek(op->dp, offset, SEEK_SET)) {
	 return -7;
     }
     bytes = fwrite((const char *)&lol_index_buffer[i],
                    (size_t)(ENTRY_SIZE), 1, op->dp);
     if (bytes != 1) {
        return -8;
     }
  } // end if olds (i)
  else {
    // So, we don't have any old blocks to write over.
    // Then what was the last old block? We must adjust
    // it to point to the next new...
    offset = start + last_old * ENTRY_SIZE;
    if (fseek(op->dp, offset, SEEK_SET)) {
	return -9;
    }
    bytes = fwrite((const char *)&lol_index_buffer[0],
                   (size_t)(ENTRY_SIZE), 1, op->dp);
    if (bytes != 1) {
	return -10;
    }
  } // end else not olds

  // Then write new indexes
  while (lol_index_buffer[i] != LAST_LOL_INDEX) {

     idx = lol_index_buffer[i];
     offset = start + idx * ENTRY_SIZE;
     if (fseek(op->dp, offset, SEEK_SET)) {
         return -9;
     }
     bytes = fwrite((const char *)&lol_index_buffer[i+1],
                    (size_t)(ENTRY_SIZE), 1, op->dp);
     if (bytes != 1) {
        return -10;
     }
     i++;
  } // end while

  return 0;
} // end lol_update_ichain
/* **********************************************************
 *
 *  lol_new_indexes:
 *  Calculate memory needed for a buffer for new indexes.
 *
 *  Return value:
 *  < 0  : Error.
 *  >= 0 : Success. Returns the number of alloc entries needed.
 *         The value in *olds is the number of old blocks
 *         needed. The value *news is the new blocks needed.
 *         Note: return value = *olds + *news if success.
 *
 * ********************************************************** */
long lol_new_indexes(lol_FILE *op, const long bytes,
      long *olds, long *mids, long *news, long *new_sz) {

  long pos, bs;
  long fs, need;
  long   left = 0;
  long blocks = 0;
  long  f_off = 0;
  long  lb_ed = 0;
  long  delta = 0;
  long   area = 0;


  bs  = (long)op->sb.bs;
  fs  = (long)op->nentry.fs;
  pos = (long)op->curr_pos;

  *olds = *news = *mids = 0;
  if (!(bytes)) {
      *new_sz = fs;
      return 0;
  }
  blocks = (long)lol_num_blocks(op, (size_t)bytes, NULL);
  if (!(fs)) {  // Size = 0, EASY Case
      // In this case we need new blocks for
      // the whole write request, except the first one.
      if (blocks) { // we may not need to check this??
	 *olds = 1;
	 *news = blocks - 1;
         *new_sz = pos + bytes;
       }
       return blocks;
   } // end if zero size file
   // Ok, we know it is a regular file, size > 0
   // and we write > 0 bytes.
   if (!(blocks)) // We must have at least one block!
       return LOL_ERR_USER;

  // Check first if we need new blocks at all.
  f_off = fs % bs;
  if (f_off) {
     lb_ed = bs - f_off;
  }
  left = fs - pos; // may be <= 0 also
  // Do we have ALSO room in the last data block of the file?
  left += lb_ed;  // add is >= 0
  if (left >= bytes) {
     // We don't need new area, the old blocks are enough
      *news = 0; *olds = blocks;
      if (fs < (pos + bytes)) {
	 *new_sz = pos + bytes;
      }
      else {
        *new_sz = fs;
      }
      return blocks;
  } // end if left >=
  // So, we need new blocks. How many?
  area = bytes - left; // >0. This is the
  //  amount of bytes we need in NEW blocks!
  //  This area begins on a block boundary but
  //  does not necessarily end there.
  need = area / bs;
  if (area % bs)
     need++; // This is the amount of NEW blocks >0
             // (Does NOT overlap with old data area)
  if (!(need)) { // Really should not be here
      return LOL_ERR_INTRN;
  }
  // We know the number of NEW blocks (need).
  // How many old blocks are there?
  if (pos >= fs) { // In this case we don't need olds
                   // at all unless pos is inside the
                   // last block.
    if (pos < (fs + lb_ed)) {
        *olds = 1;
        *news = need;
    }
    else {
      *olds = 0;
       delta = pos - fs - lb_ed;
       if (delta >= bs) {
	  *mids = delta / bs;
       }
       *news = need;
    } // end else
  } // end if pos
  else {
    *olds = left / bs;
    if (left % bs) {
       (*olds)++;
    }
    *news = need;
 } // end else

  *new_sz = fs + lb_ed + area;
  return ((*olds) + (*news));
  // What a mess!!!
} // end lol_new_indexes
/* **********************************************************
 *
 *  lol_read_ichain:
 *  Read the whole block index chain to user buffer.
 *  Return value:
 *  <  0 : error
 *  >= 0 : success. The value is number of indices in the buffer
 *
 * ********************************************************** */
int lol_read_ichain(lol_FILE *op, const size_t rbs)
{
  ULONG fs;
  long skip, off;
  long area;
  DWORD nb,  bs;
  DWORD pos, bb;
  DWORD    f_bs;
  alloc_entry   idx;
  alloc_entry c_idx;
  alloc_entry l_idx;
  int  i = 0, c = 0;
  FILE *fp;

  if ((!(op))) {
    lol_errno = EINVAL;
    return LOL_ERR_PTR;
  }
  if (!(lol_index_buffer)) {
     LOL_ERR_RETURN(EBUSY, LOL_ERR_BUSY);
  }
  // Let's make some aliases
     bs = op->sb.bs;
     nb = op->sb.nb;
  l_idx = (alloc_entry)nb - 1;
    idx = (alloc_entry)op->nentry.i_idx;
     fs = op->nentry.fs;
    pos = op->curr_pos;
     fp = op->dp;

  if ((!(rbs)) || (rbs > nb))
     LOL_ERR_RETURN(EIO, LOL_ERR_USER);
  if (!(fs))
    LOL_ERR_RETURN(EIO, LOL_ERR_USER);
  if ((!(bs)) || (!(nb)))
      LOL_ERR_RETURN(EIO, LOL_ERR_CORR);
  if ((idx > l_idx) || (idx < 0)) {
    op->eof = 1; // Does not exctly mean end-of-file, but we set eof because
                 // the chain seems corrupted and thus, it is not a good idea
                 // to continue reading.

    op->err = lol_errno = EIO;
    return LOL_ERR_CORR;
  }
  area = (long)bs;
  area *= nb;
  if (fs > area)
    LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);

     bb = pos / bs;
   f_bs =  fs / bs;

   if (fs % bs) {
       f_bs++;
   }
   if (f_bs > nb)
      LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);
   if (rbs > f_bs)
      LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);

   skip   = LOL_TABLE_START(op);
   off = skip + idx * ENTRY_SIZE;
   if (fseek (fp, off, SEEK_SET))
      LOL_ERR_RETURN(EIO, LOL_ERR_IO);
   c_idx = idx;
   for (i = 0; i < f_bs; i++)  {
     if (i >= bb) {
       if (c_idx < 0)
          LOL_ERR_RETURN(EIO, LOL_ERR_CORR);
       if (c_idx >= nb)
          LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

       lol_index_buffer[c] = c_idx;
       if (++c >= rbs) {
           break;
       }
     } // end if i >= bb
     if (fread ((char *)&c_idx,
         (size_t)(ENTRY_SIZE), 1, fp) != 1)
         LOL_ERR_RETURN(EIO, LOL_ERR_IO)
     if (c_idx == LAST_LOL_INDEX) {
         break;
     }
     if ((c_idx > l_idx) || (c_idx < 0))
        LOL_ERR_RETURN(EIO, LOL_ERR_CORR)

     off = skip + c_idx * ENTRY_SIZE;   // Next offset
     if (fseek (fp, off, SEEK_SET))
        LOL_ERR_RETURN(EIO, LOL_ERR_IO);
  } // end for i
  if (!(c))
     LOL_ERR_RETURN(EIO, LOL_ERR_CORR)
  return c;
} // end lol_read_ichain
/* **********************************************************
 *
 *  lol_new_ichain:
 *  Create a block index chain based on number of
 *  needed old and new blocks.
 *  Return value:
 *  <0  : error
 *  0   : success.
 *
 * ********************************************************** */
int lol_new_ichain(lol_FILE *op, const long olds,
                           const long news, alloc_entry *last_old)
{
  DWORD nb, bs;
  alloc_entry first_index;
  ULONG       file_size;
  long skip, offset;
  alloc_entry current_index;
  alloc_entry skip_indexes;
  size_t      bytes;
  long        tot_olds, j, i = 0;
  FILE *fp;
  int whence = SEEK_SET, err = 0;

  if ((!(op)) || (!(lol_index_buffer)) || (!(last_old))) {
    return LOL_ERR_USER;
  }

  bs   = op->sb.bs;
  nb   = op->sb.nb;
  if (olds > nb) {
     return LOL_ERR_CORR;
  }

  first_index  = op->nentry.i_idx;
  file_size    = op->nentry.fs;
  fp = op->dp;

  *last_old = current_index = first_index;
   skip     = LOL_TABLE_START(op);
   tot_olds = file_size / bs;

           if (file_size % bs)
               tot_olds++;
	   if (!(file_size))
	       tot_olds = 1;
	   if (olds > tot_olds)
               LOL_ERR_RETURN(EIO, LOL_ERR_INTRN);
	   if ((olds + news) > nb)
               LOL_ERR_RETURN(ENOSPC, LOL_ERR_SPACE);

            skip_indexes = tot_olds - olds;
            offset = skip + first_index * ENTRY_SIZE;

            if (fseek(fp, offset, SEEK_SET))
                  return LOL_ERR_IO;

	    // fp points now to the first index in index area.
	    // We must skip some indexes (skip_indexes)
	    // to reach the file position.

	    while (i < skip_indexes) {
                   *last_old = current_index;
                    bytes = fread((char *)&current_index,
                    (size_t)(ENTRY_SIZE), 1, fp);

                   if (bytes != 1)
                       return LOL_ERR_IO;

                   offset = skip + current_index * ENTRY_SIZE;
                   if (fseek(fp, offset, SEEK_SET))
                       return LOL_ERR_IO;
                  i++;
	    } // end while i

     // Do we have olds?
     for (i = 0; i < olds; i++) {

	  if (current_index < 0)
	      return LOL_ERR_CORR;
          lol_index_buffer[i] = current_index;
         *last_old = current_index;
          bytes = fread((char *)&current_index,
                  (size_t)(ENTRY_SIZE), 1, fp);
          if (bytes != 1)
              return LOL_ERR_IO;
	  if (current_index >= 0) {
             offset = skip + current_index * ENTRY_SIZE;
	     whence = SEEK_SET;
	  }
	  else {
            offset = ENTRY_SIZE;
            whence = SEEK_CUR;
	  }
          if (fseek (fp, offset, whence))
              return LOL_ERR_IO;
     } // end for olds

    // If we also have new indexes, then find them also and
    // put them into the buffer.
    // Go back to start
    j = 0; whence = news;

    if (fseek (fp, skip, SEEK_SET))
        return LOL_ERR_IO;

    while (whence) {

	 if (j >= nb) {
	    op->err = lol_errno = ENOSPC;
	    return LOL_ERR_SPACE;
	 }
         bytes = fread((char *)&current_index,
                 (size_t)(ENTRY_SIZE), 1, fp);
         if (bytes != 1)
             return LOL_ERR_IO;
	 if (current_index == FREE_LOL_INDEX) {

             // Save the index and continue
             lol_index_buffer[i++] = j;
             whence--;

	 } // end if found free block
         j++;
    } // end while

   return err;
} // end lol_new_ichain
/* **********************************************************
 * lol_num_blocks:
 * Calculate how many data blocks must be written into.
 * Not so trivial as it sounds. It is not simply
 * bytes / block size, because the file pointer may be anywhere
 * inside the file (or even outside file boundary), so we must
 * be careful!
 *
 * ********************************************************** */
size_t lol_num_blocks(lol_FILE *op, const size_t amount, struct lol_loop *q) {

  size_t        bs = 0;
  size_t    behind = 0;
  size_t     after = 0;
  size_t block_off = 0;
  size_t delta_off = 0;
  size_t     start = 0;
  size_t       end = 0;
  size_t     loops = 0;

  if ((!(op)) || (!(amount)))
     return 0;
  bs  = (size_t)op->sb.bs;
  if (!(bs)) {
      return 0;
  }
  /* SIX POSSIBILITIES:

     1: We write only some full BLOCKS
     2: We write some full BLOCKS and START of a block
     3: We write only START of a block
     4: We write only the END of a block (from somewhere in the middle to the end)
     5: We write END and some full BLOCKS
     6: We write END, some full BLOCKS and START
  */
  block_off = op->curr_pos % bs;
  delta_off = (op->curr_pos + amount) % bs;
     behind = op->curr_pos / bs;

  if (!(block_off)) { // We are on block boundary!
   /*  1: We read only some BLOCKS
       2: We read some BLOCKS and START
       3: We read only START
    */
    if (amount < bs) {
        start = amount;
    }
    else { // amount >= bs
           // We do BLOCKS and optionally START
      loops = amount / bs;
      if (amount % bs) {
          start = delta_off;
      } // end if we do both BLOCKS and START
    } // end else amount >= bs
  } // end if on block_boundary
  else {  // We are in the middle of a block, gotta be carefull !
  /*
     4: We read only the END
     5: We read END and some BLOCKS
     6: We read END, some BLOCKS and START

   */
       end = bs - block_off;
       if (end > amount) {
           end = amount;
       }
       after  = (op->curr_pos + amount) / bs;
       if (amount < bs) {  // We do END and optionally START
          if (after > behind) { // We do START too
              start = delta_off;
	  }
       }
       else {  // amount >= bs
           if (amount == bs) {
	      // Here we know, that we read both END and START but NOT BLOCKS!
              start = block_off;
	   }
           else {  // So, amount > bs
              loops = after - behind - 1;
              if (loops) {  // So, we do END and BLOCKS but shall we do START?
	          // WE do ALL
                  start = delta_off;
	      } // end if we do BLOCKS
              else { // So, don't do BLOCKS but we do START
                 start = amount - end;
              } // end else
          } // end else amount > bs
      } // end else amount >= bs
  } // end else not on boundary
  if (q) { // If q is given, save the results there
      q->num_blocks = loops;
      if (start)
	  q->num_blocks++;
      if (end)
	  q->num_blocks++;
      q->start_bytes = start;
      q->end_bytes   = end;
      q->full_loops  = loops;
  }
  if (start)
     loops++;
  if (end)
     loops++;
  return loops;
} // end lol_num_blocks
/* **********************************************************
 * lol_io_dblock:
 * Reads / writes contents of a given
 * data block into/from user buffer.
 * ret <0 : error
 * ret >=  0 : success, return value is the bytes read/written.
 *
 * ********************************************************** */
long lol_io_dblock(lol_FILE *op, const size_t bnum,
		   char *ptr, const size_t bytes, const int func)
{
  lol_io_func io;
  FILE     *fp;
  fpos_t   pos;
  size_t    bs;
  size_t items;
  size_t   off;
  size_t  left;
  long   block;
  int      ret;

  if ((!(bytes)) || (op->eof))
       return 0;
  if ((bnum < 0) || (bytes < 0))
      LOL_ERR_RETURN(EFAULT, -2);

  // This is a private function. We won't bother checking
  // which open mode the the op is...

  left = (size_t)op->nentry.fs;

  switch (func) {

       case LOL_READ :
            if (op->curr_pos >= left) {
	        op->curr_pos = left;
                op->eof = 1;
                return 0;
	    }
	    io = (lol_io_func)&fread;
	    break;

       case LOL_WRITE :
	    io = (lol_io_func)&fwrite;
	    break;

          default:
            op->err = lol_errno = EFAULT;
            return -3;

  } // end switch

  // nb  = (size_t)op->sb.nb;
  bs  = (size_t)op->sb.bs;
    if (bnum >= op->sb.nb)
        LOL_ERR_RETURN(ENFILE, -4);
    if (bytes > bs)
        LOL_ERR_RETURN(EFAULT, -5);

    left -= op->curr_pos;
    if ((bytes <= left) || (func == LOL_WRITE)) {
	 left = bytes;
    }
    fp = op->dp;
    if (fgetpos (fp, &pos))
	LOL_ERR_RETURN(errno, -7);

    block = LOL_DATA_OFFSET(bs, bnum);

    // Byte offset inside a block?
    off = op->curr_pos % bs;
    // We MUST stay inside block boundary!
    items = bs - off;
    if (left > items) {
	left = items;
    }
    block += off;
    if (fseek(fp, block, SEEK_SET)) {

	ret = ferror(fp);
	if (ret)
          LOL_ERRSET(ret)
	else
          LOL_ERRSET(EIO)

        fsetpos (fp, &pos);
	return -8;

     } // end if seek error

    if ((io(ptr, left, 1, fp)) != 1) {

	ret = ferror(fp);
	if (ret)
          LOL_ERRSET(ret)
	else
          LOL_ERRSET(EIO)

         fsetpos(fp, &pos);
	 return -9;
     }

     op->curr_pos += left;

     if (func == LOL_READ) {

       if ((op->curr_pos >= op->nentry.fs)) {

            op->curr_pos = op->nentry.fs;
            op->eof = 1;
       }
     }

   if (op->curr_pos < 0)
       op->curr_pos = 0;

   fsetpos(fp, &pos);
   return (long)left;
} // end lol_io_dblock
/* **********************************************************
 * lol_free_space: Calculate free space in a container.
 * (Block Size of the container will be returned in 'bs').
 * return value:
 * < 0  : error
 * >= 0 : free space (in bytes of blocks)
 ************************************************************ */
#define LOL_FREESP_TMP 256
long lol_free_space (const char *cont, lol_meta *usb, const int mode)
{
  // NOTE: We are not looking for corruption or invalid
  // parameters here. Just calculate free blocks & space
  const long nes = (long)(NAME_ENTRY_SIZE);
  const long temp_mem = nes * LOL_FREESP_TMP;
  const long  es = (long)(ENTRY_SIZE);
  char temp[temp_mem];
  lol_meta xsb;
  lol_meta *sb;

  FILE           *fp = 0;
  lol_nentry *buffer = 0;
  lol_nentry *nentry = 0;
  alloc_entry   *buf = 0;
  size_t         mem = 0;

  long    nb = 0, io = 0;
  long  nf = 0, doff = 0;
  long ret = 0, frac = 0;
  long bs = 0, files = 0;
  long     data_size = 0;
  long    used_space = 0;
  long   used_blocks = 0;
  long         times = 0;
  long          full = 0;

  int    i, j , k, m = 0;
  int          alloc = 0;
  int          loops = 1;

  if (!(cont))
     return LOL_ERR_PTR;

  switch (mode) {
     case  LOL_SPACE_BYTES  :
       m = LOL_SPACE_BYTES;
       break;
     case  LOL_SPACE_BLOCKS :
       m = LOL_SPACE_BLOCKS;
       break;
     default :
       return LOL_ERR_PARAM;
  } // end switch mode

  if (usb) {
    sb = usb;
  }
  else {
    sb = &xsb;
  }
  full = lol_getsize(cont, sb, NULL, RECUIRE_SB_INFO);
  if (full < LOL_THEOR_MIN_DISKSIZE) {
       return LOL_ERR_CORR;
  }
  bs = (long)sb->bs;
  nb = (long)sb->nb;
  nf = (long)sb->nf;
  if (nf > nb) {
    return LOL_ERR_CORR;
  }
  data_size  = nes * nb;
  io = lol_get_io_size(data_size, nes);
  if (io <= 0) {
      lol_debug("lol_free_space: Internal error: io <= 0");
      return LOL_ERR_INTRN;
  }

  if (io > temp_mem) {
    if (!(buffer = (lol_nentry *)lol_malloc((size_t)(io)))) {
          buffer = (lol_nentry *)temp;
          io     = temp_mem;
    } else {
       mem = (size_t)(io);
       alloc = 1;
    }
  } else {
    buffer = (lol_nentry *)temp;
    io     = temp_mem;
  }

  if (!(fp = fopen(cont, "r"))) {
       ret = LOL_ERR_IO;
       goto just_free;
  }
  if (m == LOL_SPACE_BYTES) {
      doff = (long)LOL_DENTRY_OFFSET_EXT(nb, bs);
  }
  else {
      doff = (long)LOL_TABLE_START_EXT(nb, bs);
  }
  if (fseek (fp, doff, SEEK_SET)) {
      ret = LOL_ERR_IO;
      goto closefree;
  }

 if (m == LOL_SPACE_BYTES) {

     times = data_size / io;
     frac  = data_size % io;
     k = (int)(io / nes);

 dentry_loop:
  for (i = 0; i < times; i++) {
    if ((lol_fio((char *)buffer, io, fp, LOL_READ)) != io) {
         ret = LOL_ERR_IO;
         goto closefree;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...
       nentry = &buffer[j];
       if (!(nentry->name[0])) {
	   continue;
       }
       files++;
       used_space += nentry->fs;
    } // end for j
  } // end for i
  // Now the fractional data
  if ((frac) && (loops)) {
      times = 1;
      io = frac;
      k = (int)(io / nes);
      loops = 0;
      goto dentry_loop;
  } // end if frac
 } // end if m == LOL_SPACE_BYTES
 else {
  // Read also the reserved blocks.
  data_size  = es * nb;
  io = lol_get_io_size(data_size, es);
  times = data_size / io;
  frac  = data_size % io;
  k = (int)(io / es);
  buf = (alloc_entry *)buffer;
  loops = 1;
 index_loop:
  for (i = 0; i < times; i++) {
    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         ret = LOL_ERR_IO;
         goto closefree;
    }
    // printf("DEBUG: lol_free_space: reading %ld bytes of indices\n", io);
    // Now check the entries
    for (j = 0; j < k; j++) {
      if (buf[j] != FREE_LOL_INDEX) {
         used_blocks++;
      }
    } // end for j
  } // end for i
  // Now the fractional data
  if ((frac) && (loops)) {
      times = 1;
      io = frac;
      k = (int)(io / es);
      loops = 0;
      goto index_loop;
  } // end if frac
} // end else counting blocks

  fclose(fp);
  if (alloc) {
     lol_free(mem);
  }

  full  = nb;
  full *= bs;

 if (m == LOL_SPACE_BYTES) {
   if ((used_space > full) ||
       (files != nf)) {
       return LOL_ERR_CORR;
   }
   ret = full - used_space;
 } // end if m == LOL_SPACE_BYTES
 else {
   if (used_blocks > nb) {
       return LOL_ERR_CORR;
   }
   ret = nb - used_blocks;
 } // end else we want free blocks

 return ret;

closefree:
 fclose(fp);
just_free:
 if (alloc) {
    lol_free(mem);
 }
 return ret;
} // end lol_free_space
#undef LOL_FREESP_TMP
/* ********************************************************** */
int lol_garbage_filename(const char *name) {
  size_t i, j, k, ln;
  int found;
  int errs;
  char ch;

  if (!(name))
    return -1;
  k = strlen(name);
  if (!(k))
    return -1;
  ln = strlen(lol_valid_filechars);
  errs = 0;
  for (i = 0; i < k; i++) {
    ch = name[i];
    found = 0;
    for(j = 0; j < ln; j++) {
      if (ch == lol_valid_filechars[j]) {
        found = 1;
	break;
      }
    } // end for j
    if (!(found)) {
       errs++;
    }
  } // end for i
  if (errs > 3)
     return errs;

  return 0;
} // end if lol_garbage_filename
/* ***************************************************************** */
void lol_align(const char *before, const char *after,
               const size_t len, int output) {
  size_t b_len;
  char sp = ' ';
  int  space = (int)(sp);
  size_t i, ln = len;
  int err = 0;

  switch (output) {
     case LOL_STDOUT :
       break;
     case LOL_STDERR :
	 err = 1;
       break;
     default:
       return;
  } // end switch

  if ((!(before)) || (!(after))) {
     return;
  }
  b_len = strlen(before);
  if (len <= b_len) {
    if (err) {
        lol_error("%s %s", before, after);
    }
    else {
	 printf("%s %s", before, after);
    }
    return;
  }
  ln -= b_len;
  if (err)
     lol_error("%s", before);
  else
     printf("%s", before);
  for (i = 0; i < ln; i++) {
    if (err)
       lol_error(" ");
    else
       putchar(space);
  } // end for i
  if (err)
    lol_error("%s", after);
  else
    printf("%s", after);
} // end lol_align
/* **************************************************************** */
int lol_status_msg(const char *me, const char* txt, const int type) {

  char message[256];
  char  output[64];
  char    prog[16];
  const char alias[] = "lolfs";
  size_t len = 0;
  size_t   x = 0;
  const int internal = -(LOL_FSCK_INTRN);
  int  out = LOL_STDOUT;
  int  ret = 0;
  int    i = type;

  if (!(txt)) {
     return internal;
  }
  if ((i < LOL_FSCK_OK) ||
      (i > LOL_FSCK_FATAL))
       i = LOL_FSCK_INTRN;
  if  (i > LOL_FSCK_WARN) {
     out = LOL_STDERR;
  }
  len = strlen(txt);
  if (len > 52) {
      len = 52;
  }
  memset((char *)output, 0, 64);
  if (len) {
     memcpy((char *)output, txt, len);
  }
  memset((char *)message, 0, 256);
  if (me) {
     x = strlen(me);
     if (x > 14) {
         x = 14;
     }
     memset((char *)prog, 0, 16);
     if (x) {
       memcpy((char *)prog, me, x);
     }
     else {
       strcat((char *)prog, alias);
     }
     strcat((char *)message,
            (const char *)prog);
  }
  else {
     strcat((char *)message,
            (const char *)alias);
  }
  strcat((char *)message,
          lol_prefix_list[i]);
  if (len) {
     strcat((char *)message,
            (const char *)(output));
  }
  lol_align(message, lol_tag_list[i],
            LOL_STATUS_ALIGN, out);
  if (i > 0)
     ret = -i;
  return ret;
} // end lol_status_msg
/* ***************************************************************** */
int lol_is_number(const char ch) {
  const char n[] = "1234567890";
  int i;
  for (i = 0; i < 10; i++) {
    if (ch == n[i])
      return 0;
  }
  return -1;
} // end lol_is_number
/* ***************************************************************** */
int lol_is_integer(const char *str) {
  int i, len = 0;
  int ret = 0;
  if (!(str))
    return -1;
#ifdef LOL_INLINE_MEMCPY
  for (; str[len]; len++);
#else
  if(!(str[0]))
    return -1;
  len = (int)strlen(str);
#endif
  // Don't accept integers like 0123
  if ((str[0] == '0') && (len > 1))
    return -1;
  // Check all characters
  for (i = 0; i < len; i++) {
    if ((lol_is_number((const char)str[i]))) {
        ret++;
    }
  } // end for i
  return ret;
} // end lol_is_integer
/* ***************************************************************** */
int lol_size_to_blocks(const char *size, const char *container,
                       const struct lol_super *sb,
                       const struct stat *st, DWORD *nb, int func)
{
  char    siz[256];
  long    raw_size = 0;
  long    dev_size = 0;
  long         ret = 0;
  long          bs = 0;
  long      num_bl = 0;
  long  multiplier = 1;
  size_t       len = 0;
  int        plain = 0;
  int    n_letters = 0;
  char   mult;

  if ((func != LOL_EXISTING_FILE) && (func != LOL_JUST_CALCULATE))
      return -1;
  // We must have at least input and output
  if ((!(size)) || (!(nb)))
     return -1;
  if (func == LOL_EXISTING_FILE) {
    if ((!(container)) || (!(sb)) || (!(st)))
        return -1;

    bs     = (long)sb->bs;
    num_bl = (long)sb->nb;
    if ((bs < 1) || (num_bl < 1))
      return -1;

    // Get block device size if we use one
    if ((S_ISBLK(st->st_mode))) {
        raw_size = lol_rgetsize((char *)container, NULL, NULL);
        if (raw_size < LOL_THEOR_MIN_DISKSIZE)
	   return -1;
    }
  } // end if func == LOL_EXISTING_FILE
  len = strlen(size);
  if ((len < 1) || (len > 12)) { // 12 <--> 1 Tb? Really??
     return -1;
  }
  if (size[0] == '-') {
    return -1;
  }
  mult = size[len - 1];
  n_letters = lol_is_integer(size);
  if ((n_letters < 0) || (n_letters > 1)) {
     return -1;
  }
  if ((n_letters) && (!(lol_is_number(mult)))) {
     return -1;
  }
  if (!(n_letters)) {
      plain = 1;
  }
  else {

     switch (mult) {
       case 'B' :
       case 'b' :
         break;
       case 'K' :
       case 'k' :
         multiplier = LOL_KILOBYTE;
        break;
       case 'M' :
       case 'm' :
         multiplier = LOL_MEGABYTE;
         break;
       case 'G' :
       case 'g' :
         multiplier = LOL_GIGABYTE;
         break;
       case 'T' :
       case 't' :
         multiplier = LOL_TERABYTE;
       break;
      default:
         multiplier = 0;
       break;

    } // end switch
  } // end else is number

  if (!(multiplier))
      return -1;
  if ((!(plain)) && (len < 2))
     return -1;

  LOL_MEMCPY(siz, size, len);
  siz[len] = '\0';
  if (!(plain))
    siz[len - 1] = '\0';

    ret = strtol(siz, NULL, 10);
    if (errno)
      return -1;
#ifdef HAVE_LIMITS_H
#if defined LONG_MIN && defined LONG_MAX
    if ((ret == LONG_MIN) || (ret == LONG_MAX))
        return -1;
#endif
#endif
    if (ret < 1)
      return -1;

    // Ok, we got it.
  ret *= multiplier;
  if (func == LOL_EXISTING_FILE) {

    if ((S_ISBLK(st->st_mode))) {

        dev_size = (long)LOL_DEVSIZE(num_bl, bs);
	dev_size += ret;
       if (dev_size > raw_size)
	  return -1;
    }
    ret /= bs;
    if (!(ret))
      return LOL_FS_TOOSMALL;

  } // end if func == LOL_EXISTING_FILE
  else {
    ret /= LOL_DEFAULT_BLOCKSIZE;
    if (!(ret))
      return LOL_FS_TOOSMALL;;
  }
  if (!(ret))
     return -1;

    *nb = (DWORD)(ret);
    return 0;
} // end lol_size_to_blocks
/* ********************************************************* */
long lol_get_io_size(const long size, const long blk) {

  const long sizes[] = {
    (LOL_GIGABYTE),
    (128 * LOL_MEGABYTE),
    (64 * LOL_MEGABYTE),
    (16 * LOL_MEGABYTE),
    (LOL_MEGABYTE),
    (512 *LOL_KILOBYTE),
    (64 * LOL_KILOBYTE),
    1,
    0
  };
  const long refs[] = {
    ( 32 * LOL_MEGABYTE),
    ( 16 * LOL_MEGABYTE),
    (  8 * LOL_MEGABYTE),
    (  4 * LOL_MEGABYTE),
    (512 * LOL_KILOBYTE),
    (256 * LOL_KILOBYTE),
    (64  * LOL_KILOBYTE),
       (LOL_04KILOBYTES),
    1
  };

  long defret = blk << 4;
  long rv, frac;
  int  i = 0;

  if ((size <= 0) || (blk <= 0))
     return -1;
  if (blk > size)
    return -1;
  if (size % blk)
    return -1;

  if (defret > size) {
      defret = size;
  }

  for (i = 0;  sizes[i]; i++) {
   if (size >= sizes[i]) {
       frac  =  refs[i] % blk;
       rv = refs[i] - frac;
       if (rv >= defret) {
	  return rv;
       }
       break;
   }
  }
  return defret;
} // end lol_get_io_size
/* **********************************************************
 * lol_extendfs: Extends container space by given amount
 *               of data blocks.
 * NOTE: This function does not check much. User should
 *       first use another function (lol_validcont) to
 *       evaluate if the given container is consistent.
 *
 * Input value sb should already have correct values of the
 * current super block.
 * It will - however - be overwritten with new values of
 * the resized container.
 *
 * return value:
 * < 0 : error
 *   0 : success
 ************************************************************ */
#define LOL_EXTENDFS_TMP 512
int lol_extendfs(const char *cont, const DWORD new_blocks, lol_meta *sb)
{
  const size_t  news = (size_t)(new_blocks);
  const long     nes = (long)(NAME_ENTRY_SIZE);
  const long temp_mem = nes * LOL_EXTENDFS_TMP;
  char   temp[temp_mem];
  char        *buf = 0;
  size_t       mem = 0;
  long  nb = 0, bs = 0;
  long   i = 0, io = 0;
  long add = 0, ds = 0;
  long     old_off = 0;
  long     new_off = 0;
  long   table_end = 0;
  long       times = 0;
  long     dir_end = 0;
  size_t      frac = 0;
  int         pass = 0;
  int        alloc = 0;
  FILE         *fp = 0;

  if ((!(cont)) || (!(sb)))
    return -1;
  if (new_blocks < 1)
    return -1;

  nb  = (long)sb->nb;
  bs  = (long)sb->bs;
  add = (long)new_blocks;
  add += nb;

  if (!(nb))
    return -1;

  ds  = nes;
  ds *= (long)(nb);
  io = lol_get_io_size(ds, nes);
  if (io <= 0) {
      lol_debug("lol_extendfs: Internal error: io <= 0");
      return -1;
  }

  if (io > temp_mem) {
    if (!(buf = (char *)lol_malloc((size_t)(io)))) {
          buf = temp;
          io  = temp_mem;
    } else {
       mem = (size_t)(io);
       alloc = 1;
    }
  } else {
    buf = temp;
    io  = temp_mem;
  }

  // First we relocate the index entries
  // Let's calculate their total size
  // and current and new offsets.
   ds  = (long)ENTRY_SIZE;
   ds *= (long)(nb);
  old_off  = (long)LOL_TABLE_START_EXT(nb, bs);
  new_off  = (long)LOL_TABLE_START_EXT(add, bs);
  old_off += ds;
  new_off += ds;
  table_end  = new_off;
  times = ds / io;
  frac  = (size_t)(ds % io);

  fp = fopen(cont, "r+");
  if (!(fp))
    goto ret;

do_relocate:
  if (frac) {

     old_off -= frac;
     new_off -= frac;
     if ((fseek(fp, old_off, SEEK_SET)))
         goto err;
     if ((lol_fio(buf, frac, fp, LOL_READ)) != frac)
         goto err;
     if ((fseek(fp, new_off, SEEK_SET)))
         goto err;
     if ((lol_fio(buf, frac, fp, LOL_WRITE)) != frac)
         goto err;
  } // end if frac

  for (i = 0; i < times; i++) {
     old_off -= io;
     new_off -= io;
     if ((fseek(fp, old_off, SEEK_SET)))
         goto err;
     if ((lol_fio(buf, io, fp, LOL_READ)) != io)
         goto err;
     if ((fseek(fp, new_off, SEEK_SET)))
         goto err;
     if ((lol_fio(buf, io, fp, LOL_WRITE)) != io)
         goto err;
  } // end for i
  pass++;
  // Next we add the new indexes or new name entries
  if (pass == 1) {
     if ((fseek(fp, table_end, SEEK_SET)))
         goto err;
     if ((lol_ifcopy(FREE_LOL_INDEX, news, fp)) != news) {
         goto err;
     }
  }// end if pass == 1
  else {
     frac = (size_t)(news * (size_t)(NAME_ENTRY_SIZE));
     if ((fseek(fp, dir_end, SEEK_SET)))
         goto err;
     if ((lol_fclear (frac, fp)) != frac) {
         goto err;
     }
  } // end else
  if (pass > 1) {
    // Adjust the meta block and return
     sb->nb += new_blocks;
     if ((fseek(fp, 0, SEEK_SET)))
         goto err;
     if ((fwrite((char *)sb,
	 (size_t)DISK_HEADER_SIZE, 1, fp)) != 1)
         goto err;
     if (alloc) {
        lol_free(mem);
     }
     fclose(fp);
     return 0;
  }

  // Next we relocate the name entries
  // Let's calculate their total size
  // and current and new offsets.

   ds  = (long)NAME_ENTRY_SIZE;
   ds *= (long)(nb);
  old_off  = (long)LOL_DENTRY_OFFSET_EXT(nb,  bs);
  new_off  = (long)LOL_DENTRY_OFFSET_EXT(add, bs);
  old_off += ds;
  new_off += ds;
     dir_end  = new_off;
       times  = ds / io;
        frac  = (size_t)(ds % io);

  goto do_relocate;

err:
 fclose(fp);
ret:
 if (alloc) {
     lol_free(mem);
 }
 return -1;
} // end lol_extendfs
#undef LOL_EXTENDFS_TMP
/* ********************************************************** */
// stat structure is here just for reference
#if 0
          struct stat {
               dev_t     st_dev;     /* ID of device containing file */
               ino_t     st_ino;     /* inode number */
               mode_t    st_mode;    /* protection */
               nlink_t   st_nlink;   /* number of hard links */
               uid_t     st_uid;     /* user ID of owner */
               gid_t     st_gid;     /* group ID of owner */
               dev_t     st_rdev;    /* device ID (if special file) */
               off_t     st_size;    /* total size, in bytes */
               blksize_t st_blksize; /* blocksize for file system I/O */
               blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
               time_t    st_atime;   /* time of last access */
               time_t    st_mtime;   /* time of last modification */
               time_t    st_ctime;   /* time of last status change */
           };
#endif
