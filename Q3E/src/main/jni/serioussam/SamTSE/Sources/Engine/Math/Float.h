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

#ifndef SE_INCL_FLOAT_H
#define SE_INCL_FLOAT_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif


/*
 * FPU control functions and classes.
 */

// FPU precision setting values
enum FPUPrecisionType {
  FPT_24BIT,
  FPT_53BIT,
  FPT_64BIT,
};
/* Get current precision setting of FPU. */
ENGINE_API enum FPUPrecisionType GetFPUPrecision(void);
/* Set current precision setting of FPU. */
ENGINE_API void SetFPUPrecision(enum FPUPrecisionType fptNew);

/*
 * Class that provides automatic saving/setting/restoring of FPU precision setting.
 */
class ENGINE_API CSetFPUPrecision {
private:
  enum FPUPrecisionType sfp_fptOldPrecision;
  enum FPUPrecisionType sfp_fptNewPrecision;
public:
  /* Constructor with automatic setting of FPU precision. */
  CSetFPUPrecision(enum FPUPrecisionType fptNew);
  /* Destructor with automatic restoring of FPU precision. */
  ~CSetFPUPrecision(void);
};


ENGINE_API BOOL IsValidFloat(float f);
ENGINE_API BOOL IsValidDouble(double f);


#endif  /* include-once check. */

