
#include "Model_md5mesh.h"

ID_INLINE void Writef(idFile *file, const char *fmt, ...)
{
    va_list argptr;

            va_start(argptr, fmt);
    file->VPrintf(fmt, argptr);
            va_end(argptr);

}

ID_INLINE void Writeln(idFile *file)
{
    file->Write("\n", 1);
}

ID_INLINE void WriteIndent(idFile *file, int num = 1)
{
    for(int i = 0; i < num; i++)
        file->Write("\t", 1);
}

ID_INLINE void Writefln(idFile *file, const char *fmt, ...)
{
    va_list argptr;

            va_start(argptr, fmt);
    file->VPrintf(fmt, argptr);
            va_end(argptr);

    Writeln(file);
}

/* md5mesh */
idMd5MeshFile::idMd5MeshFile(void)
        : version(MD5_VERSION)
{}

ID_INLINE idList<md5meshJoint_t> & idMd5MeshFile::Joints()
{
    return joints;
}

ID_INLINE idList<md5meshMesh_t> & idMd5MeshFile::Meshes()
{
    return meshes;
}

ID_INLINE idStr & idMd5MeshFile::Commandline()
{
    return commandline;
}

void idMd5MeshFile::Clear(void)
{
    version = MD5_VERSION;
    commandline.Clear();
    joints.SetNum(0);
    meshes.SetNum(0);
}

void idMd5MeshFile::Write(const char *path) const
{
    idFile *file = fileSystem->OpenFileWrite(path);

    // header
    Writefln(file, "MD5Version %d", version);
    Writefln(file, "commandline \"%s\"", commandline.c_str());
    Writeln(file);
    Writefln(file, "numJoints %d", joints.Num());
    Writefln(file, "numMeshes %d", meshes.Num());
    Writeln(file);

    // joint
    Writefln(file, "joints {", joints.Num());
    for(int i = 0; i < joints.Num(); i++)
    {
        WriteIndent(file);
        const md5meshJoint_t &joint = joints[i];
        idCQuat rotation = joint.orient.ToCQuat();
#if ETW_PSK
        Writef(file, "\"%s\" %d ( %.10f %.10f %.10f ) ( %.10f %.10f %.10f )", joint.boneName.c_str(), joint.parentIndex, joint.pos[0], joint.pos[1], joint.pos[2], -rotation[0], -rotation[1], -rotation[2]); // TODO why using negative
#else
        Writef(file, "\"%s\" %d ( %.10f %.10f %.10f ) ( %.10f %.10f %.10f )", joint.boneName.c_str(), joint.parentIndex, joint.pos[0], joint.pos[1], joint.pos[2], rotation[0], rotation[1], rotation[2]);
#endif
#if MD5_APPEND_COMMENT
        if(joint.parentIndex != -1)
        {
            const md5meshJoint_t &parent = joints[joint.parentIndex];
            Writef(file, " // %d %s", i, parent.boneName.c_str());
        }
#endif
        Writeln(file);
    }
    Writefln(file, "}");
    Writeln(file);

    // mesh
    for(int i = 0; i < meshes.Num(); i++)
    {
        const md5meshMesh_t &mesh = meshes[i];
        Writefln(file, "mesh {", joints.Num());

        // shader
        WriteIndent(file);
        Writefln(file, "shader \"%s\"", mesh.shader.c_str());
        Writeln(file);

        // vert
        WriteIndent(file);
        Writefln(file, "numverts %d", mesh.verts.Num());
        for(int m = 0; m < mesh.verts.Num(); m++)
        {
            const md5meshVert_t &vert = mesh.verts[m];
            WriteIndent(file);
            Writefln(file, "vert %d ( %.10f %.10f ) %d %d", m, vert.uv[0], vert.uv[1], vert.weightIndex, vert.weightElem);
        }
        Writeln(file);

        // tri
        WriteIndent(file);
        Writefln(file, "numtris %d", mesh.tris.Num());
        for(int m = 0; m < mesh.tris.Num(); m++)
        {
            const md5meshTri_t &tri = mesh.tris[m];
            WriteIndent(file);
            Writefln(file, "tri %d %d %d %d", m, tri.vertIndex1, tri.vertIndex2, tri.vertIndex3);
        }
        Writeln(file);

        // weight
        WriteIndent(file);
        Writefln(file, "numweights %d", mesh.weights.Num());
        for(int m = 0; m < mesh.weights.Num(); m++)
        {
            const md5meshWeight_t &weight = mesh.weights[m];
            WriteIndent(file);
            Writefln(file, "weight %d %d %.10f ( %.10f %.10f %.10f )", m, weight.jointIndex, weight.weightValue, weight.pos[0], weight.pos[1], weight.pos[2]);
        }
        Writeln(file);

        Writefln(file, "}");
        Writeln(file);
    }

    fileSystem->CloseFile(file);
}

bool idMd5MeshFile::Parse(const char *path)
{
    idToken		token;
    idLexer		parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS);
    int			num;
    int			count;
    md5meshJoint_t *joint;
    md5meshMesh_t *mesh;
    int			i, j;
    md5meshVert_t *vert;
    md5meshTri_t *tri;
    md5meshWeight_t *weight;

    if (!parser.LoadFile(path)) {
        return false;
    }

    parser.ExpectTokenString(MD5_VERSION_STRING);
    version = parser.ParseInt();

    // skip commandline
    parser.ExpectTokenString("commandline");
    parser.ReadToken(&token);
    commandline = token;

    // parse num joints
    parser.ExpectTokenString("numJoints");
    num  = parser.ParseInt();
    joints.SetGranularity(1);
    joints.SetNum(num);

    // parse num meshes
    parser.ExpectTokenString("numMeshes");
    num = parser.ParseInt();
    meshes.SetGranularity(1);
    meshes.SetNum(num);

    // parse joints
    parser.ExpectTokenString("joints");
    parser.ExpectTokenString("{");
    joint = joints.Ptr();

    for (i = 0; i < joints.Num(); i++, joint++) {
        // parse name
        parser.ReadToken(&token);
        joint->boneName = token;

        // parse parent
        joint->parentIndex = parser.ParseInt();

        // parse default pose
        parser.Parse1DMatrix(3, joint->pos.ToFloatPtr());
        parser.Parse1DMatrix(3, joint->orient.ToFloatPtr());
        joint->orient.w = joint->orient.CalcW();
    }

    parser.ExpectTokenString("}");

    for (j = 0, mesh = &meshes[0]; j < meshes.Num(); j++, mesh++) {
        parser.ExpectTokenString("mesh");
        parser.ExpectTokenString("{");

        // parse name
        if (parser.CheckTokenString("name")) {
            parser.ReadToken(&token);
            mesh->name = token;
        }

        // parse shader
        parser.ExpectTokenString("shader");

        parser.ReadToken(&token);
        mesh->shader = token;

        // parse texture coordinates
        parser.ExpectTokenString("numverts");
        count = parser.ParseInt();

        mesh->verts.SetNum(count);

        for (i = 0, vert = &mesh->verts[0]; i < mesh->verts.Num(); i++, vert++) {
            parser.ExpectTokenString("vert");
            parser.ParseInt();

            parser.Parse1DMatrix(2, vert->uv.ToFloatPtr());

            vert->weightIndex	= parser.ParseInt();
            vert->weightElem	= parser.ParseInt();
        }

        // parse tris
        parser.ExpectTokenString("numtris");
        count = parser.ParseInt();

        mesh->tris.SetNum(count);

        for (i = 0, tri = &mesh->tris[0]; i < count; i++, tri++) {
            parser.ExpectTokenString("tri");
            parser.ParseInt();

            tri->vertIndex1 = parser.ParseInt();
            tri->vertIndex2 = parser.ParseInt();
            tri->vertIndex3 = parser.ParseInt();
        }

        // parse weights
        parser.ExpectTokenString("numweights");
        count = parser.ParseInt();

        mesh->weights.SetNum(count);

        for (i = 0, weight = &mesh->weights[0]; i < count; i++, weight++) {
            parser.ExpectTokenString("weight");
            parser.ParseInt();

            weight->jointIndex = parser.ParseInt();

            weight->weightValue	= parser.ParseFloat();

            parser.Parse1DMatrix(3, weight->pos.ToFloatPtr());
        }

        parser.ExpectTokenString("}");
    }

    return true;
}

void idMd5MeshFile::ConvertJointTransforms(idList<md5meshJointTransform_t> &jointTransforms) const
{
    ConvertJointTransforms(joints, jointTransforms);
}

void idMd5MeshFile::ConvertJointTransforms(const idList<md5meshJoint_t> &joints, idList<md5meshJointTransform_t> &jointTransforms)
{
    jointTransforms.SetNum(joints.Num());
    md5meshJointTransform_t *jointTransform;
    int i;
    const md5meshJoint_t *md5Bone;

    for (i = 0, md5Bone = &joints[0], jointTransform = &jointTransforms[0]; i < joints.Num(); i++, md5Bone++, jointTransform++)
    {
        jointTransform->index = i;
        jointTransform->parentIndex = md5Bone->parentIndex;
        jointTransform->parent = NULL;
        jointTransform->orient = md5Bone->orient;
        jointTransform->pos = md5Bone->pos;

        idVec3 jointpos = md5Bone->pos;
#if ETW_PSK
        idMat3 jointaxis = md5Bone->orient.Inverse().ToMat3();
#else
        idMat3 jointaxis = md5Bone->orient.ToMat3();
#endif
        if(md5Bone->parentIndex >= 0)
        {
            md5meshJointTransform_t *parentJointTransform = &jointTransforms[md5Bone->parentIndex];
            jointTransform->parent = parentJointTransform;

            // convert to local coordinates
            idMat3 inv = parentJointTransform->idwm.Transpose();
            jointpos = (jointpos - parentJointTransform->idt) * inv;
            jointaxis = jointaxis * inv;
        }

        jointTransform->t = jointpos;
        jointTransform->q = jointaxis;

        if(md5Bone->parentIndex >= 0)
        {
            md5meshJointTransform_t *parentJointTransform = &jointTransforms[md5Bone->parentIndex];

            jointTransform->idwm = jointaxis * parentJointTransform->idwm;
            jointTransform->idt = parentJointTransform->idt + jointpos * parentJointTransform->idwm;
        }
        else
        {
            jointTransform->idwm = jointaxis;
            jointTransform->idt = jointpos;
        }

        jointTransform->bindmat = jointTransform->idwm;
        jointTransform->bindpos = jointTransform->idt;
    }
}

void idMd5MeshFile::CalcBounds(const idList<md5meshJointTransform_t> &list, idBounds &bounds) const
{
    bounds.Clear();

    for (int i = 0; i < meshes.Num(); i++) {
        idBounds b;
        CalcMeshBounds(meshes[i], list, b);
        bounds.AddBounds(b);
    }
}

void idMd5MeshFile::CalcMeshBounds(const md5meshMesh_t &mesh, const idList<md5meshJointTransform_t> &list, idBounds &bounds) const
{
    int						i;
    int						j;
    idVec3					pos;
    const md5meshWeight_t 	*weight;
    const md5meshVert_t 	*vert;
    const md5meshJointTransform_t *joint;

    bounds.Clear();

    vert = mesh.verts.Ptr();

    for (i = 0; i < mesh.verts.Num(); i++, vert++) {
        pos.Zero();
        weight = &mesh.weights[ vert->weightIndex ];

        joint = &list[weight->jointIndex];
        for (j = 0; j < vert->weightElem; j++, weight++) {
            pos += weight->weightValue * (joint->idwm * weight->pos + joint->idt);
        }

        bounds.AddPoint(pos);
    }
}
