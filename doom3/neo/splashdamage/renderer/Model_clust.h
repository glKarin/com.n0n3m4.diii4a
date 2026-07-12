// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MODEL_CLUST_H__
#define __MODEL_CLUST_H__

//karin: update models on every frame
//#define _DYNAMIC_STUFF_CLUST 1

class sdDeclStuffType;

class sdRenderModelStuffInstance
{
    public:
                                sdRenderModelStuffInstance(void);
    bool                        ParseBinary(idFile *file);
    const idVec3 &              GetOrigin(void) const {
        return origin;
    }
    const idVec3 &              GetColor(void) const {
        return color;
    }
    void                        GetModelMatrix(float modelMatrix[16]) const;

private:
    idVec3                      origin;
    idAngles                    angles;
    idVec3                      color;
};

class sdStuffSurface
{
public:
                                sdStuffSurface(void);

    bool                        ParseBinary(idFile *file);
    const idRenderModelStatic * SelectModel(void) const;
    float                       GetInstanceScale() const {
        return instanceScale;
    }
    const idBounds &            GetBounds() const {
        return bounds;
    }
    const idList<sdRenderModelStuffInstance> & GetInstanceList() const {
        return instanceList;
    }

private:
    int                         numInstances;
    const sdDeclStuffType       *stuffType;
    float                       instanceScale;
    idList<sdRenderModelStuffInstance> instanceList;
    idBounds                    bounds;
};

class sdRenderModelClust : public idRenderModelStatic
{
private:
    struct instance_t {
        const sdRenderModelStuffInstance *instance;
        float					    modelMatrix[16];
    };

    struct stuffSurface_t {
        const sdStuffSurface        *stuffSurface;
        const modelSurface_t        *surf;
        idList<instance_t>          instances;
#ifdef _DYNAMIC_STUFF_CLUST
        int                         numVerts;
        int                         numIndexes;
        idList<const instance_t *>  *views;
#endif
    };

public:
                                sdRenderModelClust(void);
    virtual void                InitFromFile(const char* fileName);
    virtual dynamicModel_t		IsDynamicModel() const;
    virtual idRenderModel 		*InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel);
    virtual idBounds			Bounds(const struct renderEntity_s *ent) const;
    void                        LoadModel(void);
    void                        PurgeModel(void);

private:
    bool                        ParseBinary(void);
    void                        Finish(void);

    void                        UpdateInstanceSurface(const instance_t *inst, const stuffSurface_t *stuff, srfTriangles_t *tri, int &vertBase, int &indexBase) const;
    void                        UpdateStuffSurface(stuffSurface_t *stuff, const struct renderEntity_s *ent, const struct viewDef_s *view, modelSurface_t *surf);
#ifdef _DYNAMIC_STUFF_CLUST
    bool                        CheckInstanceVisible(instance_t *inst, const stuffSurface_t *stuff, const struct renderEntity_s *ent, const struct viewDef_s *view, float distance = -1.0f);
    int                         UpdateViews(stuffSurface_t *stuff, const struct renderEntity_s *ent, const struct viewDef_s *view, float distanceSqr = -1.0f);
#endif

private:
    idList<sdStuffSurface>      surfaces;
    idList<stuffSurface_t>      drawSurfaces;
};

#endif /* !__MODEL_CLUST_H__ */
