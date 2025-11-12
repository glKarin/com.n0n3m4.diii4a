#include "Model_smd.h"
#include "Model_md5mesh.h"
#include "Model_md5anim.h"

#define SMD_VERSION_1 1

idModelSmd::idModelSmd(void)
        : version(SMD_VERSION_1),
          lexer(NULL),
          types(0)
{}

void idModelSmd::Clear(void)
{
    nodes.SetNum(0);
    triangles.SetNum(0);
    frames.SetNum(0);
    if(lexer)
    {
        lexer = NULL;
    }
    types = 0;
    version = SMD_VERSION_1;
}

bool idModelSmd::Parse(const char *filePath)
{
    Clear();

    idLexer parser;
    if(!parser.LoadFile(filePath))
    {
        common->Warning("Load smd file fail: %s", filePath);
        return false;
    }

    idToken token;

    parser.ExpectTokenString("version");
    int ver = parser.ParseInt();

    if(ver != SMD_VERSION_1)
    {
        parser.Error("Unsupported smd version: %d\n", ver);
        return false;
    }

    version = ver;

    lexer = &parser;
    bool err = false;
    while(true)
    {
        if(!parser.ReadToken(&token))
            break;

        if(!idStr::Icmp("nodes", token))
        {
            MarkType(SMD_NODE);
            if(ReadNodes() < 0)
            {
                err = true;
                break;
            }
        }
        else if(!idStr::Icmp("skeleton", token))
        {
            MarkType(SMD_SKELETON);
			if(nodes.Num() > 0)
			{
				if(ReadFrames() < 0)
                {
                    err = true;
                    break;
                }
			}
			else
			{
				parser.Warning("Unread nodes when parsing frames");
				Skip();
			}
        }
        else if(!idStr::Icmp("triangles", token))
        {
            MarkType(SMD_TRIANGLE);
            if(ReadTriangles() < 0)
            {
                err = true;
                break;
            }
        }
        else if(!idStr::Icmp("vertexanimation", token))
        {
            Skip();
        }
        else
        {
            parser.Warning("Unexpect token '%s'", token.c_str());
            Skip();
        }
    }

    if(err)
        Clear();
    else
    {
        lexer = NULL;
    }

    return !err;
}

int idModelSmd::ReadNodes(void)
{
    idToken token;

    while(true)
    {
        if(!lexer->ReadToken(&token))
        {
            lexer->Error("Read EOF when parsing nodes\n");
            return -1;
        }

        if(!idStr::Icmp("end", token))
            break;

        int i = nodes.Append(smdNode_t());
        smdNode_t &item = nodes[i];

		lexer->UnreadToken(&token);

        item.index = lexer->ParseInt();
        lexer->ReadToken(&token);
        item.name = token.c_str();
        item.parent_index = lexer->ParseInt();

        lexer->SkipRestOfLine();
    }

    return nodes.Num();
}

void idModelSmd::ReadSkeletons(idList<smdSkeleton_t> &skel)
{
	skel.SetNum(nodes.Num());
	
	for(int i = 0; i < skel.Num(); i++)
	{
		smdSkeleton_t &sk = skel[i];

		sk.bone = lexer->ParseInt();

		sk.pos[0] = lexer->ParseFloat();
		sk.pos[1] = lexer->ParseFloat();
		sk.pos[2] = lexer->ParseFloat();

		sk.rot[0] = lexer->ParseFloat();
		sk.rot[1] = lexer->ParseFloat();
		sk.rot[2] = lexer->ParseFloat();
	}
}

int idModelSmd::ReadFrames(void)
{
    idToken token;

    while(true)
    {
        if(!lexer->ReadToken(&token))
        {
            lexer->Error("Read EOF when parsing frames\n");
            return -1;
        }

        if(!idStr::Icmp("end", token))
            break;

		if(idStr::Icmp("time", token))
		{
            lexer->Error("Expect token 'time', but found '%s'\n", token.c_str());
            return -1;
		}

        int i = frames.Append(smdFrame_t());
        smdFrame_t &item = frames[i];

        item.time = lexer->ParseInt();

		ReadSkeletons(item.skeletons);
    }

    return frames.Num();
}

int idModelSmd::ReadTriangles(void)
{
    idToken token;

    while(true)
    {
        if(!lexer->ReadToken(&token))
        {
            lexer->Error("Read EOF when parsing triangles\n");
            return -1;
        }

        if(!idStr::Icmp("end", token))
            break;

        int i = triangles.Append(smdTriangle_t());
        smdTriangle_t &item = triangles[i];

        item.material = token.c_str();
        while(lexer->ReadTokenOnLine(&token))
            item.material.Append(token.c_str());

		// CW -> CCW
        for(int m = 2; m >= 0; m--)
        {
            ReadVertex(item.vertexes[m]);
        }
    }

    return triangles.Num();
}

void idModelSmd::ReadVertex(smdVertex_t &vert)
{
    vert.parent = lexer->ParseInt();

    vert.pos[0] = lexer->ParseFloat();
    vert.pos[1] = lexer->ParseFloat();
    vert.pos[2] = lexer->ParseFloat();

    vert.norm[0] = lexer->ParseFloat();
    vert.norm[1] = lexer->ParseFloat();
    vert.norm[2] = lexer->ParseFloat();

    vert.tc[0] = lexer->ParseFloat();
    vert.tc[1] = 1.0f - lexer->ParseFloat();

	vert.numlinks = 0;
	vert.links.SetNum(0);
	int linenum = lexer->GetLineNum();

	idStr str;
    lexer->ReadRestOfLine(str);
	idStr::StripWhitespace(str);
	if(str.IsEmpty())
		return;

	idLexer src;
	src.LoadMemory(str, str.Length(), va("<smd bone weight line %d>", linenum));

	vert.numlinks = src.ParseInt();
	vert.links.SetNum(vert.numlinks);

	for(int i = 0; i < vert.numlinks; i++)
	{
		vert.links[i].bone = src.ParseInt();
		vert.links[i].weight = src.ParseFloat();
	}
}

void idModelSmd::Skip(void)
{
    lexer->SkipUntilString("end");
}

void idModelSmd::MarkType(int type)
{
    types |= (1 << type);
}

bool idModelSmd::IsTypeMarked(int type) const
{
    return types & (1 << type);
}

int idModelSmd::GroupTriangle(idList<idList<const smdTriangle_t *> > &faceGroup, idStrList &matList) const
{
    faceGroup.Clear();
    matList.Clear();

    for(int i = 0; i < triangles.Num(); i++)
    {
        const smdTriangle_t *tri = &triangles[i];

        idList<const smdTriangle_t *> *group = NULL;
        for(int j = 0; j < matList.Num(); j++)
        {
            if(matList[j] == tri->material)
            {
                group = &faceGroup[j];
                break;
            }
        }
        if(NULL == group)
        {
            int index = matList.Append(tri->material);
            faceGroup.Append(idList<const smdTriangle_t *>());
            group = &faceGroup[index];
        }

        group->Append(tri);
    }

    return matList.Num();
}

const idList<smdSkeleton_t> & idModelSmd::Bones(void) const
{
	return frames[0].skeletons;
}

bool idModelSmd::HasSkeleton(void) const
{
    return frames.Num() > 0 && frames[0].skeletons.Num() > 0;
}

bool idModelSmd::IsMeshFile(void) const
{
    return triangles.Num() > 0;
}

static idQuat fromangles(const idVec3 &rot)
{
	double cx = cos(rot.x/2), sx = sin(rot.x/2),
		   cy = cos(rot.y/2), sy = sin(rot.y/2),
		   cz = cos(rot.z/2), sz = sin(rot.z/2);
	idQuat q(sx*cy*cz - cx*sy*sz,
			cx*sy*cz + sx*cy*sz,
			cx*cy*sz - sx*sy*cz,
			cx*cy*cz + sx*sy*sz);
	if(q.w > 0) q = -q;
	return q;
}

// don't compare norm
static bool smdVertex_Equals(const smdVertex_t&a, const smdVertex_t &b)
{
    bool ok = a.parent == b.parent
        && a.pos[0] == b.pos[0] && a.pos[1] == b.pos[1] && a.pos[2] == b.pos[2]
        && a.tc[0] == b.tc[0] && a.tc[1] == b.tc[1]
        && a.numlinks == b.numlinks
        ;
    if(!ok)
        return false;
    for(int i = 0; i < a.numlinks; i++)
    {
        if(a.links[i].bone != b.links[i].bone)
            return false;
        if(a.links[i].weight != b.links[i].weight)
            return false;
    }
    return true;
}

bool idModelSmd::ToMd5Mesh(idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *meshOffset, const idMat3 *meshRotation) const
{
	if(!HasSkeleton())
    {
        common->Warning("Invalid skeleton in smd");
        return false;
    }
    if(!IsMeshFile())
    {
        common->Warning("No triangles in smd");
        return false;
    }

    int i, j;
    md5meshJoint_t *md5Bone;
    const smdSkeleton_t *refBone;
    const smdNode_t *refNode;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshJointTransform_t *jointTransform;
	const idList<smdSkeleton_t> &bones = Bones();
	int numBones = bones.Num();

    md5mesh.Commandline() = va("Convert from source smd file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
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

	bool hasOrigin = false;
    for (i = 0, refBone = &bones[0], refNode = &nodes[0]; i < bones.Num(); i++, md5Bone++, refBone++, refNode++)
    {
        md5Bone->boneName = refNode->name;
		if(refNode->parent_index == -1 && !addOrigin)
		{
			if(hasOrigin)
			{
				common->Warning("Has more root bone: %d", i);
				md5Bone->parentIndex = 0;
			}
			else
			{
				md5Bone->parentIndex = -1;
				hasOrigin = true;
			}
		}
		else
		{
			md5Bone->parentIndex = refNode->parent_index;
		}

		if(addOrigin)
			md5Bone->parentIndex += 1;

        boneOrigin[0] = refBone->pos[0];
        boneOrigin[1] = refBone->pos[1];
        boneOrigin[2] = refBone->pos[2];

		idVec3 rot;
		rot[0] = refBone->rot[0];
		rot[1] = refBone->rot[1];
		rot[2] = refBone->rot[2];
		boneQuat = fromangles(rot);

		if (refNode->parent_index < 0)
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

            md5meshJoint_t *parent;

            parent = &md5Bones[md5Bone->parentIndex];

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

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    // convert md5 mesh
    idList<md5meshMesh_t> &md5Meshes = md5mesh.Meshes();

    idList<idList<const smdTriangle_t *> > triGroup;
    idStrList matList;

    int num = GroupTriangle(triGroup, matList);
    md5Meshes.SetNum(num);

    for(j = 0; j < num; j++)
    {
        const idList<const smdTriangle_t *> &triList = triGroup[j];
        md5meshMesh_t &mesh = md5Meshes[j];
        mesh.shader = matList[j];

        idList<smdVertex_t> md5Vertexes; // cache saved vert for remove dup vert to smoothing normals
        for(i = 0; i < triList.Num(); i++)
        {
            const smdTriangle_t *face = triList[i];

            int md5VertIndexes[3];
            for(int k = 0; k < 3; k++)
            {
                const smdVertex_t *vertex = &face->vertexes[k];
                // find vert is cached
                int n;
                for(n = 0; n < md5Vertexes.Num(); n++)
                {
                    if(smdVertex_Equals(*vertex, md5Vertexes[n]))
                        break;
                }
                if(n < md5Vertexes.Num())
                {
                    md5VertIndexes[k] = n;
                    continue;
                }

				idVec3 pos(vertex->pos[0], vertex->pos[1], vertex->pos[2]);
				if(meshRotation && !meshRotation->IsIdentity())
					pos *= *meshRotation;
				if(meshOffset && !meshOffset->IsZero())
					pos += *meshOffset;
				if(scale > 0.0f)
					pos *= scale;

                md5meshVert_t md5Vert;
                md5Vert.uv.Set(vertex->tc[0], vertex->tc[1]);
                md5Vert.weightIndex = mesh.weights.Num();
				if(vertex->numlinks == 0)
                {
                    common->Warning("vertex weight num is zero in triangle");
#if 0
                    return false;
#endif
					md5Vert.weightElem = 1;
                    md5meshWeight_t md5Weight;
                    md5Weight.jointIndex = vertex->parent;
					if(addOrigin)
						md5Weight.jointIndex += 1;
                    jointTransform = &jointTransforms[md5Weight.jointIndex];
					md5Weight.weightValue = 1.0f;

                    jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight.pos);

                    mesh.weights.Append(md5Weight); // Add weight
                }
				else
				{
					const idList<smdLink_t> &vertWeights = vertex->links;
					md5Vert.weightElem = vertWeights.Num();
					float w = 0.0f;
					for(int m = 0; m < vertWeights.Num(); m++)
					{
						const smdLink_t *weight = &vertWeights[m];
						md5meshWeight_t md5Weight;
						md5Weight.jointIndex = weight->bone;
						if(addOrigin)
							md5Weight.jointIndex += 1;
						jointTransform = &jointTransforms[md5Weight.jointIndex];
						if(vertWeights.Num() == 1)
						{
							if(weight->weight != 1.0f)
								common->Warning("vertex only 1 bone '%s' but weight is not 1 '%f'", nodes[weight->bone].name.c_str(), weight->weight);
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
						common->Warning("weight sum is less than 1.0: %f", w);
					}
				}
                md5VertIndexes[k] = mesh.verts.Append(md5Vert); // Add vert
                md5Vertexes.Append(*vertex); // cache vert
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

bool idModelSmd::ToMd5Anim(const idModelSmd &smd, idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *animOffset, const idMat3 *animRotation) const
{
    if(!HasSkeleton())
    {
        common->Warning("No frames in smd");
        return false;
    }

    int i, j;
    md5animHierarchy_t *md5Hierarchy;
    const md5meshJoint_t *meshJoint;
    const smdSkeleton_t *key;
    md5animFrame_t *md5Frame;
    md5meshJointTransform_t *jointTransform;
    md5meshJointTransform_t *frameTransform;
    md5animBaseframe_t *md5BaseFrame;
    md5meshJoint_t *md5Bone;
	const idList<smdSkeleton_t> &bones = smd.Bones();

    const idList<md5meshJoint_t> joints = md5mesh.Joints();
    const int numBones = joints.Num();

    md5anim.FrameRate() = 24;
    md5anim.NumAnimatedComponents() = numBones * 6;

    md5anim.Commandline() = va("Convert from source smd file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
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
    md5Bounds.SetNum(frames.Num());
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
    md5Frames.SetNum(frames.Num());
    for(i = 0; i < frames.Num(); i++)
    {
        md5animFrames_t &_frames = md5Frames[i];
        _frames.index = i;
        _frames.joints.SetNum(numBones);
		const idList<smdSkeleton_t> &animKeys = frames[i].skeletons;

        idList<md5meshJoint_t> md5Joints = joints;

        idHashTable<int> boneMap;

        for(j = 0; j < nodes.Num(); j++)
        {
            int md5Index = boneList.FindIndex(nodes[j].name);
            if(md5Index != -1)
            {
                int index = j;
                boneMap.Set(nodes[j].name, index);
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

            if(boneMap.Get(meshJoint->boneName, &rindex)) // anim key bone in mesh
			{
				key = &animKeys[*rindex];

				boneOrigin[0] = key->pos[0];
				boneOrigin[1] = key->pos[1];
				boneOrigin[2] = key->pos[2];

				idVec3 rot;
				rot[0] = key->rot[0];
				rot[1] = key->rot[1];
				rot[2] = key->rot[2];
				boneQuat = fromangles(rot);
			}
			else // anim key bone not in mesh
			{
				if(i == 0)
					common->Warning("Bone not found in psa: %s", meshJoint->boneName.c_str());
                index = j;
                if(addOrigin)
                    index--;
                const smdSkeleton_t *refBone = &bones[index];
                boneOrigin[0] = refBone->pos[0];
                boneOrigin[1] = refBone->pos[1];
                boneOrigin[2] = refBone->pos[2];

				idVec3 rot;
				rot[0] = refBone->rot[0];
				rot[1] = refBone->rot[1];
				rot[2] = refBone->rot[2];
				boneQuat = fromangles(rot);
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

        md5Frame = &_frames.joints[0];
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

#ifdef _MODEL_OBJ
bool idModelSmd::ToObj(objModel_t &objModel) const
{
    int i, j;

    if(!IsMeshFile())
    {
        common->Warning("No triangles in smd");
        return false;
    }

    idList<idList<const smdTriangle_t *> > triGroup;
    idStrList matList;

    int num = GroupTriangle(triGroup, matList);

    for(i = 0; i < num; i++)
    {
        objModel.objects.Append(new objObject_t);
        objObject_t *objObject = objModel.objects[i];
        objObject->material = matList[i];

        const idList<const smdTriangle_t *> &triList = triGroup[i];

        for(j = 0; j < triList.Num(); j++)
        {
            const smdTriangle_t *face = triList[j];

            int objVertIndexes[3];
            for(int k = 0; k < 3; k++)
            {
                const smdVertex_t *vertex = &face->vertexes[k];

                objVertIndexes[k] = objObject->vertexes.Append(idVec3(vertex->pos[0], vertex->pos[1], vertex->pos[2]));

                objObject->texcoords.Append(idVec2(vertex->tc[0], vertex->tc[1]));

                objObject->normals.Append(idVec3(vertex->norm[0], vertex->norm[1], vertex->norm[2]));
            }

			objObject->indexes.Append(objVertIndexes[0]);
			objObject->indexes.Append(objVertIndexes[2]); // swap
			objObject->indexes.Append(objVertIndexes[1]);
        }
    }

    return true;
}
#endif

void idModelSmd::Print(void) const
{
#define MODEL_PART_PRINT(x) if(IsTypeMarked(x)) { Sys_Printf(#x "\n"); }
#undef MODEL_PART_PRINT
#define MODEL_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num());   \
    if(all)                                              \
    for(int i = 0; i < list.Num(); i++) {  \
         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    MODEL_PART_PRINT(node, nodes, true, "%d, name=%s, parent=%d   ", nodes[i].index, nodes[i].name.c_str(), nodes[i].parent_index)
    MODEL_PART_PRINT(frame, frames, false, "time=%d   ", frames[i].time)
    MODEL_PART_PRINT(triangle, triangles, false, "material=%s   ", triangles[i].material.c_str())

#undef MODEL_PART_PRINT
}

#ifdef _MODEL_OBJ
static void R_ConvertSmdToObj_f(const idCmdArgs &args)
{
    const char *filePath = args.Argv(1);
    idModelSmd smd;
    if(smd.Parse(filePath))
    {
        //smd.Print();
        objModel_t objModel;
        if(smd.ToObj(objModel))
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
        common->Warning("Parse smd fail: %s", filePath);
}
#endif

static int R_ConvertSmdToMd5(const char *meshPath, bool doMesh = true, const idStrList *animPaths = NULL, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	int ret = 0;

    idModelSmd smd;
    idMd5MeshFile md5MeshFile;
    bool meshRes = false;
    if(smd.Parse(meshPath))
    {
        //smd.Print();
		if(smd.ToMd5Mesh(md5MeshFile, scale, addOrigin, offset, rotation))
		{
			if(doMesh)
			{
				md5MeshFile.Commandline().Append(va(" - %s", meshPath));
                idStr md5meshPath = R_Model_MakeOutputPath(meshPath, ".md5mesh", savePath);
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
        common->Warning("Parse smd fail: %s", meshPath);

    if(!meshRes)
        return ret;

    if(!animPaths)
        return ret;

	for(int i = 0; i < animPaths->Num(); i++)
	{
		const char *animPath = (*animPaths)[i];
		idModelSmd animSmd;
		if(animSmd.Parse(animPath))
		{
			//animSmd.Print();
			idMd5AnimFile md5AnimFile;
			if(animSmd.ToMd5Anim(smd, md5AnimFile, md5MeshFile, scale, addOrigin, offset, rotation))
			{
				md5AnimFile.Commandline().Append(va(" - %s", animPath));
                idStr md5animPath = R_Model_MakeOutputPath(animPath, ".md5anim", savePath);
				md5AnimFile.Write(md5animPath.c_str());
				common->Printf("Convert md5anim successful: %s -> %s\n", animPath, md5animPath.c_str());
				ret++;
			}
			else
				common->Warning("Convert md5anim fail: %s", animPath);
		}
		else
			common->Warning("Parse smd animation fail: %s", animPath);
	}

	return ret;
}

ID_INLINE static int R_ConvertSmdMesh(const char *meshPath, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	return R_ConvertSmdToMd5(meshPath, true, NULL, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertSmdAnim(const char *meshPath, const idStrList &animPaths, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	return R_ConvertSmdToMd5(meshPath, false, &animPaths, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertSmd(const char *meshPath, const idStrList &animPaths, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
	return R_ConvertSmdToMd5(meshPath, true, &animPaths, scale, addOrigin, offset, rotation, savePath);
}

static void R_ConvertSmdToMd5mesh_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(smd), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5MESH_USAGE(smd), args.Argv(0));
        return;
    }
    R_ConvertSmdMesh(mesh, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertSmdToMd5anim_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf(CONVERT_TO_MD5ANIM_USAGE(smd, smd), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5ANIM_USAGE(smd, smd), args.Argv(0));
        return;
    }
    R_ConvertSmdAnim(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertSmdToMd5_f(const idCmdArgs &args)
{
    if(args.Argc() < 3)
    {
        common->Printf(CONVERT_TO_MD5_USAGE(smd, smd), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5_USAGE(smd, smd), args.Argv(0));
        return;
    }
    R_ConvertSmd(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

bool R_Model_HandleSmd(const md5ConvertDef_t &convert)
{
    if(R_ConvertSmd(convert.mesh, convert.anims, 
				convert.scale, 
				convert.addOrigin, 
				convert.offset.IsZero() ? NULL : &convert.offset, 
				convert.rotation.IsIdentity() ? NULL : &convert.rotation,
                convert.savePath.IsEmpty() ? NULL : convert.savePath.c_str()
	) != 1 + convert.anims.Num())
    {
        common->Warning("Convert smd to md5mesh/md5anim fail in entityDef '%s'", convert.def->GetName());
        return false;
    }
    return true;
}

static void ArgCompletion_smd(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".smd"
            , NULL);
}

void R_SMD_AddCommand(void)
{
    cmdSystem->AddCommand("smdToMd5mesh", R_ConvertSmdToMd5mesh_f, CMD_FL_RENDERER, "Convert smd to md5mesh", ArgCompletion_smd);
    cmdSystem->AddCommand("smdToMd5anim", R_ConvertSmdToMd5anim_f, CMD_FL_RENDERER, "Convert smd to md5anim", ArgCompletion_smd);
    cmdSystem->AddCommand("smdToMd5", R_ConvertSmdToMd5_f, CMD_FL_RENDERER, "Convert smd to md5mesh/md5anim", ArgCompletion_smd);
#ifdef _MODEL_OBJ
    cmdSystem->AddCommand("smdToObj", R_ConvertSmdToObj_f, CMD_FL_RENDERER, "Convert smd to obj", ArgCompletion_smd);
#endif
}
