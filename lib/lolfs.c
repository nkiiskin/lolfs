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

/* $Id: lolfs.c, v0.40 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $" */

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

// TODO: lol_cp & others: set a func to lol_free_space so it breaks after
//       needed blocks have been found.
// TODO: Move op->cs to super block
// TODO: Add first free index to lol_FILE struct
// TODO: lol_cp takes time to copy big files to container!
// TODO: Make use of new lol_FILE members: fd, n_off, p_len and f_len etc..
// TODO: add global lol_FILE[] buffer instead of mallocing mem for files.
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
#if LOL_COLOR_MESSAGES
  "\x1b[32m[OK]\x1b[0m\n",
  "\x1b[34m[INFO]\x1b[0m\n",
  "\x1b[35m[WARNING]\x1b[0m\n",
  "\x1b[31m[ERROR]\x1b[0m\n",
  "\x1b[31m[FATAL]\x1b[0m\n",
  "\x1b[31m[INTERNAL]\x1b[0m\n",
  NULL,
#else
  "[OK]\n",
  "[INFO]\n",
  "[WARNING]\n",
  "[ERROR]\n",
  "[FATAL]\n",
  "[INTERNAL]\n",
  NULL,
#endif
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
#if 0
const struct lol_open_mode lol_open_modes[] =
{
    { .n = 0, .m = "r"  },
    { .n = 1, .m = "r+" },
    { .n = 2, .m = "r+" },
    { .n = 3, .m = "r+" },
    { .n = 4, .m = "r+" },
    { .n = 5, .m = "r+" },
};
#endif
/* ********************************************************** */
alloc_entry *lol_index_buffer = 0;
alloc_entry  lol_storage_buffer[LOL_STORAGE_SIZE+1];
const size_t LOL_INDEX_SIZE_T = (size_t)(ENTRY_SIZE);
const ssize_t LOL_INDEX_SSIZE_T = (ssize_t)(ENTRY_SIZE);
/*  Valid file characters does NOT mean that a filename must use these
 *  characters. It means that other characters than these are considered
 *  suspicious by some functions or programs (like fsck.lolfs).
 */
const char lol_valid_filechars[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.- _~:!+#%[]{}?,;&()=@";
static lol_signalstore lol_sigstore[] =
{
  { .store = {}, .sig = SIGHUP,  .set = 0},
  { .store = {}, .sig = SIGINT,  .set = 0},
  { .store = {}, .sig = SIGTERM, .set = 0},
  { .store = {}, .sig = SIGSEGV, .set = 0},
  { .store = {}, .sig = SIGQUIT, .set = 0}, 
  { .store = {}, .sig = SIGTSTP, .set = 0},
  { .store = {}, .sig = -1,      .set = 0},
};
/* ********************************************************** */
// fcntl(fd, F_SETFL, ...)
void lol_restore_sighandlers (void) {

  lol_signalstore *st;
  int i = 0;
#if LOL_TESTING
   puts("Entered lol_restore_sighandlers");
#endif
  for (;; i++) {
#if LOL_TESTING
     printf("Trying to restore lol_signal index %d\n", i);
#endif
     st = &lol_sigstore[i];
     if (st->sig < 0) {
       break;
     }
     if (st->set) {
       if (!(sigaction(st->sig, &st->store,  NULL))) {
             st->set = 0;
#if LOL_TESTING
	     printf("Restored signal %d\n", st->sig);
#endif
         } // end if sigaction succeed
	 else {
	   // Failed -> Try restoring the default handler
#if LOL_TESTING
      printf ("lol_restore_sighandlers: trying to restore DEFAULT %d\n", (int)i);
#endif
	   lol_set_defaultsignal(i);
         } // end else
       } // end if set
    } // end for i
} // end lol_restore_sighandlers
/* ********************************************************** */
static void lol_freemem_hdl(int sig, siginfo_t *so, void *c)
{
  int code = 0;
  if (so) {
     code = so->si_code;
  }

#if LOL_TESTING
  printf("lol_freemem_hdl: lol_buffer_lock = %d\n", lol_buffer_lock);
#endif

  if ((lol_index_buffer) &&
      (lol_buffer_lock == 2)) {
       free (lol_index_buffer);
#if LOL_TESTING
       printf("lol_freemem_hdl: caught signal %d, code %d\n", sig, code);
#endif

  }
#if LOL_TESTING
  printf("lol_freemem_hdl: exiting with signal %d, code %d\n", sig, code);
#endif

  fflush(0);
  if (code > 0) {
      code = -code;
  }
  exit (code);
} // end lol_freemem_hdl
/* ********************************************************** */
int lol_set_defaultsignal (const int lol_signum) {

    struct sigaction sa;
    lol_signalstore *st;

    if (lol_signum < 0) {
      return -1;
    }
    st = &lol_sigstore[lol_signum];
    if (st->set) {

       memset (&sa, 0, sizeof(sa));
       sigemptyset(&sa.sa_mask);
       sa.sa_handler = SIG_DFL;
       sa.sa_flags = SA_RESTART; // | SA_RESETHAND
       sigfillset(&sa.sa_mask);
       if (!(sigaction(st->sig, &sa, NULL))) {
#if LOL_TESTING
	  printf("Restored DEFAULT signal %d\n", st->sig);
#endif
	  return st->set = 0;
       }
    } // end if sig is set
    return -1;
} // end lol_set_defaultsignal
/* ********************************************************** */
int lol_setup_sighandlers (void) {

    struct sigaction sa;
    lol_signalstore *st;
    int i = 0, errs = 0;

    memset (&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = &lol_freemem_hdl;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigfillset(&sa.sa_mask);

    for (;; i++) {
#if LOL_TESTING
      printf ("lol_setup_sighandlers: trying to set sig index %d\n", (int)i);
#endif
       st = &lol_sigstore[i];
       if (st->sig < 0) {
	 break;
       }
       if (!(st->set)) {
          memset(&st->store, 0, sizeof(sa));
          if ((sigaction(st->sig, &sa, &st->store))) {

#if LOL_TESTING
	    printf ("lol_setup_sighandlers: FAILED to set signal %d (index %d)\n", (int)(st->sig), (int)(i));
#endif
	      st->set = 0;
	      errs--;
          }
	  else {
	      st->set = 1;
#if LOL_TESTING
	  printf("Set signal %d\n", i);
#endif
	  }
       } // end if not set
    } // end for i
#if LOL_TESTING
      printf ("lol_setup_sighandlers: returning %d\n", (int)errs);
#endif
  return errs;
} // end lol_setup_sighandlers
/* **********************************************************
 * lol_rgetsize: PRIVATE FUNCTION, USE WITH CAUTION!
 * Return the size (in bytes) of a given raw block device
 * or regular file or link to a regular file or -1, if error
 * or if size is too small to be a container.
 * Most likely must be run as root if it is a block device.
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
 * lol_get_maxfiles:
 * What is the maximum number of files a given container
 * may have if you only know it's stat and not know if it's
 * corrupted or not.
 * NOTE: Function assumes that the file has been stat'd already
 *       ant the result is in input 'st'.
 *
 * Return value:
 *   <0 : error : cannot compute
 *  >=0 : success : return value is the answer
 * ********************************************************** */
long lol_get_maxfiles(const struct stat *st) {

  long size;
  long max_files;
  const long mul = NAME_ENTRY_SIZE + ENTRY_SIZE;
  const long min_s = LOL_MINCONTAINER_SIZE;
  if (!(st))
    return -1;

  // Check the obvious errors 1st
  size = (long)st->st_size;
  if (size < min_s) {
    return -1;
  }
  // Then the send the obvious answer
  size -= DISK_HEADER_SIZE;
  max_files = size / mul;
  //max_files += (size % mul);
  return max_files;
} // end lol_get_maxfiles
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

  /*
  if (!(op))
    return -1;
  */

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

  lol_nentry p;
  off_t pos;
  off_t offset = op->dir;
  off_t off = NAME_ENTRY_SIZE;
  const DWORD nb = op->sb.nb;
  alloc_entry idx, v;
  int fd = op->fd;
  int ret = -1;

  if (nentry >= nb) {
    return -1;
  }
  off *= nentry;
  offset += off;
  if ((pos = lseek(fd, offset, SEEK_SET)) < 0) {
     return -1;
  }
  if (remove_idx) {
    // Free the index also
    if ((pread(fd, (char *)&p, NAME_ENTRY_SIZE, pos)) !=
	                        NAME_ENTRY_SIZE ) {
       goto error;
    }
    idx = p.i_idx;
    if ((idx < 0) || (idx >= nb)) {
      // invalid index, we remove only the name
       goto skip_index;
    }
    offset = op->idxs + (idx * ENTRY_SIZE);
    v = FREE_LOL_INDEX;
    if ((pwrite(fd, (char *)&v, ENTRY_SIZE,
                offset)) != ENTRY_SIZE) {
        goto error;
    }
  } // end if delete index too

 skip_index:
  memset((char *)&p, 0, NAME_ENTRY_SIZE);
  if ((write(fd, (char *)&p, NAME_ENTRY_SIZE)) !=
                             NAME_ENTRY_SIZE) {
      goto error;
  }

  ret = 0;
error:
  lseek(fd, pos, SEEK_SET);
  return ret;
} // end lol_remove_nentry
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
   const off_t off = op->idxs + (op->nentry.i_idx * ENTRY_SIZE);
   if ((pwrite(op->fd, (char *)&val, ENTRY_SIZE, off)) != ENTRY_SIZE) {
      return -1;
   }
   return 0;
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
  int rv;

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
   if (bytes > LOL_INDEX_MALLOC_MAX)
     LOL_ERRET(ENOMEM, LOL_ERR_MEM);


#if LOL_TESTING
   printf("lol_index_malloc: trying to allocate %ld bytes\n", (long)(bytes));
#endif
   if (!(lol_index_buffer = (alloc_entry *)malloc(bytes)))
      LOL_ERRET(ENOMEM, LOL_ERR_MEM);

#if LOL_TESTING
   puts("lol_index_malloc: Succes!");
#endif

   // Set handlers
   rv = lol_setup_sighandlers();
#if LOL_TESTING
   printf("lol_index_malloc: lol_setup_sighandlers returned %d\n", rv);
#endif
   if (rv < -2) { // We let 2 signal handlers fail
        free (lol_index_buffer);
        lol_errno = EAGAIN;
#if LOL_TESTING
   puts("lol_index_malloc: Failed to lol_setup_sighandlers()");
   puts("lol_index_malloc: restoring other signals back...");
#endif
        lol_restore_sighandlers();
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
#if LOL_TESTING
   puts("lol_index_malloc: set lol_buffer_lock to 2, returning OK");
#endif

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
#if LOL_TESTING
   puts("Entered lol_index_free");
#endif
   if (!(lol_buffer_lock)) {
#if LOL_TESTING
   puts("lol_index_free: No lock! returning");
#endif
     return;
   }
 if (amount > (LOL_STORAGE_SIZE)) {
#if LOL_TESTING
   puts("lol_index_free: amount > LOL_STORAGE_SIZE");
#endif
   if ((lol_index_buffer) && (lol_buffer_lock == 2)) {
         free (lol_index_buffer);
#if LOL_TESTING
	 puts("lol_index_free: calling lol_restore_sighandlers");
#endif
         lol_restore_sighandlers();
   }
#if LOL_TESTING
   else {
       puts("lol_index_free: no dynamic memory to free...");
       printf("lol_index_free: amount to free: %ld\n", (long)amount);
   }
#endif
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

  if (bytes > LOL_INDEX_MALLOC_MAX)
     return NULL;
  if ((lol_buffer_lock) || (!(bytes)))
    return NULL;
  // We use lol_index_malloc, which allocates
  // memory in ENTRY_SIZE sized units.
  LOL_CALC_ENTRIES(entries, bytes, e_size, frac);

#if LOL_TESTING
  printf("lol_malloc: trying to lol_index_malloc(%ld)\n",
         (long)(entries));
#endif
  if ((lol_index_malloc(entries))) {
#if LOL_TESTING
   printf("lol_malloc: failed to allocate(%ld)\n", (long)(entries));
#endif
     return NULL;
  }
#if LOL_TESTING
   printf("lol_malloc: Success allocated(%ld)\n", (long)(entries));
#endif
  return (void *)(lol_index_buffer);
} // end lol_malloc
/* ********************************************************** */
void lol_free(const size_t size) {
#if LOL_TESTING
   printf("Entered lol_free\n");
#endif
  if ((!(lol_buffer_lock)) || (!(size))) {
#if LOL_TESTING
   printf("lol_free: lock not set, returning\n");
#endif
     return;
  }
  if (lol_buffer_lock == 2) {
      lol_index_free(LOL_STORAGE_ALL);
#if LOL_TESTING
      printf("lol_free: called lol_index_free(%ld)\n", (long)(LOL_STORAGE_ALL));
#endif
      return;
  }
#if LOL_TESTING
   printf("lol_free: calling lol_index_free(1)\n");
#endif
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
               void *fp, const lol_io_func io)

{
  unsigned int r, i, t, n = 0;
  if ((!(ptr)) || (bytes <= 0) || (!(io))) {
    return 0;
  }
  r = (unsigned int)(bytes % LOL_DEFBUF);
  t = (unsigned int)(bytes / LOL_DEFBUF);
  for (i = 0; i < t; i++) {
        if (io((char *)&ptr[n], LOL_DEFBUF, 1, fp) != 1)
             return n;
        n += LOL_DEFBUF;
  }
  if (r) {
    if ((io((char *)&ptr[n], (size_t)(r), 1, fp)) != 1)
          return n;
        n += r;
  }
  return (size_t)(n);
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
 * Return value:
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

  off_t offset;
  ULONG fs = op->nentry.fs;
  const off_t skip = op->idxs;
  const size_t es = (size_t)(ENTRY_SIZE);
  const ssize_t ses = (ssize_t)(es);
  const alloc_entry empty = FREE_LOL_INDEX;
  const alloc_entry last  = LAST_LOL_INDEX;
  DWORD nb = op->sb.nb;
  DWORD bs = op->sb.bs;
  DWORD fblocks, i = 0;
  alloc_entry idx = op->nentry.i_idx;
  int fd = op->fd;

    // Quick check
    if ((idx >= nb) ||
	(fs > op->data_s) || (!(bs))) {
       return LOL_ERR_CORR;
    }
    // How many blocks in the file?
    LOL_FILE_BLOCKS(fs, bs, fblocks);

    if (fblocks > nb) {
        return LOL_ERR_CORR;
    }
    if (flags) { // Save first block (truncate)

      offset = skip + (idx * es);
#if LOL_TESTING
      printf("delete_chain_from: Seeking to index %d\n", idx);
#endif
      if ((lseek(fd, offset, SEEK_SET)) < 0) {
          goto error;
      }
      if ((read(fd, (char *)&idx, es)) != ses) {
          goto error;
      }
      if ((lseek(fd, -es, SEEK_CUR)) < 0) {
           goto error;
      }
      if ((write(fd, (const char *)&last, es)) != ses) {
            goto error;
      }
      i = 1;

    } // end if flags


    for (; i < fblocks; i++) {

      offset = skip + (idx * es);

#if LOL_TESTING
      printf("delete_chain_from: Seeking to index %d\n", idx);
#endif
      if ((lseek(fd, offset, SEEK_SET)) < 0) {
          goto error;
      }
      if ((read(fd, (char *)&idx, es)) != ses) {
          goto error;
      }
      if ((lseek(fd, -es, SEEK_CUR)) < 0) {
           goto error;
      }
      if ((write(fd, (const char *)&empty, es)) != ses) {
          goto error;
      }

  } // end for

  return LOL_OK;

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
#define LOL_READNENTRY_TMP 1024
int lol_read_nentry(lol_FILE *op) {

  const long  nes =  ((long)(NAME_ENTRY_SIZE));
  const long temp_mem = nes * LOL_READNENTRY_TMP;
  char temp[temp_mem];
  const char  *name = (char *)op->file;
  char  *ename;
  size_t mem = 0;
  FILE   *fp = op->dp;
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
#if LOL_TESTING
   printf("lol_read_nentry: called lol_malloc(%ld)\n", (long)(io));
#endif
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
       k = ((int)(io / nes));
       loops = 0;
      goto dentry_loop;

  } // end if frac

  ret = 0;

position:
  fsetpos(fp, &pos);
freemem:
  if (alloc) {
#if LOL_TESTING
   printf("lol_read_nentry: calling lol_index_free(%ld)\n", (long)(mem));
#endif
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
  long       nb = (long)sb->nb;
  long idx_area = op->idxs_s;
  alloc_entry newe = *idx;
  alloc_entry *buf;
  fpos_t       pos;
  size_t mem = 0;
  long offset;
  long   frac;
  long   full;
  long     io;

  alloc_entry what;
  int times;
  int k;
  int  i, j;
  int     alloc = 0;
  int      ret = -1;
  int loops = 1;

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
#if LOL_TESTING
   printf("lol_get_free_index: called lol_malloc(%ld)\n", (long)(io));
#endif
        alloc = 1;
     }

  } else {
      buf = (alloc_entry *)temp;
      io  = temp_mem;
  }

  offset = op->idxs;
  times  = (int)(idx_area / io);
  frac   = idx_area % io;
  full   = idx_area - frac;
  full  /= idx_s;

  if ((fgetpos(fp, &pos)))
       goto err;
  if ((fseek(fp, offset, SEEK_SET)))
       goto error;
  k = (int)(io / idx_s);
  if ((io % idx_s) || (frac % idx_s)) {
    lol_debug("lol_get_free_index: internal error, sorry!");
       goto error;
  }

  i = 0;

 check_loop:

  for (; i < times; i++) {
   if ((fread((char *)buf, io, 1, fp)) != 1)
       goto error;

      for (j = 0; j < k; j++) {

          if (buf[j] == FREE_LOL_INDEX) {

	     if (flag) {
	        offset = -(k);
	        offset += j;
	        offset *= idx_s;
                if ((fseek(fp, offset, SEEK_CUR)))
                    goto error;
	        if ((fwrite((const char *)&newe, (size_t)(idx_s),
		       1, fp)) != 1)
	               goto error;
	     }

	     if (loops)
                what = (i * k);
	     else
	        what = full;

	     *idx = (alloc_entry)(what + j);
	     ret = 0;
	     goto done; // OK, --> return

          } // end if found free index
      } // end for j
  } // end for i
  if ((frac) && (loops)) {
      times++;
      io = frac;
      k = (int)(io / idx_s);
      loops = 0;
      goto check_loop;
  } // end if frac

 error:
 done:
  fsetpos(fp, &pos);
 err:
  if (alloc) {
#if LOL_TESTING
   printf("lol_index_malloc: calling lol_index_free(%ld)\n", (long)(mem));
#endif
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
  const long  nes =  ((long)(NAME_ENTRY_SIZE));
  const long temp_mem = nes * LOL_FREEDENTRY_TMP;
  char temp[temp_mem];
  fpos_t      pos;
  lol_nentry *buf;
  size_t mem = 0;
  long io;
  long base = op->dir;
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
#if LOL_TESTING
   printf("lol_get_free_nentry: called lol_malloc(%ld)\n", (long)(io));
#endif
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
#if LOL_TESTING
   printf("lol_get_free_nentry: calling lol_index_free(%ld)\n", (long)(mem));
#endif
     lol_index_free(mem);
  }
  return ret;
} // end lol_get_free_nentry
#undef LOL_FREEDENTRY_TMP
/* ********************************************************** *
 * lol_touch_file:  PRIVATE function exclusively for lol_fopen.
 *
 * Create a new file of zero size.
 * Assumes that the file name is already in op->file.
 * NOTE: DOES NOT RETURN ERROR IF FILE EXISTS ALREADY!
 *
 * Return < 0 if error.
 * ********************************************************** */
#define LOL_TOUCH_ERROR(x,y) { op->err = lol_errno = (x); ret = (y); goto error; }
int lol_touch_file(lol_FILE *op) {

  lol_ninfo info;
  lol_nentry *nentry;
  long off;
  char *name = op->file;
#ifdef LOL_INLINE_MEMCPY
  char *tmp;
  int x;
#endif
  alloc_entry idx = LAST_LOL_INDEX;
  DWORD nb   = op->sb.nb;
  DWORD nf   = op->sb.nf;
  int  len, ret = -1;

  // Make some checks
  if (nf >= nb)
     LOL_ERR_RETURN(ENOSPC, -4);
  len = op->f_len;
  if (len >= LOL_FILENAME_MAX) {
      len = LOL_FILENAME_MAX - 1;
      name[len] = '\0';
  }

  // Ok, action begins here...
  // Find a free index and reserve it
  // (There should be a free index
  //  since nf < nb).
  if ((lol_get_free_index(op, &idx, LOL_MARK_USED)))
      LOL_TOUCH_ERROR(ENFILE, -10);

  // create a zero sized file
  nentry = &op->nentry;

#ifdef LOL_INLINE_MEMCPY
  tmp = (char *)nentry->name;
  for (x = 0; x < len; x++) {
       tmp[x] = name[x];
  }
#else
  LOL_MEMCPY(nentry.name, name, len);
#endif
  nentry->name[len] = '\0';
  nentry->created   = time(NULL);
  nentry->i_idx     = idx;
  nentry->fs = 0;
  info.ne = nentry;
  off = lol_get_free_nentry(op, &info);

  if (off < 0) { // Failed, free the index
     lol_set_index_value(op, FREE_LOL_INDEX);
     LOL_TOUCH_ERROR(EIO, -9);
  }

  if ((lol_supermod(op->dp, &(op->sb), LOL_INCREASE))) {
     // Try to rollback...
     if (!(lol_remove_nentry(op,(DWORD)(info.idx), 1))) {
	lol_set_index_value(op, FREE_LOL_INDEX);
     }
     LOL_TOUCH_ERROR(EIO, -13);
  }

  op->curr_pos = 0;
  op->n_idx = (alloc_entry)info.idx;
  op->n_off = info.off;
  return 0;
error:
  return ret;
} // end lol_touch_file
#undef LOL_TOUCH_ERROR
/* ********************************************************** */
int lol_truncate_file(lol_FILE *op) {

  lol_nentry *entry = &op->nentry;
  // The file is > 1 block, --> truncate to zero

  // Truncate index chain first.
  if (entry->fs > op->sb.bs) {
    if ((lol_delete_chain_from(op, LOL_SAVE_FIRST_BLOCK)))  {
        return -1;
    }
  }
  if (LOL_GOTO_DENTRY(op)) {
      return -2;
  }
  // Set zero size
  entry->fs = 0;
  entry->created = time(NULL);
  // update name entry
  if ((fwrite((const char *)entry,
       NAME_ENTRY_SIZE, 1, op->dp)) != 1) {
       return -3;
  }
  return 0;
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
  long src_bs;
#if LOL_TESTING
  printf("lol_can_replace: src_fs = %ld, dst_fs = %ld\n", src_fs, dst_fs);
  printf("                 freebl = %ld,     bs = %ld\n", freebl, bs);
#endif

  if ((bfree < 0) || (bs <= 0) ||
     (dst_fs < 0) || (src_fs < 0)) {
      return -1;
  }
  if (((src_fs) <= (dst_fs + bfree))) {
      return 0;
  }
  // How many blocks does the source file
  // allocate if put into container?
  LOL_FILE_BLOCKS(src_fs, bs, src_bs);
  // How many blocks does the existing file
  // in the container allocate?
  LOL_FILE_BLOCKS(dst_fs, bs, availab);
  // Add free blocks to it
  availab += freebl;

#if LOL_TESTING
  printf("lol_can_replace: available blocks = %ld, source blocks = %ld\n", availab, src_bs);
  if (src_bs > availab)
    puts("Will return -1 now");
#endif
  if (src_bs > availab) {
     return -1;
  }
  return 0;
} // end lol_can_replace
/* ********************************************************** */
int lol_update_ichain(lol_FILE *op, const long olds,
                      const alloc_entry last_old) {

  long start;
  long i, offset;
  alloc_entry idx;

  if (!(lol_index_buffer))
    return -1;
  if (last_old < 0)
    return -2;

  start = LOL_TABLE_START(op);
  i = olds;
  // Join old and new chains first
  if (i) {
    // Take last index of old chain
    // and make it point to first index of the new chain
     idx = lol_index_buffer[i-1];
     offset = start + idx * ENTRY_SIZE;
     if (fseek(op->dp, offset, SEEK_SET)) {
	 return -3;
     }
     if ((fwrite((const char *)&lol_index_buffer[i],
		(size_t)(ENTRY_SIZE), 1, op->dp)) != 1) {
        return -4;
     }
  } // end if olds (i)
  else {
    // So, we don't have any old blocks to write over.
    // Then what was the last old block? We must adjust
    // it to point to the next new...
    offset = start + last_old * ENTRY_SIZE;
    if (fseek(op->dp, offset, SEEK_SET)) {
	return -5;
    }
    if ((fwrite((const char *)&lol_index_buffer[0],
	       (size_t)(ENTRY_SIZE), 1, op->dp)) != 1) {
	return -6;
    }
  } // end else not olds

  // Then write new indexes
  while (lol_index_buffer[i] >= 0) {

     idx = lol_index_buffer[i];
     offset = start + idx * ENTRY_SIZE;
     if (fseek(op->dp, offset, SEEK_SET)) {
         return -7;
     }
     if ((fwrite((const char *)&lol_index_buffer[i+1],
		 (size_t)(ENTRY_SIZE), 1, op->dp)) != 1) {
        return -8;
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
 *         The value in *mids has the number of blocks which
 *         will be skipped (and not written into) if the file
 *         position is over the current file size.
 *         Note: return value = *olds + *news if success.
 *
 * ********************************************************** */
long lol_new_indexes(lol_FILE *op, const long bytes,
      long *olds, long *mids, long *news, long *new_sz) {

  long      need;
  long      left;
  long     f_off;
  long      area;
  long  room = 0;
  long delta = 0;
  const long pos = (long)op->curr_pos;
  const long  bs = (long)op->sb.bs;
  const long  fs = (long)op->nentry.fs;
  const long npos = pos + bytes;
  const long dpos = pos - fs;
  const long blocks = (long)lol_num_blocks(op, (size_t)bytes, NULL);

  *olds = *news = *mids = 0;
#if 0

  if (!(fs)) {  // Size = 0, EASY Case
      // In this case we need new blocks for
      // the whole write request, except the first one.
      if (blocks) { // we may not need to check this??
	 *olds = 1;
	 *news = blocks - 1;
         *new_sz = npos;
       }
       return blocks;
   } // end if zero size file
   // Ok, we know it is a regular file, size > 0
   // and we write > 0 bytes.
   if (!(blocks)) // We must have at least one block!
       return LOL_ERR_USER;
#endif


  // Check first if we need new blocks at all.
  f_off = fs % bs;
  if (f_off) {
     room = bs - f_off;
  }

  //  left = -dpos; // may be <= 0 also
  // Do we have ALSO room in the last data block of the file?
  left = room - dpos;  // add is >= 0
  if (left >= bytes) {
     // We don't need new area, the old blocks are enough
      *news = 0; *olds = blocks;
      if (fs < npos) {
	 *new_sz = npos;
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
#if 0
  if (!(need)) { // Really should not be here
      return LOL_ERR_INTRN;
  }
#endif

  // We know the number of NEW blocks (need).
  // How many old blocks are there?
  if (dpos >= 0) { // In this case we don't need olds
                   // at all unless pos is inside the
                   // last block.
    if (dpos < room) {
        *olds = 1;
    }
    else {
      // *olds = 0;
       delta = dpos - room;
       if (delta >= bs) {
	  *mids = delta / bs;
       }

    } // end else
  } // end if pos
  else {
    *olds = left / bs;
    if (left % bs) {
       (*olds)++;
    }
 } // end else

  *news = need;
  *new_sz = fs + room + area;
  return ((*olds) + (*news));
  // What a mess!!!
} // lol_new_indexes
/* **********************************************************
 *
 *  lol_read_ichain:
 *  This function is exclusively for lol_fread
 *  Read the whole block index chain to user buffer.
 *  Return value:
 *  <  0 : error
 *  >= 0 : success. The value is number of indices in the buffer
 *
 * ********************************************************** */
int lol_read_ichain(lol_FILE *op, const size_t rbs)
{
  off_t off;
  const off_t skip = (off_t)LOL_TABLE_START(op);
  const ULONG   fs = op->nentry.fs;
  const DWORD   nb = op->sb.nb;
  const DWORD   bs = op->sb.bs;
  alloc_entry  idx = op->nentry.i_idx;
  int c = 0;
  DWORD bb;
  int  fd, f_bs, i;

  if ((!(rbs)) || (rbs > nb))
     LOL_ERR_RETURN(EIO, LOL_ERR_USER);
  bb = op->curr_pos / bs;
  LOL_FILE_BLOCKS(fs, bs, f_bs);
  if (rbs > f_bs)
     LOL_ERR_RETURN(EFBIG, LOL_ERR_CORR);

  fd = op->fd;

  for (i = 0; i < f_bs; i++)  {

     off = skip + idx * LOL_INDEX_SIZE_T;
     if (i >= bb) {
       if ((idx < 0) || (idx >= nb))
          LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

       lol_index_buffer[c++] = idx;
       if (c >= rbs) {
           break;
       }
     } // end if i >= bb
     if ((pread (fd, (char *)&idx, LOL_INDEX_SIZE_T, off)) !=
                                  LOL_INDEX_SSIZE_T)
        LOL_ERR_RETURN(EIO, LOL_ERR_IO)
     if (idx == LAST_LOL_INDEX) {
         break;
     }
     if ((idx >= nb) || (idx < 0))
       LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

  } // end for i

  if (!(c))
    LOL_ERR_RETURN(EIO, LOL_ERR_CORR);

  return c;
} // end lol_read_ichain
/* **********************************************************
 * // CHECK UNDER THIS
 *  lol_new_ichain:
 *  Create a block index chain based on number of
 *  needed old and new blocks.
 *
 *  Return value:
 *  <0  : error
 *   0  : success.
 * ********************************************************** */
#define LOL_NEW_ICHAIN_BUF 1024
#define LOL_IC_LIMIT 64
int lol_new_ichain(lol_FILE *op, const int olds,
                           const int news, alloc_entry *last_old)
{
  alloc_entry buf[ENTRY_SIZE * LOL_NEW_ICHAIN_BUF];
  FILE     *fp = op->dp;
  const long idx_s = (long)(ENTRY_SIZE);
  const long area = op->idxs_s;
  const ULONG  fs  = op->nentry.fs;
  const int   num  = olds + news;
  const long skip  = LOL_TABLE_START(op);
  const DWORD  nb  = op->sb.nb;
  const DWORD  bs  = op->sb.bs;
  long  io = idx_s * LOL_NEW_ICHAIN_BUF;
  long      frac;
  long       off;
  alloc_entry  idx = op->nentry.i_idx;
  alloc_entry f_idx;

  int    fbs, k;
  int     times;
  int     n_idx;
  int     i = 0;
  int w = SEEK_SET;

  if (!(last_old)) {
    return LOL_ERR_USER;
  }
  if (olds > nb) {
     return LOL_ERR_CORR;
  }

  *last_old = f_idx = idx;
  LOL_FILE_BLOCKS(fs, bs, fbs);

  if (olds > fbs)
    LOL_ERR_RETURN(EIO, LOL_ERR_INTRN);
  if ((olds + news) > nb)
    LOL_ERR_RETURN(ENOSPC, LOL_ERR_SPACE);

  k = fbs - olds;
  off = skip + idx_s * f_idx;

  if (fseek(fp, off, SEEK_SET))
     return LOL_ERR_IO;

  // fp points now to the first index.
  // We must skip some indexes (k)
  // to reach the file position.

  while (i < k) {
      *last_old = idx;
      if ((fread((char *)&idx,
	  (size_t)(idx_s), 1, fp)) != 1)
          return LOL_ERR_IO;

      off = skip + idx_s * idx;
      if (fseek(fp, off, SEEK_SET))
         return LOL_ERR_IO;
      i++;
  } // end while i

  // Do we have olds?
  for (i = 0; i < olds; i++) {

     if (idx < 0)
       return LOL_ERR_CORR;
       lol_index_buffer[i] = idx;
       *last_old = idx;
     if ((fread((char *)&idx,
		(size_t)(idx_s), 1, fp)) != 1)
          return LOL_ERR_IO;
     if (idx >= 0) {
         off = skip + idx_s * idx;
	 w = SEEK_SET;
     }
     else {
         off = ENTRY_SIZE;
         w = SEEK_CUR;
     }
     if (fseek (fp, off, w))
         return LOL_ERR_IO;
  } // end for olds

  // If we also have new indexes, then find them also and
  // put them into the buffer.
  if (num <= LOL_IC_LIMIT) {
      goto quick_search;
  }

#if LOL_TESTING
  puts("lol_new_ichain: using buffered index search");
#endif
  // We try to suck in all the indices
  // at the same time
  if (io > area) {
      io = area;
  }
  times  = (int)(area / io);
  frac = area % io;

  if ((fseek(fp, skip, SEEK_SET)))
       return -1;
  n_idx = (int)(io / idx_s);
  if ((io % idx_s) || (frac % idx_s)) {
      lol_debug("lol_new_ichain: internal (io / idx) error, sorry!\n");
      return -1;
  }
  k = 0;

 check_loop:

  for (; k < times; k++) {

      if ((fread((char *)buf, io, 1, fp)) != 1) {
         return -1;
      }
      for (w = 0; w < n_idx; w++) {
          if (buf[w] == FREE_LOL_INDEX) {
            idx = (k * n_idx + w);
            if (idx >= nb) {
               lol_debug("lol_new_ichain: idx >= nb, should not happen!!\n");
	       op->err = lol_errno = ENOSPC;
	       return LOL_ERR_SPACE;
             }
	    //  printf("lol_new_ichain: %d goes to ind_buf[%d]\n", (int)idx, (int)i);
             lol_index_buffer[i++] = idx;
	     if (i >= num) {
	       return 0;
	     }
          } // end if found free index
      } // end for j
  } // end for i
  if (frac) {
      puts("lol_new_ichain: IN FRAC LOOP should not be here (1)!");
      n_idx = (int)(frac / idx_s);
      times++;
      io = frac;
      frac = 0;
      goto check_loop;
  } // end if frac

  puts("lol_new_ichain: should not be here (2)!");
  return -1;

quick_search:
#if LOL_TESTING
  puts("lol_new_ichain: using UNbuffered index search");
#endif
  // Go back to start
  w = 0;
  if (fseek (fp, skip, SEEK_SET))
      return LOL_ERR_IO;
  while (i < num) {
      if (w >= nb) {
	  op->err = lol_errno = ENOSPC;
	  return LOL_ERR_SPACE;
      }
      if ((fread((char *)&idx,
		 (size_t)(ENTRY_SIZE), 1, fp)) != 1)
          return LOL_ERR_IO;
      if (idx == FREE_LOL_INDEX) {
         // Save the index and continue
	//	printf("lol_new_ichain: %d goes to ind_buf[%d]\n", (int)w, (int)i);
          lol_index_buffer[i++] = w;

      } // end if found free block
      w++;
  } // end while

  return 0;
} // end lol_new_ichain
#undef LOL_IC_LIMIT
#undef LOL_NEW_ICHAIN_BUF
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

  const size_t pos = (size_t)op->curr_pos;
  const size_t  bs = (size_t)op->sb.bs;

  size_t block_off;
  size_t delta_off;
  size_t    behind;
  size_t      save;
  size_t after = 0;
  size_t start = 0;
  size_t loops = 0;
  size_t   end = 0;

  if (!(amount))
     return 0;

  /* SIX POSSIBILITIES:

     1: We write only some full BLOCKS
     2: We write some full BLOCKS and START of a block
     3: We write only START of a block
     4: We write only the END of a block (from somewhere in the middle to the end)
     5: We write END and some full BLOCKS
     6: We write END, some full BLOCKS and START
  */

  block_off = pos % bs;
  delta_off = (pos + amount) % bs;
     behind = pos / bs;

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
#if LOL_TESTING == 2
      printf("lol_num_blocks: got loops = %d\n", (int)loops);
#endif
      if (amount % bs) {
#if LOL_TESTING == 2
      printf("lol_num_blocks: got start = %d\n", (int)delta_off);
#endif
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
       after  = (pos + amount) / bs;
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

#if 0
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
#endif

  save = loops;
  if (start)
     loops++;
  if (end)
     loops++;

  if (q) { // If q is given, save the results there
      q->nb    = loops;
      q->start = start;
      q->loops = save;
      q->end   = end;
  } // end if q

  return loops;
} // end lol_num_blocks
/* **********************************************************
 * lol_rblock:
 * Reads contents of a given
 * data block into user buffer.
 * ret <0 : error
 * ret >=  0 : success, return value is the bytes read.
 *
 * ********************************************************** */
long lol_rblock(lol_FILE *op, const size_t bnum,
		   char *ptr, const size_t bytes)
{

  const size_t fs = (size_t)op->nentry.fs;
  // FILE     *fp;
  size_t    bs;
  size_t items;
  size_t   off;
  size_t  left;
  off_t  block;
  // int      ret;
  int       fd;

  if ((!(bytes)) || (op->eof))
       return 0;

  // This is a private function. We won't bother checking
  // which open mode the the op is...
    fd = op->fd;
    left = fs;
    if (op->curr_pos >= left) {
        op->curr_pos = left;
        op->eof = 1;
        return 0;
    }

    bs  = (size_t)op->sb.bs;
    if (bnum >= op->sb.nb)
        LOL_ERR_RETURN(ENFILE, -4);
    if (bytes > bs)
        LOL_ERR_RETURN(EFAULT, -5);

    left -= op->curr_pos;
    if (bytes <= left) {
	left = bytes;
    }
    // fp = op->dp;
    block = (off_t)LOL_DATA_OFFSET(bs, bnum);

    // Byte offset inside a block?
    off = op->curr_pos % bs;
    // We MUST stay inside block boundary!
    items = bs - off;
    if (left > items) {
	left = items;
    }
    block += off;

    //fseek(fp, block, SEEK_SET);
    if ((pread(fd, ptr, left, block)) != left) {
        LOL_ERRSET(EIO)
	return -9;
    }
    op->curr_pos += left;
    if ((op->curr_pos >= fs)) {
         op->curr_pos = fs;
         op->eof = 1;
         return (long)left;
    }
    if (op->curr_pos < 0) {
        op->curr_pos = 0;
    }

   return (long)left;
} // end lol_rblock
/* **********************************************************
 * lol_wblock:
 * Writes contents of a given
 * data block from user buffer.
 * ret <0 : error
 * ret >=  0 : success, return value is the bytes written.
 *
 * ********************************************************** */
long lol_wblock(lol_FILE *op, const size_t bnum,
		   char *ptr, const size_t bytes)
{
  size_t    bs;
  size_t items;
  size_t   off;
  size_t  left;
  off_t  block;
  int       fd;

  if (!(bytes))
      return 0;

  // This is a private function. We won't bother checking
  // which open mode the the op is...

    bs  = (size_t)op->sb.bs;
    if (bnum >= op->sb.nb)
        LOL_ERR_RETURN(ENFILE, -4);
    if (bytes > bs)
        LOL_ERR_RETURN(EFAULT, -5);
    left = bytes;
    fd = op->fd;
    block = (off_t)LOL_DATA_OFFSET(bs, bnum);

    // Byte offset inside a block?
    off = op->curr_pos % bs;
    // We MUST stay inside block boundary!
    items = bs - off;
    if (left > items) { // Does this ever happen???
	left = items;
    }
    block += off;
    if ((pwrite(fd, ptr, left, block)) != left) {

         LOL_ERRSET(EIO)
	 return -9;
     }
     op->curr_pos += left;

   return (long)left;
} // end lol_wblock
/* **********************************************************
 * lol_free_blocks: Calculate free blocks in a container.
 * (Block Size of the container will be returned in 'usb->bs').
 * return value:
 * < 0  : error
 * >= 0 : free space (in bytes of blocks)
 ************************************************************ */
#define LOL_FREEBLK_TMP 10240
long lol_free_blocks (const char *cont, lol_meta *usb)
{
  // NOTE: We are not looking for corruption or invalid
  // parameters here. Just calculate free blocks & space
  char temp[((ENTRY_SIZE) * (LOL_FREEBLK_TMP))];
  const long  es = (long)(ENTRY_SIZE);
  const long temp_mem = es * LOL_FREEBLK_TMP;
  lol_meta xsb;
  size_t  mem = 0;
  long blocks = 0;
  long    ret = 0;
  alloc_entry   *buf;
  lol_meta       *sb;
  FILE           *fp;
  long            io;
  long          doff;
  long            ds;
  long          frac;
  long         times;
  long    nb, bs, nf;
  int          j , k;
  int          i = 0;
  // Check parameters
  if (!(cont)) {
     return LOL_ERR_PTR;
  }
  if (usb) {
    sb = usb;
  }
  else {
    sb = &xsb;
  }
  // Fix this: we should move this info to sb!
  frac = lol_getsize(cont, sb, NULL, RECUIRE_SB_INFO);
  if (frac < LOL_THEOR_MIN_DISKSIZE) {
     return LOL_ERR_CORR;
  }
  bs = (long)sb->bs;
  nb = (long)sb->nb;
  nf = (long)sb->nf;
  if (nf > nb) {
    return LOL_ERR_CORR;
  }
  ds = nb * es;
  io = lol_get_io_size(ds, es);
  if (io <= 0) {
      lol_debug("lol_free_space: Internal error: io <= 0");
      return LOL_ERR_INTRN;
  }
  if (io > temp_mem) {
    if (!(buf = (alloc_entry *)lol_malloc((size_t)(io)))) {
          buf = (alloc_entry *)temp;
          io  = temp_mem;
    } else {
       mem = (size_t)(io);
#if LOL_TESTING
   printf("lol_free_space: called lol_malloc(%ld)\n", (long)(io));
#endif
    }
  } else {
    buf = (alloc_entry *)temp;
    io  = temp_mem;
  }

  times = ds / io;
  frac  = ds % io;
  k = (int)(io / es);
  doff = (long)LOL_TABLE_START_EXT(nb, bs);

  if (!(fp = fopen(cont, "r"))) {
       ret = LOL_ERR_IO;
       goto just_free;
  }
  if (fseek (fp, doff, SEEK_SET)) {
      ret = LOL_ERR_IO;
      goto closefree;
  }

 index_loop:
  for (; i < times; i++) {
    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         ret = LOL_ERR_IO;
         goto closefree;
    }
    for (j = 0; j < k; j++) {
      if (buf[j] == FREE_LOL_INDEX) {
         blocks++;
      }
    } // end for j
  } // end for i
  // Now the fractional data
  if (frac) {
      times++;
      io = frac;
      k = (int)(io / es);
      frac = 0;
      goto index_loop;
  } // end if frac

  if ((blocks > (nb - nf))) {
       ret = LOL_ERR_CORR;
  }
  else {
    ret = blocks;
  }

closefree:
 fclose(fp);
just_free:
 if (mem) {
#if LOL_TESTING
   printf("lol_free_space: calling lol_free(%ld)\n", (long)(mem));
#endif
    lol_free(mem);
 }
 return ret;
} // end lol_free_blocks
#undef LOL_FREEBLK_TMP
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
      return LOL_FS_TOOSMALL;
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
