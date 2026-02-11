## idTech4's new command

----------------------------------------------------------------------------------

> ##### Flag
* ARCHIVE = save to/load from config file
* FIXED = can't change in game
* READONLY = readonly, always can't change
* INIT = only setup on boot command
* ISSUE = maybe has bugs
* DEBUG = only for developer debug and test
* DISABLE = only for disable something and keep original source code

----------------------------------------------------------------------------------

> #### Engine
| Command | Description | Usage | Scope | Remark | Platform |
|:---|:---|:---|:---|:---|:---:|
| exportGLSLShaderSource | export GLSL shader source to filesystem |  | Engine/Renderer | Only export shaders of using OpenGLES2.0 or OpenGLES3.0 | All |
| printGLSLShaderSource | print internal GLSL shader source |  | Engine/Renderer | Only print shaders of using OpenGLES2.0 or OpenGLES3.0 | All |
| exportDevShaderSource | export internal original C-String GLSL shader source for developer |  | Engine/Renderer | Export all shaders of OpenGLES2.0 and OpenGLES3.0 | All |
| convertARB | convert ARB shader to GLSL shader |  | Engine/Renderer | It has many errors, only port some ARB shader to GLSL shader | All |
| reloadGLSLprograms | reloads GLSL programs |  | Engine/Renderer |  | All |
| cleanExternalGLSLShaderSource | Remove external GLSL shaders directory |  | Engine/Renderer |  | All |
| cleanGLSLShaderBinary | Remove GLSL shader binaries directory |  | Engine/Renderer |  | All |
| pskToMd5mesh | Convert psk to md5mesh |  | Engine/Renderer |  | All |
| psaToMd5anim | Convert psa to md5anim |  | Engine/Renderer |  | All |
| pskPsaToMd5 | Convert psk/psa to md5mesh/md5anim |  | Engine/Renderer |  | All |
| iqmToMd5mesh | Convert iqm to md5mesh |  | Engine/Renderer |  | All |
| iqmToMd5anim | Convert iqm to md5anim |  | Engine/Renderer |  | All |
| iqmToMd5 | Convert iqm to md5mesh/md5anim |  | Engine/Renderer |  | All |
| smdToMd5mesh | Convert smd to md5mesh |  | Engine/Renderer |  | All |
| smdToMd5anim | Convert smd to md5anim |  | Engine/Renderer |  | All |
| smdToMd5 | Convert smd to md5mesh/md5anim |  | Engine/Renderer |  | All |
| fbxToMd5mesh | Convert fbx to md5mesh |  | Engine/Renderer |  | All |
| fbxToMd5anim | Convert fbx to md5anim |  | Engine/Renderer |  | All |
| fbxToMd5 | Convert fbx to md5mesh/md5anim |  | Engine/Renderer |  | All |
| md5meshV6ToV10 | Convert md5mesh v6(2002 E3 demo version) to v10(2004 release version) |  | Engine/Renderer |  | All |
| md5animV6ToV10 | Convert md5anim v6(2002 E3 demo version) to v10(2004 release version) |  | Engine/Renderer |  | All |
| md5V6ToV10 | Convert md5mesh/md5anim v6(2002 E3 demo version) to v10(2004 release version) |  | Engine/Renderer |  | All |
| convertMd5Def | Convert other type animation model entityDef to md5mesh/md5anim |  | Engine/Renderer |  | All |
| cleanConvertedMd5 | Clean converted md5mesh/md5anim |  | Engine/Renderer |  | All |
| convertMd5AllDefs | Convert all other type animation models entityDef to md5mesh/md5anim |  | Engine/Renderer |  | All |
| convertImage | convert image format |  | Engine/Renderer |  | All |
| glConfig | print OpenGL config |  | Engine/Renderer | print glConfig variable | All |


> #### DOOM III
| Command | Description | Usage | Scope | Remark | Platform |
|:---|:---|:---|:---|:---|:---:|
| exportFont | Convert ttf/ttc font file to DOOM3 wide character font file |  | Engine/Renderer | require freetype2 | All |
| extractBimage | extract DOOM3-BFG's bimage image |  | Engine/Renderer | extract to TGA RGBA image files | All |
| idTech4AmmSettings | Show idTech4A++ new cvars and commands |  | Engine/Framework |  | All |
| botRunAAS | compiles an AAS file for a map for DOOM 3 multiplayer-game |  | Game/DOOM3 | Only for generate bot aas file if map has not aas file | All |
| addBot | adds a new bot |  | Game/DOOM3 | need SABotA7 files | All |
| removeBot | removes bot specified by id (0,15) |  | Game/DOOM3 | need SABotA7 files | All |
| addbots | adds multiplayer bots batch |  | Game/DOOM3 | need SABotA7 files | All |
| fillbots | fill bots to maximum of server |  | Game/DOOM3 | need SABotA7 files | All |
| removeBots | disconnect multi bots by client ID |  | Game/DOOM3 | need SABotA7 files | All |
| appendBots | append more bots(over maximum of server) |  | Game/DOOM3 | need SABotA7 files | All |
| cleanBots | disconnect all bots |  | Game/DOOM3 | need SABotA7 files | All |
| truncBots | disconnect last bots |  | Game/DOOM3 | need SABotA7 files | All |
| botLevel | setup all bot level |  | Game/DOOM3 | need SABotA7 files | All |
| botWeapons | setup all bot initial weapons |  | Game/DOOM3 | need SABotA7 files | All |
| botAmmo | setup all bot initial weapons ammo clip |  | Game/DOOM3 | need SABotA7 files | All |


> #### Quake 4
| Command | Description | Usage | Scope | Remark | Platform |
|:---|:---|:---|:---|:---|:---:|
| botRunAAS | compiles an AAS file for a map for Quake 4 multiplayer-game |  | Game/Quake4 | Only for generate bot aas file if map has not aas file | All |
| addBot | adds a new bot |  | Game/Quake4 | need SABotA9 files | All |
| removeBot | removes bot specified by id (0,15) |  | Game/Quake4 | need SABotA9 files | All |
| addbots | adds multiplayer bots batch |  | Game/Quake4 | need SABotA9 files | All |
| fillbots | fill bots to maximum of server |  | Game/Quake4 | need SABotA9 files | All |
| removeBots | disconnect multi bots by client ID |  | Game/Quake4 | need SABotA9 files | All |
| appendBots | append more bots(over maximum of server) |  | Game/Quake4 | need SABotA9 files | All |
| cleanBots | disconnect all bots |  | Game/Quake4 | need SABotA9 files | All |
| truncBots | disconnect last bots |  | Game/Quake4 | need SABotA9 files | All |
| botLevel | setup all bot level |  | Game/Quake4 | need SABotA9 files | All |
| botWeapons | setup all bot initial weapons |  | Game/Quake4 | need SABotA9 files | All |
| botAmmo | setup all bot initial weapons ammo clip |  | Game/Quake4 | need SABotA9 files | All |
	
----------------------------------------------------------------------------------
