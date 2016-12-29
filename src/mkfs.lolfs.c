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
 $Id: mkfs.lolfs.c, v0.20 2016/11/02 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
*/
/* ************************************************************************ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lolfs.h>
#include <lol_internal.h>
/* ************************************************************************ */
/* Wrapper app for lol_fs function                                          */
/* ************************************************************************ */
int main (int argc, char* argv[])
{
  return lol_fs(argc, argv);
}
/* ************************************************************************ */
