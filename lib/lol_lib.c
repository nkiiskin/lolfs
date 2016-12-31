/*
 *
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
 * $Id: lol_lib.c, v0.20 2017/01/01 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
 *
 *
 */


#ifndef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifndef _ERRNO_H
#include <errno.h>
#endif
#ifndef _STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_LINUX_FS_H
#ifndef _LINUX_FS_H
#include <linux/fs.h>
#endif
#endif
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* **********************************************************
 *  INTERFACE FUNCTION
 *  lol_fopen:
 *
 *  Opens a file inside a container for
 *  reading and/or writing.
 *
 *  The path must be given in form "/path/container:/filename".
 *  NOTE: There MUST be ':' separating the container
 *        from the actual file inside the container.
 *
 *  Return value:
 *
 *  NULL      : if error
 *  lol_FILE* : if success, returns the file handle.
 *
 * ********************************************************** */
lol_FILE *lol_fopen(const char *path, const char *mode)
{
  struct stat st;
  lol_FILE *op       = 0;
  int       r        = 0;
  int       is       = 0;
  int       mod      = 0;
  int       path_len = 0;
  int       mode_len = 0;
  int       trunc    = 0;
  long      size     = 0;
  ULONG     fs       = 0;

  if ((!path) || (!mode)) {
    lol_errno = EINVAL;
    return NULL;
  }

  path_len = strlen(path);
  mode_len = strlen(mode);

  if ((path_len < 4) || (path_len > LOL_PATH_MAX)) {
       lol_errno = ENOENT;
       return NULL;
  }

  if ((mode_len <= 0) || (mode_len > 4)) {
      lol_errno = EINVAL;
      return NULL;
  }

  if (!(op = new_lol_FILE())) {
       lol_errno = ENOMEM;
       return NULL;
  }

  mod = op->open_mode.mode_num = lol_getmode(mode);

  if ((mod < 0) || (mod >= MAX_LOL_OPEN_MODES)) {
       lol_errno = EINVAL;
       delete_return_NULL(op);
  }

  strcpy(op->open_mode.mode_str, lol_open_modes[mod].mode_str);
  strcpy(op->open_mode.vd_mode,  lol_open_modes[mod].vd_mode);

  if (lol_get_filename(path, op)) {
      // lol_errno already set
      delete_return_NULL(op);
  }

  size = lol_get_vdisksize(op->vdisk_name, &op->sb, &st, RECUIRE_SB_INFO);

  if (size < LOL_THEOR_MIN_DISKSIZE) {
      lol_errno = EIO;
      delete_return_NULL(op);
  }

  if (LOL_CHECK_MAGIC(op)) {
      lol_errno = EIO;
      delete_return_NULL(op);
  }

  op->vdisk_size = (ULONG)size;
  op->open_mode.device = st.st_mode;

  if(!(op->vdisk = fopen(op->vdisk_name, op->open_mode.vd_mode))) {
      lol_errno = EINVAL;
      delete_return_NULL(op);
  }

  is = lol_read_nentry(op);  // Can we find the dir entry?
  fs = op->nentry.file_size;

  switch (is) {  // What we do here depends on open_mode

          case  LOL_NO_SUCH_FILE:

           // Does not exist, so we return NULL
           // if trying to read (cases "r" & "r+")

		if (mod < 2)  { /* if "r" or "r+", cannot read.. */
		    lol_errno = ENOENT;
                    close_return_NULL(op);
		}

	        op->curr_pos = 0;
                // if not reading,
		// then it must be w/a, create new file...
	        if ((r = lol_touch_file(op))) {
		// lol_errno already set
                    close_return_NULL(op);
		}

                break;

            case LOL_FILE_EXISTS:

            // The file exists. In this case the
	    // op->nentry has the file info
	    // and we may both read and write

                  switch (mod) {

	                  case LOL_RDONLY:
		          case LOL_RDWR:

		               op->curr_pos = 0;

                               break;

             // "w(+)" The file will be truncated to zero
	                   case LOL_WR_CREAT_TRUNC:
 	                   case LOL_RDWR_CREAT_TRUNC:

		                 op->curr_pos = 0;
		                 trunc = 1;

                                 break;

                            case LOL_APPEND_CREAT:
              // "a" -> append

                                 op->curr_pos = fs;
                                 break;

	                    case LOL_RD_APPEND_CREAT:
              // "a+" -> FIXME: Same as "a" here!

		                 op->curr_pos = fs;
                                 break;

		            default:

                                 lol_errno = EINVAL;
                                 close_return_NULL(op);

	              } // end switch mod

                      break;

               default:

                    lol_errno = EIO;
                    close_return_NULL(op);


  } // end switch file exists

  if (trunc) { // Truncate if file was opened "w" or "w+"

      if ((r = lol_truncate_file(op))) {
	  lol_errno = EIO; // TODO: FIX! Check return value
          close_return_NULL(op);
      }

      op->curr_pos = 0;

  } // end if truncate

  lol_errno  = 0;
  op->opened = 1;

#if 0
  // Check consistency just in case..
  if ((lol_check_corr(op, LOL_CHECK_BOTH))) {
       lol_errno = EIO;
       close_return_NULL(op);
  }
#endif

  return op;
} // end lol_fopen
/* **********************************************************
 *  INTERFACE FUNCTION
 *  lol_fclose:
 *
 *  Closes a previously opened (by lol_fopen) lol-file.
 *  Upon successful completion 0 is returned. Otherwise, EOF
 *  is returned and errno  is  set  to indicate  the error.
 *
 *  Return value:
 *
 *  EOF : if error
 *  0   : if success
 *
 * ********************************************************** */
int lol_fclose(lol_FILE *op)
{
  int ret = 0;
  int i = 10;

  if (!(op)) {
       lol_errno = EBADF;
       return EOF;
   }
   if (op->vdisk) {
      do { /* Try to close 10 times */

           ret = fclose(op->vdisk);
	   i--;

	   if (ret) {
	      lol_errno = errno;
	   }
	   else {
              op->vdisk = NULL;
	      lol_errno = 0;
	   }

       } while ((i) && (ret));
   }
   else {
      lol_errno = EBADF;
      ret = EOF;
   }

   delete_lol_FILE(op);
   return ret;
} // end lol_fclose
/* **********************************************************
 *
 *  lol_feof:
 *  Return non-zero if end-of-lol-file indicator is set.
 *
 * ***********************************************************/
int lol_feof(lol_FILE *op) {
  if (!(op)) {
    lol_errno = EBADF;
    return -1;
  }
  return (int)(op->eof);
}
/* **********************************************************
 *  INTERFACE FUNCTION
 *  lol_fread:
 *
 *  Read data from the opened file stream to user buffer.
 *
 * The function lol_fread reads nmemb items of data, each
 * size bytes long, from the stream pointed to by op, storing
 * them at the location given by ptr.
 * On success, lol_fread returns the number of items read.
 * lol_fread does not distinguish between end-of-file and error,
 * and callers must use lol_feof(3) and lol_ferror(3) to determine
 * which occurred in case 0 is returned.
 * 
 * Return value:
 *
 *  if error   : 0 or items succesfully read
 *  if success : items succesfully read (This should match nmemb).
 *
 * ********************************************************** */
size_t lol_fread(void *ptr, size_t size, size_t nmemb, lol_FILE *op)
{

  struct lol_loop loop;
  alloc_entry current_index = 0;

  size_t file_size       = 0;
  size_t block_size      = 0;
  size_t left_to_read    = 0;
  size_t mem, i = 0, off = 0;
  size_t start_bytes     = 0;
  size_t end_bytes       = 0;
  size_t block_loops     = 0;
  long   amount          = 0;
  int    ret             = 0;

  if (!(ptr))
    LOL_ERR_RETURN(EINVAL, 0);

  if (!(op))
    LOL_ERR_RETURN(EBADF, 0);

  if (!(size))
    return nmemb;

  if (!(nmemb))
    return 0;

  if (lol_valid_sb(op))
      LOL_ERR_RETURN(ENFILE, 0);

  if (op->eof)
   return LOL_ERR_EOF;

  file_size   = (size_t)op->nentry.file_size;
  block_size  = (size_t)op->sb.block_size;
  left_to_read = file_size - op->curr_pos;

  if (!(left_to_read)) {

       op->eof = 1;
       return LOL_ERR_EOF;
  }

  if ((!(op->vdisk)) || (op->opened != 1))
      LOL_ERR_RETURN(EBADFD, 0);

  switch (op->open_mode.mode_num) {

      case             LOL_RDWR :
      case           LOL_RDONLY :
      case  LOL_RD_APPEND_CREAT :
      case LOL_RDWR_CREAT_TRUNC :

        break;

    default:

      op->err = lol_errno = EPERM;
      return 0;

  } // end switch

  current_index = op->nentry.i_idx;
  if ((current_index < 0) || (current_index >= op->sb.num_blocks))
    LOL_ERR_RETURN(ENFILE, 0);

  if (!(file_size)) {
        op->eof = 1;
        return LOL_ERR_EOF;
  }

  if (op->curr_pos > file_size) {
      op->eof = 1;
      return LOL_ERR_EOF;
  }

  // Ok, we actually have some data to read
  // Can we read the whole amount?
   amount = size * nmemb;

  if (amount > left_to_read)
          amount = left_to_read; // This is the amount of bytes we will read

   mem = lol_num_blocks(op, amount, &loop);

   if (lol_index_malloc(mem))
       LOL_ERR_RETURN(ENOMEM, 0);

   ret = lol_read_ichain(op, mem);
#if LOL_TESTING
   printf("DEBUG: lol_fread: index_buffer has %d indexes\n", ret);
#endif

   if ((ret <= 0) || (ret != mem)) {
#if LOL_TESTING
     printf("DEBUG: lol_fread: ret = %d, mem = %ld (should be equal)\n", ret, (long)mem);
#endif


       lol_index_free(mem);
       if ((!(lol_errno))  && (!(op->err)))
	   LOL_ERRSET(EIO);
       return 0;
    }

  start_bytes = loop.start_bytes;
  block_loops = loop.full_loops;
  end_bytes   = loop.end_bytes;

  i = 0;

  if (end_bytes) {

         current_index = lol_index_buffer[i++];

	 if (current_index < 0) {
	     // This should not happen, unless corrupted container file
	     LOL_ERRSET(EFAULT);
	     goto error;
      	 }

        amount = lol_io_dblock(op, current_index, ptr, end_bytes, LOL_READ);
	if (amount < 0) {
	  //off = 0;
	    goto error;
        }

	off += amount;
	if (amount != end_bytes) {
	    amount = -1;
	    goto error;
        }

  } // end if end_bytes

  block_loops += i;

  // Process the full blocks

      for (; i < block_loops; i++) {

           current_index = lol_index_buffer[i];

	   if (current_index < 0) {
	   // This should not happen, unless corrupted container
	      LOL_ERRSET(EFAULT);
	      goto error;
	   }

	   amount = lol_io_dblock(op, current_index, ptr+off, block_size, LOL_READ);
           if (amount < 0) {
	     //off = 0;
	       goto error;
           }

	   off += amount;

	   if (amount != block_size) {
	       amount = -1;
	       goto error;
           }

     } // end for i

     if (start_bytes) {

         current_index = lol_index_buffer[i];

	 if (current_index < 0) {
	  // This should not happen, unless corrupted container
	  LOL_ERRSET(EFAULT);
	  goto error;
                          
	 }

	 amount = lol_io_dblock(op, current_index, ptr + off, start_bytes, LOL_READ);

         if (amount < 0) {
	  // printf("lol_fread: start_bytes: ERROR lol_io_dblock %d\n", (int)amount);
	  //   off = 0;
	     goto error;
         }

	 off += amount;

	if (amount != start_bytes) {
	    amount = -1;
	    goto error;
        }

    } // end if start bytes


  off /= size;

error:

  lol_index_free(mem);

  if (op->curr_pos > file_size) {
      op->eof = 1;
  }

#if 0
  if (off < nmemb) {
      op->eof = 1;
  }
#endif

  if ((!(off)) && (!(op->err)))
       LOL_ERRSET(EIO);

  if (amount < 0) {
    off /= size;
    if (!(op->err))
       LOL_ERRSET(EIO);
  }

  return off;

} // end lol_fread
/* **********************************************************
 *  INTERFACE FUNCTION
 *  lol_fwrite:
 *
 *  Writes data from user buffer (ptr) to opened file stream (op).
 *
 * The function lol_fwrite writes nmemb items of data, each size
 * bytes long, obtaining them from the user buffer given by ptr,
 * storing them at the file stream pointed to by op.
 *
 * On success, lol_fwrite returns the number of items written.
 * If an error occurs, the return value is a short item
 * count (or zero). If zero is returned, callers should use
 * lol_feof(3) and lol_ferror(3) to determine what occurred.
 * 
 * Return value:
 *
 *  if error   : 0 or items succesfully written
 *  if success : items succesfully written (This should match nmemb).
 *
 * ********************************************************** */
size_t lol_fwrite(const void *ptr, size_t size, size_t nmemb, lol_FILE *op)
{

  struct lol_loop loop;
  alloc_entry current_index;
  alloc_entry last_old;
  long olds, mids, news;

  long new_filesize     = 0;
  long filesize         = 0;
  long write_blocks     = 0;
  long amount           = 0;
  size_t block_size     = 0;
  size_t i = 0,     off = 0;
  size_t start_bytes    = 0;
  size_t end_bytes      = 0;
  size_t block_loops    = 0;
  int  ret              = 0;

  if (!(ptr))
    LOL_ERR_RETURN(EINVAL, 0);

  if (!(op))
    LOL_ERR_RETURN(EBADF, 0);

  if (!(size))
    return nmemb;

  if (!(nmemb))
    return 0;

  if ((lol_valid_sb(op)))
      LOL_ERR_RETURN(ENFILE, 0);

  filesize    = (long)op->nentry.file_size;
  block_size  = (size_t)op->sb.block_size;

  if ((!(op->vdisk)) || (op->opened != 1))
       LOL_ERR_RETURN(EBADFD, 0);


  switch (op->open_mode.mode_num) {

      case             LOL_RDWR :
      case     LOL_APPEND_CREAT :
      case   LOL_WR_CREAT_TRUNC :
      case  LOL_RD_APPEND_CREAT :
      case LOL_RDWR_CREAT_TRUNC :

        break;

    default:
      op->err = lol_errno = EPERM;
      return 0;

  } // end switch

  current_index = op->nentry.i_idx;
  if ((current_index < 0) || (current_index >= op->sb.num_blocks))
    LOL_ERR_RETURN(ENFILE, 0);

  if ((lol_is_writable(op)))
      LOL_ERR_RETURN(EPERM, 0);

  amount = size * nmemb;

  // Compute new indexes

  write_blocks = lol_new_indexes(op,
                 (long)amount, &olds, &mids,
                 &news, &new_filesize);

  // Translate our errors..
  if (write_blocks <= 0) {

      switch (write_blocks) {

           case LOL_ERR_EOF :
	     return 0; // We do NOT set op->err here!

           case LOL_ERR_USER :
                op->err = lol_errno = EFAULT;
               return 0;

           case LOL_ERR_CORR :
                op->err = lol_errno = EIO;
               return 0;

           default :
#if LOL_TESTING
	     puts ("DEBUG: lol_fwrite err 1");
#endif
                op->err = lol_errno = EIO;
               return 0;
      } // end swith

   } // end if error

  if ((lol_index_malloc((size_t)(write_blocks))))
       LOL_ERR_RETURN(ENOMEM, 0);

    ret = lol_new_ichain(op, olds, news, &last_old);
    if (ret < 0) {

	lol_index_free((size_t)(write_blocks));
	if (!(op->err)) {
#if LOL_TESTING
	  printf("DEBUG: lol_fwrite: lol_new_ichain failed, ret = %d\n", (int)(ret));
#endif
	  op->err = lol_errno = EIO;
	}

        return 0;
    }

  // Ready to write...

   i = mids;
   lol_num_blocks(op, amount, &loop);

   start_bytes = loop.start_bytes;
   block_loops = loop.full_loops;
   end_bytes   = loop.end_bytes;


  if (end_bytes) {

      current_index = lol_index_buffer[i++];
      if (current_index < 0) {
	  LOL_ERRSET(EFAULT);
	  goto error;
      }

      amount = lol_io_dblock(op, current_index, (char *)ptr, end_bytes, LOL_WRITE);

      if (amount < 0) {
	//	  off = 0;
	  goto error;
      }

      off += amount;
      
	if (amount != end_bytes) {
	    amount = -1;
	    goto error;
        }
      

  } // end if end_bytes

  block_loops += i;

  // Process the full blocks

   for (; i < block_loops; i++) {

        current_index = lol_index_buffer[i];

	if (current_index < 0) {
	// This should not happen, unless corrupted file
			  
	   LOL_ERRSET(EFAULT);
	   goto error;
        }

	amount = lol_io_dblock(op, current_index,
                 (char *)(ptr+off), block_size, LOL_WRITE);

        if (amount < 0) {
	  //    off = 0;
	    goto error;
        }

	off += amount;
	
	if (amount != block_size) {
	    amount = -1;
	    goto error;
        }
	
    } // end for i

    if (start_bytes) {

        current_index = lol_index_buffer[i];

	if (current_index < 0) {
	    LOL_ERRSET(EFAULT);
	    goto error;
        }

	amount = lol_io_dblock(op, current_index,
                 (char *)ptr + off, start_bytes, LOL_WRITE);

        if (amount < 0) {
	  //    off = 0;
	    goto error;
        }

        off += amount;

	if (amount != start_bytes) {
	    amount = -1;
	    goto error;
        }

  } // end if start bytes

  off /= size;

    // MUST ALSO WRITE MODIFIED CHAIN !

     ret = 0;
     op->nentry.file_size = (ULONG)new_filesize;
     ret = lol_update_nentry(op);

     if (ret) {
#if LOL_TESTING
       printf("DEBUG: lol_fwrite: lol_update_nentry failed. ret = %d\n", (int)(ret));
#endif
         op->err = lol_errno = EIO;
     }

     if (new_filesize != filesize) {
         ret = lol_update_ichain(op, olds, news, last_old);
     } // end if file size changed.

     if (ret)
        LOL_ERRSET(EIO);

 error:
     lol_index_free((size_t)(write_blocks));

     if ((!(off)) && (!(op->err)))
         LOL_ERRSET(EIO);

  if (amount < 0) {
      off = 0;
      if (!(op->err))
         LOL_ERRSET(EIO);
  }

  return off;
} // end lol_fwrite
/* **********************************************************
 * lol_fseek:
 * sets the file position indicator for the stream pointed
 * to by stream.
 * ret = -1 : error
 * ret =  0 : success
 * 
 * ********************************************************** */
int lol_fseek(lol_FILE *op, long offset, int whence) {

 long pos, file_size;
 int ret = 0;

  if (!(op)) {
    lol_errno = EBADF;
    return -1;
  }
  if ((!(op->vdisk)) || (op->opened != 1)) {
    lol_errno = EBADF;
    return -1;
  }
  // It is possible to fseek over the file size.
  // But not under --> return -1 & EINVAL

  file_size = (long)op->nentry.file_size;
  pos       = (long)op->curr_pos;

      switch (whence) {

          case SEEK_SET:
	    // Seeking relative to start
            if (offset < 0) {
               ret = -1;
               op->err = lol_errno = EINVAL;
	    }
            else
	      op->curr_pos = offset;
	    break;

          case SEEK_CUR :

	    if ((pos + offset) < 0) {
	        ret = -1;
                op->err = lol_errno = EINVAL;
	    }
            else
                op->curr_pos += offset;
	    break;

          case SEEK_END :

	    if ((file_size + offset) < 0) {
	        ret = -1;
                op->err = lol_errno = EINVAL;
	    }
            else
                op->curr_pos = file_size + offset;
	    break;

          default :
            ret = -1;
	    op->err = lol_errno = EINVAL;
	    break;

      } // end switch

    if (!(ret))
        op->eof = 0;

    return ret;
} // end lol_fseek
/* **********************************************************
 * lol_unlink:
 * Deletes a file from a container.
 *
 * Return value:
 * -1 : error (sets lol_errno also)
 *  0 : success
 * 
 * ********************************************************** */
int lol_unlink(const char *name) {

  struct lol_name_entry entry;
  lol_FILE *fp;
  size_t len;
  alloc_entry e, last_block;
  int ret = 0;

  if (!(name))
    LOL_ERRET(EFAULT, -1);

  if (!(name[0]))
    LOL_ERRET(ENAMETOOLONG, -1);

  len = strlen(name);
  if ((len < 4) || (len >= LOL_PATH_MAX))
    LOL_ERRET(ENAMETOOLONG, -1);

  if (len > 0) {
      if (name[len-1] == '/')
#ifdef HAVE_LINUX_FS_H
         LOL_ERRET(EISDIR, -1);
#else
         LOL_ERRET(ENOENT, -1);
#endif
  }

  if ((lol_index_buffer) || (lol_buffer_lock))
      LOL_ERRET(EBUSY, -1);

  if (!(fp = lol_fopen(name, "r+"))) {
     // lol_errno already set
     return -1;
  }
  // Check that the entry is consistent, so that we won't
  // accidentally corrupt other files

  last_block = fp->sb.num_blocks - 1;

  if ((fp->nentry_index > last_block) ||
      (!(fp->sb.num_files))           ||
      (fp->nentry_index < 0)) {
      lol_errno = EIO;
      goto error;
  }

  e = fp->nentry.i_idx;

  if ((e > last_block) || (e < 0)) {
      lol_errno = EIO;
      goto error;
  }

  clearerr(fp->vdisk);
  if ((ret = lol_delete_chain_from(fp, LOL_UNLINK))) {

     switch (ret) {

            case LOL_ERR_BADFD :
            case LOL_ERR_PTR   :
	      lol_errno = EBADF;
	      goto error;
            break;
            case LOL_ERR_USER :
            case LOL_ERR_CORR :
	      lol_errno = EIO;
	      goto error;
            case LOL_ERR_IO :
	      // lol_errno already set
	      goto error;
            break;
            case LOL_ERR_INTRN :
	      lol_errno = EIO;
	      goto error;
            break;
            default :

	      if (ferror(fp->vdisk))
		lol_errno = errno;
              else
	        lol_errno = EBADF;

	      goto error;
            break;

     } // end switch;

     goto error;
  } // end if ret

  clearerr(fp->vdisk);
  if ((LOL_GOTO_DENTRY(fp))) {
      if (ferror(fp->vdisk))
	  lol_errno = errno;
      else
	  lol_errno = EBADF;
      goto error;
  }

  memset((char *)&entry, 0, NAME_ENTRY_SIZE);
  clearerr(fp->vdisk);
  if ((fwrite((char *)&entry,
      (size_t)(NAME_ENTRY_SIZE), 1, fp->vdisk)) != 1) {

      if ((ferror(fp->vdisk))) {
	        lol_errno = errno;
                goto error;
      }
      else{
	lol_errno = EIO;
	goto error;
      }
      goto error;
  } // end if fwrite

      // Adjust sb
      fp->sb.num_files--;
      if ((fseek(fp->vdisk, 0, SEEK_SET))) {
	 lol_errno = errno;
         goto error;
      } // end if fseek

      clearerr(fp->vdisk);
      if ((fwrite((char *)&fp->sb, (size_t)(DISK_HEADER_SIZE),
                   1, fp->vdisk)) != 1) {
	    if ((ferror(fp->vdisk))) {
	        lol_errno = errno;
                goto error;
	    }
	    else{
	      lol_errno = EIO;
	      goto error;
	    }
	 goto error;
      } // end if fwrite

   lol_fclose(fp);
   return LOL_OK;
error:
   lol_fclose(fp);
   return -1;
} // end lol_unlink
/* **********************************************************
 * lol_clearerr:
 * Clears the end-of-file and error indicators for the stream
 * pointed to by stream.
 * 
 * ret = N/A
 *
 * ********************************************************** */
void lol_clearerr(lol_FILE *op) {

  lol_errno = 0;

  if (!(op)) {
    lol_errno = EBADF;
    return;
  }
   op->eof = 0;
   op->err = 0;

} // end lol_clearerr
/* **********************************************************
 * lol_ferror:
 * Tests the error indicator for the stream pointed to by stream,
 * returning nonzero if it is set.
 * 
 *
 * ********************************************************** */
int lol_ferror(lol_FILE *op) {

  if (!(op)) {
    lol_errno = EBADF;
    return -1;
  }
  return op->err;
} // end lol_ferror
/* **********************************************************
 * lol_ftell:
 * Returns the current value of the file position
 * and -1 if error.
 *
 * ********************************************************** */
long lol_ftell(lol_FILE *s) {

  long ret = -1;

  if (s) {
    if (s->vdisk) {
        ret = (long)(s->curr_pos);
    }
    else {
        s->err = EBADF;
    }
  }
  if (ret == -1) {
      lol_errno = EBADF;
  }
  return ret;
} // end lol_ftell
/* **********************************************************
 * lol_stat:
 * Stats the file pointed to by path and fills in buf.
 * Return value:
 * -1 : error
 *  0 : success
 * ********************************************************** */
int lol_stat(const char *path, struct stat *st) {

  size_t len;
  size_t nlen;
  FILE *fp;
  lol_FILE op;
  struct lol_super sb;
  struct lol_name_entry entry;
  long raw_size;
  long doff;
  DWORD block_size;
  DWORD num_blocks;
  DWORD i, num_files;
  int ret = -1;

  if (!(path)) {
     lol_errno = EFAULT;
     return -1;
  }
  if (!(st)) {
      lol_errno = EFAULT;
      return -1;
  }

  lol_clean_fp(&op);
  if ((lol_get_filename(path, &op))) {
      lol_errno = ENOENT;
      return -1;
  }

  raw_size =  lol_get_vdisksize(op.vdisk_name, &sb,
                                NULL, RECUIRE_SB_INFO);
  if (raw_size <  LOL_THEOR_MIN_DISKSIZE) {
      lol_errno = ENOTDIR;
      return -1;
  }

  block_size = (DWORD)sb.block_size;
  num_blocks = (DWORD)sb.num_blocks;
  num_files  = (DWORD)sb.num_files;

    if (LOL_INVALID_MAGIC) {
       lol_errno = ENOTDIR;
       return -1;
    }

    if (num_files > num_blocks) {
        lol_errno = EOVERFLOW;
        return -1;
    }

    if (!(fp = fopen((char *)op.vdisk_name, "r"))) {
        lol_errno = ENOENT;
        return -1;
    }

    doff = (long)LOL_DENTRY_OFFSET_EXT(num_blocks, block_size);

    if ((fseek(fp, doff, SEEK_SET))) {
         lol_errno = ENOENT;
         goto error;
    }

    nlen = strlen((char *)op.vdisk_file);
    if ((nlen < 1) || (nlen >= LOL_FILENAME_MAX)) {
         lol_errno = ENAMETOOLONG;
         goto error;
    }

    for (i = 0; i < num_blocks; i++) {

	if ((fread ((char *)&entry, NAME_ENTRY_SIZE, 1, fp)) != 1) {
	         lol_errno = EOVERFLOW;
		 goto error;
        }

        if (!(entry.filename[0])) {
	       continue;
	}

        len = strlen((char *)entry.filename);

        if ((len >= LOL_FILENAME_MAX) || (len != nlen)) {
	     continue;
	}

        if ((strncmp((char *)entry.filename, (char *)op.vdisk_file, len))) {
	       continue;
        }
        else {

          // Gotcha !

       	  st->st_dev  = LOL_FILE_DEV; // a 'fake' lol device
          st->st_ino  = (ino_t)(i);
          st->st_mode = (S_IFREG | S_IRUSR | S_IWUSR |
                         S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

          st->st_nlink   = 1;
          st->st_uid     = 0;
          st->st_gid     = 0;
          st->st_rdev    = LOL_FILE_RDEV;
          st->st_size    = (off_t)entry.file_size;
          st->st_blksize = (blksize_t)block_size;
          if (entry.file_size) {
            st->st_blocks  = (blkcnt_t)(st->st_size / block_size);
            if (st->st_size % block_size)
	        st->st_blocks++;
	  }
	  else {
	    st->st_blocks = 1;
	  }
          st->st_atime = time(NULL);
          st->st_mtime = st->st_ctime = entry.created;
	  ret = 0;

          break;

        } // end else

  } // end for i

error:
    fclose(fp);
    return ret;
} // end lol_stat
/* **********************************************************
 * lol_mkfs: Create a lolfs container.
 * There are two distinct ways to call this function:
 * 1: Define the desired container size directly (opt = "-s")
 *     lol_mkfs("-s", "100M", 0, 0, "lol.db");
 *
 * 2: Define the data block size and number of them (opt = "-b")
 *     lol_mkfs("-b", NULL, 128, 50000, "lol.db");
 *
 * return value:
 * < 0: error
 *   0: succes
 ************************************************************ */
int lol_mkfs (const char *opt, const char *amount,
              const DWORD bs, const DWORD nb, const char *path)
{
  struct stat st;
  FILE *fp;
  struct lol_super sb; // This is the first a couple
                       // of bytes in the container file
  size_t v     =  0;
  DWORD blocks =  1;
  DWORD bsize  =  1;
  int      ret = -1;

  if ((!(path)) || (!(opt)))
      return LOL_ERR_USER;

  if ((!(opt[0])) || (!(path[0])))
     return LOL_ERR_USER;

  // Does user want to specify size or number of blocks and their size?
  if ((!(strcmp(opt, "-b"))) || (!(strcmp(opt, "--blocks")))) {

      if ((bs < 1) || (nb < 1))
	return LOL_ERR_USER;

      blocks = nb;
      bsize  = bs;

  } // end if add blocks
  else {

     if ((!(strcmp(opt, "-s"))) || (!(strcmp(opt, "--size")))) {
       // User wants to define size instead of blocks
       if (!(amount))
	  return LOL_ERR_USER;
       if (!(amount[0]))
	 return LOL_ERR_USER;

       if ((lol_size_to_blocks(amount, NULL,
            NULL, NULL, &blocks, LOL_JUST_CALCULATE))) {
	    return LOL_ERR_INTRN;
       }
       else {
	 // Got the amount of blocks!
	 bsize = LOL_DEFAULT_BLOCKSIZE;
       } // end else got it
     }
     else {
       // Neither one, user error...
       return LOL_ERR_USER;
     } // end else
  } // end else blocks

  if ((bsize < 1) || (blocks < 1)) // This should never happen here
    return LOL_ERR_INTRN;

  sb.num_files = 0;
  // We need 2 numbers, block size and the number of blocks
  sb.block_size = bsize;
  sb.num_blocks = blocks;
  // We set these two bytes to LOL_MAGIC always when creating a container
  sb.reserved[0] = sb.reserved[1] = LOL_MAGIC;

  if (!(fp = fopen(path, "w")))
      return LOL_ERR_IO;

  // Then we just write the information to the new
  // container metadata/superblock.
  // (See the definition of struct lol_super in <lolfs.h> for details).
  if ((fwrite((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
      ret = LOL_ERR_IO;
      goto error;
  }

  // After the super block, follow all the data blocks and
  // we initialize them with zeros.
  v = (size_t)(blocks * bsize);
  if ((null_fill (v, fp)) != v) {
      ret = LOL_ERR_IO;
      goto error;
  }

  // After the datablocks comes the (root-)directory entries,
  // which in a new container, we fill all of them with zeros...
  v = blocks * NAME_ENTRY_SIZE;
  if ((null_fill (v, fp)) != v) {
       ret = LOL_ERR_IO;
       goto error;

  }
  // Finally after the directory entries comes the data-allocation
  // indexes, which (in a new  container) we set all of them all to
  // FREE_LOL_INDEX value.
  v = blocks * ENTRY_SIZE;
  if ((lol_fill_with_value(FREE_LOL_INDEX, v, fp)) != v) {
       ret = LOL_ERR_IO;
       goto error;
  }

  ret = 0;
error:

  fclose(fp);
  if (!(ret))
     return 0;

  // If we failed, delete the failed container
  if (!(stat(path, &st))) {
      if (S_ISREG(st.st_mode)) {
          unlink(path);
      }
  } // end if !stat

  return ret;
} // end lol_mkfs
