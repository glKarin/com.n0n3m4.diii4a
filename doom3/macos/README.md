# Experimental ETQW port for Apple Silicon

This port runs idTech4A++'s reconstructed Enemy Territory: QUAKE Wars engine
on Apple Silicon using native Metal. It preserves the existing material system
and renders the world, virtual terrain, effects and UI through Metal command
encoders. The native backend creates no OpenGL or EGL context.

**Status:** development build tested on an Apple M4 Max with macOS 27.0
(`26A428`). Valley and Island were exercised with local bots; Refinery was
played on a retail 1.5 pure server. Full map/menu coverage, Intel Macs, other
Apple GPUs and older macOS releases have not been validated.
Experimental retail 1.5 multiplayer interoperability is available
with the separate launcher below. This is not an update to the commercial Aspyr executable.

## Build and run

Install Xcode command line tools and the build dependencies:

```sh
brew install cmake ninja pkg-config python sdl2 freetype shaderc spirv-cross
doom3/macos/build.sh
doom3/macos/launch.sh /path/to/your/ETQW
```

The game directory must contain your own updated ETQW 1.5 `base/` directory,
including `pak008.pk4` and the `megatextures/` files. No retail game data,
license keys or generated texture packs are supplied here. The publisher's
[ETQW page](https://www.splashdamage.com/games/enemy-territory-quake-wars/)
lists the official 1.5 updates; an update alone is not the full game.

The launcher checks its inputs, copies the existing upstream compatibility pack
into the writable profile and creates a small override from the repository's
imposter shader. It does not modify the supplied game directory. With no engine
arguments, it opens a loopback-only Valley match with bots. Choose a team and
deploy using Space. The optional `azerty.cfg` supplies ZQSD bindings; copy it
to the profile's `base/` directory and run `exec azerty.cfg` in the console.

`ETQW_BUILD_DIR`, `ETQW_BUILD_JOBS` and `ETQW_PROFILE` override the build directory,
parallel build jobs and profile directory. The default profile is
`~/Library/Application Support/idtech4amm/etqw-metal`.
`build.sh` targets the running macOS version unless `MACOSX_DEPLOYMENT_TARGET`
is supplied. The source needs a macOS 14 or newer SDK for the RT API; the
dependencies must also support the deployment target. The libraries used for
local validation target macOS 26/27, so these results do not establish support
for older releases. OpenAL Soft is built from the existing vendored source
with CoreAudio support. The build and launch scripts do not install dependencies.

Default rendering follows the Retina drawable size in a 1280x720-point window.
Internal 4K rendering, larger font atlases and ray tracing are optional:

```sh
doom3/macos/launch.sh /path/to/your/ETQW \
  +set r_renderWidth 3840 +set r_renderHeight 2160 \
  +set harm_gui_fontRasterScale 4 +set image_anisotropy 16 \
  +set r_metalRT 1 \
  +set net_ip 127.0.0.1 +set net_LANServer 1 +set net_allowCheats 0 \
  +set si_rules sdGameRulesObjective +set bot_minClients 8 +spawnServer valley
```

The window size is measured in points; render targets are measured in pixels.
4K rendering is downsampled when the drawable is smaller. Fonts and HUD layout
keep their logical size. Set `r_renderWidth` and `r_renderHeight` to zero to
follow drawable pixels at initialization; resolution changes require `vid_restart`.

## Retail 1.5 multiplayer

Use original, updated game archives and a separate profile for retail servers:

```sh
doom3/macos/join-retail.sh /path/to/your/ETQW HOST:PORT
```

This enables protocol 10.21. The ARM64 engine and game module exchange retail
Huffman reliable messages, compressed snapshots and 33 ms input commands.
Declaration checksums follow retail loading order and text terminator rules;
terrain files use the retail sampled checksum. PK4 content checks remain active.
The verified, installed Mac 1.5 `game001.pk4` is used as the SDK compatibility
baseline; the running module is the rebuilt ARM64 module, not the original binary.

Native Metal was connected to a stock Windows 1.5 dedicated server on loopback
and a public Linux 1.5 server with pure mode enabled. Outskirts and Area22 were
used for network checks. Refinery was verified in first person with team/class
selection, movement, firing, player names, health and changing ammunition bars.
The declaration checksum matched an independently observed original-client handshake. See
[`validation/retail-network.json`](validation/retail-network.json) for the exact scope.

For this profile, the engine embeds the repository's translated render programs
and their includes/templates at build time. Original declaration text, binary
tokens and checksums stay intact. This restores the post-processing composite,
foliage alpha and command map without installing a shader override archive on a
pure server. Two incomplete upstream subsurface programs retain the existing
fallback. The build requires Python 3.9 or newer to generate the embedded table.

Authentication-ticket servers, PunkBuster, mods and voice audio are not supported
by this path. A second voice channel is acknowledged, but audio is not decoded.
Server browsing still uses the reconstructed protocol; connect by address through
the retail launcher. The existing local launcher retains protocol 14.42.
Keep compatibility and HD texture override archives in the local profile: pure
retail servers require their original data. Retina, optional 4K rendering and
larger font atlases do not require modified game archives. Keep the game data
resident on disk; an evicted iCloud `.mega` file cannot be streamed reliably.
The retail launcher keeps the original compressed textures resident on the GPU;
it does not apply the optional local 4× texture pack.

## Renderer and limits

- The internal `qgl` command interface is retained, backed by Metal textures,
  buffers, pipeline states, render passes and readback. GLSL materials are
  compiled through shaderc and SPIRV-Cross to MSL, cached under
  `<fs_savepath>/metal-shaders/`. Unsupported commands are counted and reported
  rather than silently ignored.
- Three upload buffers rotate across frames in flight. Interleaved client
  vertex arrays are staged once per draw; pipelines, depth/stencil states and
  samplers are cached. Only shader resources that are used are bound.
- DXT1/3/5 and ATI2 normal maps remain compressed as BC1/2/3/5 textures.
  BC5 normal Z is reconstructed in the material shader. This avoids keeping
  every normal map expanded to RGBA on the GPU.
- Native rasterization and RT share one command queue and frame command buffer.
  The RT pass adds sun-shadow attenuation for opaque static world geometry,
  using four shadow rays per mask pixel and depth-aware upsampling. It requires
  hardware ray tracing (Apple M3 or newer) and is disabled when initialization
  fails. It does not provide animated-character/vehicle RT shadows, RT
  reflections or RT global illumination. Baked lighting remains in place.
- MSAA and the engine's multithreaded rendering path have not been implemented
  for this backend. The supplied launcher sets both to zero. Other unexercised
  rendering commands may still be unsupported.
- Distant trees use their 3D meshes (`harm_r_skipImposter 1`) to avoid the
  reconstructed billboard shading defect. This trades geometry cost for quality.
  Terrain luminance decoding, blend-state depth preservation and command-map
  clipping are corrected separately from the backend.

`metalInfo` reports whole-command-buffer GPU time, allocations and unsupported
calls. `metalRTInfo` reports RT initialization and geometry. `r_metalRT 0` toggles
RT off; `r_metalRTStrength` and `r_metalRTScale` control attenuation and mask size
(scale changes require `vid_restart`). The default RT scale is 0.25.

The historical renderer remains available with `+set r_metalBackend 0`.
Comparisons used ANGLE/Metal with `r_useDXT 0`; those external ANGLE libraries
are not supplied or required by native rendering. Other engine targets and
non-macOS platform builds still need regression coverage.

## Tests and recorded measurements

```sh
doom3/macos/tests/verify.sh
python3 doom3/macos/tests/verify_mega_codec.py /path/to/base/megatextures/valley_lit.mega
```

The tests compile the actual modified implementations. The Metal probe uses the
validation layer and checks indexed geometry, culling, depth occlusion, alpha
discard, interleaved attributes, matrices, texture binding, BC5, buffer lifetime,
masked/scissored clears and readback. CPU probes cover atomics, six blend forms
and rotated GUI clipping with UV interpolation. The terrain probe reads twenty
tiles from the tester's own Valley file; it ships no extracted tile data.

Local 3840x2160 measurements on the M4 Max, with locally generated 4x DDS
overrides and a 960x540 RT mask, are recorded in
[`validation/performance.json`](validation/performance.json).

| Fixed Valley view | RT off | RT on |
|---|---:|---:|
| ANGLE/Metal | 74.5 fps | 69.4 fps |
| Native Metal | 82.6 fps | 76.2 fps |

These are medians of three one-second synchronized rendering samples after
two warmups, with Metal validation disabled. They exclude normal simulation
and input processing; they are not gameplay frame-rate guarantees. Scene
effects can differ between runs. A later packaged-build check measured
76.5 fps with RT. The cleanup branch has separate build/probe validation;
the numbers above describe the preceding local development build.

To reproduce the camera once the world is displayed:

```text
benchmarkView 0 -6000 6000 90 25 0 valley_comparison
```

Remaining validation: all twelve maps and menus, long sessions, `vid_restart`,
other Apple GPUs/macOS releases, and Windows/Linux/Android regression builds.
Missing-resource warnings from the reconstructed engine remain.

## Optional local texture reconstruction

The texture tools operate on files supplied by the player. Install Pillow 12+
and NumPy in a virtual environment, then use separate input and output folders:

```sh
python3 doom3/macos/tools/upscale_textures.py --base /path/to/base --output /path/to/hd4x
python3 doom3/macos/tools/verify_texture_packs.py --base /path/to/base --packs /path/to/hd4x
```

Copy the generated `zzz_mac_hd4x_*.pk4` files into the profile's `base/` directory
to enable them; remove those overrides to disable them. The process uses Lanczos
resampling and light sharpening, not generated detail. Alpha is retained,
BC5 normals are renormalized, and original mip levels are preserved byte for
byte. The `.mega` terrain is not enlarged. The locally tested full pack contains
3352 DDS files and takes about 7.4 GB. Generated overrides are not part of this PR.

Existing upstream copyright and license notices are retained. New port code
uses GPL-3.0-or-later; bundled third-party sources keep their own notices.
