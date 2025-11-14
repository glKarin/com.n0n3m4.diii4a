#include "Model_psa.h"
#include "Model_md5anim.h"

idModelPsa::idModelPsa(void)
: file(NULL),
  types(0)
{}

idModelPsa::~idModelPsa(void)
{
    if(file)
        fileSystem->CloseFile(file);
}

void idModelPsa::MarkType(int type)
{
    types |= (1 << type);
}

bool idModelPsa::IsTypeMarked(int type) const
{
    return types & (1 << type);
}

void idModelPsa::Clear(void)
{
    bones.SetNum(0);
    animInfos.SetNum(0);
    animKeys.SetNum(0);
    if(file)
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }
    types = 0;
    memset(&header, 0, sizeof(header));
}

int idModelPsa::ReadHeader(psaHeader_t &header)
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

int idModelPsa::ReadBones(void)
{
    bones.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        psaBone_t &item = bones[i];
        memset(&item.name, 0, sizeof(item.name));
        file->Read(item.name, 64);
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

int idModelPsa::ReadAnimInfos(void)
{
    animInfos.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        psaAnimInfo_t &item = animInfos[i];
        memset(&item.action_name, 0, sizeof(item.action_name));
        file->Read(item.action_name, sizeof(item.action_name));
        memset(&item.group_name, 0, sizeof(item.group_name));
        file->Read(item.group_name, sizeof(item.group_name));
        file->ReadInt(item.total_bones);
        file->ReadInt(item.root_include);
        file->ReadInt(item.key_compression_style);
        file->ReadInt(item.key_quotum);
        file->ReadFloat(item.key_reduction);
        file->ReadFloat(item.track_time);
        file->ReadFloat(item.anim_rate);
        file->ReadInt(item.start_bone);
        file->ReadInt(item.first_raw_frame);
        file->ReadInt(item.num_raw_frames);
    }
    return animInfos.Num();
}

int idModelPsa::ReadAnimKeys(void)
{
    animKeys.SetNum(header.chunk_datacount);
    for(int i = 0; i < header.chunk_datacount; i++)
    {
        psaAnimKey_t &item = animKeys[i];
        file->ReadFloat(item.posx);
        file->ReadFloat(item.posy);
        file->ReadFloat(item.posz);
        file->ReadFloat(item.quatx);
        file->ReadFloat(item.quaty);
        file->ReadFloat(item.quatz);
        file->ReadFloat(item.quatw);
        file->ReadFloat(item.time);
    }
    return animKeys.Num();
}

int idModelPsa::Skip(void)
{
    int num = header.chunk_datasize * header.chunk_datacount;
    if(num == 0)
        return 0;
    file->Seek(num, FS_SEEK_CUR);
    return num;
}

bool idModelPsa::Parse(const char *filePath)
{
    Clear();

    file = fileSystem->OpenFileRead(filePath);
    if(!file)
    {
        common->Warning("Load psa file fail: %s", filePath);
        return false;
    }

    int headerRes;
    bool err = false;
    while((headerRes = ReadHeader(header)) > 0)
    {
        idStr str(header.chunk_id, 0, 8);
        //common->Printf("PSA header: %s, type=%d, size=%d, count=%d\n", str.c_str(), header.chunk_type, header.chunk_datasize, header.chunk_datacount);
        if(PSK_CheckId(header.chunk_id, "ANIMHEAD"))
        {
            MarkType(ANIMHEAD);
            Skip();
        }
        else if(PSK_CheckId(header.chunk_id, "BONENAME"))
        {
            MarkType(BONENAME);
            if(ReadBones() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "ANIMINFO"))
        {
            MarkType(ANIMINFO);
            if(ReadAnimInfos() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "ANIMKEYS"))
        {
            MarkType(ANIMKEYS);
            if(ReadAnimKeys() < 0)
            {
                err = true;
                break;
            }
        }
        else if(PSK_CheckId(header.chunk_id, "SCALEKEY"))
        {
            MarkType(SCALEKEY);
            Skip();
        }
        else
        {
            idStr errStr(header.chunk_id, 0, PSK_CHUNK_ID_LENGTH);
            common->Warning("Read unknown psa header type: %s", errStr.c_str());
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

bool idModelPsa::ToMd5Anim(const idModelPsk &psk, idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *animOffset, const idMat3 *animRotation) const
{
    int i, j;
    md5animHierarchy_t *md5Hierarchy;
    const md5meshJoint_t *meshJoint;
    const psaAnimKey_t *key;
    md5animFrame_t *md5Frame;
    md5meshJointTransform_t *jointTransform;
    md5meshJointTransform_t *frameTransform;
    md5animBaseframe_t *md5BaseFrame;
    md5meshJoint_t *md5Bone;

    const psaAnimInfo_t *animInfo = &animInfos[0];

    const idList<md5meshJoint_t> joints = md5mesh.Joints();
    const int numBones = joints.Num();

    md5anim.FrameRate() = (int)animInfo->anim_rate;
    md5anim.NumAnimatedComponents() = numBones * 6;

    md5anim.Commandline() = va("Convert from unreal psa file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
	if(animOffset)
		md5anim.Commandline().Append(va(", offset=%g,%g,%g", animOffset->x, animOffset->y, animOffset->z));
	if(animRotation)
	{
		idAngles angle = animRotation->ToAngles();
		md5anim.Commandline().Append(va(", rotation=%g %g %g", angle[0], angle[1], angle[2]));
	}

    // convert md5 joints
    idList<md5animHierarchy_t> &md5Bones = md5anim.Hierarchies();
    md5Bones.SetNum(numBones);

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    idStrList boneList;
    boneList.SetNum(numBones);
    for(i = 0, meshJoint = &joints[0], md5Hierarchy = &md5Bones[0]; i < numBones; i++, meshJoint++, md5Hierarchy++)
    {
        boneList[i] = meshJoint->boneName;

        md5Hierarchy->boneName = meshJoint->boneName;
        md5Hierarchy->numComp = MD5ANIM_ALL;
        md5Hierarchy->frameIndex = i * 6;
        md5Hierarchy->parentIndex = meshJoint->parentIndex;
    }

    // convert md5 bounds
    idList<md5animBounds_t> &md5Bounds = md5anim.Bounds();
    md5Bounds.SetNum(animInfo->num_raw_frames);
    for(i = 0; i < md5Bounds.Num(); i++)
    {
        md5Bounds[i].Zero();
    }

    // convert md5 baseframe
    idList<md5animBaseframe_t> &md5Baseframes = md5anim.Baseframe();
    md5Baseframes.SetNum(numBones);
    for(i = 0, jointTransform = &jointTransforms[0], md5BaseFrame = &md5Baseframes[0]; i < md5Baseframes.Num(); i++, jointTransform++, md5BaseFrame++)
    {
        md5BaseFrame->xPos = jointTransform->t.x;
        md5BaseFrame->yPos = jointTransform->t.y;
        md5BaseFrame->zPos = jointTransform->t.z;
        idCQuat q = jointTransform->q.ToQuat().ToCQuat();
        md5BaseFrame->xOrient = q.x;
        md5BaseFrame->yOrient = q.y;
        md5BaseFrame->zOrient = q.z;
    }

    // convert md5 frames
    idList<md5animFrames_t> &md5Frames = md5anim.Frames();
    md5Frames.SetNum(animInfo->num_raw_frames);
    int base;
    for(i = 0, base = 0; i < animInfo->num_raw_frames; i++, base += animInfo->total_bones)
    {
        md5animFrames_t &frames = md5Frames[i];
        frames.index = i;
        frames.joints.SetNum(numBones);

        idList<md5meshJoint_t> md5Joints = joints;

        idHashTable<int> boneMap;

        for(j = 0; j < animInfo->total_bones; j++)
        {
            int md5Index = boneList.FindIndex(bones[j].name);
            if(md5Index != -1)
            {
                int index = base + j;
                boneMap.Set(bones[j].name, index);
            }
        }

        for(j = 0; j < numBones; j++)
        {
            if(j == 0 && addOrigin)
                continue;

            meshJoint = &joints[j];

            int index = 0;
            idVec3 boneOrigin;
            idQuat boneQuat;
            md5Bone = &md5Joints[j];
            int *rindex = &index;

            md5Bone->boneName = meshJoint->boneName;
            md5Bone->parentIndex = meshJoint->parentIndex;

            if(boneMap.Get(meshJoint->boneName, &rindex)) // anim key bone in psk mesh
			{
				key = &animKeys[*rindex];
                boneOrigin[0] = key->posx;
                boneOrigin[1] = key->posy;
                boneOrigin[2] = key->posz;

                // I have really no idea why the .psk format stores the first quaternion with inverted quats.
                // Furthermore only the X and Z components of the first quat are inverted ?!?!
                if (md5Bone->parentIndex == -1)
                {
                    boneQuat[0] = key->quatx;
                    boneQuat[1] = -key->quaty;
                    boneQuat[2] = key->quatz;
                    boneQuat[3] = key->quatw;
                }
                else
                {
                    boneQuat[0] = -key->quatx;
                    boneQuat[1] = -key->quaty;
                    boneQuat[2] = -key->quatz;
                    boneQuat[3] = key->quatw;
                }
//                if(md5Bone->boneName == "Bone_knee_L_03" || md5Bone->boneName == "Bone_knee_R_03") {
//                    boneOrigin[0] = 0;
//                    boneOrigin[1] = 0;
//                    boneOrigin[2] = 0;
//                }
			}
			else // anim key bone not in psk mesh
			{
				if(i == 0)
					common->Warning("Bone not found in psa: %s", meshJoint->boneName.c_str());
#if 0
				if (md5Bone->parentIndex >= 0)
				{
					idVec3 rotated;
					idQuat quat;

					const md5meshJoint_t *parent = &joints[md5Bone->parentIndex];

					idMat3 m = parent->orient.ToMat3();
#if ETW_PSK
					rotated = m.TransposeSelf() * (meshJoint->pos - parent->pos);

					quat = (m * meshJoint->orient.ToMat3()).ToQuat();
#else
					rotated = m * (meshJoint->pos - parent->pos);

					quat = (meshJoint->orient.Inverse().ToMat3() * m).ToQuat();
#endif
					boneQuat = quat.Normalize();
					boneOrigin = rotated;
				}
				else
				{
					boneQuat = meshJoint->orient.Inverse();
					boneOrigin = meshJoint->pos;
				}
#else
                index = j;
                if(addOrigin)
                    index--;

                const pskBone_t *refBone = &psk.bones[index];
                boneOrigin[0] = refBone->localx;
                boneOrigin[1] = refBone->localy;
                boneOrigin[2] = refBone->localz;
                if (md5Bone->parentIndex < 0)
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
#endif
			}

			int rootIndex = addOrigin ? 0 : -1;
			if (md5Bone->parentIndex == rootIndex)
			{
				if(animRotation && !animRotation->IsIdentity())
				{
					boneOrigin *= *animRotation;
					boneQuat = (animRotation->Transpose() * boneQuat.ToMat3()).ToQuat();
				}
				if(animOffset && !animOffset->IsZero())
					boneOrigin += *animOffset;
			}
			if(scale > 0.0f)
				boneOrigin *= scale;

            md5Bone->pos = boneOrigin;

#if ETW_PSK
            md5Bone->orient = boneQuat;
#else
            md5Bone->orient = boneQuat.Inverse();
#endif

            md5Bone->orient.Normalize();

            if (md5Bone->parentIndex >= 0)
            {
                idVec3 rotated;
                idQuat quat;

                md5meshJoint_t *parent = &md5Joints[md5Bone->parentIndex];

                idMat3 m = parent->orient.ToMat3();
#if ETW_PSK
                rotated = m.TransposeSelf() * md5Bone->pos;

                quat = parent->orient * md5Bone->orient;
#else
                rotated = m * md5Bone->pos;

                quat = md5Bone->orient * parent->orient;
#endif
                md5Bone->orient = quat.Normalize();
                md5Bone->pos = parent->pos + rotated;
            }
        }

        idList<md5meshJointTransform_t> frameTransforms;
        idMd5MeshFile::ConvertJointTransforms(md5Joints, frameTransforms);

        // calc frame bounds
        md5mesh.CalcBounds(frameTransforms, md5Bounds[i]);

        md5Frame = &frames.joints[0];
        for(int m = 0; m < numBones; m++, md5Frame++)
        {
            frameTransform = &frameTransforms[m];
            idVec3 t;
            idCQuat q;

            t = frameTransform->t;
            q = frameTransform->q.ToCQuat();

            md5Frame->xPos = t.x;
            md5Frame->yPos = t.y;
            md5Frame->zPos = t.z;
            md5Frame->xOrient = q.x;
            md5Frame->yOrient = q.y;
            md5Frame->zOrient = q.z;
        }
    }

    return true;
}

void idModelPsa::Print(void) const
{
#define MODEL_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
	if(all) \
    for(int i = 0; i < list.Num(); i++) {    \
         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    MODEL_PART_PRINT(bone, bones, true, "%s flags=%x children=%d parent=%d quat=(%f, %f, %f, %f) pos=(%f, %f, %f) length=%f size=(%f, %f, %f)   ", bones[i].name, bones[i].flags, bones[i].num_children, bones[i].parent_index, bones[i].qx, bones[i].qy, bones[i].qz, bones[i].qw, bones[i].localx, bones[i].localy, bones[i].localz, bones[i].length, bones[i].xsize, bones[i].ysize, bones[i].zsize)
    MODEL_PART_PRINT(animinfo, animInfos, true, "action=%s group=%s bones=%d root_include=%d key_compression_style=%d key_quotum=%d key_reduction=%f track_time=%f anim_rate=%f start_bone=%d first_raw_frame=%d num_raw_frames=%d  ", animInfos[i].action_name, animInfos[i].group_name, animInfos[i].total_bones, animInfos[i].root_include, animInfos[i].key_compression_style, animInfos[i].key_quotum, animInfos[i].key_reduction, animInfos[i].track_time, animInfos[i].anim_rate, animInfos[i].start_bone, animInfos[i].first_raw_frame, animInfos[i].num_raw_frames)
//#undef MODEL_PART_PRINT
//#define MODEL_PART_PRINT(name, list, fmt, ...) \
//    Sys_Printf(#name " num: %d\n", list.Num()); \
//    for(int i = 0; i < animInfos[0].total_bones; i++) {    \
//         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
//    }
    //MODEL_PART_PRINT(animkey, animKeys, "pos=(%f, %f, %f) quat=(%f, %f, %f, %f) time=%f   ", animKeys[i].posx, animKeys[i].posy, animKeys[i].posz, animKeys[i].quatx, animKeys[i].quaty, animKeys[i].quatz, animKeys[i].quatw, animKeys[i].time)

    /*
Header ANIMHEAD, type=20100422, size=0, count=0
Header BONENAME, type=0, size=120, count=243
Header ANIMINFO, type=0, size=168, count=1
Header ANIMKEYS, type=0, size=32, count=39609
Header SCALEKEY, type=0, size=16, count=0
animinfo num: 1
action=girl015_04_wp06a_base_idle01 group=None bones=243 root_include=0 key_compression_style=0 key_quotum=39609 key_reduction=0.000000 track_time=163.000000 anim_rate=30.185184 start_bone=0 first_raw_frame=0 num_raw_frames=163  ------------------------------------------------------
     */
#undef MODEL_PART_PRINT
}

static int R_ConvertPskPsaToMd5(const char *meshPath, bool doMesh = true, const idStrList *animPaths = NULL, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	int ret = 0;

    idModelPsk psk;
    idMd5MeshFile md5MeshFile;
    bool meshRes = false;
    if(psk.Parse(meshPath))
    {
        //psk.Print();
		if(psk.ToMd5Mesh(md5MeshFile, scale, addOrigin, offset, rotation))
		{
			if(doMesh)
			{
				md5MeshFile.Commandline().Append(va(" - %s", meshPath));
                idStr md5meshPath = R_Model_MakeOutputPath(meshPath, "." MD5_MESH_EXT, savePath);
				md5MeshFile.Write(md5meshPath.c_str());
				common->Printf("Convert md5mesh successful: %s -> %s\n", meshPath, md5meshPath.c_str());
				ret++;
			}
			else
			{
				common->Printf("Convert md5mesh successful: %s\n", meshPath);
			}
			meshRes = true;
		}
		else
			common->Warning("Convert md5mesh fail: %s", meshPath);
    }
    else
        common->Warning("Parse psk fail: %s", meshPath);

    if(!meshRes)
        return ret;

    if(!animPaths)
        return ret;

	for(int i = 0; i < animPaths->Num(); i++)
	{
		const char *animPath = (*animPaths)[i];
		idModelPsa psa;
		if(psa.Parse(animPath))
		{
			//psa.Print();
            idMd5AnimFile md5AnimFile;
            if(psa.ToMd5Anim(psk, md5AnimFile, md5MeshFile, scale, addOrigin, offset, rotation))
            {
                md5AnimFile.Commandline().Append(va(" - %s", animPath));
                idStr md5animPath = R_Model_MakeOutputPath(animPath, "." MD5_ANIM_EXT, savePath);
                md5AnimFile.Write(md5animPath.c_str());
                common->Printf("Convert md5anim successful: %s -> %s\n", animPath, md5animPath.c_str());
                ret++;
            }
            else
                common->Warning("Convert md5anim fail: %s", animPath);
		}
		else
			common->Warning("Parse psa fail: %s", animPath);
	}

	return ret;
}

ID_INLINE static int R_ConvertPsk(const char *meshPath, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	return R_ConvertPskPsaToMd5(meshPath, true, NULL, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertPsa(const char *meshPath, const idStrList &animPaths, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	return R_ConvertPskPsaToMd5(meshPath, false, &animPaths, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertPskPsa(const char *meshPath, const idStrList &animPaths, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	return R_ConvertPskPsaToMd5(meshPath, true, &animPaths, scale, addOrigin, offset, rotation, savePath);
}

static void R_ConvertPskToMd5mesh_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(psk), args.Argv(0));
        return;
    }

    idStr mesh;
    float scale = -1.0f;
    bool addOrigin = false;
    idVec3 offset(0.0f, 0.0f, 0.0f);
    idMat3 rotation = mat3_identity;
    idStr savePath;
    int res = R_Model_ParseMd5ConvertCmdLine(args, &mesh, &scale, &addOrigin, &offset, &rotation, NULL, &savePath);
    if(mesh.IsEmpty())
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(psk), args.Argv(0));
        return;
    }
    R_ConvertPsk(mesh, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertPsaToMd5anim_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf(CONVERT_TO_MD5ANIM_USAGE(psk, psa), args.Argv(0));
        return;
    }

    idStr mesh;
    float scale = -1.0f;
    bool addOrigin = false;
    idVec3 offset(0.0f, 0.0f, 0.0f);
    idMat3 rotation = mat3_identity;
    idStrList anims;
    idStr savePath;
    int res = R_Model_ParseMd5ConvertCmdLine(args, &mesh, &scale, &addOrigin, &offset, &rotation, &anims, &savePath);
    if(mesh.IsEmpty())
    {
        common->Printf(CONVERT_TO_MD5ANIM_USAGE(psk, psa), args.Argv(0));
        return;
    }
    R_ConvertPsa(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertPskPsaToMd5_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf(CONVERT_TO_MD5_USAGE(psk, psa), args.Argv(0));
        return;
    }

    idStr mesh;
    float scale = -1.0f;
    bool addOrigin = false;
    idVec3 offset(0.0f, 0.0f, 0.0f);
    idMat3 rotation = mat3_identity;
    idStrList anims;
    idStr savePath;
    int res = R_Model_ParseMd5ConvertCmdLine(args, &mesh, &scale, &addOrigin, &offset, &rotation, &anims, &savePath);
    if(mesh.IsEmpty())
    {
        common->Printf(CONVERT_TO_MD5_USAGE(psk, psa), args.Argv(0));
        return;
    }
    R_ConvertPskPsa(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

bool R_Model_HandlePskPsa(const md5ConvertDef_t &convert)
{
	if(R_ConvertPskPsa(convert.mesh, convert.anims, 
				convert.scale, 
				convert.addOrigin,
				convert.offset.IsZero() ? NULL : &convert.offset, 
				convert.rotation.IsIdentity() ? NULL : &convert.rotation,
               convert.savePath.IsEmpty() ? NULL : convert.savePath.c_str()
				) != 1 + convert.anims.Num())
	{
		common->Warning("Convert psk/psa to md5mesh/md5anim fail in entityDef '%s'", convert.def->GetName());
		return false;
	}
	return true;
}

static void ArgCompletion_Psk(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".psk"
            , NULL);
}

static void ArgCompletion_PskPsa(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".psk", ".psa"
            , NULL);
}

void R_ActorX_AddCommand(void)
{
    cmdSystem->AddCommand("pskToMd5mesh", R_ConvertPskToMd5mesh_f, CMD_FL_RENDERER, "Convert psk to md5mesh", ArgCompletion_Psk);
    cmdSystem->AddCommand("psaToMd5anim", R_ConvertPsaToMd5anim_f, CMD_FL_RENDERER, "Convert psa to md5anim", ArgCompletion_PskPsa);
    cmdSystem->AddCommand("pskPsaToMd5", R_ConvertPskPsaToMd5_f, CMD_FL_RENDERER, "Convert psk/psa to md5mesh/md5anim", ArgCompletion_PskPsa);
#ifdef _MODEL_OBJ
    cmdSystem->AddCommand("pskToObj", R_ConvertPskToObj_f, CMD_FL_RENDERER, "Convert psk to obj", ArgCompletion_Psk);
#endif
}
