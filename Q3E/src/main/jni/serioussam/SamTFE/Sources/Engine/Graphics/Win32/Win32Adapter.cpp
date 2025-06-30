/* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */

#include "Engine/StdH.h"
#include <windows.h>

ULONG DetermineDesktopWidth(void)
{
  return((ULONG) ::GetSystemMetrics(SM_CXSCREEN));
}


