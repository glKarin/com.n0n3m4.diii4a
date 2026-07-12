// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __IMPOSTERGEOMETRY_H__
#define __IMPOSTERGEOMETRY_H__

#include "renderer/Model_local.h"

class sdDeclImposter;

class sdImposterGeometry
{
public:
                                sdImposterGeometry(void);
                                //~sdImposterGeometry(void);
    void                        Init(const sdDeclImposter *decl);
    void                        Purged(void);
    const idBounds &			Bounds(void) const {
        return bounds;
    }
    const sdDeclImposter        *GetDeclImposter(void) const {
        return declImposter;
    }
    const srfTriangles_t        *GetTriTriangles(void) const {
        return triSurf;
    }

private:
    const sdDeclImposter        *declImposter;
    struct srfTriangles_t       *triSurf;
    idBounds                    bounds;
};

class sdRenderModelImposter : public idRenderModelStatic
{
public:
    sdRenderModelImposter(void);
    virtual void                InitFromFile(const char* fileName);
    void                        InitFromImposterGeometry(const sdImposterGeometry* imposter);
    virtual dynamicModel_t		IsDynamicModel() const;
    void                        LoadModel(void);
    void                        PurgeModel(void);

private:
    void                        UpdateSurface(modelSurface_t *surf) const;

private:
    const sdImposterGeometry    *imposterGeometry;
};

class sdImposterGeometryManager {
public:
                                sdImposterGeometryManager(void);
                                //~sdImposterGeometryManager(void);
    void                        Init(void);
    void                        Shutdown(void);
    void                        Clear(void);
    const sdImposterGeometry    *Find(const char *name);
    const sdImposterGeometry    *Find(const sdDeclImposter *imposter);
    const sdImposterGeometry    *Get(const char *name);
    const sdImposterGeometry    *Get(const sdDeclImposter *imposter);
    idRenderModelStatic         *GetModel(const char *name);
    idRenderModelStatic         *GetModel(const sdDeclImposter *imposter);
    idRenderModelStatic         *FindModel(const char *name);
    idRenderModelStatic         *FindModel(const sdDeclImposter *imposter);

private:
    idList<sdImposterGeometry>  list;
};

extern sdImposterGeometryManager *imposterGeometryManager;

#endif /* !__IMPOSTERGEOMETRY_H__ */
