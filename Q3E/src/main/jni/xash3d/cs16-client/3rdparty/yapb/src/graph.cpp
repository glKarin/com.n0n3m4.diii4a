//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_graph_fixcamp ("graph_fixcamp", "0", "Specifies whether bot should not 'fix' camp directions of camp waypoints when loading old PWF format.");
ConVar cv_graph_url ("graph_url", product.download.chars (), "Specifies the URL from which bots will be able to download graph in case of missing local one. Set to empty, if no downloads needed.", false, 0.0f, 0.0f);
ConVar cv_graph_url_upload ("graph_url_upload", product.upload.chars (), "Specifies the URL to which bots will try to upload the graph file to database.", false, 0.0f, 0.0f);
ConVar cv_graph_auto_save_count ("graph_auto_save_count", "15", "Every N graph nodes placed on map, the graph will be saved automatically (without checks).", true, 0.0f, kMaxNodes);
ConVar cv_graph_draw_distance ("graph_draw_distance", "400", "Maximum distance to draw graph nodes from editor viewport.", true, 64.0f, 3072.0f);
ConVar cv_graph_auto_collect_db ("graph_auto_collect_db", "1", "Allows bots to exchange your graph files with graph database automatically.");

void BotGraph::reset () {
   // this function initialize the graph structures..

   m_editFlags = 0;
   m_autoSaveCount = 0;

   m_learnVelocity.clear ();
   m_learnPosition.clear ();
   m_lastNode.clear ();

   m_pathDisplayTime = 0.0f;
   m_arrowDisplayTime = 0.0f;
   m_autoPathDistance = 250.0f;
   m_hasChanged = false;
   m_narrowChecked = false;
   m_lightChecked = false;

   m_info.author.clear ();
   m_info.modified.clear ();

   m_paths.clear ();
}

int BotGraph::clearConnections (int index) {
   // this function removes the useless paths connections from and to node pointed by index. This is based on code from POD-bot MM from KWo

   if (!exists (index)) {
      return 0;
   }
   int numFixedLinks = 0;

   // wrapper form unassiged paths
   auto clearPath = [&] (int from, int to) {
      unassignPath (from, to);
      ++numFixedLinks;
   };

   if (bots.hasBotsOnline ()) {
      bots.kickEveryone (true);
   }

   struct Connection {
      int index {};
      int number {};
      float distance {};
      float angles {};

   public:
      Connection () {
         reset ();
      }

   public:
      void reset () {
         index = kInvalidNodeIndex;
         number = kInvalidNodeIndex;
         distance = kInfiniteDistance;
         angles = 0.0f;
      }
   };
   auto &path = m_paths[index];

   Connection sorted[kMaxNodeLinks] {};
   Connection top {};

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      auto &cur = sorted[i];
      const auto &link = path.links[i];

      cur.number = i;
      cur.index = link.index;
      cur.distance = static_cast <float> (link.distance);

      if (cur.index == kInvalidNodeIndex) {
         cur.distance = kInfiniteDistance;
      }

      if (cur.distance < top.distance) {
         top.distance = static_cast <float> (link.distance);
         top.number = i;
         top.index = cur.index;
      }
   }

   if (top.number == kInvalidNodeIndex) {
      msg ("Cannot find path to the closest connected node to node number %d.", index);
      return numFixedLinks;
   }
   bool sorting = false;

   // sort paths from the closest node to the farest away one...
   do {
      sorting = false;

      for (int i = 0; i < kMaxNodeLinks - 1; ++i) {
         if (sorted[i].distance > sorted[i + 1].distance) {
            cr::swap (sorted[i], sorted[i + 1]);
            sorting = true;
         }
      }
   } while (sorting);

   // calculate angles related to the angle of the closeset connected node
   for (auto &cur : sorted) {
      if (cur.index == kInvalidNodeIndex) {
         cur.distance = kInfiniteDistanceLong;
         cur.angles = 360.0f;
      }
      else if (exists (cur.index)) {
         cur.angles = ((m_paths[cur.index].origin - path.origin).angles () - (m_paths[sorted[0].index].origin - path.origin).angles ()).y;

         if (cur.angles < 0.0f) {
            cur.angles += 360.0f;
         }
      }
   }

   //  sort the paths from the lowest to the highest angle (related to the vector closest node - checked index)...
   do {
      sorting = false;

      for (int i = 0; i < kMaxNodeLinks - 1; ++i) {
         if (sorted[i].index != kInvalidNodeIndex && sorted[i].angles > sorted[i + 1].angles) {
            cr::swap (sorted[i], sorted[i + 1]);
            sorting = true;
         }
      }
   } while (sorting);

   // reset top state
   top.reset ();

   // printing all the stuff causes reliable message overflow
   ctrl.setRapidOutput (true);

   // check pass 0
   auto inspect_p0 = [&] (const int id) -> bool {
      if (id < 2) {
         return false;
      }
      auto &cur = sorted[id], &prev = sorted[id - 1], &prev2 = sorted[id - 2];

      if (cur.index == kInvalidNodeIndex || prev.index == kInvalidNodeIndex || prev2.index == kInvalidNodeIndex) {
         return false;
      }

      // store the highest index which should be tested later...
      top.index = cur.index;
      top.distance = cur.distance;
      top.angles = cur.angles;

      if (cur.angles - prev2.angles < 80.0f) {

         // leave alone ladder connections and don't remove jump connections..
         if (((path.flags & NodeFlag::Ladder)
            && (m_paths[prev.index].flags & NodeFlag::Ladder)) || (path.links[prev.number].flags & PathFlag::Jump)) {

            return false;
         }

         if ((cur.distance + prev2.distance) * 1.1f / 2.0f < prev.distance) {
            if (path.links[prev.number].index == prev.index) {
               msg ("Removing a useless (P.0.1) connection from index = %d to %d.", index, prev.index);

               // unassign this path
               clearPath (index, prev.number);

               for (int j = 0; j < kMaxNodeLinks; ++j) {
                  if (m_paths[prev.index].links[j].index == index && !(m_paths[prev.index].links[j].flags & PathFlag::Jump)) {
                     msg ("Removing a useless (P.0.2) connection from index = %d to %d.", prev.index, index);

                     // unassign this path
                     clearPath (prev.index, j);
                  }
               }
               prev.index = kInvalidNodeIndex;

               for (int j = id - 1; j < kMaxNodeLinks - 1; ++j) {
                  sorted[j] = cr::move (sorted[j + 1]);
               }
               sorted[kMaxNodeLinks - 1].index = kInvalidNodeIndex;

               // do a second check
               return true;
            }
            else {
               msg ("Failed to remove a useless (P.0) connection from index = %d to %d.", index, prev.index);
               return false;
            }
         }
      }
      return false;
   };

   for (int i = 2; i < kMaxNodeLinks; ++i) {
      while (inspect_p0 (i)) {}
   }

   // check pass 1
   if (exists (top.index) && exists (sorted[0].index) && exists (sorted[1].index)) {

      if ((sorted[1].angles - top.angles < 80.0f || 360.0f - (sorted[1].angles - top.angles) < 80.0f)
         && (!(m_paths[sorted[0].index].flags & NodeFlag::Ladder) || !(path.flags & NodeFlag::Ladder))
         && !(path.links[sorted[0].number].flags & PathFlag::Jump)) {

         if ((sorted[1].distance + top.distance) * 1.1f / 2.0f < sorted[0].distance) {
            if (path.links[sorted[0].number].index == sorted[0].index) {
               msg ("Removing a useless (P.1.1) connection from index = %d to %d.", index, sorted[0].index);

               // unassign this path
               clearPath (index, sorted[0].number);

               for (int j = 0; j < kMaxNodeLinks; ++j) {
                  if (m_paths[sorted[0].index].links[j].index == index && !(m_paths[sorted[0].index].links[j].flags & PathFlag::Jump)) {
                     msg ("Removing a useless (P.1.2) connection from index = %d to %d.", sorted[0].index, index);

                     // unassign this path
                     clearPath (sorted[0].index, j);
                  }
               }
               sorted[0].index = kInvalidNodeIndex;

               for (int j = 0; j < kMaxNodeLinks - 1; ++j) {
                  sorted[j] = cr::move (sorted[j + 1]);
               }
               sorted[kMaxNodeLinks - 1].index = kInvalidNodeIndex;
            }
            else {
               msg ("Failed to remove a useless (P.1) connection from index = %d to %d.", sorted[0].index, index);
            }
         }
      }
   }
   top.reset ();

   // check pass 2
   auto inspect_p2 = [&] (const int id) -> bool {
      if (id < 1) {
         return false;
      }
      auto &cur = sorted[id], &prev = sorted[id - 1];

      if (cur.index == kInvalidNodeIndex || prev.index == kInvalidNodeIndex) {
         return false;
      }

      if (cur.angles - prev.angles < 40.0f) {
         if (prev.distance < cur.distance * 1.1f) {

            // leave alone ladder connections and don't remove jump connections..
            if (((path.flags & NodeFlag::Ladder)
               && (m_paths[cur.index].flags & NodeFlag::Ladder)) || (path.links[cur.number].flags & PathFlag::Jump)) {

               return false;
            }

            if (path.links[cur.number].index == cur.index) {
               msg ("Removing a useless (P.2.1) connection from index = %d to %d.", index, cur.index);

               // unassign this path
               clearPath (index, cur.number);

               for (int j = 0; j < kMaxNodeLinks; ++j) {
                  if (m_paths[cur.index].links[j].index == index && !(m_paths[cur.index].links[j].flags & PathFlag::Jump)) {
                     msg ("Removing a useless (P.2.2) connection from index = %d to %d.", cur.index, index);

                     // unassign this path
                     clearPath (cur.index, j);
                  }
               }
               cur.index = kInvalidNodeIndex;

               for (int j = id - 1; j < kMaxNodeLinks - 1; ++j) {
                  sorted[j] = cr::move (sorted[j + 1]);
               }
               sorted[kMaxNodeLinks - 1].index = kInvalidNodeIndex;
               return true;
            }
            else {
               msg ("Failed to remove a useless (P.2) connection from index = %d to %d.", index, cur.index);
            }
         }
         else if (cur.distance < prev.distance * 1.1f) {
            // leave alone ladder connections and don't remove jump connections..
            if (((path.flags & NodeFlag::Ladder)
               && (m_paths[prev.index].flags & NodeFlag::Ladder))
               || (path.links[prev.number].flags & PathFlag::Jump)) {

               return false;
            }

            if (path.links[prev.number].index == prev.index) {
               msg ("Removing a useless (P.2.3) connection from index = %d to %d.", index, prev.index);

               // unassign this path
               clearPath (index, prev.number);

               for (int j = 0; j < kMaxNodeLinks; ++j) {
                  if (m_paths[prev.index].links[j].index == index && !(m_paths[prev.index].links[j].flags & PathFlag::Jump)) {
                     msg ("Removing a useless (P.2.4) connection from index = %d to %d.", prev.index, index);

                     // unassign this path
                     clearPath (prev.index, j);
                  }
               }
               prev.index = kInvalidNodeIndex;

               for (int j = id - 1; j < kMaxNodeLinks - 1; ++j) {
                  sorted[j] = cr::move (sorted[j + 1]);
               }
               sorted[kMaxNodeLinks - 1].index = kInvalidNodeIndex;

               // do a second check
               return true;
            }
            else {
               msg ("Failed to remove a useless (P.2) connection from index = %d to %d.", index, prev.index);
            }
         }
      }
      else {
         top = cur;
      }
      return false;
   };

   for (int i = 1; i < kMaxNodeLinks; ++i) {
      while (inspect_p2 (i)) {}
   }

   // check pass 3
   if (exists (top.index) && exists (sorted[0].index)) {

      if ((top.angles - sorted[0].angles < 40.0f || (360.0f - top.angles - sorted[0].angles) < 40.0f)
         && (!(m_paths[sorted[0].index].flags & NodeFlag::Ladder) || !(path.flags & NodeFlag::Ladder))
         && !(path.links[sorted[0].number].flags & PathFlag::Jump)) {

         if (top.distance * 1.1f < sorted[0].distance) {
            if (path.links[sorted[0].number].index == sorted[0].index) {
               msg ("Removing a useless (P.3.1) connection from index = %d to %d.", index, sorted[0].index);

               // unassign this path
               clearPath (index, sorted[0].number);

               for (int j = 0; j < kMaxNodeLinks; ++j) {
                  if (m_paths[sorted[0].index].links[j].index == index && !(m_paths[sorted[0].index].links[j].flags & PathFlag::Jump)) {
                     msg ("Removing a useless (P.3.2) connection from index = %d to %d.", sorted[0].index, index);

                     // unassign this path
                     clearPath (sorted[0].index, j);
                  }
               }
               sorted[0].index = kInvalidNodeIndex;

               for (int j = 0; j < kMaxNodeLinks - 1; ++j) {
                  sorted[j] = cr::move (sorted[j + 1]);
               }
               sorted[kMaxNodeLinks - 1].index = kInvalidNodeIndex;
            }
            else {
               msg ("Failed to remove a useless (P.3) connection from index = %d to %d.", sorted[0].index, index);
            }
         }
         else if (sorted[0].distance * 1.1f < top.distance && !(path.links[top.number].flags & PathFlag::Jump)) {
            if (path.links[top.number].index == top.index) {
               msg ("Removing a useless (P.3.3) connection from index = %d to %d.", index, sorted[0].index);

               // unassign this path
               clearPath (index, top.number);

               for (int j = 0; j < kMaxNodeLinks; ++j) {
                  if (m_paths[top.index].links[j].index == index && !(m_paths[top.index].links[j].flags & PathFlag::Jump)) {
                     msg ("Removing a useless (P.3.4) connection from index = %d to %d.", sorted[0].index, index);

                     // unassign this path
                     clearPath (top.index, j);
                  }
               }
               sorted[0].index = kInvalidNodeIndex;
            }
            else {
               msg ("Failed to remove a useless (P.3) connection from index = %d to %d.", sorted[0].index, index);
            }
         }
      }
   }
   ctrl.setRapidOutput (false);

   return numFixedLinks;
}

int BotGraph::getBspSize () {
   if (File bsp { strings.joinPath (game.getRunningModName (), "maps", game.getMapName ()) + ".bsp", "rb" }) {
      return static_cast <int> (bsp.length ());
   }

   // worst case, load using engine (engfuncs.pfnGetFileSize isn't available on some legacy engines)
   if (MemFile bsp { strings.joinPath (game.getRunningModName (), "maps", game.getMapName ()) + ".bsp" }) {
      return static_cast <int> (bsp.length ());
   }
   return 0;
}

void BotGraph::addPath (int addIndex, int pathIndex, float distance) {
   if (!exists (addIndex) || !exists (pathIndex) || pathIndex == addIndex) {
      return;
   }
   auto &path = m_paths[addIndex];

   // don't allow paths get connected twice
   for (const auto &link : path.links) {
      if (link.index == pathIndex) {
         msg ("Denied path creation from %d to %d (path already exists).", addIndex, pathIndex);
         return;
      }
   }
   auto integerDistance = cr::abs (static_cast <int> (distance));

   // check for free space in the connection indices
   for (auto &link : path.links) {
      if (link.index == kInvalidNodeIndex) {
         link.index = static_cast <int16_t> (pathIndex);
         link.distance = integerDistance;

         msg ("Path added from %d to %d.", addIndex, pathIndex);
         return;
      }
   }

   // there wasn't any free space. try exchanging it with a long-distance path
   int maxDistance = -kInfiniteDistanceLong;
   int slot = kInvalidNodeIndex;

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      if (path.links[i].distance > maxDistance) {
         maxDistance = path.links[i].distance;
         slot = i;
      }
   }

   if (slot != kInvalidNodeIndex) {
      msg ("Path added from %d to %d.", addIndex, pathIndex);

      path.links[slot].index = static_cast <int16_t> (pathIndex);
      path.links[slot].distance = integerDistance;
   }
}

int BotGraph::getFarest (const Vector &origin, const float maxRange) {
   // find the farest node to that origin, and return the index to this node

   int index = kInvalidNodeIndex;
   auto maxDistanceSq = cr::sqrf (maxRange);

   for (const auto &path : m_paths) {
      const float distanceSq = path.origin.distanceSq (origin);

      if (distanceSq > maxDistanceSq) {
         index = path.number;
         maxDistanceSq = distanceSq;
      }
   }
   return index;
}

int BotGraph::getForAnalyzer (const Vector &origin, const float maxRange) {
   // find the farest node to that origin, and return the index to this node

   int index = kInvalidNodeIndex;
   float maximumDistanceSq = cr::sqrf (maxRange);

   for (const auto &path : m_paths) {
      const float distanceSq = path.origin.distanceSq (origin);

      if (distanceSq < maximumDistanceSq) {
         index = path.number;
         maximumDistanceSq = distanceSq;
      }
   }
   return index;
}

int BotGraph::getNearestNoBuckets (const Vector &origin, const float range, int flags) {
   // find the nearest node to that origin and return the index

   // fallback and go thru wall the nodes...
   int index = kInvalidNodeIndex;
   float nearestDistanceSq = cr::sqrf (range);

   for (const auto &path : m_paths) {
      if (flags != -1 && !(path.flags & flags)) {
         continue; // if flag not -1 and node has no this flag, skip node
      }
      const float distanceSq = path.origin.distanceSq (origin);

      if (distanceSq < nearestDistanceSq) {
         index = path.number;
         nearestDistanceSq = distanceSq;
      }
   }
   return index;
}

int BotGraph::getEditorNearest (const float maxRange) {
   if (!hasEditFlag (GraphEdit::On)) {
      return kInvalidNodeIndex;
   }
   return getNearestNoBuckets (m_editor->v.origin, maxRange);
}

int BotGraph::getNearest (const Vector &origin, const float range, int flags) {
   // find the nearest node to that origin and return the index

   // if not alot of nodes on the map, do not bother to use buckets here, we're dont
   // get any performance improvement
   constexpr auto kMinNodesForBucketsThreshold = 164;

   if (length () < kMinNodesForBucketsThreshold) {
      return getNearestNoBuckets (origin, range, flags);
   }

   if (range > 256.0f && !cr::fequal (range, kInfiniteDistance)) {
      return getNearestNoBuckets (origin, range, flags);
   }
   const auto &bucket = getNodesInBucket (origin);

   if (bucket.length () < kMaxNodeLinks) {
      return getNearestNoBuckets (origin, range, flags);
   }

   int index = kInvalidNodeIndex;
   auto nearestDistanceSq = cr::sqrf (range);

   for (const auto &at : bucket) {
      if (flags != -1 && !(m_paths[at].flags & flags)) {
         continue; // if flag not -1 and node has no this flag, skip node
      }
      const float distanceSq = origin.distanceSq (m_paths[at].origin);

      if (distanceSq < nearestDistanceSq) {
         index = at;
         nearestDistanceSq = distanceSq;
      }
   }

   // nothing found, try to find without buckets
   if (index == kInvalidNodeIndex) {
      return getNearestNoBuckets (origin, range, flags);
   }
   return index;
}

IntArray BotGraph::getNearestInRadius (const float radius, const Vector &origin, int maxCount) {
   // returns all nodes within radius from position

   const float radiusSq = cr::sqrf (radius);

   IntArray result {};
   const auto &bucket = getNodesInBucket (origin);

   if (bucket.length () < kMaxNodeLinks || radius > cr::sqrf (256.0f)) {
      for (const auto &path : m_paths) {
         if (maxCount != -1 && static_cast <int> (result.length ()) > maxCount) {
            break;
         }

         if (origin.distanceSq (path.origin) < radiusSq) {
            result.push (path.number);
         }
      }
      return result;
   }

   for (const auto &at : bucket) {
      if (maxCount != -1 && static_cast <int> (result.length ()) > maxCount) {
         break;
      }

      if (origin.distanceSq (m_paths[at].origin) < radiusSq) {
         result.push (at);
      }
   }
   return result;
}

bool BotGraph::isAnalyzed () const {
   return (m_info.header.options & StorageOption::Analyzed);
}

void BotGraph::add (int type, const Vector &pos) {
   if (!hasEditor () && !analyzer.isAnalyzing ()) {
      return;
   }
   int index = kInvalidNodeIndex;
   Path *path = nullptr;

   bool addNewNode = true;
   Vector newOrigin = pos;

   if (newOrigin.empty ()) {
      if (!hasEditor ()) {
         return;
      }
      newOrigin = m_editor->v.origin;
   }

   if (bots.hasBotsOnline ()) {
      bots.kickEveryone (true);
   }
   m_hasChanged = true;

   switch (type) {
   case NodeAddFlag::Camp:
      index = getEditorNearest ();

      if (index != kInvalidNodeIndex) {
         path = &m_paths[index];

         if (path->flags & NodeFlag::Camp) {
            path->start = m_editor->v.v_angle.get2d ();
            emitNotify (NotifySound::Done); // play "done" sound... 
            return;
         }
      }
      break;

   case NodeAddFlag::CampEnd:
      index = getEditorNearest ();

      if (index != kInvalidNodeIndex) {
         path = &m_paths[index];

         if (!(path->flags & NodeFlag::Camp)) {
            msg ("This is not camping node.");
            return;
         }
         path->end = m_editor->v.v_angle.get2d ();
         emitNotify (NotifySound::Done); // play "done" sound... 
      }
      return;

   case NodeAddFlag::JumpStart:
      index = getEditorNearest (25.0f);

      if (index != kInvalidNodeIndex && m_paths[index].number >= 0) {
         const float distanceSq = m_editor->v.origin.distanceSq (m_paths[index].origin);

         if (distanceSq < cr::sqrf (25.0f)) {
            addNewNode = false;

            path = &m_paths[index];
            path->origin = (path->origin + m_learnPosition) * 0.5f;
         }
      }
      else {
         newOrigin = m_learnPosition;
      }
      break;

   case NodeAddFlag::JumpEnd:
      index = getEditorNearest (25.0f);

      if (index != kInvalidNodeIndex && m_paths[index].number >= 0) {
         const float distanceSq = m_editor->v.origin.distanceSq (m_paths[index].origin);

         if (distanceSq < cr::sqrf (25.0f)) {
            addNewNode = false;
            path = &m_paths[index];

            int connectionFlags = 0;

            for (const auto &link : path->links) {
               connectionFlags += link.flags;
            }

            if (connectionFlags == 0) {
               path->origin = (path->origin + m_editor->v.origin) * 0.5f;
            }
         }
      }
      break;
   }

   if (addNewNode) {
      if (analyzer.isAnalyzing ()) {
         for (const auto &cp : m_paths) {
            if (newOrigin.distanceSq (cp.origin) < cr::sqrf (24.0f)) {
               return;
            }
         }
      }
      else {
         auto nearest = getEditorNearest ();

         // do not allow to place node "inside" node, make at leat 10 units range
         if (exists (nearest) && newOrigin.distanceSq (m_paths[nearest].origin) < cr::sqrf (10.0f)) {
            msg ("Can't add node. It's way to near to %d node. Please move some units anywhere.", nearest);
            return;
         }
      }

      // need to remove limit?
      if (m_paths.length () >= kMaxNodes) {
         return;
      }
      m_paths.emplace ();

      index = length () - 1;
      path = &m_paths[index];

      path->number = index;
      path->flags = 0;

      // store the origin (location) of this node
      path->origin = newOrigin;
      path->start.clear ();
      path->end.clear ();

      path->display = 0.0f;
      path->light = kInvalidLightLevel;

      for (auto &link : path->links) {
         link.index = kInvalidNodeIndex;
         link.distance = 0;
         link.flags = 0;
         link.velocity.clear ();
      }

      // autosave nodes here and there
      if (!analyzer.isAnalyzing () && cv_graph_auto_save_count && ++m_autoSaveCount >= cv_graph_auto_save_count.as <int> ()) {
         if (saveGraphData ()) {
            msg ("Nodes has been autosaved...");
         }
         else {
            msg ("Can't autosave graph data...");
         }
         m_autoSaveCount = 0;
      }

      // store the last used node for the auto node code...
      if (!analyzer.isAnalyzing ()) {
         m_lastNode = m_editor->v.origin;
      }
   }

   if (type == NodeAddFlag::JumpStart) {
      m_lastJumpNode = index;
   }
   else if (type == NodeAddFlag::JumpEnd) {
      const float distance = m_paths[m_lastJumpNode].origin.distance (m_editor->v.origin);
      addPath (m_lastJumpNode, index, distance);

      for (auto &link : m_paths[m_lastJumpNode].links) {
         if (link.index == index) {
            link.flags |= PathFlag::Jump;
            link.velocity = m_learnVelocity;

            break;
         }
      }
      calculatePathRadius (index);
      return;
   }

   if (!path || path->number == kInvalidNodeIndex) {
      return;
   }

   if (analyzer.isCrouch () || (!analyzer.isAnalyzing () && (m_editor->v.flags & FL_DUCKING))) {
      path->flags |= NodeFlag::Crouch; // set a crouch node
   }

   if (!analyzer.isAnalyzing () && m_editor->v.movetype == MOVETYPE_FLY) {
      path->flags |= NodeFlag::Ladder;
   }
   else if (m_isOnLadder) {
      path->flags |= NodeFlag::Ladder;
   }

   switch (type) {
   case NodeAddFlag::TOnly:
      path->flags |= NodeFlag::Crossing;
      path->flags |= NodeFlag::TerroristOnly;
      break;

   case NodeAddFlag::CTOnly:
      path->flags |= NodeFlag::Crossing;
      path->flags |= NodeFlag::CTOnly;
      break;

   case NodeAddFlag::NoHostage:
      path->flags |= NodeFlag::NoHostage;
      break;

   case NodeAddFlag::Rescue:
      path->flags |= NodeFlag::Rescue;
      break;

   case NodeAddFlag::Camp:
      path->flags |= NodeFlag::Crossing;
      path->flags |= NodeFlag::Camp;

      if (!analyzer.isAnalyzing ()) {
         path->start = m_editor->v.v_angle;
         path->end = m_editor->v.v_angle;
      }
      break;

   case NodeAddFlag::Goal:
      path->flags |= NodeFlag::Goal;
      break;
   }

   // ladder nodes need careful connections
   if (path->flags & NodeFlag::Ladder) {
      float nearestDistance = kInfiniteDistance;
      int destIndex = kInvalidNodeIndex;

      TraceResult tr {};

      // calculate all the paths to this new node
      for (const auto &calc : m_paths) {
         if (calc.number == index) {
            continue; // skip the node that was just added
         }

         // other ladder nodes should connect to this
         if (calc.flags & NodeFlag::Ladder) {
            // check if the node is reachable from the new one
            game.testLine (newOrigin, calc.origin, TraceIgnore::Monsters, m_editor, &tr);

            if (cr::fequal (tr.flFraction, 1.0f)
               && cr::abs (newOrigin.x - calc.origin.x) < 64.0f
               && cr::abs (newOrigin.y - calc.origin.y) < 64.0f
               && cr::abs (newOrigin.z - calc.origin.z) < m_autoPathDistance) {

               const float distance = newOrigin.distance2d (calc.origin);

               addPath (index, calc.number, distance);
               addPath (calc.number, index, distance);
            }
         }
         else {
            const float distance = newOrigin.distance2d (calc.origin);

            if (distance < nearestDistance) {
               destIndex = calc.number;
               nearestDistance = distance;
            }

            // check if the node is reachable from the new one
            if (isNodeReacheable (newOrigin, calc.origin)) {
               addPath (index, calc.number, distance);
            }
         }
      }

      if (exists (destIndex)) {
         const float distance = newOrigin.distance2d (m_paths[destIndex].origin);

         if (analyzer.isAnalyzing ()) {
            addPath (index, destIndex, distance);
            addPath (destIndex, index, distance);
         }
         else {
            // check if the node is reachable from the new one (one-way)
            if (isNodeReacheable (newOrigin, m_paths[destIndex].origin)) {
               addPath (index, destIndex, newOrigin.distance (m_paths[destIndex].origin));
            }

            // check if the new one is reachable from the node (other way)
            if (isNodeReacheable (m_paths[destIndex].origin, newOrigin)) {
               addPath (destIndex, index, newOrigin.distance (m_paths[destIndex].origin));
            }
         }
      }
   }
   else {
      // calculate all the paths to this new node
      for (const auto &calc : m_paths) {
         if (calc.number == index) {
            continue; // skip the node that was just added
         }
         const float distance = calc.origin.distance2d (newOrigin);

         // check if the node is reachable from the new one (one-way)
         if (isNodeReacheable (newOrigin, calc.origin)) {
            addPath (index, calc.number, distance);
         }

         // check if the new one is reachable from the node (other way)
         if (isNodeReacheable (calc.origin, newOrigin)) {
            addPath (calc.number, index, distance);
         }
      }

      if (!analyzer.isAnalyzing ()) {
         clearConnections (index);
      }
   }
   emitNotify (NotifySound::Added);
   calculatePathRadius (index); // calculate the wayzone of this node

   if (analyzer.isAnalyzing ()) {
      analyzer.markOptimized (index);
   }
}

void BotGraph::erase (int target) {
   m_hasChanged = true;

   if (m_paths.empty ()) {
      return;
   }

   if (bots.hasBotsOnline ()) {
      bots.kickEveryone (true);
   }
   const int index = (target == kInvalidNodeIndex) ? getEditorNearest () : target;

   if (!exists (index)) {
      return;
   }
   auto &path = m_paths[index];

   // unassign paths that points to this nodes
   for (auto &connected : m_paths) {
      for (auto &link : connected.links) {
         if (link.index == index) {
            link.index = kInvalidNodeIndex;
            link.flags = 0;
            link.distance = 0;
            link.velocity.clear ();
         }
      }
   }

   // relink nodes so the index will match path number
   for (auto &relink : m_paths) {

      // if pathnumber bigger than deleted node...
      if (relink.number > index) {
         --relink.number;
      }

      for (auto &neighbour : relink.links) {
         if (neighbour.index > index) {
            --neighbour.index;
         }
      }
   }
   m_paths.remove (path);
   emitNotify (NotifySound::Change);
}

void BotGraph::toggleFlags (int toggleFlag) {
   // this function allow manually changing flags

   int index = getEditorNearest ();

   if (index != kInvalidNodeIndex) {
      if (m_paths[index].flags & toggleFlag) {
         m_paths[index].flags &= ~toggleFlag;
      }
      else {
         if (toggleFlag == NodeFlag::Sniper && !(m_paths[index].flags & NodeFlag::Camp)) {
            msg ("Cannot assign sniper flag to node %d. This is not camp node.", index);
            return;
         }
         m_paths[index].flags |= toggleFlag;
      }
      emitNotify (NotifySound::Done); // play "done" sound... 
   }
}

void BotGraph::setRadius (int index, float radius) {
   // this function allow manually setting the zone radius

   const int node = exists (index) ? index : getEditorNearest ();

   if (node != kInvalidNodeIndex) {
      m_paths[node].radius = radius;
      emitNotify (NotifySound::Done); // play "done" sound...

      msg ("Node %d has been set to radius %.2f.", node, radius);
   }
}

bool BotGraph::isConnected (int a, int b) {
   // this function checks if node A has a connection to node B

   if (!exists (a) || !exists (b)) {
      return false;
   }

   for (const auto &link : m_paths[a].links) {
      if (link.index == b) {
         return true;
      }
   }
   return false;
}

int BotGraph::getFacingIndex () {
   // find the node the user is pointing at

   Twin <int32_t, float> result { kInvalidNodeIndex, 5.32f };
   auto nearestNode = getEditorNearest ();

   // check bounds from eyes of editor
   const auto &editorEyes = m_editor->v.origin + m_editor->v.view_ofs;

   for (const auto &path : m_paths) {

      // skip nearest node to editor, since this used mostly for adding / removing paths
      if (path.number == nearestNode) {
         continue;
      }

      const auto &to = path.origin - m_editor->v.origin;
      auto angles = (to.angles () - m_editor->v.v_angle).clampAngles ();

      // skip the nodes that are too far away from us, and we're not looking at them directly
      if (to.lengthSq () > cr::sqrf (cv_graph_draw_distance.as <float> ()) || cr::abs (angles.y) > result.second) {
         continue;
      }

      // check if visible, (we're not using visibility tables here, as they not valid at time of node editing)
      TraceResult tr {};
      game.testLine (editorEyes, path.origin, TraceIgnore::Everything, m_editor, &tr);

      if (!cr::fequal (tr.flFraction, 1.0f)) {
         continue;
      }
      const float bestAngle = angles.y;

      angles = -m_editor->v.v_angle;
      angles.x = -angles.x;
      angles = (angles + ((path.origin - Vector (0.0f, 0.0f, (path.flags & NodeFlag::Crouch) ? 17.0f : 34.0f)) - editorEyes).angles ()).clampAngles ();

      if (angles.x > 0.0f) {
         continue;
      }
      result = { path.number, bestAngle };
   }
   return result.first;
}

void BotGraph::pathCreate (char dir) {
   // this function allow player to manually create a path from one node to another

   int nodeFrom = getEditorNearest ();

   if (nodeFrom == kInvalidNodeIndex) {
      msg ("Unable to find nearest node in 50 units.");
      return;
   }
   int nodeTo = m_facingAtIndex;

   if (!exists (nodeTo)) {
      if (exists (m_cacheNodeIndex)) {
         nodeTo = m_cacheNodeIndex;
      }
      else {
         msg ("Unable to find destination node.");
         return;
      }
   }

   if (nodeTo == nodeFrom) {
      msg ("Unable to connect node with itself.");
      return;
   }
   const float distance = m_paths[nodeFrom].origin.distance (m_paths[nodeTo].origin);

   if (dir == PathConnection::Outgoing) {
      addPath (nodeFrom, nodeTo, distance);
   }
   else if (dir == PathConnection::Incoming) {
      addPath (nodeTo, nodeFrom, distance);
   }
   else if (dir == PathConnection::Jumping) {
      if (!isConnected (nodeFrom, nodeTo)) {
         addPath (nodeFrom, nodeTo, distance);
      }

      for (auto &link : m_paths[nodeFrom].links) {
         if (link.index == nodeTo && !(link.flags & PathFlag::Jump)) {
            link.flags |= PathFlag::Jump;
            m_paths[nodeFrom].radius = 0.0f;

            msg ("Path added from %d to %d.", nodeFrom, nodeTo);
         }
         else if (link.index == nodeTo && (link.flags & PathFlag::Jump)) {
            msg ("Denied path creation from %d to %d (path already exists).", nodeFrom, nodeTo);
         }
      }
   }
   else {
      addPath (nodeFrom, nodeTo, distance);
      addPath (nodeTo, nodeFrom, distance);
   }
   emitNotify (NotifySound::Done); // play "done" sound... 
   m_hasChanged = true;
}

void BotGraph::erasePath () {
   // this function allow player to manually remove a path from one node to another

   int nodeFrom = getEditorNearest ();

   if (nodeFrom == kInvalidNodeIndex) {
      msg ("Unable to find nearest node in 50 units.");
      return;
   }
   int nodeTo = m_facingAtIndex;

   if (!exists (nodeTo)) {
      if (exists (m_cacheNodeIndex)) {
         nodeTo = m_cacheNodeIndex;
      }
      else {
         msg ("Unable to find destination node.");
         return;
      }
   }

   // helper
   auto destroy = [] (PathLink &link) -> void {
      link.index = kInvalidNodeIndex;
      link.distance = 0;
      link.flags = 0;
      link.velocity.clear ();
   };

   for (auto &link : m_paths[nodeFrom].links) {
      if (link.index == nodeTo) {
         destroy (link);
         emitNotify (NotifySound::Change);

         return;
      }
   }

   // not found this way ? check for incoming connections then
   cr::swap (nodeFrom, nodeTo);

   for (auto &link : m_paths[nodeFrom].links) {
      if (link.index == nodeTo) {
         destroy (link);
         emitNotify (NotifySound::Change);

         return;
      }
   }
   msg ("There is already no path on this node.");
}

void BotGraph::resetPath (int index) {
   int node = index;

   if (!exists (node)) {
      node = getEditorNearest ();

      if (!exists (node)) {
         msg ("Unable to find nearest node in 50 units.");
         return;
      }
   }

   // helper
   auto destroy = [] (PathLink &link) -> void {
      link.index = kInvalidNodeIndex;
      link.distance = 0;
      link.flags = 0;
      link.velocity.clear ();
   };

   // clean all incoming
   for (auto &connected : m_paths) {
      for (auto &link : connected.links) {
         if (link.index == node) {
            destroy (link);
         }
      }
   }

   // clean all outgoing connections
   for (auto &link : m_paths[node].links) {
      destroy (link);
   }
   emitNotify (NotifySound::Change);

   // notify use something evil happened
   msg ("All paths for node #%d has been reset.", node);
}

void BotGraph::cachePoint (int index) {
   const int node = exists (index) ? index : getEditorNearest ();

   if (node == kInvalidNodeIndex) {
      m_cacheNodeIndex = kInvalidNodeIndex;
      msg ("Cached node cleared (nearby point not found in 50 units range).");

      return;
   }
   m_cacheNodeIndex = node;
   msg ("Node %d has been put into memory.", m_cacheNodeIndex);
}

void BotGraph::setAutoPathDistance (const float distance) {
   m_autoPathDistance = distance;

   if (cr::fzero (distance)) {
      msg ("Autopathing is now disabled.");
   }
   else {
      msg ("Autopath distance is set to %.2f.", distance);
   }
}

void BotGraph::showStats () {
   int terrPoints = 0;
   int ctPoints = 0;
   int goalPoints = 0;
   int rescuePoints = 0;
   int campPoints = 0;
   int sniperPoints = 0;
   int noHostagePoints = 0;

   for (const auto &path : m_paths) {
      if (path.flags & NodeFlag::TerroristOnly) {
         ++terrPoints;
      }

      if (path.flags & NodeFlag::CTOnly) {
         ++ctPoints;
      }

      if (path.flags & NodeFlag::Goal) {
         ++goalPoints;
      }

      if (path.flags & NodeFlag::Rescue) {
         ++rescuePoints;
      }

      if (path.flags & NodeFlag::Camp) {
         ++campPoints;
      }

      if (path.flags & NodeFlag::Sniper) {
         ++sniperPoints;
      }

      if (path.flags & NodeFlag::NoHostage) {
         ++noHostagePoints;
      }
   }

   msg ("Nodes: %d - T Points: %d", m_paths.length (), terrPoints);
   msg ("CT Points: %d - Goal Points: %d", ctPoints, goalPoints);
   msg ("Rescue Points: %d - Camp Points: %d", rescuePoints, campPoints);
   msg ("Block Hostage Points: %d - Sniper Points: %d", noHostagePoints, sniperPoints);
}

void BotGraph::showFileInfo () {
   const auto &info = m_info.header;
   const auto &exten = m_info.exten;

   msg ("header:");
   msg ("  magic: %d", info.magic);
   msg ("  version: %d", info.version);
   msg ("  node_count: %d", info.length);
   msg ("  compressed_size: %dkB", info.compressed / 1024);
   msg ("  uncompressed_size: %dkB", info.uncompressed / 1024);
   msg ("  options: %d", info.options); // display as string ?
   msg ("  analyzed: %s", isAnalyzed () ? conf.translate ("yes") : conf.translate ("no")); // display as string ?

   msg ("");

   msg ("extensions:");
   msg ("  author: %s", exten.author);
   msg ("  modified_by: %s", exten.modified);
   msg ("  bsp_size: %d", exten.mapSize);
}

void BotGraph::emitNotify (int32_t sound) const {
   static HashMap <int32_t, String> notifySounds = {
      { NotifySound::Added, "weapons/xbow_hit1.wav" },
      { NotifySound::Change, "weapons/mine_activate.wav" },
      { NotifySound::Done, "common/wpn_hudon.wav" }
   };

   // notify editor
   if (util.isPlayer (m_editor) && !m_silenceMessages) {
      game.playSound (m_editor, notifySounds[sound].chars ());
   }
}

void BotGraph::syncCollectOnline () {
   m_isOnlineCollected = true; // once per server start

   // path to graph files
   auto graphFilesPath = bstor.buildPath (BotFile::Graph, false, true);

   // enumerate graph files
   FileEnumerator enumerator { strings.joinPath (graphFilesPath, "*.graph") };

   // listing of graphs locally available
   StringArray localGraphs {};

   // collect all the files
   while (enumerator) {
      auto match = enumerator.getMatch ();

      match = match.substr (match.findLastOf (kPathSeparator) + 1);
      match = match.substr (0, match.findFirstOf ("."));

      localGraphs.emplace (match);
      enumerator.next ();
   }

   // no graphs ? unbelievable
   if (localGraphs.empty ()) {
      return;
   }
   String uploadUrlAddress = cv_graph_url_upload.as <StringRef> ();

   // only allow to upload to non-https endpoint
   if (uploadUrlAddress.startsWith ("https")) {
      return;
   }
   String localFile = plat.tmpfname ();

   // no temp file, no fun
   if (localFile.empty ()) {
      return;
   }

   // don't forget remove temporary file
   auto unlinkTemporary = [&] () {
      if (plat.fileExists (localFile.chars ())) {
         plat.removeFile (localFile.chars ());
      }
   };

   // write out our list of files into temporary
   if (File lc { localFile, "wt" }) {
      auto graphList = String::join (localGraphs, ",");
      auto collectUrl = strings.format ("%s://%s/collect/%u", product.httpScheme, uploadUrlAddress, graphList.hash ());

      lc.puts (graphList.chars ());
      lc.close ();

      // upload to collection diff
      if (!http.uploadFile (collectUrl, localFile)) {
         unlinkTemporary ();
         return;
      }
      unlinkTemporary ();

      // download collection diff
      if (!http.downloadFile (collectUrl, localFile)) {
         return;
      }
      StringArray wanted {};

      // decode answer
      if (lc.open (localFile, "rt")) {
         String lines {};

         if (lc.getLine (lines)) {
            wanted = lines.split (",");
         }
         lc.close ();
      }
      unlinkTemporary ();

      // if 'we're have something in diff, bailout
      if (wanted.empty ()) {
         return;
      }
      localGraphs.clear ();

      // convert graphs names into full paths
      for (const auto &wn : wanted) {
         if (wn == game.getMapName ()) {
            continue; // skip current map always
         }
         localGraphs.emplace (strings.joinPath (graphFilesPath, wn) + ".graph");
      }

      // try to upload everything database wants
      for (const auto &lg : localGraphs) {
         if (!plat.fileExists (lg.chars ())) {
            continue;
         }
         StorageHeader hdr {};

         // read storage header and check if file NOT analyzed
         if (File gp { lg, "rb" }) {
            gp.read (&hdr, sizeof (StorageHeader));

            // check the magic, graph is NOT analyzed and have some viable nodes number
            if (hdr.magic == kStorageMagic && !(hdr.options & StorageOption::Analyzed) && hdr.length > 48) {
               String uploadUrl = strings.format ("%s://%s", product.httpScheme, uploadUrlAddress);

               // try to upload to database (no need check if it's succeed)
               http.uploadFile (uploadUrl, lg);
            }
            gp.close ();
         }
      }
   }
   unlinkTemporary ();
}

void BotGraph::collectOnline () {
   if (m_isOnlineCollected || !cv_graph_auto_collect_db) {
      return;
   }

   worker.enqueue ([this] () {
      syncCollectOnline ();
   });
}

void BotGraph::calculatePathRadius (int index) {
   // calculate "wayzones" for the nearest node  (meaning a dynamic distance area to vary node origin)

   auto &path = m_paths[index];
   Vector start {}, direction {};

   if ((path.flags & (NodeFlag::Ladder | NodeFlag::Goal | NodeFlag::Camp | NodeFlag::Rescue | NodeFlag::Crouch)) || m_jumpLearnNode) {
      path.radius = 0.0f;
      return;
   }

   for (const auto &test : path.links) {
      if (test.index != kInvalidNodeIndex && (m_paths[test.index].flags & NodeFlag::Ladder)) {
         path.radius = 0.0f;
         return;
      }
   }
   TraceResult tr {};
   bool wayBlocked = false;

   for (int32_t scanDistance = 32; scanDistance < 128; scanDistance += 16) {
      auto scan = static_cast <float> (scanDistance);
      start = path.origin;

      direction = Vector (0.0f, 0.0f, 0.0f).forward () * scan;
      direction = direction.angles ();

      path.radius = scan;

      for (int32_t circleRadius = 0; circleRadius < 360; circleRadius += 20) {
         const auto &forward = direction.forward ();

         auto radiusStart = start + forward * scan;
         auto radiusEnd = start + forward * scan;

         game.testHull (radiusStart, radiusEnd, TraceIgnore::Monsters, head_hull, nullptr, &tr);

         if (tr.flFraction < 1.0f) {
            game.testLine (radiusStart, radiusEnd, TraceIgnore::Monsters, nullptr, &tr);

            if (util.isDoorEntity (tr.pHit)) {
               path.radius = 0.0f;
               wayBlocked = true;

               break;
            }
            wayBlocked = true;
            path.radius -= 16.0f;

            break;
         }

         auto dropStart = start + forward * scan;
         auto dropEnd = dropStart - Vector (0.0f, 0.0f, scan + 60.0f);

         game.testHull (dropStart, dropEnd, TraceIgnore::Monsters, head_hull, nullptr, &tr);

         if (tr.flFraction >= 1.0f) {
            wayBlocked = true;
            path.radius -= 16.0f;

            break;
         }
         dropStart = start - forward * scan;
         dropEnd = dropStart - Vector (0.0f, 0.0f, scan + 60.0f);

         game.testHull (dropStart, dropEnd, TraceIgnore::Monsters, head_hull, nullptr, &tr);

         if (tr.flFraction >= 1.0f) {
            wayBlocked = true;
            path.radius -= 16.0f;
            break;
         }

         radiusEnd.z += 34.0f;
         game.testHull (radiusStart, radiusEnd, TraceIgnore::Monsters, head_hull, nullptr, &tr);

         if (tr.flFraction < 1.0f) {
            wayBlocked = true;
            path.radius -= 16.0f;

            break;
         }
         direction.y = cr::wrapAngle (direction.y + static_cast <float> (circleRadius));
      }

      if (wayBlocked) {
         break;
      }
   }
   path.radius -= 16.0f;

   if (path.radius < 0.0f) {
      path.radius = 0.0f;
   }
}

void BotGraph::syncInitLightLevels () {
   // this function get's the light level for each node on the map

   // update light levels for all nodes
   for (auto &path : m_paths) {
      path.light = illum.getLightLevel (path.origin + Vector { 0.0f, 0.0f, 16.0f });
   }
   m_lightChecked = true;

   // disable lightstyle animations on finish (will be auto-enabled on mapchange)
   illum.enableAnimation (false);
}

void BotGraph::initLightLevels () {
   // this function get's the light level for each node on the map

   // no nodes ? no light levels, and only one-time init
   if (m_paths.empty () || m_lightChecked) {
      return;
   }
   const auto &players = bots.countTeamPlayers ();

   // do calculation if some-one is already playing on the server
   if (!players.first && !players.second) {
      return;
   }

   worker.enqueue ([this] () {
      syncInitLightLevels ();
   });
}

void BotGraph::initNarrowPlaces () {
   // this function checks all nodes if they are inside narrow places. this is used to prevent
   // bots to track hidden enemies in narrow places and prevent bots from throwing flashbangs or
   // other grenades inside bad places.

   // no nodes ? 
   if (m_paths.empty () || m_narrowChecked) {
      return;
   }
   constexpr int32_t kNarrowPlacesMinGraphVersion = 2;

   // if version 2 or higher, narrow places already initialized and saved into file
   if (m_info.header.version >= kNarrowPlacesMinGraphVersion && !hasEditFlag (GraphEdit::On)) {
      m_narrowChecked = true;
      return;
   }
   TraceResult tr {};

   const auto distance = 178.0f;
   const auto worldspawn = game.getStartEntity ();
   const auto offset = Vector (0.0f, 0.0f, 16.0f);

   // check olny paths that have not too much connections
   for (auto &path : m_paths) {

      // skip any goals and camp points
      if (path.flags & (NodeFlag::Camp | NodeFlag::Goal)) {
         continue;
      }
      int linkCount = 0;

      for (const auto &link : path.links) {
         if (link.index == kInvalidNodeIndex || link.index == path.number) {
            continue;
         }

         if (++linkCount > kMaxNodeLinks / 2) {
            break;
         }
      }

      // skip nodes with too much connections, this indicated we're not in narrow place
      if (linkCount > kMaxNodeLinks / 2) {
         continue;
      }
      int accumWeight = 0;

      // we could use this one!
      for (const auto &link : path.links) {
         if (link.index == kInvalidNodeIndex || link.index == path.number) {
            continue;
         }
         const Vector &ang = ((path.origin - m_paths[link.index].origin).normalize () * distance).angles ();

         Vector forward {}, right {}, upward {};
         ang.angleVectors (&forward, &right, &upward);

         // helper lambda
         auto directionCheck = [&] (const Vector &to) -> bool {
            game.testLine (path.origin + offset, to, TraceIgnore::None, nullptr, &tr);

            // check if we're hit worldspawn entity
            if (tr.pHit == worldspawn && tr.flFraction < 1.0f) {
               return true;
            }
            return false;
         };


         if (directionCheck (-forward * distance)) {
            accumWeight += 1;
         }

         if (directionCheck (right * distance)) {
            accumWeight += 1;
         }

         if (directionCheck (-right * distance)) {
            accumWeight += 1;
         }

         if (directionCheck (upward * distance)) {
            accumWeight += 1;
         }
      }
      path.flags &= ~NodeFlag::Narrow;

      if (accumWeight > 1) {
         path.flags |= NodeFlag::Narrow;
      }
   }
   m_narrowChecked = true;
}

void BotGraph::populateNodes () {
   m_terrorPoints.clear ();
   m_ctPoints.clear ();
   m_goalPoints.clear ();
   m_campPoints.clear ();
   m_rescuePoints.clear ();
   m_sniperPoints.clear ();
   m_visitedGoals.clear ();
   m_nodeNumbers.clear ();

   for (const auto &path : m_paths) {
      if (path.flags & NodeFlag::TerroristOnly) {
         m_terrorPoints.push (path.number);
      }
      else if (path.flags & NodeFlag::CTOnly) {
         m_ctPoints.push (path.number);
      }
      else if (path.flags & NodeFlag::Goal) {
         m_goalPoints.push (path.number);
      }
      else if (path.flags & NodeFlag::Camp) {
         m_campPoints.push (path.number);
      }
      else if (path.flags & NodeFlag::Sniper) {
         m_sniperPoints.push (path.number);
      }
      else if (path.flags & NodeFlag::Rescue) {
         m_rescuePoints.push (path.number);
      }
      m_nodeNumbers.push (path.number);
   }
}

bool BotGraph::convertOldFormat () {
   MemFile fp (bstor.buildPath (BotFile::PodbotPWF, true));

   if (!fp) {
      if (!fp.open (bstor.buildPath (BotFile::EbotEWP, true))) {
         return false;
      }
   }

   PODGraphHeader header {};
   plat.bzero (&header, sizeof (header));

   // save for faster access
   auto map = game.getMapName ();

   if (fp) {

      if (fp.read (&header, sizeof (header)) == 0) {
         return false;
      }

      if (strncmp (header.header, kPodbotMagic, cr::bufsize (kPodbotMagic)) == 0) {
         if (header.fileVersion != StorageVersion::Podbot) {
            return false;
         }
         else if (!strings.matches (header.mapName, map)) {
            return false;
         }
         else {
            if (header.pointNumber == 0 || header.pointNumber > kMaxNodes) {
               return false;
            }
            reset ();

            for (int i = 0; i < header.pointNumber; ++i) {
               Path path {};
               PODPath podpath {};

               if (fp.read (&podpath, sizeof (PODPath)) == 0) {
                  return false;
               }
               convertFromPOD (path, podpath);

               // more checks of node quality
               if (path.number < 0 || path.number > header.pointNumber) {
                  return false;
               }
               // add to node array
               m_paths.push (cr::move (path));
            }
            fp.close ();

            // save new format in case loaded older one
            if (!m_paths.empty ()) {
               msg ("Converting old PWF to new format Graph.");

               m_info.author = header.author;

               // clean editor so graph will be saved with header's author
               auto editor = m_editor;
               m_editor = nullptr;

               auto result = saveGraphData ();
               m_editor = editor;

               return result;
            }
         }
      }
      else {
         return false;
      }
   }
   else {
      return false;
   }
   return false;
}

bool BotGraph::loadGraphData () {
   ExtenHeader exten {};
   int32_t outOptions = 0;

   m_info.header = {};
   m_info.exten = {};

   // re-initialize paths
   reset ();

   // initialize compression
   ULZ ulz {};
   bstor.setUlzInstance (&ulz);

   // check if loaded
   const bool dataLoaded = bstor.load <Path> (m_paths, &exten, &outOptions);

   if (dataLoaded) {
      initBuckets ();

      // add data to buckets
      for (const auto &path : m_paths) {
         addToBucket (path.origin, path.number);
      }
      StringRef author = exten.author;

      if ((outOptions & StorageOption::Official) || author.startsWith ("official") || author.length () < 2) {
         m_info.author.assign (product.name);
      }
      else {
         m_info.author.assign (author);
      }
      StringRef modified = exten.modified;

      if (!modified.empty () && !modified.contains ("(none)")) {
         m_info.modified.assign (exten.modified);
      }
      planner.init (); // initialize our little path planner
      practice.load (); // load bots practice
      vistab.load (); // load/initialize visibility

      populateNodes ();

      if (exten.mapSize > 0) {
         const int mapSize = getBspSize ();

         if (mapSize > 0 && mapSize != exten.mapSize) {
            msg ("Warning: Graph data is probably not for this map. Please check bots behaviour.");
         }
      }

      // notify user about graph problems
      if (planner.isPathsCheckFailed () && !graph.isAnalyzed ()) {
         ctrl.msg ("Warning: Graph data has failed sanity check.");
         ctrl.msg ("Warning: Bots will use only shortest-path algo for path finding.");
         ctrl.msg ("Warning: This may significantly affect bots behavior on this map.");
      }
      cv_debug_goal.set (kInvalidNodeIndex);

      // try to do graph collection, and push them to graph database automatically
      collectOnline ();

      return true;
   }
   else {
      analyzer.start ();
   }
   return false;
}

bool BotGraph::canDownload () {
   return !cv_graph_url.as <StringRef> ().empty ();
}

bool BotGraph::saveGraphData () {
   auto options = StorageOption::Graph | StorageOption::Exten;
   String editorName {};

   if (!hasEditor () && !m_info.author.empty ()) {
      editorName = m_info.author;

      if (!game.isDedicated ()) {
         options |= StorageOption::Recovered;
      }
   }
   else if (!game.isNullEntity (m_editor)) {
      editorName = m_editor->v.netname.chars ();
   }
   else {
      editorName = product.name;
   }

   // mark as analyzed
   if (analyzer.isAnalyzed ()) {
      options |= StorageOption::Analyzed;
   }

   // mark as official
   if (editorName.startsWith (product.name)) {
      options |= StorageOption::Official;
   }
   ExtenHeader exten {};

   // only modify the author if no author currently assigned to graph file
   if (m_info.author.empty () || strings.isEmpty (m_info.exten.author)) {
      strings.copy (exten.author, editorName.chars (), cr::bufsize (exten.author));
   }
   else {
      strings.copy (exten.author, m_info.exten.author, cr::bufsize (exten.author));
   }

   // only update modified by, if name differs
   if (m_info.author != editorName && !strings.isEmpty (m_info.exten.author)) {
      strings.copy (exten.modified, editorName.chars (), cr::bufsize (exten.author));
   }
   exten.mapSize = getBspSize ();

   // ensure narrow places saved into file
   m_narrowChecked = false;
   m_lightChecked = false;

   initNarrowPlaces ();

   return bstor.save <Path> (m_paths, &exten, options);
}

void BotGraph::saveOldFormat () {
   PODGraphHeader header {};

   String editorName {};

   if (!hasEditor ()  && !m_info.author.empty ()) {
      editorName = m_info.author;
   }
   else if (!game.isNullEntity (m_editor)) {
      editorName = m_editor->v.netname.chars ();
   }
   else {
      editorName = product.name;
   }

   strings.copy (header.header, kPodbotMagic, sizeof (kPodbotMagic));
   strings.copy (header.author, editorName.chars (), cr::bufsize (header.author));
   strings.copy (header.mapName, game.getMapName (), cr::bufsize (header.mapName));

   header.mapName[31] = 0;
   header.fileVersion = StorageVersion::Podbot;
   header.pointNumber = length ();

   File fp {};

   // file was opened
   if (fp.open (bstor.buildPath (BotFile::PodbotPWF), "wb")) {
      // write the node header to the file...
      fp.write (&header, sizeof (header));

      // save the node paths...
      for (const auto &path : m_paths) {
         PODPath pod {};
         convertToPOD (path, pod);

         fp.write (&pod, sizeof (PODPath));
      }
      fp.close ();
   }
   else {
      logger.error ("Error writing '%s.pwf' node file.", game.getMapName ());
   }
}

float BotGraph::calculateTravelTime (float maxSpeed, const Vector &src, const Vector &origin) {
   // this function returns 2D traveltime to a position

   return origin.distance2d (src) / maxSpeed;
}

bool BotGraph::isNodeReacheableEx (const Vector &src, const Vector &destination, const float maxHeight) const {
   TraceResult tr {};

   float distanceSq = destination.distanceSq (src);

   if ((destination.z - src.z) >= 45.0f) {
      return false;
   }

   // is the destination not close enough?
   if (distanceSq > cr::sqrf (m_autoPathDistance)) {
      return false;
   }

   // check if we go through a func_illusionary, in which case return false
   game.testHull (src, destination, TraceIgnore::Monsters, head_hull, m_editor, &tr);

   if (tr.pHit && tr.pHit->v.classname.str () == "func_illusionary") {
      return false; // don't add path nodes through func_illusionaries
   }

   // check if this node is "visible"...
   game.testLine (src, destination, TraceIgnore::Monsters, m_editor, &tr);

   const bool isDoor = util.isDoorEntity (tr.pHit);

   // if node is visible from current position (even behind head)...
   if (tr.flFraction >= 1.0f || isDoor) {
      // if it's a door check if nothing blocks behind
      if (isDoor) {
         game.testLine (tr.vecEndPos, destination, TraceIgnore::Monsters, tr.pHit, &tr);

         if (tr.flFraction < 1.0f) {
            return false;
         }
      }

      // check for special case of both nodes being in water...
      if (engfuncs.pfnPointContents (src) == CONTENTS_WATER && engfuncs.pfnPointContents (destination) == CONTENTS_WATER) {
         return true; // then they're reachable each other
      }

      // is dest node higher than src? (45 is max jump height)
      if (destination.z > src.z + 45.0f) {
         Vector sourceNew = destination;
         Vector destinationNew = destination;
         destinationNew.z = destinationNew.z - 50.0f; // straight down 50 units

         game.testLine (sourceNew, destinationNew, TraceIgnore::Monsters, m_editor, &tr);

         // check if we didn't hit anything, if not then it's in mid-air
         if (tr.flFraction >= 1.0f) {
            return false; // can't reach this one
         }
      }

      // check if distance to ground drops more than step height at points between source and destination...
      Vector direction = (destination - src).normalize (); // 1 unit long
      Vector check = src, down = src;

      down.z = down.z - 1000.0f; // straight down 1000 units

      game.testLine (check, down, TraceIgnore::Monsters, m_editor, &tr);

      float lastHeight = tr.flFraction * 1000.0f; // height from ground
      distanceSq = destination.distanceSq (check); // distance from goal

      while (distanceSq > cr::sqrf (10.0f)) {
         // move 10 units closer to the goal...
         check = check + (direction * 10.0f);

         down = check;
         down.z = down.z - 1000.0f; // straight down 1000 units

         game.testLine (check, down, TraceIgnore::Monsters, m_editor, &tr);

         const float height = tr.flFraction * 1000.0f; // height from ground

         // is the current height greater than the step height?
         if (height < lastHeight - maxHeight) {
            return false; // can't get there without jumping...
         }
         lastHeight = height;
         distanceSq = destination.distanceSq (check); // distance from goal
      }
      return true;
   }
   return false;
}


bool BotGraph::isNodeReacheable (const Vector &src, const Vector &destination) const {
   return isNodeReacheableEx (src, destination, 45.0f);
}

bool BotGraph::isNodeReacheableWithJump (const Vector &src, const Vector &destination) const {
   return isNodeReacheableEx (src, destination, cv_graph_analyze_max_jump_height.as <float> ());
}

void BotGraph::frame () {
   // this function executes frame of graph operation code.

   if (game.isNullEntity (m_editor)) {
      return; // this function is only valid with editor, and in graph enabled mode.
   }

   // keep the clipping mode enabled, or it can be turned off after new round has started
   if (graph.hasEditFlag (GraphEdit::Noclip) && util.isAlive (m_editor)) {
      m_editor->v.movetype = MOVETYPE_NOCLIP;
   }

   float nearestDistanceSq = kInfiniteDistance;
   int nearestIndex = kInvalidNodeIndex;

   // check if it's time to add jump node
   if (m_jumpLearnNode) {
      if (!m_endJumpPoint) {
         if (m_editor->v.button & IN_JUMP) {
            add (NodeAddFlag::JumpStart);

            m_timeJumpStarted = game.time ();
            m_endJumpPoint = true;
         }
         else {
            m_learnVelocity = m_editor->v.velocity;
            m_learnPosition = m_editor->v.origin;
         }
      }
      else if (((m_editor->v.flags & FL_ONGROUND) || m_editor->v.movetype == MOVETYPE_FLY) && m_timeJumpStarted + 0.1f < game.time ()) {
         add (NodeAddFlag::JumpEnd);

         m_jumpLearnNode = false;
         m_endJumpPoint = false;
      }
   }

   // check if it's a auto-add-node mode enabled
   if (hasEditFlag (GraphEdit::Auto) && (m_editor->v.flags & (FL_ONGROUND | FL_PARTIALGROUND))) {
      // find the distance from the last used node
      float distanceSq = m_lastNode.distanceSq (m_editor->v.origin);

      if (distanceSq > cr::sqrf (128.0f)) {
         // check that no other reachable nodes are nearby...
         for (const auto &path : m_paths) {
            if (isNodeReacheable (m_editor->v.origin, path.origin)) {
               distanceSq = path.origin.distanceSq (m_editor->v.origin);

               if (distanceSq < nearestDistanceSq) {
                  nearestDistanceSq = distanceSq;
               }
            }
         }

         // make sure nearest node is far enough away...
         if (nearestDistanceSq >= cr::sqrf (128.0f)) {
            add (NodeAddFlag::Normal); // place a node here
         }
      }
   }
   m_facingAtIndex = getFacingIndex ();

   // reset the minimal distance changed before
   nearestDistanceSq = kInfiniteDistance;

   // now iterate through all nodes in a map, and draw required ones
   for (auto &path : m_paths) {
      const float distanceSq = path.origin.distanceSq (m_editor->v.origin);

      // check if node is within a distance, and is visible
      if (distanceSq < cr::sqrf (cv_graph_draw_distance.as <float> ())
         && ((util.isVisible (path.origin, m_editor)
            && util.isInViewCone (path.origin, m_editor)) || !util.isAlive (m_editor) || distanceSq < cr::sqrf (64.0f))) {

         // check the distance
         if (distanceSq < nearestDistanceSq) {
            nearestIndex = path.number;
            nearestDistanceSq = distanceSq;
         }

         if (path.display + 1.0f < game.time ()) {
            float nodeHeight = 0.0f;

            // check the node height
            if (path.flags & NodeFlag::Crouch) {
               nodeHeight = 36.0f;
            }
            else {
               nodeHeight = 72.0f;
            }
            const float nodeHalfHeight = nodeHeight * 0.5f;

            // all nodes are by default are green
            Color nodeColor { -1, -1, -1 };

            // colorize all other nodes
            if (path.flags & NodeFlag::Goal) {
               nodeColor = { 128, 0, 255 };
            }
            else if (path.flags & NodeFlag::Ladder) {
               nodeColor = { 128, 64, 0 };
            }
            else if (path.flags & NodeFlag::Rescue) {
               nodeColor = { 255, 255, 255 };
            }
            else if (path.flags & NodeFlag::Camp) {
               if (path.flags & NodeFlag::TerroristOnly) {
                  nodeColor = { 255, 160, 160 };
               }
               else if (path.flags & NodeFlag::CTOnly) {
                  nodeColor = { 160, 160, 255 };
               }
               else {
                  nodeColor = { 0, 255, 255 };
               }
            }
            else if (path.flags & NodeFlag::TerroristOnly) {
               nodeColor = { 255, 0, 0 };
            }
            else if (path.flags & NodeFlag::CTOnly) {
               nodeColor = { 0, 0, 255 };
            }
            else {
               nodeColor = { 0, 255, 0 };
            }

            // colorize additional flags
            Color nodeFlagColor { -1, -1, -1 };

            // check the colors
            if (path.flags & NodeFlag::Sniper) {
               nodeFlagColor = { 130, 87, 0 };
            }
            else if (path.flags & NodeFlag::NoHostage) {
               nodeFlagColor = { 255, 255, 255 };
            }
            else if (path.flags & NodeFlag::Lift) {
               nodeFlagColor = { 255, 0, 255 };
            }
            int nodeWidth = 14;

            if (exists (m_facingAtIndex) && path.number == m_facingAtIndex) {
               nodeWidth *= 2;
            }

            // draw node without additional flags
            if (nodeFlagColor.red == -1) {
               game.drawLine (m_editor, path.origin - Vector (0, 0, nodeHalfHeight), path.origin + Vector (0, 0, nodeHalfHeight), nodeWidth + 1, 0, nodeColor, 250, 0, 10);
            }

            // draw node with flags
            else {
               game.drawLine (m_editor, path.origin - Vector (0, 0, nodeHalfHeight), path.origin - Vector (0, 0, nodeHalfHeight - nodeHeight * 0.75f), nodeWidth, 0, nodeColor, 250, 0, 10); // draw basic path
               game.drawLine (m_editor, path.origin - Vector (0, 0, nodeHalfHeight - nodeHeight * 0.75f), path.origin + Vector (0, 0, nodeHalfHeight), nodeWidth, 0, nodeFlagColor, 250, 0, 10); // draw additional path
            }
            path.display = game.time ();
         }
      }
   }

   if (nearestIndex == kInvalidNodeIndex) {
      return;
   }

   // draw arrow to a some importaint nodes
   if (exists (m_findWPIndex) || exists (m_cacheNodeIndex) || exists (m_facingAtIndex)) {
      // check for drawing code
      if (m_arrowDisplayTime + 0.5f < game.time ()) {

         // finding node - pink arrow
         if (m_findWPIndex != kInvalidNodeIndex) {
            game.drawLine (m_editor, m_editor->v.origin, m_paths[m_findWPIndex].origin, 10, 0, { 128, 0, 128 }, 200, 0, 5, DrawLine::Arrow);
         }

         // cached node - yellow arrow
         if (m_cacheNodeIndex != kInvalidNodeIndex) {
            game.drawLine (m_editor, m_editor->v.origin, m_paths[m_cacheNodeIndex].origin, 10, 0, { 255, 255, 0 }, 200, 0, 5, DrawLine::Arrow);
         }

         // node user facing at - white arrow
         if (m_facingAtIndex != kInvalidNodeIndex) {
            game.drawLine (m_editor, m_editor->v.origin, m_paths[m_facingAtIndex].origin, 10, 0, { 255, 255, 255 }, 200, 0, 5, DrawLine::Arrow);
         }
         m_arrowDisplayTime = game.time ();
      }
   }

   // draw a paths, camplines and danger directions for nearest node
   if (nearestDistanceSq < cr::clamp (m_paths[nearestIndex].radius, cr::sqrf (56.0f), cr::sqrf (90.0f))
      && m_pathDisplayTime < game.time ()) {

      m_pathDisplayTime = game.time () + 0.96f;

      // create path pointer for faster access
      const auto &path = m_paths[nearestIndex];

      // draw the camplines
      if (path.flags & NodeFlag::Camp) {
         float height = 36.0f;

         // check if it's a source
         if (path.flags & NodeFlag::Crouch) {
            height = 18.0f;
         }
         const auto &source = Vector (path.origin.x, path.origin.y, path.origin.z + height); // source
         const auto &start = path.origin + Vector (path.start.x, path.start.y, 0.0f).forward () * 500.0f; // camp start
         const auto &end = path.origin + Vector (path.end.x, path.end.y, 0.0f).forward () * 500.0f; // camp end

         // draw it now
         game.drawLine (m_editor, source, start, 10, 0, { 255, 0, 0 }, 200, 0, 10);
         game.drawLine (m_editor, source, end, 10, 0, { 255, 0, 0 }, 200, 0, 10);
      }

      // draw the connections
      for (const auto &link : path.links) {
         if (link.index == kInvalidNodeIndex) {
            continue;
         }
         // jump connection
         if (link.flags & PathFlag::Jump) {
            game.drawLine (m_editor, path.origin, m_paths[link.index].origin, 5, 0, { 255, 0, 128 }, 200, 0, 10);
         }
         else if (isConnected (link.index, nearestIndex)) { // twoway connection
            game.drawLine (m_editor, path.origin, m_paths[link.index].origin, 5, 0, { 255, 255, 0 }, 200, 0, 10);
         }
         else { // oneway connection
            game.drawLine (m_editor, path.origin, m_paths[link.index].origin, 5, 0, { 255, 255, 255 }, 200, 0, 10);
         }
      }

      // now look for oneway incoming connections
      for (const auto &connected : m_paths) {
         if (isConnected (connected.number, path.number) && !isConnected (path.number, connected.number)) {
            game.drawLine (m_editor, path.origin, connected.origin, 5, 0, { 0, 192, 96 }, 200, 0, 10);
         }
      }

      // draw the radius circle
      Vector origin = (path.flags & NodeFlag::Crouch) ? path.origin : path.origin - Vector (0.0f, 0.0f, 18.0f);
      constexpr Color radiusColor { 36, 36, 255 };

      // if radius is nonzero, draw a full circle
      if (path.radius > 0.0f) {
         const float sqr = cr::sqrtf (cr::sqrf (path.radius) * 0.5f);

         game.drawLine (m_editor, origin + Vector (path.radius, 0.0f, 0.0f), origin + Vector (sqr, -sqr, 0.0f), 5, 0, radiusColor, 200, 0, 10);
         game.drawLine (m_editor, origin + Vector (sqr, -sqr, 0.0f), origin + Vector (0.0f, -path.radius, 0.0f), 5, 0, radiusColor, 200, 0, 10);

         game.drawLine (m_editor, origin + Vector (0.0f, -path.radius, 0.0f), origin + Vector (-sqr, -sqr, 0.0f), 5, 0, radiusColor, 200, 0, 10);
         game.drawLine (m_editor, origin + Vector (-sqr, -sqr, 0.0f), origin + Vector (-path.radius, 0.0f, 0.0f), 5, 0, radiusColor, 200, 0, 10);

         game.drawLine (m_editor, origin + Vector (-path.radius, 0.0f, 0.0f), origin + Vector (-sqr, sqr, 0.0f), 5, 0, radiusColor, 200, 0, 10);
         game.drawLine (m_editor, origin + Vector (-sqr, sqr, 0.0f), origin + Vector (0.0f, path.radius, 0.0f), 5, 0, radiusColor, 200, 0, 10);

         game.drawLine (m_editor, origin + Vector (0.0f, path.radius, 0.0f), origin + Vector (sqr, sqr, 0.0f), 5, 0, radiusColor, 200, 0, 10);
         game.drawLine (m_editor, origin + Vector (sqr, sqr, 0.0f), origin + Vector (path.radius, 0.0f, 0.0f), 5, 0, radiusColor, 200, 0, 10);
      }
      else {
         const float sqr = cr::sqrtf (32.0f);

         game.drawLine (m_editor, origin + Vector (sqr, -sqr, 0.0f), origin + Vector (-sqr, sqr, 0.0f), 5, 0, radiusColor, 200, 0, 10);
         game.drawLine (m_editor, origin + Vector (-sqr, -sqr, 0.0f), origin + Vector (sqr, sqr, 0.0f), 5, 0, radiusColor, 200, 0, 10);
      }

      // draw the danger directions
      if (!m_hasChanged) {
         const int dangerIndexT = practice.getIndex (Team::Terrorist, nearestIndex, nearestIndex);
         const int dangerIndexCT = practice.getIndex (Team::CT, nearestIndex, nearestIndex);

         if (exists (dangerIndexT)) {
            game.drawLine (m_editor, path.origin, m_paths[dangerIndexT].origin, 15, 0, { 255, 0, 0 }, 200, 0, 10, DrawLine::Arrow); // draw a red arrow to this index's danger point
         }
         if (exists (dangerIndexCT)) {
            game.drawLine (m_editor, path.origin, m_paths[dangerIndexCT].origin, 15, 0, { 0, 0, 255 }, 200, 0, 10, DrawLine::Arrow); // draw a blue arrow to this index's danger point
         }
      }
      static int channel = 0;

      auto sendHudMessage = [&] (Color color, float x, float y, StringRef text) {
         static hudtextparms_t textParams {};

         textParams.channel = channel++;
         textParams.x = x;
         textParams.y = y;
         textParams.effect = 0;

         textParams.r1 = textParams.r2 = static_cast <uint8_t> (color.red);
         textParams.g1 = textParams.g2 = static_cast <uint8_t> (color.green);
         textParams.b1 = textParams.b2 = static_cast <uint8_t> (color.blue);
         textParams.a1 = textParams.a2 = static_cast <uint8_t> (1);

         textParams.fadeinTime = 0.0f;
         textParams.fadeoutTime = 0.0f;
         textParams.holdTime = m_pathDisplayTime - game.time ();
         textParams.fxTime = 0.0f;

         game.sendHudMessage (m_editor, textParams, text);

         if (channel > 3) {
            channel = 0;
         }
      };

      // very helpful stuff..
      auto getNodeData = [this] (StringRef type, int node) -> String {
         String message {}, flags {};

         const auto &p = m_paths[node];
         bool jumpPoint = false;

         // iterate through connections and find, if it's a jump path
         for (const auto &link : p.links) {

            // check if we got a valid connection
            if (link.index != kInvalidNodeIndex && (link.flags & PathFlag::Jump)) {
               jumpPoint = true;
            }
         }
         flags.assignf ("%s%s%s%s%s%s%s%s%s%s%s%s",
            (p.flags & NodeFlag::Lift) ? " LIFT" : "",
            (p.flags & NodeFlag::Crouch) ? " CROUCH" : "",
            (p.flags & NodeFlag::Camp) ? " CAMP" : "",
            (p.flags & NodeFlag::TerroristOnly) ? " TERRORIST" : "",
            (p.flags & NodeFlag::CTOnly) ? " CT" : "",
            (p.flags & NodeFlag::Sniper) ? " SNIPER" : "",
            (p.flags & NodeFlag::Goal) ? " GOAL" : "",
            (p.flags & NodeFlag::Ladder) ? " LADDER" : "",
            (p.flags & NodeFlag::Rescue) ? " RESCUE" : "",
            (p.flags & NodeFlag::DoubleJump) ? " JUMPHELP" : "",
            (p.flags & NodeFlag::NoHostage) ? " NOHOSTAGE" : "", jumpPoint ? " JUMP" : "");

         if (flags.empty ()) {
            flags.assign ("(none)");
         }

         // show the information about that point
         message.assignf ("      %s node:\n"
            "       Node %d of %d, Radius: %.1f, Light: %s\n"
            "       Flags: %s\n"
            "       Origin: (%.1f, %.1f, %.1f)\n", 
            type, node,  m_paths.length () - 1,  p.radius,
            cr::fequal (p.light, kInvalidLightLevel) ? "Invalid" : strings.format ("%1.f", p.light),
            flags, p.origin.x, p.origin.y, p.origin.z
         );

         return message;
      };

      // display some information
      sendHudMessage ({ 255, 255, 255 }, 0.0f, 0.025f, getNodeData ("Current", nearestIndex));

      // check if we need to show the cached point index
      if (m_cacheNodeIndex != kInvalidNodeIndex) {
         sendHudMessage ({ 255, 255, 255 }, 0.28f, 0.16f, getNodeData ("Cached", m_cacheNodeIndex));
      }

      // check if we need to show the facing point index
      if (m_facingAtIndex != kInvalidNodeIndex) {
         sendHudMessage ({ 255, 255, 255 }, 0.28f, 0.025f, getNodeData ("Facing", m_facingAtIndex));
      }
      String timeMessage = strings.format ("      Map: %s, Time: %s\n", game.getMapName (), util.getCurrentDateTime ());

      // if node is not changed display experience also
      if (!m_hasChanged) {
         const int dangerIndexCT = practice.getIndex (Team::CT, nearestIndex, nearestIndex);
         const int dangerIndexT = practice.getIndex (Team::Terrorist, nearestIndex, nearestIndex);

         String practiceText {};
         practiceText.assignf ("      Node practice data (index / damage):\n"
            "       CT: %d / %d\n"
            "       T:  %d / %d\n\n",
            dangerIndexCT,
            dangerIndexCT != kInvalidNodeIndex ? practice.getDamage (Team::CT, nearestIndex, dangerIndexCT) : 0,
            dangerIndexT,
            dangerIndexT != kInvalidNodeIndex ? practice.getDamage (Team::Terrorist, nearestIndex, dangerIndexT) : 0
         );

         sendHudMessage ({ 255, 255, 255 }, 0.0f, 0.16f, practiceText + timeMessage);
      }
      else {
         sendHudMessage ({ 255, 255, 255 }, 0.0f, 0.16f, timeMessage);
      }
   }
}

bool BotGraph::isConnected (int index) {
   for (const auto &path : m_paths) {
      if (path.number == index) {
         continue;
      }

      for (const auto &test : path.links) {
         if (test.index == index) {
            return true;
         }
      }
   }
   return false;
}

bool BotGraph::checkNodes (bool teleportPlayer, bool onlyPaths) {
   auto teleport = [&] (const Path &path) -> void {
      if (teleportPlayer) {
         engfuncs.pfnSetOrigin (m_editor, path.origin);
         setEditFlag (GraphEdit::On | GraphEdit::Noclip);
      }
   };
   const bool showErrors = !onlyPaths;

   int terrPoints = 0;
   int ctPoints = 0;
   int goalPoints = 0;
   int rescuePoints = 0;

   for (const auto &path : m_paths) {
      int connections = 0;

      if (path.number != static_cast <int> (m_paths.index (path))) {
         if (showErrors) {
            msg ("Node %d path differs from index %d.", path.number, m_paths.index (path));
         }
         break;
      }

      for (const auto &test : path.links) {
         if (test.index != kInvalidNodeIndex) {
            if (test.index > length ()) {
               if (showErrors) {
                  msg ("Node %d connected with invalid node %d.", path.number, test.index);
               }
               return false;
            }
            ++connections;
            break;
         }
      }

      if (connections == 0) {
         if (!isConnected (path.number)) {
            if (showErrors) {
               msg ("Node %d isn't connected with any other node.", path.number);
            }
            return false;
         }
      }

      if (path.flags & NodeFlag::Camp) {
         if (path.end.empty ()) {
            if (showErrors) {
               msg ("Node %d camp-endposition not set.", path.number);
            }
            return false;
         }
      }
      else if (path.flags & NodeFlag::TerroristOnly) {
         ++terrPoints;
      }
      else if (path.flags & NodeFlag::CTOnly) {
         ++ctPoints;
      }
      else if (path.flags & NodeFlag::Goal) {
         ++goalPoints;
      }
      else if (path.flags & NodeFlag::Rescue) {
         ++rescuePoints;
      }

      for (const auto &test : path.links) {
         if (test.index != kInvalidNodeIndex) {
            if (!exists (test.index)) {
               if (showErrors) {
                  teleport (path);

                  msg ("Node %d path index %d out of range.", path.number, test.index);
               }
               teleport (path);

               return false;
            }
            else if (test.index == path.number) {
               if (showErrors) {
                  teleport (path);

                  msg ("Node %d path index %d points to itself.", path.number, test.index);
               }
               return false;
            }
         }
      }
   }

   if (!onlyPaths && game.mapIs (MapFlags::HostageRescue)) {
      if (rescuePoints == 0) {
         msg ("You didn't set a rescue point.");
         return false;
      }
   }

   // only check paths, but not necessity of different nodes
   if (!onlyPaths) {
      if (terrPoints == 0) {
         msg ("You didn't set any terrorist important point.");
         return false;
      }
      else if (ctPoints == 0) {
         msg ("You didn't set any CT important point.");
         return false;
      }
      else if (goalPoints == 0) {
         msg ("You didn't set any goal point.");
         return false;
      }
   }

   // perform DFS instead of floyd-warshall, this shit speedup this process in a bit
   const auto length = cr::min (static_cast <size_t> (kMaxNodes), m_paths.length () + 1);

   // ensure valid capacity
   assert (length > 8 && length < static_cast <size_t> (kMaxNodes));

   PathWalk walk {};
   walk.init (length);

   Array <bool> visited {};
   visited.resize (length);

   // first check incoming connectivity, initialize the "visited" table
   for (auto &visit : visited) {
      visit = false;
   }
   walk.add (0); // always check from node number 0

   while (!walk.empty ()) {
      // pop a node from the stack
      const int current = walk.first ();
      walk.shift ();

      visited[current] = true;

      for (const auto &link : m_paths[current].links) {
         int index = link.index;

         // skip this node as it's already visited
         if (exists (index) && !visited[index]) {
            visited[index] = true;
            walk.add (index);
         }
      }
   }

   for (const auto &path : m_paths) {
      if (!visited[path.number]) {
         if (showErrors) {
            msg ("Path broken from node 0 to node %d.", path.number);
         }
         teleport (path);

         return false;
      }
   }

   // then check outgoing connectivity
   Array <IntArray> outgoingPaths {}; // store incoming paths for speedup
   outgoingPaths.resize (length);

   for (const auto &path : m_paths) {
      outgoingPaths[path.number].resize (length + 1);

      for (const auto &link : path.links) {
         if (exists (link.index)) {
            outgoingPaths[link.index].push (path.number);
         }
      }
   }

   // initialize the "visited" table
   for (auto &visit : visited) {
      visit = false;
   }
   walk.clear ();
   walk.add (0); // always check from node number 0

   while (!walk.empty ()) {
      const int current = walk.first (); // pop a node from the stack
      walk.shift ();

      for (auto &outgoing : outgoingPaths[current]) {
         if (visited[outgoing]) {
            continue; // skip this node as it's already visited
         }
         visited[outgoing] = true;
         walk.add (outgoing);
      }
   }

   for (const auto &path : m_paths) {
      if (!visited[path.number]) {
         if (showErrors) {
            teleport (path);

            msg ("Path broken from node %d to node 0.", path.number);
         }
         return false;
      }
   }
   return true;
}

void BotGraph::setVisited (int index) {
   if (!exists (index)) {
      return;
   }
   if (!isVisited (index) && (m_paths[index].flags & NodeFlag::Goal)) {
      m_visitedGoals.push (index);
   }
}

void BotGraph::clearVisited () {
   m_visitedGoals.clear ();
}

bool BotGraph::isVisited (int index) {
   for (auto &visited : m_visitedGoals) {
      if (visited == index) {
         return true;
      }
   }
   return false;
}

void BotGraph::addBasic () {
   // this function creates basic node types on map

   // first of all, if map contains ladder points, create it
   game.searchEntities ("classname", "func_ladder", [&] (edict_t *ent) {
      Vector ladderLeft = ent->v.absmin;
      Vector ladderRight = ent->v.absmax;
      ladderLeft.z = ladderRight.z;

      TraceResult tr {};
      Vector up {}, down {}, front {}, back {};

      Vector diff = ((ladderLeft - ladderRight) ^ nullptr) * 15.0f;
      front = back = game.getEntityOrigin (ent);

      front = front + diff; // front
      back = back - diff; // back

      up = down = front;
      down.z = ent->v.absmax.z;

      game.testHull (down, up, TraceIgnore::Monsters, point_hull, nullptr, &tr);

      if (engfuncs.pfnPointContents (up) == CONTENTS_SOLID || !cr::fequal (tr.flFraction, 1.0f)) {
         up = down = back;
         down.z = ent->v.absmax.z;
      }

      game.testHull (down, up - Vector (0.0f, 0.0f, 1000.0f), TraceIgnore::Monsters, point_hull, nullptr, &tr);
      up = tr.vecEndPos;

      Vector point = up + Vector (0.0f, 0.0f, 39.0f);
      m_isOnLadder = true;

      do {
         if (getNearestNoBuckets (point, 50.0f) == kInvalidNodeIndex) {
            add (NodeAddFlag::NoHostage, point);
         }
         point.z += 160.0f;
      } while (point.z < down.z - 40.0f);

      point = down + Vector (0.0f, 0.0f, 38.0f);

      if (getNearestNoBuckets (point, 50.0f) == kInvalidNodeIndex) {
         add (NodeAddFlag::NoHostage, point);
      }
      m_isOnLadder = false;

      return EntitySearchResult::Continue;
   });

   auto autoCreateForEntity = [] (int type, StringRef classname) {
      game.searchEntities ("classname", classname, [&] (edict_t *ent) {
         Vector pos = game.getEntityOrigin (ent);

         TraceResult tr {};
         game.testLine (pos, pos - Vector (0.0f, 0.0f, 999.0f), TraceIgnore::Monsters, nullptr, &tr);
         tr.vecEndPos.z += 36.0f;

         if (graph.getNearestNoBuckets (tr.vecEndPos, 50.0f) == kInvalidNodeIndex) {
            graph.add (type, tr.vecEndPos);
         }
         return EntitySearchResult::Continue;
      });
   };

   autoCreateForEntity (NodeAddFlag::Normal, "info_player_deathmatch"); // then terrortist spawnpoints
   autoCreateForEntity (NodeAddFlag::Normal, "info_player_start"); // then add ct spawnpoints
   autoCreateForEntity (NodeAddFlag::Normal, "info_vip_start"); // then vip spawnpoint

   autoCreateForEntity (NodeAddFlag::Normal, "armoury_entity"); // weapons on the map ?

   autoCreateForEntity (NodeAddFlag::Rescue, "func_hostage_rescue"); // hostage rescue zone
   autoCreateForEntity (NodeAddFlag::Rescue, "info_hostage_rescue"); // hostage rescue zone (same as above)

   autoCreateForEntity (NodeAddFlag::Goal, "func_bomb_target"); // bombspot zone
   autoCreateForEntity (NodeAddFlag::Goal, "info_bomb_target"); // bombspot zone (same as above)

   autoCreateForEntity (NodeAddFlag::Goal, "hostage_entity"); // hostage entities
   autoCreateForEntity (NodeAddFlag::Goal, "monster_scientist"); // hostage entities (same as above)

   autoCreateForEntity (NodeAddFlag::Goal, "func_vip_safetyzone"); // vip rescue (safety) zone
   autoCreateForEntity (NodeAddFlag::Goal, "func_escapezone"); // terrorist escape zone
}

void BotGraph::setBombOrigin (bool reset, const Vector &pos) {
   // this function stores the bomb position as a vector

   if (!game.mapIs (MapFlags::Demolition) || !bots.isBombPlanted ()) {
      return;
   }

   if (reset) {
      m_bombOrigin.clear ();
      bots.setBombPlanted (false);

      return;
   }

   if (!pos.empty ()) {
      m_bombOrigin = pos;
      return;
   }
   bool wasFound = false;
   auto bombModel = conf.getBombModelName ();

   game.searchEntities ("classname", "grenade", [&] (edict_t *ent) {
      if (util.isModel (ent, bombModel)) {
         m_bombOrigin = game.getEntityOrigin (ent);
         wasFound = true;

         return EntitySearchResult::Break;
      }
      return EntitySearchResult::Continue;
   });

   if (!wasFound) {
      m_bombOrigin.clear ();
      bots.setBombPlanted (false);
   }
}

void BotGraph::startLearnJump () {
   m_jumpLearnNode = true;
}

void BotGraph::setSearchIndex (int index) {
   m_findWPIndex = index;

   if (exists (m_findWPIndex)) {
      msg ("Showing direction to node %d.", m_findWPIndex);
   }
   else {
      m_findWPIndex = kInvalidNodeIndex;
   }
}

BotGraph::BotGraph () {
   m_endJumpPoint = false;
   m_jumpLearnNode = false;
   m_hasChanged = false;
   m_narrowChecked = false;
   m_lightChecked = false;
   m_timeJumpStarted = 0.0f;

   m_lastJumpNode = kInvalidNodeIndex;
   m_cacheNodeIndex = kInvalidNodeIndex;
   m_findWPIndex = kInvalidNodeIndex;
   m_facingAtIndex = kInvalidNodeIndex;
   m_isOnLadder = false;

   m_editFlags = 0;

   m_pathDisplayTime = 0.0f;
   m_arrowDisplayTime = 0.0f;
   m_autoPathDistance = 250.0f;

   m_editor = nullptr;
}

void BotGraph::eraseFromBucket (const Vector &pos, int index) {
   auto &data = m_hashTable[locateBucket (pos)];

   for (size_t i = 0; i < data.length (); ++i) {
      if (data[i] == index) {
         data.erase (i, 1);
         break;
      }
   }
}

int BotGraph::locateBucket (const Vector &pos) {
   constexpr auto kWidth = 8192;

   auto hash = [&] (float axis, int32_t shift) {
      return ((static_cast <int> (axis) + kWidth) & 0x007f80) >> shift;
   };
   return hash (pos.x, 15) + hash (pos.y, 7);
}

void BotGraph::unassignPath (int from, int to) {
   auto &link = m_paths[from].links[to];

   link.index = kInvalidNodeIndex;
   link.distance = 0;
   link.flags = 0;
   link.velocity.clear ();

   setEditFlag (GraphEdit::On);
   m_hasChanged = true;
}

void BotGraph::convertFromPOD (Path &path, const PODPath &pod) const {
   path.number = pod.number;
   path.flags = pod.flags;
   path.origin = pod.origin;
   path.start = Vector (pod.csx, pod.csy, 0.0f);
   path.end = Vector (pod.cex, pod.cey, 0.0f);

   if (cv_graph_fixcamp) {
      convertCampDirection (path);
   }
   path.radius = pod.radius;
   path.light = kInvalidLightLevel;
   path.display = 0.0f;

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      path.links[i].index = pod.index[i];
      path.links[i].distance = pod.distance[i];
      path.links[i].flags = pod.conflags[i];
      path.links[i].velocity = pod.velocity[i];
   }
   path.vis.stand = 0;
   path.vis.crouch = 0;
}

void BotGraph::convertToPOD (const Path &path, PODPath &pod) {
   pod.number = path.number;
   pod.flags = path.flags;
   pod.origin = path.origin;
   pod.radius = path.radius;
   pod.csx = path.start.x;
   pod.csy = path.start.y;
   pod.cex = path.end.x;
   pod.cey = path.end.y;

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      pod.index[i] = path.links[i].index;
      pod.distance[i] = path.links[i].distance;
      pod.conflags[i] = path.links[i].flags;
      pod.velocity[i] = path.links[i].velocity;
   }
   pod.vis.stand = path.vis.stand;
   pod.vis.crouch = path.vis.crouch;
}

void BotGraph::convertCampDirection (Path &path) const {
   // this function converts old vector based camp directions to angles, note that podbotmm graph
   // are already saved with angles, and converting this stuff may result strange look directions.

   if (m_paths.empty ()) {
      return;
   }
   const auto &offset = path.origin + Vector (0.0f, 0.0f, (path.flags & NodeFlag::Crouch) ? 15.0f : 17.0f);

   path.start = (Vector (path.start.x, path.start.y, path.origin.z) - offset).angles ();
   path.end = (Vector (path.end.x, path.end.y, path.origin.z) - offset).angles ();

   path.start.x = -path.start.x;
   path.end.x = -path.end.x;

   path.start.clampAngles ();
   path.end.clampAngles ();
}
