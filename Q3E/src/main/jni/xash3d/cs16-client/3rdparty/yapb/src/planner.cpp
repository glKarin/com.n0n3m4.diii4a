//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_path_heuristic_mode ("path_heuristic_mode", "0", "Selects the heuristic function mode. For debug purposes only.", true, 0.0f, 4.0f);
ConVar cv_path_floyd_memory_limit ("path_floyd_memory_limit", "6", "Limit maximum floyd-warshall memory (megabytes). Use Dijkstra if memory exceeds.", true, 0.0, 32.0f);
ConVar cv_path_dijkstra_simple_distance ("path_dijkstra_simple_distance", "1", "Use simple distance path calculation instead of running full Dijkstra path cycle. Used only when Floyd matrices unavailable due to memory limit.");
ConVar cv_path_astar_post_smooth ("path_astar_post_smooth", "0", "Enables post-smoothing for A*. Reduces zig-zags on paths at cost of some CPU cycles.");
ConVar cv_path_randomize_on_round_start ("path_randomize_on_round_start", "1", "Randomize pathfinding on each round start.");

float PlannerHeuristic::gfunctionKillsDist (int team, int currentIndex, int parentIndex) {
   if (parentIndex == kInvalidNodeIndex) {
      return 0.0f;
   }
   auto cost = practice.plannerGetDamage (team, currentIndex, currentIndex, true);
   const auto &current = graph[currentIndex];

   for (const auto &neighbour : current.links) {
      if (neighbour.index != kInvalidNodeIndex) {
         cost += practice.plannerGetDamage (team, neighbour.index, neighbour.index, false);
      }
   }

   if (current.flags & NodeFlag::Crouch) {
      cost *= 1.5f;
   }
   return cost;
}

float PlannerHeuristic::gfunctionKillsDistCTWithHostage (int team, int currentIndex, int parentIndex) {
   const auto &current = graph[currentIndex];

   if (current.flags & NodeFlag::NoHostage) {
      return kInfiniteHeuristic;
   }
   else if (current.flags & (NodeFlag::Crouch | NodeFlag::Ladder)) {
      return gfunctionKillsDist (team, currentIndex, parentIndex) * 500.0f;
   }
   return gfunctionKillsDist (team, currentIndex, parentIndex);
}

float PlannerHeuristic::gfunctionKills (int team, int currentIndex, int) {
   auto cost = practice.plannerGetDamage (team, currentIndex, currentIndex, false);
   const auto &current = graph[currentIndex];

   for (const auto &neighbour : current.links) {
      if (neighbour.index != kInvalidNodeIndex) {
         cost += practice.plannerGetDamage (team, neighbour.index, neighbour.index, false);
      }
   }

   if (current.flags & NodeFlag::Crouch) {
      cost *= 1.5f;
   }
   return cost;
}

auto PlannerHeuristic::gfunctionKillsCTWithHostage (int team, int currentIndex, int parentIndex) -> float {
   if (parentIndex == kInvalidNodeIndex) {
      return 0.0f;
   }
   const auto &current = graph[currentIndex];

   if (current.flags & NodeFlag::NoHostage) {
      return kInfiniteHeuristic;
   }
   else if (current.flags & (NodeFlag::Crouch | NodeFlag::Ladder)) {
      return gfunctionKills (team, currentIndex, parentIndex) * 500.0f;
   }
   return gfunctionKills (team, currentIndex, parentIndex);
}

float PlannerHeuristic::gfunctionPathDist (int, int currentIndex, int parentIndex) {
   if (parentIndex == kInvalidNodeIndex) {
      return 0.0f;
   }

   const auto &parent = graph[parentIndex];
   const auto &current = graph[currentIndex];

   for (const auto &link : parent.links) {
      if (link.index == currentIndex) {
         const auto distance = static_cast <float> (link.distance);

         // we don't like ladder or crouch point
         if (current.flags & (NodeFlag::Crouch | NodeFlag::Ladder)) {
            return distance * 1.5f;
         }
         return distance;
      }
   }
   return kInfiniteHeuristic;
}

float PlannerHeuristic::gfunctionPathDistWithHostage (int, int currentIndex, int parentIndex) {
   const auto &current = graph[currentIndex];

   if (current.flags & NodeFlag::NoHostage) {
      return kInfiniteHeuristic;
   }
   else if (current.flags & (NodeFlag::Crouch | NodeFlag::Ladder)) {
      return gfunctionPathDist (Team::Unassigned, currentIndex, parentIndex) * 500.0f;
   }
   return gfunctionPathDist (Team::Unassigned, currentIndex, parentIndex);
}

float PlannerHeuristic::hfunctionPathDist (int index, int, int goalIndex) {
   const auto &start = graph[index];
   const auto &goal = graph[goalIndex];

   const float x = start.origin.x - goal.origin.x;
   const float y = start.origin.y - goal.origin.y;
   const float z = start.origin.z - goal.origin.z;

   switch (cv_path_heuristic_mode.as <int> ()) {
   case 0:
      return cr::max (cr::max (cr::abs (x), cr::abs (y)), cr::abs (z)); // chebyshev distance

   case 1:
      return cr::abs (x) + cr::abs (y) + cr::abs (z); // manhattan distance

   case 2:
      return 0.0f; // no heuristic

   case 3: {
      const float dx = cr::abs (x);
      const float dy = cr::abs (y);
      const float dz = cr::abs (z);

      const float dmin = cr::min (cr::min (dx, dy), dz);
      const float dmax = cr::max (cr::max (dx, dy), dz);
      const float dmid = dx + dy + dz - dmin - dmax;

      const float d1 = 1.0f;
      const float d2 = cr::sqrtf (2.0f);
      const float d3 = cr::sqrtf (3.0f);

      return (d3 - d2) * dmin + (d2 - d1) * dmid + d1 * dmax; // diagonal distance
   }

   default:
   case 4:
      return 10.0f * cr::sqrtf (cr::sqrf (x) + cr::sqrf (y) + cr::sqrf (z)); // euclidean distance
   }
}

float PlannerHeuristic::hfunctionPathDistWithHostage (int index, int, int goalIndex) {
   if (graph[index].flags & NodeFlag::NoHostage) {
      return kInfiniteHeuristic;
   }
   return hfunctionPathDist (index, kInvalidNodeIndex, goalIndex);
}

float PlannerHeuristic::hfunctionNone (int index, int, int goalIndex) {
   return hfunctionPathDist (index, kInvalidNodeIndex, goalIndex) / (128.0f * 10.0f);
}

void AStarAlgo::clearRoute () {
   m_routes.resize (static_cast <size_t> (m_length));

   for (const auto &path : graph) {
      auto route = &m_routes[path.number];

      route->g = route->f = 0.0f;
      route->parent = kInvalidNodeIndex;
      route->state = RouteState::New;
   }
   m_routes.clear ();
}

bool AStarAlgo::cantSkipNode (const int a, const int b, bool skipVisCheck) {
   const auto &ag = graph[a];
   const auto &bg = graph[b];

   const bool hasZeroRadius = cr::fzero (ag.radius) || cr::fzero (bg.radius);

   if (hasZeroRadius) {
      return true;
   }

   if (!skipVisCheck) {
      const bool notVisible = !vistab.visible (ag.number, bg.number) || !vistab.visible (bg.number, ag.number);

      if (notVisible) {
         return true;
      }
   }
   const bool tooHigh = cr::abs (ag.origin.z - bg.origin.z) > 17.0f;

   if (tooHigh) {
      return true;
   }
   const bool tooNarrow = (ag.flags | bg.flags) & NodeFlag::Narrow;

   if (tooNarrow) {
      return true;
   }
   const float distanceSq = ag.origin.distanceSq (bg.origin);

   const bool tooFar = distanceSq > cr::sqrf (400.0f);
   const bool tooClose = distanceSq < cr::sqrtf (40.0f);

   if (tooFar || tooClose) {
      return true;
   }
   bool hasJumps = false;

   for (int i = 0; i < kMaxNodeLinks; ++i) {
      if ((ag.links[i].flags | bg.links[i].flags) & PathFlag::Jump) {
         hasJumps = true;
         break;
      }
   }
   return hasJumps;
}

void AStarAlgo::postSmooth (NodeAdderFn onAddedNode) {
   m_smoothedPath.clear ();

   int index = 0;
   m_smoothedPath.push (m_constructedPath.first ());

   for (size_t i = 1; i < m_constructedPath.length () - 1; ++i) {
      if (cantSkipNode (m_smoothedPath[index], m_constructedPath[i + 1])) {
         ++index;
         m_smoothedPath.push (m_constructedPath[i]);
      }
   }
   m_smoothedPath.push (m_constructedPath.last ());

   // give nodes back to bot
   for (const auto &spn : m_smoothedPath) {
      onAddedNode (spn);
   }
}

AStarResult AStarAlgo::find (int botTeam, int srcIndex, int destIndex, NodeAdderFn onAddedNode) {
   if (m_length < kMaxNodeLinks) {
      return AStarResult::InternalError; // astar needs some nodes to work with
   }

   clearRoute ();
   auto srcRoute = &m_routes[srcIndex];

   // put start node into open list
   srcRoute->g = m_gcalc (botTeam, srcIndex, kInvalidNodeIndex);
   srcRoute->f = srcRoute->g + m_hcalc (srcIndex, kInvalidNodeIndex, destIndex);
   srcRoute->state = RouteState::Open;

   m_routeQue.clear ();
   m_routeQue.emplace (srcIndex, srcRoute->g);

   const bool postSmoothPath = cv_path_astar_post_smooth && vistab.isReady ();

   // always clear constructed path
   m_constructedPath.clear ();

   // round start randomizer offset
   auto rsRandomizer = 1.0f;

   // randomize path on round start now and then
   if (cv_path_randomize_on_round_start && bots.getRoundStartTime () + 2.0f > game.time ()) {
      rsRandomizer = rg (0.5f, static_cast <float> (botTeam) * 2.0f);
   }

   while (!m_routeQue.empty ()) {
      // remove the first node from the open list
      int currentIndex = m_routeQue.pop ().index;

      // safes us from bad graph...
      if (m_routeQue.length () >= getMaxLength () - 1) {
         return AStarResult::InternalError;
      }

      // is the current node the goal node?
      if (currentIndex == destIndex) {
         // build the complete path
         do {
            if (postSmoothPath) {
               m_constructedPath.push (currentIndex);
            }
            else {
               onAddedNode (currentIndex);
            }
            currentIndex = m_routes[currentIndex].parent;
         } while (currentIndex != kInvalidNodeIndex);

         // do a post-smooth if requested
         if (postSmoothPath) {
            postSmooth (onAddedNode);
         }
         return AStarResult::Success;
      }
      auto curRoute = &m_routes[currentIndex];

      if (curRoute->state != RouteState::Open) {
         continue;
      }

      // put current node into CLOSED list
      curRoute->state = RouteState::Closed;

      // now expand the current node
      for (const auto &child : graph[currentIndex].links) {
         if (child.index == kInvalidNodeIndex) {
            continue;
         }
         auto childRoute = &m_routes[child.index];

         // calculate the F value as F = G + H
         const float g = curRoute->g + m_gcalc (botTeam, child.index, currentIndex) * rsRandomizer;
         const float h = m_hcalc (child.index, kInvalidNodeIndex, destIndex);
         const float f = plat.simd ? g + h : cr::ceilf (g + h + 0.5f);

         if (childRoute->state == RouteState::New || childRoute->f > f) {
            // put the current child into open list
            childRoute->parent = currentIndex;
            childRoute->state = RouteState::Open;

            childRoute->g = g;
            childRoute->f = f;

            m_routeQue.emplace (child.index, g);
         }
      }
   }
   return AStarResult::Failed;
}

void FloydWarshallAlgo::rebuild () {
   m_length = graph.length ();
   m_matrix.resize (static_cast <size_t> (cr::sqrf (m_length)));

   worker.enqueue ([this] () {
      syncRebuild ();
   });
}

void FloydWarshallAlgo::syncRebuild () {
   auto matrix = m_matrix.data ();

   // re-initialize matrix every load
   for (int i = 0; i < m_length; ++i) {
      for (int j = 0; j < m_length; ++j) {
         *(matrix + (i * m_length) + j) = { kInvalidNodeIndex, SHRT_MAX };
      }
   }

   for (int i = 0; i < m_length; ++i) {
      for (const auto &link : graph[i].links) {
         if (!graph.exists (link.index)) {
            continue;
         }
         *(matrix + (i * m_length) + link.index) = { link.index, link.distance };
      }
   }

   for (int i = 0; i < m_length; ++i) {
      (matrix + (i * m_length) + i)->dist = 0;
   }

   for (int k = 0; k < m_length; ++k) {
      for (int i = 0; i < m_length; ++i) {
         for (int j = 0; j < m_length; ++j) {
            int distance = (matrix + (i * m_length) + k)->dist + (matrix + (k * m_length) + j)->dist;

            if (distance < (matrix + (i * m_length) + j)->dist) {
               *(matrix + (i * m_length) + j) = { (matrix + (i * m_length) + k)->index, distance };
            }
         }
      }
   }
   save (); // save path matrix to file for faster access
}

bool FloydWarshallAlgo::load () {
   m_length = graph.length ();

   if (!m_length) {
      return false;
   }
   const bool dataLoaded = bstor.load <Matrix> (m_matrix);

   // do not rebuild if loaded
   if (dataLoaded) {
      return true;
   }
   rebuild (); // rebuilds matrix

   return true;
}

void FloydWarshallAlgo::save () const {
   if (!m_length) {
      return;
   }
   bstor.save <Matrix> (m_matrix);
}

bool FloydWarshallAlgo::find (int srcIndex, int destIndex, NodeAdderFn onAddedNode, int *pathDistance) {
   onAddedNode (srcIndex);

   while (srcIndex != destIndex) {
      srcIndex = (m_matrix.data () + (srcIndex * m_length) + destIndex)->index;

      if (srcIndex < 0) {
         return false;
      }

      if (!onAddedNode (srcIndex)) {
         return true;
      }
   }

   // only fill path distance on full path
   if (pathDistance != nullptr) {
      *pathDistance = dist (srcIndex, destIndex);
   }
   return true;
}

void DijkstraAlgo::init (const int length) {
   m_length = length;

   const auto ulength = static_cast <size_t> (length);

   m_distance.resize (ulength);
   m_parent.resize (ulength);

   m_distance.shrink ();
   m_parent.shrink ();
}

bool DijkstraAlgo::find (int srcIndex, int destIndex, NodeAdderFn onAddedNode, int *pathDistance) {
   MutexScopedLock lock (m_cs);

   m_queue.clear ();

   m_parent.fill (kInvalidNodeIndex);
   m_distance.fill (kInfiniteDistanceLong);

   m_queue.emplace (0, srcIndex);
   m_distance[srcIndex] = 0;

   while (!m_queue.empty ()) {
      const auto &route = m_queue.pop ();
      auto current = route.second;

      // finished search
      if (current == destIndex) {
         break;
      }

      if (m_distance[current] != route.first) {
         continue;
      }

      for (const auto &link : graph[current].links) {
         if (link.index != kInvalidNodeIndex) {
            const auto dlink = m_distance[current] + link.distance;

            if (dlink < m_distance[link.index]) {
               m_distance[link.index] = dlink;
               m_parent[link.index] = current;

               m_queue.emplace (m_distance[link.index], link.index);
            }
         }
      }
   }
   static SmallArray <int> pathInReverse {};
   pathInReverse.clear ();

   for (int i = destIndex; i != kInvalidNodeIndex; i = m_parent[i]) {
      pathInReverse.emplace (i);
   }
   pathInReverse.reverse ();

   for (const auto &node : pathInReverse) {
      if (!onAddedNode (node)) {
         break;
      }
   }

   // always fill path distance if we're need to
   if (pathDistance != nullptr) {
      *pathDistance = m_distance[destIndex];
   }
   return m_distance[destIndex] < kInfiniteDistanceLong;
}

int DijkstraAlgo::dist (int srcIndex, int destIndex) {
   int pathDistance = 0;

   find (srcIndex, destIndex, [&] (int) {
      return true;
   }, &pathDistance);

   return pathDistance;
}

PathPlanner::PathPlanner () {
   m_dijkstra = cr::makeUnique <DijkstraAlgo> ();
   m_floyd = cr::makeUnique <FloydWarshallAlgo> ();
}

void PathPlanner::init () {
   const int length = graph.length ();

   const float limitInMb = cv_path_floyd_memory_limit.as <float> ();
   const float memoryUse = static_cast <float> (sizeof (FloydWarshallAlgo::Matrix) * cr::sqrf (static_cast <size_t> (length)) / 1024 / 1024);

   // ensure nodes are valid
   m_pathsCheckFailed = !graph.checkNodes (false, true);

   // if we're have too much memory for floyd matrices, planner will use dijkstra or uniform planner for other than pathfinding needs
   if (memoryUse > limitInMb) {
      m_memoryLimitHit = true;

      // we're need floyd tables when graph has failed sanity checks
      if (m_pathsCheckFailed) {
         m_memoryLimitHit = false;
      }
   }
   m_dijkstra->init (length);

   // load (re-create) floyds, if we're not hitting memory limits
   if (!m_memoryLimitHit) {
      m_floyd->load ();
   }
}

bool PathPlanner::hasRealPathDistance () const {
   return !m_memoryLimitHit || !cv_path_dijkstra_simple_distance;
}

bool PathPlanner::find (int srcIndex, int destIndex, NodeAdderFn onAddedNode, int *pathDistance) {
   if (!graph.exists (srcIndex) || !graph.exists (destIndex)) {
      return false;
   }
   // limit hit, use dijkstra
   if (m_memoryLimitHit) {
      return m_dijkstra->find (srcIndex, destIndex, onAddedNode, pathDistance);
   }
   return m_floyd->find (srcIndex, destIndex, onAddedNode, pathDistance);
}

float PathPlanner::dist (int srcIndex, int destIndex) {
   if (!graph.exists (srcIndex) || !graph.exists (destIndex)) {
      return kInfiniteDistanceLong;
   }

   if (srcIndex == destIndex) {
      return 1;
   }

   // limit hit, use dijkstra
   if (m_memoryLimitHit) {
      if (cv_path_dijkstra_simple_distance) {
         return graph[srcIndex].origin.distance2d (graph[destIndex].origin);
      }
      return static_cast <float> (m_dijkstra->dist (srcIndex, destIndex));
   }
   return static_cast <float> (m_floyd->dist (srcIndex, destIndex));
}

float PathPlanner::preciseDistance (int srcIndex, int destIndex) {
   // limit hit, use dijkstra
   if (m_memoryLimitHit) {
      return static_cast <float> (m_dijkstra->dist (srcIndex, destIndex));
   }
   return static_cast <float> (m_floyd->dist (srcIndex, destIndex));
}
