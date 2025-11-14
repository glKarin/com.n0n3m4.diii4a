
#include "Model_md5anim_v6.h"

#define MD5ANIM_V6_JOINT_BOUNDSMIN "boundsMin"
#define MD5ANIM_V6_JOINT_BOUNDSMAX "boundsMax"

#define MD5ANIM_V6_ATTR_X "x"
#define MD5ANIM_V6_ATTR_Y "y"
#define MD5ANIM_V6_ATTR_Z "z"
#define MD5ANIM_V6_ATTR_PITCH "pitch"
#define MD5ANIM_V6_ATTR_YAW "yaw"
#define MD5ANIM_V6_ATTR_ROLL "roll"

idModelMd5animV6::idModelMd5animV6(void)
        : version(MD5_V6_VERSION)
{}

bool idModelMd5animV6::Parse(const char *path)
{
    idLexer	parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT);
    idToken	token;
    int		i, j;
    int		numKey;
    int		numChannels;
    md5animV6Channel_t *channel;

    Clear();

    if (!parser.LoadFile(path)) {
        return false;
    }

    parser.ExpectTokenString(MD5_VERSION_STRING);
    version = parser.ParseInt();
    if(version != MD5_V6_VERSION)
    {
        common->Warning("md5naim version is not 2002 E3 demo: %d != %d", version, MD5_V6_VERSION);
        version = MD5_V6_VERSION;
        return false;
    }

    // skip the commandline
    parser.ExpectTokenString("commandline");
    parser.ReadToken(&token);

    // parse num channels
    parser.ExpectTokenString("numchannels");
    numChannels = parser.ParseInt();

    // parse the channels
    channels.SetGranularity(1);
    channels.SetNum(numChannels);

    for (i = 0, channel = &channels[0]; i < numChannels; i++, channel++) {
        parser.ExpectTokenString("channel");
        channel->index = parser.ParseInt();
        parser.ExpectTokenString("{");

        // parse joint
        parser.ExpectTokenString("joint");
        parser.ReadToken(&token);
        channel->joint = token;

        // parse attribute
        parser.ExpectTokenString("attribute");
        parser.ReadToken(&token);
        channel->attribute = token;

        // parse start time
        parser.ExpectTokenString("starttime");
        channel->starttime = parser.ParseFloat();

        // parse end time
        parser.ExpectTokenString("endtime");
        channel->endtime = parser.ParseFloat();

        // parse framerate
        parser.ExpectTokenString("framerate");
        channel->framerate = parser.ParseFloat();

        // parse strings
        parser.ExpectTokenString("strings");
        channel->strings = parser.ParseFloat();

        // parse range
        parser.ExpectTokenString("range");
        channel->range[0] = parser.ParseInt();
        channel->range[1] = parser.ParseInt();

        // parse keys
        parser.ExpectTokenString("keys");
        numKey = parser.ParseInt();
        channel->keys.SetNum(numKey);
        for(j = 0; j < numKey; j++)
        {
            channel->keys[j] = parser.ParseFloat();
        }

        parser.ExpectTokenString("}");
    }

    return true;
}

void idModelMd5animV6::Clear(void)
{
    version = MD5_V6_VERSION;
    commandline.Clear();
    channels.Clear();
}

ID_INLINE static float md5animV6Channel_GetValue(const md5animV6Channel_t *ch, int index, float def = 0.0f)
{
    if(!ch)
        return def;

    if(index < ch->range[0])
        return ch->keys[0];
    else if(index >= ch->range[1])
        return ch->keys[ch->keys.Num() - 1];
	index -= ch->range[0];
    if(index >= ch->keys.Num())
        return ch->keys[ch->keys.Num() - 1];
    else
        return ch->keys[index];
}

typedef struct md5animV6ChannelKey_s {
    const md5animV6Channel_t *x, *y, *z;
    const md5animV6Channel_t *pitch, *yaw, *roll;

    float X(int index, float def = 0.0f) const {
        return md5animV6Channel_GetValue(x, index, def);
    }
    float Y(int index, float def = 0.0f) const {
        return md5animV6Channel_GetValue(y, index, def);
    }
    float Z(int index, float def = 0.0f) const {
        return md5animV6Channel_GetValue(z, index, def);
    }
    float Pitch(int index, float def = 0.0f) const {
        return md5animV6Channel_GetValue(pitch, index, def);
    }
    float Yaw(int index, float def = 0.0f) const {
        return md5animV6Channel_GetValue(yaw, index, def);
    }
    float Roll(int index, float def = 0.0f) const {
        return md5animV6Channel_GetValue(roll, index, def);
    }

	void Set(const md5animV6Channel_t *channel) {
        if(!idStr::Icmp(channel->attribute, MD5ANIM_V6_ATTR_X))
            x = channel;
        else if(!idStr::Icmp(channel->attribute, MD5ANIM_V6_ATTR_Y))
            y = channel;
        else if(!idStr::Icmp(channel->attribute, MD5ANIM_V6_ATTR_Z))
            z = channel;
        else if(!idStr::Icmp(channel->attribute, MD5ANIM_V6_ATTR_PITCH))
            pitch = channel;
        else if(!idStr::Icmp(channel->attribute, MD5ANIM_V6_ATTR_YAW))
            yaw = channel;
        else if(!idStr::Icmp(channel->attribute, MD5ANIM_V6_ATTR_ROLL))
            roll = channel;
        else
            common->Warning("Unknown attribute '%s' in channel", channel->attribute.c_str());
	}
} md5animV6ChannelKey_t;

bool idModelMd5animV6::ToMd5Anim(const idModelMd5meshV6 &meshv6, idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *animOffset, const idMat3 *animRotation) const
{
    int i, j;
    md5animHierarchy_t *md5Hierarchy;
    const md5meshJoint_t *meshJoint;
    const md5animV6Channel_t *channel;
    md5animFrame_t *md5Frame;
    md5meshJointTransform_t *jointTransform;
    md5meshJointTransform_t *frameTransform;
    md5animBaseframe_t *md5BaseFrame;
    md5meshJoint_t *md5Bone;
    md5animV6ChannelKey_t *rkey, *rkey2;

    const idList<md5meshJoint_t> joints = md5mesh.Joints();
    const int numBones = joints.Num();

    idHashTable<md5animV6ChannelKey_t> keyTable;
    for(i = 0, channel = &channels[0]; i < channels.Num(); i++, channel++)
    {
        if(!keyTable.Get(channel->joint, &rkey))
        {
            md5animV6ChannelKey_t key;
            key.x = key.y = key.z = key.pitch = key.yaw = key.roll = NULL;
            keyTable.Set(channel->joint, key);
            keyTable.Get(channel->joint, &rkey);
        }

		rkey->Set(channel);
    }

    keyTable.Get(MD5ANIM_V6_JOINT_BOUNDSMIN, &rkey);

    const int numFrames = rkey->x->range[1] - 1;
    md5anim.FrameRate() = (int)rkey->x->framerate;
    md5anim.NumAnimatedComponents() = numBones * 6;

    md5anim.Commandline() = va("Convert from md5anim v6(2002 E3 demo) file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
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
    keyTable.Get(MD5ANIM_V6_JOINT_BOUNDSMAX, &rkey2);
    idList<md5animBounds_t> &md5Bounds = md5anim.Bounds();
    md5Bounds.SetNum(numFrames);
    for(i = 0; i < md5Bounds.Num(); i++)
    {
        // md5Bounds[i].Zero();

        md5animBounds_t &bounds = md5Bounds[i];
        bounds[0].Set(rkey->X(i), rkey->Y(i), rkey->Z(i));
        bounds[1].Set(rkey2->X(i, bounds[0].x), rkey2->Y(i, bounds[0].y), rkey2->Z(i, bounds[0].z));
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
    md5Frames.SetNum(numFrames);
    int base;
    for(i = 0, base = 0; i < numFrames; i++)
    {
        md5animFrames_t &frames = md5Frames[i];
        frames.index = i;
        frames.joints.SetNum(numBones);

        idList<md5meshJoint_t> md5Joints = joints;

        for(j = 0; j < numBones; j++)
        {
            if(j == 0 && addOrigin)
                continue;

            meshJoint = &joints[j];

            idVec3 boneOrigin;
            idQuat boneQuat;
            md5Bone = &md5Joints[j];

            md5Bone->boneName = meshJoint->boneName;
            md5Bone->parentIndex = meshJoint->parentIndex;

            if(keyTable.Get(meshJoint->boneName, &rkey))
            {
                boneOrigin[0] = rkey->X(i);
                boneOrigin[1] = rkey->Y(i);
                boneOrigin[2] = rkey->Z(i);

                idAngles angles(rkey->Pitch(i), rkey->Yaw(i), rkey->Roll(i));
                boneQuat = angles.ToQuat();
            }
            else
            {
                if(i == 0)
                    common->Warning("Bone not found in psa: %s", meshJoint->boneName.c_str());
				boneOrigin.Set(0.0f, 0.0f, 0.0f);
				boneQuat[0] = boneQuat[1] = boneQuat[2] = 0.0f;
				boneQuat[3] = 1.0f;
            }
            boneQuat = boneQuat.Inverse();

            int rootIndex = addOrigin ? 0 : -1;
            if(md5Bone->parentIndex == rootIndex)
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
            md5Bone->orient = boneQuat.Inverse();

            if (md5Bone->parentIndex >= 0)
            {
                idVec3 rotated;
                idQuat quat;

                md5meshJoint_t *parent = &md5Joints[md5Bone->parentIndex];

                idMat3 m = parent->orient.ToMat3();
                rotated = m * md5Bone->pos;

                quat = md5Bone->orient * parent->orient;
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

void idModelMd5animV6::Print(void) const
{
#define MODEL_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
	if(all) \
    for(int i = 0; i < list.Num(); i++) {  \
         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    MODEL_PART_PRINT(channel, channels, true, "joint=%s, attribute=%s, keys=%d   ", channels[i].joint.c_str(), channels[i].attribute.c_str(), channels[i].keys.Num())

#undef MODEL_PART_PRINT
}

static int R_ConvertMd5V6ToV10(const char *meshPath, bool doMesh = true, const idStrList *animPaths = NULL, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    int ret = 0;

    idModelMd5meshV6 md5meshv6;
    idMd5MeshFile md5MeshFile;
    bool meshRes = false;
    if(md5meshv6.Parse(meshPath))
    {
        //md5meshv6.Print();
        if(md5meshv6.ToMd5Mesh(md5MeshFile, scale, addOrigin, offset, rotation))
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
        common->Warning("Parse md5mesh v6 fail: %s", meshPath);

    if(!meshRes)
        return ret;

    if(!animPaths)
        return ret;

    for(int i = 0; i < animPaths->Num(); i++)
    {
        const char *animPath = (*animPaths)[i];
        idModelMd5animV6 md5animv6;
        if(md5animv6.Parse(animPath))
        {
            //psa.Print();
            idMd5AnimFile md5AnimFile;
            if(md5animv6.ToMd5Anim(md5meshv6, md5AnimFile, md5MeshFile, scale, addOrigin, offset, rotation))
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
            common->Warning("Parse md5anim v6 fail: %s", animPath);
    }

    return ret;
}

ID_INLINE static int R_ConvertMd5meshV6(const char *meshPath, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertMd5V6ToV10(meshPath, true, NULL, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertMd5animV6(const char *meshPath, const idStrList &animPaths, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertMd5V6ToV10(meshPath, false, &animPaths, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertMd5V6(const char *meshPath, const idStrList &animPaths, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertMd5V6ToV10(meshPath, true, &animPaths, scale, addOrigin, offset, rotation, savePath);
}

static void R_ConvertMd5meshV6ToV10_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(v6 md5mesh), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5MESH_USAGE(v6 md5mesh), args.Argv(0));
        return;
    }
    R_ConvertMd5meshV6(mesh, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertMd5animV6ToV10_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf(CONVERT_TO_MD5ANIM_USAGE(v6 md5mesh, v6 md5anim), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5ANIM_USAGE(v6 md5mesh, v6 md5anim), args.Argv(0));
        return;
    }
    R_ConvertMd5animV6(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertMd5V6ToV10_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf(CONVERT_TO_MD5_USAGE(v6 md5mesh, v6 md5anim), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5_USAGE(v6 md5mesh, v6 md5anim), args.Argv(0));
        return;
    }
    R_ConvertMd5V6(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

bool R_Model_HandleMd5V6(const md5ConvertDef_t &convert)
{
    if(R_ConvertMd5V6(convert.mesh, convert.anims,
                       convert.scale,
                       convert.addOrigin,
                       convert.offset.IsZero() ? NULL : &convert.offset,
                       convert.rotation.IsIdentity() ? NULL : &convert.rotation,
                       convert.savePath.IsEmpty() ? NULL : convert.savePath.c_str()
    ) != 1 + convert.anims.Num())
    {
        common->Warning("Convert md5mesh/md5anim v6 to md5mesh/md5anim v10 fail in entityDef '%s'", convert.def->GetName());
        return false;
    }
    return true;
}

static void ArgCompletion_Md5mesh(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, "." MD5_MESH_EXT
            , NULL);
}

static void ArgCompletion_Md5(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, "." MD5_MESH_EXT, "." MD5_ANIM_EXT
            , NULL);
}

void R_Md5v6_AddCommand(void)
{
    cmdSystem->AddCommand("md5meshV6ToV10", R_ConvertMd5meshV6ToV10_f, CMD_FL_RENDERER, "Convert md5mesh v6(2002 E3 demo version) to v10(2004 release version)", ArgCompletion_Md5mesh);
    cmdSystem->AddCommand("md5animV6ToV10", R_ConvertMd5animV6ToV10_f, CMD_FL_RENDERER, "Convert md5anim v6(2002 E3 demo version) to v10(2004 release version)", ArgCompletion_Md5);
    cmdSystem->AddCommand("md5V6ToV10", R_ConvertMd5V6ToV10_f, CMD_FL_RENDERER, "Convert md5mesh/md5anim v6(2002 E3 demo version) to v10(2004 release version)", ArgCompletion_Md5);
}
