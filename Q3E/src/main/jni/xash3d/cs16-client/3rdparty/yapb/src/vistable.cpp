//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

void GraphVistable::rebuild () {
   if (!m_rebuild) {
      return;
   }
   m_length = graph.length ();

   // stop generation if graph has changed, or erased
   if (!m_length || graph.hasChanged ()) {
      m_rebuild = false;
      return;
   }
   TraceResult tr {};
   uint8_t res {}, shift {};

   if (!graph.exists (m_sliceIndex)) {
      m_sliceIndex = 0;
   }

   if (!graph.exists (m_curIndex)) {
      m_curIndex = 0;
   }
   const auto &vis = graph[m_curIndex];

   auto sourceCrouch = vis.origin;
   auto sourceStand = vis.origin;

   if (vis.flags & NodeFlag::Crouch) {
      sourceCrouch.z += 12.0f;
      sourceStand.z += 18.0f + 28.0f;
   }
   else {
      sourceCrouch.z += -18.0f + 12.0f;
      sourceStand.z += 28.0f;
   }
   auto end = m_sliceIndex + rg (250, 400);

   if (end > m_length) {
      end = m_length;
   }
   uint16_t standCount = 0, crouchCount = 0;

   for (int i = m_sliceIndex; i < end; ++i) {
      const auto &path = graph[i];

      // first check ducked visibility
      Vector dest = path.origin;

      game.testLine (sourceCrouch, dest, TraceIgnore::Monsters, nullptr, &tr);

      // check if line of sight to object is not blocked (i.e. visible)
      if (!cr::fequal (tr.flFraction, 1.0f)) {
         res = VisIndex::Stand;
      }
      else {
         res = VisIndex::None;
      }
      res <<= 1;

      game.testLine (sourceStand, dest, TraceIgnore::Monsters, nullptr, &tr);

      // check if line of sight to object is not blocked (i.e. visible)
      if (!cr::fequal (tr.flFraction, 1.0f)) {
         res |= VisIndex::Stand;
      }

      if (res != VisIndex::None) {
         dest = path.origin;

         // first check ducked visibility
         if (path.flags & NodeFlag::Crouch) {
            dest.z += 18.0f + 28.0f;
         }
         else {
            dest.z += 28.0f;
         }
         game.testLine (sourceCrouch, dest, TraceIgnore::Monsters, nullptr, &tr);

         // check if line of sight to object is not blocked (i.e. visible)
         if (!cr::fequal (tr.flFraction, 1.0f)) {
            res |= VisIndex::Crouch;
         }
         else {
            res &= VisIndex::Stand;
         }
         game.testLine (sourceStand, dest, TraceIgnore::Monsters, nullptr, &tr);

         // check if line of sight to object is not blocked (i.e. visible)
         if (!cr::fequal (tr.flFraction, 1.0f)) {
            res |= VisIndex::Stand;
         }
         else {
            res &= VisIndex::Crouch;
         }
      }
      shift = static_cast <uint8_t> ((path.number % 4) << 1);

      m_vistable[vis.number * m_length + path.number] &= ~static_cast <uint8_t> (VisIndex::Any << shift);
      m_vistable[vis.number * m_length + path.number] |= res << shift;

      if (!(res & VisIndex::Crouch)) {
         ++crouchCount;
      }

      if (!(res & VisIndex::Stand)) {
         ++standCount;
      }
   }
   graph[vis.number].vis.crouch = crouchCount;
   graph[vis.number].vis.stand = standCount;

   if (end == m_length) {
      m_sliceIndex = 0;
      m_curIndex++;
   }
   else {
      m_sliceIndex += rg (250, 400);
   }
   auto notifyProgress = [] (int value) {
      game.print ("Rebuilding vistable... %d%% done.", value);
   };

   // notify host about rebuilding
   if (m_notifyMsgTimestamp > 0.0f && m_notifyMsgTimestamp < game.time () && end == m_length) {
      notifyProgress (m_curIndex * 100 / m_length);
      m_notifyMsgTimestamp = game.time () + 1.0f;
   }

   if (m_curIndex == m_length && end == m_length) {
      notifyProgress (100);

      m_rebuild = false;
      m_notifyMsgTimestamp = 0.0f;

      save ();
   }
}

void GraphVistable::startRebuild () {
   m_rebuild = true;
   m_notifyMsgTimestamp = game.time ();
}

bool GraphVistable::visible (int srcIndex, int destIndex, VisIndex vis) {
   if (!graph.exists (srcIndex) || !graph.exists (destIndex)) {
      return false;
   }
   return !(((m_vistable[srcIndex * m_length + destIndex] >> ((destIndex % 4) << 1)) & vis) == vis);
}

void GraphVistable::load () {
   m_rebuild = true;
   m_length = graph.length ();

   m_sliceIndex = 0;
   m_curIndex = 0;
   m_notifyMsgTimestamp = 0.0f;

   if (!m_length) {
      return;
   }
   const bool dataLoaded = bstor.load <VisStorage> (m_vistable);

   // if loaded, do not recalculate visibility
   if (dataLoaded) {
      m_rebuild = false;
   }
   else {
      m_vistable.resize (static_cast <size_t> (cr::sqrf (m_length)));
      m_notifyMsgTimestamp = game.time ();
   }
}

void GraphVistable::save () const {
   if (!graph.length () || graph.hasChanged () || m_rebuild) {
      return;
   }
   bstor.save <VisStorage> (m_vistable);
}
