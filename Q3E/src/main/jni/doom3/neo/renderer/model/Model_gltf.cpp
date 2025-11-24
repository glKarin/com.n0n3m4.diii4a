#ifdef _MODEL_OBJ
#include "Model_obj.h"
#endif
#include "Model_gltf.h"
#include "../../idlib/JSON.h"
#include "Model_md5mesh.h"

#define TEXCOORD_0 TEXCOORD_[0]
#define COLOR_0 COLOR_[0]
#define JOINTS_0 JOINTS_[0]
#define WEIGHTS_0 WEIGHTS_[0]

idModelGLTF::AccessorHelper::AccessorHelper(void)
: gltf(NULL)
{
    SetIndex(-1);
}

idModelGLTF::AccessorHelper::AccessorHelper(const idModelGLTF *p, int index)
: gltf(p)
{
    SetIndex(index);
}

idModelGLTF::AccessorHelper::AccessorHelper(const idModelGLTF *p, const idList<int> indexes)
        : gltf(p)
{
    SetIndex(indexes.Num() ? indexes[0] : -1);
}

void idModelGLTF::AccessorHelper::SetIndex(int index)
{
    accessorIndex = index;
    if(index < 0 || !gltf)
    {
        accessor = NULL;
        bufferView = NULL;
        buffer = NULL;
        ptr = NULL;
        size = 0;
    }
    else
    {
        accessor = &gltf->accessors[accessorIndex];
        bufferView = &gltf->bufferViews[accessor->bufferView];
        buffer = &gltf->buffers[bufferView->buffer];
        if(buffer->uri.Num() > 0)
            ptr = buffer->uri.Ptr() + bufferView->byteOffset;
        else
            ptr = NULL;
        size = GetComponentNum() * GetComponentSize();
    }
}

int idModelGLTF::AccessorHelper::GetComponentSize(void) const
{
    switch (accessor->componentType) {
        case GLTF_BYTE:
        case GLTF_UNSIGNED_BYTE:
            return 1;
        case GLTF_SHORT:
        case GLTF_UNSIGNED_SHORT:
            return 2;
        case GLTF_UNSIGNED_INT:
        case GLTF_FLOAT:
        default:
            return 4;
    }
}

int idModelGLTF::AccessorHelper::GetComponentNum(void) const
{
    if(!idStr::Icmp(accessor->type, GLTF_VEC2))
        return 2;
    else if(!idStr::Icmp(accessor->type, GLTF_VEC3))
        return 3;
    else if(!idStr::Icmp(accessor->type, GLTF_VEC4))
        return 4;
    else if(!idStr::Icmp(accessor->type, GLTF_MAT2))
        return 4;
    else if(!idStr::Icmp(accessor->type, GLTF_MAT3))
        return 9;
    else if(!idStr::Icmp(accessor->type, GLTF_MAT4))
        return 16;
    else // if(!idStr::Icmp(accessor->type, SCALAR))
        return 1;
}



idModelGLTF::idModelGLTF(void)
: json(NULL),
	file(NULL),
  types(0)
{}

idModelGLTF::~idModelGLTF(void)
{
    if(json)
        JSON_Free(*json);
    if(file)
        fileSystem->CloseFile(file);
}

void idModelGLTF::MarkType(int type)
{
    types |= (1 << type);
}

bool idModelGLTF::IsTypeMarked(int type) const
{
    return types & (1 << type);
}

void idModelGLTF::Clear(void)
{
    asset.version.Clear();
    asset.major_version = -1;
    asset.minor_version = -1;
    scene = -1;
    scenes.SetNum(0);
    nodes.SetNum(0);
    bufferViews.SetNum(0);
    buffers.SetNum(0);
    accessors.SetNum(0);
    materials.SetNum(0);
    meshes.SetNum(0);
    skins.SetNum(0);
    animations.SetNum(0);
    if(json)
    {
        JSON_Free(*json);
        json = NULL;
    }
    if(file)
    {
        fileSystem->CloseFile(file);
        file = NULL;
    }
    types = 0;
}

int idModelGLTF::DecodeBase64Str(const char *base64, const char *mime, idList<byte> &bytes)
{
    bytes.SetNum(0);
    idStr head("data:");
    head.Append(mime);
    head.Append(";base64,");

    if(idStr::Icmpn(base64, head.c_str(), head.Length()))
        return -1;

    idBase64 decoder(base64 + head.Length());
    int len = decoder.DecodeLength();
    bytes.SetNum(len);
    len = decoder.Decode(bytes.Ptr());
    bytes.SetNum(len);
    return len;
}

int idModelGLTF::FileToBytes(const char *path, idList<byte> &bytes)
{
    bytes.SetNum(0);
    int len = fileSystem->ReadFile(path, NULL, NULL);
    if(len <= 0)
        return -1;

    bytes.SetNum(len);
    idFile *file = fileSystem->OpenFileRead(path);
    file->Read(bytes.Ptr(), len);
    fileSystem->CloseFile(file);
    return len;
}

int idModelGLTF::ReadAsset(void)
{
    const json_t &gltf = *json;
    const json_t &object = gltf["asset"];
    if(!object)
        return -1;

    const char *version = object["version"];
    int major, minor;
    if(sscanf(version, "%d.%d", &major, &minor) != 2)
        return -1;

    asset.version = version;
    asset.full_version = atof(version);
    asset.major_version = major;
    asset.minor_version = minor;
    return 1;
}

int idModelGLTF::ReadScenes(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["scenes"];
    if(!array)
        return -1;
    scenes.SetNum(array.Length());
    for(int i = 0; i < scenes.Num(); i++)
    {
        gltfScene_t &item = scenes[i];
        const json_t &object = array[i];
        const json_t &arr = object["nodes"];
        if(arr.Length() > 0)
        {
            item.nodes.SetNum(arr.Length());
            for(int m = 0; m < item.nodes.Num(); m++)
            {
                item.nodes[m] = arr[m];
            }
        }
        item.name = object["name"];
    }
    return scenes.Num();
}

int idModelGLTF::ReadScene(void)
{
    const json_t &gltf = *json;
    scene = gltf["scene"];
    return 1;
}

int idModelGLTF::ReadNodes(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["nodes"];
    if(!array)
        return -1;
    nodes.SetNum(array.Length());
    for(int i = 0; i < nodes.Num(); i++)
    {
        gltfNode_t &item = nodes[i];
        const json_t &object = array[i];
		item.mesh = -1;
        item.skin = -1;
        item.children.SetNum(0);
		item.matrix[0] = item.matrix[5] = item.matrix[10] = item.matrix[15] = 1.0f;
		item.matrix[1] = item.matrix[2] = item.matrix[3] = 
		item.matrix[4] = item.matrix[6] = item.matrix[7] = 
		item.matrix[8] = item.matrix[9] = item.matrix[11] = 
		item.matrix[12] = item.matrix[13] = item.matrix[14] = 0.0f;
        if(object["matrix"].Length() == 16)
        {
            const json_t &arr = object["matrix"];
            for(int m = 0; m < 16; m++)
            {
                item.matrix[m] = arr[m];
            }
        }
		item.rotation[0] = item.rotation[1] = item.rotation[2] = 0.0f;
		item.rotation[3] = 1.0f;
        if(object["rotation"].Length() == 4)
        {
            const json_t &arr = object["rotation"];
            for(int m = 0; m < 4; m++)
            {
                item.rotation[m] = arr[m];
            }
        }
		item.scale[0] = item.scale[1] = item.scale[2] = 1.0f;
        if(object["scale"].Length() == 3)
        {
            const json_t &arr = object["scale"];
            for(int m = 0; m < 3; m++)
            {
                item.scale[m] = arr[m];
            }
        }
		item.translation[0] = item.translation[1] = item.translation[2] = 0.0f;
        if(object["translation"].Length() == 3)
        {
            const json_t &arr = object["translation"];
            for(int m = 0; m < 3; m++)
            {
                item.translation[m] = arr[m];
            }
        }
        if(!object["mesh"].IsNull())
            item.mesh = object["mesh"];
        if(!object["skin"].IsNull())
            item.skin = object["skin"];
        const json_t &children = object["children"];
        if(children.Length())
        {
            item.children.SetNum(children.Length());
            for(int m = 0; m < item.children.Num(); m++)
                item.children[m] = children[m];
        }
        item.name = object["name"];
    }
    return nodes.Num();
}

int idModelGLTF::ReadBufferViews(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["bufferViews"];
    if(!array)
        return -1;
    bufferViews.SetNum(array.Length());
    for(int i = 0; i < bufferViews.Num(); i++)
    {
        gltfBufferView_t &item = bufferViews[i];
        const json_t &object = array[i];
        item.name = object["name"];
        item.buffer = object["buffer"];
        item.byteOffset = object["byteOffset"];
        item.byteLength = object["byteLength"];
        item.byteStride = object["byteStride"];
        item.target = object["target"];
    }
    return bufferViews.Num();
}

int idModelGLTF::ReadBuffers(const char *filePath)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["buffers"];
    if(!array)
        return -1;
    buffers.SetNum(array.Length());
    for(int i = 0; i < buffers.Num(); i++)
    {
        gltfBuffer_t &item = buffers[i];
        const json_t &object = array[i];
        item.name = object["name"];
        item.byteLength = object["byteLength"];
        const char *uri = object["uri"];
		if(uri)
		{
			const char * base64Heads[] = {
				"application/octet-stream",
				"application/gltf-buffer",
			};
			int m;
			// 1. load built-in base64 data
			for(m = 0; m < sizeof(base64Heads) / sizeof(base64Heads[0]); m++)
			{
				if(DecodeBase64Str(uri, base64Heads[m], item.uri) > 0)
					break;
			}
			// 2. load absolute path bin
			if(m >= sizeof(base64Heads) / sizeof(base64Heads[0]))
			{
				if(FileToBytes(uri, item.uri) <= 0)
				{
					// 3. load absolute path bin
					if(filePath)
					{
						idStr path(filePath);
						path.StripFilename();
						path.AppendPath(uri);
						common->Warning("Load GLTF buffer absolute uri file fail: '%s', try relative path: '%s'", uri, path.c_str());
						if(FileToBytes(path.c_str(), item.uri) <= 0)
							common->Warning("Load GLTF buffer relative uri file fail: %s", path.c_str());
					}
					else
						common->Warning("Load GLTF buffer absolute uri file fail: %s", uri);
				}
			}
		}
		//else
			item.uri.SetNum(item.byteLength);
	}
    return buffers.Num();
}

int idModelGLTF::ReadAccessors(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["accessors"];
    if(!array)
        return -1;
    accessors.SetNum(array.Length());
    for(int i = 0; i < accessors.Num(); i++)
    {
        gltfAccessor_t &item = accessors[i];
        const json_t &object = array[i];
        item.bufferView = object["bufferView"];
        item.byteOffset = object["byteOffset"];
        item.componentType = object["componentType"];
        item.normalized = object["normalized"];
        item.count = object["count"];
        item.type = object["type"];
        item.name = object["name"];
    }
    return accessors.Num();
}

int idModelGLTF::ReadMaterials(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["materials"];
    if(!array)
        return -1;
    materials.SetNum(array.Length());
    for(int i = 0; i < materials.Num(); i++)
    {
        gltfMaterial_t &item = materials[i];
        const json_t &object = array[i];
        item.name = object["name"];
    }
    return materials.Num();
}

void idModelGLTF::ReadPrimitives(const json_t &object, gltfPrimitive_t &primitive)
{
	const json_t &attr = object["attributes"];
    primitive.mode = GLTF_TRIANGLES;
    primitive.attributes.POSITION = -1;
    primitive.attributes.NORMAL = -1;
    primitive.attributes.TANGENT = -1;
    primitive.attributes.TEXCOORD_.SetNum(0);
    primitive.attributes.COLOR_.SetNum(0);
    primitive.attributes.JOINTS_.SetNum(0);
    primitive.attributes.WEIGHTS_.SetNum(0);
    {
        const json_t &number = object["mode"];
        if(!number.IsNull())
            primitive.mode = number;
    }
    {
        const json_t &number = attr["POSITION"];
        if(!number.IsNull())
            primitive.attributes.POSITION = number;
    }
    {
        const json_t &number = attr["NORMAL"];
        if(!number.IsNull())
            primitive.attributes.NORMAL = number;
    }
    {
        const json_t &number = attr["TANGENT"];
        if(!number.IsNull())
            primitive.attributes.TANGENT = number;
    }
    {
        const json_t &number = attr["TEXCOORD_0"];
        if(!number.IsNull())
        {
            primitive.attributes.TEXCOORD_.SetNum(1);
            primitive.attributes.TEXCOORD_[0] = number;
        }
    }
    {
        const json_t &number = attr["COLOR_0"];
        if(!number.IsNull())
        {
            primitive.attributes.COLOR_.SetNum(1);
            primitive.attributes.COLOR_[0] = number;
        }
    }
    {
        const json_t &number = attr["JOINTS_0"];
        if(!number.IsNull())
        {
            primitive.attributes.JOINTS_.SetNum(1);
            primitive.attributes.JOINTS_[0] = number;
        }
    }
    {
        const json_t &number = attr["WEIGHTS_0"];
        if(!number.IsNull())
        {
            primitive.attributes.WEIGHTS_.SetNum(1);
            primitive.attributes.WEIGHTS_[0] = number;
        }
    }
	primitive.indices = object["indices"];
	primitive.material = object["material"];
}

int idModelGLTF::ReadMeshes(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["meshes"];
    if(!array)
        return -1;
    meshes.SetNum(array.Length());
    for(int i = 0; i < meshes.Num(); i++)
    {
        gltfMesh_t &item = meshes[i];
        const json_t &object = array[i];
		const json_t &plist = object["primitives"];
		item.primitives.SetNum(plist.Length());
		for(int m = 0; m < item.primitives.Num(); m++)
			ReadPrimitives(plist[m], item.primitives[m]);
    }
    return meshes.Num();
}

int idModelGLTF::ReadSkins(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["skins"];
    if(!array)
        return -1;
    skins.SetNum(array.Length());
    for(int i = 0; i < skins.Num(); i++)
    {
        gltfSkin_t &item = skins[i];
        const json_t &object = array[i];
        item.inverseBindMatrices = object["inverseBindMatrices"];
        item.skeleton = object["skeleton"];
        item.name = object["name"];
        const json_t &plist = object["joints"];
        item.joints.SetNum(plist.Length());
        for(int m = 0; m < item.joints.Num(); m++)
            item.joints[m] = plist[m];
    }
    return skins.Num();
}

int idModelGLTF::ReadChannels(const json_t &array, idList<gltfChannel_t> &channels)
{
    if(!array)
        return -1;
    channels.SetNum(array.Length());
    for(int i = 0; i < channels.Num(); i++)
    {
        gltfChannel_t &item = channels[i];
        const json_t &object = array[i];
        item.sampler = object["sampler"];
        const json_t &target = object["target"];
        item.target.path = target["path"];
        item.target.node = target["node"];
    }
    return channels.Num();
}

int idModelGLTF::ReadSamplers(const json_t &array, idList<gltfSampler_t> &samplers)
{
    if(!array)
        return -1;
    samplers.SetNum(array.Length());
    for(int i = 0; i < samplers.Num(); i++)
    {
        gltfSampler_t &item = samplers[i];
        const json_t &object = array[i];
        item.input = object["input"];
        item.output = object["output"];
        item.interpolation = object["interpolation"];
    }
    return samplers.Num();
}

int idModelGLTF::ReadAnimations(void)
{
    const json_t &gltf = *json;
    const json_t &array = gltf["animations"];
    if(!array)
        return -1;
    animations.SetNum(array.Length());
    for(int i = 0; i < animations.Num(); i++)
    {
        gltfAnimation_t &item = animations[i];
        const json_t &object = array[i];
        ReadChannels(object["channels"], item.channels);
        ReadSamplers(object["samplers"], item.samplers);
        item.name = object["name"];
    }
    return animations.Num();
}

int idModelGLTF::ReadHeader(glbHeader_t &header)
{
    int num = file->Length() - file->Tell();
    if(num < 12)
    {
        common->Warning("Unexpected end of file(%d/12 bytes).", num);
        return -1;
    }

    memset(&header, 0, sizeof(header));
    file->ReadUnsignedInt(header.magic);
    file->ReadUnsignedInt(header.version);
    file->ReadUnsignedInt(header.length);

    return 12;
}

int idModelGLTF::ReadChunk(glbChunk_t &chunk, int mask)
{
	if(mask <= 0)
		mask = 0xFF;

	int res = 0;
	if(mask & 1)
	{
		file->ReadUnsignedInt(chunk.chunkLength);
		file->ReadUnsignedInt(chunk.chunkType);
		res += 8;
	}
	if(mask & 2)
	{
		chunk.chunkData.SetNum(chunk.chunkLength);
		res += file->Read(chunk.chunkData.Ptr(), chunk.chunkLength);
	}
	return res;
}

bool idModelGLTF::ParseMemory(idLexer &lexer, int parseType, const char *filePath)
{
	if(!filePath)
		filePath = "<implicit GLTF file>";

	bool err = false;

    json_t gltf;
    JSON_Init(gltf);
    if(!JSON_Parse(gltf, lexer))
    {
        common->Warning("Parse GLTF json file error: %s", filePath);
        return false;
    }

    if(parseType == PARSE_DEF)
        parseType = PARSE_FRAME;

    json = &gltf;

	do
	{
		if(ReadAsset() > 0)
			MarkType(GLTF_ASSET);
		else
		{
			common->Warning("Read GLTF assets error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadScenes() > 0)
			MarkType(GLTF_SCENES);
		else
		{
			common->Warning("Read GLTF scenes error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadScene() > 0)
			MarkType(GLTF_SCENE);
		else
		{
			common->Warning("Read GLTF scene error in '%s'", filePath);
			scene = 0;
		}

		if(ReadNodes() > 0)
			MarkType(GLTF_NODE);
		else
		{
			common->Warning("Read GLTF nodes error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadBufferViews() > 0)
			MarkType(GLTF_BUFFERVIEW);
		else
		{
			common->Warning("Read GLTF bufferViews error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadBuffers(filePath) > 0)
			MarkType(GLTF_BUFFER);
		else
		{
			common->Warning("Read GLTF buffers error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadAccessors() > 0)
			MarkType(GLTF_ACCESSOR);
		else
		{
			common->Warning("Read GLTF accessors error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadMaterials() > 0)
			MarkType(GLTF_MATERIAL);
		else
		{
			common->Warning("Read GLTF materials error in '%s'", filePath);
			err = true;
			break;
		}

		if(ReadMeshes() > 0)
			MarkType(GLTF_MESH);
		else
		{
			common->Warning("Read GLTF meshes error in '%s'", filePath);
			err = true;
			break;
		}

		// joints/animations
        if(parseType >= PARSE_JOINT)
        {
            if(ReadSkins() > 0)
                MarkType(GLTF_SKIN);
        }

        if(parseType >= PARSE_FRAME)
        {
            if(ReadAnimations() > 0)
                MarkType(GLTF_ANIMATION);
        }
	}
	while(false);

    JSON_Free(gltf);
    json = NULL;

    return !err;
}

bool idModelGLTF::ParseGLTF(const char *filePath, int parseType)
{
    Clear();

	idLexer lexer;

	if(!lexer.LoadFile(filePath))
	{
		common->Warning("Load GLTF json fail: %s", filePath);
		return false;
	}

	bool ok = ParseMemory(lexer, parseType, filePath);
	if(!ok)
		Clear();

	return ok;
}

bool idModelGLTF::ParseGLB(const char *filePath, int parseType)
{
	Clear();

    file = fileSystem->OpenFileRead(filePath);
    if(!file)
	{
		common->Warning("Load GLB file fail: %s", filePath);
        return false;
	}

	bool err = false;

	do
	{
		// header
		glbHeader_t header;
		if(ReadHeader(header) <= 0)
		{
			common->Warning("Read GLB header error in '%s'", filePath);
			err = true;
			break;
		}

		if(header.magic != GLB_MAGIC)
		{
			common->Warning("GLB header magic not match in '%s': 0x%X != 0x%X", filePath, header.magic, GLB_MAGIC);
			err = true;
			break;
		}
		if(header.length != file->Length())
		{
			common->Warning("GLB header length not match in '%s': %d != %d", filePath, header.length, file->Length());
			err = true;
			break;
		}
		
		// json chunk
		glbChunk_t chunk;
		if(ReadChunk(chunk, 1) <= 0)
		{
			common->Warning("Read GLB json chunk error in '%s'", filePath);
			err = true;
			break;
		}

		if(chunk.chunkType != GLB_CHUNK_TYPE_JSON)
		{
			common->Warning("GLB first chunk is not json in '%s': 0x%X != 0x%X", filePath, chunk.chunkType, GLB_CHUNK_TYPE_JSON);
			err = true;
			break;
		}

		if(ReadChunk(chunk, 2) <= 0)
		{
			common->Warning("Read GLB json chunk data error in '%s'", filePath);
			err = true;
			break;
		}
		int length = chunk.chunkLength;
		while(length > 0)
		{
			if(chunk.chunkData[length - 1] != 0x20)
				length--;
			else
				break;
		}

#if 0 // output gltf
		idStr out = filePath;
		out.SetFileExtension(".gltf");
		json_t j;
		JSON_Parse(j, (const char *)chunk.chunkData.Ptr(), length);
		idList<char> src;
		JSON_ToArray(src, j);
		fileSystem->WriteFile(out, src.Ptr(), src.Num());
#endif

		idLexer lexer;
		if(!lexer.LoadMemory((const char *)chunk.chunkData.Ptr(), length, filePath))
		{
			common->Warning("Load GLB json chunk data error in '%s'", filePath);
			err = true;
			break;
		}
		if(!ParseMemory(lexer, parseType, filePath))
		{
			err = true;
			break;
		}

		// bin
		if(ReadChunk(chunk, 1) <= 0)
		{
			common->Warning("Read GLB bin chunk error in '%s'", filePath);
			err = true;
			break;
		}

		if(chunk.chunkType != GLB_CHUNK_TYPE_BIN)
		{
			common->Warning("GLB second chunk is not bin in '%s': 0x%X != 0x%X", filePath, chunk.chunkType, GLB_CHUNK_TYPE_BIN);
			err = true;
			break;
		}

		if(ReadChunk(chunk, 2) <= 0)
		{
			common->Warning("Read GLB bin chunk data error in '%s'", filePath);
			err = true;
			break;
		}

		gltfBuffer_t &buffer0 = buffers[0];
		buffer0.uri = chunk.chunkData;
		buffer0.uri.SetNum(buffer0.byteLength);
	} while(false);

    if(err)
        Clear();
	else
	{
		fileSystem->CloseFile(file);
		file = NULL;
	}

    return !err;
}

bool idModelGLTF::Parse(const char *filePath, int parseType)
{
    idStr extension;
    idStr(filePath).ExtractFileExtension(extension);

    if (extension.Icmp("glb") == 0) {
        return ParseGLB(filePath, parseType);
    } else {
        return ParseGLTF(filePath, parseType);
    }
}

int idModelGLTF::FindMeshNode(const idList<int> &nodeIds) const
{
    for(int m = 0; m < nodeIds.Num(); m++)
    {
        const gltfNode_t &node = nodes[nodeIds[m]];
        if(node.mesh >= 0)
			return nodeIds[m];
		if(node.children.Num() > 0)
		{
			int nodeIndex = FindMeshNode(node.children);
			if(nodeIndex >= 0)
				return nodeIndex;
		}
    }
    return -1;
}

int idModelGLTF::FindMeshNode(void) const
{
    const gltfScene_t &curScene = scenes[scene];

    return FindMeshNode(curScene.nodes);
}

const char * idModelGLTF::FindParentNode(int index) const
{
    const gltfNode_t *node;
    int i;
    for (i = 0, node = &nodes[0]; i < nodes.Num(); i++, node++)
    {
        for (int m = 0; m < node->children.Num(); m++)
        {
            if(node->children[m] == index)
                return node->name.c_str();
        }
    }
    return NULL;
}

bool idModelGLTF::ToMd5Mesh(idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *meshOffset, const idMat3 *meshRotation) const
{
    int i, j;
    md5meshJoint_t *md5Bone;
    const gltfNode_t *refBone;
    idVec3 boneOrigin;
    idQuat boneQuat;
    const md5meshJointTransform_t *jointTransform;
    const gltfPrimitive_t *primitive;

	int meshNodeIndex = FindMeshNode();
    if(meshNodeIndex < 0)
	{
		common->Warning("Can't find mesh node in scene %d", scene);
        return false;
	}
    const gltfNode_t &meshNode = nodes[meshNodeIndex];
	if(meshNode.skin < 0)
	{
		common->Warning("Mesh node '%d' no skin", meshNodeIndex);
        return false;
	}

    const gltfSkin_t &skin = skins[meshNode.skin];

	int numBones = skin.joints.Num();

    md5mesh.Commandline() = va("Convert from khronos gltf/glb file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
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

    idHashTable<int> jointMap;
    for (i = 0; i < skin.joints.Num(); i++)
    {
        int boneIndex = skin.joints[i];
        refBone = &nodes[boneIndex];
        jointMap.Set(refBone->name, i);
    }

    for (i = 0; i < skin.joints.Num(); i++, md5Bone++)
    {
        int boneIndex = skin.joints[i];
        refBone = &nodes[boneIndex];
        md5Bone->boneName = refBone->name;
		bool isRoot = false;

        if (i == 0)
        {
            md5Bone->parentIndex = -1;
			isRoot = true;
        }
        else
        {
            int index = 0;
            int *rindex = &index;
			if(jointMap.Get(FindParentNode(boneIndex), &rindex))
				md5Bone->parentIndex = *rindex;
			else
			{
				common->Warning("Has more root bone: %d", boneIndex);
				md5Bone->parentIndex = addOrigin ? -1 : 0;
				isRoot = true;
			}
		}

		if(addOrigin)
			md5Bone->parentIndex += 1;

        boneOrigin[0] = refBone->translation[0];
        boneOrigin[1] = refBone->translation[1];
        boneOrigin[2] = refBone->translation[2];

        boneQuat[0] = refBone->rotation[0];
        boneQuat[1] = refBone->rotation[1];
        boneQuat[2] = refBone->rotation[2];
        boneQuat[3] = refBone->rotation[3];

		if (isRoot)
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

            idMat3 mat = parent->orient.ToMat3();
#if ETW_PSK
            rotated = m.TransposeSelf() * md5Bone->pos;

            quat = parent->orient * md5Bone->orient;
#else
            rotated = mat * md5Bone->pos;

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

    const gltfScene_t &curScene = scenes[scene];

	const gltfMesh_t &mesh = meshes[meshNode.mesh];

	for(i = 0, primitive = &mesh.primitives[0]; i < mesh.primitives.Num(); i++, primitive++)
	{
		if(primitive->mode != GLTF_TRIANGLES) // only handle triangles
			continue;
		if(primitive->attributes.POSITION < 0) // no vertex
		{
			common->Warning("Mesh primitive '%d' no vertex", i);
			continue;
		}
		if(primitive->indices < 0) // no index
		{
			common->Warning("Mesh primitive '%d' no index", i);
			continue;
		}

		AccessorHelper indexAccessor(this, primitive->indices);
		if(!indexAccessor.ptr)
		{
			common->Warning("Index buffer'%d' is empty", indexAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper vertexAccessor(this, primitive->attributes.POSITION);
		if(!vertexAccessor.ptr)
		{
			common->Warning("Vertex buffer'%d' is empty", vertexAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper jointAccessor(this, primitive->attributes.JOINTS_);
		if(!jointAccessor.ptr)
		{
			common->Warning("Joint buffer'%d' is empty", jointAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper texcoordAccessor(this, primitive->attributes.TEXCOORD_);
		if(primitive->attributes.TEXCOORD_.Num() > 0 && primitive->attributes.TEXCOORD_0 >= 0 && !texcoordAccessor.ptr)
		{
			common->Warning("Texcoord0 buffer'%d' is empty", texcoordAccessor.bufferView->buffer);
		}

		AccessorHelper weightAccessor(this, primitive->attributes.WEIGHTS_0);
		if(primitive->attributes.WEIGHTS_.Num() > 0 && primitive->attributes.WEIGHTS_0 >= 0 && !weightAccessor.ptr)
		{
			common->Warning("Weight buffer'%d' is empty", weightAccessor.bufferView->buffer);
		}

		int index = md5Meshes.Append(md5meshMesh_t());
		md5meshMesh_t &mesh = md5Meshes[index];
		mesh.shader = materials[primitive->material].name;

		idList<unsigned int> md5Vertexes; // cache saved vert for remove dup vert
		for(j = 0; j < indexAccessor.Count(); j+=3)
		{
			unsigned int vertexIndexes[3];
			indexAccessor.ToArray<unsigned int>(j, vertexIndexes, 3);

			int md5VertIndexes[3];
			for(int k = 0; k < 3; k++)
			{
				const unsigned int vertexIndex = vertexIndexes[k];

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

				const float *vf = vertexAccessor.Offset<float>(vertexIndex);
				idVec3 pos(vf[0], vf[1], vf[2]);

				idVec2 st;
				if(texcoordAccessor.ptr)
				{
					const float *tf = texcoordAccessor.Offset<float>(vertexIndex);
					st.Set(tf[0], tf[1]);
				}
				else
				{
					st.Set(0.0f, 0.0f);
				}

				idList<int> jointIndexes;
				jointAccessor.ToList(vertexIndex, jointIndexes);

				int numLink = 0;
				idList<float> jointWeights;
				jointWeights.SetNum(jointIndexes.Num());
				const float *wf = weightAccessor.Offset<float>(vertexIndex);
				for(int o = 0; o < jointWeights.Num(); o++)
				{
					if(wf)
					{
						if(wf[o] == 0.0f)
							break;
						jointWeights[o] = wf[o];
					}
					else
					{
						jointWeights[o] = 1.0f / (float)jointWeights.Num();
					}
					numLink++;
				}

				md5meshVert_t md5Vert;
				md5Vert.uv = st;
				md5Vert.weightIndex = mesh.weights.Num();
				md5Vert.weightElem = numLink;
				md5VertIndexes[k] = mesh.verts.Append(md5Vert); // Add vert
				md5Vertexes.Append(vertexIndex); // cache vert

				if(meshRotation && !meshRotation->IsIdentity())
					pos *= *meshRotation;
				if(meshOffset && !meshOffset->IsZero())
					pos += *meshOffset;
				if(scale > 0.0f)
					pos *= scale;

				float w = 0.0f;
				for(int o = 0; o < numLink; o++)
				{
					md5meshWeight_t md5Weight;
					md5Weight.jointIndex = jointIndexes[o];
					if(addOrigin)
						md5Weight.jointIndex += 1;
					jointTransform = &jointTransforms[md5Weight.jointIndex];
					if(numLink == 1)
					{
						if(jointWeights[o] != 1.0f)
							common->Warning("wedge '%d' only 1 bone '%s' but weight is not 1 '%f'", vertexIndex, nodes[jointIndexes[0]].name.c_str(), jointWeights[o]);
						md5Weight.weightValue = 1.0f;
					}
					else
						md5Weight.weightValue = jointWeights[o];

					w += md5Weight.weightValue;
					jointTransform->bindmat.ProjectVector(pos - jointTransform->bindpos, md5Weight.pos);

					mesh.weights.Append(md5Weight); // Add weight
				}
				if(WEIGHTS_SUM_NOT_EQUALS_ONE(w))
				{
					common->Warning("Vertex '%d' weight sum is less than 1.0: %f", vertexIndex, w);
				}
			}

			md5meshTri_t md5Tri;
			md5Tri.vertIndex1 = md5VertIndexes[0];
			md5Tri.vertIndex2 = md5VertIndexes[2];
			md5Tri.vertIndex3 = md5VertIndexes[1];
			mesh.tris.Append(md5Tri); // Add tri
		}
	}

    return true;
}

bool idModelGLTF::ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, int animIndex, float scale, bool addOrigin, const idVec3 *animOffset, const idMat3 *animRotation) const
{
	if(animIndex >= animations.Num())
		return false;

    int i, j;
    md5animHierarchy_t *md5Hierarchy;
    const md5meshJoint_t *meshJoint;
    md5animFrame_t *md5Frame;
    md5meshJointTransform_t *jointTransform;
    md5meshJointTransform_t *frameTransform;
    md5animBaseframe_t *md5BaseFrame;
    md5meshJoint_t *md5Bone;
	const gltfChannel_t *channel;
	const gltfNode_t *node;

    const gltfAnimation_t *animInfo = &animations[animIndex];
	const idList<gltfChannel_t> &channels = animInfo->channels;
	const idList<gltfSampler_t> &samplers = animInfo->samplers;

	// using first sampler input's count as frame count
	int numFrames = accessors[samplers[0].input].count;

    const idList<md5meshJoint_t> _joints = md5mesh.Joints();
    const int numBones = _joints.Num();

    md5anim.FrameRate() = MD5ANIM_DEFAULT_FRAMERATE;
    md5anim.NumAnimatedComponents() = numBones * 6;

    md5anim.Commandline() = va("Convert from gltf/glb file: scale=%f, addOrigin=%d", scale > 0.0f ? scale : 1.0, addOrigin);
	if(animOffset)
		md5anim.Commandline().Append(va(", offset=%g,%g,%g", animOffset->x, animOffset->y, animOffset->z));
	if(animRotation)
	{
		idAngles angle = animRotation->ToAngles();
		md5anim.Commandline().Append(va(", rotation=%g %g %g", angle[0], angle[1], angle[2]));
	}

	idHashTable<const gltfNode_t *> defaultKeys;
	for(i = 0, node = &nodes[0]; i < nodes.Num(); i++, node++)
	{
		if(!node->name.IsEmpty() && node->mesh < 0)
			defaultKeys.Set(node->name, node);
	}

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
    md5Bounds.SetNum(numFrames);
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
    md5Frames.SetNum(numFrames);

	idHashTable<BoneAccessor> keys;

	for(i = 0, channel = &channels[0]; i < channels.Num(); i++, channel++)
	{
		bool isTranslation = idStr::Icmp(channel->target.path, GLTF_TRANSLATION) == 0;
		bool isRotation = idStr::Icmp(channel->target.path, GLTF_ROTATION) == 0;
		if(!isTranslation && !isRotation) // don't handle scale
			continue;

		BoneAccessor *ba;
		const char *name = nodes[channel->target.node].name.c_str();
		const gltfSampler_t &sampler = samplers[channel->sampler];
		AccessorHelper ah(this, sampler.output);
		if(keys.Get(name, &ba))
		{
			if(isTranslation)
				ba->translation = ah;
			else
				ba->rotation = ah;
		}
		else
		{
			BoneAccessor a;
			if(isTranslation)
				a.translation = ah;
			else
				a.rotation = ah;
			keys.Set(name, a);
		}
	}

    for(i = 0; i < numFrames; i++)
    {
        md5animFrames_t &frames = md5Frames[i];
        frames.index = i;
        frames.joints.SetNum(numBones);

        idList<md5meshJoint_t> md5Joints = _joints;

        for(j = 0; j < numBones; j++)
        {
            if(j == 0 && addOrigin)
                continue;

            md5Bone = &md5Joints[j];
            meshJoint = &_joints[j];
            md5Bone->boneName = meshJoint->boneName;
            md5Bone->parentIndex = meshJoint->parentIndex;

            idVec3 boneOrigin;
            idQuat boneQuat;
			boneOrigin.Zero();
			boneQuat[0] = 0.0f;
			boneQuat[1] = 0.0f;
			boneQuat[2] = 0.0f;
			boneQuat[3] = 1.0f;
            
			BoneAccessor *ba;
			if(keys.Get(md5Bone->boneName, &ba))
			{
				const float *tptr = ba->translation.ptr && i < ba->translation.Count() ? ba->translation.Offset<float>(i) : NULL;
				if(tptr)
				{
					boneOrigin[0] = tptr[0];
					boneOrigin[1] = tptr[1];
					boneOrigin[2] = tptr[2];
				}

				const float *rptr = ba->rotation.ptr && i < ba->rotation.Count() ? ba->rotation.Offset<float>(i) : NULL;
				if(rptr)
				{
					boneQuat[0] = rptr[0];
					boneQuat[1] = rptr[1];
					boneQuat[2] = rptr[2];
					boneQuat[3] = rptr[3];
				}
			}
			else
			{
				if(i == 0)
					common->Warning("Bone not found: %s", md5Bone->boneName.c_str());

				const gltfNode_t **dnode;
				if(defaultKeys.Get(md5Bone->boneName, &dnode))
				{
					boneOrigin[0] = (*dnode)->translation[0];
					boneOrigin[1] = (*dnode)->translation[1];
					boneOrigin[2] = (*dnode)->translation[2];

					boneQuat[0] = (*dnode)->rotation[0];
					boneQuat[1] = (*dnode)->rotation[1];
					boneQuat[2] = (*dnode)->rotation[2];
					boneQuat[3] = (*dnode)->rotation[3];
				}
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

bool idModelGLTF::ToMd5Anim(idMd5AnimFile &md5anim, idMd5MeshFile &md5mesh, const char *animName, float scale, bool addOrigin, const idVec3 *offset, const idMat3 *rotation) const
{
    for(int i = 0; i < animations.Num(); i++)
    {
        if(!idStr::Icmp(animName, animations[i].name))
        {
            return ToMd5Anim(md5anim, md5mesh, i, scale, addOrigin, offset, rotation);
        }
    }
    common->Warning("Animation '%s' not found in gltf/glb", animName);
    return false;
}

int idModelGLTF::ToMd5AnimList(idList<idMd5AnimFile> &md5anims, idMd5MeshFile &md5mesh, float scale, bool addOrigin, const idVec3 *offset, const idMat3 *rotation) const
{
    int num = 0;

    for(int i = 0; i < animations.Num(); i++)
    {
        int index = md5anims.Append(idMd5AnimFile());
        if(!ToMd5Anim(md5anims[index], md5mesh, i, scale, addOrigin, offset, rotation))
        {
            md5anims.RemoveIndex(index);
            common->Warning("Animation '%s' convert fail", animations[i].name.c_str());
        }
        else
            num++;
    }
    return num;
}

int idModelGLTF::GroupTriangle(idList<idList<idDrawVert> > &verts, idList<idList<int> > &faces, idStrList &mats, bool keepDup) const
{
    int i, j;
    const gltfPrimitive_t *primitive;
    int num;

	int meshNodeIndex = FindMeshNode();
    if(meshNodeIndex < 0)
	{
		common->Warning("Can't find mesh node in scene %d", scene);
        return 0;
	}

	num = 0;

    const gltfNode_t &meshNode = nodes[meshNodeIndex];
	const gltfMesh_t &mesh = meshes[meshNode.mesh];

	for(i = 0, primitive = &mesh.primitives[0]; i < mesh.primitives.Num(); i++, primitive++)
	{
		if(primitive->mode != GLTF_TRIANGLES) // only handle triangles
			continue;
		if(primitive->attributes.POSITION < 0) // no vertex
		{
			common->Warning("Mesh primitive '%d' no vertex", i);
			continue;
		}
		if(primitive->indices < 0) // no index
		{
			common->Warning("Mesh primitive '%d' no index", i);
			continue;
		}

		AccessorHelper indexAccessor(this, primitive->indices);
		if(!indexAccessor.ptr)
		{
			common->Warning("Index buffer'%d' is empty", indexAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper vertexAccessor(this, primitive->attributes.POSITION);
		if(!vertexAccessor.ptr)
		{
			common->Warning("Vertex buffer'%d' is empty", vertexAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper normalAccessor(this, primitive->attributes.NORMAL);
		if(primitive->attributes.NORMAL >= 0 && !normalAccessor.ptr)
		{
			common->Warning("Normal buffer'%d' is empty", normalAccessor.bufferView->buffer);
		}

		AccessorHelper texcoordAccessor(this, primitive->attributes.TEXCOORD_);
		if(primitive->attributes.TEXCOORD_.Num() > 0 && primitive->attributes.TEXCOORD_0 >= 0 && !texcoordAccessor.ptr)
		{
			common->Warning("Texcoord0 buffer'%d' is empty", texcoordAccessor.bufferView->buffer);
		}

		int index = verts.Append(idList<idDrawVert>());
		faces.Append(idList<int>());
		mats.Append(materials[primitive->material].name);
		idList<idDrawVert> &list = verts[index];
		idList<int> &face = faces[index];
		num++;

		idList<unsigned int> objVertexes; // cache saved vert for remove dup vert

		for(j = 0; j < indexAccessor.Count(); j+=3)
		{
			unsigned int vertexIndexes[3];
			indexAccessor.ToArray<unsigned int>(j, vertexIndexes, 3);

			int objVertIndexes[3];
			for(int k = 0; k < 3; k++)
			{
				const unsigned int vertexIndex = vertexIndexes[k];

				// find vert is cached
				if(!keepDup)
				{
					int n;
					for(n = 0; n < objVertexes.Num(); n++)
					{
						if(vertexIndex == objVertexes[n])
							break;
					}
					if(n < objVertexes.Num())
					{
						objVertIndexes[k] = n;
						continue;
					}
					objVertexes.Append(vertexIndex); // cache vert
				}

				objVertIndexes[k] = list.Append(idDrawVert());
				idDrawVert &vert = list[objVertIndexes[k]];

				const float *vf = vertexAccessor.Offset<float>(vertexIndex);
				vert.xyz.Set(vf[0], vf[1], vf[2]);

				if(texcoordAccessor.ptr)
				{
					const float *tf = texcoordAccessor.Offset<float>(vertexIndex);
					vert.st.Set(tf[0], tf[1]);
				}
				else
				{
					vert.st.Set(0.0f, 0.0f);
				}

				if(normalAccessor.ptr)
				{
					const float *nf = normalAccessor.Offset<float>(vertexIndex);
					vert.normal.Set(nf[0], nf[1], nf[2]);
				}
				else
				{
					vert.normal.Set(0.0f, 0.0f, 0.0f);
				}
			}

			face.Append(objVertIndexes[0]);
			face.Append(objVertIndexes[1]);
			face.Append(objVertIndexes[2]);
		}
	}

    return num;
}

#ifdef _MODEL_OBJ
bool idModelGLTF::ToObj(objModel_t &objModel, bool keepDup) const
{
    int i, j;
    const gltfPrimitive_t *primitive;

	int meshNodeIndex = FindMeshNode();
    if(meshNodeIndex < 0)
	{
		common->Warning("Can't find mesh node in scene %d", scene);
        return false;
	}

    const gltfNode_t &meshNode = nodes[meshNodeIndex];
	const gltfMesh_t &mesh = meshes[meshNode.mesh];

	for(i = 0, primitive = &mesh.primitives[0]; i < mesh.primitives.Num(); i++, primitive++)
	{
		if(primitive->mode != GLTF_TRIANGLES) // only handle triangles
			continue;
		if(primitive->attributes.POSITION < 0) // no vertex
		{
			common->Warning("Mesh primitive '%d' no vertex", i);
			continue;
		}
		if(primitive->indices < 0) // no index
		{
			common->Warning("Mesh primitive '%d' no index", i);
			continue;
		}

		AccessorHelper indexAccessor(this, primitive->indices);
		if(!indexAccessor.ptr)
		{
			common->Warning("Index buffer'%d' is empty", indexAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper vertexAccessor(this, primitive->attributes.POSITION);
		if(!vertexAccessor.ptr)
		{
			common->Warning("Vertex buffer'%d' is empty", vertexAccessor.bufferView->buffer);
			continue;
		}

		AccessorHelper normalAccessor(this, primitive->attributes.NORMAL);
		if(primitive->attributes.NORMAL >= 0 && !normalAccessor.ptr)
		{
			common->Warning("Normal buffer'%d' is empty", normalAccessor.bufferView->buffer);
		}

		AccessorHelper texcoordAccessor(this, primitive->attributes.TEXCOORD_);
		if(primitive->attributes.TEXCOORD_.Num() > 0 && primitive->attributes.TEXCOORD_0 >= 0 && !texcoordAccessor.ptr)
		{
			common->Warning("Texcoord0 buffer'%d' is empty", texcoordAccessor.bufferView->buffer);
		}

		int index = objModel.objects.Append(new objObject_t);
		objObject_t *objObject = objModel.objects[index];
		objObject->material = materials[primitive->material].name;

		idList<unsigned int> objVertexes; // cache saved vert for remove dup vert

		for(j = 0; j < indexAccessor.Count(); j+=3)
		{
			unsigned int vertexIndexes[3];
			indexAccessor.ToArray<unsigned int>(j, vertexIndexes, 3);

			int objVertIndexes[3];
			for(int k = 0; k < 3; k++)
			{
				const unsigned int vertexIndex = vertexIndexes[k];

				// find vert is cached
				if(!keepDup)
				{
					int n;
					for(n = 0; n < objVertexes.Num(); n++)
					{
						if(vertexIndex == objVertexes[n])
							break;
					}
					if(n < objVertexes.Num())
					{
						objVertIndexes[k] = n;
						continue;
					}
					objVertexes.Append(vertexIndex); // cache vert
				}

				const float *vf = vertexAccessor.Offset<float>(vertexIndex);
				idVec3 pos(vf[0], vf[1], vf[2]);
				objVertIndexes[k] = objObject->vertexes.Append(pos);

				if(texcoordAccessor.ptr)
				{
					const float *tf = texcoordAccessor.Offset<float>(vertexIndex);
					idVec2 texcoord(tf[0], tf[1]);
					objObject->texcoords.Append(texcoord);
				}
				else
				{
					objObject->texcoords.Append(idVec2(0.0f, 0.0f));
				}

				if(normalAccessor.ptr)
				{
					const float *nf = normalAccessor.Offset<float>(vertexIndex);
					idVec3 normal(nf[0], nf[1], nf[2]);
					objObject->normals.Append(normal);
				}
				else
				{
					objObject->normals.Append(idVec3(0.0f, 0.0f, 0.0f));
				}
			}

			objObject->indexes.Append(objVertIndexes[0]);
			objObject->indexes.Append(objVertIndexes[1]);
			objObject->indexes.Append(objVertIndexes[2]);
		}
	}

    return true;
}
#endif

void idModelGLTF::Print(void) const
{
    Sys_Printf("asset: version=%d.%d\n", asset.major_version, asset.minor_version);
    Sys_Printf("scene: %d\n", scene);

#define MODEL_PART_PRINT(name, list, all, fmt, ...) \
    Sys_Printf(#name " num: %d\n", list.Num()); \
    if(all) { \
        for(int i = 0; i < list.Num(); i++) { \
             Sys_Printf("%d: " fmt "\n", i, __VA_ARGS__); \
        } \
    } \
    Sys_Printf("\n------------------------------------------------------\n");

    MODEL_PART_PRINT(scenes, scenes, true, "name=%s nodes=%d   ", scenes[i].name.c_str(), scenes[i].nodes.Num())
    MODEL_PART_PRINT(nodes, nodes, true, "name=%s mesh=%d   ", nodes[i].name.c_str(), nodes[i].mesh)
    MODEL_PART_PRINT(bufferViews, bufferViews, true, "name=%s buffer=%d byteOffset=%d byteLength=%d byteStride=%d target=%d   ", bufferViews[i].name.c_str(), bufferViews[i].buffer, bufferViews[i].byteOffset, bufferViews[i].byteLength, bufferViews[i].byteStride, bufferViews[i].target)
    MODEL_PART_PRINT(buffers, buffers, true, "name=%s byteLength=%d uri=%d   ", buffers[i].name.c_str(), buffers[i].byteLength, buffers[i].uri.Num())
    MODEL_PART_PRINT(accessor, accessors, true, "name=%s bufferView=%d byteOffset=%d componentType=%d normalized=%d count=%d type=%s   ", accessors[i].name.c_str(), accessors[i].bufferView, accessors[i].byteOffset, accessors[i].componentType, accessors[i].normalized, accessors[i].count, accessors[i].type.c_str())
    MODEL_PART_PRINT(material, materials, true, "name=%s   ", materials[i].name.c_str())
    MODEL_PART_PRINT(meshes, meshes, true, "name=%s primitives=%d weights=%d   ", meshes[i].name.c_str(), meshes[i].primitives.Num(), meshes[i].weights.Num())

#undef MODEL_PART_PRINT
}

const char * idModelGLTF::GetAnim(unsigned int index) const
{
    if(index >= (unsigned int)animations.Num())
        return NULL;
    return animations[index].name;
}

int idModelGLTF::GetAnimCount(void) const
{
    return animations.Num();
}

#ifdef _MODEL_OBJ
static void R_ConvertGLTFToObj_f(const idCmdArgs &args)
{
    const char *gltfPath = args.Argv(1);
    idModelGLTF gltf;
    if(gltf.Parse(gltfPath, idModelGLTF::PARSE_MESH))
    {
        //gltf.Print();
        objModel_t objModel;
        if(gltf.ToObj(objModel))
        {
            idStr objPath = gltfPath;
            objPath.SetFileExtension(".obj");
            OBJ_Write(&objModel, objPath.c_str());
            common->Printf("Convert obj successful: %s -> %s\n", gltfPath, objPath.c_str());
        }
        else
            common->Warning("Convert obj fail: %s", gltfPath);
    }
    else
        common->Warning("Parse GLTF fail: %s", gltfPath);
}
#endif

static int R_ConvertGLTFToMd5(const char *filePath, bool doMesh = true, const idStrList *animList = NULL, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    int ret = 0;

    idModelGLTF gltf;
    idMd5MeshFile md5MeshFile;
    bool meshRes = false;
    if(gltf.Parse(filePath, animList ? idModelIqm::PARSE_FRAME : idModelIqm::PARSE_JOINT))
    {
        //gltf.Print();
        if(gltf.ToMd5Mesh(md5MeshFile, scale, addOrigin, offset, rotation))
        {
            if(doMesh)
            {
                md5MeshFile.Commandline().Append(va(" - %s", filePath));
                idStr md5meshPath = R_Model_MakeOutputPath(filePath, "." MD5_MESH_EXT, savePath);
                md5MeshFile.Write(md5meshPath.c_str());
                common->Printf("Convert md5mesh successful: %s -> %s\n", filePath, md5meshPath.c_str());
                ret++;
            }
            else
            {
                common->Printf("Convert md5mesh successful: %s\n", filePath);
            }
            meshRes = true;
        }
        else
            common->Warning("Convert md5mesh fail: %s", filePath);
    }
    else
        common->Warning("Parse gltf fail: %s", filePath);

    if(!meshRes)
        return ret;

    if(!animList)
        return ret;

    idStrList list;
    if(!animList->Num()) // all
    {
        for(int i = 0; i < gltf.GetAnimCount(); i++)
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
            animName = gltf.GetAnim(index);
            if(!animName)
            {
                common->Warning("Invalid animation index '%d'", index);
                continue;
            }
            common->Printf("Convert gltf/glb animation to md5anim: %d -> %s\n", index, animName);
            ok = gltf.ToMd5Anim(md5AnimFile, md5MeshFile, index, scale, addOrigin, offset, rotation);
            md5animPath = filePath;
            md5animPath.StripFilename();
            md5animPath.AppendPath(animName);
            md5animPath = R_Model_MakeOutputPath(md5animPath, "." MD5_ANIM_EXT, savePath);
        }
        else
        {
            idStr name = anim;
            name.StripPath();
            name.StripFileExtension();
            common->Printf("Convert gltf/glb animation to md5anim: %s -> %s\n", anim, name.c_str());
            ok = gltf.ToMd5Anim(md5AnimFile, md5MeshFile, name.c_str(), scale, addOrigin, offset, rotation);
            animName = anim;
            md5animPath = R_Model_MakeOutputPath(anim, "." MD5_ANIM_EXT, savePath);
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

ID_INLINE static int R_ConvertGLTFMesh(const char *filePath, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertGLTFToMd5(filePath, true, NULL, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertGLTFAnim(const char *filePath, const idStrList &animList, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertGLTFToMd5(filePath, false, &animList, scale, addOrigin, offset, rotation, savePath);
}

ID_INLINE static int R_ConvertGLTF(const char *filePath, const idStrList &animList, float scale = -1.0f, bool addOrigin = false, const idVec3 *offset = NULL, const idMat3 *rotation = NULL, const char *savePath = NULL)
{
    return R_ConvertGLTFToMd5(filePath, true, &animList, scale, addOrigin, offset, rotation, savePath);
}

static void R_ConvertGLTFToMd5mesh_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5MESH_USAGE(gltf/glb), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5MESH_USAGE(gltf/glb), args.Argv(0));
        return;
    }
    R_ConvertGLTFMesh(mesh, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertGLTFToMd5anim_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5ANIM_ALL_USAGE(gltf/glb), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5ANIM_ALL_USAGE(gltf/glb), args.Argv(0));
        return;
    }
    R_ConvertGLTFAnim(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

static void R_ConvertGLTFToMd5_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf(CONVERT_TO_MD5_ALL_USAGE(gltf/glb), args.Argv(0));
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
        common->Printf(CONVERT_TO_MD5_ALL_USAGE(gltf/glb), args.Argv(0));
        return;
    }
    R_ConvertGLTF(mesh, anims, scale, addOrigin, res & CCP_OFFSET ? &offset : NULL, res & CCP_ROTATION ? &rotation : NULL, savePath.c_str());
}

bool R_Model_HandleGLTF(const md5ConvertDef_t &convert)
{
    if(R_ConvertGLTF(convert.mesh, convert.anims,
                    convert.scale,
                    convert.addOrigin,
                    convert.offset.IsZero() ? NULL : &convert.offset,
                    convert.rotation.IsIdentity() ? NULL : &convert.rotation,
                     convert.savePath.IsEmpty() ? NULL : convert.savePath.c_str()
    ) != 1 + convert.anims.Num())
    {
        common->Warning("Convert gltf/glb to md5mesh/md5anim fail in entityDef '%s'", convert.def->GetName());
        return false;
    }
    return true;
}

static void ArgCompletion_gltf(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".gltf", ".glb"
            , NULL);
}

void R_GLTF_AddCommand(void)
{
    cmdSystem->AddCommand("gltfToMd5mesh", R_ConvertGLTFToMd5mesh_f, CMD_FL_RENDERER, "Convert gltf/glb to md5mesh", ArgCompletion_gltf);
    cmdSystem->AddCommand("gltfToMd5anim", R_ConvertGLTFToMd5anim_f, CMD_FL_RENDERER, "Convert gltf/glb to md5anim", ArgCompletion_gltf);
    cmdSystem->AddCommand("gltfToMd5", R_ConvertGLTFToMd5_f, CMD_FL_RENDERER, "Convert gltf/glb to md5mesh/md5anim", ArgCompletion_gltf);
#ifdef _MODEL_OBJ
    cmdSystem->AddCommand("gltfToObj", R_ConvertGLTFToObj_f, CMD_FL_RENDERER, "Convert gltf/glb to obj", ArgCompletion_gltf);
#endif
}
