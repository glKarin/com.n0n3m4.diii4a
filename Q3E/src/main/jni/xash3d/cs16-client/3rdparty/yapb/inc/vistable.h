//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// limits for storing practice data
CR_DECLARE_SCOPED_ENUM_TYPE (VisIndex, int32_t,
   None = 0,
   Stand = 1,
   Crouch = 2,
   Any = Stand | Crouch
)

// defines visibility count
struct PathVis {
   uint16_t stand {}, crouch {};
};

class GraphVistable final : public Singleton <GraphVistable> {
public:
   using VisStorage = uint8_t;

private:
   SmallArray <VisStorage> m_vistable {};
   bool m_rebuild {};
   int m_length {};

   int m_curIndex {};
   int m_sliceIndex {};

   float m_notifyMsgTimestamp {};

public:
   explicit GraphVistable () = default;
   ~GraphVistable () = default;

public:
   bool visible (int srcIndex, int destIndex, VisIndex vis = VisIndex::Any);

   void load ();
   void save ();
   void rebuild ();

public:

   // triggers re-check for all the nodes
   void startRebuild ();

   // ready to use ?
   bool isReady () const {
      return !m_rebuild;
   }
};

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (GraphVistable, vistab);

