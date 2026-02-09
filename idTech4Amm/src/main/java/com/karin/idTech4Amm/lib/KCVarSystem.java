package com.karin.idTech4Amm.lib;

import com.karin.idTech4Amm.misc.TextHelper;
import com.n0n3m4.q3e.Q3E;
import com.n0n3m4.q3e.Q3EUtils;
import com.n0n3m4.q3e.karin.KStr;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class KCVarSystem
{
    // KARIN_NEW_GAME_BOOKMARK: add new cvars
    public static Map<String, KCVar.Group> CVars()
    {
        Map<String, KCVar.Group> _cvars = new LinkedHashMap<>();

        KCVar.Group RENDERER_CVARS = new KCVar.Group("Renderer", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_r_clearVertexBuffer", "integer", "2", "Clear vertex buffer on every frame", KCVar.FLAG_LAUNCHER,
                                "0", "Not clear(original)",
                                "1", "Only free memory",
                                "2", "Free memory and delete VBO handle"
                        ),
                        KCVar.CreateCVar("harm_r_maxAllocStackMemory", "integer", "262144", "Control allocate temporary memory when load model data. 0 = Always heap; Negative = Always stack; Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory)", 0),
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
                        KCVar.CreateCVar("harm_r_PBRNormalCorrection", "float", "0.25", "Vertex normal correction(surface smoothness) in PBR interaction lighting model(1 = pure using bump texture(lower smoothness); 0 = pure using vertex normal(high smoothness); 0.0 - 1.0 = bump texture * harm_r_PBRNormalCorrection + vertex normal * (1 - harm_r_PBRNormalCorrection))", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_PBRRoughnessCorrection", "float", "0.55", "max roughness for old specular texture. 0 = disable; else = roughness = harm_r_PBRRoughnessCorrection - texture(specularTexture, st).r", 0),
                        KCVar.CreateCVar("harm_r_PBRMetallicCorrection", "float", "0", "min metallic for old specular texture. 0 = disable; else = metallic = texture(specularTexture, st).r + harm_r_PBRMetallicCorrection", 0),
                        KCVar.CreateCVar("harm_r_PBRRMAOSpecularMap", "bool", "0", "Specular map is standard PBR RAMO texture or old non-PBR texture", 0),
                        KCVar.CreateCVar("harm_r_ambientLightingBrightness", "float", "1.0", "Lighting brightness in ambient lighting", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("r_maxFps", "integer", "0", "Limit maximum FPS. 0 = unlimited", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),

                        KCVar.CreateCVar("r_screenshotFormat", "integer", "0", "Screenshot format", 0,
                        "0", "TGA (default)",
                                "1", "BMP",
                                "2", "PNG",
                                "3", "JPG",
                                "4", "DDS",
                                "5", "EXR",
                                "6", "HDR"
                                ),
                        KCVar.CreateCVar("r_screenshotJpgQuality", "integer", "75", "Screenshot quality for JPG images (0-100)", KCVar.FLAG_POSITIVE),
                        KCVar.CreateCVar("r_screenshotPngCompression", "integer", "3", "Compression level when using PNG screenshots (0-9)", KCVar.FLAG_POSITIVE),

                        KCVar.CreateCVar("r_useShadowMapping", "bool", "0", "use shadow mapping instead of stencil shadows", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("r_forceShadowMapsOnAlphaTestedSurfaces", "bool", "0", "render perforated surface to shadow map(DOOM 3 default is 1)", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_shadowMapAlpha", "float", "1.0", "Shadow's alpha in shadow mapping", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("r_shadowMapJitterScale", "float", "2.5", "scale factor for jitter offset", KCVar.FLAG_POSITIVE),
                        KCVar.CreateCVar("r_shadowMapSplits", "integer", "3", "number of splits for cascaded shadow mapping with parallel lights(0: disable, max is 4)", 0),
                        KCVar.CreateCVar("harm_r_shadowMapCombine", "bool", "1", "combine local and global shadow mapping", KCVar.FLAG_LAUNCHER),

                        KCVar.CreateCVar("harm_r_stencilShadowTranslucent", "bool", "0", "enable translucent shadow in stencil shadow", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowAlpha", "float", "1.0", "translucent shadow's alpha in stencil shadow", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowCombine", "bool", "0", "combine local and global stencil shadow", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowSoft", "bool", "0", "enable soft stencil shadow(Only OpenGLES3.1+)", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_stencilShadowSoftBias", "float", "-1", "soft stencil shadow sampler BIAS(-1 = automatic; 0 = disable; positive = value)", 0),
                        KCVar.CreateCVar("harm_r_stencilShadowSoftCopyStencilBuffer", "bool", "0", "copy stencil buffer directly for soft stencil shadow. 0: copy depth buffer and bind and renderer stencil buffer to texture directly; 1: copy stencil buffer to texture directly", 0),
                        KCVar.CreateCVar("harm_r_autoAspectRatio", "integer", "1", "automatic setup aspect ratio of view", KCVar.FLAG_LAUNCHER,
                                "0", "Manual",
                                "1", "Force setup r_aspectRatio to -1 (default)",
                                "2", "Automatic setup r_aspectRatio to 0,1,2 by screen size"
                        ),
                        KCVar.CreateCVar("harm_r_renderToolsMultithread", "bool", "1", "Enable render tools debug with GLES in multi-threading", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_useHighPrecision", "integer", "0", "Use high precision float on GLSL shader", KCVar.FLAG_LAUNCHER | KCVar.FLAG_INIT,
                                "0", "use default precision(interaction/depth shaders use high precision, otherwise use medium precision)",
                                "1", "all shaders use high precision as default precision exclude special variables",
                                "2", "all shaders use high precision as default precision and special variables also use high precision"
                        ),
                        KCVar.CreateCVar("harm_r_occlusionCulling", "bool", "0", "Enable DOOM3-BFG occlusion culling", KCVar.FLAG_LAUNCHER | KCVar.FLAG_INIT),
                        KCVar.CreateCVar("harm_r_globalIllumination", "bool", "0", "render global illumination before draw lighting interactions", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_globalIlluminationBrightness", "float", "0.3", "global illumination brightness", KCVar.FLAG_POSITIVE | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("r_renderMode", "integer", "0", "Enable retro postprocess rendering", KCVar.FLAG_LAUNCHER,
                                "0", "None",
                                "1", "CGA",
                                "2", "CGA Highres",
                                "3", "Commodore 64",
                                "4", "Commodore 64 Highres",
                                "5", "Amstrad CPC 6128",
                                "6", "Amstrad CPC 6128 Highres",
                                "7", "Sega Genesis",
                                "8", "Sega Genesis Highres",
                                "9", "Sony PSX"
                        ),
                        KCVar.CreateCVar("harm_r_useGLSLShaderBinaryCache", "integer", "0", "Use/cache GLSL shader compiled binary(OpenGL ES2.0 not support)", KCVar.FLAG_LAUNCHER,
                                "1", "Disable",
                                "2", "Enable(check source)",
                                "3", "Enable(not check source)"
                        ),
                        KCVar.CreateCVar("r_multithread", "bool", "1", "Multithread backend", KCVar.FLAG_INIT | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_r_openglVersion", "string", "GLES3.0", "OpenGL version", KCVar.FLAG_INIT | KCVar.FLAG_LAUNCHER,
                                "GLES2", "OpenGL ES2.0",
                                "GLES3.0", "OpenGL 3.0+"
                        ),
                        KCVar.CreateCommand("multithread", "string", "Enable/disable multi-threading rendering", 0)
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
                    KCVar.CreateCVar("r_scaleMenusTo43", "integer", "0", "Scale menus, fullscreen videos and PDA to 4:3 aspect ratio", 0,
                            "0", "disable",
                            "1", "only scale menu type GUI as 4:3 aspect ratio",
                            "-1", "scale all GUI as 4:3 aspect ratio"
                    ),
                    KCVar.CreateCVar("harm_in_smoothJoystick", "bool", "0", "Enable smooth joystick(Automatic setup by Android layer)", KCVar.FLAG_AUTO),
                    KCVar.CreateCVar("com_disableAutoSaves", "bool", "0", "Don't create Autosaves when entering a new map", 0),
                    KCVar.CreateCVar("harm_gui_wideCharLang", "bool", "0", "enable wide character language support", 0),
                    KCVar.CreateCVar("harm_gui_useD3BFGFont", "bool", "0", "use DOOM3-BFG fonts instead of old fonts", KCVar.FLAG_LAUNCHER | KCVar.FLAG_INIT,
                            "0", "disable",
                            "\"\"", "disable",
                            "1", "make DOOM3 old fonts mapping to DOOM3-BFG new fonts automatic(Only for DOOM3, Quake4 and Prey not support). e.g. \n"
                            + " In DOOM 3: \n"
                            + " 'fonts/fontImage_**.dat' -> 'newfonts/Chainlink_Semi_Bold/48.dat'\n"
                            + " 'fonts/an/fontImage_**.dat' -> 'newfonts/Arial_Narrow/48.dat' \n"
                            + " 'fonts/arial/fontImage_**.dat' -> 'newfonts/Arial_Narrow/48.dat' \n"
                            + " 'fonts/bank/fontImage_**.dat' -> 'newfonts/BankGothic_Md_BT/48.dat' \n"
                            + " 'fonts/micro/fontImage_**.dat' -> 'newfonts/microgrammadbolext/48.dat' \n"
                            + "\n"
                            + " In Quake 4(`r_strogg` and `strogg` fonts always disable): \n"
                            + " 'fonts/chain_**.dat' -> 'newfonts/Chainlink_Semi_Bold/48.dat'\n"
                            + " 'fonts/lowpixel_**.dat' -> 'newfonts/microgrammadbolext/48.dat' \n"
                            + " 'fonts/marine_**.dat' -> 'newfonts/Arial_Narrow/48.dat' \n"
                            + " 'fonts/profont_**.dat' -> 'newfonts/BankGothic_Md_BT/48.dat' \n"
                            + "\n"
                            + " In Prey(`alien` font always disable): \n"
                            + " 'fonts/fontImage_**.dat' -> 'newfonts/Chainlink_Semi_Bold/48.dat'\n"
                            + " 'fonts/menu/fontImage_**.dat' -> 'newfonts/Arial_Narrow/48.dat' \n"
                            + "\n",
                            "<DOOM3-BFG font name>", "use a DOOM3-BFG new font by name override all DOOM 3/Quake 4/Prey old fonts. e.g. "
                            + " Chainlink_Semi_Bold "
                            + " Arial_Narrow "
                            + " BankGothic_Md_BT "
                            + " microgrammadbolext "
                            + " DFPHeiseiGothicW7 "
                            + " Sarasori_Rg "
                            ),
                        KCVar.CreateCVar("harm_g_skipHitEffect", "bool", "0", "Skip all hit effect in game", KCVar.FLAG_INIT | KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_fs_basepath_extras", "string", "", "Extras search paths last(split by ',')", KCVar.FLAG_INIT),
                        KCVar.CreateCVar("harm_fs_addon_extras", "string", "", "Extras search addon files directory path last(split by ',')", KCVar.FLAG_INIT),
                        KCVar.CreateCVar("harm_fs_game_base_extras", "string", "", "Extras search game mod last(split by ',')", KCVar.FLAG_INIT),
                        KCVar.CreateCVar("harm_con_float", "bool", "0", "Enable float console", 0),
                        KCVar.CreateCVar("harm_con_alwaysShow", "bool", "0", "Always show console", 0),
                        KCVar.CreateCVar("harm_con_noBackground", "bool", "0", "Don't draw console background", 0),
                        KCVar.CreateCVar("harm_con_floatGeometry", "vector4", "100 50 300 200", "Float console geometry, format is \"<left> <top> <width> <height>\"", 0),
                        KCVar.CreateCVar("harm_con_floatZoomStep", "integer", "10", "Zoom step of float console", 0),
                        KCVar.CreateCVar("harm_sys_sse2neon", "bool", "0", "Emulate MMX/SSE/SSE2 SIMD by sse2neon on arm32/arm64 device", KCVar.FLAG_INIT),
                        KCVar.CreateCommand("exportFont", "string", "Convert ttf/ttc font file to DOOM3 wide character font file", 0),
                        KCVar.CreateCommand("extractBimage", "string", "extract DOOM3-BFG's bimage image to rga RGBA image files", 0),
                        KCVar.CreateCommand("skipHitEffect", "bool", "skip all hit effect in game", 0),
                        KCVar.CreateCommand("idTech4AmmSettings", "string", "Show idTech4A++ new cvars and commands", 0)
                );
        KCVar.Group GAME_CVARS = new KCVar.Group("DOOM3", false)
                .AddCVar(
                    KCVar.CreateCVar("harm_pm_fullBodyAwareness", "bool", "0", "Enables full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset, format is \"<forward-offset> <side-offset> <up-offset>\"", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadJoint", "string", "Head", "Set head joint when without head model in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessFixed", "bool", "0", "Do not attach view position to head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_ui_showViewBody", "bool", "0", "show view body(mod)", 0),
                    KCVar.CreateCVar("harm_ui_showViewLight", "bool", "0", "show player view flashlight(mod)", 0),
                    KCVar.CreateCVar("harm_ui_viewLightShader", "string", "lights/flashlight5", "player view flashlight material texture/entityDef name", 0),
                    KCVar.CreateCVar("harm_ui_viewLightRadius", "vector3", "1280 640 640", "player view flashlight radius, format is \"<light_target> <light_right> <light_up>\"", 0),
                    KCVar.CreateCVar("harm_ui_viewLightOffset", "string", "20 0 0", "player view flashlight origin offset, format is \"<forward-offset> <side-offset> <up-offset>\"", 0),
                    KCVar.CreateCVar("harm_ui_viewLightType", "integer", "0", "player view flashlight type", 0,
                        "0", "spot light",
                        "1", "point light"
                    ),
                    KCVar.CreateCVar("harm_ui_viewLightOnWeapon", "bool", "0", "player view flashlight follow weapon position", 0),
                    KCVar.CreateCVar("harm_g_autoGenAASFileInMPGame", "bool", "1", "For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic", 0),
                    KCVar.CreateCVar("harm_si_autoFillBots", "bool", "0", "Automatic fill bots after map loaded in multiplayer game(0 = disable; -1 = max; other number = bot num)", 0),
                    KCVar.CreateCVar("harm_si_botLevel", "integer", "0", "Bot difficult level(0 = auto)", 0),
                    KCVar.CreateCVar("harm_si_botWeapons", "string", "0", "Bot initial weapons when spawn, separate by comma(,); 0=none, *=all. Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_shotgun,weapon_machinegun), and allow mix(e.g. shotgun,3,weapon_rocketlauncher). All weapon: 1=pistol, 2=shotgun, 3=machinegun, 4=chaingun, 5=handgrenade, 6=plasmagun, 7=rocketlauncher, 8=BFG, 10=chainsaw.", 0),
                    KCVar.CreateCVar("harm_si_botAmmo", "integer", "0", "Bot weapons initial ammo clip when spawn, depend on `harm_si_botWeapons`. -1=max ammo, 0=none, >0=ammo", 0),
                    KCVar.CreateCVar("harm_g_botEnableBuiltinAssets", "bool", "0", "enable built-in bot assets if external assets missing", KCVar.FLAG_INIT),
                    KCVar.CreateCVar("harm_si_useCombatBboxInMPGame", "bool", "0", "players force use combat bbox in multiplayer game", 0),
                    KCVar.CreateCommand("addBots", "string", "add multiplayer bots batch", 0),
                    KCVar.CreateCommand("removeBots", "integer", "disconnect multi bots by client ID", 0),
                    KCVar.CreateCommand("fillBots", "integer", "fill bots to maximum of server", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("appendBots", "integer", "append more bots(over maximum of server)", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("cleanBots", "", "disconnect all bots", 0),
                    KCVar.CreateCommand("truncBots", "integer", "disconnect last bots", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("botLevel", "integer", "setup all bot level", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("botWeapons", "string", "setup all bot initial weapons", 0),
                    KCVar.CreateCommand("botAmmo", "integer", "setup all bot weapons initial ammo clip", 0),
                    KCVar.CreateCommand("addBot", "string", "adds a new bot", 0),
                    KCVar.CreateCommand("removeBot", "string", "removes bot specified by id", 0)
                );
        KCVar.Group RIVENSIN_CVARS = new KCVar.Group("Rivensin", false)
                .AddCVar(
                    KCVar.CreateCVar("harm_pm_doubleJump", "bool", "1", "Enable double-jump", 0),
                    KCVar.CreateCVar("harm_pm_autoForceThirdPerson", "bool", "1", "Force set third person view after game level load end", 0),
                    KCVar.CreateCVar("harm_pm_preferCrouchViewHeight", "float", "32", "Set prefer crouch view height in Third-Person(suggest 32 - 39, less or equals 0 to disable)", KCVar.FLAG_POSITIVE)
                );

        KCVar.Group QUAKE4_CVARS = new KCVar.Group("Quake4", false)
                .AddCVar(
                    KCVar.CreateCVar("harm_gui_defaultFont", "string", "chain", "Default font name", 0,
                            "chain", "fonts/chain",
                            "lowpixel", "fonts/lowpixel",
                            "marine", "fonts/marine",
                            "profont", "fonts/profont",
                            "r_strogg", "fonts/r_strogg",
                            "strogg", "fonts/strogg"
                    ),
                    KCVar.CreateCVar("harm_g_autoGenAASFileInMPGame", "bool", "1", "For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic", 0),
                    KCVar.CreateCVar("harm_si_autoFillBots", "bool", "0", "Automatic fill bots after map loaded in multiplayer game(0 = disable; -1 = max; other number = bot num)", 0),
                    KCVar.CreateCVar("harm_si_botLevel", "integer", "0", "Bot difficult level(0 = auto)", 0),
                    KCVar.CreateCVar("harm_si_botWeapons", "string", "0", "Bot initial weapons when spawn, separate by comma(,); 0=none, *=all. Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_machinegun,weapon_shotgun), and allow mix(e.g. machinegun,3,weapon_rocketlauncher). All weapon: 1=machinegun, 2=shotgun, 3=hyperblaster, 4=grenadelauncher, 5=nailgun, 6=rocketlauncher, 7=railgun, 8=lightninggun, 9=dmg, 10=napalmgun.", 0),
                    KCVar.CreateCVar("harm_si_botAmmo", "integer", "0", "Bot weapons initial ammo clip when spawn, depend on `harm_si_botWeapons`. -1=max ammo, 0=none, >0=ammo", 0),
                    KCVar.CreateCVar("harm_g_botEnableBuiltinAssets", "bool", "0", "enable built-in bot assets if external assets missing", KCVar.FLAG_INIT),
                    KCVar.CreateCommand("addBots", "string", "adds multiplayer bots batch", 0),
                    KCVar.CreateCommand("removeBots", "integer", "disconnect multi bots by client ID", 0),
                    KCVar.CreateCommand("fillBots", "integer", "fill bots to maximum of server", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("appendBots", "integer", "append more bots(over maximum of server)", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("cleanBots", "", "disconnect all bots", 0),
                    KCVar.CreateCommand("truncBots", "integer", "disconnect last bots", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("botLevel", "integer", "setup all bot level", KCVar.FLAG_POSITIVE),
                    KCVar.CreateCommand("botWeapons", "string", "setup all bot initial weapons", 0),
                    KCVar.CreateCommand("botAmmo", "integer", "setup all bot weapons initial ammo clip", 0),
                    KCVar.CreateCommand("addBot", "string", "adds a new bot", 0),
                    KCVar.CreateCommand("removeBot", "string", "removes bot specified by id", 0),
                    KCVar.CreateCVar("harm_g_mutePlayerFootStep", "bool", "0", "Mute player's footstep sound", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwareness", "bool", "0", "Enables full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset, format is \"<forward-offset> <side-offset> <up-offset>\"", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadJoint", "string", "head_channel", "Set head joint when without head model in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessFixed", "bool", "0", "Do not attach view position to head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_ui_showViewBody", "bool", "0", "show view body(mod)", 0),
                    KCVar.CreateCVar("harm_g_mutePlayerFootStep", "bool", "0", "mute player's footstep sound", 0),
                    KCVar.CreateCVar("harm_g_allowFireWhenFocusNPC", "bool", "0", "allow fire when focus NPC", 0)
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
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessOffset", "vector3", "0 0 0", "Full-body awareness offset, format is \"<forward-offset> <side-offset> <up-offset>\"", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadJoint", "string", "neck", "Set head joint when without head model in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessFixed", "bool", "0", "Do not attach view position to head in full-body awareness", 0),
                    KCVar.CreateCVar("harm_pm_fullBodyAwarenessHeadVisible", "bool", "0", "Do not suppress head in full-body awareness", 0)
                );

        KCVar.Group DOOM3BFG_CVARS = new KCVar.Group("DOOM 3 BFG", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_r_useMediumPrecision", "bool", "0", "Use medium precision float instead of high precision in GLSL shader", KCVar.FLAG_INIT),
                        KCVar.CreateCVar("harm_image_useCompression", "integer", "0", "Use ETC1/2 compression or RGBA4444 texture for low memory(e.g. 32bits device), it will using lower memory but loading slower", KCVar.FLAG_INIT,
                                "0", "RGBA8",
                                "1", "ETC1 compression(no alpha)",
                                "2", "ETC2 compression",
                                "3", "RGBA4444"
                        ),
                        KCVar.CreateCVar("harm_image_useCompressionCache", "bool", "0", "Cache ETC1/2 compression or RGBA4444 texture to filesystem", KCVar.FLAG_INIT)
                );

        KCVar.Group TDM_CVARS = new KCVar.Group("The Dark Mod", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_r_useMediumPrecision", "bool", "0", "Use medium precision float instead of high precision in GLSL shader", KCVar.FLAG_INIT),
                        KCVar.CreateCVar("harm_r_outputGLSLSource", "bool", "0", "Output all generated GLSL shaders to 'generated_glsl/'", KCVar.FLAG_INIT)
                );

        KCVar.Group REALRTCW_CVARS = new KCVar.Group("RealRTCW", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_sv_cheats", "bool", "0", "Disable change `sv_cheats` when load map and disconnect for allow cheats", KCVar.FLAG_INIT | KCVar.FLAG_LAUNCHER
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowPersonal", "bool", "1", "Render personal stencil shadow when `cg_shadows` = 2", KCVar.FLAG_LAUNCHER
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowOp", "integer", "0", "Stencil testing operation", 0,
                                "0", "Automatic(Personal shadow using Z-Fail, other shadow using Z-Pass)",
                                "1", "Z-Pass",
                                "2", "Z-Fail"
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowMaxAngle", "integer", "-1", "Limit stencil shadow of light direction and negative-Z-axis max angle(-1=not limit)", 0
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowModel", "integer", "3", "Render stencil shadow model type mask(mask bit are 1 2 4 8 16, 0=all models; 3=all animation model)", KCVar.FLAG_POSITIVE
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowCap", "bool", "1", "Render stencil shadow volume caps(0=don't render caps, personal shadow is always render)", 0
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowInfinite", "float", "0", "Stencil shadow volume far is infinite(absolute value as volume's length; 0=512. negative value is infinite, personal shadow is always infinite)", 0
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowMask", "bool", "0", "Render stencil shadow mask(0=render mask after all shadows; 1=render mask every shadow volume)", 0
                        )
                );

        KCVar.Group ETW_CVARS = new KCVar.Group("ETW", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_r_stencilShadowPersonal", "bool", "1", "Render personal stencil shadow when `cg_shadows` = 2", KCVar.FLAG_LAUNCHER
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowOp", "integer", "0", "Stencil testing operation", 0,
                                "0", "Automatic(Personal shadow using Z-Fail, other shadow using Z-Pass)",
                                "1", "Z-Pass",
                                "2", "Z-Fail"
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowMaxAngle", "integer", "-1", "Limit stencil shadow of light direction and negative-Z-axis max angle(-1=not limit)", 0
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowModel", "integer", "3", "Render stencil shadow model type mask(mask bit are 1 2 4 8, 0=all models; 3=all animation model)", KCVar.FLAG_POSITIVE
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowCap", "bool", "1", "Render stencil shadow volume caps(0=don't render caps, personal shadow is always render)", 0
                        ),
                        KCVar.CreateCVar("harm_r_stencilShadowInfinite", "float", "0", "Stencil shadow volume far is infinite(absolute value as volume's length; 0=512. negative value is infinite, personal shadow is always infinite)", 0
                                ),
                        KCVar.CreateCVar("harm_r_stencilShadowMask", "bool", "0", "Render stencil shadow mask(0=render mask after all shadows; 1=render mask every shadow volume)", 0
                        ),
                        KCVar.CreateCVar("harm_ui_disableAndroidMacro", "bool", "0", "Disable `ANDROID` menu macro, show `HOST` menu for create game server", KCVar.FLAG_LAUNCHER
                        )
                );

        KCVar.Group ZDOOM_CVARS = new KCVar.Group("ZDOOM", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_gl_es", "integer", "0", "OpenGLES version", KCVar.FLAG_LAUNCHER | KCVar.FLAG_INIT,
                                "0", "Automatic",
                                "2", "OpenGL ES2.0",
                                "3", "OpenGL ES3.0(GLSL shader version is 100)",
                                "4", "OpenGL ES3.0(GLSL shader version is 300 es)"
                        ),
                        KCVar.CreateCVar("gl_glsl_precision", "integer", "0", "OpenGL shader default precision", KCVar.FLAG_INIT,
                                "0", "highp",
                                "1", "mediump",
                                "2", "lowp"
                        )
                );

        KCVar.Group URT_CVARS = new KCVar.Group("UrT", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_bot_autoAdd", "integer", "0", "Add bots automatic(0 = disable; -1 = maximum; Positive number = bots num)", KCVar.FLAG_LAUNCHER),
                        KCVar.CreateCVar("harm_bot_level", "integer", "0", "Bot difficulty level(0 = random; 1 - 5 = bot difficulty level)", KCVar.FLAG_LAUNCHER | KCVar.FLAG_POSITIVE)
                );

        KCVar.Group XASH3D_CVARS = new KCVar.Group("Xash3D", true)
                .AddCVar(
                        KCVar.CreateParam("sv_cl", "string", "", "Server/Client", KCVar.FLAG_LAUNCHER | KCVar.FLAG_INIT,
                                "\"\"", "Half-Life",
                                "hlsdk", "Half-Life",
                                "cs16", "Counter-Strike 1.6",
                                "cs16_yapb", "Counter-Strike 1.6(yapb)"
                                )
                );

        KCVar.Group SOURCE_CVARS = new KCVar.Group("Source", true)
                .AddCVar(
                        KCVar.CreateParam("sv_cl", "string", "", "Server/Client", KCVar.FLAG_LAUNCHER | KCVar.FLAG_INIT,
                                "\"\"", "Half-Life 2",
                                "hl2", "Half-Life 2",
                                "cstrike", "Counter-Strike: Source",
                                "portal", "Portal",
                                "dod", "Day of Defeat: Source",
                                "episodic", "Half-Life 2: Episodic 1 & 2",
                                "hl2mp", "Half-Life 2: Deathmatch",
                                "hl1", "Half-Life 1: Source"/*,
                                "hl1mp", "Half-Life 1 Deathmatch: Source"*/
                        )
                );

        KCVar.Group SKINDEEP_CVARS = new KCVar.Group("SkinDeep", true)
                .AddCVar(
                        KCVar.CreateCVar("harm_r_clearVertexBuffer", "integer", "2", "Clear vertex buffer on every frame", KCVar.FLAG_INIT,
                                "0", "Not clear(original)",
                                "1", "Only free memory",
                                "2", "Free memory and delete VBO handle"
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
        _cvars.put("ETW", ETW_CVARS);
        _cvars.put("TDM", TDM_CVARS);
        _cvars.put("ZDOOM", ZDOOM_CVARS);
        _cvars.put("UrT", URT_CVARS);
        _cvars.put("Xash3D", XASH3D_CVARS);
        _cvars.put("Source", SOURCE_CVARS);
        _cvars.put("SkinDeep", SKINDEEP_CVARS);

        return _cvars;
    }

    // KARIN_NEW_GAME_BOOKMARK: add new cvars to map
    public static List<KCVar.Group> Match(String game)
    {
        Map<String, KCVar.Group> _cvars = CVars();
        List<KCVar.Group> res = new ArrayList<>();
        if(Q3E.q3ei.isPrey)
        {
            res.add(_cvars.get("RENDERER"));
            res.add(_cvars.get("FRAMEWORK"));
        }
        else if(Q3E.q3ei.isQ4)
        {
            res.add(_cvars.get("RENDERER"));
            res.add(_cvars.get("FRAMEWORK"));
        }
        else if(Q3E.q3ei.isTDM)
            res.add(_cvars.get("TDM"));
        else if(Q3E.q3ei.isD3BFG)
            res.add(_cvars.get("DOOM3BFG"));
        else if(Q3E.q3ei.isDOOM)
            res.add(_cvars.get("ZDOOM"));
        else if(Q3E.q3ei.isETW)
            res.add(_cvars.get("ETW"));
        else if(Q3E.q3ei.isRealRTCW)
            res.add(_cvars.get("RealRTCW"));
        else if(Q3E.q3ei.isXash3D)
            res.add(_cvars.get("Xash3D"));
        else if(Q3E.q3ei.isUrT)
            res.add(_cvars.get("UrT"));
        else if(Q3E.q3ei.isSource)
            res.add(_cvars.get("Source"));
        else if(Q3E.q3ei.isSkinDeep)
            res.add(_cvars.get("SkinDeep"));
        else if(Q3E.q3ei.isD3)
        {
            res.add(_cvars.get("RENDERER"));
            res.add(_cvars.get("FRAMEWORK"));
        }

        if(null == game || game.isEmpty())
        {
            if(Q3E.q3ei.isPrey)
                res.add(_cvars.get("preybase"));
            else if(Q3E.q3ei.isQ4)
                res.add(_cvars.get("q4base"));
            else if(Q3E.q3ei.isD3)
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
