//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

constexpr int kMaxNodes = 4096; // max nodes per graph
constexpr int kMaxNodeLinks = 8; // max links for single node

// defines for nodes flags field (32 bits are available)
CR_DECLARE_SCOPED_ENUM (NodeFlag,
   Button = cr::bit (0), // use a nearby button (lifts, doors, etc.)
   Lift = cr::bit (1), // wait for lift to be down before approaching this node
   Crouch = cr::bit (2), // must crouch to reach this node
   Crossing = cr::bit (3), // a target node
   Goal = cr::bit (4), // mission goal point (bomb, hostage etc.)
   Ladder = cr::bit (5), // node is on ladder
   Rescue = cr::bit (6), // node is a hostage rescue point
   Camp = cr::bit (7), // node is a camping point
   NoHostage = cr::bit (8), // only use this node if no hostage
   DoubleJump = cr::bit (9), // bot help's another bot (requster) to get somewhere (using djump)
   Narrow = cr::bit (10), // node is inside some small space (corridor or such)
   Sniper = cr::bit (28), // it's a specific sniper point
   TerroristOnly = cr::bit (29), // it's a specific terrorist point
   CTOnly = cr::bit (30),  // it's a specific ct point
)

// defines for node connection flags field (16 bits are available)
CR_DECLARE_SCOPED_ENUM_TYPE (PathFlag, uint16_t,
   Jump = cr::bit (0) // must jump for this connection
)

// enum pathfind search type
CR_DECLARE_SCOPED_ENUM (FindPath,
   Fast = 0,
   Optimal,
   Safe
)

// defines node connection types
CR_DECLARE_SCOPED_ENUM (PathConnection,
   Outgoing = 0,
   Incoming,
   Bidirectional,
   Jumping
)

// node edit states
CR_DECLARE_SCOPED_ENUM (GraphEdit,
   On = cr::bit (1),
   Noclip = cr::bit (2),
   Auto = cr::bit (3)
)

// lift usage states
CR_DECLARE_SCOPED_ENUM (LiftState,
   None = 0,
   LookingButtonOutside,
   WaitingFor,
   EnteringIn,
   WaitingForTeammates,
   LookingButtonInside,
   TravelingBy,
   Leaving
)

// node add flags
CR_DECLARE_SCOPED_ENUM (NodeAddFlag,
   Normal = 0,
   TOnly = 1,
   CTOnly = 2,
   NoHostage = 3,
   Rescue = 4,
   Camp = 5,
   CampEnd = 6,
   JumpStart = 9,
   JumpEnd = 10,
   Goal = 100
)

CR_DECLARE_SCOPED_ENUM (NotifySound,
   Done = 0,
   Change = 1,
   Added = 2
)

#include <vistable.h>

// general waypoint header information structure for podbot
struct PODGraphHeader {
   char header[8] {};
   int32_t fileVersion {};
   int32_t pointNumber {};
   char mapName[32] {};
   char author[32] {};
};

// defines linked nodes
struct PathLink {
   Vector velocity {};
   int32_t distance {};
   uint16_t flags {};
   int16_t index {};
};

// define graph path structure for yapb
struct Path {
   int32_t number {}, flags {};
   Vector origin {}, start {}, end {};
   float radius {}, light {}, display {};
   PathLink links[kMaxNodeLinks] {};
   PathVis vis {};
};

// define waypoint structure for podbot (will convert on load)
struct PODPath {
   int32_t number {}, flags {};
   Vector origin {};
   float radius {}, csx {}, csy {}, cex {}, cey {};
   int16_t index[kMaxNodeLinks] {};
   uint16_t conflags[kMaxNodeLinks] {};
   Vector velocity[kMaxNodeLinks] {};
   int32_t distance[kMaxNodeLinks] {};
   PathVis vis {};
};

// general storage header information structure
struct StorageHeader {
   int32_t magic {};
   int32_t version {};
   int32_t options {};
   int32_t length {};
   int32_t compressed {};
   int32_t uncompressed {};
};

// extension header for graph information
struct ExtenHeader {
   char author[32] {}; // original author of graph
   int32_t mapSize {}; // bsp size for checksumming map consistency
   char modified[32] {}; // by whom modified
};

// graph operation class
class BotGraph final : public Singleton <BotGraph> {
public:
   friend class Bot;

private:
   int m_editFlags {};
   int m_cacheNodeIndex {};
   int m_lastJumpNode {};
   int m_findWPIndex {};
   int m_facingAtIndex {};
   int m_autoSaveCount {};

   float m_timeJumpStarted {};
   float m_autoPathDistance {};
   float m_pathDisplayTime {};
   float m_arrowDisplayTime {};

   bool m_isOnLadder {};
   bool m_endJumpPoint {};
   bool m_jumpLearnNode {};
   bool m_hasChanged {};
   bool m_narrowChecked {};
   bool m_silenceMessages {};
   bool m_lightChecked {};
   bool m_isOnlineCollected {};

   Vector m_learnVelocity {};
   Vector m_learnPosition {};
   Vector m_lastNode {};

   IntArray m_terrorPoints {};
   IntArray m_ctPoints {};
   IntArray m_goalPoints {};
   IntArray m_campPoints {};
   IntArray m_sniperPoints {};
   IntArray m_rescuePoints {};
   IntArray m_visitedGoals {};
   IntArray m_nodeNumbers {};

public:
   SmallArray <Path> m_paths {};
   HashMap <int32_t, Array <int32_t>, EmptyHash <int32_t>> m_hashTable {};

   struct GraphInfo {
      String author {};
      String modified {};
      ExtenHeader exten {};
      StorageHeader header {};
   } m_info {};

   edict_t *m_editor {};

public:
   BotGraph ();
   ~BotGraph () = default;

public:
   int getFacingIndex ();
   int getFarest (const Vector &origin, const float maxRange = 32.0);
   int getForAnalyzer (const Vector &origin, const float maxRange);
   int getNearest (const Vector &origin, const float range = kInfiniteDistance, int flags = -1);
   int getNearestNoBuckets (const Vector &origin, const float range = kInfiniteDistance, int flags = -1);
   int getEditorNearest (const float maxRange = 50.0f);
   int clearConnections (int index);
   int getBspSize ();
   int locateBucket (const Vector &pos);

   float calculateTravelTime (float maxSpeed, const Vector &src, const Vector &origin);

   bool convertOldFormat ();
   bool isConnected (int a, int b);
   bool isConnected (int index);
   bool isNodeReacheableEx (const Vector &src, const Vector &destination, const float maxHeight) const;
   bool isNodeReacheable (const Vector &src, const Vector &destination) const;
   bool isNodeReacheableWithJump (const Vector &src, const Vector &destination) const;
   bool checkNodes (bool teleportPlayer, bool onlyPaths = false);
   bool isVisited (int index);

   bool saveGraphData ();
   bool loadGraphData ();
   bool canDownload ();
   bool isAnalyzed () const;
   bool isConverted () const;

   void saveOldFormat ();
   void reset ();
   void frame ();
   void populateNodes ();
   void syncInitLightLevels ();
   void initLightLevels ();
   void initNarrowPlaces ();
   void addPath (int addIndex, int pathIndex, float distance);
   void add (int type, const Vector &pos = nullptr);
   void erase (int target);
   void toggleFlags (int toggleFlag);
   void setRadius (int index, float radius);
   void pathCreate (char dir);
   void erasePath ();
   void resetPath (int index);
   void cachePoint (int index);
   void calculatePathRadius (int index);
   void addBasic ();
   void setSearchIndex (int index);
   void startLearnJump ();
   void setVisited (int index);
   void clearVisited ();

   void eraseFromBucket (const Vector &pos, int index);
   void unassignPath (int from, int to);
   void convertFromPOD (Path &path, const PODPath &pod) const;
   void convertToPOD (const Path &path, PODPath &pod);
   void convertCampDirection (Path &path) const;
   void setAutoPathDistance (const float distance);
   void showStats ();
   void showFileInfo ();
   void emitNotify (int32_t sound) const;
   void syncCollectOnline ();
   void collectOnline ();

   IntArray getNearestInRadius (const float radius, const Vector &origin, int maxCount = -1);

public:
   StringRef getAuthor () const {
      return m_info.author;
   }

   StringRef getModifiedBy () const {
      return m_info.modified;
   }

   bool hasChanged () const {
      return m_hasChanged;
   }

   bool hasEditFlag (int flag) const {
      return !!(m_editFlags & flag);
   }

   void setEditFlag (int flag) {
      m_editFlags |= flag;
   }

   void clearEditFlag (int flag) {
      m_editFlags &= ~flag;
   }

   // access paths
   Path &operator [] (int index) {
      return m_paths[index];
   }

   // check nodes range
   template <typename U> bool exists (U index) const {
      return index >= 0 && index < static_cast <U> (length ());
   }

   // get real nodes num
   int32_t length () const {
      return m_paths.length <int32_t> ();
   }

   // get the random node on map
   int32_t random () const {
      return rg (0, length () - 1);
   }

   // check if has editor
   bool hasEditor () const {
      return !!m_editor;
   }

   // set's the node editor
   void setEditor (edict_t *ent) {
      m_editor = ent;
   }

   // get the current node editor
   edict_t *getEditor () {
      return m_editor;
   }

   // silence all graph messages or not
   void setMessageSilence (bool enable) {
      m_silenceMessages = enable;
   }

   // set exten header from binary storage
   void setExtenHeader (ExtenHeader *hdr) {
      memcpy (&m_info.exten, hdr, sizeof (ExtenHeader));
   }

   // set graph header from binary storage
   void setGraphHeader (StorageHeader *hdr) {
      memcpy (&m_info.header, hdr, sizeof (StorageHeader));
   }

   // gets the node numbers
   const IntArray &getNodeNumbers () {
      return m_nodeNumbers;
   }

   // reinitialize buckets
   void initBuckets () {
      m_hashTable.clear ();
   }

   // get the bucket of nodes near position
   const IntArray &getNodesInBucket (const Vector &pos) {
      return m_hashTable[locateBucket (pos)];
   }

   // add a node to position bucket
   void addToBucket (const Vector &pos, int index) {
      m_hashTable[locateBucket (pos)].emplace (index);
   }

public:
   // graph helper for sending message to correct channel
   template <typename ...Args> void msg (const char *fmt, Args &&...args);

public:
   Path *begin () {
      return m_paths.begin ();
   }

   Path *begin () const {
      return m_paths.begin ();
   }

   Path *end () {
      return m_paths.end ();
   }

   Path *end () const {
      return m_paths.end ();
   }
};

#include <manager.h>
#include <practice.h>

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (BotGraph, graph);
