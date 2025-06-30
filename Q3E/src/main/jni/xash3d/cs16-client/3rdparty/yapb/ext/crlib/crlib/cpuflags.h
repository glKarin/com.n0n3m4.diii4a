//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#if (defined(CR_LINUX) || defined(CR_MACOS)) && !defined(CR_ARCH_ARM) && !defined(CR_ARCH_PPC)
#  include <cpuid.h>
#elif defined(CR_WINDOWS) && !defined(CR_CXX_MSVC)
#  include <cpuid.h>
#endif

CR_DECLARE_SCOPED_ENUM (CpuCap,
   SSE3 = cr::bit (0),
   SSSE3 = cr::bit (9),
   SSE41 = cr::bit (19),
   SSE42 = cr::bit (20),
   AVX = cr::bit (28),
   AVX2 = cr::bit (5)
)

CR_NAMESPACE_BEGIN

// cpu flags for current cpu
class CpuFlags final : public Singleton <CpuFlags> {
public:
   bool sse3 {}, ssse3 {}, sse41 {}, sse42 {}, avx {}, avx2 {}, neon {};

public:
   CpuFlags () {
      detect ();
   }

   ~CpuFlags () = default;

private:
   void detect () {
#if !defined(CR_ARCH_ARM) && !defined(CR_ARCH_PPC)
      enum { eax, ebx, ecx, edx, regs };

      uint32_t data[regs] {};

#if defined(CR_WINDOWS) && defined(CR_CXX_MSVC)
   __cpuidex (reinterpret_cast <int32_t *> (data), 1, 0);
#else
      __get_cpuid (0x1, &data[eax], &data[ebx], &data[ecx], &data[edx]);
#endif
      sse3 = !!(data[ecx] & CpuCap::SSE3);
      ssse3 = !!(data[ecx] & CpuCap::SSSE3);
      sse41 = !!(data[ecx] & CpuCap::SSE41);
      sse42 = !!(data[ecx] & CpuCap::SSE42);
      avx = !!(data[ecx] & CpuCap::AVX);

#if defined(CR_WINDOWS) && defined(CR_CXX_MSVC)
      __cpuidex (reinterpret_cast <int32_t *> (data), 7, 0);
#else
      __get_cpuid (0x7, &data[eax], &data[ebx], &data[ecx], &data[edx]);
#endif
      avx2 = !!(data[ebx] & CpuCap::AVX2);
#else
      neon = true;
#endif
   }
};

// expose platform singleton
CR_EXPOSE_GLOBAL_SINGLETON (CpuFlags, cpuflags);

CR_NAMESPACE_END
