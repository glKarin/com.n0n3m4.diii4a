/*
===========================================================================

QUAKE 4 BSE CODE RECREATION EFFORT - (c) 2025 by Justin Marshall(IceColdDuke).

QUAKE 4 BSE CODE RECREATION EFFORT is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QUAKE 4 BSE CODE RECREATION EFFORT is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QUAKE 4 BSE CODE RECREATION EFFORT.  If not, see <http://www.gnu.org/licenses/>.

In addition, the QUAKE 4 BSE CODE RECREATION EFFORT is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "BSE.h"

rvParticleParms::SpawnFunc rvParticleParms::spawnFunctions[48] =
{
    /*00*/ &SpawnStub,
    /*01*/ &SpawnNone1,
    /*02*/ &SpawnNone2,
    /*03*/ &SpawnNone3,
    /*04*/ &SpawnStub,
    /*05*/ &SpawnOne1,
    /*06*/ &SpawnOne2,
    /*07*/ &SpawnOne3,
    /*08*/ &SpawnStub,
    /*09*/ &SpawnPoint1,
    /*10*/ &SpawnPoint2,
    /*11*/ &SpawnPoint3,
    /*12*/ &SpawnStub,
    /*13*/ &SpawnLinear1,
    /*14*/ &SpawnLinear2,
    /*15*/ &SpawnLinear3,
    /*16*/ &SpawnStub,
    /*17*/ &SpawnBox1,
    /*18*/ &SpawnBox2,
    /*19*/ &SpawnBox3,
    /*20*/ &SpawnStub,
    /*21*/ &SpawnSurfaceBox1,
    /*22*/ &SpawnSurfaceBox2,
    /*23*/ &SpawnSurfaceBox3,
    /*24*/ &SpawnStub,
    /*25*/ &SpawnBox1,
    /*26*/ &SpawnSphere2,
    /*27*/ &SpawnSphere3,
    /*28*/ &SpawnStub,
    /*29*/ &SpawnSurfaceBox1,
    /*30*/ &SpawnSurfaceSphere2,
    /*31*/ &SpawnSurfaceSphere3,
    /*32*/ &SpawnStub,
    /*33*/ &SpawnBox1,
    /*34*/ &SpawnSphere2,
    /*35*/ &SpawnCylinder3,
    /*36*/ &SpawnStub,
    /*37*/ &SpawnSurfaceBox1,
    /*38*/ &SpawnSurfaceSphere2,
    /*39*/ &SpawnSurfaceCylinder3,
    /*40*/ &SpawnStub,
    /*41*/ &SpawnStub,
    /*42*/ &SpawnSpiral2,
    /*43*/ &SpawnSpiral3,
    /*44*/ &SpawnStub,
    /*45*/ &SpawnStub,
    /*46*/ &SpawnStub,
    /*47*/ &SpawnModel3
};

extern const float kEpsilon = 0.001f;   // ODR definition
extern const float kHalf = 0.5f;

// ──────────────────────────────────────────────────────────────────────────────
//  rvParticleParms -- tiny helpers
// ──────────────────────────────────────────────────────────────────────────────
bool rvParticleParms::Compare(const rvParticleParms& rhs) const
{
    if (mSpawnType != rhs.mSpawnType || mFlags != rhs.mFlags)
        return false;

    const idVec3 dMin = mMins - rhs.mMins;
    const idVec3 dMax = mMaxs - rhs.mMaxs;

    return  fabs(dMin.x) <= kEpsilon &&
        fabs(dMin.y) <= kEpsilon &&
        fabs(dMin.z) <= kEpsilon &&
        fabs(dMax.x) <= kEpsilon &&
        fabs(dMax.y) <= kEpsilon &&
        fabs(dMax.z) <= kEpsilon;
}

//───────────────────────────────────────────────────────────────────────────────
void rvParticleParms::HandleRelativeParms(float* death, float* init, int count) const
{
    if ((mFlags & PPF_RELATIVE/* 8u */) == 0)        // “RELATIVE” flag?
        return;

    //  Quick vectorised walk – identical arithmetic, nicer to read
    for (int i = 0; i < count; ++i)
        death[i] += init[i];
}

//───────────────────────────────────────────────────────────────────────────────
void rvParticleParms::GetMinsMaxs(idVec3& mins, idVec3& maxs) const
{
    mins = idVec3(0.0f, 0.0f, 0.0f);
    maxs = idVec3(0.0f, 0.0f, 0.0f);

    switch (mSpawnType)
    {
        //  Constant-value spawns (NONE / ONE)
    case SPAWN_CONSTANT_ONE:
		mins.x = maxs.x = 1.0f;
		return;
    case SPAWN_CONSTANT_ONE+1:
		mins.x = maxs.x = 1.0f;
		mins.y = maxs.y = 1.0f;
		return;
    case SPAWN_CONSTANT_ONE+2:
		mins.x = maxs.x = 1.0f;
		mins.y = maxs.y = 1.0f;
		mins.z = maxs.z = 1.0f;
        //mins = maxs = idVec3(1.0f, 1.0f, 1.0f);
        return;

        //  Constant-point spawns
    case SPAWN_AT_POINT:
		mins.x = maxs.x = mMins.x;
		return;
    case SPAWN_AT_POINT+1:
		mins.x = maxs.x = mMins.x;
		mins.y = maxs.y = mMins.y;
		return;
    case SPAWN_AT_POINT+2:
        mins = maxs = mMins;
        return;

        //  Everything that uses the full AABB
    case SPAWN_LINEAR:
    case SPAWN_BOX:
    case SPAWN_SURFACE_BOX:
    case SPAWN_SPHERE:
    case SPAWN_SURFACE_SPHERE:
    case SPAWN_CYLINDER:
    case SPAWN_SURFACE_CYLINDER:
    case SPAWN_SPIRAL:
    case SPAWN_MODEL:
		mins.x = mMins.x;
		maxs.x = mMaxs.x;
		return;
    case SPAWN_LINEAR+1:
    case SPAWN_BOX+1:
    case SPAWN_SURFACE_BOX+1:
    case SPAWN_SPHERE+1:
    case SPAWN_SURFACE_SPHERE+1:
    case SPAWN_CYLINDER+1:
    case SPAWN_SURFACE_CYLINDER+1:
    case SPAWN_SPIRAL+1:
    case SPAWN_MODEL+1:
		mins.x = mMins.x;
		mins.y = mMins.y;
		maxs.x = mMaxs.x;
		maxs.y = mMaxs.y;
        return;
    case SPAWN_LINEAR+2:
    case SPAWN_BOX+2:
    case SPAWN_SURFACE_BOX+2:
    case SPAWN_SPHERE+2:
    case SPAWN_SURFACE_SPHERE+2:
    case SPAWN_CYLINDER+2:
    case SPAWN_SURFACE_CYLINDER+2:
    case SPAWN_SPIRAL+2:
    case SPAWN_MODEL+2:
        mins = mMins;
        maxs = mMaxs;
		return;

    default:
        return; // leave as zeroes
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Helper – turn any vector into a normalised direction.  If |v| == 0 it stays
//  zero (avoids NaNs later).
// ──────────────────────────────────────────────────────────────────────────────
static inline void NormaliseSafe(idVec3& v)
{
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq > 0.0f)
        v *= idMath::InvSqrt(lenSq);
}

//───────────────────────────────────────────────────────────────────────────────
void SpawnGetNormal(idVec3* normal, const idVec3& pos, const idVec3* centre)
{
    if (!normal) return;

    if (centre)
        *normal = pos - *centre;
    else
        *normal = pos;

    NormaliseSafe(*normal);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Macros to save repetition – expands to one-liner bodies
// ──────────────────────────────────────────────────────────────────────────────
#define SPAWN_SCALAR(name, value)                                      \
    void name(float* out, const rvParticleParms&,                      \
              idVec3*, const idVec3*) { *out = value; }

#define SPAWN_SCALAR2(name, v0, v1)                                    \
    void name(float* out, const rvParticleParms&,                      \
              idVec3*, const idVec3*) { out[0] = v0; out[1] = v1; }

#define SPAWN_VEC3_CONST(name, vx, vy, vz)                             \
    void name(idVec3* out, const rvParticleParms&,                     \
              idVec3* n, const idVec3* c) {                            \
        *out = idVec3(vx, vy, vz);                                     \
        SpawnGetNormal(n, *out, c);                                    \
    }

#define SPAWN_FLOAT3_CONST(name, vx, vy, vz)                             \
    void name(float* out, const rvParticleParms& p,                     \
              idVec3* n, const idVec3* c) {                            \
        name((idVec3 *)out, p, n, c);                                     \
    }

//───────────────────────────────────────────────────────────────────────────────
//  “NONE” spawns          → always zero
//───────────────────────────────────────────────────────────────────────────────
SPAWN_SCALAR(SpawnNone1, 0.0f)
SPAWN_SCALAR2(SpawnNone2, 0.0f, 0.0f)
SPAWN_VEC3_CONST(SpawnNone3, 0.0f, 0.0f, 0.0f)
SPAWN_FLOAT3_CONST(SpawnNone3, 0.0f, 0.0f, 0.0f)

//───────────────────────────────────────────────────────────────────────────────
//  “ONE” spawns           → always one
//───────────────────────────────────────────────────────────────────────────────
SPAWN_SCALAR(SpawnOne1, 1.0f)
SPAWN_SCALAR2(SpawnOne2, 1.0f, 1.0f)
SPAWN_VEC3_CONST(SpawnOne3, 1.0f, 1.0f, 1.0f)
SPAWN_FLOAT3_CONST(SpawnOne3, 0.0f, 0.0f, 0.0f)

void SpawnStub(float *,class rvParticleParms const &,class idVec3 *,class idVec3 const *)
{
 
}

//───────────────────────────────────────────────────────────────────────────────
//  Point -- linear -- box helpers (bodies kept verbatim but with nicer names)
//───────────────────────────────────────────────────────────────────────────────
void SpawnPoint1(float* out, const rvParticleParms& p,
    idVec3*, const idVec3*)
{
    *out = p.mMins.x;
}

void SpawnPoint2(float* out, const rvParticleParms& p,
    idVec3*, const idVec3*)
{
    out[0] = p.mMins.x;
    out[1] = p.mMins.y;
}

void SpawnPoint3(idVec3* out, const rvParticleParms& p,
    idVec3* n, const idVec3* c)
{
    *out = p.mMins;
    SpawnGetNormal(n, *out, c);
}

void SpawnPoint3(float* out, const rvParticleParms& p,
                 idVec3* n, const idVec3* c)
{
    SpawnPoint3((idVec3 *)out, p, n, c);
}

//───────────────────────────────────────────────────────────────────────────────
void SpawnLinear1(float* out, const rvParticleParms& p,
    idVec3*, const idVec3*)
{
    *out = rvRandom::flrand(0.f, 1.f) * (p.mMaxs.x - p.mMins.x) + p.mMins.x;
}

void SpawnLinear2(float* out, const rvParticleParms& p,
    idVec3*, const idVec3*)
{
    const float f = (p.mFlags & PPF_LINEAR_SPACING/* 0x10 */) ? out[0] : rvRandom::flrand(0.f, 1.f);
    out[0] = f * (p.mMaxs.x - p.mMins.x) + p.mMins.x;
    out[1] = f * (p.mMaxs.y - p.mMins.y) + p.mMins.y;
}

void SpawnLinear3(idVec3* out, const rvParticleParms& p,
    idVec3* n, const idVec3* c)
{
    const float f = (p.mFlags & PPF_LINEAR_SPACING/* 0x10 */) ? out->x : rvRandom::flrand(0.f, 1.f);
    out->x = f * (p.mMaxs.x - p.mMins.x) + p.mMins.x;
    out->y = f * (p.mMaxs.y - p.mMins.y) + p.mMins.y;
    out->z = f * (p.mMaxs.z - p.mMins.z) + p.mMins.z;
    SpawnGetNormal(n, *out, c);
}

void SpawnLinear3(float* out, const rvParticleParms& p,
                  idVec3* n, const idVec3* c)
{
    SpawnLinear3((idVec3 *)out, p, n, c);
}

//───────────────────────────────────────────────────────────────────────────────
//  Box helpers
//───────────────────────────────────────────────────────────────────────────────
void SpawnBox1(float* out, const rvParticleParms& p)
{
    *out = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
}

void SpawnBox1(float* out, const rvParticleParms& p,
                  idVec3* n, const idVec3* c)
{
    (void)n;
    (void)c;
    SpawnBox1(out, p);
}

void SpawnBox2(float* out, const rvParticleParms& p)
{
    out[0] = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
    out[1] = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
}

void SpawnBox2(float* out, const rvParticleParms& p,
               idVec3* n, const idVec3* c)
{
    (void)n;
    (void)c;
    SpawnBox2(out, p);
}

void SpawnBox3(idVec3* out, const rvParticleParms& p,
    idVec3* n, const idVec3* c)
{
    out->x = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
    out->y = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
    out->z = rvRandom::flrand(p.mMins.z, p.mMaxs.z);
    SpawnGetNormal(n, *out, c);
}

void SpawnBox3(float* out, const rvParticleParms& p,
               idVec3* n, const idVec3* c)
{
    SpawnBox3((idVec3 *)out, p, n, c);
}

//───────────────────────────────────────────────────────────────────────────────
//  Surface-box helpers
//───────────────────────────────────────────────────────────────────────────────
void SpawnSurfaceBox1(float* out, const rvParticleParms& p)
{
    out[0] = (&p.mMins.x)[rvRandom::irand(0, 1)];
}

void SpawnSurfaceBox1(float* out, const rvParticleParms& p,
                      idVec3* n, const idVec3* c)
{
    (void)n;
    (void)c;
    SpawnSurfaceBox1(out, p);
}

void SpawnSurfaceBox2(float* out, const rvParticleParms& p)
{
    switch (rvRandom::irand(0, 3))
    {
    case 0:  out[0] = p.mMins.x;
        out[1] = rvRandom::flrand(p.mMins.y, p.mMaxs.y);     break;
    case 1:  out[0] = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
        out[1] = p.mMins.y;                                 break;
    case 2:  out[0] = p.mMaxs.x;
        out[1] = rvRandom::flrand(p.mMins.y, p.mMaxs.y);     break;
    default: out[0] = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
        out[1] = p.mMaxs.y;                                 break;
    }
}

void SpawnSurfaceBox2(float* out, const rvParticleParms& p,
                      idVec3* n, const idVec3* c)
{
    (void)n;
    (void)c;
    SpawnSurfaceBox2(out, p);
}

void SpawnSurfaceBox3(idVec3* out, const rvParticleParms& p,
    idVec3* n, const idVec3* c)
{
    const int face = rvRandom::irand(0, 5);
    switch (face)
    {
    case 0:  out->x = p.mMins.x;
        out->y = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
        out->z = rvRandom::flrand(p.mMins.z, p.mMaxs.z);     break;
    case 1:  out->x = p.mMaxs.x;
        out->y = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
        out->z = rvRandom::flrand(p.mMins.z, p.mMaxs.z);     break;
    case 2:  out->x = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
        out->y = p.mMins.y;
        out->z = rvRandom::flrand(p.mMins.z, p.mMaxs.z);     break;
    case 3:  out->x = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
        out->y = p.mMaxs.y;
        out->z = rvRandom::flrand(p.mMins.z, p.mMaxs.z);     break;
    case 4:  out->x = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
        out->y = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
        out->z = p.mMins.z;                                 break;
    default: out->x = rvRandom::flrand(p.mMins.x, p.mMaxs.x);
        out->y = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
        out->z = p.mMaxs.z;                                 break;
    }

    //  Normals: either cube-face normals or radial
    if (n)
    {
        if (c) {
            static const idVec3 kCubeNormals[6] = {
             idVec3(-1.0f,  0.0f,  0.0f),
             idVec3( 1.0f,  0.0f,  0.0f),
             idVec3( 0.0f, -1.0f,  0.0f),
             idVec3( 0.0f,  1.0f,  0.0f),
             idVec3( 0.0f,  0.0f, -1.0f),
             idVec3( 0.0f,  0.0f,  1.0f)
            };
            *n = kCubeNormals[face];
        }
        else {
            SpawnGetNormal(n, *out, NULL);
        }
    }
}

void SpawnSurfaceBox3(float* out, const rvParticleParms& p,
                      idVec3* n, const idVec3* c)
{
    SpawnSurfaceBox3((idVec3 *)out, p, n, c);
}
//───────────────────────────────────────────────────────────────────────────────
//  Sphere helpers – identical maths, cleaned naming
//───────────────────────────────────────────────────────────────────────────────
static inline void RandomUnitVector2D(float& x, float& y)
{
    x = rvRandom::flrand(-1.f, 1.f);
    y = rvRandom::flrand(-1.f, 1.f);
    const float lenSq = x * x + y * y;
    if (lenSq > 0.f) {
        const float inv = idMath::InvSqrt(lenSq);
        x *= inv; y *= inv;
    }
}

void SpawnSphere2(float* out, const rvParticleParms& p)
{
    const float originX = 0.5f * (p.mMins.x + p.mMaxs.x);
    const float originY = 0.5f * (p.mMins.y + p.mMaxs.y);
    const float radiusX = 0.5f * (p.mMaxs.x - p.mMins.x);
    const float radiusY = 0.5f * (p.mMaxs.y - p.mMins.y);

    float nx, ny;  RandomUnitVector2D(nx, ny);
    out[0] = rvRandom::flrand(0.f, radiusX) * nx + originX;
    out[1] = rvRandom::flrand(0.f, radiusY) * ny + originY;
}

void SpawnSphere2(float* out, const rvParticleParms& p,
                  idVec3* n, const idVec3* c)
{
    (void)n;
    (void)c;
    SpawnSphere2(out, p);
}

void SpawnSphere3(idVec3* out, const rvParticleParms& p,
    idVec3* n, const idVec3* c)
{
    const idVec3 origin(
        0.5f * (p.mMins.x + p.mMaxs.x),
        0.5f * (p.mMins.y + p.mMaxs.y),
        0.5f * (p.mMins.z + p.mMaxs.z));

    const idVec3 radius(
        0.5f * (p.mMaxs.x - p.mMins.x),
        0.5f * (p.mMaxs.y - p.mMins.y),
        0.5f * (p.mMaxs.z - p.mMins.z));

    // Random unit vector in 3-D
    idVec3 dir(rvRandom::flrand(-1.f, 1.f),
        rvRandom::flrand(-1.f, 1.f),
        rvRandom::flrand(-1.f, 1.f));
    NormaliseSafe(dir);

    out->x = rvRandom::flrand(0.f, radius.x) * dir.x + origin.x;
    out->y = rvRandom::flrand(0.f, radius.y) * dir.y + origin.y;
    out->z = rvRandom::flrand(0.f, radius.z) * dir.z + origin.z;
    SpawnGetNormal(n, *out, c);
}

void SpawnSphere3(float* out, const rvParticleParms& p,
                  idVec3* n, const idVec3* c)
{
    SpawnSphere3((idVec3 *)out, p, n, c);
}


// ──────────────────────────────────────────────────────────────────────────────
//  SpawnSurfaceSphere2  – random point on circumference (2-D slice)
// ──────────────────────────────────────────────────────────────────────────────
void SpawnSurfaceSphere2(float* outXY, const rvParticleParms& p)
{
    const float originX = 0.5f * (p.mMaxs.x + p.mMins.x);
    const float originY = 0.5f * (p.mMaxs.y + p.mMins.y);
    const float radiusX = 0.5f * (p.mMaxs.x - p.mMins.x);
    const float radiusY = 0.5f * (p.mMaxs.y - p.mMins.y);

    // Random unit vector in 2-D
    const float dx = rvRandom::flrand(-1.0f, 1.0f);
    const float dy = rvRandom::flrand(-1.0f, 1.0f);
    const float invLen = 1.0f / sqrt(dx * dx + dy * dy);

    outXY[0] = originX + radiusX * dx * invLen;
    outXY[1] = originY + radiusY * dy * invLen;
}

void SpawnSurfaceSphere2(float* outXY, const rvParticleParms& p,
                         idVec3* normal, const idVec3* centre)
{
    (void)normal;
    (void)centre;
    SpawnSurfaceSphere2(outXY, p);
}

// ──────────────────────────────────────────────────────────────────────────────
void SpawnSurfaceSphere3(idVec3* outXYZ, const rvParticleParms& p,
    idVec3* normal, const idVec3* centre)
{
    // Sphere centre & radii
    const idVec3 origin(
        0.5f * (p.mMaxs.x + p.mMins.x),
        0.5f * (p.mMaxs.y + p.mMins.y),
        0.5f * (p.mMaxs.z + p.mMins.z)
    );
    const idVec3 radius(
        0.5f * (p.mMaxs.x - p.mMins.x),
        0.5f * (p.mMaxs.y - p.mMins.y),
        0.5f * (p.mMaxs.z - p.mMins.z)
    );

    // Random unit vector in 3-D
    float dx = rvRandom::flrand(-1.0f, 1.0f);
    float dy = rvRandom::flrand(-1.0f, 1.0f);
    float dz = rvRandom::flrand(-1.0f, 1.0f);
    const float invLen = 1.0f / sqrt(dx * dx + dy * dy + dz * dz);
    dx *= invLen; dy *= invLen; dz *= invLen;

    outXYZ->x = origin.x + radius.x * dx;
    outXYZ->y = origin.y + radius.y * dy;
    outXYZ->z = origin.z + radius.z * dz;

    SpawnGetNormal(normal, *outXYZ, centre);
}

void SpawnSurfaceSphere3(float* outXYZ, const rvParticleParms& p,
                         idVec3* normal, const idVec3* centre)
{
    SpawnSurfaceSphere3((idVec3 *)outXYZ, p, normal, centre);
}

// ──────────────────────────────────────────────────────────────────────────────
void SpawnCylinder3(idVec3* outXYZ, const rvParticleParms& p,
    idVec3* normal, const idVec3* centre)
{
    // YZ ellipse radii, X height
    const float halfH = 0.5f * (p.mMaxs.x - p.mMins.x);   // “height” along X
    const float rY = 0.5f * (p.mMaxs.y - p.mMins.y);
    const float rZ = 0.5f * (p.mMaxs.z - p.mMins.z);

    // Optional taper flag (bit 2)
    const bool  doTaper = (p.mFlags & PPF_CONE/* 4 */) != 0;

    // Fraction along height
    const float t = rvRandom::flrand(0.0f, halfH * 2.0f);
    const float tp = doTaper ? (t / (halfH * 2.0f)) : 1.0f;

    // Random direction around cylinder axis
    float dy = rvRandom::flrand(-1.0f, 1.0f);
    float dz = rvRandom::flrand(-1.0f, 1.0f);
    const float invLen = 1.0f / sqrt(dy * dy + dz * dz);
    dy *= invLen; dz *= invLen;

    outXYZ->x = p.mMins.x + t;
    outXYZ->y = 0.5f * (p.mMaxs.y + p.mMins.y) + rY * tp * dy;
    outXYZ->z = 0.5f * (p.mMaxs.z + p.mMins.z) + rZ * tp * dz;

    SpawnGetNormal(normal, *outXYZ, centre);
}

void SpawnCylinder3(float* outXYZ, const rvParticleParms& p,
                    idVec3* normal, const idVec3* centre)
{
    SpawnCylinder3((idVec3 *)outXYZ, p, normal, centre);
}

// ──────────────────────────────────────────────────────────────────────────────
void SpawnSurfaceCylinder3(idVec3* outXYZ, const rvParticleParms& p,
    idVec3* normal, const idVec3* centre)
{
    // Same maths as SpawnCylinder3 but the point is forced to the surface
    const float halfH = 0.5f * (p.mMaxs.x - p.mMins.x);
    const float rY = 0.5f * (p.mMaxs.y - p.mMins.y);
    const float rZ = 0.5f * (p.mMaxs.z - p.mMins.z);

    const bool  doTaper = (p.mFlags & PPF_CONE/* 4 */) != 0;
    const float t = rvRandom::flrand(0.0f, halfH * 2.0f);
    const float tp = doTaper ? (t / (halfH * 2.0f)) : 1.0f;

    // Unit vector around axis (surface ⇒ radius==1)
    float dy = rvRandom::flrand(-1.0f, 1.0f);
    float dz = rvRandom::flrand(-1.0f, 1.0f);
    const float invLen = 1.0f / sqrt(dy * dy + dz * dz);
    dy *= invLen; dz *= invLen;

    outXYZ->x = p.mMins.x + t;
    outXYZ->y = 0.5f * (p.mMaxs.y + p.mMins.y) + rY * tp * dy;
    outXYZ->z = 0.5f * (p.mMaxs.z + p.mMins.z) + rZ * tp * dz;

    // Normal is either the radial direction (surface) or axis-aligned if taper == 0
    if (normal)
    {
        if (centre)
        {
            if (tp != 1.0f)         // tapered side
            {
                normal->x = 0.0f;
                normal->y = dy;
                normal->z = dz;
            }
            else if (halfH == 0.0f) // degenerate – turn into flat plane
            {
                normal->x = 1.0f;  normal->y = normal->z = 0.0f;
            }
            else                    // end-caps
            {
                // Treat cylinder side as 2-D ellipse; build plane n = (-dz, 0, dy)
                normal->x = -(dz * halfH);
                normal->y = 0.0f;
                normal->z = (dy * halfH);

                const float len2 = normal->x * normal->x + normal->z * normal->z;
                if (len2 > 0.0f)
                {
                    const float inv = 1.0f / sqrt(len2);
                    normal->x *= inv;  normal->z *= inv;
                }
            }
        }
        else
        {
            SpawnGetNormal(normal, *outXYZ, NULL);
        }
    }
}

void SpawnSurfaceCylinder3(float* outXYZ, const rvParticleParms& p,
                           idVec3* normal, const idVec3* centre)
{
    SpawnSurfaceCylinder3((idVec3 *)outXYZ, p, normal, centre);
}

// ──────────────────────────────────────────────────────────────────────────────
void SpawnSpiral2(float* outXY, const rvParticleParms& p)
{
    // X == distance along spiral axis
    if (p.mFlags & PPF_LINEAR_SPACING/* 0x10 */)
        outXY[0] = (p.mMaxs.x - p.mMins.x) * outXY[0] + p.mMins.x;
    else
        outXY[0] = rvRandom::flrand(p.mMins.x, p.mMaxs.x);

    // Y == radius * cos(theta)   (theta depends on distance and p.mRange)
    const float theta = idMath::TWO_PI * outXY[0] / p.mRange;
    const float r = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
    outXY[1] = r * cos(theta);
}

void SpawnSpiral2(float* outXYZ, const rvParticleParms& p,
                  idVec3* normal, const idVec3* centre)
{
    (void)normal;
    (void)centre;
    SpawnSpiral2(outXYZ, p);
}

// ──────────────────────────────────────────────────────────────────────────────
void SpawnSpiral3(idVec3* outXYZ, const rvParticleParms& p,
    idVec3* normal, const idVec3* centre)
{
    // X coordinate: either pre-initialised (flags & 0x10) or random
    if (p.mFlags & PPF_LINEAR_SPACING/* 0x10 */)
        outXYZ->x = (p.mMaxs.x - p.mMins.x) * outXYZ->x + p.mMins.x;
    else
        outXYZ->x = rvRandom::flrand(p.mMins.x, p.mMaxs.x);

    // Random radii in Y/Z
    const float ry = rvRandom::flrand(p.mMins.y, p.mMaxs.y);
    const float rz = rvRandom::flrand(p.mMins.z, p.mMaxs.z);

    // Angle along spiral
    const float theta = idMath::TWO_PI * outXYZ->x / p.mRange;
    const float c = cos(theta);
    const float s = sin(theta);

    outXYZ->y = c * ry - s * rz;
    outXYZ->z = c * rz + s * ry;

    SpawnGetNormal(normal, *outXYZ, centre);
}

void SpawnSpiral3(float* outXYZ, const rvParticleParms& p,
                  idVec3* normal, const idVec3* centre)
{
    SpawnSpiral3((idVec3 *)outXYZ, p, normal, centre);
}

// ──────────────────────────────────────────────────────────────────────────────
//  rvParticleParms::HandleRelativeParms
//      If the emitter is “relative” (flag 8) then add init[] to death[] so the
//      values become absolute.
// ──────────────────────────────────────────────────────────────────────────────
//void rvParticleParms::HandleRelativeParms(float* death, float* init, int count)
//{
//    if (!(mFlags & 8)) return;
//
//    for (int i = 0; i < count; ++i)
//        death[i] += init[i];
//}
//
//// ──────────────────────────────────────────────────────────────────────────────
////  rvParticleParms::GetMinsMaxs – convenience used by editor UI & export tools
//// ──────────────────────────────────────────────────────────────────────────────
//void rvParticleParms::GetMinsMaxs(idVec3& mins, idVec3& maxs) const
//{
//    mins.Zero();   maxs.Zero();
//
//    switch (mSpawnType)
//    {
//    case 5:  /* SPAWN_ONE */           mins.x = maxs.x = 1.0f;       break;
//    case 6:  /* SPAWN_ONE_2D */        mins.y = maxs.y = 1.0f; [[fallthrough]];
//    case 7:  /* SPAWN_ONE_3D */        mins.z = maxs.z = 1.0f;       break;
//
//    case 9:  /* SPAWN_POINT */         mins.x = maxs.x = mMins.x;    break;
//    case 10: /* SPAWN_POINT2 */        mins.y = maxs.y = mMins.y; [[fallthrough]];
//    case 11: /* SPAWN_POINT3 */        mins.z = maxs.z = mMins.z;    break;
//
//    default: // most shapes use full mins/maxs box
//        mins = mMins;
//        maxs = mMaxs;
//    }
//}

// ──────────────────────────────────────────────────────────────────────────────
//  SpawnModel3 – sample point on an arbitrary model’s triangle surface.
//  NOTE: this code still depends on Doom-3’s internal surface structs;
//        the call to R_DeriveFacePlanes() is left intact.
// ──────────────────────────────────────────────────────────────────────────────
void SpawnModel3(float* result, const rvParticleParms* parms, idVec3* normal, const idVec3* center) {
    // Get a random surface from the model and derive face planes if necessary
    const idRenderModelStatic* misc = (const idRenderModelStatic *)parms->mMisc;
    int numSurfaces = misc->NumSurfaces();
    int surfaceIndex = rvRandom::irand(0, numSurfaces - 1);
    srfTriangles_s* surface = misc->Surface(surfaceIndex)->geometry;

    if (!surface->facePlanes) {
        R_DeriveFacePlanes(surface);
    }

    // Select a random triangle from the surface
    int numTriangles = surface->numIndexes / 3;
    int triangleIndex = rvRandom::irand(0, numTriangles - 1);
    int* triangleVerts = &surface->indexes[3 * triangleIndex];
    idDrawVert* verts = surface->verts;

    // Compute the position within the triangle using random barycentric coordinates
    float u = rvRandom::flrand(0.0f, 1.0f);
    float v = rvRandom::flrand(0.0f, 1.0f);
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }
    float w = 1.0f - u - v; // Ensure barycentric coordinates add up to 1

    idVec3 position;
    position.x = u * verts[triangleVerts[0]].xyz.x + v * verts[triangleVerts[1]].xyz.x + w * verts[triangleVerts[2]].xyz.x;
    position.y = u * verts[triangleVerts[0]].xyz.y + v * verts[triangleVerts[1]].xyz.y + w * verts[triangleVerts[2]].xyz.y;
    position.z = u * verts[triangleVerts[0]].xyz.z + v * verts[triangleVerts[1]].xyz.z + w * verts[triangleVerts[2]].xyz.z;

    // Transform the point to world space using the model's transformation matrix
    idMat3& transform = rvBSEManagerLocal::mModelToBSE;
    float newX = transform[0].x * position.x + transform[1].x * position.y + transform[2].x * position.z;
    float newY = transform[0].y * position.x + transform[1].y * position.y + transform[2].y * position.z;
    float newZ = transform[0].z * position.x + transform[1].z * position.y + transform[2].z * position.z;

    // Scale the point to fit within the specified bounds
    idVec3 mins = parms->mMins;
    idVec3 maxs = parms->mMaxs;
    float scaleX = (maxs.x - mins.x) / (transform[0].x * surface->bounds[1].x + transform[1].x * surface->bounds[1].y + transform[2].x * surface->bounds[1].z -
        (transform[0].x * surface->bounds[0].x + transform[1].x * surface->bounds[0].y + transform[2].x * surface->bounds[0].z));
    float scaleY = (maxs.y - mins.y) / (transform[0].y * surface->bounds[1].x + transform[1].y * surface->bounds[1].y + transform[2].y * surface->bounds[1].z -
        (transform[0].y * surface->bounds[0].x + transform[1].y * surface->bounds[0].y + transform[2].y * surface->bounds[0].z));
    float scaleZ = (maxs.z - mins.z) / (transform[0].z * surface->bounds[1].x + transform[1].z * surface->bounds[1].y + transform[2].z * surface->bounds[1].z -
        (transform[0].z * surface->bounds[0].x + transform[1].z * surface->bounds[0].y + transform[2].z * surface->bounds[0].z));

    // Compute the final position
    result[0] = scaleX * newX + (mins.x + maxs.x) * 0.5f - ((transform[0].x * surface->bounds[1].x + transform[1].x * surface->bounds[1].y +
        transform[2].x * surface->bounds[1].z +
        transform[0].x * surface->bounds[0].x + transform[1].x * surface->bounds[0].y +
        transform[2].x * surface->bounds[0].z) * 0.5f);
    result[1] = scaleY * newY + ((mins.y + maxs.y) * 0.5f - (transform[0].y * surface->bounds[1].x +
        transform[1].y * surface->bounds[1].y +
        transform[2].y * surface->bounds[1].z +
        transform[0].y * surface->bounds[0].x +
        transform[1].y * surface->bounds[0].y +
        transform[2].y * surface->bounds[0].z) * 0.5f);
    result[2] = scaleZ * newZ + ((mins.z + maxs.z) * 0.5f - (transform[0].z * surface->bounds[1].x +
        transform[1].z * surface->bounds[1].y +
        transform[2].z * surface->bounds[1].z +
        transform[0].z * surface->bounds[0].x +
        transform[1].z * surface->bounds[0].y +
        transform[2].z * surface->bounds[0].z) * 0.5f);

    // If normal is requested, compute and normalize it
    if (normal && center) {
        // Use the face plane's normal if available
        if (surface->facePlanes) {
            *normal = idVec3(surface->facePlanes[triangleIndex][0]/*//k a */, surface->facePlanes[triangleIndex][1]/*//k b */, surface->facePlanes[triangleIndex][2]/*//k c */);
        }
        else {
            // Otherwise, use the computed position as a normal
            *normal = position;
        }

        // Normalize the normal vector
        float normLengthSq = normal->x * normal->x + normal->y * normal->y + normal->z * normal->z;
        if (normLengthSq != 0.0f) {
            float invLength = 1.0f / sqrtf(normLengthSq);
            normal->x *= invLength;
            normal->y *= invLength;
            normal->z *= invLength;
        }
    }
}

void SpawnModel3(float* result, const rvParticleParms& parms, idVec3* normal, const idVec3* center) {
    SpawnModel3(result, &parms, normal, center);
}
