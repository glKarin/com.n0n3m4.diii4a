/*  This is for testing the local getopt.
    This test code is hereby placed into the
    public domain by David Anderson.*/

#ifdef GETOPT_FROM_SYSTEM
#define dwgetopt  getopt
#define dwgetopt_long  getopt_long
#define dwopterr  opterr
#define dwoptind  optind
#define dwoptopt  optopt
#define dwopterr  opterr
#define dwoptarg  optarg

/*  Leave dw_noargument etc to be defined
    as 0,1,2 as that matches system getopt.h */
#endif

#include "config.h"
#include <stdio.h>
#include <string.h> /* for strchr etc */
#include <stdlib.h> /* for exit() */
#include "dd_getopt.h" /* for dwgetopt in dwarfdump */
#include "dd_minimal.h"
char *argv1[20];

/* fake as we are not counting errors for dwarfdump. */
void dd_minimal_count_global_error(void) {}

/*  Use for testing dwgetopt only.
    Not a standard function. */
static void
dwgetoptresetfortestingonly(void)
{
    dwopterr   = 1;
    dwoptind   = 1;
    dwoptopt   = 0;
    dwoptreset = 0;
    dwoptarg   = 0;
}

static int turnprintable(int c)
{
    if ( c >= 0x20 && c <= 0x7f) {
        return c;
    }
    return '.';
}

static void
printcheckargs(
    int ct,
    int rchar,int xchar,
    char *roptarg,char *xoptarg,
    int roptind,int xoptind,
    const char *testid,
    int testline)
{
    printf("chkval entry ct %d test %s line %d\n",ct,testid,testline);
    printf("       rchar 0x%x('%c') xchar 0x%x('%c') roptarg "
        "%s xoptarg %s\n",
        rchar,turnprintable(rchar),
        xchar,turnprintable(xchar),
        roptarg?roptarg:"",xoptarg?xoptarg:"");
    printf("       roptind %d xoptind %d\n",roptind,xoptind);
}

/* for rchar read the real int/char returned.
   for xchar read the expected int/char.

   for roptarg, read dwoptarg (the real optarg val),
   for xoptarg read expected-optarg

   for roptind read dwoptind,
   for xoptind read expected-optind
*/
static void
chkval(
    int ct,
    int rchar,int xchar,
    char *roptarg,char *xoptarg,
    int roptind,int xoptind,
    const char *testid,
    int testline)
{
    int err = 0;

    if (rchar != xchar) {
        int pr = turnprintable(rchar);
        int px = turnprintable(xchar);
        err++;
        printf("Mismatch %d %s line  %d: got char %c "
            "%d 0x%x; exp char %c %d 0x%x\n",
            ct,testid,testline,pr,rchar,rchar,px,xchar,xchar);
    }
    if (roptarg != xoptarg) {
        /* pointers non-match */
        if (roptarg && xoptarg && (!strcmp(roptarg,xoptarg))) {
            /* strings match. */
        } else {
            err++;
            printf("Mismatch %d %s line %d: got dwoptarg "
                "%s 0x%p exp optarg %s 0x%p\n",
                ct,testid,testline,
                roptarg?roptarg:"",
                (void*)roptarg,
                xoptarg?xoptarg:"",
                (void*)xoptarg);
        }
    }
    if (roptind != xoptind) {
        err++;
        printf("Mismatch %d %s line %d: got dwoptind %d "
            "0x%x exp optind %d 0x%x\n",
            ct,testid,testline,
            roptind,roptind,xoptind,xoptind);
    }
    if (err > 0) {
        printcheckargs(ct,rchar,xchar,roptarg,xoptarg,
            roptind,xoptind,testid,testline);
        printf("FAIL getopttest %s line %d\n",testid,testline);
        exit(EXIT_FAILURE);
    }
}

static void chkval_long(
    int ct,
    int rchar,int xchar,
    char *roptarg,char *xoptarg,
    int roptind,int xoptind,
    int rlongindex, int xlongindex,
    const char *testid,int testline)
{
    int err = 0;

    chkval(ct,rchar,xchar,roptarg,xoptarg,roptind,xoptind,
        testid,testline);
    if (rlongindex != xlongindex) {
        printf("chkval_long entry ct %d test %s line %d\n",ct,
            testid,testline);
        printf("       rlongindex %d xlongindex %d\n",
            rlongindex,xlongindex);
        ++err;
        printf("Mismatch %d %s line %d:  on longopt longindex "
            "got %d expected %d\n",
            ct,testid,testline,
            rlongindex,xlongindex);
    }
    if (err > 0) {
        printf("FAIL getopttest %s line %d\n",testid,testline);
        exit(EXIT_FAILURE);
    }
}

static int
test3(void)
{
    int ct = 1;
    int c = 0;
    int argct = 8;
    argv1[0]="a.out";
    argv1[1]="-a";
    argv1[2]="-#bx";
    argv1[3]="-b";
    argv1[4]="-c";
    argv1[5]="-cfoo";
    argv1[6]="-d";
    argv1[7]="progtoread";
    argv1[8]=0;
    for ( ;(c = dwgetopt(argct, argv1, "#:abc::d")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,2,"test31",__LINE__);
        break;
    case 2:
        chkval(ct,c,'#',dwoptarg,"bx",dwoptind,3,"test32",__LINE__);
        break;
    case 3:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,4,"test33",__LINE__);
        break;
    case 4:
        chkval(ct,c,'c',dwoptarg,0,dwoptind,5,"test34",__LINE__);
        break;
    case 5:
        chkval(ct,c,'c',dwoptarg,"foo",dwoptind,6,"test35",__LINE__);
        break;
    case 6:
        chkval(ct,c,'d',dwoptarg,0,dwoptind,7,"test36",__LINE__);
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    default:
        printf("FAIL test3 unexpected ct %d\n",ct);
    }
    }
    if (strcmp(argv1[dwoptind],"progtoread")) {
        printf("FAIL test3 on non-dash dwoptind %d arg got "
            "%s exp %s\n",
            dwoptind,argv1[dwoptind],"progtoread");
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test 3\n");
    return 0;
}

static int
test2(void)
{
    int ct = 1;
    int c = 0;
    int argct = 5;
    argv1[0]="a.out";
    argv1[1]="-a";
    argv1[2]="-#pound";
    argv1[3]="-b";
    argv1[4]="-cfilename";
    argv1[5]=0;
    for ( ;(c = dwgetopt(argct, argv1,
        "#:abc::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,2,"test21",__LINE__);
        break;
    case 2:
        chkval(ct,c,'#',dwoptarg,"pound",dwoptind,3,"test22",
            __LINE__);
        break;
        break;
    case 3:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,4,"test23",__LINE__);
        break;
    case 4:
        chkval(ct,c,'c',dwoptarg,"filename",dwoptind,5,"test24",
            __LINE__);
        break;
    default:
        printf("FAIL test2 unexpected ct %d\n",ct);
        exit(EXIT_FAILURE);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test2 on non-dash arg dwoptind %d "
            "got 0x%p exp NULL\n",
            dwoptind,(void *)argv1[dwoptind]);
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test 2\n");
    return 0;
}

static int
ltest1(void)
{
    int ct = 1;
    int c = 0;
    int argct = 13;
    static struct dwoption  longopts[] =  {
        {"simplelong",       dwno_argument,      0,1000},
        {"optarglong",       dwoptional_argument,0,1001},
        {"requireoptarglong",dwrequired_argument,0,1002},
        {0,0,0,0}
    };
    int longindex = 0;

    argv1[0]="a.out";
    argv1[1]="-ab";
    argv1[2] = "--simplelong";
    argv1[3] = "--optarglong";
    argv1[4] = "--requireoptarglong=val";
    argv1[5] = "-cd";
    argv1[6]="progtoread";
    argv1[7]=0;
    for ( ;(c = dwgetopt_long(argct, argv1,
        "abcd",
        longopts,&longindex)) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,1,"ltest1.1",__LINE__);
        break;
    case 2:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,2,"ltest1.2",__LINE__);
        break;

    case 3:
        chkval_long(ct,c,1000,dwoptarg,0,dwoptind,3,longindex,0,
            "ltest1.3",__LINE__);
        break;
    case 4:
        chkval_long(ct,c,1001,dwoptarg,0,dwoptind,4,longindex,1,
            "ltest1.4",__LINE__);
        break;
    case 5:
        chkval_long(ct,c,1002,dwoptarg,"val",dwoptind,5,longindex,2,
            "ltest1.5",__LINE__);
        break;
    case 6:
        chkval(ct,c,'c',dwoptarg,0,dwoptind,5,"ltest1.6",__LINE__);
        break;
    case 7:
        chkval(ct,c,'d',dwoptarg,0,dwoptind,6,"ltest1.7",__LINE__);
        break;

    default:
        printf("FAIL ltest1 unexpected ct %d in ltest1\n",ct);
        exit(EXIT_FAILURE);
    }
    }
    if (strcmp(argv1[dwoptind],"progtoread")) {
        printf("FAIL ltest1 on non-dash arg dwoptind %d got "
            "%s exp %s\n",
            dwoptind,argv1[dwoptind],"progtoread");
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt ltest1\n");
    return 0;
}

static int
ltest2(void)
{
    int ct = 1;
    int c = 0;
    int argct = 13;
    static struct dwoption  longopts[] =  {
        {"optarglong",       dwoptional_argument,0,6},
        {0,0,0,0}
    };
    int longindex = 0;

    argv1[0]="a.out";
    argv1[1]="-ab";
    argv1[2] = "--optarglong";
    argv1[3] = "--optarglong=";
    argv1[4] = "--optarglong=val";
    argv1[5] = "-cd";
    argv1[6]="progtoread";
    argv1[7]=0;
    for ( ;(c = dwgetopt_long(argct, argv1,
        "abcd",
        longopts,&longindex)) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,1,"ltest21",__LINE__);
        break;
    case 2:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,2,"ltest22",__LINE__);
        break;

    case 3:
        chkval_long(ct,c,6,dwoptarg,0,dwoptind,3,longindex,0,
            "ltest23",__LINE__);
        break;
    case 4:
        chkval_long(ct,c,6,dwoptarg,0,dwoptind,4,longindex,0,
            "ltest24",__LINE__);
        break;
    case 5:
        chkval_long(ct,c,6,dwoptarg,"val",dwoptind,5,longindex,0,
            "ltest25",__LINE__);
        break;
    case 6:
        chkval(ct,c,'c',dwoptarg,0,dwoptind,5,"ltest26",__LINE__);
        break;
    case 7:
        chkval(ct,c,'d',dwoptarg,0,dwoptind,6,"ltest27",__LINE__);
        break;

    default:
        printf("FAIL ltest1 unexpected ct %d in ltest2\n",ct);
        exit(EXIT_FAILURE);
    }
    }
    if (strcmp(argv1[dwoptind],"progtoread")) {
        printf("FAIL ltest2 on non-dash arg dwoptind %d got "
            "%s exp %s\n",
            dwoptind,argv1[dwoptind],"progtoread");
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt ltest2\n");
    return 0;
}

static int
test1(void)
{
    int ct = 1;
    int c = 0;
    int argct = 13;
    argv1[0]="a.out";
    argv1[1]="-a";
    argv1[2]="-#";
    argv1[3]="6";
    argv1[4]="-H1";
    argv1[5]="-H";
    argv1[6]="2";
    argv1[7]="-ka";
    argv1[8]="-l";
    argv1[9]="-lv";
    argv1[10]="-x";
    argv1[11]="path=./foo";
    argv1[12]="progtoread";
    argv1[13]=0;
    for ( ;(c = dwgetopt(argct, argv1,
        "#:abc::CdDeE::fFgGhH:iIk:l::mMnNo::O:pPqQrRsS:t:"
        "u:UvVwW::x:yz"))
        != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,2,"test11",__LINE__);
        break;
    case 2:
        chkval(ct,c,'#',dwoptarg,"6",dwoptind,4,"test12",__LINE__);
        break;
    case 3:
        chkval(ct,c,'H',dwoptarg,"1",dwoptind,5,"test13",__LINE__);
        break;
    case 4:
        chkval(ct,c,'H',dwoptarg,"2",dwoptind,7,"test14",__LINE__);
        break;
    case 5:
        chkval(ct,c,'k',dwoptarg,"a",dwoptind,8,"test15",__LINE__);
        break;
    case 6:
        chkval(ct,c,'l',dwoptarg,0,dwoptind,9,"test16",__LINE__);
        break;
    case 7:
        chkval(ct,c,'l',dwoptarg,"v",dwoptind,10,"test17",__LINE__);
        break;
    case 8:
        chkval(ct,c,'x',dwoptarg,"path=./foo",dwoptind,12,"test18",
            __LINE__);
        break;
    default:
        printf("FAIL test1 unexpected ct %d in test1\n",ct);
        exit(EXIT_FAILURE);
    }
    }
    if (strcmp(argv1[dwoptind],"progtoread")) {
        printf("FAIL test1 on non-dash arg dwoptind %d got "
            "%s exp %s\n",
            dwoptind,argv1[dwoptind],"progtoread");
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test1\n");
    return 0;
}

static int
test5(void)
{
    int ct = 1;
    int c = 0;
    int argct = 8;
    argv1[0]="a.out";
    argv1[1]="-ab";
    argv1[2]="-a";
    argv1[3]="-cx";
    argv1[4]="-c";
    argv1[5]="y";
    argv1[6]="-d";
    argv1[7]="-da";
    argv1[8]=0;
    for ( ;(c = dwgetopt(argct, argv1, "abc:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,1,"test51",__LINE__);
        break;
    case 2:
        chkval(ct,c,'b',dwoptarg,0,dwoptind,2,"test52",__LINE__);
        break;
    case 3:
        chkval(ct,c,'a',dwoptarg,0,dwoptind,3,"test53",__LINE__);
        break;
    case 4:
        chkval(ct,c,'c',dwoptarg,"x",dwoptind,4,"test54",__LINE__);
        break;
    case 5:
        chkval(ct,c,'c',dwoptarg,"y",dwoptind,6,"test55",__LINE__);
        break;
    case 6:
        chkval(ct,c,'d',dwoptarg,0,dwoptind,7,"test56",__LINE__);
        break;
    case 7:
        chkval(ct,c,'d',dwoptarg,"a",dwoptind,8,"test17",__LINE__);
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char "
            "0x%x %c\n",ct,c,c);
        exit(EXIT_FAILURE);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test5 there is a non-dash arg dwoptind "
            "%d got 0x%p\n",
            dwoptind,(void *)argv1[dwoptind]);
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test5\n");
    return 0;
}

static int
test6(void)
{
    int ct = 1;
    int c = 0;
    int argct = 2;
    argv1[0]="a.out";
    argv1[1]="-H";
    argv1[2]=0;
    for ( ;(c = dwgetopt(argct, argv1, "abH:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'?',dwoptarg,0,dwoptind,2,"test61",__LINE__);
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char "
            "0x%x %c\n",ct,c,c);
        exit(EXIT_FAILURE);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test6 there is a non-dash arg dwoptind "
            "%d got 0x%p\n",
            dwoptind,(void*)argv1[dwoptind]);
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test6\n");
    return 0;
}
/*  Leading : in opt string */
static int
test7(void)
{
    int ct = 1;
    int c = 0;
    int argct = 2;
    argv1[0]="a.out";
    argv1[1]="-H";
    argv1[2]=0;
    for ( ;(c = dwgetopt(argct, argv1, ":abH:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,':',dwoptarg,0,dwoptind,2,"test7.1",__LINE__);
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char 0x%x %c\n",
            ct,c,c);
        exit(EXIT_FAILURE);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test7 there is a non-dash arg dwoptind "
            "%d got 0x%p\n",
            dwoptind,(void*)argv1[dwoptind]);
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test7\n");
    return 0;
}
static int
test8(void)
{
    int ct = 1;
    int c = 0;
    int argct = 2;
    argv1[0]="a.out";
    argv1[1]="-x";
    argv1[2]=0;
    /*  We expect a ? because the arg list is improper. */
    for ( ;(c = dwgetopt(argct, argv1, "abH:d::")) != EOF; ct++) {
    switch(ct) {
    case 1:
        chkval(ct,c,'?',dwoptarg,0,dwoptind,2,"test8.1",__LINE__);
        break;
    default:
        printf("FAIL test5 unexpected ct %d in test1 char 0x%x %c\n",
            ct,c,c);
        exit(EXIT_FAILURE);
    }
    }
    if (argv1[dwoptind]) {
        printf("FAIL test8 there is a non-dash arg "
            "dwoptind %d got 0x%p\n",
            dwoptind,(void*)argv1[dwoptind]);
        exit(EXIT_FAILURE);
    }
    printf("PASS getopt test8\n");
    return 0;
}

static int
test9(void)
{
    int ct = 1;
    int c = 0;
    int argct = 13;
    /*  If an argument is required and there is no =
        in the input, the next argv is taken as the optarg.
        This was found by experiment. */
    static struct dwoption  longopts[] =  {
        {"simplelong",       dwno_argument,      0,1},
        {"optarglong",       dwoptional_argument,0,2},
        {"requireoptarglong",dwrequired_argument,0,3},
        {0,0,0,0}
    };
    int longindex = 0;
    int stop = 0;

    argv1[0]="a.out";
    argv1[1]="-acb";
    argv1[2] = "--bogusarg";
    argv1[3] = "-cd";
    argv1[4]="progtoread";
    argv1[5]=0;
    for ( ;(c = dwgetopt_long(argct, argv1,
        "abcd",
        longopts,&longindex)) != EOF; ct++) {

        switch(ct) {
        case 1:
            chkval(ct,c,'a',dwoptarg,0,dwoptind,1,
                "ltest9.1",__LINE__);
            break;
        case 2:
            chkval(ct,c,'c',dwoptarg,0,dwoptind,1,
                "ltest9.2",__LINE__);
            break;
        case 3:
            chkval(ct,c,'b',dwoptarg,0,dwoptind,2,
                "ltest9.2",__LINE__);
            break;
        case 4:
            /* invalid long option */
            chkval_long(ct,c,'?',dwoptarg,0,dwoptind,3,
                longindex,0,
                "ltest9.4",__LINE__);
            stop = 1;
            break;
        default:
            break;
        }
        if (stop) {
            break;
        }
    }
    printf("PASS getopt test9\n");
    return 0;
}

static int
test10(void)
{
    int ct = 1;
    int c = 0;
    int argct = 13;
    /*  If an argument is required and there is no =
        in the input, the next argv is taken as the optarg.
        This was found by experiment. */
    static struct dwoption  longopts[] =  {
        {"simplelong",       dwno_argument,      0,1},
        {"optarglong",       dwoptional_argument,0,2},
        {"requireoptarglong",dwrequired_argument,0,3},
        {0,0,0,0}
    };
    int longindex = 0;
    int done=0;

    argv1[0]="a.out";
    argv1[1]="-acb";
    argv1[2] = "--optarglong";
    argv1[3] = "--simplelong=oops";
    argv1[4] = "-cd";
    argv1[5]="progtoread";
    argv1[6]=0;

    for ( ;(c = dwgetopt_long(argct, argv1,
        "abcd",
        longopts,&longindex)) != EOF; ct++) {

        switch(ct) {
        case 1:
            chkval(ct,c,'a',dwoptarg,0,dwoptind,1,
                "ltest10.1",__LINE__);
            break;
        case 2:
            chkval(ct,c,'c',dwoptarg,0,dwoptind,1,
                "ltest10.2",__LINE__);
            break;
        case 3:
            chkval(ct,c,'b',dwoptarg,0,dwoptind,2,
                "ltest10.3",__LINE__);
            break;
        case 4:
            chkval_long(ct,c,2,dwoptarg,0,dwoptind,3,
                longindex,1,
                "ltest10.4",__LINE__);
            break;
        case 5:
            /*  This one has error . =arg when
                none allowed */
            chkval_long(ct,c,'?',dwoptarg,0,
                dwoptind,4,
                longindex,1,
                "ltest10.5",__LINE__);
            done=1;
            break;
        default:
            break;
        }
        if (done) {
            break;
        }
    }
    printf("PASS getopt test10\n");
    return 0;
}

int main(int argc, const char **argv)
{
    int ct = 0;
    int failct = 0;
    printf("argc: %d\n",argc);
    for (ct = 0; ct < argc ; ++ct) {
        printf("argv[%d] = %s\n",ct,argv[ct]);
    }
    if ( argc == 3) {
        int num = 0;
        if (strcmp(argv[1],"-c")) {
            printf("FAIL: invalid arg list\n");
            exit(EXIT_FAILURE);
        }
        num = atoi(argv[2]);
        printf("Run one test, number %d\n",num);
        switch(num) {
        case 1:
            failct = test1();
            break;
        case 2:
            failct = test2();
            break;
        case 3:
            failct = test3();
            break;
        case 5:
            failct = test5();
            break;
        case 6:
            failct = test6();
            break;
        case 7:
            failct = test7();
            break;
        case 8:
            failct = test8();
            break;
        case 9:
            failct = test9();
            break;
        case 10:
            failct = test10();
            break;
        default:
            printf("FAIL: invalid test number %d\n",num);
            exit(EXIT_FAILURE);
        }
        if ( failct) {
            printf("FAIL getopttest\n");
            exit(EXIT_FAILURE);
        }
        printf("PASS getopttest\n");
        exit(EXIT_SUCCESS);
    } else if (argc != 1) {
        printf("FAIL: invalid arg list\n");
            exit(EXIT_FAILURE);
    }
    failct += ltest1();
    dwgetoptresetfortestingonly();
    failct += ltest2();
    dwgetoptresetfortestingonly();
    failct += test5();
    dwgetoptresetfortestingonly();
    failct += test1();
    dwgetoptresetfortestingonly();
    failct += test2();
    dwgetoptresetfortestingonly();
    failct += test3();
    dwgetoptresetfortestingonly();
    failct += test6();
    dwgetoptresetfortestingonly();
    failct += test7();
    dwgetoptresetfortestingonly();
    failct += test8();
    dwgetoptresetfortestingonly();
    failct += test9();
    dwgetoptresetfortestingonly();
    failct += test10();
    if ( failct) {
        printf("FAIL getopttest\n");
        exit(EXIT_FAILURE);
    }
    printf("PASS getopttest\n");
    return 0;
}
