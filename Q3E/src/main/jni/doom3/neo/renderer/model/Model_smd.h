#ifndef _MODEL_SMD_H
#define _MODEL_SMD_H

class idMd5MeshFile;
class idMd5AnimFile;

#define SMD_VERSION_1 1

#pragma pack( push, 1 )
// node   0 "Bip01" -1
typedef struct smdNode_s {
    int index;
    idStr name;
    int parent_index;
} smdNode_t;

// skeleton    0 0.000000 -1.141865 39.264595 0.000000 0.000000 -1.570795
typedef struct smdSkeleton_s {
    int bone;
    float pos[3];
    float rot[3];
} smdSkeleton_t;

typedef struct smdLink_s
{
	int bone;
	float weight;
} smdLink_t;

// triangle       5 -0.769983 -1.575950 59.517052 -0.407964 -0.899339 -0.157339 0.646484 0.027344 1 5 1.000000
typedef struct smdVertex_s {
    int parent;
    float pos[3];
    float norm[3];
    float tc[2]; // t = 1.0 - src
    int numlinks;
	idList<smdLink_t> links;
} smdVertex_t;

typedef struct smdTriangle_s {
    idStr material;
    smdVertex_t vertexes[3];
} smdTriangle_t;

typedef struct smdFrame_s {
	int time;
    idList<smdSkeleton_t> skeletons;
} smdFrame_t;
#pragma pack( pop )

typedef enum smdDataType_e {
	SMD_NODE,
	SMD_SKELETON,
	SMD_TRIANGLE,
} smdDataType_t;

class idModelSmd
{
    public:
        idModelSmd(void);
        bool Parse(const char *smdPath);
        void Print(void) const;
        bool ToMd5Mesh(idMd5MeshFile &md5mesh, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
		bool ToMd5Anim(const idModelSmd &smd, idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int flags = 0, float scale = -1.0f, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;
        bool IsMeshFile(void) const;
        bool HasSkeleton(void) const;
#ifdef _MODEL_OBJ
        bool ToObj(objModel_t &objModel) const;
#endif

    private:
        void Clear(void);
        void MarkType(int type);
        bool IsTypeMarked(int type) const;
        int ReadNodes(void);
        int ReadTriangles(void);
        void ReadSkeletons(idList<smdSkeleton_t> &skel);
        void ReadVertex(smdVertex_t &vert);
        int ReadFrames(void);
        void Skip(void);
        int GroupTriangle(idList<idList<const smdTriangle_t *> > &faceGroup, idStrList &names) const;
		const idList<smdSkeleton_t> & Bones(void) const;

    private:
        int version;
        idList<smdNode_t> nodes;
        idList<smdTriangle_t> triangles;
        idList<smdFrame_t> frames;
        idLexer *lexer;
        int types;

	friend class idRenderModelStatic;
};

#endif
