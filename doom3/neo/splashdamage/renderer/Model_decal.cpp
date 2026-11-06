// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "renderer/tr_local.h"
#include "renderer/Model_local.h"

#include "Model_decal.h"

static int c_numDecalModel = 0;

sdRenderModelDecal::sdRenderModelDecal(void)
{
	surfaces.SetNum(1);

	modelSurface_t &surface = surfaces[0];
	surface.geometry = R_AllocStaticTriSurf();
	surface.material = tr.defaultMaterial;
	surface.id = 0;
	srfTriangles_t *tri = surface.geometry;
	R_AllocStaticTriSurfVerts(tri, MAX_DECAL_VERTS);
	R_AllocStaticTriSurfIndexes(tri, MAX_DECAL_INDEXES);

	tri->tangentsCalculated = true;
	tri->generateNormals = false;

	name = va("sdRenderModelDecal_%d", c_numDecalModel++);
	purged = false;
}

sdRenderModelDecal::~sdRenderModelDecal(void) 
{
	srfTriangles_t *tri = surfaces[0].geometry;

	if (tri->indexCache)
	{
		vertexCache.Free(tri->indexCache);
		tri->indexCache = NULL;
	}
	if (tri->ambientCache)
	{
		vertexCache.Free(tri->ambientCache);
		tri->ambientCache = NULL;
	}
}

void sdRenderModelDecal::Reset(void)
{
	srfTriangles_t *tri = surfaces[0].geometry;

	tri->numVerts = 0;
	tri->numIndexes = 0;

	if (tri->indexCache)
	{
		vertexCache.Free(tri->indexCache);
		tri->indexCache = NULL;
	}
	if (tri->ambientCache)
	{
		vertexCache.Free(tri->ambientCache);
		tri->ambientCache = NULL;
	}
	bounds.Clear();
}

void sdRenderModelDecal::AddWinding(const idFixedWinding &w, const idVec4 &color)
{
	srfTriangles_t *tri = surfaces[0].geometry;
	int i;

	if (tri->numVerts + w.GetNumPoints() < MAX_DECAL_VERTS &&
	    tri->numIndexes + (w.GetNumPoints() - 2) * 3 < MAX_DECAL_INDEXES) {

		for (i = 0; i < w.GetNumPoints(); i++) {
			tri->verts[tri->numVerts + i].xyz = w[i].ToVec3();
			tri->verts[tri->numVerts + i].st[0] = w[i].s;
			tri->verts[tri->numVerts + i].st[1] = w[i].t;

			for (int k = 0 ; k < 4 ; k++) {
				int icolor = idMath::FtoiFast(color[k] * 255.0f);

				if (icolor < 0) {
					icolor = 0;
				} else if (icolor > 255) {
					icolor = 255;
				}

				tri->verts[tri->numVerts + i].color[k] = icolor;
			}
		}

		for (i = 2; i < w.GetNumPoints(); i++) {
			tri->indexes[tri->numIndexes + 0] = tri->numVerts;
			tri->indexes[tri->numIndexes + 1] = tri->numVerts + i - 1;
			tri->indexes[tri->numIndexes + 2] = tri->numVerts + i;
			tri->numIndexes += 3;
		}

		tri->numVerts += w.GetNumPoints();

		if (tri->indexCache)
		{
			vertexCache.Free(tri->indexCache);
			tri->indexCache = NULL;
		}
	}
}


void sdRenderModelDecal::CreateDecal(const idRenderModel *model, const decalProjectionInfo_t &localInfo, const idVec4 &color, const idMaterial** onlyMaterials, const int numOnlyMaterials )
{
	// check all model surfaces
	for (int surfNum = 0; surfNum < model->NumSurfaces(); surfNum++) {
		// if(parms && parms->hideSurfaceMask.Get(surfNum))
		// 	continue;
		const modelSurface_t *surf = model->Surface(surfNum);

		// if no geometry or no shader
		if (!surf->geometry || !surf->material)
		{
			continue;
		}

		if (onlyMaterials && numOnlyMaterials > 0)
		{
			bool match = false;
			for (int i = 0; i < numOnlyMaterials; i++)
			{
				if (onlyMaterials[i] == surf->material)
				{
					match = true;
					break;
				}
			}
			if (!match)
				continue;
		}

		// decals and overlays use the same rules
		if (!localInfo.force && !surf->material->AllowOverlays())
		{
			continue;
		}

		srfTriangles_t *stri = surf->geometry;

		// if the triangle bounds do not overlap with projection bounds
		if (!localInfo.projectionBounds.IntersectsBounds(stri->bounds)) {
			continue;
		}

		// allocate memory for the cull bits
		byte *cullBits = (byte *)_alloca16(stri->numVerts * sizeof(cullBits[0]));

		// catagorize all points by the planes
		SIMDProcessor->DecalPointCull(cullBits, localInfo.boundingPlanes, stri->verts, stri->numVerts);

		// find triangles inside the projection volume
		for (int triNum = 0, index = 0; index < stri->numIndexes; index += 3, triNum++) {
			int v1 = stri->indexes[index+0];
			int v2 = stri->indexes[index+1];
			int v3 = stri->indexes[index+2];

			// skip triangles completely off one side
			if (cullBits[v1] & cullBits[v2] & cullBits[v3]) {
				continue;
			}

			// skip back facing triangles
			if (stri->facePlanes && stri->facePlanesCalculated &&
			    stri->facePlanes[triNum].Normal() * localInfo.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 2].Normal() < -0.1f) {
				continue;
			}

			// create a winding with texture coordinates for the triangle
			idFixedWinding fw;
			fw.SetNumPoints(3);

			if (localInfo.parallel) {
				for (int j = 0; j < 3; j++) {
					fw[j] = stri->verts[stri->indexes[index+j]].xyz;
					fw[j].s = localInfo.textureAxis[0].Distance(fw[j].ToVec3());
					fw[j].t = localInfo.textureAxis[1].Distance(fw[j].ToVec3());
				}
			} else {
				for (int j = 0; j < 3; j++) {
					idVec3 dir;
					float scale = 0.0f;

					fw[j] = stri->verts[stri->indexes[index+j]].xyz;
					dir = fw[j].ToVec3() - localInfo.projectionOrigin;
					localInfo.boundingPlanes[NUM_DECAL_BOUNDING_PLANES - 1].RayIntersection(fw[j].ToVec3(), dir, scale);
					dir = fw[j].ToVec3() + scale * dir;
					fw[j].s = localInfo.textureAxis[0].Distance(dir);
					fw[j].t = localInfo.textureAxis[1].Distance(dir);
				}
			}

			int orBits = cullBits[v1] | cullBits[v2] | cullBits[v3];

			// clip the exact surface triangle to the projection volume
			for (int j = 0; j < NUM_DECAL_BOUNDING_PLANES; j++) {
				if (orBits & (1 << j)) {
					if (!fw.ClipInPlace(-localInfo.boundingPlanes[j])) {
						break;
					}
				}
			}

			if (fw.GetNumPoints() == 0) {
				continue;
			}

			AddWinding(fw, color);
		}
	}

	srfTriangles_t *tri = surfaces[0].geometry;
	R_BoundTriSurf(tri);
	bounds = tri->bounds;
}

void sdRenderModelDecal::SetShader(const idMaterial *mat)
{
	surfaces[0].material = mat;
}

#if 0
static const char *Decal_SnapshotName = "_Decal_Snapshot_";

dynamicModel_t sdRenderModelDecal::IsDynamicModel() const
{
    return DM_CONTINUOUS;
    //return DM_CACHED;
}

idRenderModel * sdRenderModelDecal::InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel)
{
    int					surfaceNum;
    idRenderModelStatic	*staticModel;
    modelSurface_t *surf;

    if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
        delete cachedModel;
        cachedModel = NULL;
    }

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
        assert(idStr::Icmp(cachedModel->Name(), Decal_SnapshotName) == 0);
        staticModel = static_cast<idRenderModelStatic *>(cachedModel);
    } else {
        staticModel = new idRenderModelStatic;
        staticModel->InitEmpty(Decal_SnapshotName);
    }

    staticModel->bounds.Clear();

	if (staticModel->FindSurfaceWithId(0, surfaceNum)) {
		surf = &staticModel->surfaces[surfaceNum];
	} else {
		surf = &staticModel->surfaces.Alloc();
		surf->geometry = NULL;
		surf->material = NULL;
		surf->id = 0;
	}

	surf->material = surfaces[0].material;
	surf->geometry = R_AllocStaticTriSurf();
	srfTriangles_t *tri = surfaces[0].geometry;
	R_AllocStaticTriSurfVerts(surf->geometry, tri->numVerts);
	R_AllocStaticTriSurfIndexes(surf->geometry, tri->numIndexes);
	memcpy(surf->geometry->verts, tri->verts, sizeof(*surf->geometry->verts) * tri->numVerts);
	memcpy(surf->geometry->indexes, tri->indexes, sizeof(*surf->geometry->indexes) * tri->numIndexes);
	surf->geometry->bounds = tri->bounds;

    staticModel->bounds = bounds;
    return staticModel;
}
#endif
