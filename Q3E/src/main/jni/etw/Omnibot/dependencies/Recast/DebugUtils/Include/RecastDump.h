//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef RECAST_DUMP_H
#define RECAST_DUMP_H

#include "DebugDraw.h"

struct duFileIO
{
	virtual ~duFileIO() = 0;
	virtual bool isWriting() const = 0;
	virtual bool isReading() const = 0;
	virtual bool write(const void* ptr, const size_t size) = 0;
	virtual bool read(void* ptr, const size_t size) = 0;
};

bool duDumpPolyMeshToObj(struct rcPolyMesh& pmesh, duFileIO* io);
bool duDumpPolyMeshDetailToObj(struct rcPolyMeshDetail& dmesh, duFileIO* io);

bool duDumpContourSet(struct rcContourSet& cset, duFileIO* io);
bool duReadContourSet(struct rcContourSet& cset, duFileIO* io);

bool duDumpCompactHeightfield(struct rcCompactHeightfield& chf, duFileIO* io);
bool duReadCompactHeightfield(struct rcCompactHeightfield& chf, duFileIO* io);

void duLogBuildTimes(rcContext& ctx, const int totalTileUsec);

//////////////////////////////////////////////////////////////////////////

struct duDebugDrawDump : public duDebugDraw
{
public:
	void depthMask(bool state);
	void begin(duDebugDrawPrimitives prim, float size = 1.0f);
	void vertex(const float* pos, unsigned int color);
	void vertex(const float x, const float y, const float z, unsigned int color);
	void end();

	duDebugDrawDump( duFileIO* objFile, duFileIO* matFile, const char * matname );
private:
	struct duDebugVert
	{
		unsigned int color;
		float x,y,z;
	};

	duDebugDrawPrimitives	currentPrimitive;
	float					currentSize;
	int						currentVert;
	int						currentFace;

	duDebugVert				cache[4];
	int						cacheIndex;

	enum { NumMaterials = 1024 };
	unsigned int			materials[NumMaterials];
	int						numMaterials;

	duFileIO *				objectFile;
	duFileIO *				materialFile;

	void addMaterial(unsigned int color);
	void updateDump();
};

#endif // RECAST_DUMP_H
