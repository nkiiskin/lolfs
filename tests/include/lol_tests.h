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
 $Id: lol_tests.h, v0.40 2017/02/11 Niko Kiiskinen <lolfs.bugs@gmail.com> Exp $"
 *
 ************************************************************************** */
#ifndef _LOL_TESTS_H
#define _LOL_TESTS_H  1
#endif
#ifndef HAVE_CONFIG_H
#include "../config.h"
#endif
#ifndef _LOLFS_H
#include "../include/lolfs.h"
#endif
#ifndef _LOL_INTERNAL_H
#include "../include/lol_internal.h"
#endif
/* ************************************************************************ */
typedef struct lol_testargs_t {

  char tag[64];
  lol_func func;
  int bundle;
  int argc;
  char *argv[];
} lol_testargs;


int lol_compare_files (const char *f1, const char *f2);
