


	LOLFS - Little Object List (LOL) FileSystem - by Niko Kiiskinen
                                      

        Please see file 'INSTALL' for detailed installation instructions
        The file COPYING is a copy of the General Public License v2,
        under which this software is distributed.

        This software package comes with some install scripts, some
        of which are distributed under The General Public License v3.
        See the file gpl3.txt for information about this license.



Manifesto


     LolFS is a container file. It means that it is a file (stored
     in your computer somewhere) which has other files inside it.

     Once you create a lol container file (using the included
     'mkfs.lolfs' utility or 'lol fs' command or lol_mkfs API
     function), you can then use the lolfs API, which is explained
     in <lolfs.h> to create, modify and delete files in this container.

     The installation package contains a program called 'lol',
     which serves as a wrapper app to do common file operations.
     See below for details.


    lol program:
    ============
                The 'lol' program is the main user interface to create
                and access files of a lolfs container.
                The 'lol' app has several built-in functions which
                can be used to do some common file operations.
                These functions are currently (in v 0.13) :

                - cat
                - cc
                - cp
                - df
                - fs
                - ls
                - rm
                - rs


                 lol cat function:
                 -----------------

                 lol cat  Prints the contents of a file (inside a container)
                          to standard output.
                          Use like: 'lol cat container:/readme.txt'.
                          Or like: 'lol cat container:/somefile > file.bak'.



                 lol cc function:
                 -----------------

                 lol cc     checks if a container has errors.

                            Use like: "lol cc mycontainer".




                 lol cp function:
                 ----------------

                 lol cp copies files to (and from) your container file.

                  If you want to copy a file /home/you/readme.txt to your
                  container file 'mycontainer', use:
                  lol cp /home/you/readme.txt mycontainer


                  You can copy multiple files like: 'lol cp *.jpg container'

                  If you want to copy a file which is inside the container
                  back to your normal filesystem, use:

                  lol cp mycontainer:/readme.txt /some/directory

                  NOTE: When accessing files inside your container, you must
                        separate the path with ':' like in above example.



                 lol df function:
                 ----------------

                 lol df  Shows how much space is used in container file.
                         Use like: "lol df mycontainer"



                 lol fs function:
                 ----------------

                 lol fs   creates a new container file.

                 Example:

                         Use like: "lol fs 1000 5000 mycontainer"

                         (This example creates a container file
                          'mycontainer' which has 5000 data blocks,
                          each 1000 bytes. So the storage capacity
                          for this example is 1000 * 5000 bytes).


                 lol ls function:
                 ----------------

                 lol ls   lists the files inside a container.

                          For example, if you want to list all the files
                          inside a container file "mycontainer", type:

                          lol ls mycontainer




                 lol rm function:
                 ----------------

                 lol rm   deletes a file from your container file.
                          Use like: "lol rm mycontainer:/readme.txt"

                 NOTE:   Note also here (this is common feature when accessing
                         files inside a container), that you must separate
                         the file with a ':' from it's container.



                 lol rs function:
                 ----------------

                 lol rs    extends a container file by adding more
                           empty space into it.

                 Example 1:

                         Use like: "lol rs -b 1000 mycontainer"

                         (This adds 1000 new data blocks to
                          container 'mycontainer')

                 Example 2:

                         "lol rs -s 20M mycontainer"

                         (This adds 20 Megabytes new space to
                          container 'mycontainer').



    mkfs.lolfs program:
    ===================


                "mkfs.lolfs" creates a container file.


                It takes 3 parameters:
                - block size: the number of bytes a "block" has
                              (this is a number which you can choose
                               freely. Try to choose it as average file size
                               to gain best performance).

                - number of blocks: the number of data blocks. This depends
                                    on how big container you want.
                                    Try like 50000 first and try it

                - filename: this is just the name which you like to use
                            as the name of the container file. Ex:
                            'mycontainer'

                 Use like: "mkfs.lolfs  100  5000  mycontainer"

                           ( This creates a container file 'mycontainer' which
                             has 1000 * 5000 bytes of storage capacity ).


                 ( In Linux, lolfs may be used directly with removable storage,
                   without warranty of course!
                   So, you may actually insert an SDHC card to your Linux
                   and create a lol storage directly there. In that case the
                   "filename" parameter is just the name of the device,
                   for example "mkfs.lolfs 512 4000000 /dev/sdb"
                   You MUST know what you are doing then, and of course
                   you must be root to do that - or anything similar.
                   You have been warned! :)



    fsck.lolfs program:
    ===================


                "fsck.lolfs" checks a lol container for errors.

                It takes one parameter:
                - the name of the container


                 Use like: "fsck.lolfs mycontainer"




lolfs API:
==========

     The API functions are explained in <lolfs.h>
     They are identical to their standard C counterparts,
     except the "lol_" prefix in the name:

     - For example use "lol_fopen" to open a file inside
       your container, it returns a lol_FILE* handle which
       may then be used to lol_fread, lol_fwrite, lol_fseek etc..

     - when you lol_fopen a file (or otherwise manipulate files inside
       a container file, you must separate the filesystem path from the
       file inside a container with a ':'

     Example: Create a file "test.txt" inside container called 'mycontainer'.


  #include  <string.h>
  #include  <lolfs.h>

  int main() {

      lol_FILE *fp;
      char text[] = "Hello World!\n";

      fp = lol_fopen("mycontainer:/test.txt", "w");
      lol_fwrite((char *)text, strlen(text), 1, fp);
      lol_fclose(fp);

      return 0;
  }
      


  HOW TO compile and link a lolfs enabled program:

  gcc program.c -o program -L/path/to/liblolfs -llolfs

  Usually '/path/to/liblolfs' is /usr/local/lib or /usr/local/lib64
  depending on how you configured the installation.

  
Questions, Bug reports, etc..

     Niko Kiiskinen
     nkiiskin {at} yahoo com
     https://github.com/nkiiskin/lolfs


Distribution date: Sat Dec 24 02:46:31 EET 2016
