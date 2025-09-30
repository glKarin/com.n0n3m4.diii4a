#include "BSE_Compat.h"

namespace BSE
{

void AppendToSurface(const idRenderModel *model, srfTriangles_t* tri, const idMat4 &mat, uint32_t packed)
{
	if(!model)
		return;

	if(model->NumSurfaces() == 0)
		return;

	const modelSurface_t *surf = model->Surface(0);
	if(!surf || !surf->geometry)
		return;

	const srfTriangles_t *modelTri = surf->geometry;
	if(modelTri->numVerts == 0 || modelTri->numIndexes == 0)
		return;

	const int base = tri->numVerts;
	idDrawVert* v = &tri->verts[base];

	for(int i = 0; i < modelTri->numVerts; i++)
	{
		//v[i] = modelTri->verts[i];
		v[i].xyz = mat * modelTri->verts[i].xyz;
		v[i].st = modelTri->verts[i].st;
		v[i].normal = modelTri->verts[i].normal;
		v[i].tangents[0] = modelTri->verts[i].tangents[0];
		v[i].tangents[1] = modelTri->verts[i].tangents[1];
		*(uint32_t *)&v[i].color = packed;
	}

	glIndex_t* idx = &tri->indexes[tri->numIndexes];
	for(int i = 0; i < modelTri->numIndexes; i++)
	{
		idx[i] = base + modelTri->indexes[i];
	}

	tri->numVerts += modelTri->numVerts;
	tri->numIndexes += modelTri->numIndexes;
}

int GetTexelCount(const idMaterial *material)
{
    if(!material)
        return 0;
    int ret = 0;
    for(int i = 0; i < material->GetNumStages(); i++)
    {
        const shaderStage_t *stage = material->GetStage(i);
        if ( stage->texture.image )
            ret += 1; // stage->texture.image->GetTexelCount();
    }
    return ret;
}

};

