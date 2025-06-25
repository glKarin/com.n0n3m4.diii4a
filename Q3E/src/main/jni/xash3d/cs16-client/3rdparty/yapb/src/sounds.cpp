//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

BotSounds::BotSounds () {
   // register noise cache
   m_noiseCache["player/bhit"] = Noise::NeedHandle | Noise::HitFall;
   m_noiseCache["player/head"] = Noise::NeedHandle | Noise::HitFall;
   m_noiseCache["items/gunpi"] = Noise::NeedHandle | Noise::Pickup;
   m_noiseCache["items/9mmcl"] = Noise::NeedHandle | Noise::Ammo;
   m_noiseCache["weapons/zoo"] = Noise::NeedHandle | Noise::Zoom;
   m_noiseCache["hostage/hos"] = Noise::NeedHandle | Noise::Hostage;
   m_noiseCache["debris/bust"] = Noise::NeedHandle | Noise::Broke;
   m_noiseCache["doors/doorm"] = Noise::NeedHandle | Noise::Door;
   m_noiseCache["weapons/c4_"] = Noise::NeedHandle | Noise::Defuse;
}

void BotSounds::listenNoise (edict_t *ent, StringRef sample, float volume) {
   // this function called by the sound hooking code (in emit_sound) enters the played sound into the array associated with the entity

   if (game.isNullEntity (ent) || sample.empty ()) {
      return;
   }
   const auto &origin = game.getEntityOrigin (ent);

   // something wrong with sound...
   if (origin.empty ()) {
      return;
   }
   const auto noise = m_noiseCache[sample.substr (0, 11)];

   // we're not handling theese
   if (!(noise & Noise::NeedHandle)) {
      return;
   }

   // find nearest player to sound origin
   auto findNearbyClient = [&origin] () {
      float nearest = kInfiniteDistance;
      Client *result = nullptr;

      // loop through all players
      for (auto &client : util.getClients ()) {
         if (!(client.flags & ClientFlags::Used) || !(client.flags & ClientFlags::Alive)) {
            continue;
         }
         const auto distanceSq = client.origin.distanceSq (origin);

         // now find nearest player
         if (distanceSq < nearest) {
            result = &client;
            nearest = distanceSq;
         }
      }
      return result;
   };
   auto client = findNearbyClient ();

   // update noise stats
   auto registerNoise = [&origin, &client, &volume] (float distance, float lasting) {
      client->noise.dist = distance * volume;
      client->noise.last = game.time () + lasting;
      client->noise.pos = origin;
   };

   // client wasn't found
   if (!client) {
      return;
   }

   // hit/fall sound?
   if (noise & Noise::HitFall) {
      registerNoise (768.0f, 0.52f);
   }

   // weapon pickup?
   else if (noise & Noise::Pickup) {
      registerNoise (768.0f, 0.45f);
   }

   // sniper zooming?
   else if (noise & Noise::Zoom) {
      registerNoise (512.0f, 0.10f);
   }

   // ammo pickup?
   else if (noise & Noise::Ammo) {
      registerNoise (512.0f, 0.25f);
   }

   // ct used hostage?
   else if (noise & Noise::Hostage) {
      registerNoise (1024.0f, 5.00);
   }

   // broke something?
   else if (noise & Noise::Broke) {
      registerNoise (1024.0f, 2.00f);
   }

   // someone opened a door
   else if (noise & Noise::Door) {
      registerNoise (1024.0f, 3.00f);
   }

   // some one started to defuse
   else if ((noise & Noise::Defuse) && sample[11] == 'd') {
      registerNoise (512.0f, 0.5f);
   }
}

void BotSounds::simulateNoise (int playerIndex) {
   // this function tries to simulate playing of sounds to let the bots hear sounds which aren't
   // captured through server sound hooking

   if (playerIndex < 0 || playerIndex >= game.maxClients ()) {
      return; // reliability check
   }
   auto &client = util.getClient (playerIndex);
   ClientNoise noise {};

   auto buttons = client.ent->v.button | client.ent->v.oldbuttons;

   // pressed attack button?
   if (buttons & IN_ATTACK) {
      noise.dist = 2048.0f;
      noise.last = game.time () + 0.3f;
   }

   // pressed used button?
   else if (buttons & IN_USE) {
      noise.dist = 512.0f;
      noise.last = game.time () + 0.5f;
   }

   // pressed reload button?
   else if (buttons & IN_RELOAD) {
      noise.dist = 512.0f;
      noise.last = game.time () + 0.5f;
   }

   // uses ladder?
   else if (client.ent->v.movetype == MOVETYPE_FLY) {
      if (cr::abs (client.ent->v.velocity.z) > 50.0f) {
         noise.dist = 1024.0f;
         noise.last = game.time () + 0.3f;
      }
   }
   else {
      if (mp_footsteps) {
         // moves fast enough?
         noise.dist = 1280.0f * (client.ent->v.velocity.length2d () / 260.0f);
         noise.last = game.time () + 0.3f;
      }
   }

   if (noise.dist <= 0.0f) {
      return; // didn't issue sound?
   }

   // some sound already associated
   if (client.noise.last > game.time ()) {
      if (client.noise.dist <= noise.dist) {
         // override it with new
         client.noise.dist = noise.dist;
         client.noise.last = noise.last;
         client.noise.pos = client.ent->v.origin;
      }
   }
   else if (!cr::fzero (noise.last)) {
      // just remember it
      client.noise.dist = noise.dist;
      client.noise.last = noise.last;
      client.noise.pos = client.ent->v.origin;
   }
}
