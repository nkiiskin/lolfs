


	Liblolfs - Little Object List (LOL) filesystem - by Niko Kiiskinen
                                      

        Please see file 'INSTALL' for detailed installation instructions
        The file COPYING is a copy of the General Public License v2,
        under which this software is distributed.

        This software package comes with some install scripts which
        are distributed under The General Public License v3.
        See the file gpl3.txt for information about this license.




Manifesto


     I enjoy (re-)inventing wheels, here is another excr.. example.

     Lolfs is a container file. It means that it is a file (stored
     in your computer somewhere) which has other files inside it.

     Once you create a lol container file (using the included
     'mkfs.lolfs' utility or lol_mkfs API function), you can
     then use the lolfs API explained in <lolfs.h> to create,
     modify and delete files in this container.

     The installation package also contains a program "lol",
     which may be used to do the common file operations:



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


    lol program:
    ============

                The "lol" app has a couple of built-in functions which
                can be used to do some common file tasks.
                These functions are currently (in v 0.12) :

                - ls
                - cp
                - rm
                - df
                - cat

                For example, if you want to list all the files
                inside a container file "mycontainer", type:

                lol ls mycontainer

                So, use it like "lol ls mycontainer", where
                "mycontainer" is the name of the container file

                lol cp function:
                ---------------

                lol cp copies files to (and from) your container file.

                 If you want to copy a file /home/you/readme.txt to your
                 container file 'mycontainer', use:
                 lol cp /home/you/readme.txt mycontainer

                 If you want to copy a file which is inside the container
                 back to your normal filesystem, use:

                 lol cp mycontainer:/readme.txt /some/directory

                 NOTE: When accessing files inside your container, you must
                       separate the path with ':' like in above example.


                 lol rm function:
                 ---------------

                lol rm   deletes a file from your container file.
                         Use like: "lol rm mycontainer:/readme.txt"

                 NOTE:   Note also here (this is common feature when accessing
                         files inside a container), that you must separate
                         the file with a ':' from it's container.


                 lol df function:
                 ---------------

                 lol df  Shows how much space is used in container file.
                         Use like: "lol df mycontainer"


                 lol cat function:
                 ----------------

                 lol cat  Prints the contents of a file (inside a container)
                          to standard output.
                          Use like: "lol cat mycontainer:/readme.txt"
                          Or like: "lol cat mycontainer:/somefile > somefile.bak"
      


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


  #include <string.h>
  #include <lolfs.h>

  int main() {

             lol_FILE *fp;
             char text[] = "Hello World!\n";

             fp = lol_fopen("mycontainer:/test.txt", "w");

             lol_fwrite((char *)text, strlen(text), 1, fp);
             lol_fclose(fp);
             return 0;

  }
      

  How to compile and link a lolfs enabled program:

  gcc program.c -o program -llolfs

  (You may need to include compiler option -L/path/to/librarydir
   if the linker does not find lolfs library)

  
Questions, Bug reports, etc..

     Niko Kiiskinen
     nkiiskin {at} yahoo com
     https://github.com/nkiiskin/lolfs


Distribution date: Sun Dec  4 07:36:29 EET 2016
