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
*/
/*
 $Id: lol_rs.c, v0.20 2016/11/02 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************
 * lol_rs: resize a container file.
 * Does not shrink, only adds more space.
 * ********************************************************************** */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_ERRNO_H
#ifndef _ERRNO_H
#include <errno.h>
#endif
#endif
#ifdef HAVE_LIMITS_H
#ifndef _LIMITS_H
#include <limits.h>
#endif
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
#ifdef HAVE_CTYPE_H
#ifndef _CTYPE_H
#include <ctype.h>
#endif
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
static const char
       lol_overwrite_prompt[] = "                       Overwrite [y/n]? ";
static const char
 lol_proceed_prompt[] = "                                     Proceed [y/n]? ";
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
  char nsize[64];
  struct stat         st;
  lol_meta            sb;

  char           *me = 0;
  char         *cont = 0;
  char       *option = 0;
  char       *amount = 0;
  DWORD           nb = 0;
  DWORD           bs = 0;
  DWORD         news = 0;
  size_t         len = 0;
  long         space = 0;
  int            ret = 0;
  int            val = 0;
  int            opt = 0;
  char            ch = 0;

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
	    lol_error(lol_help_txt, me);
            return -1;
        }
syntax_err:
        printf (LOL_USAGE_FMT, me,
                lol_version, lol_copyright, me, params);
        printf (lol_help_txt, me);
        return -1;
    }
  } // end if argc == 2
  if (argc != 4) {
      lol_error (LOL_USAGE_FMT, me,
                lol_version, lol_copyright, me, params);
      lol_error (lol_help_txt, me);
      return -1;
  }

  option = argv[1];
  amount = argv[2];
    cont = argv[3];

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
          lol_error (lol_help_txt, me);
          return -1;
      }
      // Syntax error...
      goto syntax_err;

    } // end else not blocks or size

  } // end else
  // Does the container exist?
  space = lol_get_vdisksize(cont, &sb, &st, RECUIRE_SB_INFO);
  if (space < LOL_THEOR_MIN_DISKSIZE) {
      lol_error(lol_cantuse_txt, me, cont);
      return -1;
  }
  if (LOL_INVALID_MAGIC) {
      lol_error(LOL_IMAGIC_FMT, me, LOL_MAG_0, LOL_MAG_1);
      lol_error(lol_usefsck_txt);
      return -1;
  }

  nb = sb.nb;
  bs = sb.bs;

  if ((!(nb)) || (!(bs))) {
      lol_error(lol_cantuse_txt, me, cont);
      return -1;
  }

  // Seems like a valid container, let's see what we'll do.
  // We only add blocks, so if user wants to add size,
  // (for example 200 Mb), we convert it to blocks first...

  switch (opt) {

     case LOL_RS_SIZE :

       if ((lol_size_to_blocks(amount, cont,
            &sb, &st, &news, LOL_EXISTING_FILE))) {
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
	  news = (DWORD)(ret);
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
  // Ok, news has the additional blocks now
  if (opt == LOL_RS_BLOCKS) {
      printf("lol %s: this will add %s blocks to container \'%s\'\n",
	   me, amount, cont);
  }
  else {
      len = strlen(amount);
      ch = amount[len - 1];

      if ((lol_is_number(ch))) {
#ifdef HAVE_CTYPE_H
	 amount[len - 1] = (char)toupper((int)(ch));
#endif
         printf("lol %s: this will add %s space to container \'%s\'\n",
	         me, amount, cont);
      }
      else {
         printf("lol %s: this will add %s Bytes to container \'%s\'\n",
	         me, amount, cont);
      }
  }

  if ((prompt_lol_rs(lol_proceed_prompt))) {
       puts("Aborted.");
       return 0;
  } // end if
#if LOL_TESTING
   lol_error("news = %u - calling extendfs\n", news);
   lol_error("bs = %u, nb = %u, st.st.size = %ld\n", sb.bs, sb.nb, (long)(st.st_size));
   return -1;
#endif

   // We have green light, let's begin expanding
  if ((lol_extendfs(cont, news, &sb, &st))) {
       lol_error("lol %s: error extending container.\n", me);
       return -1;
  } // end if error

  space = (long)nb;
  space += news;
  space *= bs;
  memset((char *)nsize, 0, 64);
  lol_ltostr(space, nsize);
  printf("Done, new size is %s\n", nsize);

  return 0;

} // end lol_rs
