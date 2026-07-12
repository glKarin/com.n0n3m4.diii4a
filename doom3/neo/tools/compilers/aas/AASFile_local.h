/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __AASFILELOCAL_H__
#define __AASFILELOCAL_H__

/*
===============================================================================

	AAS File Local

===============================================================================
*/

class idAASFileLocal : public idAASFile
{
		friend class idAASBuild;
		friend class idAASReach;
		friend class idAASCluster;
	public:
		idAASFileLocal(void);
		virtual 					~idAASFileLocal(void);

	public:
		virtual idVec3				EdgeCenter(int edgeNum) const;
		virtual idVec3				FaceCenter(int faceNum) const;
		virtual idVec3				AreaCenter(int areaNum) const;

		virtual idBounds			EdgeBounds(int edgeNum) const;
		virtual idBounds			FaceBounds(int faceNum) const;
		virtual idBounds			AreaBounds(int areaNum) const;

		virtual int					PointAreaNum(const idVec3 &origin) const;
		virtual int					PointReachableAreaNum(const idVec3 &origin, const idBounds &searchBounds, const int areaFlags, const int excludeTravelFlags) const;
		virtual int					BoundsReachableAreaNum(const idBounds &bounds, const int areaFlags, const int excludeTravelFlags) const;
		virtual void				PushPointIntoAreaNum(int areaNum, idVec3 &point) const;
		virtual bool				Trace(aasTrace_t &trace, const idVec3 &start, const idVec3 &end) const;
		virtual void				PrintInfo(void) const;

	public:
		bool						Load(const idStr &fileName, unsigned int mapFileCRC);
		bool						Write(const idStr &fileName, unsigned int mapFileCRC);

#ifdef _SPLASHDAMAGE //karin: parse binary aasb file
		bool						LoadBinary(const idStr &fileName, unsigned int mapFileCRC);
		bool						ParseIndexBinary(idFile *file, idList<aasIndex_t> &indexes);
		bool						ParsePlanesBinary(idFile *file);
		bool						ParseVerticesBinary(idFile *file);
		bool						ParseEdgesBinary(idFile *file);
		bool						ParseReachabilitiesBinary(idFile *file);
		bool						ParseAreasBinary(idFile *file);
		bool						ParseNodesBinary(idFile *file);
		bool						ParsePortalsBinary(idFile *file);
		bool						ParseClustersBinary(idFile *file);
		bool						ParseObstaclePVSsBinary(idFile *file);
		bool						ParseReachabilityNamesBinary(idFile *file);

		size_t						MemorySize(void) const;
#else
		int							MemorySize(void) const;
#endif
		void						ReportRoutingEfficiency(void) const;
		void						Optimize(void);
		void						LinkReversedReachability(void);
		void						FinishAreas(void);

		void						Clear(void);
		void						DeleteReachabilities(void);
		void						DeleteClusters(void);

	private:
		bool						ParseIndex(idLexer &src, idList<aasIndex_t> &indexes);
		bool						ParsePlanes(idLexer &src);
		bool						ParseVertices(idLexer &src);
		bool						ParseEdges(idLexer &src);
		bool						ParseFaces(idLexer &src);
		bool						ParseReachabilities(idLexer &src, int areaNum);
		bool						ParseAreas(idLexer &src);
		bool						ParseNodes(idLexer &src);
		bool						ParsePortals(idLexer &src);
		bool						ParseClusters(idLexer &src);
#ifdef _RAVEN
		virtual void					ClearTactical(void) { }

		virtual	int						GetNumFeatureIndexes(void) const { return featureIndexes.Num(); }
		virtual	aasIndex_t& GetFeatureIndex(int index) { return featureIndexes[index]; }
		virtual int						AppendFeatureIndex(aasIndex_t& featureIdx) { return featureIndexes.Append(featureIdx); }

		virtual	int						GetNumFeatures(void) const { return features.Num(); }
		virtual	aasFeature_t& GetFeature(int index) { return features[index]; }
		virtual int						AppendFeature(aasFeature_t& cluster) { return features.Append(cluster); }
		virtual bool					IsDummyFile( unsigned int mapFileCRC ) { (void)mapFileCRC; return false; }
		virtual size_t					GetMemorySize( void ) {
			return MemorySize();
		}
#endif
#ifdef _SPLASHDAMAGE
		virtual int					FindReachabilityByName( const char *name ) const;
		virtual bool				PushPointIntoArea( int areaNum, idVec3 &point ) const;
#endif

	private:
		int							BoundsReachableAreaNum_r(int nodeNum, const idBounds &bounds, const int areaFlags, const int excludeTravelFlags) const;
		int							BoundsReachableAreaNum_r(const idBounds *bounds, int nodeNum, int areaFlags, int excludeTravelFlags) const;
		void						MaxTreeDepth_r(int nodeNum, int &depth, int &maxDepth) const;
		int							MaxTreeDepth(void) const;
		int							AreaContentsTravelFlags(int areaNum) const;
		idVec3						AreaReachableGoal(int areaNum) const;
		int							NumReachabilities(void) const;
#ifdef _SPLASHDAMAGE
		void						LinkReachability(void);
		void						FlagNoPushAreas(void);

		// sample
		struct floorEdgeSplitPoint_t {
			idVec3	point;
			float	distance;
			int		edgeIndex;
		};
		struct bestReachableArea_t {
			float	v0;
			float	v1;
			int		areaFlags;
			int		excludeTravelFlags;
			int		areaNum1;
			float	distance1;
			int		areaNum2;
			float	distance2;
		};

		virtual bool				TraceHeight( aasTraceHeight_t &trace, const idVec3 &start, const idVec3 &end ) const;
		virtual bool				TraceFloor( aasTraceFloor_t &trace, const idVec3 &start, int startAreaNum, const idVec3 &end, int endAreaNum, int travelFlags ) const;
		bool						SplitFloorWinding(int areaNum, const idPlane *plane, float retDists[], int retSides[]) const;
		bool						GetFloorEdgeSplitPoints(floorEdgeSplitPoint_t *minSplitPoint, floorEdgeSplitPoint_t *maxSplitPoint, int areaNum, const idPlane *toLeftSplitPlane, const idPlane *toForwardRefPlane) const;
		float						GetFloorDistance(int areaNum, const idPlane *plane, const idVec3 *origin_a4, float minDist_a5, float maxDist_a6) const;
		void						BoundsBestReachableAreaNum(const idBounds *bounds, const idVec3 *origin, int nodeNum, const idPlane *plane, bestReachableArea_t *bestReachableArea) const;
		void						PointBestReachableAreaNum(const idVec3 *origin, bestReachableArea_t *bestReachableArea) const;

		bool						Trace(aasTrace_t *trace_a2, const idVec3 *start_a3, const idVec3 *end_a4) const;

		// offset is 103 * 4 = 412
		idList<int>					searchAreaList; // search area index
#endif
};

#endif /* !__AASFILELOCAL_H__ */
