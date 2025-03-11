# Vulkan Renderer Library for Yamagi Quake II

This is the vkQuake2 vulkan renderer library ported to Yamagi Quake II.

## Compilation

You'll need:

* clang or gcc,
* GNU Make,
* SDL2 with `sdl2-config`,
* vulkan-headers,
* vulkan-validationlayers if you like to debug issues.

For vulkan-headers on macOS with Homebrew: `brew install molten-vk`.

Type `make` to compile the library. If the compilation is successfull,
the library can be found under *release/ref_vk.dll* (Windows) or
*release/ref_vk.so* (unixoid systems).

## Usage

Copy the library next to your Yamagi Quake II executable. You can select
Vulkan through the video menu or by cvar with `vid_renderer vk` followed
by a `vid_restart`.

If you have run into issues, please attach output logs with OS/driver version
and device name to the bug report. [List](https://openbenchmarking.org/test/pts/yquake2)
of currently tested devices for the reference.

Note: Game saves outputs to `Documents\YamagiQ2\stdout.txt` under Windows.

On macOS it is necessary to set `DYLD_LIBRARY_PATH` to load the Vulkan Portability library:

`export DYLD_LIBRARY_PATH=/opt/homebrew/opt/molten-vk/lib`

## Console Variables

* **r_validation**: Toggle validation layers:
  * `0` - disabled (default in Release)
  * `1` - only errors and warnings, show image load issues
  * `2` - best-practices validation

* **vk_strings**: Print some basic Vulkan/GPU information.

* **vk_mem**: Print dynamic vertex/index/uniform/triangle fan buffer
  memory usage statistics.

* **vk_device**: Specify index of the preferred Vulkan device on systems
  with multiple GPUs:
  * `-1` - prefer first DISCRETE\_GPU (default)
  * `0..n` - use device #n (full list of devices is returned by
    `vk_strings` command)

* **vk_sampleshading**: Toggle sample shading for MSAA. (default: `1`)

* **vk_flashblend**: Toggle the blending of lights onto the environment.
  (default: `0`)

* **r_polyblend**: Blend fullscreen effects: blood, powerups etc.
  (default: `1`)

* **vk_skymip**: Toggle the usage of mipmap information for the sky
  graphics. (default: `0`)

* **vk_finish**: Inserts a `vkDeviceWaitIdle()` call on frame render
  start (default: `0`). Don't use this, it's there just for the sake of
  having a `gl_finish` equivalent!

* **vk_custom_particles**: Toggle particles type:
  * `0` - textured triangles for particle rendering
  * `1` - between using POINT\_LIST (default)
  * `2` - textured square for particle rendering

* **vk_particle_size**: Rendered particle size. (default: `40`)

* **vk_particle_att_a**: Intensity of the particle A attribute.
  (default: `0.01`)

* **vk_particle_att_b**: Intensity of the particle B attribute.
  (default: `0`)

* **vk_particle_att_c**: Intensity of the particle C attribute.
 (default: `0.01`)

* **vk_particle_min_size**: The minimum size of a rendered particle.
 (default: `2`)

* **vk_particle_max_size**: The maximum size of a rendered particle.
  (default: `40`)

* **vk_picmip**: Shrink factor for the textures. (default: `0`)

* **vk_pixel_size**: Pixel size when rendering the world, used to simulate
  lower screen resolutions. The value represents the length, in pixels, of the
  side of each pixel block. For example, with size 2 pixels are 2x2 squares,
  and at 1600x1200 the image is effectively an upscaled 800x600 image.
  (default: `1`)

* **vk_dynamic**: Use dynamic lighting. (default: `1`)

* **vk_showtris**: Display mesh triangles. (default: `0`)

* **r_lightmap**: Display lightmaps. (default: `0`)

* **vk_postprocess**: Toggle additional color/gamma correction.
  (default: `1`)

* **vk_mip_nearfilter**: Use nearest-neighbor filtering for mipmaps.
  (default: `0`)

* **vk_texturemode**: Change current texture filtering mode:
  * `VK_NEAREST` - nearest-neighbor interpolation, no mipmaps
  * `VK_LINEAR` - linear interpolation, no mipmaps
  * `VK_MIPMAP_NEAREST` - nearest-neighbor interpolation with mipmaps
  * `VK_MIPMAP_LINEAR` - linear interpolation with mipmaps (default)

* **vk_lmaptexturemode**: Same as `vk_texturemode` but applied to
  lightmap textures.

* **vk_underwater**: Warp the scene if underwater. Set to `0` to disable
  the effect. Defaults to `1`.

## Console Variables (macOS)

* **vk_molten_metalbuffers**: enable/disable Metal buffers to bind textures
  more efficiently (>= Big Sur). (default: `0`)

* **vk_molten_fastmath**: enable/disable float point op optimisations.
  (default: `0`)

### Custom model format support

Render unofficially supports  mdl/Quake 1, dkm/Daikatana and fm/Heretic2,
are provided without any warranty of support. The simplest way to check
is renaming the mdl/dkm/fm format file to md2 and place instead the original
tris.md2 file. FM is rendered with all meshes without support of
filtering/selecting the exact part of the model.

### ReRelease Support

Use [Yamagi Quake II ReRelease](https://github.com/yquake2/yquake2remaster/releases)
version with 2023 Quake ReRelease version.
