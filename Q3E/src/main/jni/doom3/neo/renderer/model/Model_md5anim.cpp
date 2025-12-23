
#include "Model_md5mesh.h"
#include "Model_md5anim.h"

/* md5anim */
idMd5AnimFile::idMd5AnimFile(void)
        : version(MD5_VERSION)
{}

void idMd5AnimFile::Clear(void)
{
    version = MD5_VERSION;
    numAnimatedComponents = 0;
    commandline.Clear();
    hierarchies.SetNum(0);
    bounds.SetNum(0);
    baseframe.SetNum(0);
    frames.SetNum(0);
}

void idMd5AnimFile::Write(const char *path) const
{
    idFile *file = fileSystem->OpenFileWrite(path);

    // header
    Writefln(file, "MD5Version %d", version);
    Writefln(file, "commandline \"%s\"", commandline.c_str());
    Writeln(file);
    Writefln(file, "numFrames %d", frames.Num());
    Writefln(file, "numJoints %d", hierarchies.Num());
    Writefln(file, "frameRate %d", frameRate);
    Writefln(file, "numAnimatedComponents %d", numAnimatedComponents);
    Writeln(file);

    // hierarchy
    Writefln(file, "hierarchy {", hierarchies.Num());
    for(int i = 0; i < hierarchies.Num(); i++)
    {
        WriteIndent(file);
        const md5animHierarchy_t &joint = hierarchies[i];
        Writef(file, "\"%s\" %d %d %d", joint.boneName.c_str(), joint.parentIndex, joint.numComp, joint.frameIndex);
#if MD5_APPEND_COMMENT
        if(joint.parentIndex != -1)
        {
            const md5animHierarchy_t &parent = hierarchies[joint.parentIndex];
            Writef(file, " // %d %s", i, parent.boneName.c_str());
        }
#endif
        Writeln(file);
    }
    Writefln(file, "}");
    Writeln(file);

    // bounds
    Writefln(file, "bounds {", bounds.Num());
    for(int i = 0; i < bounds.Num(); i++)
    {
        WriteIndent(file);
        const md5animBounds_t &bound = bounds[i];
        Writef(file, "( %.10f %.10f %.10f ) ( %.10f %.10f %.10f )", bound[0].x, bound[0].y, bound[0].z, bound[1].x, bound[1].y, bound[1].z);
#if MD5_APPEND_COMMENT
        Writef(file, " // %d", i);
#endif
        Writeln(file);
    }
    Writefln(file, "}");
    Writeln(file);

    // baseframe
    Writefln(file, "baseframe {", baseframe.Num());
    for(int i = 0; i < baseframe.Num(); i++)
    {
        WriteIndent(file);
        const md5animBaseframe_t &frame = baseframe[i];
        Writef(file, "( %.10f %.10f %.10f ) ( %.10f %.10f %.10f )", frame.xPos, frame.yPos, frame.zPos, frame.xOrient, frame.yOrient, frame.zOrient);
#if MD5_APPEND_COMMENT
            const md5animHierarchy_t &bone = hierarchies[i];
            Writef(file, " // %d %s", i, bone.boneName.c_str());
#endif
        Writeln(file);
    }
    Writefln(file, "}");
    Writeln(file);

    // frame
    for(int m = 0; m < frames.Num(); m++)
    {
        const md5animFrames_t &frameTrans = frames[m];
        Writefln(file, "frame %d {", m);
        for(int i = 0; i < frameTrans.joints.Num(); i++)
        {
            WriteIndent(file);
            const md5animFrame_t &frame = frameTrans.joints[i];
            Writef(file, "%.10f %.10f %.10f %.10f %.10f %.10f", frame.xPos, frame.yPos, frame.zPos, frame.xOrient, frame.yOrient, frame.zOrient);
#if MD5_APPEND_COMMENT
                const md5animHierarchy_t &bone = hierarchies[i];
                Writef(file, " // %d %s", i, bone.boneName.c_str());
#endif
            Writeln(file);
        }
        Writefln(file, "}");
        Writeln(file);
    }

    fileSystem->CloseFile(file);
}

bool idMd5AnimFile::Parse(const char *path)
{
    idLexer	parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT);
    idToken	token;
    int		i, j;
    int		num;
    int		numFrames;
    int		numJoints;
    md5animHierarchy_t *jointInfo;
    md5animBounds_t *bound;
    md5animBaseframe_t *baseFrame;
    md5animFrames_t *frame;
    md5animFrame_t *f;
    idList<float> componentFrames;
    float *componentPtr;

    if (!parser.LoadFile(path)) {
        return false;
    }

    parser.ExpectTokenString(MD5_VERSION_STRING);
    version = parser.ParseInt();

    // skip the commandline
    parser.ExpectTokenString("commandline");
    parser.ReadToken(&token);

    // parse num frames
    parser.ExpectTokenString("numFrames");
    numFrames = parser.ParseInt();

    // parse num joints
    parser.ExpectTokenString("numJoints");
    numJoints = parser.ParseInt();

    // parse frame rate
    parser.ExpectTokenString("frameRate");
    frameRate = parser.ParseInt();

    // parse number of animated components
    parser.ExpectTokenString("numAnimatedComponents");
    numAnimatedComponents = parser.ParseInt();

    // parse the hierarchy
    hierarchies.SetGranularity(1);
    hierarchies.SetNum(numJoints);
    parser.ExpectTokenString("hierarchy");
    parser.ExpectTokenString("{");

    for (i = 0, jointInfo = &hierarchies[0]; i < numJoints; i++, jointInfo++) {
        parser.ReadToken(&token);
        jointInfo->boneName = token;

        // parse parent num
        jointInfo->parentIndex = parser.ParseInt();

        // parse anim bits
        jointInfo->numComp = parser.ParseInt();

        // parse first component
        jointInfo->frameIndex = parser.ParseInt();
    }

    parser.ExpectTokenString("}");

    // parse bounds
    parser.ExpectTokenString("bounds");
    parser.ExpectTokenString("{");
    bounds.SetGranularity(1);
    bounds.SetNum(numFrames);

    for (i = 0, bound = &bounds[0]; i < numFrames; i++, bound++) {
        parser.Parse1DMatrix(3, (*bound)[ 0 ].ToFloatPtr());
        parser.Parse1DMatrix(3, (*bound)[ 1 ].ToFloatPtr());
    }

    parser.ExpectTokenString("}");

    // parse base frame
    baseframe.SetGranularity(1);
    baseframe.SetNum(numJoints);
    parser.ExpectTokenString("baseframe");
    parser.ExpectTokenString("{");

    for (i = 0, baseFrame = &baseframe[0]; i < numJoints; i++, baseFrame++) {
        idCQuat q;
        idVec3 pos;
        parser.Parse1DMatrix(3, pos.ToFloatPtr());
        parser.Parse1DMatrix(3, q.ToFloatPtr());  //baseFrame[ i ].q.ToFloatPtr() );
        //idCQuat q2 = q.ToQuat();//.w = baseFrame[ i ].q.CalcW();
        baseFrame->xPos = pos.x;
        baseFrame->yPos = pos.y;
        baseFrame->zPos = pos.z;
        baseFrame->xOrient = q.x;
        baseFrame->yOrient = q.y;
        baseFrame->zOrient = q.z;
    }

    parser.ExpectTokenString("}");

    // parse frames
    frames.SetGranularity(1);
    frames.SetNum(numFrames);
    for (i = 0, frame = &frames[0]; i < numFrames; i++, frame++) {
        parser.ExpectTokenString("frame");
        frame->index = parser.ParseInt();
        frame->joints.SetGranularity(1);
        frame->joints.SetNum(numJoints);

        parser.ExpectTokenString("{");

        componentFrames.SetGranularity(1);
        componentFrames.SetNum(numAnimatedComponents);

        for (j = 0, componentPtr = &componentFrames[0]; j < numAnimatedComponents; j++, componentPtr++) {
            *componentPtr = parser.ParseFloat();
        }

        for(j = 0, f = &frame->joints[0], jointInfo = &hierarchies[0]; j < numJoints; j++, f++, jointInfo++)
        {
            componentPtr = &componentFrames[jointInfo->frameIndex];
            f->xPos = f->yPos = f->zPos = 0.0f;
            f->xOrient = f->yOrient = f->zOrient = 0.0f;
            num = 0;
            if(jointInfo->numComp & MD5ANIM_TX)
                f->xPos = componentPtr[num++];
            if(jointInfo->numComp & MD5ANIM_TY)
                f->yPos = componentPtr[num++];
            if(jointInfo->numComp & MD5ANIM_TZ)
                f->zPos = componentPtr[num++];
            if(jointInfo->numComp & MD5ANIM_QX)
                f->xOrient = componentPtr[num++];
            if(jointInfo->numComp & MD5ANIM_QY)
                f->yOrient = componentPtr[num++];
            if(jointInfo->numComp & MD5ANIM_QZ)
                f->zOrient = componentPtr[num++];
        }

        parser.ExpectTokenString("}");
    }

    return true;
}

void idMd5AnimFile::CalcFrameBounds(const idMd5MeshFile &md5mesh, const idMd5AnimFile &md5anim, int frameIndex, idBounds &md5Bounds)
{
    int j;
    const md5animFrames_t &frame = md5anim.frames[frameIndex];
    const md5meshJoint_t *md5Bone;
    const md5animFrame_t *f;
    md5meshJointTransform_t *jointTransform;

    idList<md5meshJointTransform_t> frameTransforms;
    frameTransforms.SetNum(md5mesh.joints.Num());

    for(j = 0; j < md5mesh.joints.Num(); j++)
    {
        f = &frame.joints[j];
        md5Bone = &md5mesh.joints[j];
        jointTransform = &frameTransforms[j];
        jointTransform->index = j;
        jointTransform->parentIndex = md5Bone->parentIndex;
        jointTransform->parent = NULL;

        jointTransform->t.Set(f->xPos, f->yPos, f->zPos);
        jointTransform->q = idCQuat(f->xOrient, f->yOrient, f->zOrient).ToMat3();

        idVec3 jointpos = jointTransform->t;
        idMat3 jointaxis = jointTransform->q;

        if(md5Bone->parentIndex >= 0)
        {
            md5meshJointTransform_t *parentJointTransform = &frameTransforms[md5Bone->parentIndex];

            jointTransform->idwm = jointaxis * parentJointTransform->idwm;
            jointTransform->idt = parentJointTransform->idt + jointpos * parentJointTransform->idwm;
        }
        else
        {
            jointTransform->idwm = jointaxis;
            jointTransform->idt = jointpos;
        }
    }

    // calc frame bounds
    md5mesh.CalcBounds(frameTransforms, md5Bounds);
}

void idMd5AnimFile::CalcBounds(const idMd5MeshFile &mesh)
{
    for(int i = 0; i < frames.Num(); i++)
    {
        CalcFrameBounds(mesh, *this, i, bounds[i]);
    }
}
