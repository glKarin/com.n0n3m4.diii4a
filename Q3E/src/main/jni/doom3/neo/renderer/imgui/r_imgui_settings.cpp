
#include "r_imgui.h"
#include "imgui.h"

idCVar imgui_scale( "imgui_scale", "-1.0", CVAR_SYSTEM|CVAR_FLOAT|CVAR_ARCHIVE, "factor to scale ImGUI menus by (-1: auto)" );
idCVar imgui_fontScale( "imgui_fontScale", "-1.0", CVAR_SYSTEM|CVAR_FLOAT|CVAR_ARCHIVE, "factor to scale ImGUI menus by (-1: auto)" );

enum cvarFlag
{
    IG_CVAR_COMPONENT_SPACE = BIT(0),
    IG_CVAR_COMPONENT_CHECKBOX = BIT(1),
    IG_CVAR_COMPONENT_COMBO = BIT(2),
    IG_CVAR_COMPONENT_INPUT_FLOAT = BIT(3),
    IG_CVAR_COMPONENT_INT_SLIDER = BIT(4),
    IG_CVAR_COMPONENT_INPUT_INT = BIT(5),
    IG_CVAR_COMPONENT_INPUT_VECTOR = BIT(6),
    IG_CVAR_COMPONENT_INPUT = BIT(7),
    IG_CVAR_COMPONENT_BUTTON = BIT(8),

    IG_CVAR_GROUP_RENDERER = BIT(11),
    IG_CVAR_GROUP_FRAMEWORK = BIT(12),
    IG_CVAR_GROUP_GUI = BIT(13),
    IG_CVAR_GROUP_GAME = BIT(14),
    IG_CVAR_GROUP_OTHER = BIT(15),
};

typedef struct cvarSettingItem_s
{
    idCVar *cvar;
	idStr command;
	idStr name;
    int component;
    int flag;
    idList<char> options;
    idStrList values;
} cvarSettingItem_t;

typedef struct cvarSettingGroup_s
{
	idStr name;
    idList<cvarSettingItem_t> list;
} cvarSettingGroup_t;

class idImGuiSettings
{
public:
    idImGuiSettings(void) : visible(false), isInitialized(false) {}
    void AddOption(const char *name, const char *displayName = NULL, int flag = 0, const char *options = NULL);
    void Render(void);

public:
    bool visible;
    bool isInitialized;

protected:
    void RenderCheckboxCVar(const cvarSettingItem_t &item);
    void RenderComboCVar(const cvarSettingItem_t &item);
    void RenderFloatCVar(const cvarSettingItem_t &item);
    void RenderIntCVar(const cvarSettingItem_t &item);
	void RenderIntRangeCVar(const cvarSettingItem_t &item);
    void RenderVectorCVar(const cvarSettingItem_t &item);
    void RenderNullCVar(const cvarSettingItem_t &item);
    void RenderInputCVar(const cvarSettingItem_t &item);
    void RenderButtonCVar(const cvarSettingItem_t &item);

    void RenderToolTips(const idCVar *cvar);

    static void UpdateCVar(idCVar *cvar, int v);
    static void UpdateCVar(idCVar *cvar, float v);
    static void UpdateCVar(idCVar *cvar, bool v);
    static void UpdateCVar(idCVar *cvar, const char *v);
    static void ExecCommand(const char *cmd);

private:
    idList<cvarSettingGroup_s> options;
};

static idImGuiSettings imGuiSettings;

void idImGuiSettings::AddOption(const char *name, const char *displayName, int flag, const char *str)
{
	idCVar *cvar = NULL;
	bool isCmd = false;
	if(name && name[0])
	{
		isCmd = flag & IG_CVAR_COMPONENT_BUTTON;
		if(!isCmd)
		{
			cvar = cvarSystem->Find(name);
			if(!cvar)
				return;
			if(!(cvar->GetFlags() & CVAR_STATIC)) // check is built-in
				return;
			for(int i = 0; i < options.Num(); i++)
			{
				for(int m = 0; m < options[i].list.Num(); m++)
				{
					const cvarSettingItem_t &item = options[i].list[m];
					if(!item.cvar)
						continue;
					if(!idStr::Icmp(item.cvar->GetName(), name))
						return;
				}
			}
		}
		else
		{
			for(int i = 0; i < options.Num(); i++)
			{
				for(int m = 0; m < options[i].list.Num(); m++)
				{
					const cvarSettingItem_t &item = options[i].list[m];
					if(item.command.IsEmpty())
						continue;
					if(!idStr::Icmp(item.command, name))
						return;
				}
			}
		}
	}

    const char *gname;
    if(flag & IG_CVAR_GROUP_RENDERER) {
        gname = "Renderer";
    }
    else if(flag & IG_CVAR_GROUP_FRAMEWORK) {
        gname = "Framework";
    }
    else if(flag & IG_CVAR_GROUP_GUI) {
        gname = "GUI";
    }
    else if(flag & IG_CVAR_GROUP_GAME) {
        gname = "Game";
    }
    else if(flag & IG_CVAR_GROUP_OTHER) {
        gname = "Other";
    }
    else if(cvar && (cvar->GetFlags() & CVAR_RENDERER)) {
        gname = "Renderer";
    }
    else if(cvar && (cvar->GetFlags() & CVAR_SYSTEM)) {
        gname = "Framework";
    }
    else if(cvar && (cvar->GetFlags() & CVAR_GUI)) {
        gname = "GUI";
    }
    else if(cvar && (cvar->GetFlags() & CVAR_GAME)) {
        gname = "Game";
    }
    else {
        gname = "Other";
    }
    cvarSettingGroup_t *group = NULL;
    for(int i = 0; i < options.Num(); i++)
    {
        if(!idStr::Icmp(options[i].name, gname))
        {
            group = &options[i];
            break;
        }
    }
    if(!group)
    {
        cvarSettingGroup_t g;
        g.name = gname;
        int i = options.Append(g);
        group = &options[i];
    }

    cvarSettingItem_t item;
	if(displayName && displayName[0])
		item.name = displayName;
	else if(cvar)
		item.name = cvar->GetName();
	else if(isCmd)
		item.name = name;
    item.cvar = cvar;
	if(isCmd)
		item.command = name;
    item.flag = flag;
    if(str && str[0])
    {
        bool isInt = cvar->GetFlags() & CVAR_INTEGER;
        idStrList ops = idStr::Split(str, ';');
        for(int i = 0; i < ops.Num(); i++)
        {
            idStr &op = ops[i];
            idStr::StripWhitespace(op);
            idStr d;
            int index = op.Find('=');
            if(index != -1)
            {
                idStr v = op.Left(index);
                idStr::StripWhitespace(v);
                item.values.Append(v);
                d = op.Right(op.Length() - index - 1);
            }
            else
            {
                if(isInt)
                    item.values.Append(va("%d", i));
                else
                    item.values.Append(op);
                d = op;
            }
            idStr::StripWhitespace(d);
            for(int m = 0; m < d.Length(); m++)
                item.options.Append(d[m]);
            item.options.Append('\0');
        }
        item.options.Append('\0');
    }

    int comp = IG_CVAR_COMPONENT_SPACE;
    if(cvar)
    {
        if(item.flag & IG_CVAR_COMPONENT_CHECKBOX)
            comp = IG_CVAR_COMPONENT_CHECKBOX;
        else if(item.flag & IG_CVAR_COMPONENT_COMBO)
            comp = IG_CVAR_COMPONENT_COMBO;
        else if(item.flag & IG_CVAR_COMPONENT_INPUT_FLOAT)
            comp = IG_CVAR_COMPONENT_INPUT_FLOAT;
        else if(item.flag & IG_CVAR_COMPONENT_INT_SLIDER)
            comp = IG_CVAR_COMPONENT_INT_SLIDER;
        else if(item.flag & IG_CVAR_COMPONENT_INPUT_INT)
            comp = IG_CVAR_COMPONENT_INPUT_INT;
        else if(item.flag & IG_CVAR_COMPONENT_INPUT_VECTOR)
            comp = IG_CVAR_COMPONENT_INPUT_VECTOR;
        else if(item.flag & IG_CVAR_COMPONENT_INPUT)
            comp = IG_CVAR_COMPONENT_INPUT;
        else
        {
            if(cvar->GetFlags() & CVAR_BOOL)
                comp = IG_CVAR_COMPONENT_CHECKBOX;
            else if(cvar->GetFlags() & CVAR_FLOAT)
                comp = IG_CVAR_COMPONENT_INPUT_FLOAT;
            else if(cvar->GetFlags() & CVAR_INTEGER)
            {
                if(item.options.Num() > 0)
                    comp = IG_CVAR_COMPONENT_COMBO;
                else if(cvar->GetMaxValue() != cvar->GetMinValue())
                    comp = IG_CVAR_COMPONENT_INT_SLIDER;
                else
                    comp = IG_CVAR_COMPONENT_INPUT_INT;
            }
            else
                comp = IG_CVAR_COMPONENT_INPUT;
        }
    }
	else if(isCmd)
		comp = IG_CVAR_COMPONENT_BUTTON;

    item.component = comp;

    group->list.Append(item);
}

void idImGuiSettings::RenderToolTips(const idCVar *cvar)
{
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
        const char *desc = cvar->GetDescription();
        if(!desc && !desc[0])
            desc = cvar->GetName();
        ImGui::SetTooltip("%s", desc);
    }
}

void idImGuiSettings::RenderCheckboxCVar(const cvarSettingItem_t &item)
{
    bool v = item.cvar->GetBool();
    if(ImGui::Checkbox(item.name.c_str(), &v))
    {
        UpdateCVar(item.cvar, v);
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::RenderFloatCVar(const cvarSettingItem_t &item)
{
    float v = item.cvar->GetFloat();
    if(ImGui::InputFloat(item.name.c_str(), &v, 0.0f, 0.0f, "%.2f"))
    {
        UpdateCVar(item.cvar, v);
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::RenderInputCVar(const cvarSettingItem_t &item)
{
    const char *val = item.cvar->GetString();
    char v[1024];
    idStr::Copynz(v, val, sizeof(v));
    if(ImGui::InputText(item.name.c_str(), v, sizeof(v) - 1))
    {
        UpdateCVar(item.cvar, v);
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::RenderButtonCVar(const cvarSettingItem_t &item)
{
    if(ImGui::Button(item.name.c_str()))
    {
        ExecCommand(item.command);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
    {
		const char *desc = item.command.c_str();
        ImGui::SetTooltip("%s", desc);
    }
}

void idImGuiSettings::RenderIntCVar(const cvarSettingItem_t &item)
{
    int v = item.cvar->GetInteger();
    if(ImGui::InputInt(item.name.c_str(), &v, 1, 1))
    {
        UpdateCVar(item.cvar, v);
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::RenderVectorCVar(const cvarSettingItem_t &item)
{
    const char *val = item.cvar->GetString();
    float v[3] = { 0.0f };
    sscanf(val, "%f %f %f", &v[0], &v[1], &v[2]);
    if(ImGui::InputFloat3(item.name.c_str(), v, "%.2f"))
    {
        idStr str = va("%f %f %f", v[0], v[1], v[2]);
        UpdateCVar(item.cvar, str.c_str());
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::RenderIntRangeCVar(const cvarSettingItem_t &item)
{
    int v = item.cvar->GetInteger();
    if(ImGui::DragInt(item.name.c_str(), &v, 1.0f, item.cvar->GetMinValue(), item.cvar->GetMaxValue(), "%d", ImGuiSliderFlags_ClampOnInput))
    {
        UpdateCVar(item.cvar, v);
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::RenderNullCVar(const cvarSettingItem_t &item)
{
    if(item.name.IsEmpty())
        ImGui::Spacing();
    else
	    ImGui::SeparatorText(item.name.c_str());
}

void idImGuiSettings::RenderComboCVar(const cvarSettingItem_t &item)
{
    int select = 0;
    for(int i = 0; i < item.values.Num(); i++)
    {
        if(!idStr::Icmp(item.values[i], item.cvar->GetString()))
        {
            select = i;
            break;
        }
    }
    if(ImGui::Combo(item.name.c_str(), &select, item.options.Ptr()))
    {
        if(select >= 0 && select < item.values.Num())
            UpdateCVar(item.cvar, item.values[select]);
    }
    RenderToolTips(item.cvar);
}

void idImGuiSettings::Render(void)
{
    visible = true;

    ImGui::Begin("idTech4A++ settings", &visible); // Create a window called "Hello, world!" and append into it.

    if(imgui_fontScale.IsModified())
    {
        float fontScale = imgui_fontScale.GetFloat() > 0.0f ? imgui_fontScale.GetFloat() : 1.0f;
        ImGui::SetWindowFontScale(fontScale);
        imgui_fontScale.ClearModified();
    }

    if (ImGui::BeginTabBar("Special cvar and command"))
    {
        for(int i = 0; i < options.Num(); i++)
        {
            const cvarSettingGroup_t &group = options[i];
            if (ImGui::BeginTabItem(group.name.c_str()))
            {
                for(int m = 0; m < group.list.Num(); m++)
                {
                    const cvarSettingItem_t &item = group.list[m];
					bool disabled = item.cvar && (item.cvar->GetFlags() & (CVAR_INIT | CVAR_ROM));
					ImGui::BeginDisabled(disabled);
                    switch (item.component) {
                        case IG_CVAR_COMPONENT_CHECKBOX:
                            RenderCheckboxCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_COMBO:
                            RenderComboCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_INPUT_FLOAT:
                            RenderFloatCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_INT_SLIDER:
                            RenderIntRangeCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_INPUT_INT:
                            RenderIntCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_INPUT_VECTOR:
                            RenderVectorCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_INPUT:
                            RenderInputCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_BUTTON:
                            RenderButtonCVar(item);
                            break;
                        case IG_CVAR_COMPONENT_SPACE:
                            RenderNullCVar(item);
                            break;
                        default:
                            break;
                    }
					ImGui::EndDisabled();
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}


void idImGuiSettings::UpdateCVar(idCVar *cvar, int v)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        RB_ImGui_PushCVarCallback(cvar->GetName(), v);
        return;
    }
#endif
    if(cvar->GetInteger() != v)
        cvar->SetInteger(v);
}

void idImGuiSettings::UpdateCVar(idCVar *cvar, float v)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        RB_ImGui_PushCVarCallback(cvar->GetName(), v);
        return;
    }
#endif
    if(cvar->GetFloat() != v)
        cvar->SetFloat(v);
}

void idImGuiSettings::UpdateCVar(idCVar *cvar, bool v)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        RB_ImGui_PushCVarCallback(cvar->GetName(), v);
        return;
    }
#endif
    if(cvar->GetBool() != v)
        cvar->SetBool(v);
}

void idImGuiSettings::UpdateCVar(idCVar *cvar, const char *v)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        RB_ImGui_PushCVarCallback(cvar->GetName(), v);
        return;
    }
#endif
    if(idStr::Cmp(cvar->GetString(), v))
        cvar->SetString(v);
}

void idImGuiSettings::ExecCommand(const char *cmd)
{
#ifdef _MULTITHREAD
    if(multithreadActive)
    {
        RB_ImGui_PushCmdCallback(cmd);
        return;
    }
#endif
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, cmd);
}

ID_INLINE static void ImGui_RegisterCvar(const char *name = NULL, const char *displayName = NULL, int flag = 0, const char *options = NULL)
{
    imGuiSettings.AddOption(name, displayName, flag, options);
}

ID_INLINE static void ImGui_RegisterCmd(const char *name = NULL, const char *displayName = NULL, int flag = 0)
{
    imGuiSettings.AddOption(name, displayName, flag);
}

ID_INLINE static void ImGui_RegisterLabel(const char *displayName = NULL, int flag = 0)
{
    imGuiSettings.AddOption(NULL, displayName, flag, NULL);
}

ID_INLINE static void ImGui_RegisterDivide(int flag = 0)
{
    imGuiSettings.AddOption(NULL, NULL, flag, NULL);
}

void ImGui_RegisterOptions(void)
{
    if(imGuiSettings.isInitialized)
        return;

    // framework
    ImGui_RegisterLabel("Generic", IG_CVAR_GROUP_FRAMEWORK);
    ImGui_RegisterCvar("harm_com_consoleHistory", NULL, IG_CVAR_COMPONENT_COMBO, "0=Don't save;1=Save on exit;2=Save on every command");
    ImGui_RegisterCvar("harm_r_maxAllocStackMemory", "Control memory allocation on heap or stack", IG_CVAR_GROUP_FRAMEWORK);
    ImGui_RegisterCvar("com_disableAutoSaves", "Don't create Autosaves on new map");
    ImGui_RegisterDivide(IG_CVAR_GROUP_FRAMEWORK);

    // renderer
    ImGui_RegisterLabel("Lighting", IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCvar("harm_r_lightingModel", "Lighting model", IG_CVAR_COMPONENT_COMBO, "0=No light;1=Phong;2=Blinn-Phong;3=PBR;4=Ambient");
    ImGui_RegisterCvar("harm_r_specularExponent", "Specular exponent in Phong");
    ImGui_RegisterCvar("harm_r_specularExponentBlinnPhong", "Specular exponent in Blinn-Phong");
    ImGui_RegisterCvar("harm_r_specularExponentPBR", "Specular exponent in PBR");
    ImGui_RegisterCvar("harm_r_PBRNormalCorrection", "Vertex normal correction in PBR(Surface smoothness)");
    ImGui_RegisterCvar("harm_r_PBRRMAOSpecularMap", "Vertex normal correction in PBR");
    ImGui_RegisterCvar("harm_r_PBRRoughnessCorrection", "Max roughness for old specular texture in PBR");
    ImGui_RegisterCvar("harm_r_PBRMetallicCorrection", "Min metallic for old specular texture in PBR");
#ifdef _GLOBAL_ILLUMINATION
    ImGui_RegisterCvar("harm_r_globalIllumination", "Render global illumination");
    ImGui_RegisterCvar("harm_r_globalIlluminationBrightness", "Global illumination brightness");
#endif
#ifdef _NO_LIGHT
    ImGui_RegisterCvar("r_noLight", "No lighting");
#endif
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

#ifdef _SHADOW_MAPPING
    ImGui_RegisterLabel("Shadow mapping", IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCvar("r_useShadowMapping", "Soft shadow mapping", IG_CVAR_COMPONENT_CHECKBOX);
    ImGui_RegisterCvar("r_forceShadowMapsOnAlphaTestedSurfaces", "Shadow mapping on perforated surface");
    ImGui_RegisterCvar("harm_r_shadowMapAlpha", "Shadow mapping alpha");
    ImGui_RegisterCvar("harm_r_shadowMapCombine", "Combine local and global shadow mapping");
    ImGui_RegisterCvar("r_shadowMapSplits", "Cascaded shadow mapping with parallel lights", IG_CVAR_COMPONENT_INT_SLIDER);
    ImGui_RegisterCvar("harm_r_shadowMapNonParallelLightUltra", "Non-parallel light allow ultra quality shadow mapping");
#endif
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    ImGui_RegisterLabel("Stencil shadow", IG_CVAR_GROUP_RENDERER);
#ifdef _STENCIL_SHADOW_IMPROVE
    ImGui_RegisterCvar("harm_r_stencilShadowTranslucent", "Translucent stencil shadow");
    ImGui_RegisterCvar("harm_r_stencilShadowCombine", "Combine local and global stencil shadow");
    ImGui_RegisterCvar("harm_r_stencilShadowAlpha", "Stencil shadow alpha");
#ifdef _SOFT_STENCIL_SHADOW
    ImGui_RegisterCvar("harm_r_stencilShadowSoft", "Soft stencil shadow");
    ImGui_RegisterCvar("harm_r_stencilShadowSoftBias", "Soft stencil shadow sampler BIAS");
    ImGui_RegisterCvar("harm_r_shadowCarmackInverse", "Use Carmack-inverse on stencil testing for shadow");
    ImGui_RegisterCvar("harm_r_stencilShadowSoftCopyStencilBuffer", "Copy stencil buffer directly for soft stencil shadow", IG_CVAR_COMPONENT_COMBO, "0=Copy depth buffer and bind and renderer stencil buffer to texture directly;1=Copy stencil buffer to texture directly");
#endif
#endif
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    ImGui_RegisterLabel("Renderer", IG_CVAR_GROUP_RENDERER);
#ifdef _D3BFG_CULLING
    ImGui_RegisterCvar("harm_r_occlusionCulling", "DOOM3-BFG style occlusion culling", IG_CVAR_COMPONENT_CHECKBOX);
#endif
    ImGui_RegisterCvar("r_maxFps", "Max FPS", IG_CVAR_COMPONENT_INT_SLIDER);
    ImGui_RegisterCvar("r_renderMode", "Retro postprocess", IG_CVAR_COMPONENT_COMBO, "0=Doom;1=CGA;2=CGA Highres;3=Commodore 64;4=Commodore 64 Highres;5=Amstrad CPC 6128;6=Amstrad CPC 6128 Highres;7=Sega Genesis;8=Sega Genesis Highres;9=Sony PSX");
#ifdef _MULTITHREAD
    ImGui_RegisterCvar("harm_r_renderToolsMultithread", "Enable debug render on multithreading");
#endif
#ifdef _HUMANHEAD
    ImGui_RegisterCvar("harm_r_skipHHBeam", "Skip beam model render");
#endif
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    ImGui_RegisterLabel("Image", IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCvar("r_useETC1", "Use ETC1/RGBA4444 compression");
    ImGui_RegisterCvar("r_useETC1cache", "Cache compression textures");
#ifdef _OPENGLES3
    ImGui_RegisterCvar("r_useETC2", "Use ETC2 compression instead of RGBA4444");
#endif
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    ImGui_RegisterLabel("Shader", IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCvar("harm_r_useHighPrecision", "Use high precision float on GLSL shader", IG_CVAR_COMPONENT_CHECKBOX);
    ImGui_RegisterCvar("harm_r_shaderProgramDir", "External OpenGLES2 GLSL shader path");
#ifdef GL_ES_VERSION_3_0
    ImGui_RegisterCvar("harm_r_shaderProgramES3Dir", "External OpenGLES3 GLSL shader path");
#endif
    ImGui_RegisterCvar("reloadGLSLprograms", "Reload loaded GLSL shaders", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCmd("exportGLSLShaderSource", "Export built-in GLSL shaders", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCmd("printGLSLShaderSource", "Print built-in GLSL shaders", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCmd("exportDevShaderSource", "Export built-in GLSL shaders for developer", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    ImGui_RegisterLabel("System", IG_CVAR_GROUP_RENDERER);
#if !defined(__ANDROID__)
    ImGui_RegisterCvar("harm_r_openglVersion", "Clear vertex buffer on every frame", IG_CVAR_COMPONENT_COMBO, "GLES2=GLES 2.0;GLES3.0=GLES 3.0+;OpenGL_core=OpenGL desktop core;OpenGL_compatibility=OpenGL desktop compatibility");
    ImGui_RegisterCvar("r_multithread", "Enable multi-threading rendering");
#endif
    ImGui_RegisterCvar("harm_r_clearVertexBuffer", "Clear vertex buffer on every frame", IG_CVAR_COMPONENT_COMBO, "0=No clear(original);1=Only free memory;2=Free memory and delete VBO(same as 1 if multi-threading)");
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    ImGui_RegisterLabel("Other", IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterCvar("r_screenshotFormat", "Screenshot image format", IG_CVAR_COMPONENT_COMBO, "0=TGA;1=BMP;2=PNG;3=JPG;4=DDS;5=EXP;6=HDR");
    ImGui_RegisterCvar("r_screenshotJpgQuality", "Screenshot quality for JPG", IG_CVAR_COMPONENT_INT_SLIDER);
    ImGui_RegisterCvar("r_screenshotPngCompression", "Screenshot compression level for PNG", IG_CVAR_COMPONENT_INT_SLIDER);
    ImGui_RegisterCvar("harm_r_debugOpenGL", "Enable OpenGL debug");
    ImGui_RegisterCvar("harm_r_autoAspectRatio", "Setup view aspect ratio automatic", IG_CVAR_COMPONENT_COMBO, "0=Manual;1=Setup r_aspectRatio to -1;2=Setup r_aspectRatio to 0,1,2 by screen size");
    ImGui_RegisterCmd("glConfig", "Print OpenGL config", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_RENDERER);
    ImGui_RegisterDivide(IG_CVAR_GROUP_RENDERER);

    // GUI
    ImGui_RegisterLabel("Generic", IG_CVAR_GROUP_GUI);
    ImGui_RegisterCvar("r_scaleMenusTo43", "4:3 aspect ratio GUI", IG_CVAR_GROUP_GUI | IG_CVAR_COMPONENT_COMBO, "0=Disable;1=Only main menu;2=All");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GUI);

    ImGui_RegisterLabel("Font", IG_CVAR_GROUP_GUI);
    ImGui_RegisterCvar("harm_gui_wideCharLang", "Wide-character language support");
    ImGui_RegisterCvar("harm_gui_useD3BFGFont", "Using DOOM3-BFG new fonts");
#ifdef _RAVEN
    ImGui_RegisterCvar("harm_gui_defaultFont", "Default GUI font", IG_CVAR_COMPONENT_COMBO, "chain;lowpixel;marine;profont;r_strogg;strogg");
#endif
#ifdef _HUMANHEAD
    ImGui_RegisterCvar("harm_ui_translateAlienFont", "Translate alien font", IG_CVAR_COMPONENT_COMBO, "fonts;fonts/menu;");
    ImGui_RegisterCvar("harm_ui_translateAlienFontDistance", "Translate alien GUI max distance");
    ImGui_RegisterCvar("harm_ui_subtitlesTextScale", "Subtitles text scale");
#endif
    ImGui_RegisterDivide(IG_CVAR_GROUP_GUI);

	// game
    ImGui_RegisterLabel("Generic", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_g_skipHitEffect", "Skip hit effect in game when starting engine");
    ImGui_RegisterCmd("skipHitEffect", "Skip hit effect in game", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_GAME);
#ifdef _RAVEN
    ImGui_RegisterCvar("harm_g_allowFireWhenFocusNPC", "Allow fire when focus NPC");
    ImGui_RegisterCvar("harm_g_mutePlayerFootStep", "Mute player footstep sound");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);

    ImGui_RegisterLabel("Full-body awareness", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_pm_fullBodyAwareness", "Enable full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessOffset", "View offset in full-body awareness", IG_CVAR_COMPONENT_INPUT_VECTOR);
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessHeadJoint", "Head joint in full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessFixed", "Don't attach view to head in full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessHeadVisible", "Hide head in full-body awareness");
    ImGui_RegisterCvar("harm_ui_showViewBody", "Show view body");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);

    ImGui_RegisterLabel("Bot", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_si_autoFillBots", "Fill bots automatic in MP game");
    ImGui_RegisterCvar("harm_g_autoGenAASFileInMPGame", "Generate bot AAS file in MP game");
    ImGui_RegisterCvar("harm_si_botLevel", "Bot difficult level");
    ImGui_RegisterCvar("harm_si_botWeapons", "Bot initial weapons");
    ImGui_RegisterCvar("harm_si_botAmmo", "Bot initial weapon ammo clips");
    ImGui_RegisterCvar("harm_g_botEnableBuiltinAssets", "Enable built-in bot assets");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);
#elif defined(_HUMANHEAD)
    ImGui_RegisterLabel("Full-body awareness", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_pm_fullBodyAwareness", "Enable full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessOffset", "View offset in full-body awareness", IG_CVAR_COMPONENT_INPUT_VECTOR);
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessHeadJoint", "Head joint in full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessFixed", "Don't attach view to head in full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessHeadVisible", "Hide head in full-body awareness");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);
#else
    ImGui_RegisterCvar("harm_si_useCombatBboxInMPGame", "Players force use combat bbox in MP game");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);

    ImGui_RegisterLabel("Full-body awareness", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_pm_fullBodyAwareness", "Enable full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessOffset", "View offset in full-body awareness", IG_CVAR_COMPONENT_INPUT_VECTOR);
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessHeadJoint", "Head joint in full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessFixed", "Don't attach view to head in full-body awareness");
    ImGui_RegisterCvar("harm_pm_fullBodyAwarenessHeadVisible", "Hide head in full-body awareness");
    ImGui_RegisterCvar("harm_ui_showViewBody", "Show view body");

    ImGui_RegisterLabel("Flashlight", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_ui_showViewLight", "Enable player flashlight");
    ImGui_RegisterCvar("harm_ui_viewLightShader", "player view flashlight material texture/entityDef name");
    ImGui_RegisterCvar("harm_ui_viewLightOnWeapon", "Player flashlight follow weapon");
    ImGui_RegisterCvar("harm_ui_viewLightRadius", "Player flashlight radius", IG_CVAR_COMPONENT_INPUT_VECTOR);
    ImGui_RegisterCvar("harm_ui_viewLightOffset", "Player flashlight offset", IG_CVAR_COMPONENT_INPUT_VECTOR);
    ImGui_RegisterCvar("harm_ui_viewLightType", "Player flashlight type", IG_CVAR_COMPONENT_COMBO, "0=Spot light;1=Point light");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);

    ImGui_RegisterLabel("Bot", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_si_autoFillBots", "Fill bots automatic in MP game");
    ImGui_RegisterCvar("harm_g_autoGenAASFileInMPGame", "Generate bot AAS file in MP game");
    ImGui_RegisterCvar("harm_si_botLevel", "Bot difficult level");
    ImGui_RegisterCvar("harm_si_botWeapons", "Bot initial weapons");
    ImGui_RegisterCvar("harm_si_botAmmo", "Bot initial weapon ammo clips");
    ImGui_RegisterCvar("harm_g_botEnableBuiltinAssets", "Enable built-in bot assets");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);

    ImGui_RegisterLabel("Rivensin Mod", IG_CVAR_GROUP_GAME);
    ImGui_RegisterCvar("harm_pm_doubleJump", "Enable double-jump");
    ImGui_RegisterCvar("harm_pm_autoForceThirdPerson", "Third person view when start");
    ImGui_RegisterCvar("harm_pm_preferCrouchViewHeight", "Crouch view height in third person");
    ImGui_RegisterDivide(IG_CVAR_GROUP_GAME);
#endif

	// other
    ImGui_RegisterCvar("imgui_scale", "ImGui scale", IG_CVAR_GROUP_OTHER);
    ImGui_RegisterCvar("imgui_fontScale", "ImGui font scale", IG_CVAR_GROUP_OTHER);
    ImGui_RegisterCmd(IG_COMMAND_NAME " reset position", "Reset ImGui position", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_OTHER);
    ImGui_RegisterCmd(IG_COMMAND_NAME " reset size", "Reset ImGui size", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_OTHER);
    ImGui_RegisterCmd(IG_COMMAND_NAME " close", "Close ImGui", IG_CVAR_COMPONENT_BUTTON | IG_CVAR_GROUP_OTHER);

    imGuiSettings.isInitialized = true;
}

void ImGui_CloseSettings(void *)
{
    if(!imGuiSettings.visible)
    {
        RB_ImGui_Stop();
    }
}

static void ImGui_RenderSettings(void *)
{
    imGuiSettings.Render();
}

void R_ImGui_idTech4AmmSettings_f(const idCmdArgs &args)
{
    if(!R_ImGui_IsRunning())
    {
        ImGui_RegisterOptions();
        R_ImGui_SetRenderer(ImGui_RenderSettings, NULL, ImGui_CloseSettings);
        R_ImGui_Ready(true);
    }
    else
    {
        //R_ImGui_Stop();
    }
}

void R_ImGui_Startup(void)
{
    cmdSystem->AddCommand("idTech4AmmSettings", R_ImGui_idTech4AmmSettings_f, CMD_FL_SYSTEM, "Show idTech4A++ new cvars and commands");
    cmdSystem->AddCommand(IG_COMMAND_NAME, R_ImGui_Command_f, CMD_FL_SYSTEM, "DON'T EXEC");
}
