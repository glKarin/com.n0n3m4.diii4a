// RenderLight.h
//
class rvmOcclusionQuery; //karin: move class rvmOcclusionQuery to header file

class idRenderLightLocal : public idRenderLight {
public:
	idRenderLightLocal();

	virtual void			FreeRenderLight();
	virtual void			UpdateRenderLight(const renderLight_t* re, bool forceUpdate = false);
	virtual void			GetRenderLight(renderLight_t* re);
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	bool					IsVisible();

public:
	void					CreateLightRefs(void);
	void					DeriveLightData(void);
	void					FreeLightDefDerivedData(void);
private:
	void					MakeShadowFrustums(void);
	void					FreeLightDefFrustum(void);
	void					SetLightFrustum(const idPlane lightProject[4], idPlane frustum[6]);
	void					SetLightProject(idPlane lightProject[4], const idVec3 origin, const idVec3 target, const idVec3 rightVector, const idVec3 upVector, const idVec3 start, const idVec3 stop);
public:	

	renderLight_t			parms;					// specification

	bool					lightHasMoved;			// the light has changed its position since it was
													// first added, so the prelight model is not valid

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	idRenderWorldLocal* world;
	int						index;					// in world lightdefs

	int						areaNum;				// if not -1, we may be able to cull all the light's
													// interactions if !viewDef->connectedAreas[areaNum]

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory
	bool					archived;				// for demo writing

	idBounds				globalLightBounds;


	// derived information
	idPlane					lightProject[4];
	idRenderMatrix			baseLightProject;		// global xyz1 to projected light strq
	idRenderMatrix			inverseBaseLightProject;// transforms the zero-to-one cube to exactly cover the light in world space

	const idMaterial* lightShader;			// guaranteed to be valid, even if parms.shader isn't
	idImage* falloffImage;

	idVec3					globalLightOrigin;		// accounting for lightCenter and parallel


	idPlane					frustum[6];				// in global space, positive side facing out, last two are front/back
	idWinding* frustumWindings[6];		// used for culling
	srfTriangles_t* frustumTris;			// triangulated frustumWindings[]

	int						numShadowFrustums;		// one for projected lights, usually six for point lights
	shadowFrustum_t			shadowFrustums[6];

	int						viewCount;				// if == tr.viewCount, the light is on the viewDef->viewLights list
	struct idRenderLightCommitted* viewLight;

	areaReference_t* references;				// each area the light is present in will have a lightRef

	// Shadow Matrixes 
	idRenderMatrix			shadowMatrix[6];

	bool					lightRendered;

	int						visibleFrame;
	class rvmOcclusionQuery* currentOcclusionQuery;

	struct doublePortal_s* foggedPortals;
};