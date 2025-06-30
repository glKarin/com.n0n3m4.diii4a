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

#ifndef SE_INCL_TIMER_INL
#define SE_INCL_TIMER_INL
#ifdef PRAGMA_ONCE
  #pragma once
#endif

/* Constructor from seconds. */
inline CTimerValue::CTimerValue(double fSeconds)
{
  tv_llValue = (__int64) (fSeconds*_pTimer->tm_llPerformanceCounterFrequency);
}
/* Clear timer value (set it to zero). */
inline void CTimerValue::Clear(void)
{
  tv_llValue = 0;
}
/* Addition. */
inline CTimerValue &CTimerValue::operator+=(const CTimerValue &tvOther) {
  tv_llValue+=tvOther.tv_llValue;
  return *this;
};
inline CTimerValue CTimerValue::operator+(const CTimerValue &tvOther) const {
  return CTimerValue(*this)+=tvOther;
};
/* Substraction. */
inline CTimerValue &CTimerValue::operator-=(const CTimerValue &tvOther) {
  tv_llValue-=tvOther.tv_llValue;
  return *this;
};
inline CTimerValue CTimerValue::operator-(const CTimerValue &tvOther) const {
  return CTimerValue(*this)-=tvOther;
};
/* Comparisons. */
inline BOOL CTimerValue::operator<(const CTimerValue &tvOther) const {
  return tv_llValue<tvOther.tv_llValue;
}
inline BOOL CTimerValue::operator>(const CTimerValue &tvOther) const {
  return tv_llValue>tvOther.tv_llValue;
}
inline BOOL CTimerValue::operator<=(const CTimerValue &tvOther) const {
  return tv_llValue<=tvOther.tv_llValue;
}
inline BOOL CTimerValue::operator>=(const CTimerValue &tvOther) const {
  return tv_llValue>=tvOther.tv_llValue;
}
/* Get the timer value in seconds. - use for time spans only! */
inline double CTimerValue::GetSeconds(void) {
  return ((double)tv_llValue)/_pTimer->tm_llPerformanceCounterFrequency;
};
/* Get the timer value in milliseconds as integral value. */
inline __int64 CTimerValue::GetMilliseconds(void) {
  return tv_llValue/(_pTimer->tm_llPerformanceCounterFrequency/1000);
};


#endif  /* include-once check. */

