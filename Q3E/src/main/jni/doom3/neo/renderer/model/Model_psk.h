#ifndef _MODEL_PSK_H
#define _MODEL_PSK_H

#define PSK_CHUNK_ID_TOTAL_LENGTH 20
#define PSK_CHUNK_ID_LENGTH 8
#define PSK_CHUNK_HEADER_TYPE 20100422

namespace md5model
{
	class idMd5MeshFile;
};

#pragma pack( push, 1 )
/*
   VChunkHeader Struct
   ChunkID|TypeFlag|DataSize|DataCount
   0      |1       |2       |3
   */
// 20s3i
typedef struct pskHeader_s {
    char chunk_id[PSK_CHUNK_ID_TOTAL_LENGTH];
    int chunk_type;
    int chunk_datasize;
    int chunk_datacount; // chunk_bytes = chunk_datasize * chunk_datacount
} pskHeader_t;

// Vertices X | Y | Z
// 3f
typedef idVec3 pskVertex_t;

// Vertex Normals NX | NY | NZ
// 3f
typedef idVec3 pskNormal_t;

// Extra UV. U | V
// =2f
typedef idVec2 pskUv_t;

// Vertex colors. R G B A bytes. NOTE: it is Wedge color.(uses Wedges index)
// =4B
typedef struct pskColor_s {
    byte r, g, b, a;
} pskColor_t;

// Wedges (UV)   VertexId |  U |  V | MatIdx
// =IffBxxx
typedef struct pskWedge_s {
    unsigned int vertex_index;
    float u, v;
    byte material_index;
    char placeholder[3];
} pskWedge_t;

// Faces WdgIdx1 | WdgIdx2 | WdgIdx3 | MatIdx | AuxMatIdx | SmthGrp
// =IIIBBI | =HHHBBI
typedef struct pskFace_s {
    unsigned int wedge_index1, wedge_index2, wedge_index3;
    byte material_index;
    byte aux_material_index;
    unsigned int smooth_group;
} pskFace_t;

// Materials   MaterialNameRaw | TextureIndex | PolyFlags | AuxMaterial | AuxFlags |  LodBias | LodStyle
// 64s24x
typedef struct pskMaterial_s {
    char name[64];
    char placeholder[24];
} pskMaterial_t;

// Influences (Bone Weight) (VRawBoneInfluence) ( Weight | PntIdx | BoneIdx)
// fii
typedef struct pskWeight_s {
    float weight;
    int vertex_index;
    int bone_index;
} pskWeight_t;

// Bones (VBone .. VJointPos ) Name|Flgs|NumChld|PrntIdx|Qw|Qx|Qy|Qz|LocX|LocY|LocZ|Lngth|XSize|YSize|ZSize
// 64s3i11f
typedef struct pskBone_s {
    char name[64];
    int flags;
    int num_children;
    int parent_index;
    float qw, qx, qy, qz;
    float localx, localy, localz;
    float length;
    float xsize, ysize, zsize;
} pskBone_t;

// MorphInfo
// 64si
typedef struct pskMorphInfo_s {
    char name[64];
    int vertex_count;
} pskMorphInfo_t;

// MorphData
// 6fi
typedef struct pskMorphData_s {
	float position_deltax;
	float position_deltay;
	float position_deltaz;
	float tangent_z_deltax;
	float tangent_z_deltay;
	float tangent_z_deltaz;
	int point_index;
} pskMorphData_t;
#pragma pack( pop )

typedef enum pskHeaderType_e
{
    ACTRHEAD = 0,
    PNTS0000, // vertexes
    VTXW0000, // wedges/UV
    VTXW3200, // wedges/UV
    FACE0000, // faces
    FACE3200, // faces
    MATT0000, // materials
    REFSKELT, // bones
    REFSKEL0, // bones
    RAWW0000, // weights
    RAWWEIGH, // weights
    VERTEXCO, // vertex colors
    EXTRAUVS, // extra uvs
    VTXNORMS, // normals
	MRPHINFO, // shape
	MRPHDATA, // shape
    PSKHT_TOTAL,
} pskHeaderType_t;

class idModelPsa;
class idModelPsk
{
    public:
        idModelPsk(void);
        ~idModelPsk(void);
        bool Parse(const char *pskPath);
        bool Check(void) const;
        void Print(void) const;
        bool ToMd5Mesh(md5model::idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false) const;
#ifdef _MODEL_OBJ
        bool ToObj(objModel_t &objModel, bool keepDup = false) const;
#endif

    private:
        void Clear(void);
        void MarkType(int type);
        bool IsTypeMarked(int type) const;
        int ReadHeader(pskHeader_t &header);
        int ReadVertexes(void);
        int ReadWedges(void);
        int ReadFaces(void);
        int ReadMaterials(void);
        int ReadWeights(void);
        int ReadBones(void);
        int ReadNormals(void);
        int ReadColors(void);
        int ReadExtraUvs(void);
        int ReadMorphInfos(void);
        int ReadMorphDatas(void);
        int Skip(void);
        int GroupFace(idList<idList<const pskFace_t *> > &faceGroup, idStrList &names) const;

    private:
        idList<pskVertex_t> vertexes;
        idList<pskWedge_t> wedges;
        idList<pskFace_t> faces;
        idList<pskMaterial_t> materials;
        idList<pskWeight_t> weights;
        idList<pskBone_t> bones;
        idList<pskNormal_t> normals;
        idList<pskUv_t> uvs;
        idList<pskColor_t> colors;
        idList<pskMorphInfo_t> morphInfos;
        idList<pskMorphData_t> morphDatas;
        idFile *file;
        int types;
        pskHeader_t header;

    friend class idModelPsa;
	friend class idRenderModelStatic;
};

#endif
