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
 $Id: lol_ls.c, v0.30 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_STDIO_H
#ifndef _STDIO_H
#include <stdio.h>
#endif
#endif
#ifdef HAVE_STRING_H
#ifndef _STRING_H
#include <string.h>
#endif
#endif
#ifdef HAVE_TIME_H
#ifndef _TIME_H
#include <time.h>
#endif
#endif
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ****************************************************************** */
static const char   params[] = "<container>";
static const char*     lst[] =
{
  "  Example:\n",
  "          lol ls lol.db",
  "          This lists the files which are inside",
  "          container file \'lol.db\'\n",
  "          Type: 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
static void strip_time(char* t) {
  int i, ln;
  if (!(t))
    return;
  ln = (int)strlen(t);
  for(i = 0; i < ln; i++) {
    if ((t[i] == '\n') || (t[i] == '\r')) {
         t[i] = '\0';
      break;
    }
  }
} // end strip_time
/* ****************************************************************** */
#define LOL_LS_TEMP 256
int lol_ls(int argc, char* argv[])
{
  const long nes = (long)NAME_ENTRY_SIZE;
  const long temp_mem = nes * LOL_LS_TEMP;
  const char  *me = argv[0];
  char *co, *time, tmp[128];
  char       temp[temp_mem];
  struct stat st;
  lol_meta    sb;
  size_t     ln, mem = 0;
  lol_nentry    *buf = 0;
  lol_nentry  *entry = 0;
  FILE           *fp = 0;
  long     data_size = 0;
  long         times = 0;
  long nf = 0, files = 0;
  long      i = 0, k = 0;
  long    io = 0, nb = 0;
  long  frac = 0, bs = 0;
  long          corr = 0;
  int          alloc = 0;
  int     n = 0, err = 0;
  int      j = 0, sp = 0;
  int           ret = -1;

  // Process standard --help & --version options.
  if (argc == 2) {
    if (LOL_CHECK_HELP) {
        lol_show_usage(me);
        lol_help(lst);
        return 0;
    }
    if (LOL_CHECK_VERSION) {
        lol_show_version(me);
	return 0;
    }
    if (LOL_CHECK_SILENT) {
        lol_errfmt2(LOL_2E_ARGMISS, me, params);
        return -1;
    }
    if (argv[1][0] == '-') {
      if ((stat(argv[1], &st))) {
         lol_errfmt2(LOL_2E_OPTION, me, argv[1]);
	 lol_ehelpf(me);
         return -1;
      }
    }
  } // end if argc == 2

  if (argc != 2) {
        lol_show_usage(me);
	puts  ("       Lists files inside a container.");
        lol_helpf(me);
        return 0;
  }
  co = argv[1];
  if (!(lol_validcont(co, &sb, NULL))) {
       lol_errfmt2(LOL_2E_CORRCONT, me, co);
       lol_errfmt(LOL_0E_USEFSCK);
       return -1;
  }

  nf = (long)sb.nf;
  nb = (long)sb.nb;
  bs = (long)sb.bs;

  data_size  = nes * nb;
  io = lol_get_io_size(data_size, nes);
  if (io <= 0) {
      lol_debug("lol_ls: Internal error: io <= 0");
      return -1;
  }

  if (io > temp_mem) {
    if (!(buf = (lol_nentry *)lol_malloc((size_t)(io)))) {
          buf = (lol_nentry *)temp;
          io  = temp_mem;
    } else {
       mem = (size_t)(io);
       alloc = 1;
    }
  } else {
    buf = (lol_nentry *)temp;
    io  = temp_mem;
  }

  times = data_size / io;
  frac  = data_size % io;
  k = io / nes;

  if (!(fp = fopen(co, "r"))) {
       lol_errfmt2(LOL_2E_CANTREAD, me, co);
       goto just_free;
  }
  i = LOL_DENTRY_OFFSET_EXT(nb, bs);
  if (fseek (fp, i, SEEK_SET)) {
      lol_errfmt2(LOL_2E_CANTREAD, me, co);
      goto closefree;
  }

 dentry_loop:
  for (i = 0; i < times; i++) {

    if ((lol_fio((char *)buf, io, fp, LOL_READ)) != io) {
         lol_errfmt2(LOL_2E_CANTREAD, me, co);
         goto closefree;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...

       entry = &buf[j];
       if (!(entry->name[0]))
	   continue;

       files++;
       err = 0;
       if ((entry->i_idx < 0) || (entry->i_idx >= nb)) {
	    err = 1; corr++;
       }
       ln = strlen((char *)entry->name);
       if (ln > LOL_FILENAME_MAXLEN) {
           entry->name[LOL_FILENAME_MAXLEN] = '\0';
           err = 1; corr++;
       }
       time = ctime(&entry->created);
       if (time) {

          strip_time(time);
          printf("%s", time);
          ln = strlen(time);
          sp = 30 - ln;
          if (sp < 4)
	      sp = 4;
          for (n = 0; n < sp; n++) {
               LOL_SPACE();
          }
          // Append file size;
          memset(tmp, 0, 128);
          sprintf(tmp, "%lu", entry->fs);
          printf("%s", tmp);
          ln = strlen(tmp);
          sp = 16 - ln;
          if (sp < 4)
	      sp = 4;
          for (n = 0; n < sp; n++) {
              LOL_SPACE();
          }
          printf("%s", entry->name);
          if (err) {
             puts ("  w");
          }
	  else {
             puts("");
          }
      } // end if time
       else {
          printf("01-Jan 00:00:00 1970          %lu     %s  w\n",
		 entry->fs, entry->name);
	  corr++;
       }
       if (files >= nf)
	 goto ooloop;
    } // end for j
  } // end for i
  // Now the fractional data
  if (frac) {
      times = 1;
      io = frac;
      frac = 0;
       k = io / nes;
      goto dentry_loop;
  } // end if frac

ooloop:
  ret = 0;

closefree:
 fclose(fp);
just_free:
 if (alloc) {
    lol_free(mem);
 }

 if (!(ret)) {

    if (corr) {
       lol_errfmt2(LOL_2E_CORRCONT, me, co);
       lol_errfmt(LOL_0E_USEFSCK);
    }
    else {
      printf("total %ld\n", files);
    }

 } // end if !ret

 return ret;
} // end lol_ls
#undef LOL_LS_TEMP
