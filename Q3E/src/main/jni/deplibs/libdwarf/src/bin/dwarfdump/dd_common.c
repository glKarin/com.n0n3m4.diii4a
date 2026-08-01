/*
Copyright (C) 2008-2010 SN Systems.  All Rights Reserved.
Portions Copyright (C) 2008-2017 David Anderson.  All Rights Reserved.
Portions Copyright (C) 2011-2012 SN Systems Ltd.  .  All Rights Reserved.

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

/* These do little except on Windows */

#include <config.h>

#include <stdio.h> /* printf() */

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_common.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_defined_types.h"
#include "dd_sanitized.h"

/* PACKAGE_VERSION is from config.h */
/*  The Linux/Unix version does not want a version string to print
    unless -V is on the command line. */
void
print_version_details(const char * name)
{
    if (glflags.gf_show_args_flag) {
        const char *pv = dwarf_package_version();
        printf("%s [%s %s (libdwarf %s dwarfdump %s)]\n",
            sanitized(name),__DATE__,__TIME__,pv,PACKAGE_VERSION);
    }
}

void
print_args(int argc , char *argv[] )
{
    if (glflags.gf_show_args_flag) {
        int index = 1;
        printf("Arguments: ");
        for (index = 1; index < argc; ++index) {
            printf("%s ",sanitized(argv[index]));
        }
        printf("\n");
    }
}

/*  Going to stdout as of April 2018.
    dwarfdump only calls if requested by user.
    At one time this also printed glflags.program_name  */
void
print_usage_message(
    const char **text)
{
    unsigned i = 0;
    for (i = 0; *text[i]; ++i) {
        printf("%s\n", text[i]);
    }
}
