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
#ifdef _RAVEN_FX
	public:
		rvRenderModelBSE();

		virtual void				InitFromFile(const char *fileName);
		virtual void				TouchData();
		virtual dynamicModel_t		IsDynamicModel() const;
#ifdef _RAVEN
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask);
#else
		virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif
		virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
		virtual float				DepthHack() const;
		virtual int					Memory() const;

	private:
		const rvBSEParticle 		*particleSystem;
		idBounds					bounds;
#else
    public:
		rvRenderModelBSE();
        virtual void				InitFromFile(const char* fileName);
        virtual void				FinishSurfaces(bool useMikktspace);
#endif
};

#endif
