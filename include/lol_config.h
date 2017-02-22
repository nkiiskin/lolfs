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
 $Id: lol_config.h, v0.40 2016/12/26 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $
 */

#ifndef _LOL_CONFIG_H
#define _LOL_CONFIG_H  1
#endif
// Compile- and runtime time parameters that
// affect performance.


// Set LOL_TESTING to non-zero if you want to
// compile a debug version of lolfs
#ifndef LOL_TESTING
#define LOL_TESTING 0
#endif
// Define LOL_INLINE_MEMCPY if you want to use inline
// routines instead of C library calls in some routines
// (Uncomment the line if you want to use function calls)
// This definition affects use of the following functions:
// memcpy, memcmp, memset, strcmp, strcpy, strcat and maybe
// some others too. The idea is that it is not effective to
// call these functions if the associated memory is not
// big (eg. ~just a few bytes). In these cases a normal
// for-loop is already done while a function call still
// creates a stack frame.
#define LOL_INLINE_MEMCPY
// Set the current version of lolfs. This is actually set
// by configure script but we hardcode it here just in case.
#ifndef LOLFS_VERSION
#define LOLFS_VERSION ("0.40")
#endif
// If we create a container by specifying the size instead of
// number of blocks and block size, then this value will be used
// as the default block size by functions like lol_mkfs and the
// 'mkfs.lolfs' program and 'lol fs' command.
#define LOL_DEFAULT_BLOCKSIZE  1024
// Specify how much memory do we reserve from
// stack by default when allocating various data buffers
#define LOL_DEFBUF 4096
// Storage size here is the limit, after which we will
// allocate memory dynamically.
// Bigger LOL_STORAGE_SIZE may result in better performance.
// This number + 1, must be dividible by LOL_DEFAULT_BLOCK_SIZE
// Leave it as it is if you are not sure!
#define LOL_STORAGE_SIZE 32767
// Define the upper limit of how much memory
// shall we try to allocate for indexes, etc..
#ifndef LOL_02GIGABYES
#define LOL_02GIGABYES (2147483648)
#endif
#define LOL_INDEX_MALLOC_MAX LOL_02GIGABYTES
// Smallest possible container file. This number is highly
// version and architecture dependent constant; let it be
// as is if you are not sure. Easy way to figure this number
// out is by creating a container: 'lol fs -b 1 1 foo' and
// then check the size of 'foo': ls -l foo
#define LOL_THEOR_MIN_DISKSIZE 77
// Filename max length in lolfs container. Modern filesystems
// allow at least 256 characters for filenames but we are not
// so modern yet. Functions like 'lol cp' will truncate longer
// filenames to fit. Note, the actual maximum length is one
// less, since we need one byte for zero in the end of the
// filename to terminate it.
#define LOL_FILENAME_MAX 32
// Max path accepted by lol_fopen, lol_stat etc..
#define LOL_DEVICE_MAX 256
// Define copy string as shown by programs.
// This must not be edited!
#define LOLFS_COPYRIGHT ("Copyright (C) 2016, Niko Kiiskinen")
