#ifndef _MODEL_MD5MESH_H
#define _MODEL_MD5MESH_H

namespace md5model
{
    typedef struct md5meshJoint_s
    {
        idStr boneName; // The name of this bone.
        int parentIndex; // The index of this bone’s parent.
        idVec3 pos; // The X/Y/Z component of this bone’s XYZ position. world coordinate
        idQuat orient; // The X/Y/Z component of this bone’s XYZ orentation quaternion. world coordinate
    } md5meshJoint_t;

    typedef struct md5meshVert_s
    {
        idVec2 uv; // The U/V component of the UV texture coordinates.
        int weightIndex; // The index into the weight array where this vertex’s first weight is located.
        int weightElem; // The number of elements in the weight array that apply to this vertex.
    } md5meshVert_t;

    typedef struct md5meshTri_s
    {
        int vertIndex1; // The index of the first vertex for this triangle.
        int vertIndex2; // The index of the second vertex for this triangle.
        int vertIndex3; // The index of the third vertex for this triangle.
    } md5meshTri_t;

    typedef struct md5meshWeight_s
    {
        int jointIndex; // The index of the joint to which this weight applies.
        float weightValue; // The value of the weight.
        idVec3 pos; // The X/Y/Z component of this weight’s XYZ position. offset
    } md5meshWeight_t;

    typedef struct md5meshMesh_s
    {
        idStr name;
        idStr shader;
        idList<md5meshVert_t> verts;
        idList<md5meshTri_t> tris;
        idList<md5meshWeight_t> weights;
    } md5meshMesh_t;

    typedef struct md5meshJointTransform_s
    {
        int index;
        int parentIndex;
        struct md5meshJointTransform_s *parent;

        idVec3 idt; // world
        idMat3 idwm; // world
        idVec3 t; // local
        idMat3 q; // local
        idVec3 bindpos; // world
        idMat3 bindmat; // world

        idVec3 pos; // world
        idQuat orient; // world
    } md5meshJointTransform_t;

    class idMd5AnimFile;
    class idMd5MeshFile
    {
    public:
        idMd5MeshFile(void);
        void Write(const char *path) const;
        bool Parse(const char *path);
        idList<md5meshJoint_t> & Joints(void);
        idList<md5meshMesh_t> & Meshes(void);
        idStr & Commandline(void);
        void Clear(void);
        void ConvertJointTransforms(idList<md5meshJointTransform_t> &list) const;
        void CalcBounds(const idList<md5meshJointTransform_t> &list, idBounds &bounds) const;

        static void ConvertJointTransforms(const idList<md5meshJoint_t> &joints, idList<md5meshJointTransform_t> &list);

    private:
        void CalcMeshBounds(const md5meshMesh_t &mesh, const idList<md5meshJointTransform_t> &list, idBounds &bounds) const;

    private:
        int version;
        idStr commandline;
        idList<md5meshJoint_t> joints;
        idList<md5meshMesh_t> meshes;

        friend class idMd5AnimFile;
    };

};

#endif
