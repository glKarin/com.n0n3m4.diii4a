#ifdef _MODEL_OBJ
#include "Model_obj.h"
#endif
#include "Model_psk.h"

using md5model::idMd5MeshFile;

static bool PSK_CheckId(const char chunk_id[PSK_CHUNK_ID_TOTAL_LENGTH], const char *target)
{
    return !idStr::Icmpn(chunk_id, target, PSK_CHUNK_ID_LENGTH);
}

idModelPsk::idModelPsk(void)
: file(NULL),
  types(0)
{}

idModelPsk::~idModelPsk(void)
{
    if(file)
        fileSystem->CloseFile(file);
}

ID_INLINE void idModelPsk::MarkType(int type)
{
    types |= (1 << type);
}

ID_INLINE bool idModelPsk::IsTypeMarked(int type) const
{
    return types & (1 << type);
}

void idModelPsk::Clear(void)
{
    vertexes.SetNum(0);
    wedges.SetNum(0);
    faces.SetNum(0);
    materials.SetNum(0);
    weights.SetNum(0);
    bones.SetNum(0);
    normals.SetNum(0);
    uvs.SetNum(0);
    colors.SetNum(0);
    if(file)
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }
    types = 0;
    memset(&header, 0, sizeof(header));
}

int idModelPsk::ReadHeader(pskHeader_t &header)
{
    char buffer[32];
    int curPos = file->Tell();
    int num = file->Read(buffer, sizeof(buffer));
    if(num == 0)
        return 0;
    file->Seek(curPos, FS_SEEK_SET);
    if(num < 32)
    {
        common->Warning("Unexpected end of file.(%d/32 bytes)", num);
        return -1;
    }

    memset(&header, 0, sizeof(header));
    file->Read(header.chunk_id, sizeof(header.chunk_id));
    file->ReadInt(header.chunk_type);
    file->ReadInt(header.chunk_datasize);
    file->ReadInt(header.chunk_datacount);

    return 32;
}

int idModelPsk::ReadVertexes(void)
{
    vertexes.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskVertex_t &item = vertexes[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
        file->ReadFloat(item[2]);
    }
    return vertexes.Num();
}

int idModelPsk::ReadWedges(void)
{
    wedges.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskWedge_t &item = wedges[i];
        file->ReadUnsignedInt(item.vertex_index);
        file->ReadFloat(item.u);
        file->ReadFloat(item.v);
        file->ReadUnsignedChar(item.material_index);
        file->Read(item.placeholder, 3);
    }
    return wedges.Num();
}

int idModelPsk::ReadFaces(void)
{
    faces.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskFace_t &item = faces[i];
        if(wedges.Num() > 65536)
        {
            file->ReadUnsignedInt(item.wedge_index1);
            file->ReadUnsignedInt(item.wedge_index2);
            file->ReadUnsignedInt(item.wedge_index3);
        }
        else
        {
            unsigned short sh;
            file->ReadUnsignedShort(sh);
            item.wedge_index1 = sh;
            file->ReadUnsignedShort(sh);
            item.wedge_index2 = sh;
            file->ReadUnsignedShort(sh);
            item.wedge_index3 = sh;
        }
        file->ReadUnsignedChar(item.material_index);
        file->ReadUnsignedChar(item.aux_material_index);
        file->ReadUnsignedInt(item.smooth_group);
    }
    return faces.Num();
}

int idModelPsk::ReadMaterials(void)
{
    materials.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskMaterial_t &item = materials[i];
        memset(&item.name, 0, sizeof(item.name));
        file->Read(item.name, sizeof(item.name));
        file->Read(item.placeholder, 24);
    }
    return materials.Num();
}

int idModelPsk::ReadWeights(void)
{
    weights.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskWeight_t &item = weights[i];
        memset(&item, 0, sizeof(item));
        file->ReadFloat(item.weight);
        file->ReadInt(item.vertex_index);
        file->ReadInt(item.bone_index);
    }
    return weights.Num();
}

int idModelPsk::ReadBones(void)
{
    bones.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskBone_t &item = bones[i];
        memset(&item.name, 0, sizeof(item.name));
        file->Read(item.name, sizeof(item.name));
        file->ReadInt(item.flags);
        file->ReadInt(item.num_children);
        file->ReadInt(item.vertex_index);
        file->ReadFloat(item.qx);
        file->ReadFloat(item.qy);
        file->ReadFloat(item.qz);
        file->ReadFloat(item.qw);
        file->ReadFloat(item.localx);
        file->ReadFloat(item.localy);
        file->ReadFloat(item.localz);
        file->ReadFloat(item.length);
        file->ReadFloat(item.xsize);
        file->ReadFloat(item.ysize);
        file->ReadFloat(item.zsize);
    }
    return bones.Num();
}

int idModelPsk::ReadNormals(void)
{
    normals.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskNormal_t &item = normals[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
        file->ReadFloat(item[2]);
    }
    return normals.Num();
}

int idModelPsk::ReadExtraUvs(void)
{
    uvs.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskUv_t &item = uvs[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
    }
    return uvs.Num();
}

int idModelPsk::ReadColors(void)
{
    colors.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskColor_t &item = colors[i];
        file->ReadUnsignedChar(item.r);
        file->ReadUnsignedChar(item.g);
        file->ReadUnsignedChar(item.b);
        file->ReadUnsignedChar(item.a);
    }
    return colors.Num();
}

int idModelPsk::Skip(void)
{
    int num = header.chunk_datasize * header.chunk_datacount;
    if(num == 0)
        return 0;
    file->Seek(num, FS_SEEK_CUR);
    return num;
}

bool idModelPsk::Parse(const char *pskPath)
{
    Clear();

    file = fileSystem->OpenFileRead(pskPath);
    if(!file)
        return false;

    int headerRes;
    bool err = false;
    while((headerRes = ReadHeader(header)) > 0)
    {
        idStr str(header.chunk_id, 0, 8);
        common->Printf("PSK header: %s, type=%d, size=%d, count=%d\n", str.c_str(), header.chunk_type, header.chunk_datasize, header.chunk_datacount);
        if(PSK_CheckId(header.chunk_id, "ACTRHEAD"))
        {
            MarkType(ACTRHEAD);
            Skip();
        }
        else if(PSK_CheckId(header.chunk_id, "PNTS0000"))
        {
            MarkType(PNTS0000);
            if(ReadVertexes() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "VTXW0000"))
        {
            MarkType(VTXW0000);
            if(ReadWedges() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "VTXW3200"))
        {
            MarkType(VTXW3200);
            if(ReadWedges() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "FACE0000"))
        {
            if(!IsTypeMarked(VTXW0000) && !IsTypeMarked(VTXW3200))
            {
                common->Warning("Wedges not read when reading face");
                {
                    err = true;
                    break;
                }
            }
            MarkType(FACE0000);
            if(ReadFaces() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "FACE3200"))
        {
            if(!IsTypeMarked(VTXW0000) && !IsTypeMarked(VTXW3200))
            {
                common->Warning("Wedges not read when reading face");
                {
                    err = true;
                    break;
                }
            }
            MarkType(FACE3200);
            if(ReadFaces() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "MATT0000"))
        {
            MarkType(MATT0000);
            if(ReadMaterials() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "RAWW0000"))
        {
            MarkType(RAWW0000);
            if(ReadWeights() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "RAWWEIGH"))
        {
            MarkType(RAWWEIGH);
            if(ReadWeights() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "REFSKELT"))
        {
            MarkType(REFSKELT);
            if(ReadBones() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "REFSKEL0"))
        {
            MarkType(REFSKEL0);
            if(ReadBones() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "VERTEXCO"))
        {
            MarkType(VERTEXCO);
            if(ReadColors() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "EXTRAUVS"))
        {
            MarkType(EXTRAUVS);
            if(ReadExtraUvs() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "VTXNORMS"))
        {
            MarkType(VTXNORMS);
            if(ReadNormals() < 0)
            {
                err = true;
                break;
            }
        }
        else
        {
            idStr errStr(header.chunk_id, 0, PSK_CHUNK_ID_LENGTH);
            common->Warning("Read unknown psk header type: %s", errStr.c_str());
            Skip();
        }
    }

    if(!err)
        err = headerRes != 0;

    if(err)
        Clear();
    else
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }

    return !err;
}

int idModelPsk::GroupFace(idList<idList<const pskFace_t *> > &faceGroup, idStrList &matList) const
{
    faceGroup.Clear();
    matList.Clear();

    for(int i = 0; i < faces.Num(); i++)
    {
        const pskFace_t *face = &faces[i];
        const pskMaterial_t *material = &materials[face->material_index];

        idList<const pskFace_t *> *group = NULL;
        for(int j = 0; j < matList.Num(); j++)
        {
            if(matList[j] == material->name)
            {
                group = &faceGroup[j];
                break;
            }
        }
        if(NULL == group)
        {
            int index = matList.Append(material->name);
            faceGroup.Append(idList<const pskFace_t *>());
            group = &faceGroup[index];
        }

        group->Append(face);
    }

    return matList.Num();
}

bool idModelPsk::ToMd5Mesh(idMd5MeshFile &md5mesh, float scale, bool addOrigin) const
{
    using namespace md5model;

    int i, j;
    md5meshJoint_t *md5Bone;
    const pskBone_t *refBone;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshJointTransform_t *jointTransform;
	int numBones = bones.Num();

    md5mesh.Commandline() = "Convert from unreal psk file";
	if(addOrigin)
		numBones++;

    // convert md5 joints
    idList<md5meshJoint_t> &md5Bones = md5mesh.Joints();
    md5Bones.SetNum(numBones);
	md5Bone = &md5Bones[0];
	if(addOrigin)
	{
		md5Bone->boneName = "origin";
		md5Bone->parentIndex = -1;
		md5Bone->pos.Zero();
		md5Bone->orient.Set(0.0, 0.0, 0.0, 1.0);
		md5Bone++;
	}

    for (i = 0, refBone = &bones[0]; i < bones.Num(); i++, md5Bone++, refBone++)
    {
        md5Bone->boneName = refBone->name;

        if (i == 0)
        {
            md5Bone->parentIndex = -1; // refBone->vertex_index - 1; // some is -1 not 0, so always force to -1
        }
        else
        {
            md5Bone->parentIndex = refBone->vertex_index;
        }

        //Ren_Print("R_LoadPSK: '%s' has bone '%s' with parent index %i\n", modName, md5Bone->name, md5Bone->parentIndex);

        if (md5Bone->parentIndex >= bones.Num())
        {
            common->Warning("R_LoadPSK: '%d' has bone '%s' with bad parent index %i while numBones is %i", i,
                            md5Bone->boneName.c_str(), md5Bone->parentIndex, bones.Num());
            return false;
        }

		if(addOrigin)
			md5Bone->parentIndex += 1;

        boneOrigin[0] = refBone->localx;
        boneOrigin[1] = refBone->localy;
        boneOrigin[2] = refBone->localz;
		if(scale > 0.0f)
			boneOrigin *= scale;

        // I have really no idea why the .psk format stores the first quaternion with inverted quats.
        // Furthermore only the X and Z components of the first quat are inverted ?!?!
        if (i == 0)
        {
            boneQuat[0] = refBone->qx;
            boneQuat[1] = -refBone->qy;
            boneQuat[2] = refBone->qz;
            boneQuat[3] = refBone->qw;
        }
        else
        {
            boneQuat[0] = -refBone->qx;
            boneQuat[1] = -refBone->qy;
            boneQuat[2] = -refBone->qz;
            boneQuat[3] = refBone->qw;
        }

        md5Bone->pos = boneOrigin;
        //MatrixTransformPoint(unrealToQuake, boneOrigin, md5Bone->origin);

#if ETW_PSK
        md5Bone->orient = boneQuat;
#else
        md5Bone->orient = boneQuat.Inverse();
#endif

        md5Bone->orient.Normalize();

        //QuatClear(md5Bone->rotation);

        if (md5Bone->parentIndex >= 0)
        {
            idVec3 rotated;
            idQuat quat;

            md5meshJoint_t *parent;

            parent = &md5Bones[md5Bone->parentIndex];

            idMat3 m = parent->orient.ToMat3();
#if ETW_PSK
            rotated = m.TransposeSelf() * md5Bone->pos;
            //QuatTransformVector(md5Bone->rotation, md5Bone->origin, rotated);

            quat = parent->orient * md5Bone->orient;
            //QuatMultiply1(parent->rotation, md5Bone->rotation, quat);
#else
            rotated = m * md5Bone->pos;
            //QuatTransformVector(md5Bone->rotation, md5Bone->origin, rotated);

            quat = md5Bone->orient * parent->orient;
            //QuatMultiply1(parent->rotation, md5Bone->rotation, quat);
#endif
            md5Bone->orient = quat.Normalize();
            md5Bone->pos = parent->pos + rotated;
        }

//            MatrixSetupTransformFromQuat(md5Bone->inverseTransform, md5Bone->orient, md5Bone->pos);
//            mat4_inverse_self(md5Bone->inverseTransform);
    }

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    // convert md5 mesh
    idList<md5meshMesh_t> &md5Meshes = md5mesh.Meshes();

    idList<idList<const pskFace_t *> > faceGroup;
    idStrList matList;

    int num = GroupFace(faceGroup, matList);

    for(j = 0; j < num; j++)
    {
        const idList<const pskFace_t *> &faceList = faceGroup[j];
        md5Meshes.Append(md5meshMesh_t());
        md5meshMesh_t &mesh = md5Meshes[j];
        mesh.shader = matList[j];

        idList<unsigned int> md5Vertexes; // cache saved vert for remove dup vert
        for(i = 0; i < faceList.Num(); i++)
        {
            const pskFace_t *face = faceList[i];

            unsigned int wedgeIndexes[3] = {
                face->wedge_index1,
                face->wedge_index2,
                face->wedge_index3,
            };
            int md5VertIndexes[3];
            for(int k = 0; k < 3; k++)
            {
                const unsigned int wedgeIndex = wedgeIndexes[k];
                // find vert is cached
                int n;
                for(n = 0; n < md5Vertexes.Num(); n++)
                {
                    if(wedgeIndex == md5Vertexes[n])
                        break;
                }
                if(n < md5Vertexes.Num())
                {
                    md5VertIndexes[k] = n;
                    continue;
                }

                const pskWedge_t *wedge = &wedges[wedgeIndex];
                const pskVertex_t *vertex = &vertexes[wedge->vertex_index];

                idList<const pskWeight_t *> vertWeights;
                for(int m = 0; m < weights.Num(); m++)
                {
                    const pskWeight_t *weight = &weights[m];
                    if(weight->vertex_index == wedge->vertex_index && weight->weight > 0.0f)
                    {
                        vertWeights.Append(weight);
                    }
                }
                if(vertWeights.Num() > 4)
                {
                    common->Warning("wedge weight num is overflow in wedge '%d': %d", wedgeIndex, vertWeights.Num());
                    return false;
                }
                if(vertWeights.Num() == 0)
                {
                    common->Warning("wedge weight num is zero in wedge '%d'", wedgeIndex);
                    return false;
                }
                md5meshVert_t md5Vert;
                md5Vert.uv.Set(wedge->u, wedge->v);
                md5Vert.weightIndex = mesh.weights.Num();
                md5Vert.weightElem = vertWeights.Num();
                md5VertIndexes[k] = mesh.verts.Append(md5Vert); // Add vert
                md5Vertexes.Append(wedgeIndex); // cache vert

                float w = 0.0f;
                for(int m = 0; m < vertWeights.Num(); m++)
                {
                    const pskWeight_t *weight = vertWeights[m];
                    md5meshWeight_t md5Weight;
                    md5Weight.jointIndex = weight->bone_index;
					if(addOrigin)
						md5Weight.jointIndex += 1;
                    jointTransform = &jointTransforms[md5Weight.jointIndex];
                    if(vertWeights.Num() == 1)
                        md5Weight.weightValue = 1.0f;
                    else
                        md5Weight.weightValue = weight->weight;

                    w += md5Weight.weightValue;
					idVec3 pos = *vertex;
					if(scale > 0.0f)
						pos *= scale;
                    jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight.pos);

                    mesh.weights.Append(md5Weight); // Add weight
                }
                if(w < 1.0f)
                {
                    common->Warning("wedge '%d' weight sum is less than 1,0", wedgeIndex);
                }
            }
            md5meshTri_t md5Tri;
            md5Tri.vertIndex1 = md5VertIndexes[0];
            md5Tri.vertIndex2 = md5VertIndexes[1];
            md5Tri.vertIndex3 = md5VertIndexes[2];
            mesh.tris.Append(md5Tri); // Add tri
        }
    }

    return true;
}

#ifdef _MODEL_OBJ
bool idModelPsk::ToObj(objModel_t &objModel, bool keepDup) const
{
    int i, j;

    idList<idList<const pskFace_t *> > faceGroup;
    idStrList matList;

    int num = GroupFace(faceGroup, matList);

    for(j = 0; j < num; j++)
    {
        objModel.objects.Append(new objObject_t);
        objObject_t *objObject = objModel.objects[j];
        objObject->material = matList[j];

        const idList<const pskFace_t *> &faceList = faceGroup[j];
        idList<unsigned int> objVertexes; // cache saved vert for remove dup vert

        for(i = 0; i < faceList.Num(); i++)
        {
            const pskFace_t *face = faceList[i];

            unsigned int wedgeIndexes[3] = {
                face->wedge_index1,
                face->wedge_index2,
                face->wedge_index3,
            };
            int objVertIndexes[3];
            for(int k = 0; k < 3; k++)
            {
                const unsigned int wedgeIndex = wedgeIndexes[k];
                // find vert is cached
				if(!keepDup)
				{
					int n;
					for(n = 0; n < objVertexes.Num(); n++)
					{
						if(wedgeIndex == objVertexes[n])
							break;
					}
					if(n < objVertexes.Num())
					{
						objVertIndexes[k] = n;
						continue;
					}
					objVertexes.Append(wedgeIndex); // cache vert
				}

                const pskWedge_t *wedge = &wedges[wedgeIndex];
                objVertIndexes[k] = objObject->vertexes.Append(vertexes[wedge->vertex_index]);

                objObject->texcoords.Append(idVec2(wedge->u, wedge->v));

                objObject->normals.Append(wedge->vertex_index < (unsigned int)normals.Num() ? normals[wedge->vertex_index] : idVec3(0.0f, 0.0f, 0.0f));
            }

			objObject->indexes.Append(objVertIndexes[0]);
			objObject->indexes.Append(objVertIndexes[2]); // swap
			objObject->indexes.Append(objVertIndexes[1]);
        }
    }

    return true;
}
#endif

bool idModelPsk::Check(void) const
{
    int i;

    for(i = 0; i < faces.Num(); i++)
    {
        const pskFace_t *face = &faces[i];
        const pskMaterial_t *material = &materials[face->material_index];

        if(face->wedge_index1 >= (unsigned int)wedges.Num())
        {
            common->Warning("wedge index 1 '%d' is overflow in face '%d'", face->wedge_index1, i);
            return false;
        }
        if(face->wedge_index2 >= (unsigned int)wedges.Num())
        {
            common->Warning("wedge index 2 '%d' is overflow in face '%d'", face->wedge_index2, i);
            return false;
        }
        if(face->wedge_index3 >= (unsigned int)wedges.Num())
        {
            common->Warning("wedge index 3 '%d' is overflow in face '%d'", face->wedge_index3, i);
            return false;
        }

        const pskWedge_t *wedge1 = &wedges[face->wedge_index1];
        const pskWedge_t *wedge2 = &wedges[face->wedge_index2];
        const pskWedge_t *wedge3 = &wedges[face->wedge_index3];
        if(wedge1->vertex_index >= (unsigned int)vertexes.Num())
        {
            common->Warning("vertex index '%d' is overflow in wedge 1 '%d'", wedge1->vertex_index, face->wedge_index1);
            return false;
        }
        if(wedge2->vertex_index >= (unsigned int)vertexes.Num())
        {
            common->Warning("vertex index '%d' is overflow in wedge 2 '%d'", wedge2->vertex_index, face->wedge_index2);
            return false;
        }
        if(wedge3->vertex_index >= (unsigned int)vertexes.Num())
        {
            common->Warning("vertex index '%d' is overflow in wedge 3 '%d'", wedge3->vertex_index, face->wedge_index3);
            return false;
        }
    }

    return true;
}

void idModelPsk::Print(void) const
{
#define PSK_PART_PRINT(name, list, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
    for(int i = 0; i < list.Num(); i++) {  \
         Sys_Printf(fmt "\n", __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    PSK_PART_PRINT(vertex, vertexes, "(%f, %f, %f)   ", vertexes[i][0], vertexes[i][1], vertexes[i][2])
    PSK_PART_PRINT(wedge, wedges, "vertex=%u uv=(%f, %f) mat=%d   ", wedges[i].vertex_index, wedges[i].u, wedges[i].v, wedges[i].material_index)
    PSK_PART_PRINT(face, faces, "wedge=(%u, %u, %u) mat=(%d, %d) sg=%u   ", faces[i].wedge_index1, faces[i].wedge_index2, faces[i].wedge_index3, faces[i].material_index, faces[i].aux_material_index, faces[i].smooth_group)
    PSK_PART_PRINT(material, materials, "%s   ", materials[i].name)
    PSK_PART_PRINT(weight, weights, "weight=%f parent=%d bone=%d   ", weights[i].weight, weights[i].vertex_index, weights[i].bone_index)
    PSK_PART_PRINT(bone, bones, "%s flags=%x children=%d parent=%d quat=(%f, %f, %f, %f) pos=(%f, %f, %f) length=%f size=(%f, %f, %f)   ", bones[i].name, bones[i].flags, bones[i].num_children, bones[i].vertex_index, bones[i].qx, bones[i].qy, bones[i].qz, bones[i].qw, bones[i].localx, bones[i].localy, bones[i].localz, bones[i].length, bones[i].xsize, bones[i].ysize, bones[i].zsize)
    PSK_PART_PRINT(normal, normals, "(%f, %f, %f)   ", normals[i][0], normals[i][1], normals[i][2])
    PSK_PART_PRINT(color, colors, "(%u, %u, %u, %u)   ", colors[i].r, colors[i].g, colors[i].b, colors[i].a)
    PSK_PART_PRINT(extra uv, uvs, "(%f, %f)   ", uvs[i][0], uvs[i][1])

#undef PSK_PART_PRINT
}

#ifdef _MODEL_OBJ
static void R_ConvertPskToObj_f(const idCmdArgs &args)
{
    const char *pskPath = args.Argv(1);
    idModelPsk psk;
    if(psk.Parse(pskPath))
    {
        //psk.Print();
        if(psk.Check())
        {
            objModel_t objModel;
            if(psk.ToObj(objModel, true))
            {
                idStr objPath = pskPath;
                objPath.SetFileExtension(".obj");
                OBJ_Write(&objModel, objPath.c_str());
                common->Printf("Convert obj successful: %s -> %s\n", pskPath, objPath.c_str());
            }
            else
                common->Warning("Convert obj fail: %s", pskPath);
        }
        else
            common->Warning("Check psk error: %s", pskPath);
    }
    else
        common->Warning("Parse psk fail: %s", pskPath);
}
#endif

