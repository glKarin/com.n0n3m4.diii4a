//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// noise types
CR_DECLARE_SCOPED_ENUM (Noise,
   NeedHandle = cr::bit (0),
   HitFall = cr::bit (1),
   Pickup = cr::bit (2),
   Zoom = cr::bit (3),
   Ammo = cr::bit (4),
   Hostage = cr::bit (5),
   Broke = cr::bit (6),
   Door = cr::bit (7),
   Defuse = cr::bit (8)
)

class BotSounds final : public Singleton <BotSounds> {
private:
   HashMap <String, int32_t> m_noiseCache {};

public:
   BotSounds ();
   ~BotSounds () = default;

public:
   // attaches sound to client struct
   void listenNoise (edict_t *ent, StringRef sample, float volume);

   // simulate sound for players
   void simulateNoise (int playerIndex);
};

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (BotSounds, sounds);
