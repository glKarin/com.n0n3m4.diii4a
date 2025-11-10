#ifdef _MODEL_OBJ
#include "Model_obj.h"
#endif
#include "Model_psk.h"
#include "Model_md5mesh.h"

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

void idModelPsk::MarkType(int type)
{
    types |= (1 << type);
}

bool idModelPsk::IsTypeMarked(int type) const
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
    morphInfos.SetNum(0);
    morphDatas.SetNum(0);
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
        common->Warning("Unexpected end of file(%d/32 bytes).", num);
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
        file->ReadInt(item.parent_index);
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

int idModelPsk::ReadMorphInfos(void)
{
    morphInfos.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskMorphInfo_t &item = morphInfos[i];
        memset(&item.name, 0, sizeof(item.name));
        file->Read(item.name, sizeof(item.name));
        file->ReadInt(item.vertex_count);
    }
    return morphInfos.Num();
}

int idModelPsk::ReadMorphDatas(void)
{
    morphDatas.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        pskMorphData_t &item = morphDatas[i];
        file->ReadFloat(item.position_deltax);
        file->ReadFloat(item.position_deltay);
        file->ReadFloat(item.position_deltaz);
        file->ReadFloat(item.tangent_z_deltax);
        file->ReadFloat(item.tangent_z_deltay);
        file->ReadFloat(item.tangent_z_deltaz);
        file->ReadInt(item.point_index);
    }
    return morphDatas.Num();
}

int idModelPsk::Skip(void)
{
    int num = header.chunk_datasize * header.chunk_datacount;
    if(num == 0)
        return 0;
    file->Seek(num, FS_SEEK_CUR);
    return num;
}

bool idModelPsk::Parse(const char *filePath)
{
    Clear();

    file = fileSystem->OpenFileRead(filePath);
    if(!file)
    {
        common->Warning("Load psk file fail: %s", filePath);
        return false;
    }

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
        else if(PSK_CheckId(header.chunk_id, "MRPHINFO"))
        {
            MarkType(MRPHINFO);
            if(ReadMorphInfos() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "MRPHDATA"))
        {
            MarkType(MRPHDATA);
            if(ReadMorphDatas() < 0)
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

bool idModelPsk::ToMd5Mesh(idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *meshOffset, const idMat3 *meshRotation) const
{
    int i, j;
    md5meshJoint_t *md5Bone;
    const pskBone_t *refBone;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshJointTransform_t *jointTransform;
	int numBones = bones.Num();

    md5mesh.Commandline() = va("Convert from unreal psk file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
	if(meshOffset)
		md5mesh.Commandline().Append(va(", offset=%g %g %g", meshOffset->x, meshOffset->y, meshOffset->z));
	if(meshRotation)
	{
		idAngles angle = meshRotation->ToAngles();
		md5mesh.Commandline().Append(va(", rotation=%g %g %g", angle[0], angle[1], angle[2]));
	}

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
		md5Bone->orient.Set(0.0f, 0.0f, 0.0f, 1.0f);
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
			if(refBone->parent_index < 0)
			{
				common->Warning("Has more root bone: %d", i);
				md5Bone->parentIndex = addOrigin ? -1 : 0;
			}
			else
				md5Bone->parentIndex = refBone->parent_index;
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

		if(i == 0 || refBone->parent_index < 0)
		{
			if(meshRotation && !meshRotation->IsIdentity())
			{
				boneOrigin *= *meshRotation;
				boneQuat = (meshRotation->Transpose() * boneQuat.ToMat3()).ToQuat();
			}
			if(meshOffset && !meshOffset->IsZero())
				boneOrigin += *meshOffset;
		}
		if(scale > 0.0f)
			boneOrigin *= scale;

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
    md5Meshes.SetNum(num);

    for(j = 0; j < num; j++)
    {
        const idList<const pskFace_t *> &faceList = faceGroup[j];
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

				idVec3 pos = *vertex;
				if(meshRotation && !meshRotation->IsIdentity())
					pos *= *meshRotation;
				if(meshOffset && !meshOffset->IsZero())
					pos += *meshOffset;
				if(scale > 0.0f)
					pos *= scale;

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
					{
						if(weight->weight != 1.0f)
							common->Warning("wedge '%d' only 1 bone '%s' but weight is not 1 '%f'", wedgeIndex, bones[weight->bone_index].name, weight->weight);
                        md5Weight.weightValue = 1.0f;
					}
                    else
                        md5Weight.weightValue = weight->weight;

                    w += md5Weight.weightValue;
                    jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight.pos);

                    mesh.weights.Append(md5Weight); // Add weight
                }
                if(WEIGHTS_SUM_NOT_EQUALS_ONE(w))
                {
                    common->Warning("wedge '%d' weight sum is less than 1.0: %f", wedgeIndex, w);
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

    for(i = 0; i < num; i++)
    {
        objModel.objects.Append(new objObject_t);
        objObject_t *objObject = objModel.objects[i];
        objObject->material = matList[i];

        const idList<const pskFace_t *> &faceList = faceGroup[i];
        idList<unsigned int> objVertexes; // cache saved vert for remove dup vert

        for(j = 0; j < faceList.Num(); j++)
        {
            const pskFace_t *face = faceList[j];

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

void idModelPsk::Print(void) const
{
#define PSK_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
	if(all) \
    for(int i = 0; i < list.Num(); i++) {  \
         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    PSK_PART_PRINT(vertex, vertexes, false, "(%f, %f, %f)   ", vertexes[i][0], vertexes[i][1], vertexes[i][2])
    PSK_PART_PRINT(wedge, wedges, false, "vertex=%u uv=(%f, %f) mat=%d   ", wedges[i].vertex_index, wedges[i].u, wedges[i].v, wedges[i].material_index)
    PSK_PART_PRINT(face, faces, false, "wedge=(%u, %u, %u) mat=(%d, %d) sg=%u   ", faces[i].wedge_index1, faces[i].wedge_index2, faces[i].wedge_index3, faces[i].material_index, faces[i].aux_material_index, faces[i].smooth_group)
    PSK_PART_PRINT(material, materials, true, "%s   ", materials[i].name)
    PSK_PART_PRINT(weight, weights, false, "weight=%f parent=%d bone=%d   ", weights[i].weight, weights[i].vertex_index, weights[i].bone_index)
    PSK_PART_PRINT(bone, bones, true, "%s flags=%x children=%d parent=%d quat=(%f, %f, %f, %f) pos=(%f, %f, %f) length=%f size=(%f, %f, %f)   ", bones[i].name, bones[i].flags, bones[i].num_children, bones[i].parent_index, bones[i].qx, bones[i].qy, bones[i].qz, bones[i].qw, bones[i].localx, bones[i].localy, bones[i].localz, bones[i].length, bones[i].xsize, bones[i].ysize, bones[i].zsize)
    PSK_PART_PRINT(normal, normals, false, "(%f, %f, %f)   ", normals[i][0], normals[i][1], normals[i][2])
    PSK_PART_PRINT(color, colors, false, "(%u, %u, %u, %u)   ", colors[i].r, colors[i].g, colors[i].b, colors[i].a)
    PSK_PART_PRINT(extra uv, uvs, false, "(%f, %f)   ", uvs[i][0], uvs[i][1])
    PSK_PART_PRINT(morph info, morphInfos, false, "%s vertex count=%d   ", morphInfos[i].name, morphInfos[i].vertex_count)
    PSK_PART_PRINT(morph data, morphDatas, false, "position_delta=(%f, %f, %f) tangent_z_delta=(%f, %f, %f) point index=%d   ", morphDatas[i].position_deltax, morphDatas[i].position_deltay, morphDatas[i].position_deltaz, morphDatas[i].tangent_z_deltax, morphDatas[i].tangent_z_deltay, morphDatas[i].tangent_z_deltaz, morphDatas[i].point_index)

#undef PSK_PART_PRINT
}

#ifdef _MODEL_OBJ
static void R_ConvertPskToObj_f(const idCmdArgs &args)
{
    const char *filePath = args.Argv(1);
    idModelPsk psk;
    if(psk.Parse(filePath))
    {
        //psk.Print();
		objModel_t objModel;
		if(psk.ToObj(objModel, true))
		{
			idStr objPath = filePath;
			objPath.SetFileExtension(".obj");
			OBJ_Write(&objModel, objPath.c_str());
			common->Printf("Convert obj successful: %s -> %s\n", filePath, objPath.c_str());
		}
		else
			common->Warning("Convert obj fail: %s", filePath);
    }
    else
        common->Warning("Parse psk fail: %s", filePath);
}
#endif

