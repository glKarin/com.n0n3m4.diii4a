
#include "Model_md5mesh_v6.h"

idModelMd5meshV6::idModelMd5meshV6(void)
        : version(MD5_V6_VERSION)
{}

bool idModelMd5meshV6::Parse(const char *path)
{
    idToken		token;
    idLexer		parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS);
    int			num;
    int			count;
    md5meshV6Bone_t *joint;
    md5meshV6Mesh_t *mesh;
    int			i, j;
    md5meshV6Vert_t *vert;
    md5meshV6Tri_t *tri;
    md5meshV6Weight_t *weight;

    Clear();

    if (!parser.LoadFile(path)) {
        return false;
    }

    parser.ExpectTokenString(MD5_VERSION_STRING);
    version = parser.ParseInt();
    if(version != MD5_V6_VERSION)
    {
        common->Warning("md5mesh version is not 2002 E3 demo: %d != %d", version, MD5_V6_VERSION);
        version = MD5_V6_VERSION;
        return false;
    }

    // skip commandline
    parser.ExpectTokenString("commandline");
    parser.ReadToken(&token);
    commandline = token;

    // parse num bones
    parser.ExpectTokenString("numbones");
    num  = parser.ParseInt();
    bones.SetGranularity(1);
    bones.SetNum(num);

    // parse bones
    joint = bones.Ptr();

    for (i = 0; i < bones.Num(); i++, joint++) {
        parser.ExpectTokenString("bone");
        joint->index = parser.ParseInt();
        parser.ExpectTokenString("{");

        // parse name
        parser.ExpectTokenString("name");
        parser.ReadToken(&token);
        joint->name = token;


        // parse default pose
        parser.ExpectTokenString("bindpos");
        for (j = 0; j < 3; j++) {
            joint->bindpos[j] = parser.ParseFloat();
        }
        parser.ExpectTokenString("bindmat");
        for (j = 0; j < 9; j++) {
            joint->bindmat.ToFloatPtr()[j] = parser.ParseFloat();
        }

        // parse parent
        parser.ReadToken(&token);
        if(!idStr::Icmp(token, "parent"))
        {
            parser.ReadToken(&token);
            joint->parent = token;
        }
        else
            parser.UnreadToken(&token);

        parser.ExpectTokenString("}");
    }

    // parse num meshes
    parser.ExpectTokenString("nummeshes");
    num = parser.ParseInt();
    meshes.SetGranularity(1);
    meshes.SetNum(num);

    for (j = 0, mesh = &meshes[0]; j < meshes.Num(); j++, mesh++) {
        parser.ExpectTokenString("mesh");
        mesh->index = parser.ParseInt();
        parser.ExpectTokenString("{");

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

            vert->uv[0] = parser.ParseFloat();
            vert->uv[1] = parser.ParseFloat();

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

            weight->pos[0] = parser.ParseFloat();
            weight->pos[1] = parser.ParseFloat();
            weight->pos[2] = parser.ParseFloat();
        }

        parser.ExpectTokenString("}");
    }

    return true;
}

bool idModelMd5meshV6::ToMd5Mesh(idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *meshOffset, const idMat3 *meshRotation) const
{
    int i, j;
    md5meshJoint_t *md5Bone;
    md5meshMesh_t *toMesh;
    const md5meshV6Mesh_t *fromMesh;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshV6Bone_t *refBone;
    int numBones = bones.Num();

    md5mesh.Commandline() = va("Convert from md5mesh v6(2002 E3 demo) file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
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

    idHashTable<int> boneMap;
    for (i = 0, refBone = &bones[0]; i < bones.Num(); i++, refBone++)
    {
        boneMap.Set(refBone->name, i);
    }

	bool transform = ((meshRotation && !meshRotation->IsIdentity())
			|| (meshOffset && !meshOffset->IsZero())
			|| (scale > 0.0f)
			);
    bool hasOrigin = false;
    for (i = 0, refBone = &bones[0]; i < bones.Num(); i++, md5Bone++, refBone++)
    {
        md5Bone->boneName = refBone->name;

        if (refBone->parent.IsEmpty() && !addOrigin)
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
            int *rindex;
            if(boneMap.Get(refBone->parent, &rindex))
                md5Bone->parentIndex = *rindex;
            else
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
        }

        if(addOrigin)
            md5Bone->parentIndex += 1;

        md5Bone->pos = refBone->bindpos;
        md5Bone->orient = refBone->bindmat.ToQuat();
    }

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    idList<md5meshJointTransform_t> jointTransforms2;
    const idList<md5meshJointTransform_t> *jointTransformsMod;
	if(transform)
	{
		for (i = 0, md5Bone = &md5Bones[0]; i < md5Bones.Num(); i++, md5Bone++)
		{
			if(i == 0 && addOrigin)
				continue;

			boneOrigin = md5Bone->pos;

			if(meshRotation && !meshRotation->IsIdentity())
			{
				boneOrigin *= *meshRotation;
                boneQuat = md5Bone->orient.Inverse();
				boneQuat = (meshRotation->Transpose() * boneQuat.ToMat3()).ToQuat();
                md5Bone->orient = boneQuat.Inverse();
			}
			if(meshOffset && !meshOffset->IsZero())
				boneOrigin += *meshOffset;
			if(scale > 0.0f)
				boneOrigin *= scale;

			md5Bone->pos = boneOrigin;
		}

		md5mesh.ConvertJointTransforms(jointTransforms2);
        jointTransformsMod = &jointTransforms2;
	}
	else
        jointTransformsMod = &jointTransforms;

    // convert md5 mesh
    idList<md5meshMesh_t> &md5Meshes = md5mesh.Meshes();
    md5Meshes.SetNum(meshes.Num());

    for(j = 0, fromMesh = &meshes[0], toMesh = &md5Meshes[0]; j < meshes.Num(); j++, fromMesh++, toMesh++)
    {
        toMesh->shader = fromMesh->shader;
        toMesh->verts = fromMesh->verts;
        toMesh->tris = fromMesh->tris;
        toMesh->weights.SetNum(fromMesh->weights.Num());

        const md5meshV6Weight_t *weight;
        md5meshWeight_t *md5Weight;
        for(i = 0, weight = &fromMesh->weights[0], md5Weight = &toMesh->weights[0]; i < toMesh->weights.Num(); i++, weight++, md5Weight++)
        {
            md5Weight->weightValue = weight->weightValue;
            md5Weight->jointIndex = weight->jointIndex;
            if(addOrigin)
                md5Weight->jointIndex += 1;

			if(transform)
			{
				idVec3 pos;
				const md5meshJointTransform_t *jointTransform = &jointTransforms[md5Weight->jointIndex];
				jointTransform->bindmat.UnprojectVector(weight->pos, pos);
				pos += jointTransform->bindpos;

				if(meshRotation && !meshRotation->IsIdentity())
					pos *= *meshRotation;
				if(meshOffset && !meshOffset->IsZero())
					pos += *meshOffset;
				if(scale > 0.0f)
					pos *= scale;

				jointTransform = &(*jointTransformsMod)[md5Weight->jointIndex];
				jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight->pos);
			}
			else
				md5Weight->pos = weight->pos;
        }
    }

    return true;
}

void idModelMd5meshV6::Clear(void)
{
    version = MD5_V6_VERSION;
    commandline.Clear();
    bones.Clear();
    meshes.Clear();
}

void idModelMd5meshV6::Print(void) const
{
#define MODEL_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
	if(all) \
    for(int i = 0; i < list.Num(); i++) {  \
         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    MODEL_PART_PRINT(bone, bones, true, "%s, parent=%s   ", bones[i].name.c_str(), bones[i].parent.c_str())
    MODEL_PART_PRINT(mesh, meshes, true, "shader=%s, verts=%d, tris=%d, weights=%d   ", meshes[i].shader.c_str(), meshes[i].verts.Num(), meshes[i].tris.Num(), meshes[i].weights.Num())

#undef MODEL_PART_PRINT
}
