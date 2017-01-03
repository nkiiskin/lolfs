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
 $Id: lol_fs.c, v0.20 2016/11/02 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_STDIO_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
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
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ************************************************************************ */
static const char params[] =
          "Usage: %s -b <block size> <# of blocks> <name>\n       %s -s <size> <name>\n";
static const char    hlp[] = "       Type: \'%s -h\' for help.\n";
static const char*   lst[] =
{
  "  Usage: %s -b|-s <parameters> <container name>\n\n",
  "  Example 1:\n",
  "           %s -s 150M lol.db\n",
  "           This creates a container file \'lol.db\' which has\n",
  "           150 Megabytes of storage capacity.\n\n",
  "  Example 2:\n",
  "           %s -b 1000  4000 lol.db\n",
  "           This creates a container file \'lol.db\' which has 4000\n",
  "           blocks, each 1000 bytes. So the storage capacity is\n",
  "           1000 * 4000 = 4000000 bytes, which is roughly 4Mb.\n\n",
  "           Type: \'man lol\' to read the manual.\n\n",
  NULL
};
/* ************************************************************************ */
static const char
       lol_overwrite_prompt[] = "                       Overwrite [y/n]? ";
static const char
   lol_proceed_prompt[] = "                               Proceed [y/n]? ";
static const char lol_ask_wp[] =
   "        Please check if the directory is write protected";
/* ************************************************************************ */
void lol_fs_help(const char* name) {
  int i = 0;
  while (lst[i]) {
    printf(lst[i], name);
    i++;
  }
} // end lol_fs_help
/* ************************************************************************ */
static int prompt_mkfs() {
  char answer[128];

       answer[0] = (char)getchar();
       (void)getchar();
       if (answer[0] != 'y')
	 return -1;

   return 0;
} // end prompt_mkfs
/* ************************************************************************ */
int lol_fs (int argc, char* argv[])
{
  char    name[1024];
  char size_str[128];

  struct stat st;
  const ULONG crit_s = 100 * LOL_MEGABYTE;

  DWORD     bs =  0;
  DWORD     nb =  0;
  char   *file =  0;
  char  *self  =  0;
  char *amount =  0;
  ULONG   size =  0;
  int    small =  0;
  int      val =  0;
  int   file_i =  3;

  if ((!(argc)) || (!(argv))) {
     lol_error(lol_inter_err);
     return -1;
  }
  if (!(argv[0])) {
      lol_error(lol_inter_err);
      return -1;
  }

  self = argv[0];
  if (!(self[0])) {
     lol_error(lol_inter_err);
     return -1;
  }

  memset((char *)name, 0, 1024);
  // Who is calling?
  // This is a pretty ugly hack but i want
  // mkfs.lolfs to use this function also..
  if ((self[0] == 'f') && (self[1] == 's')) {
    strcat(name, "lol fs");
  }
  else {
    strcat(name, self);
  }

  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {
         printf ("%s v%s. %s\n", name,
                 lol_version, lol_copyright);
	 lol_fs_help(name);
         return 0;

    }
    if (LOL_CHECK_VERSION) {
	printf ("%s v%s %s\n", name,
                lol_version, lol_copyright);
	return 0;
    }

     lol_error("%s: syntax error\n", name);
     lol_error(hlp, name);
     return -1;

  } // end if argc == 2

  if ((argc < 4) || (argc > 5))
      goto error;

  // Does user want to specify size or number of blocks and their size?
  if (LOL_CHECK_BLOCKS) {
     if (argc != 5) {
         goto error;
     }

    // We need 2 numbers, block size and the number of blocks
    bs   = (DWORD)strtol(argv[2], NULL, 10);
    nb   = (DWORD)strtol(argv[3], NULL, 10);
    file_i = 4;
    if ((bs < 1) || (nb < 1))
        goto error;

  } // end if add blocks
  else {

     if (LOL_CHECK_SIZE) {
         if (argc != 4) {
             goto error;
         }

         amount = argv[2];
         if (!(amount))
	    goto error;
       if (!(amount[0]))
	   goto error;

       small = lol_size_to_blocks(amount, NULL,
	       NULL, NULL, &nb, LOL_JUST_CALCULATE);

       if (small == LOL_FS_TOOSMALL) {
 	   lol_error("%s: sorry, the size is too small.\n", name);
	   lol_error("        you may create tiny container\n");
	   lol_error("        using option \'-b\' and specifying\n");
	   lol_error("        the block size and number of them.\n");
	   return -1;
       } // end if too small

       if (small) {
	   goto error;
       }
       else {
	 // Got the amount of blocks!
	  bs = LOL_DEFAULT_BLOCKSIZE;
	  file_i = 3;
       } // end else got it
     } // end if got size
     else {
       goto error;
     }
  } // end else adds file size

  size = (ULONG)(bs);
  size *= ((ULONG)(nb));
  file = argv[file_i];
   val = stat(file, &st);

  memset((char *)size_str, 0, 128);
  lol_ltostr(size, size_str);

  if (!(val)) {
    // File or block device already exists
    // Warn about this and prompt
    if ((S_ISBLK(st.st_mode))) {

       printf("%s: warning! All the data in \'%s\' will be lost!\n",
               name, file);
       printf("%s", lol_proceed_prompt);
       if ((prompt_mkfs()))
	  return 0;
    }
    else {

      if ((S_ISREG(st.st_mode))) {

         printf("%s: warning! The file \'%s\' exists.\n",
              name, file);
         printf("%s", lol_overwrite_prompt);
	 if ((prompt_mkfs()))
	    return 0;

      } // end if regular file
      else {
	lol_error("%s: cannot use \'%s\'\n", name, file);
	return -1;
      }
    } // end else if S_ISBLK
  } // end if !stat
  else {

    // Prompt confimation if container size is > 100M
    if (size > crit_s) {
         printf("%s: creating container \'%s\', size %s\n",
                name, file, size_str);
         printf("%s", lol_proceed_prompt);
	 if ((prompt_mkfs()))
	     return 0;

    } // end if > crit_s

  } // end else

  // Ok, Let's do it
  if ((lol_mkfs("-b", NULL, bs, nb, file))) {
          lol_error("%s: cannot create container \'%s\'\n", name, file);
          lol_error("%s", lol_ask_wp);
          return -1;
      }
  else {
     printf("Container \'%s\' created\nStorage capacity %s\n",
             file, size_str);
  } // end else

  return 0;

error:
  lol_error("%s v%s. %s\n", name,
            lol_version, lol_copyright);
  lol_error(params, name, name);
  lol_error(hlp, name);
  return -1;

} // end lol_fs
