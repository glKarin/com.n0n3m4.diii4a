//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

#include <crlib/basic.h>
#include <crlib/array.h>
#include <crlib/string.h>
#include <crlib/twin.h>

CR_NAMESPACE_BEGIN

template <typename T> struct Hash;

template <> struct Hash <String> {
   uint32_t operator () (const String &key) const noexcept {
      return key.hash ();
   }
};

template <> struct Hash <StringRef> {
   uint32_t operator () (const StringRef &key) const noexcept {
      return key.hash ();
   }
};

template <> struct Hash <const char *> {
   uint32_t operator () (const char *key) const noexcept {
      return StringRef::fnv1a32 (key);
   }
};

template <> struct Hash <int32_t> {
   uint32_t operator () (int32_t key) const noexcept {
      auto result = static_cast <uint32_t> (key);

      result = ((result >> 16) ^ result) * 0x119de1f3;
      result = ((result >> 16) ^ result) * 0x119de1f3;
      result = (result >> 16) ^ result;

      return result;
   }
};

template <typename T> struct EmptyHash {
   uint32_t operator () (T key) const noexcept {
      return static_cast <uint32_t> (key);
   }
};

namespace detail {
   enum class HashEntryStatus : uint8_t {
      Empty,
      Occupied,
      Deleted
   };

   template <typename K, typename V> struct HashEntry final : NonCopyable {
      K key {};
      V val {};
      HashEntryStatus status { HashEntryStatus::Empty };

   public:
      HashEntry () = default;
      ~HashEntry () = default;

   public:
      HashEntry (HashEntry &&rhs) noexcept : key (cr::move (rhs.key)), val (cr::move (rhs.val)), status (rhs.status) {}

   public:
      HashEntry &operator = (HashEntry &&rhs) noexcept {
         if (this != &rhs) {
            key = cr::move (rhs.key);
            val = cr::move (rhs.val);
            status = rhs.status;
         }
         return *this;
      }
   };
};

template <typename K, typename V, typename H = Hash <K>> class HashMap {
private:
   using Entry = detail::HashEntry <K, V>;
   using Status = detail::HashEntryStatus;

   H hash_;
   size_t length_ {};
   Array <Entry> contents_;

private:
   static constexpr size_t kInitialSize = 3;
   static constexpr float kLoadFactor = static_cast <float> (kInitialSize) / 6.0f;
   static constexpr size_t kGrowFractor = static_cast <size_t> (kLoadFactor * 6.0f);
   static constexpr size_t kInvalidDeleteIndex = static_cast <size_t> (-1);

public:
   HashMap () {
      contents_.resize (kInitialSize);
   }

   HashMap (HashMap &&rhs) noexcept : contents_ (cr::move (rhs.contents_)), length_ (rhs.length_), hash_ (cr::move (rhs.hash_)) {}
   ~HashMap () = default;

   HashMap (std::initializer_list <Twin <K, V>> list) {
      contents_.resize (list.size ());

      for (const auto &elem : list) {
         insert (elem.first, cr::move (elem.second));
      }
   }

private:
   void rehash () {
      length_ = 0;

      Array <Entry> contents;
      contents.resize (contents_.length () * kGrowFractor);

      cr::swap (contents_, contents);

      for (auto &entry : contents) {
         if (entry.status == Status::Occupied) {
            insert (entry.key, cr::move (entry.val));
         }
      }
   }

   size_t getIndex (const K &key) const {
      return hash_ (key) % contents_.length ();
   }

   void setIndexOccupied (const size_t index, const K &key, V &&val) {
      contents_[index].key = key;
      contents_[index].val = cr::move (val);
      contents_[index].status = Status::Occupied;

      ++length_;

      const auto length = static_cast <float> (length_);
      const auto capacity = static_cast <float> (contents_.length ());

      if (!contents_.empty () && length / capacity >= kLoadFactor) {
         rehash ();
      }
   }

   V &insertEmpty (const K &key) {
      insert (key, cr::move (V ()));
      size_t index = getIndex (key);

      while (contents_[index].status == Status::Occupied && contents_[index].key != key) {
         index = (index + 1) % contents_.length ();
      }
      return contents_[index].val;
   }

public:
   V &operator [] (const K &key) {
      const size_t index = getIndex (key);

      switch (contents_[index].status) {
      case Status::Empty:
         return insertEmpty (key);

      case Status::Deleted:
         for (size_t i = 1; i < contents_.length (); i++) {
            const size_t probeIndex = (index + i) % contents_.length ();
            auto &entry = contents_[probeIndex];

            if (entry.status == Status::Empty) {
               return insertEmpty (key);
            }

            if (entry.status == Status::Occupied && entry.key == key) {
               return entry.val;
            }
         }
         return insertEmpty (key);

      case Status::Occupied:
         if (contents_[index].key == key) {
            return contents_[index].val;
         }

         for (size_t i = 1; i < contents_.length (); i++) {
            const size_t probeIndex = (index + i) % contents_.length ();
            auto &entry = contents_[probeIndex];

            if (entry.status == Status::Empty) {
               return insertEmpty (key);
            }

            if (entry.status == Status::Occupied && entry.key == key) {
               return entry.val;
            }
         }
         return insertEmpty (key);
      }
      return insertEmpty (key);
   }

   bool insert (const K &key, V val) {
      const size_t index = getIndex (key);

      switch (contents_[index].status) {
      case Status::Empty:
         setIndexOccupied (index, key, cr::move (val));
         return true;

      case Status::Deleted:
         for (size_t i = 1; i < contents_.length (); i++) {
            const size_t probeIndex = (index + i) % contents_.length ();
            auto &entry = contents_[probeIndex];

            if (entry.status == Status::Empty) {
               break;
            }

            if (entry.status == Status::Occupied && entry.key == key) {
               return false;
            }
         }
         setIndexOccupied (index, key, cr::move (val));
         return true;

      case Status::Occupied:
         if (contents_[index].key == key) {
            return false;
         }
         auto deleteIndex = kInvalidDeleteIndex;

         for (size_t i = 1; i < contents_.length (); i++) {
            const size_t probeIndex = (index + i) % contents_.length ();
            auto &entry = contents_[probeIndex];

            if (entry.status == Status::Empty) {
               setIndexOccupied (deleteIndex == kInvalidDeleteIndex ? probeIndex : deleteIndex, key, cr::move (val));
               return true;
            }
            if (entry.status == Status::Deleted && deleteIndex == kInvalidDeleteIndex) {
               deleteIndex = probeIndex;
            }

            if (entry.status == Status::Occupied && entry.key == key) {
               return false;
            }
         }

         if (deleteIndex != kInvalidDeleteIndex) {
            setIndexOccupied (deleteIndex, key, cr::move (val));
            return true;
         }
      }
      return true;
   }

   size_t erase (const K &key) {
      size_t index = getIndex (key);

      switch (contents_[index].status) {
      case Status::Empty:
         return 0;

      case Status::Deleted:
         for (size_t i = 1; i < contents_.length (); i++) {
            const size_t probeIndex = (index + i) % contents_.length ();
            auto &entry = contents_[probeIndex];

            if (entry.status == Status::Empty) {
               return 0;
            }

            if (entry.status == Status::Occupied && entry.key == key) {
               entry.status = Status::Deleted;
               --length_;

               return 1;
            }
         }
         return 0;

      case Status::Occupied:
         if (contents_[index].key == key) {
            contents_[index].status = Status::Deleted;
            --length_;

            return 1;
         }

         for (size_t i = 1; i < contents_.length (); i++) {
            const size_t probeIndex = (index + i) % contents_.length ();
            auto &entry = contents_[probeIndex];

            if (entry.status == Status::Empty) {
               return 0;
            }

            if (entry.status == Status::Occupied && entry.key == key) {
               entry.status = Status::Deleted;
               --length_;

               return 1;
            }
         }
         return 0;
      }
   }

   bool exists (const K &key) const {
      size_t index = getIndex (key);

      while (contents_[index].status == Status::Occupied && contents_[index].key != key) {
         index = (index + 1) % contents_.length ();
      }
      return contents_[index].status == Status::Occupied;
   }

   void foreach (Lambda <void (const K &, const V &)> callback) {
      for (const auto &entry : contents_) {
         if (entry.status == Status::Occupied) {
            callback (entry.key, entry.val);
         }
      }
   }

   void clear () {
      contents_.clear ();
      contents_.resize (kInitialSize);
      contents_.shrink ();

      length_ = 0;
   }

   constexpr size_t length () const {
      return length_;
   }

   constexpr bool empty () const {
      return !!length_;
   }

public:
   HashMap &operator = (HashMap &&rhs) noexcept {
      if (this != &rhs) {
         contents_ = cr::move (rhs.contents_);
         length_ = rhs.length_;
      }
      return *this;
   }
};

CR_NAMESPACE_END
