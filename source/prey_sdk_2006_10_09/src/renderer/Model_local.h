
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
	virtual						~idRenderModelStatic();

	virtual void				InitFromFile( const char *fileName );
	virtual void				PartialInitFromFile( const char *fileName );
	virtual void				PurgeModel();
	virtual void				Reset() {};
	virtual void				LoadModel();
#ifdef HUMANHEAD		//HUMANHEAD mdc: added to support purging of ppm modelfiles (for use in reloadModels)
	virtual void				PurgePrecompressed();
#endif
	virtual bool				IsLoaded();
	virtual void				SetLevelLoadReferenced( bool referenced );
	virtual bool				IsLevelLoadReferenced();
	virtual void				TouchData();
	virtual void				InitEmpty( const char *name );
	virtual void				AddSurface( modelSurface_t surface );
	virtual void				FinishSurfaces();
	virtual void				FreeVertexCache();
	virtual const char *		Name() const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual int					Memory() const;
	virtual unsigned int		Timestamp() const;
	virtual int					NumSurfaces() const;
	virtual int					NumBaseSurfaces() const;
	virtual const modelSurface_t *Surface( int surfaceNum ) const;
	virtual srfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const;
	virtual void				FreeSurfaceTriangles( srfTriangles_t *tris ) const;
	virtual srfTriangles_t *	ShadowHull() const;
	virtual bool				IsStaticWorldModel() const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsDefaultModel() const;
	virtual bool				IsReloadable() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual int					NumJoints( void ) const;
	virtual const idMD5Joint *	GetJoints( void ) const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const idJointQuat *	GetDefaultPose( void ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual void				ReadFromDemoFile( class idDemoFile *f );
	virtual void				WriteToDemoFile( class idDemoFile *f );
	virtual float				DepthHack() const;

	// HUMANHEAD pdm: Game access to liquid models
	virtual void				IntersectBounds( const idBounds &bounds, float displacement ) {};
	// HUMANHEAD END

	void						MakeDefaultModel();
	
	bool						LoadASE( const char *fileName );
	bool						LoadLWO( const char *fileName );
	bool						LoadFLT( const char *fileName );
	bool						LoadMA( const char *filename );

	// HUMANHEAD mdc - added support for precomputed models
	bool						LoadPPM( const char *fileName );
	bool						WritePPM( const char *fileName );
	void						DeletePPM( const char *fileName );
	// HUMANHEAD END

	bool						ConvertASEToModelSurfaces( const struct aseModel_s *ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject *lwo );
	bool						ConvertMAToModelSurfaces (const struct maModel_s *ma );

	struct aseModel_s *			ConvertLWOToASE( const struct st_lwObject *obj, const char *fileName );

	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId( void );
	bool						FindSurfaceWithId( int id, int &surfaceNum );

#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
	bool						IsGameUpdatedModel(void) { return bIsGUM; }
	void						SetGameUpdatedModel(bool gum) { bIsGUM = gum; }
	idList<modelSurface_t>		&GetSurfaces(void) { return surfaces; }
	void						SetBounds(idBounds &newBounds) { bounds = newBounds; };
#endif //HUMANHEAD END

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
	unsigned					timeStamp;

	// HUMANHEAD pdm
	idRenderModel *				HH_InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	void						HH_UpdateSurface( const struct viewDef_s *view, const struct renderEntity_s *ent, modelSurface_t *surf, const int surfaceIndex);
	// HUMANHEAD END

	static idCVar				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static idCVar				r_slopVertex;			// merge xyz coordinates this far apart
	static idCVar				r_slopTexCoord;			// merge texture coordinates this far apart
	static idCVar				r_slopNormal;			// merge normals that dot less than this

	// HUMANHEAD mdc - added for precomputed model support
	static idCVar				model_usePrebuiltModels;
	static idCVar				model_writePrebuiltModels;
	// HUMANHEAD END

#if _HH_RENDERDEMO_HACKS //HUMANHEAD rww
	bool						bIsGUM;
#endif //HUMANHEAD END
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

 	void						ParseMesh( idLexer &parser, int numJoints, const idJointMat *joints );
	// HUMANHEAD pdm: Added view parm for deformations
	void						UpdateSurface( const struct viewDef_s *view, const struct renderEntity_s *ent, const idJointMat *joints, modelSurface_t *surf, bool calculateTangents );
	idBounds					CalcBounds( const idJointMat *joints );
	int							NearestJoint( int a, int b, int c ) const;
	int							NumVerts( void ) const;
	int							NumTris( void ) const;
	int							NumWeights( void ) const;

private:
	idList<idVec2>				texCoords;			// texture coordinates
	int							numWeights;			// number of weights
#if NEW_MESH_TRANSFORM
	jointWeight_t *				weights;			// joint weights
	idVec4 *					baseVectors;		// base vertex, normal and tangents
	idVec4 *					scaledBaseVectors;	// joint weights
#else
	idVec4 *					scaledWeights;		// joint weights
	int *						weightIndex;		// pairs of: joint offset + bool true if next weight is for next vertex
#endif
	const idMaterial *			shader;				// material applied to mesh
	int							numTris;			// number of triangles
	struct deformInfo_s *		deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh
#if !NEW_MESH_TRANSFORM
	void						TransformVerts( idDrawVert *verts, const idJointMat *joints );
	void						TransformScaledVerts( idDrawVert *verts, const idJointMat *joints, float scale );
#endif

#if NEW_MESH_TRANSFORM
	void						TransformVertsNew( idDrawVert *verts, int numVerts, idBounds &bounds, const idJointMat *joints ) const;
	void						TransformVertsAndTangents( idDrawVert *verts, int numVerts, idBounds &bounds, const idJointMat *joints ) const;
	void						TransformVertsAndTangentsFast( idDrawVert *verts, int numVerts, idBounds &bounds, const idJointMat *joints ) const;
#endif
};

class idRenderModelMD5 : public idRenderModelStatic {
public:
#if NEW_MESH_TRANSFORM
								idRenderModelMD5( void );
	virtual						~idRenderModelMD5( void );
#endif
	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual void				TouchData();
	virtual void				PurgeModel();
	virtual void				LoadModel();
	virtual int					Memory() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual int					NumJoints( void ) const;
	virtual const idMD5Joint *	GetJoints( void ) const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const idJointQuat *	GetDefaultPose( void ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;

private:
	idList<idMD5Joint>			joints;
	idList<idJointQuat>			defaultPose;
	idList<idMD5Mesh>			meshes;

#if NEW_MESH_TRANSFORM
	// added the transforms from skin-space to local joint-space, for each joint, added instantiateAllSurfaces flag for MD5 to MD5R conversion
	idJointMat *				skinSpaceToLocalMats;			// transforms from "skinning space" to local joint space
#endif

	void						CalculateBounds( const idJointMat *joints );
	void						GetFrameBounds( const renderEntity_t *ent, idBounds &bounds ) const;
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
	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;

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

	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;

	virtual void				Reset();
	virtual // HUMANHEAD pdm
	void						IntersectBounds( const idBounds &bounds, float displacement );

private:
	modelSurface_t				GenerateSurface( float lerp );
	void						WaterDrop( int x, int y, float *page );
	void						Update( void );
						
	int							verts_x;
	int							verts_y;
	float						scale_x;
	float						scale_y;
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

	virtual void				InitFromFile( const char *fileName );
	virtual void				TouchData();
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual float				DepthHack() const;
	virtual int					Memory() const;

private:
	const idDeclParticle *		particleSystem;
};

/*
===============================================================================

	Beam model

===============================================================================
*/

class idRenderModelBeam : public idRenderModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;
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

	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;

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
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_s *ent ) const;
};

/*
================================================================================

	idRenderModelSprite 

================================================================================
*/
class idRenderModelSprite : public idRenderModelStatic {
public:
	virtual	dynamicModel_t	IsDynamicModel() const;
	virtual	bool			IsLoaded() const;
	virtual	idRenderModel *	InstantiateDynamicModel( const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel );
	virtual	idBounds		Bounds( const struct renderEntity_s *ent ) const;
};


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

#endif /* !__MODEL_LOCAL_H__ */
