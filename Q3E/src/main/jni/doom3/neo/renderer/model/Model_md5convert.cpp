#include "Model_md5convert.h"

#define MD5_CONVERT "md5convert"
#define MD5_CONVERT_PATH MD5_CONVERT "/"

typedef struct md5ConvertConf_s
{
    const char *type;
    md5ConvertDef_f converter;
} md5ConvertConf_t;

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
    "addOrigin" "0" // prepend 'origin' bone to front, bool, default false. It is useful when no origin bone or has more than 1 root bones or root bone is not identity position/rotation
    "offset" "0 0 0" // root bone origin offset, vector, default '0 0 0'
    "rotation" "0 0 0" // root bone orient rotation, vector, default '0 0 0'. value is angle degree(-360 - 360)

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
    convert.addOrigin = def->dict.GetBool("addOrigin", "0");
    convert.offset = def->dict.GetVector("offset", "0 0 0");
	idAngles angle;
	if(def->dict.GetAngles("rotation", "0 0 0", angle))
		convert.rotation = angle.ToMat3();
	else
		convert.rotation.Identity();

	const char prefix[] = "anim ";
    const idKeyValue *kv = def->dict.MatchPrefix(prefix);

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

static bool R_CleanConvertedMd5(const md5ConvertDef_t &convert, bool onlyMesh = false)
{
    if(!convert.def->dict.GetBool(MD5_CONVERT, "0")) // only clean has md5convert key
        return false;

    common->Printf("Cleaning converted md5 model from '%s'\n", convert.def->GetName());

    if(!convert.mesh.IsEmpty())
    {
        idStr meshPath = convert.mesh;
        meshPath.SetFileExtension("." MD5_MESH_EXT);
        common->Printf("Remove md5mesh '%s'\n", meshPath.c_str());
        while(fileSystem->ReadFile(meshPath, NULL, NULL) > 0)
            fileSystem->RemoveFile(meshPath);
    }

    if(onlyMesh)
        return true;

    for(int i = 0; i < convert.anims.Num(); i++)
    {
        idStr animPath = convert.anims[i];
        animPath.SetFileExtension("." MD5_ANIM_EXT);
        common->Printf("Remove md5anim '%s'\n", animPath.c_str());
        while(fileSystem->ReadFile(animPath, NULL, NULL) > 0)
            fileSystem->RemoveFile(animPath);
    }

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

#if 1
#include "../../idlib/JSON.h"
static void ArgCompletion_JSON(const idCmdArgs &args, void(*callback)(const char *s))
{
    cmdSystem->ArgCompletion_FolderExtension(args, callback, "", false, ".json", ".json5"
            , NULL);
}

void R_TestJSON_f(const idCmdArgs &args)
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
	idStr text;
	JSON_ToString(text, json);
	//printf("JJJ\n%s\n", text.c_str());
	fileSystem->WriteFile("test.json", text.c_str(), text.Length());
	JSON_Free(json);

	text.Clear();
	ok = JSON_Parse(json, "test.json");
    printf("JSON parse test: %d\n", ok);
	if(!ok)
		return;

    common->Printf("Save to test2.json\n");
	JSON_ToString(text, json, -1);
	//printf("JJJ\n%s\n", text.c_str());
	fileSystem->WriteFile("test2.json", text.c_str(), text.Length());
	JSON_Free(json);
}
#endif

void R_Md5Convert_AddCommand(void)
{
    cmdSystem->AddCommand("convertMd5Def", R_ConvertMd5Def_f, CMD_FL_RENDERER, "Convert other type animation model entityDef to md5mesh/md5anim", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
    cmdSystem->AddCommand("cleanConvertedMd5", R_CleanConvertedMd5_f, CMD_FL_RENDERER, "Clean converted md5mesh", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
    cmdSystem->AddCommand("convertMd5AllDefs", R_ConvertMd5AllDefs_f, CMD_FL_RENDERER, "Convert all other type animation models entityDef to md5mesh/md5anim");

    cmdSystem->AddCommand("json", R_TestJSON_f, CMD_FL_RENDERER, "test json", ArgCompletion_JSON);
}
