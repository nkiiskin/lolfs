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
 *   $Id: lolfs.h, v0.12 2016/10/30 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"
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
// lolfs API:
// First create a lol container
// using the "mkfs.lolfs" app or lol_mkfs() function.
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
// other means by using a path and filename inside a container,
// please separate the actual path in the host machine
// from the file inside the container by a ":".
// (And don't worry if your app does not do it correctly,
//  lol-API functions will return a false value just like
//  a non-lol function would).
//
// Example in C-language:
//
// lol_FILE *fp;
// fp = lol_fopen("/path/to/db:/myfile.txt", "r+"); // <- See the ":"
// (Above, the "/path/to/db" is your normal filesystem
//  path to the container file - called "db" in this
//  example. After the ":/" comes the actual file name INSIDE
//  the container, which in this case is "myfile.txt".
//
// To use this API correctly, one should first create
// at least one container file, either using the
// "mkfs.lolfs" app or the interface function "lol_mkfs()".
//
// (Sorry if I repeat myself but I feel that it might be
//  usefull to anybody who can make use of this s*it :)
//

extern int lol_errno;

lol_FILE* lol_fopen    (const char *path, const char *mode);
int       lol_fclose   (lol_FILE *);
size_t    lol_fread    (void *ptr, size_t size, size_t nmemb, lol_FILE *);
size_t    lol_fwrite   (const void *ptr, size_t size, size_t nmemb, lol_FILE *);
int       lol_feof     (lol_FILE *);
int       lol_fseek    (lol_FILE *, long offset, int whence);
void      lol_clearerr (lol_FILE *);
int       lol_ferror   (lol_FILE *);
long      lol_ftell    (lol_FILE *);
int       lol_unlink   (const char *name);
int       lol_stat     (const char *path, struct stat *);

//
// THESE NEXT FUNCTIONS (just one now) ARE PART OF LOLFS API TOO
// These are lolfs  -specific helper-functions which may or
// may not have comparable standard C counterpart.
//
// lol_mkfs() creates a new lolfs container file
//
// params: bs   = block size in bytes    (must be > 0)
//         nb   = number of these blocks (must be > 0)
//         path = name of the container-file to be created
//
// Return value:
//               < 0: error
//               = 0: Success
//
// Example in C -pseudo code:
// if ((lol_mkfs(64, 10000, "/usr/local/test.db") < 0)) {
//      ..do some error handling here..
// }
// else {
//        ..the container is ready to be used...
// }
//

int lol_mkfs (const DWORD bs, const DWORD nb, const char *path);


