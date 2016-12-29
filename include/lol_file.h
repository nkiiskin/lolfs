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
  $Id: lol_file.h, v0.20 2016/12/23 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $
 */
#ifndef _LOL_FILE_H
#define _LOL_FILE_H    1
#endif
#ifndef _LOL_CONFIG_H
#include <lol_config.h>
#endif
#include "../config.h"
// Some common types
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  UCHAR;
typedef          int   BOOL;
typedef unsigned long  ULONG;

// lol index (must be signed integer)
#if defined SIZEOF_SHORT && defined SIZEOF_INT && defined SIZEOF_LONG
#if SIZEOF_SHORT == 4
typedef short int alloc_entry;
#elif SIZEOF_INT == 4
typedef int alloc_entry;
#elif SIZEOF_LONG == 4
typedef long alloc_entry;
#else
typedef int alloc_entry;
#endif
#else
typedef int alloc_entry;
#endif

// lol container header
typedef struct lol_super {
  // private:

  DWORD block_size;
  DWORD num_blocks;
  DWORD num_files;

  /* TODO: Add reserved blocks field!! */
#if 0
  ULONG res_blocks;
#endif

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
  // Double time :)
  time_t unused;
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

typedef struct lol_indref_t
{
  int         num;
  alloc_entry   i;
  alloc_entry   j;
  alloc_entry val;

} lol_indref;
