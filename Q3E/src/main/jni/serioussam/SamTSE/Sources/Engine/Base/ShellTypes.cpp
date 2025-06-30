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

#include <Engine/Base/Shell.h>
#include <Engine/Base/Shell_internal.h>
#include "ParsingSymbols.h"

#include <Engine/Templates/DynamicStackArray.h>

#include <Engine/Templates/AllocationArray.cpp>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/DynamicStackArray.cpp>

// all types are allocated here
CAllocationArray<ShellType> _shell_ast;

// make a new type
INDEX ShellTypeNew(void)
{
  INDEX ist = _shell_ast.Allocate();
	ShellType &st = _shell_ast[ist];

	st.st_sttType = STT_ILLEGAL;
  st.st_ctArraySize = -1;
  st.st_istBaseType = -1;
  st.st_istFirstArgument = -1;
  st.st_istLastArgument = -1;
  st.st_istNextInArguments = -1;
  st.st_istPrevInArguments = -1;

	return ist;
}

// make a new type for a basic type
INDEX ShellTypeNewVoid(void)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_VOID;
	return ist;
}

INDEX ShellTypeNewIndex(void)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_INDEX;
	return ist;
}

INDEX ShellTypeNewFloat(void)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_FLOAT;
	return ist;
}

INDEX ShellTypeNewString(void)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_STRING;
	return ist;
}
INDEX ShellTypeNewByType(enum ShellTypeType stt)
{
  switch(stt) {
  default: ASSERT(FALSE);
  case STT_VOID:    return ShellTypeNewVoid();
  case STT_INDEX:   return ShellTypeNewIndex();
  case STT_FLOAT:   return ShellTypeNewFloat();
  case STT_STRING:  return ShellTypeNewString();
  }
}

// make a new pointer type
INDEX ShellTypeNewPointer(INDEX istBaseType)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_POINTER;
	st.st_istBaseType = istBaseType;
	return ist;
}

// make a new array type
INDEX ShellTypeNewArray(INDEX istMemberType, int iArraySize)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_ARRAY;
	st.st_ctArraySize = iArraySize;
	st.st_istBaseType = istMemberType;
	return ist;
}

// make a new function type
INDEX ShellTypeNewFunction(INDEX istReturnType)
{
  INDEX ist = ShellTypeNew();
	ShellType &st = _shell_ast[ist];
	st.st_sttType = STT_FUNCTION;
	st.st_istBaseType = istReturnType;
	st.st_istFirstArgument = -1;
	st.st_istLastArgument = -1;
	return ist;
}

// add an argument to a function from the left
void ShellTypeAddFunctionArgument(INDEX istFunction,INDEX istArgument)
{
	ShellType &stFunction = _shell_ast[istFunction];
	ShellType &stArgument = _shell_ast[istArgument];
  if (stFunction.st_istFirstArgument == -1) {
	  stFunction.st_istLastArgument = istArgument;
  } else {
    _shell_ast[stFunction.st_istFirstArgument].st_istPrevInArguments = istArgument;
  }
	stArgument.st_istNextInArguments = stFunction.st_istFirstArgument;
	stArgument.st_istPrevInArguments = -1;
	stFunction.st_istFirstArgument = istArgument;
}

// This mess is neccessary to prevent code optimizer to create aliases
// to _shell_ast array across recursive function calls, since the array can be
// reallocated!
// The problem is in the evaluation order created by VisualC6 optimizer.
#pragma optimize("", off)
#pragma optimize("w", off)
static volatile INDEX iTmp;
#define stDuplicate _shell_ast[istDuplicate]
#define stOriginal  _shell_ast[istOriginal]

// make a copy of a type tree
INDEX ShellTypeMakeDuplicate(INDEX istOriginal)
{
	INDEX istDuplicate = ShellTypeNew();

	// copy values of the original structure
	stDuplicate.st_sttType = stOriginal.st_sttType;

  // check type
  switch (stOriginal.st_sttType) {
  // if pointer
  case STT_POINTER:
    // copy what it points to
    iTmp = ShellTypeMakeDuplicate(stOriginal.st_istBaseType);
		stDuplicate.st_istBaseType = iTmp;
    break;

  // if an array
  case STT_ARRAY:
    // copy the member type
		stDuplicate.st_ctArraySize = stOriginal.st_ctArraySize;
    iTmp = ShellTypeMakeDuplicate(stOriginal.st_istBaseType);
    stDuplicate.st_istBaseType = iTmp;
    break;

  // if a function
  case STT_FUNCTION: {
    // copy return type
    iTmp = ShellTypeMakeDuplicate(stOriginal.st_istBaseType);
		stDuplicate.st_istBaseType = iTmp;

    // for each argument (backwards)
  	INDEX istArgOrg, istArgDup;
    for (istArgOrg=stOriginal.st_istLastArgument; 
         istArgOrg!=-1;
         istArgOrg = _shell_ast[istArgOrg].st_istPrevInArguments) {
      // copy it
      istArgDup = ShellTypeMakeDuplicate(istArgOrg);
      // add it
      ShellTypeAddFunctionArgument(istDuplicate, istArgDup);
    }
                     } break;
  }

  return istDuplicate;
}
#undef stDuplicate
#undef stOriginal 
#pragma optimize("", on)

BOOL ShellTypeIsSame(INDEX ist1, INDEX ist2)
{
  // if both are end of list
  if (ist1==-1 && ist2==-1) {
    // same
    return TRUE;
  }
  // if only one is end of list
  if (ist1==-1 || ist2==-1) {
    // different
    return FALSE;
  }

  // get the types
  ShellType &st1 = _shell_ast[ist1];
	ShellType &st2 = _shell_ast[ist2];

  // if types or base types or sizes don't match
  if (st1.st_sttType!=st2.st_sttType ||
    !ShellTypeIsSame(st1.st_istBaseType, st2.st_istBaseType)
    ||st1.st_ctArraySize!=st2.st_ctArraySize) {
    // different
    return FALSE;
  }

  // match arguments recursively
  return 
    ShellTypeIsSame(st1.st_istFirstArgument, st2.st_istFirstArgument) &&
    ShellTypeIsSame(st1.st_istNextInArguments, st2.st_istNextInArguments);
}

// delete an entire type tree
void ShellTypeDelete(INDEX istToDelete)
{
  ShellType &st = _shell_ast[istToDelete];
  
  if (st.st_istBaseType>0) {
    ShellTypeDelete(st.st_istBaseType);
  }
  if (st.st_istFirstArgument>0) {
    ShellTypeDelete(st.st_istFirstArgument);
  }
  if (st.st_istNextInArguments>0) {
    ShellTypeDelete(st.st_istNextInArguments);
  }

  _shell_ast.Free(istToDelete);
}

BOOL ShellTypeIsPointer(INDEX ist)
{
  return _shell_ast[ist].st_sttType == STT_POINTER;
}

BOOL ShellTypeIsArray(INDEX ist)
{
  return _shell_ast[ist].st_sttType == STT_ARRAY;
}

BOOL ShellTypeIsIntegral(INDEX ist)
{
  return _shell_ast[ist].st_sttType == STT_INDEX;
}
