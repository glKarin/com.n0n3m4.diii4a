# ioquake 3 for UrbanTerror

[![Build Status](https://travis-ci.org/mickael9/ioq3.svg?branch=urt)](https://travis-ci.org/mickael9/ioq3)
[Download prebuilt binaries for Linux/macOS/Windows](http://builds.iourtm9.tk/urt/)

This project is an initiative to backport the relevant ioUrbanTerror-specific
features to an up-to-date ioquake3.

This brings all of the ioquake3 features to UrbanTerror:
 - [VOIP support](voip-readme.txt) from ioquake3
 - Mumble support
 - New [OpenGL2 renderer](opengl2-readme.md)

And a lot of other things, see the [ioquake3 README](README.ioq3.md)

# New features

Those are the features that are specific to this build.

## No more map conflicts!
This build includes a few features to address the map conflict issues that can
be encountered when playing Urban Terror.

If you have a lot of third-party maps in your download folder and tried to play
in single player, you might have noticed some issues, like say the SR8 being
different in aspect, or different sounds, invalid textures, etc.

This is caused by the way quake3 search path works: when a file is requested,
it goes through the quake3 search path until the file is found, so a file that
exists higher in the search path will always "shadow" lower priority files.
Since there is no concept of "map file", you can have situations where a
resource exists in the map pk3 but gets picked from another pk3. Some maps
also like to override default resources (like weapon models, flags, sounds,
etc.)

### `fs_reorderPaks` cvar

When set to 1 (the default), this will reorder the pk3s in the search path so
that the pk3 containing the loaded map comes first, then the high priority paks
(starting with z) and finally everything else.

This way:

 - The map always has priority over everything else, respecting the mapper's
   intention.

 - Other maps can't override core resources from the game paks

 - Additional resources (like funstuff) can still be loaded from the other
   pk3s

This cvar works on clients and servers :

 - Pure servers with this cvar will affect the clients since they respect the
   search path order from the server.  This is not the case on unpure servers
   so this option will have no practical effect on unpure servers.

 - Clients with this cvar on will always benefit, whether they connect to pure
   or unpure servers.  When connecting to pure servers, the reordering is done
   after the server search order is copied

### `sv_extraPure` and `sv_extraPaks`
When set to 1, this will make the server "extra pure". The loaded paks list
sent to clients will be reduced to include only the core game paks as well as
the referenced paks (ie the map pk3). The main use for this is if you
have so many loaded paks that the server can't work in pure mode (because the
list of loaded paks is too big to fit in the server info string).

This also means that no paks other than the map pak will be loaded by clients,
disabling usage of, for instance, funstuff. This can be solved by setting the
`sv_extraPaks` cvar to a space-separated list of pk3 names that you don't want
to exclude from the search path

**Warning:** Setting this has an undesirable side-effect for clients.
The voting UI for map changes will not display custom maps (unless they're
whitelisted in `sv_extraPaks`).

### `fs_lowPriorityDownloads`
When set to 1 (the default), this puts the maps in the `download` folder at a
lower priority than anything else.

## `fs_defaultHomePath` cvar

This new cvar is similar to the ioUrbanTerror `use_defaultHomePath` cvar and
controls the default value for the `fs_homepath` cvar (which specifies where
the writable data such as map downloads and configs will be written).

Possible values are:
- 0 (default): use the value of `DEFAULT_HOMEDIR` specified at compile-time if
  defined, else use the system home path.

- 1: use the system home path (`AppData`, `~/.q3a`, `~/Library`)

- 2: use the executable directory

- 3: use the installation path, that is the value of `DEFAULT_BASEDIR`
  specified at compile-time if defined, else use current working directory.

## Improved `sv_cheats`

The `sv_cheatMode` server cvar was added to allow enabling cheats without
having to use the `devmap` command.

When set to `1`, cheats will be enabled only for the next map (can be set in `mapcycle.txt`).

When set to `2`, cheats will be enabled for every map.

Example `mapcycle.txt`:
```
ut4_abbey
{
    sv_cheatMode 1 // cheats for abbey only (automatically resets to 0)
}
ut4_turnpike
{
    // no cheats
}
```

## cmdline.txt

Some cvars can exclusively be set from the command line arguments which makes
customization harder because it requires creating wrapper scripts.

To make things easier, command line arguments will also be loaded from the
`cmdline.txt` file from the following locations (in that order):
 - the same directory as the executable
 - the installation path (if set at compile time)
 - the current working directory

This can be used, to, for instance, create a portable version that stores
all of its data in the same directory as the executable:

```
set fs_defaultHomePath 2 // Use the executable directory as the home path
```

Another use for this is to set whether your server will be visible to the
internet or only to your LAN using the `dedicated` cvar and to execute the
`server.cfg` file:

```
set dedicated 2 // Make the server visible to the internet
exec server.cfg // Start server with the specified config file
```


## Impoved keyboard handling

- Number keys in the first row are always mapped to number keys, on AZERTY
  layouts for instance. This matches with the behavior of ioUrbanTerror on
  Windows and brings it to other platforms as well.

- Numpad `2` and `8` keys aren't interpreted as up/down in console anymore.

- The leftmost key in the upper row (left of the `1` key) is now bound the
  console regardless of the current user layout.
  This can be disabled by setting `cl_consoleUseScanCode` to `0`.

- Non-ascii keys can now be mapped on all platforms
  (using the `WORLD_n` key names)

- Ctrl-V and Shift-Insert can be used to paste text on all platforms.

## Other changes

- Download UI is improved a bit

- Downloading can still be attempted if the server has no download URL set.
  In this case we use the default one (urbanterror.info).

- `+vstr` supports nested key presses.

- `/map` will now accept to load maps that are not in a pure pak
  (when connected to a pure server)

- Added `sv_autoRecordDemo`cvar to automatically create a server demo of every
  player that connects.

## Security fixes

- QVMs, `q3config.cfg` and `autoexec.cfg` can't be loaded from downloaded pk3s
  anymore.

- Only `.dll` / `.so` / `.dynlib` exensions are accepted for dynamic libraries.

- `/condump` and `/writeconfig` can only write to .txt files.

- CURL protocols (for map downloads) are limited to HTTP(S) and FTP(S).

- Fixed `stats` command exploit

- Fixed that `save` could create new directories on jump servers with persistent positions enabled

- Fix a buffer overflow when a funstuff is bigger than 13 characters

# Feature parity status with original ioUrbanTerror

## Common
- [x] UrbanTerror 4.2+ demo format (.urtdemo)
- [x] Auth system
- [x] `+vstr` command
- [x] Compressed pak list

## Client
- [x] Make `/reconnect` work across restarts
- [x] Use `download` folder for downloads
- [x] Disallow QVMs in download folder
- [x] Fancy tabbed console
- [x] dmaHD
- [x] Prompt before auto download
- [x] New mouse acceleration style (`cl_mouseAccel 2`)
- [x] Make the client query other master servers if main one does not respond
- [x] `r_altgamma` cvar for Linux users having problems with gamma settings
- [x] Escape `%` in client to server commands (allows usage of '%' in chat)
- [ ] ~~Alt-tab and `r_minimize` cvars~~ (not needed with SDL2)

## Server
- [x] Server demos
- [x] `sv_clientsPerIP` cvar
- [x] `sv_sayprefix` / `sv_tellprefix` cvars
- [x] Send UrT specific server infostring
- [x] Partial matching of map and players

This list is likely incomplete. Please let me know if I forgot anything!
