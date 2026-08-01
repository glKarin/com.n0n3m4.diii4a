/*
Copyright (C) 2011-2012 SN Systems Ltd. All Rights Reserved.
Portions Copyright (C) 2011-2019 David Anderson. All Rights Reserved.

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
/*
    These simple list-processing functions are in support
    of checking DWARF for compiler-errors of various sorts.

    These functions apply to
    pRangesInfo
        enabling checking for sanity of code addresses.
        Using .test .init .fini as one source of valid
        code ranges. And adding ranges from CUs etc.

    pVisitedInfo
        Checks that (within a CU) there are no loops
        of function references in the DWARF itself.
        Checking that we are not in an infinite loop
        following DIE references.

    pLinkOnce.
        We have no test cases for pLinkOnce as of 2022,
        gcc has linkonce sections such as
        ".gnu.linkonce.wa." but we do not have any
        to test with.
    2022.

*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <stdlib.h> /* calloc() free() */
#include <string.h> /* strcmp() */

/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */
#include <stdio.h> /* FILE decl for dd_esb.h */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "dwarf.h"
#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_defined_types.h"
#include "dd_checkutil.h"
#include "dd_glflags.h"
#include "dd_globals.h"
#include "dd_glflags.h"
#include "dd_esb.h"

/* Guessing a sensible length for max section name.  */
#define SECTION_NAME_LEN 2048

/* Private function */
static void DumpFullBucketGroup(const char *msg,
    Bucket_Group *pBucketGroup);
static unsigned long bucketgroupnext  = 0;
static unsigned long bucketnext  = 0;

static const char *
kindstring(int kind)
{
    const char *name = 0;

    switch (kind) {
    case KIND_RANGES_INFO:
        name = "KIND_RANGES_INFO";
        break;
    case KIND_LINKONCE_INFO:
        name = "KIND_LINKONCE_INFO";
        break;
    case KIND_VISITED_INFO:
        name = "KIND_VISITED_INFO";
        break;
    default:
        name = 0;
        printf("ERROR BucketGroup unknown kind of %d. Ignored\n",
            kind);
        glflags.gf_count_major_errors++;
    }
    return name;
}

Bucket_Group *
AllocateBucketGroup(int kind)
{
    Bucket_Group *pBucketGroup = (Bucket_Group *)calloc(1,
        sizeof(Bucket_Group));
    if (!pBucketGroup) {
        return NULL;
    }
    pBucketGroup->bg_number = bucketgroupnext++;
    pBucketGroup->kind = kind;
    return pBucketGroup;
}

void
ReleaseBucketGroup(Bucket_Group *pBucketGroup)
{
    Bucket *pBucket = 0;
    Bucket *pNext = 0;

    if (!pBucketGroup) {
        printf("ERROR ReleaseBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return;
    }
    for (pBucket = pBucketGroup->pHead; pBucket; pBucket = pNext ) {
        pNext = pBucket->pNext;
        free(pBucket);
    }
    pBucketGroup->pHead = NULL;
    pBucketGroup->pTail = NULL;
    free(pBucketGroup);
}

void
ResetBucketGroup(Bucket_Group *pBucketGroup)
{
    Bucket *pBucket = 0;

    if (!pBucketGroup) {
        printf("ERROR ResetBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return;
    }
    for (pBucket = pBucketGroup->pHead; pBucket;
        pBucket = pBucket->pNext) {
        pBucket->nEntries = 0;
    }
    ResetSentinelBucketGroup(pBucketGroup);
}

/* Reset sentinels in a Bucket Group. */
void
ResetSentinelBucketGroup(Bucket_Group *pBucketGroup)
{
    /* Sanity checks */
    if (!pBucketGroup) {
        printf("ERROR ResetSentinelBucketGroup passed NULL."
            " Ignored\n");
        glflags.gf_count_major_errors++;
        return;
    }
    pBucketGroup->pFirst = NULL;
    pBucketGroup->pLast = NULL;
}

void
PrintBucketGroup(const char *msg,
    Bucket_Group *pBucketGroup)
{
    if (pBucketGroup) {
        DumpFullBucketGroup(msg,pBucketGroup);
    }
}

static void
DumpFullBucketGroup(const char *msg,
    Bucket_Group *pBucketGroup)
{
    int nBucketNo = 1;
    int nIndex = 0;
    int nCount = 0;
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;
    const char *kindstr = 0;

    if (!pBucketGroup) {
        printf("ERROR BucketGroup passed in NULL. Ignored\n");
        printf("BucketGroup msg: %s\n",msg);
        glflags.gf_count_major_errors++;
        return;
    }
    kindstr = kindstring(pBucketGroup->kind);
    if (!kindstr) {
        printf("ERROR unknown bucket group kind %d\n",
            pBucketGroup->kind);
        printf("BucketGroup %s\n",msg);
        return;
    }
    printf("\nBucket Group %s\n",msg);
    printf("\nBucket Group %s index %lu"
        " [lower 0x%" DW_PR_DUx " upper 0x%" DW_PR_DUx "]\n",
        kindstr,
        pBucketGroup->bg_number,
        (Dwarf_Unsigned)pBucketGroup->lower,
        (Dwarf_Unsigned)pBucketGroup->upper);
    for (pBucket = pBucketGroup->pHead; pBucket && pBucket->nEntries;
        pBucket = pBucket->pNext) {

        printf("LowPC & HighPC records for bucket %d, at index %lu"
            " nEntries %d\n",
            nBucketNo++,
            pBucket->b_number,
            pBucket->nEntries);
        for (nIndex = 0; nIndex < pBucket->nEntries; ++nIndex) {
            pBucketData = &pBucket->Entries[nIndex];
            printf("[%06d] Key = 0x%08" DW_PR_DUx
                ", Base = 0x%08" DW_PR_DUx
                ", Low = 0x%08" DW_PR_DUx ", High = 0x%08" DW_PR_DUx
                ", Flag = %d, Name = '%s'\n",
                ++nCount,
                pBucketData->key,
                pBucketData->base,
                pBucketData->low,
                pBucketData->high,
                pBucketData->bFlag,
                pBucketData->name);
        }
    }
}

/*  Insert entry into Bucket Group.
    We make no check for duplicate information. */
void
AddEntryIntoBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key,Dwarf_Addr base,
    Dwarf_Addr low,Dwarf_Addr high,
    const char *name,
    Dwarf_Bool bFlag)
{
    Bucket *pBucket = 0;
    Bucket_Data data;

    data.bFlag = bFlag;
    data.name = name;
    data.key = key;
    data.base = base;
    data.low = low;
    data.high = high;

    if (!pBucketGroup) {
        printf("ERROR AddEntryIntoBucketGroup passed NULL."
            " Ignored\n");
        glflags.gf_count_major_errors++;
        return;
    }
    if (!pBucketGroup->pHead) {
        /* Allocate first bucket */
        pBucket = (Bucket *)calloc(1,sizeof(Bucket));
        if (!pBucket) {
            return;
        }
        pBucket->b_number = bucketnext++;
        pBucketGroup->pHead = pBucket;
        pBucketGroup->pTail = pBucket;
        pBucket->nEntries = 1;
        pBucket->Entries[0] = data;
        return;
    }
    pBucket = pBucketGroup->pTail;

    /*  Check if we have a previous allocated set of
        buckets (that have been cleared) */
    if (pBucket->nEntries) {
        if (pBucket->nEntries < BUCKET_SIZE) {
            pBucket->Entries[pBucket->nEntries] = data;
            pBucket->nEntries++;
        } else {
            /* Allocate new bucket */
            pBucket = (Bucket *)calloc(1,sizeof(Bucket));
            if (!pBucket) {
                return;
            }
            pBucketGroup->pTail->pNext = pBucket;
            pBucketGroup->pTail = pBucket;
            pBucket->b_number = bucketnext++;
            pBucket->nEntries = 1;
            pBucket->Entries[0] = data;
        }
    } else {
        /*  We have an allocated bucket with zero entries;
            search for the
            first available bucket to be used as the current
            insertion point */
        for (pBucket = pBucketGroup->pHead; pBucket;
            pBucket = pBucket->pNext) {

            if (pBucket->nEntries < BUCKET_SIZE) {
                pBucket->Entries[pBucket->nEntries] = data;
                pBucket->nEntries++;
                break;
            }
        }
    }
}

/*  For Groups where entries are individually deleted, this does
    that work.  */
Dwarf_Bool
DeleteKeyInBucketGroup(Bucket_Group *pBucketGroup,Dwarf_Addr key)
{
    int nIndex = 0;
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;

    if (!pBucketGroup) {
        printf("ERROR DeleteKeyInBucketGroup passed NULL. "
            "Ignored\n");
        glflags.gf_count_major_errors++;
        return FALSE;
    }

    /* For now do a linear search */
    for (pBucket = pBucketGroup->pHead; pBucket && pBucket->nEntries;
        pBucket = pBucket->pNext) {

        for (nIndex = 0; nIndex < pBucket->nEntries; ++nIndex) {
            pBucketData = &pBucket->Entries[nIndex];
            if (pBucketData->key == key) {
                Bucket_Data data = {FALSE,NULL,0,0,0,0};
                int nStart;
                for (nStart = nIndex + 1; nStart < pBucket->nEntries;
                    ++nStart) {

                    pBucket->Entries[nIndex] =
                        pBucket->Entries[nStart];
                    ++nIndex;
                }
                pBucket->Entries[nIndex] = data;
                --pBucket->nEntries;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*  Search to see if the address is in the range between
    low and high addresses in some Bucket Data record.
    Checks if 'address' is inside some bucket entry.
    This matches == if high is exact match
    (which usually means one-past-true-high).  */
Dwarf_Bool
FindAddressInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr address)
{
    int nIndex = 0;
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;

    if (!pBucketGroup) {
        printf("ERROR FindAdressinBucketGroup passed NULL. "
            "Ignored\n");
        glflags.gf_count_major_errors++;
        return FALSE;
    }
    /* For now do a linear search */
    for (pBucket = pBucketGroup->pHead; pBucket && pBucket->nEntries;
        pBucket = pBucket->pNext) {

        for (nIndex = 0; nIndex < pBucket->nEntries; ++nIndex) {
            pBucketData = &pBucket->Entries[nIndex];
            if (address >= pBucketData->low &&
                address <= pBucketData->high) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*  Search an entry (Bucket Data) in the Bucket Set */
Bucket_Data *FindDataInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key)
{
    int mid = 0;
    int low = 0;
    int high = 0;
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;

    if (!pBucketGroup) {
        printf("ERROR FindDataInBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return 0;
    }

    for (pBucket = pBucketGroup->pHead; pBucket;
        pBucket = pBucket->pNext) {
        /* Get lower and upper references */
        if (pBucket->nEntries) {
            low = 0;
            high = pBucket->nEntries;
            while (low < high) {
                mid = low + (high - low) / 2;
                if (pBucket->Entries[mid].key < key) {
                    low = mid + 1;
                } else {
                    high = mid;
                }
            }
            if ((low < pBucket->nEntries) &&
                (pBucket->Entries[low].key == key)) {

                pBucketData = &pBucket->Entries[low];
                /* Update sentinels to allow traversing the table */
                if (!pBucketGroup->pFirst) {
                    pBucketGroup->pFirst = pBucketData;
                }
                pBucketGroup->pLast = pBucketData;
                return pBucketData;
            }
        }
    }
    return (Bucket_Data *)NULL;
}

/*  Search an entry (Bucket Data) in the Bucket Group.
    The key is an offset, a DIE offset
    within Visited info. */
Bucket_Data *FindKeyInBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr key)
{
    int nIndex = 0;
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;

    /* Sanity checks */
    if (!pBucketGroup) {
        printf("ERROR FindKeyInBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return 0;
    }

    /* For now do a linear search */
    for (pBucket = pBucketGroup->pHead; pBucket && pBucket->nEntries;
        pBucket = pBucket->pNext) {
        for (nIndex = 0; nIndex < pBucket->nEntries; ++nIndex) {
            pBucketData = &pBucket->Entries[nIndex];
            if (pBucketData->key == key) {
                return pBucketData;
            }
        }
    }
    return (Bucket_Data *)NULL;
}

/*  Search an entry (Bucket Data) in the Bucket Set by name.
    Used to find link-once section names,
    so the bucket group passed in is pLinkOnce. */
static Bucket_Data *
FindNameInBucketGroup(Bucket_Group *pBucketGroup,char *name)
{
    int nIndex = 0;
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;

    if (!pBucketGroup) {
        printf("ERROR FindnameInBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return 0;
    }
    /* For now do a linear search. */
    for (pBucket = pBucketGroup->pHead; pBucket && pBucket->nEntries;
        pBucket = pBucket->pNext) {
        for (nIndex = 0; nIndex < pBucket->nEntries; ++nIndex) {
            pBucketData = &pBucket->Entries[nIndex];
            if (!strcmp(pBucketData->name,name)) {
                return pBucketData;
            }
        }
    }
    return (Bucket_Data *)NULL;
}

/*  Check if an address valid or not. That is,
    check if it is in  the lower -> upper range of a bucket.
    It checks <= and >= so the lower end
    and  one-past on the upper end matches.

    Used with pRangesInfo
*/
Dwarf_Bool
IsValidInBucketGroup(Bucket_Group *pBucketGroup,Dwarf_Addr address)
{
    Bucket *pBucket = 0;
    Bucket_Data *pBucketData = 0;
    int nIndex = 0;

    if (!pBucketGroup) {
        printf("ERROR IsValidInBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return FALSE;
    }
    /* Check the address is within the allowed limits */
    if (address >= pBucketGroup->lower &&
        address <= pBucketGroup->upper) {
        pBucket = pBucketGroup->pHead;
        for ( ;
            pBucket && pBucket->nEntries;
            pBucket = pBucket->pNext) {
            for (nIndex = 0; nIndex < pBucket->nEntries; ++nIndex) {
                pBucketData = &pBucket->Entries[nIndex];
                if (address >= pBucketData->low &&
                    address <= pBucketData->high) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/*  Reset limits for values in the Bucket Set */
void
ResetLimitsBucketSet(Bucket_Group *pBucketGroup)
{
    if (!pBucketGroup) {
        printf("ERROR ResetLimitsBucketSet passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return;
    }
    pBucketGroup->lower = 0;
    pBucketGroup->upper = 0;
}

/*  Limits are set only for ranges, so only in pRangesInfo.
    But is used for ranges and location lists.
    The default is set from object data
    (virt addr, size in object file) but that does not work
    sensibly in PE object files. */
void
SetLimitsBucketGroup(Bucket_Group *pBucketGroup,
    Dwarf_Addr lower,Dwarf_Addr upper)
{
    if (!pBucketGroup) {
        printf("ERROR SetLimitsBucketGroup passed NULL. Ignored\n");
        glflags.gf_count_major_errors++;
        return;
    }
    if (lower < upper) {
        pBucketGroup->lower = lower;
        pBucketGroup->upper = upper;
    }
}

/*  Check if a given (lopc,hipc) are valid for a linkonce.
    We pass in the linkonce  (instead of
    referencing the global pLinkonceInfo) as that means
    searches for pLinkonceInfo find all the uses,
    making understanding of the code a tiny bit easier.
    The section name created is supposed to be the appropriate
    linkonce section name.
*/
Dwarf_Bool IsValidInLinkonce(Bucket_Group *pLo,
    const char *name,Dwarf_Addr lopc,Dwarf_Addr hipc)
{
    static char section_name[SECTION_NAME_LEN];
    Bucket_Data *pBucketData = 0;
    /*  Since text is quite uniformly just this name,
        no need to get it
        from elsewhere, though it will not work for non-elf.  */
    const char *lo_text = ".text.";

    /*  Build the name that represents the linkonce section (.text).
        This is not defined in DWARF so not correct for all
        compilers. */
    struct esb_s sn;

    esb_constructor_fixed(&sn,section_name,sizeof(section_name));
    esb_append(&sn,lo_text);
    esb_append(&sn,name);
    pBucketData = FindNameInBucketGroup(pLo,esb_get_string(&sn));
    esb_destructor(&sn);
    if (pBucketData) {
        if (lopc >= pBucketData->low && lopc <= pBucketData->high) {
            if (hipc >= pBucketData->low &&
                hipc <= pBucketData->high) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
