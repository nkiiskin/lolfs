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

/* $Id: lolfs.c, v0.20 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $" */

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



// TODO: Fix lol_cc.c --> buffer the i/o
// TODO: lol_file.h: add last free nentry index to sb
// TODO: lolfs.c:    buffer lol_free_nentry
// TODO: lol_cc.c:   When invalid file found -> get details

/* ********************************************************** */
// Some globals
/* ********************************************************** */
int lol_errno = 0;
int lol_buffer_lock = 0;
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

const char  lol_usefsck_txt[] = LOL_FSCK_FMT;
const char  lol_cantuse_txt[] = LOL_CANTUSE_FMT;
const char lol_cantread_txt[] = LOL_CANTREAD_FMT;
const char     lol_help_txt[] = LOL_HELP_FMT;
alloc_entry *lol_index_buffer = 0;
alloc_entry  lol_storage_buffer[LOL_STORAGE_SIZE+1];
const char lol_valid_filechars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-_:!+#%[]{}?,;&()1234567890=~@";
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
 * lol_get_rawdevsize: PRIVATE FUNCTION, USE WITH CAUTION!
 * Return the size (in bytes) of a given raw block device
 * or regular file or link to a regular file or -1, if error.
 * Most likely must be run as root if it is ablock device.
 *
 *  NOTE:
 *  The function may be called with or without 'sb' and/or 'sta'
 *  if or if not those information(s) are needed.
 * ********************************************************** */
long lol_get_rawdevsize(char *device, struct lol_super *sb, struct stat *sta) {

  long ret = -1;
  int  fd;
#ifdef HAVE_LINUX_FS_H
  long size = 0;
#endif
  ssize_t bytes;
  struct stat st;

  if (!(device))
    return -1;
  if (stat(device, &st))
    return -1;
  if (sta)
    LOL_MEMCPY(sta, &st, sizeof(st));
  if (!(sb)) { // Return fast if sb info is not recuired
      if (S_ISREG(st.st_mode))
          return (long)(st.st_size);
#ifdef HAVE_LINUX_FS_H
      if (!(S_ISBLK(st.st_mode)))
           return -1;
#else
      return -1;
#endif
  } // end if sb info is not needed

  fd = open(device, O_RDONLY);
  if (fd < 0)
    return -1;
  if (sb) {
     bytes = read(fd, (char *)sb,
          (size_t)DISK_HEADER_SIZE);
     if (bytes != DISK_HEADER_SIZE)
         goto error;
  }
  if (S_ISREG(st.st_mode)) {
       ret = (long)(st.st_size);
       goto error;
  }
#ifdef HAVE_LINUX_FS_H
  if (ioctl(fd, BLKGETSIZE64, &size) < 0)
      goto error;
  if (!(S_ISBLK(st.st_mode)))
        goto error;
  ret = size;
#else
  ret = -1;
#endif
error:
  close(fd);
  return ret;
} // end lol_get_rawdevsize
/* **********************************************************
 * lol_get_vdisksize: PRIVATE FUNCTION, USE WITH CAUTION!
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
long lol_get_vdisksize(char *name, struct lol_super *sb,
                       struct stat *sta, int func)

 {

  struct stat st;
  mode_t m;
  long size;
  long bs;
  long nb;
  long min_size;

  if ((!(name)) || (!(sb)))
    return -1;

  switch (func) {

          case RECUIRE_SB_INFO:
	    size = lol_get_rawdevsize(name, sb, &st);
	    break;
          case USE_SB_INFO:
	    size = lol_get_rawdevsize(name, NULL, &st);
	    break;
          default:
            return -1;

  } // end switch

    if (size < LOL_THEOR_MIN_DISKSIZE) {
        return -1;
    }

    bs = (long)sb->bs;
    nb = (long)sb->nb;

    if ((nb < 1) || (bs < 1)) {
        return -1;
    }

    min_size = (long)LOL_DEVSIZE(nb, bs);

    if (sta) {
      LOL_MEMCPY(sta, &st, sizeof(st));
    }
    m = st.st_mode;

    if ((S_ISREG(m))) {

       if (size != min_size) {
           return -1;
       }
       else
         return size;

    } // end if regular file
    else {
#ifdef HAVE_LINUX_FS_H

          if ((S_ISBLK(m))) {

               if (size < min_size) {
	           return -1;
               }
               else {
                 return min_size;
               }
          } // end if block device

#endif
    }

    /* If the file is not a regular file
       or a block device, we can't use it! */
   return -1;
} // end lol_get_vdisksize
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
  if (op->vdisk_size < LOL_THEOR_MIN_DISKSIZE)
    return -3;
  size = (ULONG)LOL_DEVSIZE(op->sb.nb, op->sb.bs);
  if (op->vdisk_size != size)
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
  if (!(op->vdisk))
    return LOL_ERR_BADFD;
  if ((mode & LOL_CHECK_SB)) {
    if ((lol_valid_sb(op)))
       return LOL_ERR_CORR;
  } // end if LOL_CHECK_SB

  if ((mode & LOL_CHECK_FILE)) {

    bs = op->sb.bs;
    nb = op->sb.nb;
    fs = op->nentry.file_size;

    space = (ULONG)(((ULONG)(nb)) * ((ULONG)(bs)));
    if (fs > space)
      return LOL_ERR_CORR;
    if (!(op->vdisk_file)) // unnecessary?
      return LOL_ERR_CORR;
    if (!(op->vdisk_file[0]))
      return LOL_ERR_CORR;
    if (!(op->nentry.filename[0]))
      return LOL_ERR_CORR;

    f_idx = op->nentry.i_idx;
    n_idx = op->nentry_index;

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
int lol_remove_nentry(FILE *f, const DWORD nb, const DWORD bs,
                      const DWORD nentry, int remove_idx) {

  struct lol_name_entry p;
  fpos_t old_pos, tmp;
  long offset;
  alloc_entry idx, v;
  int ret = -1;

  if ((!(f)) || (nentry >= nb) ||
      (!(nb)) || (!(bs)))
      return -1;
  if (lol_try_fgetpos(f, &old_pos))
      return -1;
  if (LOL_GOTO_NENTRY(f, nb, bs, nentry))
      goto error;
  if (remove_idx) {
    // Free the index also
    if (lol_try_fgetpos(f, &tmp))
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
    if (lol_try_fsetpos(f, &tmp))
        goto error;
  } // end if delete index too

  memset((char *)&p, 0, NAME_ENTRY_SIZE);
  if (fwrite((char *)&p, NAME_ENTRY_SIZE, 1, f) != 1)
      goto error;

  ret = 0;
error:
  lol_try_fsetpos(f, &old_pos);
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
  if (lol_try_fgetpos(f, &p))
      return LOL_FALSE;
  off = (long)LOL_INDEX_OFFSET(nb, bs, idx);
  if (fseek(f, off, SEEK_SET))
      goto error;
  if ((fread((char *)&v, ENTRY_SIZE, 1, f)) != 1)
      goto error;
  ret = v;
error:
    lol_try_fsetpos(f, &p);
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
int lol_set_index_value(FILE *f, const DWORD nb, const DWORD bs,
               const alloc_entry idx, const alloc_entry val)
{
  fpos_t p;
  long off;
  int  ret = -1;

  if ((!(f)) || (idx < 0) || (idx >= nb) ||
      (!(nb)) || (!(bs)))
    return -1;
  // Don't allow illegal values, sort them out..
  if ((val == LAST_LOL_INDEX) ||
      (val == FREE_LOL_INDEX)) {
      goto legal;
  }
  if ((val < 0) || (val >= nb))
      return -1;
legal:
    if (lol_try_fgetpos(f, &p))
        return -1;
    off = (long)LOL_INDEX_OFFSET(nb, bs, idx);
    if (fseek(f, off, SEEK_SET))
        goto error;
    if (fwrite((char *)&val, ENTRY_SIZE, 1, f) != 1)
        goto error;

    ret = 0;
error:
    lol_try_fsetpos(f, &p);
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
  alloc_entry *lsb;
  size_t bytes;

  if (lol_buffer_lock)
     LOL_ERRET(EBUSY, LOL_ERR_BUSY);

  // Ok, we can aloocate
  if (!(num))
     LOL_ERRET(EINVAL, LOL_ERR_PARAM);

  if (num <= (LOL_STORAGE_SIZE)) {
      // Clear first (Fake clear)
      lsb = lol_storage_buffer;
      lsb[LOL_STORAGE_SIZE] = LAST_LOL_INDEX;
      lsb[num-1] = lsb[num] = LAST_LOL_INDEX;
      /*
      for (i = 0; i < y; i++) {
        lol_storage_buffer[i] = LAST_LOL_INDEX;
      }
      */
      lol_index_buffer = lsb;
      lol_buffer_lock = 1;
      return LOL_OK;
  } // end if small enough

  // So, we must allocate dynamic mem
   bytes  = p * es;

   if (!(lol_index_buffer = (alloc_entry *)malloc(bytes)))
       LOL_ERRET(ENOMEM, LOL_ERR_MEM)

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

  entries = bytes / e_size;
     frac = bytes % e_size;
  if (frac)
    entries++;

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
  alloc_entry buf[4096];
  const size_t es = (size_t)(ENTRY_SIZE);
  const size_t bytes = times * es;
  size_t i, tm, fr, ret = 0;

  if ((!(s)) || (!(times)))
     return 0;
  for (i = 0; i < 4096; i++) {
     buf[i] = val;
  }
  tm = bytes >> LOL_DIV_4096;
  fr = bytes % 4096;
  for (i = 0; i < tm; i++) {
    if ((fwrite(buf, 4096, 1, s)) != 1)
      goto calc; // return ret;
    ret += 4096;
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
  char buf[4096];
  if ((!(s)) || (!(bytes)))
    return 0;
  last  = bytes % 4096;
  times = bytes >> LOL_DIV_4096;
  memset(buf, 0, 4096);
  for (i = 0; i < times; i++) {
      if (fwrite(buf, 4096, 1, s) != 1) {
               return ret;
      }
      ret += 4096;
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
               FILE *s, const int func)

{
  size_t i, last, times;
  size_t t = 0;
  lol_io_func io = NULL;

  if ((!(ptr)) || (bytes <= 0) || (!(s)))
    return 0;

  switch (func) {
       case LOL_READ:
            io = &fread;
	    break;
       case LOL_WRITE:
	    io = (lol_io_func)&fwrite;
	    break;
          default:
            return 0;
  } // end switch
  last  = bytes % 4096;
  times = bytes >> LOL_DIV_4096;
  for (i = 0; i < times; i++) {
        if (io((char *)&ptr[t], 4096, 1, s) != 1)
             return t;
        t += 4096;
  }
  if (last) {
    if ((io((char *)&ptr[t], last, 1, s)) != 1)
          return t;
        t += last;
  }
  return t;
} // end lol_fio
/* ********************************************************** */
BOOL lol_is_validfile(char *name) {
  struct lol_super sb;
  long sz;
  if (!(name))
       return 0;
  sz = lol_get_vdisksize(name, &sb, NULL, RECUIRE_SB_INFO);
  if (sz < LOL_THEOR_MIN_DISKSIZE)
	return 0;
  if (LOL_INVALID_MAGIC)
       return 0;
   //  Looks like a valid lol container
   return 1;
} // end lol_is_validfile
/* ********************************************************* */
void lol_help(const char* lst[]) {
  int i = 0;
  if (!(lst))
    return;
  while (lst[i]) {
    puts(lst[i++]);
  };
} // end lol_help
/* ********************************************************* */
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
int lol_count_file_blocks(FILE *vdisk, const struct lol_super *sb,
                          const alloc_entry first_index,
                          const long dsize, long *count,
                          const int terminate)

{
  alloc_entry c_idx;
  long  nb;
  long  bs;
  fpos_t old_pos;
  long i, size;
  long skip, offset;
  int ret = -1;

  if ((!(vdisk)) || (!(sb)) || (!(count)))
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
  if (lol_try_fgetpos(vdisk, &old_pos))
      return -1;
  for (i = 0; i < nb; i++) {
        offset = skip + c_idx * ENTRY_SIZE;
        if ((fseek(vdisk, offset, SEEK_SET))) {
	   *count = i + 1;
            goto error;
        }
	if ((fread((char *)&c_idx,
		   (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
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
                 if ((fseek(vdisk, -(ENTRY_SIZE), SEEK_CUR))) {
                      goto error;
                 }
		 c_idx = LAST_LOL_INDEX;
	         if ((fwrite((char *)&c_idx,
			     (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {

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
      if ((fseek(vdisk, -(ENTRY_SIZE), SEEK_CUR))) {
           goto error;
      }
      c_idx = LAST_LOL_INDEX;
      if ((fwrite((char *)&c_idx,
		  (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
           goto error;
      }
      ret = 0;
      goto error;
  } // end if terminate
  *count = i;
     ret = 1;
error:
    lol_try_fsetpos(vdisk, &old_pos);
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
int lol_supermod(FILE *vdisk, struct lol_super *sb, const int func) {

  DWORD nb;
  DWORD bs;
  int   files;
  fpos_t pos;
  struct lol_super s;
  int fix  = 0;
  int ret = -1;

  if ((!(vdisk)) || (!(sb)))
     return -1;
  nb  = sb->nb;
  bs  = sb->bs;
  files = (int)sb->nf;
  if ((!(nb)) || (!(bs)))
    return -1;
  switch (func) {

    case  LOL_INCREASE:
      if (files == nb)
	return -1;
      if (files > nb)
	fix = 1;
      break;

    case  LOL_DECREASE:
      if (!(files))
	return -1;
      if (files < 0)
	fix = 1;
      break;

    default:
    return -1;
  } // end switch
  if (lol_try_fgetpos(vdisk, &pos))
      return -1;
  if (fseek(vdisk, 0, SEEK_SET))
      goto error;
  if ((fread((char *)&s, (size_t)(DISK_HEADER_SIZE),
       1, vdisk)) != 1)
      goto error;
  if (fseek(vdisk, 0, SEEK_SET))
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
       (size_t)(DISK_HEADER_SIZE), 1, vdisk)) != 1)
       goto error;
  if (fix)
    goto error;
    ret = 0;
 error:
  lol_try_fsetpos(vdisk, &pos);
  return ret;
} // end lol_supermod
/* **********************************************************
 * lol_inc_files: PRIVATE FUNCTION!
 * Increment the superblock file counter.
 * ********************************************************** */
int lol_inc_files(lol_FILE *op) {
    return lol_supermod(op->vdisk, &(op->sb), LOL_INCREASE);
} // end lol_inc_files
/* **********************************************************
 * lol_dec_files: PRIVATE FUNCTION!
 * Decrement the superblock file counter.
 * ********************************************************** */
int lol_dec_files(lol_FILE *op) {
    return lol_supermod(op->vdisk, &(op->sb), LOL_DECREASE);
} // end lol_dec_files
/* **********************************************************
 * delete_chain_from: PRIVATE FUNCTION!
 * Delete index chain of the argument file.
 * If flags is set, then it will save the first allocated block
 * and delete all the rest.
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
  alloc_entry last_index;
  FILE *vdisk       = 0;
  DWORD first_index = 0;
  DWORD nb = 0;
  DWORD bs = 0;
  ULONG fs = 0;
  DWORD file_blocks = 1;
  long  skip;
  long  offset;
  int   rv;
  DWORD i = 1;
  int   j = 1;

    if ((rv = lol_check_corr(op, LOL_CHECK_BOTH))) {
#if LOL_TESTING
       lol_error("lol_delete_chain_from: lol_check_corr() = %d\n", rv);
#endif
       return rv;
    }
    vdisk = op->vdisk;
    first_index = op->nentry.i_idx;
    nb = op->sb.nb;
    bs = op->sb.bs;
    fs = op->nentry.file_size;
    last_index = nb - 1;

    if (flags) {
       j = 0;
    }
    skip   = LOL_TABLE_START(op);
    offset = skip + first_index * ENTRY_SIZE;
    if (fseek(vdisk, offset, SEEK_SET)) {
#if LOL_TESTING
    lol_error("lol_delete_chain_from: fseek failed. offset = %ld\n", offset);
#endif
       lol_errno = errno;
       return LOL_ERR_IO;
    }
    // How many blocks in the file?
    if (fs) {
        file_blocks = fs / bs;
	if (fs % bs)
	    file_blocks++;
    } // end if fs
    if (file_blocks > nb) {
#if LOL_TESTING
        lol_error("lol_delete_chain_from: file_blocks (%u) > nb (%u)\n",
	          file_blocks, nb);
#endif
        return LOL_ERR_CORR;
    }
    while (1) {
      if ((fread((char *)&current_index,
                 (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
#if LOL_TESTING
    lol_error("lol_delete_chain_from: fread failed. current_index = %d\n", current_index);
#endif
	if ((ferror(vdisk))) {
	   lol_errno = errno;
           return LOL_ERR_IO;
	}
	else{
	  return LOL_ERR_CORR;
	}
      } // end if fread
      else {
	if (feof(vdisk)) {
	  return LOL_ERR_CORR;
	}
      }
      if (current_index > last_index) {
#if LOL_TESTING
    lol_error("lol_delete_chain_from: current_index (%d) > last_index (%d)\n",
	      current_index, last_index);
#endif
	return LOL_ERR_CORR;
      }
      // Clear and read next entry
      if ((fseek(vdisk, -(ENTRY_SIZE), SEEK_CUR))) {
	  lol_errno = errno;
	  return LOL_ERR_IO;
      }
      if (j) {
	  if ((fwrite((const char *)&empty,
                      (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
#if LOL_TESTING
    lol_error("lol_delete_chain_from: fwrite failed\n");
#endif
	    if ((ferror(vdisk))) {
 	         lol_errno = errno;
                 return LOL_ERR_IO;
	    }
	    else{
	        return LOL_ERR_CORR;
	    }
	  } // end if fwrite
      }
      else {
	  if ((fwrite((const char *)&last,
                      (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
#if LOL_TESTING
    lol_error("lol_delete_chain_from: fwrite [2] failed\n");
#endif

	     if ((ferror(vdisk))) {
	         lol_errno = errno;
                 return LOL_ERR_IO;
	     }
	     else{
	         return LOL_ERR_CORR;
	     }
	  } // end if fwrite
          j = 1;
      } // end else
      if (current_index == LAST_LOL_INDEX)
          return LOL_OK;
      // Next offset
      offset = skip + current_index * ENTRY_SIZE;
      if ((fseek(vdisk, offset, SEEK_SET))) {
	  lol_errno = errno;
	  return LOL_ERR_IO;
      }
      if (++i > file_blocks) // VERIFY
	  return LOL_ERR_CORR;
  } // end while
  return LOL_ERR_INTRN;
} // end lol_delete_chain_from
/* **********************************************************
 * lol_read_nentry: // PRIVATE function. Checks if file
 * (given by name in op->vdisk_file) exists.
 * ret <  0 : error
 * ret =  0 : does not exist
 * ret =  1 : exists
 *
 * The file (op->vdisk) must be already opened for reading (and
 * optionally writing). The op->sb must be set.
 * op->nentry and op->nentry_index will be overwritten.
 * ********************************************************** */
int lol_read_nentry(lol_FILE *op) {

  long  doff;
  alloc_entry i;
  fpos_t pos;
  FILE   *vdisk;
  char   *name;
  size_t len, nlen;
  DWORD  nb;
  int ret = -1;

  struct lol_name_entry *entry;
  if (!(op))
    return -1;

  vdisk      = op->vdisk;
  name       = op->vdisk_file;
  nb = op->sb.nb;
  entry      = &op->nentry;

  if ((!(vdisk)) || (!(name)))
    return -1;
  if ((!(name[0])) || (!(nb)))
    return -1;
  if (lol_try_fgetpos (vdisk, &pos))
     return -1;
  doff = (long)LOL_DENTRY_OFFSET(op);
  if (fseek (vdisk, doff, SEEK_SET)) {
      lol_errno = errno;
      goto error;
  }
  nlen = strlen(name);
  if (nlen >= LOL_FILENAME_MAX) {
      lol_errno = ENAMETOOLONG;
      goto error;
  }
  for (i = 0; i < nb; i++) {
    if ((fread ((char *)entry, (size_t)(NAME_ENTRY_SIZE),
                1, vdisk)) != 1)  {
         lol_errno = errno;
	 goto error;
    }
    if (!(entry->filename[0]))
	continue;
    len = strlen((char *)entry->filename);
    if ((len >= LOL_FILENAME_MAX) || (len != nlen))
	continue;
    if (!(strncmp((const char *)name,
         (const char*)entry->filename, len)))
    {
         lol_try_fsetpos (vdisk, &pos);
	 op->nentry_index = i;
         return 1;
    }
  } // end for i
  ret = 0;
error:
  lol_try_fsetpos (vdisk, &pos);
  return ret;
} // end lol_read_nentry
/* **********************************************************
 * lol_get_basename:
 * Copies the 'file' part from name like foo:/file to
 * buffer new_name.
 *
 * Sets lol_errno
 * Return value:
 * -1 : error
 *  0 : success
 *********************************************************** */
int lol_get_basename(const char* name, char *new_name, const int mode) {

  int i, ln, found = 0;

  if ((!(name)) || (!(new_name)))
    LOL_ERRET(EFAULT, -1);
  if (!(name[0]))
    LOL_ERRET(EINVAL, -1);

  switch (mode) {
     case LOL_FORMAT_TO_REGULAR :
     case    LOL_LOCAL_TRUNCATE :
       break;
     default:
       lol_errno = EINVAL;
       return -1;
  } // end switch mode

  ln = strlen(name);
  if ((ln < 4) &&
      (mode == LOL_FORMAT_TO_REGULAR))
    LOL_ERRET(ENOENT, -1);
  if (name[ln-1] == '/')
#ifdef HAVE_LINUX_FS_H
    LOL_ERRET(EISDIR, -1);
#else
    LOL_ERRET(EINVAL, -1);
#endif
  if (mode == LOL_FORMAT_TO_REGULAR) {
    for (i = ln - 2; i > 1; i--) {
      if ((name[i] == '/') && (name[i-1] == ':')
          && (name[i-2] != '/')) {
          found = 1;
          break;
      }
    } // end for i
  } // end if mode == LOL_FORMAT_TO_REGULAR
  else {  // Must be mode == LOL_LOCAL_TRUNCATE
    for (i = ln - 1; i >= 0; i--) {
      if (name[i] == '/') {
         found = 1;
         break;
      }
    } // end for i
  } // end else

  if (found)
      i++;
  else {
      if (mode == LOL_FORMAT_TO_REGULAR)
          LOL_ERRET(ENOENT, -1)
      else
        i = 0;
  }
  ln = strlen(&name[i]);
  if (!(ln))
    LOL_ERRET(ENAMETOOLONG, -1);
  if (ln >= LOL_FILENAME_MAX) {
     lol_errno = ENAMETOOLONG;
     ln = LOL_FILENAME_MAX-1;
  }
  memset(new_name, 0, LOL_FILENAME_MAX);
  LOL_MEMCPY(new_name, &name[i], ln);
  if (!(new_name[0])) // Should never happen here
    LOL_ERRET(EBADFD, -1);
  return 0;
} // end lol_get_basename
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
    { .device = 0, .mode_num = 0, .mode_str = "r",  .vd_mode = "r"   },
    { .device = 0, .mode_num = 1, .mode_str = "r+", .vd_mode = "r+"  },
    { .device = 0, .mode_num = 2, .mode_str = "w",  .vd_mode = "r+"  },
    { .device = 0, .mode_num = 3, .mode_str = "w+", .vd_mode = "r+"  },
    { .device = 0, .mode_num = 4, .mode_str = "a",  .vd_mode = "r+"  },
    { .device = 0, .mode_num = 5, .mode_str = "a+", .vd_mode = "r+"  },
};
/* ********************************************************** */
void lol_clean_fp(lol_FILE *fp) {
  if (!(fp))
    return;
  memset((char *)fp, 0, LOL_FILE_SIZE);
} // end lol_clean_fp
/* ********************************************************** */
lol_FILE *new_lol_FILE() {

  lol_FILE *fp = LOL_ALLOC(struct _lol_FILE);
  if (!(fp))
    return NULL;
  lol_clean_fp(fp);
  fp->open_mode.mode_num = -1;
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
 * lol_get_filename: // PRIVATE function.
 * Split full lolfile path (like: /path/mylolfile:/README)
 * into two parts ("/path/mylolfile" and "README") and
 * stores the values into op->vdisk_file and op->vdisk_name.
 *
 * Sets lol_errno
 * Return value:
 * <0 : error
 *  0 : success
 * ********************************************************** */
int lol_get_filename(const char *path, lol_FILE *op)
{
  int i, j, s;
  int len;
  int valid = 0;

  if (!(path))
    LOL_ERRET(EFAULT, -1);
  if (!(op))
    LOL_ERRET(EINVAL, -1);
  len = strlen(path);
  if (len < 4)
    LOL_ERRET(ENOENT, -1);
  // Copy second part to op->vdisk_file
  memset((char *)op->vdisk_file, 0, LOL_FILENAME_MAX);
  if (lol_get_basename(path, op->vdisk_file,
      LOL_FORMAT_TO_REGULAR))
  {
     // lol_errno already set
     return -1;
  }
  j = len - 1;
  for (i = 1; i < j; i++) {
    if ((path[i] == ':') && (path[i+1] == '/') &&
        (i != (j-1)) && (path[i-1] != '/')) {
      valid = 1; s = i - 1;
      break;
    }
  } // end for i
  if (!(valid))
    LOL_ERRET(EINVAL, -1);
  if (s > (LOL_DEVICE_MAX - 2))
     LOL_ERRET(ENAMETOOLONG, -1);
  // Copy first part to op->vdisk_name
  memset((char *)op->vdisk_name, 0, LOL_DEVICE_MAX);
  for (; s >= 0 ; s--) {
    op->vdisk_name[s] = path[s];
  } // end for s
  if (!(op->vdisk_name[0])) // Should never happen here..
      LOL_ERRET(EINVAL, -1);
  return 0;
} // end lol_get_filename
/* ********************************************************** */
int lol_is_writable(const lol_FILE *op) {

  int    m;
  DWORD nb;
  DWORD bs;
  ULONG ds;
  UCHAR fe;

  if (!(op))
    return -1;
  nb   = op->sb.nb;
  bs   = op->sb.bs;
  fe   = op->vdisk_file[0];
  m    = op->open_mode.mode_num;
  if ((!(nb)) || (!(bs)) || (!(fe)))
    return -2;
  if (m < 1) // Opened "r"?
     return -3;
  ds = (ULONG)LOL_DEVSIZE(nb, bs);
  if (op->vdisk_size != ds)
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
 * lol_try_fclose:
 * Try fclose 30 times max
 *
 * Return value:
 * < 0 : error
 *   0 : success
 * ********************************************************** */
int lol_try_fclose(FILE *fp) {
  int i;
  if (!(fp)) {
    lol_errno = EBADF;
    return -1;
  }
  for (i = 0; i < 30; i++) {
    if (!(fclose(fp)))
       return 0;
  } // end for i
  lol_errno = errno;
  return -1;
} // end lol_try_fclose
/* ********************************************************** *
 * lol_try_fgetpos:
 * Try fgetpos 30 times max
 *
 * Return value:
 * < 0 : error
 *   0 : success
 * ********************************************************** */
int lol_try_fgetpos(FILE *fp, fpos_t *pos) {
  int i;
  if ((!(fp)) || (!(pos)))
    return -1;
  for (i = 0; i < 30; i++) {
    if (!(fgetpos(fp, pos)))
       return 0;
  } // end for i
  return -1;
} // end lol_try_fgetpos
/* ********************************************************** *
 * lol_try_fsetpos:
 * Try fsetpos 30 times max
 *
 * Return value:
 * < 0 : error
 *   0 : success
 * ********************************************************** */
int lol_try_fsetpos(FILE *fp, const fpos_t *pos) {
  int i;
  if ((!(fp)) || (!(pos)))
    return -1;
  for (i = 0; i < 30; i++) {
    if (!(fsetpos(fp, pos)))
       return 0;
  } // end for i
  return -1;
} // end lol_try_fsetpos
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
  if ((lol_try_fgetpos(fp, &pos)))
     return -1;
  if ((fseek(fp, 0, SEEK_SET)))
     goto error;
  if ((fread((char *)sb, sbs, 1, fp)) != 1)
     goto error;
  rv = 0;
 error:
  lol_try_fsetpos(fp, &pos);
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
int lol_get_free_index(FILE *fp, const lol_meta *sb, alloc_entry *idx, int flag)
{
  alloc_entry temp[LOL_FREE_IDX_BUFF];
  const long idx_s = (long)(ENTRY_SIZE);
  alloc_entry     newe;
  fpos_t           pos;
  size_t       mem = 0;
  size_t      data = 0;
  long          nb = 0;
  long          bs = 0;
  alloc_entry *buf = 0;
  long           i = 0;
  long           j = 0;
  long          io = 0;
  long    idx_area = 0;
  long      offset = 0;
  long       times = 0;
  long       n_idx = 0;
  long        frac = 0;
  long        full = 0;
  int        alloc = 0;
  int         ret = -1;
  int   doing_blk  = 1;
  alloc_entry what = 0;

  if ((!(fp)) || (!(sb)) || (!(idx)))
      return -1;

  newe = *idx;
  nb = (long)sb->nb;
  bs = (long)sb->bs;

  if ((!(nb)) || (!(bs)))
     return -1;
  if (((newe >= 0) && (newe < nb)) ||
       (newe == LAST_LOL_INDEX)    ||
       (newe == FREE_LOL_INDEX)) {
    goto legal;
  }
  return -1;

 legal:
  idx_area = nb * idx_s;
  io = lol_get_io_size(idx_area, idx_s);
  if (io <= 0)
     return -1;

  // We try to suck in all the indices
  // at the same time
  if ((lol_index_malloc(io))) { // Failed -> use temp
      buf   = temp;
      io    = idx_s * LOL_FREE_IDX_BUFF;
  }
  else {
      buf   = lol_index_buffer; // Success -> use allocated buffer
      mem = (size_t)(io);
      alloc = 1;
  }
  offset = (long)LOL_TABLE_START_EXT(nb, bs);
  times  = idx_area / io;
  frac   = idx_area % io;
  full   = idx_area - frac;
  full  /= idx_s;

  if ((lol_try_fgetpos(fp, &pos)))
       goto err;
  if ((fseek(fp, offset, SEEK_SET)))
       goto error;
  n_idx = io / idx_s;
  if ((io % idx_s) || (frac % idx_s)) {
    lol_debug("internal error, sorry!");
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
  lol_try_fsetpos(fp, &pos);
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
long lol_get_free_nentry(FILE *fp, const lol_meta *sb, lol_ninfo *info) {

  char temp[LOL_FREEDENTRY_TMP * NAME_ENTRY_SIZE];
  const long  nes =  ((long)(NAME_ENTRY_SIZE));
  const long   ms =  ((long)(DISK_HEADER_SIZE));
  fpos_t      pos;
  lol_nentry *buf = 0;
  size_t      mem = 0;
  long  i = 0,  j = 0;
  long  k = 0, io = 0;
  long ds=0, base = 0;
  long nb = 0, bs = 0;
  long      times = 0;
  long       frac = 0;
  long       full = 0;
  long       ret = -1;
  int       alloc = 0;
  int       loops = 1;

  if ((!(fp)) || (!(sb))  || (!(info)))
    LOL_ERRET(EFAULT, LOL_ERR_PARAM);

  nb = (long)sb->nb;
  bs = (long)sb->bs;

  base = nb * bs + ms;
  ds = nb * nes;
  io = lol_get_io_size(ds, nes);
  if (io <= 0) {
      lol_debug("lol_df: Internal error: io <= 0");
      lol_errno = EIO; return -1;
  }
  if (!(buf = (lol_nentry *)lol_malloc((size_t)(io)))) {
        buf = (lol_nentry *)temp;
        io  = LOL_FREEDENTRY_TMP * nes;
  }
  else {
     alloc = 1;
     mem = (size_t)(io);
  }

  times = ds / io;
  frac  = ds % io;
  k     = io / nes;
  full  = ds - frac;

  if ((lol_try_fgetpos(fp, &pos)))
      goto freemem;
  j = (long)LOL_DENTRY_OFFSET_EXT(nb, bs);
  if (fseek (fp, j, SEEK_SET))
      goto position;

 dentry_loop:
  for (; i < times; i++) {

    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         goto position;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...

        if (buf[j].filename[0]) {
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
       k = io / nes;
       loops = 0;
      goto dentry_loop;
  } // end if frac
position:
  lol_try_fsetpos(fp, &pos);
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
 * Assumes that the file name is already in op->vdisk_file.
 * NOTE: DOES NOT RETURN ERROR IF FILE EXISTS ALREADY!
 * (This is basically a helper function for lol_fopen).
 *
 * Return < 0 if error.
 * ********************************************************** */
#define LOL_TOUCH_ERROR(x,y) { op->err = lol_errno = (x); ret = (y); goto error; }
int lol_touch_file(lol_FILE *op) {

  lol_ninfo info;
  FILE *fp;
  char *name;
  fpos_t pos;
  size_t len;
  DWORD nb, bs, nf;
  alloc_entry idx = LAST_LOL_INDEX;
  int mode, ret = -1;
  long off;

  if (!(op))
    LOL_ERRET(EFAULT, -1);

 // Make some aliases
  nb   = op->sb.nb;
  bs   = op->sb.bs;
  nf   = op->sb.nf;
  name = op->vdisk_file;
  mode = op->open_mode.mode_num;

  // Make some checks
  if ((!(nb)) || (!(bs)) || (!(name)))
    LOL_ERR_RETURN(EIO, -2);
  if (mode < 2) /* Opened "r" or "r+"? */
     LOL_ERR_RETURN(EPERM, -3);
  if (nf >= nb)
     LOL_ERR_RETURN(ENOSPC, -4);
  if(!(name[0]))
     LOL_ERR_RETURN(EIO, -5);
  len = strlen(name);
  if (len >= LOL_FILENAME_MAX) {
      len = LOL_FILENAME_MAX - 1;
      name[len] = '\0';
      lol_errno = ENAMETOOLONG;
  }
  fp = op->vdisk;
  if (lol_try_fgetpos(fp, &pos))
     LOL_ERR_RETURN(errno, -7);

  // Ok, action begins here...
  // Find a free index and reserve it
  if ((lol_get_free_index(fp, &op->sb, &idx, LOL_MARK_USED)))
     LOL_TOUCH_ERROR(ENOSPC, -10);

  // create a zero sized file
  memset(op->nentry.filename, 0, LOL_FILENAME_MAX);
  memcpy((char *)op->nentry.filename, name, len);
  op->nentry.created   = time(NULL);
  op->nentry.i_idx     = idx;
  op->nentry.file_size = 0;
  info.ne = &op->nentry;
  off = lol_get_free_nentry(fp, &op->sb, &info);

  if (off < 0) { // Failed, free the index
      lol_set_index_value(fp, nb, bs, idx, FREE_LOL_INDEX);
      LOL_TOUCH_ERROR(EIO, -9);
  }
  if (lol_inc_files(op)) {
     // Try to rollback...
     if (!(lol_remove_nentry(fp, nb, bs,(DWORD)(info.idx), 1))) {
	   lol_set_index_value(fp, nb, bs, idx, FREE_LOL_INDEX);
     }
     LOL_TOUCH_ERROR(EIO, -13);
  }
  op->curr_pos = 0;
  if (lol_try_fsetpos(fp, &pos))
      goto error;
  op->nentry_index = (alloc_entry)info.idx;
  return 0;
error:
  lol_try_fsetpos(fp, &pos);
  return ret;
} // end lol_touch_file
#undef LOL_TOUCH_ERROR
/* ********************************************************** */
int lol_truncate_file(lol_FILE *op) {

  int   mode;
  ULONG size;
  fpos_t pos;
  struct lol_name_entry entry;
  char *name;
  int ret = -13;

  if (!(op))
      return -1;
  if (lol_is_writable(op)) {
      return -2;
  }

  size = op->nentry.file_size;
  mode = op->open_mode.mode_num;
  name = op->vdisk_file;
  if (mode < 1) // Opened "r"?
      return -3;
  if(!(name[0]))
      return -4;
  if (!(size)) {
      return 0;
  }
  // So, the file is >= 1 bytes, --> truncate to zero
  if (lol_try_fgetpos(op->vdisk, &pos))
      return -6;
  if (LOL_GOTO_DENTRY(op)) {
      ret = -7;
      goto error;
  }
  // Read the timestamp to entry
  if ((fread((char *)&entry, (size_t)(NAME_ENTRY_SIZE),
             1, op->vdisk)) != 1) {
       ret = -8;
       goto error;
  }
  if (!(entry.filename[0])) {
       ret = -9;
       goto error;
  }
  // Got it. Truncate
  if (fseek(op->vdisk, -(NAME_ENTRY_SIZE), SEEK_CUR)) {
       ret = -10;
       goto error;
  }
  entry.file_size = 0;
  // Write new entry
  if ((fwrite((const char *)&entry,
      (size_t)(NAME_ENTRY_SIZE), 1, op->vdisk)) != 1) {
       ret = -11;
       goto error;
  }
  // Truncate chain.
  if ((lol_delete_chain_from(op, LOL_SAVE_FIRST_BLOCK))) {
        ret = -12;
        goto error;
  }
  LOL_MEMCPY(&op->nentry, &entry, NAME_ENTRY_SIZE);
  lol_try_fsetpos(op->vdisk, &pos);
  return 0;
 error:
  lol_try_fsetpos(op->vdisk, &pos);
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
/* ********************************************************** */
int lol_update_nentry(lol_FILE *op) {

  int    mode;
  fpos_t pos;
  struct lol_name_entry entry;
  char *name;

  if (!(op))
    return -1;
  if ((lol_is_writable(op))) {
    return -2;
  }
  mode = op->open_mode.mode_num;
  name = op->vdisk_file;
  if (mode < 1) // Opened "r"?
    return -3;
  if(!(name[0]))
    return -4;
  if (lol_try_fgetpos(op->vdisk, &pos))
     return -6;
  if (LOL_GOTO_DENTRY(op))
    goto error;
  if ((fread((char *)&entry, (size_t)(NAME_ENTRY_SIZE),
	     1, op->vdisk)) != 1) {
        // Read the timestamp to entry.
	goto error;
  }
  if (!(entry.filename[0]))
       goto error;
  if (fseek(op->vdisk, -(NAME_ENTRY_SIZE), SEEK_CUR)) {
      goto error;
  }
  entry.file_size = op->nentry.file_size;
  entry.i_idx     = op->nentry.i_idx;
  entry.created   = time(NULL);
  // Write new entry
  if ((fwrite((const char *)&entry,
       (size_t)(NAME_ENTRY_SIZE), 1, op->vdisk)) != 1) {
       goto error;
  }
  lol_try_fsetpos(op->vdisk, &pos);
  return 0;
 error:
  lol_try_fsetpos(op->vdisk, &pos);
  return -7;
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

  mode = op->open_mode.mode_num;
  name = op->vdisk_file;
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
     if (fseek(op->vdisk, offset, SEEK_SET)) {
	 return -7;
     }
     bytes = fwrite((const char *)&lol_index_buffer[i],
                    (size_t)(ENTRY_SIZE), 1, op->vdisk);
     if (bytes != 1) {
        return -8;
     }
  } // end if olds (i)
  else {
    // So, we don't have any old blocks to write over.
    // Then what was the last old block? We must adjust
    // it to point to the next new...
    offset = start + last_old * ENTRY_SIZE;
    if (fseek(op->vdisk, offset, SEEK_SET)) {
	return -9;
    }
    bytes = fwrite((const char *)&lol_index_buffer[0],
                   (size_t)(ENTRY_SIZE), 1, op->vdisk);
    if (bytes != 1) {
	return -10;
    }
  } // end else not olds

  // Then write new indexes
  while (lol_index_buffer[i] != LAST_LOL_INDEX) {

     idx = lol_index_buffer[i];
     offset = start + idx * ENTRY_SIZE;
     if (fseek(op->vdisk, offset, SEEK_SET)) {
         return -9;
     }
     bytes = fwrite((const char *)&lol_index_buffer[i+1],
                    (size_t)(ENTRY_SIZE), 1, op->vdisk);
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

  long pos, bs, nb;
  long fs, need;
  long   left = 0;
  long blocks = 0;
  long  f_off = 0;
  long  lb_ed = 0;
  long  delta = 0;
  long   area = 0;
  alloc_entry idx;

  if ((!(op))   || (!(olds)) ||
      (!(news)) || (!(mids)) ||
      (bytes < 0))

      return LOL_ERR_USER;

  bs  = (long)op->sb.bs;
  nb  = (long)op->sb.nb;
  fs  = (long)op->nentry.file_size;
  pos = (long)op->curr_pos;
  idx = (alloc_entry)op->nentry.i_idx;

  if (idx == LAST_LOL_INDEX)
    return LOL_ERR_CORR;
  if (idx == FREE_LOL_INDEX)
     return LOL_ERR_CORR;
  if ((idx < 0) || (idx >= nb))
     return LOL_ERR_CORR;

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
} // lol_new_indexes
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
  if (!(lol_index_buffer))
    LOL_ERR_RETURN(EBUSY, LOL_ERR_BUSY);

  // Let's make some aliases
     bs = op->sb.bs;
     nb = op->sb.nb;
  l_idx = (alloc_entry)nb - 1;
    idx = (alloc_entry)op->nentry.i_idx;
     fs = op->nentry.file_size;
    pos = op->curr_pos;
     fp = op->vdisk;

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
  FILE *vdisk;
  int whence = SEEK_SET, err = 0;

  if ((!(op)) || (!(lol_index_buffer)) || (!(last_old))) {
    return LOL_ERR_USER;
  }

  bs   = op->sb.bs;
  nb   = op->sb.nb;
  if ((!(bs)) || (!(nb)) || (olds > nb)) {
      return LOL_ERR_CORR;
  }

  first_index  = op->nentry.i_idx;
  file_size    = op->nentry.file_size;
  vdisk        = op->vdisk;

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

            if (fseek(vdisk, offset, SEEK_SET))
                  return LOL_ERR_IO;

	    // vdisk points now to the first fat index.
	    // We must skip some indexes (skip_indexes)
	    // to reach the file position.

	    while (i < skip_indexes) {
                   *last_old = current_index;
                    bytes = fread((char *)&current_index,
                    (size_t)(ENTRY_SIZE), 1, vdisk);

                   if (bytes != 1)
                       return LOL_ERR_IO;

                   offset = skip + current_index * ENTRY_SIZE;
                   if (fseek(vdisk, offset, SEEK_SET))
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
                  (size_t)(ENTRY_SIZE), 1, vdisk);
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
          if (fseek (vdisk, offset, whence))
              return LOL_ERR_IO;
     } // end for olds

    // If we also have new indexes, then find them also and
    // put them into the buffer.
    // Go back to start
    j = 0; whence = news;

    if (fseek (vdisk, skip, SEEK_SET))
        return LOL_ERR_IO;

    while (whence) {

	 if (j >= nb) {
	    op->err = lol_errno = ENOSPC;
	    return LOL_ERR_SPACE;
	 }
         bytes = fread((char *)&current_index,
                 (size_t)(ENTRY_SIZE), 1, vdisk);
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
long lol_io_dblock(lol_FILE *op, const size_t block_number,
			   char *ptr, const size_t bytes, int func)
{
  fpos_t pos;
  FILE     *fp = 0;
  size_t    nb = 0;
  size_t    bs = 0;
  size_t items = 0;
  size_t   off = 0;
  size_t  left = 0;
  long  header = 0;
  long  block  = 0;
  int     ret  = 0;

  if (!(op))
     return -1;
  if ((!(bytes)) || (op->eof))
       return 0;
  if ((!(ptr)) || (block_number < 0) || (bytes < 0))
      LOL_ERR_RETURN(EFAULT, -2);

  // This is a private function. We won't bother checking
  // which open mode the the op is...
  if ((func < 0) || (func > 1))
      LOL_ERR_RETURN(EFAULT, -3);

  nb  = (size_t)op->sb.nb;
  bs  = (size_t)op->sb.bs;
    if (block_number >= nb)
        LOL_ERR_RETURN(ENFILE, -4);
    if (bytes > bs)
        LOL_ERR_RETURN(EFAULT, -5);
    if (op->nentry.i_idx < 0)
        LOL_ERR_RETURN(ENFILE, -6);
    left = (size_t)op->nentry.file_size;
    if ((func == LOL_READ) && (op->curr_pos > left)) {
         op->eof = 1; return 0;
    }
    left -= op->curr_pos;
    if ((bytes <= left) || (func == LOL_WRITE)) {
	 left = bytes;
    }
    if (lol_try_fgetpos (op->vdisk, &pos))
	LOL_ERR_RETURN(EBUSY, -7);

    fp     = op->vdisk;
    header = (long)LOL_DATA_START;
    block  = (long)(block_number * bs);
    block  += header;

    // Byte offset inside a block?
    off = op->curr_pos % bs;
    // We MUST stay inside block boundary!
    items = bs - off;
    if (left > items)
	left = items;

    block += off;
    if (fseek(fp, block, SEEK_SET)) {

	ret = ferror(fp);
	if (ret)
	    op->err = lol_errno = ret;
	else
	    op->err = lol_errno = EIO;

        lol_try_fsetpos (fp, &pos);
	return -8;

     } // end if seek error

     items = lol_fio(ptr, left, fp, func);
     if (!(items)) {

	 ret = ferror(fp);
	 if (ret)
	     op->err = lol_errno = ret;
	 else
	    op->err = lol_errno = EIO;

         lol_try_fsetpos(fp, &pos);
	 return -9;
     }

     op->curr_pos += items;
     if ((op->curr_pos >= op->nentry.file_size)
          && (func == LOL_READ))                {

          op->curr_pos = op->nentry.file_size;
          op->eof = 1;
     }

   if (op->curr_pos < 0)
       op->curr_pos = 0;

   lol_try_fsetpos(fp, &pos);
   return (long)items;
} // end lol_io_dblock
/* **********************************************************
 * lol_free_space: Calculate free space in a container.
 * return value:
 * < 0  : error
 * >= 0 : free space
 ************************************************************ */
long lol_free_space (char *disk, const int mode)
{
  struct lol_name_entry name_e;
  struct lol_super sb;
  FILE *vdisk;
  DWORD i, nf = 0;
  DWORD   nb  = 0;
  DWORD   bs  = 0;
  DWORD used_blocks = 0;
  long free_space   = 0;
  long files        = 0;
  long occupation   = 0;
  long used_space   = 0;
  long doff         = 0;
  alloc_entry entry = 0;
  int  m            = 0;

  if (!(disk))
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

  free_space = lol_get_vdisksize(disk, &sb, NULL, RECUIRE_SB_INFO);
  if (free_space < LOL_THEOR_MIN_DISKSIZE) {
       return LOL_ERR_CORR;
  }
  if (LOL_INVALID_MAGIC) {
      return LOL_ERR_CORR;
  }

  nb = sb.nb;
  bs = sb.bs;
  nf = sb.nf;

  if (nf > nb)
    return LOL_ERR_CORR;
  if (!(vdisk = fopen(disk, "r"))) {
       return LOL_ERR_IO;
  }
  doff = (long)LOL_DENTRY_OFFSET_EXT(nb, bs);
  if ((fseek (vdisk, doff, SEEK_SET))) {
       lol_try_fclose(vdisk);
       return LOL_ERR_IO;
  }
  // Read the name entries
  for (i = 0; i < nb; i++) {

    if ((fread((char *)&name_e, (size_t)(NAME_ENTRY_SIZE), 1, vdisk) != 1)) {
       lol_try_fclose(vdisk);
       return LOL_ERR_IO;
    } // end if cannot read
    if (name_e.filename[0]) { // Should check more but this will do now
      files++;
      used_space += name_e.file_size;
    }
  } // end for i
  // Read also the reserved blocks.
  for (i = 0; i < nb; i++) {  // We could propably suck all the indexes into
                                      // a buffer with a few reads, something todo...
    if ((fread((char *)&entry, (size_t)(ENTRY_SIZE), 1, vdisk) != 1)) {
        lol_try_fclose(vdisk);
        return LOL_ERR_IO;
    } // end if cannot read
    if (entry != FREE_LOL_INDEX) {
        used_blocks++;
    }
  } // end for i

  lol_try_fclose(vdisk);

  occupation = (long)(used_blocks * bs);
  free_space = (long)(nb - used_blocks);
  if (m == LOL_SPACE_BYTES) {
      free_space *= bs;
  }
  if ((used_space > occupation)  ||
      (used_blocks > nb) ||
      (nf != files))                {

      return LOL_ERR_CORR;
  }
  return free_space;
} // end lol_free_space
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

  if (i > LOL_FSCK_WARN) {
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
  int i, len;
  int ret = 0;
  if (!(str))
    return -1;
  if(!(str[0]))
    return -1;
  len = (int)strlen(str);
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
        raw_size = lol_get_rawdevsize((char *)container, NULL, NULL);
        if (raw_size < LOL_THEOR_MIN_DISKSIZE)
	   return -1;
    }
  } // end if func == LOL_EXISTING_FILE
 len = strlen(size);
 if ((len < 1) || (len > 12)) // 12 <--> 1 Tb? Really??
    return -1;
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

  memset((char *)siz, 0, 256);
  strcat((char *)siz, size);
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
    (100 * LOL_MEGABYTE),
    (40 * LOL_MEGABYTE),
    (10 * LOL_MEGABYTE),
    (LOL_MEGABYTE),
    (LOL_MEGABYTE / 2),
    (64 * LOL_KILOBYTE),
    1,
    0
  };
  const long refs[] = {
    ( 32 * LOL_MEGABYTE),
    ( 16 * LOL_MEGABYTE),
    (  8 * LOL_MEGABYTE),
    (  2 * LOL_MEGABYTE),
    (256 * LOL_KILOBYTE),
    (128 * LOL_KILOBYTE),
    ( 16 * LOL_KILOBYTE),
       (LOL_04KILOBYTES),
    1
  };

  long frac;
  int  i = 0;

  if ((size <= 0) || (blk <= 0))
     return -1;
  if (blk > size)
    return -1;
  if (size % blk)
    return -1;

  for (i = 0;  sizes[i]; i++) {
   if (size >= sizes[i]) {
       frac  =  refs[i] % blk;
       return  (refs[i] - frac);
   }
  }
  return (16 * blk);
} // end lol_get_io_size
/* **********************************************************
 * lol_extendfs: Extends container space by given amount
 *               of data blocks.
 * NOTE: This function does not check much. User should
 *       first use another function to evaluate if the
 *       given container is valid and consistent.
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
int lol_extendfs(const char *container, const DWORD new_blocks,
                struct lol_super *sb, const struct stat *st)
{
  const size_t news = (size_t)(new_blocks);
  char      temp[4096];
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

  if ((!(container)) || (!(sb)) || (!(st)))
    return -1;
  if (new_blocks < 1)
    return -1;

  nb  = (long)sb->nb;
  bs  = (long)sb->bs;
  add = (long)new_blocks;
  add += nb;

  if (!(nb))
    return -1;

  ds  = (long)NAME_ENTRY_SIZE;
  ds *= (long)(nb);
  io = lol_get_io_size(ds, NAME_ENTRY_SIZE);
  if (io <= 0) {
      lol_debug("lol_extendfs: Internal error: io <= 0");
      return -1;
  }
  if (!(buf = (char *)lol_malloc((size_t)(io)))) {
     buf = temp;
     io  = 4096;
  }
  else {
      mem = (size_t)(io);
      alloc = 1;
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

  fp = fopen(container, "r+");
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
     if ((lol_ifcopy(FREE_LOL_INDEX, news, fp)) != news) { // frac --> news
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
     lol_try_fclose(fp);
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
 lol_try_fclose(fp);
ret:
 if (alloc) {
     lol_free(mem);
 }
 return -1;
} // end lol_extendfs
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
