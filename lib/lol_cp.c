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
 $Id: lol_cp.c, v0.30 2016/04/19 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"

*/
/* ************************************************************************** */
#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_SYS_TYPES_H
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif
#endif
#ifdef HAVE_SYS_STAT_H
#ifndef _SYS_STAT_H
#include <sys/stat.h>
#endif
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
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif

/*
 * This program copies file(s) to and from
 * containers.
 * It became quite big but we have to check
 * every little detail because we are copying
 * files and we don't want accidents!
 *
 */
/* ****************************************************************** */
// Test if we are trying to copy to a container
static BOOL copying_to_container(const int argc, const char *dest,
                                 lol_meta *sb, struct stat *st, int *ftof)
{
  char cont[LOL_PATH_BUF];
  lol_pinfo p;
  st->st_size = 1;

  if ((lol_validcont(dest, sb, st))) {
     return 1;
  }
  // If dest is not a container, check
  // if it has form: 'container:/file'
  if (argc != 3) {
     return 0;
  }

  p.fullp = (char *)dest;
  p.cont  = cont;
  p.func  = LOL_CONTPATH;
  if ((lol_pathinfo(&p))) {
     return 0;
  }
  st->st_size = 1;
  if ((lol_validcont(cont, sb, st))) {

     *ftof = 1;
     return  1;
  }
  return 0;
} // end copying_to_container
/* ****************************************************************** */
// Test if we are trying to copy to a directory or a file
static BOOL copying_from_container(const int argc, struct stat *st,
                                   const char *dir, int *exist)
{
  // 'dir' must be either an existing directory
  // or an existing file or a non-existing file
  // 'dir' MUST NOT be a container! -> But we KNOW
  // that already if this function gets called.
  *exist = 0;
  if ((stat(dir, st))) {
      if (argc == 3) {
         return 1;
      }
    return 0;
  } // end if stat
  // So, it exists, then allow only file
  // (again argc MUST be 3) or dir
  if (LOL_ISDIRP) {
      *exist = 1;
      return 1;
  }
  if (LOL_ISREGP) {
    if (argc == 3) {
      *exist = 1;
      return 1;
    }
  } // end if regular file
  return 0;
} // end copying_from_container
/* ****************************************************************** */
#define LOL_TO_NEWREG 1
#define LOL_TO_OLDREG 2
#define LOL_TO_DIR    3
int copy_from_container(const int argc, struct stat *st, int is, char *argv[]) {

 // The source files are in a container.
 // The destination may be a container,
 // normal directory (or a file)

  char temp[LOL_DEFBUF];
  char name[LOL_PATH_BUF];
  struct stat src_st;
  lol_pinfo  p;
  lol_FILE *sf;
  FILE     *df;
  const char  *me = argv[0];
  const int    nf = argc - 1;
  const char *dst = argv[nf];
  const int   len = (int)strlen(dst);
  char      *curr;
  size_t src_size;
  size_t    loops;
  size_t     frac;
  size_t    bytes;
  ino_t       ino;
  int    dest = 0;
#ifdef LOL_INLINE_MEMCPY
  int x;
#endif
  int i, j, k = 0, ow = 0;
  int ret = 0;

  if ((argc < 3) || (len > LOL_PATH_MAX)) {
     return -1;
  }
#ifdef LOL_INLINE_MEMCPY
  for (x = 0; x < len; x++) {
    name[x] = dst[x];
  }
#else
  LOL_MEMCPY(name, dst, len);
#endif

  // Does the destination exist?
  if (!(is)) {
    // If it does not exist, then there must be ONLY 3 args
    // (and It must be a copy from container to a new regular file)
    if (argc != 3) {
        return -1;
    }
    dest = LOL_TO_NEWREG;
    name[len] = '\0';
  } // end if does not exist
  else {
    // It is either an existing file or directory
    if (LOL_ISDIRP) {
        dest = LOL_TO_DIR;
	k = len;
	if (name[k-1] != '/') {
	    name[k++]  = '/';
        }
    }
    else {
      if (LOL_ISREGP) {
        if (argc != 3) {
           return -1;
        }
        else {
	   dest = LOL_TO_OLDREG;
	   name[len] = '\0';
        }
      } // end if regular file
    } // end else
  } // end else

  p.file = &name[k];
  p.func = LOL_FILENAME;

  for (i = 1; i < nf; i++) { // For each file to be copied..

    curr = argv[i];
    ow = 0;
    if ((lol_stat(curr, &src_st))) {
         lol_errfmt2(LOL_2E_INVSRC, me, curr);
         //lol_errfmt2(LOL_2E_CANTREAD, me, curr);
         ret = -1; continue;
    } // end if lol_stat

    ino = (ino_t)src_st.st_rdev;
    // We have source file
    // We need the destination name next
    if (dest == LOL_TO_DIR) {
        p.fullp = curr;
	if ((lol_pathinfo(&p))) {
             lol_errfmt2(LOL_2E_INVSRC, me, curr);
	     // lol_errfmt2(LOL_2E_CANTREAD, me, curr);
             ret = -1; continue;
	}
        // One more check, we don't allow to overwrite
        // containers with this program. If user wants
        // to do so, he/she must use another method
        if ((lol_validcont(name, NULL, NULL))) {
	   if (argc == 3) {
	      lol_errfmt2(LOL_2E_OWCONT, me, name);
              return -1;
	      //lol_errfmt2(LOL_2E_ACDENIED, me, name);
	   }
           continue;
        }
    } // end if dest is directory
    if (!(stat(name, st))) {
       if (ino == st->st_ino) {
	  if (argc == 3) {
	     lol_errfmt2(LOL_2E_ACDENIED, me, name);
	     return -1;
	  }
	  continue;
       }
       ow = 1;
    }
    if ((dest == LOL_TO_OLDREG) || (ow)) {
          // Prompt for replace..
          lol_inffmt2(LOL_2E_OW_PMT, me, name);
          temp[0] = (char)getchar();
          (void)getchar();
          if (temp[0] != 'y') {
	     continue;
          }
    } // end if it exists
    // Dest name is in 'name'
    // Source is in 'curr'
    src_size = (size_t)src_st.st_size;
    if (!(df = fopen(name, "w"))) {
          lol_errfmt2(LOL_2E_CANTCOPY, me, name);
	  ret = -1; continue;
    } // end if fopen failed
    if (!(src_size)) { // If zero size, just close and get next
           fclose(df);
	   continue;
    }
    if (!(sf = lol_fopen(curr, "r"))) {
          fclose(df);
          lol_errfmt2(LOL_2E_CANTREAD, me, curr);
	  ret = -1; continue;
    } // end if fopen failed

    loops = src_size / LOL_DEFBUF;
    frac  = src_size % LOL_DEFBUF;
    bytes = LOL_DEFBUF;

  action:
    for (j = 0; j < loops; j++) {
       if ((lol_fread((char *)temp, bytes, 1, sf)) != 1) {
   	    lol_errfmt2(LOL_2E_CANTREAD, me, curr);
	    ret = -1; frac = 0; break;
       }
       if ((fwrite((char *)temp, bytes, 1, df)) != 1) {
	    lol_errfmt2(LOL_2E_CANTWRITE, me, name);
	    ret = -1; frac = 0; break;
       }
    } // end for j
    if (frac) {
       loops = 1;
       bytes = frac;
       frac = 0;
       goto action;
    } // end if frac

    if (fclose(df)) {
        lol_fclose(sf);
        lol_errfmt1(LOL_1E_IOERR, me);
        return -1;
    }
    if (lol_fclose(sf)) {
        lol_errfmt1(LOL_1E_IOERR, me);
        return -1;
    }
  } // end for i, each file
  return ret;
} // end int copy_from_container
#undef LOL_TO_NEWREG
#undef LOL_TO_OLDREG
#undef LOL_TO_DIR
/* ****************************************************************** */
#define LOL_TYPE_UNDEF 0
#define LOL_TYPE_LOL   1
#define LOL_TYPE_REG   2
int lol_copy_to_container(const int argc, const int ftof,
              const lol_meta *db, const ino_t cont_ino, char *argv[]) {

  struct stat st, sta;
  lol_meta sb;
  char temp[LOL_DEFBUF];
  char  con[LOL_PATH_BUF];
  char file[LOL_PATH_BUF];
  char name[LOL_PATH_BUF];
  lol_pinfo p;
  lol_io_func io;
  lol_FILE *dest;
  lol_open_func  fop = (lol_open_func)&fopen;
  lol_close_func fcl = (lol_close_func)&fclose;
  const int       nf = argc - 1;
  const char     *me = argv[0];
  char         *cont = argv[nf];
  const int      len = (int)strlen(cont);
  const long      bs = db->bs;
  void         *src;
  char        *curr;
  size_t       flen;
  size_t      loops;
  size_t       frac;
  size_t      bytes;
  long     src_size;
  long   src_blocks;
  long  blocks_left = 0;
  int     replacing;
#ifdef LOL_INLINE_MEMCPY
  char *t;
  int x;
#endif
  int  i, j, ret = 0;
  int src_type;

  if ((argc < 3) || (len > LOL_PATH_MAX)) {
     return -1;
  }

  // We know the last arg IS a container,
  // the rest of the args may be files in disk
  // or files in container

  if (ftof) {

     if (argc != 3) {
        return -1;
     }

     p.fullp = cont;
     p.cont  = con;
     p.func  = LOL_CONTPATH;
     if ((lol_pathinfo(&p))) {
         lol_errfmt2(LOL_2E_CANTCOPY, me, cont);
         return -1;
     }
     cont = con;

  } // end if ftof

  blocks_left = lol_free_space(cont, &sb, LOL_SPACE_BLOCKS);
  if (blocks_left < 0) {
      lol_errfmt2(LOL_2E_CORRCONT, me, cont);
      return -1;
  }

  if (ftof) {
#ifdef LOL_INLINE_MEMCPY
     t = argv[nf];
     for (x = 0; x < len; x++) {
        name[x] = t[x];
     }
#else
     LOL_MEMCPY(name, argv[nf], len);
#endif
     name[len] = '\0';
  } // endif ftof


  for (i = 1; i < nf; i++) {

    curr = argv[i];
    replacing = 0;
    src_type = LOL_TYPE_UNDEF;

    if ((stat(curr, &st))) { // What kind of file is this one?

       if ((lol_stat(curr, &st))) {
           lol_errfmt2(LOL_2E_CANTREAD, me, curr);
	   ret = -1; continue;
       }

       else {

           fop = (lol_open_func)&lol_fopen;
           fcl = (lol_close_func)&lol_fclose;
	    io = (lol_io_func)&lol_fread;
            src_type = LOL_TYPE_LOL;

	   if (!(ftof)) {

	     if ((ino_t)st.st_rdev == cont_ino) {
               // User tries to copy like:
	       // 'lol cp cont:/file cont',
               // which does not make sense.
	       // We ignore this copy but for
	       // consistency we make a fake
	       // overwrite prompt...
               lol_inffmt2(LOL_2E_OW_PMT, me, curr);
               temp[0] = (char)getchar();
               (void)getchar();
	       // ...and pass it anyway.
	       continue;
	     }
	     // Create dest name from src name
             p.fullp = curr;
             p.file  = file;
             p.func  = LOL_FILELEN | LOL_FILENAME;
	     if ((lol_pathinfo(&p))) {
                 lol_errfmt2(LOL_2E_INVSRC, me, curr);
                 //lol_errfmt2(LOL_2E_CANTREAD, me, curr);
	         ret = -1; continue;
	     }
	     flen = p.flen;
#ifdef LOL_INLINE_MEMCPY
             for (x = 0; x < len; x++) {
                  name[x] = cont[x];
             }
#else
	     LOL_MEMCPY(name, cont, len);
#endif
             name[len] = ':';
             name[len+1] = '/';

#ifdef LOL_INLINE_MEMCPY
	     t = &name[len+2];
             for (x = 0; x < flen; x++) {
	        t[x] = file[x];
             }
#else
             LOL_MEMCPY(&name[len+2], file, flen);
#endif
	     name[len+flen+2] = '\0';
	   } // end if not ftof
       } // end else it is lol file
    } // end if stat

    else {
       // So, the file exists
       if (LOL_NOTREG) {
	  continue;
       }
       // It is a regular file, we can copy it (maybe)
       fop = (lol_open_func)&fopen;
       fcl = (lol_close_func)&fclose;
       io = (lol_io_func)&fread;
       src_type = LOL_TYPE_REG;
       if (!(ftof)) {
	 // Don't allow overwrite the container!
         if (st.st_ino == cont_ino) {
	    if (argc == 3) { // give warning only if 1 file
	       lol_errfmt2(LOL_2E_OWCONT, me, curr);
	       return -1;
	       //lol_errfmt2(LOL_2E_ACDENIED, me, curr);
	    }
            continue;
         }
         // Create dest name
         if ((lol_fnametolol(curr, cont, name, len))) {
	      // What the heck is the problem with this one?
              lol_errfmt2(LOL_2E_INVSRC, me, curr);
              ret = -1; continue;
         }
       } // end if not ftof
    } // end else stat (file exists)
    // Get source size in bytes and blocks
    src_size = st.st_size;
    LOL_FILE_BLOCKS(src_size, bs, src_blocks);
    // Ok, 'name' should have our dest name now
    // Does the file exist in the container already?
    if (!(lol_stat(name, &sta))) {
        // It is there, prompt if replace..
        lol_inffmt2(LOL_2E_OW_PMT, me, name); // (Replace prompt)
        temp[0] = (char)getchar();
        (void)getchar();
        if (temp[0] != 'y') {
	   continue;
        }
	// Check if user tries to copy like:
	// 'lol cp foo:/file foo:/file'
	// (It would cause file corruption!)
	if ((ino_t)st.st_rdev == cont_ino) {
	  if (st.st_rdev != sta.st_rdev) {
	     lol_error("lol cp: DEBUG trap. inode mismatch [%lu] [%lu]\n",
                       (ULONG)st.st_rdev, (ULONG)sta.st_rdev);
	     return -1;
	  }
	  // Source inode is the same as target
	  // container indode. Is the source also
	  // in a container?
	  if (src_type == LOL_TYPE_LOL) {
	    // Ok, we are ABSOLUTELY sure that this is
	    // intra container copy!
	    if (sta.st_ino == st.st_ino) {
	      // target same as source -> skip
	       continue;
	    } // end if same lol file
	  } // end if src_type
	} // end if intra container copy
	if (bs != sta.st_blksize) {
	    printf("lol cp: DEBUG trap. Different block sizes [%lu] [%lu]\n",
                   (ULONG)bs, (ULONG)sta.st_blksize);
	    return -1;
	}
	// Is there enough room?
        if ((lol_can_replace (src_size, (long)(sta.st_size),
	     blocks_left, bs))) {
	    lol_errfmt2(LOL_2E_NOTROOM, me, name);
            ret = -1; continue;
        } // end if lol_can_replace
	replacing = 1;
    } // end if it exists
    // Additional space check before we open
    // any files. What does the free block
    // counter say about it; do we have
    // enough room for the file?
    if ((src_blocks > blocks_left) && (!(replacing))) {
       // Let's update blocks_left counter
       // I don't trust it counts correctly, yet.. :)
        blocks_left = lol_free_space(cont, &sb, LOL_SPACE_BLOCKS);
        if ((blocks_left < 0) || (sb.nf > sb.nb)) {
             lol_errfmt2(LOL_2E_CORRCONT, me, cont);
             return -1;
        }
        if (!(blocks_left)) {
             lol_errfmt2(LOL_2E_FULLCONT, me, cont);
             return -1;
        }
        if (src_blocks > blocks_left) {
            lol_errfmt2(LOL_2E_NOTROOM, me, curr);
            ret = -1; continue;
        }
        else {
           puts("lol cp: DEBUG trap. blocks_left counter error caught!");
	   puts("(Fixed)");
	   ret = -1;
        }
    } // end if src_blocks
    // Ok, seems fine. Let's open the destination
    if (!(dest = lol_fopen(name, "w"))) {
         lol_errfmt2(LOL_2E_CANTCOPY, me, name);
	 ret = -1; continue;
    } // end if dest open failed
    // If open succeed and we are overwriting a file,
    // then it has alrady been truncated to zero,
    // thus giving us back it's blocks
    if (replacing) {
	blocks_left += src_blocks;
    }
    // If zero size -> just close and continue
    if (!(src_size)) {
        lol_fclose(dest);
	// We lost 1 block for 0 size file
        blocks_left--;
        continue;
    }
    if (!(src = fop(curr, "r"))) {
         lol_fclose(dest);
	 // We lost 1 block because the
	 // source could not been opened
	 // and we were forced to close
	 // the dest, thus the file size
	 // is zero (1 block).
         blocks_left--;
         lol_errfmt2(LOL_2E_CANTREAD, me, curr);
	 ret = -1; continue;
    } // end if src open failed

    loops = src_size / LOL_DEFBUF;
    frac  = src_size % LOL_DEFBUF;
    bytes = LOL_DEFBUF;

  action:
    for (j = 0; j < loops; j++) {
       if ((io((char *)temp, bytes, 1, src)) != 1) {
   	    lol_errfmt2(LOL_2E_CANTREAD, me, curr);
	    frac = 0; ret = -1; break;
       }
       if ((lol_fwrite((char *)temp, bytes, 1, dest)) != 1) {
	    lol_errfmt2(LOL_2E_CANTWRITE, me, name);
	    frac = 0; ret = -1; break;
       }
    } // end for j
    if (frac) {
       loops = 1;
       bytes = frac;
       frac = 0;
       goto action;
    } // end if frac

    if (lol_fclose(dest)) {
        fcl(src);
        lol_errfmt1(LOL_1E_IOERR, me);
        return -1;
    }
    if (fcl(src)) {
       lol_errfmt1(LOL_1E_IOERR, me);
       return -1;
    }

    blocks_left -= src_blocks;
  } // end for files
  return ret;
} // end lol_copy_to_container
#undef LOL_TYPE_UNDEF
#undef LOL_TYPE_LOL
#undef LOL_TYPE_REG
/* ****************************************************************** */
static const char params[] = "<file(s)>  <destination>";
static const char*   lst[] =
{
  "  Example 1:\n",
  "            lol cp lol.db:/memo.txt memo.bak",
  "            This copies the file \'memo.txt\'",
  "            which is inside a container file \'lol.db\'",
  "            to current directory as \'memo.bak\'.\n",
  "  Example 2:\n",
  "            lol cp src/* ~/lol.db",
  "            This copies all the files in the",
  "            directory \'src\' to container file",
  "            \'lol.db\' which is in the home",
  "            directory of the current user.\n",
  "          Type: 'man lol' to read the manual.\n",
  NULL
};
/* ****************************************************************** */
int lol_cp (int argc, char* argv[]) {

  const char   *me = argv[0];
  const char *dest = argv[argc-1];
  struct stat st;
  lol_meta sb;
  int is, ftof = 0;

  /* We may want to copy one of the following ways:

  1. File(s) from disk to container like:              lol cp ../#.txt foo
  2. File(s) from another container to container like: lol cp cont:/f1 cont:/f2 foo
  3. File(s) from container to disk like:              lol cp cont:/f1 cont:/f2 directory
  4. One file from container to disk like:             lol cp cont:/file name (name is a filename, not dir)

  So, we check if last arg is a container, if it is -> must be either 1 or 2 -> do copy

  If the check failed, the next check is that the last arg is either an existing directory
  or an existing file or a non-existing file (3 & 4) -> do copy
  (We also know that _all_ the rest args must be files inside a container)

  */

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
     if (argv[1][0] == '-') {
	if ((stat(argv[1], &st))) {
           lol_errfmt2(LOL_2E_OPTION, me, argv[1]);
	   lol_ehelpf(me);
           return -1;
	}
     }
  } // end if argc == 2
  if (argc < 3) {
      lol_show_usage(me);
      puts  ("       Copies file(s) to and from a container.");
      lol_helpf(me);
      return 0;
  }
  if ((copying_to_container(argc, dest, &sb, &st, &ftof))) {
       return lol_copy_to_container(argc, ftof, &sb, st.st_ino, argv);
  }
  if ((copying_from_container(argc, &st, dest, &is))) {
      return copy_from_container(argc, &st, is, argv);
  }

  lol_syntax(me);
  lol_ehelpf(me);
  return -1;
} // end lol_cp
