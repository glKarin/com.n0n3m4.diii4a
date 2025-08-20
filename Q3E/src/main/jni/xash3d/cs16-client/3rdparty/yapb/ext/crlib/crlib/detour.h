//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>

#if defined(CR_PSVITA) || defined(CR_ARCH_ARM) || defined(CR_ARCH_PPC)

CR_NAMESPACE_BEGIN

// shim for unsupported platforms
template <typename T> class Detour final : public NonCopyable {
public:
   explicit Detour () = default;

   Detour (StringRef module, StringRef name, T *address) {
      initialize (module, name, address);
   }
   ~Detour () = default;

public:
   void initialize (StringRef, StringRef, T *) {}
   void install (void *, const bool) {}

   bool valid () const { return false; }
   bool detoured () const { return false; }
   bool detour () { return false; }
   bool restore () { return false; }

public:
   template <typename... Args> decltype (auto) operator () (Args &&...args) {
      return reinterpret_cast <T *> (reinterpret_cast <uint8_t *> (NULL)) (cr::forward <Args> (args)...);
   }
};

CR_NAMESPACE_END

#else

#if !defined(CR_WINDOWS)
#  include <sys/mman.h>
#  include <pthread.h>
#endif

#include <crlib/thread.h>

CR_NAMESPACE_BEGIN

// simple detour class for x86/x64
template <typename T> class Detour final : public NonCopyable {
private:
   enum : uint32_t {
      PtrSize = sizeof (void *),

#if defined(CR_ARCH_X64)
      Offset = 2
#else
      Offset = 1
#endif
   };

#if defined(CR_ARCH_X64)
   using uintptr = uint64_t;
#else
   using uintptr = uint32_t;
#endif

private:
   Mutex cs_;

   void *original_ = nullptr;
   void *detour_ = nullptr;

   Array <uint8_t> savedBytes_ {};
   Array <uint8_t> jmpBuffer_

#ifdef CR_ARCH_X64
   { 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0 };
#else
   { 0xb8, 0x00, 0x00, 0x00, 0x00, 0xff, 0xe0 };
#endif

#if !defined(CR_WINDOWS)
   unsigned long pageSize_ { 0 };
   uintptr pageStart_ { 0 };
#endif

   bool overwritten_ { false };

private:
   bool overwriteMemory (const Array <uint8_t> &to, const bool overwritten) noexcept {
      overwritten_ = overwritten;

      MutexScopedLock lock (cs_);
#if defined(CR_WINDOWS)
      unsigned long oldProtect {};

      if (VirtualProtect (original_, to.length (), PAGE_EXECUTE_READWRITE, &oldProtect)) {
         memcpy (original_, to.data (), to.length ());

         // restore protection
         return VirtualProtect (original_, to.length (), oldProtect, &oldProtect);
      }
#else
      if (mprotect (reinterpret_cast <void *> (pageStart_), pageSize_, PROT_READ | PROT_WRITE | PROT_EXEC) != -1) {
         memcpy (original_, to.data (), to.length ());

         // restore protection
         return mprotect (reinterpret_cast <void *> (pageStart_), pageSize_, PROT_READ | PROT_EXEC) != -1;
      }
#endif
      return false;
   }

public:
   Detour (const Detour &) = delete;
   Detour &operator=(const Detour &) = delete;

public:
   explicit Detour () = default;

   Detour (StringRef module, StringRef name, T *address) {
      initialize (module, name, address);
   }

   ~Detour () {
      restore ();

      original_ = nullptr;
      detour_ = nullptr;
   }

private:
   class DetourSwitch final {
   private:
      Detour <T> *detour_;

   public:
      DetourSwitch (Detour *detour) : detour_ (detour) {
         detour_->restore ();
      }

      ~DetourSwitch () {
         detour_->detour ();
      }
   };

public:
   void initialize (StringRef module, StringRef name, T *address) {
      savedBytes_.resize (jmpBuffer_.length ());

#if !defined(CR_WINDOWS)
      (void) module;
      (void) name;

      auto ptr = reinterpret_cast <uint8_t *> (address);

      while (*reinterpret_cast <uint16_t *> (ptr) == 0x25ff) {
         ptr = **reinterpret_cast <uint8_t ***> (ptr + 2);
      }

      original_ = ptr;
      pageSize_ = static_cast <unsigned long> (sysconf (_SC_PAGE_SIZE));
#else
      auto handle = GetModuleHandleA (module.chars ());

      if (!handle) {
         original_ = reinterpret_cast <void *> (address);
         return;
      }
      original_ = reinterpret_cast <void *> (GetProcAddress (handle, name.chars ()));

      if (!original_) {
         original_ = reinterpret_cast <void *> (address);
         return;
      }
#endif
   }

   void install (void *detour, const bool enable = false) {
      if (!original_) {
         return;
      }
      detour_ = detour;

#if !defined(CR_WINDOWS)
      pageStart_ = reinterpret_cast <uintptr> (original_) & -pageSize_;
#endif

      memcpy (savedBytes_.data (), original_, savedBytes_.length ());
      memcpy (reinterpret_cast <void *> (reinterpret_cast <uintptr> (jmpBuffer_.data ()) + Offset), &detour_, PtrSize);

      if (enable) {
         this->detour ();
      }
   }

   bool valid () const {
      return original_ && detour_;
   }

   bool detoured () const {
      return overwritten_;
   }

   bool detour () {
      if (!valid ()) {
         return false;
      }
      return overwriteMemory (jmpBuffer_, true);
   }

   bool restore () {
      if (!valid ()) {
         return false;
      }
      return overwriteMemory (savedBytes_, false);
   }

   template <typename... Args> decltype (auto) operator () (Args &&...args) {
      DetourSwitch sw { this };
      return reinterpret_cast <T *> (original_) (cr::forward <Args> (args)...);
   }
};

CR_NAMESPACE_END

#endif
