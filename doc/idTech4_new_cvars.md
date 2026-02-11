## idTech4's new Cvar

----------------------------------------------------------------------------------

> #### Flag
* ARCHIVE = save to/load from config file
* FIXED = can't change in game
* READONLY = readonly, always can't change
* INIT = only setup on boot command
* ISSUE = maybe has bugs
* DEBUG = only for developer debug and test
* DISABLE = only for disable something and keep original source code

----------------------------------------------------------------------------------

> #### Renderer
| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_r_openglVersion | String | GLES3.0 | OpenGL version | INIT | GLES2, GLES3.0, OpenGL_core, OpenGL_compatibility | Engine/Renderer | OpenGL_core, OpenGL_compatibility will use OpenGL desktop version, setup with launcher on Android | Windows, Linux |
| r_multithread | Bool | 1 | Multithread backend | INIT |  | Engine/Renderer | setup with launcher on Android | Windows, Linux |
| harm_r_clearVertexBuffer | Integer | 2 | Clear vertex buffer on every frame | ARCHIVE, FIXED | 0, 1, 2 | Engine/Renderer | 0 = not clear(original);<br/> 1 = only free VBO memory;<br/> 2 = free VBO memory and delete VBO handle | All |
| harm_r_maxAllocStackMemory | Integer | 262144 | Control allocate temporary memory when load model data | ARCHIVE |  | Engine/Renderer | For load large model, because stack memory is limited on OS.<br/> 0 = Always heap;<br/> Negative = Always stack;<br/> Positive = Max stack memory limit(If less than this `byte` value, call `alloca` in stack memory, else call `malloc`/`calloc` in heap memory) | All |
| harm_r_shadowCarmackInverse | Bool | 0 | Stencil shadow using Carmack-Inverse | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_globalIllumination | Bool | 0 | Render global illumination before draw interactions | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_globalIlluminationBrightness | Float | 0.3 | Global illumination brightness | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_lightingModel | Integer | 1 | Lighting model when draw interactions | ARCHIVE | 0, 1, 2, 3 | Engine/Renderer | 1 = Phong(default);<br/> 2 = Blinn-Phong;<br/> 3 = PBR;<br/> 0 = Ambient(no lighting) | All |
| harm_r_specularExponent | Float | 3.0 | Specular exponent in Phong interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_specularExponentBlinnPhong | Float | 12.0 | Specular exponent in Blinn-Phong interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_specularExponentPBR | Float | 5.0 | Specular exponent in PBR interaction lighting model | ARCHIVE | &gt;= 0.0 | Engine/Renderer |  | All |
| harm_r_PBRNormalCorrection | Float | 0.25 | Vertex normal correction(surface smoothness) in PBR interaction lighting model | ARCHIVE | [0.0 - 1.0] | Engine/Renderer | 1 = pure using bump texture(lower smoothness); 0 = pure using vertex normal(high smoothness); 0.0 - 1.0 = bump texture * harm_r_PBRNormalCorrection + vertex normal * (1 - harm_r_PBRNormalCorrection) | All |
| harm_r_PBRRoughnessCorrection | Float | 0.55 | max roughness for old specular texture | ARCHIVE | &gt;= 0.0 | Engine/Renderer | 0 = disable; else = roughness = harm_r_PBRRoughnessCorrection - texture(specularTexture, st).r | All |
| harm_r_PBRMetallicCorrection | Float | 0 | min metallic for old specular texture | ARCHIVE | &gt;= 0.0 | Engine/Renderer | 0 = disable; else = metallic = texture(specularTexture, st).r + harm_r_PBRMetallicCorrection | All |
| harm_r_PBRRMAOSpecularMap | Bool | 0 | pecular map is standard PBR RAMO texture or old non-PBR texture | ARCHIVE |  | Engine/Renderer |  | All |
| r_maxFps | Integer | 0 | Limit maximum FPS. | ARCHIVE | &gt;= 0 | Engine/Renderer | 0 = unlimited | All |
| r_screenshotFormat | Integer | 0 | Screenshot format | ARCHIVE | 0, 1, 2, 3, 4 | Engine/Renderer | 0 = TGA (default),<br/> 1 = BMP,<br/> 2 = PNG,<br/> 3 = JPG,<br/> 4 = DDS | All |
| r_screenshotJpgQuality | Integer | 75 | Screenshot quality for JPG images | ARCHIVE | [0 - 100] | Engine/Renderer |  | All |
| r_screenshotPngCompression | Integer | 3 | Compression level when using PNG screenshots | ARCHIVE | [0 - 9] | Engine/Renderer |  | All |
| r_useShadowMapping | Bool | 0 | use shadow mapping instead of stencil shadows | ARCHIVE | All | Engine/Renderer |  | All |
| harm_r_shadowMapAlpha | Float | 1.0 | shadow's alpha in shadow mapping | ARCHIVE | [0.0 - 1.0] | Engine/Renderer |  | All |
| harm_r_shadowMapJitterScale | Float | 2.5 | scale factor for jitter offset | ARCHIVE |  | Engine/Renderer |  | All |
| r_forceShadowMapsOnAlphaTestedSurfaces | Bool | DOOM3 is 1, Quake4/Prey is 0 | render performed material shadow in shadow mapping | ARCHIVE, ISSUE |  | Engine/Renderer | 0 = same shadowing as with stencil shadows,<br/> 1 = ignore noshadows for alpha tested materials | All |
| harm_r_shadowMapCombine | Bool | 1 | combine local and global shadow mapping | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_shadowMapLightType | Integer | 0 | debug light type mask in shadow mapping | DEBUG | 0, 1, 2, 3, 4, 5, 6, 7 | Engine/Renderer | 1 = parallel;<br/> 2 = point;<br/> 4 = spot | All |
| harm_r_shadowMapDepthBuffer | Integer | 0 | render depth to color or depth texture in OpenGLES2.0 | INIT, DEBUG | 0, 1, 2, 3, 4 | Engine/Renderer | 0 = Auto;<br/> 1 = depth texture;<br/> 2 = color texture's red;<br/> 3 = color texture's rgba | All |
| harm_r_shadowMapNonParallelLightUltra | Bool | 0 | non parallel light allow ultra quality shadow map texture |  |  | Engine/Renderer | max texture size is 2048x2048 | All |
| harm_r_shadowMapLod | Integer | -1 | force using shadow map LOD |  | -1, 0, 1, 2, 3, 4 | Engine/Renderer | -1 = auto | All |
| r_shadowMapFrustumFOV | Float | 90 | oversize FOV for point light side matching |  |  | Engine/Renderer |  | All |
| harm_r_stencilShadowTranslucent | Bool | 0 | enable translucent shadow in stencil shadow | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_stencilShadowAlpha | Float | 1.0 | translucent shadow's alpha in stencil shadow | ARCHIVE | [0.0 - 1.0] | Engine/Renderer |  | All |
| harm_r_stencilShadowCombine | Bool | 0 | combine local and global stencil shadow | ARCHIVE, ISSUE |  | Engine/Renderer |  | All |
| harm_r_stencilShadowSoft | Bool | 0 | enable soft stencil shadow | ARCHIVE | only OpenGLES3.1+ | Engine/Renderer |  | All |
| harm_r_stencilShadowSoftBias | Float | -1 | soft stencil shadow sampler BIAS | ARCHIVE |  | Engine/Renderer | -1 to automatic; 0 = disable; positive = value | All |
| harm_r_stencilShadowSoftCopyStencilBuffer | Bool | 0 | copy stencil buffer directly for soft stencil shadow | ARCHIVE | 0 = copy depth buffer and bind and renderer stencil buffer to texture directly<br/>1 = copy stencil buffer to texture directly | Engine/Renderer |  | All |
| r_renderMode | Integer | 0 | retro postprocess render | ARCHIVE | 0 = Doom<br/>1 = CGA<br/>2 = CGA Highres<br/>3 = Commodore 64<br/>4 = Commodore 64 Highres<br/>5 = Amstrad CPC 6128<br/>6 = Amstrad CPC 6128 Highres<br/>7 = Sega Genesis<br/>8 = Sega Genesis Highres<br/>9 = Sony PSX | Engine/Renderer |  | All |
| harm_r_autoAspectRatio | Integer | 1 | automatic setup aspect ratio of view | ARCHIVE | 0, 1, 2 | Engine/Renderer | 0 = manual<br/>1 = force setup r_aspectRatio to -1<br/>2 = automatic setup r_aspectRatio to 0,1,2 by screen size | Android |
| harm_r_renderToolsMultithread | Bool | 1 | Enable render tools debug with GLES in multi-threading | ARCHIVE |  | Engine/Renderer |  | All |
| r_useETC1 | Bool | 0 | use ETC1 compression | INIT |  | Engine/Renderer |  | All |
| r_useETC1cache | Bool | 0 | use ETC1 compression | INIT |  | Engine/Renderer |  | All |
| r_useDXT | Bool | 0 | use DXT compression if possible | INIT |  | Engine/Renderer |  | All |
| r_useETC2 | Bool | 0 | use ETC2 compression instead of RGBA4444 | INIT |  | Engine/Renderer | Only for OpenGLES3.0+ | All |
| r_noLight | Bool | 0 | lighting disable hack | INIT |  | Engine/Renderer | 1 = disable lighting(not allow switch, must setup on command line) | All |
| r_showStencil | Bool | 0 | display the contents of the stencil index buffer |  |  | Engine/Renderer |  | All |
| harm_r_useGLSLShaderBinaryCache | Integer | 0 | Use GLSL shader compiled binary cache | INIT | 0, 1, 2 | Engine/Renderer | 0 = Disable<br/>1 = Enable and check<br/>2 = Enable and uncheck | All |
| harm_r_useHighPrecision | Integer | Android = 0; Other = 1 | Use high precision float on GLSL shade | INIT | 0, 1, 2 | Engine/Renderer | 0 = use default precision(interaction/depth shaders use high precision, otherwise use medium precision)<br/>1 = all shaders use high precision as default precision exclude special variables<br/>2 = all shaders use high precision as default precision and special variables also use high precision | All |
| harm_r_occlusionCulling | Bool | 0 | enable DOOM3-BFG occlusion culling | ARCHIVE |  | Engine/Renderer |  | All |
| harm_r_shaderProgramDir | String |  | Setup external OpenGLES2 GLSL shader program directory path | ARCHIVE |  | Engine/Renderer | empty is glslprogs(default) | All |
| harm_r_shaderProgramES3Dir | String |  | Setup external OpenGLES3 GLSL shader program directory path | ARCHIVE |  | Engine/Renderer | empty is glsl3progs(default) | All |
| harm_r_debugOpenGL | Bool | 0 | debug OpenGL | ARCHIVE | only OpenGLES3.1+ | Engine/Renderer |  | All |


> #### Engine
| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_fs_gameLibPath | String |  | Setup game dynamic library | ARCHIVE |  | Engine/Framework |  | Android |
| harm_fs_gameLibDir | String |  | Setup game dynamic library directory path | ARCHIVE |  | Engine/Framework |  | Android |
| harm_com_consoleHistory | Integer | 2 | Save/load console history | ARCHIVE | 0, 1, 2 | Engine/Framework | 0 = disable;<br/> 1 = loading in engine initialization, and saving in engine shutdown;<br/> 2 = loading in engine initialization, and saving in every executing | All |
| harm_con_float | Bool | 0 | Enable float console | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_alwaysShow | Bool | 0 | Always show console | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_noBackground | Bool | 0 | Don't draw console background | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_floatGeometry | String | 100 50 300 200 | Float console geometry, format is "<left> <top> <width> <height>"<br/>Holding left mouse or CTRL and moving mouse to move float console position<br/>Holding left mouse or CTRL and swiping mouse wheel to resize float console. | ARCHIVE |  | Engine/Framework |  | All |
| harm_con_floatZoomStep | Integer | 10 | Zoom step of float console when holding left mouse or CTRL and swiping mouse wheel for resize float console | ARCHIVE |  | Engine/Framework |  | All |
| com_disableAutoSaves | Bool | 0 | Don't create Autosaves when entering a new map | ARCHIVE |  | Engine/Framework |  | All |
| harm_fs_basepath_extras | String |  | Extras search paths last(split by ',') | INIT |  | Engine/Framework |  | All |
| harm_fs_addon_extras | String |  | Extras search addon files directory path last(split by ',') | INIT |  | Engine/Framework |  | All |
| harm_fs_game_base_extras | String |  | Extras search game mod last(split by ',') | INIT |  | Engine/Framework |  | All |
| r_scaleMenusTo43 | Integer | 0 | Scale menus, fullscreen videos and PDA to 4:3 aspect ratio | ARCHIVE | 0, 1, -1 | Engine/GUI | 0 = disable;<br/> 1 = only scale menu type GUI as 4:3 aspect ratio;<br/> -1 = scale all GUI as 4:3 aspect ratio | All |
| harm_gui_wideCharLang | Bool | 0 | enable wide character language support | ARCHIVE |  | Engine/GUI |  | All |
| harm_gui_useD3BFGFont | String | 0 | use DOOM3-BFG fonts instead of old fonts | INIT | 0, "", 1, &lt;DOOM3-BFG font name&gt; | Engine/GUI | 0 or "" = disable;<br/><br/> 1 = make DOOM3 old fonts mapping to DOOM3-BFG new fonts automatic. e.g. <br/> - In DOOM 3: <br/> * 'fonts/fontImage_xx.dat' -&gt; 'newfonts/Chainlink_Semi_Bold/48.dat'<br/> * 'fonts/an/fontImage_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/> * 'fonts/arial/fontImage_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/> * 'fonts/bank/fontImage_xx.dat' -&gt; 'newfonts/BankGothic_Md_BT/48.dat'<br/> * 'fonts/micro/fontImage_xx.dat' -&gt; 'newfonts/microgrammadbolext/48.dat'<br/> - In Quake 4(`r_strogg` and `strogg` fonts always disable): <br/> *  'fonts/chain_xx.dat' -&gt; 'newfonts/Chainlink_Semi_Bold/48.dat'<br/> * 'fonts/lowpixel_xx.dat' -&gt; 'newfonts/microgrammadbolext/48.dat'<br/> * 'fonts/marine_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/> * 'fonts/profont_xx.dat' -&gt; 'newfonts/BankGothic_Md_BT/48.dat'<br/> - In Prey(`alien` font always disable): <br/> * 'fonts/fontImage_xx.dat' -&gt; 'newfonts/Chainlink_Semi_Bold/48.dat'<br/> * 'fonts/menu/fontImage_xx.dat' -&gt; 'newfonts/Arial_Narrow/48.dat'<br/><br/> &lt;DOOM3-BFG font name&gt; = use a DOOM3-BFG new font by name override all DOOM 3/Quake 4/Prey old fonts. e.g.<br/> * Chainlink_Semi_Bold<br/> * Arial_Narrow<br/> * BankGothic_Md_BT<br/> * microgrammadbolext<br/> * DFPHeiseiGothicW7<br/> * Sarasori_Rg | All |
| s_driver | String | AudioTrack | sound driver | ARCHIVE | AudioTrack, OpenSLES | Engine/Sound | sound if without OpenAL on Android | Android |
| harm_s_OpenSLESBufferCount | Integer | 3 | Audio buffer count for OpenSLES | ARCHIVE | &gt;= 3 | Engine/Sound | min is 3, only for if without OpenAL and use OpenSLES on Android | Android |
| harm_s_useAmplitudeDataOpenAL | Bool | 0 | Use amplitude data on OpenAL | DISABLE, ISSUE |  | Engine/Sound | It cause large shake | All |
| harm_in_smoothJoystick | Bool | 0 | Enable smooth joystick |  |  | Engine/Input | Automatic setup initial value by Android layer | Android |


> #### DOOM III
| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | Head | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_showViewBody | Bool | 0 | Show view body | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_showViewLight | Bool | 0 | show player view flashlight | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightShader | String | lights/flashlight5 | player view flashlight material texture/entityDef name | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightRadius | Vector3 String | 1280 640 640 | player view flashlight radius, format is "&lt;light_target&gt; &lt;light_right&gt; &lt;light_up&gt;" | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightOffset | Vector3 String | 0 0 0 | player view flashlight origin offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_ui_viewLightType | Integer | 0 | player view flashlight type | ARCHIVE | 0=spot light; 1=point light | Game/DOOM3 |  | All |
| harm_ui_viewLightOnWeapon | Bool | 0 | player view flashlight follow weapon position | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_g_autoGenAASFileInMPGame | Bool | 1 | For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic | ARCHIVE |  | Game/DOOM3 |  | All |
| harm_si_autoFillBots | Bool | 0 | Automatic fill bots after map loaded in multiplayer game | ARCHIVE | &gt;=0 | Game/DOOM3 | 0 = disable; other number = bot num | All |
| harm_si_botLevel | Integer | 0 | Bot level | ARCHIVE | [0 - 8] | Game/DOOM3 | 0 = auto; 1 - 8 = difficult level | All |
| harm_si_botWeapons | String | 0 | Bot initial weapons when spawn, separate by comma(,) | ARCHIVE | 1=pistol; 2=shotgun; 3=machinegun; 4=chaingun; 5=handgrenade; 6=plasmagun; 7=rocketlauncher; 8=BFG; 10=chainsaw; 0=none; *=all | Game/DOOM3 | Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_shotgun,weapon_machinegun), and allow mix(e.g. shotgun,3,weapon_rocketlauncher) | All |
| harm_si_botAmmo | Integer | 0 | Bot weapons initial ammo clip when spawn, depend on harm_si_botWeapons | ARCHIVE |  | Game/DOOM3 | -1=max ammo, 0=none, &gt;0=ammo clip | All |
| harm_g_botEnableBuiltinAssets | Bool | 0 | Enable built-in bot assets if external assets missing | INIT |  | Game/DOOM3 |  | All |
| harm_si_useCombatBboxInMPGame | Bool | 0 | Players force use combat bbox in multiplayer game | ARCHIVE |  | Game/DOOM3 |  | All |


> #### DOOM 3: Rivensin
| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_pm_doubleJump | Bool | 0 | Enable double-jump | ARCHIVE |  | Game/DOOM3/Rivensin |  | All |
| harm_pm_autoForceThirdPerson | Bool | 1 | Force set third person view after game level load end | ARCHIVE |  | Game/DOOM3/Rivensin |  | All |
| harm_pm_preferCrouchViewHeight | Float | 32 | Set prefer crouch view height in Third-Person | ARCHIVE | &gt;= 0 | Game/DOOM3/Rivensin | suggest 32 - 39, less or equals 0 to disable | All |


> #### Quake 4
| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | head_channel | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/Quake4 |  | All |
| harm_gui_defaultFont | String | chain | Default font name | ARCHIVE | chain, lowpixel, marine, profont, r_strogg, strogg | Engine/Quake4/GUI | It will be available in next running | All |
| harm_ui_showViewBody | Bool | 0 | Show view body | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_autoGenAASFileInMPGame | Bool | 1 | For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic | ARCHIVE |  | Game/Quake4 |  | All |
| harm_si_autoFillBots | Bool | 0 | Automatic fill bots after map loaded in multiplayer game | ARCHIVE | &gt;=0 | Game/Quake4 | 0 = disable; other number = bot num | All |
| harm_si_botLevel | Integer | 0 | Bot level | ARCHIVE | [0 - 8] | Game/Quake4 | 0 = auto; 1 - 8 = difficult level | All |
| harm_si_botWeapons | String | 0 | Bot initial weapons when spawn, separate by comma(,) | ARCHIVE | 1=machinegun; 2=shotgun; 3=hyperblaster; 4=grenadelauncher; 5=nailgun; 6=rocketlauncher; 7=railgun; 8=lightninggun; 9=dmg; 10=napalmgun; 0=none; *=all | Game/Quake4 | Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_machinegun,weapon_shotgun), and allow mix(e.g. machinegun,3,weapon_rocketlauncher) | All |
| harm_si_botAmmo | Integer | 0 | Bot weapons initial ammo clip when spawn, depend on harm_si_botWeapons | ARCHIVE |  | Game/Quake4 | -1=max ammo, 0=none, &gt;0=ammo clip | All |
| harm_g_botEnableBuiltinAssets | Bool | 0 | Enable built-in bot assets if external assets missing | INIT |  | Game/Quake4 |  | All |
| harm_g_mutePlayerFootStep | Bool | 0 | mute player's footstep sound | ARCHIVE |  | Game/Quake4 |  | All |
| harm_g_allowFireWhenFocusNPC | Bool | 0 | allow fire when focus NPC | ARCHIVE |  | Game/Quake4 |  | All |


> #### Prey(2006)
| CVar | Type | Default | Description | Flag | Range | Scope | Remark | Platform |
|:---|:---:|:--:|:---|:---:|:---:|:---|:---|:---:|
| harm_pm_fullBodyAwareness | Bool | 0 | Enables full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessOffset | Vector3 String | 0 0 0 | Full-body awareness offset, format is "&lt;forward-offset&gt; &lt;side-offset&gt; &lt;up-offset&gt;" | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessHeadJoint | String | neck | Set head joint when without head model in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessFixed | Bool | 0 | Do not attach view position to head in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_pm_fullBodyAwarenessHeadVisible | Bool | 0 | Do not suppress head in full-body awareness | ARCHIVE |  | Game/Prey |  | All |
| harm_ui_translateAlienFont | String | fonts | Setup font name for automatic translate `alien` font text of GUI | ARCHIVE | fonts, fonts/menu, "" | Engine/Prey/GUI | empty to disable | All |
| harm_ui_translateAlienFontDistance | Float | 200 | Setup max distance of GUI to view origin for enable translate `alien` font text | ARCHIVE |  | Engine/Prey/GUI | 0 = disable;<br/> -1 = always;<br/> positive: distance value | All |
| harm_ui_subtitlesTextScale | Float | 0.32 | Subtitles's text scale | ARCHIVE |  | Engine/Prey/GUI | &lt;= 0 to unset | All |
| harm_r_skipHHBeam | Bool | 0 | Skip beam model render | DEBUG |  | Engine/Prey/Renderer |  | All |
	
----------------------------------------------------------------------------------
