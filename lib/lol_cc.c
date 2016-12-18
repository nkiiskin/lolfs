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
 $Id: lol_cc.c, v0.13 2016/12/06 Niko Kiiskinen <nkiiskin@yahoo.com> Exp $"
*/

#include <stdio.h>
#include <string.h>

#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif

#define LOL_FSCK_OK      0
#define LOL_FSCK_INFO   (1)
#define LOL_FSCK_WARN   (2)
#define LOL_FSCK_ERROR  (3)
#define LOL_FSCK_FATAL  (4)
#define LOl_FSCK_INTRN  (5)

static const char* lol_prefix_list[] = {
  ": ",
  ": info, ",
  ": warning, ",
  ": error, ",
  ": fatal error, ",
  ": INTERNAL ERROR, ",
  NULL,
};
static const char* lol_tag_list[] = {
  "[OK]\n",
  "[INFO]\n",
  "[WARNING]\n",
  "[ERROR]\n",
  "[FATAL]\n",
  "[INTERNAL ERROR]\n",
  NULL,
};
// We want to fit in terminal,
// align should be < 80
#define LOL_FSCK_ALIGN 68
#define CONT_NOT_FOUND "%s: cannot find container \'%s\'\n"
static const char      lol_fsck_cc[] = "lol cc";
static const char   lol_fsck_lolfs[] = "fsck.lolfs";
// Error messages
static const char   lol_fsck_susp_name[] = "suspicious filename: ";
static const char   lol_fsck_nentry_io[] = "cannot read entry number ";
static const char   lol_fsck_err_super[] = "cannot read header metadata";
static const char  lol_fsck_corr_super[] = "corrupted header metadata";
static const char   lol_fsck_incons_sb[] = "inconsistent container size";
static const char   lol_fsck_wrong_siz[] = "inconsistent geometry of container";
static const char   lol_fsck_wrong_dev[] = "unsupported media type";
static const char  lol_fsck_corr_entry[] = "corrupted file entry at: ";
static const char          lol_fsck_io[] = "I/O error";

/* **************************************************************** */
/* **************************************************************** */
void lol_fsck_message(const char *me, const char* txt, const int type) {

  char message[512];
  int i;

  if ((type < LOL_FSCK_OK) || (type > LOL_FSCK_FATAL)) {
      i = LOl_FSCK_INTRN;
  }
  else {
    i = type;
  }

  memset((char *)message, 0, 512);
  strcat((char *)message, (const char *)me);
  strcat((char *)message, lol_prefix_list[i]);
  strcat((char *)message, (const char *)(txt));

  lol_align(message, lol_tag_list[i], LOL_FSCK_ALIGN);

} // end lol_fsck_message
/* *************************************************************** **
 * lol_fsck_sb: verify the superblock/metadata self-consistency.
 *
 ****************************************************************** */
static int lol_fsck_sb(FILE *fp, const char* cont, const char *me) {
  char message[512];
  struct lol_super sb;
  int ret = 0;

  if (fseek(fp, 0, SEEK_SET)) {
    lol_fsck_message(me, lol_fsck_io, LOL_FSCK_FATAL);
       return LOL_FSCK_FATAL;
  }
  if ((fread((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
       lol_fsck_message(me, lol_fsck_io, LOL_FSCK_FATAL);
       return LOL_FSCK_FATAL;
  } // end if fread
  if (!(sb.num_blocks)) {
       lol_fsck_message(me, "no data blocks found", LOL_FSCK_FATAL);
       return LOL_FSCK_FATAL;
  }
  if (!(sb.block_size)) {
       lol_fsck_message(me, "data block size is zero", LOL_FSCK_FATAL);
       return LOL_FSCK_FATAL;
  }
  if (LOL_INVALID_MAGIC) {
      ret = LOL_FSCK_ERROR;
      memset((char *)message, 0, 512);
      sprintf(message, "invalid file id [0x%x, 0x%x]",
	      (int)sb.reserved[0], (int)sb.reserved[1]);
      lol_fsck_message(me, message, ret);
  }
  if (sb.num_files > sb.num_blocks) {
      ret = LOL_FSCK_WARN;
      memset((char *)message, 0, 512);
      sprintf(message, "too many (%u) files found",
	     (unsigned int)(sb.num_files));
      lol_fsck_message(me, message, ret);
      memset((char *)message, 0, 512);
      sprintf(message, "          Maximum capacity is %u",
	     (unsigned int)(sb.num_blocks));
      lol_fsck_message(me, message, LOL_FSCK_INFO);
  }


  return ret;
} // end lol_fsck_sb
/* ******************************************************************
 * lol_fsck_geom: compare the superblock/metadata information about
 *                the container's actual size.
 *
 ****************************************************************** */
static int lol_fsck_geom(FILE *fp, const char* cont, const char *me) {
  char message[512];
  char gsize_s[128];
  char  size_s[128];
  struct stat st;
  struct lol_super sb;
  long size;
  long g_size;
  mode_t mode = 0;
  int ret     = 0;

  st.st_mode = 0;
  size = lol_get_vdisksize((char *)cont, &sb, &st, RECUIRE_SB_INFO);

  if (size < LOL_THEOR_MIN_DISKSIZE) {
       lol_fsck_message(me, lol_fsck_incons_sb, LOL_FSCK_FATAL);
       return LOL_FSCK_FATAL;
  }

  memset ((char *)gsize_s, 0, 128);
  memset ((char *)size_s,  0, 128);

  g_size = (long)LOL_DEVSIZE(sb.num_blocks, sb.block_size);
  mode = st.st_mode;
 
  lol_size_to_str((unsigned long)(g_size), gsize_s);
  lol_size_to_str((unsigned long)(size), size_s);

  if (S_ISREG(mode)) {
    // In this case the calculated size must be
    // _exactly_ the same as the sb data says.
    if (g_size != size) {
	ret = LOL_FSCK_ERROR;
        memset((char *)message, 0, 512);
        sprintf(message,
        "inconsistent container size (%s)\n          Should be %s",
        size_s, gsize_s);
        lol_fsck_message(me, message, ret);
    }

  } // end if regular file
  else {
    // So, it is not a regular file
#ifdef HAVE_LINUX_FS_H
    // In Linux, the container may be a block device
    // and it may be bigger than the sb says
       if (!(S_ISBLK(mode))) {
	   ret = LOL_FSCK_FATAL;
	   lol_fsck_message(me, lol_fsck_wrong_dev, ret);
       }
       else {
	  // It is a Linux block device
	 if (g_size > size) {

	     ret = LOL_FSCK_ERROR;
             memset((char *)message, 0, 512);
	     sprintf(message, "inconsistent container size (%s)", gsize_s);
	     lol_fsck_message(me, message, ret);
             memset((char *)message, 0, 512);
	     sprintf(message, "          Should not be bigger than %s", size_s);
             lol_fsck_message(me, message, LOL_FSCK_INFO);

	 }
	 else {

	     ret = LOL_FSCK_OK;
             memset ((char *)message,  0, 512);
	     sprintf(message, "device \'%s\' geometry test passed", cont);
             lol_fsck_message(me, message, ret);
	 }

       } // end else not block dev

#else

    ret = LOL_FSCK_FATAL;
    lol_fsck_message(me, lol_fsck_wrong_dev, ret);

#endif
  } // end else regular file

  return ret;
} // end lol_fsck_geom
/* ******************************************************************
 * lol_fsck_nent: directory name entry self-consistency check.
 *
 ****************************************************************** */
static int lol_fsck_nent(FILE *fp, const char* cont, const char *me) {

  char message[512];
  struct lol_super sb;
  struct stat st;
  struct lol_name_entry nentry;
  long dentry_off;
  long cont_size;
  long dentry_area;
  long dentry_size;
  long num_dentries;
  long fract;
  long i, idx, nb, bs, nf;
  long num_corrupted;
  long assumed_fblocks;
  long actual_fblocks;
  long n_files = 0;
  int  rval, ret = 0;

  if ((stat(cont, &st))) {
       ret = LOL_FSCK_FATAL;
       lol_fsck_message(me, lol_fsck_io, ret);
       return ret;
  }
  if (fseek(fp, 0, SEEK_SET)) {
       ret = LOL_FSCK_FATAL;
       lol_fsck_message(me, lol_fsck_io, ret);
       return ret;
  }
  if ((fread((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
       ret = LOL_FSCK_FATAL;
       lol_fsck_message(me, lol_fsck_io, ret);
       return ret;
  } // end if fread

  nb = (long)sb.num_blocks;
  bs = (long)sb.block_size;
  cont_size = (long)st.st_size;
  dentry_size = (long)(NAME_ENTRY_SIZE);

  nf = (long)sb.num_files;
  dentry_off  = LOL_DENTRY_OFFSET_EXT(nb, bs);
  // printf("DEBUG: dentry begins at offset %ld\n", dentry_off);
  dentry_area = (long)(dentry_off + dentry_size);
  // We MUST have at least 1 entry to analyze.
  if (cont_size < dentry_area) {
    // an earlier check has propably warned about this.
    ret = LOL_FSCK_FATAL;
    lol_fsck_message(me, lol_fsck_wrong_siz, ret);
    return ret;
  } // end if wrong size

  // Calculate how much we have dentry data
  dentry_area = cont_size - dentry_off;
  // subtract index area
  dentry_area -= (long)(nb * (long)(ENTRY_SIZE));
  // Now, the dentry area should be > 0 and an integer
  // multiple of NAME_ENTRY_SIZE
  if (dentry_area < dentry_size) {
    ret = LOL_FSCK_FATAL;
    lol_fsck_message(me, "no files: directory corrupted", ret);
    return ret;
  } // end if wrong size

  // How many are there?
  num_dentries = dentry_area / dentry_size;
  fract = dentry_area % dentry_size;

  if ((fract) || (num_dentries != nb)) {
     // at least dentry storage is corrupted
     lol_fsck_message(me, "some files may be lost", LOL_FSCK_ERROR);
  } // end if fract

  if (fseek(fp, dentry_off, SEEK_SET)) {
       ret = LOL_FSCK_FATAL;
       lol_fsck_message(me, lol_fsck_io, ret);
       return ret;
  }

  num_corrupted = 0;
  // Let's read and see what we have
  for (i = 0; i < num_dentries; i++) {

    if ((fread((char *)&nentry,
               (size_t)(NAME_ENTRY_SIZE), 1, fp)) != 1) {
          memset((char *)message, 0, 512);
	  sprintf(message, "%s%ld", lol_fsck_nentry_io, i);
          lol_fsck_message(me, message, LOL_FSCK_WARN);
	  continue;

     } // end if fread

    // Got one!
    idx = (long)(nentry.i_idx);
    // Check 1: if no name but file_size > 0   => error
    // Check 2: if no name but allocated index => error
    if (!(nentry.filename[0])) {
        if ((nentry.file_size) || (idx)) {
           // This is f*cked up, continue
           memset((char *)message, 0, 512);
   	   sprintf(message, "%s%ld", lol_fsck_corr_entry, i);
           lol_fsck_message(me, message, LOL_FSCK_ERROR);
           num_corrupted++;

        } // end if file_size or index
	// Seems to be fine but if there is no name
	// there should not be reserved index either
	// Also, the file_size should be 0

	continue;
#if 0
  UCHAR  filename[LOL_FILENAME_MAX];
  time_t created;
  alloc_entry i_idx;
  ULONG  file_size;
#endif

    } // end if no name

    // So, there is a file
    n_files++;
    // Check if it seems to be ok
    if ((nentry.filename[((LOL_FILENAME_MAX) - 1)])) {
        num_corrupted++;
	continue;
    }
    if ((lol_garbage_filename((char *)(nentry.filename)))) {

         memset((char *)message, 0, 512);
	 sprintf(message, "%s\'%s\'", lol_fsck_susp_name,
                                 (char *)(nentry.filename));
         lol_fsck_message(me, message, LOL_FSCK_INFO);

    } // end if garbage name

    // How about the size of the file?
    assumed_fblocks = (long)nentry.file_size;

    if ((assumed_fblocks < 0) || (idx < 0) || (idx >= nb)) {
      num_corrupted++;
      lol_fsck_message(me, "invalid file size found", LOL_FSCK_ERROR);
      continue;
    }

    if (assumed_fblocks) {
        assumed_fblocks /= bs;
        if (((long)nentry.file_size) % bs)
            assumed_fblocks++;
    }
    else {
      assumed_fblocks = 1; // 0 size files consume 1 block
    } // end else

    if ((assumed_fblocks > nb)) {
       num_corrupted++;
       lol_fsck_message(me, "invalid file size found", LOL_FSCK_ERROR);
       continue;
    }

    // Compare this to the number of blocks we find
    // TODO: We could actually fix the chain here by settin the flag
    rval = (long)lol_count_file_blocks(fp, &sb,
		     nentry.i_idx, cont_size, &actual_fblocks, 0);

    if (!(rval)) { /* not error? */

      if (assumed_fblocks != actual_fblocks) {
	  num_corrupted++;
      }

      continue;

    } // end if ! rval

    memset((char *)message, 0, 512);
    sprintf(message, "found corrupted file \'%s\'", (char *)(nentry.filename));
    lol_fsck_message(me, message, LOL_FSCK_ERROR);
    num_corrupted++;


    } // end for i

    if (num_corrupted) {

          memset((char *)message, 0, 512);
	  sprintf(message, "found %ld corrupted files", num_corrupted);
          lol_fsck_message(me, message, LOL_FSCK_INFO);
    }

    // TODO: Super block is easy to fix, we just warn for now..
    if (nf != n_files) {
      lol_fsck_message(me, "the number of files is wrong", LOL_FSCK_ERROR);
      return LOL_FSCK_ERROR;
    }

    if (num_corrupted)
      return LOL_FSCK_ERROR;

   return 0;
} // end lol_fsck_nent
/* ****************************************************************** */
typedef int (*lol_check_t)(FILE *, const char *, const char *);
typedef struct lol_check_func_t
{

  lol_check_t     func;
         char    *alias;
         char   *descr;

} lol_check_func;
/* ****************************************************************** */
static const lol_check_func lol_check_funcs[] =
{
    {lol_fsck_sb,     "header", "   header metadata self consistency:"},
    {lol_fsck_geom, "geometry", " container geometry:"},
    {lol_fsck_nent, "directory", "directory consistency:"},
    {NULL, NULL, NULL},
};
/* ****************************************************************** */
static const char params[] = "container";
static const char    hlp[] = "       Type '%s -h' for help.\n";
static const char*   lst[] =
{
  "  Example:\n",
  "          lol cc lol.db",
  "          This checks the container \'lol.db\'",
  "          for errors.\n",
  "  There are 5 levels of information output:\n",
  "         - OK    (one check has has been passed).",
  "         - INFO  (some information, not error).",
  "         - WARN  (warning, something is wrong).",
  "         - ERROR (something that needs repair).",
  "         - FATAL (needs immediate repair).\n",
  "          Type 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
void lol_fsck_help(char *me) {
  // Another stupid hack
  int j = 11, i = 2;
  if (!(me))
    return;
  puts(lst[0]);
  if (me[0] != 'l') { /* If the calling functions is not 'lol'
                         then it must (hopefully) be fsck.lolfs */
    j--;
    puts("          fsck.lolfs lol.db");
  }
  else
    puts(lst[1]);
  for (; i < j; i++) {
    puts(lst[i]);
  } // end for
  if (i == 10)
    printf("Type \'man %s\' to read the manual.\n\n", me);
} // end lol_fsck_help
/* ******************************************************************
 * lol_cc: checks if a container has errors.
 ******************************************************************** */
int lol_cc (int argc, char *argv[]) {

  struct stat st;
  FILE       *fp;
  char       *me;
  char     *cont;
  char     *desc;
  char     *curr;
  int      i = 0;
  int     rv = 0;
  int    ret = 0;

  if (!(argv[0])) {
      puts(lol_inter_err);
      return -1;
  }

  // We must have a proper name
  if ((!(argv[0][0])) || (!(argv[0][1]))) {
      puts(lol_inter_err);
      return -1;
  }

  // Let's see who called us
  if (!(strcmp(argv[0], "cc")))
    me = (char *)lol_fsck_cc;
  else
    me = (char *)lol_fsck_lolfs;


  // Process standard --help & --version options.
  if (argc == 2) {

    if (LOL_CHECK_HELP) {

         printf ("%s v%s. %s\nUsage: %s %s\n", me,
                 lol_version, lol_copyright, me, params);


        lol_fsck_help(me);
        return 0;
    }

    if (LOL_CHECK_VERSION) {

	printf ("%s v%s %s\n", me,
                lol_version, lol_copyright);
	return 0;
    }
    if (argv[1][0] == '-') {

          printf("%s: unrecognized option \'%s\'\n", me, argv[1]);
          printf (hlp, me);
          return -1;
    }
  } // end if argc == 2

  if (argc != 2) {

        printf ("%s v%s. %s\nUsage: %s %s\n", me, lol_version,
                lol_copyright, me, params);

        printf (hlp, me);
        return 0;
  }

  cont = argv[1];
  if ((stat(cont, &st))) {
      printf(CONT_NOT_FOUND, me, cont);
      return -1;
  }
  if (st.st_size < LOL_THEOR_MIN_DISKSIZE) {
      printf("lol %s: \'%s\' is too small to be a container.\n", me, cont);
     return -1;
  }
  if (!(fp = fopen(cont, "r"))) {
      printf(CONT_NOT_FOUND, me, cont);
      return -1;
  }

  while (lol_check_funcs[i].func) {

    desc = lol_check_funcs[i].descr;
    curr = lol_check_funcs[i].alias;

    rv = lol_check_funcs[i].func(fp, cont, curr);
    if (!(rv))
       lol_fsck_message(curr, desc, LOL_FSCK_OK);
    if (rv == LOL_FSCK_FATAL) {
       fclose(fp);
       printf("%s: container \'%s\' has errors, exiting.\n", me, cont);
       return -1;
    }

    i++;

  } // end while funcs

  // error:

  fclose(fp);
  return ret;

} // end lol_cc
