/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#ifndef SE_INCL_FIXINT_H
#define SE_INCL_FIXINT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/*
 * iInt    - number of bits in integer part
 * iFrac   - number of bits in fractional part
 * NOTE: Since 32 bit integer holds this fix int, iInt+iFrac must be 32.
 */

/*
 * Template class for fixed integer with arbitrary sizes of integer and fractional parts
 */
template<int iInt, int iFrac>
class FixInt {
//implementation:
public:
  SLONG slHolder;   // the representation is stored here
//interface:
public:
  /* Default constructor. */
  inline FixInt<iInt, iFrac>(void) {};
  /* Copy constructor. */
  inline FixInt<iInt, iFrac>(const FixInt<iInt, iFrac> &x) { slHolder = x.slHolder; };
  /* Constructor from integer. */
  inline FixInt<iInt, iFrac>(SLONG sl) : slHolder(sl<<iFrac) {};
  inline FixInt<iInt, iFrac>(ULONG ul) : slHolder((SLONG)(ul<<iFrac)) {};
  inline FixInt<iInt, iFrac>(SWORD sw) : slHolder((SLONG)(sw<<iFrac)) {};
  inline FixInt<iInt, iFrac>(UWORD uw) : slHolder((SLONG)(uw<<iFrac)) {};
  inline FixInt<iInt, iFrac>(SBYTE sb) : slHolder((SLONG)(sb<<iFrac)) {};
  inline FixInt<iInt, iFrac>(UBYTE ub) : slHolder((SLONG)(ub<<iFrac)) {};
  //inline FixInt<iInt, iFrac>(signed int si)   : slHolder((SLONG)(si<<iFrac)) {};
  //inline FixInt<iInt, iFrac>(unsigned int ui) : slHolder((SLONG)(ui<<iFrac)) {};
  /* Constructor from float. */
  inline FixInt<iInt, iFrac>(float f)  : slHolder((SLONG)(f*(1L<<iFrac))) {};
  inline FixInt<iInt, iFrac>(double f) : slHolder((SLONG)(f*(1L<<iFrac))) {};
  /* Set holder constructor. */
  inline FixInt<iInt, iFrac>(SLONG slNewHolder, int iDummy) : slHolder(slNewHolder) {};

  /* Conversion to integer (truncatenation). */
  inline operator SLONG(void) const { return slHolder>>iFrac; };
  /* Conversion to float (exact). */
  inline operator float(void)  const { return slHolder/(float) (1L<<iFrac); };
  inline operator double(void) const { return slHolder/(double)(1L<<iFrac); };

  /* Negation. */
  inline FixInt<iInt, iFrac> operator-(void) const { return FixInt<iInt, iFrac>(-slHolder, 1); };
  /* Addition. */
  inline FixInt<iInt, iFrac> &operator+=(FixInt<iInt, iFrac> x) { slHolder += x.slHolder; return *this; };
  inline FixInt<iInt, iFrac> operator+(FixInt<iInt, iFrac> x) const { return FixInt<iInt, iFrac>(slHolder+x.slHolder, 1); };
  /* Substraction. */
  inline FixInt<iInt, iFrac> &operator-=(FixInt<iInt, iFrac> x) { slHolder -= x.slHolder; return *this;};
  inline FixInt<iInt, iFrac> operator-(FixInt<iInt, iFrac> x) const { return FixInt<iInt, iFrac>(slHolder-x.slHolder, 1); };
  /* Multiplication. */
  inline FixInt<iInt, iFrac> operator*(FixInt<iInt, iFrac> x) const { return FixInt<iInt, iFrac>( (SLONG)((((__int64) slHolder)*x.slHolder) >>iFrac), 1); };
  inline FixInt<iInt, iFrac> operator*(SLONG sl) const { return FixInt<iInt, iFrac>(slHolder*sl, 1); };
  friend inline FixInt<iInt, iFrac> operator*(SLONG sl, FixInt<iInt, iFrac> x){ return FixInt<iInt, iFrac>(x.slHolder*sl, 1); };
  inline FixInt<iInt, iFrac> &operator*=(FixInt<iInt, iFrac> x) { return *this = *this*x; };
  /* Division. */
  inline FixInt<iInt, iFrac> operator/(FixInt<iInt, iFrac> x) const { return FixInt<iInt, iFrac>( (SLONG) ( (((__int64)slHolder)<<iFrac) / x.slHolder ) , 1); };
  inline FixInt<iInt, iFrac> &operator/=(FixInt<iInt, iFrac> x) { return *this = *this/x; };

  /* Relational operators. */
  inline BOOL operator< (FixInt<iInt, iFrac> x) const { return slHolder< x.slHolder; };
  inline BOOL operator<=(FixInt<iInt, iFrac> x) const { return slHolder<=x.slHolder; };
  inline BOOL operator> (FixInt<iInt, iFrac> x) const { return slHolder> x.slHolder; };
  inline BOOL operator>=(FixInt<iInt, iFrac> x) const { return slHolder>=x.slHolder; };
  inline BOOL operator==(FixInt<iInt, iFrac> x) const { return slHolder==x.slHolder; };
  inline BOOL operator!=(FixInt<iInt, iFrac> x) const { return slHolder!=x.slHolder; };

  /* Rounding and truncatenation functions. */
  friend inline SLONG Floor(FixInt<iInt, iFrac> x) { return (x.slHolder)>>iFrac; };
  friend inline SLONG Ceil(FixInt<iInt, iFrac> x) { return (x.slHolder+(0xFFFFFFFFL>>iInt))>>iFrac; };
  /* Truncatenate to fixed number of decimal bits. */
  friend inline FixInt<iInt, iFrac> Trunc(FixInt<iInt, iFrac> x, int iDecimalPart) {
    return FixInt<iInt, iFrac>(x.slHolder& (0xFFFFFFFF*((1<<iFrac)/iDecimalPart)), 1);};
};


#endif  /* include-once check. */

