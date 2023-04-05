## idTech4A++ (Harmattan Edition)
#### DIII4A++, com.n0n3m4.diii4a, DOOM III/Quake 4/Prey(2006) for Android, 毁灭战士3/雷神之锤4/掠食(2006)安卓移植版
**Latest version:**
1.1.0harmattan27(natasha)  
**Last update release:**
2023-04-05  
**Arch:**
arm64 armv7-a  
**Platform:**
Android 4.0+  
**License:**
GPLv3

----------------------------------------------------------------------------------
### Update

> 1.1.0harmattan27 (2023-04-05)

* Fixup some line effects in Quake 4. e.g. monster body disappear effect, lines net.
* Fixup radio icon in player HUD right-top corner in Quake 4.
* Fixup dialog interactive in Quake 4. e.g. dialog of creating MP game server.
* Fixup MP game loading GUI and MP game main menu in Quake 4.
* Add integer cvar named `harm_si_autoFillBots` for automatic fill bots after map loaded in MP game in Quake 4(0 to disable). `fillbots` command support append bot num argument.
* Add automatic set bot's team in MP team game, random bot model, and fixup some bot's bugs in Quake 4.
* Add `SABot`'s aas file pak of MP game maps in `Quake 4 helper dialog`.

----------------------------------------------------------------------------------

#### About Prey(2006)
###### For playing Prey(2006)([jmarshall](https://github.com/jmarshall23) 's [PreyDoom](https://github.com/jmarshall23/PreyDoom)). Now can play all levels, but some levels has bugs.
> 1. Putting PC Prey game data file to `preybase` folder and START directly.
> 2. Some problems solution: e.g. using cvar `harm_g_translateAlienFont` to translate Alien text on GUI.
> 3. Exists bugs: e.g. some incorrect collision(using `noclip`), some menu draw(Tab window), some GUIs not work(Music CD in RoadHouse).
> 4. Because of tab window UI not support, Settings UI is not work, must edit `preyconfig.cfg` for binding extras key.
> > * bind "Your key of spirit walk" "_impulse54"
> > * bind "Your key of second mode attack of weapons" "_attackAlt"
> > * bind "Your key of toggle lighter" "_impulse16"
> > * bind "Your key of drop" "_impulse25"

----------------------------------------------------------------------------------

#### About Quake IV
###### For playing Quake 4([jmarshall](https://github.com/jmarshall23) 's [Quake4Doom](https://github.com/jmarshall23/Quake4Doom)). Now can play all levels, but some levels has bugs.  
> 1. Putting PC Quake 4 game data file to `q4base` folder.
> 2. Click `START` to open Quake 4 map level dialog in game launcher.
> 3. Suggest to extract Quake 4 patch resource to `q4base` game data folder first.
> - Quake 3 bot files(If you want to add bots in Multiplayer-Game, using command `addbot <bot_file> <bot_file> ...` or `fillbots` after enter map in console, or set `harm_si_autoFillBots` to 1 for automatic fill bots).
> - `SABot` MP game map aas files(for bots in MP game).
> 4. Then Choose map level/Start directly, all levels is working, and `New Game` s working.
> 5. Gun-lighting default is opened(can using bool cvar `harm_g_flashlightOn` to control).

###### Problems and resolutions  
> 1. ~~Door-opening/Collision~~: Now collision bug has fixed, e.g. trigger, vehicle, AI, elevator, health-station, all doors can be opened.
> 2. *Main-menu*: Now main menu and MP game menu is work, but without background color. But some GUIs can not interactive.
> 3. ~~Sound~~: It looks work well now(jmarshall's `icedTech` using DOOM3-BFG sound system).
> 4. ~~Loading-UI~~: It looks work well now.
> 5. ~~Multiplayer-Game~~: Now is working well with bots(`jmarshall` added Q3-bot engine, but need bots decl file and Multiplayer-Game map AAS file, now set cvar `harm_g_autoGenAASFileInMPGame` to 1 for generating a bad AAS file when loading map in Multiplayer-Game and not valid AAS file in current map, you can also put your MP map's AAS file to `maps/mp` folder).
> 6. *Script error*: Some maps have any script errors, it can not cause game crash, but maybe have impact on the game process.
> 7. *Particle system*: Now is not work(Quake4 using new advanced `BSE` particle system, it not open-source, `jmarshall` has realized and added by decompiling `ETQW`'s BSE binary file, also see [jmarshall23/Quake4BSE](https://github.com/jmarshall23/Quake4BSE)), but it not work yet. Now implementing a OpenBSE with DOOM3 original FX/Particle system, some effects can played, but has incorrect render.
> 8. *Entity render*: Some game entities render incorrect.
> 9. ~~Font~~: Support Q4 format fonts now. [IlDucci](https://github.com/IlDucci)'s DOOM3-format fonts of Quake 4 is not need on longer.

----------------------------------------------------------------------------------
### Screenshot
> Game

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_bathroom.png" alt="Classic bathroom">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_bathroom_jill_stars.png" alt="Classic bathroom in Rivensin mod">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4_game_2.png" alt="Quake IV on DOOM3">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey_girlfriend.png" alt="Prey(2006) on DOOM3">

> Mod

<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_roe.png" width="50%" alt="Resurrection of Evil"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_the_lost_mission.png" width="50%" alt="The lost mission">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_classic_doom3.png" width="50%" alt="Classic DOOM"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_hardcorps.png" width="50%" alt="Hardcorps">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_doom3_rivensin.png" width="50%" alt="Rivensin"><img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_quake4.png" width="50%" alt="Quake IV">
<img src="https://github.com/glKarin/com.n0n3m4.diii4a/raw/package/screenshot/Screenshot_prey.png" width="50%" alt="Prey(2006)">

----------------------------------------------------------------------------------

### Changes:

[Change logs](CHANGES.md ':include')

----------------------------------------------------------------------------------

### About:

* Source in `assets/source` folder in APK file.
	
----------------------------------------------------------------------------------

### Branch:

> `master`:
> * /DIII4A: frontend source
> * /doom3: game source

> `package`:
> * /*.apk: all version build
> * /screenshot: screenshot pictures
> * /source: Reference source
> * /pak: Game resource
> * /CHECK_FOR_UPDATE.json: Check for update config JSON

> `n0n3m4_original_old_version`:
> * Original old `n0n3m4` version source.

----------------------------------------------------------------------------------
### Extras download:

* [Google: https://drive.google.com/drive/folders/1qgFWFGICKjcQ5KfhiNBHn_JYhJN5XoLb](https://drive.google.com/drive/folders/1qgFWFGICKjcQ5KfhiNBHn_JYhJN5XoLb)
* [Baidu网盘: https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w](https://pan.baidu.com/s/1hXvKmrajAACfcCj9_ThZ_w) 提取码: `pyyj`
* [Baidu贴吧: BEYONDK2000](https://tieba.baidu.com/p/6825594793)
----------------------------------------------------------------------------------
