#include "../../idlib/precompiled.h"

#include "../../renderer/Model_local.h"

rvRenderModelBSE::rvRenderModelBSE()
{
    bounds[0].z = idMath::INFINITY;
    bounds[0].y = idMath::INFINITY;
    bounds[0].x = idMath::INFINITY;
    bounds[1].z = -idMath::INFINITY;
    bounds[1].y = -idMath::INFINITY;
    bounds[1].x = -idMath::INFINITY;
}

void rvRenderModelBSE::InitFromFile(const char *fileName)
{
    name = fileName;
}

void rvRenderModelBSE::FinishSurfaces(bool useMikktspace) {
    int i; // ebp
    int surfId; // ebx
    srfTriangles_t* tri; // eax

    bounds[1].z = 0.0;
    bounds[1].y = 0.0;
    bounds[1].x = 0.0;
    i = 0;
    bounds[0].z = 0.0;
    bounds[0].y = 0.0;
    bounds[0].x = 0.0;
    if (surfaces.Num() > 0)
    {
        surfId = 0;
        do
        {
            tri = surfaces[surfId].geometry;
            if (tri)
            {
                bounds[0].x = fminf(bounds[0].x, tri->bounds[0].x);
                bounds[0].y = fminf(bounds[0].y, tri->bounds[0].y);
                bounds[0].z = fminf(bounds[0].z, tri->bounds[0].z);
                bounds[1].x = fmaxf(bounds[1].x, tri->bounds[1].x);
                bounds[1].y = fmaxf(bounds[1].y, tri->bounds[1].y);
                bounds[1].z = fmaxf(bounds[1].z, tri->bounds[1].z);
            }
            ++i;
            ++surfId;
        } while (i < surfaces.Num());
    }
}
