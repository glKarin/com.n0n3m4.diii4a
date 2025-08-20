//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// next code is based on cs-ebot implementation, devised by EfeDursun125
class GraphAnalyze : public Singleton <GraphAnalyze> {
public:
   GraphAnalyze () = default;
   ~GraphAnalyze () = default;

private:
   float m_updateInterval {}; // time to update analyzer

   bool m_basicsCreated {}; // basics nodes were created?
   bool m_isCrouch {}; // is node to be created as crouch ?
   bool m_isAnalyzing {}; // we're in analyzing ?
   bool m_isAnalyzed {}; // current node is analyzed
   bool m_expandedNodes[kMaxNodes] {}; // all nodes expanded ?
   bool m_optimizedNodes[kMaxNodes] {}; // all nodes expanded ?

public:
   // start analysis process
   void start ();

   // update analysis process
   void update ();

   // suspend analysis
   void suspend ();

private:
   // flood with nodes
   void flood (const Vector &pos, const Vector &next, float range);

   // set update interval (keeps game from freezing)
   void setUpdateInterval ();

   // mark nodes as goals
   void markGoals ();

   // terminate analysis and save data
   void finish ();

   // optimize nodes a little
   void optimize ();

   // cleanup bad nodes
   void cleanup ();

   // show overlay message about analyzing
   void displayOverlayMessage () const;

public:

   // node should be created as crouch
   bool isCrouch () const {
      return m_isCrouch;
   }

   // is currently analyzing ?
   bool isAnalyzing () const {
      return m_isAnalyzing;
   }

   // current graph is analyzed graph ?
   bool isAnalyzed () const {
      return m_isAnalyzed;
   }

   // mark as optimized
   void markOptimized (const int index) {
      m_optimizedNodes[index] = true;
   }
};

// expose global
CR_EXPOSE_GLOBAL_SINGLETON (GraphAnalyze, analyzer);
