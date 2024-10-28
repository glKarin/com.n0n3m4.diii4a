package com.karin.idTech4Amm.lib;

import com.n0n3m4.q3e.Q3EUtils;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class KCVarSystem
{
    public static Map<String, KCVar.Group> CVars()
    {
        Map<String, KCVar.Group> _cvars = new LinkedHashMap<>();

        KCVar.Group RENDERER_CVARS = new KCVar.Group("Renderer", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_r_clearVertexBuffer", "integer", "2", "Clear vertex buffer on every frame", KCVar.FLAG_LAUNCHER,
                                "0", "Not clear(original)",
                                "1", "Only free memory",
                                "2", "Free memory and delete VBO handle(only without multi-threading, else same as 1)"
                        ),
                        KCVar.CreateCVar("harm_r_maxAllocStackMemory", "integer", "524288", "Control allocate temporary memory when load model data. 0 = Always heap; Negative = Always stack; Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory)", 0),
                        KCVar.CreateCVar("harm_r_shaderProgramDir", "string", "glslprogs", "Special external OpenGLES2.0 GLSL shader program directory path", 0),
                        KCVar.CreateCVar("harm_r_shaderProgramES3Dir", "string", "glsl3progs", "Special external OpenGLES3.0 GLSL shader program directory path", 0),

                        KCVar.CreateCVar("harm_r_shadowCarmackInverse", "bool", "0", "Stencil shadow using Carmack-Inverse", 0),
                        KCVar.CreateCVar("harm_r_lightingModel", "string", "1", "Lighting model when draw interactions", KCVar.FLAG_LAUNCHER,
                                "1", "Phong",
                                "2", "Blinn-Phong",
                                "3", "PBR",
                                "4", "Ambient",
                                "0", "No lighting"
                        ),
                        KCVar.CreateCVar("harm_r_specularExponent", "float", "3.0", "Specular exponent in Phong interaction lighting model", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_specularExponentBlinnPhong", "float", "12.0", "Specular exponent in Blinn-Phong interaction lighting model", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_specularExponentPBR", "float", "5.0", "Specular exponent in PBR interaction lighting model", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_normalCorrectionPBR", "float", "1.0", "Vertex normal correction in PBR interaction lighting model(1 = pure using bump texture; 0 = pure using vertex normal; 0.0 - 1.0 = bump texture * harm_r_specularExponentPBR + vertex normal * (1 - harm_r_specularExponentPBR))", KCVar.FLAG_POSITIVE),
                        KCVar.CreateCVar("harm_r_ambientLightingBrightness", "float", "1.0", "Lighting brightness in ambient lighting", KCVar.FLAG_POSITIVE),
                        KCVar.CreateCVar("r_maxFps", "integer", "0", "Limit maximum FPS. 0 = unlimited", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),

                        KCVar.CreateCVar("r_screenshotFormat", "integer", "0", "Screenshot format", 0,
                        "0", "TGA (default)",
                                "1", "BMP",
                                "2", "PNG",
                                "3", "JPG",
                                "4", "DDS"
                                ),
                        KCVar.CreateCVar("r_screenshotJpgQuality", "integer", "75", "Screenshot quality for JPG images (0-100)", KCVar.FLAG_POSITIVE),
                        KCVar.CreateCVar("r_screenshotPngCompression", "integer", "3", "Compression level when using PNG screenshots (0-9)", KCVar.FLAG_POSITIVE),

                        KCVar.CreateCVar("r_useShadowMapping", "bool", "0", "use shadow mapping instead of stencil shadows", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_shadowMapAlpha", "float", "1.0", "Shadow's alpha in shadow mapping", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_shadowMapJitterScale", "float", "2.5", "scale factor for jitter offset", KCVar.FLAG_POSITIVE),
                        /*KCVar.CreateCVar("harm_r_shadowMapSampleFactor", "float", "-1", "soft shadow's sample factor in shadow mapping(0: disable, -1: auto, > 0: multiple)", 0),
                        KCVar.CreateCVar("harm_r_shadowMappingScheme", "integer", "0", "shadow mapping rendering scheme", 0,
                                "0", "always using shadow mapping",
                                "1", "prelight shadow using shadow mapping, others using stencil shadow",
                                "2", "non-prelight shadow using shadow mapping, others using stencil shadow"
                        ),*/
                        KCVar.CreateCVar("r_forceShadowMapsOnAlphaTestedSurfaces", "bool", "0", "render perforated surface to shadow map", KCVar.FLAG_LAUNCHER),

                        KCVar.CreateCVar("harm_r_stencilShadowTranslucent", "bool", "0", "enable translucent shadow in stencil shadow", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowAlpha", "float", "1.0", "translucent shadow's alpha in stencil shadow", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowCombine", "bool", "0", "combine local and global stencil shadow", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowSoft", "bool", "0", "enable soft stencil shadow(Only OpenGLES3.1+)", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowSoftBias", "float", "-1", "soft stencil shadow sampler BIAS(-1 = automatic; 0 = disable; positive = value)", 0),
                        KCVar.CreateCVar("harm_r_stencilShadowSoftCopyStencilBuffer", "bool", "0", "copy stencil buffer directly for soft stencil shadow. 0: copy depth buffer and bind and renderer stencil buffer to texture directly; 1: copy stencil buffer to texture directly", 0),
                        KCVar.CreateCVar("harm_r_autoAspectRatio", "integer", "1", "automatic setup aspect ratio of view", KCVar.FLAG_LAUNCHER,
                                "0", "Manual",
                                "1", "Force setup r_aspectRatio to -1 (default)",
                                "2", "Automatic setup r_aspectRatio to 0,1,2 by screen size"),
                        KCVar.CreateCVar("harm_r_renderToolsMultithread", "bool", "0", "Enable render tools debug with GLES in multi-threading", 0)
                );
        KCVar.Group FRAMEWORK_CVARS = new KCVar.Group("Framework", true)
                .AddCVar(
                    KCVar.CreateCVar("harm_fs_gameLibPath", "string", "", "Special game dynamic library", KCVar.FLAG_LAUNCHER | KCVar.FLAG_AUTO),
                    KCVar.CreateCVar("harm_fs_gameLibDir", "string", "", "Special game dynamic library directory path(default is empty, means using apk install libs directory path", KCVar.FLAG_DISABLED),
                    KCVar.CreateCVar("harm_com_consoleHistory", "integer", "2", "Save/load console history", 0,
                            "0", "disable",
                            "1", "loading in engine initialization, and saving in engine shutdown",
                            "2", "loading in engine initialization, and saving in every e executing"
                    ),
                    KCVar.CreateCVar("r_scaleMenusTo43", "bool", "0", "Scale menus, fullscreen videos and PDA to 4:3 aspect ratio", 0),
                    KCVar.CreateCVar("harm_in_smoothJoystick", "bool", "0", "Enable smooth joystick(Automatic setup by Android layer)", KCVar.FLAG_AUTO)
                );
        KCVar.Group GAME_CVARS = new KCVar.Group("DOOM3", false)
                .AddCVar(
                    KCVar.CreateCVar("harm_pm_fullBodyAwareness", "bool", "0", "Enables full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset(forward-offset side-offset up-offset)", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadJoint", "string", "Head", "Set head joint when without head model in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessFixed", "bool", "0", "Do not attach view position to head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_ui_showViewBody", "bool", "0", "show view body(mod)", 0),
                    KCVar.CreateCVar("harm_g_normalizeMovementDirection", "integer", "-1", "Re-normalize player/walker movement direction. 0 = disable; -1 = automcatic; >0 = max degree", 0)
                );
        KCVar.Group RIVENSIN_CVARS = new KCVar.Group("Rivensin", false)
                .AddCVar(
                    KCVar.CreateCVar("harm_pm_doubleJump", "bool", "1", "Enable double-jump", 0),
                    KCVar.CreateCVar("harm_pm_autoForceThirdPerson", "bool", "1", "Force set third person view after game level load end", 0),
                    KCVar.CreateCVar("harm_pm_preferCrouchViewHeight", "float", "32", "Set prefer crouch view height in Third-Person(suggest 32 - 39, less or equals 0 to disable)", KCVar.FLAG_POSITIVE)
                );

        KCVar.Group QUAKE4_CVARS = new KCVar.Group("Quake4", false)
                .AddCVar(
                    KCVar.CreateCVar("harm_g_autoGenAASFileInMPGame", "bool", "1", "For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic", 0),
                    KCVar.CreateCVar("harm_g_vehicleWalkerMoveNormalize", "bool", "1", "Re-normalize vehicle walker movment", 0),
                    KCVar.CreateCVar("harm_gui_defaultFont", "string", "chain", "Default font name", 0,
                            "chain", "fonts/chain",
                            "lowpixel", "fonts/lowpixel",
                            "marine", "fonts/marine",
                            "profont", "fonts/profont",
                            "r_strogg", "fonts/r_strogg",
                            "strogg", "fonts/strogg"
                    ),
                    KCVar.CreateCVar("harm_si_autoFillBots", "bool", "0", "Automatic fill bots after map loaded in multiplayer game(0 = disable; other number = bot num)", 0),
                    KCVar.CreateCommand("addbots", "string", "adds multiplayer bots batch(support `tab` complete, exam. addbots bot_name1 bot_name2 ...)", 0),
                    KCVar.CreateCommand("fillbots", "integer", "fill bots(empty argument to fill max bots num, exam. fillbots 8)", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCVar("harm_g_mutePlayerFootStep", "bool", "0", "Mute player's footstep sound", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwareness", "bool", "0", "Enables full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset(forward-offset side-offset up-offset)", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadJoint", "string", "head_channel", "Set head joint when without head model in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessFixed", "bool", "0", "Do not attach view position to head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_ui_showViewBody", "bool", "0", "show view body(mod)", 0),
                    KCVar.CreateCVar("harm_g_normalizeMovementDirection", "integer", "-1", "Re-normalize player/walker movement direction. 0 = disable; -1 = automcatic; >0 = max degree", 0)
                );

        KCVar.Group PREY_CVARS = new KCVar.Group("Prey(2006)", false)
                .AddCVar(
                KCVar.CreateCVar("harm_ui_translateAlienFont", "string", "fonts", "Setup font name for automatic translate `alien` font text of GUI(empty to disable)", 0,
                        "fonts", "fonts",
                        "fonts/menu", "fonts/menu",
                        "\"\"", "Disable"
                    ),
                    KCVar.CreateCVar("harm_ui_translateAlienFontDistance", "float", "200", "Setup max distance of GUI to view origin for enable translate `alien` font text(0 = disable; -1 = always; positive: distance value)", 0),
                    KCVar.CreateCVar("harm_ui_subtitlesTextScale", "float", "0.32", "Subtitles's text scale(less or equals 0 to unset)", 0),

                    KCVar.CreateCVar("harm_pm_fullBodyAwareness", "bool", "0", "Enables full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset(forward-offset side-offset up-offset)", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadJoint", "string", "neck", "Set head joint when without head model in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessFixed", "bool", "0", "Do not attach view position to head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_g_normalizeMovementDirection", "integer", "-1", "Re-normalize player/walker movement direction. 0 = disable; -1 = automcatic; >0 = max degree", 0)
                );

        KCVar.Group DOOM3BFG_CVARS = new KCVar.Group("DOOM 3 BFG", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_image_useCompression", "integer", "0", "Use ETC1/2 compression or RGBA4444 texture for low memory(e.g. 32bits device), it will using lower memory but loading slower", KCVar.FLAG_INIT,
                                "0", "RGBA8",
                                "1", "ETC1 compression(no alpha)",
                                "2", "ETC2 compression",
                                "3", "RGBA4444"
                        ),
                        KCVar.CreateCVar("harm_image_useCompressionCache", "bool", "0", "Cache ETC1/2 compression or RGBA4444 texture to filesystem", KCVar.FLAG_INIT)
                );

        KCVar.Group REALRTCW_CVARS = new KCVar.Group("RealRTCW", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_sv_cheats", "bool", "0", "Disable change `sv_cheats` when load map and disconnect for allow cheats", KCVar.FLAG_INIT
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowPersonal", "bool", "1", "Render personal stencil shadow when `cg_shadows` = 2", 0
                        )
                );

        _cvars.put("RENDERER", RENDERER_CVARS);
        _cvars.put("FRAMEWORK", FRAMEWORK_CVARS);
        _cvars.put("base", GAME_CVARS);
        _cvars.put("rivensin", RIVENSIN_CVARS);
        _cvars.put("q4base", QUAKE4_CVARS);
        _cvars.put("preybase", PREY_CVARS);
        _cvars.put("DOOM3BFG", DOOM3BFG_CVARS);
        _cvars.put("RealRTCW", REALRTCW_CVARS);

        return _cvars;
    }

    public static List<KCVar.Group> Match(String game)
    {
        Map<String, KCVar.Group> _cvars = CVars();
        List<KCVar.Group> res = new ArrayList<>();
        if(Q3EUtils.q3ei.isPrey)
        {
            res.add(_cvars.get("RENDERER"));
            res.add(_cvars.get("FRAMEWORK"));
        }
        else if(Q3EUtils.q3ei.isQ4)
        {
            res.add(_cvars.get("RENDERER"));
            res.add(_cvars.get("FRAMEWORK"));
        }
        else if(Q3EUtils.q3ei.isQ1) ;
        else if(Q3EUtils.q3ei.isQ2) ;
        else if(Q3EUtils.q3ei.isQ3) ;
        else if(Q3EUtils.q3ei.isRTCW) ;
        else if(Q3EUtils.q3ei.isTDM) ;
        else if(Q3EUtils.q3ei.isD3BFG)
            res.add(_cvars.get("DOOM3BFG"));
        else if(Q3EUtils.q3ei.isDOOM) ;
        else if(Q3EUtils.q3ei.isETW) ;
        else if(Q3EUtils.q3ei.isRealRTCW)
            res.add(_cvars.get("RealRTCW"));
        else
        {
            res.add(_cvars.get("RENDERER"));
            res.add(_cvars.get("FRAMEWORK"));
        }

        if(null == game || game.isEmpty())
        {
            if(Q3EUtils.q3ei.isPrey)
                res.add(_cvars.get("preybase"));
            else if(Q3EUtils.q3ei.isQ4)
                res.add(_cvars.get("q4base"));
            else if(Q3EUtils.q3ei.isD3)
                res.add(_cvars.get("base"));
        }
        else
        {
            if(_cvars.containsKey(game))
                res.add(_cvars.get(game));
        }
        return res;
    }
}
