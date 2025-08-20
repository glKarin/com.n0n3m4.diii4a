//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_graph_analyze_auto_start ("graph_analyze_auto_start", "1", "Autostart analyzer if all other cases are failed.");
ConVar cv_graph_analyze_auto_save ("graph_analyze_auto_save", "1", "Auto save results of analysis to graph file. And re-add bots.");
ConVar cv_graph_analyze_distance ("graph_analyze_distance", "64", "The minimum distance to keep nodes from each other.", true, 42.0f, 128.0f);
ConVar cv_graph_analyze_max_jump_height ("graph_analyze_max_jump_height", "44", "Max jump height to test if next node will be unreachable.", true, 44.0f, 64.0f);
ConVar cv_graph_analyze_fps ("graph_analyze_fps", "30.0", "The FPS at which analyzer process is running. This keeps game from freezing during analyzing.", true, 25.0f, 99.0f);
ConVar cv_graph_analyze_clean_paths_on_finish ("graph_analyze_clean_paths_on_finish", "1", "Specifies if analyzer should clean the unnecessary paths upon finishing.");
ConVar cv_graph_analyze_optimize_nodes_on_finish ("graph_analyze_optimize_nodes_on_finish", "1", "Specifies if analyzer should merge some near-placed nodes with much of connections together.");
ConVar cv_graph_analyze_mark_goals_on_finish ("graph_analyze_mark_goals_on_finish", "1", "Specifies if analyzer should mark nodes as map goals automatically upon finish.");

void GraphAnalyze::start () {
   // start analyzer in few seconds after level initialized
   if (cv_graph_analyze_auto_start) {
      m_updateInterval = game.time () + 3.0f;
      m_basicsCreated = false;

      // set as we're analyzing
      m_isAnalyzing = true;

      // silence all graph messages
      graph.setMessageSilence (true);

      // set all nodes as not expanded
      for (auto &expanded : m_expandedNodes) {
         expanded = false;
      }

      // set all nodes as not optimized
      for (auto &optimized : m_optimizedNodes) {
         optimized = false;
      }
      ctrl.msg ("Starting map analysis.");
   }
   else {
      m_updateInterval = 0.0f;
   }
}

void GraphAnalyze::update () {
   if (cr::fzero (m_updateInterval) || !m_isAnalyzing) {
      return;
   }

   if (m_updateInterval >= game.time ()) {
      return;
   }
   else {
      displayOverlayMessage ();
   }

   // add basic nodes
   if (!m_basicsCreated) {
      graph.addBasic ();
      m_basicsCreated = true;
   }

   for (int i = 0; i < graph.length (); ++i) {
      if (m_updateInterval >= game.time ()) {
         return;
      }

      if (!graph.exists (i)) {
         return;
      }

      if (m_expandedNodes[i]) {
         continue;
      }

      m_expandedNodes[i] = true;
      setUpdateInterval ();

      auto pos = graph[i].origin;
      const auto range = cv_graph_analyze_distance.as <float> ();

      for (int dir = 1; dir < kMaxNodeLinks; ++dir) {
         switch (dir) {
         case 1:
            flood (pos, { pos.x + range, pos.y, pos.z }, range);
            break;

         case 2:
            flood (pos, { pos.x - range, pos.y, pos.z }, range);
            break;

         case 3:
            flood (pos, { pos.x, pos.y + range, pos.z }, range);
            break;

         case 4:
            flood (pos, { pos.x, pos.y - range, pos.z }, range);
            break;

         case 5:
            flood (pos, { pos.x + range, pos.y, pos.z + 128.0f }, range);
            break;

         case 6:
            flood (pos, { pos.x - range, pos.y, pos.z + 128.0f }, range);
            break;

         case 7:
            flood (pos, { pos.x, pos.y + range, pos.z + 128.0f }, range);
            break;

         case 8:
            flood (pos, { pos.x, pos.y - range, pos.z + 128.0f }, range);
            break;
         }
      }
   }

   // finish generation if no updates occurred recently
   if (m_updateInterval + 2.0f < game.time ()) {
      finish ();
      return;
   }
}

void GraphAnalyze::suspend () {
   m_updateInterval = 0.0f;
   m_isAnalyzing = false;
   m_isAnalyzed = false;
   m_basicsCreated = false;
}

void GraphAnalyze::finish () {
   // run optimization on finish
   optimize ();

   // mark goal nodes
   markGoals ();

   m_isAnalyzed = true;
   m_isAnalyzing = false;
   m_updateInterval = 0.0f;

   // un-silence all graph messages
   graph.setMessageSilence (false);

   ctrl.msg ("Completed map analysis.");

   // auto save bots graph
   if (cv_graph_analyze_auto_save) {
      if (!graph.saveGraphData ()) {
         ctrl.msg ("Can't save analyzed graph. Internal error.");
         return;
      }

      if (!graph.loadGraphData ()) {
         ctrl.msg ("Can't load analyzed graph. Internal error.");
         return;
      }
      vistab.startRebuild ();
      ctrl.enableDrawModels (false);

      cv_quota.revert ();
   }
}

void GraphAnalyze::optimize () {
   if (graph.length () == 0) {
      return;
   }

   if (!cv_graph_analyze_optimize_nodes_on_finish) {
      return;
   }
   cleanup ();

   auto smooth = [] (const Array <int> &nodes) {
      Vector result {};

      for (const auto &node : nodes) {
         result += graph[node].origin;
      }
      result /= kMaxNodeLinks;
      result.z = graph[nodes.first ()].origin.z;

      return result;
   };

   // set all nodes as not optimized
   for (auto &optimized : m_optimizedNodes) {
      optimized = false;
   }

   for (int i = 0; i < graph.length (); ++i) {
      if (m_optimizedNodes[i]) {
         continue;
      }
      const auto &path = graph[i];
      Array <int> indexes {};

      for (const auto &link : path.links) {
         if (graph.exists (link.index) && !m_optimizedNodes[link.index]
            && !AStarAlgo::cantSkipNode (path.number, link.index, true)) {

            indexes.emplace (link.index);
         }
      }

      // we're have max out node links
      if (indexes.length () >= kMaxNodeLinks) {
         const Vector &pos = smooth (indexes);

         for (const auto &index : indexes) {
            graph.erase (index);
         }
         graph.add (NodeAddFlag::Normal, pos);
      }
   }

   // clear the useless connections
   if (cv_graph_analyze_clean_paths_on_finish) {
      for (auto i = 0; i < graph.length (); ++i) {
         graph.clearConnections (i);
      }
   }
}

void GraphAnalyze::cleanup () {
   int connections = 0; // clean bad paths

   for (auto i = 0; i < graph.length (); ++i) {
      connections = 0;

      for (const auto &link : graph[i].links) {
         if (link.index != kInvalidNodeIndex) {
            if (link.index > graph.length ()) {
               graph.erase (i);
            }
            ++connections;
         }
      }

      // no connections
      if (!connections) {
         graph.erase (i);
      }

      // path number differs
      if (graph[i].number != i) {
         graph.erase (i);
      }

      for (const auto &link : graph[i].links) {
         if (link.index != kInvalidNodeIndex) {
            if (link.index >= graph.length () || link.index < -kInvalidNodeIndex) {
               graph.erase (i);
            }
            else if (link.index == i) {
               graph.erase (i);
            }
         }
      }

      if (!graph.isConnected (i)) {
         graph.erase (i);
      }
   }
}

void GraphAnalyze::displayOverlayMessage () const {
   auto listenserverEdict = game.getLocalEntity ();

   if (game.isNullEntity (listenserverEdict) || !m_isAnalyzing) {
      return;
   }
   constexpr StringRef analyzeHudMesssage =
      "+-----------------------------------------------------------------+\n"
      "         Map analysis for bots is in progress. Please Wait..       \n"
      "+-----------------------------------------------------------------+\n";

   static hudtextparms_t textParams {};

   textParams.channel = 1;
   textParams.x = -1.0f;
   textParams.y = -1.0f;
   textParams.effect = 1;

   textParams.r1 = textParams.r2 = static_cast <uint8_t> (255);
   textParams.g1 = textParams.g2 = static_cast <uint8_t> (31);
   textParams.b1 = textParams.b2 = static_cast <uint8_t> (75);
   textParams.a1 = textParams.a2 = static_cast <uint8_t> (0);

   textParams.fadeinTime = 0.0078125f;
   textParams.fadeoutTime = 0.0078125f;
   textParams.holdTime = m_updateInterval;
   textParams.fxTime = 0.25f;

   game.sendHudMessage (listenserverEdict, textParams, analyzeHudMesssage);
}

void GraphAnalyze::flood (const Vector &pos, const Vector &next, float range) {
   range *= 0.75f;

   TraceResult tr {};
   game.testHull (pos, { next.x, next.y, next.z + 19.0f }, TraceIgnore::Monsters, head_hull, nullptr, &tr);

   // we're can't reach next point
   if (!cr::fequal (tr.flFraction, 1.0f) && !util.isBreakableEntity (tr.pHit)) {
      return;
   }

   // we're have something in around, skip
   if (graph.exists (graph.getForAnalyzer (tr.vecEndPos, range))) {
      return;
   }
   game.testHull (tr.vecEndPos, { tr.vecEndPos.x, tr.vecEndPos.y, tr.vecEndPos.z - 999.0f }, TraceIgnore::Monsters, head_hull, nullptr, &tr);

   // ground is away for a break
   if (cr::fequal (tr.flFraction, 1.0f)) {
      return;
   }
   Vector nextPos = { tr.vecEndPos.x, tr.vecEndPos.y, tr.vecEndPos.z + 19.0f };

   const int endIndex = graph.getForAnalyzer (nextPos, range);
   const int targetIndex = graph.getNearestNoBuckets (nextPos, 250.0f);

   if (graph.exists (endIndex) || !graph.exists (targetIndex)) {
      return;
   }
   auto targetPos = graph[targetIndex].origin;

   // re-check there's nothing nearby, and add something we're want
   if (!graph.exists (graph.getNearestNoBuckets (nextPos, range))) {
      m_isCrouch = false;
      game.testLine (nextPos, { nextPos.x, nextPos.y, nextPos.z + 36.0f }, TraceIgnore::Monsters, nullptr, &tr);

      if (!cr::fequal (tr.flFraction, 1.0f)) {
         m_isCrouch = true;
      }
      auto testPos = m_isCrouch ? Vector { nextPos.x, nextPos.y, nextPos.z - 18.0f } : nextPos;

      if ((graph.isNodeReacheable (targetPos, testPos)
         && graph.isNodeReacheable (testPos, targetPos)) || (graph.isNodeReacheableWithJump (testPos, targetPos)
            && graph.isNodeReacheableWithJump (targetPos, testPos))) {
         graph.add (NodeAddFlag::Normal, m_isCrouch ? Vector { nextPos.x, nextPos.y, nextPos.z - 9.0f } : nextPos);
      }
   }
}

void GraphAnalyze::setUpdateInterval () {
   const auto frametime = globals->frametime;

   if ((cv_graph_analyze_fps.as <float> () + frametime) <= 1.0f / frametime) {
      m_updateInterval = game.time () + frametime * 0.06f;
   }
}

void GraphAnalyze::markGoals () {
   if (!cv_graph_analyze_mark_goals_on_finish) {
      return;
   }

   auto updateNodeFlags = [] (int type, StringRef classname) {
      game.searchEntities ("classname", classname, [&] (edict_t *ent) {
         for (auto &path : graph) {
            const auto &bb = path.origin + Vector (1.0f, 1.0f, 1.0f);

            if (ent->v.absmin.x > bb.x || ent->v.absmin.y > bb.y) {
               continue;
            }

            if (ent->v.absmax.x < bb.x || ent->v.absmax.y < bb.y) {
               continue;
            }
            path.flags |= type;
         }
         return EntitySearchResult::Continue;
      });
   };

   if (game.mapIs (MapFlags::Demolition)) {
      updateNodeFlags (NodeFlag::Goal, "func_bomb_target"); // bombspot zone
      updateNodeFlags (NodeFlag::Goal, "info_bomb_target"); // bombspot zone (same as above)
   }
   else if (game.mapIs (MapFlags::HostageRescue)) {
      updateNodeFlags (NodeFlag::Rescue, "func_hostage_rescue"); // hostage rescue zone
      updateNodeFlags (NodeFlag::Rescue, "info_hostage_rescue"); // hostage rescue zone (same as above)
      updateNodeFlags (NodeFlag::Rescue, "info_player_start"); // then add ct spawnpoints

      updateNodeFlags (NodeFlag::Goal, "hostage_entity"); // hostage entities
      updateNodeFlags (NodeFlag::Goal, "monster_scientist"); // hostage entities (same as above)
   }
   else if (game.mapIs (MapFlags::Assassination)) {
      updateNodeFlags (NodeFlag::Goal, "func_vip_safetyzone"); // vip rescue (safety) zone
      updateNodeFlags (NodeFlag::Goal, "func_escapezone"); // terrorist escape zone
   }
}
