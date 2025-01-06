# SourceTech Engine

<a href="https://discord.com/invite/TZubjHHKty"><img src="https://img.shields.io/discord/1145198169441960067?color=7289da&logo=discord&logoColor=white" alt="QS Discord" /></a>

This is a modern engine created for Quake Sandbox.

## Features:

**From Quake3e**:

* Optimized OpenGL renderer
* Optimized Vulkan renderer
* Raw mouse input support, enabled automatically instead of DirectInput(**\in_mouse 1**) if available
* Unlagged mouse events processing, can be reverted by setting **\in_lagged 1**
* **\video-pipe** - to use external ffmpeg binary as an encoder for better quality and smaller output files
* Significally reworked QVM (Quake Virtual Machine)
* Improved server-side DoS protection, much reduced memory usage
* Raised filesystem limits (up to 20,000 maps can be handled in a single directory)
* Reworked Zone memory allocator, no more out-of-memory errors
* Non-intrusive support for SDL2 backend (video, audio, input), selectable at compile time
* Tons of bug fixes and other improvements

**SourceTech features**:

* Brush limit up to 524288
* Entity limit up to 4096
* Model limit up to 4096
* Sound limit up to 4096
* Brush model limit up to 4096
* Cvar limit up to 16384
* Players and bots limit up to 256
* New weapon system with limit up to 8192
* Up to 80000000 polygons per scene
* Up to 1000000 polygons per model
* New addon system
* Simple physics (QVM-side)
* Map in UI background
* Seamless change of QVM
* Improved lighting (high resolution lightmaps and post-processing)
* Render distance for entity (server-side)
* MGui interface system with QVM and map integration
* Vehicles and additional properties of the entity
* Support huge size maps up to 4GB

## Vulkan renderer

Based on [Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition)/[quake3e](https://github.com/ec-/Quake3e) with many additions:

* 4K textures support
* High-quality per-pixel dynamic lighting
* Merged lightmaps (atlases)
* Smooth shader animations with 64 frames
* Rendering a huge number of entities

Highly recommended to use on modern systems

## OpenGL renderer

Based on classic OpenGL renderers from [idq3](https://github.com/id-Software/Quake-III-Arena)/[ioquake3](https://github.com/ioquake/ioq3)/[cnq3](https://bitbucket.org/CPMADevs/cnq3)/[openarena](https://github.com/OpenArena/engine)/[quake3e](https://github.com/ec-/Quake3e), features:

* 4K textures support
* High-quality per-pixel dynamic lighting
* Merged lightmaps (atlases)
* Smooth shader animations with 64 frames
* Rendering a huge number of entities

Performance is usually greater or equal to other opengl1 renderers

## [Build Instructions](BUILD.md)

## Links

* https://www.moddb.com/games/qs - Quake Sandbox ModDB page
* https://discord.gg/TZubjHHKty - Quake Sandbox Discord
