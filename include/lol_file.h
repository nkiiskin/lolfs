#ifndef _LOL_FILE_H
#define _LOL_FILE_H    1
#endif

#define LOLFS_VERSION      ("0.13")
#define LOLFS_COPYRIGHT    ("Copyright (C) 2016, Niko Kiiskinen")
#define LOLFS_INTERNAL_ERR ("Internal error. Sorry!")
static const char     lol_version[] = LOLFS_VERSION;
static const char   lol_copyright[] = LOLFS_COPYRIGHT;
static const char   lol_inter_err[] = LOLFS_INTERNAL_ERR;


#define LOL_FILENAME_MAX 32
#define LOL_DEVICE_MAX   256
#define LOL_PATH_MAX (LOL_DEVICE_MAX + LOL_FILENAME_MAX)

typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  UCHAR;
typedef          int   BOOL;
typedef unsigned long  ULONG;
// Our index (must be signed integer)
typedef int alloc_entry;

// Our file header
typedef struct lol_super {
  // private:
  DWORD block_size;
  DWORD num_blocks;
  DWORD num_files;
  UCHAR reserved[4];

} *lol_super_ptr;

// Directory entry
// These come after the "lol_super"
// struct in the container file.
typedef struct lol_name_entry {

  UCHAR  filename[LOL_FILENAME_MAX];
  time_t created;

// My SPARC (Sun Blade 150) box has sizeof(time_t) = 4 !!
// Must have this unused field here until I find out a better solution
// (SPARC is also BIG ENDIAN system, which makes lolfs created
//  in LITTLE ENDIAN systems incompatible there - will fix later, hmm..

#ifdef __SPARC__
  int unused;
#endif
  alloc_entry i_idx;
  // TODO: DWORD -> ULONG
  DWORD  file_size;

} *lol_name_entry_ptr;

struct lol_open_mode
{
  // private:
  mode_t device;
  int    mode_num;
  char   mode_str[6];
  char    vd_mode[4];
};
// TODO: The filename is in two places:
//     - In vdisk_file
//     - In nentry.filename
// Maybe the other should be removed
struct _lol_FILE
{

  // Everything is
  // private:

  char vdisk_file[LOL_FILENAME_MAX];
  char vdisk_name[LOL_DEVICE_MAX];

  struct lol_super sb;
  struct lol_name_entry nentry;
  // TODO: make vdisk_size ULONG
  DWORD vdisk_size;
  FILE  *vdisk;

  struct lol_open_mode open_mode;
  alloc_entry nentry_index;

  DWORD curr_pos;
  BOOL  opened;
  WORD  eof;
  int   err;

};

typedef struct _lol_FILE lol_FILE;
