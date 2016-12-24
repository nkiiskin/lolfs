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
  $Id: lol_file.h, v0.13 2016/12/23 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $
 */


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

// My SPARC box has sizeof(time_t) = 4, if your system
// has 32 bit time type, you should define this.
#ifdef __32BIT_TIME_T__
  int unused;
#endif
  alloc_entry i_idx;
  ULONG  file_size;

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
  ULONG vdisk_size;
  FILE  *vdisk;

  struct lol_open_mode open_mode;
  alloc_entry nentry_index;
  // TODO Make curr_pos ULONG
  // Because file_size is ULONG too !!
  DWORD curr_pos;
  BOOL  opened;
  WORD  eof;
  int   err;

};

typedef struct _lol_FILE lol_FILE;
