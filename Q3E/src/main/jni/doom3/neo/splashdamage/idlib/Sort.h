// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __IDLIB_SORT_H__
#define __IDLIB_SORT_H__

// a version from Microsoft's CRT that supports a functor

template< class T >
class sdSortLess {
public:	
	int operator()( const T& lhs, const T& rhs ) const {
		return lhs < rhs;
	}

};
namespace /* anonymous */ {

	/***
	*shortsort(hi, lo, width, comp) - insertion sort for sorting short arrays
	*
	*Purpose:
	*       sorts the sub-array of elements between lo and hi (inclusive)
	*       side effects:  sorts in place
	*       assumes that lo < hi
	*
	*Entry:
	*       char *lo = pointer to low element to sort
	*       char *hi = pointer to high element to sort
	*       size_t width = width in bytes of each array element
	*       int (*comp)() = pointer to function returning analog of strcmp for
	*               strings, but supplied by user for comparing the array elements.
	*               it accepts 2 pointers to elements and returns neg if 1<2, 0 if
	*               1=2, pos if 1>2.
	*
	*Exit:
	*       returns void
	*
	*Exceptions:
	*
	*******************************************************************************/
	template< class ElementIter, class Cmp >
	ID_INLINE void sdShortSort( ElementIter lo, ElementIter hi, Cmp comp ) {
		ElementIter p, max;

		/* Note: in assertions below, i and j are alway inside original bound of array to sort. */

		while( hi > lo ) {
			/* A[i] <= A[j] for i <= j, j > hi */
			max = lo;
			for( p = lo + 1; p <= hi; p++ ) {
				/* A[i] <= A[max] for lo <= i < p */
				if( comp( *p, *max ) > 0 ) {
					max = p;
				}
				/* A[i] <= A[max] for lo <= i <= p */
			}

			/* A[i] <= A[max] for lo <= i <= hi */

			idSwap( *max, *hi );

			/* A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi */

			hi--;

			/* A[i] <= A[j] for i <= j, j > hi, loop top condition established */
		}
		/* A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j, so array is sorted */
	}
}

/***
*sdSort(base, num, comp) - quicksort function for sorting arrays
*
*Purpose:
*       quicksort the array of elements
*       side effects:  sorts in place
*       maximum array size is number of elements times size of elements,
*       but is limited by the virtual address space of the processor
*
*Entry:
*       ElementIter base = base of array
*       ElementIter end = 1 past the end of the array
*       int (*comp)() = pointer to function returning analog of strcmp for
*               strings, but supplied by user for comparing the array elements.
*               it accepts 2 pointers to elements and returns neg if 1<2, 0 if 1==2, pos if 1>2.
*
*Exit:
*       returns void
*
*Exceptions:
*
*******************************************************************************/
template< class ElementIter, class Cmp >
ID_INLINE void sdQuickSort( ElementIter begin, ElementIter end, Cmp comp ) {
	/* Note: the number of stack entries required is no more than
	1 + log2(num), so 30 is sufficient for any array */
	ElementIter lo, hi;				/* ends of sub-array currently sorting */
	ElementIter mid;					/* points to middle of subarray */
	ElementIter loguy, higuy;			/* traveling pointers for partition step */
	size_t size;					/* size of the sub-array */
	
	const size_t STKSIZ = ( 8 * sizeof( ElementIter ) - 2 );
	const size_t CUTOFF = 8;            /* testing shows that this is good value */
	ElementIter lostk[STKSIZ], histk[STKSIZ];

	int stkptr;						/* stack for saving sub-array to be processed */

	if( end - begin < 2 ) {
		return;                		/* nothing to do */
	}

	stkptr = 0;                		/* initialize stack */

	lo = begin;
	hi = end - 1;					/* initialize limits */

	/* this entry point is for pseudo-recursion calling: setting
	lo and hi and jumping to here is like recursion, but stkptr is
	preserved, locals aren't, so we preserve stuff on the stack */
recurse:

	size = (hi - lo) + 1;        /* number of el's to sort */	

	/* below a certain size, it is faster to use a O(n^2) sorting method */
	if( size <= CUTOFF ) {
		sdShortSort( lo, hi, comp );
	} else {
		/* First we pick a partitioning element.  The efficiency of the
		algorithm demands that we find one that is approximately the median
		of the values, but also that we select one fast.  We choose the
		median of the first, middle, and last elements, to avoid bad
		performance in the face of already sorted data, or data that is made
		up of multiple sorted runs appended together.  Testing shows that a
		median-of-three algorithm provides better performance than simply
		picking the middle element for the latter case. */
		mid = lo + (size / 2);      /* find middle element */

		/* Sort the first, middle, last elements into order */
		if( comp( *lo, *mid ) > 0 ) {
			idSwap( *lo, *mid );
		}
		if( comp( *lo, *hi ) > 0 ) {
			idSwap( *lo, *hi );
		}
		if( comp( *mid, *hi ) > 0 ) {
			idSwap( *mid, *hi );
		}

		/* We now wish to partition the array into three pieces, one consisting
		of elements <= partition element, one of elements equal to the
		partition element, and one of elements > than it.  This is done
		below; comments indicate conditions established at every step. */

		loguy = lo;
		higuy = hi;

		/* Note that higuy decreases and loguy increases on every iteration,
		so loop must terminate. */
		for (;;) {
			/* lo <= loguy < hi, lo < higuy <= hi,
			A[i] <= A[mid] for lo <= i <= loguy,
			A[i] > A[mid] for higuy <= i < hi,
			A[hi] >= A[mid] */

			/* The doubled loop is to avoid calling comp(mid,mid), since some
			existing comparison funcs don't work when passed the same
			value for both pointers. */

			if( mid > loguy ) {
				do  {
					loguy++;
				} while( loguy < mid && comp( *loguy, *mid) <= 0 );
			}
			if( mid <= loguy ) {
				do  {
					loguy++;
				} while( loguy <= hi && comp( *loguy, *mid) <= 0 );
			}

			/* lo < loguy <= hi+1, A[i] <= A[mid] for lo <= i < loguy,
			either loguy > hi or A[loguy] > A[mid] */

			do  {
				higuy--;
			} while( higuy > mid && comp( *higuy, *mid ) > 0 );

			/* lo <= higuy < hi, A[i] > A[mid] for higuy < i < hi,
			either higuy == lo or A[higuy] <= A[mid] */

			if( higuy < loguy) {
				break;
			}

			/* if loguy > hi or higuy == lo, then we would have exited, so A[loguy] > A[mid], A[higuy] <= A[mid], loguy <= hi, higuy > lo */

			idSwap( *loguy, *higuy );

			/* If the partition element was moved, follow it.  Only need
			to check for mid == higuy, since before the swap,
			A[loguy] > A[mid] implies loguy != mid. */

			if( mid == higuy ) {
				mid = loguy;
			}

			/* A[loguy] <= A[mid], A[higuy] > A[mid]; so condition at top of loop is re-established */
		}

		/*     A[i] <= A[mid] for lo <= i < loguy,
		A[i] > A[mid] for higuy < i < hi,
		A[hi] >= A[mid]
		higuy < loguy
		implying:
		higuy == loguy-1
		or higuy == hi - 1, loguy == hi + 1, A[hi] == A[mid] */

		/* Find adjacent elements equal to the partition element.  The
		doubled loop is to avoid calling comp(mid,mid), since some
		existing comparison funcs don't work when passed the same value
		for both pointers. */

		higuy++;
		if( mid < higuy ) {
			do  {
				higuy--;
			} while( higuy > mid && comp( *higuy, *mid ) == 0 );
		}
		if( mid >= higuy ) {
			do  {
				higuy--;
			} while( higuy > lo && comp( *higuy, *mid ) == 0 );
		}

		/* OK, now we have the following:
		higuy < loguy
		lo <= higuy <= hi
		A[i]  <= A[mid] for lo <= i <= higuy
		A[i]  == A[mid] for higuy < i < loguy
		A[i]  >  A[mid] for loguy <= i < hi
		A[hi] >= A[mid] */

		/* We've finished the partition, now we want to sort the subarrays
		[lo, higuy] and [loguy, hi].
		We do the smaller one first to minimize stack usage.
		We only sort arrays of length 2 or more.*/

		if( higuy - lo >= hi - loguy ) {
			if( lo < higuy ) {
				lostk[stkptr] = lo;
				histk[stkptr] = higuy;
				++stkptr;
			}                           /* save big recursion for later */

			if( loguy < hi ) {
				lo = loguy;
				goto recurse;           /* do small recursion */
			}
		} else {
			if( loguy < hi ) {
				lostk[stkptr] = loguy;
				histk[stkptr] = hi;
				++stkptr;               /* save big recursion for later */
			}

			if( lo < higuy ) {
				hi = higuy;
				goto recurse;           /* do small recursion */
			}
		}
	}

	/* We have sorted the array, except for any pending sorts on the stack.
	Check if there are any, and do them. */

	--stkptr;
	if( stkptr >= 0 ) {
		lo = lostk[stkptr];
		hi = histk[stkptr];
		goto recurse;           /* pop subarray from stack */
	} else {
		return;                 /* all subarrays done */
	}
}

// TTimo: width undefined / unused
#if 0
/*
============
sdBinarySearch
============
*/
template< class Element, class ElementIter, class Cmp >
ID_INLINE ElementIter sdBinarySearch( const Element& element, ElementIter begin, ElementIter end, Cmp compare ) {
	ElementIter lo = begin;
	ElementIter hi = end;
	ElementIter mid;

	size_t num = hi - lo;
	size_t half;
	bool result;	

	while (lo <= hi) {
		half = num / 2;
		if( half ) {
			mid = lo + (num & 1 ? half : (half - 1)) * width;
			result = compare( element, mid );
			if( !result ) {
				return mid;
			} else if ( result < 0 ) {
				hi = mid - 1;
				num = num & 1 ? half : half - 1;
			} else {
				lo = mid + 1;
				num = half;
			}
		} else if( num != 0 ) {
			return compare( element, lo ) ? end : lo;
		} else {
			break;
		}
	}
	return end;
}
#endif


// Implementation of smooth sort
// O(n) performance for generally sorted array
// sorts inplace
// http://en.wikibooks.org/wiki/Algorithm_implementation/Sorting/Smoothsort

/**
**  Helper class for manipulation of Leonardo numbers
**
**/

class LeonardoNumber
{
    public:
    /**  Default ctor.  **/
    LeonardoNumber (void) : b (1), c (1)
        { return; }

    /**  Copy ctor.  **/
    LeonardoNumber (const LeonardoNumber & _l)  : b (_l.b), c (_l.c)
        { return; }

    /**  Return the "gap" between the actual Leonardo number and the preceeding one.  **/
    unsigned gap (void) const throw ()
        { return b - c; }


    /**  Perform an "up" operation on the actual number.  **/
    LeonardoNumber & operator ++ (void)
        { unsigned s = b; b = b + c + 1; c = s; return * this; }

    /**  Perform a "down" operation on the actual number.  **/
    LeonardoNumber & operator -- (void)
        { unsigned s = c; c = b - c - 1; b = s; return * this; }

    /**  Return "companion" value.  **/
    unsigned operator ~ (void) const
        { return c; }

    /**  Return "actual" value.  **/
    operator unsigned (void) const
        { return b; }


    private:
    unsigned b;   /**  Actual number.  **/
    unsigned c;   /**  Companion number.  **/
};


/**  Perform a "sift up" operation.  **/

/**
**  Sifts up the root of the stretch in question.
**
**    Usage: sift (<array>, <root>, <number>)
**
**    Where:     <array> Pointer to the first element of the array in question.
**                <root> Index of the root of the array in question.
**              <number> Current Leonardo's number.
**
**
**/
template <typename T>
ID_INLINE void sift (T * _m, unsigned _r, LeonardoNumber _b)
{
    unsigned r2;

    while (_b >= 3)
    {
        if (_m [_r - _b.gap ()] >= _m [_r - 1])
        r2 = _r - _b.gap ();
        else
        { r2 = _r - 1; --_b; }

        if (_m [_r] >= _m [r2])  break;
        else
        { _m [_r].swap (_m [r2]); _r = r2; --_b; }
    }


    return;
}


/**
**  Trinkles the roots of the stretches of a given array and root.
**
**    Usage: trinkle (<array>, <root>, <standart_concat>, <number>)
**
**    Where:           <array> Pointer to the first element of the array in question.
**                      <root> Index of the root of the array in question.
**           <standard_concat> Standard concatenation's codification.
**                    <number> Current Leonardo number.
**
**
**/
template <typename T>
ID_INLINE void trinkle (T * _m, unsigned _r, unsigned long long _p, LeonardoNumber _b)
{
    while (_p)
    {
        for ( ; !(_p % 2); _p >>= 1)  ++_b;

        if (!--_p || (_m [_r] >= _m [_r - _b]))  break;
        else
        if (_b == 1)
            { _m [_r].swap (_m [_r - _b]); _r -= _b; }

        else if (_b >= 3)
            {
            unsigned r2 = _r - _b.gap (), r3 = _r - _b;

            if (_m [_r - 1] >= _m [r2])
                { r2 = _r - 1; _p <<= 1; --_b; }

            if (_m [r3] >= _m [r2])
                { _m [_r].swap (_m [r3]); _r = r3; }

            else
                { _m [_r].swap (_m [r2]); _r = r2; --_b; break; }
            }
    }

    sift<T> (_m, _r, _b);


    return;
}

/**
**  Trinkles the roots of the stretches of a given array and root when the adjacent stretches are trusty.
**
**    Usage: semitrinkle (<array>, <root>, <standart_concat>, <number>)
**
**    Where:           <array> Pointer to the first element of the array in question.
**                      <root> Index of the root of the array in question.
**           <standard_concat> Standard concatenation's codification.
**                    <number> Current Leonardo's number.
**
**
**/
template <typename T>
ID_INLINE void semitrinkle (T * _m, unsigned _r, unsigned long long _p, LeonardoNumber _b)

{
    if (_m [_r - ~_b] >= _m [_r])
    {
        _m [_r].swap (_m [_r - ~_b]);
        trinkle<T> (_m, _r - ~_b, _p, _b);
    }


    return;
}

/**
**  Sorts the given array in ascending order.
**
**    Usage: smoothsort (<array>, <size>)
**
**    Where: <array> pointer to the first element of the array in question.
**            <size> length of the array to be sorted.
**
**
**/
template <typename T>
void smoothsort (T * _m, unsigned _n)
{
    if (!(_m && _n)) return;

    unsigned long long p = 1;
    LeonardoNumber b;

    for (unsigned q = 0; ++q < _n ; ++p)
    if (p % 8 == 3)
        {
        sift<T> (_m, q - 1, b);

        ++++b; p >>= 2;
        }

    else if (p % 4 == 1)
        {
        if (q + ~b < _n)  sift<T> (_m, q - 1, b);
        else  trinkle<T> (_m, q - 1, p, b);

        for (p <<= 1; --b > 1; p <<= 1)  ;
        }

    trinkle<T> (_m, _n - 1, p, b);

    for (--p; _n-- > 1; --p)
    if (b == 1)
        for ( ; !(p % 2); p >>= 1)  ++b;

    else if (b >= 3)
        {
        if (p)  semitrinkle<T> (_m, _n - b.gap (), p, b);

        --b; p <<= 1; ++p;
        semitrinkle<T> (_m, _n - 1, p, b);
        --b; p <<= 1; ++p;
        }
    return;
}


#endif // ! __IDLIB_SORT_H__
