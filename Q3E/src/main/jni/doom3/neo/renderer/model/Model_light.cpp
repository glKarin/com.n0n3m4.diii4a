// instead game::idTestModel in engine using idGameEdit interface

#define VIEW_LIGHT_DEFAULT_RADIUS_STR "1280 640 640"
#define VIEW_LIGHT_DEFAULT_OFFSET_STR "20 0 0"
#define VIEW_LIGHT_DEFAULT_COLOR_STR "1 1 1"

#define VIEW_LIGHT_DEFAULT_RADIUS_VALUE 1280, 640, 640
#define VIEW_LIGHT_DEFAULT_OFFSET_VALUE 20, 0, 0
#define VIEW_LIGHT_DEFAULT_COLOR_VALUE 1, 1, 1

#define VIEW_LIGHT_DEFAULT_RADIUS idVec3(VIEW_LIGHT_DEFAULT_RADIUS_VALUE)
#define VIEW_LIGHT_DEFAULT_OFFSET idVec3(VIEW_LIGHT_DEFAULT_OFFSET_VALUE)
#define VIEW_LIGHT_DEFAULT_COLOR idVec3(VIEW_LIGHT_DEFAULT_COLOR_VALUE)

#define VIEW_LIGHT_ENTITY_NAME "idtech4amm_view_light"
#define VIEW_LIGHT_ENTITY_DEF_PATH "def/" VIEW_LIGHT_ENTITY_NAME ".def"

#include "Model_test.h"

class idModelLight
{
public:
    idModelLight();
    ~idModelLight();

    idRenderWorld *         RenderWorld(void) {
        return R_ModelTest_RenderWorld();
    }
    void                    Render(void);
    void                    Clean(void);
    bool                    HasLight(void) const {
        return lightDef != -1;
    }
    void					CreateLight(const char *model, const idDict *dict = NULL);
    int                     CanTest(void);

private:

private:
    renderLight_t           viewLight;
    idStr                   materialName;
    qhandle_t               lightDef;
    bool                    noshadows;
    bool                    nospecular;
    bool                    parallel;
    idVec3                  radius;
    idVec3                  offset;
    idVec3                  color;
};

idModelLight::idModelLight()
: lightDef(-1),
  noshadows(false),
  nospecular(false),
  parallel(false),
  radius(VIEW_LIGHT_DEFAULT_RADIUS),
  offset(VIEW_LIGHT_DEFAULT_OFFSET),
  color(VIEW_LIGHT_DEFAULT_COLOR)
{
    memset(&viewLight, 0, sizeof(viewLight));
}

idModelLight::~idModelLight()
{
    // Clean();
}

int idModelLight::CanTest(void)
{
    return R_ModelTest_IsAllowTest();
}

void idModelLight::Clean(void)
{
    if(lightDef != -1 && RenderWorld())
        RenderWorld()->FreeLightDef(lightDef);
    memset(&viewLight, 0, sizeof(viewLight));
    materialName.Clear();
    lightDef = -1;
    noshadows = false;
    nospecular = false;
    parallel = false;
    radius.Set(VIEW_LIGHT_DEFAULT_RADIUS_VALUE);
    offset.Set(VIEW_LIGHT_DEFAULT_OFFSET_VALUE);
    color.Set(VIEW_LIGHT_DEFAULT_COLOR_VALUE);
}

void idModelLight::CreateLight(const char *material, const idDict *dict)
{
    idRenderWorld *world = RenderWorld();
    if(!world)
        return;
    if(!gameEdit->PlayerIsValid())
        return;

    Clean();

    materialName = material;
    if(materialName.IsEmpty())
        return;

    idVec3 pos;
    gameEdit->PlayerGetEyePosition(pos);
    idAngles angles;
    gameEdit->PlayerGetViewAngles(angles);
    idMat3 rotation = angles.ToMat3();

    if(dict)
    {
        noshadows = dict->GetBool("noshadows", "0");
        nospecular = dict->GetBool("nospecular", "0");
        parallel = dict->GetBool("parallel", "0");
        radius = dict->GetVector("radius", VIEW_LIGHT_DEFAULT_RADIUS_STR);
        offset = dict->GetVector("offset", VIEW_LIGHT_DEFAULT_OFFSET_STR);
        color = dict->GetVector("color", VIEW_LIGHT_DEFAULT_COLOR_STR);

        pos += offset;
    }

    idStr lightOrigin;
    lightOrigin += pos.x;
    lightOrigin += " ";
    lightOrigin += pos.y;
    lightOrigin += " ";
    lightOrigin += pos.z;

    idStr lightAxis;
    lightOrigin += rotation[0][0];
    lightOrigin += " ";
    lightOrigin += rotation[0][1];
    lightOrigin += " ";
    lightOrigin += rotation[0][2];
    lightOrigin += " ";
    lightOrigin += rotation[1][0];
    lightOrigin += " ";
    lightOrigin += rotation[1][1];
    lightOrigin += " ";
    lightOrigin += rotation[1][2];
    lightOrigin += " ";
    lightOrigin += rotation[2][0];
    lightOrigin += " ";
    lightOrigin += rotation[2][1];
    lightOrigin += " ";
    lightOrigin += rotation[2][2];

    idStr lightColor;
    lightColor += color.x;
    lightColor += " ";
    lightColor += color.y;
    lightColor += " ";
    lightColor += color.z;

    common->Printf("material: %s\n", materialName.c_str());

    idDict spawnArgs;
    memset(&viewLight, 0, sizeof(viewLight));
    spawnArgs.Clear();
    spawnArgs.Set("classname", "light");
    spawnArgs.Set("texture", materialName.c_str());
    spawnArgs.Set("origin", lightOrigin.c_str());
    spawnArgs.Set("light_rotation", lightAxis.c_str());
    spawnArgs.SetBool("noshadows", noshadows);
    spawnArgs.SetBool("nospecular", nospecular);
    spawnArgs.SetBool("parallel", parallel);
    spawnArgs.Set("start_off", "0");
    spawnArgs.Set("light_up", va("0 0 %f", radius[2]));
    spawnArgs.Set("light_right", va("0 %f 0", radius[1]));
    spawnArgs.Set("light_target", va("%f 0 0", radius[0]));
    spawnArgs.Set("_color", lightColor.c_str());

    gameEdit->ParseSpawnArgsToRenderLight(&spawnArgs, &viewLight);

    lightDef = world->AddLightDef(&viewLight);
}

void idModelLight::Render()
{
    if (lightDef != -1) {
        idRenderWorld *world = RenderWorld();
        if(!world)
            return;

        idVec3 pos;
        gameEdit->PlayerGetEyePosition(pos);
        idAngles angles;
        gameEdit->PlayerGetViewAngles(angles);

        viewLight.origin = pos;
        viewLight.axis = angles.ToMat3();

        world->UpdateLightDef(lightDef, &viewLight);
    }
}

static idModelLight modelLight;

static bool R_ModelLight_CanTest(void)
{
    int res;

    res = modelLight.CanTest();
    return R_ModelTest_CheckCanTest(res);
}

static idDict R_ModelLight_ParseLightArgs(const idCmdArgs &args, int start)
{
    idDict dict;
    dict.Clear();

    bool noshadows = false;
    bool nospecular = false;
    bool parallel = false;
    idStr radius;
    idStr offset;
    idStr color;
    for(int i = 2; i < args.Argc(); i++)
    {
        const char *arg = args.Argv(i);
        if(!idStr::Icmp(arg, "noshadows"))
            noshadows = true;
        else if(!idStr::Icmp(arg, "nospecular"))
            nospecular = true;
        else if(!idStr::Icmp(arg, "parallel"))
            parallel = true;
        else if(!idStr::Icmp(arg, "radius"))
        {
            i++;
            if(i < args.Argc())
                radius = args.Argv(i);
            else
                common->Warning("Missing radius param!");
        }
        else if(!idStr::Icmp(arg, "offset"))
        {
            i++;
            if(i < args.Argc())
                offset = args.Argv(i);
            else
                common->Warning("Missing offset param!");
        }
        else if(!idStr::Icmp(arg, "color"))
        {
            i++;
            if(i < args.Argc())
                color = args.Argv(i);
            else
                common->Warning("Missing color param!");
        }
    }

    dict.SetBool("noshadows", noshadows);
    dict.SetBool("nospecular", nospecular);
    dict.SetBool("parallel", parallel);
    if(!radius.IsEmpty())
        dict.Set("radius", radius);
    if(!offset.IsEmpty())
        dict.Set("offset", offset);
    if(!color.IsEmpty())
        dict.Set("color", color);
    return dict;
}

void R_ModelLight_CleanViewLight_f(const idCmdArgs &args)
{
    modelLight.Clean();
    common->Printf("ViewLight clean.\n");
}

void R_ModelLight_ViewLight_f(const idCmdArgs &args)
{
    idStr			name;
    idDict			dict;

    if (args.Argc() < 2) {
        common->Printf("Usage: %s <material_name> [noshadows nospecular parallel radius \"light_target light_right light_up\" offset \"forward right up\" color \"red green blue\"].\n", args.Argv(0));
        // delete the viewLight if active
        R_ModelLight_CleanViewLight_f(idCmdArgs());
        return;
    }

    if (!R_ModelLight_CanTest()) {
        return;
    }

    name = args.Argv(1);
    idStr classname = args.Argc() > 2 ? args.Argv(2) : NULL;
    idDict testArgs = R_ModelLight_ParseLightArgs(args, 2);

    modelLight.CreateLight(name, &testArgs);
    common->Printf("ViewLight active.\n");
}

void R_ModelLight_RenderFrame(int time)
{
    if(modelLight.HasLight() && modelLight.CanTest() == CAN_TEST)
    {
        modelLight.Render();
    }
}

void R_ModelLight_CleanLight(void)
{
    R_ModelLight_CleanViewLight_f(idCmdArgs());
}

void R_ModelLight_AddCommand(void)
{
    cmdSystem->AddCommand("viewLight", R_ModelLight_ViewLight_f, CMD_FL_RENDERER, "create view light", idCmdSystem::ArgCompletion_ImageName);
    cmdSystem->AddCommand("viewLightClean", R_ModelLight_CleanViewLight_f, CMD_FL_RENDERER, "clean view light");
}
