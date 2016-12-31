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
 $Id: lol_rs.c, v0.20 2016/11/02 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************
 * lol_rs: resize a container file.
 * Does not shrink, only adds more space.
 * ********************************************************************** */
#ifndef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifndef _ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif
#ifndef _STRING_H
#include <string.h>
#endif
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ********************************************************************** */
static const char params[] =
          "-b <blocks> <container>\n       lol rs -s <size> <container>";
static const char    hlp[] = "       Type: \'lol %s -h\' for help.\n";
static const char*   lst[] =
{
  "  Example 1:\n",
  "          lol rs -b 1000 lol.db",
  "          This adds 1000 new data blocks to container file \'lol.db\'.\n",
  "  Example 2:\n",
  "          lol rs -s 400k lol.db",
  "          This adds 400 Kilobytes new space to container file \'lol.db\'.\n",
  "          Type: \'man lol\' to read the manual.\n",
  NULL
};
/* ********************************************************************** */
static const char  cantuse[] = "lol %s: cannot use container %s\n";
static const char
       lol_overwrite_prompt[] = "                       Overwrite [y/n]? ";
static const char
 lol_proceed_prompt[] = "                                     Proceed [y/n]? ";
/* ************************************************************************ */
/* ************************************************************************ */
static int prompt_lol_rs(const char *txt) {
  char answer;

  if(!(txt))
    return -1;

       printf("%s", txt);
       answer = (char)getchar();
       (void)getchar();
       if (answer != 'y')
	 return -1;

   return 0;
} // end prompt_lol_rs
/* ************************************************************************ */
int lol_rs (int argc, char* argv[])
{

  struct stat st;
  struct lol_super sb;
  DWORD bs;
  DWORD nb;
  DWORD   new_blocks = 0;
  size_t         len = 0;
  long    free_space = 0;
  char           *me = 0;
  char       *option = 0;
  char       *amount = 0;
  char    *container = 0;
  int            ret = 0;
  int            val = 0;
  int            opt = 0;
  char           ch  = 0;

  me = argv[0];

  // Process standard --help & --version options.
  if (argc == 2) {
    if (LOL_CHECK_HELP) {
         printf (LOL_USAGE_FMT, me,
                 lol_version, lol_copyright, me, params);
	 lol_help(lst);
         return 0;

    }
    if (LOL_CHECK_VERSION) {
	printf (LOL_VERSION_FMT, me,
                lol_version, lol_copyright);
	return 0;
    }
    if (LOL_CHECK_SIZE) {
        lol_error(LOL_MISSING_ARG_FMT, me, "<size> <container>");
        return -1;
    }
    if (LOL_CHECK_BLOCKS) {
        lol_error(LOL_MISSING_ARG_FMT, me, "<blocks> <container>");
        return -1;
    }


    if (argv[1][0] == '-') {

        if ((stat(argv[1], &st))) {
            lol_error(LOL_WRONG_OPTION, me, argv[1]);
	    lol_error(hlp, me);
            return -1;
        }
syntax_err:
        printf (LOL_USAGE_FMT, me,
                lol_version, lol_copyright, me, params);
        printf (hlp, me);
        return -1;
    }

  } // end if argc == 2

  if (argc != 4) {
        lol_error (LOL_USAGE_FMT, me,
                lol_version, lol_copyright, me, params);
        lol_error (hlp, me);
     return -1;
  }

     option = argv[1];
     amount = argv[2];
  container = argv[3];

  // Does user want to add bytes or blocks?
  if (LOL_CHECK_BLOCKS) {

     opt = LOL_RS_BLOCKS;
     // Check also that amount is a number
     // strtol converts "1000M" to 1000 for example
     // and this may cause confusion
     if ((lol_is_integer((const char *)amount))) {

         lol_error("lol %s: number of blocks must be integer\n", me);
         lol_error("        Please try again");
         return -1;

     } // end if not integer

  } // end if add blocks
  else {

    if (LOL_CHECK_SIZE) {

        opt = LOL_RS_SIZE;

    } // end if add size
    else{
      // Maybe wrong option?
      if (option[0] == '-') {
          lol_error(LOL_WRONG_OPTION, me, option);
          lol_error (hlp, me);
          return -1;
      }
      // Syntax error...
      goto syntax_err;

    } // end else not blocks or size

  } // end else

  // Does the container exist?
  free_space = lol_get_vdisksize(container, &sb, &st, RECUIRE_SB_INFO);
  if (free_space < LOL_THEOR_MIN_DISKSIZE) {
      lol_error(cantuse, me, container);
      return -1;
  }

  if (LOL_INVALID_MAGIC) {
      lol_error("lol %s: invalid file id [0x%x, 0x%x].\n",
	         me, (int)sb.reserved[0], (int)sb.reserved[1]);
      lol_error(LOL_FSCK_FMT);
      return -1;
  }

  nb = sb.num_blocks;
  bs = sb.block_size;

  if ((!(nb)) || (!(bs))) {
      lol_error(cantuse, me, container);
      return -1;
  }

  // Seems like a valid container, let's see what we'll do

  // We only add blocks, so if user wants to add size,
  // (for example 200 Mb), we convert it to blocks first...

  switch (opt) {

     case LOL_RS_SIZE :

       if ((lol_size_to_blocks(amount, container,
            &sb, &st, &new_blocks, LOL_EXISTING_FILE))) {
	   val = -1;
       }
       break;

    case LOL_RS_BLOCKS :

      ret = strtol(amount, NULL, 10);
      if (errno)
        val = -1;
#ifdef HAVE_LIMITS_H
#if defined LONG_MIN && defined LONG_MAX
       if ((ret == LONG_MIN) || (ret == LONG_MAX))
           val = -1;
#endif
#endif

      if (ret < 1)
          val = -1;
      if (!(val))
	  new_blocks = (DWORD)(ret);

      break;

    default :
      lol_error("lol %s: %s\n", me, LOL_INTERERR_FMT);
      return -1;

  } // end switch opt

  if (val) {

      lol_error("lol %s: invalid filesize \'%s\'\n", me, amount);
      if (opt == LOL_RS_SIZE)
	lol_error("The minimum size must be at least 1 block (%ld bytes)\n",
                (long)bs);
	 return -1;

  } // end if val

  // Ok, new_blocks has the additional blocks now

  if (opt == LOL_RS_BLOCKS) {
      printf("lol %s: this will add %s blocks to container \'%s\'\n",
	   me, amount, container);
  }
  else {
      len = strlen(amount);
      ch = amount[len - 1];
      if ((lol_is_number(ch)))
         printf("lol %s: this will add %s space to container \'%s\'\n",
	         me, amount, container);
      else
         printf("lol %s: this will add %s bytes to container \'%s\'\n",
	         me, amount, container);
  }

   if ((prompt_lol_rs(lol_proceed_prompt))) {
       puts("Aborted.\n");
   } // end if
#if LOL_TESTING
   lol_error("new_blocks = %u - calling extendfs\n", new_blocks);
   lol_error("bs = %u, nb = %u, st.st.size = %ld\n", sb.block_size, sb.num_blocks, (long)(st.st_size));
   return -1;
#endif

   // We have green light, let's begin expanding
   if ((lol_extendfs(container, new_blocks, &sb, &st))) {
        lol_error("lol %s: error extending container.\n", me);
        return -1;
   } // end if error

  puts("Done");
  return 0;

} // end lol_rs

