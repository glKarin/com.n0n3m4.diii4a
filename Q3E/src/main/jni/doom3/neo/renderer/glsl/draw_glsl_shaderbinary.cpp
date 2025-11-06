
#ifdef GL_ES_VERSION_3_0

#define GLSL_SHADER_BINARY_MAGIC ((unsigned int)('i' << 24 | 'd' << 16 | 't' << 8 | 's'))
#define GLSL_SHADER_BINARY_VERSION ((unsigned int)(0x11000000 | _IDTECH4AMM_PATCH))

#define GLSL_SHADER_BINARY_HEADER_EXT "glslbinh"
#define GLSL_SHADER_BINARY_DATA_EXT "glslbin"

enum {
    GLSL_BUILT_IN = 0,
    GLSL_CUSTOM = 1,
};

#pragma pack( push, 1 )
typedef struct glslShaderBinaryHeader_s // sizeof = 193
{
    uint32_t magic;
    uint32_t version;
    char name[32];
    uint8_t type; // built-in/custom
    uint16_t shader_type; // glsl_program_t
    uint16_t glsl_version; // GLSL source version
    // GPU info
    char gl_vendor[32];
    char gl_renderer[32];
    char gl_version[32];
    char gl_shading_language_version[32];
    // checksum CRC32
    uint32_t vertex_shader_digest;
    uint32_t fragment_shader_digest;
    // binary
    uint32_t binary_digest;
    uint32_t binaryFormat; // binary format
    uint32_t length; // binary length
} glslShaderBinaryHeader_t;

typedef struct glslShaderBinaryCache_s
{
    glslShaderBinaryHeader_t header;
    void *binary;
} glslShaderBinaryCache_t;
#pragma pack( pop )

static void RB_GLSL_FreeShaderBinaryCache(glslShaderBinaryCache_t *cache)
{
    if(cache->binary)
    {
        free(cache->binary);
        cache->binary = NULL;
    }
}

static idStr RB_GLSL_GetExternalShaderBinaryPath(bool external)
{
    idStr	fullPath;
#ifdef GL_ES_VERSION_3_0
    if(USING_GLES3)
    {
        fullPath = cvarSystem->GetCVarString("harm_r_shaderProgramBinaryES3Dir");
        if(fullPath.IsEmpty())
            fullPath = _GL3BINPROGS;
    }
    else
#endif
    {
        fullPath = cvarSystem->GetCVarString("harm_r_shaderProgramBinaryDir");
        if(fullPath.IsEmpty())
            fullPath = _GLBINPROGS;
    }

    if(external)
        fullPath.AppendPath("external");

    return fullPath;
}

static void RB_GLSL_WriteShaderBinaryCache(const glslShaderBinaryCache_t *cache)
{
    idStr fullPath = RB_GLSL_GetExternalShaderBinaryPath(cache->header.type == GLSL_CUSTOM);
    fullPath.AppendPath(cache->header.name);
    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_HEADER_EXT);
    common->Printf("    Write GLSL shader binary header: %s\n", fullPath.c_str());

    idFile *file = fileSystem->OpenFileWrite(fullPath.c_str());
    file->Write(&cache->header.magic, 4);
    file->WriteUnsignedInt(cache->header.version);
    file->Write(&cache->header.name[0], sizeof(cache->header.name));
    file->WriteUnsignedChar(cache->header.type);
    file->WriteUnsignedShort(cache->header.shader_type);
    file->WriteUnsignedShort(cache->header.glsl_version);

    file->Write(&cache->header.gl_vendor[0], sizeof(cache->header.gl_vendor));
    file->Write(&cache->header.gl_renderer[0], sizeof(cache->header.gl_renderer));
    file->Write(&cache->header.gl_version[0], sizeof(cache->header.gl_version));
    file->Write(&cache->header.gl_shading_language_version[0], sizeof(cache->header.gl_shading_language_version));

    file->WriteUnsignedInt(cache->header.vertex_shader_digest);
    file->WriteUnsignedInt(cache->header.fragment_shader_digest);

    file->WriteUnsignedInt(cache->header.binary_digest);
    file->WriteUnsignedInt(cache->header.binaryFormat);
    file->WriteUnsignedInt(cache->header.length);

    fileSystem->CloseFile(file);

    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_DATA_EXT);
    common->Printf("    Write GLSL shader binary data: %s\n", fullPath.c_str());
    fileSystem->WriteFile(fullPath, cache->binary, cache->header.length);
}

static void RB_GLSL_GenShaderBinaryHeader(glslShaderBinaryHeader_t *header, const char *name, const idStr &vertexSource, const idStr &fragmentSource, int type, bool external)
{
    header->magic = GLSL_SHADER_BINARY_MAGIC;
    header->version = GLSL_SHADER_BINARY_VERSION;
    header->type = external ? GLSL_CUSTOM : GLSL_BUILT_IN;
    header->shader_type = type;
    header->glsl_version = USING_GLES3 ? 0x300 : 0x100;
    idStr::Copynz(header->name, name, sizeof(header->name));
    idStr::Copynz(header->gl_vendor, glConfig.vendor_string, sizeof(header->gl_vendor));
    idStr::Copynz(header->gl_renderer, glConfig.renderer_string, sizeof(header->gl_vendor));
    idStr::Copynz(header->gl_version, glConfig.version_string, sizeof(header->gl_vendor));
    idStr::Copynz(header->gl_shading_language_version, glConfig.shading_language_version_string, sizeof(header->gl_vendor));
    header->vertex_shader_digest = CRC32_BlockChecksum(vertexSource.c_str(), vertexSource.Length());
    header->fragment_shader_digest = CRC32_BlockChecksum(fragmentSource.c_str(), fragmentSource.Length());
}

static bool RB_GLSL_CacheShaderBinary(GLuint program, const char *name, const idStr &vertexSource, const idStr &fragmentSource, int type, bool external)
{
    if(program == 0)
        return false;

    if(!qglIsProgram(program))
        return false;

    GLint len = 0;
    qglGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &len);
    if(len == 0)
        return false;

    byte *data = (byte *)malloc(len);
    GLsizei length = 0;
    GLenum binaryFormat = 0;
    qglGetProgramBinary(program, len, &length, &binaryFormat, data);

    if(length <= 0)
    {
        free(data);
        return false;
    }

    common->Printf("    Caching external shader binary\n");

    glslShaderBinaryCache_t cache;
    memset(&cache, 0, sizeof(cache));

    RB_GLSL_GenShaderBinaryHeader(&cache.header, name, vertexSource, fragmentSource, type, external);

    cache.binary = data;
    cache.header.binaryFormat = binaryFormat;
    cache.header.length = length;
    cache.header.binary_digest = CRC32_BlockChecksum(data, length);

    RB_GLSL_WriteShaderBinaryCache(&cache);

    RB_GLSL_FreeShaderBinaryCache(&cache);

    return true;
}

static bool RB_GLSL_ReadShaderBinaryHeader(const char *path, glslShaderBinaryHeader_t *header)
{
    idFile *file = fileSystem->OpenFileRead(path, false);
    if(!file)
        return false;

    if(file->Read(&header->magic, 4) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->version) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->Read(&header->name[0], sizeof(header->name)) != sizeof(header->name))
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedChar(header->type) != 1)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedShort(header->shader_type) != 2)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedShort(header->glsl_version) != 2)
    {
        fileSystem->CloseFile(file);
        return false;
    }

    if(file->Read(&header->gl_vendor[0], sizeof(header->gl_vendor)) != sizeof(header->gl_vendor))
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->Read(&header->gl_renderer[0], sizeof(header->gl_renderer)) != sizeof(header->gl_renderer))
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->Read(&header->gl_version[0], sizeof(header->gl_version)) != sizeof(header->gl_version))
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->Read(&header->gl_shading_language_version[0], sizeof(header->gl_shading_language_version)) != sizeof(header->gl_shading_language_version))
    {
        fileSystem->CloseFile(file);
        return false;
    }

    if(file->ReadUnsignedInt(header->vertex_shader_digest) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->fragment_shader_digest) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->binary_digest) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }

    if(file->ReadUnsignedInt(header->binaryFormat) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->length) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }

    fileSystem->CloseFile(file);
    return true;
}

static bool RB_GLSL_ReadShaderBinaryData(const char *path, glslShaderBinaryCache_t *cache)
{
    idFile *file = fileSystem->OpenFileRead(path, false);
    if(!file)
    {
        common->Warning("Can not load GLSL shader binary file: %s",path);
        return false;
    }

    if(file->Length() != cache->header.length)
    {
        common->Warning("GLSL shader binary length not match: %d != %u", file->Length(), cache->header.length);
        return false;
    }

    byte *data = (byte *)malloc(cache->header.length);
    if(file->Read(data, cache->header.length) != cache->header.length)
    {
        fileSystem->CloseFile(file);
        return false;
    }

    cache->binary = data;

    fileSystem->CloseFile(file);
    return true;
}

static bool RB_GLSL_CheckShaderBinaryHeader(const glslShaderBinaryHeader_t *header)
{
    if(header->magic != GLSL_SHADER_BINARY_MAGIC)
    {
        common->Warning("GLSL shader binary header magic invalid: %u != %u", header->magic, GLSL_SHADER_BINARY_MAGIC);
        return false;
    }
    if(header->version != GLSL_SHADER_BINARY_VERSION)
    {
        common->Warning("GLSL shader binary header version not match: %u != %u", header->version, GLSL_SHADER_BINARY_VERSION);
        return false;
    }
    uint16_t glsl_version = USING_GLES3 ? 0x300 : 0x100;
    if(header->glsl_version != glsl_version)
    {
        common->Warning("GLSL shader binary header glsl version not match: %u != %u", header->glsl_version, glsl_version);
        return false;
    }

    if(idStr::Cmp(header->gl_vendor, glConfig.vendor_string))
    {
        common->Warning("GLSL shader binary header gl vendor not match: %s != %s", header->gl_vendor, glConfig.vendor_string);
        return false;
    }
    if(idStr::Cmp(header->gl_renderer, glConfig.renderer_string))
    {
        common->Warning("GLSL shader binary header gl renderer not match: %s != %s", header->gl_renderer, glConfig.renderer_string);
        return false;
    }
    if(idStr::Cmp(header->gl_version, glConfig.version_string))
    {
        common->Warning("GLSL shader binary header gl version not match: %s != %s", header->gl_version, glConfig.version_string);
        return false;
    }
    if(idStr::Cmp(header->gl_shading_language_version, glConfig.shading_language_version_string))
    {
        common->Warning("GLSL shader binary header gl shading language version not match: %s != %s", header->gl_shading_language_version, glConfig.shading_language_version_string);
        return false;
    }

    if(header->length == 0)
    {
        common->Warning("GLSL shader binary header binary length invalid");
        return false;
    }

    return true;
}

static bool RB_GLSL_CheckShaderBinaryData(const glslShaderBinaryCache_t *cache)
{
    unsigned int crc = CRC32_BlockChecksum(cache->binary, cache->header.length);

    if(cache->header.binary_digest != crc)
    {
        common->Warning("GLSL shader binary header binary CRC not match: %u != %u", cache->header.binary_digest, crc);
        return false;
    }

    return true;
}

static bool RB_GLSL_LoadShaderBinary(shaderProgram_t *shaderProgram, const char *name, const idStr &vertexSource, const idStr &fragmentSource, int type, bool external)
{
    if (!glConfig.isInitialized) {
        return false;
    }

    idStr fullPath = RB_GLSL_GetExternalShaderBinaryPath(external);

    fullPath.AppendPath(name);
    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_HEADER_EXT);

    glslShaderBinaryCache_t cache;
    memset(&cache, 0, sizeof(cache));
    if(!RB_GLSL_ReadShaderBinaryHeader(fullPath, &cache.header))
    {
        common->Printf("    Can not load GLSL shader binary cache header file: %s\n", fullPath.c_str());
        return false;
    }

    if(!RB_GLSL_CheckShaderBinaryHeader(&cache.header))
        return false;

    uint8_t from = external ? GLSL_CUSTOM : GLSL_BUILT_IN;
    if(cache.header.type != (external ? GLSL_CUSTOM : GLSL_BUILT_IN) )
    {
        common->Warning("GLSL shader binary header type not match: %u != %u", cache.header.type, from);
        return false;
    }

    if(idStr::Cmp(cache.header.name, name))
    {
        common->Warning("GLSL shader binary header name not match: %s != %s", cache.header.name, name);
        return false;
    }

    unsigned int vertexCRC = CRC32_BlockChecksum(vertexSource, vertexSource.Length());
    if(cache.header.vertex_shader_digest != vertexCRC)
    {
        common->Warning("GLSL shader binary header vertex shader digest not match: %u != %u", cache.header.vertex_shader_digest, vertexCRC);
        return false;
    }

    unsigned int fragmentCRC = CRC32_BlockChecksum(fragmentSource, fragmentSource.Length());
    if(cache.header.fragment_shader_digest != fragmentCRC)
    {
        common->Warning("GLSL shader binary header fragment shader digest not match: %u != %u", cache.header.fragment_shader_digest, fragmentCRC);
        return false;
    }

    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_DATA_EXT);
    if(!RB_GLSL_ReadShaderBinaryData(fullPath, &cache))
        return false;

    if(!RB_GLSL_CheckShaderBinaryData(&cache))
    {
        RB_GLSL_FreeShaderBinaryCache(&cache);
        return false;
    }

    RB_GLSL_DeleteShaderProgram(shaderProgram);

    if(!qglIsProgram(shaderProgram->program))
        shaderProgram->program = qglCreateProgram();
    if(shaderProgram->program == 0)
    {
        RB_GLSL_FreeShaderBinaryCache(&cache);
        SHADER_ERROR("%s::glCreateProgram() error!\n", __func__);
        return false;
    }

    GL_ClearErrors();
    qglProgramBinary(shaderProgram->program, cache.header.binaryFormat, cache.binary, cache.header.length);

    GLenum err = qglGetError();
    if(err != GL_NO_ERROR)
    {
        RB_GLSL_FreeShaderBinaryCache(&cache);
        common->Warning("%s::glProgramBinary() error!", __func__);
        return false;
    }

    RB_GLSL_GetUniformLocations(shaderProgram);
    strncpy(shaderProgram->name, name, sizeof(shaderProgram->name));
    shaderProgram->type = type;

    RB_GLSL_FreeShaderBinaryCache(&cache);

    return true;
}
#endif