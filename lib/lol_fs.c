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
 $Id: mkfs.lolfs.c, v0.13 2016/11/02 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lolfs.h>
#include <lol_internal.h>
/* ************************************************************************ */
static const char params[] = "<block size (in bytes)> <# of blocks> <name>";
static const char    hlp[] = "           Type \'%s -h\' for help.\n";
static const char*   lst[] =
{
  "  Example:\n",
  "  1000  4000 lol.db",
  "          This creates a container file \"lol.db\" which has 4000",
  "          blocks, each 1000 bytes. So the storage capacity is",
  "          1000 * 4000 = 4000000 bytes, which is roughly 4Mb.\n",
  "          Type \'man %s\' to read the manual.\n",
  NULL
};
/* ************************************************************************ */
static const char
       lol_overwrite_prompt[] = "                       Overwrite [y/n]? ";
static const char
   lol_proceed_prompt[] = "                               Proceed [y/n]? ";
/* ************************************************************************ */
// This is a terrible hack but it is the only one, hopefully...
void lol_fs_help(char* name) {
  const char lol[] = "lol";
  int i = 2;
  if (!(name))
    return;

  puts(lst[0]);
  printf("          %s", name);
  puts(lst[1]);
  for (; i < 5; i++) {
    puts(lst[i]);
  }
  if (!(strcmp(name, "lol fs")))
    printf(lst[5], lol);
  else
    printf(lst[5], name);
} // end lol_fs_help
/* ************************************************************************ */
static int prompt_mkfs() {
  char answer;

       answer = (char)getchar();
       (void)getchar();
       if (answer != 'y')
	 return -1;

   return 0;
} // end prompt_mkfs
/* ************************************************************************ */
int lol_fs (int argc, char* argv[])
{
  char  name[1024];
  struct stat st;
  DWORD bs;
  DWORD nb;
  unsigned long  size;
  int   ret;
  int   val;
  char  *self = 0;

  if ((!(argc)) || (!(argv))) {
     puts(lol_inter_err);
     return -1;
  }
  if (!(argv[0])) {
     puts(lol_inter_err);
     return -1;
  }

  self = argv[0];
  if (!(self[0])) {
     puts(lol_inter_err);
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

         printf ("%s v%s. %s\nUsage: %s %s\n", name,
                 lol_version, lol_copyright, name, params);

	 lol_fs_help(name);
         return 0;

    }
    if (LOL_CHECK_VERSION) {

	printf ("%s v%s %s\n", name,
                lol_version, lol_copyright);
	return 0;
    }

     printf("%s: unrecognized option \'%s\'\n",
            name, argv[1]);

     printf (hlp, name);
     return -1;

  } // end if argc == 2

  if (argc != 4) {
        printf ("%s v%s. %s\nUsage: %s %s\n", name,
                 lol_version, lol_copyright, name, params);

     printf (hlp, name);
     return -1;
  }

  // We need 2 numbers, block size and the number of blocks
  bs   = (DWORD)strtol(argv[1], NULL, 10);
  nb   = (DWORD)strtol(argv[2], NULL, 10);
  size = bs * nb;

  val = stat(argv[3], &st);

  if (!(val)) {
    // File or block device already exists
    // Warn about this and prompt
    if ((S_ISBLK(st.st_mode))) {

       printf("%s: warning! All the data in %s will be lost!\n",
               name, argv[3]);
       printf("%s", lol_proceed_prompt);
       ret = prompt_mkfs();
       if (ret)
	  return 0;

    }
    else {

      if ((S_ISREG(st.st_mode))) {

         printf("%s: warning! The file \"%s\" exists.\n",
              name, argv[3]);
         printf("%s", lol_overwrite_prompt);
	 ret = prompt_mkfs();
         if (ret)
	    return 0;

      } // end if regular file
      else {
	printf("%s: cannot use %s\n", name, argv[3]);
	return -1;
      }

    } // end else if S_ISBLK

  } // end if !stat


  // Ok, Let's do it
  if (lol_mkfs(bs, nb, argv[3])) {
       printf("%s: cannot create container \'%s\'\n",
              name, argv[3]);
       puts("        Please check if the directory is write protected");
       return -1;
  }
  else {
       printf("Container \'%s\' created\nStorage capacity ", argv[3]);
       memset((char *)name, 0, 1024);
       lol_size_to_str(size, name);
       puts(name);

  } // end else

  return 0;
} // end lol_fs
