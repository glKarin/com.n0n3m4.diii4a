#ifndef CURVES_H
#define CURVES_H

#define PATCH_LODS_NUM 2
#define PATCH_LOD_COLLISION 0
#define PATCH_LOD_VISUAL 1

typedef struct patchinfo_s
{
	int xsize, ysize;
	struct {
		int xtess, ytess;
	} lods[PATCH_LODS_NUM];
} patchinfo_t;

// Calculate number of resulting vertex rows/columns by given patch size and tesselation factor
// When tess=0 it means that we reduce detalization of base 3x3 patches by removing middle row and column
// "DimForTess" is "DIMension FOR TESSelation factor"
int Q3PatchDimForTess(int size, int tess);

// usage:
// to expand a 5x5 patch to 21x21 vertices (4x4 tesselation), one might use this call:
// Q3PatchSubdivideFloat(3, sizeof(float[3]), outvertices, 5, 5, sizeof(float[3]), patchvertices, 4, 4);
void Q3PatchTesselateFloat(int numcomponents, int outputstride, float *outputvertices, int patchwidth, int patchheight, int inputstride, float *patchvertices, int tesselationwidth, int tesselationheight);
// returns how much tesselation of each segment is needed to remain under tolerance
int Q3PatchTesselationOnX(int patchwidth, int patchheight, int components, const float *in, float tolerance);
// returns how much tesselation of each segment is needed to remain under tolerance
int Q3PatchTesselationOnY(int patchwidth, int patchheight, int components, const float *in, float tolerance);
// calculates elements for a grid of vertices
// (such as those produced by Q3PatchTesselate)
// (note: width and height are the actual vertex size, this produces
//  (width-1)*(height-1)*2 triangles, 3 elements each)
void Q3PatchTriangleElements(int *elements, int width, int height, int firstvertex);

int Q3PatchAdjustTesselation(int numcomponents, patchinfo_t *patch1, float *patchvertices1, patchinfo_t *patch2, float *patchvertices2);

#endif

