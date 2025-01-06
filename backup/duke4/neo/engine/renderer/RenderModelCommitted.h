// RenderModelCommitted.h
//

// a idRenderModelCommitted is created whenever a idRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// idRenderModelCommitted are allocated on the frame temporary stack memory
// a viewEntity contains everything that the back end needs out of a idRenderEntityLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// A single entityDef can generate multiple idRenderModelCommitted in a single frame, as when seen in a mirror

//
// idRenderModelCommitted
//
class idRenderModelCommitted {
public:
	idScreenRect					CalcEntityScissorRectangle(void);
	idRenderModel*					CreateDynamicModel(void);
	void							DeformDrawSurf(drawSurf_t* drawSurf) const;
	void							AddDrawsurfs(int committedRenderModelId, const idRenderLightCommitted *lightDefs);
private:
	void							GenerateSurfaceLights(int committedRenderModelId, drawSurf_t* newDrawSurf, const idRenderLightCommitted* lightDefs);
public:

	idRenderModelCommitted* next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	idRenderEntityLocal* entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the idRenderModelCommitted was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewEntity can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	idScreenRect		scissorRect;

	idRenderModel* renderModel;

	float		transposedModelViewMatrix[16];

	bool				weaponDepthHack;
	float				modelDepthHack;

	float				modelMatrix[16];		// local coords to global coords
	float				modelViewMatrix[16];	// local coords to eye coords
	idRenderMatrix		mvp;
};