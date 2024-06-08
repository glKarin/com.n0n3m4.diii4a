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

class idJointBuffer;
class idRenderWorldCommitted;

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

	virtual bool				IsSkeletalMesh() const override { return false; }

	virtual void				InitFromFile( const char *fileName );
	virtual void				PartialInitFromFile( const char *fileName );
	virtual void				PurgeModel();
	virtual void				Reset() {};
	virtual void				LoadModel();
	virtual bool				IsLoaded();
	virtual void				SetLevelLoadReferenced( bool referenced );
	virtual bool				IsLevelLoadReferenced();
	virtual void				TouchData();
	virtual void				InitEmpty( const char *name );
	virtual void				AddSurface( idModelSurface surface );
	virtual void				FinishSurfaces();
	virtual void				FreeVertexCache();
	virtual const char *		Name() const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual int					Memory() const;
	virtual ID_TIME_T				Timestamp() const;
	virtual int					NumSurfaces() const;
	virtual int					NumBaseSurfaces() const;
	virtual const idModelSurface *Surface( int surfaceNum ) const;
	virtual srfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const;
	virtual void				FreeSurfaceTriangles( srfTriangles_t *tris ) const;
	virtual srfTriangles_t *	ShadowHull() const;
	virtual bool				IsStaticWorldModel() const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsDefaultModel() const;
	virtual bool				IsReloadable() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual int					NumJoints( void ) const;
	virtual const idMD5Joint *	GetJoints( void ) const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const idJointQuat *	GetDefaultPose( void ) const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;
	virtual void				ReadFromDemoFile( class idDemoFile *f );
	virtual void				WriteToDemoFile( class idDemoFile *f );
	virtual float				DepthHack() const;
// jmarshall
	virtual bool				IsVertexAnimated() const override { return false; }
	virtual bool				CollisionRequiresInstantiate() { return false; }
// jmarshall end

	void						MakeDefaultModel();
	
	bool						LoadASE( const char *fileName );
	bool						LoadLWO( const char *fileName );
	bool						LoadFLT( const char *fileName );
	bool						LoadMA( const char *filename );

	bool						ConvertASEToModelSurfaces( const struct aseModel_s *ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject *lwo );
	bool						ConvertMAToModelSurfaces (const struct maModel_s *ma );

	struct aseModel_s *			ConvertLWOToASE( const struct st_lwObject *obj, const char *fileName );
// jmarshall
	void						LoadPlanes(const char* fileName);

	bool						LoadOBJ(const char* fileName);
	void						ParseOBJ(rvmListSTL<idDrawVert>& drawVerts, const char* fileName, const char* objFileBuffer, int length);
// jmarshall end

	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId( void );
	bool						FindSurfaceWithId( int id, int &surfaceNum );

public:
	idList<idModelSurface>		surfaces;
	idBounds					bounds;
	int							overlaysAdded;
	int							cacheFrame;

	// when an md5 is instantiated, the inverted joints array is stored to allow GPU skinning
	int							numInvertedJoints;
	idJointMat*					jointsInverted;

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
	ID_TIME_T						timeStamp;

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

	void						ParseMesh(const idStr& fileName, idLexer& parser, int numJoints, const idJointMat* joints);
	idBounds					CalcBounds(const idJointMat* joints);
	int							NearestJoint(int a, int b, int c) const;
	int							NumVerts(void) const;
	int							NumTris(void) const;
	int							NumWeights(void) const;
	const idMaterial* GetShader(void) const { return shader; }
public:
	struct deformInfo_s* deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
private:
	idList<idVec2>				texCoords;			// texture coordinates
	int							numWeights;			// number of weights
	idVec4* scaledWeights;		// joint weights
	int* weightIndex;		// pairs of: joint offset + bool true if next weight is for next vertex
	const idMaterial* shader;				// material applied to mesh
	int							numTris;			// number of triangles	
	int							surfaceNum;			// number of the static surface created for this mesh

	int							numMeshJoints;
	float						maxJointVertDist;
	byte* meshJoints;
	idBounds					bounds;
};

class idRenderModelMD5 : public idRenderModelStatic {
public:
	// jmarshall
	idRenderModelMD5();
	~idRenderModelMD5();
	// jmarshall end

	virtual void				InitFromFile(const char* fileName) override;
	virtual dynamicModel_t		IsDynamicModel() const override;
	virtual idBounds			Bounds(const renderEntity_t* ent = NULL) const override;
	virtual void				Print() const override;
	virtual void				List() const override;
	virtual void				TouchData() override;
	virtual idRenderModel* InstantiateDynamicModel(const struct renderEntity_t* ent, const idRenderWorldCommitted* view, idRenderModel* cachedModel) override;
	virtual void				PurgeModel() override;
	virtual void				LoadModel() override;
	virtual int					Memory() const override;
	virtual int					NumJoints(void) const override;
	virtual const idMD5Joint* GetJoints(void) const override;
	virtual jointHandle_t		GetJointHandle(const char* name) const override;
	virtual const char* GetJointName(jointHandle_t handle) const override;
	virtual const idJointQuat* GetDefaultPose(void) const override;
	virtual int					NearestJoint(int surfaceNum, int a, int b, int c) const override;
private:
	idJointBuffer* jointBuffer;

	idList<idMD5Joint>			joints;
	idList<idJointQuat>			defaultPose;
	idList<idMD5Mesh>			meshes;
	// jmarshall
	idJointMat* poseMat3;
	idList<idJointMat>			invertedDefaultPose;
	// jmarshall end
	void						CalculateBounds(const idJointMat* joints);
	void						DrawJoints(const renderEntity_t* ent, const struct renderView_t* view) const;
	void						ParseJoint(idLexer& parser, idMD5Joint* joint, idJointQuat* defaultPose);
};

//
// idRenderModelMD5Instance
//
class idRenderModelMD5Instance : public idRenderModelStatic {
public:
	virtual bool				IsSkeletalMesh() const override;
	void						CreateStaticMeshSurfaces(const idList<idMD5Mesh>& meshes);

	void						UpdateSurfaceGPU(struct deformInfo_s* deformInfo, const renderEntity_t* ent, idModelSurface* surf, const idMaterial* shader);
public:
	idJointBuffer* jointBuffer; // This is shared across all models of the same instance!
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
	idRenderModelMD3(); //k64
	virtual void				InitFromFile( const char *fileName );
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;
// jmarshall
	virtual bool				IsVertexAnimated() const override { return true; }
	virtual bool				CollisionRequiresInstantiate() { return true; }
// jmarshall end
private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s *		md3;			// only if type == MOD_MESH
	int							numLods;
	idList<const idMaterial*>	shaders;		// DG: md3Shader_t::shaderIndex indexes into this array //k64

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
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;

	virtual void				Reset();
	void						IntersectBounds( const idBounds &bounds, float displacement );

private:
	idModelSurface				GenerateSurface( float lerp );
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
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;
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
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;
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
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;

	int							NewTrail( idVec3 pt, int duration );
	void						UpdateTrail( int index, idVec3 pt );
	void						DrawTrail( int index, const struct renderEntity_t *ent, srfTriangles_t *tri, float globalAlpha );
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
	virtual idRenderModel *		InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual idBounds			Bounds( const struct renderEntity_t *ent ) const;
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
	virtual	idRenderModel *	InstantiateDynamicModel( const struct renderEntity_t *ent, const idRenderWorldCommitted *view, idRenderModel *cachedModel );
	virtual	idBounds		Bounds( const struct renderEntity_t *ent ) const;
};

#endif /* !__MODEL_LOCAL_H__ */
