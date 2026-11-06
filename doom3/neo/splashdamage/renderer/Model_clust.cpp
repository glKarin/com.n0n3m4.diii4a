// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "renderer/tr_local.h"
#include "renderer/Model_local.h"

#include "Model_clust.h"

#define CLUSTB_VERSION "Version 2"

static const char *Clust_SnapshotName = "_Clust_Snapshot_";
static idRandom clustRandom;

#ifdef _DYNAMIC_STUFF_CLUST
static idCVar harm_r_stuffClustDistance("harm_r_stuffClustDistance", "5000", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "max stuff distance with view origin, -1 to no limit");
static idCVar harm_r_skipStuffClust("harm_r_skipStuffClust", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "skip stuff clust model rendering");
#endif

sdRenderModelStuffInstance::sdRenderModelStuffInstance(void)
{

}

bool sdRenderModelStuffInstance::ParseBinary(idFile *file)
{
    file->ReadVec3(origin);
    file->ReadAngles(angles);
    file->ReadVec3(color);

    return true;
}

void sdRenderModelStuffInstance::GetModelMatrix(float modelMatrix[16]) const {
    //rotation = angles.ToMat3();
    R_AxisToModelMatrix(angles.ToMat3(), origin, modelMatrix);
}



sdStuffSurface::sdStuffSurface(void)
    : numInstances(0),
    stuffType(NULL),
    instanceScale(1.0f)
{
    bounds.Clear();
}

bool sdStuffSurface::ParseBinary(idFile *file)
{
    idStr token;
    const idDecl *decl;
    int num;
    sdRenderModelStuffInstance *instance;

    file->ReadInt(numInstances);
    file->ReadString(token);
    decl = declManager->FindType(DECL_STUFFTYPE, token, false);
    if (!decl)
    {
        common->Warning("Stuff type '%s' not found, using default", token.c_str());
        decl = declManager->FindType(DECL_STUFFTYPE, token, true);
    }
    stuffType = static_cast<const sdDeclStuffType *>(decl);
    file->ReadFloat(instanceScale);
    file->ReadInt(num);
    if (num < 0)
    {
        common->Warning("invalid size: %d", num);
        return false;
    }
    instanceList.SetNum(num);
    instance = instanceList.Ptr();
    for (int i = 0; i < num; i++, instance++)
    {
        if (!instance->ParseBinary(file))
            return false;
        bounds.AddPoint(instance->GetOrigin());
    }

    if (!stuffType)
    {
        common->Warning("No stuff type specified in file");
        return false;
    }
    if (instanceList.Num() == 0)
    {
        common->Warning("No models specified in file");
        return false;
    }

    return true;
}

const idRenderModelStatic * sdStuffSurface::SelectModel(void) const {
    const idRenderModel *model;

    if (stuffType->GetNumModels() == 0)
        model = renderModelManager->DefaultModel();
    else
        model = renderModelManager->FindModel(stuffType->GetModelName(clustRandom.RandomInt(stuffType->GetNumModels())));
    return static_cast<const idRenderModelStatic *>(model);
}



sdRenderModelClust::sdRenderModelClust(void)
{
}

void sdRenderModelClust::InitFromFile(const char* fileName)
{
    name = fileName;
    LoadModel();
}

dynamicModel_t sdRenderModelClust::IsDynamicModel() const
{
#ifdef _DYNAMIC_STUFF_CLUST
    return DM_CONTINUOUS;
#else
    return DM_CACHED;
#endif
}

idRenderModel * sdRenderModelClust::InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel)
{
    int					i, surfaceNum;
    idRenderModelStatic	*staticModel;
    modelSurface_t *surf;
    stuffSurface_t *mesh;

    if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
        delete cachedModel;
        cachedModel = NULL;
    }

#ifdef _DYNAMIC_STUFF_CLUST
	if (harm_r_skipStuffClust.GetBool()) {
		delete cachedModel;
		return NULL;
	}
#endif

    if (purged) {
        common->DWarning("model %s instantiated while purged", Name());
        LoadModel();
    }

    // this may be triggered by a model trace or other non-view related source, to which we should look like an empty model
    if (ent == NULL || view == NULL) {
        delete cachedModel;
        return NULL;
    }

    if (cachedModel) {
        assert(dynamic_cast<idRenderModelStatic *>(cachedModel) != NULL);
        assert(idStr::Icmp(cachedModel->Name(), Clust_SnapshotName) == 0);
        staticModel = static_cast<idRenderModelStatic *>(cachedModel);
    } else {
        staticModel = new idRenderModelStatic;
        staticModel->InitEmpty(Clust_SnapshotName);
    }

    staticModel->bounds.Clear();

    mesh = drawSurfaces.Ptr();
#ifdef _DYNAMIC_STUFF_CLUST
    idList<const instance_t *> viewList;
    float distance = harm_r_stuffClustDistance.GetFloat();
    if (distance > 0.0f) {
        distance *= distance;
    }
#endif
    for (i = 0; i < drawSurfaces.Num(); i++, mesh++) {
#ifdef _DYNAMIC_STUFF_CLUST
        mesh->views = &viewList;
        surfaceNum = UpdateViews(mesh, ent, view, distance);

        if (!surfaceNum) {
            staticModel->DeleteSurfaceWithId(i);
            continue;
        }
#endif

        if (staticModel->FindSurfaceWithId(i, surfaceNum)) {
            surf = &staticModel->surfaces[surfaceNum];
        } else {
            surf = &staticModel->surfaces.Alloc();
            surf->geometry = NULL;
            surf->material = NULL;
            surf->id = i;
        }

        UpdateStuffSurface(mesh, ent, view, surf);
    }

    staticModel->bounds = bounds;
    return staticModel;
}

idBounds sdRenderModelClust::Bounds(const struct renderEntity_s *ent) const
{
    return bounds;
}

void sdRenderModelClust::LoadModel(void)
{
    if (!purged) {
        PurgeModel();
    }
	purged = false;

    if (!ParseBinary())
    {
        MakeDefaultModel();
        return;
    }

    Finish();
}

bool sdRenderModelClust::ParseBinary(void)
{
    idFile *file;
    idStr version;

    idStr binPath = name;
    binPath.SetFileExtension(".clustb");

    file = fileSystem->OpenFileRead(binPath);
    if (!file)
        return false;

    file->ReadString(version);
    if (idStr::Icmp(version, CLUSTB_VERSION))
    {
        common->Warning("sdRenderModelStuff::InitFromFile: bad id '%s' instead of '%s'", version.c_str(), CLUSTB_VERSION);
        fileSystem->CloseFile(file);
        return false;
    }

    while (file->Tell() < file->Length())
    {
        sdStuffSurface &surface = surfaces.Alloc();
        if (!surface.ParseBinary(file))
        {
            fileSystem->CloseFile(file);
            return false;
        }
    }

    fileSystem->CloseFile(file);

    bounds.Clear();
    for (int i = 0; i < surfaces.Num(); i++) {
        bounds.AddBounds(surfaces[i].GetBounds());
    }

    return true;
}

void sdRenderModelClust::PurgeModel()
{
    drawSurfaces.Clear();
    surfaces.Clear();
    purged = true;
}

void sdRenderModelClust::Finish(void)
{
    const sdStuffSurface *surface;
    const sdRenderModelStuffInstance *instance;
    const idRenderModelStatic *model;
    stuffSurface_t *stuff;
    instance_t *inst;
    int modelIndex;
    drawSurfaces.Clear();

    surface = surfaces.Ptr();
    for (int i = 0; i < surfaces.Num(); i++, surface++)
    {
        idList<idList<const sdRenderModelStuffInstance *> > instList;
        idList<const idRenderModelStatic *> modelList;

        instance = surface->GetInstanceList().Ptr();
        for (int j = 0; j < surface->GetInstanceList().Num(); j++, instance++)
        {
            model = surface->SelectModel();
            modelIndex = modelList.FindIndex(model);
            if (modelIndex < 0)
            {
                modelIndex = modelList.Append(model);
                instList.Alloc();
            }
            instList[modelIndex].Append(instance);
        }

        for (int j = 0; j < modelList.Num(); j++) {
            model = modelList[j];
            idList<const sdRenderModelStuffInstance *> &list = instList[j];

            stuff = &drawSurfaces.Alloc();
            stuff->surf = &model->surfaces[0];
            stuff->stuffSurface = surface;
            stuff->instances.SetNum(list.Num());
            inst = stuff->instances.Ptr();
            for (int k = 0; k < list.Num(); k++, inst++) {
                inst->instance = list[k];
                inst->instance->GetModelMatrix(inst->modelMatrix);
            }
#ifdef _DYNAMIC_STUFF_CLUST
            stuff->numVerts = 0;
            stuff->numIndexes = 0;
#endif
        }
    }
}

void sdRenderModelClust::UpdateInstanceSurface(const instance_t *inst, const stuffSurface_t *stuff, srfTriangles_t *tri, int &vertBase, int &indexBase) const {
    const modelSurface_t *surf = stuff->surf;
    idDrawVert *dv = tri->verts + vertBase;
    glIndex_t *idx = tri->indexes + indexBase;
    idDrawVert *src = surf->geometry->verts;

    for (int i = 0; i < surf->geometry->numVerts; i++, src++, dv++) {
        dv->Clear();
        dv->st = src->st;
        //idVec3 pos = src->xyz * stuff->stuffSurface->GetInstanceScale() * 0.5f;
        R_LocalPointToGlobal(inst->modelMatrix, src->xyz, dv->xyz);
        R_LocalVectorToGlobal(inst->modelMatrix, src->normal, dv->normal);
        R_LocalVectorToGlobal(inst->modelMatrix, src->tangents[0], dv->tangents[0]);
        R_LocalVectorToGlobal(inst->modelMatrix, src->tangents[1], dv->tangents[1]);
        dv->color[0] = (byte)(inst->instance->GetColor()[0] * 255.0f);
        dv->color[1] = (byte)(inst->instance->GetColor()[1] * 255.0f);
        dv->color[2] = (byte)(inst->instance->GetColor()[2] * 255.0f);
		dv->color[3] = 255;
    }

    for (int i = 0; i < surf->geometry->numIndexes; i++) {
        idx[i] = vertBase + surf->geometry->indexes[i];
    }

    vertBase += surf->geometry->numVerts;
    indexBase += surf->geometry->numIndexes;
}

void sdRenderModelClust::UpdateStuffSurface(stuffSurface_t *stuff, const struct renderEntity_s *ent, const struct viewDef_s *view, modelSurface_t *surf) {
    srfTriangles_t *tri;

    surf->material = stuff->surf->material;

    if (surf->geometry) {
        /*if (stuff->numVerts == surf->geometry->numVerts && stuff->numIndexes == surf->geometry->numIndexes) {
            R_FreeStaticTriSurfVertexCaches(surf->geometry);
        }
        else*/
        {
            R_FreeStaticTriSurf(surf->geometry);
            surf->geometry = R_AllocStaticTriSurf();
        }
    } else {
        surf->geometry = R_AllocStaticTriSurf();
    }

    tri = surf->geometry;
#ifdef _DYNAMIC_STUFF_CLUST
    tri->numVerts = stuff->views->Num() * stuff->surf->geometry->numVerts;
    tri->numIndexes = stuff->views->Num() * stuff->surf->geometry->numIndexes;

    stuff->numVerts = tri->numVerts;
    stuff->numIndexes = tri->numIndexes;
#else
    tri->numVerts = stuff->instances.Num() * stuff->surf->geometry->numVerts;
    tri->numIndexes = stuff->instances.Num() * stuff->surf->geometry->numIndexes;
#endif

    R_AllocStaticTriSurfVerts(tri, tri->numVerts);
    R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);
    int vert = 0;
    int index = 0;
#ifdef _DYNAMIC_STUFF_CLUST
    for (int i = 0; i < stuff->views->Num(); i++) {
        UpdateInstanceSurface(stuff->views->operator[](i), stuff, tri, vert, index);
    }
#else
    for (int i = 0; i < stuff->instances.Num(); i++) {
        UpdateInstanceSurface(&stuff->instances[i], stuff, tri, vert, index);
    }
#endif

    R_BoundTriSurf(tri);
}

#ifdef _DYNAMIC_STUFF_CLUST
bool sdRenderModelClust::CheckInstanceVisible(instance_t *inst, const stuffSurface_t *stuff, const struct renderEntity_s *ent, const struct viewDef_s *view, float distanceSqr) {
    if (distanceSqr > 0.0f && (view->renderView.vieworg - (ent->origin + inst->instance->GetOrigin())).LengthSqr() > distanceSqr)
        return false;

    return R_CullLocalBox(stuff->surf->geometry->bounds, inst->modelMatrix, 6, view->frustum);
}

int sdRenderModelClust::UpdateViews(stuffSurface_t *stuff, const struct renderEntity_s *ent, const struct viewDef_s *view, float distanceSqr) {
    instance_t *instance;

    if (stuff->views->NumAllocated() < stuff->instances.Num()) {
        stuff->views->Clear();
        stuff->views->Resize(stuff->instances.Num());
    }
    else
        stuff->views->SetNum(0, false);
    instance = stuff->instances.Ptr();
    for (int i = 0; i < stuff->instances.Num(); i++, instance++) {
        if (CheckInstanceVisible(instance, stuff, ent, view, distanceSqr))
            stuff->views->Append(instance);
    }

    return stuff->views->Num();
}
#endif

