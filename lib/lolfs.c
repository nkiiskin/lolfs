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
/* $Id: lolfs.c, v0.20 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $" */

#include "../config.h"
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef _ERRNO_H
#include <errno.h>
#endif
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#ifndef _UNISTD_H
#include <unistd.h>
#endif
#endif
#ifndef _TIME_H
#include <time.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _FCNTL_H
#include <fcntl.h>
#endif
#ifndef _SIGNAL_H
#include <signal.h>
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
#include <lolfs.h>
#include <lol_internal.h>
/* ********************************************************** */
// Some globals
/* ********************************************************** */


// HOIDA SE lol_get_free_index !!!!!!!!!!!!!!!!!!!!!!!!!!!!


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
  "\x1b[31m[INTERNAL ERROR]\x1b[0m\n",
  NULL,
};
alloc_entry *lol_index_buffer = 0;
alloc_entry  lol_storage_buffer[LOL_STORAGE_SIZE+1];
const char lol_valid_filechars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-_:!+#%[]{}?,;&()1234567890=~@";



struct sigaction lol_sighand_store[LOL_NUM_SIGHANDLERS+1];
static const int       lol_signals[LOL_NUM_SIGHANDLERS + 1] = {
                       SIGHUP, SIGINT, SIGTERM, SIGSEGV, 0
};
/* ********************************************************** */
void lol_memset_indexbuffer(const alloc_entry value,
                            const alloc_entry num)   {

  alloc_entry i = 0;
  for (; i < num; i++) {
       lol_index_buffer[i] = value;
  }
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
void lol_index_free (const size_t amount) {

  if (!(amount))
       return;

  if (amount > (LOL_STORAGE_SIZE)) {

          if (lol_index_buffer) {
              free (lol_index_buffer);
              lol_index_buffer = 0;
          }

          lol_restore_sighandlers();
    }
    else {
        // Just clear...
        lol_memset_indexbuffer(LAST_LOL_INDEX, LOL_STORAGE_ALL);
    }
  lol_buffer_lock = 0;
} // end lol_index_free
/* ********************************************************** */
static void lol_freemem_hdl(int sig, siginfo_t *so, void *c)
{
        lol_index_free(LOL_STORAGE_ALL);
        exit(0);
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
 * or regular file or symbolic link to a regular file.
 * or -1, if error. Most likely must be run as root if
 * device is a raw block device.
 *
 *  NOTE:
 *  The function may be called with or without 'sb' and/or 'sta'
 *  if or if not those information(s) are needed.
 * ********************************************************** */
long lol_get_rawdevsize(char *device, struct lol_super *sb, struct stat *sta) {

  long ret = -1;
  int fd;
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
    memcpy((char *)sta, (const char *)&st, sizeof(st));

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
     bytes = read(fd, (char *)sb, (size_t)DISK_HEADER_SIZE);
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

    bs = (long)sb->block_size;
    nb = (long)sb->num_blocks;

    if ((nb < 1) || (bs < 1)) {
        return -1;
    }

    min_size = (long)LOL_DEVSIZE(nb, bs);

    if (sta) {
       memcpy((char *)sta, (const char *)&st, sizeof(st));
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
  if ((!(op->sb.num_blocks)) || (!(op->sb.block_size)))
    return -2;
  if (op->vdisk_size < LOL_THEOR_MIN_DISKSIZE)
    return -3;
  size = (ULONG)LOL_DEVSIZE(op->sb.num_blocks, op->sb.block_size);
  if (op->vdisk_size != size)
     return -4;
  if ((LOL_CHECK_MAGIC(op)))
    return -5;
  if ((op->sb.num_files > op->sb.num_blocks))
    return -6;

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

    if (!(op->opened))
      return LOL_ERR_USER;

    bs = op->sb.block_size;
    nb = op->sb.num_blocks;
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

    nf = op->sb.num_files;

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

  if (fgetpos(f, &old_pos))
      return -1;

  if (LOL_GOTO_NENTRY(f, nb, bs, nentry))
      goto error;

  if (remove_idx) {
    // Free the index also
    if (fgetpos(f, &tmp))
        goto error;
    if (fread((char *)&p, NAME_ENTRY_SIZE, 1, f) != 1)
      goto error;

      idx = p.i_idx;
      if ((idx < 0) || (idx >= nb))
	goto error;

    offset = (long)LOL_INDEX_OFFSET(nb, bs, idx);
    if (fseek(f, offset, SEEK_SET))
        goto error;
    v = FREE_LOL_INDEX;
    if (fwrite((char *)&v, ENTRY_SIZE, 1, f) != 1)
        goto error;

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
  fpos_t tmp;
  long offset   = 0;
  alloc_entry v = 0;
  alloc_entry ret = LOL_FALSE;

  if ((!(f)) || (idx < 0) || (idx >= nb) ||
      (!(nb)) || (!(bs))) {
      return LOL_FALSE;
  }
  if (fgetpos(f, &tmp))
      return LOL_FALSE;
  offset = (long)LOL_INDEX_OFFSET(nb, bs, idx);
  if (fseek(f, offset, SEEK_SET))
      goto error;
  if ((fread((char *)&v, ENTRY_SIZE, 1, f)) != 1)
      goto error;
  ret = v;
error:
    fsetpos(f, &tmp);
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
               const alloc_entry idx, const alloc_entry new_val)
{
  fpos_t tmp;
  long offset;
  int ret = -1;

  if ((!(f)) || (idx < 0) || (idx >= nb) ||
      (!(nb)) || (!(bs)))
    return -1;

  // Don't allow illegal values, sort them out..
  if ((new_val == LAST_LOL_INDEX) ||
      (new_val == FREE_LOL_INDEX)) {
      goto legal;
  }
  if ((new_val < 0) || (new_val >= nb))
      return -1;

legal:
    if (fgetpos(f, &tmp))
        return -1;

    offset = (long)LOL_INDEX_OFFSET(nb, bs, idx);
    if (fseek(f, offset, SEEK_SET))
        goto error;
    if (fwrite((char *)&new_val, ENTRY_SIZE, 1, f) != 1)
        goto error;

    ret = 0;
error:
    fsetpos(f, &tmp);
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
int lol_index_malloc(const size_t num_entries) {

  size_t bytes;

  if (lol_buffer_lock)
     LOL_ERRET(EBUSY, LOL_ERR_BUSY);

  if (!(num_entries))
     LOL_ERRET(EINVAL, LOL_ERR_PARAM);

  bytes  = (size_t)((num_entries + 1) * (ENTRY_SIZE));

  if (num_entries > (LOL_STORAGE_SIZE)) {

  // If the bytes needed is more than LOL_STORAGE_SIZE,
  // then we allocate memory dynamically. If it's less,
  // then, we use our global buffer.
      if (lol_index_buffer) {
          lol_buffer_lock = 1;
          LOL_ERRET(EBUSY, LOL_ERR_BUSY);
      }
      if (!(lol_index_buffer = (alloc_entry *)malloc(bytes)))
            LOL_ERRET(ENOMEM, LOL_ERR_MEM);

      // We must set the lock ASAP
      lol_buffer_lock = 1;
  } // end if num_entries
  else {
     // Small enough, we use our global storage.
        lol_index_buffer = lol_storage_buffer;
        lol_buffer_lock  = 1;
  }

  lol_memset_indexbuffer(LAST_LOL_INDEX, (const alloc_entry)(bytes + 1));
  if (num_entries > (LOL_STORAGE_SIZE)) { // handlers are only to free
                                          // dynamic memory
    if ((lol_setup_sighandlers())) {
	// We cannot call lol_index_free because
	// It would try to restore the signal
	// handlers also but we did not set them

                if (lol_index_buffer) {
                    free (lol_index_buffer);
                    lol_index_buffer = 0;
		    lol_buffer_lock  = 0;
                }

                LOL_ERRET(EBUSY, LOL_ERR_SIG);
      }
#if LOL_TESTING
      printf("Set up handlers ok\n");
#endif
  }
  //lol_buffer_lock = 1;
  return LOL_OK;
} // end lol_index_malloc
/* **********************************************************
 * lol_malloc:
 *
 *   Allocate a buffer like malloc but uses preallocated.
 *   buffer if the request is small enough. Also set signal
 *   handlers.
 *
 * NOTE: Free this memory using lol_free
 *
 * Return value:
 * A pointer to the allocated memory.
 * NULL if error
 * ********************************************************** */
void* lol_malloc(const size_t size) {
  size_t entries = 0;
  size_t frac    = 0;
  const size_t e_size = (size_t)(ENTRY_SIZE);

  if ((lol_buffer_lock) || (!(size)))
    return NULL;

  // We use lol_index_malloc, which allocates
  // memory in ENTRY_SIZE sized units.

  frac = size % e_size;
  entries = size / e_size;
  if (frac)
    entries++;

  if ((lol_index_malloc(entries)))
    return NULL;

  return (void *)(lol_index_buffer);
} // end lol_malloc
/* ********************************************************** */
void lol_free(const size_t size) {

  size_t entries = 0;
  size_t frac    = 0;
  const size_t e_size = (size_t)(ENTRY_SIZE);

  if ((!(lol_buffer_lock)) || (!(size)))
     return;

  frac = size % e_size;
  entries = size / e_size;
  if (frac)
    entries++;

  lol_index_free(entries);

} // end lol_free
/* ********************************************************** */
size_t lol_fill_with_value(const alloc_entry value,
                           size_t bytes, FILE *s)

{
  size_t i, last, times, ret = 0;
  alloc_entry buf[4096];

  if (bytes % ENTRY_SIZE)
      return 0;
  if ((bytes < ENTRY_SIZE) || (!(s)) || (!(bytes)))
      return 0;
  for (i = 0; i < 4096; i++) {
       buf[i] = value;
  }

  times = bytes >> LOL_DIV_4096;
  last  = bytes % 4096;

  for (i = 0; i < times; i++) {

      if (fwrite(buf, 4096, 1, s) != 1)
	  return ret;

         ret += 4096;
  }

  if (last) {
      if (fwrite(buf, last, 1, s) != 1)
           return ret;

      ret += last;
  }

  return ret;
} // end lol_fill_with_value
/* ********************************************************** */
size_t null_fill(const size_t bytes, FILE *s)
{
  // Can't use lol_fill_with_value because it fills in
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
}
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
int lol_size_to_str(const unsigned long size, char *s) {

  float siz = 0.0;

  if (!(s))
    return -1;

  siz = (float)size;

       if (siz < LOL_KILOBYTE) {
         sprintf(s, "%2.2f ", siz);
	 strcat(s, "bytes");
         return 0;

       }
       else {
	 if (siz < LOL_MEGABYTE) {
	     siz /= 1024.0;
             sprintf(s, "%2.2f ", siz);
	     strcat(s, "Kb");
             return 0;
	 }
	 else {
	     // >= 1Mb
	     if (siz < LOL_GIGABYTE) {
	       siz /= (float)LOL_MEGABYTE;
               sprintf(s, "%2.2f ", siz);
	       strcat(s, "Mb");
               return 0;
	     }
	     else {
	       // >= 1Gb
	       siz /= (float)LOL_GIGABYTE;
               sprintf(s, "%2.2f ", siz);
	       strcat(s, "Gb");
	     }
	 }

      } // end else size

   return 0;
} // end lol_size_to_str
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
  alloc_entry current_index;
  long  num_blocks;
  long  block_size;
  fpos_t old_pos;
  long i, size;
  long skip, offset;
  int ret = -1;

  if ((!(vdisk)) || (!(sb)) || (!(count)))
    return -1;

  current_index = first_index;
  num_blocks    = (long)sb->num_blocks;
  block_size    = (long)sb->block_size;
  *count        = 0;

  if ((!(num_blocks)) || (!(block_size)))
      return -1;

  if ((current_index < 0) || (current_index >= num_blocks))
      return 1;

  if (dsize < LOL_THEOR_MIN_DISKSIZE)
      return -1;

  *count = 1;
  size   = (long)LOL_DEVSIZE(num_blocks, block_size);

  if (dsize != size)
     return -1;

  skip = LOL_TABLE_START_EXT(num_blocks, block_size);

    if (fgetpos(vdisk, &old_pos))
        return -1;

    for (i = 0; i < num_blocks; i++) {

        offset = skip + current_index * ENTRY_SIZE;
        if ((fseek(vdisk, offset, SEEK_SET))) {
	   *count = i + 1;
            goto error;
        }

	if ((fread((char *)&current_index,
		   (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
	   *count = i + 1;
            goto error;
        }

        if (current_index == LAST_LOL_INDEX) {
	    *count = i + 1;
	    ret    = 0;
	    goto error;
        }

        if ((current_index >= num_blocks) || (current_index < 0)) {

	  if (terminate) {

	         *count = i + 1;

                 if ((fseek(vdisk, -(ENTRY_SIZE), SEEK_CUR))) {
                      goto error;
                 }
		 current_index = LAST_LOL_INDEX;
	         if ((fwrite((char *)&current_index,
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
		 current_index = LAST_LOL_INDEX;
	         if ((fwrite((char *)&current_index,
			     (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
                      goto error;
                 }

		 ret = 0;
		 goto error;

	  } // end if terminate

    *count = i;
     ret = 1;

error:
    fsetpos(vdisk, &old_pos);
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

  DWORD num_blocks;
  DWORD block_size;
  int files;

  fpos_t pos;
  struct lol_super s;
  int fix  = 0;
  int ret = -1;

  if ((!(vdisk)) || (!(sb)))
    return -1;

  num_blocks  = sb->num_blocks;
  block_size  = sb->block_size;
  files       = (int)sb->num_files;

  if ((!(num_blocks)) || (!(block_size)))
    return -1;

  switch (func) {

    case  LOL_INCREASE:

      if (files == num_blocks)
	return -1;

      if (files > num_blocks)
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

  if (fgetpos(vdisk, &pos))
    return -1;
  if (fseek(vdisk, 0, SEEK_SET))
    goto error;
  if ((fread((char *)&s, (size_t)(DISK_HEADER_SIZE), 1, vdisk)) != 1)
      goto error;
  if (fseek(vdisk, 0, SEEK_SET))
    goto error;

  if (func == LOL_INCREASE) {
      if (fix)
	s.num_files = num_blocks;
      else
        s.num_files++;
  }
  else {
    if (fix)
      s.num_files = 0;
    else
     s.num_files--;

  } // end else

  if ((fwrite((const char *)&s,
	      (size_t)(DISK_HEADER_SIZE), 1, vdisk)) != 1)
       goto error;

  if (fix)
    goto error;

    ret = 0;

 error:

  fsetpos(vdisk, &pos);
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
  DWORD num_blocks  = 0;
  DWORD block_size  = 0;
  ULONG file_size   = 0;
  DWORD file_blocks = 1;
  long  skip;
  long  offset;
  int   rv;
  DWORD i = 1;
  int   j = 1;

  if ((rv = lol_check_corr(op, LOL_CHECK_BOTH))) {
      return rv;
  }

  vdisk       = op->vdisk;
  first_index = op->nentry.i_idx;
  num_blocks  = op->sb.num_blocks;
  block_size  = op->sb.block_size;
  file_size   = op->nentry.file_size;
  last_index  = num_blocks - 1;


  //  if (first_index == LAST_LOL_INDEX)
  //  return LOL_OK;

  if (flags)
       j = 0;

    skip   = LOL_TABLE_START(op);
    offset = skip + first_index * ENTRY_SIZE;
    if (fseek(vdisk, offset, SEEK_SET)) {
       lol_errno = errno;
       return LOL_ERR_IO;
    }

    // How many blocks in the file?
    if (file_size) {
        file_blocks = file_size / block_size;
	if (file_size % block_size)
	    file_blocks++;
    } // end if file_size

    if (file_blocks > num_blocks)
        return LOL_ERR_CORR;

    while(1) {

      if ((fread((char *)&current_index,
                 (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
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
}
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
  DWORD  num_blocks;
  int ret = -1;

  struct lol_name_entry *entry;

  // CANNOT use lol_check_corr here
  if (!(op))
    return -1;

  vdisk      = op->vdisk;
  name       = op->vdisk_file;
  num_blocks = op->sb.num_blocks;
  entry      = &op->nentry;

  if ((!(vdisk)) || (!(name)))
    return -1;

  if ((!(name[0])) || (!(num_blocks)))
    return -1;

  if (fgetpos (vdisk, &pos))
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

  for (i = 0; i < num_blocks; i++) {

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

    if (!(strncmp((const char *)name, (const char*)entry->filename, len))) {
                  fsetpos (vdisk, &pos);
	          op->nentry_index = i;
                  return 1;
    }

  } // end for i

  ret = 0;

error:

  fsetpos (vdisk, &pos);
  return ret;

} // end lol_read_nentry
/* ********************************************************** */
int lol_get_basename(const char* name, char *new_name, const int mode) {

  int i, ln, found = 0;

  if ((!(name)) || (!(new_name)))
    return -1;
  if (!(name[0]))
    return -1;

  switch (mode) {

  case LOL_FORMAT_TO_REGULAR :
    break;
  case LOL_LOCAL_TRUNCATE :
    break;
  default:
    return -1;

  } // end switch mode

  ln = strlen(name);
  if (ln < 4 && mode == LOL_FORMAT_TO_REGULAR)
    return -1;

  if (name[ln-1] == '/')
    return -1;

  if (mode == LOL_FORMAT_TO_REGULAR) {
  for (i = ln - 2; i > 1; i--) {
    if (name[i] == '/' && name[i-1] == ':' && name[i-2] != '/') {
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
        return -1;
      else
        i = 0;
    }

  ln = strlen(&name[i]);
  if (!ln) {
    lol_errno = ENAMETOOLONG;
    return -1;
  }

  if (ln >= LOL_FILENAME_MAX) {
    lol_errno = ENAMETOOLONG;
    ln = LOL_FILENAME_MAX-1;
  }

  memset(new_name, 0, LOL_FILENAME_MAX);
  memcpy(new_name, &name[i], (size_t)(ln));
  if (!new_name[0]) { // Should never happen here..
    lol_errno = EBADFD;
    return -1;
  }

  return 0;
} // end lol_get_basename
/** **********************************************************
 *   TODO: Should make a better check for all these possible
 *        combinations... Not even sure if these are correct
 *
 * ******************************************************* **/
char lol_mode_combinations[MAX_LOL_OPEN_MODES][14][5] = {

  {
      {"r"}, {"rb"}, {"br"}, {"\0"}
  },

  {
      {"r+"}, {"rw"}, {"wr"}, {"rb+"}, {"r+b"}, {"br+"}, {"b+r"},
      {"rwb"}, {"rbw"}, {"wrb"}, {"wbr"}, {"brw"}, {"bwr"}, {"\0"}
  },

  {
      {"w"}, {"wb"}, {"bw"}, {"\0"}
  },

  {
      {"w+"}, {"wb+"}, {"w+b"}, {"\0"}
  },

  {
      {"a"}, {"ab"}, {"ba"}, {"\0"}
  },

  {
      {"a+"}, {"ab+"}, {"a+b"}, {"ar+"}, {"a+r"},
      {"arb+"}, {"abr+"}, {"a+rb"}, {"a+br"}, {"\0"}
  },

};
/* ********************************************************** */
static const struct lol_open_mode lol_open_modes[] =
{
    { .device = 0, .mode_num = 0, .mode_str = "r",  .vd_mode = "r"   },
    { .device = 0, .mode_num = 1, .mode_str = "r+", .vd_mode = "r+"  },
    { .device = 0, .mode_num = 2, .mode_str = "w",  .vd_mode = "r+"  },
    { .device = 0, .mode_num = 3, .mode_str = "w+", .vd_mode = "r+"  },
    { .device = 0, .mode_num = 4, .mode_str = "a",  .vd_mode = "r+"  },
    { .device = 0, .mode_num = 5, .mode_str = "a+", .vd_mode = "r+"  },

};
/* ********************************************************** */
void clean_private_lol_data(lol_FILE *fp) {

  if (!(fp))
    return;

  memset((char *)fp->vdisk_file, 0, LOL_FILENAME_MAX);
  memset((char *)fp->vdisk_name, 0, LOL_DEVICE_MAX);
  memset((char *)&fp->sb, 0, DISK_HEADER_SIZE);
  memset((char *)&fp->nentry, 0, NAME_ENTRY_SIZE);
  memset((char *)&fp->open_mode, 0, sizeof(struct lol_open_mode));
  memset((char *)fp->open_mode.mode_str, 0, 6);
  memset((char *)fp->open_mode.vd_mode, 0, 4);

  fp->vdisk         = NULL;
  fp->vdisk_size    = 0;
  fp->opened        = 0;
  fp->curr_pos      = 0;
  fp->err = fp->eof = 0;

  memset((char *)fp, 0, LOL_FILE_SIZE);
} // end clean_private_lol_data
/* ********************************************************** */
lol_FILE *new_lol_FILE() {

  lol_FILE *fp = (lol_FILE *)malloc((sizeof(struct _lol_FILE)));
  if (!fp)
    return NULL;

  clean_private_lol_data(fp);
  fp->open_mode.mode_num = -1;

  return fp;
}
/* ********************************************************** */
void delete_lol_FILE(lol_FILE *fp) {
  if (fp) {
    free(fp);
    fp = NULL;
  }
} // end delete lol_FILE
/* ********************************************************** */
#define delete_return_NULL(x) { delete_lol_FILE((x)); return NULL; }
#define close_return_NULL(x) { lol_fclose((x)); return NULL; }
/* **********************************************************
 *
 *  lol_fclose:
 *  Closes a previously opened lol-file.
 *
 * **********************************************************/
int lol_fclose(lol_FILE *op)
{
  int ret = 0;
  int i = 10;

  if (!(op)) {
       lol_errno = EBADF;
       return EOF;
   }

   if (op->vdisk) {

       do { /* Try to close 10 times */

           ret = fclose(op->vdisk);
	   i--;

	   if (ret) {
	      lol_errno = errno;
	   }
	   else {
	      lol_errno = 0;
	   }

       } while ((i) && (ret));

     op->vdisk = NULL;

   }
   else {
      lol_errno = EBADF;
      ret = EOF;
   }

    delete_lol_FILE(op);
    op = NULL;

    return ret;

} // end lol_fclose
/* **********************************************************
 * lol_get_filename: // PRIVATE function.
 * Split full lolfile path (like: /path/mylolfile:/README)
 * into two parts ("/path/mylolfile" and "README") and
 * stores the values into op->vdisk_file and op->vdisk_name.
 * ret <  0 : error
 * ret =  0 : success
 *
 * ********************************************************** */
int lol_get_filename(const char *path, lol_FILE *op)
{

  int i, j, s;
  int path_len;
  int valid = 0;


  if ((!(path)) || (!(op)))
    return -1;

  path_len = strlen(path);

  if (path_len < 4)
    return -1;

  // Copy second part to op->vdisk_file
  memset((char *)op->vdisk_file, 0, LOL_FILENAME_MAX);

  if (lol_get_basename(path, op->vdisk_file, LOL_FORMAT_TO_REGULAR))
    return -1;

  j = path_len - 1;

  for (i = 1; i < j; i++) {

    if ((path[i] == ':') && (path[i+1] == '/') &&
        (i != (j-1)) && (path[i-1] != '/')) {
      valid = 1; s = i - 1;
      break;
    }
  } // end for i

  if (!(valid)) {
     lol_errno = EINVAL;
     return -1;
  }
  if (s > (LOL_DEVICE_MAX - 2)) {
    lol_errno = ENAMETOOLONG;
    return -1;
  }
  // Copy first part to op->vdisk_name
  memset((char *)op->vdisk_name, 0, LOL_DEVICE_MAX);

  for (; s >= 0 ; s--) {
    op->vdisk_name[s] = path[s];
  } // end for s

  if (!(op->vdisk_name[0])) { // Should never happen here..
    lol_errno = EINVAL;
    return -1;
  }

  return 0;
} // end lol_get_filename
/* ********************************************************** */
int lol_is_writable(const lol_FILE *op) {

  int   mode;
  DWORD num_blocks;
  DWORD block_size;
  ULONG shouldbe;
  UCHAR file_exists;

  // Can't use lol_check_corr here
  if (!(op))
    return -1;

  num_blocks  = op->sb.num_blocks;
  block_size  = op->sb.block_size;
  file_exists = op->vdisk_file[0];
  mode        = op->open_mode.mode_num;

  if ((!(num_blocks)) || (!(block_size)) || (!(file_exists)))
    return -2;

  if (mode < 1) // Opened "r"?                                                  
    return -3;

  shouldbe = (ULONG)LOL_DEVSIZE(num_blocks, block_size);

  if (op->vdisk_size != shouldbe)
     return -4;

  return 0;
} // end lol_is_writable
/* ********************************************************** *
 * lol_get_free_index: TODO: FIX THIS FUNCTION! BUFFER THE I/O
 * Find the first free index in list and mark it as used if
 * the flag is set.
 *
 * Return value:
 * < 0 : error
 *   0 : success
 * ********************************************************** */
int lol_get_free_index(lol_FILE *op, alloc_entry *idx, int mark_used) {
  
  //  alloc_entry *buf;
  DWORD i, num_blocks;
  long start;
  fpos_t pos;
  alloc_entry entry;

  if ((!(idx)) || (!(op)) || (!(op->vdisk)))
    return -1;

  num_blocks  = op->sb.num_blocks;

  if (fgetpos(op->vdisk, &pos))
    return -2;

  start = (long)LOL_TABLE_START(op);

  if (fseek(op->vdisk, start, SEEK_SET))
     goto error;

  for (i = 0; i < num_blocks; i++) {

    if ((fread((char *)&entry, (size_t)(ENTRY_SIZE),
	       1, op->vdisk)) != 1)
	goto error;

      if (entry == FREE_LOL_INDEX) {

	if (mark_used) {

	   if (fseek(op->vdisk, -(ENTRY_SIZE), SEEK_CUR))
               goto error;
	   entry = LAST_LOL_INDEX;
	   if ((fwrite((const char *)&entry, (size_t)(ENTRY_SIZE),
		       1, op->vdisk)) != 1) {
	       goto error;
	   }

	} // end if mark_used

          *idx = (alloc_entry)(i);
           fsetpos(op->vdisk, &pos);
	   return 0;
      }

  } // end for i

error:
  fsetpos(op->vdisk, &pos);
  return -4;

} // lol_get_free_index
/* ********************************************************** *
 * lol_touch_file:  PRIVATE function!!
 * Create a new file of zero size.
 * Assumes that the file name is already in op->vdisk_file.
 * NOTE: DOES NOT RETURN ERROR IF FILE EXISTS ALREADY!
 * (This is basically a helper function for lol_fopen).
 *
 * Return < 0 if error.
 * ********************************************************** */
int lol_touch_file(lol_FILE *op) {

  long  doff;
  DWORD num_blocks;
  DWORD block_size;
  DWORD files;
  ULONG shouldbe;
  fpos_t pos;
  size_t len, bytes;
  struct lol_name_entry entry;
  alloc_entry i, idx;
  char *name = 0;
  int   mode, ret = -1;

  if (!(op)) {
    lol_errno = EFAULT;
    return -1;
  }

  num_blocks  = op->sb.num_blocks;
  block_size  = op->sb.block_size;
  files       = op->sb.num_files;
  name        = op->vdisk_file;
  mode        = op->open_mode.mode_num;

  if ((!(num_blocks)) || (!(block_size)) || (!(name))) {
    lol_errno = EIO;
    return -2;
  }

  if (mode < 2) { /* Opened "r" or "r+"? */
    lol_errno = EPERM;
    return -3;
  }

  if(!(name[0])) {
      lol_errno = EIO;
      return -4;
  }

  len = strlen(name);

  if (len >= LOL_FILENAME_MAX) {
      len = LOL_FILENAME_MAX - 1;
      name[len] = '\0';
      lol_errno = ENAMETOOLONG;
  }

  shouldbe = (ULONG)LOL_DEVSIZE(num_blocks, block_size);

  if (op->vdisk_size != shouldbe) {
     lol_errno = EIO;
     return -5;
  }

  if (files >= num_blocks) {
      op->err = lol_errno = ENOSPC;
      return -6;
  }

  if (fgetpos(op->vdisk, &pos)) {
     lol_errno = errno;
     return -7;
  }

  doff = LOL_DENTRY_OFFSET(op);

  if (fseek(op->vdisk, doff, SEEK_SET))
    { ret = -8; lol_errno = errno; goto error; }

  for (i = 0; i < num_blocks; i++) {

      bytes = fread((char *)&entry, (size_t)(NAME_ENTRY_SIZE), 1, op->vdisk);
      if (bytes != 1)
	    { ret = -9; lol_errno = errno; goto error; }

      if (!(entry.filename[0])) {

	  // We create and write op->nentry here
	  if (!(lol_get_free_index(op, &idx, LOL_MARK_USED))) {
	        op->nentry.i_idx = idx;
	  }
	  else {
                 ret = -10;
		 op->err = lol_errno = ENOSPC;
                 goto error;
          }

	  if (fseek(op->vdisk, -(NAME_ENTRY_SIZE), SEEK_CUR))
              { ret = -11; lol_errno = errno; goto error; }

	  memset(op->nentry.filename, 0, LOL_FILENAME_MAX);
	  memcpy((char *)op->nentry.filename, name, len);
          op->nentry.created = time(NULL);
	  op->nentry.file_size = 0;

          bytes = fwrite((const char *)&op->nentry,
                         (size_t)(NAME_ENTRY_SIZE), 1, op->vdisk);

           if (bytes != 1) {
	        // Free the index, since we failed to write name entry
	     lol_set_index_value(op->vdisk, num_blocks,
                                 block_size, idx, FREE_LOL_INDEX);
                ret = -12; lol_errno = errno;
                goto error;
           }

	   if (lol_inc_files(op)) {
	     // Try to rollback...
	     // memset(op->nentry.filename, 0, LOL_FILENAME_MAX);
	       if (!(lol_remove_nentry(op->vdisk, num_blocks,
				       block_size,(DWORD)(i), 1))) {
		 lol_set_index_value(op->vdisk, num_blocks,
                                     block_size, idx, FREE_LOL_INDEX);
	       }
#if LOL_TESTING
	       printf ("lol_inc_files failed: entry = %d\n", (int)(i));
#endif
	       ret = -13; lol_errno = EIO;
               goto error;
	   }

           op->curr_pos = 0;
           if (fsetpos(op->vdisk, &pos))
	       goto error;

	   op->nentry_index = i;
           return 0;

      } // end if found an empty entry

   } // end for i

error:
  fsetpos(op->vdisk, &pos);
  return ret;
} // end lol_touch_file
/* ********************************************************** */
int lol_truncate_to_zero(lol_FILE *op) {

  int   mode;
  ULONG size;
  fpos_t pos;
  struct lol_name_entry entry;
  char *name;

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

  if (fgetpos(op->vdisk, &pos))
    return -6;

  if (LOL_GOTO_DENTRY(op))
    goto error;

  // Read the timestamp to entry. FIX this. We already READ THIS!

  if ((fread((char *)&entry, (size_t)(NAME_ENTRY_SIZE),
             1, op->vdisk)) != 1) {
       goto error;
  }

  if (!(entry.filename[0]))
       goto error;

   // Got it. Truncate
   if (fseek(op->vdisk, -(NAME_ENTRY_SIZE), SEEK_CUR)) {
	       goto error;
   }

   entry.file_size = 0;

   // Write new entry
   if ((fwrite((const char *)&entry, (size_t)(NAME_ENTRY_SIZE),
               1, op->vdisk)) != 1) {
	goto error;
   }

   // Truncate chain.
   if ((lol_delete_chain_from(op, LOL_SAVE_FIRST_BLOCK))) {
       goto error;
   }

   memcpy((char *)&op->nentry, (char *)&entry, NAME_ENTRY_SIZE);
   fsetpos(op->vdisk, &pos);
   return 0;
 error:

  fsetpos(op->vdisk, &pos);
  return -7;
}
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

  if (fgetpos(op->vdisk, &pos))
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
                   (size_t)(NAME_ENTRY_SIZE),
                    1, op->vdisk)) != 1)      {

	    goto error;
        }

        fsetpos(op->vdisk, &pos);
        return 0;

 error:
  fsetpos(op->vdisk, &pos);
  return -7;
} // end lol_update_nentry
/* ********************************************************** */
int lol_update_chain(lol_FILE *op, const long olds,
                 const long news, const alloc_entry last_old) {

  int  mode;
  size_t bytes;
  alloc_entry current_new_index;
  long fat_start;
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

  // num_blocks  = (long)op->sb.num_blocks;
  mode        = op->open_mode.mode_num;
  name        = op->vdisk_file;

  if (mode < 1) // Opened "r"?
    return -5;

  if(!(name[0]))
    return -6;

  fat_start = (long)LOL_TABLE_START(op);
  i = olds;

  // Join old and new chains first

  if (i) {
    // Take last index of old chain
    // and make it point to first index of the new chain

       current_new_index = lol_index_buffer[i-1];
       offset = fat_start + current_new_index * ENTRY_SIZE;

	if (fseek(op->vdisk, offset, SEEK_SET)) {
	    return -7;
        }

        bytes = fwrite((const char *)&lol_index_buffer[i],
                       (size_t)(ENTRY_SIZE), 1, op->vdisk);

        if (bytes != 1) {
	    return -8;
	}


  } // end if olds
  else {

    // So, we don't have any old blocks to write over.
    // Then what was the last old block? We must adjust
    // it to point to the next new...

        offset = fat_start + last_old * ENTRY_SIZE;

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

       current_new_index = lol_index_buffer[i];
       offset = fat_start + current_new_index * ENTRY_SIZE;

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
} // end lol_update_chain
/* **********************************************************
 *
 *  lol_feof:
 *  Return non-zero if end-of-lol-file indicator is set.
 *
 * ***********************************************************/
int lol_feof(lol_FILE *op) {
  if (!(op)) {
    lol_errno = EBADF;
    return -1;
  }
  return (int)(op->eof);
}
/* **********************************************************
 *  INTERFACE FUNCTION
 *  lol_fopen:
 *
 *  Opens a file inside a container for
 *  reading and/or writing.
 *
 *  The path must be given in form "/path/container:/filename".
 *  NOTE: There MUST be ':' separating the container
 *        from the actual file inside the container.
 *
 *  Return value:
 *
 *  NULL      : if error
 *  lol_FILE* : if success, returns the file handle.
 *
 * ********************************************************** */
lol_FILE *lol_fopen(const char *path, const char *mode)
{
  struct stat st;
  lol_FILE *op       = 0;
  // mode_t    m        = 0;
  int       r        = 0;
  int       is       = 0;
  int       mod      = 0;
  int       path_len = 0;
  int       mode_len = 0;
  int       trunc    = 0;
  long      size     = 0;
  ULONG     fs       = 0;

  if ((!path) || (!mode)) {
    lol_errno = EINVAL;
    return NULL;
  }

  path_len = strlen(path);
  mode_len = strlen(mode);

  if ((path_len < 4) || (path_len > LOL_PATH_MAX)) {
       lol_errno = ENOENT;
       return NULL;
  }

  if ((mode_len <= 0) || (mode_len > 4)) {
      lol_errno = EINVAL;
      return NULL;
  }

  if (!(op = new_lol_FILE())) {
       lol_errno = ENOMEM;
       return NULL;
  }

  mod = op->open_mode.mode_num = lol_getmode(mode);

  if ((mod < 0) || (mod >= MAX_LOL_OPEN_MODES)) {
       lol_errno = EINVAL;
       delete_return_NULL(op);
  }

  strcpy(op->open_mode.mode_str, lol_open_modes[mod].mode_str);
  strcpy(op->open_mode.vd_mode,  lol_open_modes[mod].vd_mode);

  if (lol_get_filename(path, op)) {
      // lol_errno already set
      delete_return_NULL(op);
  }

  size = lol_get_vdisksize(op->vdisk_name, &op->sb, &st, RECUIRE_SB_INFO);

  if (size < LOL_THEOR_MIN_DISKSIZE) {
      lol_errno = EIO;
      delete_return_NULL(op);
  }

  if (LOL_CHECK_MAGIC(op)) {
      lol_errno = EIO;
      delete_return_NULL(op);
  }

  op->vdisk_size = (ULONG)size;
  op->open_mode.device = st.st_mode;

  if(!(op->vdisk = fopen(op->vdisk_name, op->open_mode.vd_mode))) {
      lol_errno = EINVAL;
      delete_return_NULL(op);
  }

  is = lol_read_nentry(op);  // Can we find the dir entry?
  fs = op->nentry.file_size;

  switch (is) {  // What we do here depends on open_mode

          case  LOL_NO_SUCH_FILE:

           // Does not exist, so we return NULL
           // if trying to read (cases "r" & "r+")

		if (mod < 2)  { /* if "r" or "r+", cannot read.. */
		    lol_errno = ENOENT;
                    close_return_NULL(op);
		}

	        op->curr_pos = 0;
                // if not reading,
		// then it must be w/a, create new file...
	        if ((r = lol_touch_file(op))) {
		// lol_errno already set
                    close_return_NULL(op);
		}

                break;

            case LOL_FILE_EXISTS:

            // The file exists. In this case the
	    // op->nentry has the file info
	    // and we may both read and write

                  switch (mod) {

	                  case LOL_RDONLY:
		          case LOL_RDWR:

		               op->curr_pos = 0;

                               break;

             // "w(+)" The file will be truncated to zero
	                   case LOL_WR_CREAT_TRUNC:
 	                   case LOL_RDWR_CREAT_TRUNC:

		                 op->curr_pos = 0;
		                 trunc = 1;

                                 break;

                            case LOL_APPEND_CREAT:
              // "a" -> append

                                 op->curr_pos = fs;
                                 break;

	                    case LOL_RD_APPEND_CREAT:
              // "a+" -> FIXME: Same as "a" here!

		                 op->curr_pos = fs;
                                 break;

		            default:

                                 lol_errno = EINVAL;
                                 close_return_NULL(op);

	              } // end switch mod

                      break;

               default:

                    lol_errno = EIO;
                    close_return_NULL(op);


  } // end switch file exists

  if (trunc) { // Truncate if file was opened "w" or "w+"

      if ((r = lol_truncate_to_zero(op))) {
	  lol_errno = EIO; // TODO: FIX! Check return value
          close_return_NULL(op);
      }

      op->curr_pos = 0;

  } // end if truncate


  lol_errno  = 0;
  op->opened = 1;

#if 0
  // Check consistency just in case..
  if ((lol_check_corr(op, LOL_CHECK_BOTH))) {
       lol_errno = EIO;
       close_return_NULL(op);
  }
#endif


  return op;
} // end lol_fopen
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
      long *olds, long *mids, long *news, long *new_file_size) {

  long pos, block_size, num_blocks;
  long file_size, blocks_needed;
  long data_left_in_file;

  long write_blocks;
  long file_offset;
  long last_block_extra_data = 0;
  long delta, new_area = 0;

  alloc_entry first_index;

  if ((!(op)) || (!(olds)) || (!(news)) || (!(mids)))
      return LOL_ERR_USER;

  block_size  = (long)op->sb.block_size;
  num_blocks  = (long)op->sb.num_blocks;
  file_size   = (long)op->nentry.file_size;
  pos         = (long)op->curr_pos;
  first_index = (alloc_entry)op->nentry.i_idx;

  if (first_index == LAST_LOL_INDEX)
    return LOL_ERR_CORR;

  if (first_index == FREE_LOL_INDEX)
     return LOL_ERR_CORR;

  if (first_index < 0)
     return LOL_ERR_CORR;

  if (first_index >= num_blocks)
    return LOL_ERR_CORR;

  if (bytes < 0)
    return LOL_ERR_USER;

  *olds = *news = *mids = 0;

  if (!(bytes)) {

      *new_file_size = file_size;
      return 0;
  }

  write_blocks = (long)lol_num_blocks(op, (size_t)bytes, NULL);

  if (!(file_size)) {  // Size = 0, EASY Case

      // In this case we need new blocks for
      // the whole write request, except the first one.

      if (write_blocks) { // we may not need to check this??

	  *olds = 1;
	  *news = write_blocks - 1;
	  *new_file_size = pos + bytes;

       }

       return write_blocks;

   } // end if zero size file

   // Ok, we know it is a regular file, size > 0
   // and we write > 0 bytes.

   if (!(write_blocks)) // We must have at least one block!
       return LOL_ERR_USER;

  // Check first if we need new blocks at all.

  file_offset = file_size % block_size;

  if (file_offset)
      last_block_extra_data = block_size - file_offset;

   data_left_in_file = file_size - pos; // may be <= 0 also

  // Do we have ALSO room in the last data block of the file?

  data_left_in_file += last_block_extra_data;  // add is >= 0

  if (data_left_in_file >= bytes) {
     // We don't need new area, the old blocks are enough

      *news = 0; *olds = write_blocks;

      if (file_size < (pos + bytes))
	  *new_file_size = pos + bytes;
      else
          *new_file_size = file_size;

      return write_blocks;

  } // end if

  // So, we need new blocks. How many?

  new_area = bytes - data_left_in_file; // >0. This is the
  //  amount of bytes we need in NEW blocks!
  // This area begins on a block boundary but
  // does not necessarily end there.

  blocks_needed = new_area / block_size;

  if (new_area % block_size)
      blocks_needed++; // This is the amount of NEW blocks >0
                       // (Does NOT overlap with old data area)

  if (!(blocks_needed)) { // Really should not be here
      return LOL_ERR_INTRN;
  }

  // We know the number of NEW blocks (blocks_needed).
  // How many old blocks are there?

  if (pos >= file_size) { // In this case we don't need olds
                          // at all unless pos is inside the
                          // last block

      if (pos < (file_size + last_block_extra_data)) {

	  *olds = 1;
          *news = blocks_needed;

      }
      else {

          *olds = 0;
           delta = pos - file_size - last_block_extra_data;

	   if (delta >= block_size) {
	       *mids = delta / block_size;

           }

           *news = blocks_needed;

      } // end else

  } // end if pos
  else {

       *olds = data_left_in_file / block_size;

       if (data_left_in_file % block_size)
	  (*olds)++;

       *news = blocks_needed;

  } // end else

  *new_file_size = file_size + last_block_extra_data + new_area;

  return ((*olds) + (*news));

} // lol_new_indexes
/* **********************************************************
 *
 *  lol_read_index_chain:
 *  Read the whole block index chain to user buffer.
 *  Return value:
 *  <  0 : error
 *  >= 0 : success. The value is number of indices in the buffer
 *
 * ********************************************************** */
int lol_read_index_chain(lol_FILE *op, const size_t read_blocks)
{

  DWORD block_size;
  DWORD num_blocks;
  ULONG file_size;
  DWORD pos, bb;
  DWORD blocks_in_file;
  alloc_entry first_index;
  long skip, offset;
  long data_area;
  alloc_entry current_index;
  alloc_entry last_index;
  int i = 0;
  int count = 0;
  FILE *vdisk;
  //int err = -1;

  if ((!(op))) {
    lol_errno = EINVAL;
    return LOL_ERR_PTR;
  }
  if (!(lol_index_buffer))
    LOL_ERR_RETURN(EBUSY, LOL_ERR_BUSY);

  block_size  = op->sb.block_size;
  num_blocks  = op->sb.num_blocks;
  first_index = (alloc_entry)op->nentry.i_idx;
  file_size   = op->nentry.file_size;
  pos         = op->curr_pos;
  vdisk       = op->vdisk;
  last_index  = (alloc_entry)num_blocks;
  last_index--;

  if ((!(read_blocks)) || (read_blocks > num_blocks))
     LOL_ERR_RETURN(EIO, LOL_ERR_USER);

  if (!(file_size))
    LOL_ERR_RETURN(EIO, LOL_ERR_USER);

  if ((!(block_size)) || (!(num_blocks)))
      LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

  if ((first_index > last_index) || (first_index < 0)) {
    op->eof = 1; // Does not exctly mean end-of-file, but we set eof because
                 // the chain seems corrupted and thus, it is not a good idea
                 // to continue reading.

    op->err = lol_errno = EIO;
    return LOL_ERR_CORR;
  }

  data_area = (long)block_size;
  data_area *= num_blocks;

  if (file_size > data_area)
    LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);

   bb = pos / block_size;
   blocks_in_file = file_size / block_size;

   if (file_size % block_size)
     blocks_in_file++;

   if (blocks_in_file > num_blocks)
      LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);

  if (read_blocks > blocks_in_file)
      LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);

   skip   = LOL_TABLE_START(op);
   offset = skip + first_index * ENTRY_SIZE;

   if (fseek (vdisk, offset, SEEK_SET))
      LOL_ERR_RETURN(EIO, LOL_ERR_IO);

   current_index = first_index;

   for (i = 0; i < blocks_in_file; i++)  {

     if (i >= bb) {

       if (current_index < 0)
          LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

       if (current_index >= num_blocks)
          LOL_ERR_RETURN(EIO, LOL_ERR_CORR);


                    lol_index_buffer[count] = current_index;
                    if (++count >= read_blocks) {
                        break;
		    }

     } // end if i >= bb


     if (fread ((char *)&current_index, (size_t)(ENTRY_SIZE), 1, vdisk) != 1)
         LOL_ERR_RETURN(EIO, LOL_ERR_IO);

     if (current_index == LAST_LOL_INDEX) {
         break;
     }

     if ((current_index > last_index) || (current_index < 0))
         LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

        offset = skip + current_index * ENTRY_SIZE;   // Next offset
        if (fseek (vdisk, offset, SEEK_SET))
            LOL_ERR_RETURN(EIO, LOL_ERR_IO);

  } // end for i

   // err = count;

   if (!(count))
       LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

   //error:
   //return err;
   return count;

} // end lol_read_index_chain
/* **********************************************************
 *
 *  lol_create_index_chain:
 *  Create a block index chain based on number of
 *  needed old and new blocks.
 *  Return value:
 *  <0  : error
 *  0   : success.
 *
 * ********************************************************** */
int lol_create_index_chain(lol_FILE *op, const long olds,
                           const long news, alloc_entry *last_old)
{

  DWORD block_size;
  DWORD num_blocks;
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

  block_size   = op->sb.block_size;
  num_blocks   = op->sb.num_blocks;

  if ((!(block_size)) || (!(num_blocks)) || (olds > num_blocks)) {
      return LOL_ERR_CORR;
  }

  first_index  = op->nentry.i_idx;
  file_size    = op->nentry.file_size;
  vdisk        = op->vdisk;

  *last_old = current_index = first_index;
   skip     = LOL_TABLE_START(op);
   tot_olds = file_size / block_size;

           if (file_size % block_size)
               tot_olds++;
	   if (!(file_size))
	       tot_olds = 1;
	   if (olds > tot_olds)
               LOL_ERR_RETURN(EIO, LOL_ERR_INTRN);
	   if ((olds + news) > num_blocks)
               LOL_ERR_RETURN(ENOSPC, LOL_ERR_SPACE);

            skip_indexes = tot_olds - olds;
            offset = skip + first_index * ENTRY_SIZE;

            if (fseek(vdisk, offset, SEEK_SET))
                  return LOL_ERR_IO;

	    // vdisk points now to the first fat index.
	    // We must skip some indexes (skip_indexes) to reach the file position.

	    while (i < skip_indexes) {
                   *last_old = current_index;
                    bytes = fread((char *)&current_index, (size_t)(ENTRY_SIZE), 1, vdisk);

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

          bytes = fread((char *)&current_index, (size_t)(ENTRY_SIZE), 1, vdisk);

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

	 if (j >= num_blocks) {

	    op->err = lol_errno = ENOSPC;
	    return LOL_ERR_SPACE;
	 }

         bytes = fread((char *)&current_index, (size_t)(ENTRY_SIZE), 1, vdisk);

         if (bytes != 1)
             return LOL_ERR_IO;

	 if (current_index == FREE_LOL_INDEX) {

             // Save the index and continue
             lol_index_buffer[i++] = j;
             whence--;

	 } // end if found free block

         j++;

    } // end while

    //error:
    return err;

} // end lol_create_index_chain
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

  size_t block_size    = 0;
  size_t blocks_behind = 0;
  size_t blocks_after  = 0;
  size_t block_offset  = 0;
  size_t delta_offset  = 0;
  size_t start_bytes   = 0;
  size_t end_bytes     = 0;
  size_t block_loops   = 0;

  if ((!(op)) || (!(amount)))
     return 0;

  block_size  = (size_t)op->sb.block_size;

  if (!(block_size)) {
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

       block_offset  = op->curr_pos % block_size;
       delta_offset  = (op->curr_pos + amount) % block_size;
       blocks_behind = op->curr_pos / block_size;

  if (!(block_offset)) { // We are on block boundary!

   /*  1: We read only some BLOCKS
       2: We read some BLOCKS and START
       3: We read only START
    */

       if (amount < block_size) {

                 start_bytes = amount;
       }
       else { // amount >= block_size

           // We do BLOCKS and optionally START

            block_loops = amount / block_size;

                if (amount % block_size) {
                   
                        start_bytes = delta_offset;

                } // end if we do both BLOCKS and START


       } // end else amount >= block_size

  } // end if on block_boundary

  else {  // We are in the middle of a block, gotta be carefull !

  /*
     4: We read only the END
     5: We read END and some BLOCKS
     6: We read END, some BLOCKS and START

   */
       end_bytes = block_size - block_offset;

       if (end_bytes > amount)
              end_bytes = amount;

       blocks_after  = (op->curr_pos + amount) / block_size;

       if (amount < block_size) {  // We do END and optionally START

                if (blocks_after > blocks_behind) { // We do START too

                    start_bytes = delta_offset;
	        }
       }
       else {  // amount >= block_size

              if (amount == block_size) {

		// Here we know, that we read both END and START but NOT BLOCKS!

                    start_bytes = block_offset;

	      }
              else {  // So, amount > block_size

                    block_loops = blocks_after - blocks_behind - 1;

                    if (block_loops) {  // So, we do END and BLOCKS but shall we do START?

		      // WE do ALL

                           start_bytes = delta_offset;

		     } // end if we do BLOCKS
                     else { // So, don't do BLOCKS but we do START

                          start_bytes = amount - end_bytes;

                     } // end else

             } // end else amount > block_size

       } // end else amount >= block_size

  } // end else not on boundary

  if (q) { // If q is given, save the results there

           q->num_blocks = block_loops;
	   if (start_bytes)
	       q->num_blocks++;
	   if (end_bytes)
	       q->num_blocks++;

           q->start_bytes = start_bytes;
           q->end_bytes   = end_bytes;
           q->full_loops  = block_loops;

  }

  if (start_bytes)
         block_loops++;

  if (end_bytes)
       block_loops++;

    return block_loops;

} // end lol_num_blocks
/* **********************************************************
 *
 *  lol_fread:
 *  Read data from the file to user buffer.
 *  
 *
 * ********************************************************** */
size_t lol_fread(void *ptr, size_t size, size_t nmemb, lol_FILE *op)
{

  struct lol_loop loop;
  alloc_entry current_index = 0;

  size_t file_size       = 0;
  size_t block_size      = 0;
  size_t left_to_read    = 0;
  size_t mem, i = 0, off = 0;
  size_t start_bytes     = 0;
  size_t end_bytes       = 0;
  size_t block_loops     = 0;
  long   amount          = 0;
  int    ret             = 0;

  if (!(ptr))
    LOL_ERR_RETURN(EINVAL, 0);

  if (!(op))
    LOL_ERR_RETURN(EBADF, 0);

  if (!(size))
    return nmemb;

  if (!(nmemb))
    return 0;

  if (lol_valid_sb(op))
      LOL_ERR_RETURN(ENFILE, 0);

  if (op->eof)
   return LOL_ERR_EOF;

  file_size   = (size_t)op->nentry.file_size;
  block_size  = (size_t)op->sb.block_size;
  left_to_read = file_size - op->curr_pos;

  if (!(left_to_read)) {

       op->eof = 1;
       return LOL_ERR_EOF;
  }

  if ((!(op->vdisk)) || (op->opened != 1))
      LOL_ERR_RETURN(EBADFD, 0);

  switch (op->open_mode.mode_num) {

      case             LOL_RDWR :
      case           LOL_RDONLY :
      case  LOL_RD_APPEND_CREAT :
      case LOL_RDWR_CREAT_TRUNC :

        break;

    default:

      op->err = lol_errno = EPERM;
      return 0;

  } // end switch

  current_index = op->nentry.i_idx;
  if ((current_index < 0) || (current_index >= op->sb.num_blocks))
    LOL_ERR_RETURN(ENFILE, 0);

  if (!(file_size)) {
        op->eof = 1;
        return LOL_ERR_EOF;
  }

  if (op->curr_pos > file_size) {
      op->eof = 1;
      return LOL_ERR_EOF;
  }

  // Ok, we actually have some data to read
  // Can we read the whole amount?
   amount = size * nmemb;

  if (amount > left_to_read)
          amount = left_to_read; // This is the amount of bytes we will read

   mem = lol_num_blocks(op, amount, &loop);

   if (lol_index_malloc(mem))
       LOL_ERR_RETURN(ENOMEM, 0);

   ret = lol_read_index_chain(op, mem);
#if LOL_TESTING
   printf("DEBUG: lol_fread: index_buffer has %d indexes\n", ret);
#endif

   if ((ret <= 0) || (ret != mem)) {
#if LOL_TESTING
     printf("DEBUG: lol_fread: ret = %d, mem = %ld (should be equal)\n", ret, (long)mem);
#endif


       lol_index_free(mem);
       if ((!(lol_errno))  && (!(op->err)))
	   LOL_ERRSET(EIO);
       return 0;
    }

  start_bytes = loop.start_bytes;
  block_loops = loop.full_loops;
  end_bytes   = loop.end_bytes;

  i = 0;

  if (end_bytes) {

         current_index = lol_index_buffer[i++];

	 if (current_index < 0) {
	     // This should not happen, unless corrupted container file
	     LOL_ERRSET(EFAULT);
	     goto error;
      	 }

        amount = lol_io_dblock(op, current_index, ptr, end_bytes, LOL_READ);
	if (amount < 0) {
	  //off = 0;
	    goto error;
        }

	off += amount;
	if (amount != end_bytes) {
	    amount = -1;
	    goto error;
        }

  } // end if end_bytes

  block_loops += i;

  // Process the full blocks

      for (; i < block_loops; i++) {

           current_index = lol_index_buffer[i];

	   if (current_index < 0) {
	   // This should not happen, unless corrupted container
	      LOL_ERRSET(EFAULT);
	      goto error;
	   }

	   amount = lol_io_dblock(op, current_index, ptr+off, block_size, LOL_READ);
           if (amount < 0) {
	     //off = 0;
	       goto error;
           }

	   off += amount;

	   if (amount != block_size) {
	       amount = -1;
	       goto error;
           }

     } // end for i

     if (start_bytes) {

         current_index = lol_index_buffer[i];

	 if (current_index < 0) {
	  // This should not happen, unless corrupted container
	  LOL_ERRSET(EFAULT);
	  goto error;
                          
	 }

	 amount = lol_io_dblock(op, current_index, ptr + off, start_bytes, LOL_READ);

         if (amount < 0) {
	  // printf("lol_fread: start_bytes: ERROR lol_io_dblock %d\n", (int)amount);
	  //   off = 0;
	     goto error;
         }

	 off += amount;

	if (amount != start_bytes) {
	    amount = -1;
	    goto error;
        }

    } // end if start bytes


  off /= size;

error:

  lol_index_free(mem);

  if (op->curr_pos > file_size) {
      op->eof = 1;
  }

#if 0
  if (off < nmemb) {
      op->eof = 1;
  }
#endif

  if ((!(off)) && (!(op->err)))
       LOL_ERRSET(EIO);

  if (amount < 0) {
    off /= size;
    if (!(op->err))
       LOL_ERRSET(EIO);
  }

  return off;

} // end lol_fread
/* **********************************************************
 *
 *  lol_fwrite:
 *  Write data from user buffer to file.
 *  
 *
 * ********************************************************** */
size_t lol_fwrite(const void *ptr, size_t size, size_t nmemb, lol_FILE *op)
{

  struct lol_loop loop;
  alloc_entry current_index;
  alloc_entry last_old;
  long olds, mids, news;

  long new_filesize     = 0;
  long filesize         = 0;
  long write_blocks     = 0;
  long amount           = 0;
  size_t block_size     = 0;
  size_t i = 0,     off = 0;
  size_t start_bytes    = 0;
  size_t end_bytes      = 0;
  size_t block_loops    = 0;
  int  ret              = 0;

  if (!(ptr))
    LOL_ERR_RETURN(EINVAL, 0);

  if (!(op))
    LOL_ERR_RETURN(EBADF, 0);

  if (!(size))
    return nmemb;

  if (!(nmemb))
    return 0;

  if (lol_valid_sb(op))
      LOL_ERR_RETURN(ENFILE, 0);

  filesize    = (long)op->nentry.file_size;
  block_size  = (size_t)op->sb.block_size;

  if ((!(op->vdisk)) || (op->opened != 1))
       LOL_ERR_RETURN(EBADFD, 0);


  switch (op->open_mode.mode_num) {

      case             LOL_RDWR :
      case     LOL_APPEND_CREAT :
      case   LOL_WR_CREAT_TRUNC :
      case  LOL_RD_APPEND_CREAT :
      case LOL_RDWR_CREAT_TRUNC :

        break;

    default:
      op->err = lol_errno = EPERM;
      return 0;

  } // end switch

  current_index = op->nentry.i_idx;
  if ((current_index < 0) || (current_index >= op->sb.num_blocks))
    LOL_ERR_RETURN(ENFILE, 0);

  if (lol_is_writable(op))
      LOL_ERR_RETURN(EPERM, 0);

  amount = size * nmemb;

  // Compute new indexes

  write_blocks = lol_new_indexes(op,
                 (long)amount, &olds, &mids,
                 &news, &new_filesize);

  // Translate our errors..
  if (write_blocks <= 0) {

      switch (write_blocks) {

           case LOL_ERR_EOF :
	     return 0; // We do NOT set op->err here!

           case LOL_ERR_USER :
                op->err = lol_errno = EFAULT;
               return 0;

           case LOL_ERR_CORR :
                op->err = lol_errno = EIO;
               return 0;

           default :
#if LOL_TESTING
	     puts ("DEBUG: lol_fwrite err 1");
#endif
                op->err = lol_errno = EIO;
               return 0;
      } // end swith

   } // end if error

   if (lol_index_malloc((size_t)(write_blocks)))
       LOL_ERR_RETURN(ENOMEM, 0);

    ret = lol_create_index_chain(op, olds, news, &last_old);
    if (ret < 0) {

	lol_index_free((size_t)(write_blocks));
	if (!(op->err)) {
#if LOL_TESTING
	  printf("DEBUG: lol_fwrite: lol_create_index_chain failed, ret = %d\n", (int)(ret));
#endif
	  op->err = lol_errno = EIO;
	}

        return 0;
    }

  // Ready to write...

   i = mids;
   lol_num_blocks(op, amount, &loop);

   start_bytes = loop.start_bytes;
   block_loops = loop.full_loops;
   end_bytes   = loop.end_bytes;


  if (end_bytes) {

      current_index = lol_index_buffer[i++];
      if (current_index < 0) {
	  LOL_ERRSET(EFAULT);
	  goto error;
      }

      amount = lol_io_dblock(op, current_index, (char *)ptr, end_bytes, LOL_WRITE);

      if (amount < 0) {
	//	  off = 0;
	  goto error;
      }

      off += amount;
      
	if (amount != end_bytes) {
	    amount = -1;
	    goto error;
        }
      

  } // end if end_bytes

  block_loops += i;

  // Process the full blocks

   for (; i < block_loops; i++) {

        current_index = lol_index_buffer[i];

	if (current_index < 0) {
	// This should not happen, unless corrupted file
			  
	   LOL_ERRSET(EFAULT);
	   goto error;
        }

	amount = lol_io_dblock(op, current_index,
                 (char *)(ptr+off), block_size, LOL_WRITE);

        if (amount < 0) {
	  //    off = 0;
	    goto error;
        }

	off += amount;
	
	if (amount != block_size) {
	    amount = -1;
	    goto error;
        }
	
    } // end for i

    if (start_bytes) {

        current_index = lol_index_buffer[i];

	if (current_index < 0) {
	    LOL_ERRSET(EFAULT);
	    goto error;
        }

	amount = lol_io_dblock(op, current_index,
                 (char *)ptr + off, start_bytes, LOL_WRITE);

        if (amount < 0) {
	  //    off = 0;
	    goto error;
        }

        off += amount;

	if (amount != start_bytes) {
	    amount = -1;
	    goto error;
        }

  } // end if start bytes

  off /= size;

    // MUST ALSO WRITE MODIFIED CHAIN !

     ret = 0;
     op->nentry.file_size = (ULONG)new_filesize;
     ret = lol_update_nentry(op);

     if (ret) {
#if LOL_TESTING
       printf("DEBUG: lol_fwrite: lol_update_nentry failed. ret = %d\n", (int)(ret));
#endif
         op->err = lol_errno = EIO;
     }

     if (new_filesize != filesize) {
         ret = lol_update_chain(op, olds, news, last_old);
     } // end if file size changed.

     if (ret)
        LOL_ERRSET(EIO);

 error:
     lol_index_free((size_t)(write_blocks));

     if ((!(off)) && (!(op->err)))
         LOL_ERRSET(EIO);

  if (amount < 0) {
      off = 0;
      if (!(op->err))
         LOL_ERRSET(EIO);
  }

  return off;

} // end lol_fwrite
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
  FILE   *fp        = 0;
  size_t num_blocks = 0;
  size_t block_size = 0;
  size_t items      = 0;
  size_t off        = 0;
  size_t left       = 0;
  long   header     = 0;
  long   block      = 0;
  int    ret        = 0;

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

  num_blocks  = (size_t)op->sb.num_blocks;
  block_size  = (size_t)op->sb.block_size;

    if (block_number >= num_blocks)
        LOL_ERR_RETURN(ENFILE, -4);

    if (bytes > block_size)
        LOL_ERR_RETURN(EFAULT, -5);

    if (op->nentry.i_idx < 0)
        LOL_ERR_RETURN(ENFILE, -6);

    left = (size_t)op->nentry.file_size;

    if ((func == LOL_READ) && (op->curr_pos > left)) {
         op->eof = 1;
         return 0;
    }

    left -= op->curr_pos;

    if ((bytes <= left) || (func == LOL_WRITE)) {
	 left = bytes;
    }

    if (fgetpos (op->vdisk, &pos))
	LOL_ERR_RETURN(EBUSY, -7);

    fp     = op->vdisk;
    header = (long)LOL_DATA_START;
    block  = (long)(block_number * block_size);
    block  += header;

    // Byte offset inside a block?
    off = op->curr_pos % block_size;
    // We MUST stay inside block boundary!
    items = block_size - off;

    if (left > items)
	left = items;

    block += off;

    if (fseek(fp, block, SEEK_SET)) {

	ret = ferror(fp);

	if (ret)
	    op->err = lol_errno = ret;
	else
	    op->err = lol_errno = EIO;

        fsetpos (fp, &pos);
	return -8;

     } // end if seek error

     items = lol_fio(ptr, left, fp, func);

     if (!(items)) {

	 ret = ferror(fp);

	 if (ret)
	     op->err = lol_errno = ret;
	 else
	    op->err = lol_errno = EIO;

         fsetpos(fp, &pos);
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

   fsetpos(fp, &pos);
   return (long)items;

} // end lol_io_dblock
/* **********************************************************
 * lol_fseek:
 * sets the file position indicator for the stream pointed
 * to by stream.
 * ret = -1 : error
 * ret =  0 : success
 * 
 * ********************************************************** */
int lol_fseek(lol_FILE *op, long offset, int whence) {

 long pos, file_size;
 int ret = 0;

  if (!(op)) {
    lol_errno = EBADF;
    return -1;
  }
  if ((!(op->vdisk)) || (op->opened != 1)) {
    lol_errno = EBADF;
    return -1;
  }
  // It is possible to fseek over the file size.
  // But not under --> return -1 & EINVAL

  file_size = (long)op->nentry.file_size;
  pos       = (long)op->curr_pos;

      switch (whence) {

          case SEEK_SET:
	    // Seeking relative to start
            if (offset < 0) {
               ret = -1;
               op->err = lol_errno = EINVAL;
	    }
            else
	      op->curr_pos = offset;
	    break;

          case SEEK_CUR :

	    if ((pos + offset) < 0) {
	        ret = -1;
                op->err = lol_errno = EINVAL;
	    }
            else
                op->curr_pos += offset;
	    break;

          case SEEK_END :

	    if ((file_size + offset) < 0) {
	        ret = -1;
                op->err = lol_errno = EINVAL;
	    }
            else
                op->curr_pos = file_size + offset;
	    break;

          default :
            ret = -1;
	    op->err = lol_errno = EINVAL;
	    break;

      } // end switch

    if (!(ret))
        op->eof = 0;

    return ret;
} // end lol_fseek
/* **********************************************************
 * lol_unlink:
 * Deletes a file in container.
 * ret = -1 : error
 * ret =  0 : success
 * 
 * ********************************************************** */
int lol_unlink(const char *name) {

  struct lol_name_entry entry;
  lol_FILE *fp;
  size_t len;
  alloc_entry e, last_block;
  int ret = 0;

  if (!(name))
    LOL_ERRET(EFAULT, -1);

  if (!(name[0]))
    LOL_ERRET(ENAMETOOLONG, -1);

  len = strlen(name);
  if ((len < 4) || (len >= LOL_PATH_MAX))
    LOL_ERRET(ENAMETOOLONG, -1);

#ifdef HAVE_LINUX_FS_H
  if (len > 0) {
      if (name[len-1] == '/')
         LOL_ERRET(EISDIR, -1);
  }
#endif

  if ((lol_index_buffer) || (lol_buffer_lock))
      LOL_ERRET(EBUSY, -1);

  if (!(fp = lol_fopen(name, "r+"))) {
    // lol_errno already set
     return LOL_ERR_GENERR;
  }
  // Check that the entry is consistent, so that we won't
  // accidentally corrupt other files

  last_block = fp->sb.num_blocks - 1;

  if ((fp->nentry_index > last_block) ||
      (!(fp->sb.num_files))           ||
      (fp->nentry_index < 0))             {

    lol_errno = EIO;
    goto error;

  }

  e = fp->nentry.i_idx;

  if ((e > last_block) || (e < 0)) {
      lol_errno = EIO;
      goto error;
  }

  clearerr(fp->vdisk);
  if ((ret = lol_delete_chain_from(fp, LOL_UNLINK))) {

     switch (ret) {

            case LOL_ERR_BADFD :
            case LOL_ERR_PTR   :
	      lol_errno = EBADF;
	      goto error;
            break;
            case LOL_ERR_USER :
            case LOL_ERR_CORR :
	      lol_errno = EIO;
	      goto error;
            case LOL_ERR_IO :
	      // lol_errno already set
	      goto error;
            break;
            case LOL_ERR_INTRN :
	      lol_errno = EIO;
	      goto error;
            break;
            default :

	      if (ferror(fp->vdisk))
		lol_errno = errno;
              else
	        lol_errno = EBADF;

	      goto error;
            break;

     } // end switch;

     goto error;
  } // end if ret

  clearerr(fp->vdisk);
  if ((LOL_GOTO_DENTRY(fp))) {
      if (ferror(fp->vdisk))
	  lol_errno = errno;
      else
	  lol_errno = EBADF;
      goto error;
  }

  memset((char *)&entry, 0, NAME_ENTRY_SIZE);
  clearerr(fp->vdisk);
  if ((fwrite((char *)&entry,
      (size_t)(NAME_ENTRY_SIZE), 1, fp->vdisk)) != 1) {

      if ((ferror(fp->vdisk))) {
	        lol_errno = errno;
                goto error;
      }
      else{
	lol_errno = EIO;
	goto error;
      }

      goto error;

  } // end if fwrite

      // Adjust sb
      fp->sb.num_files--;

      if ((fseek(fp->vdisk, 0, SEEK_SET))) {
	 lol_errno = errno;
         goto error;
      } // end if fseek

      clearerr(fp->vdisk);
      if ((fwrite((char *)&fp->sb, (size_t)(DISK_HEADER_SIZE),
                   1, fp->vdisk)) != 1)                       {

	    if ((ferror(fp->vdisk))) {
	        lol_errno = errno;
                goto error;
	    }
	    else{
	      lol_errno = EIO;
	      goto error;
	    }

	 goto error;
      } // end if fwrite

   lol_fclose(fp);
   return LOL_OK;

error:

   lol_fclose(fp);
   return LOL_ERR_GENERR;
} // end lol_unlink
/* **********************************************************
 * lol_clearerr:
 * Clears the end-of-file and error indicators for the stream
 * pointed to by stream.
 * 
 * ret = N/A
 *
 * ********************************************************** */
void lol_clearerr(lol_FILE *op) {

  lol_errno = 0;

  if (!(op)) {
    lol_errno = EBADF;
    return;
  }
   op->eof = 0;
   op->err = 0;

} // end lol_clearerr
// FIX lol_ferror: returns now mixed POSIX & lol -errors?
/* **********************************************************
 * lol_ferror:
 * Tests the error indicator for the stream pointed to by stream,
 * returning nonzero if it is set.
 * 
 *
 * ********************************************************** */
int lol_ferror(lol_FILE *op) {

  if (!(op)) {
    lol_errno = EBADF;
    return -1;
  }

  return op->err;
} // end lol_ferror
/* **********************************************************
 * lol_ftell:
 * Returns the current value of the file position
 * and -1 if error.
 *
 * ********************************************************** */
long lol_ftell(lol_FILE *s) {

  long ret = -1;

  if (s) {
    if (s->vdisk) {
        ret = (long)(s->curr_pos);
    }
    else {
      s->err = EBADF;
    }
  }

  if (ret == -1) {
      lol_errno = EBADF;
  }

  return ret;
} // end lol_ftell
/* **********************************************************
 * lol_stat:
 * Stats the file pointed to by path and fills in buf.
 * Return value:
 * -1 : error
 *  0 : success
 * ********************************************************** */
int lol_stat(const char *path, struct stat *st) {

  size_t len;
  size_t nlen;
  FILE *fp;
  lol_FILE op;
  struct lol_super sb;
  struct lol_name_entry entry;
  long raw_size;
  long doff;
  //mode_t st_mode;
  DWORD block_size;
  DWORD num_blocks;
  DWORD i, num_files;
  int ret = -1;

  if (!(path)) {
     lol_errno = EFAULT;
     return -1;
  }

  if (!(st)) {
      lol_errno = EFAULT;
      return -1;
  }

  clean_private_lol_data(&op);

  if ((lol_get_filename(path, &op))) {
      lol_errno = ENOENT;
      return -1;
  }

  raw_size =  lol_get_vdisksize(op.vdisk_name, &sb, NULL, RECUIRE_SB_INFO);

  if (raw_size <  LOL_THEOR_MIN_DISKSIZE) {
      lol_errno = ENOTDIR;
      return -1;
  }

      block_size = (DWORD)sb.block_size;
      num_blocks = (DWORD)sb.num_blocks;
      num_files  = (DWORD)sb.num_files;

      if (LOL_INVALID_MAGIC) {
	 lol_errno = ENOTDIR;
         return -1;
      }

      if (num_files > num_blocks) {
	  lol_errno = EOVERFLOW;
	  return -1;
      }

      if (!(fp = fopen((char *)op.vdisk_name, "r"))) {
	  lol_errno = ENOENT;
          return -1;
      }

      doff = (long)LOL_DENTRY_OFFSET_EXT(num_blocks, block_size);

      if ((fseek(fp, doff, SEEK_SET))) {
	   lol_errno = ENOENT;
	   goto error;
      }

      nlen = strlen((char *)op.vdisk_file);
      if ((nlen < 1) || (nlen >= LOL_FILENAME_MAX)) {
	   lol_errno = ENAMETOOLONG;
	   goto error;
      }

      for (i = 0; i < num_blocks; i++) {

	if ((fread ((char *)&entry, NAME_ENTRY_SIZE, 1, fp)) != 1) {
	         lol_errno = EOVERFLOW;
		 goto error;
        }

        if (!(entry.filename[0])) {
	       continue;
	}

        len = strlen((char *)entry.filename);

        if ((len >= LOL_FILENAME_MAX) || (len != nlen)) {
	     continue;
	}

        if ((strncmp((char *)entry.filename, (char *)op.vdisk_file, len))) {
	       continue;
        }
        else {

          // Gotcha !

       	  st->st_dev  = LOL_FILE_DEV; // a fake device
          st->st_ino  = (ino_t)(i);
          st->st_mode = (S_IFREG | S_IRUSR | S_IWUSR |
                         S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

          st->st_nlink   = 1;
          st->st_uid     = 0;
          st->st_gid     = 0;
          st->st_rdev    = LOL_FILE_RDEV;
          st->st_size    = (off_t)entry.file_size;
          st->st_blksize = (blksize_t)block_size;
          if (entry.file_size) {
            st->st_blocks  = (blkcnt_t)(st->st_size / block_size);
            if (st->st_size % block_size)
	        st->st_blocks++;
	  }
	  else {
	    st->st_blocks = 1;
	  }
          st->st_atime = time(NULL);
          st->st_mtime = st->st_ctime = entry.created;
	  ret = 0;

          break;

        } // end else

  } // end for i

 error:
    fclose(fp);
    return ret;

} // end lol_stat
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

  DWORD num_blocks  = 0;
  DWORD block_size  = 0;
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

  num_blocks = sb.num_blocks;
  block_size = sb.block_size;
  nf         = sb.num_files;

  if (nf > num_blocks)
    return LOL_ERR_CORR;

  if (!(vdisk = fopen(disk, "r"))) {
       return LOL_ERR_IO;
  }

  doff = (long)LOL_DENTRY_OFFSET_EXT(num_blocks, block_size);

  if ((fseek (vdisk, doff, SEEK_SET))) {
       fclose(vdisk);
       return LOL_ERR_IO;
  }

  // Read the name entries
  for (i = 0; i < num_blocks; i++) {

    if ((fread((char *)&name_e, (size_t)(NAME_ENTRY_SIZE), 1, vdisk) != 1)) {
       fclose(vdisk);
       return LOL_ERR_IO;
    } // end if cannot read

    if (name_e.filename[0]) { // Should check more but this will do now
      files++;
      used_space += name_e.file_size;
    }
  } // end for i

   // Read also the reserved blocks.
  for (i = 0; i < num_blocks; i++) {  // We could propably suck all the indexes into
                                      // a buffer with a few reads, something todo...

    if ((fread((char *)&entry, (size_t)(ENTRY_SIZE), 1, vdisk) != 1)) {
       fclose(vdisk);
       return LOL_ERR_IO;
    } // end if cannot read

    if (entry != FREE_LOL_INDEX)
      used_blocks++;

  } // end for i

  fclose(vdisk);

  occupation = (long)(used_blocks * block_size);
  free_space = (long)(num_blocks - used_blocks);
  if (m == LOL_SPACE_BYTES) {
      free_space *= block_size;
  }

  if ((used_space > occupation)  ||
      (used_blocks > num_blocks) ||
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
/* **********************************************************
 * lol_mkfs: Create a lolfs container.
 * There are two distinct ways to call this function:
 * 1: Define the desired container size directly (opt = "-s")
 *     lol_mkfs("-s", "100M", 0, 0, "lol.db");
 *
 * 2: Define the data block size and number of them (opt = "-b")
 *     lol_mkfs("-b", NULL, 128, 50000, "lol.db");
 *
 * return value:
 * < 0: error
 *   0: succes
 ************************************************************ */
int lol_mkfs (const char *opt, const char *amount,
              const DWORD bs, const DWORD nb, const char *path)
{
  struct stat st;
  FILE *fp;
  struct lol_super sb; // This is the first a couple
                       // of bytes in the container file
  size_t v     =  0;
  DWORD blocks =  1;
  DWORD bsize  =  1;
  int      ret = -1;

  if ((!(path)) || (!(opt)))
      return LOL_ERR_USER;

  if ((!(opt[0])) || (!(path[0])))
     return LOL_ERR_USER;

  // Does user want to specify size or number of blocks and their size?
  if ((!(strcmp(opt, "-b"))) || (!(strcmp(opt, "--blocks")))) {

      if ((bs < 1) || (nb < 1))
	return LOL_ERR_USER;

      blocks = nb;
      bsize  = bs;

  } // end if add blocks
  else {

     if ((!(strcmp(opt, "-s"))) || (!(strcmp(opt, "--size")))) {
       // User wants to define size instead of blocks
       if (!(amount))
	  return LOL_ERR_USER;
       if (!(amount[0]))
	 return LOL_ERR_USER;

       if ((lol_size_to_blocks(amount, NULL,
            NULL, NULL, &blocks, LOL_JUST_CALCULATE))) {
	    return LOL_ERR_INTRN;
       }
       else {
	 // Got the amount of blocks!
	 bsize = LOL_DEFAULT_BLOCKSIZE;
       } // end else got it
     }
     else {
       // Neither one, user error...
       return LOL_ERR_USER;
     } // end else
  } // end else blocks

  if ((bsize < 1) || (blocks < 1)) // This should never happen here
    return LOL_ERR_INTRN;

  sb.num_files = 0;
  // We need 2 numbers, block size and the number of blocks
  sb.block_size = bsize;
  sb.num_blocks = blocks;
  // We set these two bytes to LOL_MAGIC always when creating a container
  sb.reserved[0] = sb.reserved[1] = LOL_MAGIC;

  if (!(fp = fopen(path, "w")))
      return LOL_ERR_IO;

  // Then we just write the information to the new container metadata/superblock
  // (See the definition of struct lol_super in <lolfs.h> for details).
  if ((fwrite((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
      ret = LOL_ERR_IO;
      goto error;
  }

  // After the super block, follow all the data blocks and
  // we initialize them with zeros.
  v = (size_t)(blocks * bsize);
  if ((null_fill (v, fp)) != v) {
      ret = LOL_ERR_IO;
      goto error;
  }

  // After the datablocks comes the (root-)directory entries,
  // which in a new container, we fill all of them with zeros...
  v = blocks * NAME_ENTRY_SIZE;
  if ((null_fill (v, fp)) != v) {
    ret = LOL_ERR_IO;
    goto error;

  }
  // Finally after the directory entries comes the data-allocation
  // indexes, which (in a new  container) we set all of them all to
  // FREE_LOL_INDEX value.
  v = blocks * ENTRY_SIZE;
  if ((lol_fill_with_value(FREE_LOL_INDEX, v, fp)) != v) {
        ret = LOL_ERR_IO;
        goto error;
  }

  ret = 0;
error:

  fclose(fp);
  if (!(ret))
    return 0;

  if (!(stat(path, &st))) {

    if (S_ISREG(st.st_mode)) {
        unlink(path);
    }

  } // end if !stat

  return ret;
} // end lol_mkfs
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

  if ((!(before)) || (!(after)))
     return;

     b_len = strlen(before);
     if (len <= b_len) {
       if (err)
          lol_error("%s %s", before, after);
       else
	 printf("%s %s", before, after);
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
  const char numbers[] = "1234567890";
  int i;

  for (i = 0; i < 10; i++) {
    if (ch == numbers[i])
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
  if (str[0] == '0')
    return -1;
  len = (int)strlen(str);
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

    bs     = (long)sb->block_size;
    num_bl = (long)sb->num_blocks;
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
         multiplier = 1024;
        break;
       case 'M':
       case 'm':
         multiplier = 1048576;
        break;
      case 'G':
      case 'g':
        multiplier = 1073741824;
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
/* ********************************************************* */
long lol_get_io_size(const long size) {

  long j;
  long div;
  long frac;

  if (!(size))
    return 1;
  if (size >= LOL_GIGABYTE)
    return LOL_MEGABYTE;
  if (size < 4096)
     return 4096;

  for (j = 4096; j <= LOL_MEGABYTE; j <<= 1) {
    if (size == j)
      return j;
  }
  for (j = LOL_GIGABYTE; j >= 131072; j >>= 1) {
      if (size > j) {
	div = size >> 3;
	frac = div % 4096;
	div -= frac;
        return div;
      }
  } // end for j

 return 4096;
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

  char      temp[4096];
  long nb, bs, add = 0;
  char     *buffer = 0;
  long   i = 0, io = 0;
  long   data_size = 0;
  long  old_offset = 0;
  long  new_offset = 0;
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

  nb  = (long)sb->num_blocks;
  bs  = (long)sb->block_size;
  add = (long)new_blocks;
  add += nb;

  data_size  = (long)NAME_ENTRY_SIZE;
  data_size *= (long)(nb);
  io = lol_get_io_size(data_size);
  if (io < 4096) // Should never happen
      io = 4096;

  if (!(buffer = (char *)lol_malloc((size_t)(io)))) {
     buffer = temp;
     io     = 4096;
  }
  else {
    alloc = 1;
  }
  // First we relocate the index entries
  // Let's calculate their total size
  // and current and new offsets.

   data_size  = (long)ENTRY_SIZE;
   data_size *= (long)(nb);
  old_offset  = (long)LOL_TABLE_START_EXT(nb, bs);
  new_offset  = (long)LOL_TABLE_START_EXT(add, bs);
  old_offset += data_size;
  new_offset += data_size;
   table_end  = new_offset;

  times = data_size / io;
  frac  = (size_t)(data_size % io);

  fp = fopen(container, "r+");
  if (!(fp))
    goto ret;

do_relocate:

  if (frac) {

     old_offset -= frac;
     new_offset -= frac;

     if ((fseek(fp, old_offset, SEEK_SET)))
         goto err;
     if ((lol_fio(buffer, frac, fp, LOL_READ)) != frac)
         goto err;
     if ((fseek(fp, new_offset, SEEK_SET)))
         goto err;
     if ((lol_fio(buffer, frac, fp, LOL_WRITE)) != frac)
         goto err;

  } // end if frac

  for (i = 0; i < times; i++) {

     old_offset -= io;
     new_offset -= io;

     if ((fseek(fp, old_offset, SEEK_SET)))
         goto err;
     if ((lol_fio(buffer, io, fp, LOL_READ)) != io)
         goto err;
     if ((fseek(fp, new_offset, SEEK_SET)))
         goto err;
     if ((lol_fio(buffer, io, fp, LOL_WRITE)) != io)
         goto err;

  } // end for i

  pass++;

  // Next we add the new indexes or new name entries
  if (pass == 1) {
      frac = (size_t)((size_t)(new_blocks) * (size_t)(ENTRY_SIZE));
     if ((fseek(fp, table_end, SEEK_SET)))
         goto err;
      if ((lol_fill_with_value(FREE_LOL_INDEX, frac, fp)) != frac) {
          goto err;
      }
  }// end if pass == 1
  else {
     frac = (size_t)((size_t)(new_blocks) * (size_t)(NAME_ENTRY_SIZE));
     if ((fseek(fp, dir_end, SEEK_SET)))
         goto err;
     if ((null_fill (frac, fp)) != frac) {
        goto err;
     }
  } // end else

  if (pass > 1) {
    // Adjust the meta block and return
     sb->num_blocks += new_blocks;
     if ((fseek(fp, 0, SEEK_SET)))
         goto err;
     if ((fwrite((char *)sb,
	 (size_t)DISK_HEADER_SIZE, 1, fp)) != 1)
         goto err;
     if (alloc) {
        lol_free((size_t)(io));
     }
     fclose(fp);
     return 0;
  }

  // Next we relocate the name entries
  // Let's calculate their total size
  // and current and new offsets.

   data_size  = (long)NAME_ENTRY_SIZE;
   data_size *= (long)(nb);
  old_offset  = (long)LOL_DENTRY_OFFSET_EXT(nb,  bs);
  new_offset  = (long)LOL_DENTRY_OFFSET_EXT(add, bs);
  old_offset += data_size;
  new_offset += data_size;
     dir_end  = new_offset;
       times  = data_size / io;
        frac  = (size_t)(data_size % io);

  goto do_relocate;

err:
 fclose(fp);
ret:
 if (alloc) {
   lol_free((size_t)(io));
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
