/*
  Copyright (C) 2000-2010 Silicon Graphics, Inc.  All Rights Reserved.
  Portions Copyright (C) 2009-2010 SN Systems Ltd. All Rights Reserved.
  Portions Copyright (C) 2009-2020 David Anderson. All Rights Reserved.

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

#ifndef tag_common_INCLUDED
#define tag_common_INCLUDED

/* The following is the magic token used to
   distinguish real tags/attrs from group-delimiters.
   Blank lines have been eliminated by an awk script.
*/
#define MAGIC_TOKEN_VALUE 0xffffffff

/* TAG_TREE.LIST Expected input format

0xffffffff
value of a tag
value of a standard tag that may be a child of that tag
...
0xffffffff
value of a tag
value of a standard tag that may be a child of that tag
...
0xffffffff
...

No blank lines or commentary allowed, no symbols, just numbers.

*/

/* TAG_ATTR.LIST Expected input format

0xffffffff
value of a tag
value of a standard attribute that follows that tag
...
0xffffffff
value of a tag
value of a standard attribute that follows that tag
...
0xffffffff
...

No blank lines or commentary allowed, no symbols, just numbers.

*/

/* We don't need really long lines: the input file is simple. */
#define MAX_LINE_SIZE 1000
#define IS_EOF 1
#define NOT_EOF 0

extern void bad_line_input(char *format,...);
extern void trim_newline(char *line, int max);
extern Dwarf_Bool is_blank_line(char *pLine);
extern int read_value(unsigned int *outval,FILE *f);

#endif /* tag_common_INCLUDED */
