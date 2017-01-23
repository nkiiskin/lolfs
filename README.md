


   LOLFS - Little Object List (LOL) FileSystem - by Niko Kiiskinen
                                      

        Please see file 'INSTALL' for detailed installation instructions
        The file COPYING is a copy of the General Public License v2,
        under which this software is distributed.

        This software package comes with some install scripts, some
        of which are distributed under The General Public License v3.
        See the file gpl3.txt for information about this license.



  Manifesto:

     LolFS is a container file. It means that it is a file
     which has other files inside it.

     Once you create a lol container file (using the included
     'mkfs.lolfs' utility or 'lol fs' command, you can then
     store content (any files basically) into it.

     The 'lol' program is an 'interface' app to execute common file
     operations, like copying files to and from the container,
     listing files, space usage etc.. See below for details.

     But wait.. Why would anybody store files into this
     container thing - why not just create a new directory and put
     those files there?

     I don't see a good reason, this is just a toy project for me
     and I like to play with it! - I am very aware of the double
     meaning of the name of this project :D

     Maybe I will add something interesting into future releases,
     something that might actually make this project useful -
     like automatic encryption of files..

     And there is one more thing. Maybe someone can make use of it
     as part of a bigger project that needs a container, after all,
     many projects have several data files around and it might
     become handy to store them all inside one "master" file.
     Lolfs has very simple C API to put those things together,
     see bottom of this file.

     LolFS is more than just a simple container for files. It is an
     actual filesystem - you may modify the files while they are
     inside the container, using the a simple C API (See below).


    lol program
    ============

                The 'lol' program is the main user interface to create
                and access files inside a lolfs container.

                'lol' has several built-in functions which
                may be used to execute some common file operations.
                These functions are currently (in v 0.30) :

                - fs
                - cat
                - cc
                - cp
                - df
                - ls
                - rm
                - rs




                 lol fs function:
                 ----------------


                 lol fs   creates a new container file.


                 Example 1:
                             "lol fs -s 700M test.db"

                         This example creates a container file
                         'test.db' which has 700 Megabytes
                         of storage capacity.

                 Example 2:
                             "lol fs -b 1000 5000 lol.db"


                         This example creates a container file
                         'lol.db' which has 5000 data blocks,
                         each 1000 bytes. So the storage capacity
                         for this example is 1000 * 5000 bytes.

                         I highly recommend using the '-s' option
                         instead of '-b', since in most cases
                         containers created using '-s' option
                         are much faster because they are optimized
                         for fast file I/O.
                         However, there maybe situations when it is
                         sensible to customize the block size
                         using '-b' option.

                        ( When you use '-s', the block size will
                          default to 2048, eg. 2K ).



                 lol cat function:
                 -----------------


                 lol cat  Prints the contents of a file (inside a container)
                          to standard output.


                 Example 1:
                             "lol cat test.db:/readme.txt"

                 Example 2:
                             "lol cat test.db:/pic.jpg > backup.jpg"





                 lol cc function:
                 ----------------


                 lol cc     checks if a container has errors.


                 Example 1:
                             "lol cc test.db"

                 Example 2:
                             "lol cc -d test.db"

                             ( The '-d' option shows more details
                               if the container has errors )




                 lol cp function:
                 ----------------


                 lol cp copies files to (and from) a container.

                     If you want to copy a file readme.txt to your
                     container file 'test.db', use:

                 Example 1:
                             "lol cp readme.txt test.db"


                     You may -of course- copy multiple files like:

                 Example 2:
                             "lol cp *.jpg test.db"


                     If you want to copy a file which is inside the container
                     back to your host filesystem, use:

                 Example 3:
                             "lol cp test.db:/readme.txt /some/directory"

                   NOTE: When accessing files inside your container, you must
                         separate the path with ':' like in above example.



              lol df function:
              ----------------


                    lol df  Shows how much space is used in container file.

                 Example:
                           "lol df my.db"





              lol ls function:
              ----------------


                 lol ls   lists the files inside a container.

 
                         For example, if you want to list all the files
                         inside a container file "lol.db", type:

                 Example:
                           "lol ls lol.db"




               lol rm function:
               ----------------


                 lol rm   deletes a file from your container file.


                 Example:
                           "lol rm test.db:/mother_in_law.jpg"


                 NOTE:   Note also here (this is common feature when accessing
                         files inside a container), that you must separate
                         the file with a ':' from it's container.



              lol rs function:
              ----------------


                 lol rs    extends a container file by adding more
                           storage space into it. Use this function
                           if the container is full or is becoming full.


                 Example 1:

                            "lol rs -s 200M lol.db"

                             This adds 200 Megabytes new space to
                             container 'lol.db'.


                 Example 2:

                            "lol rs -b 1000 my.db"

                            This adds 1000 new data blocks to
                            container 'my.db'



    mkfs.lolfs program
    ===================


                "mkfs.lolfs" creates a new container file.


                  It works _exactly_ as 'lol fs' command. (See above).


                 NOTE:

                   In Linux, lolfs may be used directly with removable storage,
                   without warranty of course!
                   So, you may actually insert a SDHC card or USB stick to
                   your Linux and create a lol storage directly there.
                   In that case the "filename" parameter is just the name of
                   the device,
                   for example "mkfs.lolfs -b 512 4000000 /dev/sdc1"
                   You MUST know what you are doing then, and of course
                   you must be root to do that - or anything similar.
                   You have been warned!



    fsck.lolfs program
    ===================


                "fsck.lolfs" checks a lol container for errors.

                It works exactly as 'lol cc' command. (See above).





lolFS API:
==========

     The API prototypes are in <lolfs.h>
     They are identical to their standard C counterparts,
     except the "lol_" prefix in the name:

     - For example use "lol_fopen" to open a file inside
       your container, it returns a lol_FILE* handle which
       may then be used to lol_fread, lol_fwrite, lol_fseek etc..

     - when you lol_fopen a file (or otherwise manipulate files inside
       a container file, you must separate the filesystem path from the
       file inside a container with a ':'

     Example: Create a file "test.txt" inside container called 'my.db'.

              lol fs -s 20k my.db

     Then test the C API:


LolFS test program:
===================


  #include   <stdio.h>
  #include   <string.h>
  #include   <lolfs.h>



  // Github propably messes up these
  // includes... :(
  // (Read the original 'README.md' file)



 /* ======= MAIN ============ */


  int main()  {



      lol_FILE *fp;
      char text[] = "Hello World!\n";

      fp = lol_fopen("my.db:/test.txt", "w");

      if (!(fp)) {
         printf("cannot create the file\n");
         return -1;
      }
      if ((lol_fwrite((char *)text,
           strlen(text), 1, fp)) != 1) {

         printf("Dang! Could not write the file!\n");
         lol_fclose(fp);
         return -1;
      }

     // YES! The file is there!
      lol_fclose(fp);
      return 0;

  } // end main


 /* ======= END MAIN ============== */


How to compile and link a lolfs C program:

gcc test.c -o test -llolfs

(You may need to include compiler option -L/path/to/librarydir
 if the linker does not find lolfs library)

Something like:
gcc test.c -o test -L/usr/local/lib64 -llolfs -I/usr/local/include

If the build does not fail, run the test:


 './test'


(There is no ouput, it just writes a file into 'my.db' container).
Then check the list of files inside. It should show something
like this:

'lol ls my.db'

Thu Dec 29 19:56:18 2016 13 hello.txt

total 1


Then see the contents of hello.txt


'lol cat my.db:/hello.txt'

Hello World!



 Have fun with LolFS !
 =====================

 Questions, Bug reports, etc

 - Niko Kiiskinen
   lolfs.bugs@gmail.com
   https://github.com/nkiiskin/lolfs

