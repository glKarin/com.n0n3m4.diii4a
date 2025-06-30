# CS16Client [![Build Status](https://github.com/Velaron/cs16-client/actions/workflows/build.yml/badge.svg)](https://github.com/Velaron/cs16-client/actions) <img align="right" width="128" height="128" src="https://github.com/Velaron/cs16-client/raw/main/android/app/src/main/ic_launcher-playstore.png" alt="CS16Client" />
Counter-Strike reverse-engineered.

## Donate
[![Buy Me A Coffee](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/velaron)

[Support](https://www.buymeacoffee.com/velaron) me on Buy Me A Coffee, if you like my work and would like to support further development goals, like  reverse-engineering other great mods.

## Download
You can download a build at the [Releases](https://github.com/Velaron/cs16-client/releases/tag/continuous) section, or use these links:
* [Android](https://github.com/Velaron/cs16-client/releases/download/continuous/cs16-client.apk)
* [Linux](https://github.com/Velaron/cs16-client/releases/download/continuous/cs16-client_linux_i386.tar.gz) (not recommended)
* [Windows](https://github.com/Velaron/cs16-client/releases/download/continuous/cs16-client_win32_x86.zip) (not recommended)

## Installation
To run CS16Client you need the latest developer build of Xash3D FWGS, which you can get [here](https://github.com/FWGS/xash3d-fwgs/releases/tag/continuous) (unless on [Android](https://github.com/Velaron/xash3d-fwgs/releases/tag/continuous-android)).
You have to own the [game on Steam](https://store.steampowered.com/app/10/CounterStrike//) and copy `valve` and `cstrike` folders into your Xash3D FWGS directory.
After that, just install the APK and run.

## Configuration (CVars)
| CVar                     | Default       | Min | Max | Description                                                                                 |
|--------------------------|---------------|-----|-----|---------------------------------------------------------------------------------------------|
| hud_color                | "255 160 0"   | -   | -   | HUD color in RGB.                                                                           |
| cl_quakeguns             | 0             | 0   | 1   | Draw centered weapons.                                                                      |
| cl_weaponlag             | 0             | 0.0 | -   | Enable weapon lag/sway.                                                                     |
| xhair_additive           | 0             | 0   | 1   | Makes the crosshair additive.                                                               |
| xhair_color              | "0 255 0 255" | -   | -   | Crosshair's color (RGBA).                                                                   |
| xhair_dot                | 0             | 0   | 1   | Enables crosshair dot.                                                                      |
| xhair_dynamic_move       | 1             | 0   | 1   | Jumping, crouching and moving will affect the dynamic crosshair (like cl_dynamiccrosshair). |
| xhair_dynamic_scale      | 0             | 0   | -   | Scale of the dynamic crosshair movement.                                                    |
| xhair_gap_useweaponvalue | 0             | 0   | 1   | Makes the crosshair gap scale depend on the active weapon.                                  |
| xhair_enable             | 0             | 0   | 1   | Enables enhanced crosshair.                                                                 |
| xhair_gap                | 0             | 0   | -   | Space between crosshair's lines.                                                            |
| xhair_pad                | 0             | 0   | -   | Border around crosshair.                                                                    |
| xhair_size               | 4             | 0   | -   | Crosshair size.                                                                             |
| xhair_t                  | 0             | 0   | 1   | Enables T-shaped crosshair.                                                                 |
| xhair_thick              | 0             | 0   | -   | Crosshair thickness.                                                                        |

## Building
Clone the source code:
```
git clone https://github.com/Velaron/cs16-client --recursive
```
### Windows
```
cmake -A Win32 -S . -B build
cmake --build build --config Release
```
### Linux
```
cmake -S . -B build
cmake --build build --config Release
```
### Android
```
./gradlew assembleRelease
```
