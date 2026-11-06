/* Thanks to David Grason
   gist.github.com/DavidEGrayson/06cf7ea73f82a05490ba

   Any errors here were added by me in my modifications.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <limits.h>

int failcount = 0;

#undef LLONG_MAX
#undef LLONG_MIN
#undef INT64_MAX
#undef INT64_MIN
#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif
#ifndef LLONG_MIN
#define LLONG_MIN    (-LLONG_MAX - 1LL)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX   18446744073709551615ULL
#endif
#define INT64_MAX LLONG_MAX
#define INT64_MIN LLONG_MIN

/*  In libdwarf, Dwarf_Signed is a 64bit integer.
    No lesser size (or greater size) is supported
    at this time.
*/

/*  Multiplies two 64-bit signed ints if possible.
    Returns 0 on success, and puts the product of x and y
    into the result.
    Returns 1 if there was an overflow. */
static
int
int64_mult(long long x, long long y, long long * result)
{
    *result = 0;
    if (x > 0 && y > 0 && x > INT64_MAX / y) return 1;
    if (x < 0 && y > 0 && x < INT64_MIN / y) return 1;
    if (x > 0 && y < 0 && y < INT64_MIN / x) return 1;
    if (x < 0 && y < 0 &&
        (x <= INT64_MIN || y <= INT64_MIN || -x > INT64_MAX / -y))
        return 1;
    *result = x * y;
    return 0;
}

static
void test_int64_mult_success(long long x, long long y,
    long long expected,int line)
{
    long long result;

    /* x * y */
    if (int64_mult(x, y, &result))
    {
        fprintf(stderr, "unexpected overflow: %lld %lld line %d\n",
            x, y,line);
        ++failcount;
    }
    if (result != expected)
    {
        fprintf(stderr, "wrong result: %lld %lld %lld %lld line %d\n",
            x, y, expected, result,line);
        ++failcount;
    }

    // y * x should be the same
    if (int64_mult(y, x, &result))
    {
        fprintf(stderr, "unexpected overflow: %lld %lld line %d\n",
            y, x,line);
        ++failcount;
    }
    if (result != expected)
    {
        fprintf(stderr, "wrong result: %lld %lld %lld %lld line %d\n",
            y, x, expected, result,line);
        ++failcount;
    }
}

static
void test_int64_mult_error(long long x, long long y, int line)
{
    long long result;

    /* x * y */
    if (int64_mult(x, y, &result) == 0)
    {
        fprintf(stderr, "unexpected success: %lld %lld line %d\n",
            x, y,line);
        ++failcount;
    }

    /* y * x should be the same */
    if (int64_mult(y, x, &result) == 0)
    {
        fprintf(stderr, "unexpected success: %lld %lld line %d\n",
            y, x,line);
        ++failcount;
    }
}

int main(void)
{
    /* min, min */
    test_int64_mult_error(INT64_MIN, INT64_MIN,__LINE__);

    /* min, min/100 */
    test_int64_mult_error(INT64_MIN, INT64_MIN / 100,__LINE__);

    /* min, 0 */
    test_int64_mult_error(INT64_MIN, -2,__LINE__);
    test_int64_mult_error(INT64_MIN, -1,__LINE__);
    test_int64_mult_success(INT64_MIN, 0, 0,__LINE__);
    test_int64_mult_success(INT64_MIN, 1, INT64_MIN,__LINE__);
    test_int64_mult_error(INT64_MIN, 2,__LINE__);

    /* min, max/100 */
    test_int64_mult_error(INT64_MIN, INT64_MAX / 100,__LINE__);

    /* min, max */
    test_int64_mult_error(INT64_MIN, INT64_MAX,__LINE__);

    /* min/100, min/100 */
    test_int64_mult_error(INT64_MIN / 100, INT64_MIN / 100,__LINE__);

    /* min/100, 0 */
    test_int64_mult_error(INT64_MIN / 100, -101,__LINE__);
    test_int64_mult_success(INT64_MIN / 100, -100,
        0x7ffffffffffffff8,__LINE__);
    test_int64_mult_success(INT64_MIN / 100, -99,
        0x7eb851eb851eb84a,__LINE__);
    test_int64_mult_success(INT64_MIN / 100, 0, 0,__LINE__);
    test_int64_mult_success(INT64_MIN / 100, 100,
        -0x7ffffffffffffff8,__LINE__);
    test_int64_mult_error(INT64_MIN / 100, 101,__LINE__);

    /* min/100, max/100 */
    test_int64_mult_error(INT64_MIN / 100, INT64_MAX / 100,__LINE__);

    /* min/100, max */
    test_int64_mult_error(INT64_MIN / 100, INT64_MAX,__LINE__);

    /* 0, 0 */
    test_int64_mult_success(0, 0, 0,__LINE__);
    test_int64_mult_success(0, 1, 0,__LINE__);
    test_int64_mult_success(1, 1, 1,__LINE__);
    test_int64_mult_success(1, 3, 3,__LINE__);
    test_int64_mult_success(3, 3, 9,__LINE__);

    /* 0, max/100 */
    test_int64_mult_error(INT64_MAX / 100, -101,__LINE__);
    test_int64_mult_success(INT64_MAX / 100, -100,
        -0x7ffffffffffffff8,__LINE__);
    test_int64_mult_success(INT64_MAX / 100, -99,
        -0x7eb851eb851eb84a,__LINE__);
    test_int64_mult_success(INT64_MAX / 100, 0, 0,__LINE__);
    test_int64_mult_success(INT64_MAX / 100, 100,
        0x7ffffffffffffff8,__LINE__);
    test_int64_mult_error(INT64_MAX / 100, 101,__LINE__);

    /* 0, max */
    test_int64_mult_error(-2, INT64_MAX,__LINE__);
    test_int64_mult_success(-1, INT64_MAX, -INT64_MAX,__LINE__);
    test_int64_mult_success(0, INT64_MAX, 0,__LINE__);
    test_int64_mult_success(1, INT64_MAX, INT64_MAX,__LINE__);
    test_int64_mult_error(2, INT64_MAX,__LINE__);

    /* max/100, max */
    test_int64_mult_error(INT64_MAX / 100, INT64_MAX,__LINE__);
    if (failcount) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
