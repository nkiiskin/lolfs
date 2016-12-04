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
static const char   usg[] = "<block size (in bytes)> <# of blocks> <name>";
static const char  usg2[] = "           Type \'%s -h\' for help.\n";
static const char*  lst[] =
{
  "  Example:\n",
  "          mkfs.lolfs  1000  4000 lol.db",
  "          This creates a container file \"lol.db\" which has 4000",
  "          blocks, each 1000 bytes. So the storage capacity is",
  "          1000 * 4000 = 4000000 bytes, which is roughly 4Mb.\n",
  "          Type 'man mkfs.lolfs' to read the manual.\n",
  NULL
};
/* ************************************************************************ */
static void help() {
  int i = 0;
  while (lst[i]) {
    puts(lst[i++]);
  };
} // end help
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
int main (int argc, char* argv[])
{
  struct stat st;
  DWORD bs;
  DWORD nb;
  long  size;
  int   ret;
  int   val;

  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {

        printf ("%s v%s. %s\nUsage: %s %s\n", argv[0],
                lol_version, lol_copyright, argv[0], usg);
        help ();
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf ("%s v%s %s\n", argv[0],
                lol_version, lol_copyright);
	return 0;
    }

    printf("%s: unrecognized option \'%s\'\n",
          argv[0], argv[1]);
    help();
    return -1;
  } // end if argc == 2

  if (argc != 4) {

      printf ("%s v%s. %s\nUsage: %s %s\n", argv[0],
              lol_version, lol_copyright, argv[0], usg);
      printf (usg2, argv[0]);
      return 0;
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
              argv[0], argv[3]);
       printf("            Proceed [y/n]? ");
       ret = prompt_mkfs();
       if (ret)
	  return 0;

    }
    else {

      if ((S_ISREG(st.st_mode))) {

         printf("%s: warning! The file \"%s\" exists.\n",
              argv[0], argv[3]);
         printf("            Overwrite [y/n]? ");
	 ret = prompt_mkfs();
         if (ret)
	    return 0;

      } // end if regular file
      else {
	printf("%s: cannot use %s\n", argv[0], argv[3]);
	return -1;
      }

    } // end else if S_ISBLK

  } // end if !stat


  // Ok, Let's do it
  if (lol_mkfs(bs, nb, argv[3])) {
       printf("%s: cannot create container %s, size %ld bytes, (%ld Kb)\n",
              argv[0], argv[3], size, (size / 1024));
       puts("Please check if the directory is write protected");
       return -1;
  }
  else {
       puts("Container created");
  }

  return 0;
} // end main
