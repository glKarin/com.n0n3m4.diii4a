//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// limits for storing practice data
CR_DECLARE_SCOPED_ENUM_TYPE (PracticeLimit, int32_t,
   Goal = 2040,
   Damage = 2040
);

// storage for from, to, team
class DangerStorage final {
protected:
   uint16_t data[3] {};

public:
   constexpr DangerStorage () = default;

public:
   constexpr DangerStorage (const int32_t &a, const int32_t &b, const int32_t &c) :
      data { static_cast <uint16_t> (a), static_cast <uint16_t> (b), static_cast <uint16_t> (c) } {}

public:
   constexpr bool operator == (const DangerStorage &rhs) const {
      return rhs.data[2] == data[2] && rhs.data[1] == data[1] && rhs.data[0] == data[0];
   }

   constexpr bool operator != (const DangerStorage &rhs) const {
      return !operator == (rhs);
   }

public:
   // fnv1a for 3d vector hash
   constexpr uint32_t hash () const {
      constexpr uint32_t prime = 16777619u;
      constexpr uint32_t seed = 2166136261u;

      uint32_t hash = seed;

      for (const auto &key : data) {
         hash = (hash * prime) ^ key;
      }
      return hash;
   }
};

// define hash function for hash map
CR_NAMESPACE_BEGIN

template <> struct Hash <DangerStorage> {
   uint32_t operator () (const DangerStorage &key) const noexcept {
      return key.hash ();
   }
};

CR_NAMESPACE_END

class BotPractice final : public Singleton <BotPractice> {
public:
   // collected data
   struct PracticeData {
      int16_t damage {}, value {};
      int16_t index { kInvalidNodeIndex };
   };

   // used to save-restore practice data
   struct DangerSaveRestore {
      DangerStorage danger {};
      PracticeData data {};

   public:
      DangerSaveRestore () = default;

   public:
      DangerSaveRestore (const DangerStorage &ds, const PracticeData &pd) : danger (ds), data (pd) {}
   };

private:
   HashMap <DangerStorage, PracticeData> m_data {};
   int32_t m_teamHighestDamage[kGameTeamNum] {};

   // avoid concurrent access to practice
   mutable Mutex m_damageUpdateLock {};

public:
   BotPractice () = default;
   ~BotPractice () = default;

private:
   bool exists (int32_t team, int32_t start, int32_t goal) const {
      return m_data.exists ({ start, goal, team });
   }
   void syncUpdate ();

public:
   int32_t getIndex (int32_t team, int32_t start, int32_t goal);
   void setIndex (int32_t team, int32_t start, int32_t goal, int32_t value);

   int32_t getValue (int32_t team, int32_t start, int32_t goal);
   void setValue (int32_t team, int32_t start, int32_t goal, int32_t value);

   int32_t getDamage (int32_t team, int32_t start, int32_t goal);
   void setDamage (int32_t team, int32_t start, int32_t goal, int32_t value);

   // interlocked get damage
   float plannerGetDamage (int32_t team, int32_t start, int32_t goal, bool addTeamHighestDamage);

public:
   void update ();
   void load ();
   void save ();

private:
   void syncLoad ();

public:
   template <typename U = int32_t> U getHighestDamageForTeam (int32_t team) const {
      return static_cast <U> (cr::max (1, m_teamHighestDamage[team]));
   }

   void setHighestDamageForTeam (int32_t team, int32_t value) {
      MutexScopedLock lock (m_damageUpdateLock);
      m_teamHighestDamage[team] = value;
   }
};

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (BotPractice, practice);
