#include "Model_iqm.h"

#include "Model_md5mesh.h"
#include "Model_md5anim.h"

#define ofs_neighbors ofs_adjacency

#define WEIGHT_BYTE_TO_FLOAT(x) ((float)(x) * (1.0f / 255.0f))

#define IQM_PRINT_OFFSETS(offsets) do { \
Sys_Printf("offsets: vnormal=%d, vposition=%d, vtangent=%d, vtexcoord=%d, vcolor4f=%d, vblendindexes=%d, vblendweights=%d, vcolor4ub=%d, framedata=%d\n", \
offsets.vnormal, \
offsets.vposition, \
offsets.vtangent, \
offsets.vtexcoord, \
offsets.vcolor4f, \
offsets.vblendindexes, \
offsets.vblendweights, \
offsets.vcolor4ub, \
offsets.framedata \
); \
} while(0);

idModelIqm::idModelIqm(void)
: file(NULL),
types(0)
{}

idModelIqm::~idModelIqm(void)
{
    if(file)
        fileSystem->CloseFile(file);
}

void idModelIqm::Clear(void)
{
    vertexes.SetNum(0);
    texcoords.SetNum(0);
    normals.SetNum(0);
    colors.SetNum(0);
    tangents.SetNum(0);
    blendIndexes.SetNum(0);
    blendWeights.SetNum(0);
    triangles.SetNum(0);
    meshes.SetNum(0);
    bounds.SetNum(0);
    joints.SetNum(0);
    texts.SetNum(0);
    anims.SetNum(0);
    poses.SetNum(0);
    framedatas.SetNum(0);
    if(file)
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }
    memset(&header, 0, sizeof(header));
}

void idModelIqm::MarkType(int type)
{
    types |= (1 << type);
}

bool idModelIqm::IsTypeMarked(int type) const
{
    return types & (1 << type);
}

int idModelIqm::ReadHeader(iqmheader &header)
{
    if(file->Length() < (int)sizeof(header.magic))
    {
        common->Warning("Unexpected end of file(%d < %zu bytes).", file->Length(), sizeof(header.magic));
        return -1;
    }

    memset(&header, 0, sizeof(header));

    file->Read(header.magic, sizeof(header.magic));
    if(idStr::Icmpn(header.magic, IQM_MAGIC, strlen(IQM_MAGIC)))
    {
        common->Warning("Unexpected magic '%s'", header.magic);
        return -1;
    }

    file->ReadUnsignedInt(header.version);
    if(header.version != IQM_VERSION1 && header.version != IQM_VERSION2)
    {
        common->Warning("unsupported IQM version '%u'", header.version);
        return -1;
    }

    file->ReadUnsignedInt(header.filesize);
    if(header.filesize != file->Length())
    {
        common->Warning("IQM file size(%d) != header's file size(%u)", file->Length(), header.filesize);
        return -1;
    }

    file->ReadUnsignedInt(header.flags);
    file->ReadUnsignedInt(header.num_text);
    file->ReadUnsignedInt(header.ofs_text);
    file->ReadUnsignedInt(header.num_meshes);
    file->ReadUnsignedInt(header.ofs_meshes);
    file->ReadUnsignedInt(header.num_vertexarrays);
    file->ReadUnsignedInt(header.num_vertexes);
    file->ReadUnsignedInt(header.ofs_vertexarrays);
    file->ReadUnsignedInt(header.num_triangles);
    file->ReadUnsignedInt(header.ofs_triangles);
    file->ReadUnsignedInt(header.ofs_neighbors);
    file->ReadUnsignedInt(header.num_joints);
    file->ReadUnsignedInt(header.ofs_joints);
    file->ReadUnsignedInt(header.num_poses);
    file->ReadUnsignedInt(header.ofs_poses);
    file->ReadUnsignedInt(header.num_anims);
    file->ReadUnsignedInt(header.ofs_anims);
    file->ReadUnsignedInt(header.num_frames);
    file->ReadUnsignedInt(header.num_framechannels);
    file->ReadUnsignedInt(header.ofs_frames);
    file->ReadUnsignedInt(header.ofs_bounds);
    file->ReadUnsignedInt(header.num_comment);
    file->ReadUnsignedInt(header.ofs_comment);
    file->ReadUnsignedInt(header.num_extensions);
    file->ReadUnsignedInt(header.ofs_extensions);

    return sizeof(header);
}

bool idModelIqm::ReadOffsets(iqmVertexOffset_t &offsets)
{
    int i;
    const unsigned int pend = file->Length();
    file->Seek(header.ofs_vertexarrays, FS_SEEK_SET);

    memset(&offsets, 0, sizeof(offsets));

    for (i = 0;i < (int)header.num_vertexarrays;i++)
    {
        iqmvertexarray_t va;
        size_t vsize;

        memset(&va, 0, sizeof(iqmvertexarray_t));
        file->ReadUnsignedInt(va.type);
        file->ReadUnsignedInt(va.flags);
        file->ReadUnsignedInt(va.format);
        file->ReadUnsignedInt(va.size);
        file->ReadUnsignedInt(va.offset);
        vsize = header.num_vertexes*va.size;

        // Assert if no static_assert
        ID_STATIC_ASSERT(sizeof(float) == 4);
        ID_STATIC_ASSERT(sizeof(unsigned char) == 1);

        switch (va.format)
        {
            case IQM_FLOAT: vsize *= sizeof(float); break;
            case IQM_UBYTE: vsize *= sizeof(unsigned char); break;
            default: continue;
        }
        if (va.offset + vsize > pend)
            continue;
        // no need to copy the vertex data for alignment because LittleLong/LittleShort will be invoked on reading them, and the destination is aligned
        switch (va.type)
        {
            case IQM_POSITION:
                if (va.format == IQM_FLOAT && va.size == 3)
                    offsets.vposition = va.offset;
                break;
            case IQM_TEXCOORD:
                if (va.format == IQM_FLOAT && va.size == 2)
                    offsets.vtexcoord = va.offset;
                break;
            case IQM_NORMAL:
                if (va.format == IQM_FLOAT && va.size == 3)
                    offsets.vnormal = va.offset;
                break;
            case IQM_TANGENT:
                if (va.format == IQM_FLOAT && va.size == 4)
                    offsets.vtangent = va.offset;
                break;
            case IQM_BLENDINDEXES:
                if (va.format == IQM_UBYTE && va.size == 4)
                    offsets.vblendindexes = va.offset;
                break;
            case IQM_BLENDWEIGHTS:
                if (va.format == IQM_UBYTE && va.size == 4)
                    offsets.vblendweights = va.offset;
                break;
            case IQM_COLOR:
                if (va.format == IQM_FLOAT && va.size == 4)
                    offsets.vcolor4f = va.offset;
                else if (va.format == IQM_UBYTE && va.size == 4)
                    offsets.vcolor4ub = va.offset;
                break;
        }
    }

#if 0
    IQM_PRINT_OFFSETS(offsets);
#endif
    if (header.num_vertexes > 0 && (!offsets.vposition || !offsets.vtexcoord || ((header.num_frames > 0 || header.num_anims > 0) && (!offsets.vblendindexes || !offsets.vblendweights))))
    {
        common->Warning("Missing vertex array data");
        return false;
    }

    return true;
}

int idModelIqm::ReadVertexes(unsigned int vposition)
{
    if(!vposition)
        return 0;
    vertexes.SetNum(header.num_vertexes);
    file->Seek(vposition, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmVertex_t &item = vertexes[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
        file->ReadFloat(item[2]);
    }
    return vertexes.Num();
}

int idModelIqm::ReadTexcoords(unsigned int vtexcoord)
{
    if(!vtexcoord)
        return 0;
    texcoords.SetNum(header.num_vertexes);
    file->Seek(vtexcoord, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmTexcoord_t &item = texcoords[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
    }
    return texcoords.Num();
}

int idModelIqm::ReadNormals(unsigned int vnormal)
{
    if(!vnormal)
        return 0;
    normals.SetNum(header.num_vertexes);
    file->Seek(vnormal, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmVertex_t &item = normals[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
        file->ReadFloat(item[2]);
    }
    return normals.Num();
}

int idModelIqm::ReadColorsf(unsigned int vcolor4f)
{
    if(!vcolor4f)
        return 0;
    colors.SetNum(header.num_vertexes);
    file->Seek(vcolor4f, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmColor_t &item = colors[i];
#define READ_COLOR_F(c) file->ReadFloat(f); item.color[c] = (byte)(f * 255.0f);
        float f;
        READ_COLOR_F(0);
        READ_COLOR_F(1);
        READ_COLOR_F(2);
        READ_COLOR_F(3);
#undef READ_COLOR_F
    }
    return colors.Num();
}

int idModelIqm::ReadColorsub(unsigned int vcolor4ub)
{
    if(!vcolor4ub)
        return 0;
    colors.SetNum(header.num_vertexes);
    file->Seek(vcolor4ub, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmColor_t &item = colors[i];
        file->ReadUnsignedChar(item.color[0]);
        file->ReadUnsignedChar(item.color[1]);
        file->ReadUnsignedChar(item.color[2]);
        file->ReadUnsignedChar(item.color[3]);
    }
    return colors.Num();
}

int idModelIqm::ReadTangents(unsigned int vtangent)
{
    if(!vtangent)
        return 0;
    tangents.SetNum(header.num_vertexes);
    file->Seek(vtangent, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmTangent_t &item = tangents[i];
        file->ReadFloat(item[0]);
        file->ReadFloat(item[1]);
        file->ReadFloat(item[2]);
    }
    return tangents.Num();
}

int idModelIqm::ReadBlendIndexes(unsigned int vblendindexes)
{
    if(!vblendindexes)
        return 0;
    blendIndexes.SetNum(header.num_vertexes);
    file->Seek(vblendindexes, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmBlendIndex_t &item = blendIndexes[i];
        file->ReadUnsignedChar(item.index[0]);
        file->ReadUnsignedChar(item.index[1]);
        file->ReadUnsignedChar(item.index[2]);
        file->ReadUnsignedChar(item.index[3]);
    }
    return blendIndexes.Num();
}

int idModelIqm::ReadBlendWeights(unsigned int vblendweights)
{
    if(!vblendweights)
        return 0;
    blendWeights.SetNum(header.num_vertexes);
    file->Seek(vblendweights, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_vertexes; i++)
    {
        iqmBlendWeight_t &item = blendWeights[i];
        file->ReadUnsignedChar(item.weight[0]);
        file->ReadUnsignedChar(item.weight[1]);
        file->ReadUnsignedChar(item.weight[2]);
        file->ReadUnsignedChar(item.weight[3]);
    }
    return blendWeights.Num();
}

int idModelIqm::ReadTriangles(void)
{
    if(!header.num_triangles)
        return 0;
    triangles.SetNum(header.num_triangles);
    file->Seek(header.ofs_triangles, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_triangles; i++)
    {
        iqmTriangle_t &item = triangles[i];
        file->ReadInt(item.elements[0]);
        file->ReadInt(item.elements[1]);
        file->ReadInt(item.elements[2]);
    }
    return triangles.Num();
}

int idModelIqm::ReadMeshes(void)
{
    if(!header.num_meshes)
        return 0;
    meshes.SetNum(header.num_meshes);
    file->Seek(header.ofs_meshes, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_meshes; i++)
    {
        iqmMesh_t &item = meshes[i];
        file->ReadUnsignedInt(item.name);
        file->ReadUnsignedInt(item.material);
        file->ReadUnsignedInt(item.first_vertex);
        file->ReadUnsignedInt(item.num_vertexes);
        file->ReadUnsignedInt(item.first_triangle);
        file->ReadUnsignedInt(item.num_triangles);
    }
    return meshes.Num();
}

int idModelIqm::ReadBounds(void)
{
    if(!header.num_frames || !header.ofs_bounds)
        return 0;
    bounds.SetNum(header.num_frames);
    file->Seek(header.ofs_bounds, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_frames; i++)
    {
        iqmBounds_t &item = bounds[i];
        file->ReadFloat(item.bbmin[0]);
        file->ReadFloat(item.bbmin[1]);
        file->ReadFloat(item.bbmin[2]);
        file->ReadFloat(item.bbmax[0]);
        file->ReadFloat(item.bbmax[1]);
        file->ReadFloat(item.bbmax[2]);
        file->ReadFloat(item.xyradius);
        file->ReadFloat(item.radius);
    }
    return bounds.Num();
}

int idModelIqm::ReadJoints1(void)
{
    if(!header.num_joints || !header.ofs_joints)
        return 0;
    joints.SetNum(header.num_joints);
    file->Seek(header.ofs_joints, FS_SEEK_SET);
    idCQuat cq;
    idQuat q;
    for(int i = 0; i < (int)header.num_joints; i++)
    {
        iqmJoint_t &item = joints[i];
        file->ReadUnsignedInt(item.name);
        file->ReadInt(item.parent);
        file->ReadFloat(item.translate[0]);
        file->ReadFloat(item.translate[1]);
        file->ReadFloat(item.translate[2]);
        file->ReadFloat(cq[0]);
        file->ReadFloat(cq[1]);
        file->ReadFloat(cq[2]);
        q = cq.ToQuat();
        item.rotate[0] = q[0];
        item.rotate[1] = q[1];
        item.rotate[2] = q[2];
        item.rotate[3] = q[3];
        file->ReadFloat(item.scale[0]);
        file->ReadFloat(item.scale[1]);
        file->ReadFloat(item.scale[2]);
    }
    return joints.Num();
}

int idModelIqm::ReadJoints2(void)
{
    if(!header.num_joints || !header.ofs_joints)
        return 0;
    joints.SetNum(header.num_joints);
    file->Seek(header.ofs_joints, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_joints; i++)
    {
        iqmJoint_t &item = joints[i];
        file->ReadUnsignedInt(item.name);
        file->ReadInt(item.parent);
        file->ReadFloat(item.translate[0]);
        file->ReadFloat(item.translate[1]);
        file->ReadFloat(item.translate[2]);
        file->ReadFloat(item.rotate[0]);
        file->ReadFloat(item.rotate[1]);
        file->ReadFloat(item.rotate[2]);
        file->ReadFloat(item.rotate[3]);
        file->ReadFloat(item.scale[0]);
        file->ReadFloat(item.scale[1]);
        file->ReadFloat(item.scale[2]);
    }
    return joints.Num();
}

int idModelIqm::ReadJoints(void)
{
    if(header.version == 1)
        return ReadJoints1();
    else
        return ReadJoints2();
}

int idModelIqm::ReadTexts(void)
{
    char ch;
    if(!header.num_text)
        return 0;
    texts.SetNum(0);
    texts.SetGranularity(1);
    file->Seek(header.ofs_text, FS_SEEK_SET);
    int index;
    iqmText_t *item = NULL;
    for(int i = 0; i < (int)header.num_text; i++)
    {
        if(!item)
        {
            index = texts.Append(iqmText_t());
            item = &texts[index];
            item->text.Clear();
            item->offset = i;
        }
        file->ReadChar(ch);
        if(ch)
            item->text.Append(ch);
        else
            item = NULL;
    }
    return texts.Num();
}

int idModelIqm::ReadAnims(void)
{
    if(!header.num_anims || !header.ofs_anims)
        return 0;
    anims.SetNum(header.num_anims);
    file->Seek(header.ofs_anims, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_anims; i++)
    {
        iqmAnim_t &item = anims[i];
        file->ReadUnsignedInt(item.name);
        file->ReadUnsignedInt(item.first_frame);
        file->ReadUnsignedInt(item.num_frames);
        file->ReadFloat(item.framerate);
        file->ReadUnsignedInt(item.flags);
    }
    return anims.Num();
}

int idModelIqm::ReadPoses1(void)
{
    if(!header.num_poses || !header.ofs_poses)
        return 0;
    poses.SetNum(header.num_poses);
    file->Seek(header.ofs_poses, FS_SEEK_SET);
    idCQuat cq;
    idQuat q;
    for(int i = 0; i < (int)header.num_poses; i++)
    {
        iqmPose_t &item = poses[i];
        file->ReadInt(item.parent);
        file->ReadUnsignedInt(item.mask);
        file->ReadFloat(item.channeloffset[0]);
        file->ReadFloat(item.channeloffset[1]);
        file->ReadFloat(item.channeloffset[2]);
        file->ReadFloat(cq[0]);
        file->ReadFloat(cq[1]);
        file->ReadFloat(cq[2]);
        q = cq.ToQuat();
        item.channeloffset[3] = q[0];
        item.channeloffset[4] = q[1];
        item.channeloffset[5] = q[2];
        item.channeloffset[6] = q[3];
        file->ReadFloat(item.channeloffset[7]);
        file->ReadFloat(item.channeloffset[8]);
        file->ReadFloat(item.channeloffset[9]);
        file->ReadFloat(item.channelscale[0]);
        file->ReadFloat(item.channelscale[1]);
        file->ReadFloat(item.channelscale[2]);
        file->ReadFloat(cq[0]);
        file->ReadFloat(cq[1]);
        file->ReadFloat(cq[2]);
        q = cq.ToQuat();
        item.channelscale[3] = q[0];
        item.channelscale[4] = q[1];
        item.channelscale[5] = q[2];
        item.channelscale[6] = q[3];
        file->ReadFloat(item.channelscale[7]);
        file->ReadFloat(item.channelscale[8]);
        file->ReadFloat(item.channelscale[9]);
    }
    return poses.Num();
}

int idModelIqm::ReadPoses2(void)
{
    if(!header.num_poses || !header.ofs_poses)
        return 0;
    poses.SetNum(header.num_poses);
    file->Seek(header.ofs_poses, FS_SEEK_SET);
    for(int i = 0; i < (int)header.num_poses; i++)
    {
        iqmPose_t &item = poses[i];
        file->ReadInt(item.parent);
        file->ReadUnsignedInt(item.mask);
        file->ReadFloat(item.channeloffset[0]);
        file->ReadFloat(item.channeloffset[1]);
        file->ReadFloat(item.channeloffset[2]);
        file->ReadFloat(item.channeloffset[3]);
        file->ReadFloat(item.channeloffset[4]);
        file->ReadFloat(item.channeloffset[5]);
        file->ReadFloat(item.channeloffset[6]);
        file->ReadFloat(item.channeloffset[7]);
        file->ReadFloat(item.channeloffset[8]);
        file->ReadFloat(item.channeloffset[9]);
        file->ReadFloat(item.channelscale[0]);
        file->ReadFloat(item.channelscale[1]);
        file->ReadFloat(item.channelscale[2]);
        file->ReadFloat(item.channelscale[3]);
        file->ReadFloat(item.channelscale[4]);
        file->ReadFloat(item.channelscale[5]);
        file->ReadFloat(item.channelscale[6]);
        file->ReadFloat(item.channelscale[7]);
        file->ReadFloat(item.channelscale[8]);
        file->ReadFloat(item.channelscale[9]);
    }
    return poses.Num();
}

int idModelIqm::ReadPoses(void)
{
    if(header.version == 1)
        return ReadPoses1();
    else
        return ReadPoses2();
}

int idModelIqm::ReadFramedatas(void)
{
    if(!header.num_framechannels || !header.ofs_frames)
        return 0;
    framedatas.SetNum(header.num_framechannels * header.num_frames);
    file->Seek(header.ofs_frames, FS_SEEK_SET);
    idCQuat cq;
    idQuat q;
    for(int i = 0; i < (int)header.num_frames; i++)
    {
        iqmFramedata_t *item = &framedatas[i * header.num_framechannels];
        for(int j = 0; j < (int)header.num_framechannels; j++)
        {
            file->ReadUnsignedShort(item[j]);
        }
    }
    return framedatas.Num();
}

bool idModelIqm::Parse(const char *iqmPath, int type)
{
    Clear();

    file = fileSystem->OpenFileRead(iqmPath);
    if(!file)
        return false;

    if(ReadHeader(header) <= 0)
        return false;

	if(type == PARSE_DEF)
		type = PARSE_FRAME;

    const unsigned int pend = file->Length();

    if (header.version == 1)
    {
        if (header.ofs_joints + header.num_joints*sizeof(iqmjoint1_t) > pend ||
            header.ofs_poses + header.num_poses*sizeof(iqmpose1_t) > pend)
        {
            common->Warning("%s has invalid size or offset joints/poses information in version 1", iqmPath);
            return false;
        }
    }
    else
    {
        if (header.ofs_joints + header.num_joints*sizeof(iqmjoint_t) > pend ||
            header.ofs_poses + header.num_poses*sizeof(iqmpose_t) > pend)
        {
            common->Warning("%s has invalid size or offset joints/poses information", iqmPath);
            return false;
        }
    }
    if (header.ofs_text + header.num_text > pend ||
        header.ofs_meshes + header.num_meshes*sizeof(iqmmesh_t) > pend ||
        header.ofs_vertexarrays + header.num_vertexarrays*sizeof(iqmvertexarray_t) > pend ||
        header.ofs_triangles + header.num_triangles*sizeof(int[3]) > pend ||
        (header.ofs_neighbors && header.ofs_neighbors + header.num_triangles*sizeof(int[3]) > pend) ||
        header.ofs_anims + header.num_anims*sizeof(iqmanim_t) > pend ||
        header.ofs_frames + header.num_frames*header.num_framechannels*sizeof(unsigned short) > pend ||
        (header.ofs_bounds && header.ofs_bounds + header.num_frames*sizeof(iqmbounds_t) > pend) ||
        header.ofs_comment + header.num_comment > pend)
    {
        common->Warning("%s has invalid size or offset mesh information", iqmPath);
        return false;
    }

    iqmVertexOffset_t offsets;
    if(!ReadOffsets(offsets))
        return false;

	if(type != PARSE_ANIM)
	{
		if(ReadVertexes(offsets.vposition))
			MarkType(IQM_POSITION);
		if(ReadTexcoords(offsets.vtexcoord))
			MarkType(IQM_TEXCOORD);
		if(ReadNormals(offsets.vnormal))
			MarkType(IQM_NORMAL);

		if(!ReadColorsf(offsets.vcolor4f))
			ReadColorsub(offsets.vcolor4ub);
		if(colors.Num())
			MarkType(IQM_COLOR);

		if(ReadMeshes())
			MarkType(IQM_MESH);
		if(ReadTriangles())
			MarkType(IQM_TRIANGLE);
	}

	if(type >= PARSE_ALL)
	{
		if(ReadTangents(offsets.vtangent))
			MarkType(IQM_TANGENT);
	}

	if(type >= PARSE_JOINT)
	{
		if(ReadBlendIndexes(offsets.vblendindexes))
			MarkType(IQM_BLENDINDEXES);
		if(ReadBlendWeights(offsets.vblendweights))
			MarkType(IQM_BLENDWEIGHTS);

		if(ReadJoints())
			MarkType(IQM_JOINT);
	}

    if(ReadTexts())
        MarkType(IQM_TEXT);

	if(type >= PARSE_ANIM)
	{
		if(ReadAnims())
			MarkType(IQM_ANIM);
	}
	if(type >= PARSE_FRAME)
	{
		if(ReadBounds())
			MarkType(IQM_BOUNDS);
		if(ReadPoses())
			MarkType(IQM_POSE);
		if(ReadFramedatas())
			MarkType(IQM_FRAMEDATA);
	}

	bool err = false;
	if(type != PARSE_ANIM)
	{
		err = !IsTypeMarked(IQM_POSITION) || !IsTypeMarked(IQM_MESH || !IsTypeMarked(IQM_TRIANGLE));
	}
	if(type >= PARSE_JOINT)
	{
		err = !IsTypeMarked(IQM_JOINT) || !IsTypeMarked(IQM_BLENDINDEXES || !IsTypeMarked(IQM_BLENDWEIGHTS));
	}
	if(type >= PARSE_ANIM)
	{
		err = !IsTypeMarked(IQM_ANIM);
	}
	if(type >= PARSE_FRAME)
	{
		err = !IsTypeMarked(IQM_BOUNDS) || !IsTypeMarked(IQM_POSE)/* || !IsTypeMarked(IQM_FRAMEDATA)*/;
	}

	if(err)
	{
        Clear();
	}
    else
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }

    return !err;
}

bool idModelIqm::ToMd5Mesh(idMd5MeshFile &md5mesh, float scale, bool addOrigin) const
{
    int i, j;
    md5meshJoint_t *md5Bone;
    const iqmJoint_t *refBone;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshJointTransform_t *jointTransform;
	int numBones = joints.Num();

    md5mesh.Commandline() = va("Convert from iqm file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
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

    for (i = 0, refBone = &joints[0]; i < joints.Num(); i++, md5Bone++, refBone++)
    {
        md5Bone->boneName = GetText(refBone->name);
        md5Bone->parentIndex = refBone->parent;

		if(addOrigin)
			md5Bone->parentIndex += 1;

        boneOrigin[0] = refBone->translate[0];
        boneOrigin[1] = refBone->translate[1];
        boneOrigin[2] = refBone->translate[2];
		if(scale > 0.0f)
			boneOrigin *= scale;

        boneQuat[0] = refBone->rotate[0];
        boneQuat[1] = refBone->rotate[1];
        boneQuat[2] = refBone->rotate[2];
        boneQuat[3] = refBone->rotate[3];

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
    md5Meshes.SetNum(meshes.Num());

    for(j = 0; j < meshes.Num(); j++)
    {
        const iqmMesh_t *iqmMesh = &meshes[j];
        md5meshMesh_t &mesh = md5Meshes[j];
        mesh.shader = GetText(iqmMesh->material);

        idList<unsigned int> md5Vertexes; // cache saved vert for remove dup vert
        for(unsigned int u = 0; u < iqmMesh->num_triangles; u++)
        {
            const iqmTriangle_t *face = &triangles[iqmMesh->first_triangle + u];

            int md5VertIndexes[3];
            for(int k = 0; k < 3; k++)
            {
                const int vertexIndex = face->elements[k];
                // find vert is cached
                int n;
                for(n = 0; n < md5Vertexes.Num(); n++)
                {
                    if(vertexIndex == md5Vertexes[n])
                        break;
                }
                if(n < md5Vertexes.Num())
                {
                    md5VertIndexes[k] = n;
                    continue;
                }

                const pskVertex_t *vertex = &vertexes[vertexIndex];

                idList<unsigned int> vertWeights;
                const iqmBlendIndex_t *blendIndex = &blendIndexes[vertexIndex];
                const iqmBlendWeight_t *blendWeight = &blendWeights[vertexIndex];
                for(int m = 0; m < 4; m++)
                {
                    if(!blendWeight->weight[m]) //karin: if weight == 0, no more weights???
                        break;
                    vertWeights.Append(m);
                }
                if(vertWeights.Num() == 0)
                {
                    common->Warning("wedge weight num is zero in wedge '%d'", vertexIndex);
                    return false;
                }
                md5meshVert_t md5Vert;
                md5Vert.uv = texcoords[vertexIndex];
                md5Vert.weightIndex = mesh.weights.Num();
                md5Vert.weightElem = vertWeights.Num();
                md5VertIndexes[k] = mesh.verts.Append(md5Vert); // Add vert
                md5Vertexes.Append(vertexIndex); // cache vert

                float w = 0.0f;
                for(int m = 0; m < vertWeights.Num(); m++)
                {
                    unsigned int weight = vertWeights[m];
                    md5meshWeight_t md5Weight;
                    md5Weight.jointIndex = blendIndex->index[m];
					if(addOrigin)
						md5Weight.jointIndex += 1;
                    jointTransform = &jointTransforms[md5Weight.jointIndex];
                    if(vertWeights.Num() == 1)
					{
						if(blendWeight->weight[m] != 255)
							common->Warning("Vertex '%d' only 1 bone '%s' but weight is not 1 '%u'", vertexIndex, GetText(joints[blendIndex->index[m]].name), blendWeight->weight[m]);
                        md5Weight.weightValue = 1.0f;
					}
                    else
                        md5Weight.weightValue = WEIGHT_BYTE_TO_FLOAT(blendWeight->weight[m]);

                    w += md5Weight.weightValue;
					idVec3 pos = *vertex;
					if(scale > 0.0f)
						pos *= scale;
                    jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight.pos);

                    mesh.weights.Append(md5Weight); // Add weight
                }
                if(w < 1.0f)
                {
                    common->Warning("Vertex '%d' weight sum is less than 1.0: %f", vertexIndex, w);
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

int idModelIqm::ToMd5AnimList(idList<idMd5AnimFile> &md5anims, idMd5MeshFile &md5mesh, float scale, bool addOrigin) const
{
	int num = 0;

	for(int i = 0; i < anims.Num(); i++)
	{
		int index = md5anims.Append(idMd5AnimFile());
		if(!ToMd5Anim(md5anims[index], md5mesh, i, scale, addOrigin))
		{
			md5anims.RemoveIndex(index);
			common->Warning("Animation '%s' convert fail", GetText(anims[i].name));
		}
		else
			num++;
	}
	return num;
}

bool idModelIqm::ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, const char *animName, float scale, bool addOrigin) const
{
	for(int i = 0; i < anims.Num(); i++)
	{
		if(!idStr::Icmp(animName, GetText(anims[i].name)))
		{
			return ToMd5Anim(md5anim, md5mesh, i, scale, addOrigin);
		}
	}
	common->Warning("Animation '%s' not found in iqm", animName);
	return false;
}

bool idModelIqm::ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int animIndex, float scale, bool addOrigin) const
{
	if((unsigned int)animIndex >= header.num_anims)
		return false;

    int i, j;
    md5animHierarchy_t *md5Hierarchy;
    const md5meshJoint_t *meshJoint;
    const iqmFramedata_t *key;
    md5animFrame_t *md5Frame;
    md5meshJointTransform_t *jointTransform;
    md5meshJointTransform_t *frameTransform;
    md5animBaseframe_t *md5BaseFrame;
    md5meshJoint_t *md5Bone;

    const iqmAnim_t *animInfo = &anims[animIndex];

    const idList<md5meshJoint_t> _joints = md5mesh.Joints();
    const int numBones = _joints.Num();

    md5anim.FrameRate() = (int)animInfo->framerate;
    md5anim.NumAnimatedComponents() = numBones * 6;

    md5anim.Commandline() = va("Convert from iqm file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);

    // convert md5 joints
    idList<md5animHierarchy_t> &md5Bones = md5anim.Hierarchies();
    md5Bones.SetNum(numBones);

    idList<md5meshJointTransform_t> jointTransforms;
    md5mesh.ConvertJointTransforms(jointTransforms);

    idStrList boneList;
    boneList.SetNum(numBones);
    for(i = 0, meshJoint = &_joints[0], md5Hierarchy = &md5Bones[0]; i < numBones; i++, meshJoint++, md5Hierarchy++)
    {
        boneList[i] = meshJoint->boneName;

        md5Hierarchy->boneName = meshJoint->boneName;
        md5Hierarchy->numComp = MD5ANIM_ALL;
        md5Hierarchy->frameIndex = i * 6;
        md5Hierarchy->parentIndex = meshJoint->parentIndex;
    }

    // convert md5 bounds
    idList<md5animBounds_t> &md5Bounds = md5anim.Bounds();
    md5Bounds.SetNum(animInfo->num_frames);
    for(i = 0; i < md5Bounds.Num(); i++)
    {
        int index = animInfo->first_frame + i;
        const iqmBounds_t &b = bounds[index];
        md5Bounds[i][0].Set(b.bbmin[0], b.bbmin[1], b.bbmin[2]);
        md5Bounds[i][1].Set(b.bbmax[0], b.bbmax[1], b.bbmax[2]);
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
    md5Frames.SetNum(animInfo->num_frames);
    unsigned int base;
    for(i = 0, base = animInfo->first_frame * header.num_framechannels; i < (int)animInfo->num_frames; i++, base += header.num_framechannels)
    {
        md5animFrames_t &frames = md5Frames[i];
        frames.index = i;
        frames.joints.SetNum(numBones);

        idList<md5meshJoint_t> md5Joints = _joints;
        key = &framedatas[base];

        for(j = 0; j < numBones; j++)
        {
            if(j == 0 && addOrigin)
                continue;

            meshJoint = &_joints[j];

            idVec3 boneOrigin;
            idQuat boneQuat;
            md5Bone = &md5Joints[j];
            int index = j;
            if(addOrigin)
                index--;
            const iqmPose_t &pose = poses[index];

            md5Bone->boneName = meshJoint->boneName;
            md5Bone->parentIndex = meshJoint->parentIndex;

            boneOrigin[0] = pose.channeloffset[0] + (pose.mask & IQM_CM_TX ? (float)(*key++) * pose.channelscale[0] : 0.0f);
            boneOrigin[1] = pose.channeloffset[1] + (pose.mask & IQM_CM_TY ? (float)(*key++) * pose.channelscale[1] : 0.0f);
            boneOrigin[2] = pose.channeloffset[2] + (pose.mask & IQM_CM_TZ ? (float)(*key++) * pose.channelscale[2] : 0.0f);

            boneQuat[0] = pose.channeloffset[3] + (pose.mask & IQM_CM_QX ? (float)(*key++) * pose.channelscale[3] : 0);
            boneQuat[1] = pose.channeloffset[4] + (pose.mask & IQM_CM_QY ? (float)(*key++) * pose.channelscale[4] : 0);
            boneQuat[2] = pose.channeloffset[5] + (pose.mask & IQM_CM_QZ ? (float)(*key++) * pose.channelscale[5] : 0);
            if(header.version == 1)
            {
                boneQuat[3] = boneQuat.CalcW();

                if(pose.mask & IQM_CM1_SX) key++;
                if(pose.mask & IQM_CM1_SY) key++;
                if(pose.mask & IQM_CM1_SZ) key++;
            }
            else
            {
                boneQuat[3] = pose.channeloffset[6] + (pose.mask & IQM_CM_QW ? (float)(*key++) * pose.channelscale[6] : 0);
                if(pose.mask & IQM_CM_SX) key++;
                if(pose.mask & IQM_CM_SY) key++;
                if(pose.mask & IQM_CM_SZ) key++;
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

#ifdef _MODEL_OBJ
bool idModelIqm::ToObj(objModel_t &objModel) const
{
    for(int i = 0; i < meshes.Num(); i++)
    {
        const iqmMesh_t *mesh = &meshes[i];
        objModel.objects.Append(new objObject_t);
        objObject_t *objObject = objModel.objects[i];
        objObject->material = GetText(mesh->material);

        for(unsigned int u = 0; u < mesh->num_triangles; u++)
        {
            const iqmTriangle_t *face = &triangles[mesh->first_triangle + u];

            int objVertIndexes[3];
            for(int k = 0; k < 3; k++)
            {
                const int vertexIndex = face->elements[k];

                objVertIndexes[k] = objObject->vertexes.Append(vertexes[vertexIndex]);
                objObject->texcoords.Append(texcoords[vertexIndex]);
                objObject->normals.Append(vertexIndex < normals.Num() ? normals[vertexIndex] : idVec3(0.0f, 0.0f, 0.0f));
            }

			objObject->indexes.Append(objVertIndexes[0]);
			objObject->indexes.Append(objVertIndexes[2]); // swap
			objObject->indexes.Append(objVertIndexes[1]);
        }
    }

    return true;
}
#endif

void idModelIqm::Print(void) const
{
    Sys_Printf(
            "IQM Header: magic=%s, version=%u, filesize=%u, flags=%x, num_text=%u, ofs_text=%u, num_meshes=%u, ofs_meshes=%u, num_vertexarrays=%u, num_vertexes=%u, ofs_vertexarrays=%u, num_triangles=%u, ofs_triangles=%u, ofs_adjacency=%u, num_joints=%u, ofs_joints=%u, num_poses=%u, ofs_poses=%u, num_anims=%u, ofs_anims=%u, num_frames=%u, num_framechannels=%u, ofs_frames=%u, ofs_bounds=%u, num_comment=%u, ofs_comment=%u, num_extensions=%u, ofs_extensions=%u\n",
            header.magic,
            header.version,
            header.filesize,
            header.flags,
            header.num_text, header.ofs_text,
            header.num_meshes, header.ofs_meshes,
            header.num_vertexarrays, header.num_vertexes, header.ofs_vertexarrays,
            header.num_triangles, header.ofs_triangles, header.ofs_adjacency,
            header.num_joints, header.ofs_joints,
            header.num_poses, header.ofs_poses,
            header.num_anims, header.ofs_anims,
            header.num_frames, header.num_framechannels, header.ofs_frames, header.ofs_bounds,
            header.num_comment, header.ofs_comment,
            header.num_extensions, header.ofs_extensions
    );
#define IQM_PART_PRINT(x) if(IsTypeMarked(x)) { Sys_Printf(#x "\n"); }
    IQM_PART_PRINT(IQM_POSITION)
    IQM_PART_PRINT(IQM_TEXCOORD)
    IQM_PART_PRINT(IQM_NORMAL)
    IQM_PART_PRINT(IQM_TANGENT)
    IQM_PART_PRINT(IQM_BLENDINDEXES)
    IQM_PART_PRINT(IQM_BLENDWEIGHTS)
    IQM_PART_PRINT(IQM_COLOR)
    IQM_PART_PRINT(IQM_CUSTOM)
    IQM_PART_PRINT(IQM_TRIANGLE)
    IQM_PART_PRINT(IQM_MESH)
    IQM_PART_PRINT(IQM_BOUNDS)
    IQM_PART_PRINT(IQM_JOINT)
    IQM_PART_PRINT(IQM_TEXT)
    IQM_PART_PRINT(IQM_ANIM)
    IQM_PART_PRINT(IQM_POSE)
#undef IQM_PART_PRINT
#define IQM_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num());   \
    if(all)                                              \
    for(int i = 0; i < list.Num(); i++) {  \
         Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__);                                \
    }                                    \
    Sys_Printf("\n------------------------------------------------------\n");
    IQM_PART_PRINT(text, texts, true, "%s, offset=%u   ", texts[i].text.c_str(), texts[i].offset)
    IQM_PART_PRINT(vertex, vertexes, false, "(%f, %f, %f)   ", vertexes[i][0], vertexes[i][1], vertexes[i][2])
    IQM_PART_PRINT(texcoord, texcoords, false, "(%f, %f)   ", texcoords[i][0], texcoords[i][1])
    IQM_PART_PRINT(normal, normals, false, "(%f, %f, %f)   ", normals[i][0], normals[i][1], normals[i][2])
    IQM_PART_PRINT(color, colors, false, "(%u, %u, %u, %u)   ", colors[i].color[0], colors[i].color[1], colors[i].color[2], colors[i].color[3])
    IQM_PART_PRINT(tangent, tangents, false, "(%f, %f, %f)   ", tangents[i][0], tangents[i][1], tangents[i][2])
    IQM_PART_PRINT(blend index, blendIndexes, false, "(%u, %u, %u, %u)   ", blendIndexes[i].index[0], blendIndexes[i].index[1], blendIndexes[i].index[2], blendIndexes[i].index[3])
    IQM_PART_PRINT(blend weight, blendWeights, false, "(%u, %u, %u, %u)   ", blendWeights[i].weight[0], blendWeights[i].weight[1], blendWeights[i].weight[2], blendWeights[i].weight[3])
    IQM_PART_PRINT(triangle, triangles, false, "(%d, %d, %d)   ", triangles[i].elements[0], triangles[i].elements[1], triangles[i].elements[2])
    IQM_PART_PRINT(mesh, meshes, true, "name=%u, material=%u, vertex=(%u, %u), triangle=(%u, %u)   ", meshes[i].name, meshes[i].material, meshes[i].first_vertex, meshes[i].num_vertexes, meshes[i].first_triangle, meshes[i].num_triangles)
    IQM_PART_PRINT(bounds, bounds, false, "min=(%f, %f, %f), max=(%f, %f, %f), xyradius=%f, radius=%f   ", bounds[i].bbmin[0], bounds[i].bbmin[1], bounds[i].bbmin[2], bounds[i].bbmax[0], bounds[i].bbmax[1], bounds[i].bbmax[2], bounds[i].xyradius, bounds[i].radius)
    IQM_PART_PRINT(joint, joints, true, "name=%u, parent=%d, translation=(%f, %f, %f), rotation=(%f, %f, %f, %f), scale=(%f, %f, %f)   ", joints[i].name, joints[i].parent, joints[i].translate[0], joints[i].translate[1], joints[i].translate[2], joints[i].rotate[0], joints[i].rotate[1], joints[i].rotate[2], joints[i].rotate[2], joints[i].scale[0], joints[i].scale[1], joints[i].scale[2])
    IQM_PART_PRINT(animation, anims, true, "name=%u, frames=(%u, %u), framerate=%f, flags=0x%x   ", anims[i].name, anims[i].first_frame, anims[i].num_frames, anims[i].framerate, anims[i].flags)

#undef IQM_PART_PRINT
}

const char * idModelIqm::GetText(unsigned int offset) const
{
    for(int i = 0; i < texts.Num(); i++)
    {
        if(texts[i].offset == offset)
            return texts[i].text.c_str();
    }
    return NULL;
}

const char * idModelIqm::GetAnim(unsigned int index) const
{
    if(index >= (unsigned int)anims.Num())
        return NULL;
    return GetText(anims[index].name);
}

int idModelIqm::GetAnimCount(void) const
{
	return header.num_anims;
}

#ifdef _MODEL_OBJ
static void R_ConvertIqmToObj_f(const idCmdArgs &args)
{
    const char *iqmPath = args.Argv(1);
    idModelIqm iqm;
    if(iqm.Parse(iqmPath, idModelIqm::PARSE_MESH))
    {
        //iqm.Print();
		objModel_t objModel;
		if(iqm.ToObj(objModel))
		{
			idStr objPath = iqmPath;
			objPath.SetFileExtension(".obj");
			OBJ_Write(&objModel, objPath.c_str());
			common->Printf("Convert obj successful: %s -> %s\n", iqmPath, objPath.c_str());
		}
		else
			common->Warning("Convert obj fail: %s", iqmPath);
    }
    else
        common->Warning("Parse iqm fail: %s", iqmPath);
}
#endif

static void ArgCompletion_iqm(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".iqm"
            , NULL);
}

static int R_ConvertIqmToMd5(const char *iqmMesh, bool doMesh = true, const idStrList *animList = NULL, float scale = -1.0f, bool addOrigin = false)
{
    int ret = 0;

    idModelIqm iqm;
    idMd5MeshFile md5MeshFile;
    bool iqmRes = false;
    if(iqm.Parse(iqmMesh, animList ? idModelIqm::PARSE_FRAME : idModelIqm::PARSE_JOINT))
    {
        //iqm.Print();
		if(iqm.ToMd5Mesh(md5MeshFile, scale, addOrigin))
		{
			if(doMesh)
			{
				md5MeshFile.Commandline().Append(va(" - %s", iqmMesh));
				idStr md5meshPath = iqmMesh;
				md5meshPath.SetFileExtension(".md5mesh");
				md5MeshFile.Write(md5meshPath.c_str());
				common->Printf("Convert md5mesh successful: %s -> %s\n", iqmMesh, md5meshPath.c_str());
				ret++;
			}
			else
			{
				common->Printf("Convert md5mesh successful: %s\n", iqmMesh);
			}
			iqmRes = true;
		}
		else
			common->Warning("Convert md5mesh fail: %s", iqmMesh);
    }
    else
        common->Warning("Parse iqm fail: %s", iqmMesh);

    if(!iqmRes)
        return ret;

    if(!animList)
        return ret;

	idStrList list;
	if(!animList->Num()) // all
	{
		for(int i = 0; i < iqm.GetAnimCount(); i++)
			list.Append(va("%d", i));
	}
	else
		list = *animList;

    for(int i = 0; i < list.Num(); i++)
    {
        const char *anim = list[i];
		bool isNumber = true;
		for(int m = 0; m < strlen(anim); m++)
		{
			if(!isdigit(anim[m]))
			{
				isNumber = false;
				break;
			}
		}
        idMd5AnimFile md5AnimFile;
		bool ok;
        const char *animName;
        idStr md5animPath;
		if(isNumber)
		{
			int index = atoi(anim);
            animName = iqm.GetAnim(index);
            if(!animName)
            {
                common->Warning("Invalid animation index '%d'", index);
                continue;
            }
            common->Printf("Convert iqm animation to md5anim: %d -> %s\n", index, animName);
			ok = iqm.ToMd5Anim(md5AnimFile, md5MeshFile, index, scale, addOrigin);
            md5animPath = iqmMesh;
            md5animPath.StripFilename();
            md5animPath.AppendPath(animName);
            md5animPath.SetFileExtension(".md5anim");
		}
		else
		{
            idStr name = anim;
            name.StripPath();
            name.StripFileExtension();
            common->Printf("Convert iqm animation to md5anim: %s -> %s\n", anim, name.c_str());
			ok = iqm.ToMd5Anim(md5AnimFile, md5MeshFile, name.c_str(), scale, addOrigin);
			animName = anim;
            md5animPath = anim;
            md5animPath.SetFileExtension(".md5anim");
		}
        if(ok)
        {
            md5AnimFile.Commandline().Append(va(" - %s", animName));
            md5AnimFile.Write(md5animPath.c_str());
            common->Printf("Convert md5anim successful: %s -> %s\n", animName, md5animPath.c_str());
            ret++;
        }
        else
            common->Warning("Convert md5anim fail: %s", animName);
    }

    return ret;
}

ID_INLINE static int R_ConvertIqmMesh(const char *iqmMesh, float scale = -1.0f, bool addOrigin = false)
{
    return R_ConvertIqmToMd5(iqmMesh, true, NULL, scale, addOrigin);
}

ID_INLINE static int R_ConvertIqmAnim(const char *iqmMesh, const idStrList &animList, float scale = -1.0f, bool addOrigin = false)
{
    return R_ConvertIqmToMd5(iqmMesh, false, &animList, scale, addOrigin);
}

ID_INLINE static int R_ConvertIqm(const char *iqmMesh, const idStrList &animList, float scale = -1.0f, bool addOrigin = false)
{
    return R_ConvertIqmToMd5(iqmMesh, true, &animList, scale, addOrigin);
}

static void R_ConvertIqmToMd5mesh_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf("Usage: %s <iqm file>\n", args.Argv(0));
        return;
    }

    const char *iqmMesh = args.Argv(1);
    R_ConvertIqmMesh(iqmMesh);
}

static void R_ConvertIqmToMd5anim_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf("Usage: %s <iqm file> <animation name or index>...\n", args.Argv(0));
        return;
    }

    const char *iqmMesh = args.Argv(1);
    idStrList animList;

    for(int i = 2; i < args.Argc(); i++)
    {
        animList.Append(args.Argv(i));
    }

    R_ConvertIqmAnim(iqmMesh, animList);
}

static void R_ConvertIqmToMd5_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf("Usage: %s <iqm file> <animation name or index>...\n", args.Argv(0));
        return;
    }

    const char *iqmMesh = args.Argv(1);
    idStrList animList;

    for(int i = 2; i < args.Argc(); i++)
    {
        animList.Append(args.Argv(i));
    }

    R_ConvertIqm(iqmMesh, animList);
}

bool R_Model_HandleIqm(const md5ConvertDef_t &convert)
{
    if(R_ConvertIqm(convert.mesh, convert.anims, convert.scale, convert.addOrigin) != 1 + convert.anims.Num())
    {
        common->Warning("Convert iqm to md5mesh/md5anim fail in entityDef '%s'", convert.def->GetName());
        return false;
    }
    return true;
}

void R_IQM_AddCommand(void)
{
    cmdSystem->AddCommand("iqmToMd5mesh", R_ConvertIqmToMd5mesh_f, CMD_FL_RENDERER, "Convert iqm to md5mesh", ArgCompletion_iqm);
    cmdSystem->AddCommand("iqmToMd5anim", R_ConvertIqmToMd5anim_f, CMD_FL_RENDERER, "Convert iqm to md5anim", ArgCompletion_iqm);
    cmdSystem->AddCommand("iqmToMd5", R_ConvertIqmToMd5_f, CMD_FL_RENDERER, "Convert iqm to md5mesh/md5anim", ArgCompletion_iqm);
#ifdef _MODEL_OBJ
    cmdSystem->AddCommand("iqmToObj", R_ConvertIqmToObj_f, CMD_FL_RENDERER, "Convert iqm to obj", ArgCompletion_iqm);
#endif
}

#undef ofs_neighbors
