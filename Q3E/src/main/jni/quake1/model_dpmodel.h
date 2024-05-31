
#ifndef MODEL_DPMODEL_H
#define MODEL_DPMODEL_H

/*
type 2 model (hierarchical skeletal pose)
within this specification, int is assumed to be 32bit, float is assumed to be 32bit, char is assumed to be 8bit, text is assumed to be an array of chars with NULL termination
all values are big endian (also known as network byte ordering), NOT x86 little endian
general notes:
a pose is a 3x4 matrix (rotation matrix, and translate vector)
parent bones must always be lower in number than their children, models will be rejected if this is not obeyed (can be fixed by modelling utilities)
utility notes:
if a hard edge is desired (faceted lighting, or a jump to another set of skin coordinates), vertices must be duplicated
ability to visually edit groupids of triangles is highly recommended
bones should be markable as 'attach' somehow (up to the utility) and thus protected from culling of unused resources
frame 0 is always the base pose (the one the skeleton was built for)
game notes:
the loader should be very thorough about error checking, all vertex and bone indices should be validated, etc
the gamecode can look up bone numbers by name using a builtin function, for use in attachment situations (the client should have the same model as the host of the gamecode in question - that is to say if the server gamecode is setting the bone number, the client and server must have vaguely compatible models so the client understands, and if the client gamecode is setting the bone number, the server could have a completely different model with no harm done)
the triangle groupid values are up to the gamecode, it is recommended that gamecode process this in an object-oriented fashion (I.E. bullet hits entity, call that entity's function for getting properties of that groupid)
frame 0 should be usable, not skipped
speed optimizations for the saver to do:
remove all unused data (unused bones, vertices, etc, be sure to check if bones are used for attachments however)
sort triangles into strips
sort vertices according to first use in a triangle (caching benefits) after sorting triangles
speed optimizations for the loader to do:
if the model only has one frame, process it at load time to create a simple static vertex mesh to render (this is a hassle, but it is rewarding to optimize all such models)
rendering process:
1*. one or two poses are looked up by number
2*. boneposes (matrices) are interpolated, building bone matrix array
3. bones are parsed sequentially, each bone's matrix is transformed by it's parent bone (which can be -1; the model to world matrix)
4. meshs are parsed sequentially, as follows:
  1. vertices are parsed sequentially and may be influenced by more than one bone (the results of the 3x4 matrix transform will be added together - weighting is already built into these)
  2. shader is looked up and called, passing vertex buffer (temporary) and triangle indices (which are stored in the mesh)
5. rendering is complete
* - these stages can be replaced with completely dynamic animation instead of pose animations.
*/
// header for the entire file
typedef struct dpmheader_s
{
	char id[16]; // "DARKPLACESMODEL\0", length 16
	unsigned int type; // 2 (hierarchical skeletal pose)
	unsigned int filesize; // size of entire model file
	float mins[3], maxs[3], yawradius, allradius; // for clipping uses
	// these offsets are relative to the file
	unsigned int num_bones;
	unsigned int num_meshs;
	unsigned int num_frames;
	unsigned int ofs_bones; // dpmbone_t bone[num_bones];
	unsigned int ofs_meshs; // dpmmesh_t mesh[num_meshs];
	unsigned int ofs_frames; // dpmframe_t frame[num_frames];
}
dpmheader_t;
// there may be more than one of these
typedef struct dpmmesh_s
{
	// these offsets are relative to the file
	char shadername[32]; // name of the shader to use
	unsigned int num_verts;
	unsigned int num_tris;
	unsigned int ofs_verts; // dpmvertex_t vert[numvertices]; // see vertex struct
	unsigned int ofs_texcoords; // float texcoords[numvertices][2];
	unsigned int ofs_indices; // unsigned int indices[numtris*3]; // designed for glDrawElements (each triangle is 3 unsigned int indices)
	unsigned int ofs_groupids; // unsigned int groupids[numtris]; // the meaning of these values is entirely up to the gamecode and modeler
}
dpmmesh_t;
// if set on a bone, it must be protected from removal
#define DPMBONEFLAG_ATTACHMENT 1
// one per bone
typedef struct dpmbone_s
{
	// name examples: upperleftarm leftfinger1 leftfinger2 hand, etc
	char name[32];
	// parent bone number
	signed int parent;
	// flags for the bone
	unsigned int flags;
}
dpmbone_t;
// a bonepose matrix is intended to be used like this:
// (n = output vertex, v = input vertex, m = matrix, f = influence)
// n[0] = v[0] * m[0][0] + v[1] * m[0][1] + v[2] * m[0][2] + f * m[0][3];
// n[1] = v[0] * m[1][0] + v[1] * m[1][1] + v[2] * m[1][2] + f * m[1][3];
// n[2] = v[0] * m[2][0] + v[1] * m[2][1] + v[2] * m[2][2] + f * m[2][3];
typedef struct dpmbonepose_s
{
	float matrix[3][4];
}
dpmbonepose_t;
// immediately followed by bone positions for the frame
typedef struct dpmframe_s
{
	// name examples: idle_1 idle_2 idle_3 shoot_1 shoot_2 shoot_3, etc
	char name[32];
	float mins[3], maxs[3], yawradius, allradius;
	int ofs_bonepositions; // dpmbonepose_t bonepositions[bones];
}
dpmframe_t;
// one or more of these per vertex
typedef struct dpmbonevert_s
{
	// this pairing of origin and influence is intentional
	// (in SSE or 3DNow! assembly it can be done as a quad vector op
	//  (or two dual vector ops) very easily)
	float origin[3]; // vertex location (these blend)
	float influence; // influence fraction (these must add up to 1)
	// this pairing of normal and bonenum is intentional
	// (in SSE or 3DNow! assembly it can be done as a quad vector op
	//  (or two dual vector ops) very easily, the bonenum is ignored)
	float normal[3]; // surface normal (these blend)
	unsigned int bonenum; // number of the bone
}
dpmbonevert_t;
// variable size, parsed sequentially
typedef struct dpmvertex_s
{
	unsigned int numbones;
	// immediately followed by 1 or more dpmbonevert_t structures
}
dpmvertex_t;

#endif

