/*
  Copyright 2010-2018 David Anderson. All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of version 2 of the GNU General
  Public License as published by the Free Software Foundation.

  This program is distributed in the hope that it would be
  useful, but WITHOUT ANY WARRANTY; without even the implied
  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

  Further, this software is distributed without any warranty
  that it is free of the rightful claim of any third person
  regarding infringement or the like.  Any license provided
  herein, whether implied or otherwise, applies only to this
  software file.  Patent licenses, if any, provided herein
  do not apply to combinations of this program with other
  software, or any other product whatsoever.

  You should have received a copy of the GNU General Public
  License along with this program; if not, write the Free
  Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
  Boston MA 02110-1301, USA.

*/

#ifndef COMPILER_INFO_H
#define COMPILER_INFO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define COMPILER_TABLE_MAX 100

typedef struct anc {
    struct anc *next;
    char *item;
} a_name_chain;

typedef struct {
    int checks;
    int errors;
} Dwarf_Check_Result;

/*  Records information about compilers (producers) found in the
    debug information, including the check results for several
    categories (see -k option). */
typedef struct {
    const char *name;
    Dwarf_Bool verified;
    a_name_chain *cu_list;
    a_name_chain *cu_last;
    Dwarf_Check_Result results[LAST_CATEGORY];
} Compiler;

/*  Compiler statistics. */
extern void clean_up_compilers_detected(void);
extern void reset_compiler_entry(Compiler *compiler);
extern void print_checks_results(void);
extern Dwarf_Bool record_producer(char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMPILER_INFO_H */
