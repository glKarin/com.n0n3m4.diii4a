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

#ifndef SE_INCL_ERRORTABLE_H
#define SE_INCL_ERRORTABLE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

struct ErrorCode {
  SLONG ec_Code;        // error code value
  const char *ec_Name;        // error code constant name (in .h files)
  const char *ec_Description; // error description (in help files)
};

struct ErrorTable {
  INDEX et_Count;               // number of errors
  struct ErrorCode *et_Errors;  // array of error codes
};

// macro for defining error codes
#define ERRORCODE(code, description) {code, #code, description}
// macro for defining error table
#define ERRORTABLE(errorcodes) {sizeof(errorcodes)/sizeof(struct ErrorCode), errorcodes}
/* Get the name string for error code. */
ENGINE_API extern const char *ErrorName(const struct ErrorTable *pet, SLONG slErrCode);
/* Get the description string for error code. */
ENGINE_API extern const char *ErrorDescription(const struct ErrorTable *pet, SLONG slErrCode);


#endif  /* include-once check. */

