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
 $Id: lol_df.c, v0.13 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"

 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ****************************************************************** */
static const char  cantuse[] = "lol %s: cannot use %s\n";
static const char cantread[] = "lol %s: cannot read %s\n";
static const char   params[] = "container";
static const char      hlp[] = "       Type 'lol %s -h' for help.\n";
static const char*     lst[] =
{
  "  Example:\n",
  "          lol df lol.db",
  "          This shows the space usage of container",
  "          file \'lol.db\'\n",
  "          Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int lol_df (int argc, char* argv[])
{

  char number[1024];
  struct lol_name_entry name_e;
  struct lol_super sb;
  FILE *vdisk;
  char *me;
  DWORD i, nf;
  DWORD num_blocks;
  DWORD block_size;
  DWORD used_blocks = 0;
  long free_space   = 0;
  long files        = 0;
  long occupation   = 0;
  long used_space   = 0;
  long doff         = 0;
  alloc_entry entry = 0;
  float available   = 0;


  if (!(argv[0])) {

      puts(LOL_INTERERR_FMT);
      return -1;
  }
  me = argv[0];

  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {

        printf (LOL_USAGE_FMT, me, lol_version,
                lol_copyright, me, params);

        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {

	printf (LOL_VERSION_FMT, me,
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {

          printf(LOL_WRONG_OPTION, me, argv[1]);
          printf (hlp, me);
          return -1;
    }
  } // end if argc == 2

  if (argc != 2) {

        printf (LOL_USAGE_FMT, me, lol_version,
                lol_copyright, me, params);
        puts  ("       Shows container space usage.");
        printf (hlp, me);
        return 0;
  }

  free_space = lol_get_vdisksize(argv[1], &sb, NULL, RECUIRE_SB_INFO);

  if (free_space < LOL_THEOR_MIN_DISKSIZE) {

       printf(cantuse, me, argv[1]);
       return -1;
  }

  if (LOL_INVALID_MAGIC) {

    printf("lol %s: invalid file id [0x%x, 0x%x].\n",
	   me, (int)sb.reserved[0], (int)sb.reserved[1]);
      puts(LOL_FSCK_FMT);
      return -1;
  }

  num_blocks = sb.num_blocks;
  block_size = sb.block_size;

  if ((!(num_blocks)) || (!(block_size))) {

      printf(cantuse, me, argv[1]);
      return -1;
  }

  free_space = (long)(num_blocks * block_size);

  memset((char *)number, 0, 1024);
  lol_size_to_str((unsigned long)free_space, number);

  printf("Total space is %s [%u blocks, each %u bytes]\n",
         number,
         ((unsigned int)(num_blocks)),
	 ((unsigned int)(block_size)));

  nf = sb.num_files;

  if (!(vdisk = fopen(argv[1], "r"))) {

       printf(cantread, me, argv[1]);
       return -1;
  }

  doff = LOL_DENTRY_OFFSET_EXT(num_blocks, block_size);
  if (fseek (vdisk, doff, SEEK_SET)) {

       fclose(vdisk);
       printf(cantread, me, argv[1]);
       return -1;
  }

  // Read the name entries
  for (i = 0; i < num_blocks; i++) {

    if ((fread((char *)&name_e, (size_t)(NAME_ENTRY_SIZE), 1, vdisk)) != 1) {

       fclose(vdisk);
       printf(cantread, me, argv[1]);
       return -1;
    }

    if (name_e.filename[0]) { // Should check more but this will do now
        files++;
        used_space += name_e.file_size;
    }
  } // end for i

   // Read also the reserved blocks.

  for (i = 0; i < num_blocks; i++) {

   // We could propably suck all the indexes into
   // a buffer with a few reads, something todo...

    if ((fread((char *)&entry, (size_t)(ENTRY_SIZE), 1, vdisk)) != 1) {
         fclose(vdisk);
         printf(cantread, me, argv[1]);
         return -1;
    }

    if (entry != FREE_LOL_INDEX) {
        used_blocks++;
    }

  } // end for i

  fclose(vdisk);
  occupation = (long)(used_blocks * block_size);
  free_space = (long)(num_blocks - used_blocks);
  available  = (float)(free_space);
  free_space *= block_size;
  available  /= ((float)(num_blocks));
  available  *= ((float)(100.0));

  memset((char *)number, 0, 1024);
  lol_size_to_str((unsigned long)free_space, number);

  printf("Unused space is %s (%ld bytes), %2.1f%% free\n",
	 number,
         ((long)(free_space)), available);

  if (files) {

         available = 100.0 - available;
         memset((char *)number, 0, 1024);
         lol_size_to_str((unsigned long)(occupation), number);

         printf("Used space is %s (%ld bytes), %2.1f%% full\n",
                number, used_space, available);

  }
  else {
    puts("No files");
  }


  if ((used_space > occupation)  ||
      (used_blocks > num_blocks) || (nf != files)) {

    printf("lol %s: container \'%s\' has errors.\n",
            me, argv[1]);
    puts(LOL_FSCK_FMT);
  }

  return 0;

} // end lol_df
