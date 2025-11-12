#ifndef _MODEL_MD5MESH_V6_H
#define _MODEL_MD5MESH_V6_H

#define MD5_V6_VERSION				6

typedef struct md5meshV6Bone_s
{
    int index;
    idStr name; // The name of this bone.
    idVec3 bindpos; // The X/Y/Z component of this bone’s XYZ position. world coordinate
    idMat3 bindmat; // The X/Y/Z component of this bone’s XYZ orentation quaternion. world coordinate
    idStr parent; // The index of this bone’s parent.
} md5meshV6Bone_t;

typedef struct md5meshVert_s md5meshV6Vert_t;

typedef struct md5meshTri_s md5meshV6Tri_t;

typedef struct md5meshWeight_s md5meshV6Weight_t;

typedef struct md5meshV6Mesh_s
{
    int index;
    idStr shader;
    idList<md5meshV6Vert_t> verts;
    idList<md5meshV6Tri_t> tris;
    idList<md5meshV6Weight_t> weights;
} md5meshV6Mesh_t;

class idModelMd5meshV6
{
public:
    idModelMd5meshV6(void);
    bool Parse(const char *path);
    void Print(void) const;
    bool ToMd5Mesh(idMd5MeshFile &md5mesh, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL) const;

private:
    void Clear(void);

private:
    int version;
    idStr commandline;
    idList<md5meshV6Bone_t> bones;
    idList<md5meshV6Mesh_t> meshes;

    friend class idMd5AnimFile;
};

#endif
