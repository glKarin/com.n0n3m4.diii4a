# General

This directory contains all third-party dependencies of TheDarkMod, both the game and the updater.
There are some rare exceptions when library is embedded and tightly integrated directly into the engine, but this way is generally avoided.

The dependecies are fetched and built using [conan][1] package manager.
Note that the code directly uses the artefacts (headers and libs) without interacting with conan in any way.
Conan only helps us to prepare these artefacts.

## Directory structure

Here are the contents of this directory:

 * `conanfile.py` --- the main file, listing all libraries, options, and import rules
 * `custom/*` --- custom conan recipes for some libraries (in case stock recipe is not available or does not suit)
 * `profiles/*` --- conan settings for building artefacts for supported platforms
 * `artefacts/*` --- the resulting files (headers and libs) used by TDM projects

All third-party binaries **must** be inside `artefacts` directory!
Developers using DVCS for local changes should add the whole `artefacts` directory to their ignore filter.


# Instructions

## How to build

By following these steps you should be able to rebuild all the third-party dependencies available.
You might want to delete the whole `artefacts` directory beforehand, to ensure a full rebuild.

### Install

First of all, make sure that you have latest Python 3 installed, e.g. from [download page][2].

Next, install conan if not yet installed.
Detailed instructions are provided [here][3], the easiest way is via pip:

    pip install conan

You might want to read some "Getting Started" conan docs to get brief understanding of what it is and how it works.

### Prepare workspace

Conan builds packages inside a *local cache*, which by default is located somewhere in `%USERPROFILE%/.conan` and is in fact system-global.
In order to make the cache truly local, you can set environment variable in the shell you are going to use:

    # set the path to a custom directory
    set CONAN_USER_HOME=C:\tdm_conan

Note that if you do so, you should ensure that this environment variable takes effect for all the conan commands you run later.
Detailed explanation is available [here][4].

You can omit this step, then system-wide cache will be used for building libraries.

### Custom recipes

Recipes for most libraries are taken directly from central conan repository (i.e. "conan-center").
Some libraries need special handling for TDM, so we manage recipes for them ourselves. They are located in `custom` directory.

Before actually building the libs, it is necessary to copy these recipes into conan's "local cache".
The easiest way to do that is to run the small helper script:

    python 1_export_custom.py

It performs `conan export custom/{libraryname} {libraryname}/{version}@thedarkmod/local` for every library, which adds the corresponding recipe to local cache.

### Build libraries

Now you are ready to download and build all the libraries.
You might want to delete the `artefacts` directory to surely get fresh libs.

If you want to build all artefacts and commit them to svn, it is recommended to run the helper script:

    python 2_build_all.py

The script runs `conan install`, but with some specific parameters, including specific profiles.
Then build all libraries on Linux using GCC.
If you have GCC of version different from 5, you need to edit `profiles/linuxXX` and change `compiler.version`.

Given that produced artefacts may differ slightly on different machines (no two .lib-s are exactly equal),
please commit new artefacts only for the libraries which you have actually changed, don't commit changes in libraries which you did not touch.

### Unsupported platform

The SVN repo contains artefacts only for the main supported platforms, i.e. MSVC+Windows and GCC+Linux on x86/x64.
If you want to build TDM for custom CPU architecture, custom OS, or with some custom settings, then you have to do custom steps.

First of all, find codename of your CPU architecture in `.conan/settings.yml`.
If it is not present there, then decide on a codename and add it to the following arrays in yml: `arch_target`, `arch_build`, `arch`.
For instance, for Elbrus-8C I added `e2kv4` as architecture codename.
Same applies to your OS and compiler: if conan does not know it out of the box, then you have to add it to `settings.yml` somehow.

Before build, you need to import custom recipes via `1_export_custom.py`, as described above.
In order to build dependencies, run `conan install` with additional arguments:

    conan install . --build -s arch=e2kv4 -o platform_name=myelbrus

The additional **s**etting `arch=e2kv4` says: build for e2kv4 architecture (not necessary if conan supports your architecture out-of-the-box).
The additional **o**ption `platform_name=myelbrus` says: put resulting binaries into `artefacts/{packagename}/lib/myelbrus` subdirectories.

When building TDM, you have to pass the chosen value of platform_name as `THIRDPARTY_PLATFORM_OVERRIDE` option to CMake generate step:

    cmake -B build/linux64 -DCMAKE_BUILD_TYPE=Release -DTHIRDPARTY_PLATFORM_OVERRIDE=myelbrus

Without this argument, CMake won't know where to get proper artefacts, and will most likely fail with linking error.


## How to add new library

By default conan uses only one very official and stable remote: "conan-center".
Now that you have remotes, you can search for recipes of a library like this:

    conan search {libraryname} -r all

It might be useful to add asterisk at the beginning and at the ending of the library name.

If you manage to find the library, then choose one of the printed references and add it to the list in `packages.yaml`.
After that you can do usual build and your library will get downloaded and built.

If you fail to find ready-to-use conan recipe, then you have to add a custom one.
Note that going the custom recipe route most likely needs good conan skills from you!
You might want to search it on github with codenames `conan` and library name.
If you find someone else's recipe, you can add it to `custom` and it will probably work.
Also you can find some recipes in "bincrafters" remote, but you have to add this remote first.

Sometimes there is a recipe for a library, but it needs fine-tuning in order to be used in TDM.
In such case you should download the recipe, add it to `custom` too, and edit there.
Make sure to clearly mark all your changes to the recipe, or/and save its original version nearby.

Otherwise you have to create recipe yourself, or ask someone else to do it on forums.
As the last resort option (i.e. no time to waste or nobody agrees to write it), you can build the library manually and add artefacts to `artefacts` directory.
Make sure to adhere to existing directory structure and naming conventions.


## How to update library

Look into `packages.yaml` and find the library reference there.
Now do `conan search` for a newer version of the library.

If you find one, change the reference in `conanfile.py` and rebuild.
Otherwise, you are out of luck.

If you want to update a library with custom recipe, it gets more complicated.
In many cases you can apply an existing recipe to the newer version: just add source code URL and sha256 checksum for the new version in `conandata.yml`.
If this is custom-modified recipe, then you can fetch a newer recipe from central repository and try to reapply the same TDM-specified hacks to it.



[1]: https://conan.io/
[2]: https://www.python.org/downloads/
[3]: https://docs.conan.io/en/latest/installation.html
[4]: https://docs.conan.io/en/latest/mastering/custom_cache.html
