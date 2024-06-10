// RenderLightCommitted.h
//

class idRenderLightLocal;

//
// idRenderLightAttachedEntity
//
struct idRenderLightAttachedEntity
{
	idRenderEntityLocal* entity;
	float distance;
};

static const float INSIDE_LIGHT_FRUSTUM_SLOP = 32;

// idRenderLightCommitted are allocated on the frame temporary stack memory
// a idRenderLightCommitted contains everything that the back end needs out of an idRenderLightLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum

//
// idRenderLightCommitted
//
class idRenderLightCommitted {
public:
	bool TestPointInViewLight(const idVec3& org, const idRenderLightLocal* light);
	idScreenRect ClippedLightScissorRectangle(void);
	idScreenRect CalcLightScissorRectangle(void);

	struct idRenderLightCommitted* next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	idRenderLightLocal* lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the idRenderModelCommitted was never actually
	// seen through any portals
	idScreenRect			scissorRect;

	// if the view isn't inside the light, we can use the non-reversed
	// shadow drawing, avoiding the draws of the front and rear caps
	bool					viewInsideLight;

	// true if globalLightOrigin is inside the view frustum, even if it may
	// be obscured by geometry.  This allows us to skip shadows from non-visible objects
	bool					viewSeesGlobalLightOrigin;

	// if !viewInsideLight, the corresponding bit for each of the shadowFrustum
	// projection planes that the view is on the negative side of will be set,
	// allowing us to skip drawing the projected caps of shadows if we can't see the face
	int						viewSeesShadowPlaneBits;

	idVec3					globalLightOrigin;			// global light origin used by backend
	idPlane					lightProject[4];			// light project used by backend
	idPlane					fogPlane;					// fog plane for backend fog volume rendering
	const srfTriangles_t* frustumTris;				// light frustum for backend fog volume rendering
	const idMaterial* lightShader;				// light shader used by backend
	const float* shaderRegisters;			// shader registers used by backend
	idImage* falloffImage;				// falloff image used by backend

	int						shadowMapSlice;
};