//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

const float kInfiniteHeuristic = 65535.0f; // max out heuristic value

// a* route state
CR_DECLARE_SCOPED_ENUM (RouteState,
   Open = 0,
   Closed,
   New
)

// a * find path result
CR_DECLARE_SCOPED_ENUM (AStarResult,
   Success = 0,
   Failed,
   InternalError,
)

// node added
using NodeAdderFn = const Lambda <bool (int)> &;

// route twin node
template <typename HT> struct RouteTwin final {
public:
   int32_t index {};
   HT heuristic {};

   constexpr RouteTwin () = default;
   ~RouteTwin () = default;

public:
   constexpr RouteTwin (const int32_t &ri, const HT &rh) : index (ri), heuristic (rh) {}

public:
   constexpr bool operator < (const RouteTwin &rhs) const {
      return heuristic < rhs.heuristic;
   }

   constexpr bool operator > (const RouteTwin &rhs) const {
      return heuristic > rhs.heuristic;
   }
};

// bot heuristic functions for astar planner
class PlannerHeuristic final {
public:
   using Func = float (*) (int, int, int);

public:
   // least kills and number of nodes to goal for a team
   static float gfunctionKillsDist (int team, int currentIndex, int parentIndex);

   // least kills and number of nodes to goal for a team (when with hostage)
   static float gfunctionKillsDistCTWithHostage  (int team, int currentIndex, int parentIndex);

   // least kills to goal for a team
   static float gfunctionKills (int team, int currentIndex, int);

   // least kills to goal for a team (when with hostage)
   static float gfunctionKillsCTWithHostage (int team, int currentIndex, int parentIndex);

   // least distance for a team
   static float gfunctionPathDist (int, int currentIndex, int parentIndex);

   // least distance for a team (when with hostage)
   static float gfunctionPathDistWithHostage (int, int currentIndex, int parentIndex);

public:
   // square distance heuristic
   static float hfunctionPathDist (int index, int, int goalIndex);

   // square distance heuristic with hostages
   static float hfunctionPathDistWithHostage (int index, int, int goalIndex);

   // none heuristic
   static float hfunctionNone (int index, int, int goalIndex);
};

// A* algorithm for bots
class AStarAlgo final : public NonCopyable {
public:
   using HeuristicFn = PlannerHeuristic::Func;

public:
   struct Route {
      float g {}, f {};
      int parent { kInvalidNodeIndex };
      RouteState state { RouteState::New };
   };

private:
   BinaryHeap <RouteTwin <float>> m_routeQue {};
   Array <Route> m_routes {};

   HeuristicFn m_hcalc {};
   HeuristicFn m_gcalc {};

   int m_length {};

   Array <int> m_constructedPath {};
   Array <int> m_smoothedPath {};

private:
   // clears the currently built route
   void clearRoute ();

   // do a post-smoothing after a* finished constructing path
   void postSmooth (NodeAdderFn onAddedNode);

public:
   explicit AStarAlgo (const int length) {
      init (length);
   }

   AStarAlgo () = default;
   ~AStarAlgo () = default;

public:
   // do the pathfinding
   AStarResult find (int botTeam, int srcIndex, int destIndex, NodeAdderFn onAddedNode);

public:
   // initialize astar with valid path length
   void init (const int length) {
      m_length = length;
      clearRoute ();

      m_constructedPath.reserve (getMaxLength ());
      m_smoothedPath.reserve (getMaxLength ());

      m_constructedPath.shrink ();
      m_smoothedPath.shrink ();
   }

   // set the g heuristic
   void setG (HeuristicFn fn) {
      m_gcalc = fn;
   }

   // set the h heuristic
   void setH (HeuristicFn fn) {
      m_hcalc = fn;
   }

   // get route max length, route length should not be larger than half of map nodes
   size_t getMaxLength () const {
      return m_length / 2 + kMaxNodes / 256;
   }

public:
   // can the node can be skipped?
   static bool cantSkipNode (const int a, const int b, bool skipVisCheck = false);
};

// floyd-warshall shortest path algorithm
class FloydWarshallAlgo final {
private:
   int m_length {};

public:

   // floyd-warshall matrices
   struct Matrix {
      int16_t index { kInvalidNodeIndex };
      int16_t dist { SHRT_MAX };

   public:
      Matrix () = default;
      ~Matrix () = default;

   public:
      Matrix (const int index, const int dist) : index (static_cast <int16_t> (index)), dist (static_cast <int16_t> (dist)) {}
   };

private:
   SmallArray <Matrix> m_matrix {};

public:
   FloydWarshallAlgo () = default;
   ~FloydWarshallAlgo () = default;

private:
   // create floyd matrics
   void syncRebuild ();

   // async rebuild
   void rebuild ();

public:
   // load matrices from disk
   bool load ();

   // flush matrices to disk, so we will not rebuild them on load same map
   void save ();

   // do the pathfinding
   bool find (int srcIndex, int destIndex, NodeAdderFn onAddedNode, int *pathDistance = nullptr);

public:
   // distance between two nodes with pathfinder
   int dist (int srcIndex, int destIndex) {
      return static_cast <int> ((m_matrix.data () + (srcIndex * m_length) + destIndex)->dist);
   }
};

// dijkstra shortest path algorithm
class DijkstraAlgo final {
private:
   mutable Mutex m_cs {};

private:
   using Route = Twin <int, int>;

private:
   Array <int> m_distance {};
   Array <int> m_parent {};

   BinaryHeap <Route> m_queue {};
   int m_length {};

public:
   DijkstraAlgo () = default;
   ~DijkstraAlgo () = default;

public:
   // initialize dijkstra with valid path length
   void init (const int length);

   // do the pathfinding
   bool find (int srcIndex, int destIndex, NodeAdderFn onAddedNode, int *pathDistance = nullptr);

   // distance between two nodes with pathfinder
   int dist (int srcIndex, int destIndex);
};

// the bot path planner
class PathPlanner : public Singleton <PathPlanner> {
private:
   UniquePtr <DijkstraAlgo> m_dijkstra {};
   UniquePtr <FloydWarshallAlgo> m_floyd {};
   bool m_memoryLimitHit {};
   bool m_pathsCheckFailed {};

public:
   PathPlanner ();
   ~PathPlanner () = default;

public:
   // initialize all planners
   void init ();

   // has real path distance (instead  of distance2d) ?
   bool hasRealPathDistance () const;

public:
   // get the dijkstra algo
   decltype (auto) getDijkstra () {
      return m_dijkstra.get ();
   }

   // get the floyd algo
   decltype (auto) getFloydWarshall () {
      return m_floyd.get ();
   }

public:
   bool isPathsCheckFailed () const {
      return m_pathsCheckFailed;
   }

public:
   // do the pathfinding
   bool find (int srcIndex, int destIndex, NodeAdderFn onAddedNode, int *pathDistance = nullptr);

   // distance between two nodes with pathfinder
   float dist (int srcIndex, int destIndex);

   // get the precise distanace regardless of cvar
   float preciseDistance (int srcIndex, int destIndex);
};

CR_EXPOSE_GLOBAL_SINGLETON (PathPlanner, planner);
