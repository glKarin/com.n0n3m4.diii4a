//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_csdm_mode ("csdm_mode", "0", "Enables or disables CSDM / FFA mode for bots.\nAllowed values: '0', '1', '2', '3'.\nIf '0', CSDM / FFA mode is auto-detected.\nIf '1', CSDM mode is enabled, but FFA is disabled.\nIf '2', CSDM and FFA mode is enabled.\nIf '3', CSDM and FFA mode is disabled.", true, 0.0f, 3.0f);
ConVar cv_ignore_map_prefix_game_mode ("ignore_map_prefix_game_mode", "0", "If enabled, bots will not apply game modes based on map name prefix (fy_ and ka_ specifically).");
ConVar cv_threadpool_workers ("threadpool_workers", "-1", "Maximum number of threads bot will run to process some tasks. -1 means half of CPU cores used.", true, -1.0f, static_cast <float> (plat.hardwareConcurrency ()));
ConVar cv_grenadier_mode ("grenadier_mode", "0", "If enabled, bots will not apply throwing condition on grenades.");
ConVar cv_ignore_enemies_after_spawn_time ("ignore_enemies_after_spawn_time", "0", "Make bots ignore enemies for a specified here time in seconds on new round. Useful for Zombie Plague mods.", false);

ConVar sv_skycolor_r ("sv_skycolor_r", nullptr, Var::GameRef);
ConVar sv_skycolor_g ("sv_skycolor_g", nullptr, Var::GameRef);
ConVar sv_skycolor_b ("sv_skycolor_b", nullptr, Var::GameRef);

Game::Game () {
   m_startEntity = nullptr;
   m_localEntity = nullptr;

   m_precached = false;

   m_gameFlags = 0;
   m_mapFlags = 0;
   m_oneSecondFrame = 0.0f;
   m_halfSecondFrame = 0.0f;

   m_cvars.clear ();
}

void Game::precache () {
   if (m_precached) {
      return;
   }
   m_precached = true;

   m_drawModels[DrawLine::Simple] = m_engineWrap.precacheModel ("sprites/laserbeam.spr");
   m_drawModels[DrawLine::Arrow] = m_engineWrap.precacheModel ("sprites/arrow1.spr");

   m_engineWrap.precacheSound ("weapons/xbow_hit1.wav"); // node add
   m_engineWrap.precacheSound ("weapons/mine_activate.wav"); // node delete
   m_engineWrap.precacheSound ("common/wpn_hudon.wav"); // path add/delete done

   m_mapFlags = 0; // reset map type as worldspawn is the first entity spawned
   registerCvars (true);
}

void Game::levelInitialize (edict_t *entities, int max) {
   // this function precaches needed models and initialize class variables

   // enable command handling
   ctrl.setDenyCommands (false);

   // re-initialize bot's array
   bots.destroy ();

   // startup threaded worker
   worker.startup (cv_threadpool_workers.as <int> ());

   m_spawnCount[Team::CT] = 0;
   m_spawnCount[Team::Terrorist] = 0;

   // clear all breakables before initialization
   m_breakables.clear ();
   m_checkedBreakables.clear ();

   // initialize all config files
   conf.loadConfigs ();

   // update worldmodel
   illum.resetWorldModel ();

   // execute main config
   conf.loadMainConfig ();

   // ensure the server admin is confident about features he's using
   game.ensureHealthyGameEnvironment ();

   // load map-specific config
   conf.loadMapSpecificConfig ();

   // do level initialization stuff here...
   graph.loadGraphData ();

   // initialize quota management
   bots.initQuota ();

   // install the sendto hook to fake queries
   fakequeries.init ();

   // flush any print queue
   ctrl.resetFlushTimestamp ();

   // set the global timer function
   timerStorage.setTimeAddress (&globals->time);

   // restart the fakeping timer, so it'll start working after mapchange
   fakeping.restartTimer ();

   // go thru the all entities on map, and do whatever we're want
   for (int i = 0; i < max; ++i) {
      auto ent = entities + i;

      // only valid entities
      if (!ent || ent->v.classname == 0) {
         continue;
      }
      auto classname = ent->v.classname.str ();

      if (classname == "worldspawn") {
         m_startEntity = ent;
      }
      else if (classname == "player_weaponstrip") {
         if (is (GameFlags::Legacy) && strings.isEmpty (ent->v.target.chars ())) {
            ent->v.target = ent->v.targetname = engfuncs.pfnAllocString ("fake");
         }
         else if (!is (GameFlags::ReGameDLL)) {
            engfuncs.pfnRemoveEntity (ent);
         }
      }
      else if (classname == "info_player_start" || classname == "info_vip_start") {
         ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
         ent->v.renderamt = 127; // set its transparency amount
         ent->v.effects |= EF_NODRAW;

         ++m_spawnCount[Team::CT];
      }
      else if (classname == "info_player_deathmatch") {
         ent->v.rendermode = kRenderTransAlpha; // set its render mode to transparency
         ent->v.renderamt = 127; // set its transparency amount
         ent->v.effects |= EF_NODRAW;

         ++m_spawnCount[Team::Terrorist];
      }
      else if (classname == "func_vip_safetyzone" || classname == "info_vip_safetyzone") {
         m_mapFlags |= MapFlags::Assassination; // assassination map
      }
      else if (util.isHostageEntity (ent)) {
         m_mapFlags |= MapFlags::HostageRescue; // rescue map
      }
      else if (classname == "func_bomb_target" || classname == "info_bomb_target") {
         m_mapFlags |= MapFlags::Demolition; // defusion map
      }
      else if (classname == "func_escapezone") {
         m_mapFlags |= MapFlags::Escape;

         // strange thing on some ES maps, where hostage entity present there
         if (m_mapFlags & MapFlags::HostageRescue) {
            m_mapFlags &= ~MapFlags::HostageRescue;
         }
      }
      else if (util.isDoorEntity (ent)) {
         m_mapFlags |= MapFlags::HasDoors;
      }
      else if (classname.startsWith ("func_button")) {
         m_mapFlags |= MapFlags::HasButtons;
      }
      else if (util.isBreakableEntity (ent, true)) {

         // add breakable for material check
         m_checkedBreakables[indexOfEntity (ent)] = ent->v.impulse <= 0;
         m_breakables.push (ent);
      }
   }

   // next maps doesn't have map-specific entities, so determine it by name
   if (!cv_ignore_map_prefix_game_mode) {
      StringRef prefix = getMapName ();

      if (prefix.startsWith ("fy_")) {
         m_mapFlags |= MapFlags::FightYard;
      }
      else if (prefix.startsWith ("ka_")) {
         m_mapFlags |= MapFlags::KnifeArena;
      }
      else if (prefix.startsWith ("he_")) {
         m_mapFlags |= MapFlags::GrenadeWar;
      }
   }

   // reset some timers
   m_oneSecondFrame = 0.0f;
   m_halfSecondFrame = 0.0f;
}

void Game::levelShutdown () {
   // save collected practice on shutdown
   practice.save ();

   // stop thread pool
   worker.shutdown ();

   // destroy global killer entity
   bots.destroyKillerEntity ();

   // ensure players are off on xash3d
   if (game.is (GameFlags::Xash3DLegacy)) {
      bots.kickEveryone (true, false);
   }

   // set state to unprecached
   game.setUnprecached ();

   // enable lightstyle animations on level change
   illum.enableAnimation (true);

   // send message on new map
   util.setNeedForWelcome (false);

   // clear local entity
   game.setLocalEntity (nullptr);

   // reset graph state
   graph.reset ();

   // suspend any analyzer tasks
   analyzer.suspend ();

   // disable command handling
   ctrl.setDenyCommands (true);

}

void Game::drawLine (edict_t *ent, const Vector &start, const Vector &end, int width, int noise, const Color &color, int brightness, int speed, int life, DrawLine type) const {
   // this function draws a arrow visible from the client side of the player whose player entity
   // is pointed to by ent, from the vector location start to the vector location end,
   // which is supposed to last life tenths seconds, and having the color defined by RGB.

   if (!util.isPlayer (ent)) {
      return; // reliability check
   }

   MessageWriter (MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, ent)
      .writeByte (TE_BEAMPOINTS)
      .writeCoord (end.x)
      .writeCoord (end.y)
      .writeCoord (end.z)
      .writeCoord (start.x)
      .writeCoord (start.y)
      .writeCoord (start.z)
      .writeShort (m_drawModels[type])
      .writeByte (0) // framestart
      .writeByte (10) // framerate
      .writeByte (life) // life in 0.1's
      .writeByte (width) // width
      .writeByte (noise) // noise
      .writeByte (color.red) // r, g, b
      .writeByte (color.green) // r, g, b
      .writeByte (color.blue) // r, g, b
      .writeByte (brightness) // brightness
      .writeByte (speed); // speed
}

void Game::testModel (const Vector &start, const Vector &end, int hullNumber, edict_t *entToHit, TraceResult *ptr) {
   engfuncs.pfnTraceModel (start, end, hullNumber, entToHit, ptr);
}

void Game::testLine (const Vector &start, const Vector &end, int ignoreFlags, edict_t *ignoreEntity, TraceResult *ptr) {
   // this function traces a line dot by dot, starting from vecStart in the direction of vecEnd,
   // ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or false), and stops
   // at the first obstacle encountered, returning the results of the trace in the TraceResult structure
   // ptr. Such results are (amongst others) the distance traced, the hit surface, the hit plane
   // vector normal, etc. See the TraceResult structure for details. This function allows to specify
   // whether the trace starts "inside" an entity's polygonal model, and if so, to specify that entity
   // in ignoreEntity in order to ignore it as a possible obstacle.

   auto engineFlags = 0;

   if (ignoreFlags & TraceIgnore::Monsters) {
      engineFlags = 1;
   }

   if (ignoreFlags & TraceIgnore::Glass) {
      engineFlags |= 0x100;
   }
   engfuncs.pfnTraceLine (start, end, engineFlags, ignoreEntity, ptr);
}

void Game::testHull (const Vector &start, const Vector &end, int ignoreFlags, int hullNumber, edict_t *ignoreEntity, TraceResult *ptr) {
   // this function traces a hull dot by dot, starting from vecStart in the direction of vecEnd,
   // ignoring or not monsters (depending on the value of IGNORE_MONSTERS, true or
   // false), and stops at the first obstacle encountered, returning the results
   // of the trace in the TraceResult structure ptr, just like TraceLine. Hulls that can be traced
   // (by parameter hull_type) are point_hull (a line), head_hull (size of a crouching player),
   // human_hull (a normal body size) and large_hull (for monsters?). Not all the hulls in the
   // game can be traced here, this function is just useful to give a relative idea of spatial
   // reachability (i.e. can a hostage pass through that tiny hole ?) Also like TraceLine, this
   // function allows to specify whether the trace starts "inside" an entity's polygonal model,
   // and if so, to specify that entity in ignoreEntity in order to ignore it as an obstacle.

   engfuncs.pfnTraceHull (start, end, !!(ignoreFlags & TraceIgnore::Monsters), hullNumber, ignoreEntity, ptr);
}

bool Game::isDedicated () {
   // return true if server is dedicated server, false otherwise
   static const bool dedicated = engfuncs.pfnIsDedicatedServer () > 0;

   return dedicated;
}

const char *Game::getRunningModName () {
   // this function returns mod name without path

   static String name {};

   if (!name.empty ()) {
      return name.chars ();
   }

   char engineModName[StringBuffer::StaticBufferSize] {};
   engfuncs.pfnGetGameDir (engineModName);

   name = engineModName;
   size_t slash = name.findLastOf ("\\/");

   if (slash != String::InvalidIndex) {
      name = name.substr (slash + 1);
   }
   name = name.trim (" \\/");
   return name.chars ();
}

const char *Game::getMapName () {
   // this function gets the map name and store it in the map_name global string variable.

   return strings.format ("%s", globals->mapname.chars ());
}

Vector Game::getEntityOrigin (edict_t *ent) {
   // this expanded function returns the vector origin of a bounded entity, assuming that any
   // entity that has a bounding box has its center at the center of the bounding box itself.

   if (isNullEntity (ent)) {
      return nullptr;
   }

   if (ent->v.origin.empty ()) {
      return ent->v.absmin + (ent->v.size * 0.5);
   }
   return ent->v.origin;
}

void Game::registerEngineCommand (const char *command, void func ()) {
   // this function tells the engine that a new server command is being declared, in addition
   // to the standard ones, whose name is command_name. The engine is thus supposed to be aware
   // that for every "command_name" server command it receives, it should call the function
   // pointed to by "function" in order to handle it.

   // check for hl pre 1.1.0.4, as it's doesn't have pfnAddServerCommand and many more stuff we need to work
   if (!plat.isValidPtr (engfuncs.pfnAddServerCommand)) {
      logger.fatal ("%s's minimum HL engine version is 1.1.0.4 and minimum Counter-Strike is Beta 6.5. Please update your engine / game version.", product.name);
   }
   else {
      engfuncs.pfnAddServerCommand (command, func);
   }
}

void Game::playSound (edict_t *ent, const char *sound) {
   if (isNullEntity (ent)) {
      return;
   }
   engfuncs.pfnEmitSound (ent, CHAN_WEAPON, sound, 1.0f, ATTN_NORM, 0, 100);
}

void Game::setPlayerStartDrawModels () {
   static HashMap <String, String> models {
      {"info_player_start", "models/player/urban/urban.mdl"},
      {"info_player_deathmatch", "models/player/terror/terror.mdl"},
      {"info_vip_start", "models/player/vip/vip.mdl"}
   };

   models.foreach ([&] (const String &key, const String &val) {
      game.searchEntities ("classname", key, [&] (edict_t *ent) {
         m_engineWrap.setModel (ent, val.chars ());
         return EntitySearchResult::Continue;
      });
   });
}

bool Game::checkVisibility (edict_t *ent, uint8_t *set) {
   if (!set) {
      return true;
   }

   if (ent->headnode < 0) {
      for (int i = 0; i < ent->num_leafs; ++i) {
         const auto leaf = ent->leafnums[i];

         if (set[leaf >> 3] & cr::bit (leaf & 7)) {
            return true;
         }
      }
      return false;
   }

   for (int i = 0; i < MAX_ENT_LEAFS; ++i) {
      const auto leaf = ent->leafnums[i];

      if (leaf == -1) {
         break;
      }

      if (set[leaf >> 3] & cr::bit (leaf & 7)) {
         return true;
      }
   }
   return engfuncs.pfnCheckVisibility (ent, set) > 0;
}

uint8_t *Game::getVisibilitySet (Bot *bot, bool pvs) const {
   if (is (GameFlags::Xash3DLegacy)) {
      return nullptr;
   }
   auto eyes = bot->getEyesPos ();

   if (bot->isDucking ()) {
      eyes += VEC_HULL_MIN - VEC_DUCK_HULL_MIN;
   }
   return pvs ? engfuncs.pfnSetFatPVS (eyes) : engfuncs.pfnSetFatPAS (eyes);
}

void Game::sendClientMessage (bool console, edict_t *ent, StringRef message) {
   // helper to sending the client message

   // do not send messages to fake clients
   if (!util.isPlayer (ent) || util.isFakeClient (ent)) {
      return;
   }

   // if console message and destination is listenserver entity, just print via server message instead of through unreliable channel
   if (console && ent == game.getLocalEntity ()) {
      sendServerMessage (message);
      return;
   }
   const String &buffer = message;

   // used to split messages
   auto sendTextMsg = [&console, &ent] (StringRef text) {
      MessageWriter (MSG_ONE_UNRELIABLE, msgs.id (NetMsg::TextMsg), nullptr, ent)
         .writeByte (console ? HUD_PRINTCONSOLE : HUD_PRINTCENTER)
         .writeString (text.chars ());
   };

   // do not excess limit
   constexpr size_t kMaxSendLength = 125;

   // split up the string into chunks if needed (maybe check if it's multibyte?)
   if (buffer.length () > kMaxSendLength) {
      auto chunks = buffer.split (kMaxSendLength);

      // send in chunks
      for (size_t i = 0; i < chunks.length (); ++i) {
         sendTextMsg (chunks[i]);
      }
      return;
   }
   sendTextMsg (buffer);
}

void Game::sendServerMessage (StringRef message) {
   // helper to sending the client message

   // do not excess limit
   constexpr size_t kMaxSendLength = 175;

   // split up the string into chunks if needed (maybe check if it's multibyte?)
   if (message.length () > kMaxSendLength) {
      auto chunks = message.split <String> (kMaxSendLength);

      // send in chunks
      for (size_t i = 0; i < chunks.length (); ++i) {
         engfuncs.pfnServerPrint (chunks[i].chars ());
      }
      return;
   }
   engfuncs.pfnServerPrint (message.chars ());
}

void Game::sendHudMessage (edict_t *ent, const hudtextparms_t &htp, StringRef message) {
   constexpr size_t kMaxSendLength = 512;

   if (game.isNullEntity (ent)) {
      return;
   }
   MessageWriter msg (MSG_ONE_UNRELIABLE, SVC_TEMPENTITY, nullptr, ent);

   msg.writeByte (TE_TEXTMESSAGE);
   msg.writeByte (htp.channel & 0xff);
   msg.writeShort (MessageWriter::fs16 (htp.x, 13.0f));
   msg.writeShort (MessageWriter::fs16 (htp.y, 13.0f));
   msg.writeByte (htp.effect);
   msg.writeByte (htp.r1);
   msg.writeByte (htp.g1);
   msg.writeByte (htp.b1);
   msg.writeByte (htp.a1);
   msg.writeByte (htp.r2);
   msg.writeByte (htp.g2);
   msg.writeByte (htp.b2);
   msg.writeByte (htp.a2);
   msg.writeShort (MessageWriter::fu16 (htp.fadeinTime, 8.0f));
   msg.writeShort (MessageWriter::fu16 (htp.fadeoutTime, 8.0f));
   msg.writeShort (MessageWriter::fu16 (htp.holdTime, 8.0f));

   if (htp.effect == 2) {
      msg.writeShort (MessageWriter::fu16 (htp.fxTime, 8.0f));
   }
   msg.writeString (message.substr (0, kMaxSendLength).chars ());
}

void Game::prepareBotArgs (edict_t *ent, String str) {
   // the purpose of this function is to provide fakeclients (bots) with the same client
   // command-scripting advantages (putting multiple commands in one line between semicolons)
   // as real players. It is an improved version of botman's FakeClientCommand, in which you
   // supply directly the whole string as if you were typing it in the bot's "console". It
   // is supposed to work exactly like the pfnClientCommand (server-sided client command).

   m_botArgs.clear (); // always clear args

   if (str.empty ()) {
      return;
   }

   // helper to parse single (not multi) command
   auto parsePartArgs = [&] (String &args) {
      args.trim ("\r\n\t\" "); // trim new lines

      // we're have empty commands?
      if (args.empty ()) {
         return;
      }

      // find first space
      const size_t space = args.find (' ', 0);

      // if found space
      if (space != String::InvalidIndex) {
         const auto quote = space + 1; // check for quote next to space

         // check if we're got a quoted string
         if (quote < args.length () && args[quote] == '\"') {
            m_botArgs.emplace (args.substr (0, space)); // add command
            m_botArgs.emplace (args.substr (quote, args.length () - 1).trim ("\"")); // add string with trimmed quotes
         }
         else {
            for (auto &&arg : args.split (" ")) {
               m_botArgs.emplace (arg);
            }
         }
      }
      else {
         m_botArgs.emplace (args); // move all the part to args
      }
      MDLL_ClientCommand (ent);

      // clear space for next cmd 
      m_botArgs.clear ();
   };

   if (str.find (';', 0) != String::InvalidIndex) {
      for (auto &&part : str.split (";")) {
         parsePartArgs (part.trim ());
      }
   }
   else {
      parsePartArgs (str);
   }
}

bool Game::isSoftwareRenderer () {
   // xash always use "hw" structures
   if (is (GameFlags::Xash3D)) {
      return false;
   }

   // dedicated server (except xash) always use "sw" structures
   if (isDedicated ()) {
      return true;
   }
   auto model = illum.getWorldModel ();

   if (model->nodes[0].parent != nullptr) {
      return false;
   }
   const auto child = model->nodes[0].children[0];

   if (child < model->nodes || child > model->nodes + model->numnodes) {
      return false;
   }

   if (child->parent != &model->nodes[0]) {
      return false;
   }

   // and on only windows version you can use software-render game. Linux, macOS always defaults to OpenGL
   if (plat.win) {
      return plat.hasModule ("sw");
   }
   return false;
}

bool Game::is25thAnniversaryUpdate () {
   static ConVarRef sv_use_steam_networking ("sv_use_steam_networking");
   static ConVarRef host_hl25_extended_structs ("host_hl25_extended_structs");

   return sv_use_steam_networking.exists () || host_hl25_extended_structs.value () > 0.0f;
}

void Game::pushConVar (StringRef name, StringRef value, StringRef info, bool bounded, float min, float max, int32_t varType, bool missingAction, StringRef regval, ConVar *self) {
   // this function adds globally defined variable to registration stack

   ConVarReg reg {};

   reg.reg.name = name.chars ();
   reg.reg.string = value.chars ();
   reg.name = name;
   reg.missing = missingAction;
   reg.init = value;
   reg.info = info;
   reg.bounded = bounded;

   if (!regval.empty ()) {
      reg.regval = regval;
   }

   if (bounded) {
      reg.min = min;
      reg.max = max;
      reg.initial = value.as <float> ();
   }

   int eflags = FCVAR_EXTDLL;

   if (varType == Var::Normal) {
      eflags |= FCVAR_SERVER;
   }
   else if (varType == Var::ReadOnly) {
      eflags |= FCVAR_SERVER | FCVAR_SPONLY | FCVAR_PRINTABLEONLY;
   }
   else if (varType == Var::Password) {
      eflags |= FCVAR_PROTECTED;
   }

   reg.reg.flags = eflags;
   reg.self = self;
   reg.type = varType;

   m_cvars.push (cr::move (reg));
}

void ConVar::revert () {
   if (!ptr) {
      return;
   }
   const auto &cvars = game.getCvars ();

   for (const auto &var : cvars) {
      if (var.name == ptr->name) {
         set (var.init.chars ());
         break;
      }
   }
}

void ConVar::setPrefix (StringRef name, int32_t type) {
   if (type == Var::GameRef) {
      name_ = name;
      return;
   }
   name_.assignf ("%s_%s", product.cmdPri, name);
}

void Game::checkCvarsBounds () {
   for (const auto &var : m_cvars) {
      if (!var.self || !var.self->ptr) {
         continue;
      }

      // read only cvar is not changeable
      if (var.type == Var::ReadOnly && !var.init.empty ()) {
         if (var.init != var.self->as <StringRef> ()) {
            var.self->set (var.init.chars ());
         }
         continue;
      }

      if (!var.bounded || !var.self) {
         continue;
      }
      auto value = var.self->as <float> ();
      auto str = String (var.self->as <StringRef> ());

      // check the bounds and set default if out of bounds
      if (value > var.max || value < var.min || (!str.empty () && isalpha (str[0]))) {
         var.self->set (var.initial);

         // notify about that
         ctrl.msg ("Bogus value for cvar '%s', min is '%.1f' and max is '%.1f', and we're got '%s', value reverted to default '%.1f'.", var.name, var.min, var.max, str, var.initial);
         continue;
      }

      /// prevent min/max problems
      if (var.name.contains ("_max")) {
         String minVar = String (var.name);
         minVar.replace ("_max", "_min");

         for (auto &mv : m_cvars) {
            if (mv.name == minVar) {
               const auto minValue = mv.self->as <float> ();

               if (minValue > value) {
                  var.self->set (minValue);
                  mv.self->set (value);

                  // notify about that
                  ctrl.msg ("Bogus value for min/max cvar '%s' can't be higher than '%s'. Values swapped.", mv.name, var.name);
               }
            }
         }
      }
   }

   // special case for xash3d, by default engine is not calling startframe if no players on server, but our quota management and bot adding
   // mechanism assumes that starframe is called even if no players on server, so, set the xash3d's sv_forcesimulating cvar to 1 in case it's not
   if (is (GameFlags::Xash3DLegacy)) {
      static ConVarRef sv_forcesimulating ("sv_forcesimulating");

      if (sv_forcesimulating.exists () && !cr::fequal (sv_forcesimulating.value (), 1.0f)) {
         print ("Force-enable Xash3D sv_forcesimulating cvar.");
         sv_forcesimulating.set ("1.0");
      }
   }
}

void Game::setCvarDescription (const ConVar &cv, StringRef info) {
   for (auto &var : m_cvars) {
      if (var.name == cv.name ()) {
         var.info = info;
         break;
      }
   }
}

void Game::registerCvars (bool gameVars) {
   // this function pushes all added global variables to engine registration

   for (auto &var : m_cvars) {
      ConVar &self = *var.self;
      cvar_t &reg = var.reg;

      if (var.type != Var::GameRef) {
         if (var.type == Var::Xash3D && !is (GameFlags::Xash3D)) {
            continue;
         }
         self.ptr = engfuncs.pfnCVarGetPointer (reg.name);

         if (!self.ptr) {
            static cvar_t reg_ {};

            // fix metamod' memlocs not found
            if (is (GameFlags::Metamod)) {
               reg_ = var.reg;
               engfuncs.pfnCVarRegister (&reg_);
            }
            else {
               engfuncs.pfnCVarRegister (&var.reg);
            }
            self.ptr = engfuncs.pfnCVarGetPointer (reg.name);
         }
      }
      else if (gameVars) {
         self.ptr = engfuncs.pfnCVarGetPointer (reg.name);

         if (var.missing && !self.ptr) {
            if (reg.string == nullptr && !var.regval.empty ()) {
               reg.string = const_cast <char *> (var.regval.chars ());
               reg.flags |= FCVAR_SERVER;
            }
            engfuncs.pfnCVarRegister (&var.reg);
            self.ptr = engfuncs.pfnCVarGetPointer (reg.name);
         }

         if (!self.ptr) {
            logger.error ("Got nullptr on cvar %s!", reg.name);
         }
      }
   }
}

void Game::constructCSBinaryName (StringArray &libs) {
#ifdef _DIII4A //karin: load cs16 server dll
   libs.insert (0, { "libserver_cs" });
#else
   String libSuffix {}; // construct library suffix

   if (plat.android) {
      libSuffix += "_android";
   }
   else if (plat.psvita) {
      libSuffix += "_psvita";
   }

   if (plat.x64) {
      if (plat.arm) {
         libSuffix += "_arm64";
      }
      else if (plat.ppc) {
         libSuffix += "_ppc64le";
      }
      else {
         libSuffix += "_amd64";
      }
   }
   else {
      if (plat.arm) {
         // don't want to put whole build.h logic from xash3d, just set whatever is supported by the YaPB
         if (plat.android) {
            libSuffix += "_armv7l";
         }
         else {
            libSuffix += "_armv7hf";
         }
      }
      else if (!plat.nix && !plat.win && !plat.macos) {
         libSuffix += "_i386";
      }
   }

   if (libSuffix.empty ())
      libs.insert (0, { "mp", "cs", "cs_i386" });
   else {
      // on Android, it's important to have `lib` prefix, otherwise package manager won't unpack the libraries
      if (plat.android)
         libs.insert (0, { "libcs" });
      else
         libs.insert (0, { "mp", "cs" });

      for (auto &lib : libs) {
         lib += libSuffix;
      }
   }
#endif
}

bool Game::loadCSBinary () {
   StringRef modname = getRunningModName ();

   if (modname.empty ()) {
      return false;
   }

   StringArray libs {};
   constructCSBinaryName (libs);

   auto libCheck = [&] (StringRef mod, StringRef dll) {
      // try to load gamedll
      if (!m_gameLib) {
         logger.fatal ("Unable to load gamedll \"%s\". Exiting... (gamedir: %s)", dll, mod);
      }
      auto ent = m_gameLib.resolve <EntityProto> ("trigger_random_unique");

      // detect regamedll by addon entity they provide
      if (ent != nullptr) {
         m_gameFlags |= GameFlags::ReGameDLL;
      }
      return true;
   };

   // search the libraries inside game dlls directory
   for (const auto &lib : libs) {
      String path {};

      if (plat.android) {
         // this will be removed as soon as mod downloader will be implemented on engine side
         auto gamelibdir = plat.env ("XASH3D_GAMELIBDIR");
         path = strings.joinPath (gamelibdir, lib) + kLibrarySuffix;

         // if we can't read file, skip it
         if (!plat.fileExists (path.chars ())) {
            path = "";
         }
      }

      if (plat.emscripten) {
        path = String(plat.env ("XASH3D_GAMELIBPATH")); // defined by launcher
      }

      if (path.empty()) {
         path = strings.joinPath (modname, "dlls", lib) + kLibrarySuffix;

         // if we can't read file, skip it
         if (!plat.fileExists (path.chars ())) {
            continue;
         }
      }

      // special case, czero is always detected first, as it's has custom directory
      if (modname == "czero") {
         m_gameFlags |= (GameFlags::ConditionZero | GameFlags::HasBotVoice | GameFlags::HasFakePings);

         if (is (GameFlags::Metamod)) {
            return false;
         }
         m_gameLib.load (path);

         // verify dll is OK 
         return libCheck (modname, lib);
      }
      else {
         m_gameLib.load (path);

         // verify dll is OK 
         if (!libCheck (modname, lib)) {
            return false;
         }

         // detect if we're running modern game
         auto entity = m_gameLib.resolve <EntityProto> ("weapon_famas");

         // detect legacy xash3d branch
         if (engfuncs.pfnCVarGetPointer ("build") != nullptr) {
            m_gameFlags |= GameFlags::Xash3DLegacy;
         }

         // detect xash engine
         if (engfuncs.pfnCVarGetPointer ("host_ver") != nullptr) {
            m_gameFlags |= (GameFlags::Modern | GameFlags::Xash3D);

            if (entity != nullptr) {
               m_gameFlags |= GameFlags::HasBotVoice;
            }

            if (is (GameFlags::Metamod)) {
               return false;
            }
            return true;
         }

         if (entity != nullptr) {
            m_gameFlags |= (GameFlags::Modern | GameFlags::HasBotVoice | GameFlags::HasFakePings);
         }
         else {
            m_gameFlags |= GameFlags::Legacy;

            // clear modern flag just in case
            m_gameFlags &= ~GameFlags::Modern;
         }

         // allow to enable hitbox-based aiming on fresh games
         if (is (GameFlags::Modern)) {
            m_gameFlags |= GameFlags::HasStudioModels;
         }

         if (is (GameFlags::Metamod)) {
            return false;
         }
         return true;
      }
   }
   return false;
}

bool Game::postload () {

   // register logger
   logger.initialize (bstor.buildPath (BotFile::LogFile), [] (const char *msg) {
      game.print (msg);
   });

   auto ensureBotPathExists = [] (StringRef dir1, StringRef dir2) {
      File::makePath (strings.joinPath (bstor.getRunningPath (), dir1, dir2).chars ());
   };

   // ensure we're have all needed directories
   ensureBotPathExists (folders.config, folders.lang);
   ensureBotPathExists (folders.data, folders.train);
   ensureBotPathExists (folders.data, folders.graph);
   ensureBotPathExists (folders.data, folders.logs);
   ensureBotPathExists (folders.data, folders.podbot);

   // set out user agent for http stuff
   http.setUserAgent (strings.format ("%s/%s", product.name, product.version));

   // set the app name
   plat.setAppName (product.name.chars ());

   // register bot cvars
   registerCvars ();

   // set custom cvar descriptions after registering them
   util.setCustomCvarDescriptions ();

   // handle prefixes
   static StringArray prefixes = { product.cmdPri, product.cmdSec };

   // register all our handlers
   for (const auto &prefix : prefixes) {
      registerEngineCommand (prefix.chars (), [] () {
         ctrl.handleEngineCommands ();
      });
   }

   // register fake metamod command handler if we not! under mm
   if (!(is (GameFlags::Metamod))) {
      game.registerEngineCommand ("meta", [] () {
         game.print ("You're launched standalone version of %s. Metamod is not installed or not enabled!", product.name);
      });
   }

   // is 25th anniversary
   if (is25thAnniversaryUpdate ()) {
      m_gameFlags |= GameFlags::AnniversaryHL25;
   }

   // initialize weapons
   conf.initWeapons ();

   // register engine lib handle
   m_engineLib.locate (reinterpret_cast <void *> (engfuncs.pfnPrecacheModel));

   if (plat.android || plat.emscripten) {
      m_gameFlags |= (GameFlags::Xash3D | GameFlags::Mobility | GameFlags::HasBotVoice | GameFlags::ReGameDLL);

      if (is (GameFlags::Metamod)) {
         return true; // we should stop the attempt for loading the real gamedll, since metamod handle this for us
      }
   }

   const bool binaryLoaded = loadCSBinary ();

   if (!binaryLoaded && !is (GameFlags::Metamod)) {
      logger.fatal ("Mod that you has started, not supported by this bot (gamedir: %s)", getRunningModName ());
   }

   if (is (GameFlags::Metamod)) {
      m_gameLib.unload ();
      return true;
   }

   return false;
}

void Game::applyGameModes () {
   if (!is (GameFlags::Metamod | GameFlags::ReGameDLL)) {
      return;
   }

   // handle cvar cases
   switch (cv_csdm_mode.as <int> ()) {
   default:
   case 0:
      break;

      // force CSDM mode
   case 1:
      m_gameFlags |= GameFlags::CSDM;
      m_gameFlags &= ~GameFlags::FreeForAll;
      return;

      // force CSDM FFA mode
   case 2:
      m_gameFlags |= GameFlags::CSDM | GameFlags::FreeForAll;
      return;

      // force disable everything
   case 3:
      m_gameFlags &= ~(GameFlags::CSDM | GameFlags::FreeForAll);
      return;
   }

   static StringRef csdmActiveCvarName = conf.fetchCustom ("CSDMDetectCvar");
   static StringRef zmActiveCvarName = conf.fetchCustom ("ZMDetectCvar");
   static StringRef zmDelayCvarName = conf.fetchCustom ("ZMDelayCvar");

   static ConVarRef csdm_active (csdmActiveCvarName);
   static ConVarRef csdm_version ("csdm_version");
   static ConVarRef redm_active ("redm_active");
   static ConVarRef mp_freeforall ("mp_freeforall");

   // csdm is only with amxx and metamod
   if (csdm_active.exists () || redm_active.exists () || csdm_version.exists ()) {
      if (csdm_active.value () > 0.0f || redm_active.value () > 0.0f) {
         m_gameFlags |= GameFlags::CSDM;
      }
      else if (is (GameFlags::CSDM)) {
         m_gameFlags &= ~GameFlags::CSDM;
      }
   }

   // but this can be provided by regamedll
   if (mp_freeforall.exists ()) {
      if (mp_freeforall.value () > 0.0f) {
         m_gameFlags |= (GameFlags::FreeForAll | GameFlags::CSDM);
      }
      else if (is (GameFlags::FreeForAll)) {
         m_gameFlags &= ~(GameFlags::FreeForAll | GameFlags::CSDM);
      }
   }

   // does zombie mod is in use
   static ConVarRef zm_active (zmActiveCvarName);

   // do a some little support for zombie plague
   if (zm_active.exists ()) {
      static ConVarRef zm_delay (zmDelayCvarName);

      // update our ignore timer if zp_delay exists
      if (zm_delay.exists () && zm_delay.value () > 0.0f) {
         cv_ignore_enemies_after_spawn_time.set (zm_delay.value () + 3.5f);
      }
      m_gameFlags |= GameFlags::ZombieMod;
   }
   else {
      m_gameFlags &= ~GameFlags::ZombieMod;
   }
}

void Game::slowFrame () {
   const auto nextUpdate = cr::clamp (75.0f * globals->frametime, 0.5f, 1.0f);

   // run something that is should run more
   if (m_halfSecondFrame < time ()) {

      // refresh bomb origin in case some plugin moved it out
      graph.setBombOrigin ();

      // ensure the server admin is confident about features he's using
      ensureHealthyGameEnvironment ();

      // maintain round restart for first human join
      bots.maintainRoundRestart ();

      // update next update time
      m_halfSecondFrame = nextUpdate * 0.25f + time ();
   }

   if (m_oneSecondFrame > time ()) {
      return;
   }
   ctrl.maintainAdminRights ();

   // update bot difficulties to newly selected from cvar
   bots.updateBotDifficulties ();

   // check if we're need to autokill bots
   bots.maintainAutoKill ();

   // maintain leaders selection upon round start
   bots.maintainLeaders ();

   // initialize light levels
   graph.initLightLevels ();

   // initialize corridors
   graph.initNarrowPlaces ();

   // detect csdm
   applyGameModes ();

   // check the cvar bounds
   checkCvarsBounds ();

   // display welcome message
   util.checkWelcome ();

   // kick failed bots
   bots.checkNeedsToBeKicked ();

   // refresh bot infection (creature) status
   bots.refreshCreatureStatus ();

   // update client pings
   fakeping.calculate ();

   // update next update time
   m_oneSecondFrame = nextUpdate + time ();
}

void Game::searchEntities (StringRef field, StringRef value, EntitySearch functor) {
   edict_t *ent = nullptr;

   while (!isNullEntity (ent = engfuncs.pfnFindEntityByString (ent, field.chars (), value.chars ()))) {
      if ((ent->v.flags & EF_NODRAW) || (ent->v.flags & FL_CLIENT)) {
         continue;
      }

      if (functor (ent) == EntitySearchResult::Break) {
         break;
      }
   }
}

void Game::searchEntities (const Vector &position, float radius, EntitySearch functor) const {
   edict_t *ent = nullptr;
   const Vector &pos = position.empty () ? m_startEntity->v.origin : position;

   while (!isNullEntity (ent = engfuncs.pfnFindEntityInSphere (ent, pos, radius))) {
      if ((ent->v.flags & EF_NODRAW) || (ent->v.flags & FL_CLIENT)) {
         continue;
      }

      if (functor (ent) == EntitySearchResult::Break) {
         break;
      }
   }
}

bool Game::hasEntityInGame (StringRef classname) {
   return !isNullEntity (engfuncs.pfnFindEntityByString (nullptr, "classname", classname.chars ()));
}

void Game::printBotVersion () const {
   String gameVersionStr {};
   StringArray botRuntimeFlags {};

   if (is (GameFlags::Legacy)) {
      gameVersionStr.assign ("Legacy");
   }
   else if (is (GameFlags::ConditionZero)) {
      gameVersionStr.assign ("Condition Zero");
   }
   else if (is (GameFlags::Modern)) {
      gameVersionStr.assign ("v1.6");
   }

   if (is (GameFlags::Xash3D)) {
      if (is (GameFlags::Xash3DLegacy)) {
         gameVersionStr.append (" @ Xash3D-NG");
      }
      else {
         gameVersionStr.append (" @ Xash3D FWGS");
      }

      if (is (GameFlags::Mobility)) {
         gameVersionStr.append (" Mobile");
      }
      gameVersionStr.replace ("Legacy", "1.6 Limited");
   }

   if (is (GameFlags::HasBotVoice)) {
      botRuntimeFlags.push ("BotVoice");
   }

   if (is (GameFlags::ReGameDLL)) {
      botRuntimeFlags.push ("ReGameDLL");
   }

   if (is (GameFlags::HasFakePings)) {
      botRuntimeFlags.push ("FakePing");
   }

   if (is (GameFlags::Metamod)) {
      botRuntimeFlags.push ("Metamod");
   }

   if (is (GameFlags::AnniversaryHL25)) {
      botRuntimeFlags.push ("HL25");
   }

   if (botRuntimeFlags.empty ()) {
      botRuntimeFlags.push ("None");
   }

   // print if we're using sse 4.x instructions
   if (plat.simd && (cpuflags.sse41 || cpuflags.sse42 || cpuflags.neon)) {
      Array <String> simdLevels {};

      if (cpuflags.sse41) {
         simdLevels.push ("4.1");
      }
      if (cpuflags.sse42) {
         simdLevels.push ("4.2");
      }
      if (cpuflags.neon) {
         simdLevels.push ("Neon");
      }
      botRuntimeFlags.push (strings.format ("SIMD: %s", String::join (simdLevels, " & ")));
   }
   ctrl.msg ("\n%s v%s successfully loaded for game: Counter-Strike %s.\n\tFlags: %s.\n", product.name, product.version, gameVersionStr, botRuntimeFlags.empty () ? "None" : String::join (botRuntimeFlags, ", "));
}

void Game::ensureHealthyGameEnvironment () {
   const bool dedicated = isDedicated ();

   if (!dedicated || is (GameFlags::Legacy | GameFlags::Xash3D)) {
      if (!dedicated) {

         // force enable pings on listen servers if disabled at all
         if (is (GameFlags::Modern) && cv_show_latency.as <int> () == 0) {
            cv_show_latency.set (2);
         }
      }
      return; // listen servers doesn't care about it at all
   }

   // magic string that's enables the features
   constexpr auto kAllowHash = StringRef::fnv1a32 ("i'm confident for what i'm doing");
   constexpr auto kAllowHash2 = StringRef::fnv1a32 ("\"i'm confident for what i'm doing\"");

   // fetch custom variable, so fake features are explicitly enabled
   static auto enableFakeFeatures = StringRef::fnv1a32 (conf.fetchCustom ("EnableFakeBotFeatures").chars ());

   // if string matches, do not affect the cvars
   if (enableFakeFeatures == kAllowHash || enableFakeFeatures == kAllowHash2) {
      return;
   }

   auto notifyPeacefulRevert = [] (const ConVar &cv) {
      game.print ("Cvar \"%s\" reverted to peaceful value.", cv.name ());
   };

   // disable fake latency
   if (cv_show_latency.as <int> () > 1) {
      cv_show_latency.set (0);

      notifyPeacefulRevert (cv_show_latency);
   }

   // disable fake avatars
   if (cv_show_avatars) {
      cv_show_avatars.set (0);

      notifyPeacefulRevert (cv_show_avatars);
   }

   // disable fake queries 
   if (cv_enable_query_hook) {
      cv_enable_query_hook.set (0);

      notifyPeacefulRevert (cv_enable_query_hook);
   }
}

edict_t *Game::createFakeClient (StringRef name) {
   auto ent = engfuncs.pfnCreateFakeClient (name.chars ());

   if (isNullEntity (ent)) {
      return nullptr;
   }
   auto netname = ent->v.netname;
   ent->v = {}; // reset entire the entvars structure (fix from regamedll)

   // restore containing entity, name and client flags
   ent->v.pContainingEntity = ent;
   ent->v.flags = FL_FAKECLIENT | FL_CLIENT;
   ent->v.netname = netname;

   if (ent->pvPrivateData != nullptr) {
      engfuncs.pfnFreeEntPrivateData (ent);
   }
   ent->pvPrivateData = nullptr;

   return ent;
}

void Game::markBreakableAsInvalid (edict_t *ent) {
   m_checkedBreakables[indexOfEntity (ent)] = false;
}

bool Game::isDeveloperMode () const {
   static ConVarRef developer { "developer" };

   return developer.exists () && developer.value () > 0.0f;
}

void LightMeasure::initializeLightstyles () {
   // this function initializes lighting information...

   // reset all light styles
   for (auto &ls : m_lightstyle) {
      ls.length = 0;
      ls.map[0] = kNullChar;
   }

   for (auto &lsv : m_lightstyleValue) {
      lsv = 264;
   }
}

void LightMeasure::animateLight () {
   // this function performs light animations

   if (!m_doAnimation) {
      return;
   }

   // 'm' is normal light, 'a' is no light, 'z' is double bright
   const auto index = static_cast <int> (game.time () * 10.0f);

   for (auto j = 0; j < MAX_LIGHTSTYLES; ++j) {
      if (!m_lightstyle[j].length) {
         m_lightstyleValue[j] = MAX_LIGHTSTYLEVALUE;
         continue;
      }
      m_lightstyleValue[j] = static_cast <uint32_t> (m_lightstyle[j].map[index % m_lightstyle[j].length] - 'a') * 22u;
   }
}

void LightMeasure::updateLight (int style, char *value) {
   if (!m_doAnimation) {
      return;
   }

   if (style >= MAX_LIGHTSTYLES) {
      return;
   }

   if (strings.isEmpty (value)) {
      m_lightstyle[style].length = 0u;
      m_lightstyle[style].map[0] = kNullChar;

      return;
   }
   const auto copyLimit = sizeof (m_lightstyle[style].map) - sizeof (kNullChar);
   strings.copy (m_lightstyle[style].map, value, copyLimit);

   m_lightstyle[style].map[copyLimit] = kNullChar;
   m_lightstyle[style].length = static_cast <int> (strlen (m_lightstyle[style].map));
}

template <typename S, typename M> bool LightMeasure::recursiveLightPoint (const M *node, const Vector &start, const Vector &end) {
   if (!node || node->contents < 0) {
      return false;
   }

   // determine which side of the node plane our points are on, fixme: optimize for axial
   const auto plane = node->plane;

   const float front = (start | plane->normal) - plane->dist;
   const float back = (end | plane->normal) - plane->dist;

   const int side = front < 0.0f;

   // if they're both on the same side of the plane, don't bother to split just check the appropriate child
   if ((back < 0.0f) == side) {
      return recursiveLightPoint <S, M> (reinterpret_cast <M *> (node->children[side]), start, end);
   }

   // calculate mid point
   const float frac = front / (front - back);
   auto mid = start + (end - start) * frac;

   // go down front side
   if (recursiveLightPoint <S, M> (reinterpret_cast <M *> (node->children[side]), start, mid)) {
      return true; // hit something
   }

   // blow it off if it doesn't split the plane...
   if ((back < 0.0f) == !!side) {
      return false; // didn't hit anything
   }

   // check for impact on this node
   // lightspot = mid;
   // lightplane = plane;
   auto surf = reinterpret_cast <S *> (m_worldModel->surfaces) + node->firstsurface;

   for (int i = 0; i < node->numsurfaces; ++i, ++surf) {
      if (surf->flags & SURF_DRAWTILED) {
         continue; // no lightmaps
      }
      const auto tex = surf->texinfo;

      // see where in lightmap space our intersection point is
      const int s = static_cast <int> ((mid | Vector (tex->vecs[0])) + tex->vecs[0][3]);
      const int t = static_cast <int> ((mid | Vector (tex->vecs[1])) + tex->vecs[1][3]);

      // not in the bounds of our lightmap? punt...
      if (s < surf->texturemins[0] || t < surf->texturemins[1]) {
         continue;
      }

      // assuming a square lightmap (fixme: which ain't always the case), lets see if it lies in that rectangle. if not, punt...
      int ds = s - surf->texturemins[0];
      int dt = t - surf->texturemins[1];

      if (ds > surf->extents[0] || dt > surf->extents[1]) {
         continue;
      }

      if (!surf->samples) {
         return true;
      }
      ds >>= 4;
      dt >>= 4;

      m_point.reset (); // reset point color.

      const int smax = (surf->extents[0] >> 4) + 1;
      const int tmax = (surf->extents[1] >> 4) + 1;
      const int size = smax * tmax;

      auto lightmap = surf->samples + dt * smax + ds;

      // compute the lightmap color at a particular point
      for (int maps = 0; maps < MAX_LIGHTMAPS && surf->styles[maps] != 255; ++maps) {
         const auto scale = static_cast <int32_t> (m_lightstyleValue[surf->styles[maps]]);

         m_point.red += lightmap->r * scale;
         m_point.green += lightmap->g * scale;
         m_point.blue += lightmap->b * scale;

         lightmap += size; // skip to next lightmap
      }
      m_point.red >>= 8u;
      m_point.green >>= 8u;
      m_point.blue >>= 8u;

      return true;
   }
   return recursiveLightPoint <S, M> (reinterpret_cast <M *> (node->children[!side]), mid, end); // go down back side
}

float LightMeasure::getLightLevel (const Vector &point) {
   if (game.is (GameFlags::Legacy)) {
      return kInvalidLightLevel;
   }

   if (!m_worldModel) {
      return kInvalidLightLevel;
   }

   if (!m_worldModel->lightdata) {
      return 255.0f;
   }

   Vector endPoint (point);
   endPoint.z -= 2048.0f;

   static bool isSoftRenderer = game.isSoftwareRenderer ();
   static bool is25Anniversary = game.is25thAnniversaryUpdate ();

   // it's depends if we're are on dedicated or on listenserver
   auto recursiveCheck = [&] () -> bool {
      if (!isSoftRenderer) {
         if (is25Anniversary) {
            return recursiveLightPoint <msurface_hw_hl25_t, mnode_hw_t> (reinterpret_cast <mnode_hw_t *> (m_worldModel->nodes), point, endPoint);
         }
         return recursiveLightPoint <msurface_hw_t, mnode_hw_t> (reinterpret_cast <mnode_hw_t *> (m_worldModel->nodes), point, endPoint);
      }
      return recursiveLightPoint <msurface_t, mnode_t> (m_worldModel->nodes, point, endPoint);
   };

   return !recursiveCheck () ? kInvalidLightLevel : 100 * cr::sqrtf (cr::min (75.0f, static_cast <float> (m_point.avg ())) / 75.0f);
}

float LightMeasure::getSkyColor () {
   return static_cast <float> (Color (sv_skycolor_r.as <int> (), sv_skycolor_g.as <int> (), sv_skycolor_b.as <int> ()).avg ());
}

Vector PlayerHitboxEnumerator::get (edict_t *ent, int part, float updateTimestamp) {
   auto parts = &m_parts[game.indexOfEntity (ent) % kGameMaxPlayers];

   if (game.time () > parts->updated) {
      update (ent);
      parts->updated = game.time () + updateTimestamp;
   }

   switch (part) {
   default:
   case PlayerPart::Head:
      return parts->head;

   case PlayerPart::Stomach:
      return parts->stomach;

   case PlayerPart::LeftArm:
      return parts->left;

   case PlayerPart::RightArm:
      return parts->right;

   case PlayerPart::Feet:
      return parts->feet;

   case PlayerPart::RightLeg:
      return { parts->right.x, parts->right.y, parts->feet.z };

   case PlayerPart::LeftLeg:
      return { parts->left.x, parts->left.y, parts->feet.z };
   }
}

void PlayerHitboxEnumerator::update (edict_t *ent) {
   constexpr auto kInvalidHitbox = -1;

   if (!util.isAlive (ent)) {
      return;
   }
   // get info about player
   auto parts = &m_parts[game.indexOfEntity (ent) % kGameMaxPlayers];

   // set the feet without bones
   parts->feet = ent->v.origin;

   constexpr auto kStandFeet = 34.0f;
   constexpr auto kCrouchFeet = 14.0f;

   // legs position isn't calculated to reduce cpu usage, just use some universal feet spot
   if (ent->v.flags & FL_DUCKING) {
      parts->feet.z = ent->v.origin.z - kCrouchFeet;
   }
   else {
      parts->feet.z = ent->v.origin.z - kStandFeet;
   }

   auto getHitbox = [&] (studiohdr_t *hdr, mstudiobbox_t *bb, int part) {
      int hitbox = kInvalidHitbox;

      for (auto i = 0; i < hdr->numhitboxes; ++i) {
         const auto set = &bb[i];

         if (set->group != part) {
            continue;
         }
         hitbox = i;
         break;
      }
      return hitbox;
   };
   auto model = engfuncs.pfnGetModelPtr (ent);
   auto studiohdr = reinterpret_cast <studiohdr_t *> (model);

   // this can be null ?
   if (model && studiohdr) {
      auto bboxset = reinterpret_cast <mstudiobbox_t *> (reinterpret_cast <uint8_t *> (studiohdr) + studiohdr->hitboxindex);

      // get the head
      auto hitbox = getHitbox (studiohdr, bboxset, PlayerPart::Head);

      if (hitbox != kInvalidHitbox) {
         engfuncs.pfnGetBonePosition (ent, bboxset[hitbox].bone, parts->head, nullptr);

         parts->head.z += bboxset[hitbox].bbmax.z;
         parts->head = { ent->v.origin.x, ent->v.origin.y, parts->head.z };
      }

      // get the body (stomach)
      hitbox = getHitbox (studiohdr, bboxset, PlayerPart::Stomach);

      if (hitbox != kInvalidHitbox) {
         engfuncs.pfnGetBonePosition (ent, bboxset[hitbox].bone, parts->stomach, nullptr);
      }

      // get the left (arm)
      hitbox = getHitbox (studiohdr, bboxset, PlayerPart::LeftArm);

      if (hitbox != kInvalidHitbox) {
         engfuncs.pfnGetBonePosition (ent, bboxset[hitbox].bone, parts->left, nullptr);
      }

      // get the right (arm)
      hitbox = getHitbox (studiohdr, bboxset, PlayerPart::RightArm);

      if (hitbox != kInvalidHitbox) {
         engfuncs.pfnGetBonePosition (ent, bboxset[hitbox].bone, parts->right, nullptr);
      }
      return;
   }
   else {
      game.clearGameFlag (GameFlags::HasStudioModels); // yes, only a single fail will disable this
   }

   parts->head = ent->v.origin + ent->v.view_ofs;
   parts->stomach = ent->v.origin;

   parts->left = parts->head;
   parts->right = parts->head;
}

void PlayerHitboxEnumerator::reset () {
   for (auto &part : m_parts) {
      part = {};
   }
}
