// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "ImposterGeometry.h"
#include "renderer/tr_local.h"
#include "decllib/declImposter.h"

sdImposterGeometry::sdImposterGeometry(void)
    : declImposter(NULL),
        triSurf(NULL)
{

}

//sdImposterGeometry::~sdImposterGeometry(void) {}

void sdImposterGeometry::Purged(void)
{
    if (triSurf) {
        R_FreeStaticTriSurf(triSurf);
        triSurf = NULL;
    }
}

void sdImposterGeometry::Init(const sdDeclImposter *decl) {
    int num;
    idDrawVert *dv;
    glIndex_t *idx;

    Purged();

    if (!decl->IsValid() || decl->IsImplicit())
        return;

    num = decl->GetNumAngles();

    declImposter = decl;
    triSurf = R_AllocStaticTriSurf();

    //num = 1;
    triSurf->numVerts = 4 * num;
    triSurf->numIndexes = 6 * num;
    R_AllocStaticTriSurfVerts(triSurf, triSurf->numVerts);
    R_AllocStaticTriSurfIndexes(triSurf, triSurf->numIndexes);

    float rad = 0.0f;
    float unitDeg = 360.0f / (float)num;

    byte red = 0;
    byte green = 0;
    byte blue = 0;
    byte alpha = 255;

    for (int i = 0; i < num; i++) {
        const sdImposterSubImage &subImage = declImposter->GetSubImage(i);
		dv = &triSurf->verts[i * 4];
		idx = &triSurf->indexes[i * 6];

        float width = declImposter->GetScaleX();
        float height = declImposter->GetScaleY();
		idVec2 mins = subImage.GetMins() * 2.0f - vec2_one;
		idVec2 maxs = subImage.GetMaxs() * 2.0f - vec2_one;
        float x1 = mins[0] * width;
        float y1 = mins[1] * height;
        float x2 = maxs[0] * width;
        float y2 = maxs[1] * height;

        dv[ 0 ].Clear();
        dv[ 0 ].normal.Set(1.0f, 0.0f, 0.0f);
        dv[ 0 ].tangents[0].Set(0.0f, 1.0f, 0.0f);
        dv[ 0 ].tangents[1].Set(0.0f, 0.0f, 1.0f);
        dv[ 0 ].st = subImage.GetTexCoord(0);
        dv[ 0 ].color[ 0 ] = red;
        dv[ 0 ].color[ 1 ] = green;
        dv[ 0 ].color[ 2 ] = blue;
        dv[ 0 ].color[ 3 ] = alpha;
	    dv[ 0 ].xyz.Set(0.0f, x2, y2);
	    dv[ 0 ].xyz += declImposter->GetOrigin();

        dv[ 1 ].Clear();
        dv[ 1 ].normal.Set(1.0f, 0.0f, 0.0f);
        dv[ 1 ].tangents[0].Set(0.0f, 1.0f, 0.0f);
        dv[ 1 ].tangents[1].Set(0.0f, 0.0f, 1.0f);
        dv[ 1 ].st = subImage.GetTexCoord(1);
        dv[ 1 ].color[ 0 ] = red;
        dv[ 1 ].color[ 1 ] = green;
        dv[ 1 ].color[ 2 ] = blue;
        dv[ 1 ].color[ 3 ] = alpha;
	    dv[ 1 ].xyz.Set(0.0f, x1, y2);
	    dv[ 1 ].xyz += declImposter->GetOrigin();

        dv[ 2 ].Clear();
        dv[ 2 ].normal.Set(1.0f, 0.0f, 0.0f);
        dv[ 2 ].tangents[0].Set(0.0f, 1.0f, 0.0f);
        dv[ 2 ].tangents[1].Set(0.0f, 0.0f, 1.0f);
        dv[ 2 ].st = subImage.GetTexCoord(2);
        dv[ 2 ].color[ 0 ] = red;
        dv[ 2 ].color[ 1 ] = green;
        dv[ 2 ].color[ 2 ] = blue;
        dv[ 2 ].color[ 3 ] = alpha;
	    dv[ 2 ].xyz.Set(0.0f, x1, y1);
	    dv[ 2 ].xyz += declImposter->GetOrigin();

        dv[ 3 ].Clear();
        dv[ 3 ].normal.Set(1.0f, 0.0f, 0.0f);
        dv[ 3 ].tangents[0].Set(0.0f, 1.0f, 0.0f);
        dv[ 3 ].tangents[1].Set(0.0f, 0.0f, 1.0f);
        dv[ 3 ].st = subImage.GetTexCoord(3);
        dv[ 3 ].color[ 0 ] = red;
        dv[ 3 ].color[ 1 ] = green;
        dv[ 3 ].color[ 2 ] = blue;
        dv[ 3 ].color[ 3 ] = alpha;
	    dv[ 3 ].xyz.Set(0.0f, x2, y1);
	    dv[ 3 ].xyz += declImposter->GetOrigin();

        int baseVert = dv - triSurf->verts;
        idx[ 0 ] = baseVert + 0;
        idx[ 1 ] = baseVert + 1;
        idx[ 2 ] = baseVert + 3;
        idx[ 3 ] = baseVert + 1;
        idx[ 4 ] = baseVert + 2;
        idx[ 5 ] = baseVert + 3;

        if (i > 0)
        {
            idMat3 mat;
            idAngles::YawToMat3(unitDeg * (float)i, mat);
            float modelMatrix[16];
            R_AxisToModelMatrix(mat, vec3_origin, modelMatrix);
            idVec3 tmp;
            for (int j = 0; j < 4; j++)
            {
                tmp = dv[j].xyz; R_LocalPointToGlobal(modelMatrix, tmp, dv[j].xyz);
                tmp = dv[j].normal; R_LocalVectorToGlobal(modelMatrix, tmp, dv[j].normal);
                tmp = dv[j].tangents[0]; R_LocalVectorToGlobal(modelMatrix, tmp, dv[j].tangents[0]);
                tmp = dv[j].tangents[1]; R_LocalVectorToGlobal(modelMatrix, tmp, dv[j].tangents[1]);
            }
        }
    }

    R_BoundTriSurf(triSurf);
}



sdImposterGeometryManager::sdImposterGeometryManager(void) {

}

//sdImposterGeometryManager::~sdImposterGeometryManager(void) {}

void sdImposterGeometryManager::Init(void) {
    list.Clear();
}

void sdImposterGeometryManager::Shutdown(void) {
    for (int i = 0; i < list.Num(); i++) {
        list[i].Purged();
    }
    list.Clear();
}

void sdImposterGeometryManager::Clear(void) {
    idRenderModelStatic *model;

    for (int i = 0; i < list.Num(); i++) {
        model = GetModel(list[i].GetDeclImposter());
        if (model)
            renderModelManager->RemoveModel(model);
    }
}

const sdImposterGeometry * sdImposterGeometryManager::Get(const char *name) {
    for (int i = 0; i < list.Num(); i++) {
        if (!idStr::Icmp(list[i].GetDeclImposter()->GetName(), name))
            return &list[i];
    }
    return NULL;
}

const sdImposterGeometry * sdImposterGeometryManager::Get(const sdDeclImposter *imposter) {
    for (int i = 0; i < list.Num(); i++) {
        if (list[i].GetDeclImposter() == imposter)
            return &list[i];
    }
    return NULL;
}

const sdImposterGeometry * sdImposterGeometryManager::Find(const char *name) {
    const sdImposterGeometry *item = Get( name );
    if (item)
        return item;

    const idDecl *decl = declManager->FindType(DECL_IMPOSTER, name, false);
    if (!decl || !decl->IsValid() || decl->IsImplicit())
        return NULL;

    const sdDeclImposter *declImposter = static_cast<const sdDeclImposter *>(decl);

    sdImposterGeometry *newItem = &list.Alloc();
    newItem->Init(declImposter);
    return newItem;
}

const sdImposterGeometry * sdImposterGeometryManager::Find(const sdDeclImposter *imposter) {
    if (!imposter || !imposter->IsValid() || imposter->IsImplicit())
        return NULL;

    const sdImposterGeometry *item = Get( imposter );
    if (item)
        return item;

    sdImposterGeometry *newItem = &list.Alloc();
    newItem->Init(imposter);
    return newItem;
}

idRenderModelStatic * sdImposterGeometryManager::GetModel(const char *name) {
    return static_cast<idRenderModelStatic *>(renderModelManager->GetModel(name));
}

idRenderModelStatic * sdImposterGeometryManager::GetModel(const sdDeclImposter *imposter) {
    if (!imposter)
        return NULL;
    return static_cast<idRenderModelStatic *>(renderModelManager->GetModel(imposter->GetName()));
}

idRenderModelStatic * sdImposterGeometryManager::FindModel(const char *name) {
    idRenderModelStatic *model = GetModel(name);
    if (model)
        return model;

    const sdImposterGeometry *gemo = Find( name );
    if (!gemo)
        return NULL;

    sdRenderModelImposter *imposterModel = new sdRenderModelImposter;
    imposterModel->InitFromImposterGeometry(gemo);
    renderModelManager->AddModel(imposterModel);

    return imposterModel;
}

idRenderModelStatic * sdImposterGeometryManager::FindModel(const sdDeclImposter *imposter) {
	if(!imposter)
		return NULL;

    idRenderModelStatic *model = GetModel(imposter);
    if (model)
        return model;

    const sdImposterGeometry *gemo = Find( imposter );
    if (!gemo)
        return NULL;

    sdRenderModelImposter *imposterModel = new sdRenderModelImposter;
    imposterModel->InitFromImposterGeometry(gemo);
    renderModelManager->AddModel(imposterModel);

    return imposterModel;
}



sdRenderModelImposter::sdRenderModelImposter(void)
    : imposterGeometry(NULL)
{

}

void sdRenderModelImposter::InitFromFile(const char* fileName) {
    name = fileName;
	imposterGeometry = imposterGeometryManager->Find(name);
    LoadModel();
}

dynamicModel_t sdRenderModelImposter::IsDynamicModel() const {
    return DM_STATIC;
}

void sdRenderModelImposter::UpdateSurface(modelSurface_t *surf) const {
    srfTriangles_t *tri;
    idDrawVert *dv;
    glIndex_t *idx;
    idDrawVert *src;
    const srfTriangles_t *triSurf = imposterGeometry->GetTriTriangles();

    surf->material = imposterGeometry->GetDeclImposter()->GetMaterial();

    if (surf->geometry) {
		R_FreeStaticTriSurf(surf->geometry);
	}
	surf->geometry = R_AllocStaticTriSurf();

    tri = surf->geometry;
    tri->numVerts = triSurf->numVerts;
    tri->numIndexes = triSurf->numIndexes;

    R_AllocStaticTriSurfVerts(tri, tri->numVerts);
    R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);
    dv = tri->verts;
    idx = tri->indexes;
    src = triSurf->verts;

    for (int i = 0; i < triSurf->numVerts; i++, src++, dv++) {
        *dv = *src;
        // dv->color[0] = instInfo->inst.color[0];
        // dv->color[1] = instInfo->inst.color[1];
        // dv->color[2] = instInfo->inst.color[2];
        // dv->color[3] = instInfo->inst.color[3];
    }

    for (int i = 0; i < triSurf->numIndexes; i++) {
        idx[i] = triSurf->indexes[i];
    }

    R_BoundTriSurf(tri);
}

void sdRenderModelImposter::LoadModel(void) {
	if(!imposterGeometry)
	{
		PurgeModel();
		MakeDefaultModel();
		return;
	}

    modelSurface_t *surf;
	bounds.Clear();

	surfaces.SetNum(1);
	surf = &surfaces[0];
	surf->geometry = NULL;
	surf->material = NULL;
	surf->id = 0;

	UpdateSurface(surf);
}

void sdRenderModelImposter::InitFromImposterGeometry(const sdImposterGeometry* imposter) {
    name = imposter->GetDeclImposter()->GetName();
    imposterGeometry = imposter;

	LoadModel();
}

void sdRenderModelImposter::PurgeModel(void) {
    imposterGeometry = NULL;
	idRenderModelStatic::PurgeModel();
}



static sdImposterGeometryManager imposterGeometryManagerLocal;
sdImposterGeometryManager *imposterGeometryManager = &imposterGeometryManagerLocal;
