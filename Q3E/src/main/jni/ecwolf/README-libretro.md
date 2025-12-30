## Introduction

This contains a libretro port of ecwolf.

## Compilation

```shell
cmake -DLIBRETRO=1
```

For Android compilation

```shell
cmake -DLIBRETRO=1 -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake -DANDROID_ABI=$ABI
```

## Differences with standalone port

* Menus are removed and it goes straight to episode or skill selection
* All options are moved to libretro options
* Full screen control is also moved to normal libretro
* Load and save is moved to libretro facilities as well
* Quit is removed from the core and is to be handled in libretro way as well
* Invunerability cheat is moved to libretro options
* Gamma control is not supported. Use libretro filters.

## Missing features

* Only IMF music is supported
* "Read This!" is missing
* "High scores" are also missing
* MLI cheat is missing
* Demo recording and playback are missing. Note: normal libretro demos are still there.
* F1 help is missing
* ID easter egg is missing
* mouse and keyboard support, including corresponding config, is missing.
  Only retropad currently works. You can control using retropad mappings
* '0'-'9' weapon selection is missing
* Buttons 'zoom', 'reload' and 'altattack' are missing. They're used only in mods.
* Buttons 'Strafe modifier' and 'status bar' are missing
* Netplay is missing
* Difficulty confirmation screen is missing
* Resizing screen with +/- is not supported. Use options
* Commander Keen easter egg is missing
* Menu movement animation is missing
* Robert's jukebox is missing

## Known glitches

* Rewinding through intermission or death fizzling results in glitched screen
* Sometimes after loading a large asset, FPS drops. If that happens go to retroarch menu for couple of seconds and then back

## Relation to ECWolf

This repository is forked from ecwolf on 23rd February 2020 with the intention
of quicker handling libretro-related issued and collaboration. This is done
with full respect of original authors and we would be happy to collaborate with
them.
