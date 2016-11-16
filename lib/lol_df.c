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
/* $Id: lol_df.c, v0.11 2016/04/19 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $" */

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


int lol_df (int argc, char* argv[])
{

  struct lol_name_entry name_e;
  struct lol_super sb;
  FILE *vdisk;
  DWORD i, nf;

  DWORD num_blocks;
  DWORD block_size;

  DWORD used_blocks = 0;
  long free_space, files = 0;
  long occupation;
  long used_space = 0;
  alloc_entry entry = 0;
  float available = 0;

  if (argc != 2) {

       printf("Usage: lol %s  <container>\n", argv[0]);
       puts  ("       Shows container space usage.");
       return 0;
  }

  free_space = lol_get_vdisksize(argv[1], &sb, NULL, RECUIRE_SB_INFO);

  if (free_space < LOL_THEOR_MIN_DISKSIZE) {

       printf("Error: Cannot use %s\n", argv[1]);
       return -1;
  }

  if (LOL_INVALID_MAGIC) {

    printf("Error: Invalid file id [0x%x, 0x%x]. Run fsck.lolfs\n",
	  (int)sb.reserved[0], (int)sb.reserved[1]);

      return -1;
  }

  num_blocks = sb.num_blocks;
  block_size = sb.block_size;
  if ((!(num_blocks)) || (!(block_size))) {
       printf("Error: Cannot use %s\n", argv[1]);
       return -1;
  }

  free_space = (long)(num_blocks * block_size);

  printf("Total container space is %ld Kb [%u blocks, each %u bytes]\n",
         ((long)(free_space >> LOL_DIV_1024)),
         ((unsigned int)(num_blocks)),
	 ((unsigned int)(block_size)));

  nf = sb.num_files;

  if (!(vdisk = fopen(argv[1], "r"))) {

       printf("Error: Cannot read %s\n", argv[1]);
       return -1;
  }

  if (fseek (vdisk, DISK_HEADER_SIZE, SEEK_SET)) {

       fclose(vdisk);
       printf("Error: Cannot read %s\n", argv[1]);
       return -1;
  }

  // Read the name entries
  for (i = 0; i < num_blocks; i++) {

    fread((char *)&name_e, (size_t)(NAME_ENTRY_SIZE), 1, vdisk);

    if (name_e.filename[0]) { // Should check more but this will do now
      files++;
      used_space += name_e.file_size;
    }
  } // end for i

   // Read also the reserved blocks.

  for (i = 0; i < num_blocks; i++) {

   // We could propably suck all the indexes into
   // a buffer with a few reads, something todo...

    fread((char *)&entry, (size_t)(ENTRY_SIZE), 1, vdisk);

    if (entry != FREE_LOL_INDEX)
        used_blocks++;

  } // end for i

  fclose(vdisk);
  occupation = (long)(used_blocks * block_size);
  free_space = (long)(num_blocks - used_blocks);
  //printf("DEBUG: Free blocks = %ld\n", (long)(free_space));
  available  = (float)(free_space);
  free_space *= block_size;
  available  /= ((float)(num_blocks));
  available  *= ((float)(100.0));

  printf("Unused space is %ld Kb (%ld bytes), %2.1f%% free\n",
	 ((long)((free_space >> LOL_DIV_1024))),
         ((long)(free_space)), available);

  if (files) {
    /*
         available   = (float)(used_blocks);
         available  /= ((float)(num_blocks));
         available  *= ((float)(100.0));
    */
         available = 100.0 - available;
         printf("Used space is %ld Kb (%ld bytes), %2.1f%% full\n",
                ((long)(occupation >> LOL_DIV_1024)),
		used_space,
                available);

  }
  else {
    puts("No files");
  }


  if (used_space > occupation || used_blocks > num_blocks || nf != files)
  {
     puts("Filesystem has errors. Run fsck.lolfs");
  }

  return 0;

} // end main
