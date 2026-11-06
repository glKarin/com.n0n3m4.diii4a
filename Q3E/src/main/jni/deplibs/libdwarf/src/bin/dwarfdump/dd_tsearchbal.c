/* Copyright (c) 2013-2019, David Anderson
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*  The interfaces follow tsearch (See the Single
    Unix Specification) but the implementation is
    written without reference to the source of any
    version of tsearch.

    See http://www.prevanders.net/tsearch.html
    for information and an example of use.

    Based on Knuth, chapter 6.2.2
    And based on chapter 6.2.3 Balanced Trees (sometimes
    call AVL trees) Algorithm A and the sketch on deletion.

    The wikipedia page on AVL trees is also quite useful.

    A Key equation is:
        bal-factor-node-k =
            height-left-subtree - height-right-subtree
    We don't know the absolute height, but we do know the
    balance factor of the pointed-to subtrees (-1,0, or 1).
    And we always know if we are adding or deleting a node.

    Since dwarfdump does not use the tdump facility
    of tsearchbal.c, we never define  BUILD_TDUMP
*/

#include <config.h>
#include <stdio.h>  /* fprintf() printf() sprintf() */
#include <stddef.h> /* NULL size_t */
#include <stdlib.h> /* calloc() free() malloc() */

#ifdef HAVE_STDINT_H
#include <stdint.h> /* uintptr_t */
#endif /* HAVE_STDINT_H */

#include "libdwarf.h"
#include "libdwarf_private.h"
#include "dd_tsearchbal.h"

#define IMPLEMENTD15 1
#undef BUILD_TDUMP
#ifdef DW_CHECK_CONSISTENCY
struct ts_entry;
void
ddlocal_dwarf_check_balance(struct ts_entry *head,int finalprefix);
#endif

/*  HEAD is a special record. rlink points to root node.
    As of September 2022, the following is true: The keypointer
    and balance and llink fields of the HEAD node are not used.
    The treedepth field is not used except in HEAD.

    In earlier versions, llink was used as an integer
    representing treedepth in HEAD and a pointer elsewhere.
    This lead to certain casts and pointer subtractions in
    setting tree depth that a recent compiler identified
    as having undefined reault in C. Knuth had no compiiler
    concerns because his algorithms are not in C!
*/

struct ts_entry {
    /*  Keyptr points to a pointer to a record the user saved, the
        user record contains the user's key itself
        and perhaps more. We will request free,
        so const void * is not quite right.  */
    void       *keyptr;
    /*  treedepth is only set in the HEAD node,
        and in this implementation head->llink is
        always zero. */
    int         treedepth;
    /*  balance only used in the tree proper, not in HEAD */
    int         balance; /* Knuth 6.2.3 algorithm A */
    struct ts_entry * llink;
    struct ts_entry * rlink;
};

#ifdef BUILD_TDUMP
static void
printlevel(int level)
{
    int len = 0;
    int targetlen = 4 + level;
    int shownlen = 0;
    char number[40];
    /* This is a safe sprintf. No need for esb here. */
    len = sprintf(number,"<%d>",level);
    printf("%s",number);
    shownlen = len;
    while(shownlen < targetlen) {
        putchar(' ');
        ++shownlen;
    }
}

/* For debugging, mainly.
   We print the tree with the head node unnumbered
   and the root node called level 0.
   In Knuth algorithms where we have p[k] when
   k is zero k refers to the head node.   Handy
   as then the root node is not special at all.
   But here it just looks better as shown, perhaps.

   The ordering here is so that if you turned an output
   page with the left side at the top
   then the tree sort of just shows up nicely in
   what most think of as a normal way.
*/
static void
tdump_inner(struct ts_entry *t,
    char *(keyprint)(const void *),
    const char *descr, int level)
{
    const char * keyv = "";
    if (!t) {
        return;
    }
    tdump_inner(t->rlink,keyprint,"right",level+1);

    printlevel(level);
    if (t->keyptr) {
        keyv = keyprint(t->keyptr);
    }
    printf("0x%08" DW_PR_DUx " <keyptr 0x%08" DW_PR_DUx "> "
        "<%s %s> <bal %3d> "
        "<l 0x%08" DW_PR_DUx "> <r 0x%08" DW_PR_DUx "> "
        "%s\n",
        (Dwarf_Unsigned)(uintptr_t)t,
        (Dwarf_Unsigned)(uintptr_t)t->keyptr,
        t->keyptr?"key ":"null",
        keyv,
        t->balance,
        (Dwarf_Unsigned)(uintptr_t)t->llink,
        (Dwarf_Unsigned)(uintptr_t)t->rlink,
        descr);
    tdump_inner(t->llink,keyprint,"left ",level+1);
}
#endif /* BUILD_TDUMP */
#ifdef DW_CHECK_CONSISTENCY
/*  Checking that a tree (or sub tree) is in balance.
    Only meaningful for balanced trees.
    Returns the depth.
*/
int
dd_local_check_balance_inner(struct ts_entry *t,int level,
    int maxdepth,
    int *founderror,
    const char *prefix)
{
    int l = 0;
    int r = 0;
    if (level > maxdepth) {
        printf("%s Likely internal erroneous link loop, "
            "got to depth %d.\n",
            prefix,level);
        exit(EXIT_FAILURE);
    }
    if (!t) {
        return 0;
    }
    if (!t->llink && !t->rlink) {
        if (t->balance != 0) {
            printf("%s: Balance at 0x%" DW_PR_DUx
                " should be 0 is %d.\n",
                prefix,
                (Dwarf_Unsigned)(uintptr_t)t,
                t->balance);
            (*founderror)++;
        }
        return 1;
    }
    l = dd_local_check_balance_inner(t->llink,level+1,maxdepth,
        founderror,prefix);
    r = dd_local_check_balance_inner(t->rlink,level+1,maxdepth,
        founderror,prefix);
    if (l ==r && t->balance != 0) {
        printf("%s Balance at 0x%" DW_PR_DUx
            " d should be 0 is %d.\n",
            prefix,
            (Dwarf_Unsigned)(uintptr_t)t,
            t->balance);
        (*founderror)++;
        return l+1;
    }
    if (l > r) {
        if ((l-r) != 1) {
            printf("%s depth mismatch at 0x%" DW_PR_DUx
                "  l %d r %d.\n",
                prefix,
                (Dwarf_Unsigned)(uintptr_t)t,
                l,r);
            (*founderror)++;
        }
        if (t->balance != -1) {
            printf("%s Balance at 0x%" DW_PR_DUx
                " should be -1 is %d.\n",
                prefix,
                (Dwarf_Unsigned)(uintptr_t)t,
                t->balance);
            (*founderror)++;
        }
        return l+1;
    }
    if (r != l) {
        if ((r-l) != 1) {
            printf("%s depth mismatch at 0x%" DW_PR_DUx
                " r %d l %d.\n",
                prefix,
                (Dwarf_Unsigned)(uintptr_t)t,
                r,l);
            (*founderror)++;
        }
        if (t->balance != 1) {
            printf("%s Balance at 0x%" DW_PR_DUx
                " should be 1 is %d.\n",
                prefix,
                (Dwarf_Unsigned)(uintptr_t)t,
                t->balance);
            (*founderror)++;
        }
    } else {
        if (t->balance != 0) {
            printf("%s Balance at 0x%" DW_PR_DUx
                " should be 0 is %d.\n",
                prefix,
                (Dwarf_Unsigned)(uintptr_t)t,
                t->balance);
            (*founderror)++;
        }
    }
    return r+1;
}

void
ddlocal_dwarf_check_balance(struct ts_entry *head,int finalprefix)
{
    const char *prefix = 0;
    int maxdepth = 0;
    size_t headdepth = 0;
    int errcount = 0;
    int depth = 0;
    struct ts_entry*root = 0;
    if (finalprefix) {
        prefix = "BalanceError:";
    } else {
        prefix = "BalanceWarn:";
    }

    if (!head) {
        printf("%s check balance null tree ptr\n",prefix);
        return;
    }
    root = head->rlink;
    headdepth = head->treedepth;
    if (!root) {
        printf("%s check balance null tree ptr\n",prefix);
        return;
    }

    maxdepth = headdepth+10;
    /* Counting in levels, not level number of top level. */
    headdepth++;
    depth = ddlocal_dwarf_check_balance_inner(root,depth,maxdepth,
        &errcount,prefix);
    if (depth != headdepth) {
        printf("%s Head node says depth %lu, it is really %d\n",
            prefix,
            (unsigned long)headdepth,
            depth);
        ++errcount;
    }
    if (errcount) {
        printf("%s error count %d\n",prefix,errcount);
    }
    return;
}
#endif

#ifdef BUILD_TDUMP
/*  Dumping the tree to stdout. */
void
dwarf_tdump(const void*headp_in,
    char *(*keyprint)(const void *),
    const char *msg)
{
    const struct ts_entry *head = (const struct ts_entry *)headp_in;
    struct ts_entry *root = 0;
    size_t headdepth = 0;
    if (!head) {
        printf("dumptree null tree ptr : %s\n",msg);
        return;
    }
    headdepth = head->treedepth;
    printf("dumptree head ptr : 0x%08" DW_PR_DUx
        " tree-depth %d: %s\n",
        (Dwarf_Unsigned)(uintptr_t)head,
        (int)headdepth,
        msg);
    root = head->rlink;
    if (!root) {
        printf("Empty tree\n");
        return;
    }
    tdump_inner(root,keyprint,"top",0);
}
#endif /* BUILD_TDUMP */
static void
setlink(struct ts_entry*t,int a,struct ts_entry *x)
{
    if (a < 0) {
        t->llink = x;
    } else {
        t->rlink = x;
    }
}
static struct ts_entry*
getlink(struct ts_entry*t,int a)
{
    if (a < 0) {
        return(t->llink);
    }
    return(t->rlink);
}

static struct ts_entry *
allocate_ts_entry(const void *key)
{
    struct ts_entry *e = (struct ts_entry *)
        malloc(sizeof(struct ts_entry));
    if (!e) {
        return NULL;
    }
    /*  We will eventually ask it be freed, so
        being const void * in is not quite right. */
    e->keyptr = (void *)key;
    e->balance = 0;
    e->treedepth = 0;
    e->llink = 0;
    e->rlink = 0;
    return e;
}

/* Knuth step T5, the insert. */
static struct ts_entry *
tsearch_insert_k(const void *key,int kc,
    struct ts_entry *p)
{
    struct ts_entry *q = allocate_ts_entry(key);
    if (!q) {
        /* out of memory */
        return NULL;
    }
    setlink(p,kc,q);
    /* Non-NULL means inserted. */
    return q;
}

/* Knuth step T5. */
static struct ts_entry *
tsearch_do_insert(const void *key,
    int kc,
    int * inserted,
    struct ts_entry* p)
{
    struct ts_entry *q = 0;
    q =  tsearch_insert_k(key,kc,p);
    if (q) {
        *inserted = 1;
    }
    return q;
}

/*  Algorithm A of Knuth 6.2.3, balanced tree.
    key is pointer to a user data area containing the key
    and possibly more.

    We could recurse on this routine, but instead we
    iterate (like Knuth does, but using for (;;) instead
    of go-to.  */
static struct ts_entry *
tsearch_inner( const void *key, struct ts_entry* head,
    int (*compar)(const void *, const void *),
    int*inserted)
{
    /* t points to parent of p */
    struct ts_entry *t = head;
    /* p moves down tree, p starts as root. */
    struct ts_entry *p = head->rlink;
    /* s points where rebalancing may be needed. */
    struct ts_entry *s = p;
    struct ts_entry *r = 0;
    struct ts_entry *q = 0;

    int a = 0;
    int kc = 0;
    for (;;) {
        /* A2. */
        kc = compar(key,p->keyptr);
        if (kc) {
            /* A3 and A4 handled here. */
            q = getlink(p,kc);
            if (!q) {
                /* Does step A5. */
                q = tsearch_do_insert(key,kc,inserted,p);
                if (!q) {
                    /*  Out of memory. */
                    return q;
                }
                break; /* to A5. */
            }
            if (q->balance) {
                t = p;
                /* t is no longer a pointer to HEAD */
                s = q;
            }
            p = q;
            continue;
        }
        /* K = KEY(P) in Knuth. */
        /* kc == 0, we found the entry we search for. */
        return p;
    }
    /* A5:  work already done. */

    /* A6: */
    {
        /*  Balance factors on nodes between S and Q need to be
            changed from zero to +-1  */
        int kc2 = compar(key,s->keyptr);
        if (kc2 < 0) {
            a = -1;
        } else {
            a = 1;
        }
        r = p = getlink(s,a);
        while (p != q) {
            int kc3 = compar(key,p->keyptr);
            if (kc3 < 0) {
                p->balance = -1;
                p = p->llink;
            } else if (kc3 > 0) {
                p->balance = 1;
                p = p->rlink;
            } else {
                /* ASSERT: p == q */
                break;
            }
        }
    }

    /* A7: */
    {
        if (! s->balance) {
            /* Tree has grown higher. */
            s->balance = a;
            head->treedepth++;
            return q;
        }
        if (s->balance == -a) {
            /* Tree is more balanced */
            s->balance = 0;
            return q;
        }
        if (s->balance == a) {
            /* Rebalance. */
            if (r->balance == a) {
                /* single rotation, step A8. */
                p = r;
                setlink(s,a,getlink(r,-a));
                setlink(r,-a,s);
                s->balance = 0;
                r->balance = 0;
            } else if (r->balance == -a) {
                /* double rotation, step A9. */
                p = getlink(r,-a);
                setlink(r,-a,getlink(p,a));
                setlink(p,a,r);
                setlink(s,a,getlink(p,-a));
                setlink(p,-a,s);
                if (p->balance == a) {
                    s->balance = -a;
                    r->balance = 0;
                } else if (p->balance  == 0) {
                    s->balance = 0;
                    r->balance = 0;
                } else if (p->balance == -a) {
                    s->balance = 0;
                    r->balance = a;
                }
                p->balance = 0;
            } else {
                fprintf(stderr," ERROR Impossible balanced "
                    "tree situation!\n");
                /* Impossible. Cannot be here. */
                exit(EXIT_FAILURE);
            }
        } else {
            fprintf(stderr,"ERROR Impossible balanced "
                "tree situation!!\n");
            /* Impossible. Cannot be here. */
            exit(EXIT_FAILURE);
        }
    }

    /* A10: */
    if (s == t->rlink) {
        /* ASSERT: t may or may not be HEAD  */
        t->rlink = p;
    } else {
        /* ASSERT: t is not HEAD  */
        t->llink = p;
    }
#ifdef DW_CHECK_CONSISTENCY
    ddlocal_dwarf_check_balance(head,1);
#endif
    return q;
}
/* Search and, if missing, insert. */
void *
dwarf_tsearch(const void *key, void **headin,
    int (*compar)(const void *, const void *))
{
    struct ts_entry **headp = (struct ts_entry **)headin;
    struct ts_entry *head = 0;
    struct ts_entry *r = 0;
    int inserted = 0;

    if (!headp) {
        return NULL;
    }
    head = *headp;
    if (!head) {
        struct ts_entry *root = 0;
        head = allocate_ts_entry(0);
        if (!head) {
            return NULL;
        }
        root = allocate_ts_entry(key);
        if (!root) {
            free(head);
            return NULL;
        }
        head->rlink = root;
        /* head->llink is used for the depth, as a count */
        /* head points to the special head node ... */
        *headin = head;
        return (void *)(&(root->keyptr));
    }
    r = tsearch_inner(key,head,compar,&inserted);
    if (!r) {
        return NULL;
    }
    return (void *)&(r->keyptr);
}

/* Search without insert. */
void *
dwarf_tfind(const void *key, void *const*rootp,
    int (*compar)(const void *, const void *))
{
    struct ts_entry * const *phead = (struct ts_entry * const*)rootp;
    struct ts_entry *head = 0;
    struct ts_entry *p = 0;
    if (!phead) {
        return NULL;
    }
    head = *phead;
    if (!head) {
        return NULL;
    }
    p = head->rlink;
    while(p) {
        int kc = compar(key,p->keyptr);
        if (!kc) {
            return (void *)(&(p->keyptr));
        }
        p  = getlink(p,kc);
    }
    return NULL;
}

/*  Used for an array of records used in the deletion code.
    k == 0 for the special head node which is never matched by
        input.
    k == 1 etc.
*/
struct pkrecord {
    struct ts_entry *pk;
    int              ak; /* Is -1 or +1 */
};

/*  Here we rearrange the tree so the node p to be deleted
    is a node with a null left link. With that done
    we can fix pkarray and then we can use the pkarray
    to rebalance.
    It's a bit long, so we refactor out the code from
    where it is called.
    The rearrangement is Algorithm 6.2.2D in Knuth.
    PRECONDITION: p,p->rlink, pp non-null.
    RETURNS: new high index of pkarray.
*/

static unsigned
rearrange_tree_so_p_llink_null( struct pkrecord * pkarray,
    unsigned k,
    struct ts_entry *r,
    struct ts_entry *p,
    struct ts_entry *pp,
    int ppak)
{
    struct ts_entry *s = 0;
    unsigned k2 = 0; /* indexing pkarray */
    int pbalance = p->balance;

    /* Step D3 */
    /*  Since we are going to modify the tree by
        movement of a node down the tree a ways,
        we need to build pkarray with the (not yet
        found) new next node, in pkarray[k], not
        p.
        The deletion will be of p, but by then
        p will be moved in the tree so it has a null left link.
        P's possibly-non-null right link
    */
    k2 = k;
    k2++;
    r = p->rlink;
    pkarray[k2].pk = r;
    pkarray[k2].ak = -1;
    s = r->llink;
    /* Move down and left to get a null llink. */
    while (s->llink) {
        k2++;
        r = s;
        s = r->llink;
        pkarray[k2].pk = r;
        pkarray[k2].ak = -1;
    }
    /*  Now we move S up in place (in the tree)
        of the node P we will delete.
        and p replaces s.
        Finally winding up with a newly shaped balanced tree.
        */
    {
    struct ts_entry *tmp = 0;
    int sbalance = s->balance;
    s->llink = p->llink;
    r->llink = p;
    p->llink = 0;
    tmp = p->rlink;
    p->rlink = s->rlink;
    s->rlink = tmp;
    setlink(pp,ppak,s);
    s->balance = pbalance;
    p->balance = sbalance;
    /* Now the tree is rearranged and still in balance. */

    /* Replace the previous k position entry with S.
        We trace the right link off of the moved S node. */
    pkarray[k].pk = s;
    pkarray[k].ak = 1;

    r->llink = p->rlink;
    /*  Now p is out of the tree and  we start
        the rebalance at r. pkarray Index k2. */
    }
    /* Step D4 */
    free(p);
    return k2;
}

/*  Returns deleted node parent unless the head changed.
    Returns NULL if wanted node not found or the tree
        is now empty or the head node changed.
    Sets *did_delete if it found and deleted a node.
    Sets *tree_empty if there are no more user nodes present.
*/
static struct ts_entry *
tdelete_inner(const void *key,
    struct ts_entry *head,
    int (*compar)(const void *, const void *),
    int *tree_empty,
    int *did_delete)
{
    struct ts_entry *p        = 0;
    struct ts_entry *pp       = 0;
    struct pkrecord * pkarray = 0;
    size_t depth              = head->treedepth;
    unsigned k                = 0;

    /*  Allocate extra, head is on the stack we create
        here and the depth might increase.  */
    depth = depth + 4;
    pkarray = calloc(depth, sizeof(struct pkrecord));
    if (!pkarray) {
        /* Malloc fails, we could abort... */
        return NULL;
    }
    k = 0;
    pkarray[k].pk=head;
    pkarray[k].ak=1;
    p = head->rlink;
    while(p) {
        int kc = 0;
        k++;
        kc = compar(key,p->keyptr);
        pkarray[k].pk = p;
        pkarray[k].ak = kc;
        if (!kc) {
            break;
        }
        p  = getlink(p,kc);
    }
    if (!p) {
        /* Node to delete never found. */
        free(pkarray);
        return NULL;
    }

    {
        struct ts_entry *t  = 0;
        struct ts_entry *r  = 0;
        int ppak = 0;

        p = pkarray[k].pk;
        pp = pkarray[k-1].pk;
        ppak = pkarray[k-1].ak;

        /* Found a match. p to be deleted. */
        t = p;
        *did_delete = 1;
        if (!t->rlink) {
            if (k == 1 && !t->llink) {
                *tree_empty = 1;
                /* upper level will fix up head node. */
                free(t);
                free(pkarray);
                return NULL;
            }
            /*  t->llink might be NULL. */
            setlink(pp,ppak,t->llink);
            /*  ASSERT: t->llink NULL or t->llink
                has no children, balance zero and balance
                of t->llink not changing. */
            k--;
            /* Step D4. */
            free(t);
            goto balance;
        }
#ifdef IMPLEMENTD15
        /* Step D1.5 */
        if (!t->llink) {
            setlink(pp,ppak,t->rlink);
            /* we change the left link off ak */
            k--;
            /* Step D4. */
            free(t);
            goto balance;
        }
#endif /* IMPLEMENTD15 */
        /* Step D2 */
        r = t->rlink;
        if (!r->llink) {
            /* We decrease the height of the right tree.  */
            r->llink = t->llink;
            setlink(pp,ppak,r);
            pkarray[k].pk = r;
            pkarray[k].ak = 1;
            /*  The following essential line not mentioned
                in Knuth AFAICT. */
            r->balance = t->balance;
            /* Step D4. */
            free(t);
            goto balance;
        }
        /*  Step D3, we rearrange the tree
            and pkarray so the balance step can work.
            step D2 is insufficient so not done.  */
        k = rearrange_tree_so_p_llink_null(pkarray,k,
            r,p,pp,ppak);
        goto balance;
    }
    /*  Now use pkarray decide if rebalancing
        needed and, if needed, to rebalance.
        k here matches l-1 in Knuth. */
    balance:
    {
    unsigned k2 = k;
    /*  We do not want a test in the for () itself. */
    for ( ; 1 ; k2--) {
        struct ts_entry *pk = 0;
        int ak = 0;
        int bk = 0;
        if (k2 == 0) {
            /* decreased in height */
            head->treedepth--;
            goto cleanup;
        }
        pk = pkarray[k2].pk;
        if (!pk) {
            /* Nothing here to work with. Move up. */
            continue;
        }
        ak = pkarray[k2].ak;
        bk = pk->balance;
        if (bk == ak) {
            pk->balance = 0;
            continue;
        }
        if (bk == 0) {
            pk->balance = -ak;
            goto cleanup;
        }
        /* ASSERT: bk == -ak. We
            will use bk == adel here (just below). */
        /* Rebalancing required. Here we use (1) and (2)
            in 6.2.3 to adjust the nodes */
        {
            /* Rebalance.  We use s for what
                is called A in Knuth Case 1, Case 2
                page 461.   r For what is called B.
                So the link movement logic looks similar
                to the tsearch insert case.*/
            struct ts_entry *r = 0;
            struct ts_entry *s = 0;
            struct ts_entry *pa = 0;
            int pak = 0;
            int adel = -ak;

            s = pk;
            r = getlink(s,adel);
            pa = pkarray[k2-1].pk;
            pak = pkarray[k2-1].ak;
            if (r->balance == adel) {
                /* case 1. */
                setlink(s,adel,getlink(r,-adel));
                setlink(r,-adel,s);

                /* A10 in tsearch. */
                setlink(pa,pak,r);
                s->balance = 0;
                r->balance = 0;
                continue;
            } else if (r->balance == -adel) {
                /* case 2 */
                /* x plays the role of p in step A9 */
                struct ts_entry*x = getlink(r,-adel);
                setlink(r,-adel,getlink(x,adel));
                setlink(x,adel,r);
                setlink(s,adel,getlink(x,-adel));
                setlink(x,-adel,s);

                /* A10 in tsearch. */
                setlink(pa,pak,x);
                if (x->balance == adel) {
                    s->balance = -adel;
                    r->balance = 0;
                } else if (x->balance  == 0) {
                    s->balance = 0;
                    r->balance = 0;
                } else if (x->balance == -adel) {
                    s->balance = 0;
                    r->balance = adel;
                }
                x->balance = 0;
                continue;
            } else {
                /*  r->balance == 0 case 3
                    we do a single rotation and
                    we are done. */
                setlink(s,adel,getlink(r,-adel));
                setlink(r,-adel,s);
                setlink(pa,pak,r);
                r->balance = -adel;
                /*s->balance = r->balance = 0; */
                goto cleanup;
            }
        }
    }
    }
    cleanup:
    free(pkarray);
#ifdef DW_CHECK_CONSISTENCY
    ddlocal_dwarf_check_balance(head,1);
#endif
    return pp;
}

void *
dwarf_tdelete(const void *key, void **rootp,
    int (*compar)(const void *, const void *))
{
    struct ts_entry **phead = (struct ts_entry **)rootp;
    struct ts_entry *head = 0;
    /*  If a leaf is found, we have to null a parent link
        or the root */
    struct ts_entry * parentp = 0;
    int tree_empty = 0;
    int did_delete = 0;

    if (!phead) {
        return NULL;
    }
    head = *phead;
    if (!head) {
        return NULL;
    }
    if (!head->rlink) {
        return NULL;
    }
    parentp = tdelete_inner(key,head,compar,&tree_empty,&did_delete);
    if (tree_empty) {
        head->rlink = 0;
        head->llink = 0;
        free(head);
        *phead = 0;
        return NULL;
    }
    /* ASSERT: head->rlink non-null. */
    if (did_delete) {
        if (!parentp) {
            parentp = head->rlink;
        }
        return  (void *)(&(parentp->keyptr));
    }
    /* Not deleted */
    return NULL;
}

static void
dd_local_twalk_inner(struct ts_entry *p,
    void (*action)(const void *nodep,
        const DW_VISIT which, const int depth),
    unsigned level)
{
    if (!p->llink && !p->rlink) {
        action((const void *)(&(p->keyptr)),dwarf_leaf,level);
        return;
    }
    action((const void *)(&(p->keyptr)),dwarf_preorder,level);
    if (p->llink) {
        dd_local_twalk_inner(p->llink,action,level+1);
    }
    action((const void *)(&(p->keyptr)),dwarf_postorder,level);
    if (p->rlink) {
        dd_local_twalk_inner(p->rlink,action,level+1);
    }
    action((const void *)(&(p->keyptr)),dwarf_endorder,level);
}

void
dwarf_twalk(const void *rootp,
    void (*action)(const void *nodep,
        const DW_VISIT which, const int depth))
{
    const struct ts_entry *head = (const struct ts_entry *)rootp;
    struct ts_entry *root = 0;
    if (!head) {
        return;
    }
    root = head->rlink;
    if (!root) {
        return;
    }
    /* Get to actual tree. */
    dd_local_twalk_inner(root,action,0);
}

static void
dd_local_tdestroy_inner(struct ts_entry*p,
    void (*free_node)(void *nodep),
    int depth)
{
    if (p->llink) {
        dd_local_tdestroy_inner(p->llink,free_node,depth+1);
        p->llink = 0;
    }
    if (p->rlink) {
        dd_local_tdestroy_inner(p->rlink,free_node,depth+1);
        p->rlink = 0;
    }
    free_node((void *)p->keyptr);
    free(p);
}

/*  Walk the tree, freeing all space in the tree
    and calling the user's callback function on each node.

    It is up to the caller to zero out anything pointing to
    head (ie, that has the value rootp holds) after this
    returns.
*/
void
dwarf_tdestroy(void *rootp, void (*free_node)(void *nodep))
{
    struct ts_entry *head = (struct ts_entry *)rootp;
    struct ts_entry *root = 0;
    if (!head) {
        return;
    }
    root = head->rlink;
    if (root) {
        dd_local_tdestroy_inner(root,free_node,0);
    }
    free(head);
}
