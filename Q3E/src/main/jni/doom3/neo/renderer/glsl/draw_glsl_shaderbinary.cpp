
#ifdef GL_ES_VERSION_3_0

#define GLSL_SHADER_BINARY_MAGIC ((unsigned int)('i' << 24 | 'd' << 16 | 't' << 8 | 's'))
#define GLSL_SHADER_BINARY_VERSION ((unsigned int)(0x11000000 | _IDTECH4AMM_PATCH))

#define GLSL_SHADER_BINARY_HEADER_EXT "glslbinh"
#define GLSL_SHADER_BINARY_DATA_EXT "glslbin"

#define GLSL_BIN_DIGESTS_CMP(src, text) ( (src) == CRC32_BlockChecksum((text), strlen(text)) )
#define GLSL_BIN_DIGEST(target, data, len) target = CRC32_BlockChecksum((data), (len))
#define GLSL_BIN_DIGESTS_STR(target, str) GLSL_BIN_DIGEST(target, (str).c_str(), (str).Length())
#define GLSL_BIN_DIGESTS_CSTR(target, str) GLSL_BIN_DIGEST(target, (str), strlen(str))

enum {
    GLSL_VERSION_ES2 = 0x0100,
    GLSL_VERSION_ES3 = 0x0300,
};

enum {
    GLSL_BUILT_IN = 1,
    GLSL_EXTERNAL = 2,
};

enum {
    GLSL_DIGEST_CRC32 = 1,
};

#pragma pack( push, 1 )
typedef struct glslShaderBinaryHeader_s
{
    uint32_t magic;
    uint32_t version;
    char name[64];
    uint8_t type; // built-in/custom
    uint8_t digest_method; // crc32
    uint16_t shader_type; // glsl_program_t
    uint16_t glsl_version; // GLSL source version
    // GPU info
    uint32_t gl_vendor;
    uint32_t gl_renderer;
    uint32_t gl_version;
    uint32_t gl_shading_language_version;
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
    idStr	fullPath = RB_GLSL_GetExternalShaderSourcePath();

    fullPath.StripTrailing('/');
    fullPath.StripTrailing('\\');
    fullPath. Append(_GLPROGSBIN);

    if(external)
        fullPath.AppendPath("external");

    return fullPath;
}

static void RB_GLSL_RemoveShaderBinaryCache(const char *path)
{
    idStr fullPath = path;
    bool p = false;

    const char *exts[] = {
            "." GLSL_SHADER_BINARY_HEADER_EXT,
            "." GLSL_SHADER_BINARY_DATA_EXT,
    };

    for(int i = 0; i < sizeof(exts) / sizeof(exts[0]); i++)
    {
        fullPath.SetFileExtension(exts[i]);
        if(fileSystem->ReadFile(fullPath, NULL, NULL) >= 0)
        {
            fileSystem->RemoveFile(fullPath);
            p = true;
        }
    }

    if(p)
        common->Printf("    Remove GLSL shader binary cache file: %s" "/" GLSL_SHADER_BINARY_DATA_EXT "\n", fullPath.c_str());
}

static void RB_GLSL_RemoveShaderBinaryCache(const char *name, bool external)
{
    idStr fullPath = RB_GLSL_GetExternalShaderBinaryPath(external);

    fullPath.AppendPath(name);
    RB_GLSL_RemoveShaderBinaryCache(fullPath.c_str());
}

static void RB_GLSL_WriteShaderBinaryCache(const glslShaderBinaryCache_t *cache)
{
    idStr fullPath = RB_GLSL_GetExternalShaderBinaryPath(cache->header.type == GLSL_EXTERNAL);
    fullPath.AppendPath(cache->header.name);
    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_HEADER_EXT);
    common->Printf("    Write GLSL shader binary header: %s\n", fullPath.c_str());

    idFile *file = fileSystem->OpenFileWrite(fullPath.c_str());
    file->Write(&cache->header.magic, 4);
    file->WriteUnsignedInt(cache->header.version);
    file->Write(&cache->header.name[0], sizeof(cache->header.name));
    file->WriteUnsignedChar(cache->header.type);
    file->WriteUnsignedChar(cache->header.digest_method);
    file->WriteUnsignedShort(cache->header.shader_type);
    file->WriteUnsignedShort(cache->header.glsl_version);

    file->WriteUnsignedInt(cache->header.gl_vendor);
    file->WriteUnsignedInt(cache->header.gl_renderer);
    file->WriteUnsignedInt(cache->header.gl_version);
    file->WriteUnsignedInt(cache->header.gl_shading_language_version);

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
    header->type = external ? GLSL_EXTERNAL : GLSL_BUILT_IN;
    header->digest_method = GLSL_DIGEST_CRC32;
    header->shader_type = type;
    header->glsl_version = USING_GLES3 ? GLSL_VERSION_ES3 : GLSL_VERSION_ES2;
    idStr::Copynz(header->name, name, sizeof(header->name));
    GLSL_BIN_DIGESTS_CSTR(header->gl_vendor, glConfig.vendor_string);
    GLSL_BIN_DIGESTS_CSTR(header->gl_version, glConfig.version_string);
    GLSL_BIN_DIGESTS_CSTR(header->gl_renderer, glConfig.renderer_string);
    GLSL_BIN_DIGESTS_CSTR(header->gl_shading_language_version, glConfig.shading_language_version_string);

    GLSL_BIN_DIGESTS_STR(header->vertex_shader_digest, vertexSource);
    GLSL_BIN_DIGESTS_STR(header->fragment_shader_digest, fragmentSource);
}

static bool RB_GLSL_CacheShaderBinary(GLuint program, const char *name, const idStr &vertexSource, const idStr &fragmentSource, int type, bool external)
{
    RB_GLSL_RemoveShaderBinaryCache(name, external);

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
    GLSL_BIN_DIGEST(cache.header.binary_digest, data, length);

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
    if(file->ReadUnsignedChar(header->digest_method) != 1)
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

    if(file->ReadUnsignedInt(header->gl_vendor) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->gl_renderer) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->gl_version) != 4)
    {
        fileSystem->CloseFile(file);
        return false;
    }
    if(file->ReadUnsignedInt(header->gl_shading_language_version) != 4)
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
        common->Printf("    Can not load GLSL shader binary file: %s\n",path);
        return false;
    }

    if(file->Length() != cache->header.length)
    {
        common->Printf("    GLSL shader binary length not match: %d != %u\n", file->Length(), cache->header.length);
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
        common->Printf("    GLSL shader binary header magic invalid: %u != %u\n", header->magic, GLSL_SHADER_BINARY_MAGIC);
        return false;
    }
    if(header->version != GLSL_SHADER_BINARY_VERSION)
    {
        common->Printf("    GLSL shader binary header version not match: %u != %u\n", header->version, GLSL_SHADER_BINARY_VERSION);
        return false;
    }
    if(header->digest_method != GLSL_DIGEST_CRC32)
    {
        common->Printf("    GLSL shader binary header digest method not match: %d != %d\n", header->digest_method, GLSL_DIGEST_CRC32);
        return false;
    }
    uint16_t glsl_version = USING_GLES3 ? GLSL_VERSION_ES3 : GLSL_VERSION_ES2;
    if(header->glsl_version != glsl_version)
    {
        common->Printf("    GLSL shader binary header glsl version not match: %u != %u\n", header->glsl_version, glsl_version);
        return false;
    }

    if(!GLSL_BIN_DIGESTS_CMP(header->gl_vendor, glConfig.vendor_string))
    {
        common->Printf("    GLSL shader binary header gl vendor not match: %u != %s\n", header->gl_vendor, glConfig.vendor_string);
        return false;
    }
    if(!GLSL_BIN_DIGESTS_CMP(header->gl_renderer, glConfig.renderer_string))
    {
        common->Printf("    GLSL shader binary header gl renderer not match: %u != %s\n", header->gl_renderer, glConfig.renderer_string);
        return false;
    }
    if(!GLSL_BIN_DIGESTS_CMP(header->gl_version, glConfig.version_string))
    {
        common->Printf("    GLSL shader binary header gl version not match: %u != %s\n", header->gl_version, glConfig.version_string);
        return false;
    }
    if(!GLSL_BIN_DIGESTS_CMP(header->gl_shading_language_version, glConfig.shading_language_version_string))
    {
        common->Printf("    GLSL shader binary header gl shading language version not match: %u != %s\n", header->gl_shading_language_version, glConfig.shading_language_version_string);
        return false;
    }

    if(header->length == 0)
    {
        common->Printf("    GLSL shader binary header binary length invalid\n");
        return false;
    }

    return true;
}

static bool RB_GLSL_CheckShaderBinaryData(const glslShaderBinaryCache_t *cache)
{
    unsigned int crc;
    GLSL_BIN_DIGEST(crc, cache->binary, cache->header.length);

    if(cache->header.binary_digest != crc)
    {
        common->Printf("    GLSL shader binary header binary CRC not match: %u != %u\n", cache->header.binary_digest, crc);
        return false;
    }

    return true;
}

static bool RB_GLSL_LoadShaderBinaryCache(shaderProgram_t *shaderProgram, const char *name, const idStr &vertexSource, const idStr &fragmentSource, int type, bool external)
{
    idStr fullPath = RB_GLSL_GetExternalShaderBinaryPath(external);

    fullPath.AppendPath(name);
    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_HEADER_EXT);

    glslShaderBinaryCache_t cache;
    memset(&cache, 0, sizeof(cache));

    if(!RB_GLSL_ReadShaderBinaryHeader(fullPath, &cache.header))
    {
        common->Printf("    Can not load GLSL shader binary cache header file: %s\n", fullPath.c_str());
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    if(!RB_GLSL_CheckShaderBinaryHeader(&cache.header))
    {
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    uint8_t from = external ? GLSL_EXTERNAL : GLSL_BUILT_IN;
    if(cache.header.type != (external ? GLSL_EXTERNAL : GLSL_BUILT_IN) )
    {
        common->Printf("    GLSL shader binary header type not match: %u != %u\n", cache.header.type, from);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    if(idStr::Cmp(cache.header.name, name))
    {
        common->Printf("    GLSL shader binary header name not match: %s != %s\n", cache.header.name, name);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    unsigned int vertexCRC;
    GLSL_BIN_DIGESTS_STR(vertexCRC, vertexSource);
    if(cache.header.vertex_shader_digest != vertexCRC)
    {
        common->Printf("    GLSL shader binary header vertex shader digest not match: %u != %u\n", cache.header.vertex_shader_digest, vertexCRC);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    unsigned int fragmentCRC;
    GLSL_BIN_DIGESTS_STR(fragmentCRC, fragmentSource);
    if(cache.header.fragment_shader_digest != fragmentCRC)
    {
        common->Printf("    GLSL shader binary header fragment shader digest not match: %u != %u\n", cache.header.fragment_shader_digest, fragmentCRC);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    fullPath.SetFileExtension("." GLSL_SHADER_BINARY_DATA_EXT);
    if(!RB_GLSL_ReadShaderBinaryData(fullPath, &cache))
    {
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    if(!RB_GLSL_CheckShaderBinaryData(&cache))
    {
        RB_GLSL_FreeShaderBinaryCache(&cache);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    RB_GLSL_DeleteShaderProgram(shaderProgram);

    if(!qglIsProgram(shaderProgram->program))
        shaderProgram->program = qglCreateProgram();
    if(shaderProgram->program == 0)
    {
        SHADER_ERROR("%s::glCreateProgram() error!\n", __func__);
        RB_GLSL_FreeShaderBinaryCache(&cache);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    GL_ClearErrors();
    qglProgramBinary(shaderProgram->program, cache.header.binaryFormat, cache.binary, cache.header.length);

    GLenum err = qglGetError();
    if(err != GL_NO_ERROR)
    {
        common->Printf("    %s::glProgramBinary() error!\n", __func__);
        RB_GLSL_FreeShaderBinaryCache(&cache);
        RB_GLSL_RemoveShaderBinaryCache(fullPath);
        return false;
    }

    RB_GLSL_GetUniformLocations(shaderProgram);
    idStr::Copynz(shaderProgram->name, name, sizeof(shaderProgram->name));
    shaderProgram->type = type;

    RB_GLSL_FreeShaderBinaryCache(&cache);

    return true;
}

static void R_CleanGLSLShaderBinary_f(const idCmdArgs &)
{
    idStr path = RB_GLSL_GetExternalShaderBinaryPath(false);
    common->Printf("Remove GLSL shader binary cache directory '%s'\n", path.c_str());
    fileSystem->RemoveFolder(path);
}
#endif
