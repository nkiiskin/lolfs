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
 $Id: lol_ls.c, v0.13 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif


void strip_time(char* t) {
  int i, ln;
  if (!t)
    return;

  ln = strlen(t);
  for(i = 0; i < ln; i++) {
    if (t[i] == '\n' || t[i] == '\r') {
        t[i] = '\0';
      break;
    }
  }
}
/* ****************************************************************** */
static const char params[] = "container";
static const char    hlp[] = "       Type 'lol %s -h' for help.\n";
static const char*   lst[] =
{
  "  Example:\n",
  "          lol ls lol.db",
  "          This lists the files which are inside",
  "          container file \'lol.db\'\n",
  "          Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
static void help() {
  int i = 0;
  while (lst[i]) {
    puts(lst[i++]);
  };
} // end help
/* ****************************************************************** */
int lol_ls(int argc, char* argv[])
{

  struct lol_super sb;
  struct lol_name_entry entry;
  char tmp[128];
  mode_t st_mode;
  FILE *vdisk;
  DWORD i, nf, num_blocks;
  long bs, doff;
  long raw_size;
  long files = 0;
  char *time_string = 0;
  int j, ln, spaces;
  int fails = 0;
  int corr  = 0;

  // Process standard --help & --version options.
  if (argc == 2) {
    if (LOL_CHECK_HELP) {

        printf ("lol %s v%s. %s\nUsage: lol %s %s\n",
                argv[0], lol_version, lol_copyright,
                argv[0], params);
        help ();
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf ("lol %s v%s %s\n", argv[0],
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {

          printf(LOL_WRONG_OPTION, argv[0], argv[1]);
          printf (hlp, argv[0]);
          return -1;
    }
  } // end if argc == 2

  if (argc != 2) {

        printf ("lol %s v%s. %s\nUsage: lol %s %s\n", argv[0],
                 lol_version, lol_copyright, argv[0], params);
	puts  ("       Lists files inside a container");
        printf (hlp, argv[0]);
        return 0;
  }

    if (!(lol_is_validfile(argv[1]))) {
	  printf("lol %s: %s is not a valid container\n",
                  argv[0], argv[1]);
          return -1;

    }

    raw_size = lol_get_vdisksize(argv[1], &sb, &st_mode, RECUIRE_SB_INFO);
    if ((raw_size <  LOL_THEOR_MIN_DISKSIZE) ||
        (sb.num_files > sb.num_blocks))
    {

	   puts("Corruption detected. Run fsck.lolfs");
           return -1;
    }

    if (LOL_INVALID_MAGIC)
    {

       printf("Corrupted file id [0x%x, 0x%x]. Is this really a lol file?\n",
	       (unsigned int)sb.reserved[0], (unsigned int)sb.reserved[1]);
       return -1;
    }

      nf = sb.num_files;
      num_blocks = sb.num_blocks;
      bs = (long)sb.block_size;
      if (nf > num_blocks)
	corr = 1;

      if (!(vdisk = fopen(argv[1], "r"))) {
           puts("Error: cannot read directory");
	   return -1;
      }

      doff = (long)LOL_DENTRY_OFFSET_EXT(num_blocks, bs);

      if (fseek (vdisk, doff, SEEK_SET)) {
           puts("Error: cannot read directory");
	   return -1;
      }

      // Read the name entries

    for (i = 0; i < num_blocks; i++) {

      if ((fread ((char *)&entry, (size_t)(NAME_ENTRY_SIZE), 1, vdisk)) != 1)
      {
          printf("Warning: cannot read directory entry number %u\n", i);

	  if (++fails > 3) {
	      break;
          }

	  continue;
      }

      if (!(entry.filename[0]))
	  continue;

              ln = strlen((char *)entry.filename);
	      if (ln >= LOL_FILENAME_MAX) {
		  if (++fails > 3) {
		      break;
                  }
	            continue;
              }


                files++;

                 time_string = ctime(&entry.created);

                 if (time_string) {

                     strip_time(time_string);
                      printf("%s", time_string);
                       ln = strlen(time_string);
                        spaces = 30 - ln;
                        if (spaces < 4)
	                     spaces = 4;

                          for (j = 0; j < spaces; j++) {
                               printf(" ");
                          }
                           // Append file size;
                           memset(tmp, 0, 128);
                           sprintf(tmp, "%d", entry.file_size);
                           printf("%s", tmp);
                          ln = strlen(tmp);
                         spaces = 10 - ln;
                        if (spaces < 4)
	                      spaces = 4;
                       for (j = 0; j < spaces; j++) {
                             printf(" ");
                       }
                     printf("%s\n", entry.filename);
                 } // end if time_string
                 else
                    printf("01-Jan 00:00:00 1970          %d     %s\n",
                            entry.file_size, entry.filename);

  } // end for i

    if ((nf != files) || (fails) || (corr))
       puts("Corruption detected. Run fsck.lolfs");
    else
      printf("total %ld\n", files);

  fclose(vdisk);
  return 0;

} // end lol_ls
