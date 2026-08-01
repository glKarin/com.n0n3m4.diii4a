/*
  Copyright 2011-2020 David Anderson. All Rights Reserved.

  This trivial test program is hereby placed in the public domain.
*/

#include <config.h>

#include <stdio.h>  /* printf() */
#include <string.h>  /* strcmp() */
#include "dd_canonical_append.h"
#include "dd_minimal.h"

void dd_minimal_count_global_error(void) {}

#define CANBUF 25
static struct canap_s {
    char *res_exp;
    char *first;
    char *second;
} canap[] = {
    {
    "ab/c", "ab", "c"}, {
    "ab/c", "ab/", "c"}, {
    "ab/c", "ab", "/c"}, {
    "ab/c", "ab////", "/////c"}, {
    "ab/", "ab", ""}, {
    "ab/", "ab////", ""}, {
    "ab/", "ab////", ""}, {
    "/a", "", "a"}, {
    0, "/abcdefgbijkl", "pqrstuvwxyzabcd"}, {
    0, 0, 0}
};

static int
test_canonical_append(void)
{
    /* Make buf big, this is test code, so be safe. */
    char lbuf[1000];
    unsigned i;
    int failcount = 0;

    printf("Entry test_canonical_append\n");
    for (i = 0;; ++i) {
        char *res = 0;

        if (canap[i].first == 0 && canap[i].second == 0)
            break;

        res = _dwarf_canonical_append(lbuf, CANBUF, canap[i].first,
            canap[i].second);
        if (res == 0) {
            if (canap[i].res_exp == 0) {
                /* GOOD */
                printf("PASS %u\n", i);
            } else {
                ++failcount;
                printf("FAIL: entry %u wrong, expected "
                    "%s, got NULL\n",
                    i, canap[i].res_exp);
            }
        } else {
            if (canap[i].res_exp == 0) {
                ++failcount;
                printf("FAIL: entry %u wrong, got %s "
                    "expected NULL\n",
                    i, res);
            } else {
                if (strcmp(res, canap[i].res_exp) == 0) {
                    printf("PASS %u\n", i);
                    /* GOOD */
                } else {
                    ++failcount;
                    printf("FAIL: entry %u wrong, "
                        "expected %s got %s\n",
                        i, canap[i].res_exp, res);
                }
            }
        }
    }
    if (failcount) {
        printf("FAIL count %u\n", failcount);
    }
    return failcount;
}

int main(void)
{
    int errs = 0;

    errs += test_canonical_append();
    if (errs) {
        printf("FAIL. canonical path test errors\n");
        return 1;
    }
    printf("PASS canonical path tests\n");
    return 0;
}
