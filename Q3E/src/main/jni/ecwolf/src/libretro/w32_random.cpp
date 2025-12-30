#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <wincrypt.h>
#include <commctrl.h>
#include <ctime>

//==========================================================================
//
// I_MakeRNGSeed
//
// Returns a 32-bit random seed, preferably one with lots of entropy.
//
/*
** i_system.cpp
** Timers, pre-console output, IWAD selection, and misc system routines.
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
//
//==========================================================================

unsigned int I_MakeRNGSeed()
{
	unsigned int seed;

	// If RtlGenRandom is available, use that to avoid increasing the
	// working set by pulling in all of the crytographic API.
	HMODULE advapi = GetModuleHandle("advapi32.dll");
	if (advapi != NULL)
	{
		BOOLEAN (APIENTRY *RtlGenRandom)(void *, ULONG) =
			(BOOLEAN (APIENTRY *)(void *, ULONG))GetProcAddress(advapi, "SystemFunction036");
		if (RtlGenRandom != NULL)
		{
			if (RtlGenRandom(&seed, sizeof(seed)))
			{
				return seed;
			}
		}
	}

	// Use the full crytographic API to produce a seed. If that fails,
	// time() is used as a fallback.
	HCRYPTPROV prov;

	if (!CryptAcquireContext(&prov, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		return (unsigned int)time(NULL);
	}
	if (!CryptGenRandom(prov, sizeof(seed), (BYTE *)&seed))
	{
		seed = (unsigned int)time(NULL);
	}
	CryptReleaseContext(prov, 0);
	return seed;
}
#endif
