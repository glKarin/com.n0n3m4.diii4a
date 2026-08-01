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

#ifndef COMMAND_OPTIONS_H
#define COMMAND_OPTIONS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

const char *process_args(int argc, char *argv[]);
const char *do_uri_translation(const char *s,const char *context);
void uri_data_constructor(void);
void uri_data_destructor(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COMMAND_OPTIONS_H */
