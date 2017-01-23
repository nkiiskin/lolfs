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
 $Id: lol_config.h, v0.30 2016/12/26 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $
 */

#ifndef _LOL_CONFIG_H
#define _LOL_CONFIG_H  1
#endif


// Compile- and runtime time parameters
// that affect performance.


// Set LOL_TESTING to 1 if you want to
// compile a debug version of lolfs
#define LOL_TESTING 0

// Define LOL_INLINE_MEMCPY
// if you want to test inline
// functions instead of C library
// calls in some routines
#define LOL_INLINE_MEMCPY 1

// Set the current version of lolfs
// This is usually set by configure but
// we hardcode it here.
#define LOLFS_VERSION ("0.30")
// If we create a container by specifying the
// size instead of number of blocks and block size,
// then this value will be used as the default
// block size by functions like lol_mkfs.
#define LOL_DEFAULT_BLOCKSIZE  1024
// Specify how much memory do we reserve from
// stack when allocating various data buffers
#define LOL_DEFBUF 4096
// Storage size here is the limit, after which we will
// allocate memory dynamically.
// Bigger LOL_STORAGE_SIZE may result in better performance.
// This number + 1, must be dividible by LOL_DEFAULT_BLOCK_SIZE
// Leave it as it is if you are not sure!
#define LOL_STORAGE_SIZE 16383
// Define the upper limit of how much memory
// shall we try to allocate for indexes, etc..
#ifndef LOL_02GIGABYES
#define LOL_02GIGABYES (2147483648)
#endif
#define LOL_INDEX_MALLOC_MAX LOL_02GIGABYTES
// Smallest possible container file.
// This number is highly version and
// architecture dependent constant,
// so let it be if you are not sure.
// One way to figure this out is by creating a
// container: 'lol fs -b 1 1 foo' and then
// check the size of 'foo': ls -l foo
#define LOL_THEOR_MIN_DISKSIZE 77
// Filename max length in lolfs container
// Modern filesystems allow at least 256
// characters for filenames but we are not
// modern :) Functions like 'lol cp' will
// truncate longer filenames to fit.
#define LOL_FILENAME_MAX 32
// Max path accepted by lol_fopen, lol_stat etc..
#define LOL_DEVICE_MAX 256
// Define copy string as shown by programs.
// This must not be edited!
#define LOLFS_COPYRIGHT ("Copyright (C) 2016, Niko Kiiskinen")
