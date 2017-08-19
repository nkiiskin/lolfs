/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
/*
 $Id: test1.c, v0.20 2016/12/27 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_SYS_TYPES_H
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
#endif
#ifdef HAVE_SYS_STAT_H
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
#endif
#ifdef HAVE_UNISTD_H
#ifndef _UNISTD_H
#include <unistd.h>
#endif
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#endif
#ifdef HAVE_STRING_H
#ifndef _STRING_H
#include <string.h>
#endif
#endif
#ifndef _LOLFS_H
#include "../include/lolfs.h"
#endif
#ifndef _LOL_INTERNAL_H
#include "../include/lol_internal.h"
#endif
#ifndef _LOL_TESTS_H
#include "lol_tests.h"
#endif

#if 0
#include "../include/lolfs.h"
#include "../include/lol_internal.h"
#include "../include/lol_tests.h"
#endif

char *args1[] = {
  "lol",
  "rm",
  "../../1.db:/TODO",
  "../../1.db:/AUTHORS",
  "../../1.db:/test1.c",
  NULL
};
char *args2[] = {
  "lol",
  "cp",
  "../../../configure",
  "../../1.db:/foo",
  NULL
};
char *args3[] = {
  "lol",
  "cp",
  "../../1.db:/foo",
  "../..",
  NULL
};
/* ************************************************************************ */
/* LoL testsuite: test #6 (Compare file contents)                           */
/* ************************************************************************ */

/* ************************************************************************ */
/* LoL testsuite helper functions                                           */
/* ************************************************************************ */
int lol_compare_files (const char *f1, const char *f2)
{
  char buf1[1024];
  char buf2[1024];
  struct stat st;
  long size1, size2;
  size_t frac;
  int times, i;
  FILE *fp1, *fp2;
  int ret = -1;

  if ((stat(f1, &st))) {
     printf("lol_compare_files: cannot find the file \'%s\'\n", f1); 
     return -1;
  }
  size1 = (long)st.st_size;
  if ((stat(f2, &st))) {
     printf("lol_compare_files: cannot find the file \'%s\'\n", f2); 
     return -1;
  }
  size2 = (long)st.st_size;
  if (size1 != size2) {
     printf("lol_compare_files: different file sizes\n"); 
     return 1;
  }
  times = size1 / 1024;
  frac = (size_t)(size1 % 1024);

  if (!(fp1 = fopen(f1, "r"))) {
     printf("lol_compare_files: cannot open the file \'%s\'\n", f1); 
     return -1;
  }
  if (!(fp2 = fopen(f2, "r"))) {
     printf("lol_compare_files: cannot open the file \'%s\'\n", f2);
     fclose(fp1);
     return -1;
  }
  for (i = 0; i < times; i++) {
    if ((fread((char *)&buf1, 1024, 1, fp1)) != 1)
      goto error;
    if ((fread((char *)&buf2, 1024, 1, fp2)) != 1)
      goto error;
    if ((memcmp(buf1, buf2, 1024))) {
      printf ("memcmp failed i = %d\n", i);
       ret = 1;
      goto error;
    }
  } // end for i

  if (frac) {
    if ((fread((char *)&buf1, frac, 1, fp1)) != 1)
      goto error;
    if ((fread((char *)&buf2, frac, 1, fp2)) != 1)
      goto error;
    if ((memcmp(buf1, buf2, frac))) {
      printf ("memcmp failed frac = %ld\n", (long)(frac));
      ret = 1;
      goto error;
    }
  } // end if frac

  ret = 0;
  printf("The file \'%s\' and \'%s\' are the same.\n", f1, f2);

error:
 fclose(fp1);
 fclose(fp2);
 if (ret) {
  printf("The file \'%s\' and \'%s\' are NOT the same.\n", f1, f2);
  printf("(return value is %d)\n", ret);
 }
 return ret;
} // end lol_compare_files
/* ************************************************************************ */
int main (int argc, char* argv[])
{
  const char *me = argv[0];
  static const char *f1 = "../../foo";
  static const char *f2 = "../../../configure";
  int rv, ret = 0;

  // Process standard --help & --version options.
  if (argc > 1) {
    if (LOL_CHECK_HELP) {
        printf ("%s v%s. %s\n",
                me, lol_version, lol_copyright);

    } else {
       if (LOL_CHECK_VERSION) {
         printf ("%s v%s %s\n",
	         me, lol_version, lol_copyright);
       }
    }
  }

  puts ("Bundle 6, test 1: Deleting 3 files from the container 1");
  rv = lol_cmd (5, args1);
  if (!(rv)) {
    puts ("Succes: Bundle 6, test 1: deleted 3 files from container 1");
  }
  else {
    puts("Bundle 6, test 1: failed to delete 3 files from container 1");
    ret =  -1;
  }
  puts ("Bundle 6, test 2: Copying 1 file to container 1");
  rv = lol_cmd (4, args2);
  if (!(rv)) {
    puts ("Succes: Bundle 6, test 2: copied 1 file to container 1");
  }
  else {
    puts("Bundle 6, test 2: failed to copy 1 file to container 1");
    ret =  -1;
  }
  puts ("Bundle 6, test 3: Copying 1 file from container 1");
  rv = lol_cmd (4, args3);
  if (!(rv)) {
    puts ("Succes: Bundle 6, test 3: copied 1 file from container 1");
  }
  else {
    puts("Bundle 6, test 3: failed to copy 1 file from container 1");
    ret =  -1;
  }

  puts ("Bundle 6, test 4: Comparing file from container to another in the filesystem");
  rv = lol_compare_files (f1, f2);
  switch (rv) {
      case 0 :
        puts("Bundle 6, test 4: Success, the files are the same!");
      break;
      case -1 :
        puts("Bundle 6, test 4: error comparing files...");
        ret = -1;
      break;
      case 1:
        puts("Bundle 6, test 4: error, the files are not the same!");
        ret = -1;
      break;
      default :
        puts("Bundle 6, test 4: Internal error");
        ret = -1;
      break;
  } // end switch

 if(!(ret))
   puts("Success, all tests in bundle 6 succeed");
 else
   puts("Some tests of test bundle 6 failed!");

  return ret;
}
/* ************************************************************************ */
