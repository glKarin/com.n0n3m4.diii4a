/*
Copyright (C) 2000-2005 Silicon Graphics, Inc. All Rights Reserved.
Portions Copyright (C) 2007-2011 David Anderson. All Rights Reserved.

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

/* naming.h */
#ifndef NAMING_H_INCLUDED
#define NAMING_H_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

extern const char * get_FORM_CLASS_name(unsigned int val_in);
extern const char * get_TAG_name(unsigned int val_in);
extern const char * get_children_name(unsigned int val_in);
extern const char * get_FORM_name(unsigned int val_in);
extern const char * get_AT_name(unsigned int val_in);
extern const char * get_OP_name(unsigned int val_in);
extern const char * get_ATE_name(unsigned int val_in);
extern const char * get_DS_name(unsigned int val_in);
extern const char * get_END_name(unsigned int val_in);
extern const char * get_ATCF_name(unsigned int val_in);
extern const char * get_ACCESS_name(unsigned int val_in);
extern const char * get_VIS_name(unsigned int val_in);
extern const char * get_VIRTUALITY_name(unsigned int val_in);
extern const char * get_LANG_name(unsigned int val_in);
extern const char * get_LNAME_name(unsigned int val_in);
extern const char * get_ID_name(unsigned int val_in);
extern const char * get_CC_name(unsigned int val_in);
extern const char * get_INL_name(unsigned int val_in);
extern const char * get_ORD_name(unsigned int val_in);
extern const char * get_DSC_name(unsigned int val_in);
extern const char * get_LNS_name(unsigned int val_in);
extern const char * get_LNE_name(unsigned int val_in);
extern const char * get_MACINFO_name(unsigned int val_in);
extern const char * get_MACRO_name(unsigned int val_in);
extern const char * get_CFA_name(unsigned int val_in);
extern const char * get_EH_name(unsigned int val_in);
extern const char * get_FRAME_name(unsigned int val_in);
extern const char * get_CHILDREN_name(unsigned int val_in);
extern const char * get_ADDR_name(unsigned int val_in);
#ifdef __cplusplus
}
#endif
#endif /* NAMING_H_INCLUDED */
