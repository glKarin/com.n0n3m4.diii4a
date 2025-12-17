Botrix
======
Botrix (from BOT's matRIX) is a plugin for Valve Source Engine games that allows
to play with bots. Currently supported games:
* Half-Life 2: Deathmatch
* Team Fortress 2


Fast installation for HL2DM
===========================
Copy folders "addons" to your mod dir, i.e.
    "<SteamDir>/SteamApps/common/Half-Life 2 Deathmatch/hl2mp/"

Launch Half-Life 2 Deathmatch with -insecure parameter (this is not required for
dedicated servers). Create new game, map dm_underpass. Type "botrix bot add"
several times at the console to create bots.

Have fun!


Custom installation for HL2DM
=============================
As you may know Steam differs notions of GAME FOLDER (where hl2.exe is) and MOD
FOLDER (where gameinfo.txt is).

Plugin searches for folder named "botrix" in next locations:
 - MOD FOLDER/addons (default location, visible only for one mod)
 - MOD FOLDER
 - GAME FOLDER
 - BASE FOLDER (best location, visible for several mods)

Inside folder "botrix" plugin stores configuration file and waypoints. It is
better to have "botrix" folder inside BASE FOLDER to share it between games.
 
Example of SINGLE-PLAYER GAME:
  Base folder (where all games are):
    C:/Steam/SteamApps/common
  Game folder:
    C:/Steam/SteamApps/common/Half-Life 2 Deathmatch/
  Mod folder:
    C:/Steam/SteamApps/common/Half-Life 2 Deathmatch/hl2mp/ <- (addons and cfg here)
  Botrix folder:
    C:/Steam/SteamApps/common/ <- (botrix here)

Example of DEDICATED SERVER:
  Base folder (where all dedicated servers are):
    C:/DedicatedServer/
  Game folder:
    C:/DedicatedServer/HL2DM/
  Mod folder:
    C:/DedicatedServer/HL2DM/hl2mp/ <- (addons and cfg here)
  Botrix folder:
    C:/DedicatedServer/ <- (botrix here)

This plugin is UNSIGNED, so you need to execute hl2.exe with -insecure parameter
in single-player game. You can also set it by Steam, just right-click on the
game and select it's properties / set launch parameters or whatever.
Note that dedicated server doesn't need that.

For now waypoints were created for 2 maps: dm_underpass and dm_steamlab.
If you want to create waypoints use botrix commands to create and save them.
After doing that, send me your files, and I will make sure to test and publish
them on my site.

Any ideas are also appreciated, and if time will allow me, I could implement
them in future versions.

Please, feel free to send feedback to: botrix.plugin@gmail.com


Configuration for HL2DM
=======================
Inside botrix folder there is a file: config.ini. Make sure that both GAME
FOLDER and MOD FOLDER appear in section [HalfLife2Deathmatch.mod], under "games"
and "mods" keys.

For given example of SINGLE-PLAYER GAME you should have:
    games = Half-Life 2 Deathmatch
    mods = hl2mp

For given example of DEDICATED SERVER you should have:
    games = hl2dm
    mods = hl2mp


Admins
======
User of single player game has full admin rights.
For DEDICATED SERVER there is a section in config.ini: [User access]. You
should read comments for that section to know how to add admins for Botrix.


Other mods
==========
Botrix should work with other mods, say Counter-Strike: Source. Check out
*.items.* and *.weapons sections in config.ini to figure out how items and 
weapons are handled.

If you could make bots work for other mods, please send me your config file and
I will publish it on my site.


Limitations = future work
=========================
- Bots can't chat.
- Handling of medic/ingineer 'weapons'.
- Support for other mods.

