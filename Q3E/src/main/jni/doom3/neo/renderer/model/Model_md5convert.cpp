#include "Model_md5convert.h"

#define MD5_CONVERT "md5convert"
#define MD5_CONVERT_PATH MD5_CONVERT "/"

typedef struct md5ConvertConf_s
{
    const char *type;
    md5ConvertDef_f converter;
} md5ConvertConf_t;

static void R_Model_AddFlag(int &flag, const char *name)
{
    if(!idStr::Icmp(name, "addOrigin"))
        flag |= MD5CF_ADD_ORIGIN;
    else if(!idStr::Icmp(name, "renameOrigin"))
        flag |= MD5CF_RENAME_ORIGIN;

    if((flag & MD5CF_RENAME_ORIGIN) && (flag & MD5CF_ADD_ORIGIN))
        flag &= ~MD5CF_ADD_ORIGIN;
}

/*
 idTech4A++ using conversion to support other animation models like md5mesh/md5anim
 It slower, but porting easier.
 And md5anim source code is in game SDK, not like md5mesh is in engine

e.g. using psk/psa as md5mesh/md5anim
 1. in model def
 model exam_model {
    mesh other/from_other_type_model.md5mesh

    anim idle other/from_other_type_animation_idle.md5anim
    anim stand other/from_other_type_animation_stand.md5anim
    // more animations
 }

 2. in entity def
 entityDef other/from_other_type_model { // or 'md5convert/other/from_other_type_model', entityDef name must same as model's mesh name(exclude extension name) or prepend 'md5convert/' for clean manually later
    "md5convert" "1" // mark this entityDef is md5 conversion define for clean manually later
    "type" "psk" // animation model type, if not set, will using mesh's extension

    "mesh" "other/from_other_type_model.psk" // psk extension

    // convert config
    "scale" "-1.0" // scale vertexes, float, default -1.0. will not scale if less or equals than 0
    "allFlags" "addOrigin,renameOrigin" // same as `flag*` parameters, values are `addOrigin`, `renameOrigin`, split by ','. It can be override by `flag*`
    "flag1" "addOrigin" // prepend 'origin' bone to front, string, default false. It is useful when no origin bone or has more than 1 root bones or root bone is not identity position/rotation
    "flag2" "renameOrigin" // rename root bone name to 'origin', string, default false.
    "offset" "0 0 0" // root bone origin offset, vector, default '0 0 0'
    "rotation" "0 0 0" // root bone orient rotation, vector, default '0 0 0'. value is angle degree(-360 - 360)
    "savePath" "" // output path, string, default is empty, it will be output to same path

    "anim idle" "other/from_other_type_animation_idle.md5anim" // psa extension
    "anim stand" "other/from_other_type_animation_stand.md5anim" // psa extension
    // more animations
 }

 3. in game map
 exec `testModel exam_model` for testing
 It will convert md5mesh/md5anim first if md5mesh file not exists when loading md5mesh in idModelManager

 4. clean converted md5mesh
 exec `cleanConvertedMd5`
 */
static bool R_Model_ParseMd5ConvertDef(md5ConvertDef_t &convert, const idDecl *decl, bool strict)
{
    const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>(decl);

    if(!strict)
    {
        if(!def->dict.GetBool(MD5_CONVERT, "0")) // only clean has md5convert key
            return false;
    }

    const char *fileName = decl->GetName();
    common->Printf("Parse md5convert from '%s'\n", fileName);

    idStr meshPath;
    if(!def->dict.GetString("mesh", "", meshPath))
    {
        if(strict)
        {
            common->Warning("Missing mesh in entityDef '%s'", fileName);
            return false;
        }
    }

    idStr type;
    if(!def->dict.GetString("type", "", type))
    {
        common->Warning("Missing animation model type in entityDef '%s', try fetch from mesh name '%s'", fileName, meshPath.c_str());
        meshPath.ExtractFileExtension(type);

        if(type.IsEmpty())
        {
            if(strict)
            {
                common->Warning("Unable to get animation model type by mesh '%s' in entityDef '%s'", meshPath.c_str(), fileName);
                return false;
            }
        }
        common->Printf("Found type '%s' from mesh in entityDef '%s'\n", type.c_str(), fileName);
    }

    convert.type = type;
    convert.def = def;
    convert.mesh = meshPath;
    convert.scale = def->dict.GetFloat("scale", "-1.0");
    convert.offset = def->dict.GetVector("offset", "0 0 0");
	idAngles angle;
	if(def->dict.GetAngles("rotation", "0 0 0", angle))
		convert.rotation = angle.ToMat3();
	else
		convert.rotation.Identity();
    convert.savePath = def->dict.GetString("savePath", "");

    int flag = 0;
    const char *allFlags = def->dict.GetString("allFlags", "");
    if(allFlags && allFlags[0])
    {
        idStrList strs;
        idStr::SplitUnique(strs, allFlags, ',');
        for (int i = 0; i < strs.Num(); i++) {
            R_Model_AddFlag(flag, strs[i]);
        }
    }

    const char flagPrefix[] = "flag";
    const idKeyValue *kv = def->dict.MatchPrefix(flagPrefix);
    while (kv != NULL) {
        R_Model_AddFlag(flag, kv->GetValue());
        kv = def->dict.MatchPrefix(flagPrefix, kv);
    }
    convert.flags = flag;

	const char prefix[] = "anim ";
    kv = def->dict.MatchPrefix(prefix);

    while (kv != NULL) {
        convert.anims.Append(kv->GetValue());
        idStr name = kv->GetKey();
        name = name.Mid(strlen(prefix), name.Length());
        name.StripTrailingWhitespace();
        name.StripLeading(' ');
        convert.animNames.Append(name);
        kv = def->dict.MatchPrefix(prefix, kv);
    }
    if(!convert.anims.Num())
    {
        common->Warning("Missing animation in entityDef '%s'", fileName);
    }

    return true;
}

static bool R_Model_ConvertToMd5(const idDecl *decl)
{
    md5ConvertDef_t convert;
    if(!R_Model_ParseMd5ConvertDef(convert, decl, true))
        return false;

    const md5ConvertConf_t SupportConverters[] = {
#ifdef _MODEL_PSK
            {"psk", R_Model_HandlePskPsa, },
#endif
#ifdef _MODEL_IQM
            {"iqm", R_Model_HandleIqm, },
#endif
#ifdef _MODEL_SMD
            {"smd", R_Model_HandleSmd, },
#endif
#ifdef _MODEL_GLTF
            {"gltf", R_Model_HandleGLTF, },
            {"glb", R_Model_HandleGLTF, },
#endif
#ifdef _MODEL_FBX
            {"fbx", R_Model_HandleFbx, },
#endif
#ifdef _MODEL_MD5V6
            {"md5meshv6", R_Model_HandleMd5V6, },
#endif
    };

    for(int i = 0; i < sizeof(SupportConverters) / sizeof(SupportConverters[0]); i++)
    {
        const md5ConvertConf_t *conv = SupportConverters + i;
        if(!idStr::Icmp(convert.type, conv->type))
        {
            return conv->converter(convert);
        }
    }

    common->Warning("Unsupported animation model type '%s' in entityDef '%s'", convert.type.c_str(), decl->GetName());
    return false;
}

bool R_Model_ConvertToMd5(const char *fileName)
{
    const idDecl *decl = declManager->FindType(DECL_ENTITYDEF, fileName, false);
    if(!decl)
    {
        idStr meshName = MD5_CONVERT;
        meshName.AppendPath(fileName);
        common->Warning("Source animation model entityDef '%s' not found, try find '%s'", fileName, meshName.c_str());
        decl = declManager->FindType(DECL_ENTITYDEF, meshName, false);
        if(!decl)
        {
            common->Warning("Source animation model entityDef '%s' not found", meshName.c_str());
            return false;
        }
    }

	return R_Model_ConvertToMd5(decl);
}

idStr R_Model_MakeOutputPath(const char *originPath, const char *extName, const char *savePath)
{
    idStr newPath;

    if(savePath && savePath[0])
    {
        newPath = savePath;
        idStr str = originPath;
        str.StripPath();
        newPath.AppendPath(str);
    }
    else
        newPath = originPath;

    if(extName && extName[0])
        newPath.SetFileExtension(extName);

    return newPath;
}

static bool R_CleanConvertedMd5(const md5ConvertDef_t &convert, bool onlyMesh = false)
{
    if(!convert.def->dict.GetBool(MD5_CONVERT, "0")) // only clean has md5convert key
        return false;

    common->Printf("Cleaning converted md5 model from '%s'\n", convert.def->GetName());

    if(!convert.mesh.IsEmpty())
    {
        idStr meshPath = R_Model_MakeOutputPath(convert.mesh, "." MD5_MESH_EXT, convert.savePath);
        common->Printf("Remove md5mesh '%s'\n", meshPath.c_str());
        while(fileSystem->ReadFile(meshPath, NULL, NULL) > 0)
            fileSystem->RemoveFile(meshPath);
    }

    if(onlyMesh)
        return true;

    for(int i = 0; i < convert.anims.Num(); i++)
    {
        idStr animPath = R_Model_MakeOutputPath(convert.anims[i], "." MD5_ANIM_EXT, convert.savePath);
        common->Printf("Remove md5anim '%s'\n", animPath.c_str());
        while(fileSystem->ReadFile(animPath, NULL, NULL) > 0)
            fileSystem->RemoveFile(animPath);
    }

    common->Printf("Clean done\n");

    return true;
}

static void R_CleanConvertedMd5(const idDecl *decl, bool onlyMesh = false)
{
    md5ConvertDef_t convert;
    if(!R_Model_ParseMd5ConvertDef(convert, decl, false))
        return;

    R_CleanConvertedMd5(convert, onlyMesh);
}

static void R_ConvertMd5Def_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Printf("Usage: %s <entityDef>\n", args.Argv(0));
        return;
    }

    R_Model_ConvertToMd5(args.Argv(1));
}

static void R_ConvertMd5AllDefs_f(const idCmdArgs &)
{
    const int num = declManager->GetNumDecls(DECL_ENTITYDEF);
    for(int i = 0; i < num; i++)
    {
        const idDecl *decl = declManager->DeclByIndex(DECL_ENTITYDEF, i, false);
        if(decl->GetState() != DS_PARSED && idStr::Icmpn(decl->GetName(), MD5_CONVERT_PATH, strlen(MD5_CONVERT_PATH))) // only clean entityDef name starts with md5convert
            continue;
        if(decl->GetState() != DS_PARSED)
            decl = declManager->DeclByIndex(DECL_ENTITYDEF, i, true); // parse it
        const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>(decl);
        if(def->dict.GetBool(MD5_CONVERT, "0")) // only handle has md5convert key
            R_Model_ConvertToMd5(decl);
    }
}

static void R_CleanConvertedMd5_f(const idCmdArgs &args)
{
    if(args.Argc() == 1)
    {
        const int num = declManager->GetNumDecls(DECL_ENTITYDEF);
        for(int i = 0; i < num; i++)
        {
            const idDecl *decl = declManager->DeclByIndex(DECL_ENTITYDEF, i, false);
            if(decl->GetState() != DS_PARSED && idStr::Icmpn(decl->GetName(), MD5_CONVERT_PATH, strlen(MD5_CONVERT_PATH))) // only clean entityDef name starts with md5convert
                continue;
            if(decl->GetState() != DS_PARSED)
                decl = declManager->DeclByIndex(DECL_ENTITYDEF, i, true); // parse it
            R_CleanConvertedMd5(decl);
        }
    }
    else
    {
        for(int i = 1; i < args.Argc(); i++)
        {
            const idDecl *decl = declManager->FindType(DECL_ENTITYDEF, args.Argv(i), false);
            if(!decl)
                continue;
            R_CleanConvertedMd5(decl);
        }
    }
}

void R_Model_NormalizeWeights(float *arr, int num)
{
	int i;

	// no weights
	if(num == 0)
		return;

	// only a bone, force to 1.0
	if(num == 1)
	{
		if(arr[0] != 1.0f)
			arr[0] = 1.0f;
		return;
	}

	// check all bone weights sum is 1.0
	float total = 0.0f;
	for(i = 0; i < num; i++)
	{
		total += arr[i];
	}
	if(total == 1.0f)
		return;

	// scale
	float p = 1.0f / total;
	total = 0.0f;
	for(i = 0; i < num - 1; i++)
	{
		arr[i] *= p;
		total += arr[i];
	}

	// remain to last
	arr[i] = 1.0f - total;
}

void R_Model_ClearMd5Convert(md5ConvertDef_t &convert)
{
    convert.type.Clear();
    convert.def = NULL;
    convert.mesh.Clear();
    convert.scale = -1.0f;
    convert.flags = 0;
    convert.offset.Zero();
    convert.rotation.Identity();
    convert.anims.Clear();
    convert.animNames.Clear();
    convert.savePath.Clear();
}

void R_Model_PrintMd5Convert(md5ConvertDef_t &convert)
{
    common->Printf("entityDef: %s\n", convert.def ? convert.def->GetName() : "<none>");
    common->Printf("Type: %s\n", convert.type.c_str());
    common->Printf("Mesh: %s\n", convert.mesh.c_str());
    common->Printf("Scale: %f\n", convert.scale);
    common->Printf("Flags: 0x%X\n", convert.flags);
    if(convert.flags & MD5CF_ADD_ORIGIN) common->Printf("  Add origin bone\n");
    if(convert.flags & MD5CF_RENAME_ORIGIN) common->Printf("  Rename origin bone\n");
    common->Printf("Offset: %f, %f, %f\n", convert.offset[0], convert.offset[1], convert.offset[2]);
    idAngles angles = convert.rotation.ToAngles();
    common->Printf("Rotation angle: %f, %f, %f\n", angles[0], angles[1], angles[2]);
    common->Printf("Save path: %s\n", convert.savePath.c_str());
    common->Printf("Animations %d\n", convert.anims.Num());
    for(int i = 0; i < convert.anims.Num(); i++)
        common->Printf("  %d: %s\n", i, convert.anims[i].c_str());
}

int R_Model_ParseMd5ConvertCmdLine(const idCmdArgs &args, md5ConvertDef_t &convert)
{
    const char *arg;
    bool readParm = false;
    int parm = 0;
    int res = CCP_NONE;

    for(int i = 1; i < args.Argc(); i++)
    {
        arg = args.Argv(i);
        if(!idStr::Icmp(arg, "-"))
        {
            if(readParm)
                common->Warning("Expect param name, but read '-'");
            else
                readParm = true;
            parm = CCP_NONE;
            continue;
        }

        if(readParm && (parm == CCP_NONE || parm == CCP_ANIMATIONS))
        {
            if(!idStr::Icmp("scale", arg))
                parm = CCP_SCALE;
            else if(!idStr::Icmp("addOrigin", arg) || !idStr::Icmp("renameOrigin", arg))
            {
                parm = CCP_FLAGS;
                R_Model_AddFlag(convert.flags, arg);
                parm = CCP_NONE;
                readParm = false;
                res |= CCP_FLAGS;
            }
            else if(!idStr::Icmp("offset", arg))
                parm = CCP_OFFSET;
            else if(!idStr::Icmp("rotation", arg))
                parm = CCP_ROTATION;
            else if(!idStr::Icmp("animation", arg))
                parm = CCP_ANIMATIONS;
            else if(!idStr::Icmp("savePath", arg))
                parm = CCP_SAVEPATH;
            else
                common->Warning("Unknown param name '%s'", arg);

            continue;
        }

        if(parm != CCP_NONE)
        {
            switch (parm) {
                case CCP_SCALE:
                    if(sscanf(arg, "%f", &convert.scale) != 1)
                        common->Warning("Parse scale value '%s' fail", arg);
                    else
                        res |= CCP_SCALE;
                    break;
                case CCP_OFFSET: {
                    float v[3];
                    if(sscanf(arg, "%f %f %f", &v[0], &v[1], &v[2]) != 3)
                        common->Warning("Parse offset value '%s' fail", arg);
                    else
                    {
                        convert.offset.Set(v[0], v[1], v[2]);
                        res |= CCP_OFFSET;
                    }
                }
                    break;
                case CCP_ROTATION: {
                    float v[3];
                    if(sscanf(arg, "%f %f %f", &v[0], &v[1], &v[2]) != 3)
                        common->Warning("Parse rotation value '%s' fail", arg);
                    else
                    {
                        idAngles angles(v[0], v[1], v[2]);
                        convert.rotation = angles.ToMat3();
                        res |= CCP_ROTATION;
                    }
                }
                    break;
                case CCP_ANIMATIONS:
                    convert.anims.AddUnique(arg);
                    res |= CCP_ANIMATIONS;
                    break;
                case CCP_SAVEPATH:
                    convert.savePath = arg;
                    res |= CCP_SCALE;
                    break;
                default:
                    common->Warning("Unknown param type '%d'", parm);
                    break;
            }

            if(parm != CCP_ANIMATIONS)
            {
                parm = CCP_NONE;
                readParm = false;
            }
            continue;
        }

        if(convert.mesh.IsEmpty())
        {
            res |= CCP_MESH;
            convert.mesh = arg;
            convert.mesh.ExtractFileExtension(convert.type);
            continue;
        }

        convert.anims.AddUnique(arg);
        res |= CCP_ANIMATIONS;
    }

    return res;
}

int R_Model_ParseMd5ConvertCmdLine(const idCmdArgs &args, idStr *mesh, int *flags, float *scale, idVec3 *offset, idMat3 *rotation, idStrList *anims, idStr *savePath)
{
    md5ConvertDef_t convert;
    R_Model_ClearMd5Convert(convert);
    int res = R_Model_ParseMd5ConvertCmdLine(args, convert);
    R_Model_PrintMd5Convert(convert);

    if(mesh && (res & CCP_MESH))
        *mesh = convert.mesh;
    if(flags && (res & CCP_FLAGS))
        *flags = convert.flags;
    if(scale && (res & CCP_SCALE))
        *scale = convert.scale;
    if(offset && (res & CCP_OFFSET))
        *offset = convert.offset;
    if(rotation && (res & CCP_ROTATION))
        *rotation = convert.rotation;
    if(savePath && (res & CCP_SAVEPATH))
        *savePath = convert.savePath;
    if(anims && (res & CCP_ANIMATIONS))
        *anims = convert.anims;

    return res;
}

#if 1
static void ArgCompletion_JSON(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, 
			".json", ".json5", ".gltf",
            NULL);
}

static void R_TestJSON_f(const idCmdArgs &args)
{
    if(args.Argc() < 2)
    {
        common->Warning("Usage: %s <json file>", args.Argv(0));
        return;
    }

	common->Printf("Test JSON\n");
	fileSystem->RemoveFile("test.json");
	fileSystem->RemoveFile("test2.json");
    const char *jsonFile = args.Argv(1);

	json_t json;
    JSON_Init(json);
	bool ok = JSON_Parse(json, jsonFile);
	common->Printf("JSON parse: %d\n", ok);
	if(!ok)
		return;

    if(args.Argc() > 2)
    {
        idStr path;
        for(int i = 2; i < args.Argc(); i++)
        {
            path.Append(args.Argv(i));
        }

        const json_t *res = JSON_Find(json, path);
        common->Printf("Find path in json: %s -> %p\n", path.c_str(), res);
        if(res)
        {
            idStr text;
            JSON_ToString(text, *res);
            common->Printf("%s\n", text.c_str());
        }

        return;
    }

    common->Printf("Save to test.json\n");
#if 1
    idList<char> text(1024*1024);
    JSON_ToArray(text, json);
    fileSystem->WriteFile("test.json", text.Ptr(), text.Num());
    text.Clear();
#elif 1
    idFile *file = fileSystem->OpenFileWrite("test.json");
    auto text = [](void *userData, const char *data, int length) -> int {
        idFile *file = (idFile *)userData;
        return file->Write(data, length);
    };
    JSON_ToFile(text, file, json);
    fileSystem->CloseFile(file);
#elif 1
    FILE *file = fopen("test.json", "wb");
    auto text = [](void *userData, const char *data, int length) -> int {
        FILE *file = (FILE *)userData;
        return fwrite(data, 1, length, file);
    };
    JSON_ToFile(text, file, json);
    fclose(file);
#else
    idStr text;
    JSON_ToString(text, json);
    fileSystem->WriteFile("test.json", text.c_str(), text.Length());
    text.Clear();
#endif
	JSON_Free(json);

	ok = JSON_Parse(json, "test.json");
    printf("JSON parse test: %d\n", ok);
	if(!ok)
		return;

    common->Printf("Save to test2.json\n");
#if 1
    JSON_ToArray(text, json, -1);
    fileSystem->WriteFile("test2.json", text.Ptr(), text.Num());
#elif 1
    file = fileSystem->OpenFileWrite("test2.json");
    JSON_ToFile(text, file, json, -1);
    fileSystem->CloseFile(file);
#elif 1
    file = fopen("test2.json", "wb");
    JSON_ToFile(text, file, json);
    fclose(file);
#else
    JSON_ToString(text, json, -1);
    fileSystem->WriteFile("test.json", text.c_str(), text.Length());
#endif
	JSON_Free(json);
}
#endif

void R_Md5Convert_AddCommand(void)
{
    cmdSystem->AddCommand("convertMd5Def", R_ConvertMd5Def_f, CMD_FL_RENDERER, "Convert other type animation model entityDef to md5mesh/md5anim", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
    cmdSystem->AddCommand("cleanConvertedMd5", R_CleanConvertedMd5_f, CMD_FL_RENDERER, "Clean converted md5mesh", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
    cmdSystem->AddCommand("convertMd5AllDefs", R_ConvertMd5AllDefs_f, CMD_FL_RENDERER, "Convert all other type animation models entityDef to md5mesh/md5anim");

#if 1
    cmdSystem->AddCommand("json", R_TestJSON_f, CMD_FL_RENDERER, "test json", ArgCompletion_JSON);
#endif
}
