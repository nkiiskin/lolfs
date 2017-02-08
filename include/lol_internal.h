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
 */
/*
$Id: lol_internal.h, v0.30 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp$
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
#ifndef HAVE_CONFIG_H
#include "../config.h"
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
#ifndef _LOL_CONFIG_H
#include <lol_config.h>
#endif
#define DISK_HEADER_SIZE (sizeof(struct lol_super))
#define NAME_ENTRY_SIZE  (sizeof(struct lol_name_entry))
#define LOL_FILE_SIZE    (sizeof(struct _lol_FILE))
#define ENTRY_SIZE       (sizeof(alloc_entry))
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LOL_ENDIAN 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define LOL_ENDIAN 0
#else
#error "can not figure out machine byte order!"
#endif
// LOL_MAGIC number is actually 0x7b7b,
// so we only need one definition
#define LOL_MAGIC     (0x7b)
// major and minor for struct stat st_dev field
#define LOL_DEV_MAJOR  (8)
#define LOL_DEV_MINOR  (2)
// struct stat st_rdev field
#define LOL_FILE_RDEV (0)
// lol file default mode (octal 100666)
// (Regular file with permissions rw-rw-rw-)
// (S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP |
//  S_IWGRP | S_IROTH | S_IWOTH)
#define LOL_FILE_DEFAULT_MODE 33206

#define LOL_FALSE     (0xFEFFFFDF)
#define LOL_PATH_MAX  (LOL_DEVICE_MAX + LOL_FILENAME_MAX + 2)
#define LOL_PATH_BUF  (LOL_PATH_MAX + 1)
#define LOL_FILENAME_MAXLEN (LOL_FILENAME_MAX - 1)
// Some structs ////////////////////////////////////////////////


// Constants (or'd together) and
// structure for lol_pathinfo
#define LOL_PATHLEN  1
#define LOL_FILELEN  2
#define LOL_FILENAME 4
#define LOL_CONTPATH 8
#define LOL_CONTNAME 16

// The next 2 constants
// are used internally
#define LOL_SIMULATE LOL_FILELEN
#define LOL_PATHMAGIC 0x510b47d9

// This struct is being used by
// function lol_pathinfo
typedef struct lol_path_info
{
  char  *fullp;
  char   *file;
  char   *cont;
  int     flen;
  int     clen;
  int     plen;
  int     func;

} lol_pinfo;

typedef struct lol_nentryinfo_t
{
  long off; // Where the name entry begins
            // counted from the beginning of
            // the container. This is for
            // fast fseek() to the entry.

  long idx;        // The index of the entry
                   // in the name entry storage.
                   // (Some functions prefer this info).

  lol_nentry *ne;  // Pointer to a name entry struct
                   // (may be NULL if not needed)

  short int res;   // Boolean value. Is the entry
                   // Reserved (res != 0) or free (res = 0)
} lol_ninfo, *lol_ninfo_ptr;

typedef struct lol_indref_t
{
  int         num;
  alloc_entry   i;
  alloc_entry   j;
  alloc_entry val;

} lol_indref;

typedef struct lol_size_t {
    float s;
    char  n[6];
} lol_size;

struct lol_loop {
  // private:
   size_t  num_blocks;
   size_t start_bytes;
   size_t  full_loops;
   size_t   end_bytes;
};

#define lol_error(x, ...) fprintf(stderr, x, ##__VA_ARGS__)
// Indexes to global error message lists /////////////////
extern const char *lol_msg_list0[];
// indexes to global error message list
#define LOL_0E_USEFSCK  0
#define LOL_0E_INTERERR 1

// And corresponging messages with 0 variables
#define LOL_FSCK_FMT               ("        fsck.lolfs recommended\n")
#define LOL_INTERERR_FMT           ("Internal error. Sorry!\n")

#define E_DISK_FULL                ("Not enough space in disk\n")
#define LOL_DISKFULL_FMT           E_DISK_FULL
#define LOLFS_INTERNAL_ERR         LOL_INTERERR_FMT
#define E_OUT_MEM                  ("Out of memory!\n")
#define E_DISK_IO                  ("I/O error\n")
#define lol_errfmt(x)              fprintf(stderr, "%s", lol_msg_list0[(x)])

// indexes to global error message list
extern const char *lol_msg_list1[];
#define LOL_1E_OOM    0
#define LOL_1E_IOERR  1
#define LOL_1E_SYNTAX 2
#define LOL_1E_IMAGIC 3
#define LOL_1E_HELP   4
#define LOL_1E_HELP2  5
#define LOL_1E_INTER  6

// The actual messages
// Messages with 1 variable
#define LOL_OOM_FMT         ("lol %s: out of memory.\n")
#define LOL_IOERR_FMT       ("lol %s: I/O error\n")
#define LOL_SYNTAX_FMT      ("lol %s: syntax error\n")
#define LOL_IMAGIC_FMT      ("lol %s: corrupted file id [0x%x, 0x%x].\n")
#define LOL_HELP_FMT        ("       Type: 'lol %s -h' for help.\n")
#define LOL_HELP2_FMT       ("       Type: '%s -h' for help.\n")
#define LOL_INTERE_FMT      ("lol %s: internal error\n")
#define lol_mprintf1(x,y)   printf(lol_msg_list1[(x)], (y))
#define lol_errfmt1(x,y)    fprintf(stderr, lol_msg_list1[(x)], (y))
#define lol_helpf(x)        printf(lol_msg_list1[LOL_1E_HELP], (x))
#define lol_ehelpf(x)       fprintf(stderr, lol_msg_list1[LOL_1E_HELP], (x))
#define lol_helpf2(x)       printf(lol_msg_list1[LOL_1E_HELP2], (x))
#define lol_ehelpf2(x)      fprintf(stderr, lol_msg_list1[LOL_1E_HELP2], (x))
#define lol_syntax(x)       fprintf(stderr, lol_msg_list1[LOL_1E_SYNTAX], (x))

// indexes to global error message list
extern const char *lol_msg_list2[];
#define LOL_2E_CANTREAD  0
#define LOL_2E_CANTWRITE 1
#define LOL_2E_CANTUSE   2
#define LOL_2E_CANTCOPY  3
#define LOL_2E_DIRMISS   4
#define LOL_2E_NOSUCHF   5
#define LOL_2E_OPTION    6
#define LOL_2E_ARGMISS   7
#define LOL_2E_CORRCONT  8
#define LOL_2E_NOTROOM   9
#define LOL_2E_OW_PMT   10
#define LOL_2E_FULLCONT 11
#define LOL_2E_FIOERR   12
#define LOL_2E_ACDENIED 13
#define LOL_2E_INVSRC   14
#define LOL_2E_OWCONT   15
#define LOL_2E_INVFS    16
#define LOL_2E_INVFUNC  17
#define LOL_2E_ARGMISS2 18
#define LOL_2E_OPTION2  19

// The actual messages in that list are:
// Messages with 2 variables
#define LOL_EREAD_FMT  ("lol %s: cannot read from \'%s\'\n")
#define LOL_EWRITE_FMT ("lol %s: cannot write to \'%s\'\n")
#define LOL_EUSE_FMT   ("lol %s: cannot use \'%s\'\n")
#define LOL_ECP_FMT    ("lol %s: cannot copy to file \'%s\'\n")
#define LOL_ENODIR_FMT ("lol %s: cannot find directory \'%s\'\n")
#define LOL_ENOENT_FMT ("lol %s: \'%s\': No such file or directory\n")
#define LOL_EOPT_FMT   ("lol %s: unrecognized option \'%s\'\n")
#define LOL_EARG_FMT   ("lol %s: missing argument(s): \'%s\'\n")
#define LOL_ECONT_FMT  ("lol %s: container \'%s\' has errors\n")
#define LOL_NOSPC_FMT  ("lol %s: not enough room for \'%s\'\n")
#define LOL_OWPMT_FMT  ("lol %s: \'%s\' exists. Replace [y/n]? ")
#define LOL_EFULL_FMT  ("lol %s: container \'%s\' is full\n")
#define LOL_EIO_FMT    ("lol %s: I/O error \'%s\'\n")
#define LOL_EACCES_FMT ("lol %s: cannot copy to file \'%s\', access denied\n")
#define LOL_INVSRC_FMT ("lol %s: the file \'%s\' is not inside a container\n")
#define LOL_OWCONT_FMT ("lol %s: will not overwrite container \'%s\'\n")
#define LOL_EFBIG_FMT  ("lol %s: invalid filesize \'%s\'\n")
#define LOL_EFUNC_FMT  ("%s: unrecognized function \'%s\'\n")
#define LOL_EARG2_FMT  ("%s: missing argument(s): \'%s\'\n")
#define LOL_EOPT2_FMT  ("%s: unrecognized option \'%s\'\n")

#define lol_errfmt2(x, ...) fprintf(stderr, lol_msg_list2[(x)], ##__VA_ARGS__)
#define lol_inffmt2(x, ...) printf(lol_msg_list2[(x)], ##__VA_ARGS__)

// Messages with 3 variables
// indexes to global error message list
extern const char *lol_msg_list3[];
#define LOL_3E_VER    0
#define LOL_3E_VER2   1
// Messages with 3 variables
#define LOL_VERS_FMT   ("lol %s v%s %s\n")
#define LOL_VERS2_FMT  ("%s v%s %s\n")

#define lol_show_version(x)        printf(lol_msg_list3[LOL_3E_VER], (x), \
                                   lol_version, lol_copyright)
#define lol_show_version2(x)       printf(lol_msg_list3[LOL_3E_VER2], (x), \
                                   lol_version, lol_copyright)
#define lol_errfmt3(x, ...) fprintf(stderr, lol_msg_list3[(x)], ##__VA_ARGS__)
#define lol_inffmt3(x, ...) printf(lol_msg_list3[(x)], ##__VA_ARGS__)

// Messages with 5 variables
// indexes to global error message list
extern const char *lol_msg_list5[];
#define LOL_5E_USAGE   0
#define LOL_5E_USAGE2  1

#define LOL_USAGE_FMT  ("lol %s v%s. %s\nUsage: lol %s %s\n")
#define LOL_USAGE2_FMT ("%s v%s. %s\nUsage: %s %s\n")
#define lol_show_usage(x)        printf(lol_msg_list5[LOL_5E_USAGE], (x), \
                                 lol_version, lol_copyright, (x), params)
#define lol_show_usage2(x)       printf(lol_msg_list5[LOL_5E_USAGE2], (x), \
                                 lol_version, lol_copyright, (x), params)
#define lol_show_usage_err(x)    fprintf(stderr,lol_msg_list5[LOL_5E_USAGE], \
                                 (x), lol_version, lol_copyright, (x), params)


// Some macros for common expressions/tasks
#define LOL_MEMCPY(x,y,z)          memcpy((char *)(x), \
                                   (const char *)(y), ((size_t)(z)))
#define LOL_MEMSET(x,y,z)          memset((char *)(x),(int)(y),((size_t)(z)))
#define LOL_DEVSIZE(x,y)           (DISK_HEADER_SIZE + ((ULONG)(x)) * \
                                   (((ULONG)(y)) + ENTRY_SIZE + \
                                   NAME_ENTRY_SIZE))
#define LOL_CONTSIZE(x)            lol_validcont((x)->cont, &(x)->sb, \
                                   &(x)->cinfo)

// x = file size
// y = block size
// z = number of blocks (the answer)
#define LOL_FILE_BLOCKS(x,y,z)     if ((x)) { (z) = ((x) / (y)); \
                                   if ((x) % (y)) (z)++; } \
                                   else {(z) = 1;}
/*
x = entries
y = bytes
z = e_size
k = frac */
#define LOL_CALC_ENTRIES(x,y,z,k)  (x) = (y) / (z); (k) = (y) % (z); \
                                   if ((k)) { (x)++; }
#define LOL_INDEX_OFFSET(x,y,z)    (DISK_HEADER_SIZE + (x) * \
                                   (NAME_ENTRY_SIZE + (y)) + (z) * ENTRY_SIZE)
#define LOL_DATA_OFFSET(x,y)       (DISK_HEADER_SIZE + (x) * (y))
#define LOL_DENTRY_OFFSET(x)       (DISK_HEADER_SIZE + ((x)->sb.nb) * \
                                   ((x)->sb.bs))
#define LOL_DENTRY_OFFSET_EXT(x,y) ((long)(DISK_HEADER_SIZE) + \
                                   (long)(x) * (long)(y))

/*
#define LOL_GOTO_NENTRY(x,y,z,w)   (fseek((x), DISK_HEADER_SIZE + (y) * (z) + \
                                   (w) * NAME_ENTRY_SIZE, SEEK_SET))



  op->p_len = p.plen; // length of the lol_fopen path
  op->f_len = p.flen; // Length of the filename
  op->n_off; // abs offset of the name entry from beginning of cont

*/

#define LOL_GOTO_DENTRY(x)         (fseek((x)->dp, DISK_HEADER_SIZE + \
                                   (x)->n_idx * NAME_ENTRY_SIZE + \
                                   (x)->sb.bs * \
                                   (x)->sb.nb, SEEK_SET))


// #define LOL_GOTO_DENTRY(x)         (fseek((x)->dp, (x)->n_off, SEEK_SET))
#define LOL_TABLE_START_EXT(x,y)   (DISK_HEADER_SIZE + (x) * \
                                   (NAME_ENTRY_SIZE + (y)))
#define LOL_TABLE_START(x)         (DISK_HEADER_SIZE + ((x)->sb.nb) * \
                                   (((x)->sb.bs) + NAME_ENTRY_SIZE))
#define LOL_CHECK_MAGIC(x)         ((x)->sb.reserved[0] != LOL_MAGIC || \
                                   (x)->sb.reserved[1]  != LOL_MAGIC)
#define LOL_ISDIR                  (S_ISDIR(st.st_mode))
#define LOL_ISREG                  (S_ISREG(st.st_mode))
#define LOL_ISDIRP                 (S_ISDIR(st->st_mode))
#define LOL_ISREGP                 (S_ISREG(st->st_mode))
#define LOL_NOTDIR                 (!(S_ISDIR(st.st_mode)))
#define LOL_NOTREG                 (!(S_ISREG(st.st_mode)))
#define LOL_ERRET(x,y)             { lol_errno = (x); return (y); }
#define LOL_ERR_RETURN(x,y)        { op->err = lol_errno = (x); return (y); }
#define LOL_ERRSET(x)              { op->err = lol_errno = (x); }
#define LOL_ERRNO(x)               if (errno) {lol_errno = errno;} \
				   else {lol_errno = (x);}
// TODO: replace argv[1] from the following macros with argv[(x)]
#define LOL_CHECK_HELP             ((!(strcmp(argv[1], "-h"))) || \
                                   (!(strcmp(argv[1], "--help"))))
#define LOL_CHECK_VERSION          ((!(strcmp(argv[1], "-v"))) || \
                                   (!(strcmp(argv[1], "--version"))))
#define LOL_CHECK_SILENT           ((!(strcmp((argv[1]), "-s"))) || \
  	                           (!(strcmp((argv[1]), "--silent"))))
#define LOL_CHECK_DETAILS          ((!(strcmp((argv[1]), "-d"))) || \
  	                           (!(strcmp((argv[1]), "--details"))))
#define LOL_CHECK_BLOCKS           ((!(strcmp(argv[1], "-b"))) || \
                                   (!(strcmp(argv[1], "--blocks"))))
#define LOL_CHECK_SIZE             ((!(strcmp(argv[1], "-s"))) || \
                                   (!(strcmp(argv[1], "--size"))))
#define LOL_CHECK_OPTS(x,y)        ((!(strcmp((argv[1]), (x)))) || \
				    (!(strcmp((argv[1]), (y)))))
#define LOL_INVALID_MAGIC          ((sb.reserved[0]  != LOL_MAGIC) || \
				   (sb.reserved[1]   != LOL_MAGIC))
#define LOL_INVALID_MAGIC_PTR      ((sb->reserved[0] != LOL_MAGIC) || \
				   (sb->reserved[1]  != LOL_MAGIC))
#define LOL_DATA_START             (DISK_HEADER_SIZE)
#define delete_return_NULL(x,y)    { lol_errno = (x); delete_lol_FILE((y)); return NULL; }
#define close_return_NULL(x,y)     { lol_errno = (x); lol_fclose((y)); return NULL; }
#define LOL_MAG_0                  ((DWORD)sb.reserved[0])
#define LOL_MAG_1                  ((DWORD)sb.reserved[1])
#define LOL_SPACE()                putchar((int)(' '))
#define LOL_ALLOC(x)               ((x *)malloc(sizeof(x)))
#if defined __FILE__               && defined __LINE__
#define lol_debug(x)               fprintf (stderr, "%s: error file: %s, \
                                   line %d.", (x), __FILE__, __LINE__)
#else
#define lol_debug(x)               fprintf(stderr, "%s: ", (x))
#endif

// funcs for lol_fio:
#define        LOL_READ (0)
#define       LOL_WRITE (1)
#define   LOL_READ_CONT (2)
#define  LOL_WRITE_CONT (3)

// Some internal use flags
#define LOL_NO_SUCH_FILE (0)
#define LOL_FILE_EXISTS  (1)

// Private constants, not to used
//  in the interface!
#define LAST_LOL_INDEX  (-1)
#define FREE_LOL_INDEX  (-2)
// Constants for lol_check_corr
#define LOL_CHECK_SB     (1)
#define LOL_CHECK_FILE   (2)
#define LOL_CHECK_BOTH   (3)
// Constants for lol_rs
#define LOL_RS_BLOCKS    (1)
#define LOL_RS_SIZE      (2)
// Private error constants, not to be used also
#define LOL_OK           (0)
#define LOL_ERR_EOF      (0)
#define LOL_ERR_GENERR  (-1)
#define LOL_ERR LOL_ERR_GENERR
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
#define LOL_ERR_ENOENT  (-15)

#define MAX_LOL_OPEN_MODES    (6)
// Constants for lol_getsize
#define RECUIRE_SB_INFO       (1)
#define USE_SB_INFO           (2)
// Constants for lol_supermod func
#define LOL_INCREASE          (0)
#define LOL_DECREASE          (1)
// Constants for lol_free_space func
#define LOL_SPACE_BYTES       (1)
#define LOL_SPACE_BLOCKS      (2)
// Constants for lol_size_to_blocks func
#define LOL_EXISTING_FILE    (17)
#define LOL_JUST_CALCULATE   (71)
// Flags to lol_delete_chain_from:
#define LOL_UNLINK            (0)
#define LOL_SAVE_FIRST_BLOCK  (1)

// Flags for lol_get_free_index
#define LOL_MARK_USED         (1)

#define LOL_FS_TOOSMALL      (-2)
// Some integers
#define LOL_BYTEBYTE    (1)
#define LOL_KILOBYTE    (1024)
#define LOL_04KILOBYTES (4096)
#define LOL_08KILOBYTES (8192)
#define LOL_MEGABYTE    (1048576)
#define LOL_GIGABYTE    (1073741824)
#define LOL_02GIGABYTES (2147483648)
#define LOL_04GIGABYTES (4294967296)
#define LOL_08GIGABYTES (8589934592)
#define LOL_16GIGABYTES (17179869184)
#define LOL_TERABYTE    (1099511627780)

// Signal handlers (Do NOT change these!)
// These are not real signal numbers!
#define LOL_SIGHUP  0
#define LOL_SIGINT  1
#define LOL_SIGTERM 2
#define LOL_SIGSEGV 3
#define LOL_NUM_SIGHANDLERS 4

// i/o for some funcs
#define LOL_STDOUT 0
#define LOL_STDIN  1
#define LOL_STDERR 2

// types for lol_status_msg
#define LOL_FSCK_OK      0
#define LOL_FSCK_INFO   (1)
#define LOL_FSCK_WARN   (2)
#define LOL_FSCK_ERROR  (3)
#define LOL_FSCK_FATAL  (4)
#define LOL_FSCK_INTRN  (5)

extern const char* lol_prefix_list[];
extern const char*    lol_tag_list[];
extern char lol_mode_combinations[MAX_LOL_OPEN_MODES][14][5];
extern alloc_entry* lol_index_buffer;
extern int lol_buffer_lock;
extern const struct lol_open_mode lol_open_modes[];
#define LOL_MAX_LOLSIZES 5
extern const lol_size lol_sizes[];
extern const char lol_version[];
extern const char lol_copyright[];
extern const char lol_mode_ro[];
extern const char lol_mode_rw[];

// status message alignment
// We want to fit in terminal,
// align should be max 72
#define LOL_STATUS_ALIGN 70

// lol_divs
enum
{
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
enum {

  LOL_RDONLY,           // "r"
  LOL_RDWR,             // "r+"
  LOL_WR_CREAT_TRUNC,   // "w"
  LOL_RDWR_CREAT_TRUNC, // "w+"
  LOL_APPEND_CREAT,     // "a"
  LOL_RD_APPEND_CREAT,  // "a+"

};
// Some ANSI codes
#define LOL_TEXT_BOLD      "\x1b[1m"
#define LOL_TEXT_UNDERLINE "\x1b[4m"
#define LOL_COLOR_RED      "\x1b[31m"
#define LOL_COLOR_GREEN    "\x1b[32m"
#define LOL_COLOR_YELLOW   "\x1b[33m"
#define LOL_COLOR_BLUE     "\x1b[34m"
#define LOL_COLOR_MAGENTA  "\x1b[35m"
#define LOL_COLOR_CYAN     "\x1b[36m"
#define LOL_COLOR_RESET    "\x1b[0m"

#define LOL_STORAGE_ALL (LOL_STORAGE_SIZE + 1)
typedef size_t (*lol_io_func)(void *, size_t, size_t, void *);
typedef void* (*lol_open_func)(const char *, const char *);
typedef int (*lol_close_func)(void *);

// Miscallaneous helper-functions,
// not to be used in the interface

// File functions
size_t      lol_fclear(const size_t bytes, FILE *);
size_t      lol_ifcopy(const alloc_entry val, const size_t times, FILE *s);
size_t      lol_fio(char *ptr, const size_t bytes, void *s, const int func);
// Some 'get' functions
int         lol_getsb(FILE *fp, lol_meta *sb);
int         lol_getmode(const char *m);
long        lol_get_io_size(const long size, const long blk);
int         lol_pathinfo(lol_pinfo *);
int         lol_fnametolol(const char *src, const char *cont,
                           char *out, const size_t contlen);
int         lol_get_free_index(lol_FILE *op, alloc_entry *idx, int mark);
long        lol_get_free_nentry(lol_FILE *, lol_ninfo *);
long        lol_rgetsize (const char *device, lol_meta *sb, struct stat *st);
long        lol_getsize (const char *name, lol_meta *sb,
                         struct stat *st, int func);
#ifndef     lol_fgetsize
#define     lol_fgetsize(x) lol_getsize((x)->cont, &(x)->sb, \
            &(x)->cinfo, RECUIRE_SB_INFO)
#endif
alloc_entry lol_get_index_value (FILE *f, const DWORD nb,
                                 const DWORD bs, const alloc_entry idx);
// Various 'check' functions
int         lol_can_replace (const long, const long, const long, const long);
int         lol_is_number(const char ch);
int         lol_is_integer(const char *str);
size_t      lol_validcont(const char *name, lol_meta *sb, struct stat *st);
BOOL        lol_validpath(char *path);
int         lol_is_writable(const lol_FILE *op);
int         lol_valid_sb(const lol_FILE *op);
int         lol_check_corr(const lol_FILE *op, const int mode);
// Memory funcs
void*       lol_malloc(const size_t size);
void        lol_free(const size_t size);
lol_FILE   *new_lol_FILE(void);
void        delete_lol_FILE(lol_FILE *fp);
int         lol_memset_indexbuffer(const alloc_entry val, const size_t x);
// Container functions
int         lol_supermod (FILE *fp, const lol_meta *sb, const int func);
long        lol_free_space (const char *cont, lol_meta *sb, const int mode);
int         lol_extendfs(const char *container, const DWORD new_blocks,
			 struct lol_super *sb);
// Some lol_FILE helper functions
int         lol_touch_file(lol_FILE *op);
int         lol_truncate_file(lol_FILE *op);
int         lol_update_nentry(lol_FILE *op);
int         lol_read_nentry(lol_FILE *);
long        lol_io_dblock(lol_FILE *op, const size_t block_number,
			  char *ptr, const size_t bytes, const int func);
size_t      lol_num_blocks(lol_FILE *op, const size_t amount,
                           struct lol_loop *loop);
int         lol_garbage_filename(const char *name);
int         lol_remove_nentry (lol_FILE *op, const DWORD nentry, int rm_idx);
int         lol_count_file_blocks (FILE *fp, const struct lol_super *sb,
                                   const alloc_entry first_index,
                                   const long dsize, long *count,
                                   const int terminate);

// Signal handling
int         lol_setup_sighandlers(void);
void        lol_restore_sighandlers(void);
// Conversion functions
int         lol_ltostr(const long size, char *s);
int         lol_size_to_blocks(const char *size, const char *container,
                               const struct lol_super *sb,
                               const struct stat *st, DWORD *nb, int func);
// Functions that handle indices
int         lol_index_malloc(const size_t num_entries);
void        lol_index_free (const size_t amount);
int         lol_read_ichain(lol_FILE *op, const size_t blocks);
int         lol_delete_chain_from(lol_FILE *, int);
long        lol_new_indexes(lol_FILE *op, const long bytes,
                            long *olds, long *mids, long *news,
                            long *new_filesize);
int         lol_new_ichain(lol_FILE *op, const long olds, const long news,
                           alloc_entry *last_old);
int         lol_update_ichain(lol_FILE *op, const long olds,
                              const long news, const alloc_entry last_old);
int         lol_set_index_value (lol_FILE *op, const alloc_entry new_val);
// Message output
void        lol_align(const char *before, const char *after,
                      const size_t len, int out);
int         lol_status_msg(const char *me, const char* txt, const int type);
void        lol_help(const char** lst);
// The 'lol' app internal functions
// (N_LOLFUNCS must match the number of these functions)
#define N_LOLFUNCS 8
int         lol_cat  (int, char**);
int         lol_cc   (int, char**);
int         lol_cp   (int, char**);
int         lol_df   (int, char**);
int         lol_fs   (int, char**);
int         lol_ls   (int, char**);
int         lol_rm   (int, char**);
int         lol_rs   (int, char**);

// DON'T add lol_cmd to N_LOLFUNCS
int         lol_cmd  (int, char**);

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
