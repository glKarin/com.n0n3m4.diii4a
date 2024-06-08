// RenderWorldCommitted.h
//

//
// idRenderWorldCommitted
//
class idRenderWorldCommitted {
public:
	idRenderLightCommitted*				CommitRenderLight(idRenderLightLocal* light);
	idRenderModelCommitted*				CommitRenderModel(idRenderEntityLocal* def);
	void								RenderView(void);
	void								AddDrawViewCmd(void);
	void								SetViewMatrix(void);
private:
	void								SetupViewFrustum(void);
	void								ConstrainViewFrustum(void);
	void								AddModelAndLightRefs(void);
public:
	// specified in the call to DrawScene()
	renderView_t		renderView;

	float				unprojectionToCameraMatrix[16];
	idRenderMatrix		unprojectionToCameraRenderMatrix;

	float				unprojectionToWorldMatrix[16];
	idRenderMatrix		unprojectionToWorldRenderMatrix;

	float				projectionMatrix[16];
	idRenderMatrix		projectionRenderMatrix;	// tech5 version of projectionMatrix
	idRenderModelCommitted		worldSpace;

	idRenderWorldLocal* renderWorld;

	float				floatTime;

	idVec3				initialViewAreaOrigin;
	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.

	bool				isSubview;				// true if this view is not the main view
	bool				isMirror;				// the portal is a mirror, invert the face culling
	bool				isXraySubview;

	bool				isEditor;

	int					numClipPlanes;			// mirrors will often use a single clip plane
	idPlane				clipPlanes[MAX_CLIP_PLANES];		// in world space, the positive side
												// of the plane is the visible side
	idScreenRect		viewport;				// in real pixels and proper Y flip

	idScreenRect		scissor;
	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height

	idRenderWorldCommitted* superView;				// never go into an infinite subview loop 
	struct drawSurf_s* subviewSurface;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t** drawSurfs;				// we don't use an idList for this, because
	int					numDrawSurfs;			// it is allocated in frame temporary memory
	int					maxDrawSurfs;			// may be resized

	struct idRenderLightCommitted* viewLights;			// chain of all viewLights effecting view
	idRenderModelCommitted* viewEntitys;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	idPlane				frustum[5];				// positive sides face outward, [4] is the front clip plane
	idFrustum			viewFrustum;

	int					areaNum;				// -1 = not in a valid area

	bool* connectedAreas;
	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.

};