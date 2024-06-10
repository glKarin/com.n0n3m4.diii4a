// RenderEntity.h
//

class idRenderEntityLocal : public idRenderEntity {
public:
	idRenderEntityLocal();

	virtual void			FreeRenderEntity();
	virtual void			UpdateRenderEntity(const renderEntity_t* re, bool forceUpdate = false);
	virtual void			GetRenderEntity(renderEntity_t* re);
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay(const idPlane localTextureAxis[2], const idMaterial* material);
	virtual void			RemoveDecals();

public:
	void					CreateEntityRefs(void);
	void					FreeEntityDefDerivedData(bool keepDecals, bool keepCachedDynamicModel);
	void					ClearEntityDefDynamicModel(void);
	void					FreeEntityDefDecals(void);
	void					FreeEntityDefFadedDecals(int time);
	void					FreeEntityDefOverlay(void);
public:
	renderEntity_t			parms;

	idVertexBuffer*			cpuMeshVertexBuffer;	// Each renderEntity_t can have a cpu cloned vertex buffer for writing custom mesh data into.

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	idRenderWorldLocal* world;
	int						index;					// in world entityDefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
													// and should go in the dynamic frame memory, or kept
													// in the cached memory
	bool					archived;				// for demo writing

	idRenderModel* dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
													// dynamicModel if this doesn't == tr.viewCount
	idRenderModel* cachedDynamicModel;

	idBounds				referenceBounds;		// the local bounds used to place entityRefs, either from parms or a model

	// a idRenderModelCommitted is created whenever a idRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;				// if tr.viewCount == viewCount, viewEntity is valid,
													// but the entity may still be off screen
	idRenderModelCommitted* viewEntity;				// in frame temporary memory

	int						visibleCount;
	// if tr.viewCount == visibleCount, at least one ambient
	// surface has actually been added by R_AddAmbientDrawsurfs
	// note that an entity could still be in the view frustum and not be visible due
	// to portal passing

	idRenderModelDecal* decals;					// chain of decals that have been projected on this model
	idRenderModelOverlay* overlay;				// blood overlays on animated models

	areaReference_t* entityRefs;				// chain of all references

	bool					needsPortalSky;
};