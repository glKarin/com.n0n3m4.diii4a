//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// line draw
CR_DECLARE_SCOPED_ENUM (DrawLine,
   Simple,
   Arrow,
   Count
)

// trace ignore
CR_DECLARE_SCOPED_ENUM (TraceIgnore,
   None = 0,
   Glass = cr::bit (0),
   Monsters = cr::bit (1),
   Everything = Glass | Monsters
)

// variable type
CR_DECLARE_SCOPED_ENUM (Var,
   Normal = 0,
   ReadOnly,
   Password,
   NoServer,
   GameRef,
   Xash3D // registrable only on xash3d engine
)

// supported cs's
CR_DECLARE_SCOPED_ENUM (GameFlags,
   Modern = cr::bit (0), // counter-strike 1.6 and above
   Xash3D = cr::bit (1), // counter-strike 1.6 under the xash engine (additional flag)
   ConditionZero = cr::bit (2), // counter-strike: condition zero
   Legacy = cr::bit (3), // counter-strike 1.3-1.5 with/without steam
   Mobility = cr::bit (4), // additional flag that bot is running on android (additional flag)
   Unused = cr::bit (5), // not used currently
   Metamod = cr::bit (6), // game running under meta\mod
   CSDM = cr::bit (7), // csdm mod currently in use
   FreeForAll = cr::bit (8), // csdm mod with ffa mode
   ReGameDLL = cr::bit (9), // server dll is a regamedll
   HasFakePings = cr::bit (10), // on that game version we can fake bots pings
   HasBotVoice = cr::bit (11), // on that game version we can use chatter
   AnniversaryHL25 = cr::bit (12), // half-life 25th anniversary engine
   Xash3DLegacy = cr::bit (13), // old xash3d-branch
   ZombieMod = cr::bit (14), // zombie mod is active
   HasStudioModels = cr::bit (15) // game supports studio models, so we can use hitbox-based aiming
)

// defines map type
CR_DECLARE_SCOPED_ENUM (MapFlags,
   Assassination = cr::bit (0),
   HostageRescue = cr::bit (1),
   Demolition = cr::bit (2),
   Escape = cr::bit (3),
   KnifeArena = cr::bit (4),
   FightYard = cr::bit (5),
   GrenadeWar = cr::bit(6),
   HasDoors = cr::bit (10), // additional flags
   HasButtons = cr::bit (11) // map has buttons
)

// recursive entity search
CR_DECLARE_SCOPED_ENUM (EntitySearchResult,
   Continue,
   Break
)

// player body parts
CR_DECLARE_SCOPED_ENUM (PlayerPart,
   Head = 1,
   Chest,
   Stomach,
   LeftArm,
   RightArm,
   LeftLeg,
   RightLeg,
   Feet // custom!
)

// variable reg pair
struct ConVarReg {
   cvar_t reg {};
   String info {};
   String init {};
   String regval {};
   String name {};
   class ConVar *self {};
   float initial {}, min {}, max {};
   bool missing {};
   bool bounded {};
   int32_t type {};
};

// entity prototype
using EntityProto = void (*) (entvars_t *);

// rehlds has this fixed, but original hlds doesn't allocate string space  passed to precache* argument, so game will crash when unloading module using metamod
class EngineWrap final {
public:
   EngineWrap () = default;
   ~EngineWrap () = default;

private:
   const char *allocStr (const char *str) const {
      return string_t::from (engfuncs.pfnAllocString (str));
   }

public:
   int32_t precacheModel (const char *model) const {
      return engfuncs.pfnPrecacheModel (allocStr (model));
   }

   int32_t precacheSound (const char *sound) const {
      return engfuncs.pfnPrecacheSound (allocStr (sound));
   }

   void setModel (edict_t *ent, const char *model) {
      engfuncs.pfnSetModel (ent, allocStr (model));
   }
};

// player model part info enumerator
class PlayerHitboxEnumerator final {
public:
   struct Info {
      float updated {};
      Vector head {};
      Vector stomach {};
      Vector feet {};
      Vector right {};
      Vector left {};
   } m_parts[kGameMaxPlayers] {};

public:
   // get's the enemy part based on bone info
   Vector get (edict_t *ent, int part, float updateTimestamp);

   // update bones positions for given player
   void update (edict_t *ent);

   // reset all the poisitons
   void reset ();
};

// provides utility functions to not call original engine (less call-cost)
class Game final : public Singleton <Game> {
public:
   using EntitySearch = const Lambda <EntitySearchResult (edict_t *)> &;

private:
   int m_drawModels[DrawLine::Count] {};
   int m_spawnCount[Team::Unassigned] {};

   // bot client command
   StringArray m_botArgs {};

   edict_t *m_startEntity {};
   edict_t *m_localEntity {};

   Array <edict_t *> m_breakables {};
   HashMap <int32_t, bool> m_checkedBreakables {};

   SmallArray <ConVarReg> m_cvars {};
   SharedLibrary m_gameLib {};
   SharedLibrary m_engineLib {};
   EngineWrap m_engineWrap {};

   bool m_precached {};

   int m_gameFlags {};
   int m_mapFlags {};

   float m_oneSecondFrame {}; // per second updated
   float m_halfSecondFrame {}; // per half second update

public:
   Game ();
   ~Game () = default;

public:
   // preaches internal stuff
   void precache ();

   // initialize levels
   void levelInitialize (edict_t *entities, int max);

   // shutdown levels
   void levelShutdown ();

   // display world line
   void drawLine (edict_t *ent, const Vector &start, const Vector &end, int width, int noise, const Color &color, int brightness, int speed, int life, DrawLine type = DrawLine::Simple) const;

   // test line
   void testLine (const Vector &start, const Vector &end, int ignoreFlags, edict_t *ignoreEntity, TraceResult *ptr);

   // test model
   void testModel (const Vector &start, const Vector &end, int hullNumber, edict_t *entToHit, TraceResult *ptr);

   // test line
   void testHull (const Vector &start, const Vector &end, int ignoreFlags, int hullNumber, edict_t *ignoreEntity, TraceResult *ptr);

   // we are on dedicated server ?
   bool isDedicated ();

   // get stripped down mod name
   const char *getRunningModName ();

   // get the valid mapname
   const char *getMapName ();

   // get the "any" entity origin
   Vector getEntityOrigin (edict_t *ent);

   // registers a server command
   void registerEngineCommand (const char *command, void func ());

   // play's sound to client
   void playSound (edict_t *ent, const char *sound);

   // sends bot command
   void prepareBotArgs (edict_t *ent, String str);

   // adds cvar to registration stack
   void pushConVar (StringRef name, StringRef value, StringRef info, bool bounded, float min, float max, int32_t varType, bool missingAction, StringRef regval, class ConVar *self);

   // check the cvar bounds
   void checkCvarsBounds ();

   // modify cvar description
   void setCvarDescription (const ConVar &cv, StringRef info);

   // sends local registration stack for engine registration
   void registerCvars (bool gameVars = false);

   // checks whether software rendering is enabled
   bool isSoftwareRenderer ();

   // checks if this is 25th anniversary half-life update
   bool is25thAnniversaryUpdate ();

   // load the cs binary in non metamod mode
   bool loadCSBinary ();

   void constructCSBinaryName (StringArray &libs);

   // do post-load stuff
   bool postload ();

   // detects if csdm mod is in use
   void applyGameModes ();

   // executes stuff every 1 second
   void slowFrame ();

   // search entities by variable field
   void searchEntities (StringRef field, StringRef value, EntitySearch functor);

   // search entities in sphere
   void searchEntities (const Vector &position, float radius, EntitySearch functor) const;

   // check if map has entity
   bool hasEntityInGame (StringRef classname);

   // print the version to server console on startup
   void printBotVersion () const;

   // ensure prosperous gaming environment as per: https://github.com/yapb/yapb/issues/575
   void ensureHealthyGameEnvironment ();

   // creates a fake client's a nd resets all the entvars
   edict_t *createFakeClient (StringRef name);

   // mark breakable entity as invalid
   void markBreakableAsInvalid (edict_t *ent);

   // is developer mode ?
   bool isDeveloperMode () const;

   // public inlines
public:
   // get the current time on server
   float time () const {
      return globals->time;
   }

   // get "maxplayers" limit on server
   int maxClients () const {
      return globals->maxClients;
   }

   // get the fakeclient command interface
   bool isBotCmd () const {
      return !m_botArgs.empty ();
   }

   // gets custom engine args for client command
   const char *botArgs () const {
      return strings.format (String::join (m_botArgs, " ", m_botArgs[0].startsWith ("say") ? 1 : 0).chars ());
   }

   // gets custom engine argv for client command
   const char *botArgv (int32_t index) const {
      if (static_cast <size_t> (index) >= m_botArgs.length ()) {
         return "";
      }
      return m_botArgs[index].chars ();
   }

   // gets custom engine argc for client command
   int32_t botArgc () const {
      return m_botArgs.length <int32_t> ();
   }

   // gets edict pointer out of entity index
   CR_FORCE_INLINE edict_t *entityOfIndex (const int index) const {
      return static_cast <edict_t *> (m_startEntity + index);
   };

   // gets edict pointer out of entity index (player)
   CR_FORCE_INLINE edict_t *playerOfIndex (const int index) const {
      return entityOfIndex (index) + 1;
   };

   // gets edict index out of it's pointer
   CR_FORCE_INLINE int indexOfEntity (const edict_t *ent) const {
      return static_cast <int> (ent - m_startEntity);
   };

   // gets edict index of it's pointer (player)
   CR_FORCE_INLINE int indexOfPlayer (const edict_t *ent) const {
      return indexOfEntity (ent) - 1;
   }

   // verify entity isn't null
   CR_FORCE_INLINE bool isNullEntity (const edict_t *ent) const {
      return !ent || !indexOfEntity (ent) || ent->free;
   }

   // get the wroldspawn entity
   edict_t *getStartEntity () {
      return m_startEntity;
   }

   // get spawn count for team
   int getSpawnCount (int team) const {
      return m_spawnCount[team];
   }

   // gets the player team
   int getTeam (edict_t *ent) const {
      if (isNullEntity (ent)) {
         return Team::Unassigned;
      }
      return util.getClient (indexOfPlayer (ent)).team;
   }

   // gets the player team (real in ffa)
   int getRealTeam (edict_t *ent) const {
      if (isNullEntity (ent)) {
         return Team::Unassigned;
      }
      return util.getClient (indexOfPlayer (ent)).team2;
   }

   // get real gamedll team (matches gamedll indices)
   int getGameTeam (edict_t *ent) const {
      return getRealTeam (ent) + 1;
   }

   // sets the precache to uninitialized
   void setUnprecached () {
      m_precached = false;
   }

   // gets the local entity (host edict)
   edict_t *getLocalEntity () {
      return m_localEntity;
   }

   // sets the local entity (host edict)
   void setLocalEntity (edict_t *ent) {
      m_localEntity = ent;
   }

   // sets player start entity draw models
   void setPlayerStartDrawModels ();

   // check the engine visibility wrapper
   bool checkVisibility (edict_t *ent, uint8_t *set);

   // get pvs/pas visibility set
   uint8_t *getVisibilitySet (Bot *bot, bool pvs) const;

   // what kind of game engine / game dll / mod / tool we're running ?
   bool is (const int type) const {
      return !!(m_gameFlags & type);
   }

   // adds game flag
   void addGameFlag (const int type) {
      m_gameFlags |= type;
   }

   // clears game flag
   void clearGameFlag (const int type) {
      m_gameFlags &= ~type;
   }

   // gets the map type
   bool mapIs (const int type) const {
      return !!(m_mapFlags & type);
   }

   // get loaded gamelib
   const SharedLibrary &lib () {
      return m_gameLib;
   }

   // get loaded engine lib
   const SharedLibrary &elib () {
      return m_engineLib;
   }

   // get registered cvars list
   const SmallArray <ConVarReg> &getCvars () {
      return m_cvars;
   }

   // check if map has breakables
   const Array <edict_t *> &getBreakables () {
      return m_breakables;
   }

   // map has breakables ?
   bool hasBreakables () const {
      return !m_breakables.empty ();
   }

   // is breakable entity is valid ?
   bool isBreakableValid (edict_t *ent) {
      return m_checkedBreakables[indexOfEntity (ent)];
   }

   // find variable value by variable name
   StringRef findCvar (StringRef name) {
      return engfuncs.pfnCVarGetString (name.chars ());
   }

   // helper to sending the client message
   void sendClientMessage (bool console, edict_t *ent, StringRef message);
   
   // helper to sending the server message
   void sendServerMessage (StringRef message);

   // helper for sending hud messages to client
   void sendHudMessage (edict_t *ent, const hudtextparms_t &htp, StringRef message);

   // send server command
   template <typename ...Args> void serverCommand (const char *fmt, Args &&...args) {
      engfuncs.pfnServerCommand (strings.concat (strings.format (fmt, cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }

   // send a bot command
   template <typename ...Args> void botCommand (edict_t *ent, const char *fmt, Args &&...args) {
      prepareBotArgs (ent, strings.format (fmt, cr::forward <Args> (args)...));
   }

   // prints data to servers console
   template <typename ...Args> void print (const char *fmt, Args &&...args) {
      sendServerMessage (strings.concat (strings.format (conf.translate (fmt), cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }

   // prints center message to specified player
   template <typename ...Args> void clientPrint (edict_t *ent, const char *fmt, Args &&...args) {
      if (isNullEntity (ent)) {
         print (fmt, cr::forward <Args> (args)...);
         return;
      }
      sendClientMessage (true, ent, strings.concat (strings.format (conf.translate (fmt), cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }

   // prints message to client console
   template <typename ...Args> void centerPrint (edict_t *ent, const char *fmt, Args &&...args) {
      if (isNullEntity (ent)) {
         print (fmt, cr::forward <Args> (args)...);
         return;
      }
      sendClientMessage (false, ent, strings.concat (strings.format (conf.translate (fmt), cr::forward <Args> (args)...), "\n", StringBuffer::StaticBufferSize));
   }
};

// reference some game/mod cvars for access
class ConVarRef final : public NonCopyable {
private:
   cvar_t *ptr_ {};
   String name_ {};
   bool checked_ {};

public:
   ConVarRef (StringRef name) : name_ (name) {}
   ~ConVarRef () = default;

public:
   bool exists () {
      if (checked_ && !ptr_) {
         return false;
      }
      checked_ = true;
      ptr_ = engfuncs.pfnCVarGetPointer (name_.chars ());

      return ptr_ != nullptr;
   }

   template <typename U = float> U value () {
      return exists () ? static_cast <U> (ptr_->value) : static_cast <U> (0);
   }

   void set (StringRef value) {
      if (exists ()) {
         engfuncs.pfnCvar_DirectSet (ptr_, value.chars ());
      }
   }
};

// simplify access for console variables
class ConVar final : public NonCopyable {
public:
   cvar_t *ptr;

private:
   String name_ {};

public:
   ConVar () = delete;
   ~ConVar () = default;

public:
   ConVar (StringRef name, StringRef initval, int32_t type = Var::NoServer, bool regMissing = false, StringRef regVal = nullptr) : ptr (nullptr) {
      setPrefix (name, type);
      Game::instance ().pushConVar (name_.chars (), initval, "", false, 0.0f, 0.0f, type, regMissing, regVal, this);
   }

   ConVar (StringRef name, StringRef initval, StringRef info, bool bounded = true, float min = 0.0f, float max = 1.0f, int32_t type = Var::NoServer, bool regMissing = false, const char *regVal = nullptr) : ptr (nullptr) {
      setPrefix (name, type);
      Game::instance ().pushConVar (name_.chars (), initval, info, bounded, min, max, type, regMissing, regVal, this);
   }

public:
   template <typename U> constexpr U as () const {
      if constexpr (cr::is_same <U, float>::value) {
         return ptr->value;
      }
      else if constexpr (cr::is_same <U, bool>::value) {
         return ptr->value > 0.0f;
      }
      else if constexpr (cr::is_same <U, int>::value) {
         return static_cast <U> (ptr->value);
      }
      else if constexpr (cr::is_same <U, StringRef>::value) {
         return ptr->string;
      }
   }

public:
   operator bool () const {
      return as <bool> ();
   }

   operator float () const {
      return as <float> ();
   }

   operator int () const {
      return as <int> ();
   }

   operator StringRef () {
      return as <StringRef> ();
   }

public:
   StringRef name () const {
      return ptr->name;
   }

   void set (float val) {
      engfuncs.pfnCVarSetFloat (ptr->name, val);
   }

   void set (int val) {
      set (static_cast <float> (val));
   }

   void set (const char *val) {
      engfuncs.pfnCvar_DirectSet (ptr, val);
   }

   // revet cvar to default value
   void revert ();

   // set the cvar prefix if needed
   void setPrefix (StringRef name, int32_t type);
};

class MessageWriter final {
private:
   bool m_autoDestruct { false };

public:
   MessageWriter () = default;

   MessageWriter (int dest, int type, const Vector &pos = nullptr, edict_t *to = nullptr) {
      start (dest, type, pos, to);
      m_autoDestruct = true;
   }

   ~MessageWriter () {
      if (m_autoDestruct) {
         end ();
      }
   }

public:
   MessageWriter &start (int dest, int type, const Vector &pos = nullptr, edict_t *to = nullptr) {
      engfuncs.pfnMessageBegin (dest, type, pos, to);
      return *this;
   }

   void end () {
      engfuncs.pfnMessageEnd ();
   }

   MessageWriter &writeByte (int val) {
      engfuncs.pfnWriteByte (val);
      return *this;
   }

   MessageWriter &writeLong (int val) {
      engfuncs.pfnWriteLong (val);
      return *this;
   }

   MessageWriter &writeChar (int val) {
      engfuncs.pfnWriteChar (val);
      return *this;
   }

   MessageWriter &writeShort (int val) {
      engfuncs.pfnWriteShort (val);
      return *this;
   }

   MessageWriter &writeCoord (float val) {
      engfuncs.pfnWriteCoord (val);
      return *this;
   }

   MessageWriter &writeString (const char *val) {
      engfuncs.pfnWriteString (val);
      return *this;
   }

public:
   static constexpr uint16_t fu16 (float value, float scale) {
      return cr::clamp <uint16_t> (static_cast <uint16_t> (value * cr::bit (static_cast <short> (scale))), 0, USHRT_MAX);
   }

   static constexpr short fs16 (float value, float scale) {
      return cr::clamp <short> (static_cast <short> (value * cr::bit (static_cast <short> (scale))), -SHRT_MAX, SHRT_MAX);
   }
};

class LightMeasure final : public Singleton <LightMeasure> {
private:
   lightstyle_t m_lightstyle[MAX_LIGHTSTYLES] {};
   uint32_t m_lightstyleValue[MAX_LIGHTSTYLEVALUE] {};
   bool m_doAnimation = false;

   Color m_point;
   model_t *m_worldModel = nullptr;

public:
   LightMeasure () {
      initializeLightstyles ();
      m_point.reset ();
   }

public:
   void initializeLightstyles ();
   void animateLight ();
   void updateLight (int style, char *value);

   float getLightLevel (const Vector &point);
   float getSkyColor ();

private:
   template <typename S, typename M> bool recursiveLightPoint (const M *node, const Vector &start, const Vector &end);

public:
   void resetWorldModel () {
      m_worldModel = nullptr;
   }

   void setWorldModel (model_t *model) {
      if (m_worldModel) {
         return;
      }
      m_worldModel = model;
   }

   model_t *getWorldModel () const {
      return m_worldModel;
   }

   void enableAnimation (bool enable) {
      m_doAnimation = enable;
   }
};

// expose globals
CR_EXPOSE_GLOBAL_SINGLETON (Game, game);
CR_EXPOSE_GLOBAL_SINGLETON (LightMeasure, illum);
