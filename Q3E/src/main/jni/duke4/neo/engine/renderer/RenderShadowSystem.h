// RenderShadows.h
//

struct idRenderLightCommitted;

//
// rvmRenderShadowAtlasEntry_t
//
struct rvmRenderShadowAtlasEntry_t {

	rvmRenderShadowAtlasEntry_t();
	void Reset(void);
	void Mark(int uniqueLightId);

	int x;
	int y;
	int sliceSizeX;
	int sliceSizeY;
	float textureX;
	float textureY;
	int uniqueLightId;
	double lastTouchedTime;
};

//
// rvmRenderShadowSystem
//
class rvmRenderShadowSystem {
	friend class rvmRenderShadowAtlasEntry_t;
public:
	rvmRenderShadowSystem();

	// Inits the render shadow system.
	void				Init(void);

	// Shutdowns the render shadow system.
	void				Shutdown(void);

	// Returns the shadow map depth atlas image.
	idImage*			GetShadowMapDepthAtlas(void) { return shadowMapAtlasRT->GetDepthImage(); }

	// Returns the shadow map depth atlas render target.
	idRenderTexture*	GetShadowMapDepthAtlasRT(void) { return shadowMapAtlasRT; }

	// Returns the size of the shadow map;
	int					GetShadowMapSize(void) { return GetShadowMapDepthAtlas()->GetOpts().width; }

	// Returns the atlas lookup texture.
	idImage*			GetAtlasLookupImage(void) { return atlasEntriesLookupTexture; }

	// Returns the sample scale.
	float				GetAtlasSampleScale(void) { return (float)r_shadowMapAtlasSliceSize.GetInteger() / (float)r_shadowMapAtlasSize.GetInteger(); }

	// Returns the size of the shadow map atlas.
	float				GetShadowMapAtlasSize(void) { return (float)r_shadowMapAtlasSize.GetInteger(); }

	// Returns the atlas entry for the requested index.
	rvmRenderShadowAtlasEntry_t* GetShadowAtlasEntry(int idx) { return &shadowAtlasEntries[idx]; }

	// Checks to see if the light is cached.
	int					CheckShadowCache(idRenderLightCommitted *vLight);

	// Finds the next available shadow map.
	int					FindNextAvailableShadowMap(idRenderLightCommitted* vLight, int numSlices);

	// Nukes the shadow map cache.
	void				NukeShadowMapCache(void);

	// Invalidates a specific shadow map in the cache.
	void				InvalidateShadowMapForLight(int uniqueLightId);
private:
	static idCVar		r_shadowMapAtlasSize;
	static idCVar		r_shadowMapAtlasSliceSize;
	static idCVar		r_shadowMapEvictionTime;
private:
	idImage*			atlasEntriesLookupTexture;

	idRenderTexture*	shadowMapAtlasRT;
	rvmRenderShadowAtlasEntry_t* shadowAtlasEntries;
};

extern rvmRenderShadowSystem renderShadowSystem;