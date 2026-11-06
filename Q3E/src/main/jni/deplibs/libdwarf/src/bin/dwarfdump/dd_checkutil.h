/*
  Copyright (C) 2011 SN Systems Ltd. All Rights Reserved.
  Portions Copyright (C) 2011 David Anderson. All Rights Reserved.

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

#ifndef CHECKUTIL_H
#define CHECKUTIL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*  Map information.
    Depending on the specific functions used various
    fields here are either used or ignored.

*/
typedef struct {
    Dwarf_Bool bFlag;   /* General flag */
    const char *name;   /* Generic name */
    Dwarf_Addr key;     /* Used for binary search, the key
        is either a pc address or a DIE offset
        depending on which bucket table is in use. */
    Dwarf_Addr base;    /* Used for base address */
    Dwarf_Addr low;     /* Used for Low PC */
    Dwarf_Addr high;    /* Used for High PC */
} Bucket_Data;

/*  This groups Bucket_Data records into
    a 'bucket' so that a single malloc creates
    BUCKET_SIZE entries.  The intent is to reduce
    overhead (as compared to having next/previous
    pointers in each Bucket_Data and mallocing
    each Bucket_Data individually.
*/

#define BUCKET_SIZE 2040
typedef struct bucket {
    int            nEntries;
    unsigned long  b_number;
    Bucket_Data    Entries[BUCKET_SIZE];
    struct bucket *pNext;
}   Bucket;

/* This Forms the head record of a list of Buckets.
*/
typedef struct {
    int kind;             /* Kind of bucket */
    unsigned long bg_number;
    Dwarf_Addr lower;     /* Lower value for data */
    Dwarf_Addr upper;     /* Upper value for data */
    Bucket_Data *pFirst;  /* First sentinel */
    Bucket_Data *pLast;   /* Last sentinel */
    Bucket *pHead;        /* First bucket in set */
    Bucket *pTail;        /* Last bucket in set */
} Bucket_Group;

Bucket_Group *AllocateBucketGroup(int kind);
void ReleaseBucketGroup(Bucket_Group *pBucketGroup);
void ResetBucketGroup(Bucket_Group *pBucketGroup);
void ResetSentinelBucketGroup(Bucket_Group *pBucketGroup);

void PrintBucketGroup(const char *msg,
    Bucket_Group *pBucketGroup);

void AddEntryIntoBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key,Dwarf_Addr base,Dwarf_Addr low,Dwarf_Addr high,
    const char *name, Dwarf_Bool bFlag);

Dwarf_Bool DeleteKeyInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key);
Dwarf_Bool FindAddressInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr address);
Bucket_Data *FindDataInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key);
Bucket_Data *FindKeyInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key);
Dwarf_Bool IsValidInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr pc);
void ResetLimitsBucketSet(Bucket_Group *pBucketGroup);
void SetLimitsBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr lower,Dwarf_Addr upper);
Dwarf_Bool IsValidInLinkonce(Bucket_Group *pLo,
    const char *name,Dwarf_Addr lopc,Dwarf_Addr hipc);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CHECKUTIL_H */
