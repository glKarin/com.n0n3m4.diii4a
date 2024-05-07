#ifndef _RV_MODEL_BSE_H
#define _RV_MODEL_BSE_H

/*
======================
rvRenderModelBSE
======================
*/
#ifdef _RAVEN_FX
class rvBSEParticle;
#endif
class rvRenderModelBSE : public idRenderModelStatic {
public:
#ifdef _RAVEN_FX
	public:
		rvRenderModelBSE();

		virtual void				InitFromFile(const char *fileName);
		virtual void				TouchData();
		virtual dynamicModel_t		IsDynamicModel() const;
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
		virtual float				DepthHack() const;
		virtual int					Memory() const;

	private:
		const rvBSEParticle 		*particleSystem;
		idBounds					bounds;
#else
	virtual void				InitFromFile(const char* fileName);
	virtual void				FinishSurfaces(bool useMikktspace);
#endif
};

#endif
