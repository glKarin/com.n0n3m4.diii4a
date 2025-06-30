//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

int32_t ServerQueryHook::sendTo (int socket, const void *message, size_t length, int flags, const sockaddr *dest, int destLength) {
   const auto send = [&] (const Twin <const uint8_t *, size_t> &msg) -> int32_t {
      return Socket::sendto (socket, msg.first, msg.second, flags, dest, destLength);
   };

   auto packet = reinterpret_cast <const uint8_t *> (message);
   constexpr int32_t kPacketLength = 5;

   // player replies response
   if (length > kPacketLength && memcmp (packet, "\xff\xff\xff\xff", kPacketLength - 1) == 0) {
      if (packet[4] == 'D') {
         QueryBuffer buffer { packet, length, kPacketLength };
         auto count = buffer.read <uint8_t> ();

         for (uint8_t i = 0; i < count; ++i) {
            buffer.skip <uint8_t> (); // number
            auto name = buffer.readString (); // name
            buffer.skip <int32_t> (); // score

            auto ctime = buffer.read <float> (); // override connection time
            buffer.write <float> (bots.getConnectTime (name, ctime));
         }
         return send (buffer.data ());
      }
      else if (packet[4] == 'I') {
         QueryBuffer buffer { packet, length, kPacketLength };
         buffer.skip <uint8_t> (); // protocol

         // skip server name, folder, map game
         for (size_t i = 0; i < 4; ++i) {
            buffer.skipString ();
         }
         buffer.skip <short> (); // steam app id
         buffer.skip <uint8_t> (); // players
         buffer.skip <uint8_t> (); // maxplayers
         buffer.skip <uint8_t> (); // bots
         buffer.write <uint8_t> (0); // zero out bot count

         return send (buffer.data ());
      }
      else if (packet[4] == 'm') {
         QueryBuffer buffer { packet, length, kPacketLength };

         buffer.shiftToEnd (); // shift to the end of buffer
         buffer.write <uint8_t> (0); // zero out bot count

         return send (buffer.data ());
      }
   }
   return send ({ packet, length });
}

void ServerQueryHook::init () {
   // if previously requested to disable?
   if (!cv_enable_query_hook) {
      if (m_sendToDetour.detoured ()) {
         disable ();
      }
      return;
   }

   // do not detour twice
   if (m_sendToDetour.detoured () || m_sendToDetourSys.detoured ()) {
      return;
   }

   // do not enable on not dedicated server
   if (!game.isDedicated ()) {
      return;
   }
   SendToProto *sendToAddress = sendto;

   // linux workaround with sendto
   if (!plat.win && !plat.isNonX86 ()) {
      if (game.elib ()) {
         auto address = game.elib ().resolve <SendToProto *> ("sendto");

         if (address != nullptr) {
            sendToAddress = address;
         }
      }
      m_sendToDetourSys.initialize ("ws2_32.dll", "sendto", sendto);
   }
   m_sendToDetour.initialize ("ws2_32.dll", "sendto", sendToAddress);

   // enable only on modern games
   if (!game.is (GameFlags::Legacy) && (plat.nix || plat.win) && !plat.isNonX86 () && !m_sendToDetour.detoured ()) {
      m_sendToDetour.install (reinterpret_cast <void *> (ServerQueryHook::sendTo), true);

      if (!m_sendToDetourSys.detoured ()) {
         m_sendToDetourSys.install (reinterpret_cast <void *> (ServerQueryHook::sendTo), true);
      }
   }
}

SharedLibrary::Func DynamicLinkerHook::lookup (SharedLibrary::Handle module, const char *function) {
   static const auto &gamedll = game.lib ().handle ();
   static const auto &self = m_self.handle ();

   const auto resolve = [&] (SharedLibrary::Handle handle) {
      return m_dlsym (handle, function);
   };

   if (entlink.needsBypass () && !strcmp (function, "CreateInterface")) {
      entlink.setPaused (true);
      auto ret = resolve (module);

      entlink.disable ();

      return ret;
   }

   // if requested module is yapb module, put in cache the looked up symbol
   if (self != module) {
      return resolve (module);
   }

#if defined(CR_WINDOWS)
   if (HIWORD (function) == 0) {
      return resolve (module);
   }
#endif

   if (m_exports.exists (function)) {
      return m_exports[function];
   }
   auto botAddr = resolve (self);

   if (!botAddr) {
      auto gameAddr = resolve (gamedll);

      if (gameAddr) {
         return m_exports[function] = gameAddr;
      }
   }
   else {
      return m_exports[function] = botAddr;
   }
   return nullptr;
}

bool DynamicLinkerHook::callPlayerFunction (edict_t *ent) {
   auto callPlayer = [&] () {
      reinterpret_cast <EntityProto> (reinterpret_cast <void *> (m_exports["player"])) (&ent->v);
   };

   if (m_exports.exists ("player")) {
      callPlayer ();
      return true;
   }
   auto playerFunction = game.lib ().resolve <EntityProto> ("player");

   if (!playerFunction) {
      logger.error ("Cannot resolve player() function in GameDLL.");
      return false;
   }
   m_exports["player"] = reinterpret_cast <SharedLibrary::Func> (reinterpret_cast <void *> (playerFunction));
   callPlayer ();

   return true;
}

bool DynamicLinkerHook::needsBypass () const {
   return !plat.win && !game.isDedicated ();
}

void DynamicLinkerHook::initialize () {
#if defined(LINKENT_STATIC)
   return;
#endif

   if (plat.isNonX86 () || game.is (GameFlags::Metamod)) {
      return;
   }
   constexpr StringRef kKernel32Module = "kernel32.dll";

   m_dlsym.initialize (kKernel32Module, "GetProcAddress", DLSYM_FUNCTION);
   m_dlsym.install (reinterpret_cast <void *> (lookupHandler), true);

   if (needsBypass ()) {
      m_dlclose.initialize (kKernel32Module, "FreeLibrary", DLCLOSE_FUNCTION);
      m_dlclose.install (reinterpret_cast <void *> (closeHandler), true);
   }
   m_self.locate (&engfuncs);
}
