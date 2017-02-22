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
 $Id: testfuncs.c, v0.40 2017/02/11 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
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
/* ************************************************************************ */
/* LoL testsuite globals                                                    */
/* ************************************************************************ */
#if 0

  char   tag[];
  char *argv[];
  int bundle;

  lol_func func;
  int argc;




const lol_testargs bundle7[] = {
{
  "Testing foo", 7, 
lol_func f;
}



};
#endif

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
