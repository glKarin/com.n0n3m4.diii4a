#ifndef _MODEL_IQM_H
#define _MODEL_IQM_H

// from IQM SDK iqm.h
#define IQM_MAGIC "INTERQUAKEMODEL"
#define IQM_VERSION 2
#define IQM_VERSION1 1
#define IQM_VERSION2 IQM_VERSION

class idMd5MeshFile;
class idMd5AnimFile;

#pragma pack( push, 1 )
struct iqmheader
{
    char magic[16];
    unsigned int version;
    unsigned int filesize;
    unsigned int flags;
    unsigned int num_text, ofs_text;
    unsigned int num_meshes, ofs_meshes;
    unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
    unsigned int num_triangles, ofs_triangles, ofs_adjacency;
    unsigned int num_joints, ofs_joints;
    unsigned int num_poses, ofs_poses;
    unsigned int num_anims, ofs_anims;
    unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
    unsigned int num_comment, ofs_comment;
    unsigned int num_extensions, ofs_extensions;
};

struct iqmmesh
{
    unsigned int name;
    unsigned int material;
    unsigned int first_vertex, num_vertexes;
    unsigned int first_triangle, num_triangles;
};
typedef struct iqmmesh iqmmesh_t;

enum
{
    IQM_POSITION     = 0,
    IQM_TEXCOORD     = 1,
    IQM_NORMAL       = 2,
    IQM_TANGENT      = 3,
    IQM_BLENDINDEXES = 4,
    IQM_BLENDWEIGHTS = 5,
    IQM_COLOR        = 6,
    IQM_CUSTOM       = 0x10
};

#define IQM_TRIANGLE 0x11
#define IQM_MESH 0x12
#define IQM_BOUNDS 0x13
#define IQM_JOINT 0x14
#define IQM_TEXT 0x15
#define IQM_ANIM 0x16
#define IQM_POSE 0x17
#define IQM_FRAMEDATA 0x18

enum
{
    IQM_BYTE   = 0,
    IQM_UBYTE  = 1,
    IQM_SHORT  = 2,
    IQM_USHORT = 3,
    IQM_INT    = 4,
    IQM_UINT   = 5,
    IQM_HALF   = 6,
    IQM_FLOAT  = 7,
    IQM_DOUBLE = 8
};

struct iqmtriangle
{
    unsigned int vertex[3];
};

struct iqmadjacency
{
    unsigned int triangle[3];
};

struct iqmjointv1
{
    unsigned int name;
    int parent;
    float translate[3], rotate[3], scale[3];
};
typedef struct iqmjointv1 iqmjoint1_t;

// translate is translation <Tx, Ty, Tz>, and rotate is quaternion rotation <Qx, Qy, Qz, Qw>
// rotation is in relative/parent local space
// scale is pre-scaling <Sx, Sy, Sz>
// output = (input*scale)*rotation + translation
struct iqmjoint
{
    unsigned int name;
    int parent;
    float translate[3], rotate[4], scale[3];
};
typedef struct iqmjoint iqmjoint_t;

struct iqmposev1
{
    int parent;
    unsigned int mask;
    float channeloffset[9];
    float channelscale[9];
};
typedef struct iqmposev1 iqmpose1_t;

// channels 0..2 are translation <Tx, Ty, Tz> and channels 3..6 are quaternion rotation <Qx, Qy, Qz, Qw>
// rotation is in relative/parent local space
// channels 7..9 are scale <Sx, Sy, Sz>
// output = (input*scale)*rotation + translation
struct iqmpose
{
    int parent;
    unsigned int mask;
    float channeloffset[10];
    float channelscale[10];
};
typedef struct iqmpose iqmpose_t;

struct iqmanim
{
    unsigned int name;
    unsigned int first_frame, num_frames;
    float framerate;
    unsigned int flags;
};
typedef struct iqmanim iqmanim_t;

enum
{
    IQM_LOOP = 1<<0
};

struct iqmvertexarray
{
    unsigned int type;
    unsigned int flags;
    unsigned int format;
    unsigned int size;
    unsigned int offset;
};
typedef struct iqmvertexarray iqmvertexarray_t;

struct iqmbounds
{
    float bbmin[3], bbmax[3]; // the minimum and maximum coordinates of the bounding box for this animation frame
    float xyradius, radius; // the circular radius in the X-Y plane, as well as the spherical radius
};
typedef struct iqmbounds iqmbounds_t;

struct iqmextension
{
    unsigned int name;
    unsigned int num_data, ofs_data;
    unsigned int ofs_extensions; // pointer to next extension
};

// idTech4 mapping
typedef idVec3 iqmVertex_t;
typedef idVec2 iqmTexcoord_t;
typedef idVec3 iqmNormal_t;
typedef idVec3 iqmTangent_t;
typedef struct iqmColor_s { // using byte
    byte color[4];
} iqmColor_t;
typedef struct iqmBlendIndex_s {
    byte index[4];
} iqmBlendIndex_t;
typedef struct iqmBlendWeight_s {
    byte weight[4];
} iqmBlendWeight_t;
// each value is the index of the adjacent triangle for edge 0, 1, and 2, where ~0 (= -1) indicates no adjacent triangle
// indexes are relative to the iqmheader.ofs_triangles array and span all meshes, where 0 is the first triangle, 1 is the second, 2 is the third, etc.
typedef struct iqmTriangle_s {
    int elements[3];
} iqmTriangle_t;
typedef struct iqmmesh iqmMesh_t;
typedef struct iqmbounds iqmBounds_t;
typedef struct iqmjoint iqmJoint_t;
// big array of all strings, each individual string being 0 terminated, with the first string always being the empty string "" (i.e. text[0] == 0)
typedef struct iqmText_s {
    idStr text;
    unsigned int offset;
} iqmText_t;
typedef struct iqmanim iqmAnim_t;
typedef struct iqmpose iqmPose_t;
typedef unsigned short iqmFramedata_t;

#define	IQM_CM1_TX				BIT( 0 )
#define	IQM_CM1_TY				BIT( 1 )
#define	IQM_CM1_TZ				BIT( 2 )
#define	IQM_CM1_QX				BIT( 3 )
#define	IQM_CM1_QY				BIT( 4 )
#define	IQM_CM1_QZ				BIT( 5 )
#define	IQM_CM1_SX				BIT( 6 )
#define	IQM_CM1_SY				BIT( 7 )
#define	IQM_CM1_SZ				BIT( 8 )

#define	IQM_CM_TX				BIT( 0 )
#define	IQM_CM_TY				BIT( 1 )
#define	IQM_CM_TZ				BIT( 2 )
#define	IQM_CM_QX				BIT( 3 )
#define	IQM_CM_QY				BIT( 4 )
#define	IQM_CM_QZ				BIT( 5 )
#define	IQM_CM_QW				BIT( 6 )
#define	IQM_CM_SX				BIT( 7 )
#define	IQM_CM_SY				BIT( 8 )
#define	IQM_CM_SZ				BIT( 9 )
#pragma pack( pop )

class idModelIqm
{
    public:
		enum {
			PARSE_DEF = 0,
			PARSE_MESH,
			PARSE_JOINT,
			PARSE_ANIM,
			PARSE_FRAME,
			PARSE_ALL,
		};
        idModelIqm(void);
        ~idModelIqm(void);
        bool Parse(const char *iqmPath, int type = PARSE_DEF);
        void Print(void) const;
        const char * GetText(unsigned int offset) const;
        const char * GetAnim(unsigned int index) const;
        int GetAnimCount(void) const;
        bool ToMd5Mesh(idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false) const;
        bool ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int animIndex, float scale = -1.0f, bool addOrigin = false) const;
        bool ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, const char *animName, float scale = -1.0f, bool addOrigin = false) const;
        int ToMd5AnimList(idList<idMd5AnimFile> &md5anim, idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false) const;
#ifdef _MODEL_OBJ
        bool ToObj(objModel_t &objModel) const;
#endif

    private:
        typedef struct iqmVertexOffset_s
        {
            unsigned int vnormal; // float
            unsigned int vposition; // float
            unsigned int vtangent; // float
            unsigned int vtexcoord; // float
            unsigned int vcolor4f; // float
            unsigned int vblendindexes; // byte
            unsigned int vblendweights; // byte
            unsigned int vcolor4ub; // byte
            unsigned int framedata; // short
        } iqmVertexOffset_t;

        void Clear(void);
        void MarkType(int type);
        bool IsTypeMarked(int type) const;
        int ReadHeader(iqmheader &header);
        bool ReadOffsets(iqmVertexOffset_t &offsets);
        int ReadVertexes(unsigned int vposition);
        int ReadTexcoords(unsigned int vtexcoord);
        int ReadNormals(unsigned int vnormal);
        int ReadColorsf(unsigned int vcolor4f);
        int ReadColorsub(unsigned int vcolor4ub);
        int ReadTangents(unsigned int vtangent);
        int ReadBlendIndexes(unsigned int vblendindexes);
        int ReadBlendWeights(unsigned int vblendweights);
        int ReadTriangles(void);
        int ReadMeshes(void);
        int ReadBounds(void);
        int ReadJoints1(void);
        int ReadJoints2(void);
        int ReadJoints(void);
        int ReadTexts(void);
        int ReadAnims(void);
        int ReadPoses1(void);
        int ReadPoses2(void);
        int ReadPoses(void);
        int ReadFramedatas(void);

    private:
        iqmheader header;
        idList<iqmVertex_t> vertexes;
        idList<iqmTexcoord_t> texcoords;
        idList<iqmNormal_t> normals;
        idList<iqmColor_t> colors;
        idList<iqmTangent_t> tangents;
        idList<iqmBlendIndex_t> blendIndexes;
        idList<iqmBlendWeight_t> blendWeights;
        idList<iqmTriangle_t> triangles;
        idList<iqmMesh_t> meshes;
        idList<iqmBounds_t> bounds;
        idList<iqmJoint_t> joints;
        idList<iqmText_t> texts;
        idList<iqmAnim_t> anims;
        idList<iqmPose_t> poses;
        idList<iqmFramedata_t> framedatas;
        idFile *file;
        int types;

        friend class idRenderModelStatic;
};

#endif
