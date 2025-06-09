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

#include "Engine/StdH.h"

#include <Engine/Math/Float.h>

// Note: macro redefinition	for _MSC_VER
// As a result of redefinition, objects inside the world are located incorrectly.
// For Windows, you need to use the definitions from the header which is in the SDK.
#if (!defined _MSC_VER)
#define MCW_PC    0x0300
#define _MCW_PC     MCW_PC
#define _PC_24    0x0000
#define _PC_53    0x0200
#define _PC_64    0x0300
#endif

// !!! FIXME: I'd like to remove any dependency on the FPU control word from the game, asap.  --ryan.
#if (defined _MSC_VER)

// _control87 is provided by the compiler

#elif (defined __GNU_INLINE_X86_32__)

inline ULONG _control87(WORD newcw, WORD mask)
{
    WORD fpw = 0;

    // get the current FPU control word...
    __asm__ __volatile__ ("fstcw %0" : "=m" (fpw) : : "memory");

    if (mask != 0)
    {
        fpw &= ~mask;
        fpw |= (newcw & mask);
        __asm__ __volatile__ (" fldcw %0" : : "m" (fpw) : "memory");
    }
    return(fpw);
}

// (for intel compiler...)
#elif ((defined __MSVC_INLINE__) && (!defined _MSC_VER))

inline ULONG _control87(WORD newcw, WORD mask)
{
    WORD fpw = 0;

    // get the current FPU control word...
    __asm fstcw word ptr [fpw];

    if (mask != 0)
    {
        fpw &= ~mask;
        fpw |= (newcw & mask);
        __asm fldcw word ptr [fpw];
    }
    return(fpw);
}

#else

// Fake control87 for USE_PORTABLE_C version
inline ULONG _control87(WORD newcw, WORD mask)
{
    static WORD fpw=_PC_64;
    if (mask != 0)
    {
        fpw &= ~mask;
        fpw |= (newcw & mask);
    }
    return(fpw);
}

#endif

/* Get current precision setting of FPU. */
enum FPUPrecisionType GetFPUPrecision(void)
{
  // get control flags from FPU
  ULONG fpcw = _control87( 0, 0);

  // extract the precision from the flags
  switch(fpcw&_MCW_PC) {
  case _PC_24:
    return FPT_24BIT;
    break;
  case _PC_53:
    return FPT_53BIT;
    break;
  case _PC_64:
    return FPT_64BIT;
    break;
  default:
    ASSERT(FALSE);
    return FPT_24BIT;
  };
}

/* Set current precision setting of FPU. */
void SetFPUPrecision(enum FPUPrecisionType fptNew)
{
  ULONG fpcw;
  // create FPU flags from the precision
  switch(fptNew) {
  case FPT_24BIT:
    fpcw=_PC_24;
    break;
  case FPT_53BIT:
    fpcw=_PC_53;
    break;
  case FPT_64BIT:
    fpcw=_PC_64;
    break;
  default:
    ASSERT(FALSE);
    fpcw=_PC_24;
  };
  // set the FPU precission
  _control87( fpcw, MCW_PC);
}

/////////////////////////////////////////////////////////////////////
// CSetFPUPrecision
/*
 * Constructor with automatic setting of FPU precision.
 */
CSetFPUPrecision::CSetFPUPrecision(enum FPUPrecisionType fptNew)
{
  // remember old precision
  sfp_fptOldPrecision = GetFPUPrecision();
  // set new precision if needed
  sfp_fptNewPrecision = fptNew;
  if (sfp_fptNewPrecision!=sfp_fptOldPrecision) {
    SetFPUPrecision(fptNew);
  }
}

/*
 * Destructor with automatic restoring of FPU precision.
 */
CSetFPUPrecision::~CSetFPUPrecision(void)
{
  // check consistency
  ASSERT(GetFPUPrecision()==sfp_fptNewPrecision);
  // restore old precision if needed
  if (sfp_fptNewPrecision!=sfp_fptOldPrecision) {
    SetFPUPrecision(sfp_fptOldPrecision);
  }
}

BOOL IsValidFloat(float f)
{
  return _finite(f) && (*(ULONG*)&f)!=0xcdcdcdcdUL;
/*  int iClass = _fpclass(f);
  return
    iClass==_FPCLASS_NN ||
    iClass==_FPCLASS_ND ||
    iClass==_FPCLASS_NZ ||
    iClass==_FPCLASS_PZ ||
    iClass==_FPCLASS_PD ||
    iClass==_FPCLASS_PN;
    */
}

BOOL IsValidDouble(double f)
{
#ifdef _MSC_VER
  return _finite(f) && (*(unsigned __int64*)&f)!=0xcdcdcdcdcdcdcdcdI64;
#else
  return _finite(f) && (*(unsigned long long*)&f)!=0xcdcdcdcdcdcdcdcdll;
#endif
/*  int iClass = _fpclass(f);
  return
    iClass==_FPCLASS_NN ||
    iClass==_FPCLASS_ND ||
    iClass==_FPCLASS_NZ ||
    iClass==_FPCLASS_PZ ||
    iClass==_FPCLASS_PD ||
    iClass==_FPCLASS_PN;
    */
}

