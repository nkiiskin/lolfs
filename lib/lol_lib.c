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
 * $Id: lol_lib.c, v0.30 2017/01/01 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifdef HAVE_ERRNO_H
#ifndef _ERRNO_H
#include <errno.h>
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
#ifndef _LOLFS_H
#include <lolfs.h>
#endif
#ifndef _LOL_INTERNAL_H
#include <lol_internal.h>
#endif
/* ********************************************************* *
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
  lol_pinfo p;
  lol_FILE *op       = 0;
  int       r        = 0;
  int       is       = 0;
  int       mod      = 0;
  int       path_len = 0;
  int       mode_len = 0;
  int       trunc    = 0;
  long      size     = 0;
  ULONG     fs       = 0;

  lol_errno = 0;

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

  if ((mod < 0) || (mod >= MAX_LOL_OPEN_MODES))
       delete_return_NULL(EINVAL, op);

  strcpy(op->open_mode.mode_str, lol_open_modes[mod].mode_str);
  strcpy(op->open_mode.vd_mode,  lol_open_modes[mod].vd_mode);

  p.fullp = (char *)path;
  p.file  = op->file;
  p.cont  = op->cont;
  p.func  = LOL_FILENAME | LOL_CONTPATH;

  if ((lol_pathinfo(&p)))
      delete_return_NULL(EINVAL, op);

  size = lol_fgetsize(op);

  if (size < LOL_THEOR_MIN_DISKSIZE)
      delete_return_NULL(EIO, op);

  op->csiz = (ULONG)size;
  op->open_mode.device = op->cinfo.st_mode; // in 2 places this one too...
  if (!(op->dp = fopen(op->cont, op->open_mode.vd_mode)))
      delete_return_NULL(EINVAL, op);

  is = lol_read_nentry(op);  // Can we find the dir entry?
  fs = op->nentry.fs;

  switch (is) {  // What we do here depends on open_mode

          case  LOL_NO_SUCH_FILE:

           // Does not exist, so we return NULL
           // if trying to read (cases "r" & "r+")

		if (mod < 2) /* if "r" or "r+", cannot read.. */
                    close_return_NULL(ENOENT, op);

	        op->curr_pos = 0;
                // if not reading,
		// then it must be w/a, create new file...
	        if ((r = lol_touch_file(op)))
		   close_return_NULL(lol_errno, op);

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

                                 close_return_NULL(EINVAL, op);

	              } // end switch mod

                      break;

               default:

                    close_return_NULL(EIO, op);


  } // end switch file exists

  if (trunc) { // Truncate if file was opened "w" or "w+"

      if ((r = lol_truncate_file(op))) {
#if LOL_TESTING
	  lol_error("DEBUG: lol_fopen: truncate failed, ret = %d\n", r);
#endif
	  // TODO: FIX! Check return value
          close_return_NULL(EIO, op);
      }

      op->curr_pos = 0;

  } // end if truncate

  lol_errno  = 0;
  op->opened = 1;
  return op;
} // end lol_fopen
/* **********************************************************
 *  INTERFACE FUNCTION
 *  lol_fclose:
 *
 *  Closes a previously opened (by lol_fopen) lol-file.
 *  Upon successful completion 0 is returned. Otherwise, EOF
 *  is returned and errno  is  set  to indicate  the error.
 *  (In either case, any further access to the file handle
 *   results in undefined behavior, read: segmentation fault)
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

  if (!(op)) {
       lol_errno = EBADF;
       return EOF;
   }
   if (op->dp) {

       if (fclose(op->dp)) {
          ret = EOF;
	  if (errno)
	    lol_errno = errno;
	  else
	    lol_errno = EBUSY;
       }
       else {
          op->dp = NULL;
	  lol_errno = 0;
       }
   }
   else {
      lol_errno = EBADF;
      ret = EOF;
   }

   free (op);
   op = NULL;
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

  file_size    = (size_t)op->nentry.fs;
  block_size   = (size_t)op->sb.bs;
  left_to_read = file_size - op->curr_pos;

  if (!(left_to_read)) {
       op->eof = 1;
       return LOL_ERR_EOF;
  }

  if ((!(op->dp)) || (op->opened != 1))
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
  if ((current_index < 0) ||
      (current_index >= op->sb.nb))
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

  filesize    = (long)op->nentry.fs;
  block_size  = (size_t)op->sb.bs;

  if ((!(op->dp)) || (op->opened != 1))
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
  if ((current_index < 0) ||
      (current_index >= op->sb.nb))
       LOL_ERR_RETURN(ENFILE, 0)

  if ((lol_is_writable(op)))
      LOL_ERR_RETURN(EPERM, 0)

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
	     lol_error("DEBUG: lol_fwrite err 1");
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

      amount = lol_io_dblock(op, current_index,
                             (char *)ptr, end_bytes, LOL_WRITE);

      if (amount < 0) {
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
     op->nentry.fs = (ULONG)new_filesize;
     ret = lol_update_nentry(op);

     if (ret) {
#if LOL_TESTING
       printf("DEBUG: lol_fwrite: lol_update_nentry failed. ret = %d\n", (int)(ret));
#endif
         op->err = lol_errno = EIO;
     }

  if (new_filesize != filesize) {
      ret = lol_update_ichain(op, olds, news, last_old);
  }
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
  if ((!(op->dp)) || (op->opened != 1)) {
    lol_errno = EBADF;
    return -1;
  }
  // It is possible to fseek over the file size.
  // But not under --> return -1 & EINVAL

  file_size = (long)op->nentry.fs;
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

  if (name[len-1] == '/')
#ifdef HAVE_LINUX_FS_H
      LOL_ERRET(EISDIR, -1);
#else
      LOL_ERRET(ENOENT, -1);
#endif

  if ((lol_index_buffer) || (lol_buffer_lock))
      LOL_ERRET(EBUSY, -1);
  if (!(fp = lol_fopen(name, "r+"))) {
     // lol_errno already set
     return -1;
  }
  // Check that the entry is consistent, so that we won't
  // accidentally corrupt other files

  last_block = fp->sb.nb - 1;
  if ((fp->n_idx > last_block) ||
      (!(fp->sb.nf))           ||
      (fp->n_idx < 0)) {
      lol_errno = EIO;
      goto error;
  }

  e = fp->nentry.i_idx;
  if ((e > last_block) || (e < 0)) {
      lol_errno = EIO;
      goto error;
  }

  clearerr(fp->dp);
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

	      if (ferror(fp->dp))
		lol_errno = errno;
              else
	        lol_errno = EBADF;

	      goto error;
            break;

     } // end switch;

     goto error;
  } // end if ret

  clearerr(fp->dp);
  if ((LOL_GOTO_DENTRY(fp))) {
      if (ferror(fp->dp))
	  lol_errno = errno;
      else
	  lol_errno = EBADF;
      goto error;
  }

  memset((char *)&entry, 0, NAME_ENTRY_SIZE);
  clearerr(fp->dp);
  if ((fwrite((char *)&entry,
      (size_t)(NAME_ENTRY_SIZE), 1, fp->dp)) != 1) {

      if ((ferror(fp->dp))) {
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
      fp->sb.nf--;
      if ((fseek(fp->dp, 0, SEEK_SET))) {
	 lol_errno = errno;
         goto error;
      } // end if fseek

      clearerr(fp->dp);
      if ((fwrite((char *)&fp->sb, (size_t)(DISK_HEADER_SIZE),
                   1, fp->dp)) != 1) {
	    if ((ferror(fp->dp))) {
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
    if (s->dp) {
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
 * Stats the file pointed to by 'path' and fills in 'st'.
 *
 * NOTE: This function must be FAST.
 * We may thus need to use some ugly methods...
 *
 * Return value:
 * -1 : error
 *  0 : success
 * ********************************************************** */
#define LOL_STAT_TMP 256
int lol_stat(const char *path, struct stat *st) {

  const long nes = (long)(NAME_ENTRY_SIZE);
  const long temp_mem = nes * LOL_STAT_TMP;
  char temp[temp_mem];
  char cont[256];
  char fname[64];
  lol_meta sb;
  lol_pinfo p;
  FILE *fp;
  lol_nentry *buffer;
  lol_nentry *nentry;
  size_t mem = 0;
  long files = 0;
  long data;
  long doff;
  long frac;
  long   io;
  long   nb;
  long   bs;
  long   nf;
  int times;
  int i;
  int j;
  int k;
  int alloc = 0;
  int  ret = -1;

  if (!(path))
      LOL_ERRET(EINVAL, -1);
  if (!(st))
      LOL_ERRET(EINVAL, -1);

  p.fullp = (char *)path;
  p.file  = fname;
  p.cont  = cont;
  p.func  = LOL_FILENAME | LOL_CONTPATH;

  if ((lol_pathinfo(&p)))
      LOL_ERRET(EINVAL, -1);
  if ((stat(cont, st)))
      LOL_ERRET(ENOENT, -1);
  if (st->st_size < LOL_THEOR_MIN_DISKSIZE)
      LOL_ERRET(ENXIO, -1);
  if (!(fp = fopen((char *)cont, "r")))
      LOL_ERRET(EIO, -1);
  if ((fread ((char *)&sb, DISK_HEADER_SIZE, 1, fp)) != 1) {
      lol_errno = EIO;
      goto closeret;
  }
  bs = (long)sb.bs;
  nb = (long)sb.nb;
  nf = (long)sb.nf;
  if ((!(nb)) || (!(bs))) {
      lol_errno = ENXIO;
      goto closeret;
  }
  data = (long)LOL_DEVSIZE(nb, bs);
  if (data != st->st_size) {
      lol_errno = ENXIO;
      goto closeret;
  }
  if (LOL_INVALID_MAGIC) {
      lol_errno = ENXIO;
      goto closeret;
  }
  if (nf > nb) {
      lol_errno = EOVERFLOW;
      goto closeret;
  }
  data  = nes * nb;
  io = lol_get_io_size(data, nes);
  if (io <= 0) {
      lol_errno = EIO;
      lol_debug("lol_stat: Internal error: io <= 0");
      goto closeret;
  }

  if (io > temp_mem) {
    if (!(buffer = (lol_nentry *)lol_malloc((size_t)(io)))) {
          buffer = (lol_nentry *)temp;
          io     = temp_mem;
    } else {
       mem = (size_t)(io);
       alloc = 1;
    }
  } else {
    buffer = (lol_nentry *)temp;
    io     = temp_mem;
  }

  doff = (long)LOL_DENTRY_OFFSET_EXT(nb, bs);
  if ((fseek(fp, doff, SEEK_SET))) {
        if (errno) {
	    lol_errno = errno;
        }
        else {
            lol_errno = EIO;
        }
        goto closefree;
  }
  times = (int)(data / io);
  frac  = data % io;
  k = (int)(io / nes);

 dentry_loop:
  for (i = 0; i < times; i++) {

    if ((fread((char *)buffer, ((size_t)(io)), 1, fp)) != 1) {

        if (errno) {
	    lol_errno = errno;
        }
        else {
            lol_errno = EIO;
        }
        goto closefree;
    }
    // Now check the entries
    for (j = 0; j < k; j++) { // foreach entry...
       nentry = &buffer[j];
       if (!(nentry->name[0])) {
	   continue;
       }
       files++;
       if (files > nf) {
	 lol_errno = ENFILE;
	 goto closefree;
       }

        if ((strcmp((char *)nentry->name, (char *)fname))) {
	     continue;
        }
	else {

          // Gotcha !
	  // Just check that the file is consistent first..
          if ((nentry->i_idx < 0) || (nentry->i_idx >= nb)) {
             // Corrupted file!
	     lol_errno = ENFILE;
	     goto closefree;
	  }
	  // Check file size also
	  if (((nentry->fs) > (nb * bs))) {
	      lol_errno = EFBIG;
	      goto closefree;
	  }

          // The st_dev field must be ignored by apps if st_mode has S_IFREG set
          st->st_dev  = makedev(LOL_DEV_MAJOR, LOL_DEV_MINOR); // a 'fake' lol device
          st->st_mode = LOL_FILE_DEFAULT_MODE;
          st->st_nlink   = 1;
          st->st_uid     = 0;
          st->st_gid     = 0;
          st->st_rdev    = (dev_t)(st->st_ino); // Hijack the container inode
                                                // out in this field.
          st->st_ino = (ino_t)(i * k + j); // lolfile inode is just the number
	                                   // of the directory entry.
          st->st_size    = (off_t)nentry->fs;
          st->st_blksize = (blksize_t)bs;
          if (nentry->fs) {
            st->st_blocks  = (blkcnt_t)(st->st_size >> LOL_DIV_512);
            if (st->st_size % 512)
	        st->st_blocks++;
	  }
	  else {
	     st->st_blocks = 1;
	  }
          st->st_atime = time(NULL);
          st->st_mtime = st->st_ctime = nentry->created;

	  ret = 0;
          goto closefree;

	} // end else GOT IT !

    } // end for j
  } // end for i

  // Now the fractional data
  if (frac) {
      times = 1;
      io = frac;
      k = (int)(io / nes);
      frac = 0;
      goto dentry_loop;
  } // end if frac

  lol_errno = ENOENT;

closefree:
 if (alloc) {
    lol_free(mem);
 }
closeret:
 fclose(fp);
 return ret;
} // end lol_stat
#undef LOL_STAT_TMP
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
                       // of bytes of the container file
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

  sb.nf = 0;
  // We need 2 numbers, block size and the number of blocks
  sb.bs = bsize;
  sb.nb = blocks;
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
  // After the datablocks comes the (root-)directory entries,
  // which in a new container, we fill all of them with zeros...
  v += blocks * NAME_ENTRY_SIZE;
  if ((lol_fclear (v, fp)) != v) {
      ret = LOL_ERR_IO;
      goto error;
  }
  // Finally after the directory entries comes the data-allocation
  // indexes, which (in a new  container) we set all of them all to
  // FREE_LOL_INDEX value.
  v = (size_t)blocks; // * ENTRY_SIZE;
  if ((lol_ifcopy(FREE_LOL_INDEX, v, fp)) != v) {
       ret = LOL_ERR_IO;
       goto error;
  }
  ret = 0;
error:
  fclose(fp);
  if (!(ret))
     return 0;

  // If we failed, delete the container
  if (!(stat(path, &st))) {
     if (S_ISREG(st.st_mode)) {
        unlink(path);
     }
  } // end if !stat
  return ret;
} // end lol_mkfs
