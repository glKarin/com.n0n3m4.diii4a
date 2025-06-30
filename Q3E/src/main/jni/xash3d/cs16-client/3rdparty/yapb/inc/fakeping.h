//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// adopted from pingfaker amxx plugin
class PingBitMsg final {
private:
   int32_t bits_ {};
   int32_t used_ {};

   MessageWriter msg_ {};
   bool started_ {};

public:
   enum : int32_t {
      Single = 1,
      PlayerID = 5,
      Loss = 7,
      Ping = 12
   };

public:
   PingBitMsg () = default;
   ~PingBitMsg () = default;

public:
   void write (int32_t bit, int32_t size) {
      if (size > 32 - used_ || size < 1) {
         return;
      }
      const auto maxSize = cr::bit (size);

      if (bit >= maxSize) {
         bit = maxSize - 1;
      }
      bits_ = bits_ + (bit << used_);
      used_ += size;
   }

   void send (bool remaining = false) {
      while (used_ >= 8) {
         msg_.writeByte (bits_ & (cr::bit (8) - 1));
         bits_ = (bits_ >> 8);
         used_ -= 8;
      }

      if (remaining && used_ > 0) {
         msg_.writeByte (bits_);
         bits_ = used_ = 0;
      }
   }

   void start (edict_t *ent) {
      if (started_) {
         return;
      }
      msg_.start (MSG_ONE_UNRELIABLE, SVC_PINGS, nullptr, ent);
      started_ = true;
   }

   void flush () {
      if (!started_) {
         return;
      }
      write (0, Single);
      send (true);

      started_ = false;
      msg_.end ();
   }
};

// bot fakeping manager
class BotFakePingManager final : public Singleton <BotFakePingManager> {
private:
   mutable Mutex m_cs {};

private:
   CountdownTimer m_recalcTime {};
   PingBitMsg m_pbm {};

public:
   explicit BotFakePingManager () = default;
   ~BotFakePingManager () = default;

public:
   // verify game supports fakeping and it's enabled
   bool hasFeature () const;

   // reset the ping on disconnecting player
   void reset (edict_t *ent);

   // calculate our own pings for all the bots
   void syncCalculate ();

   // calculate our own pings for all the bots
   void calculate ();

   // emit pings in update client data hook
   void emit (edict_t *ent);

   // resetarts update timers
   void restartTimer ();

   // get random base ping
   int randomBase () const;
};

// expose fakeping manager
CR_EXPOSE_GLOBAL_SINGLETON (BotFakePingManager, fakeping);

