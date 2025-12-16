//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

// view frustum for bots
class Frustum : public Singleton <Frustum> {
public:
   struct Plane {
      Vector normal {};
      Vector point {};
      float result {};
   };

   enum class PlaneSide : int {
      Top = 0,
      Bottom,
      Left,
      Right,
      Near,
      Far,
      Num
   };

public:
   using Planes = Plane[static_cast <int> (PlaneSide::Num)];

public:
   static constexpr float kFov = 75.0f;
   static constexpr float kAspectRatio = 16.0f / 9.0f;
   static constexpr float kMaxViewDistance = 4096.0f;
   static constexpr float kMinViewDistance = 2.0f;

private:
   float m_farHeight {}; // height of the far frustum
   float m_farWidth {}; // width of the far frustum
   float m_nearHeight {}; // height of the near frustum
   float m_nearWidth {}; // width of the near frustum

public:
   explicit Frustum () {
      m_nearHeight = 2.0f * cr::tanf (kFov * deg2rad (1.0f) * 0.5f) * kMinViewDistance;
      m_nearWidth = m_nearHeight * kAspectRatio;

      m_farHeight = 2.0f * cr::tanf (kFov * deg2rad (1.0f) * 0.5f) * kMaxViewDistance;
      m_farWidth = m_farHeight * kAspectRatio;
   }

public:
   // updates bot view frustum
   void calculate (Planes &planes, const Vector &viewAngle, const Vector &viewOffset) const;

   // check if object inside frustum plane
   bool isObjectInsidePlane (const Plane &plane, const Vector &center, float height, float radius) const;

   // check if entity origin inside view plane
   bool check (const Planes &planes, edict_t *ent) const;
};

// declare global frustum data
CR_EXPOSE_GLOBAL_SINGLETON (Frustum, frustum);
