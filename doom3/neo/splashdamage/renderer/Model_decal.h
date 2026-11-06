// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MODEL_DECAL_H__
#define __MODEL_DECAL_H__

class sdRenderModelDecal : public idRenderModelStatic
{
public:
                                sdRenderModelDecal(void);
	virtual						~sdRenderModelDecal(void);
	virtual void				Reset(void);
	void						SetShader(const idMaterial *mat);
	void						CreateDecal(const idRenderModel *model, const decalProjectionInfo_t &localInfo, const idVec4 &color, const idMaterial** onlyMaterials, const int numOnlyMaterials );

#if 0
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idRenderModel *		InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
#endif

private:
	void						AddWinding(const idFixedWinding &w, const idVec4 &color);

private:
	static const int			MAX_DECAL_VERTS = 0x800u;
	static const int			MAX_DECAL_INDEXES = 0xC00u;
};

#endif /* !__MODEL_DECAL_H__ */

