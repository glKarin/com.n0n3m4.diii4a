
#ifndef MODEL_ZYMOTIC_H
#define MODEL_ZYMOTIC_H

typedef struct zymlump_s
{
	int start;
	int length;
} zymlump_t;

typedef struct zymtype1header_s
{
	char id[12]; // "ZYMOTICMODEL", length 12, no termination
	int type; // 0 (vertex morph) 1 (skeletal pose) or 2 (skeletal scripted)
	int filesize; // size of entire model file
	float mins[3], maxs[3], radius; // for clipping uses
	int numverts;
	int numtris;
	int numshaders;
	int numbones; // this may be zero in the vertex morph format (undecided)
	int numscenes; // 0 in skeletal scripted models

// skeletal pose header
	// lump offsets are relative to the file
	zymlump_t lump_scenes; // zymscene_t scene[numscenes]; // name and other information for each scene (see zymscene struct)
	zymlump_t lump_poses; // float pose[numposes][numbones][6]; // animation data
	zymlump_t lump_bones; // zymbone_t bone[numbones];
	zymlump_t lump_vertbonecounts; // int vertbonecounts[numvertices]; // how many bones influence each vertex (separate mainly to make this compress better)
	zymlump_t lump_verts; // zymvertex_t vert[numvertices]; // see vertex struct
	zymlump_t lump_texcoords; // float texcoords[numvertices][2];
	zymlump_t lump_render; // int renderlist[rendersize]; // sorted by shader with run lengths (int count), shaders are sequentially used, each run can be used with glDrawElements (each triangle is 3 int indices)
	zymlump_t lump_shaders; // char shadername[numshaders][32]; // shaders used on this model
	zymlump_t lump_trizone; // byte trizone[numtris]; // see trizone explanation
}
zymtype1header_t;

#define ZYMBONEFLAG_SHARED 1

typedef struct zymbone_s
{
	char name[32];
	int flags;
	int parent; // parent bone number
}
zymbone_t;

// normally the scene will loop, if this is set it will stay on the final frame
#define ZYMSCENEFLAG_NOLOOP 1

typedef struct zymscene_s
{
	char name[32];
	float mins[3], maxs[3], radius; // for clipping
	float framerate; // the scene will animate at this framerate (in frames per second)
	int flags;
	int start, length; // range of poses
}
zymscene_t;

typedef struct zymvertex_s
{
	int bonenum;
	float origin[3];
}
zymvertex_t;

#endif

