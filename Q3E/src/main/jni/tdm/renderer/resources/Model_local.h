/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __MODEL_LOCAL_H__
#define __MODEL_LOCAL_H__

/*
===============================================================================

	Static model

===============================================================================
*/

class idRenderModelStatic : public idRenderModel {
public:
	// the inherited public interface
	static idRenderModel *		Alloc();

								idRenderModelStatic();
	virtual						~idRenderModelStatic() override;

	virtual void				InitFromFile( const char *fileName ) override;
	virtual void				PartialInitFromFile( const char *fileName ) override;
	virtual void				PurgeModel() override;
	virtual void				Reset() override {};
	virtual void				LoadModel() override;
	virtual bool				IsLoaded() const override;
	virtual void				SetLevelLoadReferenced( bool referenced ) override;
	virtual bool				IsLevelLoadReferenced() override;
	virtual void				TouchData() override;
	virtual void				InitEmpty( const char *name ) override;
	virtual void				AddSurface( modelSurface_t surface ) override;
	virtual void				FinishSurfaces() override;
	virtual void				FreeVertexCache() override;
	virtual const char *		Name() const override;
	virtual void				Print() const override;
	virtual void				List() const override;
	virtual int					Memory() const override;
	virtual ID_TIME_T			Timestamp() const override;
	virtual int					GetLoadVersion() const override;
	virtual int					NumSurfaces() const override;
	virtual int					NumBaseSurfaces() const override;
	virtual const modelSurface_t *Surface( int surfaceNum ) const override;
	virtual srfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const override;
	virtual void				FreeSurfaceTriangles( srfTriangles_t *tris ) const override;
	virtual srfTriangles_t *	ShadowHull() const override;
	virtual bool				IsStaticWorldModel() const override;
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual bool				IsDefaultModel() const override;
	virtual bool				IsReloadable() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual int					NumJoints( void ) const override;
	virtual const idMD5Joint *	GetJoints( void ) const override;
	virtual jointHandle_t		GetJointHandle( const char *name ) const override;
	virtual const char *		GetJointName( jointHandle_t handle ) const override;
	virtual const idJointQuat *	GetDefaultPose( void ) const override;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;
	virtual const idBounds *	JointBounds() const override;
	virtual void				ReadFromDemoFile( class idDemoFile *f ) override;
	virtual void				WriteToDemoFile( class idDemoFile *f ) override;
	virtual float				DepthHack() const override;
	virtual void				GenerateSamples( idList<samplePointOnModel_t> &samples, const modelSamplingParameters_t &params, idRandom &rnd ) const override;
	virtual idVec3				GetSamplePosition( const struct renderEntity_s *ent, const samplePointOnModel_t &sample ) const override;
	virtual const idMaterial *	GetSampleMaterial( const struct renderEntity_s *ent, const samplePointOnModel_t &sample ) const override;

	void						MakeDefaultModel();
	
	bool						LoadOBJ( const char *fileName );
	bool						LoadASE( const char *fileName );
	bool						LoadLWO( const char *fileName );
	bool						LoadFLT( const char *fileName );
	bool						LoadMA( const char *filename );
	bool						LoadProxy( const char *filename );

	bool						ConvertASEToModelSurfaces( const struct aseModel_s *ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject *lwo );
	bool						ConvertMAToModelSurfaces (const struct maModel_s *ma );

	struct aseModel_s *			ConvertLWOToASE( const struct st_lwObject *obj, const char *fileName );

	void						TransformModel( const idRenderModelStatic *sourceModel, const idMat3 &rotation );
	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId( void );
	bool						FindSurfaceWithId( int id, int &surfaceNum );

public:
	idList<modelSurface_t>		surfaces;
	idBounds					bounds;
	int							overlaysAdded;

protected:
	int							lastModifiedFrame;
	int							lastArchivedFrame;

	idStr						name;
	srfTriangles_t *			shadowHull;
	bool						isStaticWorldModel;
	bool						defaulted;
	bool						purged;					// eventually we will have dynamic reloading
	bool						fastLoad;				// don't generate tangents and shadow data
	bool						reloadable;				// if not, reloadModels won't check timestamp
	bool						levelLoadReferenced;	// for determining if it needs to be freed
	ID_TIME_T					timeStamp;
	int							loadVersion;			// stgatilov: how many times this model was loaded
	idStr						proxySourceName;		// stgatilov #4970: name of the source model (only for proxy models)

	static idCVar				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static idCVar				r_slopVertex;			// merge xyz coordinates this far apart
	static idCVar				r_slopTexCoord;			// merge texture coordinates this far apart
	static idCVar				r_slopNormal;			// merge normals that dot less than this
};

/*
===============================================================================

	MD5 animated model

===============================================================================
*/

class idMD5Mesh {
	friend class				idRenderModelMD5;

public:
								idMD5Mesh();
								~idMD5Mesh();

 	void						ParseMesh( idLexer &parser, int numJoints, const idJointMat *joints, idBounds *jointBounds );
	void						UpdateSurface( const struct renderEntity_s *ent, const idJointMat *joints, modelSurface_t *surf ) const;
	idBounds					CalcBounds( const idJointMat *joints );
	int							NearestJoint( int a, int b, int c ) const;
	int							NumVerts( void ) const;
	int							NumTris( void ) const;
	int							NumWeights( void ) const;
	idVec3						UpdateVertex( const struct renderEntity_s *ent, const idJointMat *joints, int v ) const;

private:
	idList<idVec2>				texCoords;			// texture coordinates
	int							numWeights;			// number of weights
	idVec4 *					scaledWeights;		// joint weights
	int *						weightIndex;		// pairs of: joint offset + bool true if next weight is for next vertex
	const idMaterial *			shader;				// material applied to mesh
	int							numTris;			// number of triangles
	struct deformInfo_s *		deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh
	idList<int>					vertexStarts;		// stgatilov: which is first pair in weightIndex of k-th vertex?

	void						TransformVerts( idDrawVert *verts, const idJointMat *joints ) const;
	void						TransformScaledVerts( idDrawVert *verts, const idJointMat *joints, float scale ) const;
};

class idRenderModelMD5 : public idRenderModelStatic {
public:
	virtual void				InitFromFile( const char *fileName ) override;
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;
	virtual const idBounds *	JointBounds() const override;
	virtual void				Print() const override;
	virtual void				List() const override;
	virtual void				TouchData() override;
	virtual void				PurgeModel() override;
	virtual void				LoadModel() override;
	virtual int					Memory() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual int					NumJoints( void ) const override;
	virtual const idMD5Joint *	GetJoints( void ) const override;
	virtual jointHandle_t		GetJointHandle( const char *name ) const override;
	virtual const char *		GetJointName( jointHandle_t handle ) const override;
	virtual const idJointQuat *	GetDefaultPose( void ) const override;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const override;
	virtual void				GenerateSamples( idList<samplePointOnModel_t> &samples, const modelSamplingParameters_t &params, idRandom &rnd ) const override;
	virtual idVec3				GetSamplePosition( const struct renderEntity_s *ent, const samplePointOnModel_t &sample ) const override;
	virtual const idMaterial *	GetSampleMaterial( const struct renderEntity_s *ent, const samplePointOnModel_t &sample ) const override;

private:
	idList<idMD5Joint>			joints;
	idList<idJointQuat>			defaultPose;
	idList<idMD5Mesh>			meshes;
	idList<idBounds>			jointBounds;	// #6099: includes all vertices connected to each joint

	void						CalculateBounds( const idJointMat *joints );
	void						DrawJoints( const renderEntity_t *ent, const struct viewDef_s *view ) const;
	void						ParseJoint( idLexer &parser, idMD5Joint *joint, idJointQuat *defaultPose );
};

/*
===============================================================================

	MD3 animated model

===============================================================================
*/

struct md3Header_s;
struct md3Surface_s;

class idRenderModelMD3 : public idRenderModelStatic {
public:
	idRenderModelMD3() : md3( nullptr ) {}
	virtual void				InitFromFile( const char *fileName ) override;
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;

private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s *		md3;			// only if type == MOD_MESH
	int							numLods;

	void						LerpMeshVertexes( srfTriangles_t *tri, const struct md3Surface_s *surf, const float backlerp, const int frame, const int oldframe ) const;
};

/*
===============================================================================

	Liquid model

===============================================================================
*/

class idRenderModelLiquid : public idRenderModelStatic {
public:
								idRenderModelLiquid();

	virtual void				InitFromFile( const char *fileName ) override;
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;

	virtual void				Reset() override;
	void						IntersectBounds( const idBounds &bounds, float displacement );

private:
	modelSurface_t				GenerateSurface( float lerp );
	void						WaterDrop( int x, int y, float *page );
	void						Update( void );
						
	int							verts_x;
	int							verts_y;
	float						scale_x;
	float						scale_y;
	float						tile_x;
	float						tile_y;
	int							time;
	int							liquid_type;
	int							update_tics;
	int							seed;

	idRandom					random;
						
	const idMaterial *			shader;
	struct deformInfo_s	*		deformInfo;		// used to create srfTriangles_t from base frames
											// and new vertexes
						
	float						density;
	float						drop_height;
	int							drop_radius;
	float						drop_delay;

	idList<float>				pages;
	float *						page1;
	float *						page2;

	idList<idDrawVert>			verts;

	int							nextDropTime;

};

/*
===============================================================================

	PRT model

===============================================================================
*/

class idRenderModelPrt : public idRenderModelStatic {
public:
								idRenderModelPrt();

	virtual void				InitFromFile( const char *fileName ) override;
	virtual void				TouchData() override;
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;
	virtual float				DepthHack() const override;
	virtual int					Memory() const override;
	
public:																	
	float						SofteningRadius( const int stage ) const;	// #3878 soft particles
private:
	void						SetSofteningRadii();

private:
	const idDeclParticle *		particleSystem;
	idList<float>				softeningRadii;
};

/*
===============================================================================

	Beam model

===============================================================================
*/

class idRenderModelBeam : public idRenderModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual bool				IsLoaded() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;
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

class idRenderModelTrail : public idRenderModelStatic {
	idList<Trail_t>				trails;
	int							numActive;
	idBounds					trailBounds;

public:
								idRenderModelTrail();

	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual bool				IsLoaded() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;

	int							NewTrail( idVec3 pt, int duration );
	void						UpdateTrail( int index, idVec3 pt );
	void						DrawTrail( int index, const struct renderEntity_s *ent, srfTriangles_t *tri, float globalAlpha );
};

/*
===============================================================================

	Lightning model

===============================================================================
*/

class idRenderModelLightning : public idRenderModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual bool				IsLoaded() const override;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const override;
};

/*
================================================================================

	idRenderModelSprite 

================================================================================
*/
class idRenderModelSprite : public idRenderModelStatic {
public:
	virtual	dynamicModel_t	IsDynamicModel() const override;
	virtual	bool			IsLoaded() const override;
	virtual	idRenderModel *	InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel ) override;
	virtual	idBounds		Bounds( const struct renderEntity_s *ent ) const override;
};

#endif /* !__MODEL_LOCAL_H__ */
