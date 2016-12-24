/*
 *
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
 * $Id: lol_internal.h, v0.13 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"
 *
 *
 */



/*
 *  None of these functions, expressions, etc are NOT meant to be used
 *  by the end user.
 *  They are here ONLY because they are needed to compile the lolfs library.
 *  The user interface is in the file lolfs.h
 *
 */


#ifndef _LOL_INTERNAL_H
#define _LOL_INTERNAL_H  1
#endif
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
/* ********************************************************** */
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#ifndef _UNISTD_H
#include <unistd.h>
#endif
#ifndef _TIME_H
#include <time.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif



enum {

  LOL_RDONLY,           // "r"
  LOL_RDWR,             // "r+"
  LOL_WR_CREAT_TRUNC,   // "w"
  LOL_RDWR_CREAT_TRUNC, // "w+"
  LOL_APPEND_CREAT,     // "a"
  LOL_RD_APPEND_CREAT,  // "a+"

};

/* TODO: Move all the defining macros to a separate .h -file
         so that parameters (such as size of index buffer, size of
         alloc_entry etc..) may be changed just by editing one file.
*/


#define LOL_FILE_SIZE (sizeof(struct _lol_FILE))
#define DISK_HEADER_SIZE (sizeof(struct lol_super))
#define ENTRY_SIZE (sizeof(alloc_entry))
#define LOL_FILE_DEV  (0xf64affc8)
#define LOL_FILE_RDEV (0)
// LOL_MAGIC number is actually 0x1414, so we only need one definition
#define LOL_MAGIC 0x14
#define LOL_FALSE 0xFEFFFFDF
#define NAME_ENTRY_SIZE (sizeof(struct lol_name_entry))

// Some macros for common expressions/tasks
#define LOL_DEVSIZE(x,y)           (DISK_HEADER_SIZE + ((ULONG)(x)) * \
                                   (((ULONG)(y)) + ENTRY_SIZE + \
                                   NAME_ENTRY_SIZE))
#define LOL_INDEX_OFFSET(x,y,z)    (DISK_HEADER_SIZE + (x) * \
                                   (NAME_ENTRY_SIZE + (y)) + (z) * ENTRY_SIZE)
#define LOL_DENTRY_OFFSET(x)       (DISK_HEADER_SIZE + ((x)->sb.num_blocks) * \
                                   ((x)->sb.block_size))
#define LOL_DENTRY_OFFSET_EXT(x,y) ((long)(DISK_HEADER_SIZE) + \
                                   (long)(x) * (long)(y))
#define LOL_GOTO_NENTRY(x,y,z,w)   (fseek((x), DISK_HEADER_SIZE + (y) * (z) + \
                                   (w) * NAME_ENTRY_SIZE, SEEK_SET))
#define LOL_GOTO_DENTRY(x)         (fseek((x)->vdisk, DISK_HEADER_SIZE + \
                                   (x)->nentry_index * NAME_ENTRY_SIZE + \
                                   (x)->sb.block_size * \
                                   (x)->sb.num_blocks, SEEK_SET))
#define LOL_TABLE_START_EXT(x,y)   (DISK_HEADER_SIZE + (x) * \
                                   (NAME_ENTRY_SIZE + (y)))
#define LOL_TABLE_START(x)         (DISK_HEADER_SIZE + ((x)->sb.num_blocks) * \
                                   (((x)->sb.block_size) + NAME_ENTRY_SIZE))
#define LOL_CHECK_MAGIC(x)         ((x)->sb.reserved[0] != LOL_MAGIC || \
                                   (x)->sb.reserved[1] != LOL_MAGIC)
#define LOL_ERRET(x,y)             { lol_errno = (x); return (y); }
#define LOL_ERR_RETURN(x,y)        { op->err = lol_errno = (x); return (y); }
#define LOL_ERRSET(x)              { op->err = lol_errno = (x); }
#define LOL_CHECK_HELP             ((!(strcmp(argv[1], "-h"))) || \
                                    (!(strcmp(argv[1], "--help"))))
#define LOL_CHECK_VERSION          ((!(strcmp(argv[1], "-v"))) || \
                                    (!(strcmp(argv[1], "--version"))))
#define LOL_INVALID_MAGIC          ((sb.reserved[0] != LOL_MAGIC) ||	\
				    (sb.reserved[1] != LOL_MAGIC))
#define LOL_DATA_START             (DISK_HEADER_SIZE)
#define LOL_WRONG_OPTION           ("lol %s: unrecognized option \'%s\'\n")
#define LOL_VERSION_FMT            ("lol %s v%s %s\n")
#define LOL_USAGE_FMT              ("lol %s v%s. %s\nUsage: lol %s %s\n")
#define LOL_FSCK_FMT               ("        fsck.lolfs recommended")
#define LOL_INTERERR_FMT           ("Internal error. Sorry!")
#define LOL_TESTING    0
// #define LOL_THEOR_MIN_DISKSIZE (DISK_HEADER_SIZE + NAME_ENTRY_SIZE + ENTRY_SIZE + 2)
extern const long LOL_THEOR_MIN_DISKSIZE;
extern const long LOL_DEFAULT_BLOCKSIZE;
#define LOL_READ  0
#define LOL_WRITE 1

#define E_DISK_FULL "Not enough space in disk\n"
#define E_OUT_MEM "Out of memory!\n"
#define E_DISK_IO "I/O error\n"

// Some internal use flags
#define LOL_NO_SUCH_FILE 0
#define LOL_FILE_EXISTS  1

// Private constants, not to use in the interface!
#define LAST_LOL_INDEX  -1
#define FREE_LOL_INDEX  -2

// Constants for lol_check_corr
#define LOL_CHECK_SB    1
#define LOL_CHECK_FILE  2
#define LOL_CHECK_BOTH  3

// Private error constants, not to be used also
#define LOL_OK           (0)
#define LOL_ERR_EOF      (0)
#define LOL_ERR_GENERR  (-1)
#define LOL_ERR_SEGMENT (-2)
#define LOL_ERR_IO      (-3)
#define LOL_ERR_USER    (-4)
#define LOL_ERR_MEM     (-5)
#define LOL_ERR_CORR    (-6)
#define LOL_ERR_MODE    (-7)
#define LOL_ERR_INTRN   (-8)
#define LOL_ERR_SPACE   (-9)
#define LOL_ERR_PTR     (-10)
#define LOL_ERR_PARAM   (-11)
#define LOL_ERR_BUSY    (-12)
#define LOL_ERR_SIG     (-13)
#define LOL_ERR_BADFD   (-14)


#define LOL_KILOBYTE (1024)
#define LOL_MEGABYTE (1048576)
#define LOL_GIGABYTE (1073741824)
// Signal handlers (internal use only. Do NOT change these!)
#define LOL_SIGHUP  0
#define LOL_SIGINT  1
#define LOL_SIGTERM 2
#define LOL_SIGSEGV 3
#define LOL_NUM_SIGHANDLERS 4

enum lol_divs {
              LOL_DIV_1,
              LOL_DIV_2,
              LOL_DIV_4,
              LOL_DIV_8,
              LOL_DIV_16,
              LOL_DIV_32,
              LOL_DIV_64,
              LOL_DIV_128,
              LOL_DIV_256,
              LOL_DIV_512,
              LOL_DIV_1024,
              LOL_DIV_2048,
              LOL_DIV_4096
};

#define MAX_LOL_OPEN_MODES 6

// Constants for lol_get_vdisksize func
#define RECUIRE_SB_INFO 1
#define USE_SB_INFO     2

// Constants for lol_supermod func
#define LOL_INCREASE 0
#define LOL_DECREASE 1

// Constants for lol_free_space func
#define LOL_SPACE_BYTES   1
#define LOL_SPACE_BLOCKS  2
// Constants for lol_size_to_blocks func

// Storage size here is the limit, after which we will
// allocate memory dynamically.
// Bigger LOL_STORAGE_SIZE may result in better performance.
#define LOL_STORAGE_SIZE 16383
#define LOL_STORAGE_ALL  ((size_t)(LOL_STORAGE_SIZE + 1))

// Flags to lol_delete_chain_from:
#define LOL_UNLINK           0
#define LOL_SAVE_FIRST_BLOCK 1
// uhh..
#define LOL_FORMAT_TO_REGULAR 1
#define LOL_LOCAL_TRUNCATE    2

// Flags to lol_get_free_index:
#define LOL_MARK_USED        1

struct lol_loop {
  // private:
   size_t num_blocks;
   size_t start_bytes;
   size_t full_loops;
   size_t end_bytes;

};

typedef size_t (*lol_io_func)(void *, size_t, size_t, FILE *);

// Miscallaneous helper-functions,
// not to be used in the interface

int         lol_index_malloc(const size_t num_entries);
void        lol_index_free (const size_t amount);
void*       lol_malloc(const size_t size);
void        lol_free(const size_t size);
int         lol_valid_sb(const lol_FILE *op);
int         lol_check_corr(const lol_FILE *op, const int mode);
size_t      null_fill(const size_t bytes, FILE *);
void        lol_help(const char* lst[]);
int         lol_size_to_str(const unsigned long size, char *s);
size_t      lol_fio(char *ptr, const size_t bytes, FILE *s, const int func);
int         lol_delete_chain_from(lol_FILE *, int);
int         lol_read_nentry(lol_FILE *);
BOOL        lol_is_validfile(char *name);
int         lol_get_basename(const char* name, char *new_name, const int mode);
size_t      lol_fill_with_value(const alloc_entry value, size_t bytes, FILE *stream);
long        lol_io_dblock(lol_FILE *op, const size_t block_number,
			      char *ptr, const size_t bytes, int func);
size_t      lol_num_blocks(lol_FILE *op, const size_t amount, struct lol_loop *loop);
void        lol_restore_sighandlers(void);
long        lol_get_rawdevsize (char *device, struct lol_super *sb, struct stat *st);
long        lol_get_vdisksize (char *name, struct lol_super *sb, struct stat *st, int func);
int         lol_remove_nentry (FILE *fp, const DWORD nb, const DWORD bs, const DWORD nentry,
                               int remove_idx);
alloc_entry lol_get_index_value (FILE *f, const DWORD nb, const DWORD bs,
                                 const alloc_entry idx);
int         lol_set_index_value (FILE *f, const DWORD nb, const DWORD bs,
                                 const alloc_entry idx, const alloc_entry new_val);
int         lol_supermod (FILE *vdisk, struct lol_super *sb, const int func);
int         lol_count_file_blocks (FILE *vdisk, const struct lol_super *sb,
                                   const alloc_entry first_index, const long dsize,
                                   long *count, const int terminate);
long        lol_free_space (char *container, const int mode);
void        lol_align(const char *before, const char *after, const size_t len);
int         lol_garbage_filename(const char *name);
int         lol_size_to_blocks(const char *size, const char *container,
                               const struct lol_super *sb,
                               const struct stat *st, DWORD *nb);
int         lol_is_number(const char ch);
int         lol_is_integer(const char *str);
long        lol_get_io_size(const long size);
int         lol_extendfs(const char *container, const DWORD new_blocks,
			 struct lol_super *sb, const struct stat *st);

// N_LOLFUNCS must match the number of functions below it
#define N_LOLFUNCS 8
int lol_ls   (int a, char* b[]);
int lol_rm   (int a, char* b[]);
int lol_cp   (int a, char* b[]);
int lol_df   (int a, char* b[]);
int lol_cat  (int a, char* b[]);
int lol_fs   (int a, char* b[]);
int lol_cc   (int a, char* b[]);
int lol_rs   (int a, char* b[]);

//
// These constants are here just for reference
// (Copied from gcc include files)
//

#if 0
#define EPERM            1      /* Operation not permitted */
#define ENOENT           2      /* No such file or directory */
#define ESRCH            3      /* No such process */
#define EINTR            4      /* Interrupted system call */
#define EIO              5      /* I/O error */
#define ENXIO            6      /* No such device or address */
#define E2BIG            7      /* Argument list too long */
#define ENOEXEC          8      /* Exec format error */
#define EBADF            9      /* Bad file number */
#define ECHILD          10      /* No child processes */
#define EAGAIN          11      /* Try again */
#define ENOMEM          12      /* Out of memory */
#define EACCES          13      /* Permission denied */
#define EFAULT          14      /* Bad address */
#define ENOTBLK         15      /* Block device required */
#define EBUSY           16      /* Device or resource busy */
#define EEXIST          17      /* File exists */
#define EXDEV           18      /* Cross-device link */
#define ENODEV          19      /* No such device */
#define ENOTDIR         20      /* Not a directory */
#define EISDIR          21      /* Is a directory */
#define EINVAL          22      /* Invalid argument */
#define ENFILE          23      /* File table overflow */
#define EMFILE          24      /* Too many open files */
#define ENOTTY          25      /* Not a typewriter */
#define ETXTBSY         26      /* Text file busy */
#define EFBIG           27      /* File too large */
#define ENOSPC          28      /* No space left on device */
#define ESPIPE          29      /* Illegal seek */
#define EROFS           30      /* Read-only file system */
#define EMLINK          31      /* Too many links */
#define EPIPE           32      /* Broken pipe */
#define EDOM            33      /* Math argument out of domain of func */
#define ERANGE          34      /* Math result not representable */
#define EDEADLK         35      /* Resource deadlock would occur */
#define ENAMETOOLONG    36      /* File name too long */
#define ENOLCK          37      /* No record locks available */

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS.
 */
#define ENOSYS          38      /* Invalid system call number */

#define ENOTEMPTY       39      /* Directory not empty */
#define ELOOP           40      /* Too many symbolic links encountered */
#define EWOULDBLOCK     EAGAIN  /* Operation would block */
#define ENOMSG          42      /* No message of desired type */
#define EIDRM           43      /* Identifier removed */
#define ECHRNG          44      /* Channel number out of range */
#define EL2NSYNC        45      /* Level 2 not synchronized */
#define EL3HLT          46      /* Level 3 halted */
#define EL3RST          47      /* Level 3 reset */
#define ELNRNG          48      /* Link number out of range */
#define EUNATCH         49      /* Protocol driver not attached */
#define ENOCSI          50      /* No CSI structure available */
#define EL2HLT          51      /* Level 2 halted */
#define EBADE           52      /* Invalid exchange */
#define EBADR           53      /* Invalid request descriptor */
#define EXFULL          54      /* Exchange full */
#define ENOANO          55      /* No anode */
#define EBADRQC         56      /* Invalid request code */
#define EBADSLT         57      /* Invalid slot */

#define EDEADLOCK       EDEADLK

#define EBFONT          59      /* Bad font file format */
#define ENOSTR          60      /* Device not a stream */
#define ENODATA         61      /* No data available */
#define ETIME           62      /* Timer expired */
#define ENOSR           63      /* Out of streams resources */
#define ENONET          64      /* Machine is not on the network */
#define ENOPKG          65      /* Package not installed */
#define EREMOTE         66      /* Object is remote */
#define ENOLINK         67      /* Link has been severed */
#define EADV            68      /* Advertise error */
#define ESRMNT          69      /* Srmount error */
#define ECOMM           70      /* Communication error on send */
#define EPROTO          71      /* Protocol error */
#define EMULTIHOP       72      /* Multihop attempted */
#define EDOTDOT         73      /* RFS specific error */
#define EBADMSG         74      /* Not a data message */
#define EOVERFLOW       75      /* Value too large for defined data type */
#define ENOTUNIQ        76      /* Name not unique on network */
#define EBADFD          77      /* File descriptor in bad state */
#define EREMCHG         78      /* Remote address changed */
#define ELIBACC         79      /* Can not access a needed shared library */
#define ELIBBAD         80      /* Accessing a corrupted shared library */
#define ELIBSCN         81      /* .lib section in a.out corrupted */
#define ELIBMAX         82      /* Attempting to link in too many shared libraries */
#define ELIBEXEC        83      /* Cannot exec a shared library directly */
#define EILSEQ          84      /* Illegal byte sequence */
#define ERESTART        85      /* Interrupted system call should be restarted */
#define ESTRPIPE        86      /* Streams pipe error */
#define EUSERS          87      /* Too many users */
#define ENOTSOCK        88      /* Socket operation on non-socket */
#define EDESTADDRREQ    89      /* Destination address required */
#define EMSGSIZE        90      /* Message too long */
#define EPROTOTYPE      91      /* Protocol wrong type for socket */
#define ENOPROTOOPT     92      /* Protocol not available */
#define EPROTONOSUPPORT 93      /* Protocol not supported */
#define ESOCKTNOSUPPORT 94      /* Socket type not supported */
#define EOPNOTSUPP      95      /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT    96      /* Protocol family not supported */
#define EAFNOSUPPORT    97      /* Address family not supported by protocol */
#define EADDRINUSE      98      /* Address already in use */
#define EADDRNOTAVAIL   99      /* Cannot assign requested address */
#define ENETDOWN        100     /* Network is down */
#define ENETUNREACH     101     /* Network is unreachable */
#define ENETRESET       102     /* Network dropped connection because of reset */
#define ECONNABORTED    103     /* Software caused connection abort */
#define ECONNRESET      104     /* Connection reset by peer */
#define ENOBUFS         105     /* No buffer space available */
#define EISCONN         106     /* Transport endpoint is already connected */
#define ENOTCONN        107     /* Transport endpoint is not connected */
#define ESHUTDOWN       108     /* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS    109     /* Too many references: cannot splice */
#define ETIMEDOUT       110     /* Connection timed out */
#define ECONNREFUSED    111     /* Connection refused */
#define EHOSTDOWN       112     /* Host is down */
#define EHOSTUNREACH    113     /* No route to host */
#define EALREADY        114     /* Operation already in progress */
#define EINPROGRESS     115     /* Operation now in progress */
#define ESTALE          116     /* Stale file handle */
#define EUCLEAN         117     /* Structure needs cleaning */
#define ENOTNAM         118     /* Not a XENIX named type file */
#define ENAVAIL         119     /* No XENIX semaphores available */
#define EISNAM          120     /* Is a named type file */
#define EREMOTEIO       121     /* Remote I/O error */
#define EDQUOT          122     /* Quota exceeded */
#define ENOMEDIUM       123     /* No medium found */
#define EMEDIUMTYPE     124     /* Wrong medium type */
#define ECANCELED       125     /* Operation Canceled */
#define ENOKEY          126     /* Required key not available */
#define EKEYEXPIRED     127     /* Key has expired */
#define EKEYREVOKED     128     /* Key has been revoked */
#define EKEYREJECTED    129     /* Key was rejected by service */
/* for robust mutexes */
#define EOWNERDEAD      130     /* Owner died */
#define ENOTRECOVERABLE 131     /* State not recoverable */
#define ERFKILL         132     /* Operation not possible due to RF-kill */
#define EHWPOISON       133     /* Memory page has hardware error */

#endif
// EOF
