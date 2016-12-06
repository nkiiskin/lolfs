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
#define LOL_FSCK_ALIGN 62
#define CONT_NOT_FOUND "lol %s: cannot find container \'%s\'\n"

// Error messages

const char lol_fsck_err_super[] = "cannot read header metadata";
const char lol_fsck_corr_super[] = "corrupted header metadata";
const char  lol_fsck_wrong_dev[] = "unsupported media type";
/* **************************************************************** */
/* **************************************************************** */
void lol_fsck_message(const char *me, const char* txt, const int type) {

  char message[512];
  //size_t len;
  int i;

  if ((type < LOL_FSCK_OK) || (type > LOL_FSCK_FATAL)) {
      i = LOl_FSCK_INTRN;
  }
  else {
    i = type;
  }

  memset((char *)message, 0, 512);
  //  message[0] = '\n';
  strcat((char *)message, (const char *)me);
  strcat((char *)message, lol_prefix_list[i]);
  strcat((char *)message, (const char *)(txt));
  //len = strlen(message);
  //  message[len] = '\n';
  // printf("\n%s: fatal error, %s", (char *)me, (char *)txt);
  lol_align(message, lol_tag_list[i], LOL_FSCK_ALIGN);

} // end lol_fsck_message
/* *************************************************************** **
 * lol_fsck_sb: verify the superblock/metadata self-consistency.
 *
 ****************************************************************** */
static int lol_fsck_sb(FILE *fp, const char* cont, const char *me) {

  struct lol_super sb;
  int ret = 0;

  if (fseek(fp, 0, SEEK_SET)) {

    lol_fsck_message(me, lol_fsck_err_super, LOL_FSCK_FATAL);
       return LOL_FSCK_FATAL;
  }
  if ((fread((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
       lol_fsck_message(me, lol_fsck_err_super, LOL_FSCK_FATAL);
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
      printf("%s: warning, invalid file id [0x%x, 0x%x]\n",
	      me, (int)sb.reserved[0], (int)sb.reserved[1]);
      ret = LOL_FSCK_ERROR;
  }
  if (sb.num_files > sb.num_blocks) {
      printf("%s: warning, too many (%u) files found\n",
	     me, (unsigned int)(sb.num_files));
	     printf("          Maximum capacity is %u\n",
		    (unsigned int)(sb.num_blocks));
	     ret = LOL_FSCK_ERROR;
  }

  return ret;
} // end lol_fsck_sb
/* ******************************************************************
 * lol_fsck_geom: compare the superblock/metadata information about
 *                the container's actual size.
 *
 ****************************************************************** */
static int lol_fsck_geom(FILE *fp, const char* cont, const char *me) {

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
       lol_fsck_message(me, lol_fsck_corr_super, LOL_FSCK_FATAL);
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
       printf("%s: warning, inconsistent container size (%s)\n", me, size_s);
	     printf("          Should be %s\n", gsize_s);
	     ret = LOL_FSCK_ERROR;
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
	     printf("%s: warning, inconsistent container size (%s)\n", me, gsize_s);
	     printf("          Should not be bigger than %s\n", size_s);
	     ret = LOL_FSCK_ERROR;
	 }
	 else {
           memset ((char *)size_s,  0, 128);
	   stacat((char *)size_s, "device ");
	   stacat((char *)size_s, cont);
	   stacat((char *)size_s, " geometry test passed");
           lol_fsck_message(me, size_s, LOL_FSCK_OK);

	 }

       } // end else not block dev

#else

    ret = LOL_FSCK_FATAL;
    lol_fsck_message(me, lol_fsck_wrong_dev, ret);

#endif
  } // end else regular file

  return ret;
} // end lol_fsck_geom
/* ****************************************************************** */
typedef int (*lol_check_t)(FILE *, const char *, const char *);
typedef struct lol_check_func_t
{

  lol_check_t     func;
         char    *self;
         char   *descr;

} lol_check_func;
/* ****************************************************************** */
static const lol_check_func lol_check_funcs[] =
{
  {lol_fsck_sb,   "header", "  header metadata self consistency:"},
  {lol_fsck_geom, "geometry", "container geometry:"},
  {NULL, NULL, NULL},
};
/* ****************************************************************** */
static const char params[] = "container";
static const char    hlp[] = "       Type 'lol %s -h' for help.\n";
static const char*   lst[] =
{
  "  Example:\n",
  "          lol cc lol.db",
  "          This checks the container \'lol.db\'",
  "          for errors.\n",
  "          Type 'man lol' to read the manual.\n",
  NULL
};
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
    curr = lol_check_funcs[i].self;
    //printf("%s: %s\n", curr, desc);

    rv = lol_check_funcs[i].func(fp, cont, curr);
    if (!(rv))
       lol_fsck_message(curr, desc, LOL_FSCK_OK);
    //lol_align(desc, "[OK]\n", LOL_FSCK_ALIGN);
    if (rv == LOL_FSCK_FATAL) {
       fclose(fp);
       printf("lol %s: container \'%s\' has errors, exiting.\n", me, cont);
       return -1;
    }
    /*
    if (rv == LOL_FSCK_ERROR)
       putchar((int)('\n'));
    */

    i++;

  } // end while funcs

  // error:

  fclose(fp);
  return ret;

} // end lol_cc
