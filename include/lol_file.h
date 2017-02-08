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
  $Id: lol_file.h, v0.30 2016/12/23 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $
 */
#ifndef _LOL_FILE_H
#define _LOL_FILE_H    1
#endif
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_SYS_STAT_H
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#endif
#ifndef _LOL_CONFIG_H
#include <lol_config.h>
#endif
//#include "../config.h"
// Some common types
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  UCHAR;
typedef          int   BOOL;
typedef unsigned long  ULONG;

// lol index (must be SIGNED integer)
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

  DWORD bs;
  DWORD nb;
  DWORD nf;

  /* TODO: Add reserved blocks field!! */
#if 0
  ULONG res_blocks;
#endif

  UCHAR reserved[4];
} lol_meta, *lol_super_ptr;

// Directory entry
// These come after the "lol_super"
// struct in the container file.
typedef struct lol_name_entry {

  UCHAR  name[LOL_FILENAME_MAX];
  time_t created;

// My SPARC box has sizeof(time_t) = 4, if your system
// has 32 bit time type, you should define this.
#ifdef __32BIT_TIME_T__
  // Double time :)
  time_t unused;
#endif
  alloc_entry i_idx; // 1st index location
  ULONG  fs; // file size TODO: Make this type off_t

} lol_nentry, *lol_name_entry_ptr;

struct lol_open_mode
{
  int n;
  char m[4];
};

// TODO: The filename is in two places:
//     - In vdisk_file
//     - In nentry.name
// Maybe the other should be removed
typedef struct _lol_FILE
{
  // Everything is
  // private:

  char file[LOL_FILENAME_MAX];
  char cont[LOL_DEVICE_MAX];
  struct lol_super sb;
  struct lol_name_entry nentry;
  ULONG cs; // 'outer' size of the container
  FILE  *dp;
  //struct lol_open_mode open_mode;
  struct stat cinfo;

  long data_s; // data area size;

  long idxs; // offset where index storage begins
  long idxs_s; // index storage size;

  long dir; // offset where the directory begins
  long dir_s; // directory size;

  long n_off; // abs position of the name entry
              // from the beginning of container
  // TODO Make curr_pos ULONG
  // Because file_size is ULONG too !!
  DWORD curr_pos;
  int p_len; // length of the lol_fopen path
  int f_len; // Length of the filename
  alloc_entry n_idx; // name entry index
  int   opm; // Open mode (how the file was opened)
  BOOL  opened;
  WORD  eof;
  int   err;

} lol_FILE;
