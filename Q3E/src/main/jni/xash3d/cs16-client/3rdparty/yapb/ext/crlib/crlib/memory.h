//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>
#include <crlib/movable.h>
#include <crlib/platform.h>

// provide placment new to avoid stdc++ <new> header
#if !defined(CR_COMPAT_STL)
inline void *operator new (const size_t, void *ptr) noexcept {
   return ptr;
}
#endif

CR_NAMESPACE_BEGIN

// internal memory manager
class Memory final {
public:
   constexpr Memory () = default;
   ~Memory () = default;

public:
   template <typename T> static constexpr T *get (const size_t length = 1) noexcept {
      auto size = cr::max <size_t> (1u, length) * sizeof (T);

#if defined(CR_CXX_GCC)
      if (size >= PTRDIFF_MAX) {
         plat.abort ();
      }
#endif
      auto memory = reinterpret_cast <T *> (malloc (size));

      if (!memory) {
         char errmsg[384] {};
         snprintf (errmsg, cr::bufsize (errmsg), "Failed to allocate %zd kbytes of memory. Closing down.", size / 1024);

         plat.abort (errmsg);
      }
      return memory;
   }

   template <typename T> static constexpr void release (T *memory) noexcept {
      free (memory);
      memory = nullptr;
   }

public:
   template <typename T, typename ...Args> static constexpr T *construct (T *memory, Args &&...args) noexcept {
      new (memory) T (cr::forward <Args> (args)...);
      return memory;
   }

   template <typename T> static constexpr void destruct (T *memory) {
      memory->~T ();
   }

   template <typename T, typename ...Args> static constexpr T *getAndConstruct (Args &&...args) noexcept {
      auto memory = get <T> ();
      construct <T> (memory, cr::forward <Args> (args)...);

      return memory;
   }

   template <typename T> static constexpr void transfer (T *dest, T *src, size_t length) noexcept {
      for (size_t i = 0; i < length; ++i) {
         construct <T> (&dest[i], cr::move (src[i]));
         destruct <T> (&src[i]);
      }
   }
};

CR_NAMESPACE_END
