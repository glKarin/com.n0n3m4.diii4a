#pragma once
// ──────────────────────────────────────────────────────────────────────────────
//  Doom-3 / BSE   –   Particle-system helpers
// ──────────────────────────────────────────────────────────────────────────────
//
//  • rvParticleParms – parameter block used by the BSE (“Basic Set of Effects”)
//    runtime for spawning particles.
//
//  • A family of free “Spawn*” helpers that know how to fill out a position/
//    scalar/normal vector based on the parameter block.
//
//  All code is self-contained except for three base headers that already exist
//  in the id Tech 4 code-base:
//
//      #include "idMath.h"     //  idMat3, idVec3, idMath::TWO_PI …
//      #include "Random.h"     //  rvRandom::flrand / irand
//      #include "RenderWorld.h"//  srfTriangles_s, R_DeriveFacePlanes(…)
//
//  If you are porting outside id Tech 4, stub these 3 headers appropriately.
//
// ──────────────────────────────────────────────────────────────────────────────
#include <stddef.h>     //  size_t
#include <stdint.h>     //  uint32_t
#include <math.h>       //  fabs, sqrt
#include <float.h>       //  FLT_MAX

// Forward declarations to avoid pulling heavy headers into every unit
class idVec3;
class idMat3;

void R_DeriveFacePlanes(struct srfTriangles_s* tris);

//───────────────────────────────────────────────────────────────────────────────
//  Free helper functions – declared here, defined in the .cpp
//───────────────────────────────────────────────────────────────────────────────
void SpawnGetNormal(idVec3* normal, const idVec3& pos,
    const idVec3* centre = NULL);

void SpawnNone1(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnNone2(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnNone3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnOne1(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnOne2(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnOne3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnPoint1(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnPoint2(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnPoint3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnLinear1(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnLinear2(float* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnLinear3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnBox1(float* out,
    const rvParticleParms& p);
void SpawnBox2(float* out,
    const rvParticleParms& p);
void SpawnBox3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnSurfaceBox1(float* out,
    const rvParticleParms& p);
void SpawnSurfaceBox2(float* out,
    const rvParticleParms& p);
void SpawnSurfaceBox3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnSphere2(float* out,
    const rvParticleParms& p);
void SpawnSphere3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnSurfaceSphere2(float* out,
    const rvParticleParms& p);
void SpawnSurfaceSphere3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnCylinder3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);
void SpawnSurfaceCylinder3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnSpiral2(float* out,
    const rvParticleParms& p);
void SpawnSpiral3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);

void SpawnModel3(idVec3* out,
    const rvParticleParms& p,
    idVec3* n = NULL,
    const idVec3* c = NULL);


void SpawnStub(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnNone1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnNone2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnNone3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnOne1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnOne2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnOne3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnPoint1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnPoint2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnPoint3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnLinear1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnLinear2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnLinear3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnBox1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnBox2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnBox3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceBox1(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceBox2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceBox3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSphere2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSphere3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceSphere2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceSphere3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnCylinder3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSurfaceCylinder3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSpiral2(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnSpiral3(float*, const rvParticleParms&, idVec3*, const idVec3*);
void SpawnModel3(float*, const rvParticleParms&, idVec3*, const idVec3*);

