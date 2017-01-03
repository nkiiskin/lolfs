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
 *   $Id: lolfs.h, v0.20 2016/12/04 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
 *
 */

#ifndef _LOLFS_H
#define _LOLFS_H  1
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#ifndef _LOL_FILE_H
#include <lol_file.h>
#endif

//
// lolfs (LOL stands for "Little Object List") API:
// First create a lol container
// using the "mkfs.lolfs" app, "lol fs" command or C API-
// function lol_mkfs.
//
// After that, these functions can be used to create
// and manipulate files inside that container.
//
// All of these API-functions should function just like 
// their standard C counterparts - just the "lol_" prefix
// to distinguish them and tell that we are not accessing
// files in the actual filesystem but files inside
// a container file.
//
// Remember also, that if you "lol_fopen()" a file inside
// the container or need to refer a file by name by any
// other means, using a path and filename inside a container,
// separate the actual path in the host machine from the
// file inside the container by a ":".
//
// Example in C-language:
//
// First create container:
// shell> lol fs -s 10M ~/lol.db
// ...
// const char lolfile[] = "~/lol.db:/myfile.txt";
// const char text[] = "Hello World!\n";
// int main()
// {
//   lol_FILE *fp;
//   fp = lol_fopen(lolfile, "w");
//   if (!(fp)) {
//       printf("cannot open the file %s\n", lolfile);
//       return -1;
//   }
//   lol_fwrite((char *)text, strlen(text), 1, fp);
//   lol_fclose(fp);
//   return 0;
// }
//
// shell> lol cat db:/myfile.txt
// >      Hello World!
//
//  Above, the "~/lol.db" is your normal filesystem
//  path to the container file - called "lol.db" in
//  this example. After the ":/" comes the actual file
//  name INSIDE the container, which in this case
//  is "myfile.txt".
//
// To use this API correctly, one should first create
// at least one container file, either using the
// 'mkfs.lolfs' app, the 'lol fs' command, or the
// interface function 'lol_mkfs'.
//


extern int lol_errno;

lol_FILE* lol_fopen    (const char *path, const char *mode);
int       lol_fclose   (lol_FILE *);
size_t    lol_fread    (void *ptr, size_t size, size_t nmemb, lol_FILE *);
size_t    lol_fwrite   (const void *ptr, size_t size, size_t nmemb, lol_FILE*);
int       lol_fseek    (lol_FILE *, long offset, int whence);
int       lol_feof     (lol_FILE *);
void      lol_clearerr (lol_FILE *);
int       lol_ferror   (lol_FILE *);
long      lol_ftell    (lol_FILE *);
int       lol_unlink   (const char *name);
int       lol_stat     (const char *path, struct stat *);

/*
 * NEXT FUNCTIONS (just two now) ARE PART OF LOLFS API TOO
 * These are lolfs  -specific helper-functions which may or
 * may not have standard C counterpart.
 *
 * lol_mkfs() creates a new lolfs container file
 * There are two distinct ways to call this function:
 *
 * 1: Define the desired container size directly (opt = "-s")
 *     lol_mkfs("-s", "100M", 0, 0, "lol.db");
 *
 * 2: Define the data block size and number of them (opt = "-b")
 *     lol_mkfs("-b", NULL, 128, 50000, "lol.db");
 *
 * params:
 *         opt    = either "-s" or "-b"
 *         amount = size of the container: Ex. "500k", "350M",...
 *                  (has effect only if opt "-s" is selected).
 *         bs     = block size in bytes    (must be > 0)
 *         nb     = number of these blocks (must be > 0)
 *                  (bs and nb have effect only if opt = "-b").
 *         path   = name of the container-file to be created.
 *
 * Return value:
 *               < 0: error
 *               = 0: Success
 *
 * Example using "-b" option in C -pseudo code:
 * if ((lol_mkfs("-b", NULL, 64, 10000, "~/test.db") < 0)) {
 *      ..do some error handling here..
 * }
 * else {
 *        ..the container is ready to be used...
 * }
 *
 * Example using "-s" option in C -pseudo code:
 * if ((lol_mkfs("-s", "10G", 0, 0, "~/test.db") < 0)) {
 *      ..do some error handling here..
 * } ...
 *
 */

int lol_mkfs (const char *opt, const char *amount,
              const DWORD bs, const DWORD nb, const char *path);

// lol_exit tries to do some cleanup before exiting
void lol_exit(int status);
