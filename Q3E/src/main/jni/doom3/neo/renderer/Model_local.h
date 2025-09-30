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

#ifndef __MODEL_LOCAL_H__
#define __MODEL_LOCAL_H__

/*
===============================================================================

	Static model

===============================================================================
*/

class idRenderModelStatic : public idRenderModel
{
	public:
		// the inherited public interface
		static idRenderModel 		*Alloc();

		idRenderModelStatic();
		virtual						~idRenderModelStatic();

		virtual void				InitFromFile(const char *fileName);
		virtual void				PartialInitFromFile(const char *fileName);
		virtual void				PurgeModel();
		virtual void				Reset() {};
		virtual void				LoadModel();
		virtual bool				IsLoaded() const;
		virtual void				SetLevelLoadReferenced(bool referenced);
		virtual bool				IsLevelLoadReferenced();
		virtual void				TouchData();
		virtual void				InitEmpty(const char *name);
		virtual void				AddSurface(modelSurface_t surface);
		virtual void				FinishSurfaces();
		virtual void				FreeVertexCache();
		virtual const char 		*Name() const;
		virtual void				Print() const;
		virtual void				List() const;
		virtual int					Memory() const;
		virtual ID_TIME_T				Timestamp() const;
		virtual int					NumSurfaces() const;
		virtual int					NumBaseSurfaces() const;
		virtual const modelSurface_t *Surface(int surfaceNum) const;
		virtual srfTriangles_t 	*AllocSurfaceTriangles(int numVerts, int numIndexes) const;
		virtual void				FreeSurfaceTriangles(srfTriangles_t *tris) const;
		virtual srfTriangles_t 	*ShadowHull() const;
		virtual bool				IsStaticWorldModel() const;
		virtual dynamicModel_t		IsDynamicModel() const;
		virtual bool				IsDefaultModel() const;
		virtual bool				IsReloadable() const;
#ifdef _RAVEN
        virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual int					NumJoints(void) const;
		virtual const idMD5Joint 	*GetJoints(void) const;
		virtual jointHandle_t		GetJointHandle(const char *name) const;
		virtual const char 		*GetJointName(jointHandle_t handle) const;
		virtual const idJointQuat 	*GetDefaultPose(void) const;
		virtual int					NearestJoint(int surfaceNum, int a, int b, int c) const;
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
		virtual void				ReadFromDemoFile(class idDemoFile *f);
		virtual void				WriteToDemoFile(class idDemoFile *f);
		virtual float				DepthHack() const;

		void						MakeDefaultModel();

		bool						LoadASE(const char *fileName);
		bool						LoadLWO(const char *fileName);
		bool						LoadFLT(const char *fileName);
		bool						LoadMA(const char *filename);

		bool						ConvertASEToModelSurfaces(const struct aseModel_s *ase);
		bool						ConvertLWOToModelSurfaces(const struct st_lwObject *lwo);
		bool						ConvertMAToModelSurfaces(const struct maModel_s *ma);

		struct aseModel_s 			*ConvertLWOToASE(const struct st_lwObject *obj, const char *fileName);

#ifdef _MODEL_OBJ
        bool						LoadOBJ( const char* fileName );
        bool						ConvertOBJToModelSurfaces( const struct objModel_t* obj );
#endif
#ifdef _MODEL_DAE
        bool						LoadDAE( const char* fileName );
        bool						ConvertDAEToModelSurfaces( const struct ColladaParser* obj );
#endif

		bool						DeleteSurfaceWithId(int id);
		void						DeleteSurfacesWithNegativeId(void);
		bool						FindSurfaceWithId(int id, int &surfaceNum);
#ifdef _RAVEN //k: for ShowSurface/HideSurface, static model using surfaces index as mask: 1 << index, name is shader material name
		virtual int                 GetSurfaceMask(const char *name) const;

		virtual void                SetHasSky( bool on ) {
			hasSky = on;
		}
		virtual bool                GetHasSky( void ) const {
			return hasSky;
		}
#endif
#ifdef _HUMANHEAD
	    virtual void				IntersectBounds( const idBounds &bounds, float displacement ) { }

#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
        bool						IsGameUpdatedModel(void) { return bIsGUM; }
        void					    SetGameUpdatedModel(bool gum) { bIsGUM = gum; }
#endif //HUMANHEAD END
#endif

	public:
		idList<modelSurface_t>		surfaces;
		idBounds					bounds;
		int							overlaysAdded;

	protected:
		int							lastModifiedFrame;
		int							lastArchivedFrame;

		idStr						name;
		srfTriangles_t 			*shadowHull;
		bool						isStaticWorldModel;
		bool						defaulted;
		bool						purged;					// eventually we will have dynamic reloading
		bool						fastLoad;				// don't generate tangents and shadow data
		bool						reloadable;				// if not, reloadModels won't check timestamp
		bool						levelLoadReferenced;	// for determining if it needs to be freed
		ID_TIME_T						timeStamp;

		static idCVar				r_mergeModelSurfaces;	// combine model surfaces with the same material
		static idCVar				r_slopVertex;			// merge xyz coordinates this far apart
		static idCVar				r_slopTexCoord;			// merge texture coordinates this far apart
		static idCVar				r_slopNormal;			// merge normals that dot less than this
#ifdef _RAVEN //karin: portal sky
		bool                        hasSky;
#endif
#ifdef _HUMANHEAD
#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
	    bool						bIsGUM;
#endif //HUMANHEAD END
#endif
};

/*
===============================================================================

	MD5 animated model

===============================================================================
*/

class idMD5Mesh
{
		friend class				idRenderModelMD5;

	public:
		idMD5Mesh();
		~idMD5Mesh();

		void						ParseMesh(idLexer &parser, int numJoints, const idJointMat *joints);
		void						UpdateSurface(const struct renderEntity_s *ent, const idJointMat *joints, modelSurface_t *surf);
		idBounds					CalcBounds(const idJointMat *joints);
		int							NearestJoint(int a, int b, int c) const;
		int							NumVerts(void) const;
		int							NumTris(void) const;
		int							NumWeights(void) const;

	private:
		idList<idVec2>				texCoords;			// texture coordinates
		int							numWeights;			// number of weights
		idVec4 					*scaledWeights;		// joint weights
		int 						*weightIndex;		// pairs of: joint offset + bool true if next weight is for next vertex
		const idMaterial 			*shader;				// material applied to mesh
		int							numTris;			// number of triangles
		struct deformInfo_s 		*deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
		int							surfaceNum;			// number of the static surface created for this mesh

		void						TransformVerts(idDrawVert *verts, const idJointMat *joints);
		void						TransformScaledVerts(idDrawVert *verts, const idJointMat *joints, float scale);
};

class idRenderModelMD5 : public idRenderModelStatic
{
	public:
		virtual void				InitFromFile(const char *fileName);
		virtual dynamicModel_t		IsDynamicModel() const;
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
		virtual void				Print() const;
		virtual void				List() const;
		virtual void				TouchData();
		virtual void				PurgeModel();
		virtual void				LoadModel();
		virtual int					Memory() const;
#ifdef _RAVEN
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual int					NumJoints(void) const;
		virtual const idMD5Joint 	*GetJoints(void) const;
		virtual jointHandle_t		GetJointHandle(const char *name) const;
		virtual const char 		*GetJointName(jointHandle_t handle) const;
		virtual const idJointQuat 	*GetDefaultPose(void) const;
		virtual int					NearestJoint(int surfaceNum, int a, int b, int c) const;
#ifdef _RAVEN //k: for ShowSurface/HideSurface, md5 model using mesh index as mask: 1 << index, name is shader material name
		virtual int                 GetSurfaceMask(const char *name) const;
#endif
#if defined(_RAVEN) || defined(_HUMANHEAD) //k: for GUI view of dynamic model in idRenderWorld::GuiTrace
	    idRenderModelStatic *       DynamicModelSnapshot(void) { return staticModelInstance; }
	    void                        ClearDynamicModelSnapshot(void) { staticModelInstance = NULL; }
#endif

	private:
		idList<idMD5Joint>			joints;
		idList<idJointQuat>			defaultPose;
		idList<idMD5Mesh>			meshes;

		void						CalculateBounds(const idJointMat *joints);
		void						GetFrameBounds(const renderEntity_t *ent, idBounds &bounds) const;
		void						DrawJoints(const renderEntity_t *ent, const struct viewDef_s *view) const;
		void						ParseJoint(idLexer &parser, idMD5Joint *joint, idJointQuat *defaultPose);

#ifdef _RAVEN //k: show/hide surface
		idList<idStr>               surfaceShaderList;
#endif
#if defined(_RAVEN) || defined(_HUMANHEAD) //k: for GUI view of dynamic model in idRenderWorld::GuiTrace
		idRenderModelStatic         *staticModelInstance;
#endif
};

/*
===============================================================================

	MD3 animated model

===============================================================================
*/

struct md3Header_s;
struct md3Surface_s;

class idRenderModelMD3 : public idRenderModelStatic
{
	public:
		idRenderModelMD3();

		virtual void				InitFromFile(const char *fileName);
		virtual dynamicModel_t		IsDynamicModel() const;
#ifdef _RAVEN
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;

	private:
		int							index;			// model = tr.models[model->index]
		int							dataSize;		// just for listing purposes
		struct md3Header_s 		*md3;			// only if type == MOD_MESH
		int							numLods;
        idList<const idMaterial*>	shaders;		// DG: md3Shader_t::shaderIndex indexes into this array

		void						LerpMeshVertexes(srfTriangles_t *tri, const struct md3Surface_s *surf, const float backlerp, const int frame, const int oldframe) const;
};

/*
===============================================================================

	Liquid model

===============================================================================
*/

class idRenderModelLiquid : public idRenderModelStatic
{
	public:
		idRenderModelLiquid();

		virtual void				InitFromFile(const char *fileName);
		virtual dynamicModel_t		IsDynamicModel() const;
#ifdef _RAVEN
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;

		virtual void				Reset();
#ifdef _HUMANHEAD
		virtual // HUMANHEAD pdm
#endif
		void						IntersectBounds(const idBounds &bounds, float displacement);

	private:
		modelSurface_t				GenerateSurface(float lerp);
		void						WaterDrop(int x, int y, float *page);
		void						Update(void);

		int							verts_x;
		int							verts_y;
		float						scale_x;
		float						scale_y;
		int							time;
		int							liquid_type;
		int							update_tics;
		int							seed;

		idRandom					random;

		const idMaterial 			*shader;
		struct deformInfo_s			*deformInfo;		// used to create srfTriangles_t from base frames
		// and new vertexes

		float						density;
		float						drop_height;
		int							drop_radius;
		float						drop_delay;

		idList<float>				pages;
		float 						*page1;
		float 						*page2;

		idList<idDrawVert>			verts;

		int							nextDropTime;

};

/*
===============================================================================

	PRT model

===============================================================================
*/

class idRenderModelPrt : public idRenderModelStatic
{
	public:
		idRenderModelPrt();

		virtual void				InitFromFile(const char *fileName);
		virtual void				TouchData();
		virtual dynamicModel_t		IsDynamicModel() const;
#ifdef _RAVEN
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
		virtual float				DepthHack() const;
		virtual int					Memory() const;

	private:
		const idDeclParticle 		*particleSystem;
};

/*
===============================================================================

	Beam model

===============================================================================
*/

class idRenderModelBeam : public idRenderModelStatic
{
	public:
		virtual dynamicModel_t		IsDynamicModel() const;
		virtual bool				IsLoaded() const;
#ifdef _RAVEN
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
};

/*
===============================================================================

	Beam model

===============================================================================
*/
#define MAX_TRAIL_PTS	20

struct Trail_t {
	int							lastUpdateTime;
	int							duration;

	idVec3						pts[MAX_TRAIL_PTS];
	int							numPoints;
};

class idRenderModelTrail : public idRenderModelStatic
{
		idList<Trail_t>				trails;
		int							numActive;
		idBounds					trailBounds;

	public:
		idRenderModelTrail();

		virtual dynamicModel_t		IsDynamicModel() const;
		virtual bool				IsLoaded() const;
#ifdef _RAVEN
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;

		int							NewTrail(idVec3 pt, int duration);
		void						UpdateTrail(int index, idVec3 pt);
		void						DrawTrail(int index, const struct renderEntity_s *ent, srfTriangles_t *tri, float globalAlpha);
};

/*
===============================================================================

	Lightning model

===============================================================================
*/

class idRenderModelLightning : public idRenderModelStatic
{
	public:
		virtual dynamicModel_t		IsDynamicModel() const;
		virtual bool				IsLoaded() const;
#ifdef _RAVEN
        virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
};

/*
================================================================================

	idRenderModelSprite

================================================================================
*/
class idRenderModelSprite : public idRenderModelStatic
{
	public:
		virtual	dynamicModel_t	IsDynamicModel() const;
		virtual	bool			IsLoaded() const;
#ifdef _RAVEN
        virtual	idRenderModel 	*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual	idRenderModel 	*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual	idBounds		Bounds(const struct renderEntity_s *ent) const;
};

#ifdef _RAVEN // bse model
#ifdef _RAVEN_BSE
#include "../raven/renderer/Model_bse.h"
#else
#include "../raven/fx/Model_bse.h"
#endif
#endif

#ifdef _HUMANHEAD
// HUMANHEAD: Beams
class hhRenderModelBeam : public idRenderModelStatic {
public:
	void				InitFromFile( const char *fileName );
	void				LoadModel();

	dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel*	InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds	Bounds( const struct renderEntity_s *ent ) const;

private:
	void				UpdateSurface( const struct renderEntity_s *ent, const int index, const hhBeamNodes_t *beam, modelSurface_t *surf );
	void				UpdateQuadSurface( const struct renderEntity_s *ent, const int index, int quadIndex, const hhBeamNodes_t *beam, modelSurface_t *surf );

	struct deformInfo_s	*deformInfo;	// used to create srfTriangles_t from base frames and new vertexes						
	idList<idDrawVert>	verts;

	// endpoint quads
	struct deformInfo_s *quadDeformInfo[2];
	idList<idDrawVert>	quadVerts[2];

	const hhDeclBeam	*declBeam;
};
// END HUMANHEAD
#endif

#endif /* !__MODEL_LOCAL_H__ */
